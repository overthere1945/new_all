/******************************************************************************
 Copyright (c) 2025 - 2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#include "csr_bt_gatt_private.h"


#ifdef GATT_OFFLOAD
#define GATT_MIN_MTU             23

CsrUint16 offloadIdCounter = 1;

CsrBtResultCode GattOffloadGetCapabilities(GattOffloadCapabilities *cap)
{
    CsrBtResultCode result = GATT_OFFLOAD_RESULT_INVALID_PARAMETER;

    GATT_LOG_INFO("GattOffloadGetCapabilities");

    if (cap != NULL)
    {
        cap->clientSupported    = TRUE;
        cap->serverSupported    = TRUE;
        cap->clientProperties   = GATT_OFFLOAD_CLIENT_PROPERTIES;
        cap->serverProperties   = GATT_OFFLOAD_SERVER_PROPERTIES;

        result = GATT_OFFLOAD_RESULT_SUCCESS;
    }
    return result;
}

void GattOffloadConnInfoReqSend(CsrBtGattId gattId,
                                CsrUint32 context,
                                CsrUint16 aclHandle,
                                CsrUint16 mtu)
{
    GattOffloadConnInfoReq *prim = zpnew(GattOffloadConnInfoReq);

    GATT_LOG_INFO("GATT_OFFLOAD_CONN_INFO_REQ GattId 0x%x context 0x%x ACL hdl 0x%x MTU %d",
        gattId, context, aclHandle, mtu);

    prim->type        = GATT_OFFLOAD_CONN_INFO_REQ;
    prim->gattId      = gattId;
    prim->context     = context;
    prim->aclHandle   = aclHandle;
    prim->mtu         = mtu;

    CsrBtGattMsgTransport(prim);
}

void GattOffloadEnableServiceReqSend(CsrBtGattId gattId,
                                     CsrUint32 context,
                                     CsrBtConnId btConnId,
                                     GattOffloadService * service)
{
    GattOffloadEnableServiceReq *prim = zpnew(GattOffloadEnableServiceReq);

    GATT_LOG_INFO("GATT_OFFLOAD_ENABLE_SERVICE_REQ GattId 0x%x context 0x%x ConnId 0x%x",
        gattId, context, btConnId);

    if (service != NULL)
    {
        CsrUint16 i;

        GATT_LOG_DEBUG("offloadGattId 0x%x role %d offloadId 0x%x numChar %d",
            service->gattId, service->role, service->offloadId, service->numChar);

        for (i=0; i < service->numChar; i++)
        {
            GATT_LOG_DEBUG("Char Handle 0x%x", service->charList[i].handle);
        }
    }

    prim->type        = GATT_OFFLOAD_ENABLE_SERVICE_REQ;
    prim->gattId      = gattId;
    prim->context     = context;
    prim->btConnId    = btConnId;
    prim->service     = service;

    CsrBtGattMsgTransport(prim);
}

void GattOffloadDisableServiceReqSend(CsrBtGattId gattId,
                                      CsrUint32 context,
                                      CsrBtConnId btConnId,
                                      CsrUint16 numOffloadIds,
                                      GattOffloadId offloadIds[])
{
    GattOffloadDisableServiceReq *prim = zpnew(GattOffloadDisableServiceReq);
    CsrUint8 i;

    GATT_LOG_INFO("GATT_OFFLOAD_DISABLE_SERVICE_REQ GattId 0x%x context 0x%x connId 0x%x Num %d",
        gattId, context, btConnId, numOffloadIds);

    prim->type          = GATT_OFFLOAD_DISABLE_SERVICE_REQ;
    prim->gattId        = gattId;
    prim->context       = context;
    prim->btConnId      = btConnId;
    prim->numOffloadIds = numOffloadIds;

    for (i = 0; i < CSRMIN(GATT_OFFLOAD_MAX_ID, numOffloadIds) ; i++)
    {
        prim->offloadId[i] = offloadIds[i];        
        GATT_LOG_DEBUG("Offload Id 0x%x", offloadIds[i]);
    }

    CsrBtGattMsgTransport(prim);
}

void GattOffloadConnInfoCfmSend(GattOffloadConnInfoReq *prim,
                                      CsrBtResultCode resultCode,
                                      CsrBtConnId     btConnId,
                                      CsrBtTypedAddr *address)
{
    GattOffloadConnInfoCfm *msg = zpnew(GattOffloadConnInfoCfm);

    msg->type           = GATT_OFFLOAD_CONN_INFO_CFM;
    msg->gattId         = prim->gattId;
    msg->resultCode     = resultCode;
    msg->resultSupplier = CSR_BT_SUPPLIER_GATT_OFFLOAD;
    msg->context        = prim->context;
    msg->aclHandle      = prim->aclHandle;
    msg->btConnId       = btConnId;
    msg->address        = *address;

    GATT_LOG_INFO("GATT_OFFLOAD_CONN_INFO_CFM gattId 0x%x connId 0x%x context 0x%x result 0x%x",
         prim->gattId, btConnId, prim->context, resultCode);
    GATT_LOG_DEBUG("Addr %x %x %x",
         msg->address.addr.lap, msg->address.addr.uap, msg->address.addr.nap);

    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
}

void GattOffloadEnableServicesCfmSend(GattOffloadEnableServiceReq *prim,
                                      CsrBtResultCode resultCode,
                                      GattOffloadId offloadId)
{
    GattOffloadEnableServiceCfm *msg = zpnew(GattOffloadEnableServiceCfm);

    msg->type           = GATT_OFFLOAD_ENABLE_SERVICE_CFM;
    msg->gattId         = prim->gattId;
    msg->resultCode     = resultCode;
    msg->resultSupplier = CSR_BT_SUPPLIER_GATT_OFFLOAD;
    msg->context        = prim->context;
    msg->btConnId       = prim->btConnId;
    msg->offloadId      = offloadId;

    GATT_LOG_INFO("GATT_OFFLOAD_ENABLE_SERVICE_CFM GattId 0x%x btConnId 0x%x result 0x%x offloadId 0x%x",
      prim->gattId, msg->btConnId, resultCode, offloadId);

    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
}

void GattOffloadDisableServiceCfmSend(GattOffloadDisableServiceReq *prim,
                                      CsrBtResultCode resultCode)
{
    GattOffloadDisableServiceCfm *msg = zpnew(GattOffloadDisableServiceCfm);

    msg->type           = GATT_OFFLOAD_DISABLE_SERVICE_CFM;
    msg->gattId         = prim->gattId;
    msg->resultCode     = resultCode;
    msg->resultSupplier = CSR_BT_SUPPLIER_GATT_OFFLOAD;
    msg->context        = prim->context;
    msg->btConnId       = prim->btConnId;

    GATT_LOG_INFO("GATT_OFFLOAD_DISABLE_SERVICE_CFM gattId 0x%x connId 0x%x context 0x%x result 0x%x",
      prim->gattId, prim->btConnId, prim->context, resultCode);

    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
}

void GattOffloadErrorIndSend(CsrBtGattId gattId,
                             CsrBtConnId btConnId,
                             GattOffloadErrCode errorCode,
                             CsrUint16 cid)
{
    GattOffloadErrorInd *msg = zpnew(GattOffloadErrorInd);
    
    msg->type       = GATT_OFFLOAD_ERROR_IND;
    msg->gattId     = gattId;
    msg->btConnId   = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->cid        = cid;
    msg->errCode    = errorCode;

    GATT_LOG_INFO("GATT_OFFLOAD_ERROR_IND gattId 0x%x connId 0x%x cid 0x%x err 0x%x",
         gattId, msg->btConnId, cid, errorCode);

    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

CsrBtResultCode GattOffloadMapHciArbErrorCode(CsrBtResultCode hciResult)
{
    CsrBtResultCode gattResult;

    switch (hciResult)
    {
        case HCI_ARB_RESULT_SUCCESS:
            gattResult = GATT_OFFLOAD_RESULT_SUCCESS;
        break;

        case HCI_ARB_RESULT_INVALID_OPERATION:
            gattResult = GATT_OFFLOAD_RESULT_INVALID_OPERATION;
        break;

        case HCI_ARB_RESULT_ACL_DOES_NOT_EXIST:
            gattResult = GATT_OFFLOAD_RESULT_ACL_DOES_NOT_EXIST;
        break;

        default:
            gattResult = GATT_OFFLOAD_RESULT_INVALID_PARAMETER;
        break;
    }
    return gattResult;
}

void GattOffloadInitSvcList(CsrCmnListElm_t *elem)
{
    /* Initialise a GattOffloadSvcElement. This function is called
     * every time a new entry is made on the access indication queue list */
    GattOffloadSvcElement *element = (GattOffloadSvcElement *) elem;

    GATT_LOG_DEBUG("GattOffloadInitSvcList");

    element->gattId     = CSR_BT_GATT_INVALID_GATT_ID;
    element->offloadId  = GATT_OFFLOAD_ID_INVALID;
    element->filterId   = HCI_ARB_INVALID_FILTER_ID;
    element->service    = NULL;
}

void GattOffloadFreeSvcList(CsrCmnListElm_t *elem)
{
    /* CsrPmemFree local pointers in the GattOffloadSvcElement.  This
     * function is called every time a element is removed from the
     * queue list */
    GattOffloadSvcElement *element = (GattOffloadSvcElement *) elem;

    if (element == NULL)
        return;

    if (element->service != NULL)
    {
        GATT_LOG_INFO("GattOffloadFreeSvcList 0x%x", element->offloadId);
    
        CsrPmemFree(element->service);
        element->service = NULL;
    }

    if (element->filterId != HCI_ARB_INVALID_FILTER_ID)
    {
        HciArbDisableFilter(element->filterId);
    }
}

CsrBool GattOffloadSvcFindByOffloadId(CsrCmnListElm_t *elem, void *value)
{
    GattOffloadSvcElement *svc = (GattOffloadSvcElement *)elem;
    GattOffloadId offloadId    = *(GattOffloadId *) value;

    return (offloadId == svc->offloadId);
}

CsrBool GattOffloadCliSvcFindByOffloadId(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattClientService *element = (CsrBtGattClientService *)elem;
    GattOffloadId offloadId = *(GattOffloadId *)value;
    return ((element->offloadId == offloadId) ? TRUE : FALSE);
}

CsrBool GattOffloadAppInstFindByOffloadId(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattAppElement *element = (CsrBtGattAppElement *)elem;
    GattOffloadId offloadId = *(GattOffloadId *)value;

    return (element->offloadId == offloadId) ? TRUE : FALSE;
}

CsrBool GattOffloadConnInstFindByAclHandle(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *element = (CsrBtGattConnElement *)elem;
    CsrUint16 aclHandle = *(CsrUint16 *)value;

    return (element->aclHandle == aclHandle) ? TRUE : FALSE;
}

GattOffloadId GattOffloadGenerateOffloadId(CsrUint16 aclHandle)
{
    GattOffloadId offloadId = (aclHandle << 16 | offloadIdCounter);
    offloadIdCounter = (offloadIdCounter + 1) % 0xFFFF;
    GATT_LOG_INFO("Generated GATT OffloadId 0x%x", offloadId);
    return offloadId;
}

void GattOffloadSendErrorIndToSubscribedApp(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn = value;
    CsrBtGattAppElement *appElement = (CsrBtGattAppElement *)elem;

    if (CSR_MASK_IS_SET(appElement->eventMask, 
                        GATT_EVENT_MASK_SUBSCRIBE_OFFLOAD_EVENTS))
    {
        GattOffloadErrorIndSend(appElement->gattId,
                                conn->btConnId,
                                conn->errorCode,
                                L2CA_CID_ATTRIBUTE_PROTOCOL);
    }
}

void GattOffloadSendErrorEventToApps(GattMainInst *inst, 
                                     CsrBtGattConnElement *conn)
{
    CSR_BT_GATT_APP_INST_ITERATE(inst->appInst,
                                 GattOffloadSendErrorIndToSubscribedApp,
                                 conn);
}

CsrBtResultCode GattOffloadValidateServiceReq(GattMainInst *inst)
{
    CsrBtResultCode result = GATT_OFFLOAD_RESULT_SUCCESS;
    GattOffloadEnableServiceReq *prim = inst->msg;

    if ((prim->service == NULL) || (prim->service->numChar == 0))
    {
        result = GATT_OFFLOAD_RESULT_INVALID_PARAMETER;
    }
    else if ((prim->service->role != CSR_BT_GATT_ROLE_SERVER) &&
             (prim->service->role != CSR_BT_GATT_ROLE_CLIENT))
    {
        result = GATT_OFFLOAD_RESULT_INVALID_PARAMETER;
    }
    else if (CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                &prim->service->gattId) == NULL)
    {
        result = GATT_OFFLOAD_RESULT_INVALID_GATTID;
    }
    else
    {
        CsrBtGattConnElement *conn;
        GattOffloadId  offloadId = prim->service->offloadId;

        conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst,
                                                               &prim->btConnId);
        if (conn == NULL)
        {
            result = GATT_OFFLOAD_RESULT_INVALID_CONNID;   
        }
        else if ((offloadId != GATT_OFFLOAD_ID_INVALID) &&
            (GATT_OFFLOAD_SVC_FIND_OFFLOADID(conn->offloadSvcList, &offloadId) == NULL))
        {
            result = GATT_OFFLOAD_RESULT_INVALID_OFFLOADID;   
        }
    }
    return result;
}

CsrBtGattConnElement * GattOffloadUpdateConn(GattMainInst *inst,
                                             CsrBtTypedAddr *addr,
                                             l2ca_cid_t cid,
                                             CsrUint16 mtu, 
                                             CsrUint16 aclHandle)
{
    CsrBtConnId btConnId = CSR_BT_CONN_ID_INVALID;
    CsrBtGattConnElement *conn;

    conn = GATT_OFFLOAD_CONN_INST_FIND_FROM_ACLHANDLE(inst->connInst, &aclHandle);
    if (conn != NULL)
    {
        GATT_LOG_DEBUG("GATT conn exists");
    }
    else if (CsrBtGattFindFreeConnId(inst, &btConnId) == TRUE)
    {
        GATT_LOG_INFO("GATT conn created");
        conn = CSR_BT_GATT_CONN_INST_ADD_LAST(inst->connInst);
        conn->btConnId    = btConnId;
        conn->peerAddr    = *addr;
        conn->cid         = cid;
        conn->leConnection= TRUE;
        conn->mtu         = mtu;
        conn->aclHandle   = aclHandle;
        conn->l2capFlags  = L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_MASTER_DIRECTED);
        conn->errorCode   = GATT_OFFLOAD_ERROR_INVALID;

        /* Store the fixed channel CID in Eatt list in the end and 
         * mark the channel is available */
        conn->cidSuccess[EATT_BEARER_COUNT]  = cid;
        conn->numOfBearer[EATT_BEARER_COUNT] = CHANNEL_IS_FREE;
    }
    else
    {
        CSR_LOG_TEXT_CRITICAL((GATT_OFFLOAD, 0, "No free connId"));
    }
    return conn;
}

/* App1 registering for 10 - 100 handle and App2 for 30-40 
   Need to find the App2 if the handle is 35 */ 
CsrBtGattAppElement * GattOffloadFindAppElement(GattMainInst *inst,
                                                l2ca_cid_t cid,
                                                CsrBtGattHandle handle)
{
    CsrBtGattHandle start=0;
    CsrBtGattConnElement *conn;
    GattOffloadSvcElement *ele;
    CsrBtGattAppElement *appElement;
    CsrBtGattId gattId = CSR_BT_GATT_INVALID_GATT_ID;

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &cid);

    if (conn == NULL)
    {
        return NULL;
    }

    ele = (GattOffloadSvcElement *)CsrCmnListGetFirst(&conn->offloadSvcList);

    for ( ; ele != NULL ; ele = ele->next)
    {
        GattOffloadService *service = ele->service;

        if ((service->role == CSR_BT_GATT_ROLE_SERVER) &&
            (service->charList[0].handle <= handle) &&
            (service->charList[service->numChar-1].handle >= handle))
        {
            if (service->charList[0].handle > start)
            {
                start  = service->charList[0].handle;
                gattId = service->gattId;
            }
        }
    }

    appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &gattId);
    return appElement;
}


CsrBtGattConnElement * GattOffloadPopulateContext(GattMainInst *inst)
{
    l2ca_cid_t cid;
    CsrBtGattConnElement *conn;
    GattOffloadConnInfoReq *prim = inst->msg;
    CsrUint16 aclHandle = prim->aclHandle;
    CsrBtTypedAddr addr = HciArbGetBtAddrFromHandle(aclHandle);

    GATT_LOG_DEBUG("Populate context Addr 0x%x 0x%x 0x%x",addr.addr.lap, addr.addr.uap, addr.addr.nap);

    conn = GATT_OFFLOAD_CONN_INST_FIND_FROM_ACLHANDLE(inst->connInst, &aclHandle);
    if (conn == NULL)
    {
        cid = L2CAOFFLOAD_AttConn(aclHandle, prim->mtu, 0);
        GATT_LOG_DEBUG("CID 0x%x ACL Hdl 0x%x", cid, aclHandle);

        att_offload_conn(&addr, cid, prim->mtu);

        conn = GattOffloadUpdateConn(inst, &addr, cid, prim->mtu, aclHandle);
    }
    else
    {
        GATT_LOG_DEBUG("GATT conn already exists");
    }
    return conn;
}

void GattOffloadEnableClientService(GattMainInst *inst,
                                    GattOffloadService *service,
                                    CsrBtGattConnElement *conn,
                                    GattOffloadId offloadId)
{
    CsrBtGattHandle start, end;
    CsrBtGattClientService *cliSvcElem;

    if (conn == NULL)
        return;

    start = service->charList[0].handle;
    end = service->charList[service->numChar-1].handle;

    cliSvcElem = GATT_OFFLOAD_CLIENT_SERVICE_LIST_FIND_BY_OFFLOADID(
                            conn->cliServiceList, &offloadId);

    if (cliSvcElem == NULL)
    {
        cliSvcElem = CSR_BT_GATT_CLIENT_SERVICE_LIST_ADD_LAST(conn->cliServiceList);
        GATT_LOG_DEBUG("GATT Cli Service Element added");
    }
    cliSvcElem->start     = start;
    cliSvcElem->end       = end;
    cliSvcElem->gattId    = service->gattId;
    cliSvcElem->offloadId = offloadId;
}

void GattOffloadEnableServerService(GattMainInst *inst, 
                                    GattOffloadService *svc,
                                    CsrBtGattConnElement *conn)
{
    GattOffloadSvcElement *ele;
    CsrBtGattHandle start, end;
    CsrBtGattAppElement *appElement;

    start = svc->charList[0].handle;
    end = svc->charList[svc->numChar-1].handle;

    appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &svc->gattId);

    ele = (GattOffloadSvcElement *)CsrCmnListGetFirst(&conn->offloadSvcList);

    for ( ; ele != NULL ; ele = ele->next)
    {
        GattOffloadService *service = ele->service;

        if ((service->role == CSR_BT_GATT_ROLE_SERVER) && 
            (service->gattId == svc->gattId))
        {
            if (service->charList[0].handle < start)
            {
                start = service->charList[0].handle;
            }
            if (service->charList[service->numChar-1].handle > end)
            {
                end = service->charList[service->numChar-1].handle;
            }
        }
    }
    appElement->start = start;
    appElement->end = end;

    CSR_BT_GATT_APP_INST_SORT_BY_ATTR_VALUE(inst->appInst);
}

CsrBtResultCode GattOffloadEnableArbiterFilter(GattMainInst *inst,
                                               CsrBtGattConnElement *conn,
                                               HciArbFilterId *filterId)
{
    CsrUint16 i;
    HciArbGattCtx *ctx;
    CsrBtResultCode hciResult;
    GattOffloadSvcElement *svc; 
    GattOffloadEnableServiceReq *prim = inst->msg;
    GattOffloadService *service = prim->service;

    ctx = CsrPmemZalloc(sizeof(*ctx) + (service->numChar*sizeof(CsrBtGattHandle)));
    for (i=0; i < service->numChar ; i++)
    {
        ctx->attrHandle[i] = service->charList[i].handle;
    }
    ctx->numHandles = service->numChar;
    ctx->role       = service->role;

    svc = GATT_OFFLOAD_SVC_FIND_OFFLOADID(conn->offloadSvcList, &service->offloadId);

    if ((svc != NULL) && (svc->filterId != HCI_ARB_INVALID_FILTER_ID))
    {
        HciArbDisableFilter(svc->filterId);
    }
    hciResult = HciArbEnableGattFilter(conn->aclHandle, ctx, filterId);

    return GattOffloadMapHciArbErrorCode(hciResult);
}


CsrBool GattOffloadConnInfoReqHandler(GattMainInst *inst)
{
    CsrBtTypedAddr addr;
    CsrBtResultCode result;
    GattOffloadConnInfoReq *prim = inst->msg;
    CsrBtConnId btConnId = CSR_BT_CONN_ID_INVALID;

    CsrBtAddrZero(&addr);

    if (CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &prim->gattId) == NULL)
    {
        return FALSE;
    }

    if (prim->mtu < GATT_MIN_MTU)
    {
        result = GATT_OFFLOAD_RESULT_INVALID_PARAMETER;
    }
    else
    {
        CsrBtResultCode hciResult = HciArbValidateConnReq(prim->aclHandle);
        result = GattOffloadMapHciArbErrorCode(hciResult);
    }

    if (result == GATT_OFFLOAD_RESULT_SUCCESS)
    {
        CsrBtGattConnElement *conn = GattOffloadPopulateContext(inst);

        if ((conn == NULL) || (conn->mtu != prim->mtu))
        {
            result = GATT_OFFLOAD_RESULT_INVALID_OPERATION;
        }
        else
        {
            btConnId = conn->btConnId;
            CsrBtAddrCopy(&addr, &(conn->peerAddr));
        }
    }

    GattOffloadConnInfoCfmSend(prim, result, btConnId, &addr);
    return TRUE;
}

void GattOffloadEnableServicesCfm(GattMainInst *inst, 
                                  GattOffloadEnableServiceReq *prim,
                                  CsrBtGattQueueElement *element,                                  
                                  CsrBtResultCode result,
                                  GattOffloadId offloadId)
{
    CsrBtGattConnElement *conn;

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst,
                                                           &prim->btConnId);

    GattOffloadEnableServicesCfmSend(prim, result, offloadId);

    if (conn != NULL)
        CsrBtGattEattResetCid(conn, conn->cid, CSR_BT_GATT_RESULT_SUCCESS);

    /* This procedure has finished. Start the next if any */
    CsrBtGattQueueRestoreHandler(inst, element);
}

void GattOffloadEnableServiceRestoreHandler(GattMainInst *inst, 
                                            CsrBtGattQueueElement *element, 
                                            CsrUint16 mtu)
{
    CsrBtGattConnElement *conn = NULL;
    GattOffloadSvcElement *svc = NULL;
    GattOffloadEnableServiceReq *prim = element->gattMsg;
    HciArbFilterId filterId = HCI_ARB_INVALID_FILTER_ID;
    CsrBtResultCode result = GATT_OFFLOAD_RESULT_INVALID_CONNID;
    GattOffloadId offloadId = (prim->service != NULL) ? 
                              prim->service->offloadId : GATT_OFFLOAD_ID_INVALID;

    GATT_LOG_INFO("Gatt Enable Service Restore ");

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst,
                                                           &prim->btConnId);

    result = GattOffloadValidateServiceReq(inst);
    if ((result != GATT_OFFLOAD_RESULT_SUCCESS) ||
        (prim->service == NULL) ||
        (conn == NULL ) ||
        (mtu == 0))
    {
        GattOffloadEnableServicesCfm(inst, prim, element, result, offloadId);
        return;
    }

    result = GattOffloadEnableArbiterFilter(inst, conn, &filterId);
    if (result != GATT_OFFLOAD_RESULT_SUCCESS)
    {    
        GattOffloadEnableServicesCfm(inst, prim, element, result, offloadId);
        return;
    }

    if (offloadId == GATT_OFFLOAD_ID_INVALID)
    {
        svc = GATT_OFFLOAD_SVC_ADD_LAST(conn->offloadSvcList);
        svc->gattId    = prim->service->gattId;
        svc->offloadId = GattOffloadGenerateOffloadId(conn->aclHandle);
        offloadId      = svc->offloadId;
    }
    else
    {
        svc = GATT_OFFLOAD_SVC_FIND_OFFLOADID(conn->offloadSvcList, &offloadId);
        CsrPmemFree(svc->service);
    }
    svc->filterId = filterId;
    svc->service  = prim->service;

    if (prim->service->role == CSR_BT_GATT_ROLE_CLIENT)
    {
        GattOffloadEnableClientService(inst, prim->service, conn, offloadId);
    }
    else
    {
        GattOffloadEnableServerService(inst, prim->service, conn);
    }

    prim->service = NULL;
    GattOffloadEnableServicesCfm(inst, prim, element, result, offloadId);
}

CsrBool GattOffloadEnableServiceReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    GattOffloadEnableServiceReq *prim = inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, 
                                                             &prim->btConnId));
    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg, 
                                 prim->btConnId, 
                                 prim->gattId,
                                 GattOffloadEnableServiceRestoreHandler,
                                 NULL,
                                 NULL);

    inst->msg = NULL;
    return tmp;
}


void GattOffloadDisableServiceRestoreHandler(GattMainInst *inst, 
                                             CsrBtGattQueueElement *element, 
                                             CsrUint16 mtu)
{
    CsrBtResultCode resultCode = GATT_OFFLOAD_RESULT_SUCCESS;
    GattOffloadDisableServiceReq *prim = element->gattMsg;
    CsrBtGattConnElement *conn;

    GATT_LOG_INFO("Gatt Disable Service Restore ");

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst,
                                                           &prim->btConnId);

    if (prim->numOffloadIds >= GATT_OFFLOAD_MAX_ID)
    {
        resultCode = GATT_OFFLOAD_RESULT_INVALID_PARAMETER;
    }
    else if (conn == NULL)
    {
        GATT_LOG_INFO("Gatt conn not present 0x%x ", prim->btConnId);
        resultCode = GATT_OFFLOAD_RESULT_INVALID_CONNID;
    }
    else if (mtu == 0)
    {
        resultCode = GATT_OFFLOAD_RESULT_INVALID_CONNID;
    }
    else if (prim->numOffloadIds == 0)
    {
        CsrCmnListDeinit(&conn->cliServiceList);
        CsrCmnListDeinit(&conn->offloadSvcList);
        conn->errorCode = GATT_OFFLOAD_ERROR_INVALID;
    }
    else
    {
        CsrUint16 i;
        GattOffloadSvcElement *svc;
        CsrBtGattClientService *cliSvcElem;

        for (i=0; i< prim->numOffloadIds; i++)
        {
            svc = GATT_OFFLOAD_SVC_FIND_OFFLOADID(conn->offloadSvcList, 
                                                  &prim->offloadId[i]);
            if (svc != NULL)
            {
                CsrCmnListElementRemove(&conn->offloadSvcList, (CsrCmnListElm_t*)svc);
            }
            cliSvcElem = GATT_OFFLOAD_CLIENT_SERVICE_LIST_FIND_BY_OFFLOADID(
                            conn->cliServiceList, &prim->offloadId[i]);
            if (cliSvcElem != NULL)
            {
                GATT_LOG_INFO("cliServiceList removed 0x%x", prim->offloadId[i]);
                CsrCmnListElementRemove(&conn->cliServiceList, (CsrCmnListElm_t*)cliSvcElem);
            }
        }
    }

    GattOffloadDisableServiceCfmSend(prim, resultCode);

    /* This procedure has finished. Start the next if any */
    if (conn)
        CsrBtGattEattResetCid(conn, conn->cid, CSR_BT_GATT_RESULT_SUCCESS);
    CsrBtGattQueueRestoreHandler(inst, element);
}

CsrBool GattOffloadDisableServiceReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    GattOffloadDisableServiceReq *prim = inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, 
                                                             &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg, 
                                 prim->btConnId, 
                                 prim->gattId,
                                 GattOffloadDisableServiceRestoreHandler,
                                 NULL,
                                 NULL);

    inst->msg = NULL;
    return tmp;
}


void GattOffloadAttErrorIndHandler(GattMainInst *inst)
{
    ATT_ERROR_IND_T *prim = (ATT_ERROR_IND_T *) inst->msg;
    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst,
                                                                          &(prim->cid));

    GATT_LOG_DEBUG("Error Ind CID 0x%x error 0x%x",prim->cid, prim->error);

    if (conn && (conn->cid == prim->cid))
    {
        conn->errorCode = prim->error;
        GattOffloadSendErrorEventToApps(inst, conn);
    }
}

void CsrBtGattAttHandleValueIndHandler(GattMainInst *inst)
{
    ATT_HANDLE_VALUE_IND_T *prim = (ATT_HANDLE_VALUE_IND_T *) inst->msg;
    CsrBtGattConnElement   *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst,
                                                                            &(prim->cid));

    CsrBtGattClientService *ele;
    CsrBtGattAppElement *appInst;

    GATT_LOG_DEBUG("Handle Value Ind CID 0x%x Handle 0x%x ", prim->cid, prim->handle);

    if (conn == NULL)
    {
        GATT_LOG_DEBUG("Handle Value Ind Conn not present");
        return;
    }

    ele = (CsrBtGattClientService *)CsrCmnListGetFirst(&conn->cliServiceList);

    for ( ; ele != NULL ; ele = ele->next)
    {
        if ((prim->handle >= ele->start) && (prim->handle <= ele->end))
        {
            appInst = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &ele->gattId);
            if (appInst)
            {
                CsrUint8 *data = (prim->size_value != 0) ?
                                 CsrPmemAlloc(prim->size_value) : NULL;
                SynMemCpyS(data, prim->size_value, prim->value, prim->size_value);
                CsrBtGattNotificationIndicationIndSend(inst,
                                                   prim->cid,
                                                   prim->flags,
                                                   prim->size_value,
                                                   data,
                                                   prim->handle,
                                                   ele->gattId,
                                                   conn->btConnId);
            }
        }
    }

    if (CSR_MASK_IS_SET(prim->flags, ATT_HANDLE_VALUE_INDICATION))
    {
        attlib_handle_value_rsp(CSR_BT_GATT_IFACEQUEUE, prim->cid, NULL);
    }
}

CsrBool GattOffloadRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattConnElement *conn;
    CsrBool process = TRUE;
    CsrBtGattPrim *type = element->gattMsg;    

    if (mtu == 0)
    {
        GATT_LOG_DEBUG("Offload Restore handler mtu %d", mtu);
        return process;
    }
    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst, &element->btConnId);

    GATT_LOG_DEBUG("Offload Restore handler mtu %x cid %x element->btConnId %x conn %x", 
                  mtu, element->cid, element->btConnId, conn);

    if (conn && conn->errorCode != GATT_OFFLOAD_ERROR_INVALID)
    {
        CsrUint8 *tmp = NULL;

        if (*type == CSR_BT_GATT_READ_REQ)
        {
            process = FALSE;
            CsrBtGattReadCfmHandler((CsrBtGattReadReq *)element->gattMsg,
                                    CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                    CSR_BT_SUPPLIER_GATT,
                                    0, &tmp);
        }
        else if (*type == CSR_BT_GATT_WRITE_REQ)
        {
            process = FALSE;
            CsrBtGattWriteCfmSend((CsrBtGattPrim *)element->gattMsg,
                                  CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                  CSR_BT_SUPPLIER_GATT);
        }
        else if (*type == CSR_BT_GATT_EVENT_SEND_REQ)
        {
            process = FALSE;

            CsrBtGattEventSendCfmSend((CsrBtGattEventSendReq*)element->gattMsg,
                                      CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                      CSR_BT_SUPPLIER_GATT);
        }

        if (!process)
        {
            CsrBtGattEattResetCid(conn, element->cid, CSR_BT_GATT_RESULT_SUCCESS);

            /* This procedure has finished. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, element);
        }
    }
    return process;
}

void CsrBtGattAttRegisterCfmHandler(GattMainInst *inst)
{
    ATT_REGISTER_CFM_T *prim = (ATT_REGISTER_CFM_T *) inst->msg;

    if (prim->result == ATT_RESULT_SUCCESS)
    {
        /* Ready to work with ATT */
        CsrBtConnId btConnId = CSR_BT_GATT_LOCAL_BT_CONN_ID;
        CsrBtGattQueueElement *qElem = CsrBtGattfindQueueElementbtConnId(inst, btConnId);
        CsrBtGattQueueRestoreHandler(inst, qElem);
    }
    else
    {
        CSR_LOG_TEXT_CRITICAL((CsrBtGattLto, 0, "AttRegisterCfm Failed"));
    }
}

#endif

