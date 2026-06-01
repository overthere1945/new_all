/******************************************************************************
 Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef SYN_IPC_TX_H__
#define SYN_IPC_TX_H__

#include "csr_sched.h"
#include "csr_types.h"
#include "csr_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    TXMSG *hciQ;
    TXMSG *customQ;
    CsrSchedBgint bgint;
    /* bg interrupt for custom path */
    CsrSchedBgint bgintcustom;
    CsrBool drainHci;
} SynIpcTxInstance;

CsrBool SynIpcTxSendMsg(SynIpcTxInstance *tx, TXMSG *msg);
void SynIpcTxInit(SynIpcTxInstance *tx);
void SynIpcTxDeinit(SynIpcTxInstance *tx);

#ifdef __cplusplus
}
#endif

#endif
