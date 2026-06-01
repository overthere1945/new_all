/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/
#include "csr_synergy.h"

#include "csr_bt_gatt_private.h"
#include "csr_bt_gatt_private_lib.h"

void CsrBtGattMsgTransport(void *msg)
{
    CsrMsgTransport(CSR_BT_GATT_IFACEQUEUE, CSR_BT_GATT_PRIM, msg);
}

/* Covers Registration and Un-register an application instance to Gatt */
CsrBtGattRegisterReq *CsrBtGattRegisterReq_struct(CsrSchedQid qid, CsrUint16 context)
{
    CsrBtGattRegisterReq *prim = (CsrBtGattRegisterReq *)
                                  CsrPmemAlloc(sizeof(CsrBtGattRegisterReq));

    prim->type       = CSR_BT_GATT_REGISTER_REQ;
    prim->pHandle    = qid;
    prim->context    = context;
    return prim;
}

CsrBtGattUnregisterReq *CsrBtGattUnregisterReq_struct(CsrBtGattId gattId)
{
    CsrBtGattUnregisterReq *prim = (CsrBtGattUnregisterReq *)
                                    CsrPmemAlloc(sizeof(CsrBtGattUnregisterReq));

    prim->type      = CSR_BT_GATT_UNREGISTER_REQ;
    prim->gattId    = gattId;
    return prim;
}
/* Covers DataBase Configuration */
CsrBtGattDbAllocReq *CsrBtGattDbAllocReq_struct(CsrBtGattId gattId,
                                                CsrUint16   numOfAttrHandles,
                                                CsrUint16   preferredStartHandle)
{
    CsrBtGattDbAllocReq *prim   = (CsrBtGattDbAllocReq *)
                                   CsrPmemAlloc(sizeof(CsrBtGattDbAllocReq));

    prim->type                  = CSR_BT_GATT_DB_ALLOC_REQ;
    prim->gattId                = gattId;
    prim->numOfAttrHandles      = numOfAttrHandles;
    prim->preferredStartHandle  = preferredStartHandle; 
    return prim;
}

CsrBtGattDbDeallocReq *CsrBtGattDbDeallocReq_struct(CsrBtGattId gattId)
{
    CsrBtGattDbDeallocReq *prim = (CsrBtGattDbDeallocReq *)
                                   CsrPmemAlloc(sizeof(CsrBtGattDbDeallocReq));

    prim->type      = CSR_BT_GATT_DB_DEALLOC_REQ;
    prim->gattId    = gattId;
    return prim;
}

CsrBtGattDbAddReq *CsrBtGattDbAddReq_struct(CsrBtGattId gattId,
                                            CsrBtGattDb *db)
{
    CsrBtGattDbAddReq *prim = (CsrBtGattDbAddReq *)
                               CsrPmemAlloc(sizeof(CsrBtGattDbAddReq));

    prim->type      = CSR_BT_GATT_DB_ADD_REQ;
    prim->gattId    = gattId;
    prim->db        = db;
    return prim;
}

CsrBtGattDbRemoveReq *CsrBtGattDbRemoveReq_struct(CsrBtGattId       gattId,
                                                  CsrBtGattHandle   start,
                                                  CsrBtGattHandle   end)
{
    CsrBtGattDbRemoveReq *prim = (CsrBtGattDbRemoveReq *)
                                  CsrPmemAlloc(sizeof(CsrBtGattDbRemoveReq));

    prim->type      = CSR_BT_GATT_DB_REMOVE_REQ;
    prim->gattId    = gattId;
    prim->start     = start;
    prim->end       = end;
    return prim;
}

CsrBtGattDbAccessRes *CsrBtGattDbAccessRes_struct(CsrBtGattId              gattId,
                                                  CsrBtConnId              btConnId,
                                                  CsrBtGattHandle          attrHandle,
                                                  CsrBtGattDbAccessRspCode responseCode,
                                                  CsrUint16                valueLength,
                                                  CsrUint8                 *value)
{
    CsrBtGattDbAccessRes *prim = (CsrBtGattDbAccessRes *)
                                  CsrPmemAlloc(sizeof(CsrBtGattDbAccessRes));

    prim->type          = CSR_BT_GATT_DB_ACCESS_RES;
    prim->gattId        = gattId;
    prim->btConnId      = btConnId;
    prim->attrHandle    = attrHandle;
    prim->responseCode  = responseCode;
    prim->valueLength   = valueLength;
    prim->value         = value;
    return prim;
}

/* Covers Server Initiated Notification and Indication  */
CsrBtGattEventSendReq *CsrBtGattEventSendReq_struct(CsrBtGattId       gattId,
                                                    CsrBtConnId       btConnId,
                                                    CsrBtGattHandle   attrHandle,
                                                    CsrBtGattHandle   endGroupHandle,
                                                    CsrUint16         flags,
                                                    CsrUint16         valueLength,
                                                    CsrUint8          *value)
{
    CsrBtGattEventSendReq *prim = (CsrBtGattEventSendReq *)
                                     CsrPmemAlloc(sizeof(CsrBtGattEventSendReq));

    prim->type           = CSR_BT_GATT_EVENT_SEND_REQ;
    prim->gattId         = gattId;
    prim->btConnId       = btConnId;
    prim->attrHandle     = attrHandle;
    prim->endGroupHandle = endGroupHandle;
    prim->flags          = flags;
    prim->valueLength    = valueLength;
    prim->value          = value;
    return prim;
}

/* Covers item 2, Primary Service Discovery, in the GATT feature table  */
CsrBtGattDiscoverServicesReq *CsrBtGattDiscoverServicesReq_struct(CsrBtGattId gattId,
                                                                  CsrBtConnId btConnId,
                                                                  CsrBtUuid   uuid)
{
    CsrBtGattDiscoverServicesReq *prim = (CsrBtGattDiscoverServicesReq *)
                                          CsrPmemAlloc(sizeof(CsrBtGattDiscoverServicesReq));

    prim->type      = CSR_BT_GATT_DISCOVER_SERVICES_REQ;
    prim->gattId    = gattId;
    prim->btConnId  = btConnId;
    prim->uuid      = uuid;
    return prim;
}

/* Covers item 3, Relationship Discovery, in the GATT feature table */
CsrBtGattFindInclServicesReq *CsrBtGattFindInclServicesReq_struct(CsrBtGattId     gattId,
                                                                  CsrBtConnId     btConnId,
                                                                  CsrBtGattHandle startHandle,
                                                                  CsrBtGattHandle endGroupHandle)
{
    CsrBtGattFindInclServicesReq *prim = (CsrBtGattFindInclServicesReq *)
                                          CsrPmemAlloc(sizeof(CsrBtGattFindInclServicesReq));

    prim->type           = CSR_BT_GATT_FIND_INCL_SERVICES_REQ;
    prim->gattId         = gattId;
    prim->btConnId       = btConnId;
    prim->startHandle    = startHandle;
    prim->endGroupHandle = endGroupHandle;
    return prim;
}

/* Covers item 4, Characteristic Discovery, in the GATT feature table */
CsrBtGattDiscoverCharacReq *CsrBtGattDiscoverCharacReq_struct(CsrBtGattId     gattId,
                                                              CsrBtConnId     btConnId,
                                                              CsrBtUuid       uuid,
                                                              CsrBtGattHandle startHandle,
                                                              CsrBtGattHandle endGroupHandle)
{
    CsrBtGattDiscoverCharacReq *prim = (CsrBtGattDiscoverCharacReq *)
                                        CsrPmemAlloc(sizeof(CsrBtGattDiscoverCharacReq));

    prim->type           = CSR_BT_GATT_DISCOVER_CHARAC_REQ;
    prim->gattId         = gattId;
    prim->btConnId       = btConnId;
    prim->uuid           = uuid;
    prim->startHandle    = startHandle;
    prim->endGroupHandle = endGroupHandle;
    return prim;
}

/* Covers item 5, Characteristic Descriptor Discovery, in the GATT feature table */
CsrBtGattDiscoverCharacDescriptorsReq *CsrBtGattDiscoverCharacDescriptorsReq_struct(CsrBtGattId     gattId,
                                                                                    CsrBtConnId     btConnId,
                                                                                    CsrBtGattHandle startHandle,
                                                                                    CsrBtGattHandle endGroupHandle)
{
    CsrBtGattDiscoverCharacDescriptorsReq *prim = (CsrBtGattDiscoverCharacDescriptorsReq *)
                                                   CsrPmemAlloc(sizeof(CsrBtGattDiscoverCharacDescriptorsReq));

    prim->type           = CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_REQ;
    prim->gattId         = gattId;
    prim->btConnId       = btConnId;
    prim->startHandle    = startHandle;
    prim->endGroupHandle = endGroupHandle;
    return prim;
}

/* Covers item 6, Characteristic Value Read, and item 10 Read Characteristic Descriptor Value 
   in the GATT feature table */
CsrBtGattReadReq *CsrBtGattReadReq_struct(CsrBtGattId     gattId,
                                          CsrBtConnId     btConnId,
                                          CsrBtGattHandle handle,
                                          CsrUint16       offset,
                                          CsrUint16       flags)
{
    CsrBtGattReadReq *prim = (CsrBtGattReadReq *)
                              CsrPmemAlloc(sizeof(CsrBtGattReadReq));

    prim->type      = CSR_BT_GATT_READ_REQ;
    prim->gattId    = gattId;
    prim->btConnId  = btConnId;
    prim->handle    = handle;
    prim->flags     = flags;
    prim->offset    = offset;    
    return prim;
}

CsrBtGattReadByUuidReq *CsrBtGattReadByUuidReq_struct(CsrBtGattId       gattId,
                                                      CsrBtConnId       btConnId,
                                                      CsrBtGattHandle   startHandle,
                                                      CsrBtGattHandle   endGroupHandle,
                                                      CsrBtUuid         uuid)
{
    CsrBtGattReadByUuidReq *prim = (CsrBtGattReadByUuidReq *)
                                    CsrPmemAlloc(sizeof(CsrBtGattReadByUuidReq));

    prim->type           = CSR_BT_GATT_READ_BY_UUID_REQ;
    prim->gattId         = gattId;
    prim->btConnId       = btConnId;
    prim->startHandle    = startHandle;
    prim->endGroupHandle = endGroupHandle;
    prim->uuid           = uuid;
    return prim;
}

CsrBtGattReadMultiReq *CsrBtGattReadMultiReq_struct(CsrBtGattId     gattId,
                                                    CsrBtConnId     btConnId,
                                                    CsrUint16       handlesCount,
                                                    CsrBtGattHandle *handles)
{
    CsrBtGattReadMultiReq *prim = (CsrBtGattReadMultiReq *)
                                   CsrPmemAlloc(sizeof(CsrBtGattReadMultiReq));

    prim->type           = CSR_BT_GATT_READ_MULTI_REQ;
    prim->gattId         = gattId;
    prim->btConnId       = btConnId;
    prim->handlesCount   = handlesCount;
    prim->handles        = handles;
    return prim;
}

/* Covers item 7, Characteristic Value Write, in the GATT feature table */
CsrBtGattWriteReq *CsrBtGattWriteReq_struct(CsrBtGattId             gattId,
                                            CsrBtConnId             btConnId,
                                            CsrUint16               flags,
                                            CsrUint16               attrWritePairsCount,
                                            CsrBtGattAttrWritePairs *attrWritePairs)
{
    CsrBtGattWriteReq *prim = (CsrBtGattWriteReq *)
                               CsrPmemAlloc(sizeof(CsrBtGattWriteReq));

    prim->type                = CSR_BT_GATT_WRITE_REQ;
    prim->gattId              = gattId;
    prim->btConnId            = btConnId;
    prim->flags               = flags;
    prim->attrWritePairsCount = attrWritePairsCount;
    prim->attrWritePairs      = attrWritePairs;
    return prim;
}


/* Construct the CsrBtGattCancelReq primitive */
CsrBtGattCancelReq *CsrBtGattCancelReq_struct(CsrBtGattId gattId,
                                              CsrBtConnId btConnId)
{
    CsrBtGattCancelReq *prim = CsrPmemAlloc(sizeof(CsrBtGattCancelReq));
    prim->type     = CSR_BT_GATT_CANCEL_REQ;
    prim->gattId   = gattId;
    prim->btConnId = btConnId;
    return prim;
}

/* Construct the CsrBtGattSetEventMaskReq primitive */
CsrBtGattSetEventMaskReq *CsrBtGattSetEventMaskReq_struct(CsrBtGattId        gattId,
                                                          CsrBtGattEventMask eventMask)
{
    CsrBtGattSetEventMaskReq *prim = (CsrBtGattSetEventMaskReq *)
                                      CsrPmemAlloc(sizeof(CsrBtGattSetEventMaskReq));

    prim->type      = CSR_BT_GATT_SET_EVENT_MASK_REQ;
    prim->gattId    = gattId;
    prim->eventMask = eventMask;
    return prim;
}

/* Covers private GATT libs */
CsrBtGattReadRemoteLeNameReq *CsrBtGattReadRemoteLeNameReq_struct(CsrSchedQid    qid,
                                                                  CsrBtTypedAddr address)
{
    CsrBtGattReadRemoteLeNameReq *prim = (CsrBtGattReadRemoteLeNameReq *)
                                          CsrPmemAlloc(sizeof(CsrBtGattReadRemoteLeNameReq));

    prim->type    = CSR_BT_GATT_READ_REMOTE_LE_NAME_REQ;
    prim->pHandle = qid;
    prim->address = address;
    return prim;
}

CsrBtGattClientRegisterServiceReq *CsrBtGattClientRegisterServiceReq_struct(CsrBtGattId gattId, 
                                                                                CsrBtGattHandle start,
                                                                                CsrBtGattHandle end,
                                                                                CsrBtTypedAddr address)
{
    CsrBtGattClientRegisterServiceReq *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type     = CSR_BT_GATT_CLIENT_REGISTER_SERVICE_REQ;
    prim->gattId   = gattId;
    prim->start    = start;
    prim->end      = end;
    prim->address  = address;
    return prim;
}

CsrBtGattClientIndicationRsp *CsrBtGattClientIndicationRsp_struct(CsrBtGattId gattId, 
                                                                                 CsrBtConnId btConnId)
{
    CsrBtGattClientIndicationRsp *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type     = CSR_BT_GATT_CLIENT_INDICATION_RSP;
    prim->gattId   = gattId;
    prim->btConnId = btConnId;
    return prim;
}

/* Construct the CsrBtGattFlatDbRegisterReq primitive */
CsrBtGattFlatDbRegisterReq *CsrBtGattFlatDbRegisterReq_struct(CsrSchedQid qid,
                                                              CsrUint16 flags,
                                                              CsrUint16 size,
                                                              CsrUint16 *db)
{
    CsrBtGattFlatDbRegisterReq *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type      = CSR_BT_GATT_FLAT_DB_REGISTER_REQ;
    prim->pHandle   = qid;
    prim->flags     = flags;
    prim->size      = size;
    prim->db        = db;
    return prim;
}

/* Construct the CsrBtGattFlatDbRegisterHandleRangeReq primitive */
CsrBtGattFlatDbRegisterHandleRangeReq *CsrBtGattFlatDbRegisterHandleRangeReq_struct( \
                                                          CsrBtGattId gattId,
                                                          CsrUint16 start,
                                                          CsrUint16 end)
{
    CsrBtGattFlatDbRegisterHandleRangeReq *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type      = CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_REQ;
    prim->gattId    = gattId;
    prim->start     = start;
    prim->end       = end;
    return prim;
}

CsrBtGattClientExchangeMtuReq *CsrBtGattClientExchangeMtuReq_struct(CsrBtGattId gattId, 
                                                                    CsrBtConnId btConnId,
                                                                    CsrUint16   mtu)
{
    CsrBtGattClientExchangeMtuReq *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type      = CSR_BT_GATT_CLIENT_EXCHANGE_MTU_REQ;
    prim->gattId    = gattId;
    prim->btConnId  = btConnId;
    prim->mtu       = mtu;
    return prim;
}

CsrBtGattRemoteClientExchangeMtuRes *CsrBtGattRemoteClientExchangeMtuRes_struct(CsrBtGattId gattId, 
                                                                                CsrBtConnId btConnId,
                                                                                CsrUint16   mtu)
{
    CsrBtGattRemoteClientExchangeMtuRes *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type      = CSR_BT_GATT_REMOTE_CLIENT_EXCHANGE_MTU_RES;
    prim->gattId    = gattId;
    prim->btConnId  = btConnId;
    prim->mtu       = mtu;
    return prim;
}

CsrBtGattDbCommitReq *CsrBtGattDbCommitReq_struct(CsrBtGattId gattId)
{
    CsrBtGattDbCommitReq *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type      = CSR_BT_GATT_DB_COMMIT_REQ;
    prim->gattId    = gattId;
    return prim;
}

#ifdef CSR_BT_GATT_INSTALL_EATT
/* --------------------------------------------------------------------
  Name: 
    CsrBtGattPriorityReqSend
    
  Description: 
    This function lets the application set the Priority

  Parameters: 
    CsrBtGattId     _gattId        - Application identifier
    CsrBtConnId     _btConnId      - Connection identifier
    CsrUint8        _priority      - Application Priority in csr_bt_gatt_prim.h
                                     Valid Values for setting the priority is as below:
                                     CSR_BT_APP_PRIORITY_HIGH
                                     CSR_BT_APP_PRIORITY_MEDIUM
                                     CSR_BT_APP_PRIORITY_LOW
   -------------------------------------------------------------------- */
CsrBtGattAppPriorityChangeReq *CsrBtGattAppPriorityChangeReq_struct(CsrBtGattId gattId, 
                                                        CsrBtConnId btConnId,
                                                        CsrUint8    priority)
{
    CsrBtGattAppPriorityChangeReq *prim = (CsrBtGattAppPriorityChangeReq *)
                                  CsrPmemAlloc(sizeof(CsrBtGattAppPriorityChangeReq));

    prim->type          = CSR_BT_GATT_APP_PRIORITY_CHANGE_REQ;
    prim->gattId        = gattId;
    prim->btConnId      = btConnId;
    prim->AppPriority   = priority;
    return prim;
}

/* --------------------------------------------------------------------
  Name: 
    CsrBtGattReadMultiVarReq
    
  Description: 
    This function Covers item 4.8, Read Multiple Variable Length Characteristic
    in the GATT feature table 
  Parameters:
    CsrBtGattId     _gattId        - Application identifier
    CsrBtConnId     _btConnId      - Connection identifier
    CsrUint16       _handlesCount  - Number of attribute Handles that must be read
    CsrBtGattHandle _*handles      - An allocated pointer of two or more attribute handles
    -------------------------------------------------------------------- */

CsrBtGattReadMultiVarReq *CsrBtGattReadMultiVarReq_struct(CsrBtGattId gattId, 
                                                    CsrBtConnId     btConnId, 
                                                    CsrUint16   handlesCount, 
                                                    CsrBtGattHandle *handles)
{
    CsrBtGattReadMultiVarReq *prim = (CsrBtGattReadMultiVarReq *)
                                  CsrPmemAlloc(sizeof(CsrBtGattReadMultiVarReq));

    prim->type                = CSR_BT_GATT_READ_MULTI_VAR_REQ;
    prim->gattId              = gattId;
    prim->btConnId            = btConnId;
    prim->handlesCount        = handlesCount;
    prim->handles             = handles;
    return prim;
}

/* --------------------------------------------------------------------
  Name: 
    CsrBtGattDbAccessReadMultiVarRsp
    
  Description: 
    This function Covers item 4.8, Read Multiple Variable Length Characteristic RSP
    in the GATT feature table 
   Parameters:
   CsrBtGattId _gattId               - Application identifier
   CsrBtConnId _btConnId             - Connection identifier
   CsrBtGattDbAccessRspCode          _rspCode - One of the responseCodes defined
   in csr_bt_gatt_prim.h

   CsrBtGattHandle                   _errHandle handle of the first attribute in case of error
   received in the
   CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_IND message.

   CsrUint16 _valueLength            - Length of the attributes value which
   has been read by the application
   CsrUint8 *_values                 - An allocated pointer of the attribute
   value.
   -------------------------------------------------------------------- */

CsrBtGattDbAccessReadMultiVarRsp *CsrBtGattDbReadMultiVarRes_struct(CsrBtGattId gattId,
                                                 CsrBtConnId                  btConnId,
                                                 CsrBtGattDbAccessRspCode responseCode,
                                                 CsrBtGattHandle           errorHandle,
                                                 CsrUint16                valuesLength,
                                                 CsrUint8                      *values)
{
    CsrBtGattDbAccessReadMultiVarRsp *prim = (CsrBtGattDbAccessReadMultiVarRsp *)
                                  CsrPmemAlloc(sizeof(CsrBtGattDbAccessReadMultiVarRsp));

    prim->type                = CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_RSP;
    prim->gattId              = gattId;
    prim->btConnId            = btConnId;
    prim->responseCode        = responseCode;
    prim->errorHandle         = errorHandle;
    prim->valuesLength        = valuesLength;
    prim->values              = values;
    return prim;
}

#ifdef GATT_ENABLE_EATT_DISCONNECT
CsrBtGattEattDisconnectReq *CsrBtGattEattDisconnectReq_struct(CsrBtGattId gattId,
                                                      CsrBtConnId btConnId,
                                                      CsrUint16 numCid2Disc)
{
    CsrBtGattEattDisconnectReq *prim;
    prim = (CsrBtGattEattDisconnectReq *) CsrPmemAlloc(sizeof(CsrBtGattEattDisconnectReq));
    prim->type = CSR_BT_GATT_EATT_DISCONNECT_REQ;
    prim->gattId = gattId;
    prim->btConnId = btConnId;
    prim->numCid2Disc = numCid2Disc;
    return prim;
}
#endif /* GATT_ENABLE_EATT_DISCONNECT */

#endif /* CSR_BT_GATT_INSTALL_EATT */

/* --------------------------------------------------------------------
  Name:
    CsrBtGattConfigModeReqSend

  Description:
    This function let's the application set the Mode for list based Long write/read procedure.

  Parameters:
    CsrBtGattId     _gattId        - Application identifier
    CsrUint8        _flags         - Application Mode for long write procedure in csr_bt_gatt_prim.h
                                     Valid Value for setting the mode is as below:
                                     CSR_BT_GATT_LONG_WRITE_AS_LIST(0x01)
                                     CSR_BT_GATT_LONG_READ_AS_LIST(0x02)
   -------------------------------------------------------------------- */
CsrBtGattConfigModeReq *CsrBtGattConfigModeReq_struct(CsrBtGattId gattId,
                                                          CsrUint8 flags)
{
    CsrBtGattConfigModeReq *prim = (CsrBtGattConfigModeReq *)
                                  CsrPmemAlloc(sizeof(CsrBtGattConfigModeReq));
    prim->type          = CSR_BT_GATT_CONFIG_MODE_REQ;
    prim->gattId        = gattId;
    prim->flags         = flags;
    return prim;
}

CsrBtGattConnectBredrReq *CsrBtGattConnectBredrReq_struct(CsrBtGattId        gattId,
                                                          CsrBtTypedAddr     address)
{
    CsrBtGattConnectBredrReq *prim = (CsrBtGattConnectBredrReq *)
                                  CsrPmemAlloc(sizeof(CsrBtGattConnectBredrReq));

    prim->type          = CSR_BT_GATT_CONNECT_BREDR_REQ;
    prim->gattId        = gattId;
    prim->address       = address;
    return prim;
}

CsrBtGattAcceptBredrReq *CsrBtGattAcceptBredrReq_struct(CsrBtGattId        gattId)
{
    CsrBtGattAcceptBredrReq *prim = (CsrBtGattAcceptBredrReq *)
                                  CsrPmemAlloc(sizeof(CsrBtGattAcceptBredrReq));

    prim->type          = CSR_BT_GATT_ACCEPT_BREDR_REQ;
    prim->gattId        = gattId;
    return prim;
}

CsrBtGattCancelAcceptBredrReq *CsrBtGattCancelAcceptBredrReq_struct(CsrBtGattId        gattId)
{
    CsrBtGattCancelAcceptBredrReq *prim = (CsrBtGattCancelAcceptBredrReq *)
                                  CsrPmemAlloc(sizeof(CsrBtGattCancelAcceptBredrReq));

    prim->type          = CSR_BT_GATT_CANCEL_ACCEPT_BREDR_REQ;
    prim->gattId        = gattId;
    return prim;
}

CsrBtGattConnectBredrRes *CsrBtGattConnectBredrRes_struct(CsrBtGattId                  gattId,
                                                          CsrBtTypedAddr               address,
                                                          CsrBtGattConnectBredrResCode responseCode)
{
    CsrBtGattConnectBredrRes *prim = (CsrBtGattConnectBredrRes *)
                                  CsrPmemAlloc(sizeof(CsrBtGattConnectBredrRes));

    prim->type          = CSR_BT_GATT_CONNECT_BREDR_RES;
    prim->gattId        = gattId;
    prim->address       = address;
    prim->responseCode  = responseCode;
    return prim;
}

CsrBtGattDisconnectBredrReq *CsrBtGattDisconnectBredrReq_struct(CsrBtGattId gattId,
                                                                CsrBtTypedAddr  address)
{
    CsrBtGattDisconnectBredrReq *prim = (CsrBtGattDisconnectBredrReq *)
                                  CsrPmemAlloc(sizeof(CsrBtGattDisconnectBredrReq));

    prim->type          = CSR_BT_GATT_DISCONNECT_BREDR_REQ;
    prim->gattId        = gattId;
    prim->address       = address;
    return prim;
}
