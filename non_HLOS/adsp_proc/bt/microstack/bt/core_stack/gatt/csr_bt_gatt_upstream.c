/******************************************************************************
 Copyright (c) 2010-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_private.h"

#ifdef GATT_DATA_LOGGER
#include "gatt_data_logger.h"
#endif /* GATT_DATA_LOGGER */

void CsrBtGattMessagePut(CsrSchedQid phandle, void *msg)
{
    CsrSchedMessagePut(phandle, CSR_BT_GATT_PRIM, msg);

#ifdef GATT_DATA_LOGGER
    gattDataLoggerUpstreamMsgIndSend(phandle, msg);
#endif /* GATT_DATA_LOGGER */
}

void CsrBtGattStdCfmSend(CsrBtGattPrim   type,
                         CsrBtGattId     gattId,
                         CsrBtResultCode resultCode,
                         CsrBtSupplier   resultSupplier)
{
    CsrBtGattStdCfm *msg = (CsrBtGattStdCfm *) 
                            CsrPmemAlloc(sizeof(CsrBtGattStdCfm));
    
    msg->type           = type;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->gattId         = gattId;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattStdBtConnIdCfmSend(CsrBtGattPrim   type,
                                 CsrBtGattId     gattId,
                                 CsrBtResultCode resultCode,
                                 CsrBtSupplier   resultSupplier,
                                 CsrBtConnId     btConnId)
{
    CsrBtGattStdBtConnIdCfm *msg = (CsrBtGattStdBtConnIdCfm *) 
                                    CsrPmemAlloc(sizeof(CsrBtGattStdBtConnIdCfm));
    
    msg->type           = type;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->gattId         = gattId;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattDisconnectIndSend(CsrBtGattId gattId,
                                CsrBtResultCode resultCode,
                                CsrBtSupplier resultSupplier,
                                CsrBtConnId btConnId,
                                CsrBtTypedAddr *address,
                                CsrBtGattConnInfo connInfo)
{
    CsrBtGattDisconnectInd *msg;
    msg = CsrPmemAlloc(sizeof(CsrBtGattDisconnectInd));
    
    msg->type           = CSR_BT_GATT_DISCONNECT_IND;
    msg->reasonCode     = resultCode;
    msg->reasonSupplier = resultSupplier;
    msg->gattId         = gattId;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->address        = *address;
    msg->connInfo       = connInfo;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattConnectIndSend(CsrBtGattId gattId,
                             CsrBtResultCode resultCode,
                             CsrBtSupplier resultSupplier,
                             CsrBtConnId btConnId,
                             CsrBtGattConnInfo connInfo,
                             CsrBtTypedAddr *address,
                             CsrUint16 mtu,
                             CsrBtGattLeRole leRole,
                             l2ca_conflags_t flags)
{
    CsrBtGattConnectInd *msg = (CsrBtGattConnectInd*)CsrPmemAlloc(sizeof(CsrBtGattConnectInd));
    msg->type           = CSR_BT_GATT_CONNECT_IND;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->gattId         = gattId;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->connInfo       = connInfo;
    msg->address        = *address;
    msg->mtu            = mtu;
    msg->leRole         = leRole;
    msg->flags          = flags;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattRegisterCfmSend(CsrSchedQid       qid,
                              CsrBtGattId       gattId,
                              CsrBtResultCode   resultCode, 
                              CsrBtSupplier     resultSupplier,
                              CsrUint16         context)
{
    CsrBtGattRegisterCfm *msg = (CsrBtGattRegisterCfm *) 
                                 CsrPmemAlloc(sizeof(CsrBtGattRegisterCfm));

    msg->type           = CSR_BT_GATT_REGISTER_CFM;
    msg->gattId         = gattId;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->context        = context;
    CsrBtGattMessagePut(qid, msg);
}

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
void CsrBtGattDbAllocCfmSend(CsrBtGattId       gattId,
                             CsrBtResultCode   resultCode, 
                             CsrBtSupplier     resultSupplier,
                             CsrBtGattHandle   start,
                             CsrBtGattHandle   end,
                             CsrUint16         preferredHandle)
{
    CsrBtGattDbAllocCfm *msg = (CsrBtGattDbAllocCfm *) 
                                CsrPmemAlloc(sizeof(CsrBtGattDbAllocCfm));

    msg->type                 = CSR_BT_GATT_DB_ALLOC_CFM;
    msg->gattId               = gattId;
    msg->resultCode           = resultCode;
    msg->resultSupplier       = resultSupplier;
    msg->start                = start;
    msg->end                  = end;
    msg->preferredStartHandle = preferredHandle;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattDbDeallocCfmSend(CsrBtGattId       gattId,
                               CsrBtResultCode   resultCode, 
                               CsrBtSupplier     resultSupplier,
                               CsrBtGattHandle   start,
                               CsrBtGattHandle   end)
{
    CsrBtGattDbDeallocCfm *msg = (CsrBtGattDbDeallocCfm *) 
                                  CsrPmemAlloc(sizeof(CsrBtGattDbDeallocCfm));

    msg->type           = CSR_BT_GATT_DB_DEALLOC_CFM;
    msg->gattId         = gattId;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->start          = start;
    msg->end            = end;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattDbRemoveCfmSend(CsrBtGattId     gattId,
                              CsrBtResultCode resultCode,
                              CsrBtSupplier   resultSupplier,
                              CsrUint16       numOfAttr)
{
    CsrBtGattDbRemoveCfm *msg = (CsrBtGattDbRemoveCfm *) 
                                 CsrPmemAlloc(sizeof(CsrBtGattDbRemoveCfm));
    
    msg->type           = CSR_BT_GATT_DB_REMOVE_CFM;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->gattId         = gattId;
    msg->numOfAttr      = numOfAttr;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}
#endif

void CsrBtGattMtuChangedIndSend(CsrBtGattId     gattId,
                                CsrBtConnId     btConnId,
                                CsrUint16       mtu)
{
    CsrBtGattMtuChangedInd *msg = (CsrBtGattMtuChangedInd *)
                                    CsrPmemAlloc(sizeof(CsrBtGattMtuChangedInd));

    msg->type           = CSR_BT_GATT_MTU_CHANGED_IND;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->mtu            = mtu;
    msg->gattId         = gattId;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattDiscoverServicesIndSend(CsrBtGattQueueElement *qElem,
                                      CsrBtGattHandle       startHandle,
                                      CsrBtGattHandle       endHandle,
                                      CsrUint16             length,
                                      CsrUint8              *data)
{
    if (length == CSR_BT_UUID128_SIZE ||
        length == CSR_BT_UUID16_SIZE)
    {
        CsrBtGattDiscoverServicesInd *msg = (CsrBtGattDiscoverServicesInd*)
                                             CsrPmemAlloc(sizeof(CsrBtGattDiscoverServicesInd));
        
        qElem->msgState     = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK;
        msg->type           = CSR_BT_GATT_DISCOVER_SERVICES_IND;
        msg->gattId         = qElem->gattId;
        msg->btConnId       = (qElem->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
        msg->startHandle    = startHandle;
        msg->endHandle      = endHandle;
        msg->uuid.length    = (CsrUint8) length;
        CsrMemSet(msg->uuid.uuid, 0, CSR_BT_UUID128_SIZE);
        SynMemCpyS(msg->uuid.uuid, CSR_BT_UUID128_SIZE, data, length);    
        CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(qElem->gattId), msg);
    }
}

void CsrBtGattDiscoverCharacIndSend(CsrBtGattQueueElement   *qElem,
                                    CsrBtConnId             btConnId,
                                    CsrBtGattHandle         declarationHandle,
                                    CsrBtUuid               uuid,
                                    CsrUint8                *data)
{
    CsrBtGattDiscoverCharacInd *msg = (CsrBtGattDiscoverCharacInd*)
                                       CsrPmemAlloc(sizeof(CsrBtGattDiscoverCharacInd));
    
    qElem->msgState         = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK;
    msg->type               = CSR_BT_GATT_DISCOVER_CHARAC_IND;
    msg->gattId             = qElem->gattId;
    msg->btConnId           = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->declarationHandle  = declarationHandle;
    msg->uuid               = uuid;
    msg->property           = data[CSR_BT_GATT_CHARAC_PROPERTIES_INDEX];
    msg->valueHandle        = CSR_BT_GATT_GET_HANDLE(data, CSR_BT_GATT_CHARAC_VALUE_HANDLE_FIRST_INDEX);
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(qElem->gattId), msg);
}

void CsrBtGattFindInclServicesIndSend(CsrBtGattQueueElement *qElem,
                                      CsrBtGattHandle       attrHandle,
                                      CsrUint16             length,
                                      CsrUint8              *data)
{
    CsrBtGattFindInclServicesInd *msg = (CsrBtGattFindInclServicesInd*)
                                         CsrPmemAlloc(sizeof(CsrBtGattFindInclServicesInd));
    
    qElem->msgState     = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK;
    msg->type           = CSR_BT_GATT_FIND_INCL_SERVICES_IND;
    msg->gattId         = qElem->gattId;
    msg->btConnId       = (qElem->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->attrHandle     = attrHandle;
    msg->startHandle    = CSR_BT_GATT_GET_HANDLE(data, CSR_BT_GATT_INCLUDE_START_HANDLE_INDEX);
    msg->endGroupHandle = CSR_BT_GATT_GET_HANDLE(data, CSR_BT_GATT_INCLUDE_END_HANDLE_INDEX);
    msg->uuid.length    = (CsrUint8) (length - CSR_BT_GATT_INCLUDE_WITHOUT_UUID_LENGTH);
    CsrMemSet(msg->uuid.uuid, 0, CSR_BT_UUID128_SIZE);
    SynMemCpyS(msg->uuid.uuid, CSR_BT_UUID128_SIZE, &data[CSR_BT_GATT_INCLUDE_UUID_INDEX], msg->uuid.length);
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(qElem->gattId), msg);
}

void CsrBtGattDiscoverCharacDescriptorsIndSend(CsrBtGattQueueElement *qElem,
                                               CsrBtGattHandle       descriptorHandle,
                                               att_uuid_type_t       uuidType,
                                               CsrUint32             *attUuid)
{
    CsrBtGattDiscoverCharacDescriptorsInd *msg = (CsrBtGattDiscoverCharacDescriptorsInd *)
                                                  CsrPmemAlloc(sizeof(CsrBtGattDiscoverCharacDescriptorsInd));
    
    qElem->msgState         = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK;

    msg->type               = CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND;
    msg->gattId             = qElem->gattId;
    msg->btConnId           = (qElem->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->descriptorHandle   = descriptorHandle;
    
    if (uuidType == ATT_UUID16)
    {
        msg->uuid.length = CSR_BT_UUID16_SIZE;
        CsrMemSet(msg->uuid.uuid, 0, CSR_BT_UUID128_SIZE);
        msg->uuid.uuid[0] = (CsrUint8) (((attUuid[0]) & 0x000000FF));
        msg->uuid.uuid[1] = (CsrUint8) (((attUuid[0] >> 8) & 0x000000FF));
    }
    else
    {
        msg->uuid.length = CSR_BT_UUID128_SIZE;
        CSR_COPY_UINT32_TO_LITTLE_ENDIAN(attUuid[0], &(msg->uuid.uuid[12]));    
        CSR_COPY_UINT32_TO_LITTLE_ENDIAN(attUuid[1], &(msg->uuid.uuid[8]));   
        CSR_COPY_UINT32_TO_LITTLE_ENDIAN(attUuid[2], &(msg->uuid.uuid[4]));
        CSR_COPY_UINT32_TO_LITTLE_ENDIAN(attUuid[3], (msg->uuid.uuid));
    }       
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(qElem->gattId), msg);
}

static void csrBtGattStdReadCfmSend(CsrBtGattPrim   type,
                                    CsrBtGattId     gattId,
                                    CsrBtResultCode resultCode,
                                    CsrBtSupplier   resultSupplier,
                                    CsrBtConnId     btConnId,
                                    CsrUint16       valueLength,
                                    CsrUint8        *value,
                                    CsrBtGattHandle handle)
{
    CsrBtGattReadCfm *msg = (CsrBtGattReadCfm*)
                             CsrPmemAlloc(sizeof(CsrBtGattReadCfm));

    msg->type           = type;
    msg->gattId         = gattId;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->valueLength    = valueLength;
    msg->value          = value;
    msg->handle         = handle;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattReadCfmHandler(CsrBtGattReadReq   *prim,
                             CsrBtResultCode    resultCode,
                             CsrBtSupplier      resultSupplier,
                             CsrUint16          valueLength,
                             CsrUint8           **value)
{
    CsrUint8 *tmpValue = *value;

    switch (prim->flags)
    {
        case CSR_BT_GATT_READ_VALUE:
        {
            csrBtGattStdReadCfmSend(CSR_BT_GATT_READ_CFM,
                                    prim->gattId,
                                    resultCode,
                                    resultSupplier,
                                    (prim->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK),
                                    valueLength,
                                    tmpValue,
                                    prim->handle);
            *value = NULL;
            break;
        }
        case CSR_BT_GATT_READ_EXT_PROPERTIES:
        {
            CsrBtGattReadExtendedPropertiesCfm *msg = (CsrBtGattReadExtendedPropertiesCfm*)
                                               CsrPmemAlloc(sizeof(CsrBtGattReadExtendedPropertiesCfm));

            msg->type           = CSR_BT_GATT_READ_EXTENDED_PROPERTIES_CFM;
            msg->gattId         = prim->gattId;
            msg->resultCode     = resultCode;
            msg->resultSupplier = resultSupplier;
            msg->btConnId       = (prim->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
            msg->extProperties  = 0;
            msg->handle         = prim->handle;
            
            if (tmpValue && valueLength >= sizeof(CsrBtGattExtPropertiesBits))
            {
                msg->extProperties  = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(tmpValue);
            }
            CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
            break;
        }
        case CSR_BT_GATT_READ_USER_DESCRIPTOR:
        {
            CsrBtGattReadUserDescriptionCfm *msg = (CsrBtGattReadUserDescriptionCfm*)
                                               CsrPmemAlloc(sizeof(CsrBtGattReadUserDescriptionCfm));

            msg->type           = CSR_BT_GATT_READ_USER_DESCRIPTION_CFM;
            msg->gattId         = prim->gattId;
            msg->resultCode     = resultCode;
            msg->resultSupplier = resultSupplier;
            msg->btConnId       = (prim->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
            msg->usrDescription = CsrUtf8StrTruncate(tmpValue, valueLength);
            msg->usrDescription = tmpValue;
            msg->handle         = prim->handle;
            CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
            *value              = NULL;
            break;
        }
        case CSR_BT_GATT_READ_CLIENT_CONFIGURATION:
        {
            CsrBtGattReadClientConfigurationCfm *msg = (CsrBtGattReadClientConfigurationCfm*)
                                               CsrPmemAlloc(sizeof(CsrBtGattReadClientConfigurationCfm));

            msg->type           = CSR_BT_GATT_READ_CLIENT_CONFIGURATION_CFM;
            msg->gattId         = prim->gattId;
            msg->resultCode     = resultCode;
            msg->resultSupplier = resultSupplier;
            msg->btConnId       = (prim->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
            msg->configuration  = 0;
            msg->handle         = prim->handle;
            
            if (tmpValue && valueLength >= sizeof(CsrBtGattCliConfigBits))
            {
                msg->configuration  = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(tmpValue);
            }
            CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
            break;
        }
        case CSR_BT_GATT_READ_SERVER_CONFIGURATION:
        {
            CsrBtGattReadServerConfigurationCfm *msg = (CsrBtGattReadServerConfigurationCfm*)
                                               CsrPmemAlloc(sizeof(CsrBtGattReadServerConfigurationCfm));

            msg->type           = CSR_BT_GATT_READ_SERVER_CONFIGURATION_CFM;
            msg->gattId         = prim->gattId;
            msg->resultCode     = resultCode;
            msg->resultSupplier = resultSupplier;
            msg->btConnId       = (prim->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
            msg->configuration  = 0;
            msg->handle         = prim->handle;
            
            if (tmpValue && valueLength >= sizeof(CsrBtGattSrvConfigBits))
            {
                msg->configuration  = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(tmpValue);
            }
            CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
            break;
        }
        case CSR_BT_GATT_READ_PRESENTATION_FORMAT:
        {
            CsrBtGattReadPresentationFormatCfm *msg = (CsrBtGattReadPresentationFormatCfm*)
                                               CsrPmemZalloc(sizeof(CsrBtGattReadPresentationFormatCfm));

            msg->type           = CSR_BT_GATT_READ_PRESENTATION_FORMAT_CFM;
            msg->gattId         = prim->gattId;
            msg->resultCode     = resultCode;
            msg->resultSupplier = resultSupplier;
            msg->btConnId       = (prim->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
            msg->handle         = prim->handle;
            
            if (tmpValue && valueLength >= CSR_BT_GATT_CHARAC_PRESENTATION_FORMAT_LENGTH)
            {
                msg->format      = tmpValue[0];
                msg->exponent    = tmpValue[1];
                msg->unit        = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(&tmpValue[2]);
                msg->nameSpace   = tmpValue[4];
                msg->description = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(&tmpValue[5]);
            }
            CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
            break;
        }
        case CSR_BT_GATT_READ_AGGREGATE_FORMAT:
        {
            CsrBtGattReadAggregateFormatCfm *msg = (CsrBtGattReadAggregateFormatCfm*)
                                               CsrPmemZalloc(sizeof(CsrBtGattReadAggregateFormatCfm));

            msg->type           = CSR_BT_GATT_READ_AGGREGATE_FORMAT_CFM;
            msg->gattId         = prim->gattId;
            msg->resultCode     = resultCode;
            msg->resultSupplier = resultSupplier;
            msg->btConnId       = (prim->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
            msg->handle         = prim->handle;

            if (tmpValue && (valueLength % 2) == 0)
            {
                CsrUint16 i, t = 0;

                msg->handles      = (CsrBtGattHandle *) CsrPmemAlloc(valueLength);
                msg->handlesCount = (CsrUint16)(valueLength / 2);
                
                for (i = 0; i < msg->handlesCount; i++)
                {
                    msg->handles[i] = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(&tmpValue[t]);
                    t+=2;
                }
            }
            else
            { /* Validation fails */
                if (msg->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
                {  
                    msg->resultCode = CSR_BT_GATT_RESULT_INVALID_ATTRIBUTE_VALUE_RECEIVED;
                }
                msg->handlesCount = 0;
                msg->handles      = NULL;  
            }
            CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
            break;
        }
        default :
        { /* CSR_BT_GATT_READ_PROFILE_DEFINED */
            csrBtGattStdReadCfmSend(CSR_BT_GATT_READ_PROFILE_DEFINED_DESCRIPTOR_CFM,
                                    prim->gattId,
                                    resultCode,
                                    resultSupplier,
                                    (prim->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK),
                                    valueLength,
                                    tmpValue,
                                    prim->handle);
            *value = NULL;
            break;
        }
    }
}

#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
void CsrBtGattLongReadCfmHandler(CsrBtGattReadReq* prim,
                               CsrBtGattId     gattId,
                               CsrBtResultCode resultCode,
                               CsrBtSupplier   resultSupplier,
                               CsrBtConnId     btConnId,
                               CsrUint16       readUnitCount,
                               CsrBtGattLongAttrRead *readUnit)
{
    CsrBtGattLongReadCfm *msg = (CsrBtGattLongReadCfm *)
                    CsrPmemAlloc(sizeof(CsrBtGattLongReadCfm));

    msg->type                 = CSR_BT_GATT_LONG_READ_CFM;
    msg->gattId               = gattId;
    msg->resultCode           = resultCode;
    msg->resultSupplier       = resultSupplier;
    msg->btConnId             = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->readUnitCount        = readUnitCount;
    msg->readUnit             = readUnit;
    msg->handle               = prim->handle;

    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}
#endif /* CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET */

void CsrBtGattReadMultiCfmSend(CsrBtGattId     gattId,
                               CsrBtResultCode resultCode,
                               CsrBtSupplier   resultSupplier,
                               CsrBtConnId     btConnId,
                               CsrUint16       valueLength,
                               CsrUint8        *value,
                               CsrBtGattHandle *handle)
{
    CsrBtGattReadMultiCfm *msg = (CsrBtGattReadMultiCfm *)
                    CsrPmemAlloc(sizeof(CsrBtGattReadMultiCfm));

    msg->type           = CSR_BT_GATT_READ_MULTI_CFM;
    msg->gattId         = gattId;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->valueLength    = valueLength;
    msg->value          = value;
    msg->handle         = handle ? handle[0] : CSR_BT_GATT_ATTR_HANDLE_INVALID;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

#ifdef CSR_BT_GATT_INSTALL_EATT
void CsrBtGattReadMultiVarCfmSend(CsrBtGattId  gattId,
                               CsrBtResultCode resultCode,
                               CsrBtSupplier   resultSupplier,
                               CsrBtConnId     btConnId,
                               CsrUint16       errorHandle,
                               CsrUint16       valueLength,
                               CsrUint8        *value,
                               CsrUint16       handlesCount,
                               CsrBtGattHandle *handles)
{
    CsrBtGattReadMultiVarCfm *msg = (CsrBtGattReadMultiVarCfm *)
                    CsrPmemAlloc(sizeof(CsrBtGattReadMultiVarCfm));

    msg->type           = CSR_BT_GATT_READ_MULTI_VAR_CFM;
    msg->gattId         = gattId;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->errorHandle    = errorHandle;
    msg->valueLength    = valueLength;
    msg->value          = value;
    msg->handlesCount   = handlesCount;
    msg->handles        = handles;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattAccessMultiReadIndSend(GattMainInst         *inst,
                                     CsrUint16            cid,
                                     CsrBtGattId          gattId,
                                     CsrBtConnId          btConnId,
                                     CsrUint16            mtu,
                                     CsrBtGattAccessCheck check,
                                     CsrBtGattConnInfo    connInfo,
                                     CsrBtTypedAddr       address,
                                     CsrUint16            attrHandlesCount,
                                     CsrBtGattHandle      *handles)
{
   CsrBtGattDbAccessReadMultiVarInd *msg = (CsrBtGattDbAccessReadMultiVarInd *)
                   CsrPmemAlloc(sizeof(CsrBtGattDbAccessReadMultiVarInd));

   CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &cid);

    msg->type               = CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_IND;
    msg->gattId             = gattId;
    msg->btConnId           = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->maxRspValueLength  = mtu;
    msg->check              = check;
    msg->connInfo           = connInfo;
    msg->address            = address;
    msg->attrHandlesCount   = attrHandlesCount;
    msg->attrHandles        = handles;

    CsrBtGattUpStreamMsgSerializer(conn, msg, gattId, btConnId, cid);
}
#endif /* CSR_BT_GATT_INSTALL_EATT */

void CsrBtGattReadByUuidIndSend(CsrBtGattId     gattId,
                                CsrBtConnId     btConnId,
                                CsrBtGattHandle valueHandle,
                                CsrUint16       valueLength,
                                CsrUint8        *value)
{
    CsrBtGattReadByUuidInd *msg = (CsrBtGattReadByUuidInd *)
                                   CsrPmemAlloc(sizeof(CsrBtGattReadByUuidInd));

    msg->type           = CSR_BT_GATT_READ_BY_UUID_IND;
    msg->gattId         = gattId;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->valueHandle    = valueHandle;
    msg->valueLength    = valueLength;
    msg->value          = value;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattReadByUuidCfmSend(CsrBtGattId     gattId,
                                CsrBtResultCode resultCode,
                                CsrBtSupplier   resultSupplier,
                                CsrBtConnId     btConnId,
                                CsrBtUuid       *uuid)
{
    CsrBtGattReadByUuidCfm *msg = (CsrBtGattReadByUuidCfm *)
                                  CsrPmemAlloc(sizeof(CsrBtGattReadByUuidCfm));

    msg->type           = CSR_BT_GATT_READ_BY_UUID_CFM;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->gattId         = gattId;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->uuid           = *uuid;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattWriteCfmSend(CsrBtGattPrim    *prim,
                           CsrBtResultCode  resultCode,
                           CsrBtSupplier    resultSupplier)
{
    CsrBtGattId       gattId;
    CsrBtConnId       btConnId;
    CsrBtGattHandle   handle;
    CsrBtGattWriteCfm *msg = (CsrBtGattWriteCfm *)
                             CsrPmemAlloc(sizeof(CsrBtGattWriteCfm));

    CsrBtGattWriteReq *reqMsg = (CsrBtGattWriteReq *)prim;

    gattId = reqMsg->gattId;
    btConnId = reqMsg->btConnId;

    if (reqMsg->flags != CSR_BT_GATT_WRITE_RELIABLE &&
        reqMsg->attrWritePairsCount &&
        reqMsg->attrWritePairs)
    {
        handle = reqMsg->attrWritePairs[0].attrHandle;
    }
    else
    {
        handle = CSR_BT_GATT_ATTR_HANDLE_INVALID;
    }

    msg->type           = CSR_BT_GATT_WRITE_CFM;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->gattId         = gattId;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->handle         = handle;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
void CsrBtGattCongestionIndSend(CsrBtGattId gattId,
                                CsrBtConnId btConnId,
                                CsrBtGattHandle handle,
                                CsrBool congested)
{
    CsrBtGattCongestionInd *msg = (CsrBtGattCongestionInd*)
                             CsrPmemAlloc(sizeof(CsrBtGattCongestionInd));

    msg->type           = CSR_BT_GATT_CONGESTION_IND;
    msg->gattId         = gattId;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->attrHandle     = handle;
    msg->congested = congested;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}
#endif /* CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT */
/* This function will Serialize the ATT/EATT messages for sending it to the App */
void CsrBtGattUpStreamMsgSerializer(CsrBtGattConnElement* conn,
                                                 void* msg,
                                        CsrBtGattId gattId,
                                      CsrBtConnId btConnId,
                                             CsrUint16 cid)
{
    CsrBtGattAccessIndQueueElement *qElem = CSR_BT_GATT_ACCESS_IND_QUEUE_ADD_LAST(conn->accessIndQueue);

    qElem->gattMsg     = msg;
    qElem->gattId      = gattId;
    qElem->btConnId    = btConnId;
    qElem->cid         = cid;
    qElem->restoreFunc = CsrBtGattMessagePut;

    if (conn->accessIndQueue.count == SINGLE_ELEMENT)
    {
        CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(qElem->gattId), msg);
        qElem->msgState = CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS;
    }
    else
    {
        qElem->msgState = CSR_BT_GATT_MSG_QUEUE_QUEUED;
        CSR_BT_GATT_QUEUE_LOG_QUEUE(qElem->gattMsg);
    }
}

void CsrBtGattAccessReadIndSend(GattMainInst *inst,
                                CsrUint16 cid,
                                CsrBtGattId gattId,
                                CsrBtConnId btConnId,
                                CsrBtGattHandle handle,
                                CsrUint16 offset,
                                CsrUint16 mtu,
                                CsrBtGattAccessCheck check,
                                CsrBtGattConnInfo connInfo,
                                CsrBtTypedAddr address)
{
    CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &cid);
    CsrBtGattDbAccessReadInd *msg;
    msg = CsrPmemAlloc(sizeof(CsrBtGattDbAccessReadInd));

    msg->type = CSR_BT_GATT_DB_ACCESS_READ_IND;
    msg->gattId = gattId;
    msg->btConnId = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->attrHandle = handle;
    msg->offset = offset;
    msg->maxRspValueLength  = mtu;
    msg->check = check;
    msg->connInfo = connInfo;
    msg->address = address;

    CsrBtGattUpStreamMsgSerializer(conn, msg, gattId, btConnId, cid);
}

void CsrBtGattAccessWriteIndSend(GattMainInst *inst,
                                 CsrUint16 cid,
                                 CsrBtGattId gattId,
                                 CsrBtConnId btConnId,
                                 CsrBtGattAccessCheck check,
                                 CsrBtGattConnInfo connInfo,
                                 CsrBtTypedAddr address,
                                 CsrUint16 writeUnitCount,
                                 CsrBtGattAttrWritePairs *writeUnit,
                                 CsrBtGattHandle handle)
{
    CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &cid);
    CsrBtGattDbAccessWriteInd *msg;
    msg = CsrPmemAlloc(sizeof(CsrBtGattDbAccessWriteInd));

    msg->type = CSR_BT_GATT_DB_ACCESS_WRITE_IND;
    msg->gattId = gattId;
    msg->btConnId = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->check = check;
    msg->connInfo = connInfo;
    msg->address = address;
    msg->writeUnitCount = writeUnitCount;
    msg->writeUnit = writeUnit;
    msg->attrHandle = handle;

#ifdef GATT_OFFLOAD
    if (inst->msg != NULL)
    {
        ATT_ACCESS_IND_T *prim = (ATT_ACCESS_IND_T *) inst->msg;

        if ((prim->type == ATT_ACCESS_IND) &&
            (CSR_MASK_IS_SET(prim->flags, ATT_ACCESS_WRITE_CMD)))
        {
            msg->check |= CSR_BT_GATT_ACCESS_WRITE_CMD;
            GATT_LOG_DEBUG("GATT Write command info set");
        }
    }
#endif

    CsrBtGattUpStreamMsgSerializer(conn, msg, gattId, btConnId, cid);
}

void CsrBtGattDbAccessCompleteIndSend(CsrBtGattId gattId,
                                      CsrBtConnId btConnId,
                                      CsrBtGattConnInfo connInfo,
                                      CsrBtTypedAddr address,
                                      CsrBtGattHandle attrHandle,
                                      CsrBool commit)
{
    CsrBtGattDbAccessCompleteInd *msg;
    msg = CsrPmemAlloc(sizeof(CsrBtGattDbAccessCompleteInd));

    msg->type = CSR_BT_GATT_DB_ACCESS_COMPLETE_IND;
    msg->gattId = gattId;
    msg->btConnId = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->connInfo = connInfo;
    msg->address = address;
    msg->attrHandle = attrHandle;
    msg->commit = commit;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
void CsrBtGattReadRemoteLeNameCfmSend(CsrSchedQid appHandle,
                                      CsrUint16   remoteNameLength,
                                      CsrUint8    *remoteName)
{
    CsrBtGattReadRemoteLeNameCfm *msg = (CsrBtGattReadRemoteLeNameCfm *)
                                         CsrPmemAlloc(sizeof(CsrBtGattReadRemoteLeNameCfm));

    msg->type             = CSR_BT_GATT_READ_REMOTE_LE_NAME_CFM;
    msg->remoteNameLength = remoteNameLength;
    msg->remoteName       = remoteName;
    CsrBtGattMessagePut(appHandle, msg);
}
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
void CsrBtGattReadRemoteRpaOnlyCharCfmSend(CsrSchedQid     appHandle,
                                           CsrUint8        rpaOnlyValue,
                                           CsrBtTypedAddr  address,
                                           CsrBtResultCode resultCode,
                                           CsrBtSupplier   resultSupplier)
{
    CsrBtGattReadRemoteRpaOnlyCharCfm *msg = (CsrBtGattReadRemoteRpaOnlyCharCfm *)
                                         CsrPmemAlloc(sizeof(CsrBtGattReadRemoteRpaOnlyCharCfm));

    msg->type           = CSR_BT_GATT_READ_REMOTE_RPA_ONLY_CHAR_CFM;
    msg->rpaOnlyValue   = rpaOnlyValue;
    msg->address        = address;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    CsrBtGattMessagePut(appHandle, msg);
}
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */

void CsrBtGattUnregisterCfmHandler(GattMainInst          *inst, 
                                   CsrBtGattQueueElement *qelm, 
                                   CsrBool               qRestore)
{
    CsrBtGattAppElement *appElement = CSR_BT_GATT_APP_INST_FIND_GATT_ID(inst->appInst, &(qelm->gattId));
    CsrBtGattStdCfmSend(CSR_BT_GATT_UNREGISTER_CFM, qelm->gattId, CSR_BT_GATT_RESULT_SUCCESS, CSR_BT_SUPPLIER_GATT);
    
    if (appElement)
    { /* Remove gattId from list */     
        CSR_BT_GATT_APP_INST_REMOVE(inst->appInst, appElement);
    }
    else
    {
        CsrGeneralException(CsrBtGattLto,
                            0,
                            CSR_BT_GATT_PRIM,
                            0,
                            0,
                            "CSR_BT_GATT_UNREGISTER_REQ no appElement");
    }
    if (qRestore)
    {
        CsrBtGattQueueRestoreHandler(inst, qelm);
    }
}

#ifdef CSR_BT_GATT_INSTALL_FLAT_DB
void CsrBtGattFlatDbRegisterCfmSend(CsrSchedQid       qid,
                                    CsrBtResultCode   resultCode,
                                    CsrBtSupplier     resultSupplier)
{
    CsrBtGattFlatDbRegisterCfm *msg = (CsrBtGattFlatDbRegisterCfm *) 
                                 CsrPmemAlloc(sizeof(CsrBtGattFlatDbRegisterCfm));

    msg->type           = CSR_BT_GATT_FLAT_DB_REGISTER_CFM;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;

    CsrBtGattMessagePut(qid, msg);
}
#endif /* CSR_BT_GATT_INSTALL_FLAT_DB */

#ifdef CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
void CsrBtGattNotificationIndicationIndSend(GattMainInst *inst,
                                                  CsrUint16 cid,
                                                  CsrUint16 flags,
                                                  CsrUint16 size_value,
                                                  CsrUint8 *value,
                                                  CsrUint16 handle,
                                                  CsrBtGattId gattId,
                                                  CsrBtConnId btConnId)
{
    CsrBtGattClientIndicationInd *msg;

    msg = CsrPmemAlloc(sizeof(CsrBtGattClientIndicationInd));

    msg->type = CSR_MASK_IS_SET(flags, ATT_HANDLE_VALUE_INDICATION) ? 
                    CSR_BT_GATT_CLIENT_INDICATION_IND: CSR_BT_GATT_CLIENT_NOTIFICATION_IND;
    msg->value = (size_value != 0) ? value: NULL;
    msg->valueLength = size_value;
    msg->valueHandle = handle;
    msg->btConnId = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->gattId = gattId;

#ifndef GATT_OFFLOAD
    if (msg->type == CSR_BT_GATT_CLIENT_INDICATION_IND)
    {
        CsrBtGattConnElement* conn = CSR_BT_GATT_CONN_INST_FIND_CID(inst->connInst, &cid);
        CsrBtGattUpStreamMsgSerializer(conn, msg, gattId, btConnId, cid);
    }
    else
#endif
    {
        CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
    }
}
#endif

void CsrBtGattClientExchangeMtuCfmSend(CsrBtGattId      gattId, 
                                        CsrBtConnId     btConnId, 
                                        CsrUint16       mtu,
                                        CsrBtResultCode resultCode,
                                        CsrBtSupplier   resultSupplier)
{
    CsrBtGattClientExchangeMtuCfm *msg = CsrPmemAlloc(sizeof(*msg));

    msg->type = CSR_BT_GATT_CLIENT_EXCHANGE_MTU_CFM;
    msg->gattId = gattId;
    msg->btConnId = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->mtu = mtu;
    msg->resultCode = resultCode;
    msg->resultSupplier = resultSupplier;

    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

void CsrBtGattRemoteClientExchangeMtuIndSend(CsrBtGattId gattId, 
                                             CsrBtConnId btConnId, 
                                             CsrUint16   mtu)
{
    CsrBtGattRemoteClientExchangeMtuInd *msg = CsrPmemAlloc(sizeof(*msg));

    msg->type = CSR_BT_GATT_REMOTE_CLIENT_EXCHANGE_MTU_IND;
    msg->gattId = gattId;
    msg->btConnId = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->mtu = mtu;

    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

#ifdef CSR_BT_INSTALL_GATT_BREDR
void CsrBtGattConnectBredrIndSend(CsrBtGattId gattId,
                                  CsrBtTypedAddr *address,
                                  CsrUint16 mtu)
{
    CsrBtGattConnectBredrInd *msg = (CsrBtGattConnectBredrInd*)CsrPmemAlloc(sizeof(CsrBtGattConnectBredrInd));

    msg->type           = CSR_BT_GATT_CONNECT_BREDR_IND;
    msg->gattId         = gattId;
    msg->address        = *address;
    msg->mtu            = mtu;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}
#endif

#ifdef CSR_BT_GATT_INSTALL_EATT
void CsrBtGattEattConnectIndSend(CsrBtGattId   gattId, 
                              CsrBtConnId      btConnId, 
                              CsrUint8         eattSupportedServer, 
                              CsrBtResultCode  resultCode, 
                              CsrBtSupplier    resultSupplier)
{
    CsrBtGattEattConnectInd *msg = (CsrBtGattEattConnectInd *) 
                                 CsrPmemAlloc(sizeof(CsrBtGattEattConnectInd));

    msg->type           = CSR_BT_GATT_EATT_CONNECT_IND;
    msg->gattId         = gattId;
    msg->btConnId       = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->eattFeature    = eattSupportedServer;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}

/* App priority cfm message to the respective App. */
void CsrBtGattPriorityCfmSend(CsrBtGattId    gattId, 
                           CsrBtConnId       btConnId, 
                           CsrBtResultCode   resultCode, 
                           CsrBtSupplier     resultSupplier)
{
    CsrBtGattAppPriorityChangeCfm *msg = (CsrBtGattAppPriorityChangeCfm *) 
                                 CsrPmemAlloc(sizeof(CsrBtGattAppPriorityChangeCfm));

    msg->type           = CSR_BT_GATT_APP_PRIORITY_CHANGE_CFM;
    msg->gattId         = gattId;
    msg->btConnId       = btConnId;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}
#endif /* CSR_BT_GATT_INSTALL_EATT */

#ifdef GATT_CACHING_CLIENT_ROLE
void GattRemoteDatabaseChangedInd(CsrBtGattId             gattId,
                                       CsrBtConnId             btConnId,
                                       GattRemoteDbChangedFlag indType,
                                       CsrBtGattHandle         startHandle,
                                       CsrBtGattHandle         endHandle)
{
    GattRemoteDbChangedInd*msg = (GattRemoteDbChangedInd*)
                                 CsrPmemAlloc(sizeof(GattRemoteDbChangedInd));

    msg->type                      = GATT_REMOTE_DB_CHANGED_IND;
    msg->gattId                    = gattId;
    msg->btConnId                  = (btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);
    msg->indType                   = indType;
    msg->serviceChangeStartHandle  = startHandle;
    msg->serviceChangeEndHandle    = endHandle;

    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(gattId), msg);
}
#endif /* GATT_CACHING_CLIENT_ROLE */

void CsrBtGattEventSendCfmSend(CsrBtGattEventSendReq *prim,
                               CsrBtResultCode resultCode,
                               CsrBtSupplier   resultSupplier)
{
    CsrBtGattEventSendCfm *msg = (CsrBtGattEventSendCfm *) 
                                    CsrPmemZalloc(sizeof(CsrBtGattEventSendCfm));

    msg->type           = CSR_BT_GATT_EVENT_SEND_CFM;
    msg->resultCode     = resultCode;
    msg->resultSupplier = resultSupplier;
    msg->gattId         = prim->gattId;
    msg->btConnId       = (prim->btConnId & CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK);

#ifdef GATT_OFFLOAD
    msg->attrHandle     = prim->attrHandle;
#endif
    CsrBtGattMessagePut(CSR_BT_GATT_GET_QID_FROM_GATT_ID(prim->gattId), msg);
}


