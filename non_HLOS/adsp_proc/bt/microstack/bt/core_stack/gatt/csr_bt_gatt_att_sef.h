#ifndef CSR_BT_GATT_ATT_SEF_H__
#define CSR_BT_GATT_ATT_SEF_H__
/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_main.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(GATT_OFFLOAD) || !defined(EXCLUDE_CSR_BT_CM_MODULE)
void CsrBtGattAttRegisterCfmHandler(GattMainInst *inst);
#else
#define CsrBtGattAttRegisterCfmHandler NULL
#endif
#ifdef CSR_BT_GATT_INSTALL_EATT
void CsrBtGattEattRegisterCfmHandler(GattMainInst *inst);
void CsrBtGattEattConnectCfmHandler(GattMainInst *inst);
void CsrBtGattEattConnectIndHandler(GattMainInst *inst);
void CsrBtGattAttReadMultiVarCfmHandler(GattMainInst *inst);
void CsrBtGattAttReadMultiVarIndHandler(GattMainInst *inst);
void CsrBtGattAttHandleMultiValueCfmHandler(GattMainInst *inst);
void CsrBtGattAttHandleValueMultiIndHandler(GattMainInst *inst);
#else
#define CsrBtGattEattRegisterCfmHandler NULL
#define CsrBtGattEattConnectCfmHandler NULL
#define CsrBtGattEattConnectIndHandler NULL
#define CsrBtGattAttReadMultiVarCfmHandler NULL
#define CsrBtGattAttReadMultiVarIndHandler NULL
#define CsrBtGattAttHandleMultiValueCfmHandler NULL
#define CsrBtGattAttHandleValueMultiIndHandler NULL
#endif /* CSR_BT_GATT_INSTALL_EATT */

CsrBtGattQueueElement* CsrBtGattfindQueueElement(GattMainInst *inst, CsrUint16 cid);
CsrBtGattQueueElement* CsrBtGattfindQueueElementbtConnId(GattMainInst *inst, CsrBtConnId btConnId);
CsrBool CsrBtGattCheckCid(CsrBtGattConnElement *conn, CsrUint16 cid);
void CsrBtGattEattInvalidateCid(CsrBtGattConnElement *conn, CsrUint16 cid);

#if defined(CSR_BT_GATT_CACHING) || defined(CSR_BT_GATT_INSTALL_EATT)
void CsrBtGattCsfHandler(CsrBtGattConnElement* conn, ATT_ACCESS_IND_T* prim);
void CsrBtGattTddbStoreInfoOnDisconnection(CsrBtGattConnElement *conn);
#else
#define CsrBtGattTddbStoreInfoOnDisconnection(conn) ((void)0)
#endif

void csrBtGattNotificationHandler(CsrUint16 start, void *data);
#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
void csrBtGattWriteCmdHandler(CsrUint16 apphandle, void* data);
#endif

CsrBool isMtuExchanged(void);

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
void CsrBtGattAttAddCfmHandler(GattMainInst *inst);
void CsrBtGattAttRemoveCfmHandler(GattMainInst *inst);
#define CsrBtGattAttAddDbCfmHandler NULL
#else
#define CsrBtGattAttAddCfmHandler NULL
#define CsrBtGattAttRemoveCfmHandler NULL
void CsrBtGattAttAddDbCfmHandler(GattMainInst *inst);
#endif

void CsrBtGattAttConnectCfmHandler(GattMainInst *inst);
void CsrBtGattAttConnectIndHandler(GattMainInst *inst);

void CsrBtGattAttDisconnectIndHandler(GattMainInst *inst);


void CsrBtGattAttFindInfoCfmHandler(GattMainInst *inst);
void CsrBtGattAttFindByTypeValueCfmHandler(GattMainInst *inst);
void CsrBtGattAttReadByTypeCfmHandler(GattMainInst *inst);
void CsrBtGattAttReadCfmHandler(GattMainInst *inst);
void CsrBtGattAttReadBlobCfmHandler(GattMainInst *inst);
void CsrBtGattAttReadMultiCfmHandler(GattMainInst *inst);
void CsrBtGattAttReadByGroupTypeCfmHandler(GattMainInst *inst);
void CsrBtGattAttWriteCfmHandler(GattMainInst *inst);
void CsrBtGattAttPrepareWriteCfmHandler(GattMainInst *inst);
void CsrBtGattAttExecuteWriteCfmHandler(GattMainInst *inst);
void CsrBtGattAttHandleValueCfmHandler(GattMainInst *inst);
#if defined(CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION)
void CsrBtGattAttHandleValueIndHandler(GattMainInst *inst);
#else
#define CsrBtGattAttHandleValueIndHandler NULL
#endif
void CsrBtGattAttAccessIndHandler(GattMainInst *inst);

void CsrBtGattClientHandleReadBlobCfm(GattMainInst * inst, 
                                        CsrBtGattConnElement *conn, 
                                        CsrBtGattQueueElement *qElem,
                                        ATT_READ_BLOB_CFM_T *prim);

#ifdef CSR_BT_GATT_CACHING
void CsrBtGattAttAddRobustCachingCfmHandler(GattMainInst *inst);
void CsrBtGattAttChangeAwareIndHandler(GattMainInst *inst);
#else
#define CsrBtGattAttAddRobustCachingCfmHandler NULL
#define CsrBtGattAttChangeAwareIndHandler NULL
#endif

#ifdef GATT_DATA_LOGGER
void CsrBtGattAttDebugIndHandler(GattMainInst *inst);
#else
#define CsrBtGattAttDebugIndHandler NULL
#endif

#ifdef GATT_OFFLOAD
void GattOffloadAttErrorIndHandler(GattMainInst *inst);
#else
#define GattOffloadAttErrorIndHandler NULL
#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_ATT_SEF_H__ */

