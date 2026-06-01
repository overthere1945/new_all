/**************************************************************************
 * @file     qapi_bt_utilities.h
 * @brief    QAPI BT Utilities header file.
 * 
 * This file contains the Inteerface for 
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef QAPI_BT_UTILITIES_H
#define QAPI_BT_UTILITIES_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/


/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
/******************************************************************************
 * @struct bt_utils_timesync_req
 * @brief Structure for time sync request.
 ******************************************************************************/
typedef struct __attribute__((packed)) {
    uint16_t connHandle;                                   /**< Connection Handle */
    uint8_t  config;                                      /**< Config */
} bt_utils_timesync_req_t;

/******************************************************************************
 * @struct bt_utils_timesync_rsp
 * @brief Structure for time sync response.
 ******************************************************************************/
typedef struct __attribute__((packed)) {
    uint8_t  status;        /**< Status. */
    uint8_t  subOpcode;     /**< Sub-op code. */
    uint8_t  config;        /**< Configuration flags or value. */
    uint64_t appClock;      /**< Application clock value (timestamp). */
    uint64_t btClock;       /**< Bluetooth clock value (timestamp). */
    uint8_t  numHandles;    /**< Number of handles. */
    uint16_t connHandle;    /**< Connection handle identifier. */
    uint16_t evtCounter;    /**< Event counter. */
    uint32_t timeOffset;    /**< Time offset. */
    uint16_t interval;      /**< Connection interval. */
    uint16_t bandwidth;     /**< Connection bandwidth. */
} bt_utils_timesync_rsp_t;


/**
 * @brief Sends a timesync request on a BLE connection.
 *
 *
 * @param connHandle Bluetooth connection handle.
 * @param config     .
 *
 * @return true if the notification was sent successfully, false otherwise.
 */
bool qapi_bt_utils_timesync_req(uint16_t connHandle, uint8_t config);

#if defined(CONFIG_LPAI_BTSW_ENABLE_ISLAND_TEST)
void qapi_bt_utils_enter_exit_island_req(bool action , uint8_t *secret_pw);
#endif


#endif /** QAPI_BT_UTILITIES_H*/