/**************************************************************************
 * @file     lpai_bt_app_mgr.h
 * @brief    LPAI BT APP Manager header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef GATT_MGR_MSGS_H
#define GATT_MGR_MSGS_H

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @define UAPP_ENDPT_DISC_REQ
 * @brief Opcode for endpoint discovery request.
 */
#define UAPP_ENDPT_DISC_REQ                 0x00C0

/**
 * @define UAPP_ENDPT_DISC_RES
 * @brief Opcode for endpoint discovery response.
 */
#define UAPP_ENDPT_DISC_RES                 0x00C1
/**
 * @define UAPP_GATT_APP_REG_REQ
 * @brief Opcode for GATT MicroApp registration request.
 */
#define UAPP_GATT_APP_REG_REQ                   0x0A10

/**
 * @define UAPP_GATT_APP_REG_RESP
 * @brief Opcode for GATT MicroApp registration response (same as request).
 */
#define UAPP_GATT_APP_REG_RESP                  UAPP_GATT_APP_REG_REQ

/**
 * @define UAPP_GATT_APP_UNREG_REQ
 * @brief Opcode for GATT MicroApp unregistration request.
 */
#define UAPP_GATT_APP_UNREG_REQ                 0x0A11

/**
 * @define UAPP_GATT_APP_UNREG_RESP
 * @brief Opcode for GATT MicroApp GATT unregistration response (same as request).
 */
#define UAPP_GATT_APP_UNREG_RESP                UAPP_GATT_APP_UNREG_REQ

/**
 * @define UAPP_GATT_SESSION_REGISTERED_REQ
 * @brief Opcode for MicroApp GATT session registered request.
 */
#define UAPP_GATT_SESSION_REGISTERED_REQ        0x0A12

/**
 * @define UAPP_GATT_SESSION_REGISTERED_RESP
 * @brief Opcode for MicroApp GATT session registered response (same as request).
 */
#define UAPP_GATT_SESSION_REGISTERED_RESP       UAPP_GATT_SESSION_REGISTERED_REQ

/**
 * @define UAPP_GATT_SESSION_UNREGISTERED_IND
 * @brief Opcode for MicroApp to initiate GATT session unregister.
 */
#define UAPP_GATT_SESSION_UNREGISTERED_IND      0x0A14

/**
 * @define UAPP_GATT_SESSION_UNREGISTERED_REQ
 * @brief Opcode for MicroApp to inform that GATT session is unregistered.
 */
#define UAPP_GATT_SESSION_UNREGISTERED_REQ      0x0A13

/**
 * @define UAPP_GATT_SESSION_UNREGISTERED_RESP
 * @brief Opcode for MicroApp GATT session unregistered response (same as request).
 */
#define UAPP_GATT_SESSION_UNREGISTERED_RESP       UAPP_GATT_SESSION_UNREGISTERED_REQ

/**
 * @define UAPP_GATT_CLIENT_READ_CHAR_REQ
 * @brief Opcode for MicroApp GATT client read characteristic request.
 */
#define UAPP_GATT_CLIENT_READ_CHAR_REQ          0x0A15

/**
 * @define UAPP_GATT_CLIENT_READ_CHAR_RESP
 * @brief Opcode for MicroApp GATT client read characteristic response (same as request).
 */
#define UAPP_GATT_CLIENT_READ_CHAR_RESP         UAPP_GATT_CLIENT_READ_CHAR_REQ

/**
 * @define UAPP_GATT_CLIENT_WRITE_CHAR_REQ
 * @brief Opcode for MicroApp GATT client write characteristic request.
 */
#define UAPP_GATT_CLIENT_WRITE_CHAR_REQ         0x0A16

/**
 * @define UAPP_GATT_CLIENT_WRITE_CHAR_RESP
 * @brief Opcode for MicroApp GATT client write characteristic response (same as request).
 */
#define UAPP_GATT_CLIENT_WRITE_CHAR_RESP        UAPP_GATT_CLIENT_WRITE_CHAR_REQ

/**
 * @define UAPP_GATT_CLIENT_WRITE_CHAR_CMD
 * @brief Opcode for MicroApp GATT client write characteristic command.
 */
#define UAPP_GATT_CLIENT_WRITE_CHAR_CMD         0x0A17

/**
 * @define UAPP_GATT_CLIENT_NOTIFICATION_IND
 * @brief Opcode for MicroApp GATT client notification indication.
 */
#define UAPP_GATT_CLIENT_NOTIFICATION_IND       0x0A18

/**
 * @define UAPP_GATT_SERVER_READ_CHAR_IND
 * @brief Opcode for MicroApp GATT server read characteristic indication.
 */
#define UAPP_GATT_SERVER_READ_CHAR_IND          0x0A19

/**
 * @define UAPP_GATT_SERVER_READ_CHAR_IND_RESP
 * @brief Opcode for MicroApp GATT server read characteristic indication response (same as indication).
 */
#define UAPP_GATT_SERVER_READ_CHAR_IND_RESP     UAPP_GATT_SERVER_READ_CHAR_IND

/**
 * @define UAPP_GATT_SERVER_WRITE_CHAR_IND
 * @brief Opcode for MicroApp GATT server write characteristic indication.
 */
#define UAPP_GATT_SERVER_WRITE_CHAR_IND         0x0A1A

/**
 * @define UAPP_GATT_SERVER_WRITE_CHAR_IND_RESP
 * @brief Opcode for MicroApp GATT server write characteristic indication response (same as indication).
 */
#define UAPP_GATT_SERVER_WRITE_CHAR_IND_RESP    UAPP_GATT_SERVER_WRITE_CHAR_IND

/**
 * @define UAPP_GATT_SERVER_NOTIFICATION_REQ
 * @brief Opcode for MicroApp GATT server notification request.
 */
#define UAPP_GATT_SERVER_NOTIFICATION_REQ       0x0A1B

/**
 * @define UAPP_GATT_SERVER_NOTIFICATION_RESP
 * @brief Opcode for MicroApp GATT server notification response (same as request).
 */
#define UAPP_GATT_SERVER_NOTIFICATION_RESP      UAPP_GATT_SERVER_NOTIFICATION_REQ



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
 * @define UAPP_GATT_SESSION_UNREGISTERED_EVT

/**
 * @struct uapp_gatt_register_rsp_t
 * @brief Structure for GATT UAPP register response.
 *
 * Contains the status of the registration response.
 */
typedef struct uapp_gatt_register_rsp {
    uint8_t     status;      /**< Status of the response. */
} uapp_gatt_register_rsp_t;

/**
 * @struct uapp_gatt_unregister_rsp_t
 * @brief Structure for GATT UAPP unregister response.
 *
 * Contains the status of the unregister response.
 */
typedef struct uapp_gatt_unregister_rsp {
    uint8_t     status;      /**< Status of the response. */
} uapp_gatt_unregister_rsp_t;


/**
 * @struct uapp_gatt_session_registered_rsp_t
 * @brief Structure for GATT session registered response.
 *
 * Contains the session ID and status for a registered GATT session.
 */
typedef struct uapp_gatt_session_registered_rsp {
    int32_t session_id;      /**< ID of the GATT session. */
    int32_t status;          /**< Status of the response. */
} uapp_gatt_session_registered_rsp_t;

/**
 * @struct uapp_gatt_session_unregistered_t
 * @brief Structure for GATT session unregistered event.
 *
 * Contains the session ID for an unregistered GATT session.
 */
typedef struct uapp_gatt_session_unregistered {
    int32_t session_id;      /**< ID of the GATT session. */
} uapp_gatt_session_unregistered_t;


/**
 * @struct uapp_gatt_session_unregister_ind_t
 * @brief Structure for GATT session unregister indication request from micro app.
 *
 * Contains the session ID for a session unregister indication.
 */
typedef struct uapp_gatt_session_unregister_ind {
    int32_t session_id;      /**< ID of the GATT session. */
} uapp_gatt_session_unregister_ind_t;

/**
 * @struct uapp_gatt_char_read_req_t
 * @brief Structure for GATT characteristic read request.
 *
 * Contains the session ID and handle for a characteristic read request.
 */
typedef struct uapp_gatt_char_read_req {
    int32_t     session_id;  /**< ID of the GATT session. */
    uint32_t    context;
    uint16_t    handle;      /**< Handle of the characteristic to read. */
    uint16_t    mtu;
} uapp_gatt_char_read_req_t;

/**
 * @struct uapp_gatt_char_read_rsp_t
 * @brief Structure for GATT characteristic read response.
 *
 * Contains the session ID, handle, value length, result, and value for a characteristic read response.
 */
typedef struct uapp_gatt_char_read_rsp {
    int32_t     session_id;  /**< ID of the GATT session. */
    uint32_t    context;
    uint16_t    handle;      /**< Handle of the characteristic. */
    uint16_t    val_len;     /**< Length of the value. */
    gatt_uapp_status_t    result;      /**< Result of the read operation. */
    uint8_t     val[];       /**< Value read from the characteristic. */
} uapp_gatt_char_read_rsp_t;


/**
 * @struct uapp_gatt_char_write_req_t
 * @brief Structure for GATT characteristic write request.
 *
 * Contains the session ID, handle, value length, write type, and value for a characteristic write request.
 */
typedef struct uapp_gatt_char_write_req {
    int32_t     session_id;      /**< ID of the GATT session. */
    uint32_t    context;
    uint16_t    handle;          /**< Handle of the characteristic to write. */
    uint16_t    val_len;         /**< Length of the value. */
    bool        writeWithoutRsp; /**< Indicates if write is without response. */
    uint8_t     val[];           /**< Value to write to the characteristic. */
} uapp_gatt_char_write_req_t;


/**
 * @struct uapp_gatt_char_write_rsp_t
 * @brief Structure for GATT characteristic write response.
 *
 * Contains the session ID, handle, and result for a characteristic write response.
 */
typedef struct uapp_gatt_char_write_rsp {
    int32_t     session_id;  /**< ID of the GATT session. */
    uint32_t    context;
    uint16_t    handle;      /**< Handle of the characteristic. */
    gatt_uapp_status_t    result;      /**< Result of the write operation. */
    bool        writeWithoutRsp; /**< Indicates if write is without response. */
} uapp_gatt_char_write_rsp_t;


/**
 * @struct uapp_gatt_char_notif_ind_t
 * @brief Structure for GATT characteristic notification indication.
 *
 * Contains the session ID, handle, value length, and value for a characteristic notification indication.
 */
typedef struct uapp_gatt_char_notif_ind {
    int32_t     session_id;  /**< ID of the GATT session. */
    uint32_t    context;
    uint16_t    handle;      /**< Handle of the characteristic. */
    uint16_t    val_len;     /**< Length of the value. */
    uint8_t     val[];       /**< Value of the notification. */
} uapp_gatt_char_notif_ind_t;


/**
 * @struct uapp_gatt_char_notif_ind_rsp_t
 * @brief Structure for GATT characteristic notification indication response.
 *
 * Contains the session ID, handle, and result for a notification indication response.
 */
typedef struct uapp_gatt_char_notif_ind_rsp {
    int32_t     session_id;  /**< ID of the GATT session. */
    uint32_t    context;
    uint16_t    handle;      /**< Handle of the characteristic. */
    gatt_uapp_status_t    result;      /**< Result of the notification indication. */
} uapp_gatt_char_notif_ind_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /** GATT_MGR_MSGS_H */
