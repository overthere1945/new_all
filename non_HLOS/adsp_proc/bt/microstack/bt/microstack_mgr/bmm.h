/******************************************************************************
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#ifndef __BMM_H__
#define __BMM_H__

#include "csr_synergy.h"
#include "csr_types.h"
#include "bmm_prim.h"
#include "csr_log_text_2.h"
#include "csr_hci_prim.h"
#include "bmm_lib.h"
#include "csr_hci_common.h"
#include "hci.h"
#include "csr_bt_core_stack_pmalloc.h"
#include "dm_acl.h"
#include "csr_message_queue.h"
#include "bmm_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8 BmmProtocol;
#define BMM_RFCOMM_PROTOCOL                     ((BmmProtocol)0x01)
#define BMM_LECOC_PROTOCOL                      ((BmmProtocol)0x02)

#define BMM_EXTRACT_PROTO_FROM_CONNID(connId)   (connId & 0xFF00) >> 8
#define BMM_EXTRACT_INDEX_FROM_CONNID(connId)   connId & 0xFF

#define BMM_SOCK_MAX_CONN BMM_SOCK_MAX_RFCOMM_CONN+BMM_SOCK_MAX_RFCOMM_CONN


typedef uint8 BmmState;
#define BMM_STATE_NOT_INITIALIZED               ((BmmState)0x00)
#define BMM_STATE_INIT_SUCCESS                  ((BmmState)0x01)
#define BMM_STATE_INIT_FAILURE                  ((BmmState)0x02)
#define BMM_STATE_INIT_IN_PROGRESS              ((BmmState)0x03)


typedef uint8 BmmSocketState;
#define BMM_SOCKET_STATE_INVALID                ((BmmSocketState)0x00)
#define BMM_SOCKET_STATE_CONNECTED              ((BmmSocketState)0x01)
#define BMM_SOCKET_STATE_IN_ERROR               ((BmmSocketState)0x02)

typedef struct
{    
    bool            isValid;  
    BmmEventMask    eventMask;   /* event mask */
    BmmSchedQid     pHandle;     /* Application handle */
}BmmSubscriptions;


typedef struct
{
    BmmSocketRfcommOffloadReq ctx;

    uint16           rfcConnId;

    uint16           txCount;          /* Number of elements pending in L2CAP */
    BmmContext       txPendingContext; /* Context for blocked element */
    BmmSchedQid      txPendingHandle;  /* pHandle for blocked element */

    CsrMessageQueueType *rxQueue;      /* Receive buffer queue */
    uint16            rxAppCredits;    /* Number of acks application has given us */
    uint16            rxQueueCount;    /* Total number of waiting/unacked messages */   
}BmmRfcommConnInst;


typedef struct
{
    BmmSocketLecocOffloadReq  ctx;
    l2ca_cid_t       cid;
    uint16           txCount;          /* Number of elements pending in L2CAP */
    BmmContext       txPendingContext; /* Context for blocked element */
    BmmSchedQid      txPendingHandle;  /* pHandle for blocked element */

    struct
    {
        uint16       pduSize;          /* Size of lecoc data pdu */ 
        /* In case of segmentation, Bluestack sends multiple confirm messages 
         * for each data request. This variable stores the accumulated Length 
         * of the lecoc data pdu for which one or more confirms has already 
         * been received from Bluestack. */
        uint16       lengthCfd;     
    }txSegInfo[BMM_LECOC_TX_WINDOW_SIZE];
    uint16           dequeueIdx;
    uint16           enqueueIdx;

    CsrMessageQueueType *rxQueue;      /* Receive buffer queue */
    uint16            rxAppCredits;    /* Number of acks application has given us */
    uint16            rxQueueCount;    /* Total number of waiting/unacked messages */   
    CsrMblk          *rxCombine;       /* Partial L2CAP data resegment buffer */
}BmmLecocConnInst;

typedef union
{
  BmmRfcommConnInst *rfcomm;
  BmmLecocConnInst *lecoc;
}BmmProtocolInfo;

typedef struct
{
    bool               inUse;
    BmmConnId          connId;
    BmmSocketId        socketId;
    BmmSchedQid        dataHandle;
    BmmSocketState     state;
    BmmHciHandle       hciHandle;
    bool               creditInitialised;
    HciArbFilterId     filterId;    
    BmmProtocol        protocol;
    BmmProtocolInfo    proto;
}BmmSocketInst;

typedef struct
{
    BmmState        state;
    BmmSocketInst   socket[BMM_SOCK_MAX_CONN];
    BmmSubscriptions subscriptions[BMM_MAX_SUBSCRIPTIONS];
    BmmSchedQid       prepareStopHandle;     /* Application handle */
    BmmBtssErrorCode  errorReason;
}BmmInst;

extern BmmInst gBmm;

#define BMM_LOG_ENABLE

#if defined(BMM_LOG_ENABLE)
#define BMM_INFO(...) CSR_LOG_TEXT_INFO((BMM , 0, __VA_ARGS__))
#define BMM_DEBUG(...) CSR_LOG_TEXT_INFO((BMM , 0, __VA_ARGS__))
#define BMM_WARNING(...)  CSR_LOG_TEXT_WARNING((BMM , 0, __VA_ARGS__))
#define BMM_ERROR(...)  CSR_LOG_TEXT_ERROR((BMM , 0, __VA_ARGS__))
#define BMM_PANIC(code, str)  {CsrPanic(CSR_TECH_BT, code, str);} 
#else
#define BMM_INFO(...)
#define BMM_DEBUG(...)
#define BMM_WARNING(...)
#define BMM_ERROR(...)
#define BMM_PANIC(...)  {CsrPanic(CSR_TECH_BT, code, str);} 
#endif

#endif /* __BMM_H__ */
