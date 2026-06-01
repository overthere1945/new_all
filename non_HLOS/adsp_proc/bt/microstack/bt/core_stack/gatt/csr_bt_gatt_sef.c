/******************************************************************************
 Copyright (c) 2011-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #11 $
******************************************************************************/

#include "csr_bt_gatt_private.h"

#ifdef GATT_DATA_LOGGER
extern CsrBtGattQueueElement *restoreElement;
#endif /*  GATT_DATA_LOGGER  */

#ifdef CSR_TARGET_PRODUCT_WEARABLE
CsrBool CsrBtGattIsReliableAccessIndFinal(CsrBtConnId btConnId)
{
    CsrBtGattConnElement *conn = NULL;

    /* it should never be the case that the instance is NULL */
    if(gattMainInstPtr == NULL)
    {
        return FALSE;
    }

    conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(gattMainInstPtr->connInst, &btConnId);
    
    if (conn)
    {        
        /* check if this is the final indication in the buffer and let this 
         * provide a way for the application to know this is the final buffer
         */
        if(!CSR_BT_GATT_PREPARE_INST_FIND_CID(gattMainInstPtr->prepare, conn->cid,  CsrBtGattPrepareInstFindCidIdleState))
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif

CsrUint16 CsrBtGattfindFreeCid(GattMainInst *inst, CsrBtConnId btConnId, CsrBool legacyCid)
{
    CsrUint16 cid = INVALID_CID;
    if (btConnId == CSR_BT_GATT_LOCAL_BT_CONN_ID)
    {
        /* Iterate through all the Queues to check if this local btConnId is being used */
        CsrUint8 i;
        CsrBtGattQueueElement* qElem;
        for (i = 0; i < NO_OF_QUEUE ; i++)
        {
            qElem = CSR_BT_GATT_PROGRESS_MSG_FIND_BT_CONN_ID_FROM_QUEUE_MASK(inst->queue[i], &btConnId);
            if (qElem && qElem->cid == ATT_CID_LOCAL)
            {
                return cid;
            }
        }
        return ATT_CID_LOCAL;
    }
    else
    {
        CsrBtGattConnElement  *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst, 
                                                                   &btConnId);
        /* Find the cid for the current connection */
        if(conn)
        {
            cid = CsrBtGattGetCid(inst, conn, legacyCid);
            return cid;
        }
    }
    return L2CA_CID_INVALID;
}

CsrBool CsrBtGattNewReqHandler(GattMainInst          *inst,
                               void                  *msg,
                               CsrBtConnId           btConnId,
                               CsrBtGattId           gattId,
                               CsrBtGattRestoreType  restoreFunc,
                               CsrBtGattCancelType   cancelFunc,
                               CsrBtGattSecurityType securityFunc)
{
    
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                        &gattId);

    if (appElement)
    {
        CsrBool legacyCid = FALSE;
        CsrBtGattPrim type = *(CsrBtGattPrim *)inst->msg;

        CsrBtGattQueueElement* qElem = CSR_BT_GATT_QUEUE_ADD_LAST(inst->queue[appElement->priority]);
        qElem->gattId                = gattId;
        qElem->btConnId              = btConnId;
        qElem->gattMsg               = msg;
        qElem->restoreFunc           = restoreFunc;
        qElem->cancelFunc            = cancelFunc;

        CSR_UNUSED(securityFunc);

         /* This special handling is added for the below case to handle: 
          * There is an early EATT connection done and now EATT channel is available 
          * But MTU exchange shall happen via legacy ATT channel 
          * Check the msg type and if it's MTU Req then always select legacy channel if avaialble else
          * Queue the msg and wait for the channel to be available and then initiate MTU exchange procedure */
        if (type == CSR_BT_GATT_CLIENT_EXCHANGE_MTU_REQ)
        {
            legacyCid = TRUE;
        }

        qElem->cid = CsrBtGattfindFreeCid(inst, btConnId, legacyCid);

        if (qElem->cid != INVALID_CID)
        {
            CsrUint16 mtu = CsrBtGattValidateBtConnIdByMtu(inst, gattId, btConnId, &qElem->cid, DONT_ALLOCATE_CID);
            inst->qid     = appElement->priority;
            /* Call callback function direct */
            qElem->msgState = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS;
#ifdef GATT_OFFLOAD
            if (GattOffloadRestoreHandler(inst, qElem, mtu))
            {
                qElem->restoreFunc(inst, qElem, mtu);
            }
#else
            qElem->restoreFunc(inst, qElem, mtu);
#endif
        }
        else
        {
            /* Saved Msg */
            qElem->msgState = CSR_BT_GATT_MSG_QUEUE_QUEUED;
            CSR_BT_GATT_QUEUE_LOG_QUEUE(qElem->gattMsg);
        }
        return TRUE;
    }
    return FALSE;
}

static void csrBtGattStdCancelElementHandler(GattMainInst          *inst, 
                                             CsrBtGattQueueElement *element)
{
    if (element->msgState == CSR_BT_GATT_MSG_QUEUE_QUEUED)
    { /* This procedure is cancel before it even has started 
         to interact with ATT. E.g the Request that is being 
         cancel is still placed on the local GATT queue */  

        CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                        &element->gattId);
        if (appElement)
        {
            CSR_BT_GATT_QUEUE_REMOVE(inst->queue[appElement->priority], element);
        }
    }
    else
    { /* This procedure is cancel while GATT is interacting
         with ATT or SC. Set cancelFunc to NULL in order to prevent 
         that this procedure is cancel twice */ 
        element->msgState   = CSR_BT_GATT_MSG_QUEUE_CANCELLED;
        element->cancelFunc = NULL;
    }
}

static void csrBtGattStdCancelHandler(void            *gattInst, 
                                      void            *qElem, 
                                      CsrBtGattPrim   confirmType,
                                      CsrBtResultCode result, 
                                      CsrBtSupplier   supplier)
{
    GattMainInst          *inst    = (GattMainInst*)gattInst;
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *)qElem;

    if (result == CSR_BT_GATT_RESULT_CANCELLED)
    {
        CsrBtGattStdBtConnIdCfmSend(confirmType,
                                    element->gattId,
                                    result,
                                    supplier,
                                    element->btConnId);
        /* Set element->restoreFunc to NULL in order to make sure that 
           the application do not received the confirm message twice in case 
           of an ATT_DISCONNECT_IND */
        element->restoreFunc = NULL;
    }
    /* Else - GATT has Cancel the Procedure as part of the 
              unregister procedure */

    /* Check what to do with the pending message */
    csrBtGattStdCancelElementHandler(inst, element); 
}

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
static CsrBool csrBtGattValidateAttrHandles(CsrBtGattDb *db, CsrBtGattAppElement *element, CsrBtGattQueueElement *qElem)
{
    if (db && element)
    {
        CsrBtGattDb *dbTmp;
        CsrBtGattHandle endHdl   = CSR_BT_GATT_ATTR_HANDLE_INVALID;
        CsrBtGattHandle startHdl = CSR_BT_GATT_ATTR_HANDLE_INVALID;

        for(dbTmp = db; dbTmp != NULL; dbTmp = dbTmp->next)
        {
            CsrBtGattAttrFlags writePerms, readPerms;

            /* Ensure handles belong to the application */
            if(!(dbTmp->handle >= element->start &&
                 dbTmp->handle <= element->end))
            {
                return FALSE;
            }
            else if (dbTmp->size_value > CSR_BT_GATT_ATTR_VALUE_LEN_MAX)
            {
                return FALSE;
            }

            /* Find out if write permissions are set correctly. */
            writePerms = dbTmp->flags & (CSR_BT_GATT_ATTR_FLAGS_WRITE_ENCRYPTION |
                                         CSR_BT_GATT_ATTR_FLAGS_WRITE_AUTHENTICATION |
                                         CSR_BT_GATT_ATTR_FLAGS_WRITE_SC_AUTHENTICATION);
            switch (writePerms)
            {
                case CSR_BT_GATT_ATTR_FLAGS_NONE:
                case CSR_BT_GATT_ATTR_FLAGS_WRITE_ENCRYPTION:
                case CSR_BT_GATT_ATTR_FLAGS_WRITE_AUTHENTICATION:
#ifdef CSR_BT_INSTALL_LESC_SUPPORT
                case CSR_BT_GATT_ATTR_FLAGS_WRITE_SC_AUTHENTICATION:
#endif
                    /* Only one write permission has been set */
                    break;
                default:
                    /* Either more than one or an unsupported permission has been set */
                    return (FALSE);
            }

            /* Find out if read permissions are set correctly. */
            readPerms = dbTmp->flags & (CSR_BT_GATT_ATTR_FLAGS_READ_ENCRYPTION |
                                        CSR_BT_GATT_ATTR_FLAGS_READ_AUTHENTICATION |
                                        CSR_BT_GATT_ATTR_FLAGS_READ_SC_AUTHENTICATION);
            switch (readPerms)
            {
                case CSR_BT_GATT_ATTR_FLAGS_NONE:
                case CSR_BT_GATT_ATTR_FLAGS_READ_ENCRYPTION:
                case CSR_BT_GATT_ATTR_FLAGS_READ_AUTHENTICATION:
#ifdef CSR_BT_INSTALL_LESC_SUPPORT
                case CSR_BT_GATT_ATTR_FLAGS_READ_SC_AUTHENTICATION:
#endif
                    /* Only one read permission has been set */
                    break;
                default:
                    /* Either more than one or an unsupported permission has been set */
                    return (FALSE);
            }

            if (startHdl == CSR_BT_GATT_ATTR_HANDLE_INVALID)
            {
                startHdl = dbTmp->handle;
                endHdl   = dbTmp->handle;
            }
            else if (dbTmp->handle > endHdl)
            {
                endHdl = dbTmp->handle;
            }
        }
        /* Save Handles range as these shall be used by GATT when it receives ATT_ADD_CFM */
        qElem->data         = (CsrUint8 *)CsrPmemAlloc(CSR_BT_GATT_SERVICE_CHANGED_LENGTH);
        qElem->dataOffset   = CSR_BT_GATT_SERVICE_CHANGED_LENGTH;
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(startHdl, qElem->data);
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(endHdl, &qElem->data[2]);
        return TRUE;
    }
    return FALSE;
}
#endif

/* Covers Registration and Un-register an application instance to Gatt */
static void csrBtGattRegisterRestoreHandler(GattMainInst          *inst, 
                                            CsrBtGattQueueElement *element,
                                            CsrUint16 mtu)
{
    CsrBtGattRegisterReq  *prim    = element->gattMsg;

    /* This is to store the App context for the respective App */
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                        &element->gattId);
    appElement->context             = prim->context;

    CSR_UNUSED(mtu);

    CsrBtGattRegisterCfmSend(prim->pHandle,
                             element->gattId,
                             CSR_BT_GATT_RESULT_SUCCESS,
                             CSR_BT_SUPPLIER_GATT,
                             prim->context);

    /* This procedure has finished. Start the next if any */
    CsrBtGattQueueRestoreHandler(inst, 
                                 element);
}

static CsrBtGattAppElement *gattRegister(CsrSchedQid qid)
{
    CsrUint16            id;
    CsrBtGattId          gattId      = CSR_BT_GATT_INVALID_GATT_ID;
    CsrBtGattAppElement  *appElement = NULL;

    for (id = gattMainInstPtr->gattIdCounter + 1; id != gattMainInstPtr->gattIdCounter; id++)
    {
        gattId = CSR_BT_GATT_CREATE_GATT_ID(id, qid);

        if (gattId != CSR_BT_GATT_INVALID_GATT_ID
            && !CSR_BT_GATT_APP_INST_FIND_GATT_ID(gattMainInstPtr->appInst, &gattId))
        { /* Found an unallocated GATT ID */
            appElement = CSR_BT_GATT_APP_INST_ADD_FIRST(gattMainInstPtr->appInst);
            gattMainInstPtr->gattIdCounter = id;    /* Updates counter. */
            appElement->gattId      = gattId;
            break;
        }
    }
    return appElement;
}

CsrBtGattId CsrBtGattRegister(CsrSchedQid qid)
{
    CsrBtGattAppElement *appElement;
    appElement = gattRegister(qid);
    
    return appElement != NULL ? appElement->gattId : CSR_BT_GATT_INVALID_GATT_ID;
}

CsrBool CsrBtGattRegisterReqHandler(GattMainInst *inst)
{
    CsrBtGattRegisterReq *prim       = (CsrBtGattRegisterReq *) inst->msg;
    CsrBtGattAppElement  *appElement = NULL;
    
    appElement = gattRegister(prim->pHandle);

    if (appElement)
    { /* Must ensure that a success registration is put on queue, 
         so the register procedure cannot be run while unregister 
         is running */
        CsrBool tmp;
        tmp = CsrBtGattNewReqHandler(inst,
                                     inst->msg,   
                                     CSR_BT_GATT_LOCAL_BT_CONN_ID, 
                                     appElement->gattId, 
                                     csrBtGattRegisterRestoreHandler,
                                     NULL,
                                     NULL);
        inst->msg = NULL;
        return tmp;

    }
    else
    { /* Internal Error unable to create a new element to the appInst.
         Reply to app with an error */
        CsrBtGattRegisterCfmSend(prim->pHandle,
                                 CSR_BT_GATT_INVALID_GATT_ID,
                                 CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                 CSR_BT_SUPPLIER_GATT,
                                 prim->context);
        return TRUE;
    }
}

static void csrBtGattCancelPendingMessageInQueue(GattMainInst *inst, CsrBtGattId gattId, CsrBtConnId btConnId)
{
    CsrBtGattQueueElement *qElem = NULL;
    CsrBtGattAppElement* appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst,
                                                                             &gattId);
    /* Check if the application is in the middle of a procedure
     * that can be cancelled */
    if (appElement)
    {
        CSR_BT_GATT_QUEUE_FIND_MSG_TO_CANCEL(inst->queue[appElement->priority], &qElem, gattId, btConnId);
    }

    if (qElem)
    {
        /* Make sure that all running processes are cancelled.
         * Note the result code shall not be set to
         * CSR_BT_GATT_RESULT_CANCELLED.  If set to
         * CSR_BT_GATT_RESULT_CANCELLED the application WILL
         * receive a confirm message for the running procedure */
        qElem->cancelFunc(inst, qElem, CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID, CSR_BT_SUPPLIER_GATT);
    }
}

static CsrBtConnId csrBtGattIterateReleaseAllConn(GattMainInst *inst,
                                                  CsrBtGattId gattId)
{
    CsrBtGattConnElement *conn;
    CsrBtGattClientService* ele;

    conn = CSR_BT_GATT_CONN_INST_GET_FIRST(inst->connInst);


    while (conn)
    {
        /* Remove all the instances of registered client services by given application *
         * Here if the element exists, it gets removed from client service list, otherwise
         * breaks out of the loop  */

        do{
            ele = CSR_BT_GATT_CLIENT_SERVICE_LIST_FIND_BY_GATTID(conn->cliServiceList, &gattId);

            if (ele)
            {
                CsrCmnListElementRemove(&conn->cliServiceList, (CsrCmnListElm_t*)ele);
            }
        } while (ele);

        csrBtGattCancelPendingMessageInQueue(inst, gattId, conn->btConnId);
        conn = conn->next;
    }

    return CSR_BT_GATT_LOCAL_BT_CONN_ID;
}

static CsrBool csrBtGattRemoveQueuedMessages(CsrCmnListElm_t *elem, void *value)
{
    /* Remove all queue message on the given  gattId */
    CsrBtGattQueueElement *qElem = (CsrBtGattQueueElement *) elem;
    CsrBtGattId           gattId = *(CsrBtGattId *) value;

    if (qElem->gattId   == gattId &&
        qElem->msgState == CSR_BT_GATT_MSG_QUEUE_QUEUED)
    {
        /* Remove the queued messages */
        return TRUE;
    }
    return FALSE;
}

static void csrBtGattStartLocalUnregisterHandler(GattMainInst          *inst,
                                                 CsrBtGattAppElement   *app,
                                                 CsrBtGattQueueElement *qElem)
{
#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
    if (app->start > 0)
    { /* GATT makes sure all handles belonging to the
         application is remove from the data base */
        attlib_remove_req(CSR_BT_GATT_IFACEQUEUE, app->start, app->end, NULL);
    }
    else
#endif /* !CSR_BT_GATT_INSTALL_FLAT_DB */
    {
        CsrBtGattUnregisterApp(inst, app, qElem);
    }
}

static void csrBtGattUnregisterRestoreHandler(GattMainInst *inst, 
                                              CsrBtGattQueueElement *element, 
                                              CsrUint16 mtu)
{
    CsrBtGattUnregisterReq *prim = element->gattMsg;
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                        &prim->gattId);

    CsrBtConnId lastConnId;

    CSR_UNUSED(mtu);

    if (appElement)
    {
        /* Remove all queued messages that belongs to the 
           application being unregistered */
        CSR_BT_GATT_QUEUE_ITERATE_REMOVE(inst->queue[appElement->priority], 
                                         csrBtGattRemoveQueuedMessages, 
                                         &prim->gattId);

        /* Release all the connection which belong to the application
           being unregistered */
        lastConnId = csrBtGattIterateReleaseAllConn(inst, prim->gattId);

        /* If the last btConnId to be cancelled was 'local' then we
         * are not waiting for e.g. ATT to finish - and we can start
         * the local unregistration immediately */
        if (lastConnId == CSR_BT_GATT_LOCAL_BT_CONN_ID)
        {   /* Start local unregister procedure */ 
            csrBtGattStartLocalUnregisterHandler(inst, appElement, element);
        }
    }
    else
    {
        CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "UnregRestoreHdlr no appElement"));
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattUnregisterReqHandler(GattMainInst *inst)
{
    CsrBtGattUnregisterReq *prim = (CsrBtGattUnregisterReq *)inst->msg;
    CsrBool tmp = CsrBtGattNewReqHandler(inst, 
                                         inst->msg,
                                         CSR_BT_GATT_LOCAL_BT_CONN_ID, 
                                         prim->gattId, 
                                         csrBtGattUnregisterRestoreHandler,
                                         NULL,
                                         NULL);
    if (tmp)
    {
        inst->msg = NULL;
    }
    return tmp;
}

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
/* Covers DataBase Configuration */

static CsrUint16 csrBtGattAppInstGetFirstAttrHandle(GattMainInst     *inst,
                                                    CsrUint16        numOfAttrHandles,
                                                    CsrUint16        preferredStartHandle)
{ /* Check if GATT can assign the number of requested attribute
     handles after the worst fit algorithm */
    CsrBtGattAppElement *elem;

    CsrUint16 startHandle  = CSR_BT_GATT_ATTR_HANDLE_INVALID;
    CsrUint16 gap          = 0;
    CsrUint16 maxGap       = 0;
    CsrUint16 nextFree     = 0;
#ifdef CSR_BT_GATT_EXCLUDE_MANDATORY_DB_REGISTRATION
    CsrBool   firstValidHandleRange = TRUE;
#endif

    for (elem = CSR_BT_GATT_APP_INST_GET_FIRST(inst->appInst); elem; elem = elem->next)
    {
        if (elem->start != CSR_BT_GATT_ATTR_HANDLE_INVALID)
        {
#ifdef CSR_BT_GATT_EXCLUDE_MANDATORY_DB_REGISTRATION
            /* We have reached here, this means its not the first allocation.
             *
             * Now application entries in the appInst are sorted in order of increasing attribute handle values.
             * We need to see if there is a gap in the attribute handle range in the beginning itself.
             * e.g. if we have allocated handles for only one application and that too from handle 0x0008-0x0010
             * In this example we have a gap in the beginning itself from 0x0001 - 0x0007.
             *
             * If there is a gap in the beginning itself, there are following three possibilities:
             *  1. APP has given a valid preferred start handle and its value falls in the beginning gap
             *      [our behaviour]: We will allocate handle from beginning gap if gap is sufficient for the required handle range. Otherwise we would look
             *      for other gaps or at the end.
             *  2. APP has given a valid preferred start handle and its value does not fall in the beginning gap
             *      [our behaviour]: We will not allocate handles from the beginning gap even when the gap is big enough to accomodate the request. We will
             *      look for other gaps or at the end.
             *  3. APP has given CSR_BT_GATT_ATTR_HANDLE_INVALID in preferred start handle value
             *      [our behaviour]: We will not allocate handles from the beginning gap even when the gap is big enough to accomodate the request. We will
             *      look for other gaps or at the end.
             */
            if(firstValidHandleRange)
            {
                /* If it is the first valid handle range, we need to see if we can accomodate the new request
                 * before the first valid handle range itself.
                 */
                if((preferredStartHandle != CSR_BT_GATT_ATTR_HANDLE_INVALID)
                    && (elem->start != (CSR_BT_GATT_ATTR_HANDLE_INVALID + 1)) /* This means we have a gap before the first allocated attribute handle */
                    && (preferredStartHandle < elem->start)
                    && ((elem->start - 1) >= numOfAttrHandles))
                {
                    return preferredStartHandle;
                }

                firstValidHandleRange = FALSE;
            }
#endif
            nextFree = elem->end + 1;

            if (elem->next && 
                ((elem->next)->start != CSR_BT_GATT_ATTR_HANDLE_INVALID))
            {
                gap = ((elem->next)->start - nextFree);
            }
            else if (elem->end == CSR_BT_GATT_ATTR_HANDLE_MAX)
            {
                gap = 0;
            }
            else
            {
                gap = (CSR_BT_GATT_ATTR_HANDLE_MAX - nextFree);
            }
            
            if (preferredStartHandle > 0           && 
                gap >= numOfAttrHandles            &&
                preferredStartHandle >= nextFree   &&
                (preferredStartHandle + numOfAttrHandles) <= (nextFree + gap))
            {
                return preferredStartHandle;
            }
            else if (gap >= numOfAttrHandles)
            {
                /* Found a free gap which exact match the number of
                 * requested attributes 
                 */
                return nextFree;
            }
            else if (gap > maxGap)
            {
                /* Found the largest gap so far, update maxGap */
                maxGap      = gap;
                startHandle = nextFree;
            }
        }
    }

    if (maxGap > numOfAttrHandles)
    {
        /* GATT can assign the the number of requested attribute
         * handles 
         */
        return startHandle;
    }
#ifdef CSR_BT_GATT_EXCLUDE_MANDATORY_DB_REGISTRATION
    else if(startHandle != CSR_BT_GATT_ATTR_HANDLE_INVALID)
    {
        /* Could not assign the requested number of attribute handles */
        return CSR_BT_GATT_ATTR_HANDLE_INVALID;
    }
    else
    {
        /* This is for the case where first allocation is being done for any APP.
         * If preferred start handle contains an CSR_BT_GATT_ATTR_HANDLE_INVALID value, we can start allocation
         * with handle value (CSR_BT_GATT_ATTR_HANDLE_INVALID + 1).
         *
         * It is recommended that application managing the GATT service should ask for handle allocation first.
         * If some other application is asking for handle allocation first, it should give a valid preferred 
         * start handle value in the call.
         */
        if(preferredStartHandle != CSR_BT_GATT_ATTR_HANDLE_INVALID)
        {
            return preferredStartHandle;
        }
        else
        {
            return (CSR_BT_GATT_ATTR_HANDLE_INVALID + 1);
        }
    }
#else
    return CSR_BT_GATT_ATTR_HANDLE_INVALID;
#endif
}

CsrBool CsrBtGattDbAllocReqHandler(GattMainInst *inst)
{
    CsrBtGattDbAllocReq *prim = (CsrBtGattDbAllocReq*)inst->msg;
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                        &prim->gattId);

    if (appElement)
    {
        CsrBtResultCode resultCode = CSR_BT_GATT_RESULT_SUCCESS;

        if (appElement->start > 0)
        {
            /* The application have already allocated some attribute handles */
            resultCode = CSR_BT_GATT_RESULT_ATTR_HANDLES_ALREADY_ALLOCATED;
        }
        else if (prim->numOfAttrHandles == 0)
        {
            resultCode = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
        }
        else
        {
            CsrUint16 start = csrBtGattAppInstGetFirstAttrHandle(inst, 
                                                                 prim->numOfAttrHandles,
                                                                 prim->preferredStartHandle);
            if (start == CSR_BT_GATT_ATTR_HANDLE_INVALID)
            {
                /* Cannot meet the application request - Return ERROR */
                resultCode = CSR_BT_GATT_RESULT_INSUFFICIENT_NUM_OF_HANDLES;
            }
            else
            {
                /* Is able to meet the application request add it to appInst */
                appElement->start = start;
                appElement->end   = (start + prim->numOfAttrHandles);
                CSR_BT_GATT_APP_INST_SORT_BY_ATTR_VALUE(inst->appInst);
            }
        }
        /* Reply to app */
        CsrBtGattDbAllocCfmSend(prim->gattId,
                                resultCode,
                                CSR_BT_SUPPLIER_GATT,
                                appElement->start,
                                appElement->end,
                                prim->preferredStartHandle);
        return TRUE;
    }
    return FALSE;
}

static void csrBtGattDbDeallocRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattDbDeallocReq *prim       = (CsrBtGattDbDeallocReq*) element->gattMsg;
    CsrBtGattAppElement   *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                          &prim->gattId);
    CSR_UNUSED(mtu);
    
    if (appElement)
    {
        if (appElement->start == 0)
        {
            CsrBtGattDbDeallocCfmSend(prim->gattId,
                                      CSR_BT_GATT_RESULT_SUCCESS,
                                      CSR_BT_SUPPLIER_GATT,
                                      appElement->start,
                                      appElement->end);

            /* This procedure has finished. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, element);
        }
        else
        { /* GATT makes sure the handles that the application want to
             deallocate also is remove from the data base */
            attlib_remove_req(CSR_BT_GATT_IFACEQUEUE, 
                              appElement->start, 
                              appElement->end, 
                              NULL);
        }
    }
    else
    {
        CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "DbDeallocRestoreHdlr no appElement"));
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattDbDeallocReqHandler(GattMainInst *inst)
{
    CsrBtGattDbDeallocReq *prim = (CsrBtGattDbDeallocReq*)inst->msg;
    CsrBool tmp = CsrBtGattNewReqHandler(inst,
                                         inst->msg,   
                                         CSR_BT_GATT_LOCAL_BT_CONN_ID, 
                                         prim->gattId,
                                         csrBtGattDbDeallocRestoreHandler,
                                         NULL,
                                         NULL);
    inst->msg = NULL;
    return tmp;
}

static void csrBtGattDbAddRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattDbAddReq     *prim       = (CsrBtGattDbAddReq*)element->gattMsg;
    CsrBtGattAppElement   *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                          &prim->gattId);
    
    CSR_UNUSED(mtu);

    if (csrBtGattValidateAttrHandles(prim->db, appElement, element))
    {
        attlib_add_req(CSR_BT_GATT_IFACEQUEUE, prim->db, NULL);
        prim->db = NULL;
    }
    else
    {
        CsrBtGattStdCfmSend(CSR_BT_GATT_DB_ADD_CFM,
                            prim->gattId,
                            CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER,
                            CSR_BT_SUPPLIER_GATT);
        
        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattDbAddReqHandler(GattMainInst *inst)
{
    CsrBtGattDbAddReq *prim      = (CsrBtGattDbAddReq*)inst->msg;
    CsrBool tmp = CsrBtGattNewReqHandler(inst,
                                         inst->msg, 
                                         CSR_BT_GATT_LOCAL_BT_CONN_ID, 
                                         prim->gattId,
                                         csrBtGattDbAddRestoreHandler,
                                         NULL,
                                         NULL);
    inst->msg = NULL;
    return tmp;
}

static void csrBtGattDbRemoveRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattDbRemoveReq  *prim       = (CsrBtGattDbRemoveReq*)element->gattMsg;
    CsrBtGattAppElement   *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &prim->gattId);
        
    CSR_UNUSED(mtu);

    if (appElement)
    {
        if (prim->start >= appElement->start &&
            prim->end <=   appElement->end)
        {
            attlib_remove_req(CSR_BT_GATT_IFACEQUEUE, 
                              prim->start, 
                              prim->end, 
                              NULL);
        }
        else
        {
            CsrBtGattDbRemoveCfmSend(prim->gattId,
                                     CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER,
                                     CSR_BT_SUPPLIER_GATT,
                                     0);

            /* This procedure has finished. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, element);
        }
    }
    else
    {
        CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "DbRemoveRestoreHdlr no appElement"));
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattDbRemoveReqHandler(GattMainInst *inst)
{
    CsrBtGattDbRemoveReq *prim = (CsrBtGattDbRemoveReq*)inst->msg;

    CsrBool tmp = CsrBtGattNewReqHandler(inst,
                                         inst->msg, 
                                         CSR_BT_GATT_LOCAL_BT_CONN_ID, 
                                         prim->gattId,
                                         csrBtGattDbRemoveRestoreHandler,
                                         NULL,
                                         NULL);
    inst->msg = NULL;
    return tmp;
}
#else

static void csrBtGattFlatDbRegisterRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    ATT_ADD_DB_REQ_T *attPrim;
    CsrBtGattFlatDbRegisterReq *prim  = element->gattMsg;

    CSR_UNUSED(mtu);
    CSR_UNUSED(inst);

    /* For now, frame and send the ATT_ADD_DB_REQ directly mainly because of
     * 1. attlib API for ATT_ADD_DB_REQ not enabled in Synergy
     * 2. attlib API doesn't consider flags parameter*/

    /* Validation of the database is currently not done in Synergy
     * as the database is generated from gattdb tool.
     * This can be added in future if required */
    attPrim = (ATT_ADD_DB_REQ_T *) CsrPmemZalloc(sizeof(*attPrim));

    attPrim->type = ATT_ADD_DB_REQ;
    attPrim->phandle = CSR_BT_GATT_IFACEQUEUE;
    attPrim->flags = prim->flags;
    attPrim->size_db = prim->size;
    attPrim->db = prim->db;

    ATT_PutMsg(attPrim);

    prim->db = NULL;
}

CsrBool CsrBtGattFlatDbRegisterReqHandler(GattMainInst *inst)
{
    CsrBool tmp = CsrBtGattNewReqHandler(inst,
                                         inst->msg,
                                         CSR_BT_GATT_LOCAL_BT_CONN_ID,
                                         inst->privateGattId,
                                         csrBtGattFlatDbRegisterRestoreHandler,
                                         NULL,
                                         NULL);
    inst->msg = NULL;
    return tmp;
}

CsrBool CsrBtGattFlatDbRegisterHandleRangeReqHandler(GattMainInst *inst)
{
    CsrBtGattFlatDbRegisterHandleRangeReq *prim = 
                                    (CsrBtGattFlatDbRegisterHandleRangeReq*)inst->msg;
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                        &prim->gattId);

    if (appElement)
    {
        /* If application wants to unregister the service range, it can set
         * the start and end handle as 0 */

        appElement->start = prim->start;
        appElement->end   = prim->end;
        CSR_BT_GATT_APP_INST_SORT_BY_ATTR_VALUE(inst->appInst);

        /* Reply to app */
        CsrBtGattStdCfmSend(CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM,
                            prim->gattId,
                            CSR_BT_GATT_RESULT_SUCCESS,
                            CSR_BT_SUPPLIER_GATT);

        return TRUE;
    }
    return FALSE;
}

#endif

static void csrBtGattAccessQueueRestoreHandler(CsrBtGattConnElement *conn, CsrBtGattAccessIndQueueElement *qElem)
{
    CsrBtGattAccessIndQueueElement *tmp;
    CsrBtConnId btConnId = qElem->btConnId;

    /* Remove the Current qElement from the Queue */
    CSR_BT_GATT_QUEUE_REMOVE(conn->accessIndQueue, qElem);

    /* Check for any outstanding request for the given btconnId */
    tmp = CSR_BT_GATT_ACCESS_IND_QUEUE_FIND_BT_CONN_ID(conn->accessIndQueue, &btConnId);
    if (tmp)
    {
        tmp->restoreFunc(CSR_BT_GATT_GET_QID_FROM_GATT_ID(tmp->gattId), tmp->gattMsg);
        tmp->msgState = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS;
    }
}

CsrBool CsrBtGattDbAccessResHandler(GattMainInst *inst)
{
    l2ca_cid_t cid;
    CsrBtGattDbAccessRes *prim = (CsrBtGattDbAccessRes*)inst->msg;

    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &prim->gattId);

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    if (CsrBtGattValidateBtConnIdByMtu(inst, prim->gattId, prim->btConnId, &cid, DONT_ALLOCATE_CID) > 0)
    {
        CsrBool commit         = FALSE;
        att_result_t attResult = (att_result_t) prim->responseCode;

        CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst,
                                                                       &prim->btConnId);

        CsrBtGattAccessIndQueueElement *qElem = CSR_BT_GATT_ACCESS_IND_QUEUE_FIND_BT_CONN_ID(conn->accessIndQueue, &prim->btConnId);

        /* Assign cid to qElem cid in case of EATT */
        if (qElem)
        {
            cid = qElem->cid;
        }

        if (prim->responseCode == CSR_BT_GATT_ACCESS_RES_SUCCESS)
        { /* Check if the response belongs to a Prepare Write Request or not */
            CsrBtGattPrepareBuffer *buf = CSR_BT_GATT_PREPARE_INST_FIND_CID(inst->prepare, 
                                                                            cid, 
                                                                            CsrBtGattPrepareInstFindCidPendingState);           
            if (buf)
            { /* It does belong to a Prepare Write Request. 
                 Change the state to CSR_BT_GATT_PREPEXEC_DONE */ 
                buf->state = CSR_BT_GATT_PREPEXEC_DONE;
                
                /* Now, check if there are another element that needs to be sent 
                   to the application before the response is sent to ATT */
                buf = CSR_BT_GATT_PREPARE_INST_FIND_CID(inst->prepare, 
                                                        cid, 
                                                        CsrBtGattPrepareInstFindCidIdleState);
                if (buf)
                {
                    CsrBtGattAppElement *element;
                    CsrBtTypedAddr      address;
                    CsrUint16           mtu      = CSR_BT_ATT_MTU_DEFAULT;
                    CsrBtGattConnInfo   connInfo = CSR_BT_GATT_CONNINFO_LE;
                    CsrBtConnId         btConnId = CSR_BT_CONN_ID_INVALID;
                    CsrBtAddrZero(&(address));

                    element = CsrBtGattAccessIndGetParams(inst, cid, buf->handle, 
                                                      &btConnId, &connInfo, &mtu, &address);

                    if (element)
                    { /* Generate a list of all the Elements which match the cid/attribute handle pair and remove 
                         all these elements from the list except one. Note GATT need to keep one element of each handle 
                         in order to be able to sent the CSR_BT_GATT_DB_ACCESS_COMPLETE_IND message */ 
                        CsrBtGattPrepareAttrElem ids;

                        if (qElem)
                        {
                            csrBtGattAccessQueueRestoreHandler(conn, qElem);
                        }

                        CSR_BT_GATT_PREPARE_INST_GET_ATTR_LIST(inst->prepare, buf->cid, buf->handle, 
                                                               CSR_BT_GATT_ACCESS_CHECK_RELIABLE_WRITE, ids);

                        /* Send message to the application */
                        CsrBtGattAccessWriteIndSend(inst, cid,
                                                    element->gattId, btConnId, CSR_BT_GATT_ACCESS_CHECK_RELIABLE_WRITE,
                                                    connInfo, address, ids.unitCount, ids.unit, 
                                                    CsrBtGattResolveServerHandle(element->start,ids.attrHandle));
                        return TRUE;
                    }
                    else
                    { /* The application has unregister itself while the prepare write were active. E.g. the
                         attribute handle does not exist anymore */ 
                        attResult = ATT_RESULT_ATTR_NOT_FOUND;
                    }     
                }
                else
                { /* All Prepare Write Requests has been succesfully responded by the application */
                    commit = TRUE;
                }
            }
            /* Else - The Response does not belong to a Prepare Write Request */
        }
        /* Else - The application has rejected the request to read or write an attribute */ 
        
        CSR_BT_GATT_PREPARE_INST_CLEAN_UP(inst->prepare, inst, cid, commit);
        if (appElement)
        {
            attlib_access_rsp(CSR_BT_GATT_IFACEQUEUE, 
                              cid, 
                              CsrBtGattGetAbsoluteServerHandle(appElement->start,prim->attrHandle), 
                              attResult, 
                              prim->valueLength, 
                              prim->value, 
                              NULL);
        }
        else
        {
            attlib_access_rsp(CSR_BT_GATT_IFACEQUEUE, 
                              cid, 
                              prim->attrHandle, 
                              attResult, 
                              prim->valueLength, 
                              prim->value, 
                              NULL);
        }
        if (qElem)
        {
            csrBtGattAccessQueueRestoreHandler(conn, qElem);
        }
        prim->value = NULL;
    }
    /* Else - The connection has been release while waiting for this response. */
    return TRUE;
}

static void csrBtGattEventSendRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{ /* This message are normally use by a Server application 
     to send a notification or an indication to the Client.
     However this message is also use intern by GATT to trigger 
     a ATT_READ_BLOB_REQ message if a ATT_HANDLE_VALUE_IND
     message is received with a long attribute. If the 
     message is use as a intern GATT message the 
     prim->gattId == inst->privateGattId */
    CsrBtGattEventSendReq *prim    = (CsrBtGattEventSendReq*)element->gattMsg;
    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, 
                                                              &element->cid);

    if (prim->gattId == inst->privateGattId)
    { /* This message is sent by GATT itself  */
        if(conn)
        { 
            if (prim->flags == CSR_BT_GATT_INDICATION_EVENT)
            { /* This message is sent by GATT itself in order to send 
                 a service Changed Event Indication message to the peer device */
              /* This indication is send by GATT itself in order to update peer device
                 for change in Encrypted Data Key Material. */
                if (CSR_BT_GATT_IS_SERVICE_CHANGE_ENABLED(conn) 
#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
                   || (GATT_IS_ENCRYPTED_DATA_KEY_MATERIAL_INDICATION_ENABLED(conn->connFlags) &&
                      (prim->attrHandle == GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE(inst->localLeFeatures)))
#endif /*  EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */
                   )
                { /* The peer device Still want this information */
                    
                    if (prim->valueLength > (mtu - CSR_BT_GATT_ATT_NOTIFICATION_HEADER_LENGTH))
                    {
                        prim->valueLength = (mtu - CSR_BT_GATT_ATT_NOTIFICATION_HEADER_LENGTH);
                    }
                    /* Save Handles range as these shall be used by GATT 
                       when it receives ATT_HANDLE_VALUE_CFM */
                    element->data = (CsrUint8 *)CsrPmemZalloc(prim->valueLength);
                    SynMemCpyS(element->data, prim->valueLength, prim->value, prim->valueLength);
#ifdef GATT_DATA_LOGGER
                    restoreElement = element;
#endif /* GATT_DATA_LOGGER */
                    attlib_handle_value_req(CSR_BT_GATT_IFACEQUEUE, 
                                            element->cid,
                                            prim->attrHandle,
                                            prim->flags,
                                            prim->valueLength,
                                            prim->value,
                                            NULL);

                    /* Can not fail due to security */
                    prim->value = NULL;
                }
                else
                { /* The Peer device do not want this information anymore. 
                     This procedure has finished. Start the next if any  */
                    CsrBtGattQueueRestoreHandler(inst, element);
                }
            }
        }
        else
        {
            /* This procedure has finished. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, element);
        }
    }
    else
    { /* This message is sent by a Server application in order  
         to send a notification or an indication to the Client. */
        CsrBtResultCode resultCode = CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID;
        
        if (mtu > 0)
        {
            if (prim->flags == CSR_BT_GATT_SERVICE_CHANGED_EVENT)
            { /* A Service Changed event shall be sent. As this message
                 is deprecated GATT just return a Success.  */
                resultCode = CSR_BT_GATT_RESULT_SUCCESS;
            }
            else
            { /* Called CsrBtGattNotificationEventReqSend or 
                 CsrBtGattIndicationEventReqSend no need to do
                 any manipulation */
                CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                                 &element->gattId);
                /* Save Handles range as these shall be used by GATT 
                   when it receives ATT_HANDLE_VALUE_CFM */
                element->data = (CsrUint8 *)CsrPmemZalloc(prim->valueLength);
                SynMemCpyS(element->data, prim->valueLength, prim->value, prim->valueLength);
                if (prim->flags == CSR_BT_GATT_NOTIFICATION_EVENT || prim->flags == CSR_BT_GATT_INDICATION_EVENT)
                {
                    /* As per Core Spec v5.2 : ATT Specificaton 
                     * For Notification and Indication Events :
                     * If the attribute value is longer than (ATT_MTU-3) octets, 
                     * then only the first (ATT_MTU-3) octets of this attributes 
                     * value can be sent in a notification or an indication. 
                     */
                    if (prim->valueLength > (mtu - CSR_BT_GATT_ATT_NOTIFICATION_HEADER_LENGTH))
                    {
                        prim->valueLength = (mtu - CSR_BT_GATT_ATT_NOTIFICATION_HEADER_LENGTH);
                    }
#ifdef GATT_DATA_LOGGER
                    restoreElement = element;
#endif /* GATT_DATA_LOGGER */
                    if (appElement)
                    {
                        attlib_handle_value_req(CSR_BT_GATT_IFACEQUEUE, 
                                                element->cid,
                                                CsrBtGattGetAbsoluteServerHandle(appElement->start,prim->attrHandle),
                                                prim->flags,
                                                prim->valueLength,
                                                prim->value,
                                                NULL);
                    }
                    else
                    {
                        attlib_handle_value_req(CSR_BT_GATT_IFACEQUEUE, 
                                                element->cid,
                                                prim->attrHandle,
                                                prim->flags,
                                                prim->valueLength,
                                                prim->value,
                                                NULL);
                    }
                    /* Can not fail due to security */
                    prim->value = NULL;
                    return;
                }
#ifdef CSR_BT_GATT_INSTALL_EATT
                else if(prim->flags == CSR_BT_GATT_MULTI_VAR_NOTIFICATION_EVENT)
                {


                    if (appElement && CSR_BT_GATT_IS_MHVN_ENABLED(conn->connFlags))
                    {
                        attlib_multi_handle_value_ntf_req(CSR_BT_GATT_IFACEQUEUE,
                                                         appElement->context,
                                                         element->cid,
                                                         prim->valueLength,
                                                         prim->value,
                                                         NULL);
                        /* Can not fail due to security */
                        prim->value = NULL;
                        return;
                    }
                    else
                    {
                        /* Operation Cancelled event shall be sent to the App
                         * In case of remote is not supporting MHVN */
                        resultCode = CSR_BT_GATT_RESULT_CANCELLED;
                    }
                }
#endif /* CSR_BT_GATT_INSTALL_EATT */
            }
        }
        /* Else - No connection, reply to app and restore queue */

        CsrBtGattEattResetCid(conn, element->cid, CSR_BT_GATT_RESULT_SUCCESS);

        CsrBtGattEventSendCfmSend(prim, resultCode, CSR_BT_SUPPLIER_GATT);

        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}


#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
void GattEncryptedDataKeyMaterialIndicationSend(GattMainInst* inst,
                                                CsrBtGattConnElement* conn,
                                                uint8* keyMaterial)
{
    CsrBtGattEventSendReq*  newMsg;
    if( keyMaterial )
    {
        newMsg = CsrBtGattEventSendReq_struct(inst->privateGattId,
                                              conn->btConnId,
                                              GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE(inst->localLeFeatures),
                                              CSR_BT_GATT_ATTR_HANDLE_INVALID,
                                              CSR_BT_GATT_INDICATION_EVENT,
                                              GATT_ENCRYPTED_DATA_KEY_MATERIAL_LENGTH,
                                              keyMaterial);

        (void)(CsrBtGattNewReqHandler(inst,
                                      newMsg,
                                      conn->btConnId,
                                      inst->privateGattId,
                                      csrBtGattEventSendRestoreHandler,
                                      NULL,
                                      NULL));
    }
}
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */

#if defined(CSR_BT_GATT_CACHING)
void CsrBtGattServiceChangedIndicationSend(GattMainInst         *inst, 
                                           CsrBtGattConnElement *conn,
                                           CsrBtGattHandle      startHandle,
                                           CsrBtGattHandle      endHandle)  
{
    CsrBtGattEventSendReq *newMsg;

    CsrUint8 *value = (CsrUint8 *)CsrPmemAlloc(CSR_BT_GATT_SERVICE_CHANGED_LENGTH);
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(startHandle, value);
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(endHandle, &(value[2])); 
  
    newMsg = CsrBtGattEventSendReq_struct(inst->privateGattId,
                                          conn->btConnId,
                                          HANDLE_GATT_SERVICE_CHANGED,
                                          CSR_BT_GATT_ATTR_HANDLE_INVALID,
                                          CSR_BT_GATT_INDICATION_EVENT,
                                          CSR_BT_GATT_SERVICE_CHANGED_LENGTH,
                                          value);
    (void)(CsrBtGattNewReqHandler(inst,
                                  newMsg,
                                  conn->btConnId,
                                  inst->privateGattId,
                                  csrBtGattEventSendRestoreHandler,
                                  NULL,
                                  NULL));
}
#endif

CsrBool CsrBtGattEventSendReqHandler(GattMainInst *inst)
{ 
    CsrBool tmp;
    CsrBtGattEventSendReq *prim = (CsrBtGattEventSendReq*)inst->msg;
    
    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));
    
    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg,   
                                 prim->btConnId, 
                                 prim->gattId,
                                 csrBtGattEventSendRestoreHandler,
                                 NULL,
                                 NULL);
    inst->msg = NULL;
    return tmp;
}

/* Covers item 2, Primary Service Discovery, in the GATT feature table  */
static void csrBtGattDiscoverServicesCancelHandler(void *gattInst, void *qElem,
                                                   CsrBtResultCode result, CsrBtSupplier supplier)
{
    /* GATT is using ATT to Discover Primary Services. E.g
             the LE-Radio is being uses */
    csrBtGattStdCancelHandler(gattInst, 
                              qElem, 
                              CSR_BT_GATT_DISCOVER_SERVICES_CFM,
                              result, 
                              supplier);
}

static void csrBtGattDiscoverServicesRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattDiscoverServicesReq *prim = element->gattMsg;

    if (mtu > 0)
    {
        if (prim->uuid.length == CSR_BT_UUID16_SIZE &&
        CSR_BT_UUID_GET_16(prim->uuid) == CSR_BT_GATT_UUID_PRIMARY_SERVICE_DECL)
        { /* Discover all primary services */
            CsrUint32 primaryService[4] = {0x00002800, 0, 0, 0};

            attlib_read_by_group_type_req(CSR_BT_GATT_IFACEQUEUE,
                                          element->cid,
                                          CSR_BT_GATT_ATTR_HANDLE_START,
                                          CSR_BT_GATT_ATTR_HANDLE_MAX,
                                          ATT_UUID16,
                                          (uint32_t *) primaryService,
                                          NULL);
        }
        else
        { /* Discover primary services by using a service UUID */
            CsrUint8 *value = (CsrUint8*) CsrPmemAlloc(prim->uuid.length);
            SynMemCpyS(value,
                       prim->uuid.length,
                       prim->uuid.uuid,
                       prim->uuid.length);

            attlib_find_by_type_value_req(CSR_BT_GATT_IFACEQUEUE,
                                          element->cid,
                                          CSR_BT_GATT_ATTR_HANDLE_START,
                                          CSR_BT_GATT_ATTR_HANDLE_MAX,
                                          CSR_BT_GATT_UUID_PRIMARY_SERVICE_DECL,
                                          prim->uuid.length,
                                          value,
                                          NULL);
        }
    }
    else
    {
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_DISCOVER_SERVICES_CFM,
                                    prim->gattId,
                                    CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID,
                                    CSR_BT_SUPPLIER_GATT,
                                    prim->btConnId);

        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattDiscoverServicesReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    CsrBtGattDiscoverServicesReq *prim = (CsrBtGattDiscoverServicesReq*)inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg,   
                                 prim->btConnId, 
                                 prim->gattId,
                                 csrBtGattDiscoverServicesRestoreHandler,
                                 csrBtGattDiscoverServicesCancelHandler,
                                 NULL);
    inst->msg = NULL;
    return tmp;
}

/* Covers item 3, Relationship Discovery, in the GATT feature table */
static void csrBtGattFindInclServicesCancelHandler(void *gattInst, void *qElem,
                                                   CsrBtResultCode result, CsrBtSupplier supplier)
{
    csrBtGattStdCancelHandler(gattInst, qElem, CSR_BT_GATT_FIND_INCL_SERVICES_CFM,
                              result, supplier);
}

extern CsrUint32 gattInclUuid[4];

static void csrBtGattFindInclServicesRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattFindInclServicesReq *prim = element->gattMsg;

    if (mtu > 0)
    {
        attlib_read_by_type_req(CSR_BT_GATT_IFACEQUEUE,
                                element->cid,
                                prim->startHandle,
                                prim->endGroupHandle,
                                ATT_UUID16,
                                (uint32_t *) &gattInclUuid[0],
                                NULL);
    }
    else
    {
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_FIND_INCL_SERVICES_IND,
                                    prim->gattId,
                                    CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID,
                                    CSR_BT_SUPPLIER_GATT,
                                    prim->btConnId);

        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattFindInclServicesReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    CsrBtGattFindInclServicesReq *prim = (CsrBtGattFindInclServicesReq*)inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg,   
                                 prim->btConnId, 
                                 prim->gattId,
                                 csrBtGattFindInclServicesRestoreHandler,
                                 csrBtGattFindInclServicesCancelHandler,
                                 NULL);
    inst->msg = NULL;
    return tmp;
}

/* Covers item 4, Characteristic Discovery, in the GATT feature table */
static void csrBtGattDiscoverCharacCancelHandler(void            *gattInst, 
                                                 void            *qElem,
                                                 CsrBtResultCode result, 
                                                 CsrBtSupplier   supplier)
{
    csrBtGattStdCancelHandler(gattInst, 
                              qElem, 
                              CSR_BT_GATT_DISCOVER_CHARAC_CFM,
                              result, 
                              supplier);
}

CsrUint32 gattCharacUuid[4] = {0x00002803, 0, 0, 0};

static void csrBtGattDiscoverCharacRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattDiscoverCharacReq *prim = element->gattMsg;

    if (mtu > 0)
    {
        attlib_read_by_type_req(CSR_BT_GATT_IFACEQUEUE,
                                element->cid,
                                prim->startHandle,
                                prim->endGroupHandle,
                                ATT_UUID16,
                                (uint32_t *) gattCharacUuid,
                                NULL);
    }
    else
    {
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_DISCOVER_CHARAC_CFM,
                                    prim->gattId,
                                    CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID,
                                    CSR_BT_SUPPLIER_GATT,
                                    prim->btConnId);

        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattDiscoverCharacReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    CsrBtGattDiscoverCharacReq *prim = (CsrBtGattDiscoverCharacReq*)inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg,
                                 prim->btConnId, 
                                 prim->gattId,
                                 csrBtGattDiscoverCharacRestoreHandler,
                                 csrBtGattDiscoverCharacCancelHandler,
                                 NULL);
    inst->msg = NULL;
    return tmp;
}

/* Covers item 5, Characteristic Descriptor Discovery, in the GATT feature table */
static void csrBtGattDiscoverCharacDescriptorsCancelHandler(void            *gattInst, 
                                                            void            *qElem,
                                                            CsrBtResultCode result, 
                                                            CsrBtSupplier   supplier)
{
    csrBtGattStdCancelHandler(gattInst, 
                              qElem, 
                              CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM,
                              result, 
                              supplier);
}

static void csrBtGattDiscoverCharacDescriptorsRestoreHandler(GattMainInst *inst, 
                                                             CsrBtGattQueueElement *element, 
                                                             CsrUint16 mtu)
{
    CsrBtGattDiscoverCharacDescriptorsReq *prim = element->gattMsg;

    if (mtu > 0)
    {
        attlib_find_info_req(CSR_BT_GATT_IFACEQUEUE,
                             element->cid,
                             prim->startHandle,
                             prim->endGroupHandle,
                             NULL);
    }
    else
    {
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM,
                                    prim->gattId,
                                    CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID,
                                    CSR_BT_SUPPLIER_GATT,
                                    prim->btConnId);

        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattDiscoverCharacDescriptorsReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    CsrBtGattDiscoverCharacDescriptorsReq *prim = (CsrBtGattDiscoverCharacDescriptorsReq*)inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg,   
                                 prim->btConnId, 
                                 prim->gattId,
                                 csrBtGattDiscoverCharacDescriptorsRestoreHandler,
                                 csrBtGattDiscoverCharacDescriptorsCancelHandler,
                                 NULL);
    inst->msg = NULL;
    return tmp;

}

static void csrBtGattReadCancelHandler(void *gattInst, void *qElem,
                                       CsrBtResultCode result, CsrBtSupplier supplier)
{
    GattMainInst          *inst    = (GattMainInst*)gattInst;
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *) qElem;

    if (result == CSR_BT_GATT_RESULT_CANCELLED)
    {
        CsrUint8 *tmp = NULL;
        CsrBtGattReadCfmHandler((CsrBtGattReadReq*) element->gattMsg,
                                result,
                                supplier,
                                0,
                                &tmp);
        
        /* Set element->restoreFunc to NULL in order to make sure that 
           the application do not received the confirm message twice in case 
           of an ATT_DISCONNECT_IND */
        element->restoreFunc = NULL;
    }
    /* Else - GATT has Cancel the Procedure as part of the 
              unregister procedure */

    /* Check what to do with the pending message */
    csrBtGattStdCancelElementHandler(inst, element); 
}

static void csrBtGattReadRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattReadReq *prim    = element->gattMsg;
    CsrBtResultCode result = CSR_BT_GATT_RESULT_SUCCESS;

    if (mtu > 0)
    {
        element->attrHandle = prim->handle;

        if (prim->offset == 0)
        {
            attlib_read_req(CSR_BT_GATT_IFACEQUEUE,
                            element->cid,
                            prim->handle,
                            NULL);
        }
        else
        {
            element->data = (CsrUint8 *) CsrPmemZalloc(CSR_BT_GATT_LONG_READ_ATTR_MAX_VALUE);
            attlib_read_blob_req(CSR_BT_GATT_IFACEQUEUE, 
                                 element->cid, 
                                 prim->handle, 
                                 prim->offset, 
                                 NULL);
        }
    }
    else
    {
        result = CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID;
    }

    if (result != CSR_BT_GATT_RESULT_SUCCESS)
    {
        CsrUint8 *tmp = NULL;
        CsrBtGattReadCfmHandler(prim,
                                result,
                                CSR_BT_SUPPLIER_GATT,
                                0,
                                &tmp);

        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattReadReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    CsrBtGattReadReq *prim = (CsrBtGattReadReq*)inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg, 
                                 prim->btConnId, 
                                 prim->gattId,
                                 csrBtGattReadRestoreHandler,
                                 csrBtGattReadCancelHandler,
                                 NULL);
    inst->msg = NULL;
    return tmp;
}

static void csrBtGattReadByUuidCancelHandler(void               *gattInst,
                                             void               *qElem,
                                             CsrBtResultCode    result,
                                             CsrBtSupplier      supplier)
{
    GattMainInst          *inst    = (GattMainInst*) gattInst;
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *) qElem;

    if (result == CSR_BT_GATT_RESULT_CANCELLED)
    {
        CsrBtGattReadByUuidReq *reqMsg = (CsrBtGattReadByUuidReq *) element->gattMsg;

        CsrBtGattReadByUuidCfmSend(element->gattId,
                                   result,
                                   supplier,
                                   element->btConnId,
                                   &(reqMsg->uuid));
        /* Set element->restoreFunc to NULL in order to make sure that
           the application do not received the confirm message twice in case
           of an ATT_DISCONNECT_IND */
        element->restoreFunc = NULL;
    }
    /* Else - GATT has Cancel the Procedure as part of the
              unregister procedure */

    /* Check what to do with the pending message */
    csrBtGattStdCancelElementHandler(inst, element);
}

static void csrBtGattReadByUuidRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattReadByUuidReq *prim = element->gattMsg;

    if (mtu > 0)
    {
        CsrUint32              attUuid[4];
        att_uuid_type_t        uuidType;
        CsrBtGattGetAttUuid(prim->uuid, attUuid, &uuidType);
        /* element->attrHandle is set prim->startHandle so GATT is 
           able to replay this command in case of security failure */
        element->attrHandle = prim->startHandle;
        attlib_read_by_type_req(CSR_BT_GATT_IFACEQUEUE,
                                element->cid,
                                prim->startHandle,
                                prim->endGroupHandle,
                                uuidType,
                                (uint32_t *) attUuid,
                                NULL);
    }
    else
    { 
        if (prim->gattId != inst->privateGattId)
        { /* Called by an application */
            CsrBtGattReadByUuidCfmSend(prim->gattId,
                                       CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID,
                                       CSR_BT_SUPPLIER_GATT,
                                       prim->btConnId,
                                       &(prim->uuid));
        }
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
        else
        { /* Called by GATT in order to read the remote LE name */
            if (element->dataElemIndex > 0)
            { /* SC has requested to read the name, and the LE connection has 
                 release before GATT had time to read it. Send Result to SC */
                CsrBtGattReadRemoteLeNameCfmSend((CsrSchedQid)element->dataElemIndex, 0, NULL);
            }
        }
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

void CsrBtGattReadByUuidPrivateHandler(GattMainInst *inst, 
                                       CsrBtUuid16  uuidToRead, 
                                       CsrBtConnId  btConnId)
{
    CsrBtUuid             uuid;
    void                  *msg;
    CsrBtGattRestoreType  restoreFunc;

    CsrMemSet(uuid.uuid, 0, CSR_BT_UUID128_SIZE);
    uuid.length = (CsrUint16) (sizeof(CsrBtUuid16));
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(uuidToRead, uuid.uuid);
    
    msg = CsrBtGattReadByUuidReq_struct(inst->privateGattId,
                                        btConnId,
                                        CSR_BT_GATT_ATTR_HANDLE_START,
                                        CSR_BT_GATT_ATTR_HANDLE_MAX,
                                        uuid);

    restoreFunc = csrBtGattReadByUuidRestoreHandler;
    (void)(CsrBtGattNewReqHandler(inst, 
                                  msg,  
                                  btConnId, 
                                  inst->privateGattId, 
                                  restoreFunc,
                                  NULL, 
                                  NULL));

}

CsrBool CsrBtGattReadByUuidReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    CsrBtGattReadByUuidReq *prim = (CsrBtGattReadByUuidReq*)inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg,   
                                 prim->btConnId, 
                                 prim->gattId,
                                 csrBtGattReadByUuidRestoreHandler,
                                 csrBtGattReadByUuidCancelHandler,
                                 NULL);
    inst->msg = NULL;
    return tmp;
}

static void csrBtGattReadMultiCancelHandler(void            *gattInst, 
                                            void            *qElem,
                                            CsrBtResultCode result, 
                                            CsrBtSupplier   supplier)
{
    GattMainInst          *inst    = (GattMainInst*)gattInst;
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *) qElem;
    
    if (result == CSR_BT_GATT_RESULT_CANCELLED)
    {
        CsrBtGattReadMultiReq *req = (CsrBtGattReadMultiReq *) element->gattMsg;

        CsrBtGattReadMultiCfmSend(element->gattId,
                                  result,
                                  supplier,
                                  element->btConnId,
                                  0,
                                  NULL,
                                  (req->handlesCount ? req->handles : NULL));

        /* Set element->restoreFunc to NULL in order to make sure that 
           the application do not received the confirm message twice in case 
           of an ATT_DISCONNECT_IND */
        element->restoreFunc = NULL;
    }
    /* Else - GATT has Cancel the Procedure as part of the 
              unregister procedure */

    /* Check what to do with the pending message */
    csrBtGattStdCancelElementHandler(inst, element); 
}

static void csrBtGattReadMultiRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattReadMultiReq *prim = element->gattMsg;

    if (mtu > 0)
    {
        attlib_read_multi_req(CSR_BT_GATT_IFACEQUEUE,
                              element->cid,
                              prim->handlesCount,
                              CsrMemDup(prim->handles, prim->handlesCount * sizeof(CsrBtGattHandle)),
                              NULL);
        /* Note: prim->handles copied so we can replay this command in
         * case of security failure */
    }
    else
    {
        CsrBtGattReadMultiCfmSend(prim->gattId,
                                  CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID,
                                  CSR_BT_SUPPLIER_GATT,
                                  prim->btConnId,
                                  0,
                                  NULL,
                                  (prim->handlesCount ? prim->handles : NULL));

        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattReadMultiReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    CsrBtGattReadMultiReq *prim = (CsrBtGattReadMultiReq*)inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst, 
                                 inst->msg,   
                                 prim->btConnId, 
                                 prim->gattId,
                                 csrBtGattReadMultiRestoreHandler,
                                 csrBtGattReadMultiCancelHandler,
                                 NULL);
    inst->msg = NULL;
    return tmp;

}

/* Covers item 7, Characteristic Value Write, in the GATT feature table */
void CsrBtGattGetAttPrepareWriteSend(CsrBtGattConnElement  *conn,
                                     CsrBtGattQueueElement *qElem,
                                     CsrUint16             prepareWriteOffset,
                                     CsrUint16             mtu,
                                     CsrUint16             valueLength,
                                     CsrUint8              *value)
{
    CsrUint8  *prepareValue;
    CsrUint16 prepareValueLength;

    if ((valueLength - qElem->dataOffset) > (mtu - CSR_BT_GATT_ATT_PREPARE_WRITE_HEADER_LENGTH))
    { /* Need to fragment the attribute value */ 
        prepareValueLength = (mtu - CSR_BT_GATT_ATT_PREPARE_WRITE_HEADER_LENGTH);
    }
    else
    { /* Send the last fragment or the entire attribute value */ 
        prepareValueLength = (valueLength - qElem->dataOffset);
    }

    prepareValue = (CsrUint8*)CsrPmemAlloc(prepareValueLength);
    SynMemCpyS(prepareValue, prepareValueLength, &value[qElem->dataOffset], prepareValueLength);

    attlib_prepare_write_req(CSR_BT_GATT_IFACEQUEUE,
                             qElem->cid,
                             qElem->attrHandle,
                             prepareWriteOffset,
                             prepareValueLength,
                             prepareValue,
                             NULL);

    CsrBtGattEattSetCid(conn, qElem->cid);
}

void CsrBtGattWriteRequestCancelHandler(void               *gattInst, 
                                           void            *qElem,
                                           CsrBtResultCode result, 
                                           CsrBtSupplier   supplier)
{
    GattMainInst          *inst    = (GattMainInst*) gattInst;
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *) qElem;

    if (result == CSR_BT_GATT_RESULT_CANCELLED && 
        element->msgState == CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_SECURITY)
    { /* The Write request procedure is cancelled while security 
         is pending. */
        CsrBtGattWriteCfmSend((CsrBtGattPrim *) element->gattMsg,
                              result,
                              supplier);

        /* Set element->restoreFunc to NULL in order to make sure that 
           the application do not received the confirm message twice in case 
           of an ATT_DISCONNECT_IND */
        element->restoreFunc = NULL;
    }
    /* Else - GATT has Cancel the Procedure as part of the 
              unregister procedure or while a ATT_WRITE_REQ
              is pending.  */

    /* Check what to do with the pending message */
    csrBtGattStdCancelElementHandler(inst, element); 
}

void CsrBtGattWriteCancelHandler(void               *gattInst, 
                                    void            *qElem,
                                    CsrBtResultCode result, 
                                    CsrBtSupplier   supplier)
{ /* GATT is using this callback function to cancel the
     ATT_PREPARE_WRITE_REQ sub procedure. GATT also uses this
     callback function if the write procedure is cancel before
     it even has started to interact with ATT.
     After receiving a CSR_BT_GATT_WRITE_CLIENT_CONFIGURATION_REQ
     or a CSR_BT_GATT_WRITE_REQ message (which do not uses a
     ATT_PREPARE_WRITE_REQ) the cancel callback function is either 
     set to NULL or to csrBtGattWriteRequestCancelHandler */
    GattMainInst          *inst    = (GattMainInst*) gattInst;
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *) qElem;

    if (result == CSR_BT_GATT_RESULT_CANCELLED)
    {
        CsrBtGattWriteCfmSend((CsrBtGattPrim *) element->gattMsg,
                              result,
                              supplier);

        /* Set element->restoreFunc to NULL in order to make sure that 
           the application do not received the confirm message twice in case 
           of an ATT_DISCONNECT_IND */
        element->restoreFunc = NULL;
    }
    /* Else - GATT has Cancel the Procedure as part of the 
              unregister procedure */
    csrBtGattStdCancelElementHandler(inst, element);
}

static void csrBtGattWriteRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtResultCode         resultCode = CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID;
    CsrBtSupplier           resultSupplier = CSR_BT_SUPPLIER_GATT;

    CsrBtGattWriteReq       *prim      = element->gattMsg;
    CsrBtGattConnElement    *conn      = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &element->cid);
 
    if (prim->btConnId == CSR_BT_GATT_LOCAL_BT_CONN_ID)
    { /* Make sure that the application only can change the value of an 
         attribute that it owns */
        CsrUint32 i;
        CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &prim->gattId);
        
        if (appElement)
        {
#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
            if (prim->attrWritePairs[0].attrHandle == GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE(inst->localLeFeatures))
            {
                /* The application is updating the Encrypted Data Key Matrial value using GATT write request.
                   Send the write CFM to application from GATT itself. */
                GattUpdateEncryptedDataKeyMaterial(inst, prim);
                resultCode = CSR_BT_GATT_RESULT_SUCCESS;
                mtu = 0;
            }
            else
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */
            {
                for (i = 0; i < prim->attrWritePairsCount; i++)
                {

                    if ((prim->attrWritePairs[i].attrHandle < appElement->start ||
                        prim->attrWritePairs[i].attrHandle > appElement->end) && prim->attrWritePairs[i].attrHandle != CSR_BT_GATT_ATTR_HANDLE_CONNECTION_PARAMS)
                    { /* The application is trying to change the value of an
                         attribute which it do not own. Return Error */
                        mtu = 0;
                        resultCode = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
                        break;
                    }
                }
            }
        }
        else
        {
            CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "WriteRestoreHdlr no appElement"));
        }
    }

    if (mtu > 0 && prim->attrWritePairs != NULL)
    {
        CsrBtGattAttrWritePairs *attrPair = &(prim->attrWritePairs[0]);
        element->attrHandle = attrPair->attrHandle;

        switch (prim->flags)
        {
            case CSR_BT_GATT_WRITE_REQUEST:
            { 
                /* attrPair->value is copied so GATT can replay this in case of 
                   security failure. In case of a prepare write this is done by 
                   CsrBtGattGetAttPrepareWriteSend. */
                if (attrPair->offset > 0 ||
                    attrPair->valueLength > (mtu - CSR_BT_GATT_ATT_WRITE_HEADER_LENGTH))
                { /* Must use ATT_PREPARE_WRITE_REQ. Note, GATT do not need 
                     to set element->cancelFunc as it is already set to 
                     csrBtGattWriteCancelHandler */
                    CsrBtGattGetAttPrepareWriteSend(conn,
                                                    element,
                                                    attrPair->offset,
                                                    mtu, 
                                                    attrPair->valueLength, 
                                                    attrPair->value);
                }
                else
                { /* Note element->cancelFunc is set to 
                     csrBtGattWriteRequestCancelHandler 
                     because a Write Request can only be cancel while 
                     setting up security. */
                    element->dataOffset   = attrPair->valueLength;
                    element->data         = attrPair->value;
                    element->attrHandle   = attrPair->attrHandle;
                    attrPair->value       = NULL;
                    attlib_write_req(CSR_BT_GATT_IFACEQUEUE,
                                     element->cid,
                                     attrPair->attrHandle,
                                     prim->flags,
                                     attrPair->valueLength,
                                     CsrMemDup(element->data, element->dataOffset),
                                     NULL);
                }
                return;
            }
            case CSR_BT_GATT_WRITE_COMMAND:
            { 
                if (attrPair->valueLength > (mtu - CSR_BT_GATT_ATT_WRITE_HEADER_LENGTH))
                {
                    resultCode = CSR_BT_GATT_RESULT_INVALID_LENGTH;
                }
                else
                { /* Note: attrPair->value are not copied because the server shall not 
                     send any response so GATT shall never replay this command. That is
                     also why this command cannot be cancel. To prevent this 
                     element->cancelFunc is set to NULL */
#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
                    element->data = (CsrUint8 *)CsrPmemZalloc(attrPair->valueLength);
                    SynMemCpyS(element->data, attrPair->valueLength, attrPair->value, attrPair->valueLength);
#endif

                    attlib_write_req(CSR_BT_GATT_IFACEQUEUE,
                                     element->cid,
                                     attrPair->attrHandle,
                                     prim->flags,
                                     attrPair->valueLength,
                                     attrPair->value,
                                     NULL);

                    attrPair->value     = NULL;
                    element->cancelFunc = NULL;
                    return;
                }
                break;
            }
            case CSR_BT_GATT_WRITE_SIGNED_COMMAND:
            {
                if (attrPair->valueLength > (mtu - CSR_BT_GATT_ATT_SIGNED_WRITE_HEADER_LENGTH))
                {
                    resultCode = CSR_BT_GATT_RESULT_INVALID_LENGTH;
                }
                else
                { /* Note: attrPair->value are not copied because the server shall not 
                     send any response so GATT shall never replay this command. That is
                     also why this command cannot be cancel. To prevent this 
                     element->cancelFunc is set to NULL */
                    attlib_write_req(CSR_BT_GATT_IFACEQUEUE,
                                     element->cid,
                                     attrPair->attrHandle,
                                     prim->flags,
                                     attrPair->valueLength,
                                     attrPair->value,
                                     NULL);

                    attrPair->value     = NULL;
                    element->cancelFunc = NULL;
                    return;
                }
                break;
            }
            case CSR_BT_GATT_WRITE_RELIABLE:
            {
                /* attrPair->value is copied so GATT can replay
                   this in case of security failure. This is done by 
                   CsrBtGattGetAttPrepareWriteSend. Note, GATT do not 
                   need to set element->cancelFunc as it is already 
                   set to csrBtGattWriteCancelHandler */
                CsrBtGattGetAttPrepareWriteSend(conn,
                                                element,
                                                attrPair->offset,
                                                mtu, 
                                                attrPair->valueLength, 
                                                attrPair->value);
                return;
            }
            default:
            {
                resultCode = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
                break;
            }
        }
    }
    else if(prim->btConnId != CSR_BT_GATT_LOCAL_BT_CONN_ID)
    {
        /* Adding a standard att error here for clients to understand the cause of the failure */
        resultCode = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
        resultSupplier = CSR_BT_SUPPLIER_ATT;
    }


    CsrBtGattWriteCfmSend((CsrBtGattPrim *) prim,
                          resultCode,
                          resultSupplier);

    CsrBtGattEattResetCid(conn, element->cid, CSR_BT_GATT_RESULT_SUCCESS);
    /* This procedure has finished. Start the next if any */
    CsrBtGattQueueRestoreHandler(inst, element);
}

CsrBool CsrBtGattWriteReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    CsrBtGattWriteReq *prim = (CsrBtGattWriteReq*)inst->msg;
   
    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst,
                                 inst->msg,   
                                 prim->btConnId, 
                                 prim->gattId,
                                 csrBtGattWriteRestoreHandler,
                                 CsrBtGattWriteCancelHandler,
                                 NULL);
    inst->msg = NULL;
    return tmp;
}

/* Covers cancel of a GATT procedure */
CsrBool CsrBtGattCancelReqHandler(GattMainInst *inst)
{
    CsrBtGattCancelReq *prim = (CsrBtGattCancelReq*) inst->msg;

    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                        &prim->gattId);

    if (CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &(prim->gattId)))
    {   
        CsrBtGattQueueElement *qElem = NULL;

        (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

        if (appElement)
        {
            CSR_BT_GATT_QUEUE_FIND_MSG_TO_CANCEL(inst->queue[appElement->priority], &qElem, prim->gattId, prim->btConnId);
        }

        if (qElem)
        {
            qElem->cancelFunc(inst, qElem, CSR_BT_GATT_RESULT_CANCELLED, CSR_BT_SUPPLIER_GATT);
        }
        /* Else Nothing to cancel just ignore */
        return TRUE;
    }
    /* Invalid gattId */
    return FALSE;
}

/* This function is called when iterating through the application instance list.
    elem will be pointer to CsrBtGattAppElement & value will be a pointer to 
    CsrBtGattConnElement */
static void csrBtGattSendConnectEventToSubscribedApp(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattAppElement *appElement = (CsrBtGattAppElement *)elem;
    CsrBtGattConnElement *conn = value;

    if (conn->leConnection)
    {
        if (CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ATT_LE_FIXED_CHANNEL_STATUS))
        {
            /* LE Connection instance, send connect ind if app has subscribed for it */
            CsrBtGattConnectIndSend(appElement->gattId,
                                    CSR_BT_GATT_RESULT_SUCCESS,
                                    CSR_BT_SUPPLIER_GATT,
                                    conn->btConnId,
                                    CSR_BT_GATT_CONNINFO_LE,
                                    &conn->peerAddr,
                                    conn->mtu,
                                    CsrBtGattGetConnectionLeRole(conn->l2capFlags),
                                    conn->l2capFlags);
        }
    }
#ifdef CSR_BT_INSTALL_GATT_BREDR
    else
    {
        if (CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONNECT_BREDR_CHANNEL_STATUS))
        {
            /* BR/EDR Connection instance, send connect ind if app has subscribed for it */
            CsrBtGattConnectIndSend(appElement->gattId,
                                    CSR_BT_GATT_RESULT_SUCCESS,
                                    CSR_BT_SUPPLIER_GATT,
                                    conn->btConnId,
                                    CSR_BT_GATT_CONNINFO_BREDR,
                                    &conn->peerAddr,
                                    conn->mtu,
                                    CSR_BT_GATT_LE_ROLE_UNDEFINED,
                                    conn->l2capFlags);
        }
    }
#endif
}

static void csrBtGattSendDisconnectEventToSubscribedApp(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattAppElement *appElement = (CsrBtGattAppElement *)elem;
    CsrBtGattConnElement *conn = value;

    if (conn->leConnection)
    {
#ifndef GATT_OFFLOAD
        if (CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ATT_LE_FIXED_CHANNEL_STATUS))
#else            
        if (CSR_MASK_IS_SET(appElement->eventMask, GATT_EVENT_MASK_SUBSCRIBE_OFFLOAD_EVENTS))
#endif            
        {
            CsrBtResultCode result = CSR_BT_GATT_RESULT_SUCCESS;
            /* TWM Disconnect is for LE Case only */
            if (conn->reasonCode == CSR_BT_GATT_RESULT_LINK_TRANSFERRED)
            {
                result = conn->reasonCode;
            }

            CsrBtGattDisconnectIndSend(appElement->gattId,
                                       result,
                                       CSR_BT_SUPPLIER_GATT,
                                       conn->btConnId,
                                       &conn->peerAddr,
                                       CSR_BT_GATT_CONNINFO_LE);
        }
    }
#ifdef CSR_BT_INSTALL_GATT_BREDR
    else
    {
        if (CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONNECT_BREDR_CHANNEL_STATUS))
        {
            CsrBtGattDisconnectIndSend(appElement->gattId,
                                       CSR_BT_GATT_RESULT_SUCCESS,
                                       CSR_BT_SUPPLIER_GATT,
                                       conn->btConnId,
                                       &conn->peerAddr,
                                       CSR_BT_GATT_CONNINFO_BREDR);
        }
    }
#endif
}

#ifdef GATT_CACHING_CLIENT_ROLE
static void csrBtGattSendOutOfSyncEventToSubscribedApp(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattAppElement *appElement = (CsrBtGattAppElement *)elem;
    CsrBtGattConnElement *conn      = value;

    if (CSR_MASK_IS_SET(appElement->eventMask, GATT_EVENT_MASK_SUBSCRIBE_DATABASE_SYNC_STATUS))
    {
        CsrUint8 dataBaseFlag;
        CsrBtGattHandle statrtHandle = CSR_BT_GATT_ATTR_HANDLE_MAX;

        if (conn->pendingHash == TRUE)
        {
            dataBaseFlag = GATT_REMOTE_DATABASE_OUT_OF_SYNC;
        }
        else
        {
            statrtHandle = CSR_BT_GATT_ATTR_HANDLE_START;
            dataBaseFlag = GATT_REMOTE_DATABASE_IN_SYNC;

            /* Reset the SSF information from conn flag and service changed handle persisted inforation as the remote db is updated and need to redisocver on reconnection */
            CSR_BT_GATT_CLEAR_SSF(conn->connFlags);
            conn->serviceChangeHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
        }

        /* Send data base indication status to the subscribed App */
        GattRemoteDatabaseChangedInd(appElement->gattId,
                                         conn->btConnId,
                                         dataBaseFlag,
                                         statrtHandle,
                                         CSR_BT_GATT_ATTR_HANDLE_MAX);
    }
}
#endif /* GATT_CACHING_CLIENT_ROLE */

#ifdef CSR_BT_GATT_INSTALL_EATT
static void csrBtGattSendEattConnectEventToSubscribedApp(CsrCmnListElm_t* elem, void* value)
{
    CsrBtGattAppElement* appElement = (CsrBtGattAppElement*)elem;
    CsrBtGattConnElement* conn = value;

    if (CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_EATT_LE_CONNECT_CHANNEL_STATUS))
    {
        CsrBtGattEattConnectIndSend(appElement->gattId,
            conn->btConnId,
            MODE_EATT,
            CSR_BT_GATT_RESULT_SUCCESS,
            CSR_BT_SUPPLIER_GATT);
    }
}
#endif /* CSR_BT_GATT_INSTALL_EATT */

/* This function is called when iterating through the connection instance list.
    elem will be pointer to CsrBtGattConnElement & value will be a pointer to 
    CsrBtGattAppElement */
static void csrBtGattSendConnectEvent(CsrCmnListElm_t *elem, void *value)
{
    /* To reuse the function swap the pointers */
    csrBtGattSendConnectEventToSubscribedApp(value, elem);
}

void CsrBtGattSendConnectEventToApps(GattMainInst *inst, CsrBtGattConnElement *conn)
{
    CSR_BT_GATT_APP_INST_ITERATE(inst->appInst,
                                 csrBtGattSendConnectEventToSubscribedApp,
                                 conn);
}

void CsrBtGattSendDisconnectEventToApps(GattMainInst *inst, CsrBtGattConnElement *conn)
{
    CSR_BT_GATT_APP_INST_ITERATE(inst->appInst,
                                 csrBtGattSendDisconnectEventToSubscribedApp,
                                 conn);
}
#ifdef CSR_BT_GATT_INSTALL_EATT
void CsrBtGattSendEattConnectEventToApps(GattMainInst *inst, CsrBtGattConnElement *conn)
{
    CSR_BT_GATT_APP_INST_ITERATE(inst->appInst,
                                 csrBtGattSendEattConnectEventToSubscribedApp,
                                 conn);
}
#endif /* CSR_BT_GATT_INSTALL_EATT */

#ifdef GATT_CACHING_CLIENT_ROLE
void CsrBtGattSendOutOfSyncEventToApps(GattMainInst* inst, CsrBtGattConnElement* conn)
{
    CSR_BT_GATT_APP_INST_ITERATE(inst->appInst,
                                 csrBtGattSendOutOfSyncEventToSubscribedApp,
                                 conn);
}
#endif /* GATT_CACHING_CLIENT_ROLE */

CsrBool CsrBtGattSetEventMaskReqHandler(GattMainInst *inst)
{
    CsrBtGattSetEventMaskReq *prim  = (CsrBtGattSetEventMaskReq *) inst->msg;
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                        &prim->gattId);
    CsrBtGattEventMask mask;

    if (appElement)
    {
        /* Ensure only valid bits are set in the CFM */
        mask = prim->eventMask & (CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ALL);

        if ((prim->eventMask - mask) != 0)
        {
            /* Attempt to subscribe to invalid mask */
            CsrBtGattStdCfmSend(CSR_BT_GATT_SET_EVENT_MASK_CFM,
                                prim->gattId,
                                CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER,
                                CSR_BT_SUPPLIER_GATT);
        }
        else
        {
            CsrBtGattStdCfmSend(CSR_BT_GATT_SET_EVENT_MASK_CFM,
                                prim->gattId,
                                CSR_BT_GATT_RESULT_SUCCESS,
                                CSR_BT_SUPPLIER_GATT);

            if (mask & CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ATT_LE_FIXED_CHANNEL_STATUS && 
                !(appElement->eventMask & CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ATT_LE_FIXED_CHANNEL_STATUS))
            {
                appElement->eventMask |= CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ATT_LE_FIXED_CHANNEL_STATUS;
                /* Iterate through the connection list & send connect event to this app */
                CSR_BT_GATT_CONN_INST_ITERATE(inst->connInst,
                                              csrBtGattSendConnectEvent, appElement);
            }
#ifdef CSR_BT_INSTALL_GATT_BREDR
            if (mask & CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONNECT_BREDR_CHANNEL_STATUS &&
                !(appElement->eventMask & CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONNECT_BREDR_CHANNEL_STATUS))
            {
               if (inst->bredrAppHandle == CSR_SCHED_QID_INVALID)
               {
                    appElement->eventMask |= CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONNECT_BREDR_CHANNEL_STATUS;
                    inst->bredrAppHandle = appElement->gattId;
               }
               else
               {
                   CsrBtGattStdCfmSend(CSR_BT_GATT_SET_EVENT_MASK_CFM,
                                       prim->gattId,
                                       CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER,
                                       CSR_BT_SUPPLIER_GATT);
               }
            }
#endif

#ifdef CSR_BT_INSTALL_GATT_BREDR
            if ((mask == CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_NONE) &&
                (appElement->eventMask & CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONNECT_BREDR_CHANNEL_STATUS))
            { /* Resetting the bredr handle stored in GATT main instance */
                inst->bredrAppHandle = CSR_SCHED_QID_INVALID;
            }
#endif
            /* Setup new mask */
            appElement->eventMask = mask;
        }
        return TRUE;
    }

    /* Invalid gattId */
    return FALSE;
}

#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
void CsrBtGattReadRemoteLeNameReqHandler(GattMainInst *inst)
{
    CsrBtGattReadRemoteLeNameReq *prim = (CsrBtGattReadRemoteLeNameReq *) inst->msg;
    CsrUint8  *remoteName      = NULL;
    CsrUint16 remoteNameLength = 0;
    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst, 
                                                                         CsrBtGattFindConnectedConnInstFromAddress,
                                                                         &(prim->address)); 
    if (conn)
    {
        if (conn->remoteName)
        { /* Gatt has already read the name */
            remoteNameLength = conn->remoteNameLength;
            remoteName = (CsrUint8 *) CsrPmemAlloc(remoteNameLength);
            SynMemCpyS(remoteName, remoteNameLength, conn->remoteName, remoteNameLength);
        }
        else
        { /* Check if GATT is reading the name or it failed to read it */
            CsrBtGattQueueElement *qElem = NULL;
            CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_ATTR_HANDLE(inst->appInst, &prim->pHandle);

            if (appElement)
            {
                CSR_BT_GATT_QUEUE_FIND_PRIVATE_READ_NAME_MSG(inst->queue[appElement->priority],
                                                         &qElem, 
                                                         inst->privateGattId, 
                                                         conn->btConnId);
            }
            if (qElem && qElem->dataElemIndex == 0)
            { /* This message is received before GATT had time to read the name.
                 Set qElem->dataElemIndex to prim->qid to mark that GATT shall 
                 sent a CsrBtGattReadRemoteLeNameCfm message to prim->qid (SC)
                 as soon as it has read the name. Note the qElem->dataElemIndex
                 is not used by a normal CsrBtGattReadByUuidReq procedure, so
                 GATT is using this parameter to mark if that this request
                 is pending. */ 
                qElem->dataElemIndex = (CsrUint16) prim->pHandle;
                return;
            }
            /* Else - GATT failed to read the name */
        }
    }
    CsrBtGattReadRemoteLeNameCfmSend(prim->pHandle, remoteNameLength, remoteName);
}
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

#ifdef CSR_BT_INSTALL_GATT_BREDR
static void csrBtGattConnectBredrRestoreHandler(GattMainInst *inst,
                                                CsrBtGattQueueElement *element,
                                                CsrUint16 mtu)
{
    CsrBtGattConnectBredrReq *prim = element->gattMsg;
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst,
                                                                        &prim->gattId);

    CsrBtResultCode result = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;

    CSR_UNUSED(mtu);

    if (appElement)
    {
        if (CsrBtAddrIsPublic(prim->address)
            && !CsrBtBdAddrEqZero(&(prim->address.addr)))
        { /* Address parameter is acceptable */
            CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_BREDR(inst->connInst,
                                         CsrBtGattFindConnInstFromAddressFlagsBredr,
                                         &(prim->address));
            if (!conn)
            {
                L2CA_CONNECTION_T method = L2CA_CONNECTION_BR_EDR;

                conn = CSR_BT_GATT_CONN_INST_ADD_LAST(inst->connInst);

                conn->peerAddr = prim->address;
                conn->gattId = prim->gattId;
                conn->leConnection = FALSE;

                attlib_connect_req(CSR_BT_GATT_IFACEQUEUE,
                                   &prim->address,
                                   method,
                                   L2CA_CONFLAG_ENUM(method),
                                   NULL);

                result = CSR_BT_GATT_RESULT_CONNECTION_PENDING;
            }
            else
            { /* Race condition or duplicate request */
                result = CSR_BT_GATT_RESULT_ALREADY_CONNECTED;
            }
        }

        CsrBtGattStdCfmSend(CSR_BT_GATT_CONNECT_BREDR_CFM,
                            prim->gattId,
                            result,
                            CSR_BT_SUPPLIER_GATT);

        if (result != CSR_BT_GATT_RESULT_CONNECTION_PENDING)
            CsrBtGattQueueRestoreHandler(inst, element);
    }
}
CsrBool CsrBtGattConnectBredrReqHandler(GattMainInst *inst)

{
    CsrBtGattConnectBredrReq *prim = (CsrBtGattConnectBredrReq *)inst->msg;
    CsrBool tmp = CsrBtGattNewReqHandler(inst,
                                         inst->msg,
                                         CSR_BT_GATT_LOCAL_BT_CONN_ID,
                                         prim->gattId,
                                         csrBtGattConnectBredrRestoreHandler,
                                         NULL,
                                         NULL);
    if (tmp)
    {
        inst->msg = NULL;
    }
    return tmp;
}

CsrBool CsrBtGattAcceptBredrReqHandler(GattMainInst *inst)
{
    CsrBtGattAcceptBredrReq *prim = (CsrBtGattAcceptBredrReq *)inst->msg;
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst,
                                                                        &prim->gattId);

    if (appElement)
    {
#ifndef EXCLUDE_CSR_BT_CM_MODULE
        CsrBtCml2caConnectAcceptReqSend(CSR_BT_GATT_IFACEQUEUE,
                                        L2CA_PSM_INVALID,
                                        0,
                                        SECL4_IN_LEVEL_2,
                                        0,
                                        L2CA_FLUSH_TO_DEFAULT,
                                        NULL,
                                        0,
                                        CSR_BT_SC_DEFAULT_ENC_KEY_SIZE);
#endif

        CsrBtGattStdCfmSend(CSR_BT_GATT_ACCEPT_BREDR_CFM,
                            prim->gattId,
                            CSR_BT_GATT_RESULT_SUCCESS,
                            CSR_BT_SUPPLIER_GATT);
    }

    inst->msg = NULL;
    return TRUE;
}

CsrBool CsrBtGattCancelAcceptBredrReqHandler(GattMainInst *inst)
{
    CsrBtGattCancelAcceptBredrReq *prim = (CsrBtGattCancelAcceptBredrReq *)inst->msg;
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst,
                                                                        &prim->gattId);

    if (appElement)
    {
#ifndef EXCLUDE_CSR_BT_CM_MODULE
            CsrBtCml2caCancelConnectAcceptReqSend(CSR_BT_GATT_IFACEQUEUE, L2CA_PSM_INVALID);
#endif
            CsrBtGattStdCfmSend(CSR_BT_GATT_CANCEL_ACCEPT_BREDR_CFM,
                                prim->gattId,
                                CSR_BT_GATT_RESULT_SUCCESS,
                                CSR_BT_SUPPLIER_GATT);
    }

    inst->msg = NULL;
    return TRUE;
}

CsrBool CsrBtGattConnectBredrResHandler(GattMainInst *inst)
{
    CsrBtGattConnectBredrRes *prim = inst->msg;
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst,
                                                                        &prim->gattId);

    if (appElement && (appElement->gattId == inst->bredrAppHandle))
    {
        CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_BREDR(inst->connInst,
                                                                                         CsrBtGattFindConnInstFromAddressFlagsBredr,
                                                                                         &(prim->address));

        if (conn)
        {
#ifdef INSTALL_ATT_BREDR
            l2ca_conn_result_t response;

            if (prim->responseCode == CSR_BT_GATT_BREDR_ACCEPT_CONNECTION)
            {
                response = L2CA_CONNECT_SUCCESS;
            }
            else
            {
                response = L2CA_CONNECT_REJ_PSM;
            }
            attlib_connect_rsp(CSR_BT_GATT_IFACEQUEUE, conn->cid, response, NULL);
#endif
        }
    }
    return TRUE;
}

CsrBool CsrBtGattDisconnectBredrReqHandler(GattMainInst *inst)
{
    CsrBtGattDisconnectBredrReq *prim = (CsrBtGattDisconnectBredrReq*)inst->msg;

    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_BREDR(inst->connInst,
                                                                                     CsrBtGattFindConnInstFromAddressFlagsBredr,
                                                                                     &(prim->address));

    if (conn)
    {
        /* Releasing connected logical link or cancelling
         a  BrEdrConnect connection */
        attlib_disconnect_req(CSR_BT_GATT_IFACEQUEUE,
                              conn->cid,
                              NULL);
    }

    inst->msg = NULL;
    return TRUE;
}
#endif

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
void CsrBtGattReadRemoteRpaOnlyCharReqHandler(GattMainInst *inst)
{
    CsrBtGattReadRemoteRpaOnlyCharReq *prim = (CsrBtGattReadRemoteRpaOnlyCharReq *) inst->msg;
    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
                                                                         CsrBtGattFindConnectedConnInstFromAddress,
                                                                         &(prim->address));
    if (conn)
    {
        CsrBtGattReadByUuidPrivateHandler(inst, CSR_BT_GATT_UUID_RESOLVABLE_PRIVATE_ADDRESS_ONLY_CHARAC, conn->btConnId);
    }
    else
    {
        CsrBtGattReadRemoteRpaOnlyCharCfmSend(CSR_BT_CM_IFACEQUEUE,
                                              CSR_BT_GATT_RPA_ONLY_VALUE_INVALID,
                                              prim->address,
                                              CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID,
                                              CSR_BT_SUPPLIER_GATT);
    }
}
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */

/* Documentation: Capture that this api can be called as soon as a btconnid is generated */
#ifdef CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
CsrBool CsrBtGattClientRegisterServiceReqHandler(GattMainInst *inst)
{
    CsrBtGattClientRegisterServiceReq *prim = inst->msg;
    CsrBtGattConnElement *conn;
    CsrBtGattClientService *cliSvcElem;

    conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
                                                   CsrBtGattFindConnInstFromAddress,
                                                   &(prim->address));
    if (conn)
    {
        cliSvcElem = CSR_BT_GATT_CLIENT_SERVICE_LIST_ADD_LAST(conn->cliServiceList);
        cliSvcElem->start = prim->start;
        cliSvcElem->end = prim->end;
        cliSvcElem->gattId = prim->gattId;
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM,
                                    prim->gattId,
                                    CSR_BT_GATT_RESULT_SUCCESS,
                                    CSR_BT_SUPPLIER_GATT,
                                    conn->btConnId);
    }
    else
    {
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM,
                                    prim->gattId,
                                    CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID,
                                    CSR_BT_SUPPLIER_GATT,
                                    0);
    }

    return TRUE;
}
#ifndef GATT_OFFLOAD
CsrBool CsrBtGattClientIndicationRspHandler(GattMainInst *inst)
{
    CsrBtGattClientIndicationRsp *prim = inst->msg;
    CsrBtGattConnElement *conn;

    conn = CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId);

    if (conn)
    {
        l2ca_cid_t cid = conn->cid;

        CsrBtGattAccessIndQueueElement* qElem = CSR_BT_GATT_ACCESS_IND_QUEUE_FIND_BT_CONN_ID(conn->accessIndQueue, &prim->btConnId);

        /* With EATT it is always going to get serialized and there will be an element
           so we can directly Assign cid to qElem cid in case of EATT */
        cid = qElem->cid;

        attlib_handle_value_rsp(CSR_BT_GATT_IFACEQUEUE, cid, NULL);

        csrBtGattAccessQueueRestoreHandler(conn, qElem);

    }

    return TRUE;
}
#endif
#endif /* CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION */




#ifdef CSR_BT_GATT_INSTALL_EATT
static void CsrBtGattPriorityReqRestoreHandler(GattMainInst *inst,
                                   CsrBtGattQueueElement *element,
                                                    CsrUint16 mtu)
{
    CsrBtGattAppPriorityChangeReq *prim            = element->gattMsg;
    bool                      invalidPriority  = TRUE;
    CsrBtResultCode           resutlCode       = CSR_BT_GATT_RESULT_SUCCESS;

    CsrBtGattAppElement      *appElement       = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst,
                                                                        &prim->gattId);
    CsrBtGattConnElement     *conn             = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst,
                                                                        &prim->btConnId);

    CSR_UNUSED(mtu);

    if (conn)
    {
        /* In the connected state if App has initiated App priority change proceudre
           Gatt has to reset the allocated CID */
        CsrBtGattEattResetCid(conn, element->cid, ATT_RESULT_SUCCESS);

        CsrBtGattPriorityCfmSend(prim->gattId,
                               prim->btConnId,
      CSR_BT_GATT_PRIORITY_CHANGE_NOT_ALLOWED,
                        CSR_BT_SUPPLIER_GATT);
    }
    else 
    {
        if (appElement)
        {
            if (prim->AppPriority <= CSR_BT_APP_PRIORITY_HIGH)
            {
                appElement->priority = prim->AppPriority;
                invalidPriority = FALSE;
            }
        }
        if (invalidPriority == TRUE)
        {
            resutlCode = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
        }

        CsrBtGattPriorityCfmSend(prim->gattId,
                               prim->btConnId,
                                   resutlCode,
                        CSR_BT_SUPPLIER_GATT);
    }
    /* This procedure has finished. Start the next if any */
    CsrBtGattQueueRestoreHandler(inst, element);
}

CsrBool CsrBtGattPriorityReqHandler(GattMainInst *inst)
{
    CsrBtGattAppPriorityChangeReq   *prim     = inst->msg;

    CsrBool tmp = CsrBtGattNewReqHandler(inst,
                                         inst->msg,
                                         prim->btConnId,
                                         prim->gattId,
                                         CsrBtGattPriorityReqRestoreHandler,
                                         NULL,
                                         NULL);
    inst->msg = NULL;
    return tmp;
}

static void csrBtGattReadMultiVarCancelHandler(void        *gattInst,
                                               void           *qElem,
                                               CsrBtResultCode result,
                                               CsrBtSupplier supplier)
{
    GattMainInst          *inst    = (GattMainInst*)gattInst;
    CsrBtGattQueueElement *element = (CsrBtGattQueueElement *) qElem;
    
    if (result == CSR_BT_GATT_RESULT_CANCELLED)
    {
        CsrBtGattReadMultiVarReq *req = (CsrBtGattReadMultiVarReq *) element->gattMsg;

        CsrBtGattReadMultiVarCfmSend(element->gattId,
                                  result,
                                  supplier,
                                  element->btConnId,
                                  0,
                                  0,
                                  NULL,
                                  req->handlesCount,
                                  (req->handlesCount ? req->handles : NULL));

        /* Set element->restoreFunc to NULL in order to make sure that 
           the application do not received the confirm message twice in case 
           of an ATT_DISCONNECT_IND */
        element->restoreFunc = NULL;
    }
    /* Else - GATT has Cancel the Procedure as part of the 
              unregister procedure */

    /* Check what to do with the pending message */
    csrBtGattStdCancelElementHandler(inst, element); 
}

static void csrBtGattReadMultiVarRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu)
{
    CsrBtGattReadMultiVarReq *prim = element->gattMsg;
    if (mtu > 0)
    {
        attlib_read_multi_var_req(CSR_BT_GATT_IFACEQUEUE,
                              element->cid,
                              prim->handlesCount,
                              CsrMemDup(prim->handles, prim->handlesCount * sizeof(CsrBtGattHandle)),
                              NULL);
        /* Note: prim->handles copied so we can replay this command in
         * case of security failure */
    }
    else
    {
        CsrBtGattReadMultiVarCfmSend(prim->gattId,
                                  CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID,
                                  CSR_BT_SUPPLIER_GATT,
                                  prim->btConnId,
                                  0,
                                  0,
                                  NULL,
                                  prim->handlesCount,
                                  (prim->handlesCount ? prim->handles : NULL));

        /* This procedure has finished. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, element);
    }
}

CsrBool CsrBtGattReadMultiVarReqHandler(GattMainInst *inst)
{
    CsrBool tmp;
    CsrBtGattReadMultiVarReq *prim = (CsrBtGattReadMultiVarReq*)inst->msg;

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    tmp = CsrBtGattNewReqHandler(inst, 
                                 inst->msg,
                                 prim->btConnId,
                                 prim->gattId,
                                 csrBtGattReadMultiVarRestoreHandler,
                                 csrBtGattReadMultiVarCancelHandler,
                                 NULL);
    inst->msg = NULL;
    return tmp;
}

CsrBool CsrBtGattReadMultiVarRspHandler(GattMainInst *inst)
{
    l2ca_cid_t cid;
    CsrBtGattDbAccessReadMultiVarRsp *prim = (CsrBtGattDbAccessReadMultiVarRsp*)inst->msg;

    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(inst->connInst, 
                                                                   &prim->btConnId);

    CsrBtGattAccessIndQueueElement* qElem = CSR_BT_GATT_ACCESS_IND_QUEUE_FIND_BT_CONN_ID(conn->accessIndQueue, &prim->btConnId);
    if (qElem)
    {
        cid = qElem->cid;
    }

    (void)(CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    if (CsrBtGattValidateBtConnIdByMtu(inst, prim->gattId, prim->btConnId, &cid, DONT_ALLOCATE_CID) > 0)
    {
        att_result_t attResult = (att_result_t) prim->responseCode;

        attlib_read_multi_var_rsp(CSR_BT_GATT_IFACEQUEUE,
                          cid,
                          attResult,
                          prim->errorHandle,
                          prim->valuesLength,
                          prim->values,
                          NULL);

        if (qElem)
        {
            csrBtGattAccessQueueRestoreHandler(conn, qElem);
        }
        prim->values = NULL;
    }
    /* Else - The connection has been release while waiting for this response. */
    return TRUE;
}

#ifdef GATT_ENABLE_EATT_DISCONNECT
CsrBool CsrBtGattEattDisconnectReqHandler(GattMainInst *inst)
{
    CsrBtGattEattDisconnectReq *prim = inst->msg;
    CsrBtGattConnElement *conn;

    conn = (CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(inst->connInst, &prim->btConnId));

    if (conn)
    {
        CsrUint16 i;
        CsrUint16 numCid2Disc = CSRMIN(EATT_BEARER_COUNT, prim->numCid2Disc);

        for (i = 0; ((i < EATT_BEARER_COUNT) && numCid2Disc); i++)
        {
            if (conn->numOfBearer[i] != CHANNEL_IS_INVALID)
            {
                attlib_disconnect_req(CSR_BT_GATT_IFACEQUEUE, conn->cidSuccess[i], NULL);
                --numCid2Disc;
            }
        }
    }
    return TRUE;
}
#endif /* GATT_ENABLE_EATT_DISCONNECT */

#endif /* CSR_BT_GATT_INSTALL_EATT */

/* Covers Mode request for an application instance to Gatt */
/* Documentation: Capture that this api can be called as soon as a gatt Registration is done */
#if defined(CSR_BT_GATT_INSTALL_SERVER_LONG_WRITE_OFFSET) || defined(CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET)
CsrBool CsrBtGattConfigModeReqHandler(GattMainInst *inst)
{
    CsrBtGattConfigModeReq  *prim         = inst->msg;
    CsrBtResultCode         result        = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
    CsrBtGattAppElement     *appElement   = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst,
                                                                        &prim->gattId);

    if (appElement)
    {
        if (prim->flags > CSR_BT_GATT_FEATURE_FLAGS_NONE && prim->flags <= CSR_BT_GATT_OPERATION_NEW_MODE)
        {
            /* Append the modes in appElement for selecting the procedures(legacy or new method) */
            appElement->flags = appElement->flags | prim->flags;
            result = CSR_BT_GATT_RESULT_SUCCESS;
        }
    }

    /* Send result to the application */
    CsrBtGattStdCfmSend(CSR_BT_GATT_CONFIG_MODE_CFM,
                        prim->gattId,
                        result,
                        CSR_BT_GATT_GET_SUPPLIER(result));
    return TRUE;
}
#endif
