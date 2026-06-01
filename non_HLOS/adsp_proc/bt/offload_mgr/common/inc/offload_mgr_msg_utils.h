/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/


/*===========================================================================
File        offload_mgr_proto_utils.h
===========================================================================*/
/**
 * @file offload_mgr_proto_utils.h
 * @brief Common proto utilities used by the offload framework.
 *
 * @details This file contains utility functions and definitions that are commonly used 
 *            within the offload framework for handling proto messages.
 */

#ifndef OFFLOAD_MGR_MSG_UTILS_H
#define OFFLOAD_MGR_MSG_UTILS_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>


/*===========================================================================
                        EVENTS FOR INTERMODULE COMMUNICATION
===========================================================================*/

/*===========================================================================
DEFINES      BT Offload Socket HAL Proto Messages
===========================================================================*/
/**
 * @brief Proto messages coming from BT offload Socket HAL.
 *
 * @details These opcodes are used for both request and response events.
 *
 * @define BT_OFFLOAD_OPEN_SOCKET
 * @brief Opcode for socket open request and response.
 */
#define BT_OFFLOAD_OPEN_SOCKET                              0x0F03    

/**
 * @define BT_OFFLOAD_CLOSE_SOCKET
 * @brief Opcode for socket close request and response.
 */
#define BT_OFFLOAD_CLOSE_SOCKET                             0x0F04

/**
 * @define BT_OFFLOAD_SOCKET_CLOSE_IND
 * @brief Opcode for socket close indication.
 */
#define BT_OFFLOAD_SOCKET_CLOSE_IND                         0x0F05

/**
 * @define BT_OFFLOAD_SOCKET_CAPS
 * @brief Opcode for socket capabilities request and response.
 */
#define BT_OFFLOAD_SOCKET_CAPS                              0x0F06
          

#endif /* OFFLOAD_MGR_MSG_UTILS_H */