/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #27 $
******************************************************************************/

#ifndef HCI_CMD_ARB_PRIM_H__
#define HCI_CMD_ARB_PRIM_H__



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

#define HCI_CMD_SENDER_MICRO_STACK                      (1)
#define HCI_CMD_SENDER_PRIMARY_STACK                    (2)
#define HCI_CMD_SPL_ARBITRATION_LOGIC                   (3)


/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/
#define HCI_CMD_ARB_PRIM_DOWNSTREAM_LOWEST              (0x0000)
#define HCI_CMD_ARB_HOUSE_CLEANING_REQ                  ((CsrPrim) (0x0000 + HCI_CMD_ARB_PRIM_DOWNSTREAM_LOWEST))
#define HCI_CMD_ARB_CMD_REQ                             ((CsrPrim) (0x0001 + HCI_CMD_ARB_PRIM_DOWNSTREAM_LOWEST))
#define HCI_CMD_ARB_VS_CMD_REQ                          ((CsrPrim) (0x0002 + HCI_CMD_ARB_PRIM_DOWNSTREAM_LOWEST))
#define HCI_PERI_CMD_ARB_CMD_REQ                        ((CsrPrim) (0x0003 + HCI_CMD_ARB_PRIM_DOWNSTREAM_LOWEST))

#define HCI_CMD_ARB_PRIM_DOWNSTREAM_HIGHEST             ((CsrPrim) (HCI_PERI_CMD_ARB_CMD_REQ))
/* **** */

/* **** */
#define HCI_CMD_ARB_PRIM_UPSTREAM_LOWEST                (0x0100 + CSR_PRIM_UPSTREAM)
#define HCI_CMD_ARB_CMD_CFM                             ((CsrPrim) (0x0001 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define HCI_CMD_ARB_PRIM_UPSTREAM_HIGHEST               ((CsrPrim) (0x0001 + MS_SEND_PRIM_UPSTREAM_LOWEST))

/*******************************************************************************
 * End primitive definitions
 *******************************************************************************/


/*************************************************************************************
 Primitive typedefs
************************************************************************************/
typedef CsrPrim HciCmdArbPrim;

typedef struct
{
    HciCmdArbPrim                 type;
    uint8                         cmdSender;
    CsrUint8                      *payload;
    CsrUint16                     len;
} HciArbCmdReq;

typedef struct
{
    HciCmdArbPrim                 type;
    uint8                         reqSender;
} HciArbHouseCleaningReq;

typedef struct
{
    HciCmdArbPrim                 type;
    uint8                         cmdSender;
    void                          *data;
} HciArbVsCmdReq;

typedef struct
{
    HciCmdArbPrim                 type;
    uint8                         cmdSender;
    CsrUint8                      *payload;
    CsrUint16                     len;
} HciPeriArbCmdReq;


/*------------------------------------------------------------------------
 *
 *      UNION OF PRIMITIVES
 *
 *-----------------------------------------------------------------------*/
typedef union
{
    HciCmdArbPrim                 type;
    HciArbCmdReq                  hciArbCmdReq;
    HciArbHouseCleaningReq        hciArbHouseCleaningReq;
    HciPeriArbCmdReq              hciPeriArbCmdReq;
}HCI_CMD_ARB_PRIM_T;

#ifdef __cplusplus
}
#endif

#endif /* ifndef HCI_CMD_ARB_PRIM_H__ */
