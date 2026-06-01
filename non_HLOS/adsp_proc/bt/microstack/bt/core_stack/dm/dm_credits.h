/*******************************************************************************
Copyright (C) 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifndef _DM_CREDITS_H_
#define _DM_CREDITS_H_

#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include "dm_acl.h"
#include INC_DIR(l2cap,l2cap_types.h)

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Generic flow control/data credit structure for BR/EDR buffers*/
typedef struct
{
    uint16_t max_acl_data_packet_length;
    uint16_t data_block_length;
    uint16_t total_num_data_blocks;
    uint16_t used_data_blocks;
} DM_HCI_FLOW_CONTROL_T;

/* enum for the BR/EDR or, LE type device */
typedef enum
{
    DM_AMP_ACL_TYPE_BR_EDR = 0,
    DM_AMP_ACL_TYPE_LE
}DM_AMP_ACL_TYPE_T;

typedef struct
{
    DM_HCI_FLOW_CONTROL_T fc;
    DM_HCI_FLOW_CONTROL_T le_fc;
} DM_CREDIT_CB_T;

extern DM_CREDIT_CB_T dm_credit_cb;

#ifdef SUPPORT_SEPARATE_LE_BUFFERS
#define DM_GET_FC_TYPE(type) (((type) == DM_AMP_ACL_TYPE_BR_EDR) \
                               ? (&dm_credit_cb.fc) : (&dm_credit_cb.le_fc))
#else
#define DM_GET_FC_TYPE(type) (&dm_credit_cb.fc)
#endif

/* Data handling */
uint16_t dm_amp_getdatacredits(TXQUEUE_T *queue, L2CAP_CH_DATA_SIZES_T *sizes,
                               DM_AMP_ACL_TYPE_T type);
void dm_amp_reserve_credit(TXQUEUE_T *queue, DM_AMP_ACL_TYPE_T type);

#ifdef SUPPORT_SEPARATE_LE_BUFFERS
DM_HCI_FLOW_CONTROL_T * dm_amp_get_fc_type( DM_ACL_T *p_acl);
#else
#define dm_amp_get_fc_type(p_acl)  (&dm_credit_cb.fc)
#endif

#ifdef __cplusplus
}
#endif


#endif /* _DM_CREDITS_H_ */
