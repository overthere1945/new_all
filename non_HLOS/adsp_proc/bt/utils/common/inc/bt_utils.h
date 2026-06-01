/*=============================================================================
  @file bt_utils.h

  @brief Handles the bt utils requests from AWM.

*******************************************************************************
* Copyright (c) 2025 Qualcomm Technologies, Inc.  All Rights Reserved
* Qualcomm Technologies Confidential and Proprietary.
********************************************************************************/
#ifndef BT_UTILS_H
#define BT_UTILS_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "stdint.h"
#include "qurt_signal.h"

//#define SUPPORT_POC_SETUP

#define IGNORE_UNUSED(param) (void)(param)

#define BT_ISLAND_PROXY_VOTE_SUPPORT 


/*=============================================================================
                        TYPE DEFINITIONS
=============================================================================*/
/**
 * @enum bt_utils_cmds_t
 * BT Utils Commands and Response for Communication between AWM and ADSP
 */
typedef enum{
    BT_UTILS_CMD_START = 0x7000,
    BT_UTILS_TIME_SYNC = BT_UTILS_CMD_START,
    BT_UTILS_ENTER_ISLAND,
    BT_UTILS_EXIT_ISLAND,
    BT_UTILS_TIME_SYNC_RSP = 0x7200,
    BT_UTILS_CMD_MAX,
}bt_utils_cmds_t;


/**
 * @brief Structure for BT timesync.
 * This structure holds the hardware timestamp value used for Bluetooth time synchronization.
 */
typedef struct {
    uint64_t hw_timestamp; /**< Hardware timestamp */
    qurt_signal_t signal;
} bt_timesync_t;

/**
 * @brief BT Utils structure.
 * This structure encapsulates Bluetooth time synchronization information.
 */
typedef struct {
    bt_timesync_t timesync; /**< Bluetooth time synchronization data. */
} bt_utils_t;


 /**
 * @struct bt_utils_timesync_rsp_t
 * @brief Structure representing the Bluetooth time synchronization response.
 *
 * This packed structure contains all relevant fields returned as part of a
 * Bluetooth time synchronization response, including clock values, connection
 * parameters, and status information.
 */
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
 * @struct bt_utils_timesync_req
 * @brief Structure for time sync request.
 */
typedef struct __attribute__((packed)) {
    uint16_t connHandle;                                   /**< Connection Handle */
    uint8_t  config;                                      /**< Config */
} bt_utils_timesync_req_t;

/*=============================================================================
                    GLOBAL FUNCTION DECLARATIONS
=============================================================================*/ 

/**
 * @brief Fetch a 2-byte little-endian value from a byte array.
 *
 * This static inline function reads two bytes from the provided array
 * (assumed to be in little-endian order) and returns the corresponding
 * 16-bit unsigned integer.
 *
 * @param arr Pointer to the byte array (minimum 2 bytes).
 * @return 16-bit unsigned integer value constructed from arr[0] and arr[1].
 */
static inline uint16_t FETCH_2_BYTE_LITTLE_ENDIAN_FROM_ARR(void *arr)
{
    uint8_t *data = (uint8_t *)arr;
    return ((uint16_t)data[0] | (((uint16_t)data[1]) << 8U));
}

/**
 * @brief Initialize Bluetooth utilities.
 *
 * This function performs necessary initialization for Bluetooth utility operations.
 */
void bt_utils_init(void);

/**
 * @brief Deinitialize Bluetooth utilities.
 *
 * This function cleans up resources used by Bluetooth utility operations.
 */
void bt_utils_deinit(void);

/**
 * @brief Handle a Bluetooth utility command.
 *
 * This function processes a Bluetooth utility command based on the provided opcode,
 * length, and command data.
 *
 * @param opcode Command operation code.
 * @param len Length of the command data.
 * @param cmd Pointer to the command data.
 */
void bt_utils_cmd_handler(uint16_t opcode, uint16_t len, void *cmd);

#endif /* BT_UTILS_H */
