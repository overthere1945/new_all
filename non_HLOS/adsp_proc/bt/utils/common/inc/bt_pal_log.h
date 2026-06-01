/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      bt_pal_log.h
===========================================================================*/
/**
 * @file bt_pal_log.h
 * @brief Header file for logging in the BT PAL.
 *
 * @details This file defines logging macros and log level configurations
 *          used throughout the BT PAL. It provides a
 *          consistent interface for logging messages at various severity levels.
 */

#ifndef BT_PAL_LOG_H
#define BT_PAL_LOG_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "qurt.h"
#include "msg_diag_service.h"
#include "msg.h"
#include <assert.h>
// #include "log.h"
#include "micro_msg_diag_service.h"

/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/

/**
 * @brief Alias for logging messages with no arguments.
 */
#define MSG_0 MSG

/**
 * @brief Logging levels for BT PAL logs.
 */
#define BT_PAL_LOG_LEVEL_LOW   2 /**< Lowest priority: verbose/debug logs */
#define BT_PAL_LOG_LEVEL_MED   1 /**< Medium priority: informational logs */
#define BT_PAL_LOG_LEVEL_HIGH  0 /**< Highest priority: critical logs */

/**
 * @brief Default log level if not defined elsewhere.
 */
#ifndef BT_PAL_LOG_LEVEL
#define BT_PAL_LOG_LEVEL BT_PAL_LOG_LEVEL_HIGH
#endif

/**
 * @brief Generic logging macro that selects the appropriate MSG macro
 *        based on the number of arguments.
 *
 * @param prio   Logging priority (e.g., HIGH, MED, LOW, ERROR, FATAL)
 * @param xx_fmt Format string
 * @param ...    Optional arguments for the format string
 */
#define BT_PAL_LOG(prio, xx_fmt, ...)  MICRO_MSG(MSG_SSID_BT_PAL, MSG_LEGACY_##prio, xx_fmt, ##__VA_ARGS__)
// \
//   _MSG_JOIN(MSG_, MSG_NARG(xx_fmt, ##__VA_ARGS__)) \
//   (MSG_SSID_BT_PAL, \
//    MSG_LEGACY_##prio, \
//    xx_fmt, \
//    ##__VA_ARGS__)

/**
 * @name BT_PAL_LOGX Macros
 * @brief Convenience macros for logging at specific severity levels.
 * @{
 */
#define BT_PAL_LOGF(fmt, ...) BT_PAL_LOG(FATAL, fmt, ##__VA_ARGS__) /**< Fatal error log */
#define BT_PAL_LOGE(fmt, ...) BT_PAL_LOG(ERROR, fmt, ##__VA_ARGS__) /**< Error log */

#if BT_PAL_LOG_LEVEL >= BT_PAL_LOG_LEVEL_HIGH
  #define BT_PAL_LOGH(fmt, ...) BT_PAL_LOG(HIGH, fmt, ##__VA_ARGS__) /**< High priority log */
#else
  #define BT_PAL_LOGH(fmt, ...) do{}while(0)
#endif

#if BT_PAL_LOG_LEVEL >= BT_PAL_LOG_LEVEL_MED
  #define BT_PAL_LOGM(fmt, ...) BT_PAL_LOG(MED, fmt, ##__VA_ARGS__) /**< Medium priority log */
#else
  #define BT_PAL_LOGM(fmt, ...) do{}while(0)
#endif

#if BT_PAL_LOG_LEVEL >= BT_PAL_LOG_LEVEL_LOW
  #define BT_PAL_LOGL(fmt, ...) BT_PAL_LOG(LOW, fmt, ##__VA_ARGS__) /**< Low priority log */
#else
  #define BT_PAL_LOGL(fmt, ...) do{}while(0)
#endif
/** @} */

/**
 * @brief Logs a sequence of bytes in hexadecimal format.
 *
 * @param data     Pointer to the byte array.
 * @param data_len Number of bytes to log.
 *
 * @note This macro logs each byte individually using BT_PAL_LOGL.
 */
#define BT_PAL_LOG_BYTES(data, data_len) \
 do { \
   for (int i = 0; i < data_len; i++) { \
     BT_PAL_LOGL("%02x", ((uint8_t *)data)[i]); \
   } \
 } while(0)

#endif /* BT_PAL_LOG_H */
