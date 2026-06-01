/******************************************************************************
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#ifndef __BMM_PRIVATE_H__
#define __BMM_PRIVATE_H__

#include "csr_synergy.h"
#include "csr_types.h"
#include "bmm_prim.h"
#include "csr_log_text_2.h"
#include "bmm_lib.h"
#include "csr_bt_core_stack_pmalloc.h"
#include "csr_tm_bluecore_prim.h"
#include "bmm.h"
#include "socket_offload.h"
#include "rfcomm_lib.h"
#include "bmm_config.h"


#ifdef __cplusplus
extern "C" {
#endif


void BmmSocketRfcommOffloadReqHandler(BmmSocketRfcommOffloadReq * prim);
void BmmSocketLecocOffloadReqHandler(BmmSocketLecocOffloadReq * prim);
void BmmSocketCloseReqHandler(BmmSocketCloseReq * prim);
void BmmSocketSetParamsReqHandler(BmmSocketSetParamsReq * prim);
void BmmSocketDataReqHandler(BmmSocketDataReq * prim);
void BmmSocketDataRspHandler(BmmSocketDataRsp * prim);
void BmmSetEventMaskReqHandler(BmmSetEventMaskReq * prim);
void BmmActivateTransportCfmHandler(CsrTmBluecoreActivateTransportCfm * prim);
void BmmPrepareStopReqHandler(BmmPrepareStopReq * prim);

void BmmHciArbiterBtOffIndHandler(HciArbiterBtOffInd *prim);
void BmmHciArbiterBtssCrashIndHandler(HciArbiterBtssCrashInd *prim);

void BmmSocketRfcommOffloadCfmSend(BmmSocketRfcommOffloadReq * prim, BmmConnId connId, BmmResultCode result);
void BmmSocketLecocOffloadCfmSend(BmmSocketLecocOffloadReq * prim, BmmConnId connId, BmmResultCode result);
void BmmSocketCloseCfmSend(BmmSchedQid phandle, BmmConnId connId, BmmResultCode result);
void BmmSocketErrorSend(BmmSchedQid phandle, BmmConnId  connId, BmmErrorCode error);
void BmmSocketSetParamsCfmSend(BmmSocketSetParamsReq *prim, uint32 value, BmmResultCode  result);
void BmmSetEventMaskCfmSend(BmmSchedQid phandle, BmmEventMask mask, BmmResultCode  result);
void BmmSocketDataIndSend(BmmSchedQid phandle, BmmConnId connId, uint16 dataLen, uint8 *data, BmmContext context);
void BmmSocketDataCfmSend(BmmSchedQid phandle, BmmConnId connId, BmmContext context, BmmResultCode result);
void BmmInitializedIndSend(BmmSchedQid phandle, BmmInitStatus status);
void BmmDeinitializedIndSend(BmmSchedQid phandle);
void BmmPrepareStopCfmSend(BmmSchedQid phandle, BmmResultCode  result);
void BmmBtssErrorIndSend(BmmSchedQid pHandle, BmmBtssErrorCode reason);


bool BmmIsConnIdValid(BmmConnId connId);
void BmmPropagateEvent(BmmEventMask event);
uint16 BmmGetFreeConnIdIndex(void);
BmmResultCode BmmValidateSocketOffloadReq(void *prim, uint8 * index);
BmmResultCode BmmValidateSocketParamsReq(BmmSocketSetParamsReq *prim);
BmmConnId BmmGenerateConnId(BmmProtocol protocol, BmmSocketId socketId, uint8 index);
void BmmDisconnectL2capConnForRfcomm(BmmRfcommConnInst *rfcomm);
void BmmInitSocketInst(BmmSocketInst *socketInst, 
                       BmmConnId connId, BmmSocketId socketId, 
                       BmmSchedQid pHandle, BmmHciHandle hciHandle);
BmmResultCode BmmMapHciArbErrorCode(CsrBtResultCode hciResult);

void BmmLecocDataWriteCfmHandler(L2CA_DATAWRITE_CFM_T *prim);
void BmmLecocDataReadIndHandler(L2CA_DATAREAD_IND_T **prim);
void BmmLecocAddCreditCfmHandler(L2CA_ADD_CREDIT_CFM_T *prim);
void BmmLecocDisconnectIndHandler(L2CA_DISCONNECT_IND_T *prim);
void BmmRfcommDataWriteCfmHandler(RFC_DATAWRITE_CFM_T *prim);
void BmmRfcommDataReadIndHandler(RFC_DATAREAD_IND_T **prim);
void BmmRfcommDisconnectIndHandler(RFC_DISCONNECT_IND_T *prim);
void BmmLecocClearRxQueue(BmmLecocConnInst *conn);
void BmmRfcommClearRxQueue(BmmRfcommConnInst *conn);
uint8 BmmGetIndexFromRfcommConId(uint16 rfcommConnId);
void BmmStartSchedTaskWaitTimer(void);


#endif /* __BMM_PRIVATE_H__ */
