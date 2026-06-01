/******************************************************************************
 * @file     lpai_bt_gatt_server_app.h
 * @brief    LPAI BT GATT Server Application header file.
 *
 * This file contains macros, type definitions, and constants related to the
 * Bluetooth GATT Server reference micro application for LPAI platforms.
 * It provides necessary identifiers, service names, and data structures to
 * enable and manage GATT server functionality.
 *
 * @copyright
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

#ifndef LPAI_BT_GATT_SERVER_APP_H
#define LPAI_BT_GATT_SERVER_APP_H

/** Program Specific Header file Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <stringl.h>
#include "qapi_bt_gatt.h"

/** @def GATT_SERVER_APP_HUB_ID
 *  @brief Unique identifier for the GATT Server Application hub.
 */
#define GATT_SERVER_APP_HUB_ID              0xFBFBFBFBFBFBFB0A

/** @def GATT_SERVER_APP_EP_ID
 *  @brief Unique endpoint ID for the GATT Server Application.
 */
#define GATT_SERVER_APP_EP_ID               0xFAFAFAFAFAFAFA07

/** @def GATT_SERVER_APP_EP_NAME
 *  @brief String literal for the GATT Server Application endpoint name.
 */
#define GATT_SERVER_APP_EP_NAME             "uServerTestApp"

/** @def GATT_SERVER_APP_EP_SVC_NAME
 *  @brief String literal for the GATT Server Application service name.
 */
#define GATT_SERVER_APP_EP_SVC_NAME         "GATT_SERVER_TEST_APP_SERVICE"

/** @def GATT_SERVER_APP_MAX_SESSIONS
 *  @brief Maximum number of concurrent GATT sessions supported by the server app.
 */
#define GATT_SERVER_APP_MAX_SESSIONS       GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS

/**
 * @def SERVER_APP_BULK_TRANSFER
 * @brief Macro used for enabling bulk transfer for test purpose.
 */
#define SERVER_APP_BULK_TRANSFER

/** @struct server_app_gatt_char
 *  @brief Structure representing a GATT characteristic instance on the server.
 *
 *  This struct holds metadata for a single characteristic, including its GATT
 *  database handle, value length, and pointer to its value data.
 */
typedef struct server_app_gatt_char
{
    uint16_t handle;       /**< Attribute handle assigned by GATT DB. */
    uint16_t val_len;      /**< Current length of the characteristic value. */
    uint8_t *val;          /**< Pointer to value buffer for the characteristic. */
} server_app_gatt_char_t;


/** @struct server_app_gatt_session
 *  @brief Structure representing a single GATT server session.
 *
 *  Encapsulates details needed to keep track of a session's state and
 *  its characteristics list.
 */
typedef struct server_app_gatt_session 
{
    int32_t sessionId;                  /**< Unique session identifier. */
    uint16_t num_chars;                 /**< Number of characteristics in the session. */
    server_app_gatt_char_t *char_list;  /**< Pointer to list of session's characteristics. */
} server_app_gatt_session_t;

/**
 * @brief Function to send GATT Server Characteristic Notfication to peer device.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attrHandle  GATT characteristic handle.
 * @param valLen     GATT characteristic value len.
 *
 * @return qapi_gatt_result_t
 */
qapi_gatt_result_t gatt_server_app_notif_send(int32_t sessionId, uint16_t attrHandle, uint16_t valLen);

/**
 * @brief DeInitializes the GATT Server Microapp Structures.
 *
 * This function resets the necessary configurations, data structures, and
 * resources required to start the Gatt Server Gatt Structures.
 *
 * @note This function does not take any parameters and does not return a value.
 */
void gatt_server_uapp_deinit();

#ifdef SERVER_APP_BULK_TRANSFER
/**
 * @brief Function to initiate Tx bulk transfer for GATT Server app. 
 * This function sends GATT Notifications to peer device for pktCnt times.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attHandle  GATT characteristic handle.
 * @param pktCnt  Packet Counter to repeat bulk transfer.
 * @param valLen     GATT characteristic value len.
 *
 * @return None
 */
void gatt_server_app_tx_bulk_transfer(int32_t sessionId, uint16_t attHandle, uint16_t pktCnt, uint16_t valLen);

/**
 * @brief Function to initiate Rx bulk transfer for GATT Server app. 
 * This function receives GATT writeWithoutResponse from peer device for pktCnt times.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attHandle  GATT characteristic handle.
 * @param pktCnt  Packet Counter to repeat bulk transfer.
 *
 * @return None
 */
void gatt_server_app_rx_bulk_transfer(int32_t sessionId, uint16_t attHandle, uint16_t pktCnt);



#endif /* SERVER_APP_BULK_TRANSFER */

#endif /*LPAI_BT_GATT_SERVER_APP_H */

