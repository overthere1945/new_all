/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      lpai_transport_log.h
===========================================================================*/
/**
 * @file lpai_transport_log.h
 * @brief  Lpai transport logging
 *
 * @details 
 * Handles the internal logging files used in lpai transport. 
 */
#ifndef LPAI_TRANSPORT_LOG_H
#define LPAI_TRANSPORT_LOG_H

/*============================================================================*
                           INCLUDE FILES
*============================================================================*/
#include "bt_pal_log.h"

/*===========================================================================
                           MACRO DEFINITIONS
===========================================================================*/
#define LPAI_TRANSPORT_LOGL BT_PAL_LOGL
#define LPAI_TRANSPORT_LOGM BT_PAL_LOGM
#define LPAI_TRANSPORT_LOGH BT_PAL_LOGH
#define LPAI_TRANSPORT_LOGF BT_PAL_LOGF
#define LPAI_TRANSPORT_LOGE BT_PAL_LOGE


/**
 * @brief Logs a sequence of bytes in hexadecimal format.
 *
 * This macro iterates over a given data buffer and logs each byte
 * in two-digit hexadecimal format using the BT_PAL_LOGM logging function.
 *
 * @param data Pointer to the data buffer to be logged.
 * @param data_len Number of bytes to log from the data buffer.
 *
 * Example usage:
 * @code
 * uint8_t buffer[] = {0x01, 0xAB, 0xFF};
 * LPAI_TRANSPORT_LOG_BYTES(buffer, sizeof(buffer));
 * @endcode
 */

#define LPAI_TRANSPORT_LOG_BYTES(data, data_len) \
 do { \
   for (int i = 0; i < data_len; i++) { \
     BT_PAL_LOGM("%02x", ((uint8_t *)data)[i]); \
   } \
 } while(0)
#endif /* LPAI_TRANSPORT_LOG_H */
