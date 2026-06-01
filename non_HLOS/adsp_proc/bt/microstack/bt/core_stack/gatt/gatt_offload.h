#ifndef GATT_OFFLOAD_H__
#define GATT_OFFLOAD_H__
/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_main.h"
#include "hci_arbiter_int.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GATT_OFFLOAD_ERROR_INVALID                ((GattOffloadErrCode)0xFFFF)

typedef struct GattOffloadServiceListTag
{
    struct GattOffloadServiceListTag      *next;
    struct GattOffloadServiceListTag      *prev;
    CsrBtGattId                            gattId;
    GattOffloadId                          offloadId;
    CsrUint32                              filterId;
    GattOffloadService                     *service;
} GattOffloadSvcElement;

#define GATT_OFFLOAD_SVC_FIND_OFFLOADID(_offloadList,_offloadId)            \
    ((GattOffloadSvcElement *)CsrCmnListSearch(&(_offloadList),             \
                                             GattOffloadSvcFindByOffloadId, \
                                             (void *)(_offloadId)))

#define GATT_OFFLOAD_SVC_ADD_LAST(_offloadList)                             \
    (GattOffloadSvcElement *)CsrCmnListElementAddLast(&(_offloadList),      \
                                     sizeof(GattOffloadSvcElement))         \


#define GATT_OFFLOAD_CLIENT_SERVICE_LIST_FIND_BY_OFFLOADID(_cliList,_offloadId) \
    ((CsrBtGattClientService *)CsrCmnListSearch(&(_cliList),                 \
                                           GattOffloadCliSvcFindByOffloadId, \
                                           (void *)(_offloadId)))


#define GATT_OFFLOAD_APP_INST_FIND_OFFLOADID(_appList,_offloadId)           \
    ((CsrBtGattAppElement *)CsrCmnListSearch(&(_appList),                   \
                                         GattOffloadAppInstFindByOffloadId, \
                                         (void *)(_offloadId)))


#define GATT_OFFLOAD_CONN_INST_FIND_FROM_ACLHANDLE(_connList, _aclHandle)    \
        ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList),             \
                                      GattOffloadConnInstFindByAclHandle,   \
                                      (void *)(_aclHandle)))

void GattOffloadInitSvcList(CsrCmnListElm_t *elem);
void GattOffloadFreeSvcList(CsrCmnListElm_t *elem);

void GattOffloadSendErrorEventToApps(GattMainInst *inst, 
                                     CsrBtGattConnElement *conn);

CsrBtGattAppElement * GattOffloadFindAppElement(GattMainInst *inst,
                                                l2ca_cid_t cid,
                                                CsrBtGattHandle handle);

CsrBool GattOffloadRestoreHandler(GattMainInst *inst, 
                                  CsrBtGattQueueElement *element, 
                                  CsrUint16 mtu);


#define GATT_LOG_INFO(...) CSR_LOG_TEXT_INFO((GATT , 0, __VA_ARGS__))
#define GATT_LOG_DEBUG(...) CSR_LOG_TEXT_INFO((GATT, 0, __VA_ARGS__))

#ifdef __cplusplus
}
#endif

#endif /* GATT_OFFLOAD_H__ */

