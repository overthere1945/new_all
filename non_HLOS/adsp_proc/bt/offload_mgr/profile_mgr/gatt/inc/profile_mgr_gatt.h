/*==============================================================================*/
/*
* Copyright (c) 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      profile_mgr_gatt.h
===========================================================================*/
/**
 * @file profile_mgr_gatt.h
 * @brief Public header file for GATT profile offload.
 *
 * @details This file contains the public API definitions and data structures for the GATT offload.
 */

#ifndef PROFILE_MGR_GATT_H
#define PROFILE_MGR_GATT_H 

#include "gatt_offload_mgr.pb.h"
#include "csr_bt_gatt_lib.h"

/**
 * @def BT_OFFLOAD_GATT_REGISTER_SERVICE
 * @brief Opcode for registering a GATT service via BT GATT HAL.
 * @details Used for both request and response events.
 */
#define BT_OFFLOAD_GATT_REGISTER_SERVICE                    0x0F10

/**
 * @def BT_OFFLOAD_GATT_UNREGISTER_SERVICE
 * @brief Opcode for unregistering a GATT service via BT GATT HAL.
 * @details Used for both request and response events.
 */
#define BT_OFFLOAD_GATT_UNREGISTER_SERVICE                  0x0F11

/**
 * @def BT_OFFLOAD_GATT_CLEAR_SERVICES
 * @brief Opcode to clear all GATT services on an ACL handle.
 * @details Used for both request and response events.
 */
#define BT_OFFLOAD_GATT_CLEAR_SERVICES                      0x0F12

/**
 * @def BT_OFFLOAD_GATT_GET_CAPABILITIES
 * @brief Opcode for requesting and responding with GATT offload capabilities.
 * @details Used for both request and response events.
 */
#define BT_OFFLOAD_GATT_GET_CAPABILITIES                    0x0F13

/**
 * @def BT_OFFLOAD_GATT_ERROR_REPORT
 * @brief Opcode for sending error reports on ACL handle to GATT HAL.
 */
#define BT_OFFLOAD_GATT_ERROR_REPORT                        0x0F14

/**
 * @def GATT_OFFLOAD_MAX_ENDPOINT
 * @brief Maximum number of GATT offload endpoints supported.
 */
#define GATT_OFFLOAD_MAX_ENDPOINT               (3)

/**
 * @def GATT_OFFLOAD_ENDPOINT_MAX_SERVICES
 * @brief Maximum number of services per endpoint.
 */
#define GATT_OFFLOAD_ENDPOINT_MAX_SERVICES      (2)

/**
 * @def GATT_OFFLOAD_SERVICE_MAX_SESSIONS
 * @brief Maximum number of sessions per service.
 */
#define GATT_OFFLOAD_SERVICE_MAX_SESSIONS       (2)

/**
 * @def GATT_OFFLOAD_SESSION_MAX_CHARS
 * @brief Maximum number of characteristics per session.
 */
#define GATT_OFFLOAD_SESSION_MAX_CHARS          (3)

/**
 * @def GATT_OFFLOAD_MAX_MTU_SIZE
 * @brief Maximum MTU size supported.
 */
#define GATT_OFFLOAD_MAX_MTU_SIZE               (512)

/**
 * @def GATT_OFFLOAD_UUID_BYTE_LEN
 * @brief Length of UUID in bytes.
 */
#define GATT_OFFLOAD_UUID_BYTE_LEN              (16)

/**
 * @def GATT_OFFLOAD_MAX_SESSIONS
 * @brief Maximum number of sessions across all endpoints.
 */
#define GATT_OFFLOAD_MAX_SESSIONS               (GATT_OFFLOAD_MAX_ENDPOINT * GATT_OFFLOAD_ENDPOINT_MAX_SERVICES)

/**
 * @def GATT_OFFLOAD_SERVICE_MAX_CHARS
 * @brief Maximum number of characteristics per service.
 */
#define GATT_OFFLOAD_SERVICE_MAX_CHARS          (GATT_OFFLOAD_SERVICE_MAX_SESSIONS * GATT_OFFLOAD_SESSION_MAX_CHARS)

/**
 * @def GATT_OFFLOAD_FRAMEWORK_REG_CB_CTX
 * @brief Framework registration callback context.
 */
#define GATT_OFFLOAD_FRAMEWORK_REG_CB_CTX        GATT_OFFLOAD_MAX_ENDPOINT

/**
 * @def GATT_OFFLOAD_INVALID_SESSION_ID
 * @brief Invalid session ID value.
 */
#define GATT_OFFLOAD_INVALID_SESSION_ID         0

/**
 * @def GATT_OFFLOAD_INVALID_ENDPOINT_ID
 * @brief Invalid endpoint ID value.
 */
#define GATT_OFFLOAD_INVALID_ENDPOINT_ID        0

/**
 * @def GATT_OFFLOAD_INVALID_CHAR_HANDLE
 * @brief Invalid characteristic handle value.
 */
#define GATT_OFFLOAD_INVALID_CHAR_HANDLE        0

/**
 * @def GATT_OFFLOAD_PENDING_OP_NONE
 * @brief No pending GATT operation in progress.
 */
#define GATT_OFFLOAD_PENDING_OP_NONE            0

/**
 * @def GATT_OFFLOAD_INVALID_CONTEXT
 * @brief Invalid session context from uapp.
 */
#define GATT_OFFLOAD_INVALID_CONTEXT            0

/**
 * @typedef gatt_session_id_t
 * @brief Type definition for GATT session ID.
 */
typedef int32_t gatt_session_id_t;

/**
 * @enum gatt_offload_hal_status_t
 * @brief Status codes for GATT offload HAL operations.
 */
typedef enum 
{
    GATT_OFFLOAD_STATUS_SUCCESS = 0,           /**< Operation successful */
    GATT_OFFLOAD_STATUS_INVALID_ENDPOINT,      /**< Invalid endpoint */
    GATT_OFFLOAD_STATUS_UNSUPPORTED_ROLE,      /**< Unsupported role */
    GATT_OFFLOAD_STATUS_INSUFF_RESOURCES,      /**< Insufficient resources */
    GATT_OFFLOAD_STATUS_INTERNAL_ERROR,        /**< Internal error */
    GATT_OFFLOAD_STATUS_MAX = GATT_OFFLOAD_STATUS_INTERNAL_ERROR, /**< Max value */
} gatt_offload_hal_status_t;

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
 * @enum ustack_op_t
 * @brief Ustack operation types.
 */
typedef enum 
{
    USTACK_OFFLOAD_FW_REGISTRATION = 0,        /**< Framework registration */
    USTACK_UAPP_REGISTRATION,                  /**< UAPP registration */
    USTACL_ACL_CONN_INFO,                      /**< ACL connection info */
    USTACK_ENABLE_SERVICE,                     /**< Enable service */
    USTACK_DISABLE_SERVICE,                    /**< Disable service */
    USTACK_CLEAR_SERVICES,                     /**< Clear services */
    USTACK_OP_MAX = USTACK_CLEAR_SERVICES,     /**< Max value */
}ustack_op_t;

/**
 * @struct gatt_chars_t
 * @brief Structure representing a GATT characteristic.
 */
typedef struct gatt_chars
{
    uint16_t        handle;    /**< Characteristic handle */
    uint16_t        prop;      /**< Characteristic properties */
} gatt_chars_t; 

/**
 * @struct gatt_session_t
 * @brief Structure representing a GATT session.
 */
typedef struct gatt_session
{
    gatt_session_id_t   session_id;                /**< Session ID */
    gatt_chars_t        char_list[GATT_OFFLOAD_SESSION_MAX_CHARS];              /**< List of characteristics */
    uint32_t            uapp_ctx;
    uint16_t            tx_pending_op;
    uint16_t            tx_pending_handle;
} gatt_session_t;

/**
 * @struct encoded_buf_t
 * @brief Structure representing an encoded buffer.
 */
typedef struct encoded_buf 
{
    void            *data;     /**< Pointer to buffer data */
    uint16_t        len;       /**< Length of buffer */
}encoded_buf_t;

/**
 * @struct gatt_svc_t
 * @brief Structure representing a GATT service.
 */
typedef struct gatt_svc
{
    uint16_t        acl_handle;                                    /**< ACL handle */
    uint16_t        mtu;                                           /**< MTU size */
    uint32_t        btConnId;                                      /**< Bluetooth connection ID */
    uint32_t        offloadId;                                     /**< Offload ID */
    uint8_t         role;                                          /**< Role */
    uint8_t         uuid[GATT_OFFLOAD_UUID_BYTE_LEN];              /**< Service UUID */
    gatt_session_t  sessions[GATT_OFFLOAD_SERVICE_MAX_SESSIONS];   /**< Sessions */
    encoded_buf_t   buf;                                           /**< Encoded buffer */
}gatt_svc_t;
 
/**
 * @struct gatt_endpoint_t
 * @brief Structure representing a GATT endpoint.
 */
typedef struct gatt_endpoint
{
    uint64_t        ep_id;                                         /**< Endpoint ID */
    uint32_t        gattId;                                        /**< GATT ID */
    gatt_svc_t      svc_list[GATT_OFFLOAD_ENDPOINT_MAX_SERVICES];  /**< List of services */
} gatt_endpoint_t; 

/**
 * @struct ustack_cb_ctx_t
 * @brief Structure representing a Ustack callback context.
 */
typedef struct ustack_cb_ctx {
    ustack_op_t             operation;     /**< Operation type */
    void *                  data;          /**< Pointer to operation data */
}ustack_cb_ctx_t;

/**
 * @enum gatt_endpoint_match_t
 * @brief Endpoint matching types.
 */
typedef enum 
{
    ENDPT_MATCH_EP_ID,     /**< Match by endpoint ID */
    ENDPT_MATCH_GATT_ID    /**< Match by GATT ID */
} gatt_endpoint_match_t;

/**
 * @struct ustack_evt_hdr
 * @brief Structure representing a Ustack event header.
 */
typedef struct ustack_evt_hdr_t
{
    CsrBtGattPrim type;    /**< Event type */
    CsrBtGattId   gattId;  /**< GATT ID */
} ustack_evt_hdr;

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_offload
===========================================================================*/
/**
 * @brief Handles offload request from HAL.
 * @details
 *   - Decodes register service event.
 *   - Validates endpoint and characteristic limits.
 *   - Creates offload instance and session.
 *   - Stores HAL GATT context for later use.
 *   - Handles offload event in state machine.
 * @param evt Pointer to event data.
 * @param len Length of event data.
 */
void profile_mgr_gatt_hal_offload(uint8_t *evt, size_t len);

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_unoffload
===========================================================================*/
/**
 * @brief Handles unoffload request from HAL.
 * @details
 *   - Decodes unregister service event.
 *   - Finds offload instance by session ID.
 *   - Handles unoffload event in state machine.
 * @param evt Pointer to event data.
 * @param len Length of event data.
 */
void profile_mgr_gatt_hal_unoffload(uint8_t *evt, size_t len);

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_get_caps
===========================================================================*/
/**
 * @brief Gets GATT offload capabilities.
 * @details
 *   - Retrieves GATT offload capabilities from MicroStack.
 *   - Encodes and sends capabilities response to HAL.
 */
void profile_mgr_gatt_hal_get_caps(void);

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_clear_service
===========================================================================*/
/**
 * @brief Handles clear service request from HAL.
 * @details
 *   - Decodes clear services event.
 *   - Clears all sessions on ACL handle.
 *   - Sends disable service request to MicroStack or clear services response to HAL.
 * @param evt Pointer to event data.
 * @param len Length of event data.
 */
void profile_mgr_gatt_hal_clear_service(uint8_t *evt, size_t len);

/*===========================================================================
FUNCTION      profile_mgr_gatt_uapp_register
===========================================================================*/
/**
 * @brief Registers a MicroApp (UAPP) to handle GATT offload.
 * @details
 *   - Allocates endpoint slot and sends registration request.
 *   - Sends response if resources are insufficient.
 * @param endpoint_id Endpoint ID.
 */
void profile_mgr_gatt_uapp_register(uint64_t endpoint_id);

/*===========================================================================
FUNCTION      profile_mgr_gatt_uapp_unregister
===========================================================================*/
/**
 * @brief Unregisters a MicroApp (UAPP).
 * @details
 *   - Finds and unregisters endpoint, clears data.
 *   - Sends response to MicroApp.
 * @param endpoint_id Endpoint ID.
 */
void profile_mgr_gatt_uapp_unregister(uint64_t endpoint_id);

/*===========================================================================
FUNCTION      profile_mgr_gatt_uapp_char_evt
===========================================================================*/
/**
 * @brief Handles characteristic events from MicroApp (UAPP).
 * @details
 *   - Decodes event and dispatches read/write/notification requests to Micro stack.
 * @param endpointId Endpoint ID.
 * @param opcode Operation code.
 * @param evt Pointer to event data.
 * @param len Length of event data.
 */
void profile_mgr_gatt_uapp_char_evt(uint64_t endpointId, uint16_t opcode, void *evt, uint16_t len);

/*===========================================================================
FUNCTION      gatt_session_mgr_init
===========================================================================*/
/**
 * @brief Initializes the GATT session manager.
 * @details
 *   - Registers client functions for offload manager.
 *   - Initializes GATT profile manager.
 */
void gatt_session_mgr_init(void);

/*===========================================================================
FUNCTION      gatt_session_mgr_deinit
===========================================================================*/
/**
 * @brief Deinitializes the GATT session manager.
 * @details
 *   - Deinitializes GATT profile manager.
 *   - Deregisters client functions from offload manager.
 */
void gatt_session_mgr_deinit(void);

/*===========================================================================
FUNCTION      profile_mgr_gatt_unoffload_to_uapp
===========================================================================*/
/**
 * @brief Unoffloads GATT session with MicroApp (UAPP).
 * @details
 *   - Prepares and sends session unregistered event to MicroApp.
 * @param data Pointer to offload instance.
 */
void profile_mgr_gatt_unoffload_to_uapp(void *data);

#endif /* PROFILE_MGR_GATT_H */
