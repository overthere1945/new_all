/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_fw.h
===========================================================================*/
/**
 * @file offload_fw.h
 * @brief Exposes the init function of the offload framework to the build system.
 *
 * @details Contains the declarations necessary for initializing the offload framework,
 *          making it accessible to the build system.
 */

#ifndef OFFLOAD_FW_H
#define OFFLOAD_FW_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "qurt_pipe.h"
#include "qurt_error.h"
#include "qurt_thread.h"
#include "qurt_signal.h"
#include "lpai_transport.h"
#include "offload_mgr_thread.h"

/*=============================================================================
                        MACRO DEFINITIONS
=============================================================================*/
/**
 * @brief Maximum number of tasks in the Offload Framework.
 */
#define OFFLOAD_FW_TASKS_MAX                    2

/*=============================================================================
                        TYPE DEFINITIONS
=============================================================================*/
/**
 * @brief Structure for Offload Framework Task.
 */
 typedef struct offload_fw_task {
    const char* thread_name;                                /**< Name of the thread. */
    const int thread_priority;                              /**< Priority of the thread. */
    const int thread_stack_size;                            /**< Stack size of the thread. */
    void (*init_fn)(void);                                  /**< Initialization function. */
    qurt_thread_t (*start_fn)(char*, const int, const int); /**< Start function. */
    qurt_thread_t handle;                                   /**< Handle for the thread. */
} offload_fw_task;

/*=============================================================================
                    GLOBAL FUNCTION DECLARATIONS
=============================================================================*/
/**
 * @brief Initializes the Offload Framework.
 */
void Offload_Fw_Init(void);

#endif /* OFFLOAD_FW_H */