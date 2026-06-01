/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/


/*===========================================================================
File        offload_mgr_log.h
===========================================================================*/
/**
 * @file offload_mgr_log.h
 * @brief defines macros used for offload manager logging
 * @details This file defines the macros used for offload manager logging. 
 * 	    Please refer bt_pal_log.h file for more details. 
 */

#ifndef OFFLOAD_MGR_LOG_H
#define OFFLOAD_MGR_LOG_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "bt_pal_log.h"

/*===========================================================================
                        MACRO DEFENITIONS
===========================================================================*/
#define OFFLOAD_MGR_LOGL BT_PAL_LOGL
#define OFFLOAD_MGR_LOGM BT_PAL_LOGM
#define OFFLOAD_MGR_LOGH BT_PAL_LOGH
#define OFFLOAD_MGR_LOGF BT_PAL_LOGF
#define OFFLOAD_MGR_LOGE BT_PAL_LOGE

/**
 * @brief Macro for logging fatal errors in the Offload Manager.
 *
 * @param fmt Format string for the log message.
 * @param ... Additional arguments for the format string.
 */
#define OFFLOAD_MGR_FATAL(fmt, ...) \
	do { \
		OFFLOAD_MGR_LOGF(fmt,##__VA_ARGS__); \
		assert(0); \
	}while(0)


/**
 * @brief Macro for asserting fatal conditions in the Offload Manager.
 *
 * @param x Condition to assert.
 */
#define OFFLOAD_MGR_ASSERT_FATAL(x) \
	do { \
		if(!(x)) { \
			OFFLOAD_MGR_FATAL("fatal"); \
		} \
	}while(0)

#define SOCKET_MGR_LOG_SOCKET_ID(value) \
do { \
} while (0)

#define ENDPT_MGR_LOG_RPC_MSG(rpc_msg) \
do { \
OFFLOAD_MGR_LOGL("endpt_mgr_handle_evt %d %d %d %d\n", \
     rpc_msg->command_type, rpc_msg->opcode, \
     rpc_msg->data_len, rpc_msg->message_format); \
}while(0)

#endif /* OFFLOAD_MGR_LOG_H */
