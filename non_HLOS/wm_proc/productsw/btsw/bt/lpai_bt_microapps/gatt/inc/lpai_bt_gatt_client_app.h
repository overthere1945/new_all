/**************************************************************************//**
 * @file     lpai_bt_gatt_client_app.h
 * @brief    Header file for the LPAI Bluetooth GATT Client Application.
 *
 * This header defines configuration constants and macros used by
 * the LPAI BT GATT Client App.
 *
 * @copyright
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_GATT_CLIENT_APP_H
#define LPAI_BT_GATT_CLIENT_APP_H

#include "qapi_bt_gatt.h"

/**
 * @def GATT_CLIENT_APP_HUB_ID
 * @brief Unique identifier for the GATT Client App hub.
 */
#define GATT_CLIENT_APP_HUB_ID              0xFBFBFBFBFBFBFB0A

/**
 * @def GATT_CLIENT_APP_EP_ID
 * @brief Unique endpoint ID for the GATT Client App.
 */
#define GATT_CLIENT_APP_EP_ID               0xFAFAFAFAFAFAFA08

/**
 * @def GATT_CLIENT_APP_EP_NAME
 * @brief String name for the GATT Client App endpoint.
 */
#define GATT_CLIENT_APP_EP_NAME             "uClientTestApp"

/**
 * @def GATT_CLIENT_APP_EP_SVC_NAME
 * @brief String name for the GATT Client App service.
 */
#define GATT_CLIENT_APP_EP_SVC_NAME         "GATT_CLIENT_TEST_APP_SERVICE"

/**
 * @def CLIENT_APP_BULK_TRANSFER
 * @brief Macro used for enabling bulk transfer for test purpose.
 */
#define CLIENT_APP_BULK_TRANSFER

/**
 * @brief Function to send GATT Client Write Request or Command to peer device.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attrHandle  GATT characteristic handle.
 * @param valLen     GATT characteristic value len.
 * @param write_cmd  Request is write request or write without response (write_cmd).
 *
 * @return qapi_gatt_result_t
 */
qapi_gatt_result_t gatt_client_app_send_write_req(int32_t sessionId, uint16_t attrHandle, uint16_t valLen, bool write_cmd);

/**
 * @brief DeInitializes the GATT Client Microapp Structures.
 *
 * This function resets the necessary configurations, data structures, and
 * resources required to start the Gatt Client Gatt Structures.
 *
 * @note This function does not take any parameters and does not return a value.
 */
void gatt_client_uapp_deinit();

#ifdef CLIENT_APP_BULK_TRANSFER
/**
 * @brief Function to initiate Tx bulk transfer for GATT Client app. 
 * This function sends writeWithoutResponse to peer device for pktCnt times.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attHandle  GATT characteristic handle.
 * @param pktCnt  Packet Counter to repeat bulk transfer.
 * @param valLen     GATT characteristic value len.
 *
 * @return None
 */
void gatt_client_app_tx_bulk_transfer(int32_t sessionId, uint16_t attHandle, uint16_t pktCnt, uint16_t valLen);


/**
 * @brief Function to initiate Rx bulk transfer for GATT Client app. 
 * This function receives Notifications from peer device for pktCnt times.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attHandle  GATT characteristic handle.
 * @param pktCnt  Packet Counter to repeat bulk transfer.
 *
 * @return None
 */
void gatt_client_app_rx_bulk_transfer(int32_t sessionId, uint16_t attHandle, uint16_t pktCnt);


#endif /* CLIENT_APP_BULK_TRANSFER */

#endif /*LPAI_BT_GATT_CLIENT_APP_H */
