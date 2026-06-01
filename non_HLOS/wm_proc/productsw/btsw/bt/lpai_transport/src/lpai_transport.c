/*============================================================================*
Include Files
*============================================================================*/
#include <stdio.h>
#include <stdint.h>
#include "lpai_transport.h"
#include "lpai_transport_internal.h"
#include "stringl.h"
#include <zephyr/sys/printk.h>
#include "lpai_bt_app_mgr.h"
#include "lpai_bt_npa.h"

/* test */
static glink_link_info_type link_cbs[3];
/*============================================================================*
Definitions for GLINK Channels
*============================================================================*/
#define LPAI_TRANSPORT_PIPE_ELEMENT_SIZE  12
#define LPAI_TRANSPORT_PIPE_ELEMENT_COUNT 10

#define GLINK_PADDING     4

#define MAX_GLINK_INTENTS 2

#define MAX_GLINK_INTENT_SIZE 1024

static lpai_transport_glink_interface_t lpai_transport_glink_interfaces[LPAI_TRANSPORT_LINKS_MAX];

static uint8_t link_reg_count = 0;

static int glink_reg_result = 0;


/* Global static configuration for the lpai transport */
static const lpai_transport_static_link_cfg_t LPAI_TRANSPORT_LINK_CFG[] = {
	[LPAI_TRANSPORT_Q6_LINK_IDX] = {
		.remote_name = "lpass",
		.xport_name  = "SMEM",
		.init_fn     = LPAI_Transport_Glink_Q6_Init,
		.channels    = {
							[LPAI_TRANSPORT_Q6_CTRL_CHNL_IDX] = {
							.chnl_name = "am_bt_app_chnl",
							.rx_cb     = NULL,
					   },					   
		},
	},
	[LPAI_TRANSPORT_HLOS_LINK_IDX] = {
		.remote_name = "apss",
		.xport_name  = "SMEM",
		.init_fn     = LPAI_Transport_Glink_HLOS_Init,
		.channels    = {
						[LPAI_TRANSPORT_HLOS_CTRL_CHNL_IDX] = {
							.chnl_name = "contexthub_hal_app_mgr_chnl",
							.rx_cb     = NULL, 
					   }, 
		},
	},
};




K_MSGQ_DEFINE(lpai_bt_transport_msgq , sizeof(lpai_transport_pipe_msg_t),20,1);

K_THREAD_DEFINE(lpai_bt_transport , 1536,
    LPAI_Transport_Task_Handler,NULL,NULL,NULL,
    0x12,0,0);

static lpai_transport_glink_channels_t lpai_transport_glink_channels;
static lpai_transport_main_t lpai_transport_main; /* todo : check the name */

static void LPAI_Transport_Glink_Link_State_Cb(glink_link_info_type *link_info,void *priv);
static void LPAI_Transport_Glink_Channel_State_Cb(glink_handle_type handle,
        const void *priv, glink_channel_event_type event);

static bool LPAI_Transport_Glink_Rx_Intent_Req_Cb(glink_handle_type handle,
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

static glink_err_type LPAI_Transport_Glink_Heap_Allocate_Cb
(
    glink_handle_type handle,       /* handle for the glink channel */
    const void        *priv,        /* priv client data passed in glink_open */
    const void        *pkt_priv,    /* pkt specific private data */
    size_t            intent_size,  /* Intent size */
    void              **buffer_ptr  /* Output buffer pointer */
);

static void LPAI_Transport_Glink_Heap_Deallocate_Cb
(
   glink_handle_type handle,       /* handle for the glink channel */
   const void        *priv,        /* priv client data passed in glink_open */
   const void        *pkt_priv,    /* pkt specific private data */
   size_t            intent_size,  /* Intent size */
   void              *buffer_ptr  /* Output buffer pointer */
);


void LPAI_Transport_Set_Channel_Names(uint8_t link_idx)
{
	lpai_transport_glink_interface_t *link;
	lpai_transport_glink_channel_t *channels;
	uint8_t channels_len;

	link = &lpai_transport_glink_interfaces[link_idx];
	channels = link->channels;
	channels_len = link->channels_len;

	for(int i = 0; i < channels_len; i++) {
		channels[i].open_cfg.name = 
		    LPAI_TRANSPORT_LINK_CFG[link_idx].channels[i].chnl_name;
	}
}

void LPAI_Transport_Glink_HLOS_Init() 
{
	lpai_transport_glink_channel_t* channels;
	lpai_transport_glink_interface_t* link;
	const lpai_transport_static_link_cfg_t* static_link_cfg;

	static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[LPAI_TRANSPORT_HLOS_LINK_IDX];
	channels = lpai_transport_glink_channels.hlos_channels;
	for(int idx = 0; idx < LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX; idx++)
	{
		channels[idx].chnl_state = GLINK_REMOTE_DISCONNECTED;
		channels[idx].idx = idx;
		channels[idx].link_idx = LPAI_TRANSPORT_HLOS_LINK_IDX;
		channels[idx].num_intents = 0;
	}

	link = &lpai_transport_glink_interfaces[LPAI_TRANSPORT_HLOS_LINK_IDX];
	link->link_state = GLINK_LINK_STATE_DOWN;
	link->link_idx = LPAI_TRANSPORT_HLOS_LINK_IDX;
	link->channels = lpai_transport_glink_channels.hlos_channels;
	link->channels_len = LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX;
	link->xport_name = static_link_cfg->xport_name;
	link->remote_name = static_link_cfg->remote_name;
}

void LPAI_Transport_Glink_Q6_Init() 
{
	lpai_transport_glink_channel_t* channels;
	lpai_transport_glink_interface_t* link;
	const lpai_transport_static_link_cfg_t* static_link_cfg;

	static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[LPAI_TRANSPORT_Q6_LINK_IDX];
	for(int idx = 0; idx < LPAI_TRANSPORT_Q6_GLINK_CHNLS_MAX; idx++)
	{
		channels = lpai_transport_glink_channels.q6_channels;
		channels[idx].chnl_state = GLINK_REMOTE_DISCONNECTED;
		channels[idx].idx = idx;
		channels[idx].link_idx = LPAI_TRANSPORT_Q6_LINK_IDX;
		channels[idx].num_intents = 0;
	}

	link = &lpai_transport_glink_interfaces[LPAI_TRANSPORT_Q6_LINK_IDX];
	link->link_state = GLINK_LINK_STATE_DOWN;
	link->link_idx = LPAI_TRANSPORT_Q6_LINK_IDX;
	link->channels = lpai_transport_glink_channels.q6_channels;
	link->channels_len = LPAI_TRANSPORT_Q6_GLINK_CHNLS_MAX;
	link->xport_name = static_link_cfg->xport_name;
	link->remote_name = static_link_cfg->remote_name;
}


void LPAI_Transport_Glink_Init()
{
	lpai_transport_glink_interface_t *link;
	const lpai_transport_static_link_cfg_t* static_link_cfg;
	lpai_transport_glink_init_fn_t init_fn;

	/* call the init function for each transport */
	for(uint8_t i = 0; i < LPAI_TRANSPORT_LINKS_MAX; i++) {
		static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[i];
		init_fn = static_link_cfg->init_fn;
		if(init_fn != NULL) {
			init_fn();
		}
	}
}

void LPAI_Transport_Glink_Register_Link(uint8_t link_idx)
{
	glink_err_type glink_result;
	glink_link_id_type glink_id;
	const lpai_transport_static_link_cfg_t* static_link_cfg;
	void *priv;

    GLINK_LINK_ID_STRUCT_INIT(glink_id);
	static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[link_idx];
    glink_id.xport = static_link_cfg->xport_name;
    glink_id.remote_ss = static_link_cfg->remote_name;
    glink_id.link_notifier = LPAI_Transport_Glink_Link_State_Cb;
	priv = &lpai_transport_glink_interfaces[link_idx].link_idx;

    glink_result = glink_register_link_state_cb(&glink_id, priv);

    if (glink_result != GLINK_STATUS_SUCCESS)
    {
        LPAI_TRANSPORT_LOG("glink_register_link_state_cb for link_idx failed "
                "result code %d\n",link_idx, glink_result);
		glink_reg_result = glink_result;
    }
	else
	{
		link_reg_count++;
	}
}

void LPAI_Transport_Glink_Start()
{
	/* Register for link_registration callback of all the interfaces */
	for(uint8_t i = 0; i < LPAI_TRANSPORT_LINKS_MAX; i++) {
		LPAI_Transport_Glink_Register_Link(i);
	}
}


static int indx = 0;

static void LPAI_Transport_Glink_Link_State_Cb(glink_link_info_type *link_info, void *priv)
{
	lpai_transport_pipe_msg_t pipe_event;

	uint8_t link_idx = *(uint8_t*)priv;
	// uint8_t link_idx = find_link_idx_from_link_info(link_info);


	/* testing code */
	link_cbs[indx] = *link_info;
	indx++;
	/* end */


	lpai_transport_glink_interfaces[link_idx].link_state = link_info->link_state; /* todo : check the names (links instead of interfaces ) */
    pipe_event.pipe_msg_opcode = LPAI_TRANSPORT_LINK_STATE;
	pipe_event.link_idx = link_idx;
	pipe_event.size = 0;

	//qurt_pipe_try_send(lpai_transport_main.lpai_transport_pipe, &pipe_event);
	k_msgq_put(&lpai_bt_transport_msgq,&pipe_event,K_NO_WAIT);
}

void LPAI_Transport_Glink_Open_Channels(uint8_t link_idx)
{
	glink_err_type glink_result;
	lpai_transport_glink_interface_t* link;
	lpai_transport_glink_channel_t* channels;
	uint8_t channels_len;

	link = &lpai_transport_glink_interfaces[link_idx];
	LPAI_Transport_Set_Channel_Names(link_idx);
	channels = link->channels;
	channels_len = link->channels_len;

	btsw_vote_island_exit();

	for(int i = 0; i < channels_len; i++)
	{
		channels[i].open_cfg.transport = link->xport_name;
		channels[i].open_cfg.remote_ss = link->remote_name;
		channels[i].open_cfg.priv = &(channels[i]);
		channels[i].open_cfg.notify_state = LPAI_Transport_Glink_Channel_State_Cb;
		channels[i].open_cfg.notify_rx_intent_req = LPAI_Transport_Glink_Rx_Intent_Req_Cb;
		channels[i].open_cfg.notify_rx = LPAI_Transport_Glink_Rx_Notification_Cb;
		channels[i].open_cfg.notify_tx_done = LPAI_Transport_Glink_Tx_Done_Cb;
        channels[i].open_cfg.options = GLINK_OPT_CLIENT_BUFFER_ALLOCATION;
        channels[i].open_cfg.notify_allocate = LPAI_Transport_Glink_Heap_Allocate_Cb;
        channels[i].open_cfg.notify_deallocate = LPAI_Transport_Glink_Heap_Deallocate_Cb;

		glink_result = glink_open(&channels[i].open_cfg,
		                     &channels[i].glink_hdl);

	    if (glink_result != GLINK_STATUS_SUCCESS)
	    {
	        LPAI_TRANSPORT_LOG("glink_open failed for link_idx : %d channel number %d"
	                "result code %d\n", link_idx, i, glink_result);
	    }
	}

	btsw_vote_island_entry();
}

void LPAI_Transport_Glink_Close_Channels(uint8_t link_idx)
{
	lpai_transport_glink_interface_t *link;

	link = &lpai_transport_glink_interfaces[link_idx];

	btsw_vote_island_exit();

	for(int idx = 0; idx < link->channels_len; idx++)
	{
		/* todo : handle close failure */
		glink_close(link->channels[idx].glink_hdl);
	}

	btsw_vote_island_entry();
}


void LPAI_Transport_Glink_Handle_Link_State(lpai_transport_pipe_msg_t *pipe_evt)
{
	uint8_t link_idx = pipe_evt->link_idx;
	glink_link_state_type link_state = lpai_transport_glink_interfaces[link_idx].link_state;

	switch(link_state)
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
	lpai_transport_pipe_msg_t pipe_event;
	lpai_transport_glink_channel_t* channel = (lpai_transport_glink_channel_t*)priv;

	channel->chnl_state = event;
	pipe_event.pipe_msg_opcode = LPAI_TRANSPORT_NOTIFY_CHANNEL_STATE;
	pipe_event.data_ptr = &(channel->chnl_state);
	pipe_event.chnl_idx = channel->idx;
	pipe_event.link_idx = channel->link_idx;

	//qurt_pipe_try_send(lpai_transport_main.lpai_transport_pipe,&pipe_event);
	k_msgq_put(&lpai_bt_transport_msgq,&pipe_event,K_NO_WAIT);
}

/* Last seen */
void LPAI_Transport_Handle_Glink_Channel_State(lpai_transport_pipe_msg_t *pipe_evt)
{
	uint8_t channel_idx;
	glink_channel_event_type channel_event;
	uint8_t link_idx;
	glink_err_type glink_result;
	indx+=2;

	channel_idx = (pipe_evt->chnl_idx);
	channel_event = *(glink_channel_event_type*)(pipe_evt->data_ptr);
	link_idx = pipe_evt->link_idx;
	lpai_transport_glink_channel_t* channel = &lpai_transport_glink_interfaces[link_idx].channels[channel_idx];

	if (link_idx == LPAI_TRANSPORT_HLOS_LINK_IDX) 
	{
		btsw_vote_island_exit();
	}

	switch(channel_event)
	{
		case GLINK_CONNECTED:
		{
			if (link_idx != LPAI_TRANSPORT_HLOS_LINK_IDX)
			{
				indx+=2;
				glink_result = glink_queue_rx_intent(channel->glink_hdl, NULL, MAX_GLINK_INTENT_SIZE);
				if(glink_result == GLINK_STATUS_SUCCESS)
				{
					channel->num_intents++;
				}
			}
		}
		break;
		case GLINK_REMOTE_DISCONNECTED:
		{

			glink_result = glink_close(channel->glink_hdl);
			if(glink_result == GLINK_STATUS_SUCCESS)
			{
				channel->num_intents = 0;
			}
		}
		break;
		case GLINK_LOCAL_DISCONNECTED:
		{
			extern void handle_bt_off();
			handle_bt_off();
			glink_result = glink_open(&(channel->open_cfg),
		                     &(channel->glink_hdl));
		}
		break;
		default:
		break;
	}

	if (link_idx == LPAI_TRANSPORT_HLOS_LINK_IDX) 
	{
		btsw_vote_island_entry();
	}
}

const char* get_channel_name_from_idx(uint8_t link_idx, uint8_t chnl_idx) {
	return lpai_transport_glink_interfaces[link_idx].channels[chnl_idx].open_cfg.name;
}

static bool LPAI_Transport_Glink_Rx_Intent_Req_Cb(glink_handle_type handle,
        const void *priv, size_t req_size)
{
	glink_err_type glink_result;
	size_t size = req_size;
	lpai_transport_glink_interface_t* link;
	lpai_transport_glink_channel_t* channel;

	channel = (lpai_transport_glink_channel_t*)priv;
	link = &lpai_transport_glink_interfaces[channel->link_idx];

	LPAI_TRANSPORT_LOG("Intent request on channel %s : ", get_channel_name_from_idx(channel->link_idx, channel->chnl_idx));

	/*32-bit aligned*/
	if(size % GLINK_PADDING)
	{
		size = (size + 0x3) & ~(0x3);
	}

	if(channel->num_intents < MAX_GLINK_INTENTS && size <= MAX_GLINK_INTENT_SIZE)
	{
		glink_result = glink_queue_rx_intent(channel->glink_hdl, NULL,size);
		if(glink_result == GLINK_STATUS_SUCCESS)
		{	
			channel->num_intents++;
			return true;
		}
	}
	return false;
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
	lpai_transport_pipe_msg_t pipe_evt;
	lpai_transport_glink_channel_t* channel;
	lpai_transport_rx_cb_t rx_cb;
	glink_err_type glink_result;
	const lpai_transport_static_link_cfg_t* static_link_cfg;

	indx+=2;


	channel = (lpai_transport_glink_channel_t*)priv;
	rx_cb = NULL;
	if (channel != NULL)
	{
        if (ptr == NULL || size == 0 || size > MAX_GLINK_INTENT_SIZE)
        {
            LPAI_TRANSPORT_LOG("Invalid Rx data: ptr=%p, size=%zu on link_idx:%d, channel_idx:%d\n",
                              ptr, size, channel->link_idx, channel->idx);
            glink_rx_done(channel->glink_hdl, ptr, true);
            return;
        }

		void *data_ptr = malloc(size);
		static_link_cfg = &LPAI_TRANSPORT_LINK_CFG[channel->link_idx];
		if (data_ptr == NULL)
		{
			/* fatal */
			LPAI_TRANSPORT_LOG("Not Enough Memory to allocate for Incoming Rx data \
			                    on link_idx : %d, channel_idx : %d \n", channel->link_idx,
								                                 channel->idx);
            glink_rx_done(channel->glink_hdl, ptr, true);
		}
		else
		{
			memscpy(data_ptr,size,ptr,size);

			/* reuse the intent */
			glink_result = glink_rx_done(channel->glink_hdl, ptr, true);
            if(glink_result != GLINK_STATUS_SUCCESS)
            {
                LPAI_TRANSPORT_LOG("glink_rx_done failed with error %d\n", glink_result);
                free(data_ptr);
                return;
            }

			lpai_bt_app_mgr_send_dataEvent(LPAI_BT_APP_MGR_ADSP_RX,size,data_ptr);

			/* Call the corresponding api for rx */
			rx_cb = static_link_cfg->channels[channel->idx].rx_cb;
			if(rx_cb != NULL) {
				rx_cb((uint8_t*)data_ptr,size);
			}
		}
	}
}

static void LPAI_Transport_Glink_Tx_Done_Cb(glink_handle_type handle,
        const void *priv, const void *pkt_priv,
        const void *ptr, size_t size)
{
	lpai_transport_pipe_msg_t pipe_evt;
	lpai_transport_glink_channel_t *channel = (lpai_transport_glink_channel_t*)priv;
    
	pipe_evt.chnl_idx = channel->idx;
	pipe_evt.link_idx = channel->link_idx;
	pipe_evt.data_ptr = ptr;
	pipe_evt.pipe_msg_opcode = LPAI_TRANSPORT_NOTIFY_TX_DONE;

	//qurt_pipe_try_send(lpai_transport_main.lpai_transport_pipe,&pipe_evt);
	k_msgq_put(&lpai_bt_transport_msgq,&pipe_evt,K_NO_WAIT);
}

void LPAI_Transport_Handle_Glink_Tx_Done(lpai_transport_pipe_msg_t *pipe_evt)
{
	if(pipe_evt->data_ptr != NULL) {
		// void *ptr;
		// memcpy(ptr, pipe_evt->data_ptr, sizeof(ptr));
		// free(ptr);
		free(pipe_evt->data_ptr);
	}
}

/**
 * @brief This callback is called from GLINK module to get allocated memory for intent by GLINK Client.
 * This callback is triggered when intent is queued with glink_queue_rx_intent.
 * @param[in] handle handle for the glink channel
 * @param[in] priv priv client data passed in glink_open
 * @param[in] pkt_priv pkt specific private data
 * @param[in] intent_size Intent size
 * @param[out] buffer_ptr Output buffer pointer
 * @return Returns glink_err_type
 * @note This API is called only if GLINK_OPT_CLIENT_BUFFER_ALLOCATION is set during glink_open.
 */
 static glink_err_type LPAI_Transport_Glink_Heap_Allocate_Cb
 (
     glink_handle_type handle,       /* handle for the glink channel */
     const void        *priv,        /* priv client data passed in glink_open */
     const void        *pkt_priv,    /* pkt specific private data */
     size_t            intent_size,  /* Intent size */
     void              **buffer_ptr  /* Output buffer pointer */
 )
{
    lpai_transport_glink_channel_t* channel;
    glink_err_type ret = GLINK_STATUS_INVALID_PARAM;

    /*explict NULL pointer initialization */
    if (buffer_ptr)
    {
        *buffer_ptr = NULL;
    }

    /* NULL Pointer validation checks */
    if (!buffer_ptr || !priv)
    {
        LPAI_TRANSPORT_LOG("Heap_Allocate_Cb Null poiter check %d\n", ret);
        return ret;
    }

    /* Intent size validation checks */
    if (intent_size <= 0 || intent_size > MAX_GLINK_INTENT_SIZE)
    {
        LPAI_TRANSPORT_LOG("Heap_Allocate_Cb Invalid Intent Size %d\n", ret);
        return ret;
    }

    channel = (lpai_transport_glink_channel_t*)priv;
    if (channel->glink_hdl == handle)
    {
        *buffer_ptr = malloc(intent_size);
        if (*buffer_ptr == NULL)
        {
            LPAI_TRANSPORT_LOG("Heap_Allocate_Cb: malloc failed for size %d\n", intent_size);
            ret = GLINK_STATUS_OUT_OF_RESOURCES;
        }
        else
        {
            ret = GLINK_STATUS_SUCCESS;
        }
    }
    return ret;
}

 /**
  * @brief This callback is called from GLINK module to free already allocated memory for intent by GLINK Client.
  * This callback is triggered when intent is rx_done() is called with re-use=false, or when GLINK channel gets disconnected.
  * @param[in] handle handle for the glink channel
  * @param[in] priv priv client data passed in glink_open
  * @param[in] pkt_priv pkt specific private data
  * @param[in] intent_size Intent size
  * @param[in] buffer_ptr buffer pointer to be freed
  * @note This API is called only if GLINK_OPT_CLIENT_BUFFER_ALLOCATION is set during glink_open.
  */
 static void LPAI_Transport_Glink_Heap_Deallocate_Cb
 (
    glink_handle_type handle,       /* handle for the glink channel */
    const void        *priv,        /* priv client data passed in glink_open */
    const void        *pkt_priv,    /* pkt specific private data */
    size_t            intent_size,  /* Intent size */
    void              *buffer_ptr  /* Output buffer pointer */
 )
{
    lpai_transport_glink_channel_t* channel;
    if (!priv || !buffer_ptr)
    {
        LPAI_TRANSPORT_LOG("Heap_Deallocate_Cb Invalid Param\n");
        return;
    }

    channel = (lpai_transport_glink_channel_t*)priv;
    if (channel->glink_hdl == handle)
    {
        free(buffer_ptr);
        buffer_ptr = NULL;
    }
    else
    {
        LPAI_TRANSPORT_LOG("Heap_Deallocate_Cb Invalid handle\n");
    }
}

void LPAI_Transport_Init()
{
	// qurt_pipe_attr_t   pipe_attr;
	
	// qurt_pipe_attr_init(&pipe_attr);
	// qurt_pipe_attr_set_elements(&pipe_attr, LPAI_TRANSPORT_PIPE_ELEMENT_COUNT);
	// qurt_pipe_attr_set_element_size(&pipe_attr, LPAI_TRANSPORT_PIPE_ELEMENT_SIZE);
	// qurt_pipe_create(&(lpai_transport_main.lpai_transport_pipe), &pipe_attr);

	LPAI_Transport_Glink_Init();
}

glink_err_type LPAI_Transport_Send_Data_On_Chnl(uint8_t link_idx, uint8_t chnl_idx, const void *ptr , size_t size)
{
	glink_err_type glink_result = GLINK_STATUS_FAILURE;

	size_t req_size = size;
	lpai_transport_glink_interface_t *link = &lpai_transport_glink_interfaces[link_idx];
	lpai_transport_glink_channel_t *channel = &(link->channels[chnl_idx]);

	if(channel->chnl_state != GLINK_CONNECTED) {
		LPAI_TRANSPORT_LOG("Can't do tx on channel with link_idx : %d and channel idx : %d"
		                    " because the channel state is : %d", 
		                    link_idx, chnl_idx, channel->chnl_state);
		return glink_result;
	}
	void *ctrl_buf;
	if(req_size % GLINK_PADDING != 0)
	{
		req_size = (req_size + 0x3) & ~(0x3);
	}

	ctrl_buf = malloc(req_size);

	if(ctrl_buf)
	{
		memscpy(ctrl_buf,size,ptr,size);

		glink_result = glink_tx(channel->glink_hdl, NULL, (uint32_t*)ctrl_buf, req_size, 0);
		
		if(glink_result != GLINK_STATUS_SUCCESS)
		{
			LPAI_TRANSPORT_LOG("glink_tx failed\n", glink_result);
			free(ctrl_buf);
		}
	}
	else
	{
		LPAI_TRANSPORT_LOG("\n LPAI_Transport_Send_Data_On_Chnl: "
		"memory allocation failed for link_idx : %d channel_idx : %d\n", link_idx, chnl_idx);
	}

	return glink_result;
}

inline glink_err_type LPAI_Transport_SendDataFromMicroAppsToQ6(const void *ptr , size_t size) 
{
       return LPAI_Transport_Send_Data_On_Chnl(LPAI_TRANSPORT_Q6_LINK_IDX, 
                  LPAI_TRANSPORT_Q6_CTRL_CHNL_IDX, ptr, size);
}

inline glink_err_type LPAI_Transport_SendDataFromMicroAppsToHLOS(const void *ptr , size_t size) 
{
       return LPAI_Transport_Send_Data_On_Chnl(LPAI_TRANSPORT_HLOS_LINK_IDX, 
                  LPAI_TRANSPORT_HLOS_CTRL_CHNL_IDX, ptr, size);
}

void LPAI_Transport_Handle_Glink_Rx(lpai_transport_pipe_msg_t *pipe_evt)
{
	return;
}

void LPAI_Transport_Task_Handler(void *param1 , void *param2 , void *param3)
{
	//qurt_pipe_t lpai_transport_pipe = (qurt_pipe_t)param;

	lpai_transport_pipe_msg_t lpai_transport_pipe_event;

	/* TODO : Check this again */
	//qurt_allocate_secure_context(QURT_THREAD_SECURE_CALLABLE_STACK_SIZE_MID);
	LPAI_Transport_Init();
	LPAI_Transport_Glink_Start();
	//while(lpai_transport_main.thread_running)
	while(1)
	{
		//qurt_pipe_receive(lpai_transport_pipe,&lpai_transport_pipe_event);

		k_msgq_get(&lpai_bt_transport_msgq, &lpai_transport_pipe_event,  K_FOREVER);

		switch(lpai_transport_pipe_event.pipe_msg_opcode)
		{
			case LPAI_TRANSPORT_LINK_STATE:
			{
				LPAI_Transport_Glink_Handle_Link_State(&lpai_transport_pipe_event);
			}
			break;
			case LPAI_TRANSPORT_NOTIFY_CHANNEL_STATE:
			{
				LPAI_Transport_Handle_Glink_Channel_State(&lpai_transport_pipe_event);
			}
			break;
			case LPAI_TRANSPORT_NOTIFY_RX_INTENT_REQ:
			{
				LPAI_Transport_Handle_Glink_Rx_Intent(&lpai_transport_pipe_event);
			}
			break;
			case LPAI_TRANSPORT_NOTIFY_RX:
			{
				LPAI_Transport_Handle_Glink_Rx(&lpai_transport_pipe_event);
			}
			break;
			case LPAI_TRANSPORT_PERFORM_TX:
			{
	
			}
			break;
			case LPAI_TRANSPORT_NOTIFY_TX_DONE:
			{
				LPAI_Transport_Handle_Glink_Tx_Done(&lpai_transport_pipe_event);
			}
			break;
			case LPAI_TRANSPORT_STOP_THREAD:
			{
                //qurt_pipe_delete(lpai_transport_pipe);
               
                lpai_transport_main.thread_running = false;
			}
			break;
			default:
			break;
		}
	}

	qurt_thread_stop();
}


qurt_thread_t LPAI_Transport_Start(const uint8_t *thread_name,
        uint16_t thread_priority,
        uint32_t thread_stack_size)
{
	qurt_thread_attr_t th_attr;
	qurt_thread_t      th_hndl;
	int res;

	qurt_thread_attr_init(&th_attr);
	qurt_thread_attr_set_name(&th_attr, (const char*)thread_name);
	qurt_thread_attr_set_priority(&th_attr, thread_priority);
	qurt_thread_attr_set_stack_size(&th_attr, thread_stack_size);
	qurt_thread_attr_set_privileged(&th_attr);
	res = qurt_thread_create(
			&th_hndl,
			&th_attr,
			LPAI_Transport_Task_Handler,
			&lpai_transport_main.lpai_transport_pipe);

	if (res == QURT_EOK)
	{
		lpai_transport_main.thread_running = true;
		lpai_transport_main.thread_handle = th_hndl;
		return(th_hndl);
	}
	else
	{
		return(NULL);
	}
}

bool LPAI_Transport_Disable (qurt_signal_t* signal)
{
	lpai_transport_pipe_msg_t pipe_evt;
    pipe_evt.pipe_msg_opcode = LPAI_TRANSPORT_STOP_THREAD;
    qurt_pipe_try_send_to_front(lpai_transport_main.lpai_transport_pipe,&pipe_evt);
    
    return true;
}


/* TODOS : 
1. handle number of intents separately per channel ask Tanish
2. ask vishwas if same pair of subsystems have more than two transports 
3. try to put abstraction with respect to callbacks do this
4. doxygen, naming convention, check with Nayla, 
5. memory map vs ss_transport, try to abstract glink from application.. 
6. testing on Varshas code, 
7. Testcase and documentation, add test plan to confluence
*/


/*
To be discussed with Venkatesh and Tanish
1. How can we put a small shim in place,   maybe add some configuration and provide some init api
2. Handling of multiple transports with same pair of subsystems. 
3. subtask the lecoc and glink
*/