#ifndef CSR_BT_GATT_PRIVATE_UTILS_H__
#define CSR_BT_GATT_PRIVATE_UTILS_H__
/******************************************************************************
 Copyright (c) 2011-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #6 $
******************************************************************************/
#include "csr_synergy.h"
#include "csr_bt_gatt_main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Misc */
CsrBool CsrBtGattNewReqHandler(GattMainInst          *inst,
                               void                  *msg,
                               CsrBtConnId           btConnId,
                               CsrBtGattId           gattId,
                               CsrBtGattRestoreType  restoreFunc,
                               CsrBtGattCancelType   cancelFunc,
                               CsrBtGattSecurityType securityFunc);

CsrBtGattAppElement * CsrBtGattAccessIndGetParams(GattMainInst      *inst, 
                                                  l2ca_cid_t        cid,
                                                  CsrUint16         handle,
                                                  CsrBtConnId       *btConnId,
                                                  CsrBtGattConnInfo *connInfo,
                                                  CsrUint16         *mtu,
                                                  CsrBtTypedAddr    *address);

void CsrBtGattInitHandler(GattMainInst *inst);

void CsrBtGattGetAttUuid(CsrBtUuid uuid,
                         CsrUint32 *attUuid,
                         att_uuid_type_t *uuidType);

CsrUint16 CsrBtGattValidateBtConnIdByMtu(GattMainInst *inst,
                                         CsrBtGattId  gattId,
                                         CsrBtConnId  btConnId,
                                         l2ca_cid_t   *cid,
                                         CsrBool      cidAllocation);

typedef struct
{
    GattMainInst         *inst;
    CsrBtGattConnElement *conn;
    CsrBtTypedAddr       address;
    CsrBtGattHandle      startHdl;
    CsrBtGattHandle      endHdl;
} CsrBtGattServiceChangedElem;

CsrInt32  CsrBtGattAppInstSortByAttributeValue(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2);
CsrBool   CsrBtGattAppInstFindGattId(CsrCmnListElm_t *elem, void *value);
CsrBool   CsrBtGattAppInstFindAttrHandle(CsrCmnListElm_t *elem, void *value);

void CsrBtGattAttServiceChangedMsgHandler(GattMainInst           *inst, 
                                          ATT_HANDLE_VALUE_IND_T *prim, 
                                          CsrBtGattConnElement   *conn);

#define CSR_BT_GATT_APP_INST_ADD_FIRST(_appList) \
    (CsrBtGattAppElement *)CsrCmnListElementAddFirst(&(_appList), \
                                                     sizeof(CsrBtGattAppElement))

#define CSR_BT_GATT_APP_INST_FIND_GATT_ID(_appList,_gattId) \
    ((CsrBtGattAppElement *)CsrCmnListSearch(&(_appList), \
                                             CsrBtGattAppInstFindGattId, \
                                             (void *)(_gattId)))

#define CSR_BT_GATT_APP_INST_GET_FIRST(_appList) \
    ((CsrBtGattAppElement *)CsrCmnListGetFirst(&(_appList)))

#define CSR_BT_GATT_APP_INST_FIND_ATTR_HANDLE(_appList,_attrHandle) \
    ((CsrBtGattAppElement *)CsrCmnListSearch(&(_appList), \
                                             CsrBtGattAppInstFindAttrHandle, \
                                             (void *)(_attrHandle)))

#define CSR_BT_GATT_APP_INST_REMOVE(_appList,_appElem) \
                                    (CsrCmnListElementRemove((CsrCmnList_t *)&(_appList), \
                                                             (CsrCmnListElm_t *)(_appElem)))

#define CSR_BT_GATT_APP_INST_ITERATE(_appList,_func,_dataPtr) \
                                    (CsrCmnListIterate(&(_appList), (_func), (void *)(_dataPtr)))

#define CSR_BT_GATT_APP_INST_SORT_BY_ATTR_VALUE(_appList) \
                                               (CsrCmnListSort(&(_appList), \
                                                CsrBtGattAppInstSortByAttributeValue))

/* Prepare write handlers */
typedef struct
{
    CsrBtGattAttrWritePairs *unit;
    CsrUint16               unitCount;
    CsrUint16               count;
    l2ca_cid_t              cid;
    CsrBtGattHandle         attrHandle;
    CsrBtGattAccessCheck    check;
} CsrBtGattPrepareAttrElem;

typedef struct
{
    GattMainInst            *inst;
    l2ca_cid_t              cid;
    CsrBool                 commit;
} CsrBtGattPrepareCleanElem;

typedef struct
{
    l2ca_cid_t              cid;
    CsrBtGattHandle         attrHandle;
} CsrBtGattPrepareMultiElem;

#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
/* Long read handlers */
typedef struct
{
    CsrBtGattLongAttrRead  *readUnit;
    CsrUint16              unitCount;
    CsrUint16              count;
} CsrBtGattLongReadAttrElem;
#endif

CsrBool  CsrBtGattPrepareInstMultipleAttrHandles(CsrCmnListElm_t *elem, void *value);
CsrBool  CsrBtGattPrepareInstFindCidIdleState(CsrCmnListElm_t *elem, void *value);
CsrBool  CsrBtGattPrepareInstFindCidPendingState(CsrCmnListElm_t *elem, void *value);
CsrBool  CsrBtGattPrepareInstCleanUp(CsrCmnListElm_t *elem, void *value);
CsrBool  CsrBtGattPrepareInstGetAttrList(CsrCmnListElm_t *elem, void *value);
CsrInt32 CsrBtGattPrepareInstSortByOffset(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2);

#ifdef CSR_BT_GATT_INSTALL_SERVER_LONG_WRITE_OFFSET
void CsrBtGattPrepareInstLongWriteList(CsrCmnListElm_t *elem, void *value);
#endif

#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
CsrBool CsrBtGattPrepareInstLongReadList(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_LONG_READ_INST_ADD_LAST(_prepareList) \
        (CsrBtGattLongAttrReadBuffer *) CsrCmnListElementAddLast(&(_prepareList), \
                                                            sizeof(CsrBtGattLongAttrReadBuffer))
#endif

#define CSR_BT_GATT_PREPARE_INST_ADD_LAST(_prepareList) \
    (CsrBtGattPrepareBuffer *) CsrCmnListElementAddLast(&(_prepareList), \
                                                        sizeof(CsrBtGattPrepareBuffer))

#define CSR_BT_GATT_PREPARE_INST_FIND_CID(_prepareList,_cid,_func) \
    ((CsrBtGattPrepareBuffer *) CsrCmnListSearch(&(_prepareList), (_func), (void *)(&_cid)))

#define CSR_BT_GATT_PREPARE_INST_MULTIPLE_ATTR_HANDLES(_prepareList,_elem,_cid,_handle) \
{ \
    CsrBtGattPrepareMultiElem _ids; \
    _ids.cid        = (_cid); \
    _ids.attrHandle = (_handle); \
    (*_elem)        = ((CsrBtGattPrepareBuffer *) CsrCmnListSearch(&(_prepareList), \
                                                      CsrBtGattPrepareInstMultipleAttrHandles, \
                                                      (void *)(&_ids))); \
};

#define CSR_BT_GATT_PREPARE_INST_SORT_BY_OFFSET(_prepareList) \
                                               (CsrCmnListSort(&(_prepareList), \
                                                CsrBtGattPrepareInstSortByOffset))

#define CSR_BT_GATT_PREPARE_INST_CLEAN_UP(_prepareList,_inst,_cid,_commit) \
{ \
    CsrBtGattPrepareCleanElem _ids; \
    _ids.inst   = (_inst); \
    _ids.cid    = (_cid); \
    _ids.commit = (_commit); \
    (CsrCmnListIterateAllowRemove(&(_prepareList), \
                                  CsrBtGattPrepareInstCleanUp, \
                                  (void *)(&_ids))); \
};

#define CSR_BT_GATT_PREPARE_INST_GET_ATTR_LIST(_prepareList,_cid,_handle,_check,_ids) \
{ \
    CsrUint32 _c     = CsrCmnListGetCount((&_prepareList)); \
    _ids.unit        = (CsrBtGattAttrWritePairs *) CsrPmemZalloc(sizeof(CsrBtGattAttrWritePairs) * _c); \
    _ids.cid         = (_cid); \
    _ids.attrHandle  = (_handle); \
    _ids.check       = (_check); \
    _ids.count       = 0; \
    _ids.unitCount   = 0; \
    (CsrCmnListIterateAllowRemove(&(_prepareList), \
                                  CsrBtGattPrepareInstGetAttrList, \
                                  (void *)(&_ids))); \
};

#ifdef CSR_BT_GATT_INSTALL_SERVER_LONG_WRITE_OFFSET
/* Iterate through the prepare list and store value and valuelength to ids */
#define CSR_BT_GATT_PREPARE_INST_ITERATE_LIST(_prepareList,_cid,_handle,_check,_ids) \
{ \
    CsrUint32 _c     = CsrCmnListGetCount((&_prepareList)); \
    _ids.unit        = (CsrBtGattAttrWritePairs *) CsrPmemZalloc(sizeof(CsrBtGattAttrWritePairs) * _c); \
    _ids.cid         = (_cid); \
    _ids.attrHandle  = (_handle); \
    _ids.check       = (_check); \
    _ids.count       = 0; \
    _ids.unitCount   = 0; \
   (CsrCmnListIterate(&(_prepareList), \
                              CsrBtGattPrepareInstLongWriteList, \
                              (void *)(&_ids))); \
};
#endif

#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
/* Iterate through the prepare list and store value and valuelength to ids */
#define CSR_BT_GATT_LONG_READ_ITERATE_LIST(_prepareList,_ids) \
{ \
    CsrUint32 _c     = CsrCmnListGetCount((&_prepareList)); \
    _ids.readUnit    = (CsrBtGattLongAttrRead *) CsrPmemZalloc(sizeof(CsrBtGattLongAttrRead) * _c); \
    _ids.count       = 0; \
    _ids.unitCount   = 0; \
   (CsrCmnListIterateAllowRemove(&(_prepareList), \
                              CsrBtGattPrepareInstLongReadList, \
                              (void *)(&_ids))); \
};
#endif

#ifdef CSR_BT_GATT_INSTALL_SERVER_HANDLE_RESOLUTION
/* To resolve the Attribute Handles of the Attribute Write Pair units
 * before sending it to application  */
#define CSR_BT_GATT_PREPARE_INST_RESOLVE_ATTR_HANDLE(_elem,_ids) \
{ \
    CsrUint8 _i = 0; \
    _ids.attrHandle = CsrBtGattResolveServerHandle(_elem->start, _ids.attrHandle); \
    for (_i = 0; _i < _ids.unitCount; _i++) \
    { \
        _ids.unit[_i].attrHandle = CsrBtGattResolveServerHandle(_elem->start, _ids.unit[_i].attrHandle); \
    } \
};
#endif

CsrBool CsrBtGattConnInstFindBtconnIdFromIdMask(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_CONN_INST_FIND_BTCONN_ID_FROM_ID_MASK(_connList,_btConnId) \
    ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList), \
                                             CsrBtGattConnInstFindBtconnIdFromIdMask, \
                                             (void *)(_btConnId)))

CsrBool CsrBtGattConnInstFindConnectedBtConnId(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(_connList,_btConnId) \
    ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList), \
                                             CsrBtGattConnInstFindConnectedBtConnId, \
                                             (void *)(_btConnId)))

CsrBool CsrBtGattConnInstFindConnectedCid(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_CONN_INST_FIND_CONNECTED_CID(_connList,_cid) \
    ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList), \
                                             CsrBtGattConnInstFindConnectedCid, \
                                             (void *)(_cid)))

CsrBool CsrBtGattConnInstFindBtCid(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_CONN_INST_FIND_CID(_connList,_cid) \
    ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList), \
                                             CsrBtGattConnInstFindBtCid, \
                                             (void *)(_cid)))

#define CSR_BT_GATT_CONN_INST_ADD_LAST(_connList) \
    (CsrBtGattConnElement *)CsrCmnListElementAddLast(&(_connList), \
                                                     sizeof(CsrBtGattConnElement))

#define CSR_BT_GATT_CONN_INST_REMOVE(_connList,_connElement) \
    (CsrCmnListElementRemove(&(_connList), \
                             (CsrCmnListElm_t *)(_connElement)))

#define CSR_BT_GATT_CONN_INST_GET_FIRST(_connList) \
    ((CsrBtGattConnElement *)CsrCmnListGetFirst(&(_connList)))

CsrBool CsrBtGattFindConnectedConnInstFromAddress(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtGattFindConnInstFromAddress(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtGattFindConnInstFromAddressFlags(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtGattFindConnInstFromAddressFlagsBredr(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtGattFindConnInstFromAddressFlagsLe(CsrCmnListElm_t *elem, void *value);

CsrBool CsrBtGattGetPeerServiceChangeHandles(CsrBtAddressType addressType,
                                             const CsrBtDeviceAddr *addr,
                                             CsrBtGattHandle *serviceChange);
#define CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(_connList,_func,_address) \
        ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList), \
                                                  _func, \
                                                  (void *)(_address)))

#define CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS(_connList,_func,_msg) \
        ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList), \
                                                  _func, \
                                                  (void *)(_msg)))

#define CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_BREDR(_connList,_func,_msg) \
        ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList), \
                                                  _func, \
                                                  (void *)(_msg)))

#define CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS_FLAGS_LE(_connList,_func,_msg) \
        ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList), \
                                                  _func, \
                                                  (void *)(_msg)))

CsrBool CsrBtGattConnInstFindFromState(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_CONN_INST_FIND_FROM_STATE(_connList,_state) \
        ((CsrBtGattConnElement *)CsrCmnListSearch(&(_connList), \
                                                  CsrBtGattConnInstFindFromState, \
                                                  (void *)(_state)))

#define CSR_BT_GATT_NUMBER_OF_PHYSICAL_CONN (6)

#define CSR_BT_GATT_CONN_INST_ITERATE(_connList,_func,_dataPtr) \
                                    (CsrCmnListIterate(&(_connList), (_func), (void *)(_dataPtr)))

typedef struct
{
    l2ca_cid_t  cid;
    CsrUint16   length;
    CsrUint8    *name;
} CsrBtGattConnUpdateRemoteNameIds;

#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
void CsrBtGattConnInstUpdateRemoteName(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_CONN_INST_UPDATE_REMOTE_NAME(_connList,_cid,_length, _name) \
{ \
    CsrBtGattConnUpdateRemoteNameIds _ids; \
    _ids.cid    = (_cid); \
    _ids.length = (_length); \
    _ids.name   = (_name); \
    (CsrCmnListIterate(&(_connList), CsrBtGattConnInstUpdateRemoteName, (void *)(&_ids))); \
};
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

void CsrBtGattConnInstCopyReasonCode(CsrCmnListElm_t *elem, void *value);

void CsrBtGattSendConnectEventToApps(GattMainInst *inst, CsrBtGattConnElement *conn);
void CsrBtGattSendDisconnectEventToApps(GattMainInst *inst, CsrBtGattConnElement *conn);

#ifdef CSR_BT_GATT_INSTALL_EATT
void CsrBtGattSendEattConnectEventToApps(GattMainInst *inst, CsrBtGattConnElement *conn);
#endif /* CSR_BT_GATT_INSTALL_EATT */

#define CSR_BT_GATT_CONN_INST_COUNT(_connList) (CsrCmnListGetCount(&(_connList)))

#define CSR_BT_GATT_CONN_INST_GET_FROM_INDEX(_connList,_index) (CsrBtGattConnElement *) CsrCmnListGetFromIndex(&_connList, _index)

/* Gatt queue func */
CsrBool CsrBtGattQueueFindBtConnId(CsrCmnListElm_t *elem, void *value);

CsrBool CsrBtGattAccessIndQueueFindBtConnId(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtGattQueueFindQueuedMsgBtConnIdFromQueueMask(CsrCmnListElm_t* elem, void* value);
CsrBool CsrBtGattQueueFindProgressMsgBtConnIdFromQueueMask(CsrCmnListElm_t* elem, void* value);

CsrBool CsrBtGattQueueFindBtConnIdFromQueueMask(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtGattQueueFindCid(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtGattQueueFindMsgToCancel(CsrCmnListElm_t *elem, void *value);
void CsrBtGattQueueRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element);
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
CsrBool CsrBtGattQueueFindPrivateReadName(CsrCmnListElm_t *elem, void *value);
#endif

#define CSR_BT_GATT_QUEUE_FIND_BT_CONN_ID(_queueList,_btConnId) \
    ((CsrBtGattQueueElement *)CsrCmnListSearch(&(_queueList), \
                                               CsrBtGattQueueFindBtConnId, \
                                               (void *)(_btConnId)))

#define CSR_BT_GATT_QUEUE_FIND_BT_CONN_ID_FROM_QUEUE_MASK(_queueList,_btConnId) \
    ((CsrBtGattQueueElement *)CsrCmnListSearch(&(_queueList), \
                                               CsrBtGattQueueFindBtConnIdFromQueueMask, \
                                               (void *)(_btConnId)))

#define CSR_BT_GATT_QUEUE_FIND_CID(_queueList,_cid) \
    ((CsrBtGattQueueElement *)CsrCmnListSearch(&(_queueList), \
                                               CsrBtGattQueueFindCid, \
                                               (void *)(_cid)))

#define CSR_BT_GATT_ACCESS_IND_QUEUE_FIND_BT_CONN_ID(_queueList,_btConnId) \
    ((CsrBtGattAccessIndQueueElement *)CsrCmnListSearch(&(_queueList), \
                                               CsrBtGattAccessIndQueueFindBtConnId, \
                                               (void *)(_btConnId)))

#define CSR_BT_GATT_QUEUED_MSG_FIND_BT_CONN_ID_FROM_QUEUE_MASK(_queueList,_btConnId) \
    ((CsrBtGattQueueElement *)CsrCmnListSearch(&(_queueList), \
                                               CsrBtGattQueueFindQueuedMsgBtConnIdFromQueueMask, \
                                               (void *)(_btConnId)))
                                               
#define CSR_BT_GATT_PROGRESS_MSG_FIND_BT_CONN_ID_FROM_QUEUE_MASK(_queueList,_btConnId) \
    ((CsrBtGattQueueElement *)CsrCmnListSearch(&(_queueList), \
                                               CsrBtGattQueueFindProgressMsgBtConnIdFromQueueMask, \
                                               (void *)(_btConnId)))

/* Gatt conn inst func */
typedef struct
{
    CsrBtConnId btConnId;
    CsrBtGattId gattId;
} CsrBtGattConnFindIds;

typedef CsrBtGattConnFindIds CsrBtGattFindMsgToCancelIds;

#define CSR_BT_GATT_QUEUE_FIND_MSG_TO_CANCEL(_queueList,_qElem,_gattId,_btConnId) \
{ \
    CsrBtGattFindMsgToCancelIds _ids; \
    _ids.btConnId = (_btConnId); \
    _ids.gattId = (_gattId); \
    (*_qElem) = ((CsrBtGattQueueElement *)CsrCmnListSearch(&(_queueList), \
                                                         CsrBtGattQueueFindMsgToCancel, \
                                                         (void *)(&_ids))); \
};

#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
typedef CsrBtGattConnFindIds CsrBtGattFindPrivateNameMsgIds;

#define CSR_BT_GATT_QUEUE_FIND_PRIVATE_READ_NAME_MSG(_queueList,_qElem,_gattId,_btConnId) \
{ \
    CsrBtGattFindPrivateNameMsgIds _ids; \
    _ids.btConnId = (_btConnId); \
    _ids.gattId = (_gattId); \
    (*_qElem)   = ((CsrBtGattQueueElement *)CsrCmnListSearch(&(_queueList), \
                                                         CsrBtGattQueueFindPrivateReadName, \
                                                         (void *)(&_ids))); \
};
#endif /* CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME */

#define CSR_BT_GATT_QUEUE_LOG_QUEUE(_gattMsg)           \
    CSR_LOG_TEXT_INFO((CsrBtGattLto,                    \
                       CSR_BT_GATT_LTSO_MSG_QUEUE,      \
                       "Queued 0x%04X",                 \
                       *(CsrBtGattPrim *) (_gattMsg)))

#define CSR_BT_GATT_QUEUE_LOG_RESTORE(_gattMsg)         \
    CSR_LOG_TEXT_INFO((CsrBtGattLto,                    \
                       CSR_BT_GATT_LTSO_MSG_QUEUE,      \
                       "Restored 0x%04X",               \
                       *(CsrBtGattPrim *) (_gattMsg)))

#define CSR_BT_GATT_QUEUE_LOG_REMOVE(_gattMsg)          \
    CSR_LOG_TEXT_INFO((CsrBtGattLto,                    \
                       CSR_BT_GATT_LTSO_MSG_QUEUE,      \
                       "Removed 0x%04X",                \
                       *(CsrBtGattPrim *) (_gattMsg)))

#define CSR_BT_GATT_QUEUE_REMOVE(_queueList,_qElem) \
    (CsrCmnListElementRemove(&(_queueList), \
                             (CsrCmnListElm_t *)(_qElem)))

#define CSR_BT_GATT_QUEUE_ADD_LAST(_queueList) \
    (CsrBtGattQueueElement *)CsrCmnListElementAddLast(&(_queueList), \
                                                      sizeof(CsrBtGattQueueElement))

#define CSR_BT_GATT_QUEUE_ITERATE_REMOVE(_queueList,_func,_dataPtr) \
                              (CsrCmnListIterateAllowRemove(&(_queueList), (_func), (void *)(_dataPtr)))

#define CSR_BT_GATT_ACCESS_IND_QUEUE_ADD_LAST(_queueList) \
    (CsrBtGattAccessIndQueueElement *)CsrCmnListElementAddLast(&(_queueList), \
                                                      sizeof(CsrBtGattAccessIndQueueElement))

/* Gatt Service Handle List func */
typedef struct
{
    GattMainInst         *inst;
    CsrBtTypedAddr       address;
    CsrBtGattConnElement *conn;
} CsrBtGattAttServiceChangedElem;

#ifdef CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
#define CSR_BT_GATT_CLIENT_SERVICE_LIST_ADD_LAST(_cliSvcList) \
            (CsrBtGattClientService *)CsrCmnListElementAddLast(&(_cliSvcList), \
                                                             sizeof(CsrBtGattClientService))

CsrBool CsrBtGattCliSvcFindByGattId(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_CLIENT_SERVICE_LIST_FIND_BY_GATTID(_cliSvcList,_gattId) \
            ((CsrBtGattClientService *)CsrCmnListSearch(&(_cliSvcList), \
                                                       CsrBtGattCliSvcFindByGattId, \
                                                       (void *)(_gattId)))

CsrBool CsrBtGattCliSvcFindByHandle(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_CLIENT_SERVICE_LIST_FIND_BY_HANDLE(_cliSvcList,_handle) \
            ((CsrBtGattClientService *)CsrCmnListSearch(&(_cliSvcList), \
                                                       CsrBtGattCliSvcFindByHandle, \
                                                       (void *)(_handle)))
#endif /* CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION */

void CsrBtGattUnregisterApp(GattMainInst *inst, CsrBtGattAppElement *app, CsrBtGattQueueElement * qElem);

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
CsrBtGattDb *CsrBtGattGetMandatoryDbEntry(GattMainInst *inst, CsrUint8* pLocalLeFeatures);
#endif /* CSR_BT_GATT_INSTALL_FLAT_DB */

CsrBool CsrBtGattFindFreeConnId(GattMainInst *inst, CsrBtConnId *newBtConnId);
CsrBtGattLeRole CsrBtGattGetConnectionLeRole(l2ca_conflags_t l2capFlags);
CsrUint16 CsrBtGattResolveServerHandle(CsrUint16 startHandle, CsrUint16 handle);
CsrUint16 CsrBtGattGetAbsoluteServerHandle(CsrUint16 startHandle, CsrUint16 handle);


void CsrBtGattWriteRequestCancelHandler(void               *gattInst, 
                                           void            *qElem,
                                           CsrBtResultCode result, 
                                           CsrBtSupplier   supplier);

void CsrBtGattWriteCancelHandler(void               *gattInst, 
                                    void            *qElem,
                                    CsrBtResultCode result, 
                                    CsrBtSupplier   supplier);

void CsrBtGattClientInitiateReadBlob(CsrBtGattQueueElement *element, CsrBtGattEventSendReq *prim);

void CsrBtGattAttQueueCleanupOnDisconnection(GattMainInst *inst, CsrBtConnId btConnId);

void CsrBtGattClientSendMtuEventToSubscribedApp(CsrCmnListElm_t *elem, void *value);

#ifdef CSR_BT_GATT_CACHING
#define CSR_BT_GATT_SET_CHANGE_UNAWARE(flags) (CSR_MASK_SET(flags, CSR_BT_GATT_PS_CHANGE_UNAWARE_MASK))
#define CSR_BT_GATT_SET_CHANGE_AWARE(flags)   (CSR_MASK_UNSET(flags, CSR_BT_GATT_PS_CHANGE_UNAWARE_MASK))
#define CSR_BT_GATT_IS_CHANGE_UNAWARE(flags)  (CSR_MASK_IS_SET(flags, CSR_BT_GATT_PS_CHANGE_UNAWARE_MASK))
#define CSR_BT_GATT_IS_CHANGE_AWARE(flags)    (CSR_MASK_IS_UNSET(flags, CSR_BT_GATT_PS_CHANGE_UNAWARE_MASK))

#define CSR_BT_GATT_SET_ROBUST_CACHING(flags)   (CSR_MASK_SET(flags, CSR_BT_GATT_PS_ROBUST_CACHING_MASK))
#define CSR_BT_GATT_CLEAR_ROBUST_CACHING(flags) (CSR_MASK_UNSET(flags, CSR_BT_GATT_PS_ROBUST_CACHING_MASK))
#define CSR_BT_GATT_IS_ROBUST_CACHING_ENABLED(flags) (CSR_MASK_IS_SET(flags, CSR_BT_GATT_PS_ROBUST_CACHING_MASK))

#define CSR_BT_GATT_SET_SERVICE_CHANGE(conn)   (CSR_MASK_SET((conn)->connFlags, CSR_BT_GATT_PS_SERVICE_CHANGE_MASK))
#define CSR_BT_GATT_CLEAR_SERVICE_CHANGE(conn) (CSR_MASK_UNSET((conn)->connFlags, CSR_BT_GATT_PS_SERVICE_CHANGE_MASK))
#define CSR_BT_GATT_IS_SERVICE_CHANGE_ENABLED(conn) (CSR_MASK_IS_SET((conn)->connFlags, CSR_BT_GATT_PS_SERVICE_CHANGE_MASK))

#define CSR_BT_GATT_SET_DB_INITIALISED(inst)  (inst)->dbInitialised = TRUE
#define CSR_BT_GATT_IS_DB_MODIFICATION(inst)  ((inst)->dbInitialised)

void CsrBtGattCachingNewConn(GattMainInst *inst, CsrBtGattConnElement *conn);
void CsrBtGattReadDbHashRestoreHandler(GattMainInst *inst, CsrBtGattQueueElement *element, CsrUint16 mtu);
void CsrBtGattCachingAccessIndHandler(GattMainInst *inst, ATT_ACCESS_IND_T *prim);
void CsrBtGattDbHashHandler(GattMainInst *inst, ATT_READ_BY_TYPE_CFM_T *prim);
#else
#define CsrBtGattCachingNewConn(inst, conn) ((void)0)
#define CSR_BT_GATT_IS_SERVICE_CHANGE_ENABLED(conn) FALSE
#endif

#ifdef CSR_BT_GATT_INSTALL_EATT
#define CSR_BT_GATT_PS_EATT_MASK                 0x08 /* EATT enabled by remote device */
#define CSR_BT_GATT_PS_MHVN_MASK                 0x10 /* MHVN enabled by remote device */
#define CSR_BT_GATT_PS_SSF_MASK                  0x20 /* SSF Mask */

#define CSR_BT_GATT_SET_EATT(flags)   (CSR_MASK_SET(flags, CSR_BT_GATT_PS_EATT_MASK))
#define CSR_BT_GATT_CLEAR_EATT(flags) (CSR_MASK_UNSET(flags, CSR_BT_GATT_PS_EATT_MASK))
#define CSR_BT_GATT_IS_EATT_ENABLED(flags) (CSR_MASK_IS_SET(flags, CSR_BT_GATT_PS_EATT_MASK))
    
#define CSR_BT_GATT_SET_MHVN(flags)   (CSR_MASK_SET(flags, CSR_BT_GATT_PS_MHVN_MASK))
#define CSR_BT_GATT_CLEAR_MHVN(flags) (CSR_MASK_UNSET(flags, CSR_BT_GATT_PS_MHVN_MASK))
#define CSR_BT_GATT_IS_MHVN_ENABLED(flags) (CSR_MASK_IS_SET(flags, CSR_BT_GATT_PS_MHVN_MASK))
    
#define CSR_BT_GATT_SET_SSF(flags)   (CSR_MASK_SET(flags, CSR_BT_GATT_PS_SSF_MASK))
#define CSR_BT_GATT_CLEAR_SSF(flags) (CSR_MASK_UNSET(flags, CSR_BT_GATT_PS_SSF_MASK))
#define CSR_BT_GATT_IS_SSF_ENABLED(flags) (CSR_MASK_IS_SET(flags, CSR_BT_GATT_PS_SSF_MASK))
#endif

#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL

#define GATT_ENCRYPTED_DATA_KEY_MATERIAL_INDICATION_ENABLED                     0x02
#define GATT_ENCRYPTED_DATA_KEY_MATERIAL_LENGTH                                 0x18

#define GATT_PS_ENCRYPTED_DATA_KEY_MATERIAL_INDICATION_MASK                     0x80 /* Encrypted Data Key Material indication enabled by remote device */
#define GATT_PS_ENCRYPTED_DATA_KEY_MATERIAL_CHANGE_UNAWARE_MASK                 0x0100 /* Encrypted Data Key Material change is aware to peer device */

#define GATT_SET_ENCRYPTED_DATA_KEY_MATERIAL_INDICATION_ENABLE(flags)           (CSR_MASK_SET(flags, GATT_PS_ENCRYPTED_DATA_KEY_MATERIAL_INDICATION_MASK))
#define GATT_CLEAR_ENCRYTPED_DATA_KEY_MATERIAL_INDICATION_ENABLE(flags)         (CSR_MASK_UNSET(flags, GATT_PS_ENCRYPTED_DATA_KEY_MATERIAL_INDICATION_MASK))
#define GATT_IS_ENCRYPTED_DATA_KEY_MATERIAL_INDICATION_ENABLED(flags)           (CSR_MASK_IS_SET(flags, GATT_PS_ENCRYPTED_DATA_KEY_MATERIAL_INDICATION_MASK))

#define GATT_SET_ENCRYTPED_DATA_KEY_MATERIAL_CHANGE_UNAWARE(flags)              (CSR_MASK_SET(flags, GATT_PS_ENCRYPTED_DATA_KEY_MATERIAL_CHANGE_UNAWARE_MASK))
#define GATT_CLEAR_ENCRYTPED_DATA_KEY_MATERIAL_CHANGE_UNAWARE(flags)            (CSR_MASK_UNSET(flags, GATT_PS_ENCRYPTED_DATA_KEY_MATERIAL_CHANGE_UNAWARE_MASK))
#define GATT_IS_ENCRYPTED_DATA_KEY_MATERIAL_CHANGE_UNAWARE(flags)               (CSR_MASK_IS_SET(flags, GATT_PS_ENCRYPTED_DATA_KEY_MATERIAL_CHANGE_UNAWARE_MASK))

void GattEncryptedDataKeyMaterialIndicationSend(GattMainInst* inst,
                                                CsrBtGattConnElement* conn,
                                                uint8* keyMaterial);

CsrBool GattUpdateEncryptedDataKeyMaterial(GattMainInst* inst, CsrBtGattWriteReq *prim);

void GattGapMsgHandler(GattMainInst* inst, ATT_ACCESS_IND_T* prim);

#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */

#ifdef GATT_CACHING_CLIENT_ROLE
#define CSR_BT_GATT_PS_CLIENT_ROBUST_CACHING_MASK               0x40      /* ROBUST Caching Feature Mask */
#define CSR_BT_GATT_SET_CLIENT_ROBUST_CACHING(flags)            (CSR_MASK_SET(flags, CSR_BT_GATT_PS_CLIENT_ROBUST_CACHING_MASK))
#define CSR_BT_GATT_CLEAR_CLIENT_ROBUST_CACHING(flags)          (CSR_MASK_UNSET(flags, CSR_BT_GATT_PS_CLIENT_ROBUST_CACHING_MASK))
#define CSR_BT_GATT_IS_CLIENT_ROBUST_CACHING_ENABLED(flags)     (CSR_MASK_IS_SET(flags, CSR_BT_GATT_PS_CLIENT_ROBUST_CACHING_MASK))
#endif /* GATT_CACHING_CLIENT_ROLE */

#define CsrBtGattException(type, msg) CsrGeneralException(CsrBtGattLto, 0, CSR_BT_ATT_PRIM, (type), 0, msg)

void CsrBtGattClientSendMtuEventToSubscribedApp(CsrCmnListElm_t *elem, void *value);

CsrUint8 CsrBtGattEattResetCid(CsrBtGattConnElement *conn, CsrUint16 cid, CsrUint16 status);
void CsrBtGattEattSetCid(CsrBtGattConnElement *conn, CsrUint16 cid);
CsrUint16 CsrBtGattfindFreeCid(GattMainInst *inst, CsrBtConnId btConnId, CsrBool legacyCid);
CsrUint16 CsrBtGattGetCid(GattMainInst *inst,CsrBtGattConnElement* conn, CsrBool legacyCid);


#ifdef GATT_CACHING_CLIENT_ROLE
#define GATT_WRITE_SERVICE_CHANGED_CCCD_SIZE      ((CsrUint16) 0x0002)

CsrBtGattConnElement* SecondTransportConn(GattMainInst* inst, CsrBtGattConnElement* conn);
CsrBool CsrBtGattCheckCids(CsrBtGattConnElement* conn);
CsrBool FlushPendingGattMessage(CsrBtGattPrim* prim, CsrBtGattQueueElement* qElem);
void HashHandler(GattMainInst* inst, CsrBtGattConnElement* conn);
void DbOutOfSyncAttMsgHandler(uint16 type, GattMainInst * inst);
void CsrBtFlushGattQueuedMsg(GattMainInst* inst, CsrBtConnId btConnId);
void GattSetHashPendingFunc(CsrCmnListElm_t* elem, void* value);
void CsrBtGattSendOutOfSyncEventToApps(GattMainInst* inst, CsrBtGattConnElement* conn);
#endif /* GATT_CACHING_CLIENT_ROLE */

void CsrBtGattPersistGattInfo(CsrBtAddressType addressType,
    const CsrBtDeviceAddr* addr,
    CsrBtGattConnElement *conn);


#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_PRIVATE_UTILS_H__ */

