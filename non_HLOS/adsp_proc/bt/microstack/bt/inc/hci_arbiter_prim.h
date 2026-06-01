/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#ifndef __HCI_ARBITER_PRIM_H__
#define __HCI_ARBITER_PRIM_H__

#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif


/* This file captures Hci Arbiter APIs for Micro Stack */

/* Basic types */
typedef uint16          HciArbiterPrim;


typedef uint16 HciArbiterResultCode;
#define HCI_ARBITER_RESULT_SUCCESS                    ((HciArbiterResultCode)0x0000)
#define HCI_ARBITER_RESULT_BTOFF_FAILURE              ((HciArbiterResultCode)0x0001)


typedef uint8_t HciArbiterBtssErrorCode;
#define HCI_ARBITER_BTSS_PERI_ERROR                   ((HciArbiterBtssErrorCode)0x01)
#define HCI_ARBITER_BTSS_BT_ERROR                     ((HciArbiterBtssErrorCode)0x02)
#define HCI_ARBITER_BTSS_SMC_ERROR                    ((HciArbiterBtssErrorCode)0x03)

/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/
/* Downstream */
#define HCI_ARBITER_BT_OFF_IND                    ((HciArbiterPrim)0x0001)
#define HCI_ARBITER_BTSS_CRASH_IND                ((HciArbiterPrim)0x0002)


typedef struct
{
    HciArbiterPrim            type;               /* Message type, always HCI_ARBITER_BT_OFF_IND */
    HciArbiterResultCode      status;
} HciArbiterBtOffInd;

typedef struct
{
    HciArbiterPrim            type;               /* Message type, always HCI_ARBITER_BTSS_CRASH_IND */
    HciArbiterBtssErrorCode   reason;
} HciArbiterBtssCrashInd;


#ifdef __cplusplus
}
#endif

#endif /* __HCI_ARBITER_PRIM_H__ */

