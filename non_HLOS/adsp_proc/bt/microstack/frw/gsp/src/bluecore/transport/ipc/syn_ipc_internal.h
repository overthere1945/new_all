/******************************************************************************
 Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef SYN_IPC_INTERNAL_H__
#define SYN_IPC_INTERNAL_H__

#include "csr_panic.h"
#include "csr_types.h"
#include "csr_macro.h"
#include "csr_log_text_2.h"
#include "syn_ipc.h"
#include "syn_ipc_rx.h"
#include "syn_ipc_tx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(SynIpcLto);

/* Sub Origins */
#define SYN_IPC_LTSO_GEN                         1
#define SYN_IPC_LTSO_TX                          2
#define SYN_IPC_LTSO_RX                          3

#define SYN_IPC_STATE_UNINITIALIZED              0
#define SYN_IPC_STATE_INITIALIZED                1
#define SYN_IPC_STATE_STARTED                    2

typedef struct
{
    CsrUint8 state;
    void *ipcDrvHandle;
    void *ipcDrvCustomHandle;
    SynIpcTxInstance tx;
    SynIpcRxInstance rx;
    void *transport;
} SynIpcInstance;

/* Extracts IPC instance from TX instance */
#define SYN_IPC_INST_FROM_TX(_tx)                                                \
    ((SynIpcInstance *)(((CsrUint8 *) (_tx)) - CsrOffsetOf(SynIpcInstance, tx)))

/* Extracts IPC instance from RX instance */
#define SYN_IPC_INST_FROM_RX(_rx)                                                \
    ((SynIpcInstance *)(((CsrUint8 *) (_rx)) - CsrOffsetOf(SynIpcInstance, rx)))

/* Check for the HCI messages */
#define SYN_IPC_IS_HCI_PACKET(_msg)                                              \
    ((_msg)->chan == TRANSPORT_CHANNEL_HCI ||                                   \
     (_msg)->chan == TRANSPORT_CHANNEL_ACL)

typedef struct
{
    SynIpcExtensionCallback cb;
    SynIpcHciPktTypeReq pktTypeReq;
} synIpcExtension;

#ifdef __cplusplus
}
#endif

#endif
