/*******************************************************************************
Copyright (C) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifndef _DM_ACL_H_
#define _DM_ACL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qbl_adapter_hci.h"                    /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_panic.h"                  /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_pmalloc.h"                /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_types.h"                  /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_logging.h"                /* QBLUESTACK_REFACTOR */
#include <string.h>
#include <stdio.h>
#include INC_DIR(mblk,mblk.h)
#include INC_DIR(common,common.h)
#include INC_DIR(common,error.h)
#include INC_DIR(tbdaddr,tpbdaddr.h)
#include INC_DIR(bluestack,bluetooth.h)         /* QBLUESTACK_REFACTOR */
#include "qbl_patch.h"
#include "hci.h"
#include "dm_acl_credits.h"
#include "l2cap_private.h"
#ifdef INCLUDE_HCI_ARBITER
#include "hci_arbiter_private.h"
#endif


/* Given pointer to ACL and client name, returns pointer to client data */
#define DM_ACL_CLIENT_GET_DATA(p_acl, client) (&(p_acl)->client)

/* Given pointer to client data and client name, returns pointer to ACL */
#define DM_ACL_CLIENT_GET_ACL(client_data, client) \
    ((DM_ACL_T*)(((uint8_t*)(client_data)) - offsetof(DM_ACL_T, client)))

#define DM_ACL_CLIENT_GET_HANDLE(client_data, client) \
        (DM_ACL_CLIENT_GET_ACL(client_data, client)->handle)

/* Pointer to first ACL record */
#define DM_ACL_FIRST() p_acl_list


typedef struct DM_ACL_T_tag
{
    struct DM_ACL_T_tag *p_next;
    hci_connection_handle_t handle;     /* HCI connection handle */
    TYPED_BD_ADDR_T addrt;              /* Peer BD address */
    PHYSICAL_TRANSPORT_T transport;
    L2CAP_CHCB_T dm_acl_client_l2cap;   /* L2CAP */
#ifdef INCLUDE_HCI_ARBITER
    void * arbiter;                     /* HCI Arbiter */
#endif
    uint16_t pending_credits;           /* Pending credits for sent data on dying ACL. */
} DM_ACL_T;

DM_ACL_T *dm_acl_find_by_handle(hci_connection_handle_t handle);
DM_ACL_T *dm_acl_find_by_tbdaddr_acl_type(const TYPED_BD_ADDR_T *const addrt,
                                          PHYSICAL_TRANSPORT_T phy_trnsprt);
bool_t dm_acl_is_ble(const DM_ACL_T *p_acl);

extern DM_ACL_T *p_acl_list;

#ifdef SUPPORT_SEPARATE_LE_BUFFERS
DM_HCI_FLOW_CONTROL_T * dm_amp_get_fc_type(DM_ACL_T *p_acl);
#else
#define dm_amp_get_fc_type(p_acl)  (&dm_credit_cb.fc)
#endif

DM_ACL_T *dm_acl_new(const TYPED_BD_ADDR_T *const addrt,
                     hci_connection_handle_t handle,
                     PHYSICAL_TRANSPORT_T transport);

bool dm_acl_handle_is_ble(uint16 hciHandle);
bool_t dm_acl_is_connected(const DM_ACL_T *const p_acl);

void dm_acl_free(DM_ACL_T *acl);

void *dm_acl_arb_find_by_handle(hci_connection_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _DM_ACL_H_ */
