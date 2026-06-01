#ifndef CSR_BT_GATT_UPSTREAM_H__
#define CSR_BT_GATT_UPSTREAM_H__
/******************************************************************************
 Copyright (c) 2010-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_bt_gatt_main.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrBtGattMessagePut(CsrSchedQid phandle, void *msg);

void CsrBtGattStdCfmSend(CsrBtGattPrim   type,
                         CsrBtGattId     gattId,
                         CsrBtResultCode resultCode,
                         CsrBtSupplier   resultSupplier);

void CsrBtGattStdBtConnIdCfmSend(CsrBtGattPrim   type,
                                 CsrBtGattId     gattId,
                                 CsrBtResultCode resultCode,
                                 CsrBtSupplier   resultSupplier,
                                  CsrBtConnId     btConnId);

void CsrBtGattEventSendCfmSend(CsrBtGattEventSendReq *prim,
                               CsrBtResultCode resultCode,
                               CsrBtSupplier   resultSupplier);

void CsrBtGattDisconnectIndSend(CsrBtGattId gattId,
                                CsrBtResultCode resultCode,
                                CsrBtSupplier resultSupplier,
                                CsrBtConnId btConnId,
                                CsrBtTypedAddr *address,
                                CsrBtGattConnInfo connInfo);

void CsrBtGattConnectIndSend(CsrBtGattId gattId,
                             CsrBtResultCode resultCode,
                             CsrBtSupplier resultSupplier,
                             CsrBtConnId btConnId,
                             CsrBtGattConnInfo connFlags,
                             CsrBtTypedAddr *address,
                             CsrUint16        mtu,
                             CsrBtGattLeRole leRole,
                             l2ca_conflags_t flags);

void CsrBtGattRegisterCfmSend(CsrSchedQid       qid,
                              CsrBtGattId       gattId,
                              CsrBtResultCode   resultCode, 
                              CsrBtSupplier     resultSupplier,
                              CsrUint16         context);

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
void CsrBtGattDbAllocCfmSend(CsrBtGattId       gattId,
                             CsrBtResultCode   resultCode, 
                             CsrBtSupplier     resultSupplier,
                             CsrBtGattHandle   start,
                             CsrBtGattHandle   end,
                             CsrUint16         preferredHandle);

void CsrBtGattDbDeallocCfmSend(CsrBtGattId       gattId,
                               CsrBtResultCode   resultCode, 
                               CsrBtSupplier     resultSupplier,
                               CsrBtGattHandle   start,
                               CsrBtGattHandle   end);

void CsrBtGattDbRemoveCfmSend(CsrBtGattId     gattId,
                              CsrBtResultCode resultCode,
                              CsrBtSupplier   resultSupplier,
                              CsrUint16       numOfAttr);
#endif

void CsrBtGattMtuChangedIndSend(CsrBtGattId     gattId,
                                CsrBtConnId     btConnId,
                                CsrUint16       mtu);

void CsrBtGattReportIndSend(CsrBtGattId gattId,
                            CsrBtGattReportEvent eventType,
                            CsrBtTypedAddr *address,
                            CsrBtTypedAddr *permanentAddress,
                            CsrUint8 lengthData,
                            CsrUint8 *data,
                            CsrInt8 rssi);

void CsrBtGattDiscoverServicesIndSend(CsrBtGattQueueElement *qElem,
                                      CsrBtGattHandle       startHandle,
                                      CsrBtGattHandle       endHandle,
                                      CsrUint16             length,
                                      CsrUint8              *data);

void CsrBtGattDiscoverCharacIndSend(CsrBtGattQueueElement   *qElem,
                                    CsrBtConnId             btConnId,
                                    CsrBtGattHandle         declarationHandle,
                                    CsrBtUuid               uuid,
                                    CsrUint8                *data);

void CsrBtGattFindInclServicesIndSend(CsrBtGattQueueElement *qElem,
                                      CsrBtGattHandle       attrHandle,
                                      CsrUint16             length,
                                      CsrUint8              *data);

void CsrBtGattDiscoverCharacDescriptorsIndSend(CsrBtGattQueueElement *qElem,
                                               CsrBtGattHandle       descriptorHandle,
                                               att_uuid_type_t       uuidType,
                                               CsrUint32             *attUuid);

void CsrBtGattReadCfmHandler(CsrBtGattReadReq   *prim,
                             CsrBtResultCode    resultCode,
                             CsrBtSupplier      resultSupplier,
                             CsrUint16          valueLength,
                             CsrUint8           **value);

#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
void CsrBtGattLongReadCfmHandler(CsrBtGattReadReq* prim,
                               CsrBtGattId     gattId,
                               CsrBtResultCode resultCode,
                               CsrBtSupplier   resultSupplier,
                               CsrBtConnId     btConnId,
                               CsrUint16       readUnitCount,
                               CsrBtGattLongAttrRead *readUnit);
#endif /* CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET */

void CsrBtGattReadMultiCfmSend(CsrBtGattId     gattId,
                               CsrBtResultCode resultCode,
                               CsrBtSupplier   resultSupplier,
                               CsrBtConnId     btConnId,
                               CsrUint16       valueLength,
                               CsrUint8        *value,
                               CsrBtGattHandle *handle);

void CsrBtGattUpStreamMsgSerializer(CsrBtGattConnElement* conn,
                                void* msg,
                                CsrBtGattId gattId,
                                CsrBtConnId btConnId,
                                CsrUint16 cid);

#ifdef CSR_BT_GATT_INSTALL_EATT
void CsrBtGattEattConnectIndSend(CsrBtGattId   gattId,
                              CsrBtConnId      btConnId,
                              CsrUint8         eattSuportedServer,
                              CsrBtResultCode  resultCode,
                              CsrBtSupplier    resultSupplier);

void CsrBtGattPriorityCfmSend(CsrBtGattId       gattId,
                              CsrBtConnId       btConnId,
                              CsrBtResultCode   resultCode,
                              CsrBtSupplier     resultSupplier);

void CsrBtGattReadMultiVarCfmSend(CsrBtGattId  gattId,
                               CsrBtResultCode resultCode,
                               CsrBtSupplier   resultSupplier,
                               CsrBtConnId     btConnId,
                               CsrUint16       errorHandle,
                               CsrUint16       valueLength,
                               CsrUint8        *value,
                               CsrUint16       handlesCount,
                               CsrBtGattHandle *handles);

void CsrBtGattAccessMultiReadIndSend(GattMainInst         *inst,
                                     CsrUint16            cid,
                                     CsrBtGattId          gattId,
                                     CsrBtConnId          btConnId,
                                     CsrUint16            mtu,
                                     CsrBtGattAccessCheck check,
                                     CsrBtGattConnInfo    connInfo,
                                     CsrBtTypedAddr       address,
                                     CsrUint16            attrHandlesCount,
                                     CsrBtGattHandle      *handles);


#endif /* CSR_BT_GATT_INSTALL_EATT */

void CsrBtGattNotificationIndSend(CsrBtGattId gattId,
                                  CsrBtConnId btConnId,
                                  CsrBtTypedAddr address,
                                  CsrBtGattHandle valueHandle,
                                  CsrUint16 valueLength,
                                  CsrUint8 *value,
                                  CsrBtGattConnInfo connInfo);

void CsrBtGattReadByUuidIndSend(CsrBtGattId     gattId,
                                CsrBtConnId     btConnId,
                                CsrBtGattHandle valueHandle,
                                CsrUint16       valueLength,
                                CsrUint8        *value);

void CsrBtGattReadByUuidCfmSend(CsrBtGattId     gattId,
                                CsrBtResultCode resultCode,
                                CsrBtSupplier   resultSupplier,
                                CsrBtConnId     btConnId,
                                CsrBtUuid       *uuid);

void CsrBtGattWriteCfmSend(CsrBtGattPrim    *prim,
                           CsrBtResultCode  resultCode,
                           CsrBtSupplier    resultSupplier);

#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
void CsrBtGattCongestionIndSend(CsrBtGattId gattId,
                           CsrBtConnId btConnId,
                           CsrBtGattHandle   handle,
                           CsrBool congested);
#endif /* CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT */

void CsrBtGattPhysicalLinkStatusIndSend(CsrBtGattId       gattId, 
                                        CsrBtTypedAddr    address,
                                        CsrBtGattConnInfo connInfo,
                                        CsrBool           status);

void CsrBtGattAccessWriteIndSend(GattMainInst *inst,
                                              CsrUint16 cid,
                                              CsrBtGattId gattId,
                                              CsrBtConnId btConnId,
                                              CsrBtGattAccessCheck check,
                                              CsrBtGattConnInfo connInfo,
                                              CsrBtTypedAddr address,
                                              CsrUint16 writeUnitCount,
                                              CsrBtGattAttrWritePairs *writeUnit,
                                              CsrBtGattHandle handle);


void CsrBtGattAccessReadIndSend(GattMainInst *inst,
                                             CsrUint16 cid,
                                             CsrBtGattId gattId,
                                             CsrBtConnId btConnId,
                                             CsrBtGattHandle handle,
                                             CsrUint16 offset,
                                             CsrUint16 mtu,
                                             CsrBtGattAccessCheck check,
                                             CsrBtGattConnInfo connInfo,
                                             CsrBtTypedAddr address);


void CsrBtGattDbAccessCompleteIndSend(CsrBtGattId gattId,
                                      CsrBtConnId btConnId,
                                      CsrBtGattConnInfo connInfo,
                                      CsrBtTypedAddr address,
                                      CsrBtGattHandle attrHandle,
                                      CsrBool commit);

void CsrBtGattReadRemoteLeNameCfmSend(CsrSchedQid appHandle,
                                      CsrUint16   remoteNameLength,
                                      CsrUint8    *remoteName);

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
void CsrBtGattReadRemoteRpaOnlyCharCfmSend(CsrSchedQid     appHandle,
                                           CsrUint8        rpaOnlyValue,
                                           CsrBtTypedAddr  address,
                                           CsrBtResultCode resultCode,
                                           CsrBtSupplier   resultSupplier);
#endif

void CsrBtGattUnregisterCfmHandler(GattMainInst          *inst, 
                                   CsrBtGattQueueElement *qelm, 
                                   CsrBool               qRestore);

void CsrBtGattClientExchangeMtuCfmSend(CsrBtGattId      gattId, 
                                        CsrBtConnId     btConnId, 
                                        CsrUint16       mtu,
                                        CsrBtResultCode resultCode,
                                        CsrBtSupplier   resultSupplier);
void CsrBtGattRemoteClientExchangeMtuIndSend(CsrBtGattId gattId, 
                                             CsrBtConnId btConnId, 
                                             CsrUint16   mtu);

#ifdef CSR_BT_GATT_INSTALL_FLAT_DB
void CsrBtGattFlatDbRegisterCfmSend(CsrSchedQid       qid,
                                    CsrBtResultCode   resultCode,
                                    CsrBtSupplier     resultSupplier);
#endif /* CSR_BT_GATT_INSTALL_FLAT_DB */

#ifdef CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
void CsrBtGattNotificationIndicationIndSend(GattMainInst *inst,
                                                CsrUint16 cid,
                                                CsrUint16 flags,
                                                CsrUint16 size_value,
                                                CsrUint8 *value,
                                                CsrUint16 handle,
                                                CsrBtGattId gattId,
                                                CsrBtConnId btConnId);
#endif /* CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION */

#ifdef CSR_BT_INSTALL_GATT_BREDR
void CsrBtGattConnectBredrIndSend(CsrBtGattId gattId,
                                  CsrBtTypedAddr *address,
                                  CsrUint16 mtu);
#endif

#ifdef GATT_CACHING_CLIENT_ROLE
void GattRemoteDatabaseChangedInd(CsrBtGattId             gattId,
                                       CsrBtConnId             btConnId,
                                       GattRemoteDbChangedFlag indType,
                                       CsrBtGattHandle         startHandle,
                                       CsrBtGattHandle         endHandle);

#endif /* GATT_CACHING_CLIENT_ROLE */

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_UPSTREAM_H__ */

