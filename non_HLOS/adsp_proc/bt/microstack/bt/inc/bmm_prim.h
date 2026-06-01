/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#ifndef __BMM_PRIM_H__
#define __BMM_PRIM_H__

#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif


/* This file captures Bluetooth Microstack Manager(BMM) APIs for Micro Stack */

/* Basic types */
typedef uint16          BmmPrim;
typedef uint16          BmmHciHandle;
typedef uint16          BmmL2capCid;
typedef uint16          BmmL2capMtu;
typedef uint16          BmmL2capMps;
typedef CsrUint64       BmmSocketId;
typedef uint32          BmmConnId;
typedef uint32          BmmContext;
typedef CsrSchedQid     BmmSchedQid;


typedef uint16 BmmResultCode;
#define BMM_RESULT_SUCCESS                          ((BmmResultCode)0x0000) 
#define BMM_RESULT_INVALID_PARAMETER                ((BmmResultCode)0x0001) 
#define BMM_RESULT_INVALID_OPERATION                ((BmmResultCode)0x0002) 
#define BMM_RESULT_INVALID_CONNID                   ((BmmResultCode)0x0003)
#define BMM_RESULT_OPERATION_FAILED                 ((BmmResultCode)0x0004)
#define BMM_RESULT_INSUFFICIENT_RESOURCES           ((BmmResultCode)0x0005)
#define BMM_RESULT_FLOW_CONTROL_VIOLATED            ((BmmResultCode)0x0006)
#define BMM_RESULT_MTU_VIOLATION                    ((BmmResultCode)0x0007)
#define BMM_RESULT_SOCKETID_ALREADY_EXISTS          ((BmmResultCode)0x0008)
/* Socket error, Application is not expected to send anymore data and 
 * waits for close socket from primary stack */
#define BMM_RESULT_CONNID_IN_ERROR                  ((BmmResultCode)0x0009)
#define BMM_RESULT_ACL_DOES_NOT_EXIST               ((BmmResultCode)0x000A)
#define BMM_RESULT_MAX_CONNECTIONS_REACHED          ((BmmResultCode)0x000B)
#define BMM_RESULT_MAX_APPS_REACHED                 ((BmmResultCode)0x000C)
#define BMM_RESULT_STOP_FAILURE                     ((BmmResultCode)0x000D)

typedef uint16 BmmErrorCode;
#define BMM_MTU_VIOLATION                           ((BmmErrorCode)0x0000) 
#define BMM_MPS_VIOLATION                           ((BmmErrorCode)0x0001) 
#define BMM_PROTOCOL_VIOLATION                      ((BmmErrorCode)0x0002)

typedef uint8 BmmInitStatus;
/* Microstack initialised, HCI transport is up */
#define BMM_INIT_SUCCESS                            ((BmmInitStatus)0x01)
#define BMM_INIT_FAILURE                            ((BmmInitStatus)0x02)


typedef uint8 BmmDeinitStatus;
#define BMM_DEINIT_SUCCESS                          ((BmmDeinitStatus)0x01)
#define BMM_DEINIT_FAILURE                          ((BmmDeinitStatus)0x02) 


typedef uint16 BmmSocketParam;
/* 
 * BMM_SOCK_PREFERRED_RX_CREDITS is used for releasing the credits to remote 
 * This is the preferred Rx credit, but Micro stack can override this value
 * This can be called once Offload app is initialized to enable Rx data path
 * Relevant for LECOC and RFCOMM socket
 */
#define BMM_SOCK_PREFERRED_RX_CREDITS               ((BmmSocketParam)0x0001)
/* 
 * This is optional
 * Application can set app handle for data path
 * BMM_SOCKET_DATA_IND is sent on this handle
 * Ideally application is expected to set this before enabling  
 * BMM_SOCK_PREFERRED_RX_CREDITS
 */
#define BMM_SOCK_DATA_APP_HANDLE                    ((BmmSocketParam)0x0002)
#define BMM_SOCK_MAX_SOCK_PARAMS                    BMM_SOCK_DATA_APP_HANDLE


typedef uint16 BmmEventMask;
/* Defines for event that the application can subscribe for */
#define BMM_EVENT_MASK_SUBSCRIBE_NONE               ((BmmEventMask) 0x0000)
#define BMM_EVENT_MASK_SUBSCRIBE_INITIALIZED        ((BmmEventMask) 0x0001)
#define BMM_EVENT_MASK_SUBSCRIBE_DEINITIALIZED      ((BmmEventMask) 0x0002)
#define BMM_EVENT_MASK_SUBSCRIBE_BTSS_ERROR         ((BmmEventMask) 0x0004)

#define BMM_EVENT_MASK_RESERVER_VALUES_MASK         (0x0007)


#define BMM_INVALID_CONNID                          ((BmmConnId)0x00000000)

typedef struct
{
    uint16         maxRfcommConn;       /* Max no of offloaded RFCOMM connections */
    uint16         maxFrameSize;        /* max supported frame size */
} BmmRfcommCap;

typedef struct
{
    uint16         maxLecocConn;        /* Max no of offloaded LECOC connections */
    uint16         mtu;                 /* Max supported MTU */
} BmmLecocCap;

typedef struct
{
    BmmRfcommCap   rfcomm;
    BmmLecocCap    lecoc;
} BmmSocketCapabilities;

typedef uint8 BmmStopReason;
#define BMM_STOP_REASON_SSR           (BmmStopReason)0x01 /*!< Handle stop as a result of SSR*/
#define BMM_STOP_REASON_USER          (BmmStopReason)0x02 /*!< Handle stop as a result of user enforced stop*/
#define BMM_STOP_REASON_FMD           (BmmStopReason)0x03 /*!< Handle stop as a result of FMD*/

typedef uint8 BmmBtssErrorCode;
#define BMM_BTSS_PERI_ERROR                          ((BmmBtssErrorCode)0x01)
#define BMM_BTSS_BT_ERROR                            ((BmmBtssErrorCode)0x02)
#define BMM_BTSS_SMC_ERROR                           ((BmmBtssErrorCode)0x03)

/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/
/* Downstream */
#define BMM_SOCKET_RFCOMM_OFFLOAD_REQ       ((BmmPrim)0x0001)
#define BMM_SOCKET_LECOC_OFFLOAD_REQ        ((BmmPrim)0x0002)
#define BMM_SOCKET_CLOSE_REQ                ((BmmPrim)0x0003)
#define BMM_SOCKET_SET_PARAMS_REQ           ((BmmPrim)0x0004)
#define BMM_SOCKET_DATA_REQ                 ((BmmPrim)0x0005)
#define BMM_SOCKET_DATA_RSP                 ((BmmPrim)0x0006)
#define BMM_SET_EVENT_MASK_REQ              ((BmmPrim)0x0007)
#define BMM_PREPARE_STOP_REQ                ((BmmPrim)0x0008)

/* Upstream */
#define BMM_SOCKET_RFCOMM_OFFLOAD_CFM       ((BmmPrim)0x8000)
#define BMM_SOCKET_LECOC_OFFLOAD_CFM        ((BmmPrim)0x8001)
#define BMM_SOCKET_CLOSE_CFM                ((BmmPrim)0x8002)
#define BMM_SOCKET_SET_PARAMS_CFM           ((BmmPrim)0x8003)
#define BMM_SOCKET_ERROR_IND                ((BmmPrim)0x8004)
#define BMM_SOCKET_DATA_CFM                 ((BmmPrim)0x8005)
#define BMM_SOCKET_DATA_IND                 ((BmmPrim)0x8006)
#define BMM_SET_EVENT_MASK_CFM              ((BmmPrim)0x8007)
#define BMM_INITIALIZED_IND                 ((BmmPrim)0x8008)
#define BMM_DEINITIALIZED_IND               ((BmmPrim)0x8009)
#define BMM_PREPARE_STOP_CFM                ((BmmPrim)0x800A)
#define BMM_BTSS_ERROR_IND                  ((BmmPrim)0x800B)


typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_RFCOMM_OFFLOAD_REQ */
    BmmSocketId         socketId;           /* Socket Id */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmHciHandle        hciHandle;          /* HCI Connection handle */
    BmmL2capCid         localCid;           /* Local L2CAP CID */
    BmmL2capCid         remoteCid;          /* Remote L2CAP CID */
    BmmL2capMtu         localMtu;           /* Local L2CAP MTU */
    BmmL2capMtu         remoteMtu;          /* Remote L2CAP MTU */
    uint8               dlci;               /* RFCOMM DLCI */ 
    uint16              initialRxCredits;   /* Local credits for DLCI */ 
    uint16              initialTxCredits;   /* Remote credits for DLCI */ 
    uint16              maxFrameSize;       /* Max frame size for DLCI */ 
    uint16              muxInitiator;       /* Set to 1 if RFCOMM Multiplexer is initiated by Primary stack */
    BmmContext          context;            /* same context returned in BmmSocketRfcommOffloadCfm */
} BmmSocketRfcommOffloadReq;

typedef struct 
{
    BmmPrim             type;               /* Message type */
    BmmSocketId         socketId;           /* socket Id */
    BmmConnId           connId;             /* conn Id */
    BmmContext          context;            /* context */
    BmmResultCode       resultCode;         /* Result code */
} BmmSocketOffloadStdCfm;

typedef BmmSocketOffloadStdCfm BmmSocketRfcommOffloadCfm;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_LECOC_OFFLOAD_REQ */
    BmmSocketId         socketId;           /* Socket Id */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmHciHandle        hciHandle;          /* HCI Connection handle */
    BmmL2capCid         localCid;           /* Local L2CAP CID */
    BmmL2capCid         remoteCid;          /* Remote L2CAP CID */
    BmmL2capMtu         localMtu;           /* Local L2CAP MTU */
    BmmL2capMtu         remoteMtu;          /* Remote L2CAP MTU */
    BmmL2capMps         localMps;           /* Local L2CAP MPS */
    BmmL2capMps         remoteMps;          /* Remote L2CAP MPS */
    uint16              initialRxCredits;   /* Local credits for LECOC channel */
    uint16              initialTxCredits;   /* Remote credits for LECOC channel */
    BmmContext          context;            /* Same context returned in BmmSocketLecocOffloadCfm */
} BmmSocketLecocOffloadReq;

typedef BmmSocketOffloadStdCfm BmmSocketLecocOffloadCfm;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_CLOSE_REQ */
    BmmConnId           connId;             /* connection is associated with socket to be closed */        
    BmmSchedQid         pHandle;            /* Application handle */
} BmmSocketCloseReq;


typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_CLOSE_CFM */
    BmmConnId           connId;             /* conn Id */
    BmmResultCode       resultCode;         /* Result code */
} BmmSocketCloseCfm;


typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_ERROR_IND */
    BmmConnId           connId;             /* conn Id */
    BmmErrorCode        error;              /* error code */
} BmmSocketErrorInd;


typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_SET_PARAMS_REQ */
    BmmConnId           connId;             /* conn Id */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmSocketParam      param;              /* Param to be configured */
    uint32              value;              /* Value of Param to be configured */ 
    BmmContext          context;            /* Same context returned in BmmSetSocketParamsCfm */
} BmmSocketSetParamsReq;


typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_SET_PARAMS_CFM */
    BmmConnId           connId;             /* conn Id */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmSocketParam      param;              /* Param configured */
    uint32              value;              /* Value set by the Micro stack */ 
    BmmContext          context;            /* context */ 
    BmmResultCode       resultCode;         /* Result code */
} BmmSocketSetParamsCfm;


typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_DATA_REQ */
    BmmConnId           connId;             /* conn Id */
    BmmSchedQid         pHandle;            /* Application handle */
    uint16              dataLen;            /* length of data */ 
    uint8               *data;              /* data */ 
    BmmContext          context;            /* Same context returned in BmmSocketDataCfm */
} BmmSocketDataReq;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_DATA_CFM */
    BmmConnId           connId;             /* conn Id */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmContext          context;            /* context */ 
    BmmResultCode       resultCode;         /* Result code */
} BmmSocketDataCfm;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_DATA_IND */
    BmmConnId           connId;             /* conn Id */
    BmmSchedQid         pHandle;            /* Application handle */
    uint16              dataLen;            /* length of data */
    uint8               *data;              /* data */ 
    BmmContext          context;            /* context */
} BmmSocketDataInd;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SOCKET_DATA_RSP */
    BmmConnId           connId;             /* conn Id */
    BmmContext          context;            /* context */
} BmmSocketDataRsp;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SET_EVENT_MASK_REQ */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmEventMask        eventMask;          /* event mask */
} BmmSetEventMaskReq;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_SET_EVENT_MASK_CFM */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmEventMask        eventMask;          /* event mask */
    BmmResultCode       resultCode;         /* Result code */
} BmmSetEventMaskCfm;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_INITIALIZED_IND */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmInitStatus       status;             /* init status */
} BmmInitializedInd;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_DEINITIALIZED_IND */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmDeinitStatus     status;             /* deinit status */
} BmmDeinitializedInd;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_PREPARE_STOP_REQ */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmStopReason       reason;             /* bmm prepare stop reason */
} BmmPrepareStopReq;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_PREPARE_STOP_CFM */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmResultCode       resultCode;         /* Result code */
} BmmPrepareStopCfm;

typedef struct
{
    BmmPrim             type;               /* Message type, always BMM_BTSS_ERROR_IND */
    BmmSchedQid         pHandle;            /* Application handle */
    BmmBtssErrorCode    reason;             /* error reason */
} BmmBtssErrorInd;

#ifdef __cplusplus
}
#endif

#endif /* __BMM_PRIM_H__ */

