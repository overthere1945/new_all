/******************************************************************************
 Copyright (c) 2010-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #20 $
******************************************************************************/

#include "csr_bt_gatt_private.h"
#if defined(CSR_TARGET_PRODUCT_IOT) || defined(CSR_TARGET_PRODUCT_WEARABLE)
#include "dm_private.h"
#endif

#ifdef GATT_DATA_LOGGER
#include "gatt_data_logger.h"
#endif /* GATT_DATA_LOGGER */

#define INVALID_HANDLE 0xFF


static CsrBool csrBtGattValidateQueueMsgState(att_result_t result, CsrUint8 queueMsgState)
{
    if (queueMsgState == CSR_BT_GATT_MSG_QUEUE_CANCELLED)
    { /* The procure is cancelled */
        if (result == ATT_RESULT_SUCCESS_MORE ||
            result == ATT_RESULT_SUCCESS_SENT)
        { /* ATT need to send more messages to GATT before
             this procedure can be stop */
            return FALSE;
        }
    }
    return TRUE;
}

static CsrBool csrBtGattGetResultCode(CsrUint8              queueMsgState,
                                      att_result_t          result,
                                      CsrBtResultCode       *resultCode,
                                      CsrBtSupplier         *resultSupplier)
{
    *resultCode     = CSR_BT_GATT_RESULT_SUCCESS;
    *resultSupplier = CSR_BT_SUPPLIER_GATT;

    switch (result)
    {
        case ATT_RESULT_SUCCESS:
            {
                if (queueMsgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
                {
                    return TRUE;
                }
                break;
            }
        case ATT_RESULT_SUCCESS_MORE:
            /* fallthrough */
        case ATT_RESULT_SUCCESS_SENT:
            {
                return TRUE;
            }
        case ATT_RESULT_ATTR_NOT_FOUND:
            {
                if (queueMsgState != CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK)
                {
                    *resultCode      = (CsrBtResultCode) result;
                    *resultSupplier  = CSR_BT_SUPPLIER_ATT;
                }
                break;
            }
        default:
            {
                *resultCode      = (CsrBtResultCode) result;
                *resultSupplier  = CSR_BT_SUPPLIER_ATT;
                break;
            }
    }
    return FALSE;
}

#ifdef GATT_CACHING_CLIENT_ROLE
static void gattClearHashVariables(CsrCmnListElm_t* elem, void* value)
{
    CsrBtGattConnElement *conn = (CsrBtGattConnElement*)elem;
    CsrBtTypedAddr *addr = (CsrBtTypedAddr*)value;

    if (conn && addr)
    {
        if (CsrBtAddrEq(&(conn->peerAddr), addr))
        {
            conn->pendingHash = FALSE;
        }
    }
}

static void enableServiceChangeCcd(GattMainInst* inst, CsrBtGattConnElement* conn)
{
    uint8* value = (uint8*)CsrPmemAlloc(GATT_WRITE_SERVICE_CHANGED_CCCD_SIZE * sizeof(CsrUint8));

    value[0] = GATT_SERVICE_CHANGED_CCCD;
    value[1] = 0x00;

    /* Enable the ccd for service changed indication to get the service changed indication from remote */
    CsrBtGattWriteReqSend(inst->privateGattId,
                            conn->btConnId,
                            conn->serviceChangeHandle+1,
                            0,
                            GATT_WRITE_SERVICE_CHANGED_CCCD_SIZE,
                            value);
}

static void serviceChangedHandler(GattMainInst* inst, CsrBtGattConnElement* conn, CsrUint16 handle)
{
    /* Check the indication for service changed */
    if (conn && conn->serviceChangeHandle == handle)
    {
        CsrBtGattConnElement* conn1 = SecondTransportConn(inst, conn);

        /* Send remote database modification status event to subscribed applications */
        if (conn->pendingHash != TRUE)
        {
            conn->pendingHash = TRUE;
            CsrBtGattSendOutOfSyncEventToApps(inst, conn);
        }

        if (CSR_BT_GATT_IS_CLIENT_ROBUST_CACHING_ENABLED(conn->connFlags))
        {
            HashHandler(inst, conn);
        }
        else
        {
            /* Else case as remote won't support robust caching sending the service changed alone will make 
             * the device data base change aware */
            conn->pendingHash = FALSE;
            CsrBtGattSendOutOfSyncEventToApps(inst, conn);
            return;
        }

        /* Mark the service changed indication served now */
        conn->serviceChangedIndState = SERVICE_CHANGED_INDICATION_SERVED;
        if (conn1)
        {
            conn1->serviceChangedIndState = SERVICE_CHANGED_INDICATION_SERVED;
        }
    }
}

static void serviceChangedReadCfmHandler(GattMainInst* inst, ATT_READ_BY_TYPE_CFM_T *prim)
{
    /* GATT were looking the Service Changed Characteristic Value. Note,
       if GATT is reading the Service Changed Characteristic
       and the result is SUCCESS the peer server do not follow the spec
       as the Service Changed Characteristic Value shall be set to Not Readable.*/
    if ((prim->result == ATT_RESULT_READ_NOT_PERMITTED || prim->result == ATT_RESULT_SUCCESS) &&
        prim->handle != CSR_BT_GATT_ATTR_HANDLE_INVALID)
    {
        CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &prim->cid);

        /* Store the service changed handle in conn */
        if (conn)
        {
            conn->serviceChangeHandle = prim->handle;
            enableServiceChangeCcd(inst, conn);
        }
    }
}

static void hashReadCfmHandler(GattMainInst* inst, ATT_READ_BY_TYPE_CFM_T* prim)
{
    if (prim->result == ATT_RESULT_SUCCESS)
    {
        CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &prim->cid);
        CsrBtGattConnElement* conn1 = SecondTransportConn(inst, conn);

        if (conn->serviceChangeHandle 
            && prim->handle != CSR_BT_GATT_ATTR_HANDLE_INVALID
            && !CSR_BT_GATT_IS_CLIENT_ROBUST_CACHING_ENABLED(conn->connFlags))
        {
            CSR_BT_GATT_SET_CLIENT_ROBUST_CACHING(conn->connFlags);
        }

        if (conn && conn->pendingHash)
        {
            CsrCmnListIterate(&inst->connInst, gattClearHashVariables, &conn->peerAddr);

            CsrBtFlushGattQueuedMsg(inst, conn->btConnId);
            if (conn1)
            {
                CsrBtFlushGattQueuedMsg(inst, conn1->btConnId);
            }

            /* Send connection event to subscribed applications */
            CsrBtGattSendOutOfSyncEventToApps(inst, conn);
        }
    }
}
#endif /* GATT_CACHING_CLIENT_ROLE */

static CsrBool csrBtGattAttDiscoverCharacHandler(GattMainInst               *inst,
                                                 CsrBtGattQueueElement      *qElem,
                                                 CsrBtGattDiscoverCharacReq *reqMsg,
                                                 ATT_READ_BY_TYPE_CFM_T     *prim)
{
    CsrBtResultCode resultCode;
    CsrBtSupplier   resultSupplier;
    CSR_UNUSED(inst);

    if (csrBtGattGetResultCode(qElem->msgState,
                               prim->result,
                               &resultCode,
                               &resultSupplier))
    {
        if (prim->size_value == CSR_BT_GATT_CHARAC_DECLARATION_MIN_LENGTH ||
            prim->size_value == CSR_BT_GATT_CHARAC_DECLARATION_MAX_LENGTH)
        {
            CsrBtUuid uuid;
            uuid.length = (CsrUint8) (prim->size_value - CSR_BT_GATT_CHARAC_PROPERTIES_LENGTH - CSR_BT_GATT_CHARAC_VALUE_HANDLE_LENGTH);
            CsrMemSet(uuid.uuid, 0, CSR_BT_UUID128_SIZE);
            SynMemCpyS(uuid.uuid, CSR_BT_UUID128_SIZE, &prim->value[CSR_BT_GATT_CHARAC_UUID_FIRST_INDEX], uuid.length);

            if (reqMsg->uuid.length == CSR_BT_UUID16_SIZE &&
                CSR_BT_UUID_GET_16(reqMsg->uuid) == CSR_BT_GATT_UUID_CHARACTERISTIC_DECL)
            { /* Discovers all Characteristics of a service,
                 Send result to the application */
                CsrBtGattDiscoverCharacIndSend(qElem,
                                               reqMsg->btConnId,
                                               prim->handle,
                                               uuid,
                                               prim->value);
            }
            else if (reqMsg->uuid.length == uuid.length &&
                     !CsrMemCmp(reqMsg->uuid.uuid, uuid.uuid, uuid.length))
            { /* Discover Characteristics by UUID, where the UUID
                 do match. Send result to the application */
                CsrBtGattDiscoverCharacIndSend(qElem,
                                               reqMsg->btConnId,
                                               prim->handle,
                                               uuid,
                                               prim->value);

            }
            /* Else - Discover Characteristics by UUID where the UUID
               don't match, just ignore */
        }
        /* Else - Invalid length, just ignore */

        if (prim->handle + 1 <= reqMsg->endGroupHandle)
        {
            return TRUE;
        }
    }

    if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
    {
        if (prim->result == ATT_RESULT_SUCCESS &&
            qElem->msgState != CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK)
        {
            resultCode     = (CsrBtResultCode) ATT_RESULT_ATTR_NOT_FOUND;
            resultSupplier = CSR_BT_SUPPLIER_ATT;
        }
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_DISCOVER_CHARAC_CFM,
                                    qElem->gattId,
                                    resultCode,
                                    resultSupplier,
                                    reqMsg->btConnId);
    }
    /* Else - This message has sent to the application when GATT
       received CSR_BT_GATT_CANCEL_REQ */
    return FALSE;
}

static CsrBool csrBtGattAttFindInclServiceHandler(CsrBtGattQueueElement  *qElem,
                                                  CsrBtGattHandle        endGroupHandle,
                                                  ATT_READ_BY_TYPE_CFM_T *prim,
                                                  CsrBtGattHandle        *inclHandle)
{
    CsrBtResultCode resultCode;
    CsrBtSupplier   resultSupplier;

    *inclHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;

    if (csrBtGattGetResultCode(qElem->msgState,
                               prim->result,
                               &resultCode,
                               &resultSupplier))
    {
        if (prim->size_value == CSR_BT_GATT_INCLUDE_WITH_UUID_LENGTH)
        { /* The service UUID must be a 16-bit Bluetooth UUID */
            CsrBtGattFindInclServicesIndSend(qElem,
                                             prim->handle,
                                             prim->size_value,
                                             prim->value);
            if (prim->handle + 1 <= endGroupHandle)
            { /* Read next */
                return TRUE;
            }
        }
        else if (prim->size_value >= CSR_BT_GATT_INCLUDE_WITHOUT_UUID_LENGTH &&
                 prim->result == ATT_RESULT_SUCCESS)
        { /* The service UUID must be a 128-bit Bluetooth UUID */
            qElem->attrHandle   = prim->handle;
            qElem->data         = (CsrUint8 *) CsrPmemZalloc(CSR_BT_GATT_INCLUDE_128_BIT_LENGTH);
            qElem->dataOffset   = CSR_BT_GATT_INCLUDE_WITHOUT_UUID_LENGTH;
            SynMemCpyS(qElem->data, CSR_BT_GATT_INCLUDE_128_BIT_LENGTH, prim->value, CSR_BT_GATT_INCLUDE_WITHOUT_UUID_LENGTH);
            *inclHandle = CSR_BT_GATT_GET_HANDLE(prim->value, CSR_BT_GATT_INCLUDE_START_HANDLE_INDEX);
            return FALSE;
        }
        else
        { /* The peer device include definition declaration
             don't follow the GATT specification. */
            if (prim->handle + 1 <= endGroupHandle)
            { /* Read next, this result is ignore */
                return TRUE;
            }
            else if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK)
            { /* The peer device has only one include definition declaration
                 which don't follow the spec return Error */
                resultCode      = (CsrBtResultCode)ATT_RESULT_INVALID_PDU;
                resultSupplier  = CSR_BT_SUPPLIER_ATT;
            }
        }
    }

    if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
    {
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_FIND_INCL_SERVICES_CFM,
                                    qElem->gattId,
                                    resultCode,
                                    resultSupplier,
                                    qElem->btConnId);
    }
    /* Else - This message has sent to the application when GATT
       received CSR_BT_GATT_CANCEL_REQ */
    return FALSE;
}

CsrUint32 gattInclUuid[4] = {0x00002802, 0, 0, 0};

static CsrBool csrBtGattAttReadByTypeFindInclServiceHandler(CsrBtGattQueueElement  *qElem,
                                                            ATT_READ_BY_TYPE_CFM_T *prim, 
                                                            CsrBtGattConnElement   *conn)
{
    CsrBtGattHandle inclHandle;
    CsrBtGattFindInclServicesReq *reqMsg = (CsrBtGattFindInclServicesReq *) qElem->gattMsg;

    if (csrBtGattAttFindInclServiceHandler(qElem,
                                           reqMsg->endGroupHandle,
                                           prim,
                                           &inclHandle))
    {
        if (prim->result == ATT_RESULT_SUCCESS)
        {
            attlib_read_by_type_req(CSR_BT_GATT_IFACEQUEUE,
                                    prim->cid,
                                    (CsrUint16)(prim->handle + 1),
                                    reqMsg->endGroupHandle,
                                    ATT_UUID16,
                                    (uint32_t *) &gattInclUuid[0],
                                    NULL);

            CsrBtGattEattSetCid(conn, prim->cid);
        }
        /* Else - Wait for another ATT_READ_BY_TYPE_CFM message */
    }
    else
    {
        if (inclHandle == CSR_BT_GATT_ATTR_HANDLE_INVALID)
        { /* Return TRUE to indicate that this procedure is finish */
            return TRUE;
        }
        else
        { /* The service UUID must be a 128-bit Bluetooth UUID,
             used the Read Request to get it                */
            attlib_read_req(CSR_BT_GATT_IFACEQUEUE,
                            prim->cid,
                            inclHandle,
                            NULL);

            CsrBtGattEattSetCid(conn, prim->cid);
        }
    }
    return FALSE;
}

static CsrBool csrBtGattAttReadCfmFindInclServiceHandler(CsrBtGattQueueElement *qElem,
                                                                              ATT_READ_CFM_T        *prim,
                                                                              CsrBtGattConnElement  *conn)
{
    CsrBtGattFindInclServicesReq *reqMsg = (CsrBtGattFindInclServicesReq *) qElem->gattMsg;

    if (prim->result == ATT_RESULT_SUCCESS &&
        qElem->dataOffset + prim->size_value == CSR_BT_GATT_INCLUDE_128_BIT_LENGTH)
    {
        SynMemCpyS(&(qElem->data[qElem->dataOffset]), prim->size_value, prim->value, prim->size_value);
        CsrBtGattFindInclServicesIndSend(qElem,
                                         qElem->attrHandle,
                                         CSR_BT_GATT_INCLUDE_128_BIT_LENGTH,
                                         qElem->data);
    }


    if (qElem->attrHandle + 1 <= reqMsg->endGroupHandle)
    {
        attlib_read_by_type_req(CSR_BT_GATT_IFACEQUEUE,
                                prim->cid,
                                (CsrUint16)(qElem->attrHandle + 1),
                                reqMsg->endGroupHandle,
                                ATT_UUID16,
                                (uint32_t *) &gattInclUuid[0],
                                NULL);
        /* Must free qElem->data and set it to NULL as it may be
           re-used later */
        CsrPmemFree(qElem->data);
        qElem->data = NULL;
        qElem->attrHandle = CSR_BT_GATT_ATTR_HANDLE_INVALID;

        CsrBtGattEattSetCid(conn, prim->cid);
    }
    else
    { /* This procedure is finish. Start the next if any and
         send a confirm message to the application */
        if (qElem->msgState == CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK)
        {
            CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_FIND_INCL_SERVICES_CFM,
                                        qElem->gattId,
                                        CSR_BT_GATT_RESULT_SUCCESS,
                                        CSR_BT_SUPPLIER_GATT,
                                        qElem->btConnId);
        }
        else
        {
            CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_FIND_INCL_SERVICES_CFM,
                                        qElem->gattId,
                                        (CsrBtResultCode) ATT_RESULT_ATTR_NOT_FOUND,
                                        CSR_BT_SUPPLIER_ATT,
                                        qElem->btConnId);
        }
        /* Return TRUE to indicate that this procedure is finish */
        return TRUE;
    }
    return FALSE;
}

static CsrBool csrBtGattAttPrimaryServiceHandler(CsrBtGattQueueElement *qElem,
                                                 att_result_t          result,
                                                 CsrUint16             handle,
                                                 CsrUint16             end,
                                                 CsrUint16             length,
                                                 CsrUint8              *value)
{
    CsrBtResultCode resultCode;
    CsrBtSupplier   resultSupplier;

    if (csrBtGattGetResultCode(qElem->msgState,
                               result,
                               &resultCode,
                               &resultSupplier))
    {
        CsrBtGattDiscoverServicesIndSend(qElem,
                                         handle,
                                         end,
                                         length,
                                         value);
        if (end < CSR_BT_GATT_ATTR_HANDLE_MAX)
        {
            return TRUE;
        }
    }

    if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
    {
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_DISCOVER_SERVICES_CFM,
                                    qElem->gattId,
                                    resultCode,
                                    resultSupplier,
                                    qElem->btConnId);
    }
    /* Else - This message has sent to the application when GATT
       received CSR_BT_GATT_CANCEL_REQ */
    return FALSE;
}

static CsrBool csrBtGattAttFindInfoUuidCheck(CsrBtGattQueueElement *qElem,
                                             att_uuid_type_t       uuidType,
                                             CsrUint32             attUuid)
{
    if (qElem->msgState == CSR_BT_GATT_MSG_QUEUE_IGNORE_CHARAC_DESCRIPTOR)
    {
        return FALSE;
    }
    else
    {
        if (uuidType == ATT_UUID16)
        {
            if (attUuid == CSR_BT_GATT_UUID_PRIMARY_SERVICE_DECL)
            { /* Include characteristic declaration as a part of upstream cfm as well */
                qElem->msgState = CSR_BT_GATT_MSG_QUEUE_IGNORE_CHARAC_DESCRIPTOR;
                return FALSE;
            }
        }
    }
    return TRUE;
}

static CsrBool csrBtGattAttDiscoverCharacDescriptorsHandler(CsrBtGattQueueElement *qElem,
                                                            CsrBtGattHandle       endGroupHandle,
                                                            ATT_FIND_INFO_CFM_T   *prim)
{
    CsrBtResultCode resultCode;
    CsrBtSupplier   resultSupplier;

    if (csrBtGattGetResultCode(qElem->msgState,
                               prim->result,
                               &resultCode,
                               &resultSupplier))
    {
        if (csrBtGattAttFindInfoUuidCheck(qElem, prim->uuid_type, prim->uuid[0]))
        {
            CsrBtGattDiscoverCharacDescriptorsIndSend(qElem,
                                                      prim->handle,
                                                      prim->uuid_type,
                                                      (CsrUint32 *) prim->uuid);

            if (prim->handle + 1 <= endGroupHandle)
            { /* Read next */
                return TRUE;
            }
        }
        else
        {
            if (prim->result == ATT_RESULT_SUCCESS_MORE)
            {
                return TRUE;
            }
            /* Else - prim->result must be ATT_RESULT_SUCCESS */
        }
    }

    if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
    {
        if (qElem->msgState == CSR_BT_GATT_MSG_QUEUE_IGNORE_CHARAC_DESCRIPTOR)
        {
            resultCode     = CSR_BT_GATT_RESULT_INVALID_HANDLE_RANGE;
            resultSupplier = CSR_BT_SUPPLIER_GATT;
        }
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM,
                                    qElem->gattId,
                                    resultCode,
                                    resultSupplier,
                                    qElem->btConnId);
    }
    /* Else - This message has sent to the application when GATT
       received CSR_BT_GATT_CANCEL_REQ */
    return FALSE;
}

static CsrBool csrBtGattAttReadCfmReadByUuidHandler(GattMainInst           *inst,
                                                    CsrBtGattQueueElement  *qElem,
                                                    CsrUint16              mtu,
                                                    CsrBtGattHandle        endGroupHandle,
                                                    ATT_READ_BY_TYPE_CFM_T *prim,
                                                    CsrBool                *readBlob,
                                                    CsrBool                *readDescriptor)
{
    CsrBtResultCode resultCode;
    CsrBtSupplier   resultSupplier;
    
    CsrBtGattReadByUuidReq *reqMsg = (CsrBtGattReadByUuidReq *) qElem->gattMsg;
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
    CsrBool         remoteNameRead = FALSE;
#endif
    *readBlob                      = FALSE;
    *readDescriptor                = FALSE; 

#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
    if (reqMsg->uuid.length == CSR_BT_UUID16_SIZE && 
        CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_DEVICE_NAME_CHARAC)
    { /* The Remote Name Characteristic Value has been read */
        remoteNameRead = TRUE;
    }
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

    if (prim->size_value > CSR_BT_GATT_ATTR_VALUE_LEN_MAX)
    {
        prim->result = ATT_RESULT_INVALID_LENGTH;
    }

    if (csrBtGattGetResultCode(qElem->msgState,
                               prim->result,
                               &resultCode,
                               &resultSupplier))
    {
        if (prim->result == ATT_RESULT_SUCCESS &&
            prim->size_value >= (mtu - CSR_BT_GATT_ATT_READ_BY_TYPE_HEADER_LENGTH) &&
            prim->size_value < CSR_BT_GATT_ATTR_VALUE_LEN_MAX)
        {
            *readBlob = TRUE;
            return TRUE;
        }
        else
        {
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
            if (remoteNameRead)
            { /* The Remote Name Characteristic Value has been read, Update saved named */
                CSR_BT_GATT_CONN_INST_UPDATE_REMOTE_NAME(inst->connInst,
                                                         prim->cid,
                                                         prim->size_value,
                                                         prim->value);  
            }
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

            if (qElem->gattId != inst->privateGattId)
            {
                qElem->msgState = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK;
                CsrBtGattReadByUuidIndSend(qElem->gattId,
                                           qElem->btConnId,
                                           prim->handle,
                                           prim->size_value,
                                           prim->value);
                prim->value = NULL;
                
                if (prim->handle + 1 <= endGroupHandle)
                {
                    return TRUE;
                }
            }
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
            else
            { /* GATT have called ATT_READ_BY_TYPE_REQ */
                if (remoteNameRead)
                { /* In order to read the read the remote name */
                     if (qElem->dataElemIndex > 0)
                     { /* SC has requested to read the name, 
                          before before GATT had time to read it. 
                          Send Result to SC */
                        CsrBtGattReadRemoteLeNameCfmSend((CsrSchedQid)qElem->dataElemIndex, 
                                                          prim->size_value, 
                                                          prim->value);
                        prim->value = NULL;
                     }
                }
                /* Else - In order to find the attribute Handle of the Service Changed Characteristic Value */
            }
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */
        }
    }

    if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
    {
        if (qElem->gattId != inst->privateGattId)
        {

#ifdef GATT_CACHING_CLIENT_ROLE
            hashReadCfmHandler(inst, prim);
#endif /* GATT_CACHING_CLIENT_ROLE */

            CsrBtGattReadByUuidCfmSend(qElem->gattId,
                                       resultCode,
                                       resultSupplier,
                                       qElem->btConnId,
                                       &(reqMsg->uuid));
        }
        else
        { /* GATT have called ATT_READ_BY_TYPE_REQ in order
             to either find the attribute Handle of the Service Changed
             Characteristic Value or to read the remote name */
#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
            if (reqMsg->uuid.length == CSR_BT_UUID16_SIZE &&
                CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_CENTRAL_ADDRESS_RESOLUTION_CHARAC)
            {
                if (prim->result == ATT_RESULT_SUCCESS)
                {
                    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &prim->cid);

                    if (conn && prim->value)
                    {
                        CsrBtTdDbGattInfo info = { 0 };

                        if (CsrBtTdDbGetGattInfo(conn->peerAddr.type,
                                                 &conn->peerAddr.addr,
                                                 &info) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
                        {
                            info.carValue = *prim->value;
                            CsrBtTdDbSetGattInfo(conn->peerAddr.type,
                                                 &conn->peerAddr.addr,
                                                 &info);
                        }
                    }
                }
            }
            else if (reqMsg->uuid.length == CSR_BT_UUID16_SIZE &&
                     CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_RESOLVABLE_PRIVATE_ADDRESS_ONLY_CHARAC)
            {
                CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &prim->cid);
                if (conn)
                {
                    CsrUint8 value;

                    if ((prim->result == ATT_RESULT_SUCCESS) && prim->value)
                    {
                        value = *prim->value;
                    }
                    else
                    {
                        value = CSR_BT_GATT_RPA_ONLY_VALUE_INVALID;
                    }

#if defined(CSR_TARGET_PRODUCT_VM) || defined(CSR_TARGET_PRODUCT_WEARABLE)
                    CsrBtGattReadRemoteRpaOnlyCharCfmSend(CSR_BT_CM_IFACEQUEUE,
                                                          value,
                                                          conn->peerAddr,
                                                          resultCode,
                                                          resultSupplier);
#else
                    CsrBtGattReadRemoteRpaOnlyCharCfmSend(CSR_BT_SC_IFACEQUEUE,
                                                          value,
                                                          conn->peerAddr,
                                                          resultCode,
                                                          resultSupplier);

#endif
                }
            }
#ifdef CSR_BT_GATT_INSTALL_EATT
            else if (reqMsg->uuid.length == CSR_BT_UUID16_SIZE &&
                     CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_SERVER_SUPPORTED_FEATURES_CHARAC)
            {
                 if (prim->result == ATT_RESULT_SUCCESS)
                 {
                     CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &prim->cid);
                     if (conn && (prim->value))
                     {
                         /* Check for the server supported feature value currently its one bit but in future
                            when other fields are present then need to check the first bit for EATT */
                         if (*prim->value)
                         {
                             CSR_BT_GATT_SET_SSF(conn->connFlags);
                         }
                     }
                 }
            }
#ifdef GATT_CACHING_CLIENT_ROLE
            else if (reqMsg->uuid.length == CSR_BT_UUID16_SIZE &&
                (CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_DATABASE_HASH_CHARAC ||
                 CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_SERVICE_CHANGED_CHARAC))
            {
                 if (CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_SERVICE_CHANGED_CHARAC)
                 {
                     serviceChangedReadCfmHandler(inst, prim);
                 }
                 else
                 {
                     hashReadCfmHandler(inst, prim);
                 }
            }
#endif /* GATT_CACHING_CLIENT_ROLE */
            else if (reqMsg->uuid.length == CSR_BT_UUID16_SIZE &&
                CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_CLIENT_SUPPORTED_FEATURES_CHARAC)
            { /* Write local CSF vlaue if Client Supported Features Characteristics present in remote GATT server */
                if (prim->result == ATT_RESULT_SUCCESS)
                {
                    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &prim->cid);
                    if (conn)
                    {
                        CsrUint8  *value;
                        CsrUint16 length = CSR_BT_GATT_CSF_VALUE_LENGTH;
                        value  = (CsrUint8 *) CsrPmemAlloc(length);
                        value[0] = CSR_BT_GATT_CSF_MULTI_HANDLE_VALUE_NTF_ENABLE | CSR_BT_GATT_CSF_EATT_ENABLE;
#ifdef GATT_CACHING_CLIENT_ROLE
                        /* Enable EATT, MHVN and Robust Caching by setting the value to 7(Enable bit 0,1,2), Refer core spec */
                        value[0] |= CSR_BT_GATT_CSF_ROBUST_CACHING_ENABLE;
#endif /* GATT_CACHING_CLIENT_ROLE */

                        /* Do the write operation to inform other side about local CSF */
                        CsrBtGattWriteReqSend(reqMsg->gattId,
                                            reqMsg->btConnId,
                                                prim->handle,
                                                           0,
                                                      length,
                                                      value);
                    }
                }
            }
#endif /* CSR_BT_GATT_INSTALL_EATT */
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
            /* Else - failed to Read the Remote Name Characteristic Value */
        }
    }
    /* Else - This message has sent to the application when GATT
              received CSR_BT_GATT_CANCEL_REQ */
    return FALSE;
}

static CsrBool csrBtGattAttReadByTypeReadByUuidHandler(GattMainInst           *inst,
                                                       CsrBtGattQueueElement  *qElem,
                                                       ATT_READ_BY_TYPE_CFM_T *prim,
                                                       CsrUint16               mtu,
                                                       CsrBtGattConnElement   *conn)
{
    CsrBool readBlob;
    CsrBool readDescriptor;
    CsrBtGattReadByUuidReq *reqMsg = (CsrBtGattReadByUuidReq *) qElem->gattMsg;

    if (csrBtGattAttReadCfmReadByUuidHandler(inst,
                                             qElem,
                                             mtu,
                                             reqMsg->endGroupHandle,
                                             prim,
                                             &readBlob,
                                             &readDescriptor))
    {
        if (prim->result == ATT_RESULT_SUCCESS)
        {
            if (readBlob)
            {
                qElem->data = (CsrUint8 *) CsrPmemZalloc(CSR_BT_GATT_LONG_READ_ATTR_MAX_VALUE);
                qElem->dataOffset = prim->size_value;
                qElem->attrHandle = prim->handle;
                SynMemCpyS(qElem->data, CSR_BT_GATT_LONG_READ_ATTR_MAX_VALUE, prim->value, prim->size_value);
                /* Else - Security have passed on the given handle, do not set
                 qElem->securityFunc avoid retry deadlock */

                attlib_read_blob_req(CSR_BT_GATT_IFACEQUEUE,
                                     prim->cid,
                                     qElem->attrHandle,
                                     qElem->dataOffset,
                                     NULL);

                CsrBtGattEattSetCid(conn, prim->cid);
            }
            else
            {
                CsrUint32       attUuid[4];
                att_uuid_type_t uuidType;
                CsrBtGattGetAttUuid(reqMsg->uuid, attUuid, &uuidType);
                
                /* Note, in order to be able to handle that the server requires 
                   security on the next handle being read, GATT shall set the 
                   security callback function again, because the function 
                   csrBtGattScLeSecurityCfmHandler set it to NULL and GATT 
                   shall allow one security procedure per handle being read.
                   The qElem->attrHandle is set to
                   prim->handle + 1 so GATT is able replay this command in 
                   case of security failure */
                qElem->attrHandle   = (CsrUint16)(prim->handle + 1);
                attlib_read_by_type_req(CSR_BT_GATT_IFACEQUEUE,
                                        prim->cid,
                                        qElem->attrHandle,
                                        reqMsg->endGroupHandle,
                                        uuidType,
                                        (uint32_t *) attUuid,
                                        NULL);

                CsrBtGattEattSetCid(conn, prim->cid);
            }
        }
        /* Else - Wait for another ATT_READ_BY_TYPE_CFM message */
    }
    else
    { 
        if (!readDescriptor)
        { /* Indicates that this procedure is finish */
            return TRUE;
        }
        /* Else - Indicates that the procedure is not finish. E.g. it is trying to discover
                  the Client Configuration Descriptors as part of setting the Service Change */
    }
    return FALSE;
}

static CsrBool csrBtGattValidatePrepareWriteData(CsrUint16               writeFlags,
                                                 ATT_PREPARE_WRITE_CFM_T *prim,
                                                 CsrUint8                *srcValue)
{
    if (writeFlags == CSR_BT_GATT_WRITE_RELIABLE)
    {
        if(CsrMemCmp(srcValue, prim->value, prim->size_value))
        {
            return FALSE;
        }
    }
    return TRUE;
}


#ifndef EXCLUDE_CSR_BT_CM_MODULE
/* Public ATT upstream handlers */
void CsrBtGattAttRegisterCfmHandler(GattMainInst *inst)
{ /* Received an ATT subsystem register confirmation */
    ATT_REGISTER_CFM_T *prim = (ATT_REGISTER_CFM_T *) inst->msg;

    if (prim->result == ATT_RESULT_SUCCESS)
    {
        /* Ready to work with ATT */
    }
    else
    {
        CSR_LOG_TEXT_CRITICAL((CsrBtGattLto, 0, "AttRegisterCfm Failed"));
    }
}

#ifdef CSR_BT_GATT_INSTALL_EATT
void CsrBtGattEattRegisterCfmHandler(GattMainInst *inst)
{   /* Received an EATT subsystem register confirmation */
    ATT_ENHANCED_REGISTER_CFM_T *prim = (ATT_ENHANCED_REGISTER_CFM_T *) inst->msg;

    if (prim->result == ATT_RESULT_SUCCESS)
    {
        /* Ready to work with EATT */
    }
    else
    {
        CSR_LOG_TEXT_CRITICAL((CsrBtGattLto, 0, "ATT_REGISTER_CFM Failed"));
    }
}

void CsrBtGattEattConnectCfmHandler(GattMainInst *inst)
{
    ATT_ENHANCED_CONNECT_CFM_T *cfm = inst->msg;
    CsrBtGattConnElement *conn      = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
                                                                              CsrBtGattFindConnInstFromAddressFlagsLe,
                                                                              &(cfm->tp_addrt.addrt));

    /* Store the EATT cid in conn entry when the result is success or partial sucess and cid success is non zero */
    if (cfm->result == L2CA_CONNECT_SUCCESS || (cfm->result == L2CA_CONNECT_REJ_RESOURCES && cfm->num_cid_success !=0))
    {
        if (conn)
        {
            CsrUint8 i;
            CsrUint8 cidIndex;
            /* Store the Eatt Connection parameter to conn element */
            /* Check for cfm mtu before overriding it, if buggy remote device has sent zero mtu/less than EATT_MTU_MIN will leads to isseus */
            if (cfm->mtu >= EATT_MTU_MIN)
            {
                conn->mtu = conn->mtu < cfm->mtu ? (conn->mtu):(cfm->mtu);
            }

            conn->l2capFlags     = cfm->flags;
            conn->eattConnection = TRUE;

            /* Check if the confirmation is for local initiated EATT channel by checking the flags */
            if (!(cfm->flags & L2CA_CONFLAG_INCOMING))
                conn->localInitiated = LOCAL_EATT_SUCCESS;

            /* Store the cid count successfully created as part of Eatt cfm */
            conn->numCidSucess = conn->numCidSucess + cfm->num_cid_success;

            /* This will find the current index, this step is required to find the index
               From where we have to set the channel as free */
             cidIndex = conn->numCidSucess - cfm->num_cid_success;

            /* Store the successfully created CID to conn element and set the channel as free */
            for (i = cidIndex; i < conn->numCidSucess; i++)
            {
                /* Clear all the CID and mark it as available as EATT connection is sucessfully completed */
                conn->cidSuccess[i]  = cfm->cid_success[i - cidIndex];
                conn->numOfBearer[i] = CHANNEL_IS_FREE;
            }
            /* Send connection event to subscribed applications */
            CsrBtGattSendEattConnectEventToApps(inst, conn);
        }
        else
        {
            CsrGeneralException(CsrBtGattLto,
                                0,
                                CSR_BT_ATT_PRIM,
                                cfm->type,
                                0,
                                "ATT_CONNECT_CFM: Unable to find a bt conn id");
        }
    }
    else if((conn) && (cfm->result != L2CA_CONNECT_INITIATING))
    {
        /* Check if the confirmation is for local initiated EATT channel by checking the flags */
        /* Clear the local initiated state to idle if the locally initiated EATT connection was a failure. */
        if (!(cfm->flags & L2CA_CONFLAG_INCOMING))
            conn->localInitiated = LOCAL_EATT_IDLE;

    }
    else
    {
        CSR_LOG_TEXT_INFO((CsrBtGattLto, 0, "CsrBtGattEattConnectCfmHandler cfm->result: %x \n",cfm->result));
    }
}

void CsrBtGattEattConnectIndHandler(GattMainInst *inst)
{
    ATT_ENHANCED_CONNECT_IND_T *ind = inst->msg;
    CsrBtGattConnElement *conn      = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(inst->connInst,
                                                                              CsrBtGattFindConnInstFromAddressFlagsLe,
                                                                              &(ind->tp_addrt.addrt));

    /* This condition is added to make sure EATT connection is not already present
       If the EATT connection is already done, then reject the eatt psm */
    if (conn && conn->numCidSucess < EATT_BEARER_COUNT)
    {
        CsrUint16 noOfRemoteCid         = ind->num_cid;
        CsrUint8 localInitiatedEattConn = LOCAL_INITIATED_EATT_BEARER;
        CsrUint16 result                = L2CA_CONNECT_SUCCESS;
        CsrUint8 spaceLeft              = EATT_BEARER_COUNT - conn->numCidSucess;

        /* Of conn->numCidSucess need to account for locally initiated ones
         * If the local initiated is established then set the localInitiatedEattConn to zero 
         * In that case this number is already accounted under conn->numCidSucess */
        if (conn->localInitiated == LOCAL_EATT_SUCCESS)
        {
            localInitiatedEattConn = 0;
        }

        spaceLeft -= localInitiatedEattConn;

        /* If Remote device is trying to initiate eatt connection with
         * More than spaceLeft available then accept only NO_OF_EATT_BEARER */
        if (spaceLeft <= noOfRemoteCid)
        {
            /* Truncate the total incoming connection based on current space avalaible for total number of
             * EATT connection */
            noOfRemoteCid = spaceLeft;

            /* If number of CID is becoming lesser after all the calculation or GATT is truncating the remote EATT connections
             * to fewer, set the result to L2CA_CONNECT_REJ_RESOURCES */
            if (noOfRemoteCid < ind->num_cid)
                result = L2CA_CONNECT_REJ_RESOURCES;
        }

        CSR_LOG_TEXT_INFO((CsrBtGattLto, 0, "CsrBtGattEattConnectIndHandler ind->num_cid: %x spaceLeft :%x\n",ind->num_cid, spaceLeft));

        attlib_enhanced_connect_rsp(ind->phandle,
                                ind->identifier,
                                noOfRemoteCid,
                                result,
                                inst->preferredEattMtu,
                                INITIAL_CREDITS,
                                ZERO_PRIORITY,
                                NULL);

    }
    else
    {
        /* Reject the connection if remote device is intiating again the Eatt connection 
           When there Max EATT connection is already present */
        attlib_enhanced_connect_rsp(ind->phandle,
                                ind->identifier,
                                0x00,                        /* Resources constraint so reject all CIDs */
                                L2CA_CONNECT_REJ_RESOURCES,
                                inst->preferredEattMtu,
                                INITIAL_CREDITS,
                                ZERO_PRIORITY,
                                NULL);

    }
}
#endif /* CSR_BT_GATT_INSTALL_EATT */
#endif


#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
void CsrBtGattAttAddCfmHandler(GattMainInst *inst)
{
    ATT_ADD_CFM_T *prim          = (ATT_ADD_CFM_T *) inst->msg;
    CsrBtConnId btConnId         = CSR_BT_GATT_LOCAL_BT_CONN_ID;
    CsrBtGattQueueElement *qElem = CsrBtGattfindQueueElementbtConnId(inst, btConnId);

    if (qElem)
    {
        
        if (qElem->gattId == inst->privateGattId)
        {
            if (prim->result != ATT_RESULT_SUCCESS)
            { /* Unable to Register Mandatory GATT db doing the Init procedure */
            
                CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttAddCfm fail %d", prim->result));
            }
            /* Else - GATT has added mandatory attributes to the database */

#ifndef EXCLUDE_CSR_BT_CM_MODULE
            /* Request to get some LE conntroler informations */
            CsrBtCmLeGetControllerInfoReqSend(CSR_BT_GATT_IFACEQUEUE);
#else
            /* This procedure is finish. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, qElem);
#endif
        }
        else
        {
            /* Need to store prim->result in local variable as the
               function CsrBtGattSdsRequestHandler is freeing
               inst->msg */
            att_result_t result = prim->result;
        
           /* Send result to the application */
            CsrBtGattStdCfmSend(CSR_BT_GATT_DB_ADD_CFM,
                                qElem->gattId,
                                result,
                                CSR_BT_GATT_GET_SUPPLIER(result));

            /* This procedure is finish. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, qElem);
        }
    }
    else
    {
        CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttAddCfm qElem not found"));
    }
}

void CsrBtGattAttRemoveCfmHandler(GattMainInst *inst)
{ /* Confirmation to attribute removal from the data base.
     This function return FALSE if an exception has occurred */
    ATT_REMOVE_CFM_T      *prim  = (ATT_REMOVE_CFM_T *) inst->msg;
    CsrBtConnId btConnId         = CSR_BT_GATT_LOCAL_BT_CONN_ID;
    CsrBtGattQueueElement *qElem = CsrBtGattfindQueueElementbtConnId(inst, btConnId);

    if (qElem && qElem->gattMsg)
    { /* Sent the result to the application */
        CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &qElem->gattId);

        if (appElement)
        {
            CsrPrim type          = *(CsrPrim *)qElem->gattMsg;

            switch (type)
            {
                case CSR_BT_GATT_DB_REMOVE_REQ:
                { /* The application has requested GATT to remove
                     some handles from the data base */
                    if (prim->result == ATT_RESULT_SUCCESS)
                    {
                       /*Do nothing*/
                    }
                    CsrBtGattDbRemoveCfmSend(qElem->gattId,
                                             prim->result,
                                             CSR_BT_GATT_GET_SUPPLIER(prim->result),
                                             prim->num_attr);

                    /* This procedure is finish. Start the next if any */
                    CsrBtGattQueueRestoreHandler(inst, qElem);
                    break;
                }
                case CSR_BT_GATT_DB_DEALLOC_REQ:
                { /* GATT have made sure that the handles that the application
                     want to deallocate also is remove from the data base */
                    CsrBtGattDbDeallocCfmSend(qElem->gattId,
                                              CSR_BT_GATT_RESULT_SUCCESS,
                                              CSR_BT_SUPPLIER_GATT,
                                              appElement->start,
                                              appElement->end);

                    appElement->start = CSR_BT_GATT_ATTR_HANDLE_INVALID;
                    appElement->end   = CSR_BT_GATT_ATTR_HANDLE_INVALID;

                    CSR_BT_GATT_APP_INST_SORT_BY_ATTR_VALUE(inst->appInst);

                    /* This procedure is finish. Start the next if any */
                    CsrBtGattQueueRestoreHandler(inst, qElem);
                    break;
                }
                case CSR_BT_GATT_UNREGISTER_REQ:
                { /* The application is being unregister from GATT
                     Ensure that the scan settings are in sync with
                     remaining applications. */

                    CsrBtGattUnregisterApp(inst, appElement, qElem);
                    break;
                }
                default:
                {
                    CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttRemoveCfm invalid type %x", type));
                    break;
                }
            }
        }
        else
        {
            CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttRemoveCfm no app"));
        }
    }
    else
    {
        CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttRemoveCfm no qElem"));
    }
}
#else
void CsrBtGattAttAddDbCfmHandler(GattMainInst *inst)
{
    ATT_ADD_DB_CFM_T *prim       = (ATT_ADD_DB_CFM_T *) inst->msg;
    CsrBtConnId btConnId         = CSR_BT_GATT_LOCAL_BT_CONN_ID;
    CsrBtGattQueueElement *qElem = CsrBtGattfindQueueElementbtConnId(inst, btConnId);

    if (qElem)
    {
        CsrBtGattFlatDbRegisterReq *reqMsg = (CsrBtGattFlatDbRegisterReq *)qElem->gattMsg;
        att_result_t result = prim->result;

        if (result != ATT_RESULT_SUCCESS)
        { /* Unable to Register Mandatory db doing the Init procedure */
            CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttAddDbCfm Fail %d", result));
        }
#ifdef CSR_BT_GATT_CACHING
        else
        {
            CsrBtGattReadDbHashRestoreHandler(NULL, NULL, 0);
            return;
        }
#endif

        /* Send result to the application */
        CsrBtGattFlatDbRegisterCfmSend(reqMsg->pHandle,
                            result,
                            CSR_BT_GATT_GET_SUPPLIER(result));

        /* This procedure is finish. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, qElem);
    }
}
#endif

/* Handling ATT_CONNECT_IND and ATT_CONNECT_CFM events */
static void gattAttConnectEventHandler(GattMainInst *inst, 
                                       CsrBtGattConnElement *conn,
                                       CsrUint16 cid,
                                       CsrUint16 mtu,
                                       l2ca_conflags_t flags)
{
    CsrBtConnId btConnId = CSR_BT_CONN_ID_INVALID;
#ifdef GATT_CACHING_CLIENT_ROLE
    CsrBtGattHandle serviceChange;
#endif /* GATT_CACHING_CLIENT_ROLE */

    /* For LE connection either ATT_CONNECT_IND or ATT_CONNECT_CFM event get processed.
     * Valid btConnId indicates ATT_CONNECT_IND event already processed. */
    if (conn->btConnId != CSR_BT_CONN_ID_INVALID)
    { /* Skip processing of ATT_CONNECT_CFM event */
        return;
    }

    if (CsrBtGattFindFreeConnId(inst, &btConnId) != FALSE)
    {
        conn->btConnId      = btConnId;
        conn->mtu           = mtu;
        conn->cid           = cid;
        conn->l2capFlags    = flags;

        /* Store the fixed channel CID in Eatt list in the end and mark the channel is available */
        conn->cidSuccess[EATT_BEARER_COUNT]  = conn->cid;
        conn->numOfBearer[EATT_BEARER_COUNT] = CHANNEL_IS_FREE;

#ifdef CSR_BT_GATT_CACHING
        /* Check for GATT caching actions for new connection */
        CsrBtGattCachingNewConn(inst, conn);
#endif
        /* Send connection event to subscribed applications */
        CsrBtGattSendConnectEventToApps(inst, conn);

#if defined(CSR_BT_GATT_CACHING) || defined(CSR_BT_GATT_INSTALL_EATT) || (!defined(EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL))

        /* Check for the connflags to see SSF is already stored */
        CsrBtGattTdDbReadFeatureInfo(&conn->peerAddr,
                          &conn->connFlags);
#endif

#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
        /* Check for the configFlag to see Encrypted data key material is already stored */
        GattTdDbReadEncryptedDataKeyMaterialClientConfig(&conn->peerAddr,
                                                         &conn->connFlags);
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */

        if (conn->leConnection)
        {
#ifdef CSR_BT_GATT_INSTALL_EATT
            /* Query the server and client supported feature only for LE connection */
            if (!(CSR_BT_GATT_IS_SSF_ENABLED(conn->connFlags)))
            {
                CsrBtGattReadByUuidPrivateHandler(inst, CSR_BT_GATT_UUID_SERVER_SUPPORTED_FEATURES_CHARAC, conn->btConnId);
                CsrBtGattReadByUuidPrivateHandler(inst, CSR_BT_GATT_UUID_CLIENT_SUPPORTED_FEATURES_CHARAC, conn->btConnId);
            }
#endif /* CSR_BT_GATT_INSTALL_EATT */

#ifdef GATT_CACHING_CLIENT_ROLE
            /* Get the service changed handles from the PS */
            if (CsrBtGattGetPeerServiceChangeHandles(conn->peerAddr.type,
                &conn->peerAddr.addr,
                &serviceChange))
            {
                conn->serviceChangeHandle = serviceChange;
            }
#endif /* GATT_CACHING_CLIENT_ROLE */
        }

    }
    else
    {
        attlib_disconnect_req(CSR_BT_GATT_IFACEQUEUE, cid, NULL);

        CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttConnectInd/AttConnectCfm : Unable to find btconnid"));
    }
}

void CsrBtGattAttConnectCfmHandler(GattMainInst *inst)
{
    ATT_CONNECT_CFM_T *cfm = inst->msg;
    CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS(inst->connInst,
                                 CsrBtGattFindConnInstFromAddressFlags,
                                 cfm);
    CsrBtConnId searchBtConnId   = CSR_BT_GATT_LOCAL_BT_CONN_ID;
    CsrBtGattQueueElement *qElem = CsrBtGattfindQueueElementbtConnId(inst, searchBtConnId);

    if (cfm->result == L2CA_CONNECT_SUCCESS)
    {
        if(conn)
        {
            gattAttConnectEventHandler(inst, conn, cfm->cid, cfm->mtu, cfm->flags);

            if (!(conn->leConnection))
                CsrBtGattQueueRestoreHandler(inst, qElem);
        }
        else
        {
            attlib_disconnect_req(CSR_BT_GATT_IFACEQUEUE, cfm->cid, NULL);

            CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "No Connection. Redundant"));
        }
    }
    else 
    {
        if (!conn)
        {
            if (cfm->result == L2CA_CONNECT_INITIATING)
            /*connection is not present. Disassociate with ATT. */
            attlib_disconnect_req(CSR_BT_GATT_IFACEQUEUE, cfm->cid, NULL);
        }
#ifdef CSR_BT_INSTALL_GATT_BREDR
        else
        {
            if (!L2CA_CONFLAG_IS_LE(cfm->flags))
            {
                if (CSR_MASK_IS_SET(cfm->flags, L2CA_CONFLAG_INCOMING))
                { /* Incoming connection request rejected by Application or failed due to some other reason,
                     remove conn inst and send cfm to app */
                    CsrBtGattStdCfmSend(CSR_BT_GATT_ACCEPT_BREDR_CFM,
                                        inst->bredrAppHandle,
                                        CSR_BT_GATT_RESULT_CONNECTION_FAILED,
                                        CSR_BT_SUPPLIER_GATT);

                    CSR_BT_GATT_CONN_INST_REMOVE(inst->connInst, conn);
                    CsrBtGattQueueRestoreHandler(inst, qElem);
                }
                else
                { /* Connection failed with remote device (acceptor), remove conn inst and send cfm to app */
                    if (cfm->result != L2CA_CONNECT_INITIATING && cfm->result != L2CA_CONNECT_PENDING)
                    {
                        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_CONNECT_BREDR_CFM,
                                                    conn->gattId,
                                                    CSR_BT_GATT_RESULT_CONNECTION_FAILED,
                                                    CSR_BT_SUPPLIER_GATT,
                                                    CSR_BT_CONN_ID_INVALID);

                        CSR_BT_GATT_CONN_INST_REMOVE(inst->connInst, conn);
                        CsrBtGattQueueRestoreHandler(inst, qElem);
                    }
                }
            }
        }
#endif
    }
}

void CsrBtGattAttConnectIndHandler(GattMainInst *inst)
{
    ATT_CONNECT_IND_T *ind = (ATT_CONNECT_IND_T*)inst->msg;

    if (L2CA_CONFLAG_IS_LE(ind->flags))
    {
        CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_LE(inst->connInst,
                                     CsrBtGattFindConnInstFromAddressFlagsLe,
                                     &(ind->addrt));
        /* GATT connection element is created either on receiving CSR_BT_CM_LE_EVENT_CONNECTION_IND or 
         * ATT_CONNECT_IND. */
        if (!conn)
        {
            conn = CSR_BT_GATT_CONN_INST_ADD_LAST(inst->connInst);
            conn->peerAddr = ind->addrt;
        }

        gattAttConnectEventHandler(inst, conn, ind->cid, ind->mtu, ind->flags);
    }

#ifdef CSR_BT_INSTALL_GATT_BREDR
    else if (CSR_MASK_IS_SET(ind->flags, L2CA_CONFLAG_INCOMING))
    { /* A peer device request to setup an BR/EDR connection */
        CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_BREDR(inst->connInst,
                                     CsrBtGattFindConnInstFromAddressFlagsBredr,
                                     &(ind->addrt));
        if (!conn)
        { /* GATT is ready to accept an incoming BR/EDR connection.
         Accept it. Note and ATT_CONNECT_CFM is received afterwards */
            conn = CSR_BT_GATT_CONN_INST_ADD_LAST(inst->connInst);

            conn->peerAddr = ind->addrt;
            conn->cid = ind->cid;
            conn->leConnection = FALSE;

            if (inst->bredrAppHandle != CSR_SCHED_QID_INVALID)
            {
                CsrBtGattConnectBredrIndSend(inst->bredrAppHandle,
                                             &conn->peerAddr,
                                             conn->mtu);
            }
        }
        else
        { /* Race condition */
#ifdef INSTALL_ATT_BREDR
            attlib_connect_rsp(CSR_BT_GATT_IFACEQUEUE, conn->cid, L2CA_CONNECT_REJ_PSM, NULL);
#endif
        }
    }
#endif /* CSR_BT_INSTALL_GATT_BREDR */
}

void CsrBtGattAttQueueCleanupOnDisconnection(GattMainInst *inst, CsrBtConnId btConnId)
{
    CsrUint8 i;
    CsrBtGattQueueElement* qElem;

    for (i = 0; i < NO_OF_QUEUE; i++)
    {
        qElem = CSR_BT_GATT_QUEUE_FIND_BT_CONN_ID(inst->queue[i], &btConnId);

        /* For all the messages with the given btConnId in queue, call their respective restoreFunc().
         * CsrBtGattConnElement won't be present for this btConnId and in such a case 
         * CsrBtGattQueueRestoreHandler() will only free the CsrBtGattQueueElement and will
         * no longer iterate to find the next message in queue for this btConnId and won't call its
         * restoreFunc(). This is done to prevent call stack overflow due to recursive calling of 
         * CsrBtGattQueueRestoreHandler().
         */
        while (qElem)
        {
            inst->qid = i;
            if (qElem->gattMsg &&
                qElem->restoreFunc)
            { /* Call the queue message restore function. This will
                 make sure that the application receives an error
                 message */
                qElem->restoreFunc(inst, qElem, 0);
            }
            else
            { /* Just restore the queue */
                CsrBtGattQueueRestoreHandler(inst, qElem);
            }
            qElem = CSR_BT_GATT_QUEUE_FIND_BT_CONN_ID(inst->queue[i], &btConnId);
        }
    }
}

void CsrBtGattAttDisconnectIndHandler(GattMainInst *inst)
{
    ATT_DISCONNECT_IND_T *ind  = (ATT_DISCONNECT_IND_T *) inst->msg;
    CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &ind->cid); 
    CsrBtConnId btConnId = conn ? conn->btConnId:0;

#ifdef CSR_BT_GATT_INSTALL_EATT
    if ((conn) && (conn->cid != ind->cid))
    {
        /* Check for the cid, if its an eatt cid , mark the channel as invalid
           And then return from there as EATT disconnect we are not informing to the APP
           If its a ATT Connection continue with the existing implementation */
        CsrBtGattEattInvalidateCid(conn, ind->cid);
        CSR_BT_GATT_PREPARE_INST_CLEAN_UP(inst->prepare, inst, ind->cid, FALSE);
        return;
    }
#endif /* CSR_BT_GATT_INSTALL_EATT */

    /* Make sure that all Prepare Write Elements are remove. E.g. the physical 
       connection may be release while the GATT server has stored some Prepare
       Write Request, and before the GATT server has received a Execute Write Request */
    CSR_BT_GATT_PREPARE_INST_CLEAN_UP(inst->prepare, inst, ind->cid, FALSE);

    if (conn)
    {
        /* Mark prepared writes as bad and fail */
        CsrBtGattTddbStoreInfoOnDisconnection(conn);

        /* Send Link transferred reason in case of handover */
        if (ind->reason == L2CA_DISCONNECT_LINK_TRANSFERRED)
        {
            conn->reasonCode = CSR_BT_GATT_RESULT_LINK_TRANSFERRED;
        }

        CsrBtGattSendDisconnectEventToApps(inst, conn);
        /* Remove the conn element */
        CSR_BT_GATT_CONN_INST_REMOVE(inst->connInst, conn);
        /* Note Queue clean up has to be called after CSR_BT_GATT_CONN_INST_REMOVE, 
            do not move this above CSR_BT_GATT_CONN_INST_REMOVE */
        CsrBtGattAttQueueCleanupOnDisconnection(inst, btConnId);
    }
}

void CsrBtGattClientSendMtuEventToSubscribedApp(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattAppElement *appElement = (CsrBtGattAppElement *)elem;
    CsrBtGattConnElement *conn = value;

    if (CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ATT_LE_FIXED_CHANNEL_STATUS))
    {
        CsrBtGattMtuChangedIndSend(appElement->gattId, conn->btConnId, conn->mtu);
    }
}


#if 0
static CsrBool csrBtGattFindAppSubForMtuExcInd(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattAppElement *element = (CsrBtGattAppElement *)elem;
    CSR_UNUSED(value);
    return (element->eventMask & CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_REMOTE_MTU_EXCHANGE_IND) ? TRUE : FALSE;
}

#define CSR_BT_GATT_FIND_APP_SUBSCRIBED_FOR_MTU_EXCHANGE_IND(_appList) \
    ((CsrBtGattAppElement *)CsrCmnListSearch(&(_appList), csrBtGattFindAppSubForMtuExcInd, NULL))

#endif

void CsrBtGattAttFindInfoCfmHandler(GattMainInst *inst)
{
    ATT_FIND_INFO_CFM_T *prim      = (ATT_FIND_INFO_CFM_T *) inst->msg;
    CsrBtGattQueueElement *qElem   = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrBtGattConnElement  *conn    = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);
    CsrUint16    cidIdentifier     = CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        l2ca_cid_t cid;
        if(CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID) > 0 &&
           csrBtGattValidateQueueMsgState(prim->result, qElem->msgState))
        {
            CsrBtGattPrim type = *(CsrBtGattPrim *) qElem->gattMsg;

            switch (type)
            {
                case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_REQ:
                {
                    CsrBtGattDiscoverCharacDescriptorsReq *reqMsg = 
                                        (CsrBtGattDiscoverCharacDescriptorsReq *) qElem->gattMsg;

                    if (csrBtGattAttDiscoverCharacDescriptorsHandler(qElem, reqMsg->endGroupHandle, prim))
                    {
                        if (prim->result == ATT_RESULT_SUCCESS)
                        {
                            attlib_find_info_req(CSR_BT_GATT_IFACEQUEUE,
                                                 prim->cid,
                                                 (CsrUint16)(prim->handle + 1),
                                                 reqMsg->endGroupHandle,
                                                 NULL);

                            if (conn)
                            {
                                CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                            }
                        }
                        /* Else - Wait for another ATT_FIND_INFO_CFM message */
                    }
                    else
                    { /* This procedure is finish. Start the next if any */
                        CsrBtGattQueueRestoreHandler(inst, qElem);
                    }
                    break;
                }
                default :
                {
                    CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttFindInfoCfm: invalid type %x", type));
                }
            }
        }
        /* Else - The connection is invalid or this procedure has
           been cancelled. If cancelled, GATT shall wait for
           another ATT_FIND_INFO_CFM message */
    }
    /* Else - ATT_FIND_INFO_REQ and ATT_DISCONNECT_IND has cross */
}

void CsrBtGattAttFindByTypeValueCfmHandler(GattMainInst *inst)
{
    ATT_FIND_BY_TYPE_VALUE_CFM_T *prim = (ATT_FIND_BY_TYPE_VALUE_CFM_T *) inst->msg;
    CsrBtGattQueueElement *qElem   = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrBtGattConnElement  *conn    = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);
    CsrUint16     cidIdentifier    = CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        l2ca_cid_t cid;
        if(CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID) > 0 &&
           csrBtGattValidateQueueMsgState(prim->result, qElem->msgState))
        {
            CsrBtGattDiscoverServicesReq *reqMsg = (CsrBtGattDiscoverServicesReq *) qElem->gattMsg;

            if (csrBtGattAttPrimaryServiceHandler(qElem,
                                                  prim->result,
                                                  prim->handle,
                                                  prim->end,
                                                  reqMsg->uuid.length,
                                                  reqMsg->uuid.uuid))
            {
                if (prim->result == ATT_RESULT_SUCCESS)
                {
                    CsrUint8 *value = (CsrUint8 *) CsrPmemAlloc(reqMsg->uuid.length);
                    SynMemCpyS(value, reqMsg->uuid.length, reqMsg->uuid.uuid, reqMsg->uuid.length);
                    attlib_find_by_type_value_req(CSR_BT_GATT_IFACEQUEUE,
                                                  prim->cid,
                                                  (CsrUint16)(prim->end + 1),
                                                  CSR_BT_GATT_ATTR_HANDLE_MAX,
                                                  CSR_BT_GATT_UUID_PRIMARY_SERVICE_DECL,
                                                  reqMsg->uuid.length,
                                                  value,
                                                  NULL);

                    if (conn)
                    {
                        CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                    }
                }
                /* Else - Wait for another ATT_FIND_BY_TYPE_VALUE_CFM message */
            }
            else
            { /* This procedure is finish. Start the next if any */
                CsrBtGattQueueRestoreHandler(inst, qElem);
            }
        }
        /* Else - The connection is invalid or this procedure has
           been cancelled. If cancelled, GATT shall wait for
           another ATT_FIND_BY_TYPE_VALUE_CFM message */
    }
    /* Else - ATT_FIND_BY_TYPE_VALUE_REQ and ATT_DISCONNECT_IND has cross */
}

extern CsrUint32 gattCharacUuid[4];

void CsrBtGattAttReadByTypeCfmHandler(GattMainInst *inst)
{
    /* Note according to ATT spec the Read Blob Request would be used to
       read the remaining octets of a long attribute value. GATT only
       check if Read blob must be use when the CSR_BT_GATT_READ_BY_UUID_REQ
       procedure is running. The reason for this is that the other two
       procedures CSR_BT_GATT_FIND_INCL_SERVICES_REQ and
       CSR_BT_GATT_DISCOVER_CHARAC_REQ never will exceed ATT default mtu (23)
       as CSR_BT_GATT_FIND_INCL_SERVICES_REQ return a 128 bit uuid and
       CSR_BT_GATT_DISCOVER_CHARAC_REQ reads the Characteristic Declaration
       which max length is CSR_BT_GATT_CHARAC_DECLARATION_MAX_LENGTH (19 octets) */
    ATT_READ_BY_TYPE_CFM_T *prim = (ATT_READ_BY_TYPE_CFM_T *) inst->msg;
    CsrBtGattConnElement   *conn = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);
    CsrBtGattQueueElement *qElem = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrUint16      cidIdentifier = CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        CsrBtGattPrim type = *(CsrBtGattPrim *) qElem->gattMsg;
        l2ca_cid_t cid;
        CsrUint16 mtu = CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID);

#ifdef CSR_BT_GATT_CACHING
        if ((prim->cid == ATT_CID_LOCAL) &&
            ((type == CSR_BT_GATT_FLAT_DB_REGISTER_REQ) || (type == CSR_BT_GATT_DB_COMMIT_REQ)))
        {
            /* This procedure comes during DB initialization.
            Either CSR_BT_GATT_FLAT_DB_REGISTER_REQ or CSR_BT_GATT_DB_COMMIT_REQ is in queue in case of flat db and dynamic db.
            To prevent going into this function for ATT_CID_LOCAL(i.e. Gatt operations on local server), this check is needed.*/
            CsrBtGattDbHashHandler(inst, prim);
            return;
        }
#endif

        if(mtu > 0 && csrBtGattValidateQueueMsgState(prim->result, qElem->msgState))
        {
            switch (type)
            {
                case CSR_BT_GATT_FIND_INCL_SERVICES_REQ:
                {
                    if (csrBtGattAttReadByTypeFindInclServiceHandler(qElem, prim, conn))
                    { /* This procedure is finish. Start the next if any */
                        CsrBtGattQueueRestoreHandler(inst, qElem);
                    }
                    /* Else - Wait for another ATT_READ_BY_TYPE_CFM message
                       or is reading a 128-bit Bluetooth UUID */
                    break;
                }
                case CSR_BT_GATT_DISCOVER_CHARAC_REQ:
                {
                    CsrBtGattDiscoverCharacReq *reqMsg = (CsrBtGattDiscoverCharacReq *) qElem->gattMsg;

                    if (csrBtGattAttDiscoverCharacHandler(inst,
                                                          qElem,
                                                          reqMsg,
                                                          prim))
                    {
                        if (prim->result == ATT_RESULT_SUCCESS)
                        {
                            attlib_read_by_type_req(CSR_BT_GATT_IFACEQUEUE,
                                                    prim->cid,
                                                    (CsrUint16)(prim->handle + 1),
                                                    reqMsg->endGroupHandle,
                                                    ATT_UUID16,
                                                    (uint32_t *) gattCharacUuid,
                                                    NULL);

                            if (conn)
                            {
                                CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                            }
                        }
                        /* Else - Wait for another ATT_READ_BY_TYPE_CFM message */
                    }
                    else
                    { /* This procedure is finish. Start the next if any */
                        CsrBtGattQueueRestoreHandler(inst, qElem);
                    }
                    break;
                }
                case CSR_BT_GATT_READ_BY_UUID_REQ:
                {
                    if (csrBtGattAttReadByTypeReadByUuidHandler(inst, qElem, prim, mtu, conn))
                    { /* This procedure is finish. Start the next if any */
                        CsrBtGattQueueRestoreHandler(inst, qElem);
                    }
                    /* Else - Wait for another ATT_READ_BY_TYPE_CFM message */
                    break;
                }
                default :
                {
                    CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttReadByTypeCfm: invalid type %x", type));
                }
            }
        }
        /* Else - The connection is invalid or this procedure has
           been cancelled. If cancelled, GATT shall wait for
           another ATT_READ_BY_TYPE_CFM message */
    }
    /* Else - ATT_READ_BY_TYPE_REQ and ATT_DISCONNECT_IND has cross */
}

void CsrBtGattAttReadCfmHandler(GattMainInst *inst)
{
    ATT_READ_CFM_T *prim          = (ATT_READ_CFM_T *) inst->msg;
    CsrBtGattConnElement  *conn   = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);
    CsrBtGattQueueElement *qElem  = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrUint16     cidIdentifier   = CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        l2ca_cid_t cid;
        CsrUint16 mtu = CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID);

        if(mtu > 0)
        {
            if (qElem->msgState == CSR_BT_GATT_MSG_QUEUE_CANCELLED)
            { /* This running procedure has been cancelled. E.g.
                 either a CSR_BT_GATT_FIND_INCL_SERVICES_CFM or
                 a proper Read Cfm message were sent to the
                 application at the time GATT received
                 CSR_BT_GATT_CANCEL_REQ.

                 This procedure is finish. Start the next if any */
                CsrBtGattQueueRestoreHandler(inst, qElem);
            }
            else
            {
                CsrBtGattPrim type = *(CsrBtGattPrim *) qElem->gattMsg;

                switch (type)
                {
                    case CSR_BT_GATT_FIND_INCL_SERVICES_REQ:
                        if (csrBtGattAttReadCfmFindInclServiceHandler(qElem, prim, conn))
                        { /* This procedure is finish. Start the next if any */
                            CsrBtGattQueueRestoreHandler(inst, qElem);
                        }
                        /* Else - Looking for more included services */
                        break;

                    case CSR_BT_GATT_READ_REQ:
                    {
                        CsrBtGattReadReq *reqMsg = (CsrBtGattReadReq *) qElem->gattMsg;

                        if (prim->result == ATT_RESULT_SUCCESS &&
                            prim->size_value >= (mtu - CSR_BT_GATT_ATT_READ_HEADER_LENGTH))
                        {
                            if (prim->size_value < CSR_BT_GATT_LONG_READ_ATTR_MAX_VALUE)
                            {
#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
                                /* Add data to the long  Read buffer */
                                CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &qElem->gattId);
                                if (appElement && (appElement->flags & CSR_BT_GATT_LONG_READ_AS_LIST) == CSR_BT_GATT_LONG_READ_AS_LIST)
                                {
                                    CsrBtGattLongAttrReadBuffer* buf = CSR_BT_GATT_LONG_READ_INST_ADD_LAST(qElem->longReadBuffer);
                                    buf->dataLength = prim->size_value;
                                    buf->data = prim->value;
                                    buf->offset = qElem->dataOffset;
                                    /* Set the prim->value to NULL it means its GATT responsbility to free the memory later on */
                                    prim->value = NULL;
                                }
                                else
#endif
                                {
                                    qElem->data = (CsrUint8 *) CsrPmemZalloc(CSR_BT_GATT_LONG_READ_ATTR_MAX_VALUE);
                                    SynMemCpyS(qElem->data,
                                               CSR_BT_GATT_LONG_READ_ATTR_MAX_VALUE,
                                               prim->value,
                                               prim->size_value);
                                }

                                qElem->dataOffset = prim->size_value;
                                attlib_read_blob_req(CSR_BT_GATT_IFACEQUEUE,
                                                     prim->cid,
                                                     qElem->attrHandle,
                                                     qElem->dataOffset,
                                                     NULL);

                                if (conn)
                                {
                                    CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                                }
                            }
                            else if (prim->size_value == CSR_BT_GATT_LONG_READ_ATTR_MAX_VALUE)
                            {
                                CsrBtGattReadCfmHandler(reqMsg,
                                                        (CsrBtResultCode) prim->result,
                                                        CSR_BT_GATT_GET_SUPPLIER(prim->result),
                                                        prim->size_value,
                                                        &prim->value);

                                /* This procedure is finish. Start the next if any */
                                CsrBtGattQueueRestoreHandler(inst, qElem);
                            }
                            else
                            {
                                CsrBtGattReadCfmHandler(reqMsg,
                                                        ATT_RESULT_INVALID_LENGTH,
                                                        CSR_BT_SUPPLIER_ATT,
                                                        prim->size_value,
                                                        &prim->value);
                            }
                        }
                        else
                        {
                            CsrBtGattReadCfmHandler(reqMsg,
                                                    (CsrBtResultCode) prim->result,
                                                    CSR_BT_GATT_GET_SUPPLIER(prim->result),
                                                    prim->size_value,
                                                    &prim->value);

                            /* This procedure is finish. Start the next if any */
                            CsrBtGattQueueRestoreHandler(inst, qElem);
                        }
                        break;
                    }

                    default :
                    {
                        CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttReadCfm: invalid type %d", type));
                        break;
                    }
                }
            }
        }
    }
    /* Else - ATT_READ_REQ and ATT_DISCONNECT_IND has cross */
}

void CsrBtGattAttReadBlobCfmHandler(GattMainInst *inst)
{
    ATT_READ_BLOB_CFM_T *prim     = inst->msg;
    CsrBtGattConnElement* conn    = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);
    CsrBtGattQueueElement *qElem  = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrUint16      cidIdentifier  = CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        CsrUint16 mtu = 0;

        if (qElem->gattId != inst->privateGattId)
        {
            l2ca_cid_t cid;
            mtu = CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID);
        }
        else if (conn)
        {
            mtu = conn->mtu;
        }

        if(mtu > 0)
        { /* Still connected */
            CsrBtGattPrim type = *(CsrBtGattPrim *) qElem->gattMsg;

            if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
            { /* This message has not been cancel by the application */
                if (prim->result == ATT_RESULT_SUCCESS)
                { /* The data piece has been read with success, check if 
                     this were the last piece or not */
                    if (prim->value)
                    {
                        CsrUint16 offset;
                        if (type == CSR_BT_GATT_READ_REQ)
                        {
                            offset = ((CsrBtGattReadReq *) qElem->gattMsg)->offset;
                        }
                        else
                        {
                            offset = 0;
                        }

                        if (qElem->dataOffset + prim->size_value < CSR_BT_GATT_LONG_READ_ATTR_MAX_VALUE)
                        {  /* Attribute size is within specification defined limits  */
#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
                            CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &qElem->gattId);
                            if ((appElement) && (appElement->flags & CSR_BT_GATT_LONG_READ_AS_LIST) == CSR_BT_GATT_LONG_READ_AS_LIST)
                            {
                                /* Add data to the long read buffer */
                                CsrBtGattLongAttrReadBuffer *buf = CSR_BT_GATT_LONG_READ_INST_ADD_LAST(qElem->longReadBuffer);
                                buf->dataLength                  = prim->size_value;
                                buf->data                        = prim->value;
                                buf->offset                      = qElem->dataOffset;
                                /* Set the prim->value to NULL it means it's GATT responsbility to free the memory later on */
                                prim->value                      = NULL;
                            }
                            else
#endif
                            {
                                SynMemCpyS(&(qElem->data[qElem->dataOffset]),
                                    prim->size_value,
                                    prim->value,
                                    prim->size_value);
                            }
                            qElem->dataOffset += prim->size_value;

                            if (prim->size_value >= (mtu - CSR_BT_GATT_ATT_READ_BLOB_HEADER_LENGTH))
                            { /* There may be more of attribute to read. Read the next piece of data */
                                attlib_read_blob_req(CSR_BT_GATT_IFACEQUEUE,
                                                     prim->cid,
                                                     qElem->attrHandle,
                                                     (CsrUint16) (qElem->dataOffset + offset),
                                                     NULL);

                                if (conn)
                                {
                                    CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                                }
                                return;
                            }
                        }
                        else
                        {
                            /* Truncated attribute length(send the error code with truncated value). Stop the procedure and disconnect. */
                            prim->result = CSR_BT_GATT_RESULT_TRUNCATED_DATA;
                        }
                    }

                    /* Else - All data received */
                }
                else if (qElem->dataOffset > 0 &&
                         (prim->result == ATT_RESULT_INVALID_OFFSET        ||
                          prim->result == ATT_RESULT_REQUEST_NOT_SUPPORTED ||
                          prim->result == ATT_RESULT_NOT_LONG))
                { /* Offset specified was past the end of the long attribute,
                     or the remote device do not support the read blob command,
                     or the attribute were not long. Note if 
                     qElem->dataOffset == 0 read blob has been call direct 
                     and must consider as the first read request message. */
                    prim->result = ATT_RESULT_SUCCESS;
                }
                /* Else - Failed to read blob */
            }
            /* Else - The ongoing procedure has been cancelled */

            switch (type)
            {
                case CSR_BT_GATT_READ_REQ:
                {

                    if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
                    {
#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
                        /* Check for longreadbuffer count, if count is non zero App opted for new long read */
                        if (qElem->longReadBuffer.count)
                        {
                            CsrBtGattLongReadAttrElem ids;
                            /* Traverse the longReadBuffer list to get number of readunitcount and readuint */
                            CSR_BT_GATT_LONG_READ_ITERATE_LIST(qElem->longReadBuffer,ids);

                            CsrBtGattLongReadCfmHandler((CsrBtGattReadReq*)qElem->gattMsg,
                                                qElem->gattId,
                                                (CsrBtResultCode) prim->result,
                                                CSR_BT_GATT_GET_SUPPLIER(prim->result),
                                                qElem->btConnId,
                                                ids.unitCount,
                                                ids.readUnit);
                        }
                        else
#endif
                        {
                            CsrBtGattReadCfmHandler((CsrBtGattReadReq *) qElem->gattMsg,
                                                    (CsrBtResultCode) prim->result,
                                                    CSR_BT_GATT_GET_SUPPLIER(prim->result),
                                                    (qElem->dataOffset),
                                                    &qElem->data);
                        }
                    }
                    /* Else - This procedure has been cancelled. E.g. the
                       proper Read Cfm message was sent to the
                       application at the time GATT received
                       CSR_BT_GATT_CANCEL_REQ.  */

                    /* This procedure is finish. Start the next if any */
                    CsrBtGattQueueRestoreHandler(inst, qElem);
                    break;
                }
                case CSR_BT_GATT_READ_BY_UUID_REQ:
                {
                    if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
                    {
                        CsrBtGattReadByUuidReq *reqMsg = (CsrBtGattReadByUuidReq *) qElem->gattMsg;

                        if (prim->result == ATT_RESULT_SUCCESS)
                        {
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
                            if (reqMsg->uuid.length == CSR_BT_UUID16_SIZE &&
                                CSR_GET_UINT16_FROM_LITTLE_ENDIAN(reqMsg->uuid.uuid) == CSR_BT_GATT_UUID_DEVICE_NAME_CHARAC)
                            { /* The Remote Name Characteristic Value has been read, Update saved named */
                                CSR_BT_GATT_CONN_INST_UPDATE_REMOTE_NAME(inst->connInst,
                                                                         prim->cid,
                                                                         qElem->dataOffset,
                                                                         qElem->data);  
                            }
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

                            if (qElem->gattId != inst->privateGattId)
                            {
                                qElem->msgState = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK;
                                CsrBtGattReadByUuidIndSend(qElem->gattId,
                                                           qElem->btConnId,
                                                           qElem->attrHandle,
                                                           qElem->dataOffset,
                                                           qElem->data);
                            }
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
                            else
                            { /* GATT have called ATT_READ_BY_TYPE_REQ 
                                 in order to to Read the remote name 
                                 Characteristic Value. Note in this process 
                                 ATT_READ_BLOB_REQ were called in order 
                                 to received a complete long name */
                                if (qElem->dataElemIndex > 0)
                                { /* SC has requested to read the name, 
                                     before before GATT had time to read it. 
                                     Send Result to SC */
                                    CsrBtGattReadRemoteLeNameCfmSend((CsrSchedQid)qElem->dataElemIndex, 
                                                                     qElem->dataOffset, 
                                                                     qElem->data);
                                }
                            }
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */
                        }
                        else
                        {
                            CsrPmemFree(qElem->data);
                        }
                        qElem->data = NULL;

                        if (qElem->gattId != inst->privateGattId                   && 
                            qElem->attrHandle + 1 <= reqMsg->endGroupHandle        &&
                            prim->result != ATT_RESULT_INSUFFICIENT_AUTHENTICATION &&
                            prim->result != ATT_RESULT_INSUFFICIENT_ENCRYPTION     &&
                            prim->result != ATT_RESULT_SIGN_FAILED)
                        { /* GATT shall only continue if read blob do not failed 
                             for security reasons or if GATT itself have tried to
                             read the remote name Characteristic Value. 
                             Note, GATT will try to run the security one time per handle. 
                             This is handle by the function CsrBtGattCheckSecurity. Also
                             note that if GATT itself is trying to read the Read Name
                             it do not need to continue as a peer device only shall 
                             have one of these Characteristic */
                            CsrUint32       attUuid[4];
                            att_uuid_type_t uuidType;
                            CsrBtGattGetAttUuid(reqMsg->uuid, attUuid, &uuidType);
                            /* Note, in order to be able to handle that the server requires 
                               security on the next handle being read, GATT shall set the 
                               security callback function again because the function
                               csrBtGattScLeSecurityCfmHandler set it to NULL and GATT 
                               shall allow one security procedure per handle being read. 
                               The qElem->attrHandle is set to
                               qElem->attrHandle + 1 so GATT is able replay this command in 
                               case of security failure */
                            qElem->attrHandle++;
                            attlib_read_by_type_req(CSR_BT_GATT_IFACEQUEUE,
                                                    prim->cid,
                                                    qElem->attrHandle,
                                                    reqMsg->endGroupHandle,
                                                    uuidType,
                                                    (uint32_t *) attUuid,
                                                    NULL);

                            if (conn)
                            {
                                CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                            }
                        }
                        else
                        {
                            if (qElem->gattId != inst->privateGattId)
                            {
                                CsrBtResultCode resultCode;
                                CsrBtSupplier   resultSupplier;

                                /* No need to check the return value of
                                   csrBtGattGetResultCode as it is sent
                                   direct to the application */
                                (void)(csrBtGattGetResultCode(qElem->msgState,
                                                              prim->result,
                                                              &resultCode,
                                                              &resultSupplier));

                                CsrBtGattReadByUuidCfmSend(qElem->gattId,
                                                           resultCode,
                                                           resultSupplier,
                                                           qElem->btConnId,
                                                           &(reqMsg->uuid));
                            }
                            /* Else - GATT have called ATT_READ_BY_TYPE_REQ 
                                 in order to to Read the remote name 
                                 Characteristic Value. Note in this process 
                                 ATT_READ_BLOB_REQ were called in order 
                                 to received a complete name */

                            /* This procedure is finish. Start the next if any */
                            CsrBtGattQueueRestoreHandler(inst, qElem);
                        }
                    }
                    else
                    { /* This procedure has been cancelled. E.g.
                         CSR_BT_GATT_READ_BY_UUID_CFM was sent
                         to the application at the time GATT received
                         CSR_BT_GATT_CANCEL_REQ.
                         This procedure is finish. Start the next if any */
                        CsrBtGattQueueRestoreHandler(inst, qElem);
                    }
                    break;
                }
                default:
                {
                    CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "AttReadBlobCfm: invalid type %d", type));
                }
            }
        }
    }
    /* Else - ATT_READ_BLOB_REQ and ATT_DISCONNECT_IND has cross */
}

void CsrBtGattAttReadMultiCfmHandler(GattMainInst *inst)
{
    ATT_READ_MULTI_CFM_T *prim    = (ATT_READ_MULTI_CFM_T *) inst->msg;
    CsrBtGattQueueElement *qElem  = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrBtGattConnElement* conn    = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);

    CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        l2ca_cid_t cid;
        if(CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID) > 0)
        {
            if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
            {
                CsrBtGattReadMultiReq *req = (CsrBtGattReadMultiReq *) qElem->gattMsg;

                CsrBtGattReadMultiCfmSend(qElem->gattId,
                                          prim->result,
                                          CSR_BT_GATT_GET_SUPPLIER(prim->result),
                                          qElem->btConnId,
                                          prim->size_value,
                                          prim->value,
                                          (req->handlesCount ? req->handles : NULL));

                prim->value = NULL;
            }
            /* Else -This procedure has been cancelled. E.g.
               CSR_BT_GATT_READ_MULTI_CFM was sent
               to the application at the time GATT received
               CSR_BT_GATT_CANCEL_REQ. */

            /* This procedure is finish. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, qElem);
        }
    }
    /* Else - ATT_READ_MULTI_REQ and ATT_DISCONNECT_IND has cross */
}

static CsrBtGattAccessCheck csrBtGattGetAccessCheckValue(CsrUint16 flags)
{
    CsrBtGattAccessCheck check = CSR_BT_GATT_ACCESS_CHECK_NONE;

    check |= (CsrBtGattAccessCheck)(flags & ATT_ACCESS_PERMISSION
                                    ? CSR_BT_GATT_ACCESS_CHECK_AUTHORISATION
                                    : CSR_BT_GATT_ACCESS_CHECK_NONE);

    check |= (CsrBtGattAccessCheck)(flags & ATT_ACCESS_ENCRYPTION_KEY_LEN
                                    ? CSR_BT_GATT_ACCESS_CHECK_ENCR_KEY_SIZE
                                    : CSR_BT_GATT_ACCESS_CHECK_NONE);
    return check;
}

#ifdef CSR_BT_GATT_INSTALL_EATT
void CsrBtGattAttReadMultiVarCfmHandler(GattMainInst *inst)
{
    ATT_READ_MULTI_VAR_CFM_T *prim   = (ATT_READ_MULTI_VAR_CFM_T *) inst->msg;
    CsrBtGattQueueElement *qElem     = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrBtGattConnElement  *conn      = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);
    CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        l2ca_cid_t cid;

        if(CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID) > 0)
        {

            if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
            {
                CsrBtGattReadMultiVarReq *req = (CsrBtGattReadMultiVarReq *) qElem->gattMsg;

                CsrBtGattReadMultiVarCfmSend(qElem->gattId,
                                          prim->result,
                                          CSR_BT_GATT_GET_SUPPLIER(prim->result),
                                          qElem->btConnId,
                                          prim->error_handle,
                                          prim->size_value,
                                          prim->value,
                                          req->handlesCount,
                                          req->handles);
                req->handles = NULL;
                prim->value = NULL;
            }
            /* Else -This procedure has been cancelled. E.g.
               CSR_BT_GATT_READ_MULTI_VAR_CFM was sent
               to the application at the time GATT received
               CSR_BT_GATT_CANCEL_REQ. */

            /* This procedure is finish. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, qElem);
        }
    }
    /* Else - ATT_READ_MULTI_VAR_REQ and ATT_DISCONNECT_IND has cross */
}

void CsrBtGattAttReadMultiVarIndHandler(GattMainInst *inst)
{
    ATT_READ_MULTI_VAR_IND_T *prim  = (ATT_READ_MULTI_VAR_IND_T *) inst->msg;

    CsrBtGattAppElement *appElement;
    CsrBtTypedAddr      address;
    CsrUint16           mtu       = CSR_BT_GATT_ATTR_VALUE_LEN_MAX;
    CsrBtGattConnInfo   connInfo  = CSR_BT_GATT_CONNINFO_LE;
    CsrBtConnId         btConnId  = CSR_BT_CONN_ID_INVALID;
    att_result_t        attResult = ATT_RESULT_SUCCESS;
    CsrBtAddrZero(&(address));

    appElement = CsrBtGattAccessIndGetParams(inst, prim->cid, prim->handles[0],
                                              &btConnId, &connInfo, &mtu, &address);

    if(appElement)
    {
        /* Has requested to read attributes value */
        CsrBtGattAccessMultiReadIndSend(inst, prim->cid,
                                             appElement->gattId,
                                                       btConnId,
          (CsrUint16)(mtu - CSR_BT_GATT_ATT_READ_HEADER_LENGTH),
                      csrBtGattGetAccessCheckValue(prim->flags),
                                                       connInfo,
                                                        address,
                                             prim->size_handles,
                                                 prim->handles);
        prim->handles = NULL;
    }
    else
    { /* Did not find an owner of the handle or an valid connection */
        attResult = ATT_RESULT_ATTR_NOT_FOUND;
        attlib_read_multi_var_rsp(CSR_BT_GATT_IFACEQUEUE, prim->cid, attResult, 0, 0, NULL, NULL);
    }
}

void CsrBtGattAttHandleMultiValueCfmHandler(GattMainInst *inst)
{
    ATT_MULTI_HANDLE_VALUE_NTF_CFM_T *prim  = (ATT_MULTI_HANDLE_VALUE_NTF_CFM_T*) inst->msg;

    CsrBtGattQueueElement *qElem = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrBtGattConnElement *conn   = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst,
                                                                            &(prim->cid));
    CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        /* send the confirmation to the App */
        CsrBtGattStdBtConnIdCfmSend(CSR_BT_GATT_EVENT_SEND_CFM,
            qElem->gattId,
            prim->result,
            CSR_BT_GATT_GET_SUPPLIER(prim->result),
            qElem->btConnId);

        /* This procedure is finish. Start the next if any */
        CsrBtGattQueueRestoreHandler(inst, qElem);
    }
}

void CsrBtGattAttHandleValueMultiIndHandler(GattMainInst *inst)
{
    ATT_MULTI_HANDLE_VALUE_NTF_IND_T *prim  = (ATT_MULTI_HANDLE_VALUE_NTF_IND_T *) inst->msg;
    CsrBtGattConnElement   *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst,
                                                                            &(prim->cid));
    CsrBtGattClientService *ele;
    CsrBtGattAppElement *appInst;

    if (conn)
    {
        CsrUint16 handle;
        CsrUint16 length;
        CsrUint8* value;
        CsrUint16 traverseTuple = 0;

        /* The purpose of the loop is to extract handle length value from the tuple list
           One by One to get handle length and value for the whole list and keep sending
           The information to the Application */
        while(traverseTuple < prim->size_value)
        {
            /* This expression is derived based on the below description from core spec
               Format of the Handle Length Value Tuple : Handle (2 bytes),
               Length (2 bytes), Value (Length bytes).
               Where Handle + length size  = 4 */
            handle = (prim->value[traverseTuple + 1] << 8) | prim->value[traverseTuple];
            length = (prim->value[traverseTuple + 3] << 8) | prim->value[traverseTuple + 2];
            value = prim->value + traverseTuple + 4;

            ele = CSR_BT_GATT_CLIENT_SERVICE_LIST_FIND_BY_HANDLE(conn->cliServiceList,
                                                                    &(handle));
#ifdef CSR_BT_INSTALL_GATT_BREDR
            if (!ele)
            {
                CsrBtGattConnElement *conn1;

                if (conn->leConnection)
                {
                    conn1 = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_BREDR(inst->connInst,
                                                 CsrBtGattFindConnInstFromAddressFlagsBredr,
                                                 &(conn->peerAddr));
                }
                else
                {
                    conn1 = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_LE(inst->connInst,
                                                 CsrBtGattFindConnInstFromAddressFlagsLe,
                                                 &(conn->peerAddr));
                }

                if (conn1)
                {
                    ele = CSR_BT_GATT_CLIENT_SERVICE_LIST_FIND_BY_HANDLE(conn1->cliServiceList,
                                                                            &(handle));
                }
            }
#endif
            if (ele)
            {
                /* Check if the App is still registered */
                appInst = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &ele->gattId);

                if (appInst)
                {
                    CsrUint8 *ntfValue = (CsrUint8 *) CsrPmemZalloc(length);
                    SynMemCpyS(ntfValue, length, value, length);

                    CsrBtGattNotificationIndicationIndSend(inst,
                                                   prim->cid,
                                                   ATT_HANDLE_VALUE_NOTIFICATION,
                                                   length,
                                                   ntfValue,
                                                   handle,
                                                   ele->gattId,
                                                   conn->btConnId);
                }
                else
                {
                    /* TBD: Remove the element */
                }
            }
            /* Update the value to get the other tuple in the list*/
            traverseTuple = 4 + traverseTuple + length;
        }
    }
}
#endif /* CSR_BT_GATT_INSTALL_EATT */

void CsrBtGattAttReadByGroupTypeCfmHandler(GattMainInst *inst)
{
    /* Note according to ATT spec the Read Blob Request would be used to
       read the remaining octets of a long attribute value. GATT do not
       use Read blob because ATT_READ_BY_GROUP_TYPE_REQ only are used
       to Discover/Read Primary Services (Service Declarations) which
       means that the value never will exceed ATT default MTU (23).
       Also note that GATT do not handle security because the server 
       shall not require authentication or authorisation when the GATT
       client reads Primary Services */
    ATT_READ_BY_GROUP_TYPE_CFM_T *prim   = (ATT_READ_BY_GROUP_TYPE_CFM_T *) inst->msg;
    CsrBtGattQueueElement *qElem  = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrBtGattConnElement  *conn   = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);
    CsrUint16     cidIdentifier   = CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        l2ca_cid_t cid;
        if(CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID) > 0 &&
           csrBtGattValidateQueueMsgState(prim->result, qElem->msgState))
        {
            if (csrBtGattAttPrimaryServiceHandler(qElem,
                                                  prim->result,
                                                  prim->handle,
                                                  prim->end,
                                                  prim->size_value,
                                                  prim->value))
            {
                if (prim->result == ATT_RESULT_SUCCESS)
                {
                    CsrUint32 primaryService[4] = {0x0002800, 0, 0, 0};

                    attlib_read_by_group_type_req(CSR_BT_GATT_IFACEQUEUE,
                                                  prim->cid,
                                                  (CsrUint16)(prim->end + 1),
                                                  CSR_BT_GATT_ATTR_HANDLE_MAX,
                                                  ATT_UUID16,
                                                  (uint32_t *) primaryService,
                                                  NULL);
                    if (conn)
                    {
                        CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                    }
                }
                /* Else - Wait for another ATT_READ_BY_GROUP_TYPE_CFM message */
            }
            else
            { /* This procedure is finish. Start the next if any */
                CsrBtGattQueueRestoreHandler(inst, qElem);
            }
        }
        /* Else - The connection is invalid or this procedure has
           been cancelled. If cancelled, GATT shall wait for
           another ATT_READ_BY_GROUP_TYPE_CFM message */
    }
    /* Else - ATT_READ_BY_GROUP_TYPE_REQ and ATT_DISCONNECT_IND has cross */
}

#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT

/* Handler function to send Write command again incase of getting CID as ATT_BUSY */
void csrBtGattWriteCmdHandler(CsrUint16 apphandle, void* data)
{
    CsrBtGattQueueElement *qElem  = (CsrBtGattQueueElement *) data;
    CsrBtGattWriteReq *prim;
    CSR_UNUSED(apphandle);

    if (qElem && (prim = qElem->gattMsg))
    {
        CsrUint8 * writeCmdData;
        CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(gattMainInstPtr->connInst, &prim->btConnId); 

        if(conn)
        {
            qElem->msgRetryCount++;

            writeCmdData = (CsrUint8 *)CsrPmemZalloc(prim->attrWritePairs[0].valueLength);
            SynMemCpyS(writeCmdData, prim->attrWritePairs[0].valueLength, qElem->data, prim->attrWritePairs[0].valueLength);

            attlib_write_req(CSR_BT_GATT_IFACEQUEUE,
                            qElem->cid,
                            qElem->attrHandle,
                            prim->flags,
                            prim->attrWritePairs[0].valueLength,
                            writeCmdData,
                            NULL);
        }
    }
}
#endif /* CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT */
void CsrBtGattAttWriteCfmHandler(GattMainInst *inst)
{
    ATT_WRITE_CFM_T *prim         = inst->msg;
    CsrBtGattQueueElement *qElem  = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrBtGattConnElement   *conn  = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);

#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
    CsrUint16  cidIdentifier      = CsrBtGattEattResetCid(conn, prim->cid, prim->result);
#else
    CsrBtGattEattResetCid(conn, prim->cid, prim->result);
#endif

    if (qElem && qElem->gattMsg)
    {
        l2ca_cid_t cid;

        if(CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID) > 0)
        {
            if (qElem->gattId == inst->privateGattId)
            { 
#ifdef CSR_BT_GATT_INSTALL_EATT
                if((conn) && (CsrBtGattCheckCid(conn, prim->cid)))
                {
#ifdef GATT_CACHING_CLIENT_ROLE
                    if (!conn->serviceChangeHandle)
                    {
                        CsrBtGattReadByUuidPrivateHandler(inst, CSR_BT_GATT_UUID_SERVICE_CHANGED_CHARAC, conn->btConnId);
                        CsrBtGattReadByUuidPrivateHandler(inst, CSR_BT_GATT_UUID_DATABASE_HASH_CHARAC, conn->btConnId);
                    }
#endif /* GATT_CACHING_CLIENT_ROLE */
                    /* Need to clear the Queue when GATT internally trigger Write request and the operation is allocated
                       Not local CID else this element will remain in queue forever and cause memory corruption/crash */
                    CsrBtGattQueueRestoreHandler(inst, qElem);
                }
#endif
                if (prim->cid == ATT_CID_LOCAL)
                {
                    /* This procedure is finish. Start the next if any */
                    CsrBtGattQueueRestoreHandler(inst, qElem);
                }
            }
            else
            {
#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
                {
                    CsrBtGattWriteReq       *reqPrim = qElem->gattMsg;
                    /* If the ATT result is busy then we need to try again so extract the message and need to try sending
                       back to ATT after CSR_BT_GATT_MSG_RETRY_TIMER_VALUE ms timer expiry */
                    if(reqPrim && (reqPrim->flags == CSR_BT_GATT_WRITE_COMMAND) && conn)
                    {
                        CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                                         &qElem->gattId);
                        if(appElement)
                        {
                            if (prim->result == ATT_RESULT_BUSY)
                            {
                                if(CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONGESTION_STATUS) &&
                                    (qElem->msgRetryCount == 0))
                                {
                                    CsrBtGattCongestionIndSend(appElement->gattId, conn->btConnId, appElement->start, TRUE);
                                }

                                /* Wait for CSR_BT_GATT_MSG_RETRY_TIMER_VALUE ms */
                                qElem->txTimer = CsrSchedTimerSet(CSR_BT_GATT_MSG_RETRY_TIMER_VALUE * CSR_SCHED_MILLISECOND,
                                    csrBtGattWriteCmdHandler,
                                    0,
                                    qElem);

                                CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                                return;
                            }
                            else if((prim->result == ATT_RESULT_SUCCESS) && (qElem->msgRetryCount != 0))
                            {
                                if(CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONGESTION_STATUS))
                                {
                                    CsrBtGattCongestionIndSend(appElement->gattId, conn->btConnId, appElement->start, FALSE);
                                }
                            }
                        }
                    }
                }
#endif /* CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT */
                CsrBtGattWriteCfmSend(qElem->gattMsg,
                                      (CsrBtResultCode) prim->result,
                                      CSR_BT_GATT_GET_SUPPLIER(prim->result));

                /* This procedure has completed. Start the next if any */
                CsrBtGattQueueRestoreHandler(inst, qElem);
            }
        }
    }
    /* Else - ATT_WRITE_REQ and ATT_DISCONNECT_IND has cross */
}

void CsrBtGattAttPrepareWriteCfmHandler(GattMainInst *inst)
{
    ATT_PREPARE_WRITE_CFM_T *prim = (ATT_PREPARE_WRITE_CFM_T *) inst->msg;
    CsrBtGattConnElement  *conn   = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);

    CsrBtGattQueueElement *qElem  = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrUint16      cidIdentifier  = CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        l2ca_cid_t cid;
        CsrUint16 mtu = CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID);

        if (mtu > 0)
        {
            if (prim->result == ATT_RESULT_SUCCESS)
            {
                if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
                {
                    CsrBtGattWriteReq *reqMsg = (CsrBtGattWriteReq *) qElem->gattMsg;
                    CsrBtGattAttrWritePairs *attrPair = &(reqMsg->attrWritePairs[qElem->dataElemIndex]);

                    if (qElem->dataOffset + prim->size_value <= attrPair->valueLength &&
                        csrBtGattValidatePrepareWriteData(reqMsg->flags,
                                                          prim,
                                                          &(attrPair->value[qElem->dataOffset])))
                    {
                        qElem->dataOffset = (CsrUint16)(qElem->dataOffset + prim->size_value);

                        if (qElem->dataOffset < attrPair->valueLength)
                        { /* Send the next piece of the attribute value */
                            CsrBtGattGetAttPrepareWriteSend(conn,
                                                            qElem,
                                                            (CsrUint16)(attrPair->offset + qElem->dataOffset),
                                                            mtu,
                                                            attrPair->valueLength,
                                                            attrPair->value);
                        }
                        else
                        { /* One entire attribute value is sent */
                            qElem->dataElemIndex++;


                            if (qElem->dataElemIndex < reqMsg->attrWritePairsCount)
                            { /* Start sending the next attribute value. 
                                 Note, in order to be able to handle that the server requires 
                                 security on the next handle being written, GATT shall set the 
                                 security callback function again, because the function 
                                 csrBtGattScLeSecurityCfmHandler set it to NULL and GATT 
                                 shall allow one security procedure per handle being written. */
                                attrPair              = &(reqMsg->attrWritePairs[qElem->dataElemIndex]);
                                qElem->attrHandle     = attrPair->attrHandle;
                                qElem->dataOffset     = 0;

                                CsrBtGattGetAttPrepareWriteSend(conn,
                                                                qElem,
                                                                attrPair->offset,
                                                                mtu,
                                                                attrPair->valueLength,
                                                                attrPair->value);
                            }
                            else
                            { /* All attribute values are sent - Execute.
                                 Note set cancelFunc to NULL because when
                                 attlib_execute_write_req is called a write
                                 procedure cannot be cancel */
                                qElem->cancelFunc = NULL;
                                attlib_execute_write_req(CSR_BT_GATT_IFACEQUEUE,
                                                         prim->cid,
                                                         ATT_EXECUTE_WRITE,
                                                         NULL);

                                if (conn)
                                {
                                    CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                                }
                            }
                        }
                    }
                    else
                    { /* The data received is not the same data as sent. Cancel
                         the attlib_prepare_write_req procedure and set cancelFunc
                         to NULL because when attlib_execute_write_req is called
                         a write procedure cannot be cancel */
                        qElem->msgState   = CSR_BT_GATT_MSG_QUEUE_EXECUTE_WRITE_CANCEL;
                        qElem->cancelFunc = NULL;
                        attlib_execute_write_req(CSR_BT_GATT_IFACEQUEUE,
                                                 prim->cid,
                                                 ATT_EXECUTE_CANCEL,
                                                 NULL);

                        if (conn)
                        {
                            CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                        }
                    }
                }
                else
                { /* This procedure has been cancel by the application */
                    attlib_execute_write_req(CSR_BT_GATT_IFACEQUEUE,
                                             prim->cid,
                                             ATT_EXECUTE_CANCEL,
                                             NULL);

                    if (conn)
                    {
                        CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                    }
                }
            }
            else
            {
                if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
                {
                    CsrBtGattWriteCfmSend((CsrBtGattPrim *) qElem->gattMsg,
                                          (CsrBtResultCode) prim->result,
                                          CSR_BT_GATT_GET_SUPPLIER(prim->result));
                }
                /* Else -This procedure has been cancelled. E.g.
                   CSR_BT_GATT_WRITE_CFM was sent
                   to the application at the time GATT received
                   CSR_BT_GATT_CANCEL_REQ. */

                if (qElem->dataElemIndex > 0 || qElem->dataOffset > 0)
                { /* At least one ATT_PREPARE_WRITE_CFM with success has been 
                     received. Set qElem->msgState to Cancelled in order
                     to make sure that CSR_BT_GATT_WRITE_CFM is not
                     sent to the application twice and set qElem->cancelFunc 
                     to NULL in order to make sure that this procedure cannot 
                     be cancel twice */
                    qElem->cancelFunc = NULL;
                    qElem->msgState   = CSR_BT_GATT_MSG_QUEUE_CANCELLED;
                    attlib_execute_write_req(CSR_BT_GATT_IFACEQUEUE,
                                             prim->cid,
                                             ATT_EXECUTE_CANCEL,
                                             NULL);

                    if (conn)
                    {
                        CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                    }
                }
                else
                { /* GATT shall not issue a ATT_EXECUTE_WRITE_REQ because
                     it have not received any ATT_PREPARE_WRITE_CFM messages
                     with success. This procedure is finish. Start the next if any */
                    CsrBtGattQueueRestoreHandler(inst, qElem);
                }
            }
        }
    }
    /* Else - ATT_PREPARE_WRITE_REQ and ATT_DISCONNECT_IND has cross */
}

void CsrBtGattAttExecuteWriteCfmHandler(GattMainInst *inst)
{
    ATT_EXECUTE_WRITE_CFM_T *prim  = (ATT_EXECUTE_WRITE_CFM_T *) inst->msg;

    CsrBtGattQueueElement *qElem   = CsrBtGattfindQueueElement(inst, prim->cid);
    CsrBtGattConnElement  *conn    = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &prim->cid);
    CsrBtGattEattResetCid(conn, prim->cid, prim->result);

    if (qElem && qElem->gattMsg)
    {
        l2ca_cid_t cid;
        if(CsrBtGattValidateBtConnIdByMtu(inst, qElem->gattId, qElem->btConnId, &cid, DONT_ALLOCATE_CID) > 0)
        {
            if (qElem->msgState != CSR_BT_GATT_MSG_QUEUE_CANCELLED)
            {
                CsrBtResultCode resultCode;
                CsrBtSupplier   resultSupplier = CSR_BT_SUPPLIER_GATT;

                if (qElem->msgState == CSR_BT_GATT_MSG_QUEUE_EXECUTE_WRITE_CANCEL)
                {
                    resultCode     = CSR_BT_GATT_RESULT_RELIABLE_WRITE_VALIDATION_ERROR;
                    resultSupplier = CSR_BT_SUPPLIER_GATT;
                }
                else
                {
                    resultCode     = (CsrBtResultCode) prim->result;
                    resultSupplier = CSR_BT_GATT_GET_SUPPLIER(prim->result);
                }

                CsrBtGattWriteCfmSend((CsrBtGattPrim *) qElem->gattMsg,
                                      resultCode,
                                      resultSupplier);
            }
            /* Else - This procedure has been cancelled or GATT has 
                      received a ATT_PREPARE_WRITE_CFM with error after
                      it has received at least one successful ATT_PREPARE_WRITE_CFM.
                      E.g. CSR_BT_GATT_WRITE_CFM was sent to the application at the 
                      time GATT received CSR_BT_GATT_CANCEL_REQ or at the time it 
                      received an ATT_PREPARE_WRITE_CFM message with error */

            /* This procedure is finish. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, qElem);
        }
    }
    /* Else - ATT_EXECUTE_WRITE_REQ and ATT_DISCONNECT_IND has cross */
}

void CsrBtGattAttHandleValueCfmHandler(GattMainInst *inst)
{
    ATT_HANDLE_VALUE_CFM_T *prim;
    CsrBtGattConnElement   *conn;

    if ((inst == NULL) || (inst->msg == NULL))
    {
        return;
    }

    prim  = (ATT_HANDLE_VALUE_CFM_T *) inst->msg;
    conn  = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &(prim->cid));

    if(conn == NULL)
        return;

    if (prim->result != ATT_RESULT_SUCCESS_SENT)
    {
        CsrBtGattQueueElement *qElem  = CsrBtGattfindQueueElement(inst, prim->cid);
        /* In case of Indication two ATT_HANDLE_VALUE_CFMs are sent, one when the
          indication was sent (ATT_RESULT_SUCCESS_SENT), and another when the client
          confirmed the indication (ATT_RESULT_SUCCESS)
          So GATT need to reset the CID inside here not in the beginning else
          GATT will think CID is free in case of indication and result into ATT_BUSY
          if GATT will schedule one more operation using the same CID */
        CsrUint16  cidIdentifier      = CsrBtGattEattResetCid(conn, prim->cid, prim->result);

        if (qElem && qElem->gattMsg)
        {
#if defined(CSR_BT_GATT_CACHING) || (!defined(EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL))
            if (qElem->gattId == inst->privateGattId)
            { /* GATT has sent a Service Change Indication message to the peer device */

                if (prim->result == ATT_RESULT_SUCCESS)
                {
#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
                    /* Retriving the gatt message from queue element to check indication conformation is recived for which attribute handle. */
                    CsrBtGattEventSendReq* msg = (CsrBtGattEventSendReq*) qElem->gattMsg;
                    if (msg->attrHandle == GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE(inst->localLeFeatures))
                    {
                        /* Clear the flag for Encrypted Data Key Material change unaware after reciving indication conformation */
                        GATT_CLEAR_ENCRYTPED_DATA_KEY_MATERIAL_CHANGE_UNAWARE(conn->connFlags);
                        /* Handling for Encrypted Data Key Material indication is finished and contiunue next procedure if any. */
                        CsrBtGattQueueRestoreHandler(inst, qElem);
                        return;
                    }
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */
#ifdef CSR_BT_GATT_CACHING
                    if (conn && !CSR_BT_GATT_IS_ROBUST_CACHING_ENABLED(conn->connFlags))
                    {
                        /* For robust caching disabled clients, move the state to change aware
                         * and for robust caching enabled clients, move to change aware
                         * on receiving ATT_CHANGE_AWARE_IND */
                        CSR_BT_GATT_SET_CHANGE_AWARE(conn->connFlags);
                    }
#endif
                }

            }
            else
#endif
            {
                /* If the ATT result is busy then we need to try again so extract the message and need to try sending
                   back to ATT after CSR_BT_GATT_MSG_RETRY_TIMER_VALUE ms timer expiry */
                if (prim->result == ATT_RESULT_BUSY && qElem->msgState != CSR_BT_GATT_MSG_QUEUE_RETRY)
                {
                    CsrUint16 appHandle = INVALID_HANDLE;
                    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                                 &qElem->gattId);
                    if (appElement)
                    {
                        appHandle = appElement->start;
                    }
#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
                    if(appElement && CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONGESTION_STATUS) &&
                        (qElem->msgRetryCount == 0))
                    {
                        CsrBtGattCongestionIndSend(appElement->gattId, conn->btConnId, appElement->start, TRUE);
                    }
#endif /* CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT */

                    /* Wait for CSR_BT_GATT_MSG_RETRY_TIMER_VALUE ms */
                    qElem->txTimer = CsrSchedTimerSet(CSR_BT_GATT_MSG_RETRY_TIMER_VALUE * CSR_SCHED_MILLISECOND,
                        csrBtGattNotificationHandler,
                        appHandle,
                        qElem);

                    CSR_MASK_SET(conn->numOfBearer[cidIdentifier], CHANNEL_IS_BUSY);
                    return;
                }
#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
                else if ((prim->result == ATT_RESULT_SUCCESS) && (qElem->msgRetryCount != 0))
                {
                    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, 
                                                                                 &qElem->gattId);
                    if(appElement && CSR_MASK_IS_SET(appElement->eventMask, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONGESTION_STATUS))
                    {
                        CsrBtGattCongestionIndSend(appElement->gattId, conn->btConnId, appElement->start, FALSE);
                    }
                }
#endif /* CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT */

                CsrBtGattEventSendCfmSend((CsrBtGattEventSendReq*)qElem->gattMsg,
                                          prim->result,
                                          CSR_BT_GATT_GET_SUPPLIER(prim->result));
            }

            /* This procedure is finish. Start the next if any */
            CsrBtGattQueueRestoreHandler(inst, qElem);
        }
        /* Else - ATT_HANDLE_VALUE_REQ and ATT_DISCONNECT_IND has cross */
    }
    /* Else - Indication sent, awaiting confirmation from the client */
}

#if defined(CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION)
#ifndef GATT_OFFLOAD
void CsrBtGattAttHandleValueIndHandler(GattMainInst *inst)
{
    ATT_HANDLE_VALUE_IND_T *prim = (ATT_HANDLE_VALUE_IND_T *) inst->msg;
    CsrBtGattConnElement   *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst,
                                                                            &(prim->cid));

    CsrBtGattClientService *ele;
    CsrBtGattAppElement *appInst;

    if (conn)
    {
        ele = CSR_BT_GATT_CLIENT_SERVICE_LIST_FIND_BY_HANDLE(conn->cliServiceList,
                                                                &(prim->handle));
#ifdef CSR_BT_INSTALL_GATT_BREDR
        if (!ele)
        {
            CsrBtGattConnElement *conn1;

            if (conn->leConnection)
            {
                conn1 = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_BREDR(inst->connInst,
                                             CsrBtGattFindConnInstFromAddressFlagsBredr,
                                             &(conn->peerAddr));
            }
            else
            {
                conn1 = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_LE(inst->connInst,
                                             CsrBtGattFindConnInstFromAddressFlagsLe,
                                             &(conn->peerAddr));
            }

            if (conn1)
            {
                ele = CSR_BT_GATT_CLIENT_SERVICE_LIST_FIND_BY_HANDLE(conn1->cliServiceList,
                                                                        &(prim->handle));
            }
        }
#endif

        if (ele)
        {
            /* Check if the app is still registered */
            appInst = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &ele->gattId);

            if (appInst)
            {
                CsrBtGattNotificationIndicationIndSend(inst,
                                                   prim->cid,
                                                   prim->flags,
                                                   prim->size_value,
                                                   prim->value,
                                                   prim->handle,
                                                   ele->gattId,
                                                   conn->btConnId);
                prim->value = NULL;
                /* attlib_free((ATT_UPRIM_T *) inst->msg); */
            }
            else
            {
                /* TBD: Remove the element */
            }
            return;
        }
    }

    /* Either conn inst or no app has registered to receive the message, 
        auto respond otherwise this might lead to a transport disconnection */
    if (CSR_MASK_IS_SET(prim->flags, ATT_HANDLE_VALUE_INDICATION))
    {
        attlib_handle_value_rsp(CSR_BT_GATT_IFACEQUEUE, prim->cid, NULL);

 #ifdef GATT_CACHING_CLIENT_ROLE
        serviceChangedHandler(inst, conn, prim->handle);
 #endif /* GATT_CACHING_CLIENT_ROLE */
    }
    /* attlib_free((ATT_UPRIM_T *) inst->msg); */
}
#endif
#endif /* CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION */

void CsrBtGattAttAccessIndHandler(GattMainInst *inst)
{
    CsrBtGattAppElement *appElement = (CsrBtGattAppElement *)NULL;
    CsrBtTypedAddr      address;
    CsrUint16           mtu       = CSR_BT_ATT_MTU_DEFAULT;
    CsrBtGattConnInfo   connInfo  = CSR_BT_GATT_CONNINFO_LE;
    CsrBtConnId         btConnId  = CSR_BT_CONN_ID_INVALID;
    att_result_t        attResult = ATT_RESULT_SUCCESS;
    ATT_ACCESS_IND_T    *prim     = (ATT_ACCESS_IND_T *) inst->msg;
    CsrBtAddrZero(&(address));

    if(prim->handle != CSR_BT_GATT_ATTR_HANDLE_INVALID)
    {
        appElement = CsrBtGattAccessIndGetParams(inst, prim->cid, prim->handle, 
                                              &btConnId, &connInfo, &mtu, &address);
    }
    else if(prim->flags == ATT_ACCESS_WRITE_COMPLETE || prim->flags == ATT_ACCESS_WRITE_CANCELLED)
    {
        /* This block is for the ATT Execute Write request. Since there is no attribute handle in ATT Execute write
         * request PDU, bluestack gives CSR_BT_GATT_ATTR_HANDLE_INVALID as handle in the ATT access indication
         * So we enter the below IF block with the first app element from the registered apps list and at the time 
         * of processing the Execute write request, proper handle would be determined from the buffered Prepare write requests.
         */
        appElement = CSR_BT_GATT_APP_INST_GET_FIRST(inst->appInst);
    }

    if (appElement)
    {
        /* This will handle the CSF Characteristic for robust caching and EATT feature */
#if defined(CSR_BT_GATT_CACHING) || defined(CSR_BT_GATT_INSTALL_EATT)
        if (prim->handle == HANDLE_GATT_CLIENT_SUPPORTED_FEATURES)
        {
            CsrBtGattConnElement *connElem = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(inst->connInst, &(prim->cid));
            if(connElem)
            {
                CsrBtGattCsfHandler(connElem, prim);
                return;
            }
        }
#endif

#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
        if (prim->handle == GATT_ENCRYPTED_DATA_KEY_MATERIAL_CLIENT_CONFIG_HANDLE(inst->localLeFeatures) || 
            prim->handle == GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE(inst->localLeFeatures))
        {
            GattGapMsgHandler(inst, prim);
            return;
        }
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */
        /* Found an owner of the Handle and a valid connection  */
#if defined(CSR_BT_GATT_CACHING)
        if (prim->handle == HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG)
        {
            /* GATT Service specific handles */
            CsrBtGattCachingAccessIndHandler(inst, prim);
        }
        else
#endif
        if (CSR_MASK_IS_SET(prim->flags, ATT_ACCESS_READ))
        { /* Has requested to read an attribute value */
            CsrBtGattAccessReadIndSend(inst,
                                       prim->cid,
                                       appElement->gattId,
                                       btConnId,
                                       CsrBtGattResolveServerHandle(appElement->start, prim->handle),
                                       prim->offset,
                                       (CsrUint16)(mtu - CSR_BT_GATT_ATT_READ_HEADER_LENGTH),
                                       csrBtGattGetAccessCheckValue(prim->flags),
                                       connInfo,
                                       address);
        }
        else if (CSR_MASK_IS_SET(prim->flags, ATT_ACCESS_DISALLOWED_OVER_RADIO))
        {
            attlib_access_rsp(CSR_BT_GATT_IFACEQUEUE,
                              prim->cid,
                              prim->handle,
                              CSR_BT_GATT_ACCESS_RES_INVALID_TRANSPORT,
                              0,
                              NULL,
                              NULL);
        }
        else
        { /* Has requested to write an attribute value. */
            if (CSR_MASK_IS_SET(prim->flags, (ATT_ACCESS_WRITE | ATT_ACCESS_WRITE_COMPLETE)))
            { /* A Write Request, a Write Command, or a Signed Write Command has been
                 received */
                CsrBtGattAttrWritePairs *unit = NULL;
                CsrUint16 writeUnitCount      = 0;

#ifdef INSTALL_BT_STANDALONE_MODE
                CSR_LOG_TEXT_INFO((CsrBtGattLto, 0, "Start %x, End %x, gattId %x  handle %x", appElement->start, appElement->end, prim->cid,  prim->handle));

                if(appElement->gattId == inst->privateGattId)
                {
                    attlib_access_rsp(CSR_BT_GATT_IFACEQUEUE, prim->cid, prim->handle, attResult, 0, NULL, NULL);
                    return;
                }
#endif
                
                if (prim->size_value > 0)
                {
                    writeUnitCount      = 1;
                    unit                = CsrPmemAlloc(sizeof(CsrBtGattAttrWritePairs));
                    unit->attrHandle    = CsrBtGattResolveServerHandle(appElement->start,prim->handle);
                    unit->offset        = prim->offset;
                    unit->valueLength   = prim->size_value;
                    unit->value         = prim->value;
                    prim->value         = NULL;
                }
                /* Else - GATT need permission from the application */

                CsrBtGattAccessWriteIndSend(inst,
                                            prim->cid,
                                            appElement->gattId,
                                            btConnId,
                                            csrBtGattGetAccessCheckValue(prim->flags),
                                            connInfo,
                                            address,
                                            writeUnitCount,
                                            unit,
                                            CsrBtGattResolveServerHandle(appElement->start,prim->handle));
            }
            else
            { /* Queued Writes is being used, E.g. a Prepare Write Request or a
                 Execute Write Request has been received */
                if (CSR_MASK_IS_SET(prim->flags, ATT_ACCESS_PERMISSION) || 
                    CSR_MASK_IS_SET(prim->flags, ATT_ACCESS_WRITE))
                { /* An Prepare Write Request has been received */
                    if (prim->size_value > 0)
                    { /* Add data to the prepare write buffer */
                        CsrBtGattPrepareBuffer *buf = CSR_BT_GATT_PREPARE_INST_ADD_LAST(inst->prepare);
                        buf->cid                    = prim->cid;
                        buf->handle                 = prim->handle;
                        buf->offset                 = prim->offset;
                        buf->dataLength             = prim->size_value;
                        buf->data                   = prim->value;
                        buf->state                  = CSR_BT_GATT_PREPEXEC_IDLE;
                        prim->value                 = NULL;
                    }
                    else
                    { /* GATT need permission from the application */
                        CsrBtGattAccessWriteIndSend(inst,
                                                    prim->cid,
                                                    appElement->gattId,
                                                    btConnId,
                                                    csrBtGattGetAccessCheckValue(prim->flags),
                                                    connInfo,
                                                    address,
                                                    0,
                                                    NULL,
                                                    CsrBtGattResolveServerHandle(appElement->start,prim->handle));
                    }
                }
                else if (CSR_MASK_IS_SET(prim->flags, ATT_ACCESS_WRITE_CANCELLED))
                {
                    /* now ATT is sending the execute cancel request to GATT even though the 
                     * prepare request buffer is queued in ATT, this was introduced to indicate
                     * to the application that the access check that came for the prepare 
                     * request are now dropped, but since we are not notifying the application, 
                     * GATT is automatically responding with SUCCESS to ATT */
                    CSR_BT_GATT_PREPARE_INST_CLEAN_UP(inst->prepare, inst, prim->cid, FALSE);
                    attlib_access_rsp(CSR_BT_GATT_IFACEQUEUE, prim->cid, prim->handle, attResult, 0, NULL, NULL);
                    return;
                }
                else
                { /* An Execute Write Request has been received. Empty the prepared write buffer. */
                    CsrBtGattPrepareBuffer *buf = CSR_BT_GATT_PREPARE_INST_FIND_CID(inst->prepare, 
                                                                                    prim->cid,
                                                                                    CsrBtGattPrepareInstFindCidIdleState);

                    /* Sort the list so the smallest offset is placed first */
                    CSR_BT_GATT_PREPARE_INST_SORT_BY_OFFSET(inst->prepare);
                    
                    if (buf)
                    { 
                        CsrBtGattAppElement *tmp = CsrBtGattAccessIndGetParams(inst, buf->cid, buf->handle, 
                                                                      &btConnId, &connInfo, &mtu, &address);

                        if (tmp)
                        {
                            CsrBtGattPrepareAttrElem ids;
                            CsrBtGattAccessCheck check = CSR_BT_GATT_ACCESS_CHECK_NONE;
#ifdef CSR_BT_GATT_INSTALL_SERVER_LONG_WRITE_OFFSET
                            if ((tmp->flags & CSR_BT_GATT_LONG_WRITE_AS_LIST) == CSR_BT_GATT_LONG_WRITE_AS_LIST)
                            {
                                /* Once all values are stored need to send the data pointers to upper service */
                                CSR_BT_GATT_PREPARE_INST_ITERATE_LIST(inst->prepare, buf->cid, buf->handle, check, ids);
                            }
                            else
#endif
                            { /* Generate a list of all the Elements which match the cid/attribute handle pair */
                                CsrBtGattPrepareBuffer   *multi;
                                CSR_BT_GATT_PREPARE_INST_MULTIPLE_ATTR_HANDLES(inst->prepare, &multi, buf->cid, buf->handle);

                                if (multi)
                                { /* GATT need to keep one element of each handle in order to be able to
                                     sent the CSR_BT_GATT_DB_ACCESS_COMPLETE_IND message. Note is only
                                     necessary because multiple attribute handle values is written in a 
                                     single operation. E.g. a Reliable Write has been Requested with multiple
                                     handles */
                                    check = CSR_BT_GATT_ACCESS_CHECK_RELIABLE_WRITE;
                                }
                                /* Else - Remove all element from the list, because GATT shall not sent a
                                          CSR_BT_GATT_DB_ACCESS_COMPLETE_IND message to the application */
                                CSR_BT_GATT_PREPARE_INST_GET_ATTR_LIST(inst->prepare, buf->cid, buf->handle, check, ids);
                            }
#ifdef CSR_BT_GATT_INSTALL_SERVER_HANDLE_RESOLUTION
                            /* Resolving the attribute handles before sending it to application 
                             * It does the attribute handle translation for each attribute write 'unit' 
                             * present in 'ids'  */
                            CSR_BT_GATT_PREPARE_INST_RESOLVE_ATTR_HANDLE(tmp, ids);
#endif
                            /* Send message to the application */
                            CsrBtGattAccessWriteIndSend(inst, 
                                                        prim->cid, 
                                                        tmp->gattId, btConnId, check, connInfo, 
                                                        address, ids.unitCount, ids.unit, 
                                                        ids.attrHandle);
                        }
                        else
                        { /* The application has unregister itself while the prepare write were active. E.g. the
                             attribute handle does not exist anymore */ 
                            attResult = ATT_RESULT_ATTR_NOT_FOUND;
                        }
                    }
                    else
                    { /* No Prepare Write Request with an payload has been received */
                        attResult = ATT_RESULT_WRITE_NOT_PERMITTED;
                    }
                }
            }
        }
    }
    else
    { /* Did not find an owner of the handle or an valid connection */
        attResult = ATT_RESULT_ATTR_NOT_FOUND;
    }

    CSR_LOG_TEXT_INFO((CsrBtGattLto, 0, "Access Rsp Result %x", attResult));

    if (attResult != ATT_RESULT_SUCCESS)
    { /* Make sure that all Prepare Write Elements is remove before ATT is inform */
        CSR_BT_GATT_PREPARE_INST_CLEAN_UP(inst->prepare, inst, prim->cid, FALSE);
        attlib_access_rsp(CSR_BT_GATT_IFACEQUEUE, prim->cid, prim->handle, attResult, 0, NULL, NULL);
    }
}

/* Handler function to send Notification again incase of getting CID as ATT_BUSY */
void csrBtGattNotificationHandler(CsrUint16 start, void* data)
{
    CsrBtGattQueueElement *qElem  = (CsrBtGattQueueElement *) data;

    if (qElem && qElem->gattMsg && (qElem->msgState == CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS))
    {
        CsrUint8 * notificationData;
        CsrBtGattEventSendReq* prim = (CsrBtGattEventSendReq*)qElem->gattMsg;
        CsrBtGattConnElement *conn = CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(gattMainInstPtr->connInst, &prim->btConnId);

        if(conn && prim)
        {
#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
            qElem->msgRetryCount++;
            notificationData = (CsrUint8 *)CsrPmemZalloc(prim->valueLength);
            SynMemCpyS(notificationData, prim->valueLength, qElem->data, prim->valueLength);
#else
            notificationData = qElem->data;
            qElem->msgState = CSR_BT_GATT_MSG_QUEUE_RETRY;
            qElem->data = NULL;
#endif

            if (start != INVALID_HANDLE)
            {
                attlib_handle_value_req(CSR_BT_GATT_IFACEQUEUE,
                                    qElem->cid,
                                    CsrBtGattGetAbsoluteServerHandle(start, prim->attrHandle),
                                    prim->flags,
                                    prim->valueLength,
                                    notificationData,
                                    NULL);
            }
            else
            {
                attlib_handle_value_req(CSR_BT_GATT_IFACEQUEUE,
                                    qElem->cid,
                                    prim->attrHandle,
                                    prim->flags,
                                    prim->valueLength,
                                    notificationData,
                                    NULL);
            }
        }
    }
}

#ifdef CSR_BT_GATT_CACHING
static void CsrBtGattAddRobustCachingCSF(CsrBtTypedAddr *addrt, CsrUint16 changeAware, PHYSICAL_TRANSPORT_T tp_type)
{
    TP_BD_ADDR_T tp_addrt;

    tp_addrt.tp_type = tp_type;
    tp_addrt.addrt = *addrt;

    attlib_add_robust_caching_req(CSR_BT_GATT_IFACEQUEUE,
                                  0,
                                  &tp_addrt,
                                  changeAware,
                                  NULL);
}
#endif

#if defined(CSR_BT_GATT_CACHING) || defined(CSR_BT_GATT_INSTALL_EATT)
void CsrBtGattCsfHandler(CsrBtGattConnElement* conn,
    ATT_ACCESS_IND_T* prim)
{
    att_result_t attResult = ATT_RESULT_SUCCESS;
    CsrUint8* value = NULL;
    CsrUint16 length = 0;

    if (CSR_MASK_IS_SET(prim->flags, ATT_ACCESS_READ))
    {
        /* Read operation for Client Supported Features */
        length = CSR_BT_GATT_CSF_VALUE_LENGTH;
        value = (CsrUint8*)CsrPmemAlloc(length);
#ifdef CSR_BT_GATT_CACHING
        *value = CSR_BT_GATT_IS_ROBUST_CACHING_ENABLED(conn->connFlags) ? 1 : 0;
#endif

#ifdef CSR_BT_GATT_INSTALL_EATT
        /* Read operation for EATT and MHVN */
        if (CSR_BT_GATT_IS_EATT_ENABLED(conn->connFlags))
        {
            *value = *value | CSR_BT_GATT_CSF_EATT_ENABLE;
        }

        if (CSR_BT_GATT_IS_MHVN_ENABLED(conn->connFlags))
        {
            *value = *value | CSR_BT_GATT_CSF_MULTI_HANDLE_VALUE_NTF_ENABLE;
        }
#endif /* CSR_BT_GATT_INSTALL_EATT */
    }
    else if (prim->size_value == CSR_BT_GATT_CSF_VALUE_LENGTH)
    {
        /* Write operation for Client Supported Features */
        CsrUint8 csfVal = prim->value[0];

#ifdef CSR_BT_GATT_CACHING
        if (CSR_BIT_IS_SET(csfVal, GATT_ROBUST_CACHING_FB)) /* 00000001 & (1 << 0) = 1 */
        { /* Set transport type based on the connection info */
            PHYSICAL_TRANSPORT_T tp_type = (conn->leConnection) ? LE_ACL :  BREDR_ACL;

            CSR_BT_GATT_SET_ROBUST_CACHING(conn->connFlags);
            CsrBtGattAddRobustCachingCSF(&conn->peerAddr, ATT_CHANGE_AWARE_CLIENT, tp_type);
        }
        else
        {
            /* A client shall not clear any bits it has set. The server shall
            Respond to any such request with Value Not Allowed error */
            if (CSR_BT_GATT_IS_ROBUST_CACHING_ENABLED(conn->connFlags))
            {
                attResult = ATT_RESULT_VALUE_NOT_ALLOWED;
            }
        }
#endif

#ifdef CSR_BT_GATT_INSTALL_EATT
        if (CSR_BIT_IS_SET(csfVal, GATT_EATT_SUPPORT_FB)) /* 00000010 & (1 << 1) = 2 */
        {
            CSR_BT_GATT_SET_EATT(conn->connFlags);
        }
        else
        {
            /* A client shall not clear EATT bit it has set. The server shall
            Respond to any such request with Value Not Allowed error */
            if (CSR_BT_GATT_IS_EATT_ENABLED(conn->connFlags))
            {
                attResult = ATT_RESULT_VALUE_NOT_ALLOWED;
            }
        }

        if (CSR_BIT_IS_SET(csfVal, GATT_MULT_HANDLE_VALUE_NTF_FB)) /* 00000100 & (1 << 2) = 4 */
        {
            CSR_BT_GATT_SET_MHVN(conn->connFlags);
        }
        else
        {
            /* A client shall not clear MHVN bit it has set. The server shall
            Respond to any such request with Value Not Allowed error */
            if (CSR_BT_GATT_IS_MHVN_ENABLED(conn->connFlags))
            {
                attResult = ATT_RESULT_VALUE_NOT_ALLOWED;
            }
        }
#endif
    }
    else
    {
        /* Write operation for Client Supported Features - invalid length*/
        attResult = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
    }

    attlib_access_rsp(CSR_BT_GATT_IFACEQUEUE, prim->cid,
        prim->handle, attResult, length, value, NULL);
}

void CsrBtGattTddbStoreInfoOnDisconnection(CsrBtGattConnElement *conn)
{
    CsrBtTypedAddr addr;

    /* Assign ID Address if device is bonded */
#if defined(CSR_TARGET_PRODUCT_IOT) || defined(CSR_TARGET_PRODUCT_WEARABLE)
    /* Applicable for IOT config
     * This config is for memory constraint devices, so idAddr is not stored
     * to optimise both RAM and code size.
     * Also incontext API is also used in other scenarios. */
    if (!dm_sm_is_device_bonded(&conn->peerAddr, &addr))
    {
        addr = conn->peerAddr;
    }
#else
    /* For Random Peer Address, use idAddr to check
     * if it is trusted peer device. */
    if (conn->peerAddr.type == CSR_BT_ADDR_PUBLIC)
    {
        addr = conn->peerAddr;
    }
    else
    {
        addr = conn->idAddr;
    }
#endif

    /* Check for any potential GATT Caching actions when a GATT
     * connection is disconnected for a trusted device */
    if (CsrBtTdDbDeviceExists(addr.type, &addr.addr))
    {
        CSR_LOG_TEXT_INFO((CsrBtGattLto,
                           CSR_BT_GATT_LTSO_GENERAL,
                           "Disconnection PS Flags 0x%02X",
                           conn->connFlags));

        CsrBtGattPersistGattInfo(addr.type,
            &addr.addr,
            conn);
    }
}
#endif

/* This function will find and update the current Queue Id for the given Eatt CID 
   And return The respective Queue element which belong to the given Eatt CID   */
CsrBtGattQueueElement* CsrBtGattfindQueueElement(GattMainInst *inst, CsrUint16 cid)
{
    CsrUint8 i;
    CsrBtGattQueueElement *qElem = NULL;
    /* Seach all the Queue and update the qid once qElement for the given CID is found */
    for (i = 0; i < NO_OF_QUEUE ; i++)
    {
        qElem = CSR_BT_GATT_QUEUE_FIND_CID(inst->queue[i], &cid);
        if (qElem)
        {
            inst->qid = i;
            break;
        }
    }
    return qElem;
}

CsrBtGattQueueElement* CsrBtGattfindQueueElementbtConnId(GattMainInst *inst, CsrBtConnId btConnId)
{
    CsrUint8 i;
    CsrBtGattQueueElement *qElem = NULL;
    for (i = 0; i < NO_OF_QUEUE; i++)
    {
        qElem = CSR_BT_GATT_PROGRESS_MSG_FIND_BT_CONN_ID_FROM_QUEUE_MASK(inst->queue[i], &btConnId);
        if (qElem && qElem->cid == ATT_CID_LOCAL)
        {
            inst->qid = i;
            break;
        }
    }
    return qElem;
}

void CsrBtGattEattInvalidateCid(CsrBtGattConnElement *conn, CsrUint16 cid)
{
    CsrUint8 i;
    /* Traverse the loop to find a cid */
    for (i = 0; i < EATT_BEARER_COUNT; i++)
    {
        if (conn->cidSuccess[i] == cid)
        {
            CSR_MASK_SET(conn->numOfBearer[i], CHANNEL_IS_INVALID);
            break;
        }
    }
}

CsrBool CsrBtGattCheckCid(CsrBtGattConnElement *conn, CsrUint16 cid)
{
    CsrUint8 i;
    CsrBool found = FALSE;
    /* Traverse the loop to find a cid */
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

#ifdef GATT_DATA_LOGGER
void CsrBtGattAttDebugIndHandler(GattMainInst *inst)
{
    gattDataLoggerUpstreamAttDebugIndSend( inst->msg );
}
#endif

