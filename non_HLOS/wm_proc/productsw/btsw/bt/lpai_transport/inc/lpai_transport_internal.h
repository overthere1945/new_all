#ifndef _LPAI_TRANSPORT_INTERNAL_H_
#define _LPAI_TRANSPORT_INTERNAL_H_

/**
 * @file lpai_transport_internal.h
 *
 * @brief lpai_transport Internal Header File
 */

/*============================================================================*
Include Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "lpai_transport.h"
#include "qurt_signal.h"
#include "qurt_error.h"
#include "qurt_thread.h"
#include "glink.h"

#define LPAI_TRANSPORT_LOG(...)
//#define malloc qapi_bt_malloc
//#define free   qapi_bt_free
/*===========================================================================
                           MACRO DEFINITIONS
===========================================================================*/
#define LPAI_TRANSPORT_MAX_CHNLS_PER_LINK 1


/*===========================================================================
                           TYPE DEFINITIONS
===========================================================================*/
typedef enum{
	LPAI_TRANSPORT_LINK_STATE,             
	LPAI_TRANSPORT_NOTIFY_CHANNEL_STATE,  
	LPAI_TRANSPORT_NOTIFY_RX_INTENT_REQ, 
	LPAI_TRANSPORT_NOTIFY_RX,
	LPAI_TRANSPORT_PERFORM_TX,
	LPAI_TRANSPORT_NOTIFY_TX_DONE,
	LPAI_TRANSPORT_STOP_THREAD,
}LPAI_TRANSPORT_GLINK_MSG_OPCODES;

typedef enum{
	LPAI_TRANSPORT_Q6_LINK_IDX = 0, 
	LPAI_TRANSPORT_HLOS_LINK_IDX, 
	LPAI_TRANSPORT_LINKS_MAX
}LPAI_TRANSPORT_LINKS_NUM;

typedef enum{
	LPAI_TRANSPORT_Q6_CTRL_CHNL_IDX = 0,
	LPAI_TRANSPORT_Q6_GLINK_CHNLS_MAX,
}LPAI_TRANSPORT_Q6_CHNL_IDXS;

typedef enum{
	LPAI_TRANSPORT_HLOS_CTRL_CHNL_IDX = 0,
	LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX,
}LPAI_TRANSPORT_HLOS_CHNL_IDXS;

/* function pointer definition of rx_callbacks of the applications */
typedef void(*lpai_transport_rx_cb_t)(const uint8_t* blob, size_t len);

typedef void(*lpai_transport_glink_init_fn_t)();

/* Structure to hold the channel information */
typedef struct lpai_transport_glink_channel{
	/* current channel state */
    glink_channel_event_type chnl_state;

	/* glink specific configuration for channel */
    glink_open_config_type open_cfg;          

	/* handle allocated by glink driver */
    glink_handle_type glink_hdl;

	/* link idx of the channel 
	 * could be LPAI_TRANSPORT_Q6_LINK_IDX OR 
	 * LPAI_TRANSPORT_Q6_LINK_IDX */
	uint8_t link_idx;

	/* Channel index of the channel in the link */
	uint8_t idx;

	/** Number of Intents Possible on Channel */
	uint8_t num_intents;

}lpai_transport_glink_channel_t;

/* structure to hold the transport metadata */
typedef struct lpai_transport_main{
	/* stores the status of the transport thread */
	bool thread_running;

	/* thread handle */
	qurt_thread_t thread_handle;

	/* pipe to manage event posting */
	struct k_msgq lpai_transport_pipe;
}lpai_transport_main_t;

/* structure to hold the interfaces info */
typedef struct lpai_transport_glink_interface {
	/* current state of link */
	glink_link_state_type link_state;

	/* link index in the lpai_transport_glink_interfaces array */
	uint8_t link_idx;

	/* channels associated with the link */
	lpai_transport_glink_channel_t* channels;

	/* length of the channels array */
	uint8_t channels_len;

	/* xport_name of the link */
	const char* xport_name;

	/* remote name of the link */
	const char* remote_name;
}lpai_transport_glink_interface_t;

/* structure to maintain all channels at one place */
typedef struct lpai_transport_glink_channels {
	/* channels required for interface with q6 */
	lpai_transport_glink_channel_t q6_channels[LPAI_TRANSPORT_Q6_GLINK_CHNLS_MAX];

	/* channels required for interface with hlos */
	lpai_transport_glink_channel_t hlos_channels[LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX];
}lpai_transport_glink_channels_t;

typedef struct lpai_transport_static_chnl_cfg {
	lpai_transport_rx_cb_t rx_cb;
	char* chnl_name;
}lpai_transport_static_chnl_cfg_t;

typedef struct lpai_transport_static_link_cfg {
	lpai_transport_glink_init_fn_t init_fn;
	char* xport_name;
	char* remote_name;
	lpai_transport_static_chnl_cfg_t channels[LPAI_TRANSPORT_MAX_CHNLS_PER_LINK];
	
}lpai_transport_static_link_cfg_t;


/* struct to maintain the pipe event data */
typedef struct{
	/* opcode of the event received */
	uint8_t pipe_msg_opcode;

	/* link index to which event belongs */
	uint8_t link_idx;

	/* chnl index to which event belongs */
	uint8_t chnl_idx;

	/* data ptr for additional data */
	const void *data_ptr;

	/* size of the data passed in data_ptr */
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
/*=========================================================================*/
void LPAI_Transport_Glink_Init();

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_Start
===========================================================================*/
/**

  starts the link callback regisration for all interfaces

  @param[in]      None

  @return         None

  @sideeffects    None.
*/
/*=========================================================================*/
void LPAI_Transport_Glink_Start();

/*===========================================================================
FUNCTION      LPAI_Transport_Task_Handler
===========================================================================*/
/**

  Function handling task thread and events posted on transport thread

  @param[in]      *param        param passed as a part of event

  @return         None

  @sideeffects    None.
*/
/*=========================================================================*/
void LPAI_Transport_Task_Handler(void *param1 , void *param2 , void *param3);

/*===========================================================================
FUNCTION      LPAI_Transport_Set_Channel_Names
===========================================================================*/
/**

  Function handling task thread and events posted on transport thread

  @param[in]      link_idx   link index of the channels to be opened

  @return         None

  @sideeffects    None.
*/
/*=========================================================================*/
void LPAI_Transport_Set_Channel_Names(uint8_t link_idx);

/*===========================================================================
FUNCTION      LPAI_Transport_Set_Channel_Names
===========================================================================*/
/**

  Function to open all channel names

  @param[in]      link_idx   link index of the channels

  @return         None

  @sideeffects    None.
*/
/*=========================================================================*/
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
/*=========================================================================*/
void LPAI_Transport_Glink_HLOS_Init();

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_Q6_Init
===========================================================================*/
/**

  Init function for Interface with Q6

  @param[in]      None

  @return         None

  @sideeffects    None.
*/
/*=========================================================================*/
void LPAI_Transport_Glink_Q6_Init();

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_Close_Channels
===========================================================================*/
/**

  Close the channels corresponding to given link index

  @param[in]      link_idx   link_idx of the channels to be closed

  @return         None

  @sideeffects    None.
*/
/*=========================================================================*/
void LPAI_Transport_Glink_Close_Channels(uint8_t link_idx);

/*===========================================================================
FUNCTION      LPAI_Transport_Glink_Handle_Link_State
===========================================================================*/
/**

  Function to handle link_state callback

  @param[in]      *pipe_evt   pipe event

  @return         None

  @sideeffects    None.
*/
/*=========================================================================*/
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
/*=========================================================================*/
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
/*=========================================================================*/
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
/*=========================================================================*/
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
/*=========================================================================*/
void LPAI_Transport_Handle_Glink_Tx_Done(lpai_transport_pipe_msg_t *pipe_evt);


#endif /* _LPAI_TRANSPORT_INTERNAL_H_ */
