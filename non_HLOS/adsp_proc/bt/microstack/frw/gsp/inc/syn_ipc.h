/******************************************************************************
Copyright (c) 2020-2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
******************************************************************************/
#ifndef SYN_IPC_H__
#define SYN_IPC_H__
#include "syn_ipc_config.h"

/* Datatype for callback messages */
typedef CsrUint8 SynIpcMsgType;

/* callback messages */
/* Message to indicate a HCI packet. SynIpcMsgHci would 
   contain the HCI packet information */
#define SYN_IPC_MSG_HCI        (SynIpcMsgType) 0x00
/* Message to indicate a Custom packet from BT Subsustem. 
   SynIpcMsgCustom would contain the packet information */
#define SYN_IPC_MSG_CUSTOM     (SynIpcMsgType) 0x01
/* Message to communicate few important info from syn 
   ipc driver. SynIpcMsgDriver */
#define SYN_IPC_MSG_DRIVER     (SynIpcMsgType) 0x02

typedef CsrUint8 SynIpcHciPktType;

/* HCI Package Headers */
#define SYN_IPC_HCI_CMD         (SynIpcHciPktType) 0x01
#define SYN_IPC_HCI_ACL         (SynIpcHciPktType) 0x02
#define SYN_IPC_HCI_EVT         (SynIpcHciPktType) 0x04
#define SYN_IPC_HCI_PKT_CUSTOM  (SynIpcHciPktType) 0xFE

/* Message structure for SYN_IPC_MSG_HCI */
typedef struct
{
    SynIpcHciPktType    pktType;
    CsrUint32           dataLen;
    CsrUint8            *data;
} SynIpcHciPdu;

typedef SynIpcHciPdu SynIpcMsgHci;

typedef CsrUint8 SynIpcMsgId;

/* SYN_IPC Messages used in tx path */
#define SYN_IPC_MSG_BT_HCI                              (SynIpcMsgId) 0x00
#define SYN_IPC_MSG_BT_OFFLOAD_SWITCH_TXP_REQ           (SynIpcMsgId) 0x01
#define SYN_IPC_MSG_BT_OFFLOAD_GET_CONTEXT_RSP          (SynIpcMsgId) 0x04
#define SYN_IPC_MSG_BT_OFFLOAD_GET_BUFFERED_PDU_RSP     (SynIpcMsgId) 0x06

/* IPC Custom Messages received from BT Subsystem */
#define SYN_IPC_MSG_BT_OFFLOAD_SWITCH_TXP_CFM           (SynIpcMsgId) 0x02
#define SYN_IPC_MSG_BT_OFFLOAD_GET_CONTEXT_IND          (SynIpcMsgId) 0x03
#define SYN_IPC_MSG_BT_OFFLOAD_GET_BUFFERED_PDU_IND     (SynIpcMsgId) 0x05

/* Message structure for SYN_IPC_MSG_CUSTOM */
typedef struct
{
    SynIpcMsgId     msgId;
    CsrUint32       dataLen;
    CsrUint8        *data;
} SynIpcMsgCustom;

typedef CsrUint16 SynIpcMsgDriverId;
#define SYN_IPC_MSG_DRIVER_RECEIVE_MEMORY_FULL          (SynIpcMsgDriverId)0

typedef struct
{
    SynIpcMsgDriverId id;
} SynIpcMsgDriver;

typedef union
{
    SynIpcMsgHci        hci;
    SynIpcMsgCustom     custom;
    SynIpcMsgDriver     driver;
} SynIpcMsg;

typedef CsrUint8 SynIpcResult;
#define SYN_IPC_RESULT_SUCCESS  (SynIpcResult)0
#define SYN_IPC_RESULT_CONSUMED (SynIpcResult)1
#define SYN_IPC_RESULT_IGNORED  (SynIpcResult)2
#define SYN_IPC_RESULT_FTM_MODE (SynIpcResult)3

/* msg points to SynIpcMsg */
typedef SynIpcResult (*SynIpcExtensionCallback)(SynIpcMsgType  type,
                                                void           *msg);

/* SynIpcTxExtensionCallback

   When HCI Command / ACL packet originating from host stack is 
   received by IPC driver, this callback is invoked. 
   Message type in callback will be set to SYN_IPC_MSG_HCI.

   SYN_IPC_MSG_HCI: On receiving this message application can choose to 
   only peek & let IPC driver forward the message to BTSS in which case 
   it shall return SYN_IPC_RESULT_IGNORED. In certain cases if application 
   could completly consume the message then it shall return 
   SYN_IPC_RESULT_CONSUMED, in which case the SynIpcMsgHci->data need not 
   be freed.
*/
typedef SynIpcExtensionCallback SynIpcTxExtensionCallback;

/* SynIpcRxExtensionCallback

   When HCI Event / ACL packet originating from bt subsystem is 
   received by IPC driver, this callback is invoked. 
   Message type in callback will be set to SYN_IPC_MSG_HCI.

   SYN_IPC_MSG_HCI: On receiving this message application can choose to 
   only peek & let IPC driver forward the message to Host stack in which case 
   it shall return SYN_IPC_RESULT_IGNORED. In certain cases if application 
   could completly consume the message then it shall return 
   SYN_IPC_RESULT_CONSUMED.
*/
typedef SynIpcExtensionCallback SynIpcRxExtensionCallback;

/* SynIpcCustomRxCallback

   When Custom message originating from bt subsystem is 
   received by IPC driver, this callback is invoked. 
   Message type in callback will be set to SYN_IPC_MSG_HCI.

   SYN_IPC_MSG_CUSTOM: On receiving this message application can act 
   upon the messages or choose to ignore. SynIpcMsgCustom->data need not be
   freed. Currently return value is ignored by the syn ipc.
*/
typedef SynIpcExtensionCallback SynIpcCustomRxCallback;


/* Bit fields */
#define SYN_IPC_HCI_PKT_CMD     0x01
#define SYN_IPC_HCI_PKT_ACL     0x02
#define SYN_IPC_HCI_PKT_EVT     0x04
typedef CsrUint8 SynIpcHciPktTypeReq;

/* Application packet types supported currently are SYN_IPC_HCI_PKT_CMD */
CsrBool SynIpcRegisterTxExtension(SynIpcTxExtensionCallback txCb, SynIpcHciPktTypeReq pktTypeReq);
/* Application packet types are SYN_IPC_HCI_PKT_EVT & SYN_IPC_HCI_PKT_ACL */
CsrBool SynIpcRegisterRxExtension(SynIpcRxExtensionCallback rxCb, SynIpcHciPktTypeReq pktTypeReq);
void SynIpcRegisterCustom(SynIpcCustomRxCallback rxCb); 
void SynIpcCustomMessageSend(SynIpcMsgId msgId, CsrUint8 *data, CsrUint16 dataLen, CsrBool drainHci, CsrBool freeMemory);
void SynIpcSendToHci(SynIpcHciPktType hciPktType, CsrUint8 *data, CsrUint32 dataLen);

#if defined (CSR_PLATFORM_WINDOWS) || defined (CSR_PLATFORM_LINUX)
#if (SYN_BT_HOST_TRANSPORT == IPC)
void SynIpcSendToHciTest(SynIpcHciPktType hciPktType, CsrUint8 *data, CsrUint32 dataLen);
#endif /* (SYN_BT_HOST_TRANSPORT == IPC) */
#endif /* (CSR_PLATFORM_WINDOWS) || defined (CSR_PLATFORM_LINUX) */

/* This api allows application to construct and send a HCI pdu to the
    host stack. */
CsrUint8 SynIpcSendHciPdu(SynIpcHciPdu *hci);

#if defined (CSR_PLATFORM_WINDOWS) || defined (CSR_PLATFORM_LINUX)
#if (SYN_BT_HOST_TRANSPORT != IPC)
#define SynIpcRegisterTxExtension(a,b)
#define SynIpcRegisterRxExtension(a,b)
#define SynIpcRegisterCustom(a) NULL
#define SynIpcCustomMessageSend(a,b,c,d, e)
#define SynIpcMemLockedPacketCount() 0
#define SynIpcMemReadLocked(a, b) 
#define SynIpcSendToHci(a,b,c)
#endif /* (SYN_BT_HOST_TRANSPORT != IPC) */
#endif

#endif
