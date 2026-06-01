/******************************************************************************
 Copyright (c) 2010-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #8 $
******************************************************************************/

#include "csr_bt_gatt_private.h"

#if (defined(CSR_LOG_ENABLE) && defined(LE_AUDIO_ENABLE))
#include "lea_logging.h"
#endif

#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)
static const CsrCharString *subOrigins[] =
{
    "MESSAGE_QUEUE",
};
#endif /* CSR_LOG_ENABLE */

CsrBtGattAppElement * CsrBtGattAccessIndGetParams(GattMainInst      *inst, 
                                                  l2ca_cid_t        cid,
                                                  CsrUint16         handle,
                                                  CsrBtConnId       *btConnId,
                                                  CsrBtGattConnInfo *connInfo,
                                                  CsrUint16         *mtu,
                                                  CsrBtTypedAddr    *address)
{
#ifndef GATT_OFFLOAD 
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_ATTR_HANDLE(inst->appInst, &handle);
#else 
    CsrBtGattAppElement *appElement = GattOffloadFindAppElement(inst, cid, handle);
#endif

    if (appElement)
    {
        if (cid == ATT_CID_LOCAL)
        { /* The local device is reading from or writing from its own database */ 
            *btConnId = CSR_BT_GATT_LOCAL_BT_CONN_ID;
            *mtu      = CSR_BT_GATT_LOCAL_MAX_MTU;
            *connInfo = CSR_BT_GATT_CONNINFO_LE;
            CsrBtAddrZero(address);
            return appElement;
        }
        else
        {
            CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &cid);

            if (conn)
            { /* A connection is established */
                *btConnId = conn->btConnId;
                *mtu      = conn->mtu;
                *address  = conn->peerAddr;
                *connInfo = (CsrBtGattConnInfo)(L2CA_CONFLAG_IS_LE(conn->l2capFlags) ? CSR_BT_GATT_CONNINFO_LE : CSR_BT_GATT_CONNINFO_BREDR);
                return appElement;
            }
            /* Else - No valid connection found. */
        }
    }
    return NULL;
}

void CsrBtGattGetAttUuid(CsrBtUuid       uuid,
                         CsrUint32       *attUuid,
                         att_uuid_type_t *uuidType)
{
    if (uuid.length == CSR_BT_UUID16_SIZE)
    {
        *uuidType    = ATT_UUID16;
        attUuid[0]  = (CsrUint32)((uuid.uuid[1] << 8) | uuid.uuid[0]);
        attUuid[1]  = 0;
        attUuid[2]  = 0;
        attUuid[3]  = 0;
    }
    else
    {
        *uuidType   = ATT_UUID128;
        attUuid[0] = (CsrUint32)((uuid.uuid[15] << 24) | (uuid.uuid[14] << 16) | (uuid.uuid[13] << 8) | uuid.uuid[12]);
        attUuid[1] = (CsrUint32)((uuid.uuid[11] << 24) | (uuid.uuid[10] << 16) | (uuid.uuid[9] << 8) | uuid.uuid[8]);
        attUuid[2] = (CsrUint32)((uuid.uuid[7] << 24) | (uuid.uuid[6] << 16) | (uuid.uuid[5] << 8) | uuid.uuid[4]);
        attUuid[3] = (CsrUint32)((uuid.uuid[3] << 24) | (uuid.uuid[2] << 16) | (uuid.uuid[1] << 8) | uuid.uuid[0]);
    }
}


/* --------------------------------------------------------------------
  Helper function used to control the Prepare List
   -------------------------------------------------------------------- */
static void csrBtGattFreePrepare(CsrCmnListElm_t *elem)
{
    CsrBtGattPrepareBuffer *prep = (CsrBtGattPrepareBuffer *) elem;
    CsrPmemFree(prep->data);
}

#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
static void csrBtGattFreeLongReadBuffer(CsrCmnListElm_t *elem)
{
    CsrBtGattLongAttrReadBuffer *prep = (CsrBtGattLongAttrReadBuffer *) elem;
    CsrPmemFree(prep->data);
}
#endif

CsrBool  CsrBtGattPrepareInstMultipleAttrHandles(CsrCmnListElm_t *elem, void *value)
{ /* Find out if multiple attribute handle values is written in a single operation
     or not. */
    CsrBtGattPrepareBuffer *buf    = (CsrBtGattPrepareBuffer *) elem;
    CsrBtGattPrepareMultiElem *ids = (CsrBtGattPrepareMultiElem *) value;
    return ((buf->cid == ids->cid && buf->handle != ids->attrHandle) ? TRUE : FALSE);
}

CsrBool CsrBtGattPrepareInstFindCidIdleState(CsrCmnListElm_t *elem, void *value)
{ /* Find Prepare Write Elements from cid and state CSR_BT_GATT_PREPEXEC_IDLE */
    CsrBtGattPrepareBuffer *buf = (CsrBtGattPrepareBuffer *) elem;
    l2ca_cid_t cid              = *(l2ca_cid_t *) value;    
    return ((buf->cid == cid && buf->state == CSR_BT_GATT_PREPEXEC_IDLE) ? TRUE : FALSE);
}

CsrBool CsrBtGattPrepareInstFindCidPendingState(CsrCmnListElm_t *elem, void *value)
{ /* Find Prepare Write Elements from cid and state CSR_BT_GATT_PREPEXEC_PENDING */
    CsrBtGattPrepareBuffer *buf = (CsrBtGattPrepareBuffer *) elem;
    l2ca_cid_t cid              = *(l2ca_cid_t *) value;    
    return ((buf->cid == cid && buf->state == CSR_BT_GATT_PREPEXEC_PENDING) ? TRUE : FALSE);
}

CsrBool CsrBtGattPrepareInstCleanUp(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattPrepareBuffer *buf    = (CsrBtGattPrepareBuffer *) elem;
    CsrBtGattPrepareCleanElem *ids = (CsrBtGattPrepareCleanElem *) value;
    
    if (buf->cid == ids->cid)
    { /* Found an element which match the cid. Return TRUE
         to remove this element */
        if (buf->state == CSR_BT_GATT_PREPEXEC_PENDING ||
            buf->state == CSR_BT_GATT_PREPEXEC_DONE)
        { /* An CSR_BT_GATT_DB_ACCESS_WRITE_IND message has already been 
             sent to the application. GATT need to sent a 
             CSR_BT_GATT_DB_ACCESS_COMPLETE_IND to the application. 
             Note GATT can only have one element per handle when it is in one 
             of these states, please see CsrBtGattPrepareInstGetAttrList */
            CsrUint16           mtu;
            CsrBtTypedAddr      address;
            CsrBtGattAppElement *app;
            CsrBtGattConnInfo   connInfo = CSR_BT_GATT_CONNINFO_LE;
            CsrBtConnId         btConnId = CSR_BT_CONN_ID_INVALID;
            CsrBtAddrZero(&(address));

            app = CsrBtGattAccessIndGetParams(ids->inst, buf->cid, buf->handle, 
                                              &btConnId, &connInfo, &mtu, &address);

            if (app)
            {
                CsrBtGattDbAccessCompleteIndSend(app->gattId, btConnId, connInfo,
                                                address, 
                                                CsrBtGattResolveServerHandle(app->start, buf->handle),
                                                ids->commit);
            }
            CSR_UNUSED(mtu);
        }
        return TRUE;
    }
    return FALSE;    
}

CsrBool CsrBtGattPrepareInstGetAttrList(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattPrepareBuffer *buf   = (CsrBtGattPrepareBuffer *) elem;
    CsrBtGattPrepareAttrElem *ids = (CsrBtGattPrepareAttrElem *) value;

    if (buf->cid == ids->cid && buf->handle == ids->attrHandle)
    {
        CsrBtGattAttrWritePairs *unit = &(ids->unit[ids->count]);

        if (unit->value != NULL)
        {
            if ((unit->valueLength + unit->offset) == buf->offset &&
                ((CsrUint32) unit->valueLength + buf->dataLength) <= L2CA_FLOWSPEC_MAX_SDU)
            {
                CsrUint16 length = (CsrUint16)(unit->valueLength + buf->dataLength);
                CsrUint8  *tmp = (CsrUint8 *) CsrPmemAlloc(length);                
                SynMemCpyS(tmp, length, unit->value, unit->valueLength);
                SynMemCpyS(&(tmp[unit->valueLength]), buf->dataLength, buf->data, buf->dataLength);
                CsrPmemFree(unit->value);
                unit->value       = tmp;
                unit->valueLength = length;
                return TRUE;
            }
            else
            {
                ids->count++;
                unit = &(ids->unit[ids->count]);
            }
        }   
        unit->attrHandle  = buf->handle;
        unit->offset      = buf->offset;
        unit->valueLength = buf->dataLength;
        unit->value       = buf->data;
        buf->data         = NULL;
        ids->unitCount++;
        *(CsrBtGattPrepareAttrElem*)value = *ids;
        
        if (ids->check == CSR_BT_GATT_ACCESS_CHECK_RELIABLE_WRITE && ids->count == 0)
        { /* GATT need to keep one element of each handle in order to be able to
             sent the CSR_BT_GATT_DB_ACCESS_COMPLETE_IND message. Note is is only
             necessary if if multiple attribute handle values is written in a 
             single operation. E.g. a Reliable Write has been Request with multiple
             handles */
            buf->state = CSR_BT_GATT_PREPEXEC_PENDING;
            return FALSE;    
        }
        return TRUE;
    }
    return FALSE;
}

CsrInt32 CsrBtGattPrepareInstSortByOffset(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2)
{ /* Sort so smallest offset is placed first */
    CsrBtGattPrepareBuffer *elemA = (CsrBtGattPrepareBuffer *) elem1;
    CsrBtGattPrepareBuffer *elemB = (CsrBtGattPrepareBuffer *) elem2;

    if (elemA->offset < elemB->offset)
    {
        return -1;
    }
    else if (elemA->offset > elemB->offset)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

#ifdef CSR_BT_GATT_INSTALL_SERVER_LONG_WRITE_OFFSET
void CsrBtGattPrepareInstLongWriteList(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattPrepareBuffer *buf   = (CsrBtGattPrepareBuffer *) elem;
    CsrBtGattPrepareAttrElem *ids = (CsrBtGattPrepareAttrElem *) value;
    CsrBtGattAttrWritePairs *unit = &(ids->unit[ids->count]);
    if (buf->cid == ids->cid && buf->handle == ids->attrHandle)
    {
        unit->attrHandle  = buf->handle;
        unit->offset      = buf->offset;
        unit->valueLength = buf->dataLength;
        unit->value       = buf->data;
        buf->state        = CSR_BT_GATT_PREPEXEC_LONG_WRITE_PENDING;
        buf->data         = NULL;

        /* Increment the next location to save second array of pointer information */
        ids->count++;
        /* Increment the number of write unit count */
        ids->unitCount++;
        *(CsrBtGattPrepareAttrElem*)value = *ids;
    }
    /* Else - Case should not come as long write new method is only supported for long write 
       not for Reliable write */
}
#endif

#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
CsrBool CsrBtGattPrepareInstLongReadList(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattLongAttrReadBuffer   *buf   = (CsrBtGattLongAttrReadBuffer *) elem;
    CsrBtGattLongReadAttrElem     *ids   = (CsrBtGattLongReadAttrElem *) value;
    CsrBtGattLongAttrRead         *unit  = &(ids->readUnit[ids->count]);

    unit->offset      = buf->offset;
    unit->valueLength = buf->dataLength;
    unit->value       = buf->data;
    buf->data         = NULL;

    /* Increment the next location to save second array of pointer information */
    ids->count++;
    /* Increment the number of Read unit count */
    ids->unitCount++;
    *(CsrBtGattLongReadAttrElem*)value = *ids;
    return TRUE;
}
#endif

/* --------------------------------------------------------------------
  Helper function used to control the application List
   -------------------------------------------------------------------- */
CsrBool CsrBtGattAppInstFindGattId(CsrCmnListElm_t *elem, void *value)
{ /* Returns TRUE if appInst "elem" has given gattId "value" */
    CsrBtGattAppElement *element = (CsrBtGattAppElement *)elem;
    CsrBtGattId         gattId   = *(CsrBtGattId *) value;
    return (element->gattId == gattId) ? TRUE : FALSE;
}

CsrBool CsrBtGattAppInstFindAttrHandle(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattAppElement *element = (CsrBtGattAppElement *)elem;
    CsrUint16           handle   = *(CsrUint16 *)value;

    if (handle >= element->start && handle <= element->end)
    {
        return TRUE;
    }
    return FALSE;
}

CsrInt32 CsrBtGattAppInstSortByAttributeValue(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2)
{
    /* Sort attr start handles so smallest value is placed first */
    CsrBtGattAppElement *elemA = (CsrBtGattAppElement *)elem1;
    CsrBtGattAppElement *elemB = (CsrBtGattAppElement *)elem2;

    if (elemA->start < elemB->start)
    {
        return -1;
    }
    else if (elemA->start > elemB->start)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void csrBtGattInitAppInst(CsrCmnListElm_t *elem)
{
    /* Initialise a CsrBtGattAppElement. This function is called every
     * time a new entry is added to the app list */
    CsrBtGattAppElement *app = (CsrBtGattAppElement*) elem;

    app->gattId      = CSR_BT_GATT_INVALID_GATT_ID;
    /* app->qid         = CSR_SCHED_QID_INVALID; */
    app->start       = 0;
    app->end         = 0;
    app->eventMask   = CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_NONE;
    app->priority    = CSR_BT_APP_PRIORITY_LOW;
    app->flags       = CSR_BT_GATT_FLAGS_NONE;
}

/* ------------------------------------------------------------------------------
  Helper function used to control the Pending Access Indication Command Queue List
   -------------------------------------------------------------------- --------*/
static void csrBtGattInitAccessIndQueueInst(CsrCmnListElm_t *elem)
{
    /* Initialise a CsrBtGattAcessIndQueueElement. This function is called
     * every time a new entry is made on the access indication queue list */
    CsrBtGattAccessIndQueueElement *element = (CsrBtGattAccessIndQueueElement *) elem;
    element->cid = 0;
    element->gattMsg = NULL;
    element->restoreFunc = NULL;
    element->btConnId   = 0;
}

static void csrBtGattFreeAccessIndQueueInst(CsrCmnListElm_t *elem)
{
    /* CsrPmemFree local pointers in the CsrBtGattAcessIndQueueElement.  This
     * function is called every time a element is removed from the
     * queue list */
    CsrBtGattAccessIndQueueElement *element = (CsrBtGattAccessIndQueueElement *) elem;

    if (element->msgState == CSR_BT_GATT_MSG_QUEUE_QUEUED)
    { /* Message is being removed without being processed */
        CSR_BT_GATT_QUEUE_LOG_REMOVE(element->gattMsg);
    }
}

/* --------------------------------------------------------------------
  Helper function used to control the Connection List
   -------------------------------------------------------------------- */
static void csrBtGattInitConnInst(CsrCmnListElm_t *elem)
{
    /* Initialise a CsrBtGattConnElement. This function is called
     * every time a new entry is made on the conn list */
    CsrBtGattConnElement *element = (CsrBtGattConnElement *) elem;
    CsrUint8 i;

    CsrBtAddrZero(&(element->peerAddr));
    element->l2capFlags = 0;
    element->btConnId = CSR_BT_CONN_ID_INVALID;
    element->cid = L2CA_CID_INVALID;
    element->gattId = CSR_BT_GATT_INVALID_GATT_ID;
    element->mtu = 0;
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
    element->remoteName         = NULL;
    element->remoteNameLength   = 0;
#endif
    element->reasonCode           = CSR_BT_GATT_RESULT_SUCCESS;
    element->supplier             = CSR_BT_SUPPLIER_GATT;
#ifdef CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
    CsrCmnListInit(&element->cliServiceList, 0, NULL, NULL);
#endif
    CsrCmnListInit(&element->accessIndQueue, 0, csrBtGattInitAccessIndQueueInst, csrBtGattFreeAccessIndQueueInst);
#ifdef CSR_BT_GATT_INSTALL_EATT
    element->eattConnection = FALSE;
    element->localInitiated = LOCAL_EATT_IDLE;
    element->numCidSucess   = 0x0;
#endif /* CSR_BT_GATT_INSTALL_EATT */
    CsrMemSet(&(element->cidSuccess), 0, (sizeof(CsrUint16)*(EATT_BEARER_COUNT + ATT_BEARER)));

    for (i = 0; i < EATT_BEARER_COUNT + ATT_BEARER; i++)
    {   
        /* Mark all the CID invalid and mark it as available once EATT connection is sucessfully completed */
        element->numOfBearer[i] = CHANNEL_IS_INVALID;
    }
    element->leConnection            = TRUE;
#ifdef GATT_CACHING_CLIENT_ROLE
    element->pendingHash             = FALSE;

#if (GATT_SERVICE_CHANGED_CCCD == GATT_CCCD_NTF_IND_DISABLE)
    element->serviceChangedIndState = SERVICE_CHANGED_INDICATION_DISABLED;
#else
    element->serviceChangedIndState = SERVICE_CHANGED_INDICATION_IDLE;
#endif

#endif /* GATT_CACHING_CLIENT_ROLE */
    element->serviceChangeHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID;
#ifdef GATT_OFFLOAD
    CsrCmnListInit(&element->offloadSvcList, 0, GattOffloadInitSvcList, GattOffloadFreeSvcList);
#endif
}

static void csrBtGattFreeConnInst(CsrCmnListElm_t *elem)
{
    /* CsrPmemFree local pointers in the CsrBtGattConnElement. This
     * function is called every time an element is removed from the
     * conn list */
    CsrBtGattConnElement *element = (CsrBtGattConnElement *) elem;
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
    CsrPmemFree(element->remoteName);
#endif

#ifdef CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
    CsrCmnListDeinit(&element->cliServiceList);
#endif

#ifdef GATT_OFFLOAD
    CsrCmnListDeinit(&element->offloadSvcList);
#endif

#ifdef CSR_BT_GATT_INSTALL_EATT
    CsrCmnListDeinit(&element->accessIndQueue);
#endif /* CSR_BT_GATT_INSTALL_EATT */
}

static bool gattConnInstAddressMatches(CsrBtGattConnElement* conn, const CsrBtTypedAddr *peerAddr)
{
    bool addrMatches = FALSE;
    if (CsrBtAddrEq(&(conn->peerAddr), peerAddr))
    {
        addrMatches = TRUE;
    }
#ifndef EXCLUDE_CSR_BT_CM_MODULE    
    else
    {
        if (!CsrBtBdAddrEqZero(&conn->idAddr.addr) && CsrBtAddrEq(&(conn->idAddr), peerAddr))
        {
            addrMatches = TRUE;
        }
    }
#endif    
    return addrMatches;
}

CsrBool CsrBtGattConnInstFindBtconnIdFromIdMask(CsrCmnListElm_t *elem, void *value)
{
    CsrBtConnId btConnId       = *(CsrBtConnId *) value;
    CsrBtGattConnElement *conn = (CsrBtGattConnElement *) elem;

    if ((conn->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK) == btConnId)
    {
        *(CsrBtConnId*)value = conn->btConnId;
        return TRUE;
    }
    return FALSE;
}

CsrBool CsrBtGattConnInstFindConnectedBtConnId(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) value;
    return ((conn->btConnId == btConnId &&
            CSR_BT_GATT_CONN_IS_CONNECTED(conn->state)) ? TRUE : FALSE);
}

CsrBool CsrBtGattConnInstFindConnectedCid(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement *)elem;
    l2ca_cid_t cid             = *(l2ca_cid_t *) value;
    bool found                 = FALSE;
    CsrUint8 i;

    if (CSR_BT_GATT_CONN_IS_CONNECTED(conn->state))
    {
        for (i = 0; i < EATT_BEARER_COUNT + ATT_BEARER; i++)
        {
            CSR_LOG_TEXT_DEBUG((GATT_OFFLOAD, 0, "cid %x %x", conn->cidSuccess[i], cid));
        
            if (conn->cidSuccess[i] == cid)
            {
                found = TRUE;
                break;
            }
        }
    }
    CSR_LOG_TEXT_DEBUG((GATT_OFFLOAD, 0, "CsrBtGattConnInstFindConnectedCid %x", found));
    
    return found;
}

void CsrBtGattConnInstCopyReasonCode(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn1 = (CsrBtGattConnElement *) elem;
    CsrBtGattConnElement *conn2 = (CsrBtGattConnElement *) value;

    if (conn1->cid == conn2->cid)
    {
        conn1->reasonCode = conn2->reasonCode;
        conn1->supplier = conn2->supplier;
    }
}

CsrBool CsrBtGattConnInstFindBtCid(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement *)elem;
    l2ca_cid_t cid             = *(l2ca_cid_t *) value;
    bool found                 = FALSE;
    CsrUint8 i;

    for (i = 0; i < EATT_BEARER_COUNT + ATT_BEARER; i++)
    {
        if (conn->cidSuccess[i] == cid)
        {
            found = TRUE;
            break;
        }
    }
    return found;
}

CsrBool CsrBtGattFindConnectedConnInstFromAddress(CsrCmnListElm_t *elem, void *value)
{
     CsrBtGattConnElement *conn = (CsrBtGattConnElement *)elem;
     CsrBtTypedAddr address     = *(CsrBtTypedAddr *) value;

     return ((gattConnInstAddressMatches(conn, &address) &&
              (CSR_BT_GATT_CONN_IS_CONNECTED(conn->state))) ? TRUE : FALSE);
}

CsrBool CsrBtGattFindConnInstFromAddress(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement *)elem;
    CsrBtTypedAddr address     = *(CsrBtTypedAddr *) value;

    return (gattConnInstAddressMatches(conn, &address) ? TRUE : FALSE);
}

CsrBool CsrBtGattFindConnInstFromAddressFlags(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement *)elem;
    ATT_CONNECT_CFM_T *cfm  = (ATT_CONNECT_CFM_T *) value;
    CsrBool sameAddr;
    sameAddr = (gattConnInstAddressMatches(conn, &(cfm->addrt)));
    if (sameAddr)
    {
        if (L2CA_CONFLAG_IS_LE(cfm->flags))
        {
            if (conn->leConnection)
                return TRUE;
        }
        else
        {
            if (!(conn->leConnection))
                return TRUE;
        }
    }
    return FALSE;
}

CsrBool CsrBtGattFindConnInstFromAddressFlagsBredr(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement *)elem;
    CsrBtTypedAddr address     = *(CsrBtTypedAddr *) value;
    return (gattConnInstAddressMatches(conn, &address) && !(conn->leConnection) ? TRUE : FALSE);
}

CsrBool CsrBtGattFindConnInstFromAddressFlagsLe(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement *)elem;
    CsrBtTypedAddr address  = *(CsrBtTypedAddr *) value;;
    return (gattConnInstAddressMatches(conn, &address) && (conn->leConnection) ? TRUE : FALSE);
}

#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
void CsrBtGattConnInstUpdateRemoteName(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattConnElement *conn            = (CsrBtGattConnElement *)elem;
    CsrBtGattConnUpdateRemoteNameIds *ids = (CsrBtGattConnUpdateRemoteNameIds *) value;
    bool update                           = FALSE;
    CsrUint8 i;

    if (CSR_BT_GATT_CONN_IS_CONNECTED(conn->state))
    {
        for (i = 0; i < EATT_BEARER_COUNT + ATT_BEARER; i++)
        {
            if (conn->cidSuccess[i] == ids->cid)
            {
                update = TRUE;
                break;
            }
        }
    }

    if (update)
    {
        CsrPmemFree(conn->remoteName);
        conn->remoteName       = (CsrUint8 *) CsrPmemAlloc(ids->length);
        conn->remoteNameLength = ids->length;
        SynMemCpyS(conn->remoteName, ids->length, ids->name, ids->length);
    }
}
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

/* --------------------------------------------------------------------
  Helper function used to control the Pending Command Queue List
   -------------------------------------------------------------------- */
static void csrBtGattInitQueueInst(CsrCmnListElm_t *elem)
{
    /* Initialise a CsrBtGattQueueElement. This function is call every
     * time a new entry is made on the queue list */
    CsrBtGattQueueElement *qElem = (CsrBtGattQueueElement *) elem;
    
    /* GATT need to initialise the cid to a value that
       ATT never will use. Normally GATT will use 
       L2CA_CID_INVALID but in this case it is not 
       possible because ATT_CID_LOCAL is defined
       to L2CA_CID_INVALID as well. Instead
       we using L2CA_CID_AMP_MANAGER because this
       value never shall be used between ATT/GATT */
    qElem->cid            = L2CA_CID_AMP_MANAGER;
    qElem->btConnId       = CSR_BT_CONN_ID_INVALID;
    qElem->msgState       = CSR_BT_GATT_MSG_QUEUE_IDLE;
    qElem->dataOffset     = 0;
    qElem->dataElemIndex  = 0;    
    qElem->gattMsg        = NULL;
    qElem->data           = NULL;
    qElem->attrHandle     = CSR_BT_GATT_ATTR_HANDLE_INVALID;
    qElem->restoreFunc    = NULL;
    qElem->cancelFunc     = NULL;
    qElem->txTimer        = CSR_SCHED_TID_INVALID;
#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
    CsrCmnListInit(&qElem->longReadBuffer, 0, NULL, csrBtGattFreeLongReadBuffer);
#endif
}

static void csrBtGattFreeQueueInst(CsrCmnListElm_t *elem)
{
    /* CsrPmemFree local pointers in the CsrBtGattQueueElement.  This
     * function is called every time a element is removed from the
     * queue list */
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *) elem;

    if (element->txTimer)
    {
        CsrSchedTimerCancel(element->txTimer, NULL, NULL);
        element->txTimer = (CsrSchedTid) CSR_SCHED_TID_INVALID;
    }

    if (element->msgState == CSR_BT_GATT_MSG_QUEUE_QUEUED)
    { /* Message is being removed without being processed */
        CSR_BT_GATT_QUEUE_LOG_REMOVE(element->gattMsg);
    }

    if (element->data)
    {
        CsrPmemFree(element->data);
        element->data = NULL;
    }

    if (element->gattMsg)
    {
        CsrBtGattFreeDownstreamMessageContents(CSR_BT_GATT_PRIM, element->gattMsg);
        SynergyMessageFree(CSR_BT_GATT_PRIM, element->gattMsg);
        element->gattMsg = NULL;
    }

    /* If the current blob read message got cancelled by the app then need to clean the longReadBuffer list */
#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
    if (element->longReadBuffer.count)
    {
       CsrCmnListDeinit(&element->longReadBuffer);
    }
#endif
}

CsrBool CsrBtGattQueueFindBtConnId(CsrCmnListElm_t *elem, void *value)
{
    /* Goes though the queue list in order to find a matching
     * btConnId. If it find a match the first found
     * CsrBtGattQueueElement is returned */
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *)elem;
    CsrBtConnId           btConnId = *(CsrBtConnId *)value;
    return ((element->btConnId == btConnId &&
             btConnId != CSR_BT_CONN_ID_INVALID) ? TRUE : FALSE);
}

CsrBool CsrBtGattAccessIndQueueFindBtConnId(CsrCmnListElm_t *elem, void *value)
{
    /* Goes though the queue list in order to find a matching btConnId.
       If it finds a match the first found
       CsrBtGattQueueElement will be returned */
    CsrBtGattAccessIndQueueElement *element = (CsrBtGattAccessIndQueueElement *)elem;
    CsrBtConnId           btConnId = *(CsrBtConnId *)value;
    return ((element->btConnId == btConnId &&
             btConnId != CSR_BT_CONN_ID_INVALID) ? TRUE : FALSE);
}

CsrBool CsrBtGattQueueFindQueuedMsgBtConnIdFromQueueMask(CsrCmnListElm_t* elem, void* value)
{
    CsrBtGattQueueElement* element = (CsrBtGattQueueElement*)elem;
    CsrBtConnId           btConnId = *(CsrBtConnId*)value;

    return ((element->msgState == CSR_BT_GATT_MSG_QUEUE_QUEUED) && ((element->btConnId & CSR_BT_GATT_BT_CONN_ID_QUEUE_MASK) == (btConnId & CSR_BT_GATT_BT_CONN_ID_QUEUE_MASK) &&
        element->btConnId != CSR_BT_CONN_ID_INVALID) ? TRUE : FALSE);
}

CsrBool CsrBtGattQueueFindProgressMsgBtConnIdFromQueueMask(CsrCmnListElm_t* elem, void* value)
{
    CsrBtGattQueueElement* element = (CsrBtGattQueueElement*)elem;
    CsrBtConnId           btConnId = *(CsrBtConnId*)value;

    return ((element->msgState == CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS) && ((element->btConnId & CSR_BT_GATT_BT_CONN_ID_QUEUE_MASK) == (btConnId & CSR_BT_GATT_BT_CONN_ID_QUEUE_MASK) &&
        element->btConnId != CSR_BT_CONN_ID_INVALID) ? TRUE : FALSE);
}

CsrBool CsrBtGattQueueFindBtConnIdFromQueueMask(CsrCmnListElm_t *elem, void *value)
{ 
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *)elem;
    CsrBtConnId           btConnId = *(CsrBtConnId *)value;
    
    return (((element->btConnId & CSR_BT_GATT_BT_CONN_ID_QUEUE_MASK) == (btConnId & CSR_BT_GATT_BT_CONN_ID_QUEUE_MASK) &&
             element->btConnId != CSR_BT_CONN_ID_INVALID) ? TRUE : FALSE);
}

CsrBool CsrBtGattQueueFindCid(CsrCmnListElm_t *elem, void *value)
{
    /* Goes though the queue list in order to find a matching
     * cid. If it find a match the first found
     * CsrBtGattQueueElement is returned */
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *)elem;
    l2ca_cid_t cid = *(l2ca_cid_t *)value;
    return ((element->cid == cid &&
             element->btConnId != CSR_BT_CONN_ID_INVALID) ? TRUE : FALSE);
}

CsrBool CsrBtGattQueueFindMsgToCancel(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattQueueElement *element   = (CsrBtGattQueueElement *)elem;
    CsrBtGattFindMsgToCancelIds *ids = (CsrBtGattFindMsgToCancelIds *)value;

    return ((element->gattId   == ids->gattId &&
             element->btConnId == ids->btConnId && 
             element->cancelFunc) 
             ? TRUE : FALSE);
}

#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
CsrBool CsrBtGattQueueFindPrivateReadName(CsrCmnListElm_t *elem, void *value)
{ /* Goes though the queue list in order to find out if the message
     trigget by CsrBtGattReadByUuidPrivateHandler is still on queue */     
     
    CsrBtGattQueueElement *element      = (CsrBtGattQueueElement *)elem;
    CsrBtGattFindPrivateNameMsgIds *ids = (CsrBtGattFindPrivateNameMsgIds *)value;

    if (element->btConnId   == ids->btConnId &&
        element->gattId     == ids->gattId   &&
        element->gattMsg                     &&
        (*(CsrBtGattPrim *) element->gattMsg) == CSR_BT_GATT_READ_BY_UUID_REQ)
    {
        CsrBtGattReadByUuidReq *reqMsg = (CsrBtGattReadByUuidReq *) element->gattMsg;

        if (reqMsg->uuid.length == CSR_BT_UUID16_SIZE && 
            CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_DEVICE_NAME_CHARAC)
        { /* The Remote Name Characteristic Value is still on the Queue */
            return TRUE;
        }
    }
    return FALSE;
}
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

CsrUint16 CsrBtGattValidateBtConnIdByMtu(GattMainInst *inst,
                                         CsrBtGattId  gattId,
                                         CsrBtConnId  btConnId,
                                         l2ca_cid_t   *cid,
                                         CsrBool      cidAllocation)
{
    CSR_UNUSED(gattId);
    if (btConnId == CSR_BT_GATT_LOCAL_BT_CONN_ID)
    {
        *cid = ATT_CID_LOCAL;
        return CSR_BT_GATT_LOCAL_MAX_MTU;
    }
    else
    {
        CsrBtGattConnElement  *conn = NULL;

        conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst, 
                                                                   &btConnId);

        if (conn &&
            CSR_BT_GATT_CONN_IS_CONNECTED(conn->state))
        {
            if(cidAllocation == ALLOCATE_CID)
            {
                *cid = conn->cid;
            }
            return conn->mtu;
        }
    }
    *cid = L2CA_CID_INVALID;
    return 0;
}

void CsrBtGattQueueRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element)
{
    if (element)
    {
        CsrBtGattQueueElement *tmp;

        CsrBtConnId btConnId = element->btConnId;
        CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst, &element->btConnId); 
        CsrInt8 i;

        CSR_BT_GATT_QUEUE_REMOVE(inst->queue[inst->qid], element);

        /* If no CsrBtGattConnElement present for this btConnId and if it is not local btConnId,
         * don't iterate to find the next message in queue for this btConnId and don't call its
         * restoreFunc(). This is done to prevent call stack overflow due to recursive calling of 
         * CsrBtGattQueueRestoreHandler(). CsrBtGattAttQueueCleanupOnDisconnection() will iterate
         * for all the messages in queue for this btConnId and call their restoreFunc().
         */
        if((!conn) && (btConnId != CSR_BT_GATT_LOCAL_BT_CONN_ID))
        {
            return;
        }

        for (i = NO_OF_QUEUE - 1; i >= 0; i--)
        {
            tmp = CSR_BT_GATT_QUEUED_MSG_FIND_BT_CONN_ID_FROM_QUEUE_MASK(inst->queue[i], &btConnId);

            if (tmp && tmp->gattMsg && tmp->restoreFunc)
            {
                CsrBtGattPrim type = *(CsrBtGattPrim *)tmp->gattMsg;
                CsrBool legacyCid = FALSE;

                if (type == CSR_BT_GATT_CLIENT_EXCHANGE_MTU_REQ)
                {
                    legacyCid = TRUE;
                }

                tmp->cid = CsrBtGattfindFreeCid(inst, btConnId, legacyCid);
                if (tmp->cid != INVALID_CID)
                {
                    CsrUint16 mtu = CsrBtGattValidateBtConnIdByMtu(inst, tmp->gattId, tmp->btConnId, &tmp->cid, DONT_ALLOCATE_CID);
                    tmp->msgState = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS;
                    CSR_BT_GATT_QUEUE_LOG_RESTORE(tmp->gattMsg);
#ifdef GATT_OFFLOAD
                    if (GattOffloadRestoreHandler(inst, tmp, mtu))
                    {
                        tmp->restoreFunc(inst, tmp, mtu);
                    }
#else
                    tmp->restoreFunc(inst, tmp, mtu);
#endif
                }
            }
        }
    }
}

/* Initialisation handler */
void CsrBtGattInitHandler(GattMainInst *inst)
{
    CsrBtGattAppElement *appElement;
    CsrBtGattQueueElement *qElememt;
    CsrUint8 i;
#ifdef CSR_BT_GATT_INSTALL_EATT
    inst->preferredEattMtu = EATT_PREFERRED_MTU;
    inst->preferredGattBearer = GATT_PREFER_EATT_OVER_ATT;
#else
    inst->preferredGattBearer = GATT_PREFER_ATT_OVER_EATT;
#endif /* CSR_BT_GATT_INSTALL_EATT */

    /* Create a gattId for gatt itself */
    inst->privateGattId = CSR_BT_GATT_CREATE_GATT_ID(1, CSR_BT_GATT_IFACEQUEUE);
#ifdef CSR_BT_INSTALL_GATT_BREDR
    inst->bredrAppHandle = CSR_SCHED_QID_INVALID;
#endif

#ifndef EXCLUDE_CSR_BT_CM_MODULE
    /* Ensure a valid (empty) local name. CM will send us the correct
     * one as soon as it has been read and/or changes */
    inst->localName = (CsrUtf8String *) CsrPmemZalloc(sizeof(CsrUtf8String));
#endif
    /* Initialize the local supported features value */
    CsrMemSet(inst->localLeFeatures, 0x00, sizeof(inst->localLeFeatures));

    /* Init the queues, conn, applist and sdp list */
    for (i = 0; i < NO_OF_QUEUE ;i++)
    {
        CsrCmnListInit(&inst->queue[i], 0, csrBtGattInitQueueInst, csrBtGattFreeQueueInst);
    }

    CsrCmnListInit(&inst->connInst, 0, csrBtGattInitConnInst, csrBtGattFreeConnInst);
	CsrCmnListInit(&inst->appInst, 0, csrBtGattInitAppInst, NULL);
    CsrCmnListInit(&inst->prepare, 0, NULL, csrBtGattFreePrepare);

    /* Add an dummy element in to the queue in order to make sure that
     * the queue is block while GATT init itself */
    qElememt            = CSR_BT_GATT_QUEUE_ADD_LAST(inst->queue[0]);
    qElememt->gattId    = inst->privateGattId;
    qElememt->btConnId  = CSR_BT_GATT_LOCAL_BT_CONN_ID;
    qElememt->msgState  = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS;
    qElememt->cid       = ATT_CID_LOCAL;
    /* GATT must assign some attribute handles to itself in order to
     * add some mandatory attributes to the database. Note GATT cannot
     * make these entries before it knows the local device name*/
    appElement = CSR_BT_GATT_APP_INST_ADD_FIRST(inst->appInst);

    CSR_LOG_TEXT_REGISTER(&(CsrBtGattLto),
                          "BT_GATT",
                          CSR_ARRAY_SIZE(subOrigins),
                          subOrigins);

#if (defined(CSR_LOG_ENABLE) && defined(LE_AUDIO_ENABLE))
    LeaLoggingRegister();
#endif
    /* appElement->qid      = CSR_BT_GATT_IFACEQUEUE; */
#ifdef CSR_BT_GATT_EXCLUDE_MANDATORY_DB_REGISTRATION
    appElement->start    = CSR_BT_GATT_ATTR_HANDLE_INVALID;
    appElement->end      = CSR_BT_GATT_ATTR_HANDLE_INVALID;
#else
    appElement->start    = CSR_BT_GATT_ATTR_HANDLE_START;
    appElement->end      = CSR_BT_GATT_ATTR_HANDLE_END;
#endif

    appElement->gattId   = inst->privateGattId;

#ifdef EXCLUDE_CSR_BT_CM_MODULE
    /* Register GATT to the ATT subsystem */
    attlib_register_req(CSR_BT_GATT_IFACEQUEUE, NULL);
#else /* EXCLUDE_CSR_BT_CM_MODULE */
    {
        CsrBtCmEventMask cmEventMask = CSR_BT_GATT_DEFAULT_CM_EVENT_MASK;

        cmEventMask |= CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ADDR_MAPPED_IND |
                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SECURITY_EVENT_IND;
        CsrBtCmSetEventMaskReqSend(CSR_BT_GATT_IFACEQUEUE,
                                   cmEventMask,
                                   CSR_BT_CM_EVENT_MASK_COND_ALL);

        /* Register GATT with the CM (for report indications) */
        CsrBtCmRegisterHandlerReqSend(CSR_BT_CM_HANDLER_TYPE_LE,
                                      CSR_BT_GATT_IFACEQUEUE,
                                      0); /*flags, not used*/
    }
#endif /* !EXCLUDE_CSR_BT_CM_MODULE */

#ifdef CSR_BT_GATT_CACHING

    /*This init call is added to make sure there is a database existing for the
      queries to run as expected.*/
    CsrBtGattCachingDbInit();
#endif
}

/*******************************************************************************
 * Public Util Helpers
 *******************************************************************************/

CsrBool CsrBtGattCliSvcFindByGattId(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattClientService *element = (CsrBtGattClientService *)elem;
    CsrBtGattId gattId = *(CsrBtGattId *)value;
    return ((element->gattId == gattId) ? TRUE : FALSE);
}

CsrBool CsrBtGattCliSvcFindByHandle(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattClientService *element = (CsrBtGattClientService *)elem;
    CsrBtGattHandle handle = *(CsrBtGattHandle *)value;

    return (handle >= element->start && handle <= element->end) ? TRUE : FALSE;
}

void CsrBtGattUnregisterApp(GattMainInst *inst, CsrBtGattAppElement *app, CsrBtGattQueueElement * qElem)
{
    CSR_UNUSED(app);
    {
        /* The application is not using the advert or scan procedure. 
            E.g. the Unregister procedure is done send confirm to the application and restore the Queue */
        CsrBtGattUnregisterCfmHandler(inst, qElem, TRUE);
    }
}

#ifdef GATT_CACHING_CLIENT_ROLE
CsrBool CsrBtGattGetPeerServiceChangeHandles(CsrBtAddressType addressType,
                                             const CsrBtDeviceAddr *addr,
                                             CsrBtGattHandle *serviceChange)
{
    CsrBtTdDbGattInfo gattInfo;

    if (CsrBtTdDbGetGattInfo(addressType,
                             addr,
                             &gattInfo) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        *serviceChange = gattInfo.serviceChange;
        return TRUE;
    }
    return FALSE;
}
#endif /* GATT_CACHING_CLIENT_ROLE */

CsrBool CsrBtGattFindFreeConnId(GattMainInst *inst, CsrBtConnId *newBtConnId)
{ /* This function returns TRUE if a free btConnId is found, otherwise it return
     FALSE */   
    CsrBool freeConnId   = FALSE;
    CsrBtConnId btConnId = *newBtConnId;

    if (btConnId == CSR_BT_CONN_ID_INVALID)
    {
        CsrUint16 i;
        for (i = 1; i <= 0x0FFF; i++)
        {
            btConnId = (i << CSR_BT_GATT_BT_CONN_ID_APP_ID_SHIFT_VALUE);

            if (!CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &btConnId))
            { /* Found a free btConnId, return it */
                *newBtConnId = btConnId;
                freeConnId   = TRUE;
                break;
            }
        }
    }
    else
    { /* The btConnId has allready been set */  
        freeConnId = TRUE;
    }
    return freeConnId;
}

/* Utility function to fetch GATT's LE role in a connection*/
CsrBtGattLeRole CsrBtGattGetConnectionLeRole(l2ca_conflags_t l2capFlags)
{
    L2CA_CONNECTION_T method;
    CsrBtGattLeRole leRole = CSR_BT_GATT_LE_ROLE_UNDEFINED;

    method = CSR_BT_GATT_GET_L2CA_METHOD(l2capFlags);

    if (method == L2CA_CONNECTION_LE_MASTER_DIRECTED ||
        method == L2CA_CONNECTION_LE_MASTER_WHITELIST)
    { /* Le Role is Central */
        leRole = CSR_BT_GATT_LE_ROLE_MASTER;
    }
    else
    { /* Le Role is Peripheral */
        leRole = CSR_BT_GATT_LE_ROLE_SLAVE;
    }

    return leRole;
}

CsrUint16 CsrBtGattResolveServerHandle(CsrUint16 startHandle, CsrUint16 handle)
{
#ifdef CSR_BT_GATT_INSTALL_SERVER_HANDLE_RESOLUTION
    return ((startHandle <= handle) ? ((handle - startHandle) + 1) : 0xFFFF);
#else
    CSR_UNUSED(startHandle);
    return handle;
#endif
}

CsrUint16 CsrBtGattGetAbsoluteServerHandle(CsrUint16 startHandle, CsrUint16 handle)
{
#ifdef CSR_BT_GATT_INSTALL_SERVER_HANDLE_RESOLUTION
    return (startHandle + handle - 1);
#else
    CSR_UNUSED(startHandle);
    return handle;
#endif
}

CsrUint16 CsrBtGattGetMtuFromConnInst(CsrBtConnId btConnId)
{
    CsrBtGattConnElement *conn = NULL;

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(gattMainInstPtr->connInst, 
                                                           &btConnId);
    if (conn && CSR_BT_GATT_CONN_IS_CONNECTED(conn->state))
    {
        return conn->mtu;
    }
    return 0;
}

CsrBool CsrBtGattClientUtilFindAddrByConnIdEx(CsrBtConnId btConnId,
                                              CsrBtTypedAddr *addr)
{
    CsrBtGattConnElement *conn = NULL;
    CsrBtAddrZero(addr);

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(gattMainInstPtr->connInst, 
                                                           &btConnId);
    if (conn && CSR_BT_GATT_CONN_IS_CONNECTED(conn->state))
    {
        *addr = conn->peerAddr;
        return TRUE;
    }
    return FALSE;
}

CsrUint16 CsrBtGattClientUtilGetCidByConnId(CsrBtConnId btConnId)
{
    CsrBtGattConnElement *conn = NULL;

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(gattMainInstPtr->connInst, 
                                                           &btConnId);
    if (conn && CSR_BT_GATT_CONN_IS_CONNECTED(conn->state))
    {
        return conn->cid;
    }
    return L2CA_CID_INVALID;
}


CsrBool CsrBtGattClientUtilFindIdentityAddrByConnIdEx(CsrBtConnId btConnId,
                                              CsrBtTypedAddr *addr)
{
    CsrBtGattConnElement *conn = NULL;
    CsrBtAddrZero(addr);

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(gattMainInstPtr->connInst,
                                                           &btConnId);
    if (conn && CSR_BT_GATT_CONN_IS_CONNECTED(conn->state))
    {
#ifndef EXCLUDE_CSR_BT_CM_MODULE    
       if (!CsrBtBdAddrEqZero(&conn->idAddr.addr))
        {
            *addr = conn->idAddr;
            return TRUE;
        }
#endif       
    }
    return FALSE;
}

#ifdef INSTALL_CTM
/******************************************************************************
  GattIsClientHandleRegistered : Returns TRUE if handle is registered by application
  Application registers the handle range using CsrBtGattClientRegisterServiceReq
******************************************************************************/
CsrBool GattIsClientHandleRegistered(CsrBtTypedAddr *address, CsrUint16 handle)
{
    CsrBtGattConnElement *conn;
    CsrBtGattClientService *ele = NULL;

    conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(gattMainInstPtr->connInst,
                                                   CsrBtGattFindConnInstFromAddress,
                                                   address);
    if (conn)
    {
        ele = CSR_BT_GATT_CLIENT_SERVICE_LIST_FIND_BY_HANDLE(conn->cliServiceList, &handle);
    }

    return (ele != NULL);
}

/******************************************************************************
  GattAttConnectCfmHandler : Handler function for processing ATT Connect cfm  
******************************************************************************/
void GattAttConnectCfmHandler(ATT_CONNECT_CFM_T *cfm)
{
    CsrBtGattConnElement *conn;
    gattMainInstPtr->msg = (void *)cfm;

    conn = CSR_BT_GATT_CONN_INST_ADD_LAST(gattMainInstPtr->connInst);
    conn->peerAddr = cfm->addrt;

    CsrBtGattAttConnectCfmHandler(gattMainInstPtr);

    attlib_free((ATT_UPRIM_T *)cfm);
    gattMainInstPtr->msg = NULL;
}
#endif

#ifdef CSR_BT_GATT_INSTALL_EATT
/* Helper function to set the EATT MTU */
void GattSetEattMtu(CsrUint16 desiredEattMtu)
{
    if (desiredEattMtu < EATT_MTU_MIN)
    {
        gattMainInstPtr->preferredEattMtu = EATT_MTU_MIN;
    }
    else
    {
        gattMainInstPtr->preferredEattMtu = desiredEattMtu;
    }
}

/* Application selection for preferred GATT bearer */
void GattSelectPreferredBearer(CsrUint8 preferredGattBearer)
{
    gattMainInstPtr->preferredGattBearer = preferredGattBearer;
}
#endif /* CSR_BT_GATT_INSTALL_EATT */

/* Helper functions to get,set and reset the Eatt CID */
CsrUint16 CsrBtGattGetCid(GattMainInst *inst,CsrBtGattConnElement* conn, CsrBool legacyCid)
{
    CsrUint8 i;
    CsrUint8 totalBearer = EATT_BEARER_COUNT + ATT_BEARER;
    CsrUint8 bearerIndex;
    CsrUint16 cid = INVALID_CID;

    if (legacyCid)
    {
         if (conn->numOfBearer[EATT_BEARER_COUNT] == CHANNEL_IS_FREE)
         {
             cid = conn->cidSuccess[EATT_BEARER_COUNT];
             CSR_MASK_SET(conn->numOfBearer[EATT_BEARER_COUNT], CHANNEL_IS_BUSY);
         }
         return cid;
    }

    else
    {
        for (i=0; i < totalBearer; i++)
        {
            if (inst->preferredGattBearer == GATT_PREFER_ATT_OVER_EATT)
            {
                bearerIndex = (totalBearer -1) - i;
            }
            else if (inst->preferredGattBearer == GATT_PREFER_EATT_OVER_ATT)
            {
                bearerIndex = i;
            }
            else
            {
                /* Preferred GATT Bearer is not set right, Falling back to default (EATT is preferred over ATT) */
                bearerIndex = i;
            }
    
            if (conn->numOfBearer[bearerIndex] == CHANNEL_IS_FREE
#ifdef GATT_OFFLOAD
                || ((conn->errorCode != GATT_OFFLOAD_ERROR_INVALID) && (conn->numOfBearer[bearerIndex] == CHANNEL_IS_INVALID))
#endif
#ifdef CSR_BT_GATT_REMOTE_DB_CHANGE
            && conn->pendingHash == FALSE
#endif
               )
            {                
                cid = conn->cidSuccess[bearerIndex];
                CSR_LOG_TEXT_DEBUG((GATT_OFFLOAD, 0, "Marking channel busy %x ", cid));
                CSR_MASK_SET(conn->numOfBearer[bearerIndex], CHANNEL_IS_BUSY);
                break;
            }
        }
        return cid;
    }
}

CsrUint8 CsrBtGattEattResetCid(CsrBtGattConnElement* conn, CsrUint16 cid, CsrUint16 status)
{
    CsrUint8    cidIdentifier = 0;
    if (conn)
    {
        CsrUint8 i;
        /* Search for the given CID once match is found mark the CID as free */
        for (i = 0; i < EATT_BEARER_COUNT + ATT_BEARER; i++)
        {
            if (conn->cidSuccess[i] == cid)
            {
                /* Check for the CID status if the status is TO it means this channel got disconnected due to GATT TO
                 * And this channel need to mark it to invalid so that GATT will never use it in future for other GATT 
                 * Transaction */
#ifdef GATT_OFFLOAD
                if (status != ATT_RESULT_SUCCESS_MORE)
                {
                    cidIdentifier = i;
                    CSR_MASK_UNSET(conn->numOfBearer[i], CHANNEL_IS_BUSY);
                    CSR_LOG_TEXT_DEBUG((GATT_OFFLOAD, 0, "Unset channel busy %x ", conn->numOfBearer[i]));
                }
#else
                if (status == ATT_RESULT_TIMEOUT)
                {
                    conn->numOfBearer[i] = CHANNEL_IS_INVALID;
                }
                else if (status != ATT_RESULT_SUCCESS_MORE)
                {
                    cidIdentifier = i;
                    CSR_MASK_UNSET(conn->numOfBearer[i], CHANNEL_IS_BUSY);
                }
#endif
                break;
            }
        }
    }
    return cidIdentifier;
}

void CsrBtGattEattSetCid(CsrBtGattConnElement* conn, CsrUint16 cid)
{
    if (conn)
    {
        CsrUint8 i;
        /* Search for the given CID once match is found mark the CID as busy */
        for (i = 0; i < EATT_BEARER_COUNT + ATT_BEARER; i++)
        {
            if (conn->cidSuccess[i] == cid)
            {
                CSR_MASK_SET(conn->numOfBearer[i], CHANNEL_IS_BUSY);
                break;
            }
        }
    }
}

void CsrBtGattPersistGattInfo(CsrBtAddressType addressType,
                            const CsrBtDeviceAddr* addr,
                            CsrBtGattConnElement *conn)
{
#if defined(CSR_BT_GATT_CACHING) || defined (CSR_BT_GATT_INSTALL_EATT) || (!defined(EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL))

    CsrBtTdDbGattInfo gattInfo = { 0 };

    CsrBtTdDbGetGattInfo(addressType, addr, &gattInfo);

#ifdef GATT_CACHING_CLIENT_ROLE
    gattInfo.serviceChange = conn->serviceChangeHandle;
#endif /* GATT_CACHING_CLIENT_ROLE */

    gattInfo.gattSuppFeatInfo = conn->connFlags;
    (void)CsrBtTdDbSetGattInfo(addressType, addr, &gattInfo);
#endif    
}

#ifdef GATT_CACHING_CLIENT_ROLE
CsrBool CsrBtGattCheckCids(CsrBtGattConnElement* conn)
{
    CsrUint8 i;
    CsrBool flag = TRUE;

    for (i = 0; i < EATT_BEARER_COUNT + ATT_BEARER; i++)
    {
        /* Check for something is going on over the air */
        if (conn->numOfBearer[i] == CHANNEL_IS_BUSY)
        {
            flag = FALSE;
        }
    }
    return flag;
}

void CsrBtFlushGattQueuedMsg(GattMainInst* inst, CsrBtConnId btConnId)
{
    CsrUint8 i;
    CsrBtGattQueueElement* qElem = NULL;

    for (i = 0; i < NO_OF_QUEUE; i++)
    {
        qElem = CSR_BT_GATT_QUEUED_MSG_FIND_BT_CONN_ID_FROM_QUEUE_MASK(inst->queue[i], &btConnId);

        while (qElem)
        {
            CsrBtGattPrim* prim;

            if (qElem->gattMsg)
            {
                prim = (CsrBtGattPrim*)qElem->gattMsg;

                if (FlushPendingGattMessage(prim, qElem))
                {
                    CSR_BT_GATT_QUEUE_REMOVE(inst->queue[i], qElem);

                    /* Get the next Queued messaged to flush*/
                    qElem = CSR_BT_GATT_QUEUED_MSG_FIND_BT_CONN_ID_FROM_QUEUE_MASK(inst->queue[i], &btConnId);
                }
            }
        }
    }
}

CsrBool FlushPendingGattMessage(CsrBtGattPrim* prim, CsrBtGattQueueElement* qElem)
{
    CsrBool flushed = FALSE;

    switch (*prim)
    {
        case CSR_BT_GATT_DISCOVER_SERVICES_REQ:
        {
            CsrBtGattDiscoverServicesReq* p = qElem->gattMsg;

            CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_DISCOVER_SERVICES_CFM,
                p->gattId,
                ATT_RESULT_DATABASE_OUT_OF_SYNC,
                CSR_BT_SUPPLIER_ATT,
                p->btConnId);
            flushed = TRUE;
            break;
        }

        case CSR_BT_GATT_DISCOVER_CHARAC_REQ:
        {
            CsrBtGattDiscoverCharacReq* p = qElem->gattMsg;
            CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_DISCOVER_CHARAC_CFM,
                p->gattId,
                ATT_RESULT_DATABASE_OUT_OF_SYNC,
                CSR_BT_SUPPLIER_ATT,
                p->btConnId);
            flushed = TRUE;
            break;
        }

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_REQ:
        {
            CsrBtGattDiscoverCharacDescriptorsReq* p = qElem->gattMsg;
            CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM,
                p->gattId,
                ATT_RESULT_DATABASE_OUT_OF_SYNC,
                CSR_BT_SUPPLIER_ATT,
                p->btConnId);
            flushed = TRUE;
            break;
        }

        case CSR_BT_GATT_FIND_INCL_SERVICES_REQ:
        {
            CsrBtGattFindInclServicesReq* p = qElem->gattMsg;
            CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_FIND_INCL_SERVICES_IND,
                p->gattId,
                ATT_RESULT_DATABASE_OUT_OF_SYNC,
                CSR_BT_SUPPLIER_ATT,
                p->btConnId);
            flushed = TRUE;
            break;
        }

        case CSR_BT_GATT_READ_REQ:
        {
            CsrBtGattReadReq* p = qElem->gattMsg;
            CsrUint8* tmp = NULL;
            CsrBtGattReadCfmHandler(p,
                ATT_RESULT_DATABASE_OUT_OF_SYNC,
                CSR_BT_SUPPLIER_ATT,
                0,
                &tmp);
            flushed = TRUE;
            break;
        }

        case CSR_BT_GATT_READ_BY_UUID_REQ:
        {
            CsrBtGattReadByUuidReq* p = (CsrBtGattReadByUuidReq*)qElem->gattMsg;

            CsrBtGattReadByUuidCfmSend(p->gattId,
                    ATT_RESULT_DATABASE_OUT_OF_SYNC,
                    CSR_BT_SUPPLIER_ATT,
                    p->btConnId,
                    &(p->uuid));
            flushed = TRUE;

            break;
        }

        case CSR_BT_GATT_READ_MULTI_REQ:
        {
            CsrBtGattReadMultiReq* p = qElem->gattMsg;
            CsrBtGattReadMultiCfmSend(p->gattId,
                ATT_RESULT_DATABASE_OUT_OF_SYNC,
                CSR_BT_SUPPLIER_ATT,
                p->btConnId,
                0,
                NULL,
                (p->handlesCount ? p->handles : NULL));
            flushed = TRUE;
            break;
        }

        case CSR_BT_GATT_WRITE_REQ:
        {
            CsrBtGattWriteReq* p = qElem->gattMsg;
            CsrBtGattWriteCfmSend((CsrBtGattPrim*)p,
                ATT_RESULT_DATABASE_OUT_OF_SYNC,
                CSR_BT_SUPPLIER_ATT);
            flushed = TRUE;
            break;
        }

        case CSR_BT_GATT_READ_MULTI_VAR_REQ:
        {
            CsrBtGattReadMultiVarReq* p = qElem->gattMsg;
            CsrBtGattReadMultiVarCfmSend(p->gattId,
                ATT_RESULT_DATABASE_OUT_OF_SYNC,
                CSR_BT_SUPPLIER_ATT,
                p->btConnId,
                0,
                0,
                NULL,
                p->handlesCount,
                (p->handlesCount ? p->handles : NULL));
            flushed = TRUE;
            break;
        }
        default:
        {
            /* add panic/logging here */
            CSR_LOG_TEXT_ERROR((CsrBtGattLto, 0, "flushMessage primType : %x \n", *prim));
            break;
        }
    }
    return flushed;
}

CsrBtGattConnElement* SecondTransportConn(GattMainInst* inst, CsrBtGattConnElement* conn)
{
    CsrBtGattConnElement* conn1 = NULL;

    if (conn->leConnection)
    {
        conn1 = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
            CsrBtGattFindConnInstFromAddressFlagsBredr,
            &(conn->peerAddr));
    }
    else
    {
        conn1 = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
            CsrBtGattFindConnInstFromAddressFlagsLe,
            &(conn->peerAddr));
    }

    return conn1;
}

void HashHandler(GattMainInst* inst, CsrBtGattConnElement* conn)
{
    CsrBtGattConnElement* conn1;
    CsrBool overtheAirTx = FALSE;

    conn1 = SecondTransportConn(inst, conn);

    /* Check for the current transport there are any pending transaction over the air or not */
    /* Table(0 means no on going transaction over the air) :
        LE      BR / EDR    derived condition :
        0        0                  1
        0        1                  0
        1        0                  0
        1        0                  0 */
    if (!CsrBtGattCheckCids(conn))
    {
        overtheAirTx = TRUE;
    }

    /* Check for the second transport if there is any pending transaction over the air or not
     * Once found there is no on going transaction over the air for the current device*/
    if (!overtheAirTx)
    {
        if (conn1 && !CsrBtGattCheckCids(conn1))
        {
            overtheAirTx = TRUE;
        }
    }

    /* Database hash read will be triggered only when there is no on going transaction for both the transport */
    if (!overtheAirTx)
    {
        CsrBtFlushGattQueuedMsg(inst, conn->btConnId);
        if (conn1)
        {
            CsrBtFlushGattQueuedMsg(inst, conn1->btConnId);
        }

        /* Gatt will trigger the internal hash read to the remote device */
        CSR_LOG_TEXT_INFO((CsrBtGattLto, 0, "HashHandler conn : %x\n", conn->btConnId));
        
        /* Unblock the cid by moving the flag to False so that hash will get processed and 
        *  then again block the flag to TRUE so that no new request will get queued or flushed */
        conn->pendingHash = FALSE;
        CsrBtGattReadByUuidPrivateHandler(inst, CSR_BT_GATT_UUID_DATABASE_HASH_CHARAC, conn->btConnId);
        conn->pendingHash = TRUE;

        conn->serviceChangedIndState = SERVICE_CHANGED_INDICATION_IDLE;
        if (conn1)
        {
            conn1->serviceChangedIndState = SERVICE_CHANGED_INDICATION_IDLE;
        }
    }
    else
    {
        CSR_LOG_TEXT_INFO((CsrBtGattLto, 0, "HashHandler: Something over the air is happening conn->pendingHash %d\n", conn->pendingHash));
    }
}

void GattSetHashPendingFunc(CsrCmnListElm_t* elem, void* value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement*)elem;
    CsrBtTypedAddr *addr = (CsrBtTypedAddr*)value;

    if (conn && addr)
    {
        if (gattConnInstAddressMatches(conn, addr))
        {
            conn->pendingHash = TRUE;
        }
    }
}

void DbOutOfSyncAttMsgHandler(uint16 type, GattMainInst* inst)
{
    CsrBool dbOutOfSync = FALSE;
    uint16 cid = 0xffff;
    CsrBtGattConnElement* conn = NULL;
    ATT_UPRIM_T* prim = inst->msg;
    att_result_t result = ATT_RESULT_SUCCESS;

    switch (type)
    {
        case ATT_FIND_INFO_CFM:
        {
            cid = prim->att_find_info_cfm.cid;
            result = prim->att_find_info_cfm.result;
            break;
        }

        case ATT_FIND_BY_TYPE_VALUE_CFM:
        {
            cid = prim->att_find_by_type_value_cfm.cid;
            result = prim->att_find_by_type_value_cfm.result;
            break;
        }

        case ATT_READ_BY_TYPE_CFM:
        {
            cid = prim->att_read_by_type_cfm.cid;
            result = prim->att_read_by_type_cfm.result;
            break;
        }

        case ATT_READ_CFM:
        {
            cid = prim->att_read_cfm.cid;
            result = prim->att_read_cfm.result;
            break;
        }

        case ATT_READ_BLOB_CFM:
        {
            cid = prim->att_read_blob_cfm.cid;
            result = prim->att_read_blob_cfm.result;
            break;
        }

        case ATT_READ_MULTI_CFM:
        {
            cid = prim->att_read_multi_cfm.cid;
            result = prim->att_read_multi_cfm.result;
            break;
        }

        case ATT_READ_BY_GROUP_TYPE_CFM:
        {
            cid = prim->att_read_by_group_type_cfm.cid;
            result = prim->att_read_by_group_type_cfm.result;
            break;
        }

        case ATT_WRITE_CFM:
        {
            cid = prim->att_write_cfm.cid;
            result = prim->att_write_cfm.result;
            break;
        }

        case ATT_PREPARE_WRITE_CFM:
        {
            cid = prim->att_prepare_write_cfm.cid;
            result = prim->att_prepare_write_cfm.result;
            break;
        }

        case ATT_EXECUTE_WRITE_CFM:
        {
            cid = prim->att_execute_write_cfm.cid;
            result = prim->att_execute_write_cfm.result;
            break;
        }

        case ATT_HANDLE_VALUE_CFM:
        {
            cid = prim->att_handle_value_cfm.cid;
            result = prim->att_handle_value_cfm.result;
            break;
        }

        case ATT_READ_MULTI_VAR_CFM:
        {
            cid = prim->att_read_multi_var_cfm.cid;
            result = prim->att_read_multi_var_cfm.result;
            break;
        }

        default:
        {
            break;
        }
    }

    if (result == ATT_RESULT_DATABASE_OUT_OF_SYNC)
    {
        dbOutOfSync = TRUE;
    }

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &(cid));

    if (conn == NULL)
    {
        CSR_LOG_TEXT_INFO((CsrBtGattLto, 0, "DbOutOfSyncAttMsgHandler: ATT operation type %x\n", type));
        return;
    }

    if (dbOutOfSync)
    {
        /* Send connection event to subscribed applications */
        if (!conn->pendingHash)
        {
            CsrCmnListIterate(&inst->connInst, GattSetHashPendingFunc, &conn->peerAddr);
            
            /* Send connection event to subscribed applications */
            CsrBtGattSendOutOfSyncEventToApps(inst, conn);
        }
    }

    /* This condition is kept outside as there are some messages whose response came late but triggered before db update
     * Will come with success and scheduler will miss to trigger the hash so kept outside
     * Need to trigger the hash and clear all the channels including both transport 
     * hash reading is triggered in two cases as: 
     * case 1: Service changed indication is received but all the ATT transactions are not over yet
     * case 2: All the transactions are over then wait for service changed indication from remote and read hash
     * this is decided to handle the case when service changed indication came late from remote and will
     * result into extra hash read */
    if (conn->pendingHash
        && CSR_BT_GATT_IS_CLIENT_ROBUST_CACHING_ENABLED(conn->connFlags)
        && (conn->serviceChangedIndState == SERVICE_CHANGED_INDICATION_SERVED
        || conn->serviceChangedIndState == SERVICE_CHANGED_INDICATION_DISABLED)
       )
    {
        HashHandler(inst, conn);
    }
}
#endif /* GATT_CACHING_CLIENT_ROLE */
