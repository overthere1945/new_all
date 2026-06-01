/*==============================================================================*/
/*
* Copyright (c) 2024 - 2026 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      lpai_transport_internal.h
===========================================================================*/
/**
 * @file lpai_transport_internal.h
 * @brief Internal APIs used by Lpai transport module.
 *
 * @details 
 * Internal APIs used by the Lpai transport module.
 *
 * @note 
 * 1. LPAI_TRANSPORT_MAX_CHNLS_PER_LINK should be checked based on the maximum channels
 *    present per link in the configuration. Currently, HLOS channels are 3,
 *    and AM channels are 1. Hence, LPAI_TRANSPORT_MAX_CHNLS_PER_LINK = LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX
 *    is set.
 * 2. If any new Glink interface with a subsystem apart from HLOS or M55 is added,
 *    it should also be added to the lpai_transport_glink_channels_t structure.
 */


#ifndef _LPAI_TRANSPORT_INTERNAL_H_
#define _LPAI_TRANSPORT_INTERNAL_H_

/*============================================================================*
                           INCLUDE FILES
*============================================================================*/
#include "lpai_transport.h"
#include "qurt_pipe.h"
#include "qurt_signal.h"
#include "qurt_error.h"
#include "qurt_thread.h"
#include "glink.h"
#include "qurt.h"

/*===========================================================================
                           MACRO DEFINITIONS
===========================================================================*/
/**
 * Currently HLOS has 4 channels open with Q6 (5 with ENABLE_WLAN_TSF_GPIO), 
 * Hence using LPAI_TRANSPORT_MAX_CHNLS_PER_LINK = LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX
 * Note : Subject to change if the channels for link change
 */
#define LPAI_TRANSPORT_MAX_CHNLS_PER_LINK LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX
#define LPAI_TRANSPORT_PIPE_ELEMENT_SIZE  12
#define LPAI_TRANSPORT_PIPE_ELEMENT_COUNT 10


/*===========================================================================
                           TYPE DEFINITIONS
===========================================================================*/
/**
 * @brief Enum for GLink message opcodes.
 */
typedef enum {
  LPAI_TRANSPORT_LINK_STATE,             /**< Transport link state */
  LPAI_TRANSPORT_NOTIFY_CHANNEL_STATE,   /**< Notify channel state */
  LPAI_TRANSPORT_NOTIFY_RX_INTENT_REQ,   /**< Notify RX intent request */
  LPAI_TRANSPORT_NOTIFY_RX,              /**< Notify RX */
  LPAI_TRANSPORT_PERFORM_TX,             /**< Perform TX */
  LPAI_TRANSPORT_NOTIFY_TX_DONE,         /**< Notify TX done */
  LPAI_TRANSPORT_STOP_THREAD,            /**< Stop thread */
  LPAI_TRANSPORT_DUMMY,                  /**< Dummy opcode */
} LPAI_TRANSPORT_GLINK_MSG_OPCODES;


/*===========================================================================
                           LOCAL FUNCTION DECLARATIONS
===========================================================================*/
/*===========================================================================
FUNCTION    lpai_transport_rx_cb_t
===========================================================================*/
/**
 * Function pointer definition for RX callbacks of the applications.
 *
 * @param link_id ID of the link.
 * @param channel_id ID of the channel.
 * @param blob Pointer to the data blob.
 * @param len Length of the data blob.
 */
typedef void(*lpai_transport_rx_cb_t)(uint8_t link_id, uint8_t channel_id, const uint8* blob, size_t len);


/*===========================================================================
FUNCTION    lpai_transport_glink_init_fn_t
===========================================================================*/
/**
 * Function pointer definition for Glink initialization function.
 *
 * @param None
 * @return None
 * @sideeffects None.
 */
typedef void(*lpai_transport_glink_init_fn_t)(void);

/*===========================================================================
STRUCTURE      lpai_transport_glink_channel_t
===========================================================================*/
/**
 * Structure to hold the channel information.
 *
 * @param chnl_state Current channel state.
 * @param open_cfg Glink specific configuration for channel.
 * @param glink_hdl Handle allocated by glink driver.
 * @param link_idx Link index of the channel (could be LPAI_TRANSPORT_Q6_LINK_IDX or LPAI_TRANSPORT_Q6_LINK_IDX).
 * @param idx Channel index of the channel in the link.
 */
typedef struct lpai_transport_glink_channel{
  glink_channel_event_type chnl_state;
  glink_open_config_type open_cfg;          
  glink_handle_type glink_hdl;
  uint8_t link_idx;
  uint8_t idx;
}lpai_transport_glink_channel_t;

/*===========================================================================
STRUCTURE      lpai_transport_main_t
===========================================================================*/
/**
 * Structure to hold the transport metadata.
 *
 * @param thread_running Stores the status of the transport thread.
 * @param thread_handle Thread handle.
 * @param lpai_transport_pipe Pipe to manage event posting.
 * @param lpai_transport_pipe_buffer Buffer to store pipe elements.
 * @param lpai_transport_stack Stack for the transport thread.
 */
typedef struct lpai_transport_main{
	bool thread_running;
	qurt_thread_t thread_handle;
	qurt_pipe_t *lpai_transport_pipe;
  qurt_pipe_data_t lpai_transport_pipe_buffer[LPAI_TRANSPORT_PIPE_ELEMENT_COUNT];
  unsigned char lpai_transport_stack[LPAI_TRANSPORT_THREAD_STACK_SIZE];
}lpai_transport_main_t;



/*===========================================================================
STRUCTURE    lpai_transport_glink_interface_t
===========================================================================*/
/**
 * Structure to hold the interfaces info.
 *
 * @param link_state Current state of the link.
 * @param link_idx Link index in the lpai_transport_glink_interfaces array.
 * @param channels Channels associated with the link.
 * @param channels_len Length of the channels array.
 * @param xport_name Transport name of the link.
 * @param remote_name Remote name of the link.
 */
typedef struct lpai_transport_glink_interface {
	glink_link_state_type link_state;
	uint8_t link_idx;
	lpai_transport_glink_channel_t* channels;
	uint8_t channels_len;
	const char* xport_name;
	const char* remote_name;
}lpai_transport_glink_interface_t;

/*===========================================================================
STRUCTURE    lpai_transport_glink_channels_t
===========================================================================*/
/**
 * Structure to maintain all channels at one place.
 *
 * @param awm_channels Channels required for interface with Q6.
 * @param hlos_channels Channels required for interface with HLOS.
 */
typedef struct lpai_transport_glink_channels {
	lpai_transport_glink_channel_t awm_channels[LPAI_TRANSPORT_AWM_GLINK_CHNLS_MAX];
	lpai_transport_glink_channel_t hlos_channels[LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX];
}lpai_transport_glink_channels_t;

/*===========================================================================
STRUCTURE    lpai_transport_static_chnl_cfg_t
===========================================================================*/
/**
 * Structure to hold the static channel configuration.
 *
 * @param rx_cb Callback function for receiving data.
 * @param chnl_name Name of the channel.
 * @param num_intents number of intents to be queued for the channel
 */
typedef struct lpai_transport_static_chnl_cfg {
	lpai_transport_rx_cb_t rx_cb;
	char* chnl_name;
  uint16_t num_intents;
  uint16_t intent_size;
} lpai_transport_static_chnl_cfg_t;

/*===========================================================================
STRUCTURE    lpai_transport_static_link_cfg_t
===========================================================================*/
/**
 * Structure to hold the static link configuration.
 *
 * @param init_fn Initialization function for the link.
 * @param xport_name Transport name of the link.
 * @param remote_name Remote name of the link.
 * @param channels Array of static channels.
 */
typedef struct lpai_transport_static_link_cfg {
	lpai_transport_glink_init_fn_t init_fn;
	char* xport_name;
	uint8_t hub_type;
	char* remote_name;
	lpai_transport_static_chnl_cfg_t channels[LPAI_TRANSPORT_MAX_CHNLS_PER_LINK];
}lpai_transport_static_link_cfg_t;

/*===========================================================================
STRUCTURE    lpai_transport_pipe_msg_t
===========================================================================*/
/**
 * Structure to maintain the pipe event data.
 *
 * @param pipe_msg_opcode Opcode of the event received.
 * @param link_idx Link index to which the event belongs.
 * @param chnl_idx Channel index to which the event belongs.
 * @param data_ptr Pointer to additional data.
 * @param size Size of the data passed in data_ptr.
 */
typedef struct{
	uint8_t pipe_msg_opcode;
	uint8_t link_idx;
	uint8_t chnl_idx;
	void *data_ptr;
	size_t size;
}lpai_transport_pipe_msg_t;


/*===========================================================================
                    LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_Init
===========================================================================*/
/**
  Init the Glink memory and all the interfaces
  @param[in]      None
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Glink_Init(void);

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_Start
===========================================================================*/
/**
  starts the link callback regisration for all interfaces
  @param[in]      None
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Glink_Start(void);

/*===========================================================================
FUNCTION      LPAI_Transport_Task_Handler
===========================================================================*/
/**
  Function handling task thread and events posted on transport thread
  @param[in]      *param        param passed as a part of event
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Task_Handler(void *param);

/*===========================================================================
FUNCTION      LPAI_Transport_Set_Channel_Names
===========================================================================*/
/**
  Function to set channel names
  @param[in]      link_idx   link index of the channels
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Set_Channel_Names(uint8_t link_idx);

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_HLOS_Init
===========================================================================*/
/**
  Init function for Interface with HLOS
  @param[in]      None
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Glink_HLOS_Init(void);

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_AWM_Init
===========================================================================*/
/**
  Init function for Interface with AWM
  @brief 1. set the link configuration from static configuration. 
         2. set the channel configuration from static configuration. 
  @param[in]      None
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Glink_AWM_Init(void);

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_Close_Channels
===========================================================================*/
/**
  Close the channels corresponding to given link index
  @param[in]      link_idx   link_idx of the channels to be closed
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Glink_Close_Channels(uint8 link_idx);

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_Handle_Link_State
===========================================================================*/
/**
  Function to handle link_state callback
  @param[in]      *pipe_evt   pipe event
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Glink_Handle_Link_State(lpai_transport_pipe_msg_t *pipe_evt);

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_Handle_Channel_State
===========================================================================*/
/**
  Function to handle glink channel_state callback
  @param[in]      *pipe_evt   pipe event
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Glink_Handle_Channel_State(lpai_transport_pipe_msg_t *pipe_evt);

/*===========================================================================
FUNCTION      LPAI_Transport_Handle_Glink_Rx_Intent
===========================================================================*/
/**
  Function to handle glink_rx_intent request
  @param[in]      *pipe_evt   pipe event
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Handle_Glink_Rx_Intent(lpai_transport_pipe_msg_t *pipe_evt);

/*===========================================================================
FUNCTION      LPAI_Transport_Handle_Glink_Rx_Intent
===========================================================================*/
/**
  Function to handle glink_rx_intent request
  @param[in]      *pipe_evt   pipe event
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Handle_Glink_Rx(lpai_transport_pipe_msg_t *pipe_evt);

/*===========================================================================
FUNCTION      LPAI_Transport_Handle_Glink_Rx_Intent
===========================================================================*/
/**
  Function to handle glink_rx_intent request
  @param[in]      *pipe_evt   pipe event
  @return         None
  @sideeffects    None.
*/
void LPAI_Transport_Handle_Glink_Tx_Done(lpai_transport_pipe_msg_t *pipe_evt);
#endif /* _LPAI_TRANSPORT_INTERNAL_H_ */
