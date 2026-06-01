/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #27 $
******************************************************************************/

#ifndef HCI_ATT_ARB_PRIM_H__
#define HCI_ATT_ARB_PRIM_H__



#include "csr_synergy.h"
#include "csr_sched.h"
#include "csr_bt_profiles.h"
#include "csr_types.h"
#include "csr_bt_util.h"
#include "csr_bt_addr.h"
#if defined(HYDRA) || defined(CAA)
#include "hci.h"
#else
#include "hci_prim.h"
#endif
#include "l2cap_prim.h"

#include "dm_prim.h"
#include "csr_bt_result.h"
#include "csr_mblk.h"
#include "csr_prim_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HCI_ATT_SENDER_MICRO_STACK = 1,
    HCI_ATT_SENDER_PRIMARY_STACK
}AttMsgSender;

typedef enum
{
    ATT_CLIENT,
    ATT_SERVER,
}ATT_ROLE;

/*
#define HCI_ATT_SENDER_MICRO_STACK                      (1)
#define HCI_ATT_SENDER_PRIMARY_STACK                    (2)
*/

/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/
#define HCI_ATT_ARB_PRIM_DOWNSTREAM_LOWEST              (0x0000)
#define HCI_ATT_ARB_HOUSE_CLEANING_REQ                  ((CsrPrim) (0x0000 + HCI_ATT_ARB_PRIM_DOWNSTREAM_LOWEST))
#define HCI_ATT_ARB_ATT_MSG                             ((CsrPrim) (0x0001 + HCI_ATT_ARB_PRIM_DOWNSTREAM_LOWEST))

#define HCI_ATT_ARB_PRIM_DOWNSTREAM_HIGHEST             ((CsrPrim) (HCI_ATT_ARB_ATT_MSG))
/* **** */

/* **** */
#define HCI_ATT_ARB_PRIM_UPSTREAM_LOWEST                (0x0100 + CSR_PRIM_UPSTREAM)
#define HCI_ATT_ARB_ATT_MSG_CFM                         ((CsrPrim) (0x0001 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define HCI_ATT_ARB_PRIM_UPSTREAM_HIGHEST               ((CsrPrim) (0x0001 + MS_SEND_PRIM_UPSTREAM_LOWEST))

/*******************************************************************************
 * End primitive definitions
 *******************************************************************************/


/*************************************************************************************
 Primitive typedefs
************************************************************************************/
typedef CsrPrim HciAttArbPrim;

typedef struct
{
    HciAttArbPrim                   type;
    AttMsgSender                    attSender;
    TXQE_T                          *txqe;
    L2CAP_CHCB_T                    *chcb;
    uint8_t                         priority;
    bool_t                          fromNhcp;
} HciAttArbAttMsgReq;

typedef struct
{
    HciAttArbPrim                 type;
    AttMsgSender                  reqSender;
    L2CAP_CHCB_T                  *chcb;
} HciAttArbHouseCleaningReq;

/*------------------------------------------------------------------------
 *
 *      UNION OF PRIMITIVES
 *
 *-----------------------------------------------------------------------*/
typedef union
{
    HciAttArbPrim                 type;
    HciAttArbAttMsgReq            hciArbAttReq;
    HciAttArbHouseCleaningReq     hciAttArbHouseCleaningReq;
}HCI_ATT_ARB_PRIM_T;

#ifdef __cplusplus
}
#endif

#endif /* ifndef HCI_ATT_ARB_PRIM_H__ */
