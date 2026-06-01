#ifndef _CSR_BT_GATT_MAIN_H_
#define _CSR_BT_GATT_MAIN_H_
/******************************************************************************
 Copyright (c) 2010-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #12 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_sched.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_message_queue.h"
#include "csr_log_text_2.h"
#include "csr_bt_util.h"
#include "csr_bt_tasks.h"
#include "att_prim.h"
#include "attlib.h"
#include "l2cap_prim.h"
#include "l2caplib.h"
#include "csr_bt_gatt_lib.h"


#include "csr_list.h"


#ifdef __cplusplus
extern "C" {
#endif



CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtGattLto);
/* Log suborigins */
#define CSR_BT_GATT_LTSO_GENERAL                    (0)
#define CSR_BT_GATT_LTSO_MSG_QUEUE                  (1)

#define CSR_BT_GATT_BD_EDR_CONNECT_BT_CONN_ID       ((CsrBtConnId)(0x00020000 | ATT_CID_LOCAL))
#define CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK           0xFFFF0000u 

/* When LE GAP Procedures are with synergy GATT, connection ID's are uniquely identified for peer device by
   Reading L2CAP CID values saved in least significanct 4 nibbles in connection ID.
   With GAP less GATT, connection ID's are uniquely identified for peer device based on generated connection IDs
   Saved in most significant 3 nibbles in connection ID */
#define CSR_BT_GATT_BT_CONN_ID_QUEUE_MASK            CSR_BT_GATT_BT_CONN_ID_APP_ID_MASK

#define CSR_BT_GATT_BT_CONN_ID_APP_ID_SHIFT_VALUE    (20)

#ifndef CSR_BT_GATT_WHITELIST_SIZE_MAX
#define CSR_BT_GATT_WHITELIST_SIZE_MAX               0x0004
#endif

/* Special handle values */
#define CSR_BT_GATT_ATTR_HANDLE_START                           ((CsrBtGattHandle) 0x0001)

#ifndef CSR_TARGET_PRODUCT_VM
/* NOTE : Update these values if there is any change in GATT/GAP database */
#ifndef CSR_BT_GATT_CACHING
#define HANDLE_DEVICE_NAME                                      ((CsrBtGattHandle)(CSR_BT_GATT_ATTR_HANDLE_START + 2))
#define HANDLE_GAP_PREFERRED_CONN_PARAMS_END                    ((CsrBtGattHandle)(CSR_BT_GATT_ATTR_HANDLE_START + 0x08))

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
#define HANDLE_GATT_SERVICE_CHANGED                             ((CsrBtGattHandle)(CSR_BT_GATT_ATTR_HANDLE_START + 17))
#define HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG               ((CsrBtGattHandle)(HANDLE_GATT_SERVICE_CHANGED + 1))
#ifndef CSR_BT_GATT_INSTALL_EATT
#define CSR_BT_GATT_ATTR_HANDLE_END                             ((CsrBtGattHandle)(HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG))
#else  /* !CSR_BT_GATT_INSTALL_EATT */
#define HANDLE_GATT_CLIENT_SUPPORTED_FEATURES                   ((CsrBtGattHandle)(HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG + 2))
#define CSR_BT_GATT_ATTR_HANDLE_END                             ((CsrBtGattHandle)(HANDLE_GATT_CLIENT_SUPPORTED_FEATURES))
#endif /* CSR_BT_GATT_INSTALL_EATT */
#else  /* !CSR_BT_GATT_INSTALL_FLAT_DB */
/* TBD : update this if required */
#define HANDLE_GATT_SERVICE_CHANGED                             ((CsrBtGattHandle)(0x0003))
#define HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG               ((CsrBtGattHandle)(0x0004))
#define HANDLE_GATT_CLIENT_SUPPORTED_FEATURES                   ((CsrBtGattHandle)(0x0006))
#define CSR_BT_GATT_ATTR_HANDLE_END                             ((CsrBtGattHandle)(0x0006))
#endif /* CSR_BT_GATT_INSTALL_FLAT_DB */
#else  /* !CSR_BT_GATT_CACHING */
#define HANDLE_GATT_SERVICE                                     ((CsrBtGattHandle)(0x0001))
#define HANDLE_GATT_SERVICE_CHANGED                             ((CsrBtGattHandle)(0x0003))
#define HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG               ((CsrBtGattHandle)(0x0004))
#define HANDLE_GATT_CLIENT_SUPPORTED_FEATURES                   ((CsrBtGattHandle)(0x0006))
#define HANDLE_GAP_SERVICE                                      ((CsrBtGattHandle)(0x0009))
#define HANDLE_DEVICE_NAME                                      ((CsrBtGattHandle)(HANDLE_GAP_SERVICE + 2))
#define HANDLE_GAP_PREFERRED_CONN_PARAMS_END                    ((CsrBtGattHandle)(HANDLE_GAP_SERVICE + 0x06))

#ifdef CSR_BT_GATT_INSTALL_FLAT_DB
#define CSR_BT_GATT_ATTR_HANDLE_END                             ((CsrBtGattHandle)8)
#else  /* CSR_BT_GATT_INSTALL_FLAT_DB */
#ifdef INSTALL_GATT_SECURITY_LEVELS
#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
#define CSR_BT_GATT_ATTR_HANDLE_END                             ((CsrBtGattHandle)26)
#else
#define CSR_BT_GATT_ATTR_HANDLE_END                             ((CsrBtGattHandle)23)
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */
#else /* INSTALL_GATT_SECURITY_LEVELS */
#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
#define CSR_BT_GATT_ATTR_HANDLE_END                             ((CsrBtGattHandle)24)
#else
#define CSR_BT_GATT_ATTR_HANDLE_END                             ((CsrBtGattHandle)21)
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */
#endif /* INSTALL_GATT_SECURITY_LEVELS */
#endif /* !CSR_BT_GATT_INSTALL_FLAT_DB */
#endif /* CSR_BT_GATT_CACHING */

#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT

#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA

#define GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE_OFFSET(localLeFeatures)        (CSR_BT_LE_LOCAL_FEATURE_SUPPORTED(localLeFeatures, CSR_BT_LE_FEATURE_LL_PRIVACY) ? 4 : 0)

#else 

#define GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE_OFFSET(localLeFeatures)        (CSR_BT_LE_LOCAL_FEATURE_SUPPORTED(localLeFeatures, CSR_BT_LE_FEATURE_LL_PRIVACY) ? 2 : 0)

#endif /* CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA */
#else

#define GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE_OFFSET(localLeFeatures)        0

#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT1 */

#define GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE(localLeFeatures)               (HANDLE_GAP_PREFERRED_CONN_PARAMS_END + 0x0002 + GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE_OFFSET(localLeFeatures))
#define GATT_ENCRYPTED_DATA_KEY_MATERIAL_CLIENT_CONFIG_HANDLE(localLeFeatures) (GATT_ENCRYPTED_DATA_KEY_MATERIAL_HANDLE(localLeFeatures) +0x0001)
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */

#else  /* !CSR_TARGET_PRODUCT_VM */
/* Note : In gatt_server_db.dbi file, GATT_SERVER_SUPPORTED_FEATURES should
 *        be added as the last characteristics.
 *        If this characteristics is removed, this has to be updated accordingly */
#define CSR_BT_GATT_ATTR_HANDLE_END    HANDLE_GATT_SERVER_SUPPORTED_FEATURES

#endif /* CSR_TARGET_PRODUCT_VM */

#if defined(CSR_BT_GATT_CACHING) || defined(GATT_CACHING_CLIENT_ROLE)
#define GATT_ROBUST_CACHING_FB 0x0
#define CSR_BT_GATT_CSF_ROBUST_CACHING_ENABLE  1
#endif

#ifdef CSR_BT_GATT_INSTALL_EATT
#define GATT_EATT_SUPPORT_FB 0x1
#define GATT_MULT_HANDLE_VALUE_NTF_FB 0x2
#define CSR_BT_GATT_CSF_EATT_ENABLE 0x2
#define CSR_BT_GATT_CSF_MULTI_HANDLE_VALUE_NTF_ENABLE 0x4
#endif /* CSR_BT_GATT_INSTALL_EATT */

/* Different ATT Offset defines */ 
#define CSR_BT_GATT_ATT_READ_HEADER_LENGTH          (1)
#define CSR_BT_GATT_ATT_READ_BLOB_HEADER_LENGTH     (1)
#define CSR_BT_GATT_ATT_READ_BY_TYPE_HEADER_LENGTH  (4)
#define CSR_BT_GATT_ATT_WRITE_HEADER_LENGTH         (3)
#define CSR_BT_GATT_ATT_PREPARE_WRITE_HEADER_LENGTH (5)
#define CSR_BT_GATT_ATT_SIGNED_WRITE_HEADER_LENGTH (15)
#define CSR_BT_GATT_ATT_NOTIFICATION_HEADER_LENGTH  (3)

/* Spec defined MTUs */
#define CSR_BT_ATT_MTU_MAX                          (CSR_BT_GATT_ATTR_VALUE_LEN_MAX + 5)
#define CSR_BT_ATT_MTU_DEFAULT                      (23)

/* Local DB operations MTU */
#define CSR_BT_GATT_LOCAL_MAX_MTU                   (0xFFFF)

/* Define msg queue states */
#define CSR_BT_GATT_MSG_QUEUE_IDLE                          (0x00)
#define CSR_BT_GATT_MSG_QUEUE_QUEUED                        (0x01)
#define CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS                   (0x02)
#define CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_ACK               (0x03)
#define CSR_BT_GATT_MSG_QUEUE_CANCELLED                     (0x04)
#define CSR_BT_GATT_MSG_QUEUE_IGNORE_CHARAC_DESCRIPTOR      (0x05)
#define CSR_BT_GATT_MSG_QUEUE_EXECUTE_WRITE_CANCEL          (0x06)
#define CSR_BT_GATT_MSG_QUEUE_IN_PROGRESS_SECURITY          (0x07)
#define CSR_BT_GATT_MSG_QUEUE_RETRY                         (0x08)

#ifndef CSR_BT_GATT_MSG_RETRY_TIMER_VALUE
#define CSR_BT_GATT_MSG_RETRY_TIMER_VALUE                   (300) /* In ms */
#endif

/* Define invalid conn param update identifier  */
#define CSR_BT_GATT_CONN_PARAM_UPDATE_IDENTIFIER_INVALID  (0x0000)

#define CSR_BT_GATT_ADV_NAMELEN_THRES             (0x05) /* how many bytes required to send partial name */

/* Different attribute length and Indexes */ 
#define CSR_BT_GATT_CHARAC_DECLARATION_MIN_LENGTH       (5)
#define CSR_BT_GATT_CHARAC_DECLARATION_MAX_LENGTH      (19)
#define CSR_BT_GATT_CHARAC_PROPERTIES_LENGTH            (1)
#define CSR_BT_GATT_CHARAC_PROPERTIES_INDEX             (0)
#define CSR_BT_GATT_CHARAC_VALUE_HANDLE_LENGTH          (2) 
#define CSR_BT_GATT_CHARAC_VALUE_HANDLE_FIRST_INDEX     (1)
#define CSR_BT_GATT_CHARAC_UUID_FIRST_INDEX             (3)
#define CSR_BT_GATT_INCLUDE_WITH_UUID_LENGTH            (6)
#define CSR_BT_GATT_INCLUDE_WITHOUT_UUID_LENGTH         (4)
#define CSR_BT_GATT_INCLUDE_128_BIT_LENGTH             (20)
#define CSR_BT_GATT_INCLUDE_START_HANDLE_INDEX          (0)
#define CSR_BT_GATT_INCLUDE_END_HANDLE_INDEX            (2)
#define CSR_BT_GATT_INCLUDE_UUID_INDEX                  (4)  
#define CSR_BT_GATT_CHARAC_PRESENTATION_FORMAT_LENGTH   (7)
#define CSR_BT_GATT_SERVICE_CHANGED_LENGTH              (4)
#define CSR_BT_GATT_CLIENT_CONFIG_VALUE_LENGTH          (2)
#define CSR_BT_GATT_CSF_VALUE_LENGTH                    (1)

/* default CM events to subscribe for */
#ifdef CSR_BT_GATT_INSTALL_EATT
#define CSR_BT_GATT_DEFAULT_CM_EVENT_MASK ((CsrUint32)(CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY | \
                                                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE | \
                                                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED | \
                                                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE | \
                                                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SIMPLE_PAIRING_COMPLETE | \
                                                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE))
#else
#define CSR_BT_GATT_DEFAULT_CM_EVENT_MASK ((CsrUint32)(CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY | \
                                                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE | \
                                                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED | \
                                                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE | \
                                                       CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE))
#endif /* CSR_BT_GATT_INSTALL_EATT */

/* Helper macros */
#define CSR_BT_GATT_GET_QID_MASK                        ((CsrBtGattId) 0x0000FFFF)
#define CSR_BT_GATT_CREATE_GATT_ID(_id, _queue)         ((CsrBtGattId) ((_id << 16) | _queue))
#define CSR_BT_GATT_GET_QID_FROM_GATT_ID(_gattId)       ((CsrUint16) (CSR_BT_GATT_GET_QID_MASK & _gattId))

#define CSR_BT_GATT_GET_HANDLE(source, index)           ((CsrBtGattHandle)(source[index] | source[index + 1] << 8))
#define CSR_BT_GATT_GET_L2CA_METHOD(f)                  ((L2CA_CONNECTION_T)(((f) & L2CA_CONFLAG_ENUM_MASK)>>L2CA_CONFLAG_ENUM_OFFSET))
#define CSR_BT_GATT_GET_CONNINFO(conn)                  ((CsrBtGattConnInfo)(L2CA_CONFLAG_IS_LE((conn)->l2capFlags) ? CSR_BT_GATT_CONNINFO_LE : CSR_BT_GATT_CONNINFO_BREDR))
#define CSR_BT_GATT_GET_SUPPLIER(_result)               ((CsrBtSupplier)((_result == ATT_RESULT_SUCCESS) ? CSR_BT_SUPPLIER_GATT : CSR_BT_SUPPLIER_ATT))
        
#define CSR_BT_GATT_TENTH_OF_A_SECOND                   (CSR_SCHED_SECOND / 10)

#define CSR_BT_GATT_CONN_IS_CONNECTED(st)               TRUE

#define CSR_BT_GATT_ATT_PRIM_UP_COUNT (ATT_ERROR_IND +1- ATT_PRIM_UP)

#define MODE_ATT                       0x00
#define DONT_ALLOCATE_CID              0x00
#define ALLOCATE_CID                   0x01

/* Don't change the ATT bearer number as its single value */
#define ATT_BEARER                     0x01
#define CHANNEL_IS_FREE                0x00
#define CHANNEL_IS_BUSY                0x01
#define CHANNEL_IS_INVALID             0x02
#define SINGLE_ELEMENT                 0x01
#define INVALID_CID                    0xFF

#ifdef CSR_BT_GATT_INSTALL_EATT
/* This macro is used to define transport type while gatt registering to EATT 
   During GATT init Time */
#define ATT_EATT_LE_TRANSPORT_SUPPORT  0x02
/* EATT local defined variable */
#define MODE_EATT                      0x01
/* These macros are defined for maintaining local initiated EATT */
#define LOCAL_EATT_IDLE                0x00
#define LOCAL_EATT_INITIATING          0x01
#define LOCAL_EATT_SUCCESS             0x02

/* Setting the INITIAL CREDITS for all the configuration to be 0x03 */
#define INITIAL_CREDITS                0x03
#define ZERO_PRIORITY                  0x00
#define NO_OF_QUEUE                    0x03
#define BYTE_1                         0x01
#define BYTE_2                         0x02
#define BYTE_3                         0x03
#define BYTE_4                         0x04
#define EATT_BEARER_COUNT              NO_OF_EATT_BEARER
#else
#define EATT_BEARER_COUNT              0x00
#define NO_OF_QUEUE                    0x01
#define NO_OF_EATT_BEARER              0x00
#endif /* CSR_BT_GATT_INSTALL_EATT */
/* Minimum spec defined MTU size for EATT */
#define EATT_MTU_MIN                   0x40

/* There is a default mode will be assigned to App element during App Init
 * By Default All App will get assigned legacy way of handling Long Write/Long Read */
#define CSR_BT_GATT_FEATURE_FLAGS_NONE         0x00

/* Everytime new flag is introduced in csr_bt_gatt_prim.h need to append the same below as well */
#define CSR_BT_GATT_OPERATION_NEW_MODE              (CsrUint32)((CSR_BT_GATT_LONG_WRITE_AS_LIST | \
                                                                  CSR_BT_GATT_LONG_READ_AS_LIST))

struct CsrBtGattQueueElementTag;
typedef struct CsrBtGattQueueElementTag CsrBtGattQueueElement;

struct GattMainInstTag;
typedef struct GattMainInstTag GattMainInst;

typedef void (*CsrBtGattRestoreType)(GattMainInst *gattInst, CsrBtGattQueueElement *qElem, CsrUint16 mtu);
typedef void (*CsrBtGattCancelType)(void *gattInst, void *qElem, CsrBtResultCode result, CsrBtSupplier supplier);
typedef CsrBool (*CsrBtGattSecurityType)(void *gattInst, void *qElem, CsrBtResultCode result, CsrBtSupplier supplier);
typedef void (*CsrBtGattAccessIndRestoreType)(CsrSchedQid phandle, void *msg);


struct CsrBtGattQueueElementTag
{
    struct CsrBtGattQueueElementTag            *next; /* must be 1st */
    struct CsrBtGattQueueElementTag            *prev; /* must be 2nd */
    CsrBtConnId                                 btConnId;
    CsrBtGattId                                 gattId;
    void                                       *gattMsg;
#ifdef CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT
    CsrUint16                                   msgRetryCount;
#endif /* CSR_BT_INSTALL_GATT_CONGESTION_INDICATION_SUPPORT */
    CsrUint8                                    msgState;
    CsrBtGattHandle                             attrHandle;
    CsrUint8                                   *data;
    CsrUint16                                   dataOffset;
    CsrUint16                                   dataElemIndex;
    CsrBtGattRestoreType                        restoreFunc;
    CsrBtGattCancelType                         cancelFunc;
    l2ca_cid_t                                  cid; /* l2cap connection identifier */
    CsrSchedTid                                 txTimer; /* timer for GATT retry operation */
#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
    CsrCmnList_t                                longReadBuffer;
#endif
};

typedef struct CsrBtGattAppElementTag
{
    struct CsrBtGattAppElementTag              *next; /* must be 1st */
    struct CsrBtGattAppElementTag              *prev; /* must be 2nd */
    CsrBtGattId                                gattId;
    /* CsrSchedQid                                qid; */
    CsrUint16                                  start; /* db start handle */
    CsrUint16                                  end; /* db end handle */
    CsrBtGattEventMask                         eventMask; /* Event Mask */
    CsrUint8                                    priority;
    CsrUint8                                    flags; 
    CsrUint32                                   context;
#ifdef GATT_OFFLOAD
    GattOffloadId                               offloadId;
#endif    
} CsrBtGattAppElement;

typedef struct CsrBtGattClientServiceTag
{
    struct CsrBtGattClientServiceTag           *next;
    struct CsrBtGattClientServiceTag           *prev;
    CsrBtGattHandle                             start;/* Start handle of the service */
    CsrBtGattHandle                             end;/* End handle of the service */
    CsrBtGattId                                 gattId; /* application owning this service range */
#ifdef GATT_OFFLOAD
    GattOffloadId                               offloadId;
#endif
} CsrBtGattClientService;


typedef struct CsrBtGattAccessIndQueueElementTag
{
    struct CsrBtGattAccessIndQueueElementTag    *next; /* must be 1st */
    struct CsrBtGattAccessIndQueueElementTag    *prev; /* must be 2nd */
    void                                        *gattMsg;
    CsrUint8                                    msgState;
    CsrBtGattAccessIndRestoreType               restoreFunc;
    l2ca_cid_t                                  cid; /* l2cap connection identifier */
    CsrBtConnId                                 btConnId;
    CsrBtGattId                                 gattId;
}CsrBtGattAccessIndQueueElement;

/* Connection instance. One per ATT connection */
typedef struct CsrBtGattConnElementTag
{
    struct CsrBtGattConnElementTag             *next; /* must be 1st */
    struct CsrBtGattConnElementTag             *prev; /* must be 2nd */
    CsrBtTypedAddr                              peerAddr; /* Peer address */
    l2ca_cid_t                                  cid; /* l2cap connection identifier */
    l2ca_conflags_t                             l2capFlags; /* L2CAP connection flags */
    CsrBtConnId                                 btConnId; /* connection id and CID */
    CsrUint16                                   mtu; /* local+remote mtu */
    CsrBtGattId                                 gattId; /* application owner for this connection */
#ifdef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
    CsrUint16                                   remoteNameLength; /* The length of the remote name */
    CsrUint8                                    *remoteName; /* The remote name */
#endif
#ifndef EXCLUDE_CSR_BT_CM_MODULE
    CsrBtTypedAddr                              idAddr;   /* Peer identity address */
    CsrBool                                     encrypted;  /* Indicates if the link is connected or not */
#endif
#if defined(CSR_BT_GATT_CACHING) || defined (CSR_BT_GATT_INSTALL_EATT) || (!defined(EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL))
    CsrBtGattFeatureInfo                        connFlags;
#endif
    CsrBtResultCode                             reasonCode;
    CsrBtSupplier                               supplier;
#ifdef CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
    CsrCmnList_t                                cliServiceList;/* CsrBtGattClientService */
#endif
    l2ca_cid_t                                  cidSuccess[NO_OF_EATT_BEARER + ATT_BEARER];
    CsrUint8                                    numOfBearer[NO_OF_EATT_BEARER + ATT_BEARER];
    CsrCmnList_t                                accessIndQueue;
#ifdef CSR_BT_GATT_INSTALL_EATT
    CsrBool                                     eattConnection;
    CsrUint16                                   numCidSucess;
    CsrUint8                                    localInitiated;
#endif
    CsrBool                                     leConnection;  /* Indicates if the connection type is LE or BR/EDR */
#ifdef GATT_CACHING_CLIENT_ROLE
    CsrUint8                                    pendingHash;
    CsrUint8                                    serviceChangedIndState;
#endif /* GATT_CACHING_CLIENT_ROLE */
    CsrBtGattHandle                             serviceChangeHandle;
#ifdef GATT_OFFLOAD
    CsrCmnList_t                                offloadSvcList;/* GattOffloadSvcElement */
    CsrUint16                                   aclHandle;
    GattOffloadErrCode                          errorCode;
#endif /* GATT_OFFLOAD */
} CsrBtGattConnElement;

/* Prepare/execute buffer state */
#define CSR_BT_GATT_PREPEXEC_IDLE                       (0x00) /* new element */
#define CSR_BT_GATT_PREPEXEC_PENDING                    (0x01) /* Waiting for an acknowledge from the application */
#define CSR_BT_GATT_PREPEXEC_LONG_WRITE_PENDING         (0x02) 
#define CSR_BT_GATT_PREPEXEC_DONE                       (0x06) /* all done, ready to remove element */

typedef struct CsrBtGattPrepareBufferTag
{
    struct CsrBtGattPrepareBufferTag           *next;      /* must be 1st */
    struct CsrBtGattPrepareBufferTag           *prev;      /* must be 2nd */
    l2ca_cid_t                                 cid;        /* l2cap connection identifier */
    CsrBtGattHandle                            handle;     /* Attribute handle */
    CsrUint16                                  offset;     /* Data offset */
    CsrUint16                                  dataLength; /* Data Length */
    CsrUint8                                   state;      /* CSR_BT_GATT_PREPEXEC_... */   
    CsrUint8                                   *data;      /* Data Pointer */
} CsrBtGattPrepareBuffer;

#ifdef CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
typedef struct CsrBtGattLongAttrReadBufferTag
{
    struct CsrBtGattLongAttrReadBufferTag      *next;      /* must be 1st */
    struct CsrBtGattLongAttrReadBufferTag      *prev;      /* must be 2nd */
    CsrUint16                                  offset;     /* Data offset */
    CsrUint16                                  dataLength; /* Data Length */
    CsrUint8                                   *data;      /* Data Pointer */
} CsrBtGattLongAttrReadBuffer;
#endif

/* Main instance */
struct GattMainInstTag
{
    CsrBtGattId                                 privateGattId;
#ifndef EXCLUDE_CSR_BT_CM_MODULE
    CsrUtf8String                               *localName; /* local device name */
#endif
    void                                        *msg; /* current pending message, pointer */
    CsrCmnList_t                                queue[NO_OF_QUEUE]; /* pending command queue list of type CsrBtGattQueueElement */
    CsrCmnList_t                                appInst; /* application list of type CsrBtGattAppElement */
    CsrCmnList_t                                connInst; /* connection list of type CsrBtGattConnElement */
    CsrCmnList_t                                prepare; /* prepare write buffering */
    CsrUint16                                   gattIdCounter;
    CsrUint8                                    localLeFeatures[8]; /* 64 bits Local LE supported features */
#ifdef CSR_BT_GATT_CACHING
    CsrBool                                     dbInitialised;
#endif
    CsrUint8                                     qid;
#ifdef CSR_TARGET_PRODUCT_VM
    CsrBtGattCallbackFunctionPointer cb;
#endif
#ifdef CSR_BT_INSTALL_GATT_BREDR
    CsrBtGattId                                 bredrAppHandle;
#endif
#ifdef CSR_BT_GATT_INSTALL_EATT
    CsrUint16                                   preferredEattMtu;
#endif /* CSR_BT_GATT_INSTALL_EATT */
    CsrUint8                                    preferredGattBearer;
#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
    CsrUint8*                                   keyMaterial;
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */
};

/* Connection arguments (result code) abstraction for the FSM */
typedef struct
{
    GattMainInst     *inst;
    CsrBtResultCode  result;
    CsrBtSupplier    supplier;
} GattConnArgs;

#ifdef CSR_BT_GLOBAL_INSTANCE
extern GattMainInst gattMainInstance;
#endif /* CSR_BT_GLOBAL_INSTANCE */

extern GattMainInst *gattMainInstPtr;


typedef CsrBool (*CsrBtGattHandlerType)(GattMainInst *inst);
typedef void (*CsrBtGattAttHandlerType)(GattMainInst *inst);

#ifdef __cplusplus
}
#endif

#endif

