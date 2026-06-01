#ifndef CSR_BT_GATT_PRIM_H__
#define CSR_BT_GATT_PRIM_H__

/******************************************************************************
 Copyright (c) 2010-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #7 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_result.h"
#include "csr_bt_profiles.h"
#include "att_prim.h"
#ifndef EXCLUDE_CSR_BT_CM_MODULE
#include "csr_bt_cm_prim.h"
#endif
#include "csr_bt_uuids.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_prim.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* search_string="CsrBtGattPrim" */
/* conversion_rule="UPPERCASE_START_AND_REMOVE_UNDERSCORES" */

/* Basic types */
typedef CsrPrim         CsrBtGattPrim;
typedef CsrUint16       CsrBtGattHandle;

/*
    Connection Identifier definition of "connection_id_t" corresponding
    its definition in adk
  */
typedef CsrUint32       connection_id_t;
typedef connection_id_t ConnectionId;

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
typedef att_attr_t      CsrBtGattDb;
#else
typedef struct CsrBtGattDb_tag
{
    CsrUint16 handle;                    /*!< Attribute handle */
    CsrUint16 perm;                      /*!< Attribute permissions - uses ATT_PERM range */
    CsrUint16 flags;                     /*!< Attribute flags - uses ATT_ATTR range */
    CsrUint32 uuid[4];                   /*!< Attribute UUID */
    CsrUint16 size_value;                /*!< The size of the value */
    CsrUint8 *value;                     /*!< value */

    struct CsrBtGattDb_tag *next;
} CsrBtGattDb;
#endif

typedef CsrUint32       CsrBtGattId;
typedef CsrUint32       CsrBtGattConnFlags;/* No longer used , will likely be removed from future releases */
typedef CsrUint32       CsrBtGattConnInfo;
typedef CsrUint8        CsrBtGattScanFlags;/* No longer used , will likely be removed from future releases */
typedef CsrUint16       CsrBtGattDbAccessRspCode;
typedef CsrUint16       CsrBtGattCliConfigBits;
typedef CsrUint16       CsrBtGattSrvConfigBits;
typedef CsrUint8        CsrBtGattPropertiesBits;
typedef CsrUint16       CsrBtGattExtPropertiesBits;
typedef CsrUint16       CsrBtGattAttrFlags;
typedef CsrUint16       CsrBtGattPermFlags;
typedef CsrUint8        CsrBtGattFormats;
typedef CsrUint8        CsrBtGattPeripheralPrivacyFlag;/* No longer used , will likely be removed from future releases */
typedef CsrUint32       CsrBtGattEventMask;
typedef CsrUint16       CsrBtGattAccessCheck;
typedef CsrUint8        CsrBtGattReportEvent;/* No longer used , will likely be removed from future releases */
typedef CsrUint16       CsrBtGattSecurityFlags;/* No longer used , will likely be removed from future releases */
typedef CsrUint8        CsrBtGattLeRole;
#if defined(CSR_BT_GATT_CACHING) || defined (CSR_BT_GATT_INSTALL_EATT) || (!defined(EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL))
typedef CsrUint16       CsrBtGattFeatureInfo;
#endif
typedef CsrUint16       CsrBtGattConnectBredrResCode;
typedef CsrUint8        GattRemoteDbChangedFlag;


/* Special GATT identifiers */
#define CSR_BT_GATT_INVALID_GATT_ID                             ((CsrBtGattId)0x00000000)
#define CSR_BT_GATT_LOCAL_BT_CONN_ID                            ((CsrBtConnId)(0x00010000 | ATT_CID_LOCAL))

/* For GATT to prefer over their bearers, ATT over EATT or vice-versa */
#define GATT_PREFER_EATT_OVER_ATT   ((CsrUint8) 0x00)
#define GATT_PREFER_ATT_OVER_EATT   ((CsrUint8) 0x01)

/* Special handle values */
#define CSR_BT_GATT_ATTR_HANDLE_INVALID                         ((CsrBtGattHandle) 0x0000)
#define CSR_BT_GATT_ATTR_HANDLE_MAX                             ((CsrBtGattHandle) 0xFFFF)

/* Maximum attribute value length */
#define CSR_BT_GATT_ATTR_VALUE_LEN_MAX                          512

/* Default length of attribute value to read; can be configured upto CSR_BT_GATT_ATTR_VALUE_LEN_MAX.
 * Set its value based on product's requirement and platform's memory availability */
#define CSR_BT_GATT_READ_ATTR_MAX_VALUE                         160

/* Special LE connection parameter values */
#define CSR_BT_GATT_INVALID_CONN_INTERVAL                       ((CsrUint16) 0x0000) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_INVALID_CONN_LATENCY                        ((CsrUint16) 0x0000) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_INVALID_CONN_SUPERVISION_TIMEOUT            ((CsrUint16) 0x0000) /* No longer used , will likely be removed from future releases */

/* Connect flags. Comments show what what flags are valid where. */
#define CSR_BT_GATT_FLAGS_NONE                                  ((CsrBtGattConnFlags) 0x00000000)    /* no special options, use LE radio (all) *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_WHITELIST                             ((CsrBtGattConnFlags) 0x00000001)    /* allow connections from whitelist only (peripheral/central) *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_UNDIRECTED                            ((CsrBtGattConnFlags) 0x00000002)    /* If a connection exists in LE slave role, GATT returns success with connection
                                                                                                       information of existing connection, else starts new un-directed peripheral procedure. *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_ADVERTISE_TIMEOUT                     ((CsrBtGattConnFlags) 0x00000004)    /* undirected advertising times out (peripheral) *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_NONDISCOVERABLE                       ((CsrBtGattConnFlags) 0x00000008)    /* AD flags are non-discoverable (advertise/peripheral) *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_LIMITED_DISCOVERABLE                  ((CsrBtGattConnFlags) 0x00000010)    /* AD flags are limited discoverable (advertise/peripheral) *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_DISABLE_SCAN_RESPONSE                 ((CsrBtGattConnFlags) 0x00000020)    /* disable scan response (advertise) *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_APPEND_DATA                           ((CsrBtGattConnFlags) 0x00000040)    /* append advertise/scan-rsp data (advertise/peripheral) *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_NO_AUTO_SECURITY                      ((CsrBtGattConnFlags) 0x00000080)    /* do not attempt to highten security (central/peripheral) *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_NO_AUTO_SIGN_UPGRADE                  ((CsrBtGattConnFlags) 0x00000100)    /* do not allow sign-to-normal write commands while encrypted *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_WHITELIST_SCANRSP                     ((CsrBtGattConnFlags) 0x00000200)    /* allow scan response to whitelist only (advertise/peripheral)*//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_UNDIRECTED_NEW                        ((CsrBtGattConnFlags) 0x00000402)    /* GATT starts new un-directed peripheral procedure irrespective
                                                                                                        of existing LE connections */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_ATTEMPTED_SECURITY                    ((CsrBtGattConnFlags) 0x40000000)    /* for internal use only */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_FLAGS_CENTRAL_TIMEOUT                       CSR_BT_GATT_FLAGS_ADVERTISE_TIMEOUT /* Central connection times out */ /* No longer used , will likely be removed from future releases */

/* Connection info flags */
#define CSR_BT_GATT_CONNINFO_LE                                 ((CsrBtGattConnInfo)0x000000000) /* connection runs on the LE radio */
#define CSR_BT_GATT_CONNINFO_BREDR                              ((CsrBtGattConnInfo)0x000000001) /* connection runs on the BREDR radio */
/* Le Role */
#define CSR_BT_GATT_LE_ROLE_UNDEFINED                           ((CsrBtGattLeRole)0x00)         /* Using the BREDR radio */
#define CSR_BT_GATT_LE_ROLE_MASTER                              ((CsrBtGattLeRole)0x01)         /* Using the LE radio as Master/Central   */
#define CSR_BT_GATT_LE_ROLE_SLAVE                               ((CsrBtGattLeRole)0x02)         /* Using the LE radio as Slave/Peripheral */

/* Scan mode flags */
#define CSR_BT_GATT_SCAN_STANDARD                               ((CsrBtGattScanFlags)0x00)      /* standard active scan *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_SCAN_PASSIVE                                ((CsrBtGattScanFlags)0x01)      /* standard passive scan *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_SCAN_WHITELIST                              ((CsrBtGattScanFlags)0x02)      /* enable whitelist filtering *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_SCAN_NONDUP                                 ((CsrBtGattScanFlags)0x04)      /* do not attempt to filter duplicates *//* No longer used , will likely be removed from future releases */

/* LE security flags. Note bonding is controlled by SC based on keyDistribution preferences */
#define CSR_BT_GATT_SECURITY_FLAGS_DEFAULT                      ((CsrBtGattSecurityFlags) CSR_BT_SC_LE_USE_DEFAULT) /* Default low energy authentication requirement  *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_SECURITY_FLAGS_UNAUTHENTICATED              ((CsrBtGattSecurityFlags) CSR_BT_SC_LE_SECURITY_ENCRYPTION) /* Encrypt the link without MITM protection *//* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_SECURITY_FLAGS_AUTHENTICATED                ((CsrBtGattSecurityFlags) CSR_BT_SC_LE_SECURITY_MITM) /* Encrypt the link with MITM protection *//* No longer used , will likely be removed from future releases */

#define CSR_BT_APP_PRIORITY_LOW                                 ((CsrUint8)0x00)
#define CSR_BT_APP_PRIORITY_MEDIUM                              ((CsrUint8)0x01)
#define CSR_BT_APP_PRIORITY_HIGH                                ((CsrUint8)0x02)


/* BR/EDR Connect response code */
#define CSR_BT_GATT_BREDR_ACCEPT_CONNECTION                      ((CsrBtGattConnectBredrResCode)0x00)      /* Accept the incoming connection request */
#define CSR_BT_GATT_BREDR_REJECT_CONNECTION                      ((CsrBtGattConnectBredrResCode)0x01)      /* Reject the incoming connection request */

/* Long Write selection flag for long write as list method procedure */
#define CSR_BT_GATT_LONG_WRITE_AS_LIST                           ((CsrUint8)0x01)
/* Long Read selection flag for long read as list method procedure */
#define CSR_BT_GATT_LONG_READ_AS_LIST                            ((CsrUint8)0x02)

/*******************************************************************************
 * Database Access response codes
 *******************************************************************************/

/* Operation was successful */
#define CSR_BT_GATT_ACCESS_RES_SUCCESS                          ((CsrBtGattDbAccessRspCode) ATT_RESULT_SUCCESS)
/* The attr handle given was not valid */
#define CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE                   ((CsrBtGattDbAccessRspCode) ATT_RESULT_INVALID_HANDLE)
/* The attr cannot be read */
#define CSR_BT_GATT_ACCESS_RES_READ_NOT_PERMITTED               ((CsrBtGattDbAccessRspCode) ATT_RESULT_READ_NOT_PERMITTED)
/* The attr cannot be written */
#define CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED              ((CsrBtGattDbAccessRspCode) ATT_RESULT_WRITE_NOT_PERMITTED)
/* The attr PDU was invalid */
#define CSR_BT_GATT_ACCESS_RES_INVALID_PDU                      ((CsrBtGattDbAccessRspCode) ATT_RESULT_INVALID_PDU)
/* The attr requires authentication before it can be read or written */
#define CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_AUTHENTICATION      ((CsrBtGattDbAccessRspCode) ATT_RESULT_INSUFFICIENT_AUTHENTICATION)
/* Target device doesn't support request */
#define CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED            ((CsrBtGattDbAccessRspCode) ATT_RESULT_REQUEST_NOT_SUPPORTED)
/* Offset specified was past the end of the long attribute */
#define CSR_BT_GATT_ACCESS_RES_INVALID_OFFSET                   ((CsrBtGattDbAccessRspCode) ATT_RESULT_INVALID_OFFSET)
/* The attr requires authorisation before it can be read or written */
#define CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_AUTHORISATION       ((CsrBtGattDbAccessRspCode) ATT_RESULT_INSUFFICIENT_AUTHORIZATION)
/* Too many prepare writes have been queued */
#define CSR_BT_GATT_ACCESS_RES_PREPARE_QUEUE_FULL               ((CsrBtGattDbAccessRspCode) ATT_RESULT_PREPARE_QUEUE_FULL)
/* No attr found within the given attribute handle range */
#define CSR_BT_GATT_ACCESS_RES_ATTR_NOT_FOUND                   ((CsrBtGattDbAccessRspCode) ATT_RESULT_ATTR_NOT_FOUND)
/* This attr cannot be read or written using the Read Blob Request or Prepare Write Requests */
#define CSR_BT_GATT_ACCESS_RES_NOT_LONG                         ((CsrBtGattDbAccessRspCode) ATT_RESULT_NOT_LONG)
/* The Encryption Key Size used for encrypting this link is insufficient */
#define CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_ENCR_KEY_SIZE       ((CsrBtGattDbAccessRspCode) ATT_RESULT_INSUFFICIENT_ENCR_KEY_SIZE)
/* The attr value length is invalid for the operation */
#define CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH                   ((CsrBtGattDbAccessRspCode) ATT_RESULT_INVALID_LENGTH)
/* The attr request that was requested has encountered an error that was very unlikely */
#define CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR                   ((CsrBtGattDbAccessRspCode) ATT_RESULT_UNLIKELY_ERROR)
/* The attr requires encryption before it can be read or written */
#define CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_ENCRYPTION          ((CsrBtGattDbAccessRspCode) ATT_RESULT_INSUFFICIENT_ENCRYPTION)
/* The attr type is not a supported grouping attr as defined by a higher layer specification */
#define CSR_BT_GATT_ACCESS_RES_UNSUPPORTED_GROUP_TYPE           ((CsrBtGattDbAccessRspCode) ATT_RESULT_UNSUPPORTED_GROUP_TYPE)
/* Insufficient Resources to complete the request */
#define CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES           ((CsrBtGattDbAccessRspCode) ATT_RESULT_INSUFFICIENT_RESOURCES)
/* Write Request Rejected */
#define CSR_BT_GATT_ACCESS_RES_WRITE_REQUEST_REJECTED           ((CsrBtGattDbAccessRspCode) 0x00FC)
/* Client Characteristic Configuration Descriptor Improperly Configured */
#define CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF    ((CsrBtGattDbAccessRspCode) 0x00FD)
/* A request cannot be serviced because an operation that has been previously triggered is still in progress */
#define CSR_BT_GATT_ACCESS_RES_PROCEDURE_ALREADY_IN_PROGRESS    ((CsrBtGattDbAccessRspCode) 0x00FE)
/* The attribute value is out of range as defined by a Profile or Service specification */
#define CSR_BT_GATT_ACCESS_RES_OUT_OF_RANGE                     ((CsrBtGattDbAccessRspCode) 0x00FF)

/* GATT error codes for the CSR_BT_SUPPLIER_GATT */
#define CSR_BT_GATT_RESULT_SUCCESS                              ((CsrBtResultCode)0x0000) /* Not an error */
#define CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER               ((CsrBtResultCode)0x0001) /* Invalid/unacceptable parameters */
#define CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID                      ((CsrBtResultCode)0x0002) /* Unknown connection id */
#define CSR_BT_GATT_RESULT_ALREADY_ACTIVATED                    ((CsrBtResultCode)0x0003) /* App have called activate req before */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_INTERNAL_ERROR                       ((CsrBtResultCode)0x0004) /* Internal GATT Error */
#define CSR_BT_GATT_RESULT_INSUFFICIENT_NUM_OF_HANDLES          ((CsrBtResultCode)0x0005) /* Insufficient number of free attribute handles */
#define CSR_BT_GATT_RESULT_ATTR_HANDLES_ALREADY_ALLOCATED       ((CsrBtResultCode)0x0006) /* The application have already allocated attribute handles */
#define CSR_BT_GATT_RESULT_CANCELLED                            ((CsrBtResultCode)0x0007) /* Operation cancelled */
#define CSR_BT_GATT_RESULT_SCATTERNET                           ((CsrBtResultCode)0x0008) /* Scatternet not allowed */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_INVALID_LENGTH                       ((CsrBtResultCode)0x0009) /* The attribute value length is invalid for the operation */
#define CSR_BT_GATT_RESULT_RELIABLE_WRITE_VALIDATION_ERROR      ((CsrBtResultCode)0x000A) /* Validation of the written attribute value failed */
#define CSR_BT_GATT_RESULT_INVALID_ATTRIBUTE_VALUE_RECEIVED     ((CsrBtResultCode)0x000B) /* An invalid attribute value is received */
#define CSR_BT_GATT_RESULT_CLIENT_CONFIGURATION_IN_USED         ((CsrBtResultCode)0x000C) /* The application is allready using Client Configuration */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_BR_EDR_NO_PRIMARY_SERVICES_FOUND     ((CsrBtResultCode)0x000D) /* No Primary Services that support BR/EDR were found  */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_INVALID_HANDLE_RANGE                 ((CsrBtResultCode)0x000E) /* The given handle range is invalid */
#define CSR_BT_GATT_RESULT_PARAM_CONN_UPDATE_LOCAL_REJECT       ((CsrBtResultCode)0x000F) /* A local service/application has rejected connection parameter update request */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_WHITE_FILTER_IN_USE                  ((CsrBtResultCode)0x0010) /* The application tries to add/clear its whitelist while using it */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_ALREADY_CONNECTED                    ((CsrBtResultCode)0x0011) /* The application is already connected to the given address */
#define CSR_BT_GATT_RESULT_ALREADY_CONNECTING                   ((CsrBtResultCode)0x0012) /* The application is already connecting to the given address */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_WHITE_LIST_LIMIT_EXCEEDED            ((CsrBtResultCode)0x0013) /* The Procedure fails because the limit of the number of devices on the whitelist is exceeded */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_ALREADY_ADVERTISING                  ((CsrBtResultCode)0x0014) /* The application is already advertising nothing to do */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_WHITE_FILTER_CONTROL_FAILED          ((CsrBtResultCode)0x0015) /* GATT should not get control over the whitelist during the Central Procedure */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_RESULT_BUSY                                 ((CsrBtResultCode)0x0016) /* GATT can not proccess current request. */
#define CSR_BT_GATT_PRIORITY_CHANGE_NOT_ALLOWED                 ((CsrBtResultCode)0x0017) /* APP Priority change is not allowed after EATT connection */
#define CSR_BT_GATT_RESULT_LINK_TRANSFERRED                     ((CsrBtResultCode)0x0018) /* GATT disconnected as transferred to TWS Peer */
#define CSR_BT_GATT_RESULT_CONNECTION_PENDING                   ((CsrBtResultCode)0x0019) /* BR/EDR Connection pending */
#define CSR_BT_GATT_RESULT_CONNECTION_FAILED                    ((CsrBtResultCode)0x001A) /* BR/EDR Connection failed */
/* When there is a limitation in GATT,It will restrict long read to truncated length and return result as CSR_BT_GATT_RESULT_TRUNCATED_DATA */
#define CSR_BT_GATT_RESULT_TRUNCATED_DATA                       ((CsrBtResultCode)0x001B) /* GATT will send truncated code for long read when attribute length is more */

/* Report event types */
#define CSR_BT_GATT_EVENT_CONNECTABLE_UNDIRECTED                ((CsrBtGattReportEvent)HCI_ULP_EV_ADVERT_CONNECTABLE_UNDIRECTED)/* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_EVENT_CONNECTABLE_DIRECTED                  ((CsrBtGattReportEvent)HCI_ULP_EV_ADVERT_CONNECTABLE_DIRECTED)/* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_EVENT_SCANNABLE_UNDIRECTED                  ((CsrBtGattReportEvent)HCI_ULP_EV_ADVERT_DISCOVERABLE)/* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_EVENT_NON_CONNECTABLE                       ((CsrBtGattReportEvent)HCI_ULP_EV_ADVERT_NON_CONNECTABLE)/* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_EVENT_SCAN_RESPONSE                         ((CsrBtGattReportEvent)HCI_ULP_EV_ADVERT_SCAN_RESPONSE)/* No longer used , will likely be removed from future releases */

/* Attribute Permission flags */
#define CSR_BT_GATT_PERM_FLAGS_NONE                             ((CsrBtGattPermFlags) 0x0000)                 /* Not Readable Not Writeable */
#define CSR_BT_GATT_PERM_FLAGS_READ                             ((CsrBtGattPermFlags) ATT_PERM_READ)          /* Permit reads */
#define CSR_BT_GATT_PERM_FLAGS_WRITE_CMD                        ((CsrBtGattPermFlags) ATT_PERM_WRITE_CMD)     /* Permit writes without response */
#define CSR_BT_GATT_PERM_FLAGS_WRITE_REQ                        ((CsrBtGattPermFlags) ATT_PERM_WRITE_REQ)     /* Permit writes with response */
#define CSR_BT_GATT_PERM_FLAGS_WRITE                            ((CsrBtGattPermFlags)(ATT_PERM_WRITE_CMD | ATT_PERM_WRITE_REQ)) /* Permit writes with and without response */
#define CSR_BT_GATT_PERM_FLAGS_AUTH_SIGNED_WRITES               ((CsrBtGattPermFlags)(ATT_PERM_AUTHENTICATED | ATT_PERM_WRITE_CMD)) /* Permit authenticated signed writes */    

/*******************************************************************************
 * Attribute flags
 *******************************************************************************/

/* No attribute flags*/
#define CSR_BT_GATT_ATTR_FLAGS_NONE                             ((CsrBtGattAttrFlags) 0x0000)
/* Attribute length can be changed */
#define CSR_BT_GATT_ATTR_FLAGS_DYNLEN                           ((CsrBtGattAttrFlags) ATT_ATTR_DYNLEN)
/* Read access to the attribute sends CSR_BT_GATT_DB_ACCESS_READ_IND
 * to the application */
#define CSR_BT_GATT_ATTR_FLAGS_IRQ_READ                         ((CsrBtGattAttrFlags) ATT_ATTR_IRQ_R)
/* Write access to the attribute sends CSR_BT_GATT_DB_ACCESS_WRITE_IND
 * to the application */
#define CSR_BT_GATT_ATTR_FLAGS_IRQ_WRITE                        ((CsrBtGattAttrFlags) ATT_ATTR_IRQ_W)
/* Legacy Encrypted link required for read access. */
#define CSR_BT_GATT_ATTR_FLAGS_READ_ENCRYPTION                  ((CsrBtGattAttrFlags) ATT_ATTR_SEC_R_ENUM(ATT_ATTR_SEC_ENCRYPTION))
/* Legacy Authenticated (MITM) link required for read access. */
#define CSR_BT_GATT_ATTR_FLAGS_READ_AUTHENTICATION              ((CsrBtGattAttrFlags) ATT_ATTR_SEC_R_ENUM(ATT_ATTR_SEC_AUTHENTICATION))
/* Secure Connections Authenticated (SC_MITM) link required for read access. */
#define CSR_BT_GATT_ATTR_FLAGS_READ_SC_AUTHENTICATION           ((CsrBtGattAttrFlags) ATT_ATTR_SEC_R_ENUM(ATT_ATTR_SEC_SC_AUTHENTICATION))
/* Legacy Encrypted link required for write access. */
#define CSR_BT_GATT_ATTR_FLAGS_WRITE_ENCRYPTION                 ((CsrBtGattAttrFlags) ATT_ATTR_SEC_W_ENUM(ATT_ATTR_SEC_ENCRYPTION))
/* Legacy Authenticated (MITM) link required for write access. */
#define CSR_BT_GATT_ATTR_FLAGS_WRITE_AUTHENTICATION             ((CsrBtGattAttrFlags) ATT_ATTR_SEC_W_ENUM(ATT_ATTR_SEC_AUTHENTICATION))
/* Secure Connections Authenticated (SC_MITM) link required for write access. */
#define CSR_BT_GATT_ATTR_FLAGS_WRITE_SC_AUTHENTICATION          ((CsrBtGattAttrFlags) ATT_ATTR_SEC_W_ENUM(ATT_ATTR_SEC_SC_AUTHENTICATION))
/* Authorisation (require application access ind/rsp before read/write is allowed).
   I.e. a CSR_BT_GATT_DB_ACCESS_READ_IND or a CSR_BT_GATT_DB_ACCESS_WRITE_IND it sent to the application */
#define CSR_BT_GATT_ATTR_FLAGS_AUTHORISATION                    ((CsrBtGattAttrFlags) ATT_ATTR_AUTHORIZATION)
/* Encryption key size checks required. 
   I.e. a CSR_BT_GATT_DB_ACCESS_READ_IND or a CSR_BT_GATT_DB_ACCESS_WRITE_IND it sent to the application */
#define CSR_BT_GATT_ATTR_FLAGS_ENCR_KEY_SIZE                    ((CsrBtGattAttrFlags) ATT_ATTR_ENC_KEY_REQUIREMENTS)
/* Disable access or LE radio */
#define CSR_BT_GATT_ATTR_FLAGS_DISABLE_LE                       ((CsrBtGattAttrFlags) ATT_ATTR_DISABLE_ACCESS_LE)
/* Disable access on BR/EDR radio */
#define CSR_BT_GATT_ATTR_FLAGS_DISABLE_BREDR                    ((CsrBtGattAttrFlags) ATT_ATTR_DISABLE_ACCESS_BR_EDR)

/*******************************************************************************
 * Characteristic Properties bit fiels definition
 *******************************************************************************/

/* If set, permits broadcasts of the Charac Value using Charac Configuration Descriptor. */
#define CSR_BT_GATT_CHARAC_PROPERTIES_BROADCAST                 ((CsrBtGattPropertiesBits) ATT_PERM_CONFIGURE_BROADCAST)
/* If set, permits reads of the Charac Value. */
#define CSR_BT_GATT_CHARAC_PROPERTIES_READ                      ((CsrBtGattPropertiesBits) ATT_PERM_READ)
/* If set, permit writes of the Charac Value without response. */
#define CSR_BT_GATT_CHARAC_PROPERTIES_WRITE_WITHOUT_RESPONSE    ((CsrBtGattPropertiesBits) ATT_PERM_WRITE_CMD)
/* If set, permits writes of the Charac Value with response. */
#define CSR_BT_GATT_CHARAC_PROPERTIES_WRITE                     ((CsrBtGattPropertiesBits) ATT_PERM_WRITE_REQ)
/* If set, permits notifications of a Charac Value without acknowledgment. */
#define CSR_BT_GATT_CHARAC_PROPERTIES_NOTIFY                    ((CsrBtGattPropertiesBits) ATT_PERM_NOTIFY)
/*If set, permits indications of a Charac Value with acknowledgment. */
#define CSR_BT_GATT_CHARAC_PROPERTIES_INDICATE                  ((CsrBtGattPropertiesBits) ATT_PERM_INDICATE)
/* If set, permits signed writes to the Charac Value. */
#define CSR_BT_GATT_CHARAC_PROPERTIES_AUTH_SIGNED_WRITES        ((CsrBtGattPropertiesBits) ATT_PERM_AUTHENTICATED)
/* If set, additional charac properties are defined in the Charac Ext Properties Descriptor. */
#define CSR_BT_GATT_CHARAC_PROPERTIES_EXTENDED_PROPERTIES       ((CsrBtGattPropertiesBits) ATT_PERM_EXTENDED)

/* Characteristic Extended Properties bit fiels definition. The
 * Characteristic Extended Properties bit field describes additional
 * properties on how the Characteristic Value can be used, or how the
 * characteristic descriptors can be accessed */

/* If set, permits reliable writes of the Charac Value. (0x0001)*/
#define CSR_BT_GATT_CHARAC_EXT_PROPERTIES_RELIABLE_WRITE        ((CsrBtGattExtPropertiesBits) ATT_PERM_RELIABLE_WRITE)

/* If set, permits writes to the characteristic descriptor. (0x0002) */
#define CSR_BT_GATT_CHARAC_EXT_PROPERTIES_WRITE_AUX             ((CsrBtGattExtPropertiesBits) ATT_PERM_WRITE_AUX)

/* Client Characteristic Configuration bit definition */
#define CSR_BT_GATT_CLIENT_CHARAC_CONFIG_DEFAULT                ((CsrBtGattCliConfigBits) 0x0000)
#define CSR_BT_GATT_CLIENT_CHARAC_CONFIG_NOTIFICATION           ((CsrBtGattCliConfigBits) 0x0001)
#define CSR_BT_GATT_CLIENT_CHARAC_CONFIG_INDICATION             ((CsrBtGattCliConfigBits) 0x0002)

/* Server Characteristic Configuration bit definition */
#define CSR_BT_GATT_SERVER_CHARAC_CONFIG_DISABLE                ((CsrBtGattSrvConfigBits) 0x0000)
#define CSR_BT_GATT_SERVER_CHARAC_CONFIG_BROADCAST              ((CsrBtGattSrvConfigBits) 0x0001)

/* Characteristic Format types use in the Characteristic Presentation Format declaration */
#define CSR_BT_GATT_CHARAC_FORMAT_RFU                           ((CsrBtGattFormats) 0x00) /* Reserved for future used */
#define CSR_BT_GATT_CHARAC_FORMAT_BOOLEAN                       ((CsrBtGattFormats) 0x01) /* Unsigned 1-bit, 0 = FALSE, 1 = TRUE */
#define CSR_BT_GATT_CHARAC_FORMAT_2BIT                          ((CsrBtGattFormats) 0x02) /* Unsigned 2-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_NIBBLE                        ((CsrBtGattFormats) 0x03) /* Unsigned 4-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_UINT8                         ((CsrBtGattFormats) 0x04) /* Unsigned 8-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_UINT12                        ((CsrBtGattFormats) 0x05) /* Unsigned 12-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_UINT16                        ((CsrBtGattFormats) 0x06) /* Unsigned 16-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_UINT24                        ((CsrBtGattFormats) 0x07) /* Unsigned 24-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_UINT32                        ((CsrBtGattFormats) 0x08) /* Unsigned 32-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_UINT48                        ((CsrBtGattFormats) 0x09) /* Unsigned 48-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_UINT64                        ((CsrBtGattFormats) 0x0A) /* Unsigned 64-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_UINT128                       ((CsrBtGattFormats) 0x0B) /* Unsigned 128-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_SINT8                         ((CsrBtGattFormats) 0x0C) /* Signed 8-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_SINT12                        ((CsrBtGattFormats) 0x0D) /* Signed 12-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_SINT16                        ((CsrBtGattFormats) 0x0E) /* Signed 16-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_SINT24                        ((CsrBtGattFormats) 0x0F) /* Signed 24-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_SINT32                        ((CsrBtGattFormats) 0x10) /* Signed 32-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_SINT48                        ((CsrBtGattFormats) 0x11) /* Signed 48-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_SINT64                        ((CsrBtGattFormats) 0x12) /* Signed 64-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_SINT128                       ((CsrBtGattFormats) 0x13) /* Signed 128-bit integer */
#define CSR_BT_GATT_CHARAC_FORMAT_FLOAT32                       ((CsrBtGattFormats) 0x14) /* IEEE-754 32-bit floating point */
#define CSR_BT_GATT_CHARAC_FORMAT_FLOAT64                       ((CsrBtGattFormats) 0x15) /* IEEE-754 64-bit floating point */
#define CSR_BT_GATT_CHARAC_FORMAT_SFLOAT                        ((CsrBtGattFormats) 0x16) /* IEEE-11073 16-bit SFLOAT */
#define CSR_BT_GATT_CHARAC_FORMAT_FLOAT                         ((CsrBtGattFormats) 0x17) /* IEEE-11073 32-bit FLOAT */
#define CSR_BT_GATT_CHARAC_FORMAT_DUINT16                       ((CsrBtGattFormats) 0x18) /* IEEE-20601 format */
#define CSR_BT_GATT_CHARAC_FORMAT_UTF8S                         ((CsrBtGattFormats) 0x19) /* UTF8-String */
#define CSR_BT_GATT_CHARAC_FORMAT_UTF16S                        ((CsrBtGattFormats) 0x1A) /* UTF16-String */
#define CSR_BT_GATT_CHARAC_FORMAT_STRUCT                        ((CsrBtGattFormats) 0x1B) /* Opaque structure */

/* Characteristic Peripheral Privacy Flag types use in the Peripheral Privacy Flag Characteristis */
#define CSR_BT_GATT_PERIPHERAL_PRIVACY_DISABLED                 ((CsrBtGattPeripheralPrivacyFlag) 0x00) /* Privacy is disabled */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_PERIPHERAL_PRIVACY_ENABLED                  ((CsrBtGattPeripheralPrivacyFlag) 0x01) /* Privacy is enabled */ /* No longer used , will likely be removed from future releases */

/* Defines the event that the application can subscribe for */
#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_NONE                           ((CsrBtGattEventMask) 0x00000000)
#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_PHYSICAL_LINK_STATUS           ((CsrBtGattEventMask) 0x00000001) /* LE/BR/EDR link status */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_PARAM_CONN_UPDATE_IND          ((CsrBtGattEventMask) 0x00000002) /* Slave initiate LE Conn Param Update */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_RESTART_IND                    ((CsrBtGattEventMask) 0x00000004) /* Ind received if Advertise or Scan cannot be restarted */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_SCAN_RSP_STATUS                ((CsrBtGattEventMask) 0x00000008) /* Ind received when advertise Filter Policy regarding scan response data changes  */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_SERVICE_CHANGED                ((CsrBtGattEventMask) 0x00000010) /* Ind received when the services database on a peer device is changes, i.e. added, removed or modified  */ /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ATT_LE_FIXED_CHANNEL_STATUS    ((CsrBtGattEventMask) 0x00000020) /* ATT fixed channel link status indicated through CsrBtGattConnectInd or CsrBtGattDisconnectInd */
#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_REMOTE_MTU_EXCHANGE_IND        ((CsrBtGattEventMask) 0x00000040) /* Subscribe to obtain the MTU Exchange Indication. 
                                                                                                             Note: This has to be subscribed by only one application */
#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_EATT_LE_CONNECT_CHANNEL_STATUS ((CsrBtGattEventMask) 0x00000080) /* Subscribe to obtain the Eatt connect Indication. 
                                                                                                             Note: This has to be subscribed by only one application */

#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONNECT_BREDR_CHANNEL_STATUS ((CsrBtGattEventMask) 0x00000100) /* Subscribe to obtain the BR/EDR connect Indication.
                                                                                                             Note: This has to be subscribed by only one application */

#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONGESTION_STATUS              ((CsrBtGattEventMask) 0x00000200) /* Subscribe to obtain the congestion status. Congestion event would be sent when stack is not able to 
                                                                                                             send notification or write command out because of congestion. After that, the Congestion clear event 
                                                                                                             will be sent once stack is able to send the notification or write command out.
                                                                                                            */
#define GATT_EVENT_MASK_SUBSCRIBE_DATABASE_SYNC_STATUS                  ((CsrBtGattEventMask) 0x00000400) 

#define GATT_EVENT_MASK_SUBSCRIBE_RESERVED1                             ((CsrBtGattEventMask) 0x00000800)  /* Reserved for internal GATT usage. */

#define GATT_EVENT_MASK_SUBSCRIBE_OFFLOAD_EVENTS                        ((CsrBtGattEventMask) 0x00001000)  /* Applicable only for GATT offload */


#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ALL                    (CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ATT_LE_FIXED_CHANNEL_STATUS | \
                                                                 CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_REMOTE_MTU_EXCHANGE_IND| \
                                                                 CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_EATT_LE_CONNECT_CHANNEL_STATUS | \
                                                                 CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONNECT_BREDR_CHANNEL_STATUS | \
                                                                 CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_CONGESTION_STATUS | \
                                                                 GATT_EVENT_MASK_SUBSCRIBE_DATABASE_SYNC_STATUS | \
                                                                 GATT_EVENT_MASK_SUBSCRIBE_RESERVED1 | \
                                                                 GATT_EVENT_MASK_SUBSCRIBE_OFFLOAD_EVENTS | \
                                                                 CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_NONE)

#define CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_GAP CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ALL /* No longer used , will likely be removed from future releases */
/* Special check requirements for read/write access indications */
#define CSR_BT_GATT_ACCESS_CHECK_NONE                           ((CsrBtGattAccessCheck) 0x0000) /* no auth/strength checks */
#define CSR_BT_GATT_ACCESS_CHECK_AUTHORISATION                  ((CsrBtGattAccessCheck) 0x0001) /* authorisation access check */
#define CSR_BT_GATT_ACCESS_CHECK_ENCR_KEY_SIZE                  ((CsrBtGattAccessCheck) 0x0002) /* encryption key size check */
#define CSR_BT_GATT_ACCESS_CHECK_RELIABLE_WRITE                 ((CsrBtGattAccessCheck) 0x0004) /* reliable write - app shall wait for
                                                                                                 * final 'execute write ok' before committing
                                                                                                 * database changes */

#define CSR_BT_GATT_ACCESS_WRITE_CMD                            ((CsrBtGattAccessCheck) 0x0008) /* Set for write command from remote device
                                                                                                 * Applicable only for GATT offload
                                                                                                 */
                                                                                                 

/* local handle where the preferred connection parameteres are stored in the database */
#define CSR_BT_GATT_ATTR_HANDLE_CONNECTION_PARAMS    ((CsrBtGattHandle)(7))

#define GATT_CCCD_NTF_IND_DISABLE                     0x00
#define GATT_CCCD_NTF_ENABLE                          0x01
#define GATT_CCCD_IND_ENABLE                          0x02

#define GATT_REMOTE_DATABASE_IN_SYNC          ((GattRemoteDbChangedFlag)0x01)
#define GATT_REMOTE_DATABASE_OUT_OF_SYNC      ((GattRemoteDbChangedFlag)0x02)


/*******************************************************************************
 * Advertising/scan-response AD Type values for Low Energy.
 *
 * Note GATT is responsible of setting and inserting the Flags 
 * AD type (CSR_BT_GATT_AD_TYPE_FLAGS) into the Advertising Data.
 * E.g. the application shall never add the Flags AD type to the 
 * Advertising or Scan response Data
 *******************************************************************************/
#define CSR_BT_GATT_AD_TYPE_FLAGS                               (CSR_BT_EIR_DATA_TYPE_FLAGS) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_16BIT_UUID_LIST_INCOMPLETE          (CSR_BT_EIR_DATA_TYPE_MORE_16_BIT_UUID) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_16BIT_UUID_LIST_COMPLETE            (CSR_BT_EIR_DATA_TYPE_COMPLETE_16_BIT_UUID) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_128BIT_UUID_LIST_INCOMPLETE         (CSR_BT_EIR_DATA_TYPE_MORE_128_BIT_UUID) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_128BIT_UUID_LIST_COMPLETE           (CSR_BT_EIR_DATA_TYPE_COMPLETE_128_BIT_UUID) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_LOCAL_NAME_SHORT                    (CSR_BT_EIR_DATA_TYPE_SHORT_LOCAL_NAME) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_LOCAL_NAME_COMPLETE                 (CSR_BT_EIR_DATA_TYPE_COMPLETE_LOCAL_NAME) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_TX_POWER_LEVEL                      (CSR_BT_EIR_DATA_TYPE_TX_POWER) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_SLAVE_CONNECTION_INTERVAL_RANGE     (CSR_BT_EIR_DATA_TYPE_SLAVE_CONN_INTERVAL_RANGE) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_SERVICE_SOLICITATION_16BIT_UUID     (CSR_BT_EIR_DATA_TYPE_SERV_SOLICITATION_16_BIT_UUID) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_SERVICE_SOLICITATION_128BIT_UUID    (CSR_BT_EIR_DATA_TYPE_SERV_SOLICITATION_128_BIT_UUID) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_SERVICE_DATA                        (CSR_BT_EIR_DATA_TYPE_SERVICE_DATA) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_PUBLIC_TARGET_ADDRESS               (CSR_BT_EIR_DATA_TYPE_PUBLIC_TARGET_ADDRESS) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_RANDOM_TARGET_ADDRESS               (CSR_BT_EIR_DATA_TYPE_RANDOM_TARGET_ADDRESS) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_APPEARANCE                          (CSR_BT_EIR_DATA_TYPE_APPEARANCE) /* No longer used , will likely be removed from future releases */
#define CSR_BT_GATT_AD_TYPE_MANUFACTURER_SPECIFIC_DATA          (CSR_BT_EIR_DATA_TYPE_MANUFACTURER_SPECIFIC_DATA) /* No longer used , will likely be removed from future releases */

/* Flat database flags*/
#define GATT_FLAT_DB_NO_FLAGS                    0x0000
#define GATT_FLAT_DB_OVERWRITE                   0x0001

/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/

/* (CsrBtGattPrim) is removed from those prims for which autogen of serializer/dissector is not applicable */

/* Downstream */
#define CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST              (0x0000)

#define CSR_BT_GATT_REGISTER_REQ                        ((CsrBtGattPrim)(0x0000 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_UNREGISTER_REQ                      ((CsrBtGattPrim)(0x0001 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ALLOC_REQ                        ((CsrBtGattPrim)(0x0002 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DB_DEALLOC_REQ                      ((CsrBtGattPrim)(0x0003 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ADD_REQ                          ((CsrBtGattPrim)(0x0004 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DB_REMOVE_REQ                       ((CsrBtGattPrim)(0x0005 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ACCESS_RES                       ((CsrBtGattPrim)(0x0006 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_EVENT_SEND_REQ                      ((CsrBtGattPrim)(0x0007 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DISCOVER_SERVICES_REQ               ((CsrBtGattPrim)(0x0008 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DISCOVER_CHARAC_REQ                 ((CsrBtGattPrim)(0x0009 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_REQ     ((CsrBtGattPrim)(0x000A + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_FIND_INCL_SERVICES_REQ              ((CsrBtGattPrim)(0x000B + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_READ_REQ                            ((CsrBtGattPrim)(0x000C + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_READ_BY_UUID_REQ                    ((CsrBtGattPrim)(0x000D + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_READ_MULTI_REQ                      ((CsrBtGattPrim)(0x000E + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_WRITE_REQ                           ((CsrBtGattPrim)(0x000F + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_CANCEL_REQ                          ((CsrBtGattPrim)(0x0010 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_SET_EVENT_MASK_REQ                  ((CsrBtGattPrim)(0x0011 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_CLIENT_REGISTER_SERVICE_REQ         ((CsrBtGattPrim)(0x0012 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_CLIENT_INDICATION_RSP               ((CsrBtGattPrim)(0x0013 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_FLAT_DB_REGISTER_REQ                                (0x0014 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */
#define CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_REQ   ((CsrBtGattPrim)(0x0015 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_CLIENT_EXCHANGE_MTU_REQ             ((CsrBtGattPrim)(0x0016 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_REMOTE_CLIENT_EXCHANGE_MTU_RES      ((CsrBtGattPrim)(0x0017 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DB_COMMIT_REQ                       ((CsrBtGattPrim)(0x0018 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_APP_PRIORITY_CHANGE_REQ             ((CsrBtGattPrim)(0x0019 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_READ_MULTI_VAR_REQ                  ((CsrBtGattPrim)(0x001A + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_RSP        ((CsrBtGattPrim)(0x001B + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_EATT_DISCONNECT_REQ                 ((CsrBtGattPrim)(0x001C + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_CONNECT_BREDR_REQ                   ((CsrBtGattPrim)(0x001D + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_ACCEPT_BREDR_REQ                    ((CsrBtGattPrim)(0x001E + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_CANCEL_ACCEPT_BREDR_REQ             ((CsrBtGattPrim)(0x001F + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_CONNECT_BREDR_RES                   ((CsrBtGattPrim)(0x0020 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_DISCONNECT_BREDR_REQ                ((CsrBtGattPrim)(0x0021 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_GATT_CONFIG_MODE_REQ                     ((CsrBtGattPrim)(0x0022 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST))
#define GATT_OFFLOAD_CONN_INFO_REQ                                      (0x0023 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */
#define GATT_OFFLOAD_ENABLE_SERVICE_REQ                                 (0x0024 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */
#define GATT_OFFLOAD_DISABLE_SERVICE_REQ                                (0x0025 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */

#define CSR_BT_GATT_PRIM_DOWNSTREAM_HIGHEST                             (0x0025 + CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST)

/* Upstream */
#define CSR_BT_GATT_PRIM_UPSTREAM_LOWEST                                (0x0000 + CSR_PRIM_UPSTREAM)

#define CSR_BT_GATT_REGISTER_CFM                        ((CsrBtGattPrim)(0x0000 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_UNREGISTER_CFM                      ((CsrBtGattPrim)(0x0001 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ALLOC_CFM                        ((CsrBtGattPrim)(0x0002 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DB_DEALLOC_CFM                      ((CsrBtGattPrim)(0x0003 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ADD_CFM                          ((CsrBtGattPrim)(0x0004 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DB_REMOVE_CFM                       ((CsrBtGattPrim)(0x0005 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ACCESS_READ_IND                  ((CsrBtGattPrim)(0x0006 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ACCESS_WRITE_IND                 ((CsrBtGattPrim)(0x0007 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_EVENT_SEND_CFM                      ((CsrBtGattPrim)(0x0008 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))  
#define CSR_BT_GATT_CONNECT_IND                         ((CsrBtGattPrim)(0x0009 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DISCONNECT_IND                      ((CsrBtGattPrim)(0x000A + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_MTU_CHANGED_IND                     ((CsrBtGattPrim)(0x000B + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DISCOVER_SERVICES_IND               ((CsrBtGattPrim)(0x000C + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DISCOVER_SERVICES_CFM               ((CsrBtGattPrim)(0x000D + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DISCOVER_CHARAC_IND                 ((CsrBtGattPrim)(0x000E + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DISCOVER_CHARAC_CFM                 ((CsrBtGattPrim)(0x000F + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND     ((CsrBtGattPrim)(0x0010 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM     ((CsrBtGattPrim)(0x0011 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_FIND_INCL_SERVICES_IND              ((CsrBtGattPrim)(0x0012 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_FIND_INCL_SERVICES_CFM              ((CsrBtGattPrim)(0x0013 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_CFM                            ((CsrBtGattPrim)(0x0014 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_MULTI_CFM                      ((CsrBtGattPrim)(0x0015 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_BY_UUID_IND                    ((CsrBtGattPrim)(0x0016 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_BY_UUID_CFM                    ((CsrBtGattPrim)(0x0017 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_EXTENDED_PROPERTIES_CFM        ((CsrBtGattPrim)(0x0018 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_USER_DESCRIPTION_CFM           ((CsrBtGattPrim)(0x0019 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_CLIENT_CONFIGURATION_CFM       ((CsrBtGattPrim)(0x001A + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_SERVER_CONFIGURATION_CFM       ((CsrBtGattPrim)(0x001B + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_PRESENTATION_FORMAT_CFM        ((CsrBtGattPrim)(0x001C + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_AGGREGATE_FORMAT_CFM           ((CsrBtGattPrim)(0x001D + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_PROFILE_DEFINED_DESCRIPTOR_CFM ((CsrBtGattPrim)(0x001E + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_WRITE_CFM                           ((CsrBtGattPrim)(0x001F + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_SET_EVENT_MASK_CFM                  ((CsrBtGattPrim)(0x0020 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ACCESS_COMPLETE_IND              ((CsrBtGattPrim)(0x0021 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM         ((CsrBtGattPrim)(0x0022 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_CLIENT_INDICATION_IND               ((CsrBtGattPrim)(0x0023 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_CLIENT_NOTIFICATION_IND             ((CsrBtGattPrim)(0x0024 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_FLAT_DB_REGISTER_CFM                ((CsrBtGattPrim)(0x0025 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM   ((CsrBtGattPrim)(0x0026 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_CLIENT_EXCHANGE_MTU_CFM             ((CsrBtGattPrim)(0x0027 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_REMOTE_CLIENT_EXCHANGE_MTU_IND      ((CsrBtGattPrim)(0x0028 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DB_COMMIT_CFM                       ((CsrBtGattPrim)(0x0029 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_APP_PRIORITY_CHANGE_CFM             ((CsrBtGattPrim)(0x002A + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_EATT_CONNECT_IND                    ((CsrBtGattPrim)(0x002B + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_IND        ((CsrBtGattPrim)(0x002C + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_READ_MULTI_VAR_CFM                  ((CsrBtGattPrim)(0x002D + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_CONNECT_BREDR_CFM                   ((CsrBtGattPrim)(0x002E + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_ACCEPT_BREDR_CFM                    ((CsrBtGattPrim)(0x002F + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_CANCEL_ACCEPT_BREDR_CFM             ((CsrBtGattPrim)(0x0030 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_CONNECT_BREDR_IND                   ((CsrBtGattPrim)(0x0031 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_CONFIG_MODE_CFM                     ((CsrBtGattPrim)(0x0032 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_GATT_LONG_READ_CFM                                       (0x0033 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */
#define CSR_BT_GATT_CONGESTION_IND                      ((CsrBtGattPrim)(0x0034 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define GATT_REMOTE_DB_CHANGED_IND                      ((CsrBtGattPrim)(0x0035 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST))
#define GATT_RESERVED1_IND                                              (0x0036 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */
#define GATT_OFFLOAD_CONN_INFO_CFM                                      (0x0037 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */
#define GATT_OFFLOAD_ENABLE_SERVICE_CFM                                 (0x0038 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */
#define GATT_OFFLOAD_DISABLE_SERVICE_CFM                                (0x0039 + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */
#define GATT_OFFLOAD_ERROR_IND                                          (0x003A + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST) /* typecasting is removed for this prim to exclude from serializer generation */


#define CSR_BT_GATT_PRIM_UPSTREAM_HIGHEST               (0x003A + CSR_BT_GATT_PRIM_UPSTREAM_LOWEST)

#define CSR_BT_GATT_PRIM_DOWNSTREAM_COUNT               (CSR_BT_GATT_PRIM_DOWNSTREAM_HIGHEST + 1 - CSR_BT_GATT_PRIM_DOWNSTREAM_LOWEST)
#define CSR_BT_GATT_PRIM_UPSTREAM_COUNT                 (CSR_BT_GATT_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_GATT_PRIM_UPSTREAM_LOWEST)

/*******************************************************************************
 * Common structures
 *******************************************************************************/
typedef struct
{
    CsrBtGattHandle     attrHandle;             /* The handle of the attribute to be written */
    CsrUint16           offset;                 /* Reserved for future used */
    CsrUint16           valueLength;            /* Length of the value */
    CsrUint8           *value;                  /* Pointer to Value */
} CsrBtGattAttrWritePairs;   

typedef struct 
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
} CsrBtGattStdCfm;

typedef CsrBtGattStdCfm CsrBtGattConfigModeCfm;
typedef CsrBtGattStdCfm CsrBtGattDbCommitCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrBtConnId         btConnId;               /* Connection identifier */
} CsrBtGattStdBtConnIdCfm;

/*******************************************************************************
 * Primitive signal type definitions - APPLICATION INTERFACE
 *******************************************************************************/

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtGattConnInfo   connInfo;               /* Connection info flags (radio type etc.) */
    CsrBtTypedAddr      address;                /* Peer address */
    CsrUint16           mtu;                    /* Maximum packet size */
    CsrBtGattLeRole     leRole;                 /* Defines which role the connection has on the LE Radio */   
    l2ca_conflags_t     flags;                  /* L2CAP connection flags*/
} CsrBtGattConnectInd;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtReasonCode     reasonCode;             /* Reason code */
    CsrBtSupplier       reasonSupplier;         /* Reason code supplier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtTypedAddr      address;                /* Peer address */
    CsrBtGattConnInfo   connInfo;               /* Connection info flags (radio type etc.) */
} CsrBtGattDisconnectInd;

/* Covers Registration and Un-register an application instance to Gatt */
typedef struct
{
    CsrBtGattPrim          type;                   /* Identity */
    CsrSchedQid            pHandle;                /* Application handle */
    CsrUint16              context;                /* Value returned in CsrBtGattRegisterCfm */
} CsrBtGattRegisterReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* An application handle provide by GATT */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrUint16           context;                /* Value returned from CsrBtGattRegisterReq */
} CsrBtGattRegisterCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
} CsrBtGattUnregisterReq;

typedef CsrBtGattStdCfm CsrBtGattUnregisterCfm;

/* Covers DataBase Configuration */
typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrUint16           numOfAttrHandles;       /* Number of attribute handles */
    CsrUint16           preferredStartHandle;   /* The StartHandle the application prefers. 0 = no preference */
} CsrBtGattDbAllocReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrBtGattHandle     start;                  /* Start attribute handle */
    CsrBtGattHandle     end;                    /* End attribute handle */
    CsrUint16           preferredStartHandle;   /* The preferredStartHandle given in CsrBtGattDbAllocReq */
} CsrBtGattDbAllocCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
} CsrBtGattDbDeallocReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrBtGattHandle     start;                  /* Start attribute handle */
    CsrBtGattHandle     end;                    /* End attribute handle */
} CsrBtGattDbDeallocCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application handle */
    CsrBtGattDb         *db;                    /* Database */
} CsrBtGattDbAddReq;

typedef CsrBtGattStdCfm CsrBtGattDbAddCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application handle */
    CsrBtGattHandle     start;                  /* Start attribute handle for removal (inclusive) */
    CsrBtGattHandle     end;                    /* End attribute handle for removal (inclusive) */
} CsrBtGattDbRemoveReq;

typedef struct 
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result supplier */
    CsrUint16           numOfAttr;              /* Number of attributes removed */
} CsrBtGattDbRemoveCfm;

/* Covers Flat dataBase Registration */
typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrSchedQid         pHandle;                /* The application handle */
    CsrUint16           flags;                  /* Flags */
    CsrUint16           size;                   /* The size of the database */
    CsrUint16           *db;                    /* Database */
} CsrBtGattFlatDbRegisterReq;

typedef struct 
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
} CsrBtGattFlatDbRegisterCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtGattHandle     start;                  /* First handle in the pre-built GATT database that the server task is interested in */
    CsrBtGattHandle     end;                    /* Last handle in the pre-built GATT database that the server task is interested in */
} CsrBtGattFlatDbRegisterHandleRangeReq;

typedef CsrBtGattStdCfm CsrBtGattFlatDbRegisterHandleRangeCfm;

typedef struct
{
    CsrBtGattPrim           type;               /* Identity */
    CsrBtGattId             gattId;             /* Application handle */
    CsrBtConnId             btConnId;           /* Connection identifier */
    CsrBtGattHandle         attrHandle;         /* The handle of the attribute */
    CsrUint16               offset;             /* The offset of the first octet to be accessed */
    CsrUint16               maxRspValueLength;  /* The maximum length that the value of the attribute must have */
    CsrBtGattAccessCheck    check;              /* Special conditions that needs to be checked */
    CsrBtGattConnInfo       connInfo;           /* Connection info flags (radio type etc.) */
    CsrBtTypedAddr          address;            /* Peer address */
} CsrBtGattDbAccessReadInd;

typedef struct
{
    CsrBtGattPrim           type;               /* Identity */
    CsrBtGattId             gattId;             /* Application handle */
    CsrBtConnId             btConnId;           /* Connection identifier */
    CsrBtGattAccessCheck    check;              /* Special conditions that needs to be checked */
    CsrBtGattConnInfo       connInfo;           /* Connection info flags (radio type etc.) */
    CsrBtTypedAddr          address;            /* Peer address */
    CsrUint16               writeUnitCount;     /* Number of sub-write units in list */
    CsrBtGattAttrWritePairs *writeUnit;         /* Array of sub-write units. Only offset/value/valueLength should be used. */
    CsrBtGattHandle         attrHandle;         /* The handle of the attribute. Return this handle in the AccessRes. */
} CsrBtGattDbAccessWriteInd;

typedef struct
{
    CsrBtGattPrim            type;              /* Identity */
    CsrBtGattId              gattId;            /* Application handle */
    CsrBtConnId              btConnId;          /* Connection identifier */
    CsrBtGattHandle          attrHandle;        /* The handle of the attribute */
    CsrBtGattDbAccessRspCode responseCode;      /* Database Access response code */
    CsrUint16                valueLength;       /* Length of the attribute that has been read */
    CsrUint8                *value;             /* The value of the attribute that has been read*/
} CsrBtGattDbAccessRes;

typedef struct
{
    CsrBtGattPrim           type;               /* Identity: CSR_BT_GATT_DB_ACCESS_COMPLETE_IND */
    CsrBtGattId             gattId;             /* Application handle */
    CsrBtConnId             btConnId;           /* Connection identifier */
    CsrBtGattConnInfo       connInfo;           /* Connection info flags (radio type etc.) */
    CsrBtTypedAddr          address;            /* Peer address */
    CsrBtGattHandle         attrHandle;         /* The handle of the attribute */
    CsrBool                 commit;             /* True only if all prepare writes succedeed */
} CsrBtGattDbAccessCompleteInd;

typedef struct
{
    CsrUint16           offset;                 /* Reserved for future used */
    CsrUint16           valueLength;            /* Length of the value */
    CsrUint8            *value;                 /* Pointer to Value */
} CsrBtGattLongAttrRead;


/* Covers Server Initiated Notification, Indication, and Service Changed */

/* Intern used Event Send Request flags */

/* Send a Notification Event to the Client */
#define CSR_BT_GATT_NOTIFICATION_EVENT      ((CsrUint16) ATT_HANDLE_VALUE_NOTIFICATION) 
/* Send an Indication Event to the Client */
#define CSR_BT_GATT_INDICATION_EVENT        ((CsrUint16) ATT_HANDLE_VALUE_INDICATION)   
/* Received a long Notification/Indication Event from server. Note for intern used only */
#define CSR_BT_GATT_LONG_ATTRIBUTE_EVENT    ((CsrUint16) 0x0000)
/* Send a Service Changed Event to the Client. Note for intern used only */
#define CSR_BT_GATT_SERVICE_CHANGED_EVENT   ((CsrUint16) 0xFFFF)
/* Send Multi Varibale Notification Event to the Event */
#define CSR_BT_GATT_MULTI_VAR_NOTIFICATION_EVENT      ((CsrUint16) 0x0003)

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application handle */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtGattHandle     attrHandle;             /* Attribute Handle */
    CsrBtGattHandle     endGroupHandle;         /* End of Affected Attribute Handle Range, Only used by service Changed*/
    CsrUint16           flags;                  /* Request Flag for internal use only */
    CsrUint16           valueLength;            /* Length of the Value in octects */
    CsrUint8           *value;                  /* The current value of the attribute */
} CsrBtGattEventSendReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrBtConnId         btConnId;               /* Connection identifier */
#ifdef GATT_OFFLOAD    
    CsrBtGattHandle     attrHandle;             /* Handle */
#endif
} CsrBtGattEventSendCfm;

/* Covers item 1, Server Configuration, in the GATT feature table */
typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrUint16           mtu;                    /* MTU for the connection */
} CsrBtGattMtuChangedInd;

/* Covers item 2, Primary Service Discovery, in the GATT feature table */
typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtUuid           uuid;                   /* Service UUID, if CSR_BT_GATT_UUID_PRIMARY_SERVICE_DECL all primary services are found */
} CsrBtGattDiscoverServicesReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtGattHandle     startHandle;            /* Start handle */
    CsrBtGattHandle     endHandle;              /* The End Group handle */
    CsrBtUuid           uuid;                   /* The Service UUID */
} CsrBtGattDiscoverServicesInd;

typedef CsrBtGattStdBtConnIdCfm CsrBtGattDiscoverServicesCfm;

/* Covers item 3, Relationship Discovery, in the GATT feature table */
typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtGattHandle     startHandle;            /* Starting handle of the specified service */
    CsrBtGattHandle     endGroupHandle;         /* Ending handle of the specified service */
} CsrBtGattFindInclServicesReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* connection identifier */
    CsrBtGattHandle     attrHandle;             /* Attribute Handle of the Include Service */
    CsrBtGattHandle     startHandle;            /* Starting Handle of the Included Service declaration */
    CsrBtGattHandle     endGroupHandle;         /* The End Group handle */
    CsrBtUuid           uuid;                   /* The Service UUID */
} CsrBtGattFindInclServicesInd;

typedef CsrBtGattStdBtConnIdCfm CsrBtGattFindInclServicesCfm;

/* Covers item 4, Characteristic Discovery, in the GATT feature table */
typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtUuid           uuid;                   /* Characteristic UUID. If CSR_BT_GATT_UUID_CHARACTERISTIC_DECL all characs of a service are found */
    CsrBtGattHandle     startHandle;            /* Starting handle of the specified service */
    CsrBtGattHandle     endGroupHandle;         /* End group handle of the specified service */
} CsrBtGattDiscoverCharacReq;

typedef struct
{
    CsrBtGattPrim           type;               /* Identity */
    CsrBtGattId             gattId;             /* Application identifier */
    CsrBtConnId             btConnId;           /* Connection identifier */
    CsrBtGattHandle         declarationHandle;  /* Handle for the characteristic declaration*/
    CsrBtGattPropertiesBits property;           /* Characteristic Property */
    CsrBtUuid               uuid;               /* Characteristic UUID */
    CsrBtGattHandle         valueHandle;        /* Characteristic Value Handle */
} CsrBtGattDiscoverCharacInd;

typedef CsrBtGattStdBtConnIdCfm CsrBtGattDiscoverCharacCfm;

/* Covers item 5, Characteristic Descriptor Discovery, in the GATT feature table */
typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtGattHandle     startHandle;            /* Starting handle of the specified characteristic value + 1 */
    CsrBtGattHandle     endGroupHandle;         /* End Group handle of the specified characteristic */
} CsrBtGattDiscoverCharacDescriptorsReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtUuid           uuid;                   /* Characteristic Descriptor UUID */
    CsrBtGattHandle     descriptorHandle;       /* The handle of the Characteristic Descriptor declaration */
} CsrBtGattDiscoverCharacDescriptorsInd;

typedef CsrBtGattStdBtConnIdCfm CsrBtGattDiscoverCharacDescriptorsCfm;

/* Covers item 6, Characteristic Value Read, and item 10 Characteristic Descriptor Value Read 
   in the GATT feature table */

/* Intern used Read Request flags */
#define CSR_BT_GATT_READ_VALUE                 ((CsrUint16) 0x0000) /* Reads the Characteristic Value declaration */ 
#define CSR_BT_GATT_READ_EXT_PROPERTIES        ((CsrUint16) 0x0001) /* Reads the Characteristic Extended Properties declaration */
#define CSR_BT_GATT_READ_USER_DESCRIPTOR       ((CsrUint16) 0x0002) /* Reads the Characteristic User Description declaration */
#define CSR_BT_GATT_READ_CLIENT_CONFIGURATION  ((CsrUint16) 0x0003) /* Reads the Client Characteristic Configuration declaration */
#define CSR_BT_GATT_READ_SERVER_CONFIGURATION  ((CsrUint16) 0x0004) /* Reads the Server Characteristic Configuration declaration */
#define CSR_BT_GATT_READ_PRESENTATION_FORMAT   ((CsrUint16) 0x0005) /* Reads the Characteristic Presentation Format declaration */
#define CSR_BT_GATT_READ_AGGREGATE_FORMAT      ((CsrUint16) 0x0006) /* Reads the Characteristic Aggregate Format declaration */
#define CSR_BT_GATT_READ_PROFILE_DEFINED       ((CsrUint16) 0x0007) /* Reads a Profile Defined Characteristic declaration */

typedef struct
{
    CsrBtGattPrim       type;               /* Identity */
    CsrBtGattId         gattId;             /* Application identifier */
    CsrBtConnId         btConnId;           /* Connection identifier */
    CsrBtGattHandle     handle;             /* Characteristic Value Handle */
    CsrUint16           offset;             /* The offset of the first octet that shall be read */
    CsrUint16           flags;              /* For intern use only */
} CsrBtGattReadReq;

typedef struct
{
    CsrBtGattPrim       type;               /* Identity */
    CsrBtGattId         gattId;             /* Application identifier */
    CsrBtResultCode     resultCode;         /* Result code */
    CsrBtSupplier       resultSupplier;     /* Result error code supplier */
    CsrBtConnId         btConnId;           /* Connection identifier */
    CsrUint16           valueLength;        /* Length of the Characteristic Value in octects */
    CsrUint8           *value;              /* Pointer to the Characteristic Value */
    CsrBtGattHandle     handle;             /* Attribute Handle */
} CsrBtGattReadCfm;

typedef struct
{
    CsrBtGattPrim         type;               /* Identity */
    CsrBtGattId           gattId;             /* Application identifier */
    CsrBtResultCode       resultCode;         /* Result code */
    CsrBtSupplier         resultSupplier;     /* Result error code supplier */
    CsrBtConnId           btConnId;           /* Connection identifier */
    CsrUint16             readUnitCount;      /* Number of sub-read units in list */
    CsrBtGattLongAttrRead *readUnit;          /* Array of sub-read units. Only offset/value/valueLength should be used. */
    CsrBtGattHandle       handle;             /* Attribute Handle */
} CsrBtGattLongReadCfm;


typedef struct
{
    CsrBtGattPrim               type;           /* Identity */
    CsrBtGattId                 gattId;         /* Application identifier */
    CsrBtResultCode             resultCode;     /* Result code */
    CsrBtSupplier               resultSupplier; /* Result error code supplier */
    CsrBtConnId                 btConnId;       /* Connection identifier */
    CsrBtGattExtPropertiesBits  extProperties;  /* The Characteristic Extended Properties bit field */
    CsrBtGattHandle             handle;         /* Extended properties descriptor handle */
} CsrBtGattReadExtendedPropertiesCfm;

typedef struct
{
    CsrBtGattPrim           type;           /* Identity */
    CsrBtGattId             gattId;         /* Application identifier */
    CsrBtResultCode         resultCode;     /* Result code */
    CsrBtSupplier           resultSupplier; /* Result error code supplier */
    CsrBtConnId             btConnId;       /* Connection identifier */
    CsrUtf8String           *usrDescription;/* Characteristic User Description UTF-8 String */
    CsrBtGattHandle         handle;         /* User description descriptor handle */
} CsrBtGattReadUserDescriptionCfm;

typedef struct
{
    CsrBtGattPrim           type;           /* Identity */
    CsrBtGattId             gattId;         /* Application identifier */
    CsrBtResultCode         resultCode;     /* Result code */
    CsrBtSupplier           resultSupplier; /* Result error code supplier */
    CsrBtConnId             btConnId;       /* Connection identifier */
    CsrBtGattCliConfigBits  configuration;  /* Client Characteristic Configuration bits */
    CsrBtGattHandle         handle;         /* Client characteristic configuration descriptor handle */
} CsrBtGattReadClientConfigurationCfm;

typedef struct
{
    CsrBtGattPrim           type;           /* Identity */
    CsrBtGattId             gattId;         /* Application identifier */
    CsrBtResultCode         resultCode;     /* Result code */
    CsrBtSupplier           resultSupplier; /* Result error code supplier */
    CsrBtConnId             btConnId;       /* Connection identifier */
    CsrBtGattSrvConfigBits  configuration;  /* Server Characteristic Configuration bits */
    CsrBtGattHandle         handle;         /* Server characteristic configuration descriptor handle */
} CsrBtGattReadServerConfigurationCfm;

typedef struct
{
    CsrBtGattPrim           type;           /* Identity */
    CsrBtGattId             gattId;         /* Application identifier */
    CsrBtResultCode         resultCode;     /* Result code */
    CsrBtSupplier           resultSupplier; /* Result error code supplier */
    CsrBtConnId             btConnId;       /* Connection identifier */
    CsrBtGattFormats        format;         /* Format of the value of this characteristic */
    CsrUint8                exponent;       /* Exponent field determines how the value is formatted */
    CsrUint16               unit;           /* The Unit is a UUID defined in the Assigned Numbers Specification */
    CsrUint8                nameSpace;      /* The Name Space field identify defined in the Assigned Numbers Specification */
    CsrUint16               description;    /* The Description is an enumerated value as defined in the Assigned Numbers Specification */
    CsrBtGattHandle         handle;         /* Presentation format descriptor handle */
} CsrBtGattReadPresentationFormatCfm;

typedef struct
{
    CsrBtGattPrim           type;           /* Identity */
    CsrBtGattId             gattId;         /* Application identifier */
    CsrBtResultCode         resultCode;     /* Result code */
    CsrBtSupplier           resultSupplier; /* Result error code supplier */
    CsrBtConnId             btConnId;       /* Connection identifier */
    CsrUint16               handlesCount;   /* Num of attribute Handles */
    CsrBtGattHandle         *handles;       /* List of Attribute Handles for the Characteristic Presentation Format Declarations */
    CsrBtGattHandle         handle;         /* Aggregate presentation format descriptor handle */
} CsrBtGattReadAggregateFormatCfm;

typedef CsrBtGattReadCfm CsrBtGattReadProfileDefinedDescriptorCfm;

typedef struct
{
    CsrBtGattPrim       type;               /* Identity */
    CsrBtGattId         gattId;             /* Application identifier */
    CsrBtConnId         btConnId;           /* Connection identifier */
    CsrUint16           handlesCount;       /* Num of attribute Handles */
    CsrBtGattHandle     *handles;           /* A set of two or more attribute handles. First handle is returned in CsrBtGattReadMultiCfm */
} CsrBtGattReadMultiReq;

typedef CsrBtGattReadCfm CsrBtGattReadMultiCfm;

typedef struct
{
    CsrBtGattPrim       type;               /* Identity */
    CsrBtGattId         gattId;             /* Application identifier */
    CsrBtConnId         btConnId;           /* Connection identifier */
    CsrBtGattHandle     startHandle;        /* Starting handle from where the read shall start */
    CsrBtGattHandle     endGroupHandle;     /* Ending handle of where the read shall end */
    CsrBtUuid           uuid;               /* Characteristic UUID */
} CsrBtGattReadByUuidReq;

typedef struct
{
    CsrBtGattPrim       type;               /* Identity */
    CsrBtGattId         gattId;             /* Application identifier */
    CsrBtConnId         btConnId;           /* Connection identifier */
    CsrBtGattHandle     valueHandle;        /* Characteristic Value Handle */
    CsrUint16           valueLength;        /* Length of the Characteristic Value in octets */
    CsrUint8           *value;              /* Pointer to the Characteristic Value */
} CsrBtGattReadByUuidInd;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtUuid           uuid;                   /* Characteristic UUID */
} CsrBtGattReadByUuidCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBool             congested;              /* Congestion status*/
    CsrBtGattHandle     attrHandle;
} CsrBtGattCongestionInd;

typedef struct
{
    CsrBtGattPrim                    type;                             /* Identity */
    CsrBtGattId                      gattId;                           /* Application identifier */
    CsrBtConnId                      btConnId;                         /* Connection identifier */
    CsrBtGattHandle                  serviceChangeStartHandle;         /* Start handle of service changed */
    CsrBtGattHandle                  serviceChangeEndHandle;           /* End handle of service changed */
    GattRemoteDbChangedFlag          indType;                          /* Ind type*/
} GattRemoteDbChangedInd;
/* Covers item 7 Characteristic Value Write and item 11 Characteristic Descriptor Value Write, 
   in the GATT feature table */

/* Intern used Write Request flags */

/* Send Write Request to the server */
#define CSR_BT_GATT_WRITE_REQUEST        ((CsrUint16) ATT_WRITE_REQUEST) 
/* Send Write Command to the server */
#define CSR_BT_GATT_WRITE_COMMAND        ((CsrUint16) ATT_WRITE_COMMAND) 
/* Send Signed Write Command to the server, can only be sent to bonded server */
#define CSR_BT_GATT_WRITE_SIGNED_COMMAND ((CsrUint16) (ATT_WRITE_COMMAND | ATT_WRITE_SIGNED))  
/* Send Reliable Write to the server*/
#define CSR_BT_GATT_WRITE_RELIABLE       ((CsrUint16) 0xFFFF)            

typedef struct 
{
    CsrBtGattPrim           type;               /* Identity */
    CsrBtGattId             gattId;             /* Application identifier */
    CsrBtConnId             btConnId;           /* Connection identifier */
    CsrUint16               flags;              /* For intern use only */
    CsrUint16               attrWritePairsCount;/* Num of attribute to be written */ 
    CsrBtGattAttrWritePairs *attrWritePairs;    /* Handle, value, offset of the attributes that must be written*/  
} CsrBtGattWriteReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtGattHandle     handle;                 /* Attribute Handle. CSR_BT_GATT_ATTR_HANDLE_INVALID in case of reliable write */
} CsrBtGattWriteCfm;

/* Allow the application to cancel a given procedure, Note the confirm
 * message is the confirm message of the procedure being cancelled. */
typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
} CsrBtGattCancelReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtGattEventMask  eventMask;              /* Defines the event(s) to subscribe for */
} CsrBtGattSetEventMaskReq;

typedef CsrBtGattStdCfm CsrBtGattSetEventMaskCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtGattHandle     start;                  /* Start handle of the service */
    CsrBtGattHandle     end;                    /* End handle of the service */
    CsrBtTypedAddr      address;                /* Peer address */
} CsrBtGattClientRegisterServiceReq;

typedef CsrBtGattStdBtConnIdCfm CsrBtGattClientRegisterServiceCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtGattHandle     valueHandle;            /* Attribute Handle of the Characteristic Value */
    CsrUint16           valueLength;            /* Length of the Characteristic Value in octects */
    CsrUint8           *value;                  /* Pointer to the Characteristic Value */
} CsrBtGattClientIndicationInd;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
} CsrBtGattClientIndicationRsp;

typedef CsrBtGattClientIndicationInd CsrBtGattClientNotificationInd;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrUint16           mtu;                    /* Client receive MTU size */
} CsrBtGattClientExchangeMtuReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrUint16           mtu;                    /* Maximum packet size */
} CsrBtGattClientExchangeMtuCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrUint16           mtu;                    /* Client receive MTU size */
} CsrBtGattRemoteClientExchangeMtuInd;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrUint16           mtu;                    /* Client receive MTU size */
} CsrBtGattRemoteClientExchangeMtuRes;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
}CsrBtGattDbCommitReq;

typedef CsrBtGattStdCfm CsrBtGattFlatDbCommitCfm;


/* GATT_MANAGER_SERVER_ACCESS_IND_T message for intermediate message
 *  creation */

typedef struct
{
    /*! Connection identifier of remote device. */
    connection_id_t         cid;
    /*! Handle being accessed. */
    uint16                  handle;
    /*! Flags - uses ATT_ACCESS range. */
    uint16                  flags;
    /*! The offset of the first octet to be accessed. */
    uint16                  offset;
    /*! Length of the value. */
    uint16                  size_value;
    /*! Value data. */
    uint8                   *value;
} GATT_MANAGER_SERVER_ACCESS_IND_T;

/*  CsrBtGattAccessInd message for intermediate message
 *  creation (required for GATT Long Write) */
typedef struct
{
    /* Connection identifier of remote device. */
    connection_id_t cid;
    /* Handle being accessed. */
    uint16 handle;
    /* Flags - uses ATT_ACCESS range. */
    uint16 flags;
    /* The offset of the first octet to be accessed -
     * set only in case of GATT Read, 0 otherwise */
    uint16  offset;
    /* The number of writeUnit buffers */
    uint16 numWriteUnits;
    /* Data Buffers */
    CsrBtGattAttrWritePairs *writeUnit;
} CsrBtGattAccessInd;

/* Covers priority Req/Cfm part of the Application */
typedef struct
{
    CsrBtGattPrim       type;
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrUint8            AppPriority;            /* Application Priority */
} CsrBtGattAppPriorityChangeReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* An application handle provide by GATT */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
}CsrBtGattAppPriorityChangeCfm;

typedef struct 
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* An application handle provide by GATT */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrUint8            eattFeature;            /* whether eatt feature is present or not*/
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
}CsrBtGattEattConnectInd;

/* Covers Read multiple Var Req part of the Application */
typedef CsrBtGattReadMultiReq CsrBtGattReadMultiVarReq;
typedef struct
{
    CsrBtGattPrim       type;               /* Identity */
    CsrBtGattId         gattId;             /* Application identifier */
    CsrBtResultCode     resultCode;         /* Result code */
    CsrBtSupplier       resultSupplier;     /* Result error code supplier */
    CsrBtConnId         btConnId;           /* Connection identifier */
    CsrUint16           errorHandle;        /* Handle of the first attribute in error*/
    CsrUint16           valueLength;        /* Length of the Characteristic Value in octects */
    CsrUint8            *value;             /* Pointer to the Characteristic Value */
    CsrUint16           handlesCount;       /* Num of attribute Handles */
    CsrBtGattHandle     *handles;           /* Pointer to Attribute Handles */
} CsrBtGattReadMultiVarCfm;

typedef struct
{
    CsrBtGattPrim            type;                /* Identity */
    CsrBtGattId              gattId;              /* Application handle */
    CsrBtConnId              btConnId;            /* Connection identifier */
    CsrUint16                maxRspValueLength;   /* The maximum length that the value of the attribute must have */
    CsrBtGattAccessCheck     check;               /* Special conditions that needs to be checked */
    CsrBtGattConnInfo        connInfo;            /* Connection info flags (radio type etc.) */
    CsrBtTypedAddr           address;             /* Peer address */
    CsrUint16                attrHandlesCount;    /* Number of attribute handles */
    CsrBtGattHandle          *attrHandles;        /* A set of two or more attribute handles */
} CsrBtGattDbAccessReadMultiVarInd;

typedef struct
{
    CsrBtGattPrim            type;                /* Identity */
    CsrBtGattId              gattId;              /* Application handle */
    CsrBtConnId              btConnId;            /* Connection identifier */
    CsrBtGattDbAccessRspCode responseCode;        /* Database Access response code */
    CsrUint16                errorHandle;         /* Handle of the first attribute in error*/
    CsrUint16                valuesLength;        /* Length of the attributes that has been read  */
    CsrUint8                 *values;             /* The value of the attributes that has been read */
} CsrBtGattDbAccessReadMultiVarRsp;

typedef CsrBtGattEventSendReq CsrBtGattMultiEventSendReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtConnId         btConnId;               /* connection instance index */
    CsrUint16           numCid2Disc;            /* No. of EATT CIDs to disconnect */
} CsrBtGattEattDisconnectReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* Application identifier */
    CsrUint8            flags;                  /* Long Write Mode (flags can be extended in future for other purpose as well)*/
}CsrBtGattConfigModeReq;

/* Covers BR/EDR connect procedures */
typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtTypedAddr      address;                /* Peer address */
} CsrBtGattConnectBredrReq;

typedef CsrBtGattStdCfm CsrBtGattConnectBredrCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
} CsrBtGattAcceptBredrReq;

typedef CsrBtGattStdCfm CsrBtGattAcceptBredrCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
} CsrBtGattCancelAcceptBredrReq;

typedef CsrBtGattStdCfm CsrBtGattCancelAcceptBredrCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtTypedAddr      address;                /* Peer address */
    CsrUint16           mtu;                    /* Maximum packet size */
} CsrBtGattConnectBredrInd;

typedef struct
{
    CsrBtGattPrim                type;                   /* Identity */
    CsrBtGattId                  gattId;                 /* Application identifier */
    CsrBtTypedAddr               address;                /* Peer address */
    CsrBtGattConnectBredrResCode responseCode;           /* BR/EDR Connection response code */
} CsrBtGattConnectBredrRes;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtTypedAddr      address;                /* Peer address */
} CsrBtGattDisconnectBredrReq;

#ifdef GATT_OFFLOAD

typedef CsrUint32                                         GattOffloadId;
#define GATT_OFFLOAD_ID_INVALID                           ((GattOffloadId) 0x00000000)

#define GATT_OFFLOAD_MAX_ID                                5

#define GATT_OFFLOAD_MAX_SERVICES                          5

typedef CsrUint8        CsrBtGattRole;
#define CSR_BT_GATT_ROLE_SERVER                            ((CsrBtGattRole)0x00)  /* Using the GATT Server role */
#define CSR_BT_GATT_ROLE_CLIENT                            ((CsrBtGattRole)0x01)  /* Using the GATT client role */
#define CSR_BT_GATT_ROLE_MAX                               ((CsrBtGattRole)0x02)

#define GATT_OFFLOAD_RESULT_SUCCESS                        ((CsrBtResultCode)0x0000) /* Not an error */
#define GATT_OFFLOAD_RESULT_INVALID_PARAMETER              ((CsrBtResultCode)0x0001) /* Invalid/unacceptable parameters */                                                                                       
#define GATT_OFFLOAD_RESULT_INVALID_OPERATION              ((CsrBtResultCode)0x0002) 
#define GATT_OFFLOAD_RESULT_INVALID_GATTID                 ((CsrBtResultCode)0x0003) 
#define GATT_OFFLOAD_RESULT_ACL_DOES_NOT_EXIST             ((CsrBtResultCode)0x0004)
#define GATT_OFFLOAD_RESULT_INVALID_CONNID                 ((CsrBtResultCode)0x0005)
#define GATT_OFFLOAD_RESULT_INVALID_OFFLOADID              ((CsrBtResultCode)0x0006)

typedef CsrUint16 GattOffloadErrCode;
#define GATT_OFFLOAD_ERROR_RESPONSE_TIMEOUT                ((GattOffloadErrCode)0x0000) 
#define GATT_OFFLOAD_ERROR_PROTOCOL_VIOLATION              ((GattOffloadErrCode)0x0001)
#define GATT_OFFLOAD_ERROR_DATABASE_OUT_OF_SYNC            ((GattOffloadErrCode)0x0002)

/* Sorted list of offloaded characteristics w.r.t attribute handle */
typedef struct CsrBtGattChar_tag
{
    CsrBtGattHandle handle;                    /*!< Attribute handle */
    CsrUint16       properties;                /*!< Attribute properties */
} CsrBtGattChar;

typedef struct
{
    CsrBtGattId    gattId;                     /*!< Micro app gatt id which handles offloaded service*/
    CsrBtGattRole  role;                       /*!< Defines GATT service role */
    GattOffloadId  offloadId;                  /*!< Generated by stack for every service/app */
    CsrBtUuid      uuid;                       /*!< Service UUID */
    CsrUint16      numChar;                    /*!< Number of characteristics */
    CsrBtGattChar  charList[1];                /*!< Variable len array */
} GattOffloadService;

typedef struct
{
    CsrBool     clientSupported;
    CsrBool     serverSupported;
    CsrUint16   clientProperties;              /*!< Supported client properties */
    CsrUint16   serverProperties;              /*!< Supported server properties */
} GattOffloadCapabilities;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* CFM is sent to this gattId */
    CsrUint32           context;                /* Value returned in GattOffloadConnInfoCfm */
    CsrUint16           aclHandle;              /* ACL connection handle for offloaded conn */
    CsrUint16           mtu;                    /* MTU for ATT fixed channel */
} GattOffloadConnInfoReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* gattId received in REQ */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrUint32           context;                /* Value in GattOffloadConnInfoReq */
    CsrUint16           aclHandle;              /* ACL connection handle for offloaded conn */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrBtTypedAddr      address;                /* Peer address */
} GattOffloadConnInfoCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* CFM is sent to this gattId */
    CsrUint32           context;                /* Value returned in GattOffloadEnableServiceCfm */
    CsrBtConnId         btConnId;               /* Connection identifier */
    GattOffloadService  *service;               /* Offloaded service */
} GattOffloadEnableServiceReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* gattId received in REQ */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrUint32           context;                /* Value in GattOffloadEnableServiceReq */
    CsrBtConnId         btConnId;               /* Connection identifier */
    GattOffloadId       offloadId;
} GattOffloadEnableServiceCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* CFM is sent to this gattId */
    CsrUint32           context;                /* Value returned in GattOffloadDisableServiceCfm */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrUint16           numOffloadIds;
    GattOffloadId       offloadId[GATT_OFFLOAD_MAX_ID];
} GattOffloadDisableServiceReq;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* gattId received in REQ */
    CsrBtResultCode     resultCode;             /* Result code */
    CsrBtSupplier       resultSupplier;         /* Result error code supplier */
    CsrUint32           context;                /* Value in GattOffloadDisableServiceReq */
    CsrBtConnId         btConnId;               /* Connection identifier */
} GattOffloadDisableServiceCfm;

typedef struct
{
    CsrBtGattPrim       type;                   /* Identity */
    CsrBtGattId         gattId;                 /* The application handle */
    CsrBtConnId         btConnId;               /* Connection identifier */
    CsrUint16           cid;                    /* CID */
    GattOffloadErrCode  errCode;                /* Error code */
} GattOffloadErrorInd;


#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_PRIM_H__ */
