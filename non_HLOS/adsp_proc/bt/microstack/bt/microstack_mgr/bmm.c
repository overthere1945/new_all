/******************************************************************************
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "bmm_private.h"

BmmInst gBmm;


void BmmSocketRfcommOffloadReqHandler(BmmSocketRfcommOffloadReq * prim)
{
    uint8 index;
    BmmResultCode result;
    CsrBtResultCode hciResult;
    BmmConnId connId = BMM_INVALID_CONNID;
    HciArbFilterId filterId = HCI_ARB_INVALID_FILTER_ID;

    BMM_INFO("==> BMM_SOCKET_RFCOMM_OFFLOAD_REQ Rcvd");
    BMM_INFO("HciHandle 0x%x, Local Cid 0x%x, Remote Cid 0x%x, Dlci 0x%x",
             prim->hciHandle, prim->localCid, prim->remoteCid, prim->dlci);
    BMM_INFO("max Frame Size %d, Rx credits %d, Tx Credits %d, Mux %d",
             prim->maxFrameSize, prim->initialRxCredits, 
             prim->initialTxCredits, prim->muxInitiator);

    result = BmmValidateSocketOffloadReq((void *)prim, &index);

    if (result == BMM_RESULT_SUCCESS)
    {
        connId = BmmGenerateConnId(BMM_RFCOMM_PROTOCOL, prim->socketId, index);

        hciResult = HciArbEnableRfcommFilter(prim->hciHandle, prim->localCid, 
                                             prim->dlci, &filterId);

        result = BmmMapHciArbErrorCode(hciResult);
        if (result == BMM_RESULT_SUCCESS)
        {
            l2ca_cid_t cid;
            uint16 rfcConnId=0;
            BmmSocketInst *socketInst = &gBmm.socket[index];

            BmmInitSocketInst(socketInst, connId, prim->socketId, 
                              prim->pHandle, prim->hciHandle); 

            socketInst->filterId = filterId;
            socketInst->protocol = BMM_RFCOMM_PROTOCOL;
            socketInst->proto.rfcomm = zpnew(BmmRfcommConnInst);

            socketInst->proto.rfcomm->rxAppCredits = BMM_RFCOMM_RX_APP_CREDIT;

            SynMemCpyS(&(socketInst->proto.rfcomm->ctx), 
                         sizeof(*prim), prim, sizeof(*prim));

            cid = L2CAOFFLOAD_RfcommConn(prim->hciHandle, 
                                         prim->localCid, prim->remoteCid, 
                                         prim->localMtu, prim->remoteMtu, 
                                         0, RFCOMM_IFACEQUEUE);

            rfcConnId = rfcomm_offload_dlci(prim->dlci, prim->initialTxCredits,
                                            prim->maxFrameSize, 
                                            prim->muxInitiator, 
                                            cid, prim->localMtu, BMM_IFACEQUEUE);

            socketInst->proto.rfcomm->rfcConnId = rfcConnId;
        }
    }

    BmmSocketRfcommOffloadCfmSend(prim, connId, result);
}


void BmmSocketLecocOffloadReqHandler(BmmSocketLecocOffloadReq * prim)
{
    uint8 index;
    BmmResultCode result;
    CsrBtResultCode hciResult;
    BmmConnId connId = BMM_INVALID_CONNID;
    HciArbFilterId filterId = HCI_ARB_INVALID_FILTER_ID;

    BMM_INFO("==> BMM_SOCKET_LECOC_OFFLOAD_REQ Rcvd");
    BMM_INFO("HciHandle 0x%x, Local Cid 0x%x, Remote Cid 0x%x, Rx credits %d, Tx credits %d",
             prim->hciHandle, prim->localCid, prim->remoteCid, 
             prim->initialRxCredits, prim->initialTxCredits);
    BMM_INFO("local mtu %d, remote mtu %d, local mps %d, remote mps %d",
             prim->localMtu, prim->remoteMtu, prim->localMps, prim->remoteMps);

    result = BmmValidateSocketOffloadReq((void *)prim, &index);

    if (result == BMM_RESULT_SUCCESS)
    {
        connId = BmmGenerateConnId(BMM_LECOC_PROTOCOL, prim->socketId, index);

        hciResult = HciArbEnableLecocFilter(prim->hciHandle, prim->localCid,
                                            prim->remoteCid, &filterId);

        result = BmmMapHciArbErrorCode(hciResult);
        if (result == BMM_RESULT_SUCCESS)
        {
            l2ca_cid_t cid;
            BmmSocketInst *socketInst = &gBmm.socket[index];

            BmmInitSocketInst(socketInst, connId, prim->socketId, 
                              prim->pHandle, prim->hciHandle); 

            socketInst->filterId = filterId;
            socketInst->protocol = BMM_LECOC_PROTOCOL;
            socketInst->proto.lecoc = zpnew(BmmLecocConnInst);
            SynMemCpyS(&(socketInst->proto.lecoc->ctx), 
                         sizeof(*prim), prim, sizeof(*prim));

            socketInst->proto.lecoc->rxAppCredits = BMM_LECOC_RX_APP_CREDIT;

            cid = L2CAOFFLOAD_LecocConn(prim->hciHandle, 
                                         prim->localCid, prim->remoteCid, 
                                         prim->localMtu, prim->remoteMtu,
                                         prim->localMps, prim->remoteMps,
                                         prim->initialRxCredits,
                                         prim->initialTxCredits,
                                         connId, BMM_IFACEQUEUE);
            socketInst->proto.lecoc->cid = cid;
        }
    }

    BmmSocketLecocOffloadCfmSend(prim, connId, result);
}


void BmmSocketCloseReqHandler(BmmSocketCloseReq * prim)
{
    uint8 index = BMM_EXTRACT_INDEX_FROM_CONNID(prim->connId);
    BmmProtocol proto = BMM_EXTRACT_PROTO_FROM_CONNID(prim->connId);

    BMM_INFO("==> BMM_SOCKET_CLOSE_REQ 0x%x", prim->connId);

    if (!BmmIsConnIdValid(prim->connId))
    {
        BmmSocketCloseCfmSend(prim->pHandle, prim->connId, BMM_RESULT_INVALID_CONNID);
        return;
    }

    if (proto == BMM_RFCOMM_PROTOCOL)
    {
        BmmRfcommConnInst *rfcomm = gBmm.socket[index].proto.rfcomm;

        rfcomm_disconnect_dlci(rfcomm->rfcConnId);
        BmmDisconnectL2capConnForRfcomm(rfcomm);
        BmmRfcommClearRxQueue(rfcomm);

        CsrPmemFree(rfcomm);
        gBmm.socket[index].proto.rfcomm = NULL;
    }
    else if (proto == BMM_LECOC_PROTOCOL)
    {
        BmmLecocConnInst * lecoc = gBmm.socket[index].proto.lecoc;

        L2CAOFFLOAD_DisconnectCid(lecoc->ctx.hciHandle, lecoc->ctx.remoteCid);
        BmmLecocClearRxQueue(lecoc);
        CsrPmemFree(lecoc);
        gBmm.socket[index].proto.lecoc = NULL;
    }

    HciArbDisableFilter(gBmm.socket[index].filterId);

    CsrMemSet(&gBmm.socket[index], 0x00, sizeof(BmmSocketInst));
    gBmm.socket[index].inUse = FALSE;

    BmmSocketCloseCfmSend(prim->pHandle, prim->connId, BMM_RESULT_SUCCESS);
}

void BmmSocketSetParamsReqHandler(BmmSocketSetParamsReq * prim)
{
    BmmResultCode result;
    uint32 value = prim->value;
    uint8 index = BMM_EXTRACT_INDEX_FROM_CONNID(prim->connId);
    BmmProtocol proto = BMM_EXTRACT_PROTO_FROM_CONNID(prim->connId);

    BMM_INFO("==> BMM_SOCKET_SET_PARAMS_REQ Rcvd");

    result = BmmValidateSocketParamsReq(prim);
    if (result != BMM_RESULT_SUCCESS)
    {
        BmmSocketSetParamsCfmSend(prim, 0, result);
        return;
    }

    switch (prim->param)
    {
        case BMM_SOCK_DATA_APP_HANDLE:
        {
            gBmm.socket[index].dataHandle = (BmmSchedQid)prim->value;
            break;
        }

        case BMM_SOCK_PREFERRED_RX_CREDITS:
        {
            if (proto == BMM_RFCOMM_PROTOCOL)
            {
                uint16 rfcConnId = gBmm.socket[index].proto.rfcomm->rfcConnId;
                value = CSRMIN(prim->value, BMM_RFCOMM_INIT_CREDITS);
                /* data write cfm not sent for credit only frame */
                rfc_datawrite_req(rfcConnId, 0, NULL, value);
            }
            else if (proto == BMM_LECOC_PROTOCOL)
            {
                l2ca_cid_t cid = gBmm.socket[index].proto.lecoc->cid;
                value = CSRMIN(prim->value, BMM_LECOC_INIT_CREDITS);
                L2CA_AddCreditReq(cid, gBmm.socket[index].connId, value);
            }
            gBmm.socket[index].creditInitialised = TRUE;
            break;
        }

        default:
        {
            result = BMM_RESULT_INVALID_PARAMETER;
            break;
        }
    }
    BmmSocketSetParamsCfmSend(prim, value, result);
}


void BmmSetEventMaskReqHandler(BmmSetEventMaskReq * prim)
{
    uint16 i;
    uint16 freeIndex = BMM_MAX_SUBSCRIPTIONS;
    uint16 foundIndex = BMM_MAX_SUBSCRIPTIONS;
    BmmResultCode result = BMM_RESULT_SUCCESS;
    BmmEventMask mask = prim->eventMask & BMM_EVENT_MASK_RESERVER_VALUES_MASK;

    BMM_INFO("==> BMM_SET_EVENT_MASK_REQ Rcvd");

    for (i=0; i< BMM_MAX_SUBSCRIPTIONS; i++)
    {
        if ((gBmm.subscriptions[i].isValid == FALSE) &&
            (freeIndex == BMM_MAX_SUBSCRIPTIONS))
        {
            freeIndex = i;
        }
        else if ((gBmm.subscriptions[i].isValid == TRUE) &&
            (gBmm.subscriptions[i].pHandle == prim->pHandle))
        {
            foundIndex = i;
            break;
        }
    }

    if (mask == BMM_EVENT_MASK_SUBSCRIBE_NONE)
    {
        if (foundIndex < BMM_MAX_SUBSCRIPTIONS)
        {
            gBmm.subscriptions[foundIndex].isValid = FALSE;
            gBmm.subscriptions[foundIndex].eventMask = 0;
            gBmm.subscriptions[foundIndex].pHandle = 0;
        }
        else
        {
            result = BMM_RESULT_INVALID_OPERATION;
        }
    }
    else
    {
        if (foundIndex < BMM_MAX_SUBSCRIPTIONS)
        {
            gBmm.subscriptions[foundIndex].eventMask |= mask;
        }
        else if (freeIndex < BMM_MAX_SUBSCRIPTIONS)
        {
            gBmm.subscriptions[freeIndex].isValid = TRUE;
            gBmm.subscriptions[freeIndex].eventMask = mask;
            gBmm.subscriptions[freeIndex].pHandle = prim->pHandle;
        }
        else
        {
            result = BMM_RESULT_MAX_APPS_REACHED;
        }
    }

    BmmSetEventMaskCfmSend(prim->pHandle, mask, result);

    if ((result == BMM_RESULT_SUCCESS) &&
        (mask & BMM_EVENT_MASK_SUBSCRIBE_INITIALIZED) &&
        ((gBmm.state == BMM_STATE_INIT_SUCCESS) || (gBmm.state == BMM_STATE_INIT_FAILURE)))
    {
        BmmInitStatus state = (gBmm.state == BMM_STATE_INIT_SUCCESS) ? 
                              BMM_INIT_SUCCESS : BMM_INIT_FAILURE;
        BmmInitializedIndSend(prim->pHandle, state);
    }
}

void BmmActivateTransportCfmHandler(CsrTmBluecoreActivateTransportCfm * prim)
{    
    if (prim->result == CSR_RESULT_SUCCESS)
    {
        BMM_INFO("Transport activation success");
        gBmm.state = BMM_STATE_INIT_SUCCESS;
    }
    else
    {
        BMM_INFO("Transport activation failure %d", prim->result);
        gBmm.state = BMM_STATE_INIT_FAILURE;
    }
    BmmPropagateEvent(BMM_EVENT_MASK_SUBSCRIBE_INITIALIZED);
}

