/**
 * @file  qapi_ipc.h
 * @brief IPC qapi interface.
 */

/*-------------------------------------------------------------------------
 * Copyright (c) 2020 - 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 * Notifications and licenses are retained for attribution purposes only
 *-----------------------------------------------------------------------*/

#include "csr_synergy.h"
#include "csr_bt_bluestack_types.h"

#define boolean CsrBool

#ifndef __QAPI_IPC_H__
#define __QAPI_IPC_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
/*-------------------------------------------------------------------------
 * Preprocessor Definitions
 * ----------------------------------------------------------------------*/
/** Message header format.
 *
 * +--------+----------+-----------+-----------+-----------+
 * | BitPos |    15    |  14 - 13  |  12 - 8   |   7 - 0   |
 * +--------+----------+-----------+-----------+-----------+
 * | Field  |   ACK    |   Rsvd    |  client   |   msgID   |
 * +--------+----------+-----------+-----------+-----------+
 *
 *   ACK         : Indicator for Tx ACK
 *
 * - Rsvd        : Reserved for future use
 *
 * - client      : Determines client of the message (@ref IPC_CLIENT)
 *                 Use @ref IPC_GET_CLIENT_ID to get client ID.
 *
 * - msgID       : Contains unique message ID for given client.
 *                 Use @ref IPC_GET_MSG_ID to get message ID.
 */

/** Macros to be used to construct message header.
 * @{
 */
#define QAPI_IPC_CLIENT_ID_MASK         (0x1F00u)
#define QAPI_IPC_MSG_ID_MASK            (0x00FFu)
#define QAPI_IPC_ACK_MASK               (0x8000u)

#define QAPI_IPC_CLIENT_ID_SHIFT        (8u)
/** @} */

#define QAPI_IPC_BUILD_MSG_HEADER(client, msg_id) \
                    (((uint16) client << QAPI_IPC_CLIENT_ID_SHIFT) | msg_id)

/** Accessor macros for message header.
 * @{
 */
#define QAPI_IPC_GET_CLIENT_ID(msg_hdr) \
        ((qapi_ipc_client_t) (((msg_hdr) & QAPI_IPC_CLIENT_ID_MASK) >> \
                                                    QAPI_IPC_CLIENT_ID_SHIFT))

#define QAPI_IPC_GET_MSG_ID(msg_hdr) \
                                ((uint8_t) ((msg_hdr) & QAPI_IPC_MSG_ID_MASK))

#define QAPI_IPC_IS_RX_MSG(msgHeader)   ((msgHeader & QAPI_IPC_ACK_MASK) == 0)
/** @} */

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

/** IPC message IDs: will be defined in individual client module */

/** IPC client list */
typedef enum
{
    QAPI_IPC_CLIENT_BT_HCI_E,
    QAPI_IPC_CLIENT_BT_HOST_CUSTOM_E,
    QAPI_IPC_CLIENT_BT_APPS_CUSTOM_E,
    QAPI_IPC_CLIENT_BT_AUDIO_E,
    QAPI_IPC_CLIENT_AUDIO_CODEC_CMDS_E,
    QAPI_IPC_CLIENT_AUDIO_LOCAL_PLAYBACK_CMDS_E,
    QAPI_IPC_CLIENT_AUDIO_SVA_SESSION_CMDS_E,
    QAPI_IPC_CLIENT_AUDIO_LOCAL_PLAYBACK_EVENTS_E,
    QAPI_IPC_CLIENT_AUDIO_SVA_DATA_EXCHANGE_EVENTS_E,
    QAPI_IPC_CLIENT_AUDIO_SVA_SESSION_EVENTS_E,
    /* Add more clients here */
    QAPI_IPC_CLIENT_MAX_E
} qapi_ipc_client_t;

/** IPC Ring type */
typedef enum
{
    QAPI_IPC_RING_TYPE_DEFAULT_E,
    QAPI_IPC_RING_TYPE_PRIORITY_E,
    QAPI_IPC_RING_TYPE_MAX_E
} qapi_ipc_ring_type_t;

/** IPC Send API return status code */
typedef enum
{
    QAPI_IPC_STATUS_SUCCESS_E,
    QAPI_IPC_STATUS_CLIENT_NOT_REGD_E,
    QAPI_IPC_STATUS_INVALID_PARAM_E,
    QAPI_IPC_STATUS_RING_BUFFER_FULL_E,
    QAPI_IPC_STATUS_MAX_E
} qapi_ipc_err_status_t;

/** IPC Message structure */
typedef struct
{
    uint16_t            msg_hdr;    /**< Message header */
    uint16_t            msg_len;    /**< Length of message */
    uint8_t *           payload;    /**< Pointer to message payload (LE) */
} qapi_ipc_msg_t;

/** IPC TX Message structure */
typedef struct
{
    qapi_ipc_msg_t          msg;        /**< IPC message  */
    qapi_ipc_ring_type_t    ring_type;  /**< Ring type */
} qapi_ipc_tx_msg_t;


/** Prototype for client callback which it needs to register to request buffer.
 *  The received message will be copied into this buffer. This function is
 *  called with interrupt disabled.
 *
 * @param[in]   rx_get_buf_cb_arg  Client argument passed during registration
 * @param[in]   ipc_msg            IPC message
 *
 * @return  Pointer to the receive buffer
 *
 * @note  The address in msg will be translated address pointing to the
 *        message payload in peer subsystem's memory.
 */
typedef uint8_t * (*QAPI_IPC_RX_GET_BUF_CB)( void * rx_get_buf_cb_arg,
                                             const qapi_ipc_msg_t * ipc_msg );

/** Prototype for client callback which it needs to register for handling
 *  the received message. This function is called on interrupt blocked, 
 *  so it should avoid heavy processing time. Ideally, the callback function
 *  should send a notification to the client so as to consume the message
 *  at later point of time.
 *
 * @param[in]   rx_msg_cb_arg   Client context which was passed during registration
 * @param[in]   ipc_msg         IPC message
 * @param[in]   payload         IPC message payload (if !NULL)
 *
 * @return  None.
 * 
 * @note pPayload param will be NULL for a zero-length message. It will be
 *       NULL in case of ACK for a message sent earlier.
 */
typedef void (*QAPI_IPC_RX_MSG_CB)( void * rx_msg_cb_arg,
                                    const qapi_ipc_msg_t * ipc_msg,
                                    const uint8 *payload );

/** Client callback information */
typedef struct
{
    struct
    {
        QAPI_IPC_RX_GET_BUF_CB  cb;     /**< Callback to get Rx buffer */
        void *                  cb_arg; /**< Argument for get Rx Buf callback */
    } rx_get_buf;

    struct
    {
        QAPI_IPC_RX_MSG_CB      cb;     /**< Callback for Rx message */
        void *                  cb_arg; /**< Argument for Rx message callback */
    } rx_msg;
    qapi_ipc_msg_t              *cb_msg;/**< Msg pointer client to provide*/
} qapi_ipc_client_cb_t;


/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
/**********************************************************************//**
 * Registers a client with the IPC driver.
 *
 * @param[in]   client          Client ID
 * @param[in]   pClientCb       Client callback info
 *
 * @return  TRUE if client got registered, else FALSE.
 */
extern boolean qapi_IPC_RegisterClient
(
    qapi_ipc_client_t client,
    qapi_ipc_client_cb_t * client_cb
);

/**********************************************************************//**
 * De-registers a client from the IPC driver
 *
 * @param[in]   client          Client ID
 *
 * @return  TRUE if client is de-registered, else FALSE.
 */
extern boolean qapi_IPC_DeRegisterClient
(
    qapi_ipc_client_t client
);

/**********************************************************************//**
 * Allocate memory for IPC message from specifc region.
 *
 * @param[in]   size  Allocation size
 *
 * @return  A pointer to the allocated block, or NULL if the block
 *          could not be allocated.
 */
extern uint8_t * qapi_IPC_malloc
(
    size_t size
);

/**********************************************************************//**
 * Deallocate a block of memory and returns it to the dedicted heap for IPC.
 *
 * @param[in]   ptr         A pointer to the memory block that needs to be freed
 *
 * @return  None.
 */
extern void qapi_IPC_free( uint8_t * ptr );

/**********************************************************************//**
 * Sends IPC message to peer processor.
 *
 * @param[in]   txMsg       Pointer to Tx message
 *
 * @return  Status of operation.
 *
 * @note The client MUST allocate memory for the message payload by
 *       calling @ref qapi_IPC_malloc API.
 *
 *       The message buffer MUST be freed by the client once it receives
 *       ACK from the peer subsystem.
 */
extern qapi_ipc_err_status_t qapi_IPC_TxMsg
(
    qapi_ipc_tx_msg_t * tx_msg
);

#endif /* __QAPI_IPC_H__ */
