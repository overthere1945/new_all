/******************************************************************************
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "bmm_private.h"


uint16 BmmGetFreeConnIdIndex(void)
{
    uint16 i;

    for (i = 0; i < BMM_SOCK_MAX_CONN; i++)
    {
        if (!gBmm.socket[i].inUse)
        {
            return i;
        }
    }
    return BMM_SOCK_MAX_CONN;
}


BmmConnId BmmGenerateConnId(BmmProtocol protocol, BmmSocketId socketId, uint8 index)
{
    return ((socketId & 0xFFFF) << 16 | (protocol << 8) | index);
}

bool BmmIsConnIdValid(BmmConnId connId)
{
    uint8 index = BMM_EXTRACT_INDEX_FROM_CONNID(connId);

    if ((index < BMM_SOCK_MAX_CONN) &&
         gBmm.socket[index].inUse &&
        (gBmm.socket[index].connId == connId))
    {
        BmmProtocol proto = BMM_EXTRACT_PROTO_FROM_CONNID(connId);

        if (gBmm.socket[index].protocol != proto)
        {
            BMM_PANIC(BT_OFFLOAD_PANIC_PROTO_MISMATCH, "proto mismatch");
        }
        else if ((proto == BMM_RFCOMM_PROTOCOL) &&
                 (gBmm.socket[index].proto.rfcomm == NULL))
        {
            BMM_PANIC(BT_OFFLOAD_PANIC_RFCOMM_NULL, "RFCOMM NULL ptr");
        }
        else if ((proto == BMM_LECOC_PROTOCOL) && 
                 (gBmm.socket[index].proto.lecoc == NULL))
        {
            BMM_PANIC(BT_OFFLOAD_PANIC_LECOC_NULL, "LECOC NULL ptr");
        }
        else
        {
            return TRUE;
        }
    }
    BMM_WARNING("Connection id invalid %d", connId);

    return FALSE;
}


BmmResultCode BmmValidateSocketOffloadReq(void *prim, uint8 * index)
{
    uint8 i;
    uint16 rxCredits;
    uint16 activeSockets = 0;
    uint16 maxSockets = 0;
    BmmSocketId socketId;
    BmmProtocol protocol;
    BmmPrim *type = (BmmPrim *)prim;

    if (*type == BMM_SOCKET_RFCOMM_OFFLOAD_REQ)
    {
        BmmSocketRfcommOffloadReq * rfcomm = (BmmSocketRfcommOffloadReq *)prim;
        socketId = rfcomm->socketId;
        protocol = BMM_RFCOMM_PROTOCOL;
        maxSockets = BMM_SOCK_MAX_RFCOMM_CONN;
        rxCredits = rfcomm->initialRxCredits;

        if (rfcomm->dlci == 0)
        {
            return BMM_RESULT_INVALID_PARAMETER;
        }
    }
    else if (*type == BMM_SOCKET_LECOC_OFFLOAD_REQ)
    {
        BmmSocketLecocOffloadReq * lecoc = (BmmSocketLecocOffloadReq *)prim;
        socketId = lecoc->socketId;
        protocol = BMM_LECOC_PROTOCOL;
        maxSockets = BMM_SOCK_MAX_LECOC_CONN;
        rxCredits = lecoc->initialRxCredits;
    }
    else
    {
        return BMM_RESULT_INVALID_OPERATION;
    }

    for (i = 0; i < BMM_SOCK_MAX_CONN; i++)
    {
        if (gBmm.socket[i].inUse)
        {
            if (gBmm.socket[i].socketId == socketId)
            {
                return BMM_RESULT_SOCKETID_ALREADY_EXISTS;
            }
            
            if (gBmm.socket[i].protocol == protocol)
            {
                activeSockets++;
            }
        }
    }

    if (activeSockets >= maxSockets)
    {
        return BMM_RESULT_MAX_CONNECTIONS_REACHED;
    }

    if (rxCredits != 0)
    {
        return BMM_RESULT_INVALID_PARAMETER;
    }

    if ((*index = BmmGetFreeConnIdIndex()) == BMM_SOCK_MAX_CONN)
    {
        return BMM_RESULT_MAX_CONNECTIONS_REACHED;
    }

    return BMM_RESULT_SUCCESS;
}

BmmResultCode BmmValidateSocketParamsReq(BmmSocketSetParamsReq *prim)
{
    uint8 index = BMM_EXTRACT_INDEX_FROM_CONNID(prim->connId);

    if (!BmmIsConnIdValid(prim->connId))
    {
        return BMM_RESULT_INVALID_CONNID;
    }

#ifdef BMM_ALLOW_CREDITS_REQ_ONLY_DURING_INIT
    if ((prim->param == BMM_SOCK_PREFERRED_RX_CREDITS) &&
        gBmm.socket[index].creditInitialised)
    {
        return BMM_RESULT_INVALID_OPERATION;
    }
#endif

    return BMM_RESULT_SUCCESS;
}


void BmmPropagateEvent(BmmEventMask event)
{
    uint16 i;
    
    for (i=0; i< BMM_MAX_SUBSCRIPTIONS; i++)
    {
        if ((gBmm.subscriptions[i].isValid == TRUE) &&
            (gBmm.subscriptions[i].eventMask & event))
        {
            switch (event)
            {
                case BMM_EVENT_MASK_SUBSCRIBE_INITIALIZED:
                {
                    BmmInitStatus status = (gBmm.state == BMM_STATE_INIT_SUCCESS) ? 
                                            BMM_INIT_SUCCESS : BMM_INIT_FAILURE;
                    BmmInitializedIndSend(gBmm.subscriptions[i].pHandle, status); 
                    break;
                }

                case BMM_EVENT_MASK_SUBSCRIBE_DEINITIALIZED:
                {
                    BmmDeinitializedIndSend(gBmm.subscriptions[i].pHandle);
                    break;
                }

                case BMM_EVENT_MASK_SUBSCRIBE_BTSS_ERROR:
                {
                    BmmBtssErrorIndSend(gBmm.subscriptions[i].pHandle, gBmm.errorReason);
                    break;
                }

                default:
                {
                    break;
                }
            }
        }
    }
}


uint8 BmmGetIndexFromRfcommConId(uint16 rfcommConnId)
{
    uint8 i;

    for (i = 0; i < BMM_SOCK_MAX_CONN; i++)
    {
        if ((gBmm.socket[i].inUse) && 
            (gBmm.socket[i].protocol == BMM_RFCOMM_PROTOCOL) &&
            (gBmm.socket[i].proto.rfcomm != NULL) &&
            (gBmm.socket[i].proto.rfcomm->rfcConnId == rfcommConnId))
        {
            return i;
        }
    }
    return BMM_SOCK_MAX_CONN;
}

void BmmInitSocketInst(BmmSocketInst *socketInst, 
                       BmmConnId connId, BmmSocketId socketId, 
                       BmmSchedQid pHandle, BmmHciHandle hciHandle)
{
    socketInst->inUse = TRUE;
    socketInst->connId = connId;
    socketInst->socketId = socketId;
    socketInst->dataHandle = pHandle;
    socketInst->hciHandle = hciHandle;    
    socketInst->state = BMM_SOCKET_STATE_CONNECTED;
}


void BmmDisconnectL2capConnForRfcomm(BmmRfcommConnInst *rfcomm)
{
    uint8 i;
    uint8 rfcommCount =0;
    BmmHciHandle hciHandle = rfcomm->ctx.hciHandle;
    BmmL2capCid remoteCid = rfcomm->ctx.remoteCid;

    for (i = 0; i < BMM_SOCK_MAX_CONN; i++)
    {
        if ((gBmm.socket[i].inUse) && 
            (gBmm.socket[i].protocol == BMM_RFCOMM_PROTOCOL) &&
            (gBmm.socket[i].hciHandle == hciHandle))
        {
            rfcommCount++;
        }
    }

    if (rfcommCount == 1)
    {
        /* Last DLCI is getting disconnected, L2CAP conn can be disconnected */ 
        L2CAOFFLOAD_DisconnectCid(hciHandle, remoteCid);
    }
}

BmmResultCode BmmMapHciArbErrorCode(CsrBtResultCode hciResult)
{
    BmmResultCode bmmResult;

    switch (hciResult)
    {
        case HCI_ARB_RESULT_SUCCESS:
            bmmResult = BMM_RESULT_SUCCESS;
        break;

        case HCI_ARB_RESULT_INVALID_OPERATION:
            bmmResult = BMM_RESULT_INVALID_OPERATION;
        break;

        case HCI_ARB_RESULT_ACL_DOES_NOT_EXIST:
            bmmResult = BMM_RESULT_ACL_DOES_NOT_EXIST;
        break;

        default:
            bmmResult = BMM_RESULT_INVALID_PARAMETER;
        break;
    }
    return bmmResult;
}

