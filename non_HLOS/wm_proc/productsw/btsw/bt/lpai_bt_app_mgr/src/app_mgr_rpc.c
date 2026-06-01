/*************************************************************************
 * @file     app_mgr_rpc.c
 * @brief    BT App Manager RPC source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <zephyr/sys/printk.h>
#include "lpai_bt_app_mgr.h"
#include "lpai_bt_heap.h"
#include "lpai_bt_app_mgr_rpc.h"
#include "lpai_transport.h"
#include "lpai_bt_state_mgr.h"
#include "lpai_bt_hal_mgr.h"
#include "lpai_bt_app_mgr_adsp_handler.h"

/*===========================================================================
                        EXTERNAL DATA DECLARATIONS
===========================================================================*/



/*===========================================================================
                        PUBLIC/GLOBAL DATA DEFINITIONS
===========================================================================*/
uint8_t app_mgr_transport_buf[1024] __aligned(4);

extern appMgrContext_t appMgrCtx;


/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/


/**
 * @brief Method to find if a recevied end point exists and return the index where end point identifier is stored
 * @param[in]   endPointId    End Pointe Identifier to be Searched
 * @param[out]  None
 * @return      index         Index where end point identifier is stored in the global app manager context , -1 if it does not exist
 */
static int8_t bt_app_mgr_find_endpoint(uint64_t endPointId)
{
	for(uint8_t idx = 0; idx < MAX_END_POINTS_SUPPORTED; idx++)
	{
		if(appMgrCtx.appContext[idx].endPointDetail.endPointId.epId == endPointId)
		{
			return idx;
		}
	}

	return -1;
}


/**
 * @brief  Method to Handle Socket Open Request Received from ADSP for Socket Offload.
 * After end point is validated , the corrosponding callback function for an end point is invoked to handle it appropriately
 * @param[in]  endPointId     End Point Identifier
 * @param[in]  eventId        Event Identifier for Socket Open Request
 * @param[in]  socketData     Pointer to Socket Context Information for Socket Open
 * @param[in]  dataLen        Length of the Socket Context Information for Socket Open
 * @param[out] None
 * @return     None
 */
static void lpai_bt_app_mgr_handle_open_socket_req(uint64_t endPointId, uint16_t eventId, uint8_t *socketData , uint16_t dataLen , bool proto_encoded)
{

	int8_t endPointIdx = bt_app_mgr_find_endpoint(endPointId);
	if(endPointIdx != -1)
	{
		/**Send the Socket Open Request to Appropriate End Point for Handling */
		appMgrCtx.appContext[endPointIdx].appCb(endPointId, eventId , dataLen, (void *)socketData,proto_encoded);
	}
	else
	{
		/**Todo : To be decided if a such a response is to be sent*/
		uapp_socket_open_rsp_t socket_open_rsp;
		socket_open_rsp.status = SOCKET_OPEN_ENDPOINT_NOT_FOUND;
		socket_open_rsp.socket_id = 0xFF;
		lpai_bt_appmgr_send_endpt_msg_adsp(endPointId,UAPP_OPEN_SOCKET_RES,sizeof(socket_open_rsp),&socket_open_rsp, false);
	}

}


/**
 * @brief  Method to Handle Socket Close Command Received from ADSP once Socket Close is performed.
 * After end point is validated , the corrosponding callback function for an end point is invoked to handle it appropriately
 * @param[in]  endPointId     End Point Identifier
 * @param[in]  eventId        Event Identifier for Socket Close Command
 * @param[in]  socketData     Pointer to Socket Context Information for Socket Close
 * @param[in]  dataLen        Length of the Socket Context Information for Socket Close
 * @param[out] None
 * @return     None
 */
static void lpai_bt_app_manager_handle_close_socket_cmd(uint64_t endPointId , uint16_t eventId , uint8_t *socketData , uint16_t dataLen , bool proto_encoded)
{
    
	int8_t endPointIdx = bt_app_mgr_find_endpoint(endPointId);
	if(endPointIdx != -1)
	{
		/**Send the Socket Open Request to Appropriate End Point for Handling */
		appMgrCtx.appContext[endPointIdx].appCb(endPointId, eventId , dataLen, (void *)socketData,proto_encoded);
	}
	else
	{
		/**Todo : To be decided if Error Response is to be sent for Invalid end point*/
		printk("Close Request Received for Invalid Socket");
	}
}


/**
 * @brief  Method to Handle Data Tx Response Received from ADSP.
 * After end point is validated , the corrosponding callback function for an end point is invoked to handle tx responses and perform further operations
 * if required
 * @param[in]  endPointId     End Point Identifier
 * @param[in]  eventId        Event Identifier for Tx Data Response
 * @param[in]  socketData     Pointer to Socket Context Information for Tx Data Response
 * @param[in]  dataLen        Length of the Socket Context Information for Tx Data Response
 * @param[out] None
 * @return     None
 */
static void lpai_bt_app_mgr_handle_data_rsp(uint64_t endPointId ,uint16_t eventId ,uint8_t *socketData , uint16_t dataLen ,bool proto_encoded)
{
    int8_t endPointIdx = bt_app_mgr_find_endpoint(endPointId);
	if(endPointIdx != -1)
	{
		/**Send the Socket Open Request to Appropriate End Point for Handling */
		appMgrCtx.appContext[endPointIdx].appCb(endPointId, eventId , dataLen, (void *)socketData,proto_encoded);
	}
	else
	{
		/**Todo : To be decided if Error Response is to be sent for Invalid end point*/
		printk("Data Received for Invalid End Point\n");
	}

    
}


/**
 * @brief  Method to Handle Data Rx Indications Received from ADSP.
 * After end point is validated , the corrosponding callback function for an end point is invoked to handle rx indications
 * @param[in]  endPointId     End Point Identifier
 * @param[in]  eventId        Event Identifier for Rx Data Indication
 * @param[in]  socketData     Pointer to Socket Context Information for Rx Data Indication
 * @param[in]  dataLen        Length of the Socket Context Information for Rx Data Indication
 * @param[out] None
 * @return     None
 */
static void lpai_bt_app_mgr_handle_data_ind(uint64_t endPointId ,uint16_t eventId ,uint8_t *socketData , uint16_t dataLen , bool proto_encoded)
{
    int8_t endPointIdx = bt_app_mgr_find_endpoint(endPointId);
	if(endPointIdx != -1)
	{
		/**Send the Socket Open Request to Appropriate End Point for Handling */
		appMgrCtx.appContext[endPointIdx].appCb(endPointId, eventId , dataLen, (void *)socketData,proto_encoded);
	}
	else
	{
		/**Todo : To be decided if Error Response is to be sent for Invalid end point*/
		printk("Data Received for Invalid End Point\n");
	}
}


app_mgr_rpc_header_t *
app_mgr_rpc_make_header(uint16_t command_type, uint16_t opcode, uint16_t message_format,
                          uint64_t ep_id, int extra_size, int *out_size)
{
    int packet_size = APP_MGR_RPC_HDR_SIZE + extra_size;
    app_mgr_rpc_header_t *header;

    *out_size = packet_size;
    header = (app_mgr_rpc_header_t *)app_mgr_transport_buf;

    header->command_type = command_type;
    header->opcode = opcode;
    header->message_format = message_format;
    header->endpoint_id = ep_id;

    return header;
}

void *
app_mgr_rpc_get_empty_msg()
{
    app_mgr_rpc_header_t *header;

    header = (app_mgr_rpc_header_t *)app_mgr_transport_buf;

    return (void*)(header->data);
}

static void app_mgr_rpc_header_init_default(app_mgr_rpc_header_t *header)
{
	header->command_type = OFFLOAD_APP_CMD;
	header->opcode = 0;
	header->message_format = MESSAGE_FORMAT_RAW;
	header->data_len = 0;
	header->endpoint_id = ((uint64_t)0xFBFBFBFBFBFBFB0A);
}

app_mgr_rpc_header_t *app_mgr_rpc_get_default_header(void)
{
    app_mgr_rpc_header_t *header = (app_mgr_rpc_header_t *)app_mgr_transport_buf;
    app_mgr_rpc_header_init_default(header);
    return header;
}

static void app_mgr_rpc_handle_offloaded_evts(app_mgr_rpc_header_t *header) {
	bool proto_encoded = (header->message_format == MESSAGE_FORMAT_PROTO)? true : false;
	switch(header->opcode) {
		case UAPP_BT_STATUS:
		{
			lpai_bt_status_evt(*((bt_status_t *)header->data));
			return;
		}
		break;

		case UAPP_ENDPT_DISC_REQ:
		{
			/** Endpt discovery */
			printk("End Point Discovery Request Received\n");
			lpai_proto_out_buff proto_buff;
			proto_buff.out_buff = bt_heap_alloc(BTSW_COMMON_HEAP,PROTO_BUFF_SIZE);
			proto_buff.size = PROTO_BUFF_SIZE;
			if(encode_end_point_discovery_rsp(&proto_buff,&appMgrCtx))
			{
				printk("Encoded Length : %d\n",proto_buff.size);
				lpai_bt_appmgr_send_endpt_discovery_response_adsp(proto_buff.size,proto_buff.out_buff);
			}
			else
			{
				printk("Encoding Failed !");
			}
			bt_heap_free(BTSW_COMMON_HEAP,proto_buff.out_buff);
			return;
		}
		break;

		case UAPP_OPEN_SOCKET_REQ:
		{
			lpai_bt_app_mgr_handle_open_socket_req(header->endpoint_id , UAPP_OPEN_SOCKET_REQ, header->data ,header->data_len,proto_encoded);
		}
		break;
		case UAPP_DATA_TX_RES:
		{
			lpai_bt_app_mgr_handle_data_rsp(header->endpoint_id,UAPP_DATA_TX_RES,header->data,header->data_len,proto_encoded);
		}
		break;
		case UAPP_DATA_RX_IND:
		{
			lpai_bt_app_mgr_handle_data_ind(header->endpoint_id,UAPP_DATA_RX_IND,header->data,header->data_len,proto_encoded);
		}
		break;
		case UAPP_SOCKET_CLOSE_CMD:
		{
			lpai_bt_app_manager_handle_close_socket_cmd(header->endpoint_id , UAPP_SOCKET_CLOSE_CMD, header->data, header->data_len,proto_encoded);
		}
		break;
	}
	
	#ifdef CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER
		bool is_gatt_msg = gatt_mgr_check_endpoint(header->endpoint_id);
		if(is_gatt_msg) {
			/* for now only gatt */
			extern void gatt_mgr_evt_handler(uint16_t opcode, uint64_t ep_id, uint8_t *data, uint16_t data_len);
			gatt_mgr_evt_handler(header->opcode, header->endpoint_id, header->data, header->data_len - sizeof(app_mgr_rpc_header_t));
			return;
		}
	#endif
	
}




/**
 * @brief  Method to Handle Events for MicroApps registered with LPAI BT App Manager on AWM
 * The Method is invoked each time an event is received from ADSP for any microapp on AWM
 * @param[in]   dataBuff  Data Buffer with information related to MicroApps on AWM
 * @param[in]   len       Length of incoming data in the Data Buffer from ADSP
 * @param[out]  None
 * @return      None
 */
static void app_mgr_rpc_handle_microapp_evts(const uint8_t* dataBuff, size_t len)
{
	printk("Handling Events for NONOFFLOADED APPS \n");
	app_mgr_rpc_header_t *header = (app_mgr_rpc_header_t *)dataBuff;
	bool proto_encoded = (header->message_format == MESSAGE_FORMAT_PROTO)? true : false;
	for(uint8_t i=0;i<MAX_MICROAPPS_SUPPORTED;i++)
	{
		if(appMgrCtx.microAppContext[i].appHandle == header->endpoint_id)
		{
			appMgrCtx.microAppContext[i].appCb(header->opcode,header->data_len,header->data,proto_encoded);
			return;
		}
	}
	printk("No Micropp with this app Handle Found\n");
}

/**
 * @brief  Method to Handle Events for End Points and MicroApps registered with LPAI BT App Manager on AWM
 * The Method is invoked each time an event is received from ADSP for any of BT realted activites taking
 * place across the system.
 * @param[in]   dataBuff  Data Buffer with information related to End Points or MicroApps on AWM
 * @param[in]   len       Length of incoming data in the Data Buffer from ADSP
 * @param[out]  None
 * @return      None
 */
void app_mgr_rpc_adsp_evt_handler(const uint8_t* dataBuff, uint16_t len)
{
    if(dataBuff != NULL)
    {
    	app_mgr_rpc_header_t *header = (app_mgr_rpc_header_t *)dataBuff;
		//printk("app_mgr_rpc_adsp_evt_handler[%04x] data_len: %d\n", header->opcode, header->data_len);
    	switch(header->command_type)
    	{
    		case OFFLOAD_APP_CMD:
    		{
				app_mgr_rpc_handle_offloaded_evts(header);
    		}
    		break;
    		case NONOFFLOAD_APP_CMD:
    		{
    			app_mgr_rpc_handle_microapp_evts(dataBuff,len);
    		}
    		break;

    		default:
    			break;
    	}
    }
    else
    {
    	printk("No Handling to be performed as data is not available\n");
    }

}



/**
 * @breif Method for end points to send their response to ADSP.The client only needs to send application data which is intended
 * for the receiving entity on ADSP. The additional task of adding the appropriate header to the application data/response
 * is handled by the Bt App Manager.
 * Any memory if dynamically allocated will need to be freed by the Client Applcaition using the interface API.
 * Success is returned if the data was successfully sent over Glink.
 * In case of any failures , the retry decision lies with the Client Application
 * @param[in]   endPointId  Identity of the End Point Sending the Message
 * @param[in]   opcode      Message Opcode to identify the Message
 * @param[in]   dataLen     Length of the data to be send to ADSP
 * @param[in]   appDataBuf  Data Buffer containing the Application Data
 * @param[out]  None
 * @return      int Return Type to indicate if the data was sent successfully , failure otherwise
 */
int app_mgr_rpc_send_endpt_msg_adsp(uint64_t endPointId, uint16_t opcode , uint16_t dataLen, bool proto_enabled)
{
	glink_err_type result = GLINK_STATUS_FAILURE;
	app_mgr_rpc_header_t *header = app_mgr_rpc_get_default_header();
	int total_data_len = dataLen + sizeof(app_mgr_rpc_header_t);
	header->command_type = OFFLOAD_APP_CMD;
	if (proto_enabled)
	{
		header->message_format = MESSAGE_FORMAT_PROTO;
	}
	else 
	{
		header->message_format = MESSAGE_FORMAT_RAW;
	}
	header->data_len = dataLen;
	header->opcode = opcode;
	header->endpoint_id = endPointId;
	//printk("dataLen[%d] total_data_len[%d]\n", dataLen, total_data_len);
	
	result =  LPAI_Transport_SendDataFromMicroAppsToQ6(header,total_data_len);

	return result;
}


glink_err_type lpai_bt_appmgr_send_endpt_discovery_response_adsp(uint16_t dataLen , const void *appDataBuf)
{
	uint8_t *data_ptr = NULL;
	glink_err_type result = GLINK_STATUS_FAILURE;
	uint16_t header_size = sizeof(app_mgr_rpc_header_t);
	uint16_t total_data_len = dataLen + header_size;

	data_ptr = malloc(total_data_len);
	if(data_ptr != NULL)
	{
		app_mgr_rpc_header_t header;
		header.command_type = OFFLOAD_APP_CMD;
		header.message_format = MESSAGE_FORMAT_PROTO;
		header.data_len = dataLen;
		header.opcode = UAPP_ENDPT_DISC_RES;
		header.endpoint_id = 0xFFFF;
		memscpy(data_ptr,header_size,&header,header_size);
		memscpy(data_ptr+header_size,dataLen,appDataBuf,dataLen);
		result =  LPAI_Transport_SendDataFromMicroAppsToQ6(data_ptr,total_data_len);
		printk("Result for End Point Discovery Response : %d\n",result);
	}
	else
	{
		printk("Memory Allocation Failed for Tx Operation");
	}

	free(data_ptr);
	data_ptr = NULL;
	return result;
}




glink_err_type lpai_bt_appmgr_send_microapp_msg_adsp(uint16_t opcode , uint16_t dataLen , void *message)
{

	glink_err_type result = GLINK_STATUS_FAILURE;
	
	if(appMgrCtx.btStatus == BT_STATUS_OFF)
	{
		printk("BT Not Enabled , No data can be transferred");
		return GLINK_STATUS_FAILURE;
	}
	uint8_t *data_ptr = NULL;
	uint16_t header_size = sizeof(app_mgr_rpc_header_t);
	uint16_t total_data_len = dataLen + header_size;

	data_ptr = malloc(total_data_len);
	if(data_ptr != NULL)
	{
		app_mgr_rpc_header_t header;
		header.command_type = NONOFFLOAD_APP_CMD;
		header.message_format = MESSAGE_FORMAT_RAW;
		header.data_len = dataLen;
		header.opcode = opcode;
		header.endpoint_id = 0xFFFF;

		memscpy(data_ptr,header_size,&header,header_size);
		memscpy(data_ptr+header_size,dataLen,message,dataLen);

		result = LPAI_Transport_SendDataFromMicroAppsToQ6(data_ptr , total_data_len);
		printk("Micoapp Glink Tx : %d \n",result);
	}
	else
	{
		printk("Memory Allocation Failed for Tx Operation");
	}
	free(data_ptr);
	return result;
}





/**
 * @breif Method for end points to send their response to ADSP.The client only needs to send application data which is intended
 * for the receiving entity on ADSP. The additional task of adding the appropriate header to the application data/response
 * is handled by the Bt App Manager.
 * Any memory if dynamically allocated will need to be freed by the Client Applcaition using the interface API.
 * Success is returned if the data was successfully sent over Glink.
 * In case of any failures , the retry decision lies with the Client Application
 * @param[in]   endPointId  Identity of the End Point Sending the Message
 * @param[in]   opcode      Message Opcode to identify the Message
 * @param[in]   dataLen     Length of the data to be send to ADSP
 * @param[in]   appDataBuf  Data Buffer containing the Application Data
 * @param[out]  None
 * @return      glink_err_type Return Type to indicate if the data was sent successfully , failure otherwise
 */
glink_err_type lpai_bt_appmgr_send_endpt_msg_adsp(uint64_t endPointId , uint16_t opcode , uint16_t dataLen , const void *appDataBuf, bool proto_enabled)
{
	glink_err_type result = GLINK_STATUS_FAILURE;
	
	if(appMgrCtx.btStatus == BT_STATUS_OFF)
	{
		printk("BT Not Enabled , No data can be transferred");
		return GLINK_STATUS_FAILURE;
	}
	uint8_t *data_ptr = NULL;
	uint16_t header_size = sizeof(endpt_mgr_rpc_header_t);
	uint16_t total_data_len = dataLen + header_size;

	data_ptr = malloc(total_data_len);
	if(data_ptr != NULL)
	{
		endpt_mgr_rpc_header_t header;
		header.cmd_type = OFFLOAD_APP_CMD;
        if (proto_enabled)
        {
            header.message_format = MESSAGE_FORMAT_PROTO;
        }
        else 
        {
            header.message_format = MESSAGE_FORMAT_RAW;
        }
		header.data_len = dataLen;
		header.opcode = opcode;
		header.endpoint_id = endPointId;
		memscpy(data_ptr,header_size,&header,header_size);
        if (dataLen)
    		memscpy(data_ptr+header_size,dataLen,appDataBuf,dataLen);
		result =  LPAI_Transport_SendDataFromMicroAppsToQ6(data_ptr,total_data_len);
	}
	else
	{
		printk("Memory Allocation Failed for Tx Operation");
	}

    free(data_ptr);
    data_ptr = NULL;
    return result;
}

