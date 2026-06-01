/*==============================================================================*/
/*
* Copyright (c) 2024 - 2026 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_thread.h
===========================================================================*/
/**
 * @file offload_mgr_thread.h
 * @brief Public APIs for offload manager thread.
 *
 * @details This file contains the public API definitions for managing threads 
 *          within the offload manager.
 */

#ifndef OFFLOAD_MGR_THREAD_H
#define OFFLOAD_MGR_THREAD_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "offload_mgr_transport_shim.h"
#include "offload_fw.h"
#include "bt_utils.h"
#include "offload_mgr_sm.h"


/*===========================================================================
                      MACRO DEFINITIONS
===========================================================================*/
/** 
 * @brief MicroStack Thread Priority 
 *
 * @note To be slightly higher than Offload Manager thread 
 */
#define BT_MICROSTACK_THREAD_PRIORITY    0x3D

/** 
 * @brief Offload manager Thread 
 */
#define OFFLOAD_MGR_PIPE_ELEMENT_COUNT    		50
#define OFFLOAD_MGR_THREAD_STACK_SIZE           2048
#define OFFLOAD_MGR_THREAD_STACK_PRIORITY       62
extern const char *offload_mgr_thread_name;

/*=============================================================================
                        TYPE DEFINITIONS
=============================================================================*/
/**
 * @brief Enumeration for Offload Manager Message Opcodes.
 */
typedef enum{
	OFFLOAD_MGR_MSG_GLINK_SOCKET_OFFLOAD_RX, 
	OFFLOAD_MGR_MSG_GLINK_GATT_OFFLOAD_RX, 
    OFFLOAD_MGR_MSG_GLINK_CONTEXT_HUB_RX,
    OFFLOAD_MGR_MSG_MICROSTACK_RX, 
    OFFLOAD_MGR_MSG_GLINK_APP_MANAGER_RX, 
	OFFLOAD_MGR_MSG_BT_STATE_EVT_RX,
#ifdef ENABLE_WLAN_TSF_GPIO
    OFFLOAD_MGR_MSG_GLINK_WLAN_GPIO_RX,
#endif
#ifdef SUPPORT_POC_SETUP
	MICRO_APP_MSG_RX = 8,
	GLINK_HCI_EVT_MSG_RX = 9,
#endif /* SUPPORT_POC_SETUP */
OFFLOAD_MGR_MSG_MICROSTACK_PREP_STOP_DONE = 10,
	LPAI_TRANSPORT_MSG,
	OFFLOAD_MGR_DUMMY,
}OFFLOAD_MGR_MSG_OPCODES;


/**
 * @brief Structure for Offload Manager Main.
 */
typedef struct offload_mgr_main {
    bool thread_running;                                                       /**< Indicates if the thread is running. */
    qurt_thread_t thread_handle;                                               /**< Handle for the thread. */
    qurt_pipe_t offload_mgr_pipe;                                             /**< Pointer to the pipe. */
    qurt_pipe_data_t offload_mgr_pipe_buffer[OFFLOAD_MGR_PIPE_ELEMENT_COUNT];  /**< Buffer for the pipe data. */
    unsigned char offload_mgr_stack[OFFLOAD_MGR_THREAD_STACK_SIZE];            /**< Stack for the thread. */
} offload_mgr_main_t;

/**
 * @brief Structure for Offload Manager Pipe Message.
 */
typedef struct{
	uint16_t pipe_msg_opcode;
	const void *data_ptr;
	uint16_t size;
}offload_mgr_pipe_msg_t;


/*===========================================================================
                    GLOBAL FUNCTION DECLARATIONS
===========================================================================*/
/*
 * @brief Method for Initialization of Offload Mgr Thread
 */
void Offload_Mgr_Init(void);


/**
 * @brief Method for Start of Offload Manager Thread.
 *
 * @param thread_name Name of the thread.
 * @param thread_priority Priority of the thread.
 * @param thread_stack_size Stack size of the thread.
 * @return Handle to the started thread.
 */
qurt_thread_t Offload_Mgr_Start( char *thread_name,
        const int thread_priority,
        const int thread_stack_size);

/*===========================================================================
FUNCTION        Offload_Mgr_Task_Handler
===========================================================================*/
/**
 * @brief Task Handler Routine for Offload Manager Thread.
 *
 * @param param Parameters for the task handler.
 */
void Offload_Mgr_Task_Handler(void *param);

/*===========================================================================
FUNCTION        Offload_Mgr_Post_Event
===========================================================================*/
/**
 * @brief Method to Post Event to Offload Manager.
 *
 * @param opcode opcode of the event to be posted
 * @param data_ptr data pointer
 * @param size size of the data
 */
void Offload_Mgr_Post_Event(OFFLOAD_MGR_MSG_OPCODES opcode, uint8_t *data_ptr, size_t size);

#endif /*OFFLOAD_MGR_THREAD_H */
