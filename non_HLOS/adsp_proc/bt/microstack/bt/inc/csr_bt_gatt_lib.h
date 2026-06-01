#ifndef CSR_BT_GATT_LIB_H__
#define CSR_BT_GATT_LIB_H__
/******************************************************************************
 Copyright (c) 2010-2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #5 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_msg_transport.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_utils.h"
#include "csr_bt_tasks.h"
#include "csr_pmem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Common put_message functions to reduce code size */
void CsrBtGattMsgTransport(void *msg);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattRegisterReq

   Description:
   Request to register an application instance to Gatt.
   Note this is an incontext api and applications should always 
   invoke this API from the synergy scheduler task handler functions only. 
   This should never be invoked from the task init function or 
   from an independent thread.

  Parameters: 
    CsrSchedQid             _qid      - Protocol handle of the higher layer entity 
                                        registering with GATT.
    CsrUint16               _context  - Registration context.

  Returns:
    This returns a gatt identifier on success. On failure it returns 
    CSR_BT_GATT_INVALID_GATT_ID.
   -------------------------------------------------------------------- */
CsrBtGattId CsrBtGattRegister(CsrSchedQid qid);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattRegisterReqSend

   Description:
   Request to Register an application instance to Gatt.

   As confirmation the application will receive a
   CSR_BT_GATT_REGISTER_CFM message.
   
  Parameters: 
    CsrSchedQid             _qid      - Protocol handle of the higher layer entity 
                                        registering with GATT.
    CsrUint16               _context  - Registration context.
   -------------------------------------------------------------------- */
CsrBtGattRegisterReq *CsrBtGattRegisterReq_struct(CsrSchedQid qid, CsrUint16 context);
#define CsrBtGattRegisterReqSend(_qid,_context) {                       \
        CsrBtGattRegisterReq *msg__;                                    \
        msg__=CsrBtGattRegisterReq_struct(_qid,_context);               \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattUnregisterReqSend

   Description:
   Request to Unregister/Remove an application instance from Gatt.

   As confirmation the application will receive a
   CSR_BT_GATT_UNREGISTER_CFM message.

   Note that if the application has:
   1 A Connection or has created one, GATT will ensure that it is released.
   2 Added a local data base, GATT will ensure that it is removed.
   3 Reserved/allocated a range of attribute handles, GATT will ensure
   that these are deallocated.

   Also note GATT does NOT ensure that the following is done:

   1 If the application has requested a peer server to enable broadcast,
   by using CsrBtGattWriteServerConfigurationReqSend, GATT does
   NOT ensure that broadcast is disabled.

   2 If the application has requested a peer server to send notification
   or indication, by using CsrBtGattWriteClientConfigurationReqSend,
   and the local and remote device are still paired, GATT does
   NOT ensure that the server stops sending these events.

   E.g. it is up to the application to make sure this is done
   before CsrBtGattUnregisterReqSend is called.

   Parameters:
   CsrBtGattId _gattId - Application identifier
   -------------------------------------------------------------------- */
CsrBtGattUnregisterReq *CsrBtGattUnregisterReq_struct(CsrBtGattId gattId);

#define CsrBtGattUnregisterReqSend(_gattId) {                           \
        CsrBtGattUnregisterReq *msg__;                                  \
        msg__=CsrBtGattUnregisterReq_struct(_gattId);                   \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDbAllocReqSend

   Description:

   This request shall be used by the server, for reserving/allocating
   a range of attribute handles which it can use later for
   creating the local database.

   As confirmation the application will receive a
   CSR_BT_GATT_DB_ALLOC_CFM message.

   Parameters:
   CsrBtGattId _gattId               - Application identifier
   CsrUint16   _numOfAttrHandles     - Number of attribute handles
   CsrUint16   _preferredStartHandle - The StartHandle the application
   prefers, 0 = no preference
   -------------------------------------------------------------------- */
CsrBtGattDbAllocReq *CsrBtGattDbAllocReq_struct(CsrBtGattId gattId,
                                                CsrUint16   numOfAttrHandles,
                                                CsrUint16   preferredStartHandle);

#define CsrBtGattDbAllocReqSend(_gattId,_numOfAttrHandles,_preferredStartHandle) { \
        CsrBtGattDbAllocReq *msg__;                                     \
        msg__=CsrBtGattDbAllocReq_struct(_gattId,_numOfAttrHandles,_preferredStartHandle); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDbDeallocReqSend

   Description:

   The server application can free/deallocate the range of
   attribute handles that are previously allocated by calling
   CsrBtGattDbAllocReqSend.

   As confirmation the application will receive a
   CSR_BT_GATT_DB_DEALLOC_CFM message.

   Note that if some of the handles being deallocated
   are still present in the local Database, GATT will
   automatically remove them.

   Parameters:
   CsrBtGattId _gattId - Application identifier
   -------------------------------------------------------------------- */
CsrBtGattDbDeallocReq *CsrBtGattDbDeallocReq_struct(CsrBtGattId gattId);

#define CsrBtGattDbDeallocReqSend(_gattId) {                            \
        CsrBtGattDbDeallocReq *msg__;                                   \
        msg__=CsrBtGattDbDeallocReq_struct(_gattId);                    \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDbAddReqSend

   Description:

   This request shall be used by the server, for adding attributes to
   the local database.

   As confirmation the application will receive a
   CSR_BT_GATT_DB_ADD_CFM message.

   Parameters:
   CsrBtGattId _gattId - Application identifier
   CsrBtGattDb _db     - The local database
   -------------------------------------------------------------------- */
CsrBtGattDbAddReq *CsrBtGattDbAddReq_struct(CsrBtGattId gattId,
                                                CsrBtGattDb *db);
    
#define CsrBtGattDbAddReqSend(_gattId,_db) {                            \
            CsrBtGattDbAddReq *msg__;                                       \
            msg__=CsrBtGattDbAddReq_struct(_gattId,_db);                    \
            CsrBtGattMsgTransport(msg__);}
    
    /* --------------------------------------------------------------------
       Name:
       CsrBtGattDbRemoveReqSend
    
       Description:
    
       This request shall be use, by the server, for removing attributes
       from the local database.
    
       As confirmation the application will receive a
       CSR_BT_GATT_DB_REMOVE_CFM message.
    
       Parameters:
       CsrBtGattId _gattId    - Application identifier
       CsrBtGattHandle _start - First handle number to be removed
       CsrBtGattHandle _end   - Last handle number to be removed
       -------------------------------------------------------------------- */
CsrBtGattDbRemoveReq *CsrBtGattDbRemoveReq_struct(CsrBtGattId     gattId,
                                                      CsrBtGattHandle start,
                                                      CsrBtGattHandle end);
    
#define CsrBtGattDbRemoveReqSend(_gattId,_start,_end) {                 \
            CsrBtGattDbRemoveReq *msg__;                                    \
            msg__=CsrBtGattDbRemoveReq_struct(_gattId,_start,_end);         \
            CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattDbAccessRes primitive which
   is used by the functions:
   CsrBtGattDbWriteAccessResSend
   CsrBtGattDbReadAccessResSend
   -------------------------------------------------------------------- */
CsrBtGattDbAccessRes *CsrBtGattDbAccessRes_struct(CsrBtGattId              gattId,
                                                  CsrBtConnId              btConnId,
                                                  CsrBtGattHandle          attrHandle,
                                                  CsrBtGattDbAccessRspCode responseCode,
                                                  CsrUint16                valueLength,
                                                  CsrUint8                 *value);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDbWriteAccessResSend

   Description:
   This function shall always be called after the
   application has received a CSR_BT_GATT_DB_ACCESS_WRITE_IND
   message.

   Parameters:
   CsrBtGattId _gattId               - Application identifier
   CsrBtConnId _btConnId             - Connection identifier
   CsrBtGattHandle _handle           - The attribute handle
   received in the
   CSR_BT_GATT_DB_ACCESS_WRITE_IND
   message.
   CsrBtGattDbAccessRspCode _rspCode - One of the responseCodes defined
   in csr_bt_gatt_prim.h
   -------------------------------------------------------------------- */
#define CsrBtGattDbWriteAccessResSend(_gattId,_btConnId,_handle,_rspCode) { \
        CsrBtGattDbAccessRes *msg__;                                    \
        msg__=CsrBtGattDbAccessRes_struct(_gattId,_btConnId,_handle,_rspCode,0,NULL); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDbReadAccessResSend

   Description:
   This function shall always be called after the
   application has received a CSR_BT_GATT_DB_ACCESS_READ_IND
   message.

   Parameters:
   CsrBtGattId _gattId               - Application identifier
   CsrBtConnId _btConnId             - Connection identifier
   CsrBtGattHandle _handle           - The attribute handle
   received in the
   CSR_BT_GATT_DB_ACCESS_READ_IND
   message.
   CsrBtGattDbAccessRspCode _rspCode - One of the responseCodes defined
   in csr_bt_gatt_prim.h
   CsrUint16 _valueLength            - Length of the attribute value which
   has been read by the application
   CsrUint8 *_value                  - An allocated pointer of the attribute
   value.
   -------------------------------------------------------------------- */
#define CsrBtGattDbReadAccessResSend(_gattId,_btConnId,_handle,_rspCode,_valueLength,_value) { \
        CsrBtGattDbAccessRes *msg__;                                    \
        msg__=CsrBtGattDbAccessRes_struct(_gattId,_btConnId,_handle,_rspCode,_valueLength,_value); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattEventSendReq primitive which
   is used by the functions:
   CsrBtGattNotificationEventReqSend
   CsrBtGattIndicationEventReqSend
   CsrBtGattServiceChangedEventReqSend
   -------------------------------------------------------------------- */
CsrBtGattEventSendReq *CsrBtGattEventSendReq_struct(CsrBtGattId     gattId,
                                                    CsrBtConnId     btConnId,
                                                    CsrBtGattHandle attrHandle,
                                                    CsrBtGattHandle endGroupHandle,
                                                    CsrUint16       flags,
                                                    CsrUint16       valueLength,
                                                    CsrUint8        *value);
/* --------------------------------------------------------------------
   Name:
   CsrBtGattNotificationEventReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:

   This request is used, by the server, for sending a Charateristic
   Value notification to a Client, as defined in [Vol. 3]
   Part G section 4.10.1 - Notifications.

   As confirmation the application will receive a
   CSR_BT_GATT_EVENT_SEND_CFM message.

   Parameters:
   CsrBtGattId     _gattId      - Application identifier
   CsrBtConnId     _btConnId    - Connection identifier
   CsrBtGattHandle _attrHandle  - The Characteristic Value Handle being notified
   CsrUint16       _valueLength - Length of the attribute value that must be sent to Client
   CsrUint8 *      _value       - An allocated pointer of the attribute value
   -------------------------------------------------------------------- */
#define CsrBtGattNotificationEventReqSend(_gattId,_btConnId,_attrHandle,_valueLength,_value) { \
        CsrBtGattEventSendReq *msg__;                                   \
        msg__=CsrBtGattEventSendReq_struct(_gattId,_btConnId,_attrHandle,CSR_BT_GATT_ATTR_HANDLE_INVALID, \
                                           CSR_BT_GATT_NOTIFICATION_EVENT,_valueLength,_value); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattIndicationEventReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:

   This request is used, by the server, for sending a Charateristic
   Value indication to a Client, as defined in [Vol. 3]
   Part G section 4.11.1 - Indications.

   As confirmation the application will receive a
   CSR_BT_GATT_EVENT_SEND_CFM message.

   Parameters:
   CsrBtGattId     _gattId      - Application identifier
   CsrBtConnId     _btConnId    - Connection identifier
   CsrBtGattHandle _attrHandle  - The Characteristic Value Handle being indicated
   CsrUint16       _valueLength - Length of the attribute value that must be sent to Client
   CsrUint8 *      _value       - An allocated pointer of the attribute value
   -------------------------------------------------------------------- */
#define CsrBtGattIndicationEventReqSend(_gattId,_btConnId,_attrHandle,_valueLength,_value) { \
        CsrBtGattEventSendReq *msg__;                                   \
        msg__=CsrBtGattEventSendReq_struct(_gattId,_btConnId,_attrHandle,CSR_BT_GATT_ATTR_HANDLE_INVALID, \
                                           CSR_BT_GATT_INDICATION_EVENT,_valueLength,_value); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattServiceChangedEventReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request shall be used, by the server application, for
   indicating to connected devices that services have changed.
   A GATT based service is considered modified if the
   binding of the Attribute Handles to the associated Attributes
   group within a service definition are changed

   As confirmation the application will receive a
   CSR_BT_GATT_EVENT_SEND_CFM message.

   Note, for clients that have a trusted relationship (i.e. bond)
   with the local server, the attribute cache is valid across connections.
   For clients with a trusted relationship and not in a connection
   when a service change occurs, the local server shall send an indication
   when the client reconnects to the local server. For clients that do not
   have a trusted relationship with the local server, the attribute cache is
   valid only during the connection. Clients without a trusted relationship
   shall receive an indication when the service change occurs only during
   the current connection. For more info please refer to
   [Vol. 3] Part G section 7.1 - Service Changed

   Parameters:
   CsrBtGattId     _gattId      - Application identifier
   CsrBtConnId     _btConnId    - Connection identifier
   CsrBtGattHandle _startHandle - Start of Affected Attribute Handle Range
   CsrBtGattHandle _endHandle   - End of Affected Attribute Handle Range
   -------------------------------------------------------------------- */
#define CsrBtGattServiceChangedEventReqSend(_gattId,_btConnId,_startHandle,_endHandle) { \
        CsrBtGattEventSendReq *msg__;                                   \
        msg__=CsrBtGattEventSendReq_struct(_gattId,_btConnId,_startHandle, \
                                           _endHandle,CSR_BT_GATT_SERVICE_CHANGED_EVENT,0,NULL); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattDiscoverServicesReq primitive
   which is used by the functions:
   CsrBtGattDiscoverAllPrimaryServicesReqSend
   CsrBtGattDiscoverAllPrimaryServicesLocalReqSend
   CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend
   CsrBtGattDiscoverPrimaryServicesBy16BitUuidLocalReqSend
   CsrBtGattDiscoverPrimaryServicesBy128BitUuidReqSend
   CsrBtGattDiscoverPrimaryServicesBy128BitUuidLocalReqSend

   These functions Covers item 2, Primary Service Discovery,
   in the GATT feature table, which is defined in The BLUETOOTH
   SPECIFICATION Version 4.0 Vol 3 Part G section 4.2 -
   Feature Support And Procedure Mapping.
   -------------------------------------------------------------------- */
CsrBtGattDiscoverServicesReq *CsrBtGattDiscoverServicesReq_struct(CsrBtGattId gattId,
                                                                  CsrBtConnId btConnId,
                                                                  CsrBtUuid   uuid);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverAllPrimaryServicesReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request is used by a client for discovering all the primary
   services on a server. This procedure is defined in [Vol. 3]
   Part G section 4.4.1 - Discover All Primary Services

   Every time a primary service has been discovered a
   CSR_BT_GATT_DISCOVER_SERVICES_IND message is sent to the
   application. This procedure is complete when a
   CSR_BT_GATT_DISCOVER_SERVICES_CFM message is received

   The application is permitted to cancel this procedure early
   if a desired primary service is found prior to discovering
   all the primary services on the server. This can be done by
   calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_DISCOVER_SERVICES_CFM message.

   Parameters:
   CsrBtGattId _gattId   - Application identifier
   CsrBtConnId _btConnId - Connection identifier
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverAllPrimaryServicesReqSend(_gattId,_btConnId) { \
        CsrBtGattDiscoverServicesReq *msg__;                            \
        CsrBtUuid _uuid;                                                \
        CsrMemSet(_uuid.uuid, 0, CSR_BT_UUID128_SIZE);                  \
        _uuid.length = (CsrUint16) (sizeof(CsrBtUuid16));               \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(CSR_BT_GATT_UUID_PRIMARY_SERVICE_DECL,_uuid.uuid); \
        msg__=CsrBtGattDiscoverServicesReq_struct(_gattId,_btConnId,_uuid); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverAllPrimaryServicesLocalReqSend

   Description:
   The only differece re. the CsrBtGattDiscoverAllPrimaryServicesReqSend
   is that the primary services are discovered from the local
   database and not from a peer server.

   Parameters:
   CsrBtGattId _gattId   - Application identifier
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverAllPrimaryServicesLocalReqSend(_gattId) {      \
        CsrBtGattDiscoverAllPrimaryServicesReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request is used by a client for discovering a specific
   primary service on a server when only the 16-Bit Service
   UUID is known. This procedure is defined in [Vol. 3]
   Part G section 4.4.2 - Discover Primary Service by Service UUID.

   Every time a primary service with a matching 16-Bit service UUID
   has been discovered a CSR_BT_GATT_DISCOVER_SERVICES_IND message is
   sent to the application. This procedure is complete when a
   CSR_BT_GATT_DISCOVER_SERVICES_CFM message is received.

   Please note, that a specific primary service may exist multiple
   times on a server.

   The application is permitted to cancel this procedure early
   if a desired primary service is found prior to discovering
   all the primary services on the server. This can is done by
   calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_DISCOVER_SERVICES_CFM message.

   Parameters:
   CsrBtGattId _gattId   - Application identifier
   CsrBtConnId _btConnId - Connection identifier
   CsrBtUuid16 _uuid16   - The 16-bit service UUID to look for.
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend(_gattId,_btConnId,_uuid16) { \
        CsrBtGattDiscoverServicesReq *msg__;                            \
        CsrBtUuid _uuid;                                                \
        CsrMemSet(_uuid.uuid, 0, CSR_BT_UUID128_SIZE);                  \
        _uuid.length = (CsrUint16) (sizeof(CsrBtUuid16));               \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(_uuid16,_uuid.uuid);           \
        msg__=CsrBtGattDiscoverServicesReq_struct(_gattId,_btConnId,_uuid); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverPrimaryServicesBy16BitUuidLocalReqSend

   Description:
   As the only difference re. CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend
   is that the specified primary service are discovered from the local database
   and not from a peer server.

   Parameters:
   CsrBtGattId _gattId   - Application identifier
   CsrBtUuid16 _uuid16   - The 16-bit service UUID to look for.
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverPrimaryServicesBy16BitUuidLocalReqSend(_gattId,_uuid16) { \
        CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_uuid16);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverPrimaryServicesBy128BitUuidReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request is used by a client for discovering a specific
   primary service on a server when only the 128-Bit Service
   UUID is known. This procedure is defined in [Vol. 3]
   Part G section 4.4.2 - Discover Primary Service by Service UUID.

   Every time a primary service with a matching 128-Bit service UUID
   has been discovered a CSR_BT_GATT_DISCOVER_SERVICES_IND message is
   sent to the application. This procedure is complete when a
   CSR_BT_GATT_DISCOVER_SERVICES_CFM message is received

   Please note, that a specific primary service may exist multiple
   times on a server.

   The application is permitted to cancel this procedure early
   if a desired primary service is found prior to discovering
   all the primary services on the server. This can is done by
   calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_DISCOVER_SERVICES_CFM message.

   Parameters:
   CsrBtGattId _gattId   - Application identifier
   CsrBtConnId _btConnId - Connection identifier
   CsrBtUuid128 _uuid128 - The 128-bit service UUID to look for.
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverPrimaryServicesBy128BitUuidReqSend(_gattId,_btConnId,_uuid128) { \
        CsrBtGattDiscoverServicesReq *msg__;                            \
        CsrBtUuid _uuid;                                                \
        _uuid.length = (CsrUint16) (sizeof(CsrBtUuid128));              \
        SynMemCpyS(_uuid.uuid,CSR_BT_UUID128_SIZE,_uuid128,CSR_BT_UUID128_SIZE);\
        msg__=CsrBtGattDiscoverServicesReq_struct(_gattId,_btConnId,_uuid); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverPrimaryServicesBy128BitUuidLocalReqSend

   Description:
   As the only difference re. CsrBtGattDiscoverPrimaryServicesBy128BitUuidReqSend
   is that the specified primary service is discovered from the local
   database and not from a peer server.

   Parameters:
   CsrBtGattId  _gattId   - Application identifier
   CsrBtUuid128 _uuid128  - The 128-bit service UUID to look for.
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverPrimaryServicesBy128BitUuidLocalReqSend(_gattId,_uuid128) { \
        CsrBtGattDiscoverPrimaryServicesBy128BitUuidReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_uuid128);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverPrimaryServicesByUuidReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request is used by a client for discovering a specific
   primary service on a server when the 128-bit or 16-bit service UUID is known. 
   This procedure is defined in [Vol. 3]
   Part G section 4.4.2 - Discover Primary Service by Service UUID.

   Every time a primary service with a matching service UUID
   has been discovered a CSR_BT_GATT_DISCOVER_SERVICES_IND message is
   sent to the application. This procedure is complete when a
   CSR_BT_GATT_DISCOVER_SERVICES_CFM message is received

   Please note, that a specific primary service may exist multiple
   times on a server.

   The application is permitted to cancel this procedure early
   if a desired primary service is found prior to discovering
   all the primary services on the server. This can is done by
   calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_DISCOVER_SERVICES_CFM message.

   Parameters:
   CsrBtGattId _gattId       - Application identifier
   CsrBtConnId _btConnId     - Connection identifier
   att_uuid_type_t _uuidType - UUID type
   CsrBtUuid32[] _uuid       - The 128-bit or 16-bit service UUID to look for.
   
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverPrimaryServicesByUuidReqSend(_gattId,_btConnId,_uuidType,_uuid) {           \
        CsrBtUuid _srvUuid;                                                                          \
        if(_uuidType == ATT_UUID16) {                                                        \
            CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend(_gattId,_btConnId,CSR_BT_UUID_GET_UUID16_FROM_UUID32_ARRAY(_uuid)) }   \
        else if(_uuidType == ATT_UUID128) {                                                  \
            CSR_BT_UUID_GET_UUID128_FROM_UUID32_ARRAY(_srvUuid, _uuid);                              \
            CsrBtGattDiscoverPrimaryServicesBy128BitUuidReqSend(_gattId,_btConnId,_srvUuid.uuid); }  \
        }

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattFindInclServicesReq primitive
   which is used by the functions:
   CsrBtGattFindInclServicesReqSend
   CsrBtGattFindInclServicesLocalReqSend

   These functions Covers item 3, Relationship Discovery,
   in the GATT feature table, which is defined in The BLUETOOTH
   SPECIFICATION Version 4.0 Vol 3 Part G section 4.2 -
   Feature Support And Procedure Mapping.
   -------------------------------------------------------------------- */
CsrBtGattFindInclServicesReq *CsrBtGattFindInclServicesReq_struct(CsrBtGattId gattId,
                                                                  CsrBtConnId btConnId,
                                                                  CsrBtGattHandle startHandle,
                                                                  CsrBtGattHandle endGroupHandle);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattFindInclServicesReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request is used by a client for finding include service
   declarations within a service definition on a server, as
   defined in [Vol. 3]  Part G section 4.5.1 - Find Included Services.

   The service specified is identified by the service handle range.
   E.g. the Starting Handle shall be set to starting handle of the
   specified service and the End Group Handle shall be set to the
   ending handle of the specified service.

   Every time an Included Service Declaration has been discovered a
   CSR_BT_GATT_FIND_INCL_SERVICES_IND message is
   sent to the application. This procedure is complete when a
   CSR_BT_GATT_FIND_INCL_SERVICES_CFM message is received.

   The application is permitted to cancel this procedure early
   if a desired included service is found prior to discovering
   all the included services on the server. This can be done by
   calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_FIND_INCL_SERVICES_CFM message.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtConnId _btConnId        - Connection identifier
   CsrBtGattHandle _startHandle - The start handle of the specified service.
   CsrBtGattHandle _endHandle   - The end handle of the specified service.
   -------------------------------------------------------------------- */
#define CsrBtGattFindInclServicesReqSend(_gattId,_btConnId,_startHandle,_endHandle) { \
        CsrBtGattFindInclServicesReq *msg__;                            \
        msg__=CsrBtGattFindInclServicesReq_struct(_gattId,_btConnId,_startHandle,_endHandle); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattFindInclServicesLocalReqSend

   Description:
   The only difference re. CsrBtGattFindInclServicesReqSend is that
   the include service declarations are discovered from the local
   database and not from a peer server.

   Parameters:
   CsrBtGattId     _gattId      - Application identifier
   CsrBtGattHandle _startHandle - The start handle of the specified service.
   CsrBtGattHandle _endHandle   - The end handle of the specified service.
   -------------------------------------------------------------------- */
#define CsrBtGattFindInclServicesLocalReqSend(_gattId,_startHandle,_endHandle) { \
        CsrBtGattFindInclServicesReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_startHandle,_endHandle);}

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattDiscoverCharacReq primitive
   which is used by the functions:
   CsrBtGattDiscoverAllCharacOfAServiceReqSend
   CsrBtGattDiscoverAllCharacOfAServiceLocalReqSend
   CsrBtGattDiscoverCharacBy16BitUuidReqSend
   CsrBtGattDiscoverCharacBy16BitUuidLocalReqSend
   CsrBtGattDiscoverCharacBy128BitUuidReqSend
   CsrBtGattDiscoverCharacBy128BitUuidLocalReqSend

   These functions cover item 4, Characteristic Discovery,
   in the GATT feature table, which is defined in The BLUETOOTH
   SPECIFICATION Version 4.0 Vol 3 Part G section 4.2 -
   Feature Support And Procedure Mapping.
   -------------------------------------------------------------------- */
CsrBtGattDiscoverCharacReq *CsrBtGattDiscoverCharacReq_struct(CsrBtGattId     gattId,
                                                              CsrBtConnId     btConnId,
                                                              CsrBtUuid       uuid,
                                                              CsrBtGattHandle startHandle,
                                                              CsrBtGattHandle endGroupHandle);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverAllCharacOfAServiceReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request is used by a client for finding all the characteristic
   declarations within a service definition on a server when only
   the service handle range is known, as defined in [Vol. 3] Part
   G section 4.6.1 - Discover All Characteristics of a Service.

   The service specified is identified by the service handle range.
   E.g. the Starting Handle shall be set to starting handle of the
   specified service and the Ending Handle shall be set to the
   ending handle of the specified service.

   Every time a characteristic declaration has been discovered
   a CSR_BT_GATT_DISCOVER_CHARAC_IND message is
   sent to the application. This procedure is complete when a
   CSR_BT_GATT_DISCOVER_CHARAC_CFM message is received.

   The application is permitted to cancel this procedure early
   if a desired characteristic is found prior to discovering
   all the characteristics of the specified service supported
   on the server. This can is done by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_DISCOVER_CHARAC_CFM message.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtConnId _btConnId        - Connection identifier
   CsrBtGattHandle _startHandle - The start handle of the specified service.
   CsrBtGattHandle _endHandle   - The end handle of the specified service.
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverAllCharacOfAServiceReqSend(_gattId,_btConnId,_startHandle,_endHandle) { \
        CsrBtGattDiscoverCharacReq *msg__;                              \
        CsrBtUuid _uuid;                                                \
        CsrMemSet(_uuid.uuid, 0, CSR_BT_UUID128_SIZE);                  \
        _uuid.length = (CsrUint16) (sizeof(CsrBtUuid16));               \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(CSR_BT_GATT_UUID_CHARACTERISTIC_DECL,_uuid.uuid); \
        msg__=CsrBtGattDiscoverCharacReq_struct(_gattId,_btConnId,_uuid,_startHandle,_endHandle); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverAllCharacOfAServiceLocalReqSend

   Description:
   The only difference re. CsrBtGattDiscoverAllCharacOfAServiceReqSend
   is that all the characteristic declarations within a service
   definition are discovered from the local database and not from
   a peer server.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtGattHandle _startHandle - The start handle of the specified service.
   CsrBtGattHandle _endHandle   - The end handle of the specified service.
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverAllCharacOfAServiceLocalReqSend(_gattId,_startHandle,_endHandle) { \
        CsrBtGattDiscoverAllCharacOfAServiceReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID, \
                                                    _startHandle,_endHandle);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverCharacBy16BitUuidReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request is used by a client for discovering service
   characteristics on a server when only the service handle
   ranges are known and the 16-Bit characteristic UUID is known,
   as defined in [Vol. 3] Part G section 4.6.2 - Discover
   Characteristics by UUID.

   The service specified is identified by the service handle range.
   E.g. the Starting Handle shall be set to starting handle of the
   specified service and the Ending Handle shall be set to the
   ending handle of the specified service.

   Every time a characteristic declaration with a matching 16-Bit
   characteristic UUID has been discovered a
   CSR_BT_GATT_DISCOVER_CHARAC_IND message is sent to the
   application. This procedure is complete when a
   CSR_BT_GATT_DISCOVER_CHARAC_CFM message is received.

   The application is permitted to cancel this procedure early
   if a desired characteristic is found prior to discovering
   all the characteristics of the specified service supported
   on the server. This can is done by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_DISCOVER_CHARAC_CFM message.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtConnId _btConnId        - Connection identifier
   CsrBtGattHandle _startHandle - The start handle of the specified service.
   CsrBtGattHandle _endHandle   - The end handle of the specified service.
   CsrBtUuid16 _uuid16          - The 16-bit characteristic UUID
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverCharacBy16BitUuidReqSend(_gattId,_btConnId,_startHandle, \
                                                  _endHandle,_uuid16) { \
        CsrBtGattDiscoverCharacReq *msg__;                              \
        CsrBtUuid _uuid;                                                \
        CsrMemSet(_uuid.uuid, 0, CSR_BT_UUID128_SIZE);                  \
        _uuid.length = (CsrUint16) (sizeof(CsrBtUuid16));               \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(_uuid16,_uuid.uuid);           \
        msg__=CsrBtGattDiscoverCharacReq_struct(_gattId,_btConnId,_uuid,_startHandle,_endHandle); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverCharacBy16BitUuidLocalReqSend

   Description:
   The only difference re. CsrBtGattDiscoverCharacBy16BitUuidReqSend
   is that characteristics are discovered from the local database and
   not from a peer server.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtGattHandle _startHandle - The start handle of the specified service.
   CsrBtGattHandle _endHandle   - The end handle of the specified service.
   CsrBtUuid16 _uuid16          - The 16-bit characteristic UUID
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverCharacBy16BitUuidLocalReqSend(_gattId,_startHandle,_endHandle,_uuid16) { \
        CsrBtGattDiscoverCharacBy16BitUuidReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID, \
                                                  _startHandle,_endHandle,_uuid16);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverCharacBy128BitUuidReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request is used by a client for discovering service
   characteristics on a server when only the service handle
   ranges are known and the 128-Bit characteristic UUID is known,
   as defined in [Vol. 3] Part G section 4.6.2 - Discover
   Characteristics by UUID.

   The service specified is identified by the service handle range.
   E.g. the Starting Handle shall be set to starting handle of the
   specified service and the Ending Handle shall be set to the
   ending handle of the specified service.

   Every time a characteristic declaration with a matching 128-Bit
   characteristic UUID has been discovered a
   CSR_BT_GATT_DISCOVER_CHARAC_IND message is sent to the
   application. This procedure is complete when a
   CSR_BT_GATT_DISCOVER_CHARAC_CFM message is received.

   The application is permitted to cancel this procedure early
   if a desired characteristic is found prior to discovering
   all the characteristics of the specified service supported
   on the server. This can be done by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_DISCOVER_CHARAC_CFM message.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtConnId _btConnId        - Connection identifier
   CsrBtGattHandle _startHandle - The start handle of the specified service.
   CsrBtGattHandle _endHandle   - The end handle of the specified service.
   CsrBtUuid128 _uuid128        - The 128-bit characteristic UUID
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverCharacBy128BitUuidReqSend(_gattId,_btConnId,_startHandle, \
                                                   _endHandle,_uuid128) { \
        CsrBtGattDiscoverCharacReq *msg__;                              \
        CsrBtUuid _uuid;                                                \
        _uuid.length = (CsrUint16) (sizeof(CsrBtUuid128));              \
        SynMemCpyS(_uuid.uuid,CSR_BT_UUID128_SIZE,_uuid128,CSR_BT_UUID128_SIZE);             \
        msg__=CsrBtGattDiscoverCharacReq_struct(_gattId,_btConnId,_uuid,_startHandle,_endHandle); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverCharacBy128BitUuidLocalReqSend

   Description:
   The only difference re. CsrBtGattDiscoverCharacBy128BitUuidReqSend
   is that characteristics are discovered from the local database and
   not from a peer server.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtGattHandle _startHandle - The start handle of the specified service.
   CsrBtGattHandle _endHandle   - The end handle of the specified service.
   CsrBtUuid128 _uuid128        - The 128-bit characteristic UUID
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverCharacBy128BitUuidLocalReqSend(_gattId,_startHandle, \
                                                        _endHandle,_uuid128) { \
        CsrBtGattDiscoverCharacBy128BitUuidReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID, \
                                                   _startHandle,_endHandle,_uuid128);}

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattDiscoverCharacDescriptorsReq
   primitive which is used by the functions:
   CsrBtGattDiscoverAllCharacDescriptorsReqSend
   CsrBtGattDiscoverAllCharacDescriptorsLocalReqSend

   These functions Cover item 5, Characteristic Descriptor Discovery,
   in the GATT feature table, which is defined in The BLUETOOTH
   SPECIFICATION Version 4.0 Vol 3 Part G section 4.2 -
   Feature Support And Procedure Mapping.
   -------------------------------------------------------------------- */
CsrBtGattDiscoverCharacDescriptorsReq *CsrBtGattDiscoverCharacDescriptorsReq_struct(CsrBtGattId gattId,
                                                                                    CsrBtConnId btConnId,
                                                                                    CsrBtGattHandle startHandle,
                                                                                    CsrBtGattHandle endGroupHandle);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverAllCharacDescriptorsReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3

   Description:
   This request is used by a client for finding all the characteristic
   descriptor's Attribute Handles and Attribute Types within a
   characteristic definition when only the characteristic handle
   range is known, as defined in [Vol. 3] Part G section 4.7.1 -
   Discover All Characteristic Descriptors.

   The service specified is identified by the service handle range.
   E.g. the Starting Handle shall be set to the handle of the
   specified characteristic value + 1 and the Ending Handle set to
   the ending handle of the specified characteristic.

   Every time a characteristic descriptor has been discovered
   a CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND message is
   sent to the application. This procedure is complete when a
   CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM message is received.

   The application is permitted to cancel this procedure early
   if a desired Characteristic Descriptor is found prior to
   discovering all the characteristic descriptors of the specified
   characteristic. This can is done by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM message.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtConnId _btConnId        - Connection identifier
   CsrBtGattHandle _startHandle - The handle of the specified characteristic value + 1
   CsrBtGattHandle _endHandle   - The end handle of the specified characteristic.
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverAllCharacDescriptorsReqSend(_gattId,_btConnId, \
                                                     _startHandle,_endHandle) { \
        CsrBtGattDiscoverCharacDescriptorsReq *msg__;                   \
        msg__=CsrBtGattDiscoverCharacDescriptorsReq_struct(_gattId,_btConnId, \
                                                           _startHandle,_endHandle); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDiscoverAllCharacDescriptorsLocalReqSend

   Description:
   The only difference re. CsrBtGattDiscoverAllCharacDescriptorsReqSend
   is that all the characteristic descriptor's Attribute Handles
   and Attribute Types within a characteristic definition are
   discovered from the local database and not from a peer server.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtGattHandle _startHandle - The handle of the specified characteristic value + 1
   CsrBtGattHandle _endHandle   - The end handle of the specified characteristic.
   -------------------------------------------------------------------- */
#define CsrBtGattDiscoverAllCharacDescriptorsLocalReqSend(_gattId,_startHandle,_endHandle) { \
        CsrBtGattDiscoverAllCharacDescriptorsReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID, \
                                                     _startHandle,_endHandle);}

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattReadReq
   primitive which is used by the functions:
   CsrBtGattReadReqSend
   CsrBtGattReadLocalReqSend
   CsrBtGattReadExtendedPropertiesReqSend
   CsrBtGattReadExtendedPropertiesLocalReqSend
   CsrBtGattReadUserDescriptionReqSend
   CsrBtGattReadUserDescriptionLocalReqSend
   CsrBtGattReadClientConfigurationReqSend
   CsrBtGattReadServerConfigurationReqSend
   CsrBtGattReadServerConfigurationLocalReqSend
   CsrBtGattReadPresentationFormatReqSend
   CsrBtGattReadPresentationFormatLocalReqSend
   CsrBtGattReadAggregateFormatReqSend
   CsrBtGattReadAggregateFormatLocalReqSend
   CsrBtGattReadProfileDefinedDescriptorReqSend
   CsrBtGattReadProfileDefinedDescriptorReqSend
   CsrBtGattReadProfileDefinedDescriptorLocalReqSend

   These functions Cover part of item 6, Characteristic Value Read,
   and item 10, Characteristic Descriptor Value Read, in the GATT
   feature table, which is defined in The BLUETOOTH SPECIFICATION
   Version 4.0 Vol 3 Part G section 4.2 - Feature Support And
   Procedure Mapping.
   -------------------------------------------------------------------- */
CsrBtGattReadReq *CsrBtGattReadReq_struct(CsrBtGattId     gattId,
                                          CsrBtConnId     btConnId,
                                          CsrBtGattHandle handle,
                                          CsrUint16       offset,
                                          CsrUint16       flags);
/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Characteristic Value or a Long Characteristic Value
   from a server, as defined in [Vol. 3] Part G, section 4.8.1 - Read
   Characteristic Value and section 4.8.3 - Read Long Characteristic Values

   As confirmation the application will receive a CSR_BT_GATT_READ_CFM
   message.

   This function shall only be used if the Characteristic Properties
   'Read' (0x02) bit is enabled. The Characteristic Properties bit
   field is part of the Characteristic Declaration Attribute Value
   which is defined in [Vol. 3] Part G,
   section 3.3.1.1 - Characteristic Properties.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_CFM message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The Charateristic Value Handle
   CsrUint16 _offset       - The offset of the first octet that shall be read
   -------------------------------------------------------------------- */
#define CsrBtGattReadReqSend(_gattId,_btConnId,_handle,_offset) {       \
        CsrBtGattReadReq *msg__;                                        \
        msg__=CsrBtGattReadReq_struct(_gattId,_btConnId,_handle,_offset,CSR_BT_GATT_READ_VALUE); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadLocalReqSend

   Description:
   The only difference re. CsrBtGattReadReqSend is that the Characteristic Value
   or the Long Characteristic Value is read from the local database
   and not from a peer server.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtGattHandle _handle - The Charateristic Value Handle
   -------------------------------------------------------------------- */
#define CsrBtGattReadLocalReqSend(_gattId,_handle) {                    \
        CsrBtGattReadReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_handle,0);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadExtendedPropertiesReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Characteristic Extended Properties declaration,
   by using the procedure defined in [Vol. 3] Part G,
   section 4.12.1 - Read Characteristic Descriptors

   As confirmation the application will receive a
   CSR_BT_GATT_READ_EXTENDED_PROPERTIES_CFM message.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_EXTENDED_PROPERTIES_CFM message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The handle of the Characteristic Extended
   Properties declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadExtendedPropertiesReqSend(_gattId,_btConnId,_handle) { \
        CsrBtGattReadReq *msg__;                                        \
        msg__=CsrBtGattReadReq_struct(_gattId,_btConnId,_handle,0,CSR_BT_GATT_READ_EXT_PROPERTIES); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadExtendedPropertiesLocalReqSend

   Description:
   The only difference re. CsrBtGattReadExtendedPropertiesReqSend is that
   the Characteristic Extended Properties declaration is read from
   the local database and not from a peer server.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtGattHandle _handle - The handle of the Characteristic Extended
   Properties declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadExtendedPropertiesLocalReqSend(_gattId,_handle) {  \
        CsrBtGattReadExtendedPropertiesReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_handle);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadUserDescriptionReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Characteristic User Description declaration,
   by using the procedures defined in [Vol. 3] Part G,
   section 4.12.1 - Read Characteristic Descriptors and
   section 4.12.2 - Read Long Characteristic Descriptors.

   As confirmation the application will received a
   CSR_BT_GATT_READ_USER_DESCRIPTION_CFM message.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_USER_DESCRIPTION_CFM message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The handle of the Characteristic User
   Description declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadUserDescriptionReqSend(_gattId,_btConnId,_handle) { \
        CsrBtGattReadReq *msg__;                                        \
        msg__=CsrBtGattReadReq_struct(_gattId,_btConnId,_handle,0,      \
                                      CSR_BT_GATT_READ_USER_DESCRIPTOR); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadUserDescriptionLocalReqSend

   Description:
   The only difference re. CsrBtGattReadUserDescriptionReqSend is that
   the Characteristic User Description declaration is read from
   the local database and not from a peer server.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtGattHandle _handle - The handle of the Characteristic User
   Description declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadUserDescriptionLocalReqSend(_gattId,_handle) {     \
        CsrBtGattReadUserDescriptionReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_handle);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadClientConfigurationReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Client Characteristic Configuration
   declaration, by using the procedures defined in [Vol. 3]
   Part G, section 4.12.1 - Read Characteristic Descriptors.

   As confirmation the application will receive a
   CSR_BT_GATT_READ_CLIENT_CONFIGURATION_CFM message.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_CLIENT_CONFIGURATION_CFM message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The handle of the Client Characteristic
   Configuration declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadClientConfigurationReqSend(_gattId,_btConnId,_handle) { \
        CsrBtGattReadReq *msg__;                                        \
        msg__=CsrBtGattReadReq_struct(_gattId,_btConnId,_handle,0,      \
                                      CSR_BT_GATT_READ_CLIENT_CONFIGURATION); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadServerConfigurationReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Server Characteristic Configuration declaration,
   by using the procedures defined in [Vol. 3] Part G,
   section 4.12.1 - Read Characteristic Descriptors.

   As confirmation the application will receive a
   CSR_BT_GATT_READ_SERVER_CONFIGURATION_CFM message.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_SERVER_CONFIGURATION_CFM message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The handle of the Server Characteristic
   Configuration declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadServerConfigurationReqSend(_gattId,_btConnId,_handle) { \
        CsrBtGattReadReq *msg__;                                        \
        msg__=CsrBtGattReadReq_struct(_gattId,_btConnId,_handle,0,      \
                                      CSR_BT_GATT_READ_SERVER_CONFIGURATION); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadServerConfigurationLocalReqSend

   Description:
   The only difference re. CsrBtGattReadServerConfigurationReqSend is that
   the Server Characteristic Configuration declaration is read from
   the local database and not from a peer server.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtGattHandle _handle - The handle of the Server Characteristic
   Configuration declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadServerConfigurationLocalReqSend(_gattId,_handle) { \
        CsrBtGattReadServerConfigurationReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_handle);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadPresentationFormatReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Characteristic Presentation Format declaration,
   by using the procedures defined in [Vol. 3] Part G,
   section 4.12.1 - Read Characteristic Descriptors.

   As confirmation the application will receive a
   CSR_BT_GATT_READ_PRESENTATION_FORMAT_CFM message.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_PRESENTATION_FORMAT_CFM message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The handle of the Characteristic
   Presentation Format declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadPresentationFormatReqSend(_gattId,_btConnId,_handle) { \
        CsrBtGattReadReq *msg__;                                        \
        msg__=CsrBtGattReadReq_struct(_gattId,_btConnId,_handle,0,      \
                                      CSR_BT_GATT_READ_PRESENTATION_FORMAT); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadPresentationFormatLocalReqSend

   Description:
   The only diffence re. CsrBtGattReadPresentationFormatReqSend is that
   the Characteristic Presentation Format declaration is read from
   the local database and not from a peer server.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtGattHandle _handle - The handle of the Characteristic
   Presentation Format declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadPresentationFormatLocalReqSend(_gattId,_handle) {  \
        CsrBtGattReadPresentationFormatReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_handle);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadAggregateFormatReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Characteristic Aggregate Format declaration,
   by using the procedures defined in [Vol. 3] Part G,
   section 4.12.1 - Read Characteristic Descriptors and
   section 4.12.2 - Read Long Characteristic Descriptors.

   As confirmation the application will receive a
   CSR_BT_GATT_READ_AGGREGATE_FORMAT_CFM message.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_AGGREGATE_FORMAT_CFM message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The handle of the Characteristic
   Aggregate Format declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadAggregateFormatReqSend(_gattId,_btConnId,_handle) { \
        CsrBtGattReadReq *msg__;                                        \
        msg__=CsrBtGattReadReq_struct(_gattId,_btConnId,_handle,0,      \
                                      CSR_BT_GATT_READ_AGGREGATE_FORMAT); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadAggregateFormatLocalReqSend

   Description:
   The only differnce re. CsrBtGattReadAggregateFormatReqSend is that
   the Characteristic Aggregate Format declaration is read from
   the local database and not from a peer server.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtGattHandle _handle - The handle of the Characteristic
   Aggregate Format declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadAggregateFormatLocalReqSend(_gattId,_handle) {     \
        CsrBtGattReadAggregateFormatReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_handle);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadProfileDefinedDescriptorReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Characteristic Descriptor that is defined
   by a higher layer Profile. E.g. a Characteristic Descriptor
   that is not part of the standard ones defined in [Vol. 3]
   Part G, section 3.3.3 - Characteristic Descriptor Declarations.

   To Read such a Descriptor GATT is using the procedures defined
   in [Vol. 3] Part G,
   section 4.12.1 - Read Characteristic Descriptors and
   section 4.12.2 - Read Long Characteristic Descriptors.

   As confirmation the application will receive a
   CSR_BT_GATT_READ_PROFILE_DEFINED_DESCRIPTOR_CFM message.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_PROFILE_DEFINED_DESCRIPTOR_CFM message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The handle of the Profile Defined
   Description declaration
   CsrUint16 _offset       - The offset of the first octet that shall be read
   -------------------------------------------------------------------- */
#define CsrBtGattReadProfileDefinedDescriptorReqSend(_gattId,_btConnId,_handle,_offset) { \
        CsrBtGattReadReq *msg__;                                        \
        msg__=CsrBtGattReadReq_struct(_gattId,_btConnId,_handle,_offset, \
                                      CSR_BT_GATT_READ_PROFILE_DEFINED); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadProfileDefinedDescriptorLocalReqSend

   Description:
   The only difference re. CsrBtGattReadProfileDefinedDescriptorReqSend
   is that the Characteristic Descriptor which is defined by a
   higher layer Profile is read from the local database and not
   from a peer server.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtGattHandle _handle - The handle of the Profile Defined
   Description declaration
   -------------------------------------------------------------------- */
#define CsrBtGattReadProfileDefinedDescriptorLocalReqSend(_gattId,_handle) { \
        CsrBtGattReadProfileDefinedDescriptorReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID, \
                                                     _handle,0);}

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattReadByUuidReq
   primitive which is used by the functions:
   CsrBtGattReadBy16BitUuidReqSend
   CsrBtGattReadBy16BitUuidLocalReqSend
   CsrBtGattReadBy128BitUuidReqSend
   CsrBtGattReadBy128BitUuidLocalReqSend

   These functions Cover part of item 6, Characteristic Value Read,
   in the GATT feature table, which is defined in The BLUETOOTH
   SPECIFICATION Version 4.0 Vol 3 Part G section 4.2 -
   Feature Support And Procedure Mapping.
   -------------------------------------------------------------------- */
CsrBtGattReadByUuidReq *CsrBtGattReadByUuidReq_struct(CsrBtGattId     gattId,
                                                      CsrBtConnId     btConnId,
                                                      CsrBtGattHandle startHandle,
                                                      CsrBtGattHandle endGroupHandle,
                                                      CsrBtUuid       uuid);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadBy16BitUuidReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Characteristic Value from a server when
   the client only knows the 16-bit characteristic UUID and
   does not know the handle of the characteristic. This
   procedure is defined in [Vol. 3] Part G, section 4.8.2 -
   Read Using Characteristic UUID

   Every time a Characteristic Value has been read the
   CSR_BT_GATT_READ_BY_UUID_IND message is sent to the
   application. This procedure is complete when a
   CSR_BT_GATT_READ_BY_UUID_CFM message is received

   This function shall only be used if the Characteristic Properties
   'Read' (0x02) bit is enabled. The Characteristic Properties bit
   field is part of the Characteristic Declaration Attribute Value
   which is defined in [Vol. 3] Part G,
   section 3.3.1.1 - Characteristic Properties.

   The application is permitted to cancel this procedure early
   if a desired Characteristic Value has been read prior to
   reading all the characteristic values. This can is done
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_BY_UUID_CFM message.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtConnId _btConnId        - Connection identifier
   CsrBtGattHandle _startHandle - The Start Handle
   CsrBtGattHandle _endHandle   - The End Handle.
   Note, the Start and End Handle
   parameters shall be set to the
   range over which this read
   is to be performed. This is
   typically the handle range for
   the service in which the
   characteristic belongs.
   CsrBtUuid16 _uuid16          - The known 16-bit characteristic UUID
   -------------------------------------------------------------------- */
#define CsrBtGattReadBy16BitUuidReqSend(_gattId,_btConnId,_startHandle,_endHandle,_uuid16) { \
        CsrBtGattReadByUuidReq *msg__;                                  \
        CsrBtUuid _uuid;                                                \
        CsrMemSet(_uuid.uuid, 0, CSR_BT_UUID128_SIZE);                  \
        _uuid.length = (CsrUint16) (sizeof(CsrBtUuid16));               \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(_uuid16,_uuid.uuid);           \
        msg__=CsrBtGattReadByUuidReq_struct(_gattId,_btConnId,_startHandle,_endHandle,_uuid); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadBy16BitUuidLocalReqSend

   Description:
   The only difference re. CsrBtGattReadBy16BitUuidReqSend is that the
   Characteristic Value is read from the local database
   and not from a peer server.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtGattHandle _startHandle - The Start Handle
   CsrBtGattHandle _endHandle   - The End Handle.
   Note, the Start and End Handle
   parameters shall be set to the
   range over which this read
   is to be performed. This is
   typically the handle range for
   the service in which the
   characteristic belongs.
   CsrBtUuid16 _uuid16          - The known 16-bit characteristic UUID
   -------------------------------------------------------------------- */
#define CsrBtGattReadBy16BitUuidLocalReqSend(_gattId,_startHandle,_endHandle,_uuid16) { \
        CsrBtGattReadBy16BitUuidReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_startHandle, \
                                        _endHandle,_uuid16);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadBy128BitUuidReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read a Characteristic Value from a server when
   the client only knows the 128-bit characteristic UUID and
   does not know the handle of the characteristic. This
   procedure is defined in [Vol. 3] Part G, section 4.8.2 -
   Read Using Characteristic UUID

   Every time a Characteristic Value has been read a
   CSR_BT_GATT_READ_BY_UUID_IND message is sent to the
   application. This procedure is complete when a
   CSR_BT_GATT_READ_BY_UUID_CFM message is received

   This function shall only be used if the Characteristic Properties
   'Read' (0x02) bit is enabled. The Characteristic Properties bit
   field is part of the Characteristic Declaration Attribute Value
   which is defined in [Vol. 3] Part G,
   section 3.3.1.1 - Characteristic Properties.

   The application is permitted to cancel this procedure early
   if a desired Characteristic Value has been read prior to
   reading all the characteristic values. This can is done
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_BY_UUID_CFM message.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtConnId _btConnId        - Connection identifier
   CsrBtGattHandle _startHandle - The Start Handle
   CsrBtGattHandle _endHandle   - The End Handle.
   Note, the Start and End Handle
   parameters shall be set to the
   range over which this read
   is to be performed. This is
   typically the handle range for
   the service in which the
   characteristic belongs.
   CsrBtUuid128 _uuid128        - The known 128-bit characteristic UUID
   -------------------------------------------------------------------- */
#define CsrBtGattReadBy128BitUuidReqSend(_gattId,_btConnId,_startHandle, \
                                         _endHandle,_uuid128) {         \
        CsrBtGattReadByUuidReq *msg__;                                  \
        CsrBtUuid _uuid;                                                \
        _uuid.length = (CsrUint16) (sizeof(CsrBtUuid128));              \
        SynMemCpyS(_uuid.uuid,CSR_BT_UUID128_SIZE,_uuid128,CSR_BT_UUID128_SIZE);             \
        msg__=CsrBtGattReadByUuidReq_struct(_gattId,_btConnId,_startHandle,_endHandle,_uuid); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadBy128BitUuidLocalReqSend

   Description:
   As CsrBtGattReadBy128BitUuidReqSend only difference is that the
   Characteristic Value is read from the local database
   and not from a peer server.

   Parameters:
   CsrBtGattId _gattId          - Application identifier
   CsrBtGattHandle _startHandle - The Start Handle
   CsrBtGattHandle _endHandle   - The End Handle.
   Note, the Start and End Handle
   parameters shall be set to the
   range over which this read
   is to be performed. This is
   typically the handle range for
   the service in which the
   characteristic belongs.
   CsrBtUuid128 _uuid128        - The known 128-bit characteristic UUID
   -------------------------------------------------------------------- */
#define CsrBtGattReadBy128BitUuidLocalReqSend(_gattId,_startHandle,_endHandle,_uuid128) { \
        CsrBtGattReadBy128BitUuidReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_startHandle, \
                                         _endHandle,_uuid128);}

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattReadMultiReq
   primitive which is used by the functions:
   CsrBtGattReadMultiReqSend
   CsrBtGattReadMultiLocalReqSend

   These functions cover part of item 6, Characteristic Value Read,
   in the GATT feature table, which is defined in The BLUETOOTH
   SPECIFICATION Version 4.0 Vol 3 Part G section 4.2 -
   Feature Support And Procedure Mapping.
   -------------------------------------------------------------------- */
CsrBtGattReadMultiReq *CsrBtGattReadMultiReq_struct(CsrBtGattId gattId,
                                                    CsrBtConnId btConnId,
                                                    CsrUint16 handlesCount,
                                                    CsrBtGattHandle *handles);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadMultiReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Read multiple Characteristic Values from a server, as
   defined in [Vol. 3] Part G, section 4.8.4 Read Multiple
   Characteristic Values

   As confirmation the application will receive a
   CSR_BT_GATT_READ_MULTI_CFM message.

   This function shall only be used if the Characteristic Properties
   'Read' (0x02) bit is enabled. The Characteristic Properties bit
   field is part of the Characteristic Declaration Attribute Value
   which is defined in [Vol. 3] Part G,
   section 3.3.1.1 - Characteristic Properties.

   Note: A client should not request to read multiple Characteristic
   Values when the Set Of Values parameter of the response is equal to
   (mtu - 1) octets in length, since it is not possible to determine
   if the last Characteristic Value was read or additional
   Characteristic Values exist but were truncated.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_MULTI_CFM message.

   Parameters:
   CsrBtGattId _gattId       - Application identifier
   CsrBtConnId _btConnId     - Connection identifier
   CsrUint16 _numOfHandles   - Number of attribute Handles that must be read
   CsrBtGattHandle *_handles - An allocated pointer of two or more attribute
   handles
   -------------------------------------------------------------------- */
#define CsrBtGattReadMultiReqSend(_gattId,_btConnId,_numOfHandles,_handles) { \
        CsrBtGattReadMultiReq *msg__;                                   \
        msg__=CsrBtGattReadMultiReq_struct(_gattId,_btConnId,_numOfHandles,_handles); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadMultiLocalReqSend

   Description:
   The only difference re. CsrBtGattReadMultiReqSend is that the multiple
   Characteristic Values are read from the local database and not
   from a peer server.

   Parameters:
   CsrBtGattId _gattId       - Application identifier
   CsrUint16 _numOfHandles   - Number of attribute handles that must be read
   CsrBtGattHandle *_handles - An allocated pointer of two or more attribute
   handles
   -------------------------------------------------------------------- */
#define CsrBtGattReadMultiLocalReqSend(_gattId,_numOfHandles,_handles) { \
        CsrBtGattReadMultiReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_numOfHandles,_handles);}

/* --------------------------------------------------------------------
   Common structure for the CsrBtGattWriteReq primitive which
   is used by the functions:
   CsrBtGattWriteCmdReqSend
   CsrBtGattWriteReqSend
   CsrBtGattWriteLocalReqSend
   CsrBtGattWriteSignedCmdReqSend
   CsrBtGattReliableWritesReqSend
   CsrBtGattWriteUserDescriptionReqSend
   CsrBtGattWriteUserDescriptionLocalReqSend
   CsrBtGattWriteServerConfigurationReqSend
   CsrBtGattWriteServerConfigurationLocalReqSend
   CsrBtGattWriteProfileDefinedDescriptorReqSend
   CsrBtGattWriteProfileDefinedDescriptorLocalReqSend

   These functions covers item 7, Characteristic Value Write,
   and part of item 11, Characteristic Descriptor Value Write,
   in the GATT feature table, which is defined in The BLUETOOTH
   SPECIFICATION Version 4.0 Vol 3 Part G section 4.2 -
   Feature Support And Procedure Mapping.
   -------------------------------------------------------------------- */
CsrBtGattWriteReq *CsrBtGattWriteReq_struct(CsrBtGattId             gattId,
                                            CsrBtConnId             btConnId,
                                            CsrUint16               flags,
                                            CsrUint16               attrWritePairsCount,
                                            CsrBtGattAttrWritePairs *attrWritePairs);

#define csrBtGattCommonWriteReqSend(_gattId,_btConnId,_handle,_offset,_valueLength,_value,_flags) { \
        CsrBtGattWriteReq *msg__;                                       \
        CsrBtGattAttrWritePairs *_ap;                                   \
        _ap = (CsrBtGattAttrWritePairs *) CsrPmemAlloc(sizeof(CsrBtGattAttrWritePairs)); \
        _ap->attrHandle  = _handle;                                     \
        _ap->offset      = _offset;                                     \
        _ap->valueLength = _valueLength;                                \
        _ap->value       = _value;                                      \
        msg__=CsrBtGattWriteReq_struct(_gattId,_btConnId,_flags,1,_ap); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteCmdReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Write a Characteristic Value to a server without any
   response, as defined in [Vol. 3] Part G, section 4.9.1 -
   Write Without Response.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   This function shall only be used if the Characteristic Properties
   -Write Without Response- (0x04) bit is enabled. The Characteristic
   Properties bit field is part of the Characteristic Declaration
   Attribute Value which is defined in [Vol. 3] Part G,
   section 3.3.1.1 - Characteristic Properties

   Note this sub-procedure cannot be used to write a long characteristic
   value, instead CsrBtGattWriteReqSend should be used.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The Charateristic Value Handle
   CsrUint16 _valueLength  - Length of the attribute value that must be written
   CsrUint8 *_value        - An allocated pointer of the attribute value
   -------------------------------------------------------------------- */
#define CsrBtGattWriteCmdReqSend(_gattId,_btConnId,_handle,_valueLength,_value) { \
        csrBtGattCommonWriteReqSend(_gattId,_btConnId,_handle,0,_valueLength, \
                                    _value,CSR_BT_GATT_WRITE_COMMAND);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Write a Characteristic Value or a Long Characteristic Value
   to a server, as defined in [Vol. 3] Part G, section 4.9.3 - Write
   Characteristic Value and section 4.9.4 - Write Long Characteristic Values

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   This function shall only be used if the Characteristic Properties
   'Write' (0x08) bit is enabled. The Characteristic Properties bit
   field is part of the Characteristic Declaration Attribute Value
   which is defined in [Vol. 3] Part G,
   section 3.3.1.1 - Characteristic Properties.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend. GATT will however only be able
   to cancel this procedure if a long characteristic value is being
   written.

   As confirmation the application will still receive the
   CSR_BT_GATT_WRITE_CFM message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The Charateristic Value Handle
   CsrUint16 _offset       - The offset of the first octet to be written
   CsrUint16 _valueLength  - Length of the attribute value that must be written
   CsrUint8 *_value        - An allocated pointer of the attribute value
   -------------------------------------------------------------------- */
#define CsrBtGattWriteReqSend(_gattId,_btConnId,_handle,_offset,_valueLength,_value) { \
        csrBtGattCommonWriteReqSend(_gattId,_btConnId,_handle,_offset,  \
                                    _valueLength,_value,CSR_BT_GATT_WRITE_REQUEST);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteLocalReqSend

   Description:
   Request to Write/Update a Characteristic Value or a Long Characteristic
   Value from the local database.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtGattHandle _handle - The local Charateristic Value Handle
   CsrUint16 _valueLength  - Length of the local attribute value that must be written
   CsrUint8 *_value        - An allocated pointer of the local attribute value
   -------------------------------------------------------------------- */
#define CsrBtGattWriteLocalReqSend(_gattId,_handle,_valueLength,_value) { \
        csrBtGattCommonWriteReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_handle,0, \
                                    _valueLength,_value,CSR_BT_GATT_WRITE_REQUEST);}


/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteLocalPreferredConnParametersReqSend

   Description:
   Request to Write/Update the Preferred Connection Parameter Characteristic Value
   from the local database.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   Parameters:
   CsrBtGattId _gattId           - Application identifier
   CsrUint8 *_value              - An allocated pointer of the local attribute value
   CsrUint16 *_connIntervalMin    - Connection Interval Minimum value
   CsrUint16 *_connIntervalMax    - Connection Interval Maximum value
   CsrUint16 *_connLatency        - Connection latency value
   CsrUint16 *_supervisionTimeout - supervision Timeout value
   -------------------------------------------------------------------- */
#define CsrBtGattWriteLocalPreferredConnParametersReqSend(_gattId, _connIntervalMin, _connIntervalMax, _connLatency, _supervisionTimeout) { \
        CsrUint8 *peripheralPreferredConnectionValue = CsrPmemZalloc(8); \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(_connIntervalMin, &peripheralPreferredConnectionValue[0]); \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(_connIntervalMax, &peripheralPreferredConnectionValue[2]); \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(_connLatency, &peripheralPreferredConnectionValue[4]); \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(_supervisionTimeout, &peripheralPreferredConnectionValue[6]); \
        csrBtGattCommonWriteReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,CSR_BT_GATT_ATTR_HANDLE_CONNECTION_PARAMS,0, \
                                    8,peripheralPreferredConnectionValue ,CSR_BT_GATT_WRITE_REQUEST);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteSignedCmdReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Write a Signed Characteristic Value to a server without any
   response, as defined in [Vol. 3] Part G, section 4.9.2 - Signed Write
   Without Response.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   This function shall only be used if the Characteristic Properties
   'Authenticated Signed Writes' (0x40) bit is enabled and the client
   and server device share a bond as defined in [Vol. 3] Part C,
   Generic Access Profile. The Characteristic Properties bit field is
   part of the Characteristic Declaration Attribute Value which is defined
   in [Vol. 3] Part G, section 3.3.1.1 - Characteristic Properties.

   Note this sub-procedure cannot be used to write a long Attribute.

   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The Charateristic Value Handle
   CsrUint16 _valueLength  - Length of the attribute value that must be written
   CsrUint8 *_value        - An allocated pointer of the attribute value
   -------------------------------------------------------------------- */
#define CsrBtGattWriteSignedCmdReqSend(_gattId,_btConnId,_handle,_valueLength,_value) { \
        csrBtGattCommonWriteReqSend(_gattId,_btConnId,_handle,0,_valueLength,_value, \
                                    CSR_BT_GATT_WRITE_SIGNED_COMMAND);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReliableWritesReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Write a Characteristic Value to a server with assurance,
   as defined in [Vol. 3] Part G, section 4.9.5 - Reliable Writes.

   Note this Request can also be used when multiple values must be written,
   in order and in a single operation.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   This function shall only be used if the Characteristic Extended
   Properties 'Reliable Write' (0x0001) bit is enabled.
   The Characteristic Extended Properties bit field is part of the
   Characteristic Extended Properties declaration Attribute Value
   which is defined in [Vol. 3] Part G, section 3.3.3.1 -
   Characteristic Extended Properties.

   The application is permitted to cancel this procedure early
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_WRITE_CFM message.

   Parameters:
   CsrBtGattId _gattId                      - Application identifier
   CsrBtConnId _btConnId                    - Connection identifier
   CsrUint16 _attrWritePairsCount           - Number of Charateristic
   Attributes Values to be
   written
   CsrBtGattAttrWritePairs *_attrWritePairs - An allocated list of
   Charateristic Attribute
   Value Handles, offsets
   and Attribute Values.
   -------------------------------------------------------------------- */
#define CsrBtGattReliableWritesReqSend(_gattId,_btConnId,               \
                                       _attrWritePairsCount,_attrWritePairs) { \
        CsrBtGattWriteReq *msg__;                                       \
        msg__=CsrBtGattWriteReq_struct(_gattId,_btConnId,CSR_BT_GATT_WRITE_RELIABLE, \
                                       _attrWritePairsCount,_attrWritePairs); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteUserDescriptionReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Write a Characteristic User Description Declaration
   Attribute Value to a server, as defined in [Vol. 3] Part G,
   section 4.12.3 - Write Characteristic Descriptors and
   section 4.12.4 - Write Long Characteristic Descriptors.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   This function shall only be used if the Characteristic Extended
   Properties 'Writable Auxiliaries' (0x0002) bit is enabled.
   The Characteristic Extended Properties bit field is part of the
   Characteristic Extended Properties declaration Attribute Value
   which is defined in [Vol. 3] Part G, section 3.3.3.1 -
   Characteristic Extended Properties.

   The application is permitted to cancel this procedure early
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_WRITE_CFM message.

   Parameters:
   CsrBtGattId _gattId        - Application identifier
   CsrBtConnId _btConnId      - Connection identifier
   CsrBtGattHandle _handle    - The handle of the Characteristic User
   Description declaration
   CsrUtf8String *_utf8String - An allocated pointer of the characteristic
   user description UTF-8 string
   -------------------------------------------------------------------- */
#define CsrBtGattWriteUserDescriptionReqSend(_gattId,_btConnId,_handle,_utf8String) { \
        CsrUint16 _utf8StringLength = (CsrUint16)(CsrUtf8StringLengthInBytes(_utf8String) + 1); \
        csrBtGattCommonWriteReqSend(_gattId,_btConnId,_handle,0,_utf8StringLength, \
                                    _utf8String,CSR_BT_GATT_WRITE_REQUEST);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteUserDescriptionLocalReqSend

   Description:
   Request to Write/Update a Characteristic User Description Declaration
   Attribute Value from the local database.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   Parameters:
   CsrBtGattId _gattId         - Application identifier
   CsrBtGattHandle _handle     - The handle of the Characteristic User
   Description declaration
   CsrUtf8String *_utf8String  - An allocated pointer of the characteristic
   user description UTF-8 string
   -------------------------------------------------------------------- */
#define CsrBtGattWriteUserDescriptionLocalReqSend(_gattId,_handle,_utf8String) { \
        CsrBtGattWriteUserDescriptionReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID, \
                                             _handle,_utf8String);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteServerConfigurationReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Write a Server Characteristic Configuration declaration
   Attribute Value to a server, as defined in [Vol. 3] Part G,
   section 4.12.3 - Write Characteristic Descriptors.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   This function shall only be used if the Characteristic Properties
   -Broadcast- (0x01) bit is enabled. The Characteristic Properties bit
   field is part of the Characteristic Declaration Attribute Value
   which is defined in [Vol. 3] Part G, section 3.3.1.1 -
   Characteristic Properties.

   Note a client may write this configuration descriptor to control
   the configuration of this characteristic on the server for all
   clients. There is a single instantiation of the Server
   Characteristic Configuration for all clients. Reads of the Server
   Characteristic Configuration shows the configuration all clients
   and writes affect the configuration for all clients.
   Authentication and authorisation may be required by the server
   to write the configuration descriptor.

   Parameters:
   CsrBtGattId _gattId                - Application identifier
   CsrBtConnId _btConnId              - Connection identifier
   CsrBtGattHandle _handle            - The handle of the Server
   Characteristic
   Configuration declaration
   CsrBtGattSrvConfigBits _configBits - The Server Characteristic
   Configuration bit field value:
   CSR_BT_GATT_SERVER_CHARAC_CONFIG_DISABLE -
   Disable Broadcast
   CSR_BT_GATT_SERVER_CHARAC_CONFIG_BROADCAST -
   Enable Broadcast
   -------------------------------------------------------------------- */
#define CsrBtGattWriteServerConfigurationReqSend(_gattId,_btConnId,_handle,_configBits) { \
        CsrUint16 _valueLength = (CsrUint16) (sizeof(CsrBtGattSrvConfigBits)); \
        CsrUint8 *_value = (CsrUint8 *) CsrPmemAlloc(_valueLength);     \
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(_configBits,_value);           \
        csrBtGattCommonWriteReqSend(_gattId,_btConnId,_handle,0,_valueLength, \
                                    _value,CSR_BT_GATT_WRITE_REQUEST);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteServerConfigurationLocalReqSend

   Description:
   Request to Write/Update a Server Characteristic Configuration declaration
   Attribute Value from the local database.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   Parameters:
   CsrBtGattId _gattId                - Application identifier
   CsrBtGattHandle _handle            - The handle of the Server
   Characteristic Configuration
   declaration
   CsrBtGattSrvConfigBits _configBits - The Server Characteristic
   Configuration bit field value:
   CSR_BT_GATT_SERVER_CHARAC_CONFIG_DISABLE -
   Disable Broadcast
   CSR_BT_GATT_SERVER_CHARAC_CONFIG_BROADCAST -
   Enable Broadcast
   -------------------------------------------------------------------- */
#define CsrBtGattWriteServerConfigurationLocalReqSend(_gattId,_handle,_configBits) { \
        CsrBtGattWriteServerConfigurationReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID,_handle,_configBits);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteProfileDefinedDescriptorReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 4.0 Vol 3.

   Description:
   Request to Write a higher layer characteristic descriptor declaration,
   which is not part of the standard ones defined in [Vol. 3] Part G,
   section 3.3.3 - Characteristic Descriptor Declarations.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message.

   The application is permitted to cancel this procedure early
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_WRITE_CFM message.


   Parameters:
   CsrBtGattId _gattId     - Application identifier
   CsrBtConnId _btConnId   - Connection identifier
   CsrBtGattHandle _handle - The handle of the Profile Defined
   Description declaration
   CsrUint16 _offset       - The offset of the first octet to be written
   CsrUint16 _valueLength  - Length of the attribute value that must be written
   CsrUint8 *_value        - An allocated pointer of the attribute value
   -------------------------------------------------------------------- */
#define CsrBtGattWriteProfileDefinedDescriptorReqSend(_gattId,_btConnId,_handle,_offset,_valueLength,_value) { \
        CsrBtGattWriteReqSend(_gattId,_btConnId,_handle,_offset,_valueLength,_value);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattWriteProfileDefinedDescriptorLocalReqSend

   Description:
   Request to Write/Update a higher layer characteristic descriptor
   declaration Attribute Value of the local database.

   As confirmation the application will receive a CSR_BT_GATT_WRITE_CFM
   message

   Parameters:
   CsrBtGattId _gattId     - Application identifier.
   CsrBtGattHandle _handle - The handle of the Profile Defined
   Description declaration.
   CsrUint16 _valueLength  - Length of the attribute value that must be written
   CsrUint8 *_value        - An allocated pointer of the attribute value
   -------------------------------------------------------------------- */
#define CsrBtGattWriteProfileDefinedDescriptorLocalReqSend(_gattId,_handle, \
                                                           _valueLength,_value) { \
        CsrBtGattWriteProfileDefinedDescriptorReqSend(_gattId,CSR_BT_GATT_LOCAL_BT_CONN_ID, \
                                                      _handle,0,_valueLength,_value);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattCancelReqSend

   Description:
   Request to Cancel a running GATT procedure.

   As confirmation the application will receive the confirm message
   of the GATT procedure being cancelled.

   Note the following procedures may be cancelled:

   CsrBtGattDiscoverAllPrimaryServicesReqSend
   CsrBtGattDiscoverAllPrimaryServicesLocalReqSend
   CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend
   CsrBtGattDiscoverPrimaryServicesBy16BitUuidLocalReqSend
   CsrBtGattDiscoverPrimaryServicesBy128BitUuidReqSend
   CsrBtGattDiscoverPrimaryServicesBy128BitUuidLocalReqSend
   CsrBtGattFindInclServicesReqSend
   CsrBtGattFindInclServicesLocalReqSend
   CsrBtGattDiscoverAllCharacOfAServiceReqSend
   CsrBtGattDiscoverAllCharacOfAServiceLocalReqSend
   CsrBtGattDiscoverCharacBy16BitUuidReqSend
   CsrBtGattDiscoverCharacBy16BitUuidLocalReqSend
   CsrBtGattDiscoverCharacBy128BitUuidReqSend
   CsrBtGattDiscoverCharacBy128BitUuidLocalReqSend
   CsrBtGattDiscoverAllCharacDescriptorsReqSend
   CsrBtGattDiscoverAllCharacDescriptorsLocalReqSend
   CsrBtGattReadReqSend
   CsrBtGattReadBy16BitUuidReqSend
   CsrBtGattReadBy16BitUuidLocalReqSend
   CsrBtGattReadBy128BitUuidReqSend
   CsrBtGattReadBy128BitUuidLocalReqSend
   CsrBtGattReadMultiReqSend
   CsrBtGattWriteReqSend
   CsrBtGattReliableWritesReqSend
   CsrBtGattWriteUserDescriptionReqSend
   CsrBtGattWriteProfileDefinedDescriptorReqSend
   CsrBtGattReadExtendedPropertiesReqSend
   CsrBtGattReadUserDescriptionReqSend
   CsrBtGattReadClientConfigurationReqSend
   CsrBtGattReadServerConfigurationReqSend
   CsrBtGattReadPresentationFormatReqSend
   CsrBtGattReadAggregateFormatReqSend
   CsrBtGattReadAggregateFormatReqSend
   CsrBtGattReadProfileDefinedDescriptorReqSend

   Parameters:
   CsrBtGattId _gattId   - Application identifier
   CsrBtConnId _btConnId - Connection identifier
   -------------------------------------------------------------------- */
CsrBtGattCancelReq *CsrBtGattCancelReq_struct(CsrBtGattId gattId, CsrBtConnId btConnId);

#define CsrBtGattCancelReqSend(_gattId, _btConnId) {                    \
        CsrBtGattCancelReq *msg__;                                      \
        msg__=CsrBtGattCancelReq_struct(_gattId, _btConnId);            \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
  Name: 
    CsrBtGattSetEventMaskReqSend
    
  Description: 
    
    Allows the application to subscribe for the events:
    CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_NONE,
    CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_PHYSICAL_LINK_STATUS
    CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_PARAM_CONN_UPDATE_IND.
    CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_RESTART_IND
    CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_SCAN_RSP_STATUS
    CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_SERVICE_CHANGED

   If set to CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_PHYSICAL_LINK_STATUS
   the application will receive a CSR_BT_GATT_PHYSICAL_LINK_STATUS_IND
   message whenever a physical GATT/ATT link is established or disconnected.

   If set to CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_NONE the application
   will stop receiving the event(s) defined by CsrBtGattEventMask.

   As confirmation the application will receive a
   CSR_BT_GATT_SET_EVENT_MASK_CFM message.

   Parameters:
   CsrBtGattId _gattId        - Application identifier
   CsrBtGattEventMask _eventM - Defines the event(s) to subscribe for
   -------------------------------------------------------------------- */
CsrBtGattSetEventMaskReq *CsrBtGattSetEventMaskReq_struct(CsrBtGattId        gattId,
                                                          CsrBtGattEventMask eventMask);

#define CsrBtGattSetEventMaskReqSend(_gattId,_eventM) {                 \
        CsrBtGattSetEventMaskReq *msg__;                                \
        msg__=CsrBtGattSetEventMaskReq_struct(_gattId,_eventM);         \
        CsrBtGattMsgTransport(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtGattFreeUpstreamMessageContents
 *
 *  DESCRIPTION
 *      Function to free contents of all GATT upstream primitives *
 *
 *  PARAMETERS
 *      eventClass :  Must be CSR_BT_GATT_PRIM,
 *      msg:          The upstream message received from Synergy BT GATT
 *----------------------------------------------------------------------------*/
extern void CsrBtGattFreeUpstreamMessageContents(CsrUint16 eventClass,
                                                 void      *message);


/* --------------------------------------------------------------------
  Name: 
    CsrBtGattClientRegisterServiceReqSend
    
  Description: 
    This function registers the service range which the client app is 
    intereseted in getting the notification or indication.
    Note: In order to get the notification or indication client app 
    should configure the client characteristic configuration 
    on the remote device.

  Note:
  When the GATT_CACHING_CLIENT_ROLE feature is enabled for the product then,
  Application sitting on top of GATT shall not register for the GATT service range
  as these characertistics are getting handled within GATT e.g. serivce changed 
  indication Handling.When Remote database modification will happen,
  GATT will autonomously reply for the service changed indication.

  Parameters: 
    CsrBtGattId     _gattId        - Application identifier
    CsrBtGattHandle _start         - Start handle
    CsrBtGattHandle _end           - End handle
    CsrBtTypedAddr  _address       - The device address of the peer server 
   -------------------------------------------------------------------- */
CsrBtGattClientRegisterServiceReq *CsrBtGattClientRegisterServiceReq_struct(CsrBtGattId gattId, 
                                                                                CsrBtGattHandle start,
                                                                                CsrBtGattHandle end,
                                                                                CsrBtTypedAddr address);

#define CsrBtGattClientRegisterServiceReqSend(_gattId,_start,_end,_address) {         \
        CsrBtGattClientRegisterServiceReq *msg__;                                               \
        msg__=CsrBtGattClientRegisterServiceReq_struct(_gattId,_start,_end,_address); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
  Name: 
    CsrBtGattClientIndicationRspSend
    
  Description: 
    This function lets the client app to send the response for the handle indication which was 
    earlier sent as part of CSR_BT_GATT_CLIENT_INDICATION_IND.

  Parameters: 
    CsrBtGattId     _gattId        - Application identifier
    CsrBtConnId     _btConnId      - Connection identifier 
   -------------------------------------------------------------------- */
CsrBtGattClientIndicationRsp *CsrBtGattClientIndicationRsp_struct(CsrBtGattId gattId, 
                                                                                 CsrBtConnId btConnId);

#define CsrBtGattClientIndicationRspSend(_gattId,_btConnId) {        \
        CsrBtGattClientIndicationRsp *msg__;                         \
        msg__=CsrBtGattClientIndicationRsp_struct(_gattId,_btConnId);\
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattFlatDbRegisterReqSend

   Description:
   This request shall be used by the server, for registering flat database.
   As confirmation the application will receive CSR_BT_GATT_FLAT_DB_REGISTER_CFM.

   Parameters:
   CsrSchedQid _qid - Protocol handle of the application
   CsrUint16 _flags - Flags
   CsrUint16 _size_db - Size of flat database
   CsrUint16 _db - Flat database
   -------------------------------------------------------------------- */
CsrBtGattFlatDbRegisterReq *CsrBtGattFlatDbRegisterReq_struct(CsrSchedQid qid,
                                                    CsrUint16 flags,
                                                    CsrUint16 size_db,
                                                    CsrUint16 *db);

#define CsrBtGattFlatDbRegisterReqSend(_qid,_flags,_size_db,_db) {          \
        CsrBtGattFlatDbRegisterReq *msg__;                                  \
        msg__=CsrBtGattFlatDbRegisterReq_struct(_qid,_flags,_size_db,_db);  \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattFlatDbRegisterHandleRangeReqSend

   Description:
   This request shall be used by the server, for registering the handle range
   for a specific service.
   As confirmation the application will receive CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM

   Parameters:
   CsrBtGattId _gattId - Application identifier
   CsrUint16 _start - First handle in the GATT database that the server task is interested in
   CsrUint16 _end   - Last handle in the GATT database that the server task is interested in
   -------------------------------------------------------------------- */
CsrBtGattFlatDbRegisterHandleRangeReq                                            \
            *CsrBtGattFlatDbRegisterHandleRangeReq_struct(CsrBtGattId gattId,
                                                          CsrUint16 start,
                                                          CsrUint16 end);

#define CsrBtGattFlatDbRegisterHandleRangeReqSend(_gattId,_start,_end) {         \
        CsrBtGattFlatDbRegisterHandleRangeReq *msg__;                            \
        msg__=CsrBtGattFlatDbRegisterHandleRangeReq_struct(_gattId,_start,_end); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
  Name: 
    CsrBtGattClientExchangeMtuReqSend
    
  Description: 
    This function lets client application exchange the mtu. Note: This
    function has to be called only once during the connection life cycle and
    should be invoked as soon as a connection is established before any
    other gatt operations are performed.

  Parameters: 
    CsrBtGattId     _gattId        - Application identifier
    CsrBtGattHandle _start         - Start handle
    CsrBtGattHandle _end           - End handle
    CsrBtTypedAddr  _address       - The device address of the peer server 
   -------------------------------------------------------------------- */
CsrBtGattClientExchangeMtuReq *CsrBtGattClientExchangeMtuReq_struct(CsrBtGattId gattId, 
                                                                    CsrBtConnId btConnId,
                                                                    CsrUint16   mtu);

#define CsrBtGattClientExchangeMtuReqSend(_gattId,_btConnId,_mtu) {        \
        CsrBtGattClientExchangeMtuReq *msg__;                              \
        msg__=CsrBtGattClientExchangeMtuReq_struct(_gattId,_btConnId,_mtu);\
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
  Name: 
    CsrBtGattRemoteClientExchangeMtuResSend
    
  Description: 
    This function lets application respond to the exchange the mtu request sent by 
    remote client.

  Parameters: 
    CsrBtGattId     _gattId        - Application identifier
    CsrBtGattHandle _start         - Start handle
    CsrBtGattHandle _end           - End handle
    CsrBtTypedAddr  _address       - The device address of the peer server 
   -------------------------------------------------------------------- */
CsrBtGattRemoteClientExchangeMtuRes *CsrBtGattRemoteClientExchangeMtuRes_struct(CsrBtGattId gattId, 
                                                                                CsrBtConnId btConnId,
                                                                                CsrUint16   mtu);

#define CsrBtGattRemoteClientExchangeMtuResSend(_gattId,_btConnId,_mtu) {        \
        CsrBtGattRemoteClientExchangeMtuRes *msg__;                              \
        msg__=CsrBtGattRemoteClientExchangeMtuRes_struct(_gattId,_btConnId,_mtu);\
        CsrBtGattMsgTransport(msg__);}


/* --------------------------------------------------------------------
   Name:
   CsrBtGattDbCommitReqSend

   Description:

   This request shall be used by the server to inform that database initialisation
   or modification is complete.
   This is applicable only for dynamic database with GATT caching support
   i.e CsrBtGattDbAdd/RemoveReqSend.
   If application is not adding any services apart from mandatory GATT and GAP, 
   this is still required to be sent by the application.

   Application is also expected to add paired device list before sending this primitive.

   As confirmation the application will receive a CSR_BT_GATT_DB_COMMIT_CFM.
   Application is expected to enable connection specific procedures e.g advertisement
   only after the database initialisation is complete.

   Parameters:
   CsrBtGattId _gattId - Application identifier
   -------------------------------------------------------------------- */
CsrBtGattDbCommitReq *CsrBtGattDbCommitReq_struct(CsrBtGattId gattId);

#define CsrBtGattDbCommitReqSend(_gattId) {                                \
        CsrBtGattDbCommitReq *msg__;                                       \
        msg__=CsrBtGattDbCommitReq_struct(_gattId);                        \
        CsrBtGattMsgTransport(msg__);}


#ifdef CSR_BT_GATT_INSTALL_EATT
/* --------------------------------------------------------------------
   Name:
   CsrBtGattPriorityReqSend

   Description: This sub-procedure let's the Application set the priority.
                This sub procedure will use by Application in the non-
                connected state to choose the Application Priority.
                List of Valid Value which App can use is as below:
                CSR_BT_APP_PRIORITY_LOW    (0x00)
                CSR_BT_APP_PRIORITY_MEDIUM (0x01)
                CSR_BT_APP_PRIORITY_HIGH   (0x02)

                Once App Priority setting is successfull, App will recieve a CFM message as:
                CSR_BT_GATT_RESULT_SUCCESS(0x0).

                If the Application is using some other value which is not permitted/allowed, it will
                Recieve an error message as:
                CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER

                If Application will try to use this sub proceudre in the connected state,
                App will recieve an error message as:
                CSR_BT_GATT_PRIORITY_CHANGE_NOT_ALLOWED

   Parameters:
   CsrBtGattId _gattId      - Application identifier
   CsrBtConnId _btConnId    - Connection identifier
   CsrUint16   _appPriority - AppPriority in csr_bt_gatt_prim.h.
                              Valid Values for setting the priority is as below:
                              CSR_BT_APP_PRIORITY_LOW    (0x00)
                              CSR_BT_APP_PRIORITY_MEDIUM (0x01)
                              CSR_BT_APP_PRIORITY_HIGH   (0x02)
   -------------------------------------------------------------------- */
CsrBtGattAppPriorityChangeReq *CsrBtGattAppPriorityChangeReq_struct(CsrBtGattId gattId, 
                                                     CsrUint32 btConnId,
                                                  CsrUint8 appPriority);

#define CsrBtGattPriorityReqSend(_gattId,_btConnId,_appPriority) {                       \
            CsrBtGattAppPriorityChangeReq *msg__;                                    \
            msg__=CsrBtGattAppPriorityChangeReq_struct(_gattId,_btConnId,_appPriority);               \
            CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------------
    Common structure for the CsrBtGattDbAccessReadMultiVarRsp primitive 
    which is used by the function:
    CsrBtGattDbReadMultiVarResSend
-------------------------------------------------------------------------- */
CsrBtGattDbAccessReadMultiVarRsp *CsrBtGattDbReadMultiVarRes_struct(CsrBtGattId gattId,
                                                 CsrBtConnId                  btConnId,
                                                 CsrBtGattDbAccessRspCode responseCode,
                                                 CsrBtGattHandle           errorHandle,
                                                 CsrUint16                valuesLength,
                                                 CsrUint8                     *values);

/* ---------------------------------------------------------------------
   Name:
   CsrBtGattDbReadMultiVarResSend

   Description:
   This function shall always be called after the
   application has received a CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_IND
   message.

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
   CsrUint8 *_value                  - An allocated pointer of the attribute
   value.
----------------------------------------------------------------------- */
#define CsrBtGattDbReadMultiVarResSend(_gattId,_btConnId,_rspCode,_errHandle,_valueLength,_value) { \
            CsrBtGattDbAccessReadMultiVarRsp *msg__;                                    \
            msg__=CsrBtGattDbReadMultiVarRes_struct(_gattId,_btConnId,_rspCode,_errHandle,_valueLength,_value); \
            CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattMultiVarLenNotificationEventReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 5.2 Vol 3

   Description:

   This request is used, by the server, for sending a Charateristic
   Value notifications to a Client, as defined in [Vol. 3]
   Part G section 4.10.2 - Multiple Variable Length Notifications.

   As confirmation the application will receive a
   CSR_BT_GATT_EVENT_SEND_CFM message.

   Parameters:
   CsrBtGattId     _gattId      - Application identifier
   CsrBtConnId     _btConnId    - Connection identifier
   CsrUint16       _valueLength - Length of the attribute value that must be sent to Client
   CsrUint8 *      _values      - The Handle Length Values Tuple List parameter
   -------------------------------------------------------------------- */
#define CsrBtGattMultiVarLenNotificationEventReqSend(_gattId,_btConnId,_valueLength,_values) { \
        CsrBtGattEventSendReq *msg__;                                   \
        msg__=CsrBtGattEventSendReq_struct(_gattId,_btConnId,0,CSR_BT_GATT_ATTR_HANDLE_INVALID, \
                                           CSR_BT_GATT_MULTI_VAR_NOTIFICATION_EVENT,_valueLength,_values); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattReadMultiVarReqSend

   Document Reference:
   [Vol. 3] - The BLUETOOTH SPECIFICATION Version 5.2 Vol 3.

   Description:
   This sub-procedure is used to read multiple variable length Characteristic
   Values from a server when the client knows the Characteristic Value Handles.
   The Attribute protocol ATT_READ_MULTIPLE_VARIABLE_REQ PDU is used
   with the Set Of Handles parameter set to the Characteristic Value Handles.
   The ATT_READ_MULTIPLE_VARIABLE_RSP PDU returns the Characteristic
   Values in the Length Value Tuple List parameter.


   As confirmation the application will receive a
   CSR_BT_GATT_READ_MULTI_VAR_CFM message.

   This function shall only be used if the Characteristic Properties
   'Read' (0x02) bit is enabled. The Characteristic Properties bit
   field is part of the Characteristic Declaration Attribute Value
   which is defined in [Vol. 3] Part G,
   section 3.3.1.1 - Characteristic Properties.

   Note: A client should not request to read multiple Characteristic
   Values when the Set Of Values parameter of the response is equal to
   (mtu - 1) octets in length, since it is not possible to determine
   if the last Characteristic Value was read or additional
   Characteristic Values exist but were truncated.

   The application is permitted to cancel this procedure early,
   by calling CsrBtGattCancelReqSend.

   As confirmation the application will still receive the
   CSR_BT_GATT_READ_MULTI__VAR_CFM message.

   Parameters:
   CsrBtGattId _gattId       - Application identifier
   CsrBtConnId _btConnId     - Connection identifier
   CsrUint16 _numOfHandles   - Number of attribute Handles that must be read
   CsrBtGattHandle *_handles - An allocated pointer of two or more attribute
   handles
   -------------------------------------------------------------------- */
CsrBtGattReadMultiVarReq *CsrBtGattReadMultiVarReq_struct(CsrBtGattId gattId,
                                                    CsrBtConnId btConnId,
                                                  CsrUint16 handlesCount,
                                               CsrBtGattHandle *handles);
#define CsrBtGattReadMultiVarReqSend(_gattId,_btConnId,_numOfHandles,_handles) { \
        CsrBtGattReadMultiVarReq *msg__;                                   \
        msg__=CsrBtGattReadMultiVarReq_struct(_gattId,_btConnId,_numOfHandles,_handles); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   GattSetEattMtu

   Description:
   Using this API, Application can indicate about the MTU size to be
   applied on the upcoming EATT connections. The value set here would
   be negotiated for all subsequent EATT connections. In case this API
   is not called then a default MTU size (EATT_PREFERRED_MTU) would be
   used to negotiate the MTU size for the EATT connection with the
   remote device.

   There is no confirmation to this API.

   Note:
   It is Application's responsibility to configure MTU to a sensible
   value so that memory blocks of size MTU value is going to be readily
   available from the memory pool, otherwise chip may panic because
   of heap exhaustion if overall memory usage in the system is high.
   If application tries  to set it lesser than spec defined
   minimum (0x40), this API would set the value to minimum itself,
   instead of rejecting it.

   Parameters:
   CsrUint16    desiredEattMtu - EATT MTU size which App/Test can set
   -------------------------------------------------------------------- */
void GattSetEattMtu(CsrUint16 desiredEattMtu);

/* --------------------------------------------------------------------
   Name:
    GattSelectPreferredBearer

   Description:
    Application can use this API to provide its choice of preferred
    GATT bearer, ATT or EATT. Please note, the preference is honored
    only when the bearer is available.

    By default, EATT is always preferred over ATT bearer.

   Note:
    It is strongly recommended to use this API for Development/Test
    purpose.
    EATT is available only on LE connections.

    There is no confirmation to the Application.

   Parameters:
    CsrUint8  preferredGattBearer - If GATT_PREFER_ATT_OVER_EATT,
                                     ATT is preferred over EATT.
                                    If GATT_PREFER_ATT_OVER_EATT,
                                     EATT is preferred over ATT.
-------------------------------------------------------------------- */
void GattSelectPreferredBearer(CsrUint8 preferredGattBearer);

#ifdef GATT_ENABLE_EATT_DISCONNECT
/* --------------------------------------------------------------------
   Name:
    CsrBtGattEattDisconnectReqSend

   Description:
    This function lets the application to disconnect a specified no. of 
    EATT CIDs established with a remote device.

   Note:
    This API is intended to be used for Development/Test Purpose Only.
    There is no confirmation sent back to the Application.

   Parameters:
    CsrBtGattId _gattId            - Application identifier
    CsrBtConnId _btConnId          - Connection identifier
    CsrUint16 _numCid2Disc         - Max no. of EATT CIDs to disconnect
   -------------------------------------------------------------------- */
CsrBtGattEattDisconnectReq *CsrBtGattEattDisconnectReq_struct(CsrBtGattId gattId,
                                                        CsrBtConnId btConnId,
                                                        CsrUint16 numCid2Disc);
#define CsrBtGattEattDisconnectReqSend(_gattId,_btConnId, _numCid2Disc) {                 \
        CsrBtGattEattDisconnectReq *msg__;                                  \
        msg__=CsrBtGattEattDisconnectReq_struct(_gattId,_btConnId, _numCid2Disc);         \
        CsrBtGattMsgTransport(msg__);}
#endif /* GATT_ENABLE_EATT_DISCONNECT */

#endif /* CSR_BT_GATT_INSTALL_EATT */

/* --------------------------------------------------------------------
   Name:
   CsrBtGattConfigModeReqSend

   Description: This sub-procedure let's the Application decide the long
                Write mode procedure.
                This sub procedure will use by Application in the non-
                connected state to choose the Long write/Read as a list mode.
                If App is setting the flags for write/read, App will receive
                message(data) in the form of array of pointers.If App is not setting the
                respective flag for write/read, App will receive message with long data
                in one single big buffer.
                List of Valid Value which App can use is as below:
                CSR_BT_GATT_LONG_WRITE_AS_LIST(0x01)
                CSR_BT_GATT_LONG_READ_AS_LIST(0x02)

                Once Mode is set successfully, App will recieve a CFM message as:
                CSR_BT_GATT_CONFIG_MODE_CFM(0x0).

                If the Application is using some other value which is not permitted/allowed, it will
                Recieve an error message as:
                CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER

   Parameters:
   CsrBtGattId _gattId      - Application identifier
   CsrUint8    _flags -       Mode in csr_bt_gatt_prim.h.
                              Valid Value for setting the Mode is as below:
                              CSR_BT_GATT_LONG_WRITE_AS_LIST(0x01)
                              CSR_BT_GATT_LONG_WRITE_AS_LIST(0x02)

   Note: flags can be extended in future for other feature settings as well.
   This method is supported for long read and long write procedure and not for reliable write.
   -------------------------------------------------------------------- */

CsrBtGattConfigModeReq *CsrBtGattConfigModeReq_struct(CsrBtGattId gattId, 
                                                CsrUint8 flags);

#define CsrBtGattConfigModeReqSend(_gattId,_flags) {                       \
                CsrBtGattConfigModeReq *msg__;                                    \
                msg__= CsrBtGattConfigModeReq_struct(_gattId,_flags);               \
                CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattConnectBredrReqSend

   Description:
   Request to initiate the establishment of a BR/EDR connection

   As confirmation the application will receive a
   CSR_BT_GATT_CONNECT_BREDR_CFM message. If the
   CSR_BT_GATT_CONNECT_BREDR_CFM message returns connection pending
   the application will later receive the
   CSR_BT_GATT_CONNECT_IND message when connection is established
   or else another CSR_BT_GATT_BREDR_CONNECT_CFM with failure reason code.

   The application is permitted to cancel this procedure
   after it has received a pending
   CSR_BT_GATT_CONNECT_BREDR_CFM message, by calling
   CsrBtGattDisconnectBredrReqSend.


  Parameters:
    CsrBtGattId _gattId       - Application identifier
    CsrBtTypedAddr _address   - The device address of the peer server
   -------------------------------------------------------------------- */
CsrBtGattConnectBredrReq *CsrBtGattConnectBredrReq_struct(CsrBtGattId        gattId,
                                                          CsrBtTypedAddr     address);

#define CsrBtGattConnectBredrReqSend(_gattId,_address) {         \
        CsrBtGattConnectBredrReq *msg__;                         \
        msg__=CsrBtGattConnectBredrReq_struct(_gattId,_address); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattAcceptBredrReqSend

   Description:
   Request to accept the establishment of a BR/EDR connection.

   As confirmation the application will receive a
   CSR_BT_GATT_ACCEPT_BREDR_CFM message. If the
   CSR_BT_GATT_ACCEPT_BREDR_CFM message returns success
   the application will later receive the
   CSR_BT_GATT_CONNECT_IND message when connection is established
   or else another CSR_BT_GATT_ACCEPT_BREDR_CFM with failure reason code.

   The application is permitted to cancel this procedure
   after it has received a successful
   CSR_BT_GATT_ACCEPT_BREDR_CFM message, by calling
   CsrBtGattDisconnectBredrReqSend.


  Parameters:
    CsrBtGattId _gattId       - Application identifier
   -------------------------------------------------------------------- */
CsrBtGattAcceptBredrReq *CsrBtGattAcceptBredrReq_struct(CsrBtGattId        gattId);
#define CsrBtGattAcceptBredrReqSend(_gattId) {                \
        CsrBtGattAcceptBredrReq *msg__;                                 \
        msg__=CsrBtGattAcceptBredrReq_struct(_gattId);        \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattCancelAcceptBredrReqSend

   Description:
   Request to cancel the original request for establishment of a BR/EDR connection.

   As confirmation the application will receive a
   CSR_BT_GATT_CANCEL_ACCEPT_BREDR_CFM message.

  Parameters:
    CsrBtGattId _gattId       - Application identifier
   -------------------------------------------------------------------- */
CsrBtGattCancelAcceptBredrReq *CsrBtGattCancelAcceptBredrReq_struct(CsrBtGattId gattId);
#define CsrBtGattCancelAcceptBredrReqSend(_gattId) {                   \
        CsrBtGattCancelAcceptBredrReq *msg__;                                 \
        msg__=CsrBtGattCancelAcceptBredrReq_struct(_gattId);           \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattConnectBredrResSend

   Description:
   This function shall always be called after the
   application has received a CSR_BT_GATT_CONNECT_BREDR_IND
   message.

   Parameters:
   CsrBtGattId _gattId               - Application identifier
   CsrBtTypedAddr  _address           - The device address of the peer server
   CsrBtGattConnectBredrResCode _rspCode           - One of the responseCodes defined
   in csr_bt_gatt_prim.h

   -------------------------------------------------------------------- */
CsrBtGattConnectBredrRes *CsrBtGattConnectBredrRes_struct(CsrBtGattId                  gattId,
                                                          CsrBtTypedAddr               address,
                                                          CsrBtGattConnectBredrResCode responseCode);
#define CsrBtGattConnectBredrResSend(_gattId,_address,_rspCode) {         \
        CsrBtGattConnectBredrRes *msg__;                                  \
        msg__=CsrBtGattConnectBredrRes_struct(_gattId,_address,_rspCode); \
        CsrBtGattMsgTransport(msg__);}

/* --------------------------------------------------------------------
   Name:
   CsrBtGattDisconnectBredrReqSend

   Description:
   Request to release/cancel a BR/EDR connection

   As confirmation the application will receive a
   CSR_BT_GATT_DISCONNECT_IND message.

   Parameters:
   CsrBtGattId _gattId    - Application identifier
   CsrBtTypedAddr  _address           - The device address of the peer server
   -------------------------------------------------------------------- */
CsrBtGattDisconnectBredrReq *CsrBtGattDisconnectBredrReq_struct(CsrBtGattId gattId,
                                                                CsrBtTypedAddr  address);
#define CsrBtGattDisconnectBredrReqSend(_gattId,_address) {                 \
        CsrBtGattDisconnectBredrReq *msg__;                                 \
        msg__=CsrBtGattDisconnectBredrReq_struct(_gattId,_address);         \
        CsrBtGattMsgTransport(msg__);}

#ifdef CSR_TARGET_PRODUCT_WEARABLE
 /* --------------------------------------------------------------------
   Name:
   CsrBtGattIsReliableAccessIndFinal

   Description:
   This API mainly needs to be used in the case of reliable writes to the server 
   after the application receives a CSR_BT_GATT_DB_ACCESS_WRITE_IND with data. 
   This API was introduced to manage the correct sequencing of preparewrite and  
   executewrite to the application for wearables. The return value indicates if 
   this was the last of the indication in the sequence of data received.

   Parameters:
   CsrBtConnId btConnId    - Connection identifier
   -------------------------------------------------------------------- */
CsrBool CsrBtGattIsReliableAccessIndFinal(CsrBtConnId btConnId);
#endif

#ifdef GATT_OFFLOAD
/* --------------------------------------------------------------------
   Name:
   GattOffloadConnInfoReqSend

   Description:

   This request shall be used for populating GATT connection in Microstack.
   NOTE :
   1. Currently works only for ATT fixed channel
   2. Returns btConnId in CFM which is device specific, same btConnId is used 
      for subsequent GATT operations

   App will receive GATT_OFFLOAD_CONN_INFO_CFM message as a confirmation.

   Parameters:
   CsrBtGattId gattId      - Application identifier. CFM is sent to this gatt id.
   CsrUint32  context      - Same context sent back in CFM
   CsrUint16  aclHandle    - Offloaded ACL connection handle
   CsrUint16  mtu          - MTU for offloaded ATT fixed channel
   -------------------------------------------------------------------- */
void GattOffloadConnInfoReqSend(CsrBtGattId gattId,
                                CsrUint32   context,
                                CsrUint16   aclHandle,
                                CsrUint16   mtu);

/* --------------------------------------------------------------------
   Name:
   GattOffloadEnableServiceReqSend

   Description:

   This request shall be used for offloading a GATT service to Microstack.
   NOTE :
   1. This API expects a complete set of offloaded characteristics for a service
   2. Only characteristics that are offloaded are included in the list
   3. This API expects a sorted list of attr handles in offloaded service
   4. This API can be used for enabling / modifying an offloaded service
   5. Characteristic descriptors can not be offloaded
   6. Separate GATT registrtaion is expected for each offloaded app   
   7. When a new service is added, offloadId is set to GATT_OFFLOAD_ID_INVALID.
      Microstack generates offloadId and is returned in CFM. 
      Same offloadId is used for updating/disabling offloaded service.
   8. Application is expected to complete any pending operations before
      disabling offloaded characteristics
   9. If offload framework successfully configures Microstack and not offload app, 
      then Offload framework is expected to disable the service
   10.Application must respect the specified properties, if the application sends
      a request for an operation that the attribute does not support, offload 
      manager is expected to return an error status in the response.

   App will receive GATT_OFFLOAD_ENABLE_SERVICE_CFM message as a confirmation.

   Parameters:
   CsrBtGattId gattId      - Application identifier. CFM is sent to this gatt id.
   CsrUint32   context     - Same context sent back in CFM
   CsrBtConnId btConnId    - Connection identifier
   GattOffloadService * service - An allocated pointer of the offloaded service
   -------------------------------------------------------------------- */
void GattOffloadEnableServiceReqSend(CsrBtGattId gattId, 
                                     CsrUint32 context,
                                     CsrBtConnId btConnId,
                                     GattOffloadService * service);

/* --------------------------------------------------------------------
   Name:
   GattOffloadDisableServiceReqSend

   Description:

   This request shall be used for disabling offloaded GATT service.

   App will receive GATT_OFFLOAD_DISABLE_SERVICE_CFM message as a confirmation.

   NOTE :
   1. Application is expected to complete any pending operations before
      disabling a service
   2. Application can issue one or more offloadedIds in the same REQ
   3. Application can set numOffloadIds to 0 to disable all offloaded Ids for 
      the btConnId.

   Parameters:
   CsrBtGattId   gattId    - Application identifier. CFM is sent to this gatt id.
   CsrUint32     context   - Same context sent back in CFM
   CsrBtConnId   btConnId  - Connection identifier
   CsrUint16     numOffloadIds - Num of offloaded ids
   GattOffloadId offloadId[]   - List of offloaded Ids
   -------------------------------------------------------------------- */
void GattOffloadDisableServiceReqSend(CsrBtGattId gattId,
                                      CsrUint32 context,
                                      CsrBtConnId btConnId,
                                      CsrUint16 numOffloadIds,
                                      GattOffloadId offloadId[]);
/* --------------------------------------------------------------------
   Name:
   GattOffloadGetCapabilities

   Description:

   This API shall be used for getting GATT offload capabilities.
   
   Returns:
     This returns GATT_OFFLOAD_RESULT_SUCCESS on success. 
     returns GATT_OFFLOAD_RESULT_INVALID_PARAMETER for invalid params.
   -------------------------------------------------------------------- */
CsrBtResultCode GattOffloadGetCapabilities(GattOffloadCapabilities *cap);

#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_LIB_H__ */
