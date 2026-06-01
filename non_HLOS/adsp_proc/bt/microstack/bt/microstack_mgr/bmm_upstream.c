/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "csr_synergy.h"

#include "bmm_lib.h"
#include "csr_log_text_2.h"
#include "csr_hci_lib.h"
#include "bmm_private.h"
#include "csr_bt_tasks.h"

void BmmPutUpstreamMsg(BmmSchedQid pHandle, void *msg)
{
    CsrSchedMessagePut(pHandle, BMM_PRIM, msg);
}


void BmmSocketOffloadCfmSend(BmmPrim type,
                             BmmSocketId socketId,
                             BmmConnId connId,                             
                             BmmResultCode result,
                             BmmContext context,
                             BmmSchedQid pHandle)
{
    BmmSocketOffloadStdCfm *msg = zpnew(BmmSocketOffloadStdCfm);

    msg->type       = type;
    msg->socketId   = socketId;
    msg->connId     = (result == BMM_RESULT_SUCCESS) ? connId : BMM_INVALID_CONNID;
    msg->context    = context;
    msg->resultCode = result;
    
    BmmPutUpstreamMsg(pHandle, msg);
}


void BmmSocketRfcommOffloadCfmSend(BmmSocketRfcommOffloadReq * prim,
                                   BmmConnId connId,    
                                   BmmResultCode result)
{
    BMM_INFO("<== BMM_SOCKET_RFCOMM_OFFLOAD_CFM connId 0x%x context 0x%x handle 0x%x result 0x%x",
            connId, prim->context, prim->pHandle, result);

    BmmSocketOffloadCfmSend(BMM_SOCKET_RFCOMM_OFFLOAD_CFM, 
                            prim->socketId, connId, result, 
                            prim->context, prim->pHandle);
}


void BmmSocketLecocOffloadCfmSend(BmmSocketLecocOffloadReq * prim,
                                  BmmConnId connId,    
                                  BmmResultCode result)
{
    BMM_INFO("<== BMM_SOCKET_LECOC_OFFLOAD_CFM connId 0x%x context 0x%x handle 0x%x result 0x%x",
            connId, prim->context, prim->pHandle, result);

    BmmSocketOffloadCfmSend(BMM_SOCKET_LECOC_OFFLOAD_CFM, 
                            prim->socketId, connId, result, 
                            prim->context, prim->pHandle);
}


void BmmSocketCloseCfmSend(BmmSchedQid pHandle, 
                           BmmConnId connId, 
                           BmmResultCode result)
{
    BmmSocketCloseCfm *msg = zpnew(BmmSocketCloseCfm);

    BMM_INFO("<== BMM_SOCKET_CLOSE_CFM connId 0x%x handle 0x%x result 0x%x", 
            connId, pHandle, result);

    msg->type       = BMM_SOCKET_CLOSE_CFM;
    msg->connId     = connId;
    msg->resultCode = result;
    
    BmmPutUpstreamMsg(pHandle, msg);    
}

void BmmSocketDataCfmSend(BmmSchedQid pHandle, 
                          BmmConnId connId,
                          BmmContext context,
                          BmmResultCode result)
{
    BmmSocketDataCfm *msg = zpnew(BmmSocketDataCfm);

    BMM_DEBUG("<== BMM_SOCKET_DATA_CFM connId 0x%x context 0x%x result 0x%x", 
            connId, context, result);

    msg->type       = BMM_SOCKET_DATA_CFM;
    msg->connId     = connId;
    msg->pHandle    = pHandle;
    msg->context    = context;
    msg->resultCode = result;

    BmmPutUpstreamMsg(pHandle, msg);    
}

void BmmSocketDataIndSend(BmmSchedQid pHandle, 
                          BmmConnId connId,
                          uint16 dataLen,
                          uint8 *data,
                          BmmContext context)
{
    BmmSocketDataInd *msg = zpnew(BmmSocketDataInd);

    BMM_DEBUG("<== BMM_SOCKET_DATA_IND connId 0x%x len 0x%x context 0x%x", 
            connId, dataLen, context);

    msg->type       = BMM_SOCKET_DATA_IND;
    msg->connId     = connId;
    msg->pHandle    = pHandle;
    msg->dataLen    = dataLen;
    msg->data       = data;
    msg->context    = context;

    BmmPutUpstreamMsg(pHandle, msg);
}

void BmmSocketErrorSend(BmmSchedQid pHandle,
                        BmmConnId  connId, 
                        BmmErrorCode error)
{
    BmmSocketErrorInd *msg = zpnew(BmmSocketErrorInd);

    BMM_INFO("<== BMM_SOCKET_ERROR_IND connId 0x%x handle 0x%x result 0x%x", 
            connId, pHandle, error);

    msg->type       = BMM_SOCKET_ERROR_IND;
    msg->connId     = connId;
    msg->error      = error;
    
    BmmPutUpstreamMsg(pHandle, msg);
}

void BmmSocketSetParamsCfmSend(BmmSocketSetParamsReq *prim,
                               uint32 value,
                               BmmResultCode  result)
{
    BmmSocketSetParamsCfm *msg = zpnew(BmmSocketSetParamsCfm);

    BMM_INFO("<== BMM_SOCKET_SET_PARAMS_CFM connId 0x%x param 0x%x value 0x%x result 0x%x", 
            prim->connId, prim->param, value, result);

    msg->type       = BMM_SOCKET_SET_PARAMS_CFM;
    msg->pHandle    = prim->pHandle;
    msg->connId     = prim->connId;
    msg->param      = prim->param;
    msg->value      = value;
    msg->context    = prim->context;
    msg->resultCode = result;
    
    BmmPutUpstreamMsg(prim->pHandle, msg);
}

void BmmSetEventMaskCfmSend(BmmSchedQid pHandle, BmmEventMask mask, 
                            BmmResultCode result)
{
    BmmSetEventMaskCfm *msg = zpnew(BmmSetEventMaskCfm);

    BMM_INFO("<== BMM_SET_EVENT_MASK_CFM handle 0x%x mask 0x%x result 0x%x", 
            pHandle, mask, result);

    msg->type       = BMM_SET_EVENT_MASK_CFM;
    msg->pHandle    = pHandle;
    msg->eventMask  = mask;
    msg->resultCode = result;

    BmmPutUpstreamMsg(pHandle, msg);
}

void BmmInitializedIndSend(BmmSchedQid pHandle, BmmInitStatus status)
{
    BmmInitializedInd *msg = zpnew(BmmInitializedInd);

    BMM_INFO("<== BMM_INITIALIZED_IND handle 0x%x status 0x%x", pHandle, status);

    msg->type       = BMM_INITIALIZED_IND;
    msg->pHandle    = pHandle;
    msg->status     = status;

    BmmPutUpstreamMsg(pHandle, msg);
}

void BmmDeinitializedIndSend(BmmSchedQid pHandle)
{
    BmmDeinitializedInd *msg = zpnew(BmmDeinitializedInd);

    BMM_INFO("<== BMM_DEINITIALIZED_IND handle 0x%x", pHandle);

    msg->type       = BMM_DEINITIALIZED_IND;
    msg->pHandle    = pHandle;
    msg->status     = BMM_DEINIT_SUCCESS;

    BmmPutUpstreamMsg(pHandle, msg);
}

void BmmPrepareStopCfmSend(BmmSchedQid pHandle, BmmResultCode result)
{
    BmmPrepareStopCfm *msg = zpnew(BmmPrepareStopCfm);

    BMM_INFO("<== BMM_PREPARE_STOP_CFM handle 0x%x result 0x%x",
            pHandle, result);

    msg->type       = BMM_PREPARE_STOP_CFM;
    msg->pHandle    = pHandle;
    msg->resultCode = result;

    BmmPutUpstreamMsg(pHandle, msg);
}

void BmmBtssErrorIndSend(BmmSchedQid pHandle, BmmBtssErrorCode reason)
{
    BmmBtssErrorInd *msg = zpnew(BmmBtssErrorInd);

    BMM_INFO("<== BMM_BTSS_ERROR_IND handle 0x%x reason 0x%x", pHandle, reason);

    msg->type       = BMM_BTSS_ERROR_IND;
    msg->pHandle    = pHandle;
    msg->reason     = reason;

    BmmPutUpstreamMsg(pHandle, msg);
}
