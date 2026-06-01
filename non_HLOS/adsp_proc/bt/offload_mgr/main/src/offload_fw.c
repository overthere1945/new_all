/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_fw.c
===========================================================================*/
/**
 * @file offload_fw.c
 * @brief Offload framework entry point file.
 *
 * @details This file serves as the entry point for the offload framework.
 */

/*===========================================================================
						INCLUDE FILES
===========================================================================*/
#include "offload_mgr_thread.h"
#include "string.h"
#include "lpai_transport.h"

/*=============================================================================
					GLOBAL DATA DECLARATIONS
=============================================================================*/
/**
 * @brief Structure for Offload Framework Task.
 */
offload_fw_task offload_fw_tasks[] = {
	{
		/* offload mgr */
		"offload_mgr",
		OFFLOAD_MGR_THREAD_STACK_PRIORITY,
		OFFLOAD_MGR_THREAD_STACK_SIZE,
		Offload_Mgr_Init,
		Offload_Mgr_Start,
		NULL,
	},
#ifdef LPAI_TRANSPORT_THREAD_ENABLED
	{
		/* lpai transport */
		"lpai_transport",
		LPAI_TRANSPORT_THREAD_STACK_PRIORITY,
		LPAI_TRANSPORT_THREAD_STACK_SIZE,
		LPAI_Transport_Init,
		LPAI_Transport_Start,
		NULL,
	},
#endif
	/* any other tasks */

};

/*=============================================================================
					FUNCTION DEFINITIONS
=============================================================================*/

/*===========================================================================
FUNCTION      Offload_Fw_Init
===========================================================================*/
/**
 * @brief Initializes the Offload Framework.
 *
 * @details Initializes the Offload Framework by starting the
 * framework tasks at the time of boot.
 */
void Offload_Fw_Init(void)
{
	uint8_t num_tasks = 0;
	void (*init_fn)(void);
	qurt_thread_t (*start_fn)(char *, const int, const int);
	char task_name[20];
	size_t task_name_len = 0;
	offload_fw_task *task;
	/* init and start all the tasks in offload_fw_tasks array */
	num_tasks = sizeof(offload_fw_tasks) / sizeof(offload_fw_task);
	for (int i = 0; i < num_tasks; i++)
	{
		task = &offload_fw_tasks[i];
		init_fn = task->init_fn;
		start_fn = task->start_fn;
		task_name_len = strlen(task->thread_name);
		memcpy(task_name, task->thread_name, task_name_len);
		task_name[task_name_len] = '\0'; //Append NULL termination
		init_fn();
		task->handle = start_fn(task_name, task->thread_priority, task->thread_stack_size);
	}
}
 