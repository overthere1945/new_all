/******************************************************************************
 * @file     lpai_bt_gatt_common.h
 * @brief    LPAI BT GATT Common header file used for communication with App Manager.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

#ifndef LPAI_BT_GATT_COMMON_H
#define LPAI_BT_GATT_COMMON_H

/** Program Specific Header file Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <stringl.h>
#include "lpai_bt_app_mgr_client_interface.h"
#include "lpai_bt_app_mgr_adsp_handler.h"
#include "lpai_bt_gatt.pb.h"

/**
 * @def GATT_MAX_UAPPS
 * @brief Maximum number of GATT micro-apps supported.
 */
#define GATT_MAX_UAPPS                          2

/**
 * @def GATT_OFFLOAD_UAPP_MAX_SESSIONS
 * @brief Maximum number of offload sessions per micro-app.
 */
#define GATT_OFFLOAD_UAPP_MAX_SESSIONS          2

/**
 * @def GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS
 * @brief Maximum number of characteristics per session.
 */
#define GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS    2

/**
 * @def GATT_OFFLOAD_UUID_BYTE_LEN
 * @brief Number of bytes in a UUID.
 */
#define GATT_OFFLOAD_UUID_BYTE_LEN              16

/**
 * @def GATT_OFFLOAD_SERVER_ROLE
 * @brief Role value for a GATT server.
 */
#define GATT_OFFLOAD_SERVER_ROLE                0x00

/**
 * @def GATT_OFFLOAD_CLIENT_ROLE
 * @brief Role value for a GATT client.
 */
#define GATT_OFFLOAD_CLIENT_ROLE                0x01

/** Pending operations on characteristics */
#define GATT_PENDING_OP_CHAR_NONE               0x00   /**< No pending operation */
#define GATT_PENDING_OP_CLIENT_CHAR_READ_REQ    0x01   /**< Pending client read request */
#define GATT_PENDING_OP_CLIENT_CHAR_WRITE_REQ   0x02   /**< Pending client write request */
#define GATT_PENDING_OP_CLIENT_CHAR_NOTIF_IND   0x03   /**< Pending client notification indication */
#define GATT_PENDING_OP_SERVER_CHAR_READ_REQ    0x10   /**< Pending server read request */
#define GATT_PENDING_OP_SERVER_CHAR_WRITE_REQ   0x20   /**< Pending server write request */
#define GATT_PENDING_OP_SERVER_CHAR_NOTIF_IND   0x30   /**< Pending server notification indication */


/** Macro used to disable logs that are used for debugging purpose */
//#define DEBUG_BUILD_LOG

/**
 * @brief Micro app callback type to be registered with GATT
 * @param endpointId Endpoint identifier
 * @param opcode Operation code
 * @param data Callback data
 * @param data_len Length of data
 */
typedef void (*uapp_cb_t)(uint64_t endpointId, uint16_t opcode, void *data, uint16_t data_len);

/**
 * @enum gatt_uapp_status_t
 * @brief Result codes for GATT UAPP operations.
 */
typedef enum 
{
    statusOk = 0,        // This is the only success status, all other variants are for
                   // failures
    statusDisconnected,  // The request failed because the Bluetooth link is disconnected
    statusRequestInFlight,  // The app has sent a request when a prior request is still in flight
    statusInvalidParameter,  // The app supplied an invalid parameter, such as an
                      // unknown session ID or attribute ID, or a span of the
                      // wrong size
    statusInvalidOperation,  // The attribute does not support the requested operation
    statusOperationTimeout,  // A GATT operation has timed out
    statusAttributesRejected,  // The app rejected the acquired attributes
    statusSessionClosed,       // The request failed because the session was closed
    statusResourceExhausted,   // The request failed due to lack of resources

    // The following is a mapping of the ATT_ERROR_RSP error codes from BT
    // spec 3.4.11 These are received from the connected peer and forwarded from
    // the GATT manager to GATT app, they do not originate within the GATT manager
    // The values have been changed to fit into this generic Status enum space
    statusGATTInvalidHandle,      // The attribute handle given was not valid on this server
    statusGATTReadNotPermitted,   // The attribute cannot be read
    statusGATTWriteNotPermitted,  // The attribute cannot be written
    statusGATTInvalidPDU,         // The attribute PDU was invalid
    statusGATTInsufficientAuthentication,  // The attribute requires authentication
                                    // before it can be read or written
    statusGATTRequestNotSupported,  // ATT Server does not support the request received
                             // from the client
    statusGATTInvalidOffset,  // Offset specified was past the end of the attribute
    statusGATTInsufficientAuthorization,  // The attribute requires authorization
                                   // before it can be read or written
    statusGATTPrepareQueueFull,           // Too many prepare writes have been queued
    statusGATTAttributeNotFound,  // No attribute found within the given attribute
                           // handle range
    statusGATTAttributeNotLong,   // The attribute cannot be read using the
                           // ATT_READ_BLOB_REQ PDU
    statusGATTEncryptionKeySizeTooShort,    // The Encryption Key Size used for
                                     // encrypting this link is too short
    statusGATTInvalidAttributeValueLength,  // The attribute value length is invalid
                                     // for the operation
    statusGATTUnlikelyError,  // The attribute request has encountered an error that
                       // was unlikely
    statusGATTInsufficientEncryption,  // The attribute requires encryption before it
                                // can be read or written
    statusGATTUnsupportedGroupType,   // The attribute type is not a supported grouping
                               // attribute as defined by a higher layer
                               // specification
    statusGATTInsufficientResources,  // Insufficient Resources to complete the request
    statusGATTDatabaseOutOfSync,  // The server requests the client to rediscover the
                           // database
    statusGATTValueNotAllowed,    // The attribute parameter value was not allowed
    statusGATTApplicationError,   // Application error code defined by a higher layer
                           // specification
    statusGATTCommonProfileandServiceError,  // Common profile and service error codes

    statusUnknown,  // A generic error for all other failures
}gatt_uapp_status_t;


/**
 * @struct uapp_gatt_register_rsp_t
 * @brief Structure for GATT micro-app register response.
 */
typedef struct uapp_gatt_register_rsp {
    uint8_t     status;  /**< Status of the response */
} uapp_gatt_register_rsp_t;

/**
 * @struct uapp_gatt_unregister_rsp_t
 * @brief Structure for GATT micro-app unregister response.
 */
typedef struct uapp_gatt_unregister_rsp {
    uint8_t     status;  /**< Status of the response */
} uapp_gatt_unregister_rsp_t;

/**
 * @struct uapp_gatt_register_service_rsp_t
 * @brief Structure for GATT register service response.
 */
typedef struct uapp_gatt_register_service_rsp {
    int32_t session_id;  /**< ID of the GATT session */
    int32_t status;      /**< Status of the response */
} uapp_gatt_register_service_rsp_t;

/**
 * @struct uapp_gatt_unregister_service_evt_t
 * @brief Structure for GATT unregister service event from DSP.
 */
typedef struct uapp_gatt_unregister_service_evt {
    int32_t session_id;  /**< ID of the GATT session */
} uapp_gatt_unregister_service_evt_t;

/**
 * @struct uapp_gatt_unregister_service_ind_t
 * @brief Structure for GATT unregister service request indication.
 */
typedef struct uapp_gatt_unregister_service_ind {
    int32_t session_id;  /**< ID of the GATT session */
} uapp_gatt_unregister_service_ind_t;

/**
 * @struct uapp_gatt_char_read_req_t
 * @brief Structure representing a GATT characteristic read request.
 */
typedef struct uapp_gatt_char_read_req {
    int32_t     session_id;    /**< ID of the GATT session */
    uint32_t    context;    /**< Used to store uapp span address Applicable for Client role.*/
    uint16_t    handle;        /**< Handle of the characteristic */
    uint16_t    mtu;        /**< Negotiated MTU. Applicable for Server role. */
} uapp_gatt_char_read_req_t;

/**
 * @struct uapp_gatt_char_read_rsp_t
 * @brief Structure representing a GATT characteristic read response.
 */
typedef struct uapp_gatt_char_read_rsp {
    int32_t     session_id;    /**< ID of the GATT session */
    uint32_t    context;    /**< Used to store uapp span address Applicable for Client role.*/
    uint16_t    handle;        /**< Handle of the characteristic */
    uint16_t    val_len;       /**< Length of the value */
    gatt_uapp_status_t    result;        /**< Result of the operation */
    uint8_t     val[];         /**< Value read */
} uapp_gatt_char_read_rsp_t;

/**
 * @struct uapp_gatt_char_write_req_t
 * @brief Structure representing a GATT characteristic write request.
 */
typedef struct uapp_gatt_char_write_req {
    int32_t     session_id;       /**< ID of the GATT session */
    uint32_t    context;        /**< Used to store uapp span address Applicable for Client role.*/
    uint16_t    handle;           /**< Handle of the characteristic */
    uint16_t    val_len;          /**< Length of the value to write */
    bool        writeWithoutRsp;  /**< Write without response flag */
    uint8_t     val[];            /**< Value to write */
} uapp_gatt_char_write_req_t;

/**
 * @struct uapp_gatt_char_write_rsp_t
 * @brief Structure representing a GATT characteristic write response.
 */
typedef struct uapp_gatt_char_write_rsp {
    int32_t     session_id;    /**< ID of the GATT session */
    uint32_t    context;        /**< Used to store uapp span address Applicable for Client role.*/
    uint16_t    handle;        /**< Handle of the characteristic */
    gatt_uapp_status_t    result;        /**< Result of the operation */
} uapp_gatt_char_write_rsp_t;

/**
 * @struct uapp_gatt_char_notif_ind_t
 * @brief Structure representing a GATT notification or indication.
 */
typedef struct uapp_gatt_char_notif_ind {
    int32_t     session_id;    /**< ID of the GATT session */
    uint32_t    context;        /**< Used to store uapp span address Applicable for Server role.*/
    uint16_t    handle;        /**< Handle of the characteristic */
    uint16_t    val_len;       /**< Length of the value */
    uint8_t     val[];         /**< Value */
} uapp_gatt_char_notif_ind_t;

/**
 * @struct uapp_gatt_char_notif_ind_rsp_t
 * @brief Structure representing response to GATT notification or indication.
 */
typedef struct uapp_gatt_char_notif_ind_rsp {
    int32_t     session_id;    /**< ID of the GATT session */
    uint32_t    context;        /**< Used to store uapp span address Applicable for Server role.*/
    uint16_t    handle;        /**< Handle of the characteristic */
    gatt_uapp_status_t    result;        /**< Result of the operation */
} uapp_gatt_char_notif_ind_rsp_t;

/**
 * @struct gatt_char_holder_t
 * @brief Structure holding the properties and UUID of a GATT characteristic.
 * It is used to during proto decoding of Glink message.
 */
typedef struct gatt_char_holder {
    uint8_t uuid[GATT_OFFLOAD_UUID_BYTE_LEN];  /**< 128-bit UUID for characteristic */
    int32_t properties;                        /**< Properties of characteristic */
    int32_t valueHandle;                       /**< Value handle */
} gatt_char_holder_t;

/**
 * @struct char_list_holder_t
 * @brief Structure holding a list of GATT characteristics.
 * It is used to during proto decoding of Glink message.
 */
typedef struct {
    gatt_char_holder_t chars[GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS]; /**< Array of characteristics */
    uint8_t num_chars;                                              /**< Number of characteristics */
} char_list_holder_t;

/**
 * @brief Compare two 16-byte UUIDs.
 * @param uuid1 First UUID buffer
 * @param uuid2 Second UUID buffer
 * @return True if UUIDs are identical, else false
 */
bool gatt_compare_uuids(uint8_t *uuid1, uint8_t *uuid2);

/**
 * @struct gatt_pending_op_t
 * @brief Structure for holding a pending GATT operation.
 */
typedef struct gatt_pending_op {
    int32_t sessionId; /**< GATT Session ID */
    uint16_t handle;   /**< Characteristic handle */
    uint8_t op;        /**< Pending operation code */
} gatt_pending_op_t;

/**
 * @struct gatt_uapps_t
 * @brief Structure for storing micro-app registration, session and pending op info.
 */
typedef struct gatt_uapp {
    uint64_t      ep_id;                                  /**< Endpoint ID */
    uapp_cb_t     uapp_cb;                                /**< Callback function */
    int32_t       sessionId[GATT_OFFLOAD_UAPP_MAX_SESSIONS]; /**< Session IDs */
} gatt_uapps_t;


/**
 * @brief Initializes the Common GATT Structures.
 *
 * This function resets the necessary configurations, data structures, and
 * resources required to start the Common Gatt Structures.
 *
 * @note This function does not take any parameters and does not return a value.
 */
void gatt_common_deinit();

#endif /* LPAI_BT_GATT_COMMON_H */