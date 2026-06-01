#ifndef CSR_BT_GATT_SEF_H__
#define CSR_BT_GATT_SEF_H__
/******************************************************************************
 Copyright (c) 2010-2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Advertisment priority */
#define CSR_BT_GATT_ADV_PRIO_LOW                  ((CsrUint8) 0x00)
#define CSR_BT_GATT_ADV_PRIO_MEDIUM               ((CsrUint8) 0x01)
#define CSR_BT_GATT_ADV_PRIO_HIGH                 ((CsrUint8) 0x02)

/* Covers Registration and Un-register an application instance to Gatt */
CsrBool CsrBtGattRegisterReqHandler(GattMainInst *inst);
CsrBool CsrBtGattUnregisterReqHandler(GattMainInst *inst);
#ifdef CSR_BT_GATT_INSTALL_EATT
CsrBool CsrBtGattPriorityReqHandler(GattMainInst *inst);
CsrBool CsrBtGattReadMultiVarReqHandler(GattMainInst *inst);
CsrBool CsrBtGattReadMultiVarRspHandler(GattMainInst *inst);
#ifdef GATT_ENABLE_EATT_DISCONNECT
CsrBool CsrBtGattEattDisconnectReqHandler(GattMainInst *inst);
#else
#define CsrBtGattEattDisconnectReqHandler NULL
#endif /* GATT_ENABLE_EATT_DISCONNECT */
#else
#define CsrBtGattPriorityReqHandler NULL
#define CsrBtGattReadMultiVarReqHandler NULL
#define CsrBtGattReadMultiVarRspHandler NULL
#define CsrBtGattEattDisconnectReqHandler NULL
#endif /* CSR_BT_GATT_INSTALL_EATT */

#if defined(CSR_BT_GATT_INSTALL_SERVER_LONG_WRITE_OFFSET) || defined(CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET)
CsrBool CsrBtGattConfigModeReqHandler(GattMainInst *inst);
#else
#define CsrBtGattConfigModeReqHandler NULL
#endif

#ifdef GATT_OFFLOAD
CsrBool GattOffloadEnableServiceReqHandler(GattMainInst *inst);
CsrBool GattOffloadDisableServiceReqHandler(GattMainInst *inst);
CsrBool GattOffloadConnInfoReqHandler(GattMainInst *inst);
#else
#define GattOffloadEnableServiceReqHandler NULL
#define GattOffloadDisableServiceReqHandler NULL
#define GattOffloadConnInfoReqHandler NULL
#endif

/* Covers DataBase Configuration */
#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
CsrBool CsrBtGattDbAllocReqHandler(GattMainInst *inst);
CsrBool CsrBtGattDbDeallocReqHandler(GattMainInst *inst);
CsrBool CsrBtGattDbAddReqHandler(GattMainInst *inst);
CsrBool CsrBtGattDbRemoveReqHandler(GattMainInst *inst);
#define CsrBtGattFlatDbRegisterReqHandler NULL
#define CsrBtGattFlatDbRegisterHandleRangeReqHandler NULL
#else
#define CsrBtGattDbAllocReqHandler NULL
#define CsrBtGattDbDeallocReqHandler NULL
#define CsrBtGattDbAddReqHandler NULL
#define CsrBtGattDbRemoveReqHandler NULL
CsrBool CsrBtGattFlatDbRegisterReqHandler(GattMainInst *inst);
CsrBool CsrBtGattFlatDbRegisterHandleRangeReqHandler(GattMainInst *inst);
#endif

CsrBool CsrBtGattDbAccessResHandler(GattMainInst *inst);

/* Covers Server Initiated Notification and Indication  */
CsrBool CsrBtGattEventSendReqHandler(GattMainInst *inst);
void CsrBtGattServiceChangedIndicationSend(GattMainInst         *inst, 
                                           CsrBtGattConnElement *conn,
                                           CsrBtGattHandle      startHandle,
                                           CsrBtGattHandle      endHandle); 

#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
void GattEncryptedDataKeyMaterailIndicationSend(GattMainInst* inst,
                                                CsrBtGattConnElement* conn);
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */

/* Covers item 1, Server Configuration, in the GATT feature table */
void CsrBtGattExchangeMtuHandler(GattMainInst   *inst, 
                                 CsrUint16      mtu, 
                                 CsrBtConnId    btConnId, 
                                 CsrBtTypedAddr address);

/* Covers item 2, Primary Service Discovery, in the GATT feature table  */
CsrBool CsrBtGattDiscoverServicesReqHandler(GattMainInst *inst);

/* Covers item 3, Relationship Discovery, in the GATT feature table */
CsrBool CsrBtGattFindInclServicesReqHandler(GattMainInst *inst);

/* Covers item 4, Characteristic Discovery, in the GATT feature table */
CsrBool CsrBtGattDiscoverCharacReqHandler(GattMainInst *inst);

/* Covers item 5, Characteristic Descriptor Discovery, in the GATT feature table */
CsrBool CsrBtGattDiscoverCharacDescriptorsReqHandler(GattMainInst *inst);

/* Covers item 6, Characteristic Value Read, and  item 10, 
   Read Characteristic Descriptor Value,in the GATT feature table */
CsrBool CsrBtGattReadReqHandler(GattMainInst *inst);
CsrBool CsrBtGattReadByUuidReqHandler(GattMainInst *inst);
CsrBool CsrBtGattReadMultiReqHandler(GattMainInst *inst);
CsrBool CsrBtGattReadByUuidSecurityHandler(void            *gattInst,
                                           void            *qElem,
                                           CsrBtResultCode result, 
                                           CsrBtSupplier   supplier);

/* Covers item 7, Characteristic Value Write, in the GATT feature table */
void CsrBtGattGetAttPrepareWriteSend(CsrBtGattConnElement  *conn,
                                     CsrBtGattQueueElement *qElem,
                                     CsrUint16             prepareWriteOffset,
                                     CsrUint16             mtu,
                                     CsrUint16             valueLength,
                                     CsrUint8              *value);

CsrBool CsrBtGattWriteReqHandler(GattMainInst *inst);

/* Event Mask Handler*/
CsrBool CsrBtGattSetEventMaskReqHandler(GattMainInst *inst);

/* Cancel */
CsrBool CsrBtGattCancelReqHandler(GattMainInst *inst);

/* To start the Private Read By UUID. E.g CSR_BT_GATT_UUID_DEVICE_NAME_CHARAC or CSR_BT_GATT_UUID_SERVICE_CHANGED_CHARAC  */
void CsrBtGattReadByUuidPrivateHandler(GattMainInst *inst, CsrBtUuid16 uuidToRead, CsrBtConnId btConnId);

/* Write value to the Client Configuration Despriptor which belongs to the Service Changed Characteristic */
void CsrBtGattWriteServiceChangedHandler(CsrBtGattQueueElement *qElem, CsrBtGattCliConfigBits configValue);

/* Private stuff */
void CsrBtGattReadRemoteLeNameReqHandler(GattMainInst *inst);

#ifdef CSR_BT_INSTALL_GATT_BREDR
CsrBool CsrBtGattConnectBredrReqHandler(GattMainInst *inst);

CsrBool CsrBtGattAcceptBredrReqHandler(GattMainInst *inst);

CsrBool CsrBtGattCancelAcceptBredrReqHandler(GattMainInst *inst);

CsrBool CsrBtGattConnectBredrResHandler(GattMainInst *inst);

CsrBool CsrBtGattDisconnectBredrReqHandler(GattMainInst *inst);
#else
#define CsrBtGattConnectBredrReqHandler NULL

#define CsrBtGattAcceptBredrReqHandler NULL

#define CsrBtGattCancelAcceptBredrReqHandler NULL

#define CsrBtGattConnectBredrResHandler NULL

#define CsrBtGattDisconnectBredrReqHandler NULL
#endif

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
void CsrBtGattReadRemoteRpaOnlyCharReqHandler(GattMainInst *inst);
#endif

/* Prototypes - free_downsteam */
void CsrBtGattFreeDownstreamMessageContents(CsrUint16 eventClass, void *message);

#ifdef CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
CsrBool CsrBtGattClientRegisterServiceReqHandler(GattMainInst *inst);
#ifndef GATT_OFFLOAD
CsrBool CsrBtGattClientIndicationRspHandler(GattMainInst *inst);
#else
#define CsrBtGattClientIndicationRspHandler NULL
#endif
#else
#define CsrBtGattClientRegisterServiceReqHandler NULL
#define CsrBtGattClientIndicationRspHandler NULL
#endif /* CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION */


#if defined(CSR_BT_GATT_CACHING) && !defined(CSR_BT_GATT_INSTALL_FLAT_DB)
CsrBool CsrBtGattDbCommitReqHandler(GattMainInst *inst);
#else
#define CsrBtGattDbCommitReqHandler NULL
#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_SEF_H__ */

