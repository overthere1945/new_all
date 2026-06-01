/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "csr_synergy.h"

#include "bmm_lib.h"
#include "csr_log_text_2.h"
#include "csr_hci_lib.h"
#include "csr_bt_tasks.h"

void BmmPutDownstreamMsg(void *msg)
{
    CsrMsgTransport(BMM_IFACEQUEUE, BMM_PRIM, msg);    
}

BmmResultCode BmmSocketGetCapabilities(BmmSocketCapabilities *cap)
{
    BmmResultCode result = BMM_RESULT_INVALID_PARAMETER;
    
    if (cap != NULL)
    {
        cap->rfcomm.maxFrameSize = BMM_SOCK_MAX_RFCOMM_FRAME_SIZE;
        cap->rfcomm.maxRfcommConn = BMM_SOCK_MAX_RFCOMM_CONN;

        cap->lecoc.maxLecocConn = BMM_SOCK_MAX_LECOC_CONN;
        cap->lecoc.mtu= BMM_SOCK_MAX_LECOC_MTU;

        result = BMM_RESULT_SUCCESS;
    }
    return result;
}

void BmmSetEventMaskReqSend(BmmSchedQid pHandle, BmmEventMask eventMask)
{
    BmmSetEventMaskReq *prim = zpnew(BmmSetEventMaskReq);
    prim->type             = BMM_SET_EVENT_MASK_REQ;
    prim->pHandle          = pHandle;
    prim->eventMask        = eventMask;

    BmmPutDownstreamMsg(prim);
}

void BmmSocketRfcommOffloadReqSend(BmmSocketId   socketId,
                                   BmmSchedQid   pHandle,
                                   BmmHciHandle  hciHandle, 
                                   BmmL2capCid   localCid,
                                   BmmL2capCid   remoteCid,
                                   BmmL2capMtu   localMtu,
                                   BmmL2capMtu   remoteMtu,
                                   uint8         dlci,
                                   uint16        initialRxCredits,
                                   uint16        initialTxCredits,
                                   uint16        maxFrameSize,
                                   uint16        muxInitiator,
                                   BmmContext    context)
{
    BmmSocketRfcommOffloadReq *prim = zpnew(BmmSocketRfcommOffloadReq);

    prim->type             = BMM_SOCKET_RFCOMM_OFFLOAD_REQ;
    prim->socketId         = socketId;
    prim->pHandle          = pHandle;
    prim->hciHandle        = hciHandle;
    prim->localCid         = localCid;
    prim->remoteCid        = remoteCid;
    prim->localMtu         = localMtu;
    prim->remoteMtu        = remoteMtu;
    prim->dlci             = dlci;
    prim->initialRxCredits = initialRxCredits;
    prim->initialTxCredits = initialTxCredits;
    prim->maxFrameSize     = maxFrameSize;
    prim->muxInitiator     = muxInitiator;
    prim->context          = context;

    BmmPutDownstreamMsg(prim);
}

void BmmSocketLecocOffloadReqSend(BmmSocketId   socketId,
                                  BmmSchedQid   pHandle,
                                  BmmHciHandle  hciHandle, 
                                  BmmL2capCid   localCid,
                                  BmmL2capCid   remoteCid,
                                  BmmL2capMtu   localMtu,
                                  BmmL2capMtu   remoteMtu,
                                  BmmL2capMps   localMps,
                                  BmmL2capMps   remoteMps,
                                  uint16        initialRxCredits,
                                  uint16        initialTxCredits,
                                  BmmContext    context)
{
    BmmSocketLecocOffloadReq *prim = zpnew(BmmSocketLecocOffloadReq);

    prim->type             = BMM_SOCKET_LECOC_OFFLOAD_REQ;
    prim->socketId         = socketId;
    prim->pHandle          = pHandle;
    prim->hciHandle        = hciHandle;
    prim->localCid         = localCid;
    prim->remoteCid        = remoteCid;
    prim->localMtu         = localMtu;
    prim->remoteMtu        = remoteMtu;
    prim->localMps         = localMps;
    prim->remoteMps        = remoteMps;
    prim->initialRxCredits = initialRxCredits;
    prim->initialTxCredits = initialTxCredits;
    prim->context          = context;

    BmmPutDownstreamMsg(prim);
}


void BmmSocketCloseReqSend(BmmConnId connId, BmmSchedQid pHandle)
{
    BmmSocketCloseReq *prim = zpnew(BmmSocketCloseReq);

    prim->type     = BMM_SOCKET_CLOSE_REQ;
    prim->connId   = connId;
    prim->pHandle  = pHandle;

    BmmPutDownstreamMsg(prim);
}


void BmmSocketDataReqSend(BmmConnId    connId,
                          BmmSchedQid  pHandle,    
                          uint16       dataLen,
                          uint8        *data,
                          BmmContext   context)
{
    BmmSocketDataReq *prim = zpnew(BmmSocketDataReq);

    prim->type     = BMM_SOCKET_DATA_REQ;
    prim->connId   = connId;
    prim->pHandle  = pHandle;
    prim->dataLen  = dataLen;
    prim->data     = data;
    prim->context  = context;

    BmmPutDownstreamMsg(prim);
}

void BmmSocketDataRspSend(BmmConnId    connId,
                          BmmContext   context)
{
    BmmSocketDataRsp *prim = zpnew(BmmSocketDataRsp);

    prim->type     = BMM_SOCKET_DATA_RSP;
    prim->connId   = connId;
    prim->context  = context;

    BmmPutDownstreamMsg(prim);
}


void BmmSocketSetParamsReqSend(BmmConnId       connId,
                               BmmSchedQid     pHandle,    
                               BmmSocketParam  param,
                               uint32          value,
                               BmmContext      context)
{
    BmmSocketSetParamsReq *prim = zpnew(BmmSocketSetParamsReq);

    prim->type     = BMM_SOCKET_SET_PARAMS_REQ;
    prim->connId   = connId;
    prim->pHandle  = pHandle;
    prim->param    = param;
    prim->value    = value;
    prim->context  = context;

    BmmPutDownstreamMsg(prim);
}

void BmmPrepareStopReqSend(BmmSchedQid     pHandle,
                           BmmStopReason   reason)
{
    BmmPrepareStopReq *prim = zpnew(BmmPrepareStopReq);

    prim->type        = BMM_PREPARE_STOP_REQ;
    prim->pHandle     = pHandle;
    prim->reason      = reason;

    BmmPutDownstreamMsg(prim);
}

void BmmFreePrimitive(void *msg)
{
    BmmPrim *prim = (BmmPrim *) msg;

    if (prim == NULL)
        return;

    switch(*prim)
    {
        case BMM_SOCKET_DATA_REQ:
        {
            BmmSocketDataReq *p = msg;
            CsrPmemFree(p->data);
            p->data = NULL;
            break;
        }

        case BMM_SOCKET_DATA_IND:
        {
            BmmSocketDataInd *p = msg;
            CsrPmemFree(p->data);
            p->data = NULL;
            break;
        }

        default:
        {
            break;
        }
    }
}

