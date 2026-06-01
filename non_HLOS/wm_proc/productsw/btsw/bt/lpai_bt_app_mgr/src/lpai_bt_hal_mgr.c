/**************************************************************************
 * @file     lpai_bt_hal_mgr.c
 * @brief    LPAI BT HAL Manager Source file.
* 			 It contains functions for encoding end point discovery resposne from ADSP
			 and other messages from APSS.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

 /*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "lpai_bt_hal_mgr.h"
#include "pb_encode.h"
#include "contexthub_hal.pb.h"
#include "bt_utilities.h"
#include <zephyr/sys/printk.h>
#include "lpai_bt_heap.h"


/*===========================================================================
                      FUNCTION DEFINITIONS
===========================================================================*/
/**
 * @brief Encodes the End Point Discovery Response to be Sent from AWM to ADSP
 *        when it is requested from APSS .
 * @param[in]  out_buff  Pointer to Output Buffer where encoded string is stored after encoding is performed
 * @param[in]  appMgrCtx Pointer to App Manager Context Structure which store information related to end points
 * @param[out] out_buff  Output Buffer Pointer which stores encoded string
 * @return     bool      true if Encoding is successful, false otherwise
 */
bool encode_end_point_discovery_rsp(lpai_proto_out_buff *proto_buff,appMgrContext_t *appMgrCtx)
{
    bool encode = false;

	pb_ostream_t stream = pb_ostream_from_buffer(proto_buff->out_buff,proto_buff->size);

	google_contexthub_proto_EndpointDiscoveyRsp endpt_discovery_rsp = google_contexthub_proto_EndpointDiscoveyRsp_init_default;

	endpt_discovery_rsp.num_endpoints = appMgrCtx->numEndPointsRegisterd;

	endpt_discovery_rsp.endpoints.arg = appMgrCtx;
	endpt_discovery_rsp.endpoints.funcs.encode = encode_end_point_details;
	encode = pb_encode(&stream, google_contexthub_proto_EndpointDiscoveyRsp_fields, &endpt_discovery_rsp);

	if(!encode)
		printk("Encoding error : %s\n",stream.errmsg);
    
	proto_buff->size = stream.bytes_written;

	return encode;
}


/**
 * @brief Encodes the End Point Details for each end point during end point discovery Response.
 * @param[in]  stream  Protocol Buffer Output Stream
 * @param[in]  field   Protocol Buffer Fields requied for encoding particular structures
 * @param[in]  arg     Pointer to argument passed for a function which encodes callback fields
 * @return     bool    true if Encoding is successful, false otherwise
 */
bool encode_end_point_details(pb_ostream_t * stream, const pb_field_t * field, void * const * arg )
{
	bool success = true;
    appMgrContext_t *app_mgr_ctx = (appMgrContext_t *)*arg;
	google_contexthub_proto_EndpointDetails *endpt_details = bt_heap_alloc(BTSW_COMMON_HEAP,sizeof(google_contexthub_proto_EndpointDetails) * (app_mgr_ctx->numEndPointsRegisterd));
	if(endpt_details != NULL)
	{
		for(uint8_t i=0; i<app_mgr_ctx->numEndPointsRegisterd; i++)
		{

			endpt_details[i].endpointId.hubID = app_mgr_ctx->appContext[i].endPointDetail.endPointId.hubId;
			endpt_details[i].endpointId.endpointID = app_mgr_ctx->appContext[i].endPointDetail.endPointId.epId;
			endpt_details[i].type = google_contexthub_proto_EndpointType_GENERIC;
			endpt_details[i].name.arg = app_mgr_ctx->appContext[i].endPointDetail.name;
			endpt_details[i].name.funcs.encode = encode_string;
			endpt_details[i].version = (0x01 + i);
			endpt_details[i].tag.arg = NULL;
			endpt_details[i].tag.funcs.encode = NULL;
			endpt_details[i].requiredPermissions.arg = NULL;
			endpt_details[i].requiredPermissions.funcs.encode = NULL;
			endpt_details[i].services.format = google_contexthub_proto_RpcFormat_CUSTOM;
			endpt_details[i].services.majorVersion = app_mgr_ctx->appContext[i].endPointDetail.endPointService.majorVersion;
			endpt_details[i].services.minorVersion = app_mgr_ctx->appContext[i].endPointDetail.endPointService.minorVersion;
			endpt_details[i].services.serviceDescriptor.arg = app_mgr_ctx->appContext[i].endPointDetail.endPointService.serviceDescriptor;
			endpt_details[i].services.serviceDescriptor.funcs.encode = encode_string;

			if (!pb_encode_tag_for_field(stream, field))
			{
				success = false;
				break;
			}

			if(!pb_encode_submessage(stream, google_contexthub_proto_EndpointDetails_fields,&endpt_details[i]))
			{
				success = false;
				break;
			}
		}
	}
	else
	{
		printk("Not Enough Memory for Encoding !\n");
		return false;
	}

	bt_heap_free(BTSW_COMMON_HEAP,endpt_details);
	endpt_details = NULL;

	return success;

}




