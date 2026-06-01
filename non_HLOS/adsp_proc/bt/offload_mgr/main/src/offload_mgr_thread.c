/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2026 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_thread.c
===========================================================================*/
/**
 * @file offload_mgr_thread.c
 * @brief Offload manager task file.
 *
 * @details This file contains the implementation of tasks  and handling of
 *           posted by App manager on AWM, Socket HAL and Context Hub HAL
 *           on APSS and Microstack sitting on Q6.
 */

/*============================================================================*
								INCLUDE FILES
*============================================================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "qurt_pipe.h"
#include "qurt_thread.h"
#include "offload_mgr_thread.h"
#include "socket_mgr.h"
#include "bt_state_mgr.h"
#include "endpt_mgr.h"
#include "bt_utils.h"
#include "hci_arbiter_lib.h"
#include "lpai_transport.h"
#include "offload_mgr_client_interface.h"
#include "bt_pal_heap.h"
#include "bt_pal_npa.h"
#ifdef ENABLE_WLAN_TSF_GPIO
#include "wlan_gpio.h"
#endif

/*=============================================================================
					MACRO DEFINITIONS
=============================================================================*/

/*=============================================================================
					EXTERN APIs/VARIABLES
=============================================================================*/
/*=============================================================================
					GLOBAL DATA DECLARATIONS
=============================================================================*/
offload_mgr_main_t offload_mgr_main;
const char *offload_mgr_thread_name = "offload_mgr";

/*=============================================================================
					LOCAL FUNCTION DEFINITIONS
=============================================================================*/
/*===========================================================================
FUNCTION      OffloadMgr_Thread_Bootstrap
===========================================================================*/
/**
 * @brief Initializes components required before the offload manager thread runs.
 */
static void OffloadMgr_Thread_Bootstrap()
{
	/* Platform Inits */
	bt_pal_heap_mgr_init();

#ifndef LPAI_TRANSPORT_THREAD_ENABLED
	/** If LPAI transport is not using a separate thread,
	*  initialize it from here.
	*/
	/* TBD: Disabled to prevent crash, to be checked post fix on mproc glink */
	LPAI_Transport_Init();
#endif

	bt_state_mgr_init();
}

/*=============================================================================
					GLOBAL FUNCTION DEFINITIONS
=============================================================================*/
/**
 * @brief Deinitializes the Offload Manager.
 */
void Offload_Manager_Deinit(void)
{
}

/*===========================================================================
FUNCTION      Offload_Mgr_Init
===========================================================================*/
/**
 * @brief Initializes the Offload Manager.
 */
void Offload_Mgr_Init(void)
{
	/* thread configuration */
    qurt_pipe_attr_t pipe_attr;
 
    qurt_pipe_attr_init(&pipe_attr);
    qurt_pipe_attr_set_elements(&pipe_attr, OFFLOAD_MGR_PIPE_ELEMENT_COUNT);
    qurt_pipe_attr_set_buffer(&pipe_attr, offload_mgr_main.offload_mgr_pipe_buffer);
	qurt_pipe_attr_set_buffer_partition(&pipe_attr, QURT_THREAD_ATTR_TCB_PARTITION_TCM);
    qurt_pipe_init(&(offload_mgr_main.offload_mgr_pipe), &pipe_attr);
}

/*===========================================================================
FUNCTION      Offload_Mgr_Start
===========================================================================*/
/**
 * @brief Starts the Offload Manager thread.
 */
qurt_thread_t Offload_Mgr_Start(char *thread_name,
								const int thread_priority,
								const int thread_stack_size)
{
	qurt_thread_attr_t th_attr;
	qurt_thread_t th_hndl;
	int res;
	offload_mgr_main.thread_running = false;

	qurt_thread_attr_init(&th_attr);
	qurt_thread_attr_set_name(&th_attr, (char *)thread_name);
	qurt_thread_attr_set_priority(&th_attr, thread_priority);
	qurt_thread_attr_set_stack_addr(&th_attr, offload_mgr_main.offload_mgr_stack);
	qurt_thread_attr_set_stack_size(&th_attr, thread_stack_size);
	qurt_thread_attr_set_tcb_partition (&th_attr, QURT_THREAD_ATTR_TCB_PARTITION_TCM);
	res = qurt_thread_create(
		&th_hndl,
		&th_attr,
		Offload_Mgr_Task_Handler,
		&(offload_mgr_main.offload_mgr_pipe));

	if (res == QURT_EOK)
	{
		offload_mgr_main.thread_running = true;
		offload_mgr_main.thread_handle = th_hndl;
		return (th_hndl);
	}
	else
	{
		return (NULL);
	}
}

/*===========================================================================
FUNCTION      Offload_Mgr_Post_Event
===========================================================================*/
/**
 * @brief Posts an event to the Offload Manager.
 */
void Offload_Mgr_Post_Event(OFFLOAD_MGR_MSG_OPCODES opcode, uint8_t *data_ptr, size_t len)
{
	int rc = QURT_EOK;
	offload_mgr_pipe_msg_t *pipe_event = bt_pal_malloc(sizeof(offload_mgr_pipe_msg_t));
	if (pipe_event == NULL)
    {
        BT_PAL_ASSERT_FATAL("Offload_Mgr_Post_Event: malloc failed with opcde:%x, ", opcode, 0, 0);
        return;
    }

	pipe_event->pipe_msg_opcode = opcode;
	pipe_event->data_ptr = data_ptr;
	pipe_event->size = len;

	rc = qurt_pipe_send_cancellable(&(offload_mgr_main.offload_mgr_pipe), (qurt_pipe_data_t)pipe_event);
	OFFLOAD_MGR_ASSERT_FATAL(rc == QURT_EOK);
}


/*===========================================================================
FUNCTION      log_pipe_evt
===========================================================================*/
/**
 * @brief Logs the contents of an offload manager pipe event.
 *
 * This function takes a pointer to an offload manager pipe event structure as input,
 * and logs the size and opcode of the event, as well as the contents of the event data.
 *
 * @param[in] pipe_evt  Pointer to the offload manager pipe event structure to be logged.
 * @return              None.
 */
static void log_pipe_evt(offload_mgr_pipe_msg_t *pipe_evt) {
	if(pipe_evt == NULL)return;
	OFFLOAD_MGR_LOGM("Offload_Mgr_Task_Handler: size:%d opcode:%d\n", pipe_evt->size, 
	pipe_evt->pipe_msg_opcode);

#ifndef LPAI_TRANSPORT_THREAD_ENABLED
	/** don't print the data for lpai transport : can create confusion 
	 *  its handled separately in glink module */
	if(pipe_evt->pipe_msg_opcode == LPAI_TRANSPORT_MSG) {
		return;
	}
#endif
	for (int i = 0; i < pipe_evt->size; i++)
	{
		OFFLOAD_MGR_LOGL("data[%d] = %02x\n", i, ((uint8_t* )pipe_evt->data_ptr)[i]);
	}
}

/*===========================================================================
FUNCTION      Offload_Mgr_Task_Handler
===========================================================================*/
/**
 * @brief Task handler routine for the Offload Manager thread.
 */
void Offload_Mgr_Task_Handler(void *param)
{
	qurt_pipe_t *offload_mgr_pipe = (qurt_pipe_t *)param;
	offload_mgr_pipe_msg_t *pipe_evt = NULL;

	OffloadMgr_Thread_Bootstrap();

	while (offload_mgr_main.thread_running)
	{
		pipe_evt = (offload_mgr_pipe_msg_t *)qurt_pipe_receive(offload_mgr_pipe);

		log_pipe_evt(pipe_evt);

		switch (pipe_evt->pipe_msg_opcode)
		{
		case OFFLOAD_MGR_MSG_GLINK_SOCKET_OFFLOAD_RX:
		case OFFLOAD_MGR_MSG_GLINK_GATT_OFFLOAD_RX:
            /* Fall through */
		{
			/* todo : need to recheck this */
			uint8_t *bt_evt = (uint8_t *)pipe_evt->data_ptr;
			uint16_t hal_event = FETCH_2_BYTE_LITTLE_ENDIAN_FROM_ARR(bt_evt);
			/* all messages from socket/gatt hal are proto encoded skipping that part */
			uint16_t bt_evt_len = FETCH_2_BYTE_LITTLE_ENDIAN_FROM_ARR(bt_evt + 4);

			offload_mgr_evt_handler(hal_event, (uint8_t *)(&bt_evt[6]), bt_evt_len);
			bt_pal_free((void *)pipe_evt->data_ptr);
		}
		break;
#ifndef LPAI_TRANSPORT_THREAD_ENABLED
		case LPAI_TRANSPORT_MSG:
		{
			LPAI_Transport_Handle_Event((void *)pipe_evt->data_ptr);
		}
		break;
#endif

		case OFFLOAD_MGR_MSG_MICROSTACK_PREP_STOP_DONE:
		{
			/* TBD, is there some cleanup that needs to be done ? */
			bt_state_mgr_microstack_stop();
		}
		break;

		case OFFLOAD_MGR_MSG_MICROSTACK_RX:
		{
			microstack_msg_t *microstack_msg = (microstack_msg_t *)pipe_evt->data_ptr;
			offload_mgr_microstack_msg_handler(microstack_msg->handle,
											   microstack_msg->eventClass,
											   microstack_msg->message);
			bt_pal_free(microstack_msg);
		}
		break;

		case OFFLOAD_MGR_MSG_GLINK_APP_MANAGER_RX:
		{
			/* send the message back */
			uint8_t *app_mgr_evt = (uint8_t *)pipe_evt->data_ptr;
			endpt_mgr_handle_awm_evt(app_mgr_evt, pipe_evt->size);
			bt_pal_free((void *)pipe_evt->data_ptr);
		}
		break;

		case OFFLOAD_MGR_MSG_GLINK_CONTEXT_HUB_RX:
		{
			uint8_t *ctx_hub_evt = (uint8_t *)pipe_evt->data_ptr;
			uint16_t opcode = FETCH_2_BYTE_LITTLE_ENDIAN_FROM_ARR(ctx_hub_evt);
			/* only msg of discovery is proto encoded */
			uint16_t ctx_hub_evt_len = FETCH_2_BYTE_LITTLE_ENDIAN_FROM_ARR(ctx_hub_evt + 4);
			endpt_mgr_handle_context_hub_evt(opcode, ctx_hub_evt + 6, ctx_hub_evt_len);
			bt_pal_free((void *)pipe_evt->data_ptr);
		}
		break;

		case OFFLOAD_MGR_MSG_BT_STATE_EVT_RX:
		{
			uint8_t *bt_evt = (uint8_t *)pipe_evt->data_ptr;
			uint16_t hdr = FETCH_2_BYTE_LITTLE_ENDIAN_FROM_ARR(bt_evt);

			if (hdr == BT_ENABLE)
			{
				bt_state_on_handle(bt_evt);
			}
			else // BT_DISABLE
			{
				bt_state_off_handle(bt_evt);
			}
			bt_pal_free(pipe_evt->data_ptr);
		}
		break;

#ifdef ENABLE_WLAN_TSF_GPIO
		case OFFLOAD_MGR_MSG_GLINK_WLAN_GPIO_RX:
		{
			uint8_t *wlan_gpio_evt = (uint8_t *)pipe_evt->data_ptr;

			wlan_gpio_handle_message(wlan_gpio_evt, pipe_evt->size);
			bt_pal_free((void *)pipe_evt->data_ptr);
		}
		break;
#endif
		default:
			break;
		}
		bt_pal_free(pipe_evt);
	}
}
