/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      endpt_mgr_rpc.h
===========================================================================*/
/**
 * @file endpt_mgr_rpc.h
 * @brief Defines the RPC structures to be used for communication with APP manager.
 *
 * @details This file uses flexible array members to handle variable length structures efficiently.
 *          For more information, please read here:
 
 */


#ifndef ENDPT_MGR_RPC_H
#define ENDPT_MGR_RPC_H

/*============================================================================*
                           INCLUDE FILES
*============================================================================*/
#include<stdint.h>
#include "bt_pal_heap.h"
#include "bt_pal_assert.h"
#include "offload_mgr_transport_shim.h"
#include "profile_mgr_gatt.h"

/*============================================================================*
                           MACRO DEFINITIONS
*============================================================================*/

/*===========================================================================
DEFINES      Message Opcodes for communication with AWM
===========================================================================*/
/**
 * @brief Proto messages coming from BT offload Socket HAL.
 *
 * @details These opcodes are used for both request and response events.
 *
 * @define UAPP_BT_STATUS
 * @brief Opcode for Bluetooth status.
 */
#define UAPP_BT_STATUS                      0x0F00

/**
 * @define UAPP_OPEN_SOCKET_REQ
 * @brief Opcode for socket open request.
 */
#define UAPP_OPEN_SOCKET_REQ                0x0A01

/**
 * @define UAPP_OPEN_SOCKET_RES
 * @brief Opcode for socket open response.
 */
#define UAPP_OPEN_SOCKET_RES                UAPP_OPEN_SOCKET_REQ

/**
 * @define UAPP_SOCKET_CLOSE_REQ
 * @brief Opcode for socket close request.
 */
#define UAPP_SOCKET_CLOSE_REQ               0x0A02
/**
 * @define UAPP_SOCKET_CLOSE_RES
 * @brief Opcode for socket close response.
 */
#define UAPP_SOCKET_CLOSE_RES               UAPP_SOCKET_CLOSE_REQ

/**
 * @define UAPP_DATA_TX_REQ
 * @brief Opcode for data transmission request.
 */
#define UAPP_DATA_TX_REQ                    0x0A03

/**
 * @define UAPP_DATA_TX_CFM
 * @brief Opcode for data transmission confirmation.
 */
#define UAPP_DATA_TX_CFM                    UAPP_DATA_TX_REQ

/**
 * @define UAPP_DATA_RX_IND
 * @brief Opcode for data reception indication.
 */
#define UAPP_DATA_RX_IND                    0x0A04

#define UAPP_SOCKET_CLOSE_IND               0x0A05

/**
 * @define UAPP_DATA_RX_RES
 * @brief Opcode for data reception response.
 */
#define UAPP_DATA_RX_RES                    UAPP_DATA_RX_IND

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
 * @define UAPP_GATT_SESSION_UNREGISTERED_IND
 * @brief Opcode for MicroApp to initiate GATT session unregister.
 */
#define UAPP_GATT_SESSION_UNREGISTERED_IND      0x0A14

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
 * @define ENDPT_MGR_RPC_HDR_SIZE
 * @brief Size of the RPC header.
 */
#define ENDPT_MGR_RPC_HDR_SIZE              sizeof(endpt_mgr_rpc_header_t)

/**
 * @define ENDPT_MGR_RPC_SKIP_HDR
 * @brief Macro to skip the RPC header in a packet.
 * @param packet The packet to skip the header in.
 */
#define ENDPT_MGR_RPC_SKIP_HDR(packet)      (packet + ENDPT_MGR_RPC_HDR_SIZE)

/**
 * @enum message_format_t
 * @brief Enumeration for message formats.
 */
typedef enum {
    MESSAGE_FORMAT_RAW,   /**< Raw message format */
    MESSAGE_FORMAT_PROTO, /**< Proto message format */
} message_format_t;

/**
 * @enum command_type_t
 * @brief Enumeration for command types.
 */
typedef enum {
    OFFLOAD_APP_CMD,   /**< Offload application command */
    NONOFFLOAD_APP_CMD,/**< Non-offload application command */
} command_type_t;

/**
 * @enum bt_status_t
 * @brief Enumeration for Bluetooth status.
 */
typedef enum {
    BT_STATUS_ON,  /**< Bluetooth is on */
    BT_STATUS_OFF, /**< Bluetooth is off */
} bt_status_t;

/**
 * @struct endpt_mgr_rpc_header
 * @brief Structure for RPC header.
 */
typedef struct endpt_mgr_rpc_header {
    uint16_t command_type;    /**< Type of command */
    uint16_t opcode;          /**< Operation code */
    uint16_t message_format;  /**< Format of the message */
    uint16_t data_len;        /**< Length of the data */
    uint64_t endpoint_id;     /**< ID of the endpoint */
    uint8_t data[];           /**< Data payload */
} endpt_mgr_rpc_header_t;

typedef struct lecoc_channel_info {
    uint32_t remotemtu;
    uint32_t remoteMps;
}lecoc_channel_info_t;

typedef struct rfcomm_channel_info {
    uint32_t remotemtu;
    uint32_t  maxFrameSize;
}rfcomm_channel_info_t;


typedef union socket_info {
    lecoc_channel_info_t lecoc_socket_info;
    rfcomm_channel_info_t rfcomm_socket_info;
}socket_info_t;
   
/**
 * @struct uapp_socket_open_req
 * @brief Structure for socket open request.
 */
typedef struct uapp_socket_open_req {
    uint64_t socket_id;
    uint8_t  socket_type; 
    socket_info_t socket_info;
}uapp_socket_open_req_t;

/**
 * @struct uapp_socket_open_rsp
 * @brief Structure for socket open response.
 */
typedef struct uapp_socket_open_rsp {
    uint64_t socket_id; /**< ID of the socket */
    uint8_t status;     /**< Status of the request */
} uapp_socket_open_rsp_t;

/**
 * @struct uapp_socket_close_req
 * @brief Structure for socket close request.
 */
typedef struct uapp_socket_close_req {
    uint64_t socket_id; /**< ID of the socket */
} uapp_socket_close_req_t;

typedef struct uapp_socket_close_rsp {
    uint64_t socket_id; /**< ID of the socket */
} uapp_socket_close_rsp_t;


/**
 * @struct uapp_socket_close_ind_t
 * @brief Structure representing a socket close indication.
 */
typedef struct uapp_socket_close_ind {
    uint64_t socket_id; /**< ID of the socket */
    int32_t reason;     /**< Reason for the close */
} uapp_socket_close_ind_t;

/**
 * @struct uapp_data_tx_req
 * @brief Structure for data transmission request.
 */
typedef struct uapp_data_tx_req {
    uint64_t socket_id; /**< ID of the socket */
    int data_len;       /**< Length of the data */
    uint8_t data[];     /**< Data payload */
} uapp_data_tx_req_t;

/**
 * @struct uapp_data_tx_cfm
 * @brief Structure for data transmission confirmation.
 */
typedef struct uapp_data_tx_cfm {
    uint64_t socket_id; /**< ID of the socket */
    uint8_t status;     /**< Status of the transmission */
} uapp_data_tx_cfm_t;

/**
 * @struct uapp_data_rx_ind
 * @brief Structure for data reception indication.
 */
typedef struct uapp_data_rx_ind {
    uint64_t socket_id; /**< ID of the socket */
    int data_len;       /**< Length of the data */
    uint8_t data[];     /**< Data payload */
} uapp_data_rx_ind_t;

/**
 * @struct uapp_data_rx_rsp
 * @brief Structure for data reception response.
 */
typedef struct uapp_data_rx_rsp {
    uint64_t socket_id; /**< ID of the socket */
} uapp_data_rx_rsp_t;

/**
 * @struct bt_status_msg
 * @brief Structure for Bluetooth status message.
 */
typedef struct bt_status_msg {
    bt_status_t bt_status; /**< Bluetooth status */
} bt_status_msg_t;


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
    uint32_t    context;    /**< Used to store uapp span address Applicable for Client role.*/
    uint16_t    handle;      /**< Handle of the characteristic to read. */
    uint16_t    mtu;        /**< Negotiated MTU. Applicable for Server role. */
} uapp_gatt_char_read_req_t;

/**
 * @struct uapp_gatt_char_read_rsp_t
 * @brief Structure for GATT characteristic read response.
 *
 * Contains the session ID, handle, value length, result, and value for a characteristic read response.
 */
typedef struct uapp_gatt_char_read_rsp {
    int32_t     session_id;  /**< ID of the GATT session. */
    uint32_t    context;    /**< Used to store uapp span address Applicable for Client role.*/
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
    uint32_t    context;        /**< Used to store uapp span address Applicable for Client role.*/
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
    uint32_t    context;        /**< Used to store uapp span address Applicable for Client role.*/
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
    uint32_t    context;        /**< Used to store uapp span address Applicable for Server role.*/
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
    uint32_t    context;        /**< Used to store uapp span address Applicable for Server role.*/
    uint16_t    handle;      /**< Handle of the characteristic. */
    gatt_uapp_status_t    result;      /**< Result of the notification indication. */
} uapp_gatt_char_notif_ind_rsp_t;

endpt_mgr_rpc_header_t *endpt_mgr_rpc_get_default_header(void);

#endif /* ENDPT_MGR_RPC_H */
