/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      socket_mgr_proto.c
===========================================================================*/
/**
 * @file socket_mgr_proto.c
 * @brief Proto encoders and decoders for socket offload.
 *
 * @details This file contains the implementation of proto encoders 
 *          and decoders used for socket offload.
 */

/*===========================================================================
							INCLUDE FILES
===========================================================================*/
#include <string.h>
#include <stdio.h>
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "socket_mgr.pb.h"
#include "offload_mgr_transport_shim.h"
#include "offload_mgr_log.h"
#include "socket.h"
#include "offload_mgr_proto_utils.h"

/*===========================================================================
							FUNCTION DEFINITIONS
===========================================================================*/

bool encode_socket_open_rsp(uint16_t opcode, google_offload_proto_socket_open_rsp *socket_open_rsp, uint8_t *reason)
{
	bool encode = false;
	pb_ostream_t stream = pb_ostream_from_buffer(offload_mgr_transport_shim_buf + OFFLOAD_MGR_TRANSPORT_SHIM_HDR_LEN, OFFLOAD_MGR_TRANSPORT_SHIM_BUF_LEN);
	socket_open_rsp->reason.arg = reason;
	socket_open_rsp->reason.funcs.encode = encode__bytes;
	encode = pb_encode(&stream, google_offload_proto_socket_open_rsp_fields , socket_open_rsp);

	/* Frame the header */
	offload_mgr_transport_shim_encoded_buf_len = stream.bytes_written;
	Offload_Mgr_Update_Header(offload_mgr_transport_shim_buf, opcode, stream.bytes_written);

	OFFLOAD_MGR_LOGL("encode_socket_open_rsp: encode : %d", encode);

	return encode;
}

bool encode_socket_close_rsp(uint16_t opcode, google_offload_proto_socket_close_rsp *socket_close_rsp, uint8_t *reason)
{
	bool encode = false;
	pb_ostream_t stream = pb_ostream_from_buffer(offload_mgr_transport_shim_buf + OFFLOAD_MGR_TRANSPORT_SHIM_HDR_LEN, OFFLOAD_MGR_TRANSPORT_SHIM_BUF_LEN);

	socket_close_rsp->reason.arg = reason;
	socket_close_rsp->reason.funcs.encode = encode__bytes;

	encode = pb_encode(&stream, google_offload_proto_socket_close_rsp_fields , socket_close_rsp);
	offload_mgr_transport_shim_encoded_buf_len = stream.bytes_written;

	/* Frame the header */
	Offload_Mgr_Update_Header(offload_mgr_transport_shim_buf, opcode, stream.bytes_written);

	OFFLOAD_MGR_LOGL("encode_socket_close_rsp: encode : %d", encode);

	return encode;	
}


bool encode_socket_close_ind(uint16_t opcode, google_offload_proto_socket_close_ind *socket_close_ind)
{
	bool encode = false;
	pb_ostream_t stream = pb_ostream_from_buffer(offload_mgr_transport_shim_buf + OFFLOAD_MGR_TRANSPORT_SHIM_HDR_LEN, OFFLOAD_MGR_TRANSPORT_SHIM_BUF_LEN);

	encode = pb_encode(&stream, google_offload_proto_socket_close_ind_fields, socket_close_ind);

	/* Frame the header */
	Offload_Mgr_Update_Header(offload_mgr_transport_shim_buf, opcode, stream.bytes_written);

	OFFLOAD_MGR_LOGL("encode_socket_close_ind: encode : %d", encode);
	
	return encode;	
}

static void log_socket_data(google_offload_proto_socket_open *socket_open) {

	OFFLOAD_MGR_LOGL("Socket Id : ");
	SOCKET_MGR_LOG_SOCKET_ID(socket_open->ctx.socket_id);
	OFFLOAD_MGR_LOGL("Protocol Type: %d\n", socket_open->ctx.type);

		switch(socket_open->ctx.type) {
				case google_offload_proto_Protocol_LE_COC:
				{
						OFFLOAD_MGR_LOGL("aclConnectionHandle: %d localCid: %d remoteCid: %d psm: %d localMtu: %d remoteMtu: %d", 
										  socket_open->ctx.aclConnectionHandle, socket_open->ctx.channel_info.lecoc_channel_info.localCid, 
						                  socket_open->ctx.channel_info.lecoc_channel_info.remoteCid, socket_open->ctx.channel_info.lecoc_channel_info.psm, 
										  socket_open->ctx.channel_info.lecoc_channel_info.localMtu, socket_open->ctx.channel_info.lecoc_channel_info.remoteMtu);
						OFFLOAD_MGR_LOGL("localMps: %d remoteMps: %d initialRxCredits: %d initialTxCredits: %d", 
										  socket_open->ctx.channel_info.lecoc_channel_info.localMps, socket_open->ctx.channel_info.lecoc_channel_info.remoteMps, 
										  socket_open->ctx.channel_info.lecoc_channel_info.initialRxCredits, socket_open->ctx.channel_info.lecoc_channel_info.initialTxCredits);
				}
				break;
				case google_offload_proto_Protocol_RFCOMM:
				{
						OFFLOAD_MGR_LOGL("aclConnectionHandle: %d endpointID: %d hubID: %d localCid: %d remoteCid: %d localmtu: %d",
										 socket_open->ctx.aclConnectionHandle, socket_open->ctx.offloadEndpointId.endpointID, socket_open->ctx.offloadEndpointId.hubID,
										 socket_open->ctx.channel_info.rfcomm_channel_info.localCid, socket_open->ctx.channel_info.rfcomm_channel_info.remoteCid, 
										 socket_open->ctx.channel_info.rfcomm_channel_info.localmtu);
						OFFLOAD_MGR_LOGL("remotemtu: %d initialRxCredits: %d initialTxCredits: %d dlci: %d maxFrameSize: %d muxInitiatorFlag: %d",
										 socket_open->ctx.channel_info.rfcomm_channel_info.remotemtu, 
										 socket_open->ctx.channel_info.rfcomm_channel_info.initialRxCredits, socket_open->ctx.channel_info.rfcomm_channel_info.initialTxCredits,
										 socket_open->ctx.channel_info.rfcomm_channel_info.dlci, socket_open->ctx.channel_info.rfcomm_channel_info.maxFrameSize,
										 socket_open->ctx.channel_info.rfcomm_channel_info.muxInitiatorFlag);
				}
				break;
		}
}

bool decode_socket_open(uint8_t *evt, size_t len, google_offload_proto_socket_open *socket_open)
{
	bool decode = false;
	pb_istream_t stream = pb_istream_from_buffer(evt, len);
	uint8_t socket_name[50];
	
	socket_open->ctx.socket_name.arg = socket_name;
	socket_open->ctx.socket_name.funcs.decode = decode__bytes;

	decode = pb_decode(&stream, google_offload_proto_socket_open_fields , socket_open);
	OFFLOAD_MGR_LOGL("Decode for open: %d\n", decode);
	if (!decode)
	{
		OFFLOAD_MGR_LOGL("%s", PB_GET_ERROR(&stream));
	}
	
	log_socket_data(socket_open);
	return decode;
}

bool decode_socket_close(uint8_t *evt, size_t len, socket_id_t *socketId)
{
	bool decode = false;
	google_offload_proto_socket_close socket_close = google_offload_proto_socket_close_init_default;
	pb_istream_t stream = pb_istream_from_buffer(evt, len);
	decode = pb_decode(&stream, google_offload_proto_socket_close_fields, &socket_close);
	OFFLOAD_MGR_LOGL("Decode for close: %d\n", decode);

	if (decode)
	{
		OFFLOAD_MGR_LOGL("decode_socket_close socket id : ");
		SOCKET_MGR_LOG_SOCKET_ID(socket_close.sock_id);
		*socketId = socket_close.sock_id;
	}
	return decode;
}

bool encode_socket_capabilities(uint16_t opcode, google_offload_proto_SocketCapabilities *socket_caps) {
	bool encode = false;
	pb_ostream_t stream = pb_ostream_from_buffer(offload_mgr_transport_shim_buf + OFFLOAD_MGR_TRANSPORT_SHIM_HDR_LEN, OFFLOAD_MGR_TRANSPORT_SHIM_BUF_LEN);

	encode = pb_encode(&stream, google_offload_proto_SocketCapabilities_fields, socket_caps);

	/* Frame the header */
	Offload_Mgr_Update_Header(offload_mgr_transport_shim_buf, opcode, stream.bytes_written);
	OFFLOAD_MGR_LOGL("encode_socket_capabilities : encode : %d", encode);
	return encode;	
}
