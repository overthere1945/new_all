/******************************************************************************
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "bmm_private.h"
#include "rfcomm_lib.h"

#define BMM_INT_ERROR       ((BmmErrorCode)0x00FF)

void BmmRfcommDataWriteCfmHandler(RFC_DATAWRITE_CFM_T *prim)
{
    uint8 index = BmmGetIndexFromRfcommConId(prim->conn_id);
    BmmRfcommConnInst *conn;

    if (index == BMM_SOCK_MAX_CONN)
    {
        BMM_WARNING("RFC_DATAWRITE_CFM Ignored");
        return;
    }

    conn = gBmm.socket[index].proto.rfcomm;

    BMM_DEBUG("RFC_DATAWRITE_CFM status 0x%x count %d",prim->status, conn->txCount);

    if (prim->status != RFC_DATAWRITE_PENDING)
    {
        if (conn->txCount == BMM_RFCOMM_TX_WINDOW_SIZE)
        {
            BmmResultCode status = BMM_RESULT_SUCCESS;

            if (gBmm.socket[index].state == BMM_SOCKET_STATE_IN_ERROR)
            {
                status = BMM_RESULT_CONNID_IN_ERROR;
            }
            BmmSocketDataCfmSend(conn->txPendingHandle,
                                 gBmm.socket[index].connId,
                                 conn->txPendingContext, status);
            conn->txPendingHandle = 0;
            conn->txPendingContext = 0;
        }

        if (conn->txCount > 0)
            conn->txCount--;
    }
}


void BmmRfcommDataWriteReqHandler(BmmSocketDataReq * prim, BmmRfcommConnInst *conn)
{
    bool tx = TRUE;

    conn->txCount++;

    if (conn->txCount < BMM_RFCOMM_TX_WINDOW_SIZE)
    {
        /* Immediate ack */
        BmmSocketDataCfmSend(prim->pHandle, prim->connId, 
                             prim->context, BMM_RESULT_SUCCESS);
    }
    else if(conn->txCount == BMM_RFCOMM_TX_WINDOW_SIZE)
    {
        /* Buffer full, no ack. Store context number */
        conn->txPendingContext = prim->context;
        conn->txPendingHandle  = prim->pHandle;
    }
    else
    {
        /* Buffer overflow - discard */
        conn->txCount--;

        tx = FALSE;
        BmmSocketDataCfmSend(prim->pHandle, prim->connId, 
                             prim->context, BMM_RESULT_FLOW_CONTROL_VIOLATED);
    }

    if (tx)
    {
        CsrMblk *mblk = CsrMblkDataCreate(prim->data, prim->dataLen, TRUE);

        rfc_datawrite_req(conn->rfcConnId, prim->dataLen, mblk, 0);
        prim->data = NULL;
    }
}


void BmmRfcommClearRxQueue(BmmRfcommConnInst *conn)
{
    uint16 eventClass;
    void *msg;

    while ((conn->rxQueueCount > 0) &&
           CsrMessageQueuePop(&conn->rxQueue, &eventClass, &msg))
    {
        RFC_DATAREAD_IND_T *prim = (RFC_DATAREAD_IND_T*)msg;
        
        conn->rxQueueCount--;
        CsrPmemFree(prim);
    }
}

void BmmRfcommRestoreRxQueue(BmmRfcommConnInst *conn)
{
    uint16 eventClass;
    void *msg;

    while ((conn->rxAppCredits > 0) &&
           (conn->rxQueueCount > 0) &&
           CsrMessageQueuePop(&conn->rxQueue, &eventClass, &msg))
    {
        RFC_DATAREAD_IND_T *prim = (RFC_DATAREAD_IND_T*)msg;
        uint8 index = BmmGetIndexFromRfcommConId(prim->conn_id);
        
        if ((index < BMM_SOCK_MAX_CONN) &&
            (prim->payload != NULL)     &&
            (gBmm.socket[index].state == BMM_SOCKET_STATE_CONNECTED))
        {
            conn->rxAppCredits--;
            conn->rxQueueCount--;

            BmmSocketDataIndSend(gBmm.socket[index].dataHandle, 
                                 gBmm.socket[index].connId,
                                 CsrMblkGetLength((CsrMblk *)prim->payload),
                                 CsrBtMblkConsumeToMemory((CsrMblk **) &prim->payload),
                                 index);
        }
        rfc_dataread_rsp(prim->conn_id);
        CsrPmemFree(prim);
    }
}

void BmmRfcommDataReadIndHandler(RFC_DATAREAD_IND_T **pprim)
{
    RFC_DATAREAD_IND_T *prim = *pprim;
    uint8 index = BmmGetIndexFromRfcommConId(prim->conn_id);
    BmmRfcommConnInst *conn;

    if ((index == BMM_SOCK_MAX_CONN) ||
        (prim->payload == NULL)      ||
        (gBmm.socket[index].state != BMM_SOCKET_STATE_CONNECTED))
    {
        BMM_WARNING("RFC_DATAREAD_IND Ignored");
        return;
    }

    conn = gBmm.socket[index].proto.rfcomm;

    if ((conn->rxAppCredits > 0) && (conn->rxQueue == NULL))
    {
        /* Pass to app as we have credits */
        conn->rxAppCredits--;

        BmmSocketDataIndSend(gBmm.socket[index].dataHandle,
                             gBmm.socket[index].connId,
                             CsrMblkGetLength((CsrMblk *)prim->payload),
                             CsrBtMblkConsumeToMemory((CsrMblk **) &prim->payload),
                             index);
        rfc_dataread_rsp(prim->conn_id);
        prim->payload = NULL;
    }
    else
    {
        /* Application already processing a data indication. Store on
         * queue and do some clever accounting */
        conn->rxQueueCount++;

        CsrMessageQueuePush(&conn->rxQueue, RFCOMM_PRIM, prim);

        if (conn->rxAppCredits > 0)
        {
            BmmRfcommRestoreRxQueue(conn);
        }
        *pprim = NULL;
    }
}

void BmmRfcommDisconnectIndHandler(RFC_DISCONNECT_IND_T *prim)
{
    uint8 index = BmmGetIndexFromRfcommConId(prim->conn_id);

    if ((index != BMM_SOCK_MAX_CONN) && 
        (gBmm.socket[index].state == BMM_SOCKET_STATE_CONNECTED) &&
        (prim->reason == RFC_INVALID_PAYLOAD))
    {
        BmmRfcommConnInst *conn = gBmm.socket[index].proto.rfcomm;
        gBmm.socket[index].state = BMM_SOCKET_STATE_IN_ERROR;

        BmmSocketErrorSend(conn->ctx.pHandle, 
                           gBmm.socket[index].connId, BMM_MTU_VIOLATION);

    }
}

void BmmLecocDataWriteReqHandler(BmmSocketDataReq * prim, BmmLecocConnInst *conn)
{
    bool tx = TRUE;

    conn->txCount++;

    if (conn->txCount < BMM_LECOC_TX_WINDOW_SIZE)
    {
        /* Immediate ack */
        BmmSocketDataCfmSend(prim->pHandle, prim->connId,
                             prim->context, BMM_RESULT_SUCCESS);
    }
    else if(conn->txCount == BMM_LECOC_TX_WINDOW_SIZE)
    {
        /* Buffer full, no ack. Store context number */
        conn->txPendingContext = prim->context;
        conn->txPendingHandle  = prim->pHandle;
    }
    else
    {
        /* Buffer overflow - discard */
        conn->txCount--;

        tx = FALSE;
        BmmSocketDataCfmSend(prim->pHandle, prim->connId,
                             prim->context, BMM_RESULT_FLOW_CONTROL_VIOLATED);
    }

    if (tx)
    {
        CsrMblk *mblk = CsrMblkDataCreate(prim->data, prim->dataLen, TRUE);

        conn->txSegInfo[conn->enqueueIdx].pduSize = prim->dataLen;
        conn->txSegInfo[conn->enqueueIdx].lengthCfd = 0;
        conn->enqueueIdx = (conn->enqueueIdx + 1) % BMM_LECOC_TX_WINDOW_SIZE;

        /* Pass to L2CAP. L2CAP has it's own tx queue so there is no
         * need to have one here too */
        L2CA_DataWriteReqEx(conn->cid, 0, mblk, 
                            BMM_EXTRACT_INDEX_FROM_CONNID(prim->connId));

        prim->data = NULL;
    }
}

void BmmLecocDataWriteCfmHandler(L2CA_DATAWRITE_CFM_T *prim)
{
    BmmLecocConnInst *conn;
    uint8 index = prim->req_ctx;

    BMM_DEBUG("L2CA_DATAWRITE_CFM Result 0x%x", prim->result);

    if ((index >= BMM_SOCK_MAX_CONN) ||
        !gBmm.socket[index].inUse ||
        (gBmm.socket[index].connId != prim->con_ctx) ||
        (gBmm.socket[index].proto.lecoc == NULL))
    {
        BMM_WARNING("L2CA_DATAWRITE_CFM Ignored");
        return;
    }

    conn = gBmm.socket[index].proto.lecoc;

    /* With Segmentation support enabled for LECOC in Bluestack, 
     * it will send separate confirmation messages for every segment transmitted
     * BMM is supposed to decrement the Tx count only after it has received the 
     * confirmation messages for the whole SDU.
     */
    conn->txSegInfo[conn->dequeueIdx].lengthCfd += prim->length;

    if (conn->txSegInfo[conn->dequeueIdx].lengthCfd == 
        conn->txSegInfo[conn->dequeueIdx].pduSize)
    {
        conn->dequeueIdx = (conn->dequeueIdx + 1) % BMM_LECOC_TX_WINDOW_SIZE;
    }
    else
    {
        return;
    }

    /* We can not really do anything about failed datawrites as we
     * have already passed up the CFM to the application. In case
     * of a link loss, the app is sure to know anyway... */

    if (conn->txCount >= BMM_LECOC_TX_WINDOW_SIZE)
    {
        conn->txCount--;
        /* Buffer was full, so this will be the unlocker. Send up
         * stored context */
        if (prim->result != L2CA_DATA_LOCAL_ABORTED)
        {
            BmmResultCode status = BMM_RESULT_SUCCESS;

            if (gBmm.socket[index].state == BMM_SOCKET_STATE_IN_ERROR)
            {
                status = BMM_RESULT_CONNID_IN_ERROR;
            }
            BmmSocketDataCfmSend(conn->txPendingHandle,
                                 gBmm.socket[index].connId, 
                                 conn->txPendingContext, status);
            conn->txPendingHandle = 0;
            conn->txPendingContext = 0;
        }
    }
    else
    {
        /* Buffer wasn't full. Ack already sent */
        conn->txCount--;
    }
}

void BmmLecocClearRxQueue(BmmLecocConnInst *conn)
{
    uint16 eventClass;
    void *msg;

    BMM_INFO("BmmLecocClearRxQueue Count %d", conn->rxQueueCount);

    while ((conn->rxQueueCount > 0) &&
           CsrMessageQueuePop(&conn->rxQueue, &eventClass, &msg))
    {
        L2CA_DATAREAD_IND_T *prim = (L2CA_DATAREAD_IND_T*)msg;

        conn->rxQueueCount--;
        CsrPmemFree(prim);
    }

    CsrMblkDestroy(conn->rxCombine);
    conn->rxCombine = NULL;
}


void BmmLecocRestoreRxQueue(BmmLecocConnInst *conn)
{
    uint16 eventClass;
    void *msg;

    while ((conn->rxAppCredits > 0) &&
           (conn->rxQueueCount > 0) &&
           CsrMessageQueuePop(&conn->rxQueue, &eventClass, &msg))
    {
        L2CA_DATAREAD_IND_T *prim = (L2CA_DATAREAD_IND_T*)msg;
        BmmConnId connId = (BmmConnId)prim->con_ctx;
        uint8 index = BMM_EXTRACT_INDEX_FROM_CONNID(connId);

        if (BmmIsConnIdValid(connId) && 
            (index < BMM_SOCK_MAX_CONN) &&
            gBmm.socket[index].state == BMM_SOCKET_STATE_CONNECTED)
        {
            conn->rxAppCredits--;
            conn->rxQueueCount--;

            BmmSocketDataIndSend(gBmm.socket[index].dataHandle, 
                                 gBmm.socket[index].connId,
                                 CsrMblkGetLength((CsrMblk *)prim->data),
                                 CsrBtMblkConsumeToMemory((CsrMblk **) &prim->data),
                                 index);
        }
        L2CA_DataReadRsp(prim->cid, prim->packets);

        CsrPmemFree(prim);
    }
}


bool BmmLecocDataReadCombine(BmmLecocConnInst *conn, L2CA_DATAREAD_IND_T *prim)
{
    /* Always join new fragment to end */
    if((prim->result == L2CA_DATA_PARTIAL) ||
       (prim->result == L2CA_DATA_PARTIAL_END))
    {
        conn->rxCombine = CsrMblkAddTail(conn->rxCombine, prim->data);
        
        if(prim->result == L2CA_DATA_PARTIAL_END)
        {
            /* Fully resegmented. Pass data in to normal handler */
            prim->result = L2CA_DATA_SUCCESS;
            prim->data = conn->rxCombine;
            conn->rxCombine = NULL;
            return TRUE;
        }
        else
        {
            /* Still incomplete */
            prim->data = NULL;
        }
    }
    else
    {
        if (conn->rxCombine != NULL)
        {
            CsrMblkDestroy(conn->rxCombine);
            conn->rxCombine = NULL;
        }
    }

    /* Good case bailed out early */
    return FALSE;
}

void BmmLecocDataReadIndHandler(L2CA_DATAREAD_IND_T **pprim)
{
    L2CA_DATAREAD_IND_T *prim = *pprim;
    BmmConnId connId = (BmmConnId)prim->con_ctx;
    uint8 index = BMM_EXTRACT_INDEX_FROM_CONNID(connId);
    BmmLecocConnInst *conn;

    if (!BmmIsConnIdValid(connId) || (index >= BMM_SOCK_MAX_CONN))
    {
        L2CA_DataReadRsp(prim->cid, prim->packets);
        return;
    }

    if (gBmm.socket[index].protocol != BMM_LECOC_PROTOCOL ||
        gBmm.socket[index].state != BMM_SOCKET_STATE_CONNECTED)
    {
        L2CA_DataReadRsp(prim->cid, prim->packets);
        return;
    }

    conn = gBmm.socket[index].proto.lecoc;

    if (!BmmLecocDataReadCombine(conn, prim))
    {
        /* Partial packet not complete, so ack the fragment we received */
        L2CA_DataReadRsp(prim->cid, prim->packets);
        return;
    }

    if ((conn->rxAppCredits > 0) && (conn->rxQueue == NULL))
    {
        /* Pass to app as we have credits */
        conn->rxAppCredits--;

        BmmSocketDataIndSend(gBmm.socket[index].dataHandle, 
                             gBmm.socket[index].connId,
                             CsrMblkGetLength((CsrMblk *)prim->data),
                             CsrBtMblkConsumeToMemory((CsrMblk **) &prim->data),
                             index);
        prim->data = NULL;

        /* Ack data as rx queue is empty */
        L2CA_DataReadRsp(prim->cid, prim->packets);
    }
    else
    {
        /* Application already processing a data indication. Store on
         * queue and do some clever accounting */
        conn->rxQueueCount++;

        CsrMessageQueuePush(&conn->rxQueue, L2CAP_PRIM, prim);

        if (conn->rxAppCredits > 0)
        {
            BmmLecocRestoreRxQueue(conn);
        }
        *pprim = NULL;
    }
}

void BmmLecocAddCreditCfmHandler(L2CA_ADD_CREDIT_CFM_T *prim)
{
    BMM_INFO("L2CA_ADD_CREDIT_CFM  ConnId 0x%x Result 0x%x", 
            prim->context, prim->result);
}

void BmmLecocDisconnectIndHandler(L2CA_DISCONNECT_IND_T *prim)
{
    BmmConnId connId = (BmmConnId)prim->con_ctx;
    uint8 index = BMM_EXTRACT_INDEX_FROM_CONNID(connId);
    BmmErrorCode error;

    BMM_INFO("L2CA_DISCONNECT_IND ConnId 0x%x Reason 0x%x", 
            prim->con_ctx, prim->reason);

    if (prim->reason == L2CA_DISCONNECT_MPS_VIOLATION)
        error = BMM_MPS_VIOLATION;
    else if (prim->reason == L2CA_DISCONNECT_MTU_VIOLATION)
        error = BMM_MTU_VIOLATION;
    else if ((prim->reason == L2CA_DISCONNECT_SAR_VIOLATION) ||
             (prim->reason == L2CA_DISCONNECT_FLOW_VIOLATION))
        error = BMM_PROTOCOL_VIOLATION;     
    else
        error = BMM_INT_ERROR;

    if (BmmIsConnIdValid(connId) && 
       (gBmm.socket[index].protocol == BMM_LECOC_PROTOCOL) &&
       (gBmm.socket[index].state == BMM_SOCKET_STATE_CONNECTED) &&
       (error != BMM_INT_ERROR))
    {
        BmmLecocConnInst *conn = gBmm.socket[index].proto.lecoc;
        
        gBmm.socket[index].state = BMM_SOCKET_STATE_IN_ERROR;
        BmmSocketErrorSend(conn->ctx.pHandle, connId, error);
    }
}


void BmmSocketDataRspHandler(BmmSocketDataRsp * prim)
{
    uint8 index = prim->context;

    if ((index >= BMM_SOCK_MAX_CONN) ||
        (!gBmm.socket[index].inUse) ||
        (gBmm.socket[index].connId != prim->connId))
    {
        BMM_INFO("BMM_SOCKET_DATA_RSP Ignored");
        return;
    }

    BMM_DEBUG("==> BMM_SOCKET_DATA_RSP Rcvd %d", gBmm.socket[index].protocol);

    if (gBmm.socket[index].protocol == BMM_LECOC_PROTOCOL)
    {
        BmmLecocConnInst *conn = gBmm.socket[index].proto.lecoc;

        conn->rxAppCredits++;
        BmmLecocRestoreRxQueue(conn);
    }
    else if (gBmm.socket[index].protocol == BMM_RFCOMM_PROTOCOL)
    {
        BmmRfcommConnInst *conn = gBmm.socket[index].proto.rfcomm;

        conn->rxAppCredits++;
        BmmRfcommRestoreRxQueue(conn);
    }
}

BmmResultCode BmmValidateSocketDataReq(BmmSocketDataReq * prim)
{
    uint8 index = BMM_EXTRACT_INDEX_FROM_CONNID(prim->connId);
    BmmProtocol proto = BMM_EXTRACT_PROTO_FROM_CONNID(prim->connId);
    BmmL2capMtu remoteMtu = 0;

    if (!BmmIsConnIdValid(prim->connId))
    {
        return BMM_RESULT_INVALID_CONNID;
    }

    if (gBmm.socket[index].state == BMM_SOCKET_STATE_IN_ERROR)
    {
        return BMM_RESULT_CONNID_IN_ERROR;
    }

    if (proto == BMM_RFCOMM_PROTOCOL)
    {
        remoteMtu = gBmm.socket[index].proto.rfcomm->ctx.maxFrameSize;
    }
    else if (proto == BMM_LECOC_PROTOCOL)
    {
        remoteMtu = gBmm.socket[index].proto.lecoc->ctx.remoteMtu;
    }

    if (dm_acl_find_by_handle(gBmm.socket[index].hciHandle) == NULL)
    {
        return BMM_RESULT_ACL_DOES_NOT_EXIST;
    }

    if (prim->dataLen > remoteMtu)
    {
        return BMM_RESULT_MTU_VIOLATION;
    }
    return BMM_RESULT_SUCCESS;
}

void BmmSocketDataReqHandler(BmmSocketDataReq * prim)
{
    BmmResultCode status = BmmValidateSocketDataReq(prim);
    uint8 index = BMM_EXTRACT_INDEX_FROM_CONNID(prim->connId);
    BmmProtocol proto = BMM_EXTRACT_PROTO_FROM_CONNID(prim->connId);

    BMM_DEBUG("==> BMM_SOCKET_DATA_REQ Rcvd 0x%x", prim->connId);

    if (status != BMM_RESULT_SUCCESS)
    {
        BmmSocketDataCfmSend(prim->pHandle, prim->connId, prim->context, status);
    }
    else if (proto == BMM_LECOC_PROTOCOL)
    {
        BmmLecocDataWriteReqHandler(prim, gBmm.socket[index].proto.lecoc);
    }
    else if (proto == BMM_RFCOMM_PROTOCOL)
    {
        BmmRfcommDataWriteReqHandler(prim, gBmm.socket[index].proto.rfcomm);
    }
}


