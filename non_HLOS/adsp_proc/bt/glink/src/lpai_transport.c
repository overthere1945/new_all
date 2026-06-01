/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2026 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      lpai_transport.c
===========================================================================*/
/**
 * @file lpai_transport.c
 * @brief Lpai transport used by offload manager.
 *
 * @details
 * This file contains the implementation of the Lpai transport used by the offload manager.
 *
 * @note
 * 1. Any alteration like addition, deletion, or change of link or channel
 *    should happen only to the LPAI_TRANSPORT_LINK_CFG global variable.
 *    A change in this variable is enough.
 * 2. The links/channel indexes must be altered in LPAI_TRANSPORT_LINKS_NUM,
 *    LPAI_TRANSPORT_AWM_CHNL_IDXS, and LPAI_TRANSPORT_HLOS_CHNL_IDXS on any
 *    change in LPAI_TRANSPORT_LINK_CFG.
 * 3. The pointer returned in rx_cb is expected to be freed by the entity using it
 *    and is not freed by Lpai Transport.
 */

/*============================================================================*
								INCLUDE FILES
*============================================================================*/
#include <stdio.h>
#include <stdint.h>
#include "lpai_transport.h"
#include "lpai_transport_internal.h"
#include "stringl.h"
#include "bt_pal_heap.h"
#include "bt_utils.h"
#include "offload_mgr_thread.h"
#include "bt_state_mgr.h"
#include "uSleep_mode_trans.h"
#include "qurt_island.h"

/*============================================================================*
								EXTERN APIS/VARS
*============================================================================*/
extern void Offload_Mgr_Post_Transport_Event(uint8_t link_id, uint8_t channel_id, const uint8 *blob, size_t len);
extern void Offload_Mgr_Post_Transport_Event_POC_M55(uint8_t link_id, uint8_t channel_id, const uint8 *blob, size_t len);
extern void Offload_Mgr_Post_Transport_Event_POC_HLOS(uint8_t link_id, uint8_t channel_idx, const uint8 *blob, size_t len);
extern void Offload_Mgr_Post_arbiter_event(uint8_t link_id, uint8_t channel_id, const uint8 *blob, size_t len);

/*============================================================================*
								MACRO DEFINITIONS
*============================================================================*/
#define GLINK_PADDING 4

/*============================================================================*
								GLOBAL DATA DECLARATIONS
*============================================================================*/
const char *lpai_transport_thread_name = "lpai_transport";

/*===========================================================================
ARRAY      LPAI_TRANSPORT_LINK_CFG
===========================================================================*/
/**
 * @brief Static configuration for LPAI transport links.
 *
 * @details This array defines the static configuration for LPAI transport links, including remote names,
 *          transport names, initialization functions, and channel configurations.
 *
 * @array
 * | Link Index                      | Remote Name   | Transport Name | Initialization Function          | Channel Index                            | Channel Name                             | RX Callback Function                  | Number of intents |
 * |---------------------------------|---------------|----------------|----------------------------------|----------------------------------------  |---------------------------------------   |---------------------------------------| 2                 |
 * | LPAI_TRANSPORT_AWM_LINK_IDX     | "aon"         | "bg"           | LPAI_Transport_Glink_AWM_Init    | LPAI_TRANSPORT_AWM_APP_MGR_CHNL_IDX      | "am_bt_app_chnl"                         | Offload_Mgr_Post_Transport_Event      | 2                 |
 * | LPAI_TRANSPORT_HLOS_LINK_IDX    | "apss"        | "SMEM"         | LPAI_Transport_Glink_HLOS_Init   | LPAI_TRANSPORT_HLOS_BT_SOCKET_CHNL_IDX   | "bt_socket_hal_chnl" or "socket_offload" | Offload_Mgr_Post_Transport_Event      | 2                 |
 * |                                 |               |                |                                  | LPAI_TRANSPORT_HLOS_CONTEXT_HUB_CHNL_IDX | "contexthub_hal_offload_mgr_chnl"        | Offload_Mgr_Post_Transport_Event      | 2                 |
 * |                                 |               |                |                                  | LPAI_TRANSPORT_HLOS_HCI_CHNL_IDX         | "arbiter_hci_chnl" or "hci_arbiter"      | Offload_Mgr_Post_arbiter_event        | 2                 |
 * @endarray
 */
static const lpai_transport_static_link_cfg_t LPAI_TRANSPORT_LINK_CFG[] = {
	[LPAI_TRANSPORT_AWM_LINK_IDX] = {
		.remote_name = "am",
		.xport_name  = "SMEM",
		.init_fn     = LPAI_Transport_Glink_AWM_Init,
		.hub_type    = GLINK_HUB_TYPE_MICRO,
		.channels    = {
							[LPAI_TRANSPORT_AWM_APP_MGR_CHNL_IDX] = {
							.chnl_name = "am_bt_app_chnl", /* todo */
							.rx_cb     = Offload_Mgr_Post_Transport_Event,
							.num_intents = 2, 
							.intent_size = 1024
					   }, 
		},
	},
	[LPAI_TRANSPORT_HLOS_LINK_IDX] = {
		.remote_name = "apss",
		.xport_name  = "SMEM",
		.init_fn     = LPAI_Transport_Glink_HLOS_Init,
		.hub_type    = GLINK_HUB_TYPE_DEFAULT,
		.channels    = {
						[LPAI_TRANSPORT_HLOS_BT_SOCKET_CHNL_IDX] = {
#ifndef SUPPORT_POC_SETUP 
							.chnl_name = "bt_socket_hal_chnl",
#else 
							.chnl_name = "socket_offload",
#endif /* SUPPORT_POC_SETUP */
							.rx_cb     = Offload_Mgr_Post_Transport_Event,
							.num_intents = 2, 
							.intent_size = 512,
					   },
					   [LPAI_TRANSPORT_HLOS_CONTEXT_HUB_CHNL_IDX] = {
							.chnl_name = "contexthub_hal_offload_mgr_chnl",
							.rx_cb     = Offload_Mgr_Post_Transport_Event, 
							.num_intents = 2, 
							.intent_size = 512,
					   },
					   [LPAI_TRANSPORT_HLOS_HCI_CHNL_IDX] = {
#ifndef SUPPORT_POC_SETUP
							.chnl_name = "hci_arbiter_chnl",
#else
							.chnl_name = "hci_arbiter",
#endif /* SUPPORT_POC_SETUP */
							.rx_cb     = Offload_Mgr_Post_arbiter_event, 
							.num_intents = 2, 
							.intent_size = 1024,
					   },
                       [LPAI_TRANSPORT_HLOS_BT_GATT_CHNL_IDX] = {
                             .chnl_name = "gatt_offload_chnl",
                             .rx_cb     = Offload_Mgr_Post_Transport_Event, 
							 .num_intents = 2,
							 .intent_size = 1024,
                       },
#ifdef ENABLE_WLAN_TSF_GPIO
                       [LPAI_TRANSPORT_HLOS_WLAN_GPIO_CHNL_IDX] = {
                             .chnl_name = "wlan_gpio_chnl",
                             .rx_cb     = Offload_Mgr_Post_Transport_Event,
                             .num_intents = 2,
                             .intent_size = 32,
                       },
#endif
		},
	},
};

/*============================================================================*
								GLOBAL VARIABLES
*============================================================================*/
/**
 * @brief	Array to store all the glink interfaces/links
 */
static lpai_transport_glink_interface_t lpai_transport_glink_interfaces[LPAI_TRANSPORT_LINKS_MAX];
/**
 * @brief 	Variable holding all the glink channels
 */
static lpai_transport_glink_channels_t lpai_transport_glink_channels;
/**
 * @brief 	holds the lpai transport thread level context
 */
#ifdef LPAI_TRANSPORT_THREAD_ENABLED
static lpai_transport_main_t lpai_transport_main;
#endif

/*============================================================================*
							LOCAL FUNCTION DECLARATIONS
*============================================================================*/
static void LPAI_Transport_Glink_Link_State_Cb(glink_link_info_type *link_info, void *priv);
static void LPAI_Transport_Glink_Channel_State_Cb(glink_handle_type handle,
												  const void *priv, glink_channel_event_type event);

static boolean LPAI_Transport_Glink_Rx_Intent_Req_Cb(glink_handle_type handle,
													 const void *priv, size_t req_size);

static void LPAI_Transport_Glink_Rx_Notification_Cb(glink_handle_type handle,
													const void *priv,
													const void *pkt_priv,
													const void *ptr,
													size_t size,
													size_t intent_used);

static void LPAI_Transport_Glink_Tx_Done_Cb(glink_handle_type handle,
											const void *priv, const void *pkt_priv,
											const void *ptr, size_t size);

void LPAI_Transport_Set_Channel_Names(uint8_t link_idx)
{
	lpai_transport_glink_interface_t *link;
	lpai_transport_glink_channel_t *channels;
	uint8_t channels_len;

	link = &lpai_transport_glink_interfaces[link_idx];
	channels = link->channels;
	channels_len = link->channels_len;

	for (int i = 0; i < channels_len; i++)
	{
		channels[i].open_cfg.name =
			LPAI_TRANSPORT_LINK_CFG[link_idx].channels[i].chnl_name;
	}
}

/*===========================================================================
FUNCTION    LPAI_Transport_Malloc
===========================================================================*/
/**
 * @brief Allocates memory of the specified size.
 *
 * @param size Size of the memory to allocate.
 * @return Pointer to the allocated memory, or NULL if allocation fails.
 */
static void *LPAI_Transport_Malloc(int size)
{
	void *data = bt_pal_malloc(size);
	return data;
}

/*===========================================================================
FUNCTION    LPAI_Transport_Free
===========================================================================*/
/**
 * @brief Frees the allocated memory and sets the pointer to NULL.
 *
 * @param ptr Pointer to the memory to be freed.
 */
static void LPAI_Transport_Free(void *ptr)
{
	bt_pal_free(ptr);
	ptr = NULL;
}

void LPAI_Transport_Queue_Evt(qurt_pipe_data_t data)
{
#ifdef LPAI_TRANSPORT_THREAD_ENABLED
	qurt_pipe_try_send(lpai_transport_main.lpai_transport_pipe, data);
#else
	Offload_Mgr_Post_Event(LPAI_TRANSPORT_MSG, (uint8_t *)data, 0);
#endif
}

void LPAI_Transport_Glink_HLOS_Init(void)
{
	lpai_transport_glink_channel_t *channels;
	lpai_transport_glink_interface_t *link;
	const lpai_transport_static_link_cfg_t *static_link_cfg;

	static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[LPAI_TRANSPORT_HLOS_LINK_IDX];
	channels = lpai_transport_glink_channels.hlos_channels;
	for (int idx = 0; idx < LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX; idx++)
	{
		channels[idx].chnl_state = GLINK_REMOTE_DISCONNECTED;
		channels[idx].idx = idx;
		channels[idx].link_idx = LPAI_TRANSPORT_HLOS_LINK_IDX;
	}

	link = &lpai_transport_glink_interfaces[LPAI_TRANSPORT_HLOS_LINK_IDX];
	link->link_state = GLINK_LINK_STATE_DOWN;
	link->link_idx = LPAI_TRANSPORT_HLOS_LINK_IDX;
	link->channels = lpai_transport_glink_channels.hlos_channels;
	link->channels_len = LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX;
	link->xport_name = static_link_cfg->xport_name;
	link->remote_name = static_link_cfg->remote_name;
}

void LPAI_Transport_Glink_AWM_Init(void)
{
	lpai_transport_glink_channel_t *channels;
	lpai_transport_glink_interface_t *link;
	const lpai_transport_static_link_cfg_t *static_link_cfg;

	static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[LPAI_TRANSPORT_AWM_LINK_IDX];
	for (int idx = 0; idx < LPAI_TRANSPORT_AWM_GLINK_CHNLS_MAX; idx++)
	{
		channels = lpai_transport_glink_channels.awm_channels;
		channels[idx].chnl_state = GLINK_REMOTE_DISCONNECTED;
		channels[idx].idx = idx;
		channels[idx].link_idx = LPAI_TRANSPORT_AWM_LINK_IDX;
	}

	link = &lpai_transport_glink_interfaces[LPAI_TRANSPORT_AWM_LINK_IDX];
	link->link_state = GLINK_LINK_STATE_DOWN;
	link->link_idx = LPAI_TRANSPORT_AWM_LINK_IDX;
	link->channels = lpai_transport_glink_channels.awm_channels;
	link->channels_len = LPAI_TRANSPORT_AWM_GLINK_CHNLS_MAX;
	link->xport_name = static_link_cfg->xport_name;
	link->remote_name = static_link_cfg->remote_name;
}


void LPAI_Transport_Glink_Init(void)
{
	const lpai_transport_static_link_cfg_t* static_link_cfg;
	lpai_transport_glink_init_fn_t init_fn;

	/* call the init function for each transport */
	for (uint8_t i = 0; i < LPAI_TRANSPORT_LINKS_MAX; i++)
	{
		static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[i];
		init_fn = static_link_cfg->init_fn;
		if (init_fn != NULL)
		{
			init_fn();
		}
	}
}

/*===========================================================================
FUNCTION    LPAI_Transport_Glink_Register_Link
===========================================================================*/
/**
 * Register the link with the link index
 *
 * @param link_idx  index of the link to be registered
 */
void LPAI_Transport_Glink_Register_Link(uint8_t link_idx)
{
	glink_err_type glink_result;
	glink_link_id_type glink_id;
	const lpai_transport_static_link_cfg_t *static_link_cfg;
    void *priv;

	static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[link_idx];
	priv            = (void *)&lpai_transport_glink_interfaces[link_idx].link_idx;

    GLINK_LINK_ID_STRUCT_INIT(glink_id);
  	GLINK_LINK_ID_SET_HUB(glink_id, static_link_cfg->hub_type);

    glink_id.xport         = static_link_cfg->xport_name;
    glink_id.remote_ss     = static_link_cfg->remote_name;
    glink_id.link_notifier = LPAI_Transport_Glink_Link_State_Cb;

    glink_result = glink_register_link_state_cb(&glink_id, priv);

    if (glink_result != GLINK_STATUS_SUCCESS)
    {
        LPAI_TRANSPORT_LOGE("glink_register_link_state_cb for link_idx failed "
                "result code %d\n",link_idx, glink_result);
    }
}

void LPAI_Transport_Glink_Start(void)
{
	/* Register for link_registration callback of all the interfaces */
	for(uint8_t i = 0; i < LPAI_TRANSPORT_LINKS_MAX; i++) 
	{
		LPAI_Transport_Glink_Register_Link(i);
	}
}



static void LPAI_Transport_Glink_Link_State_Cb(glink_link_info_type *link_info, void *priv)
{
	uint8_t link_idx = *(uint8_t *)priv;
	lpai_transport_pipe_msg_t *pipe_event = LPAI_Transport_Malloc(sizeof(lpai_transport_pipe_msg_t));
	if (pipe_event == NULL)
	{
		LPAI_TRANSPORT_LOGE("LPAI_Transport_Glink_Link_State_Cb : unable to alloc pipe_event");
		return;
	}

	lpai_transport_glink_interfaces[link_idx].link_state = link_info->link_state;
	pipe_event->pipe_msg_opcode = LPAI_TRANSPORT_LINK_STATE;
	pipe_event->link_idx = link_idx;
	pipe_event->size = 0;

	LPAI_Transport_Queue_Evt((qurt_pipe_data_t)pipe_event);
}

/*===========================================================================
FUNCTION    LPAI_Transport_Glink_Open_Channels
===========================================================================*/
/**
 * Open channels of the link with link index.
 *
 * @param link_idx  link index of the link.
 */
void LPAI_Transport_Glink_Open_Channels(uint8_t link_idx)
{
	glink_err_type glink_result;
	lpai_transport_glink_interface_t *link;
	lpai_transport_glink_channel_t *channels;
	const lpai_transport_static_link_cfg_t *static_link_cfg;
	uint8_t channels_len;

	static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[link_idx];
	link = &lpai_transport_glink_interfaces[link_idx];
	//LPAI_Transport_Set_Channel_Names(link_idx);
	channels = link->channels;
	channels_len = link->channels_len;

	for(int i = 0; i < channels_len; i++)
	{
		GLINK_OPEN_CONFIG_INIT(channels[i].open_cfg);
		GLINK_OPEN_CONFIG_SET_HUB(channels[i].open_cfg, static_link_cfg->hub_type);

		channels[i].open_cfg.name = 
		LPAI_TRANSPORT_LINK_CFG[link_idx].channels[i].chnl_name;

		channels[i].open_cfg.transport = link->xport_name;
		channels[i].open_cfg.remote_ss = link->remote_name;
		channels[i].open_cfg.priv = &(channels[i]);
		channels[i].open_cfg.notify_state = LPAI_Transport_Glink_Channel_State_Cb;
		channels[i].open_cfg.notify_rx_intent_req = LPAI_Transport_Glink_Rx_Intent_Req_Cb;
		channels[i].open_cfg.notify_rx = LPAI_Transport_Glink_Rx_Notification_Cb;
		channels[i].open_cfg.notify_tx_done = LPAI_Transport_Glink_Tx_Done_Cb;

		glink_result = glink_open(&channels[i].open_cfg,
								  &channels[i].glink_hdl);

		if (glink_result != GLINK_STATUS_SUCCESS)
		{
			LPAI_TRANSPORT_LOGE("glink_open failed for link_idx : %d channel number %d"
							   "result code %d\n",
							   link_idx, i, glink_result);
		}
	}
}

void LPAI_Transport_Glink_Close_Channels(uint8_t link_idx)
{
	lpai_transport_glink_interface_t *link;
	glink_err_type glink_result = 0;

	link = &lpai_transport_glink_interfaces[link_idx];

	for (int idx = 0; idx < link->channels_len; idx++)
	{
		glink_result = glink_close(link->channels[idx].glink_hdl);
		if (glink_result != 0)
		{
			LPAI_TRANSPORT_LOGE("Unable to close channel with idx %d glink_result : %d", idx, glink_result);
		}
	}
}

void LPAI_Transport_Glink_Handle_Link_State(lpai_transport_pipe_msg_t *pipe_evt)
{
	uint8_t link_idx = pipe_evt->link_idx;
	glink_link_state_type link_state = lpai_transport_glink_interfaces[link_idx].link_state;

	switch (link_state)
	{
	case GLINK_LINK_STATE_UP:
	{
		LPAI_Transport_Glink_Open_Channels(link_idx);
	}
	break;

	case GLINK_LINK_STATE_DOWN:
	{
		LPAI_Transport_Glink_Close_Channels(link_idx);
	}
	break;

	default:
		break;
	}
}


static void LPAI_Transport_Glink_Channel_State_Cb(glink_handle_type handle,
												  const void *priv, glink_channel_event_type event)
{
	lpai_transport_glink_channel_t *channel = (lpai_transport_glink_channel_t *)priv;
	lpai_transport_static_chnl_cfg_t *static_chnl_cfg = &(LPAI_TRANSPORT_LINK_CFG[channel->link_idx].channels[channel->idx]);
	int glink_result;
	lpai_transport_pipe_msg_t *pipe_event = LPAI_Transport_Malloc(sizeof(lpai_transport_pipe_msg_t));
	if (pipe_event == NULL)
	{
		LPAI_TRANSPORT_LOGE("LPAI_Transport_Glink_Channel_State_Cb : unable to alloc pipe_event");
		return;
	}

	channel->chnl_state = event;
	pipe_event->pipe_msg_opcode = LPAI_TRANSPORT_NOTIFY_CHANNEL_STATE;
	pipe_event->data_ptr = &(channel->chnl_state);
	pipe_event->chnl_idx = channel->idx;
	pipe_event->link_idx = channel->link_idx;

	/** Queing intents at the start */
	if (channel->chnl_state == GLINK_CONNECTED)
	{
		for(uint16_t i = 0; i < static_chnl_cfg->num_intents; i++) {
			glink_result = glink_queue_rx_intent(channel->glink_hdl, NULL, static_chnl_cfg->intent_size);
			if (glink_result != GLINK_STATUS_SUCCESS)
			{
				LPAI_TRANSPORT_LOGE("LPAI_Transport_Handle_Glink_Channel_State glink_queue_rx_intent failed : reason : %d chnl_idx : %d link_idx : %d\n",
								glink_result, channel->idx, channel->link_idx);
			}
		}
	}

	LPAI_Transport_Queue_Evt((qurt_pipe_data_t)pipe_event);
}

void LPAI_Transport_Handle_Glink_Channel_State(lpai_transport_pipe_msg_t *pipe_evt)
{
	uint8_t channel_idx;
	glink_channel_event_type channel_event;
	uint8_t link_idx;

	channel_idx = (pipe_evt->chnl_idx);
	channel_event = *(glink_channel_event_type *)(pipe_evt->data_ptr);
	link_idx = pipe_evt->link_idx;

	glink_err_type glink_result;
    lpai_transport_glink_interface_t *link;
    lpai_transport_glink_channel_t *channel;
    link = &lpai_transport_glink_interfaces[link_idx];
    channel = &link->channels[channel_idx];
    LPAI_TRANSPORT_LOGM("Glink_Channel_State %d, link_idx %d channel_idx %d", channel_event, link_idx, channel_idx);

	switch (channel_event)
	{
	case GLINK_CONNECTED:
	{
	}
	break;
	case GLINK_REMOTE_DISCONNECTED:
	{
        /* HLOS HAL disconnect with Glink channel during BT Off sequence.
        From our end glink channel needs to closed and re-opened to be ready for next BT-On sequence. */
        /* Close the glink channel */
		glink_result = glink_close(channel->glink_hdl);
		if (glink_result != 0)
		{
			LPAI_TRANSPORT_LOGE("Unable to close channel with idx %d glink_result : %d", link_idx, glink_result);
		}

        /* When there is HLOS BT HAL crashes, all BT HAL Glink channels will be disconnected. In this case,
        bt_state_mgr need to check BT_DISABLE is received before Glink is disconnected. Otherwise it is considered as
        HAL crash and bt_state_mgr need to mock BT-Off sequence to clean-up on LPASS side. */
		if (link_idx == LPAI_TRANSPORT_HLOS_LINK_IDX && (channel_idx == LPAI_TRANSPORT_HLOS_BT_SOCKET_CHNL_IDX ||
			channel_idx == LPAI_TRANSPORT_HLOS_HCI_CHNL_IDX || channel_idx == LPAI_TRANSPORT_HLOS_BT_GATT_CHNL_IDX))
		{
			bt_state_check_and_mock_off();
		}
	}
	break;
	case GLINK_LOCAL_DISCONNECTED:
	{

        /* Open this channel again so that when HAL restarts, it is ready to get connected. */
		glink_result = glink_open(&channel->open_cfg,
								  &channel->glink_hdl);

		if (glink_result != GLINK_STATUS_SUCCESS)
		{
			LPAI_TRANSPORT_LOGE("glink_open failed for link_idx : %d channel number %d"
							   "result code %d\n",
							   link_idx, channel_idx, glink_result);
		}
	}
	break;
	default:
		break;
	}
}

/*===========================================================================
FUNCTION    get_channel_name_from_idx
===========================================================================*/
/**
 * utility function to find channel name from channel index
 *
 * @param link_idx  index of the link
 * @param chnl_idx  index of the channel
 */
const char *get_channel_name_from_idx(uint8_t link_idx, uint8_t chnl_idx)
{
	return lpai_transport_glink_interfaces[link_idx].channels[chnl_idx].open_cfg.name;
}

static boolean LPAI_Transport_Glink_Rx_Intent_Req_Cb(glink_handle_type handle,
													 const void *priv, size_t req_size)
{
	glink_err_type glink_result;
	size_t size;
	lpai_transport_glink_interface_t *link;
	lpai_transport_glink_channel_t *channel;

	channel = (lpai_transport_glink_channel_t *)priv;
	link = &lpai_transport_glink_interfaces[channel->link_idx];

	size = req_size;
	LPAI_TRANSPORT_LOGM("Intent request on channel %s : ", get_channel_name_from_idx(channel->link_idx, channel->idx));

	/*32-bit aligned*/
	if (size % GLINK_PADDING)
	{
		size = (size + 0x3) & ~(0x3);
	}

	glink_result = glink_queue_rx_intent(channel->glink_hdl, NULL, size);
	if (glink_result != GLINK_STATUS_SUCCESS)
	{
		LPAI_TRANSPORT_LOGE("glink_queue_rx_intent failed : reason : %d", glink_result);
		return false;
	}
	return true;
}

void LPAI_Transport_Handle_Glink_Rx_Intent(lpai_transport_pipe_msg_t *pipe_evt)
{
	/* currently handling intent request directly from glink context */
}

static void LPAI_Transport_Glink_Rx_Notification_Cb(glink_handle_type handle,
													const void *priv,
													const void *pkt_priv,
													const void *ptr,
													size_t size,
													size_t intent_used)
{
	lpai_transport_glink_channel_t *channel;
	lpai_transport_rx_cb_t rx_cb;
	glink_err_type glink_result;
	const lpai_transport_static_link_cfg_t *static_link_cfg;

	channel = (lpai_transport_glink_channel_t *)priv;
	rx_cb = NULL;
	if (channel != NULL)
	{
		static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[channel->link_idx];

		void *data_ptr = NULL;

		data_ptr = LPAI_Transport_Malloc(size); /* the entity receiving is  responsible for free */
		if (data_ptr == NULL)
		{
			/* fatal */
			LPAI_TRANSPORT_LOGE("Not Enough Memory to allocate for Incoming Rx data \
			                    on link_idx : %d, channel_idx : %d \n",
							   channel->link_idx,
							   channel->idx);
		}
		else
		{
			memscpy(data_ptr, size, ptr, size);

			/* reuse the intent */
			glink_result = glink_rx_done(channel->glink_hdl, ptr, TRUE);
			/* Call the corresponding api for rx */
			rx_cb = static_link_cfg->channels[channel->idx].rx_cb;
			if (rx_cb != NULL)
			{
				rx_cb(channel->link_idx, channel->idx, (uint8_t *)data_ptr, size);
			}
		}
	}
}

static void LPAI_Transport_Glink_Tx_Done_Cb(glink_handle_type handle,
											const void *priv, const void *pkt_priv,
											const void *ptr, size_t size)
{
    lpai_transport_glink_channel_t *channel = (lpai_transport_glink_channel_t *)priv;
	if(channel != NULL) {
		LPAI_TRANSPORT_LOGL("tx_done:channel_id[%d]link_id[%d]", channel->idx, channel->link_idx);
	}
	else {
		LPAI_TRANSPORT_LOGE("tx_done:unknown channel");
	}
    if(ptr != NULL)
    {
    	LPAI_Transport_Free(ptr);
    }
}

void LPAI_Transport_Handle_Glink_Tx_Done(lpai_transport_pipe_msg_t *pipe_evt)
{
	if (pipe_evt->data_ptr != NULL)
	{
		LPAI_Transport_Free(pipe_evt->data_ptr);
	}
}

void LPAI_Transport_Init(void)
{
	LPAI_Transport_Glink_Init();

#ifdef LPAI_TRANSPORT_THREAD_ENABLED
	qurt_pipe_attr_t pipe_attr;

	qurt_pipe_attr_init(&pipe_attr);
	qurt_pipe_attr_set_elements(&pipe_attr, LPAI_TRANSPORT_PIPE_ELEMENT_COUNT);
	// qurt_pipe_attr_set_element_size(&pipe_attr, LPAI_TRANSPORT_PIPE_ELEMENT_SIZE);
	qurt_pipe_attr_set_buffer(&pipe_attr, lpai_transport_main.lpai_transport_pipe_buffer);
	qurt_pipe_create(&(lpai_transport_main.lpai_transport_pipe), &pipe_attr);
#else
	LPAI_Transport_Glink_Start();
#endif

}

glink_err_type LPAI_Transport_Send_Data_On_Chnl(uint8_t link_idx, uint8_t chnl_idx, const void *ptr , size_t size)
{
	glink_err_type glink_result = GLINK_STATUS_FAILURE;

	size_t req_size = size;
	lpai_transport_glink_interface_t *link = &lpai_transport_glink_interfaces[link_idx];
	lpai_transport_glink_channel_t *channel = &(link->channels[chnl_idx]);


	if (channel->chnl_state != GLINK_CONNECTED)
	{
		LPAI_TRANSPORT_LOGE("Can't do tx on channel with link_idx : %d and channel idx : %d"
						   " because the channel state is : %d",
						   link_idx, chnl_idx, channel->chnl_state);
		return glink_result;
	}
	void *ctrl_buf = NULL;
	if (req_size % GLINK_PADDING != 0)
	{
		req_size = (req_size + 0x3) & ~(0x3);
	}

	ctrl_buf = LPAI_Transport_Malloc(req_size);

	if (ctrl_buf)
	{
		memscpy(ctrl_buf, size, ptr, size);

		if(link->link_idx == LPAI_TRANSPORT_HLOS_LINK_IDX)
		{
			if (qurt_island_get_status())
			{
				uSleep_exit();
			}
		}

		glink_result = glink_tx(channel->glink_hdl, NULL, (uint32 *)ctrl_buf, req_size, GLINK_TX_REQ_INTENT);

		/* log the sent data */
		LPAI_TRANSPORT_LOGM("Tx data on link %d channel %d data : ", link_idx, chnl_idx);
		for (int i = 0; i < size; i++)
		{
			LPAI_TRANSPORT_LOGL("%02x ", ((uint8_t *)ctrl_buf)[i]);
		}

		if (glink_result != GLINK_STATUS_SUCCESS)
		{
			LPAI_TRANSPORT_LOGE("glink_tx failed %d\n", glink_result);
			LPAI_Transport_Free(ctrl_buf);
		}
	}
	else
	{
		LPAI_TRANSPORT_LOGF("\n LPAI_Transport_Send_Data_On_Chnl: "
						   "memory allocation failed for link_idx : %d channel_idx : %d\n",
						   link_idx, chnl_idx);
	}

	return glink_result;
}

void LPAI_Transport_Handle_Glink_Rx(lpai_transport_pipe_msg_t *pipe_evt)
{
	return;
}

#ifdef LPAI_TRANSPORT_THREAD_ENABLED /* If enabled runs from separate thread */
void LPAI_Transport_Task_Handler(void *param)
{
	qurt_pipe_t *lpai_transport_pipe = (qurt_pipe_t *)param;

	lpai_transport_pipe_msg_t *lpai_transport_pipe_event;

	LPAI_Transport_Glink_Start();

	while (lpai_transport_main.thread_running)
	{
		lpai_transport_pipe_event = (lpai_transport_pipe_msg_t *)qurt_pipe_receive(lpai_transport_pipe);

		switch (lpai_transport_pipe_event->pipe_msg_opcode)
		{
		case LPAI_TRANSPORT_LINK_STATE:
		{
			LPAI_Transport_Glink_Handle_Link_State(lpai_transport_pipe_event);
		}
		break;
		case LPAI_TRANSPORT_NOTIFY_CHANNEL_STATE:
		{
			LPAI_Transport_Handle_Glink_Channel_State(lpai_transport_pipe_event);
		}
		break;
		case LPAI_TRANSPORT_NOTIFY_RX_INTENT_REQ:
		{
			LPAI_Transport_Handle_Glink_Rx_Intent(lpai_transport_pipe_event);
		}
		break;
		case LPAI_TRANSPORT_NOTIFY_RX:
		{
			LPAI_Transport_Handle_Glink_Rx(lpai_transport_pipe_event);
		}
		break;
		case LPAI_TRANSPORT_PERFORM_TX:
		{
		}
		break;
		case LPAI_TRANSPORT_NOTIFY_TX_DONE:
		{
			LPAI_Transport_Handle_Glink_Tx_Done(lpai_transport_pipe_event);
		}
		break;
		case LPAI_TRANSPORT_STOP_THREAD:
		{
			qurt_pipe_delete(lpai_transport_pipe);

			lpai_transport_main.thread_running = FALSE;
		}
		break;

		default:
			break;
		}
		LPAI_Transport_Free(lpai_transport_pipe_event);
	}

	qurt_thread_stop();
}
#else
void LPAI_Transport_Handle_Event(void *data)
{
	lpai_transport_pipe_msg_t *lpai_transport_pipe_event = (lpai_transport_pipe_msg_t *)data;
	LPAI_TRANSPORT_LOGM("LPAI_Transport_Handle_Event : link_idx : %d chnl_idx : %d opcode : %d", lpai_transport_pipe_event->link_idx, lpai_transport_pipe_event->chnl_idx, lpai_transport_pipe_event->pipe_msg_opcode);
	switch (lpai_transport_pipe_event->pipe_msg_opcode)
	{
	case LPAI_TRANSPORT_LINK_STATE:
	{
		LPAI_Transport_Glink_Handle_Link_State(lpai_transport_pipe_event);
	}
	break;
	case LPAI_TRANSPORT_NOTIFY_CHANNEL_STATE:
	{
		LPAI_Transport_Handle_Glink_Channel_State(lpai_transport_pipe_event);
	}
	break;
	case LPAI_TRANSPORT_NOTIFY_RX_INTENT_REQ:
	{
		LPAI_Transport_Handle_Glink_Rx_Intent(lpai_transport_pipe_event);
	}
	break;
	case LPAI_TRANSPORT_NOTIFY_RX:
	{
		LPAI_Transport_Handle_Glink_Rx(lpai_transport_pipe_event);
	}
	break;
	case LPAI_TRANSPORT_PERFORM_TX:
	{
	}
	break;
	case LPAI_TRANSPORT_NOTIFY_TX_DONE:
	{
		LPAI_Transport_Handle_Glink_Tx_Done(lpai_transport_pipe_event);
	}
	break;
	case LPAI_TRANSPORT_STOP_THREAD:
	{
	}
	break;

	default:
		break;
	}
	LPAI_Transport_Free(lpai_transport_pipe_event);
}
#endif

#ifdef LPAI_TRANSPORT_THREAD_ENABLED
qurt_thread_t LPAI_Transport_Start(char *thread_name,
								   const int thread_priority,
								   const int thread_stack_size)
{
	qurt_thread_attr_t th_attr;
	qurt_thread_t th_hndl;
	int res;

	qurt_thread_attr_init(&th_attr);
	qurt_thread_attr_set_name(&th_attr, thread_name);
	qurt_thread_attr_set_priority(&th_attr, thread_priority);
	qurt_thread_attr_set_stack_addr(&th_attr, lpai_transport_main.lpai_transport_stack);
	qurt_thread_attr_set_stack_size(&th_attr, thread_stack_size);
	res = qurt_thread_create(
		&th_hndl,
		&th_attr,
		LPAI_Transport_Task_Handler,
		lpai_transport_main.lpai_transport_pipe);

	if (res == QURT_EOK)
	{
		lpai_transport_main.thread_running = TRUE;
		lpai_transport_main.thread_handle = th_hndl;
		return (th_hndl);
	}
	else
	{
		return (NULL);
	}
}

bool LPAI_Transport_Disable(qurt_signal_t *signal)
{
	lpai_transport_pipe_msg_t *pipe_event = LPAI_Transport_Malloc(sizeof(lpai_transport_pipe_msg_t));
	if (pipe_event == NULL)
	{
		LPAI_TRANSPORT_LOGM("LPAI_Transport_Disable : unable to alloc pipe_event");
		return;
	}
	pipe_event->pipe_msg_opcode = LPAI_TRANSPORT_STOP_THREAD;
	LPAI_Transport_Queue_Evt((qurt_pipe_data_t)pipe_event);

	return TRUE;
}
#endif
