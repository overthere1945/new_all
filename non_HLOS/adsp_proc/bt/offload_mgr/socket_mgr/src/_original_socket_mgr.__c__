/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      socket_mgr.c
===========================================================================*/
/**
 * @file socket_mgr.c
 * @brief implementation of socket manager. 
 *
 * @details socket manager is responsible for managing sockets.
 */

#include "offload_mgr_sm.h"
#include "offload_mgr_client_interface.h"
#include "bmm_lib.h"
#include "bt_main.h"
#include "offload_mgr_transport_shim.h"
#include "endpt_mgr.h"
#include "endpt_mgr_rpc.h"

/*===========================================================================
FUNCTION      store_socket_data
===========================================================================*/
/**
 * @brief Store socket data.
 * @details This static function stores data for a socket using the
 *          provided Google offload protocol socket open structure.
 * @param[in] socket Pointer to the socket_t structure representing the socket.
 * @param[in] socket_open Pointer to the google_offload_proto_socket_open
 *            structure containing the socket open data.
 */
void socket_mgr_store_socket_data(void *d1, void *d2)
{
    offload_instance_t *offload = d1;
    google_offload_proto_socket_open *socket_open = d2;

    offload->offload_data.socket_data.socket_id = socket_open->ctx.socket_id;
    offload->ep_id.hub_id = socket_open->ctx.offloadEndpointId.hubID;
    offload->ep_id.id = socket_open->ctx.offloadEndpointId.endpointID;
    offload->offload_data.socket_data.socket_type = (socket_type_t)(socket_open->ctx.type);
}

/*===========================================================================
FUNCTION      socket_mgr_get_socket_capabilities
===========================================================================*/
/**
 * @brief Get socket capabilities.
 * @details This function retrieves the socket capabilities and encodes them 
 *          into a Google offload protocol socket capabilities structure.
 */
void socket_mgr_get_socket_capabilities(void)
{
    BmmResultCode rc;
    BmmSocketCapabilities cap;
    rc = BmmSocketGetCapabilities(&cap);

    if (rc != 0)
    {
        /* log or panic */
        OFFLOAD_MGR_FATAL("offload_mgr_sm_get_socket_capabilities : "
                          "failed to receive socket capabilites resultCode : %d",
                          rc);
    }

    /* Validate and set MAX value based on the platform limits */
    if (cap.lecoc.mtu > SOCKET_MGR_LECOC_MAX_LECOC_MTU)
    {
        cap.lecoc.mtu = SOCKET_MGR_LECOC_MAX_LECOC_MTU;
    }
    if (cap.rfcomm.maxFrameSize > SOCKET_MGR_RFCOMM_MAX_FRAME_SIZE)
    {
        cap.rfcomm.maxFrameSize = SOCKET_MGR_RFCOMM_MAX_FRAME_SIZE;
    }

    google_offload_proto_SocketCapabilities socket_caps = {
       	.rfcomm_capabilities = {
            cap.rfcomm.maxRfcommConn,
            cap.rfcomm.maxFrameSize,
        },
        .lecoc_capabilities = {
            cap.lecoc.maxLecocConn,
            cap.lecoc.mtu,
        },
	};

    encode_socket_capabilities(BT_OFFLOAD_SOCKET_CAPS, &socket_caps);
    /* send response */
    Offload_Mgr_SendDataToBTSocketHal(offload_mgr_transport_shim_buf,
                                      offload_mgr_transport_shim_encoded_buf_len);
}

/*===========================================================================
FUNCTION      socket_mgr_offload_req_to_profile
===========================================================================*/
/**
 * @brief Handle offload request to profile.
 * @details This function determines the type of socket open request and 
 *          calls the corresponding profile manager API.
 * @param[in] d1 Pointer to the offload_instance_t structure.
 * @param[in] d2 Pointer to the google_offload_proto_socket_open structure.
 */
void socket_mgr_offload_req_to_profile(void *d1, void *d2) {
    /* call profile api based on type */
    offload_instance_t *offload = d1;
    google_offload_proto_socket_open *socket_open = d2;
    switch(socket_open->ctx.type) {
        case SOCKET_TYPE_LECOC:
            profile_mgr_lecoc_socket_open(socket_open, offload);
            break;
        
        case SOCKET_TYPE_RFCOMM:
            profile_mgr_rfcomm_socket_open(socket_open, offload);
            break;
        
        default:
            OFFLOAD_MGR_ASSERT_FATAL(0);
            break;
    }
}

/*===========================================================================
FUNCTION      socket_mgr_unoffload_req_to_profile
===========================================================================*/
/**
 * @brief Handle unoffload request to profile.
 * @details This function determines the type of socket close request and 
 *          calls the corresponding profile manager API.
 * @param[in] d1 Pointer to the offload_instance_t structure.
 * @param[in] d2 Pointer to the socket_id_t structure.
 */
void socket_mgr_unoffload_req_to_profile(void *d1, void *d2) {
    /* call profile api based on type */
    offload_instance_t *offload = d1;
    socket_id_t *socket_id = d2;
    switch(offload->offload_data.socket_data.socket_type) {
        case SOCKET_TYPE_LECOC:
            profile_mgr_lecoc_socket_close(offload->offload_data.socket_data.pseudo_id);
            break;
        
        case SOCKET_TYPE_RFCOMM:
            profile_mgr_rfcomm_socket_close(offload->offload_data.socket_data.pseudo_id);
            break;
        
        default:
            OFFLOAD_MGR_ASSERT_FATAL(0);
            break;
    }
}

/*===========================================================================
FUNCTION      fill_socket_info
===========================================================================*/
/**
 * @brief Fill socket information.
 * @details This static function populates the socket_info_t structure with 
 *          data from the socket_data_t structure based on the socket type.
 * @param[out] socket_info Pointer to the socket_info_t structure to be filled.
 * @param[in] socket Pointer to the socket_data_t structure containing the socket data.
 */
static void fill_socket_info(socket_info_t *socket_info, socket_data_t *socket)
{
    switch (socket->socket_type)
    {
    case SOCKET_TYPE_LECOC:
    {
        socket_info->lecoc_socket_info.remoteMps = socket->profile_data.lecoc_data.remoteMps;
        socket_info->lecoc_socket_info.remotemtu = socket->profile_data.lecoc_data.remoteMtu;
    }
    break;
    case SOCKET_TYPE_RFCOMM:
    {
    	socket_info->rfcomm_socket_info.remotemtu = socket->profile_data.rfcomm_data.remoteMtu;
    	socket_info->rfcomm_socket_info.maxFrameSize = socket->profile_data.rfcomm_data.maxFrameSize;
    }
    break;
    default:
    {
        /* invalid socket type */
        OFFLOAD_MGR_ASSERT_FATAL(0);
    }
    }
}

/*===========================================================================
FUNCTION      socket_mgr_offload_req_to_microapp
===========================================================================*/
/**
 * @brief Handle offload request to microapp.
 * @details This function sends an open socket request to the microapp.
 * @param[in] data Pointer to the offload_instance_t structure.
 */
void socket_mgr_offload_req_to_microapp(void *data) {
    offload_instance_t *offload = data;

    if(offload == NULL) {
        OFFLOAD_MGR_ASSERT_FATAL(0);
    }
    endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
    header->opcode = UAPP_OPEN_SOCKET_REQ;
    header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(uapp_socket_open_req_t);
    header->endpoint_id = offload->ep_id.id;

    uapp_socket_open_req_t *req = (uapp_socket_open_req_t *)header->data;
    req->socket_id = offload->offload_data.socket_data.socket_id;
    req->socket_type = offload->offload_data.socket_data.socket_type;

    socket_info_t *socket_info = &(req->socket_info);
    fill_socket_info(socket_info, &(offload->offload_data.socket_data));

    endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
}

/*===========================================================================
FUNCTION      socket_mgr_unoffload_req_to_microapp
===========================================================================*/
/**
 * @brief Handle unoffload request to microapp.
 * @details This function sends a close socket request to the microapp.
 * @param[in] data Pointer to the offload_instance_t structure.
 */
void socket_mgr_unoffload_req_to_microapp(void *data) {
    offload_instance_t *offload = data;
    /* header */
    endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
    header->opcode = UAPP_SOCKET_CLOSE_REQ;
    header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(uapp_socket_close_req_t);
    header->endpoint_id = offload->ep_id.id;
    /* msg */
    uapp_socket_close_req_t *req = (uapp_socket_close_req_t *)header->data;
    req->socket_id = offload->offload_data.socket_data.socket_id;

    endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);

    /* Mock to send unoffload resp from uapp */
    offload_mgr_uapp_unoffload_rsp_cb(offload->offload_instance_id, 0); /* always sending success */
}

/*===========================================================================
FUNCTION      socket_mgr_send_offload_resp_to_hal
===========================================================================*/
/**
 * @brief Send offload response to HAL.
 * @details This function sends an open socket response to the HAL.
 * @param[in] data Pointer to the offload_instance_t structure.
 * @param[in] status Status of the offload operation.
 */
void socket_mgr_send_offload_resp_to_hal(void *data, int status) {
    offload_instance_t *offload = data;
    /* status is failure or success */
    google_offload_proto_socket_open_rsp socket_open_rsp = {
        .sock_id = offload->offload_data.socket_data.socket_id,
        .status = status,
    };
    char *reason = "unknown";
    encode_socket_open_rsp(BT_OFFLOAD_OPEN_SOCKET, &socket_open_rsp, (uint8_t *)reason);
    Offload_Mgr_SendDataToBTSocketHal(offload_mgr_transport_shim_buf,
                                      offload_mgr_transport_shim_encoded_buf_len);
}

/*===========================================================================
FUNCTION      socket_mgr_send_unoffload_resp_to_hal
===========================================================================*/
/**
 * @brief Send unoffload response to HAL.
 * @details This function sends a close socket response to the HAL.
 * @param[in] data Pointer to the offload_instance_t structure.
 * @param[in] status Status of the unoffload operation.
 */
void socket_mgr_send_unoffload_resp_to_hal(void *data, int status) {
    offload_instance_t *offload = data;
    /* status is failure or success */
    google_offload_proto_socket_close_rsp socket_close_rsp = {
        .sock_id = offload->offload_data.socket_data.socket_id,
    };
    char *reason = "unknown";
    encode_socket_close_rsp(BT_OFFLOAD_CLOSE_SOCKET, &socket_close_rsp, (uint8_t *)reason);
    Offload_Mgr_SendDataToBTSocketHal(offload_mgr_transport_shim_buf,
                                      offload_mgr_transport_shim_encoded_buf_len);
}

/*===========================================================================
FUNCTION      socket_mgr_offload_to_microapp_success_cb
===========================================================================*/
/**
 * @brief Offload to microapp success callback.
 * @details This function is called when the offload operation to the microapp is successful.
 * @param[in] data Pointer to the offload_instance_t structure.
 * @param[in] status Status of the offload operation.
 */
void socket_mgr_offload_to_microapp_success_cb(void *data, int status) {
    offload_instance_t *offload = data;
    if(offload == NULL) {
        OFFLOAD_MGR_LOGM("socket_mgr_offload_to_microapp_success_cb : offload is null");
        return;
    }
    switch(offload->offload_data.socket_data.socket_type) {
        case SOCKET_TYPE_LECOC:
        {
            profile_mgr_lecoc_microapp_socket_open_cb(offload);
        }
        break;
        case SOCKET_TYPE_RFCOMM:
        {
            profile_mgr_rfcomm_microapp_socket_open_cb(offload);
        }
        break;
        default:
        {
        }
        break;
    }
}

/*===========================================================================
FUNCTION      socket_mgr_unoffload_from_microapp_success_cb
===========================================================================*/
/**
 * @brief Unoffload from microapp success callback.
 * @details This function is called when the unoffload operation from the microapp is successful.
 * @param[in] data Pointer to the offload_instance_t structure.
 * @param[in] status Status of the unoffload operation.
 */
void socket_mgr_unoffload_from_microapp_success_cb(void *data, int status) {
    /* nothing to do here */
}

/*===========================================================================
FUNCTION      socket_mgr_clear_offload_data
===========================================================================*/
/**
 * @brief Clear offload data.
 * @details This function clears the offload data.
 * @param[in] data Pointer to the offload_instance_t structure.
 */
void socket_mgr_clear_offload_data(void *data) {

}

/*===========================================================================
FUNCTION      socket_mgr_init
===========================================================================*/
/**
 * @brief Initialize socket manager.
 * @details This function initializes the socket manager and registers the client functions.
 * 1. these registered functions are triggered from offload manager state machine.. 
 * 2. initialize profiles lecoc and rfcomm
 */
void socket_mgr_init(void) {
    offload_mgr_client_fn_t fns = {
        .clear_offload_data = socket_mgr_clear_offload_data,
        .store_offload_data = socket_mgr_store_socket_data,
        .offload_req_to_profile = socket_mgr_offload_req_to_profile,
        .unoffload_req_to_profile = socket_mgr_unoffload_req_to_profile,
        .offload_req_to_microapp = socket_mgr_offload_req_to_microapp,
        .unoffload_req_to_microapp = socket_mgr_unoffload_req_to_microapp,
        .send_offload_resp_to_hal = socket_mgr_send_offload_resp_to_hal,
        .send_unoffload_resp_to_hal = socket_mgr_send_unoffload_resp_to_hal,
        .offload_to_microapp_success_cb = socket_mgr_offload_to_microapp_success_cb,
        .unoffload_from_microapp_success_cb = socket_mgr_unoffload_from_microapp_success_cb,
    };
    offload_mgr_client_register(OFFLOAD_INSTANCE_TYPE_SOCKET, &fns);

    profile_mgr_lecoc_init();
    profile_mgr_rfcomm_init();
}

void socket_mgr_deinit(void) {
    profile_mgr_lecoc_deinit();
    profile_mgr_rfcomm_deinit();

    offload_mgr_client_deregister(OFFLOAD_INSTANCE_TYPE_SOCKET);
}

/*===========================================================================
FUNCTION      socket_mgr_microstack_offload_cfm
===========================================================================*/
/**
 * @brief Microstack offload confirmation.
 * @details This function is called when the microstack offload operation is confirmed.
 * @param[in] message Pointer to the confirmation message.
 */
void socket_mgr_microstack_offload_cfm(void *message) {
    BmmSocketOffloadStdCfm *cfm = (BmmSocketOffloadStdCfm *)message;
    offload_instance_t *offload = (offload_instance_t *)cfm->context;
    if (offload == NULL)
    {
        OFFLOAD_MGR_LOGM("socket_mgr_offload_cfm : invalid offload cfm for socket id : %d", cfm->socketId);
        return;
    }

    offload->offload_data.socket_data.pseudo_id = cfm->connId;
    offload_mgr_profile_offload_cfm_cb(offload->offload_instance_id, cfm->resultCode);
}

/*===========================================================================
FUNCTION      socket_mgr_microstack_unoffload_cfm
===========================================================================*/
/**
 * @brief Microstack unoffload confirmation.
 * @details This function is called when the microstack unoffload operation is confirmed.
 * @param[in] message Pointer to the confirmation message.
 */

void socket_mgr_microstack_unoffload_cfm(void *message) {
    BmmSocketCloseCfm *cfm = (BmmSocketCloseCfm *)message;
    const offload_instance_t *offload = offload_instance_find_by_pseudo_id(cfm->connId);
    if (offload == NULL)
    {
        OFFLOAD_MGR_LOGM("socket_mgr_unoffload_cfm : invalid unoffload cfm for socket id : %d", cfm->connId);
        return;
    }

    offload_mgr_profile_unoffload_cfm_cb(offload->offload_instance_id, cfm->resultCode);
}


/*===========================================================================
FUNCTION      socket_mgr_send_microstack_socket_close_ind_to_hal
===========================================================================*/
/**
 * @brief Send microstack socket close indication to HAL.
 * @details This function sends a socket close indication to the HAL.
 * @param[in] socket_id Socket ID.
 * @param[in] status Status of the socket close operation.
 */
void socket_mgr_send_microstack_socket_close_ind_to_hal(socket_id_t socket_id, int status) {
        google_offload_proto_socket_close_ind ind = {
            .sock_id = socket_id,
            .reason = status,
        };
        encode_socket_close_ind(BT_OFFLOAD_SOCKET_CLOSE_IND, &ind);
        Offload_Mgr_SendDataToBTSocketHal(offload_mgr_transport_shim_buf,
            offload_mgr_transport_shim_encoded_buf_len);
}

/*===========================================================================
FUNCTION      socket_mgr_data_tx
===========================================================================*/
/**
 * @brief Send data to the microapp.
 * @details This function sends data to the microapp for the specified socket ID.
 * @param[in] socket_id Socket ID.
 * @param[in] data Pointer to the data to be sent.
 * @param[in] data_length Length of the data to be sent.
 */
void socket_mgr_data_tx(uint64_t socket_id, void *data, int data_length)
{
    const offload_instance_t *offload = offload_instance_find_by_socket_id(socket_id);
    if (offload == NULL)
    {
        OFFLOAD_MGR_LOGL("socket_mgr_data_tx : invalid socket");
        return;
    }

    switch (offload->offload_data.socket_data.socket_type)
    {
    case SOCKET_TYPE_LECOC:
    {
        OFFLOAD_MGR_LOGL("Data tx on connid : %d", offload->offload_data.socket_data.pseudo_id);
        // BT_OFFLOAD_MGR_LOG_BYTES(data, data_length);
        profile_mgr_lecoc_data_tx((offload_instance_t *)offload, data_length, data);
    }
    break;
    case SOCKET_TYPE_RFCOMM:
    {
    	OFFLOAD_MGR_LOGL("Data tx on connid : %d", offload->offload_data.socket_data.pseudo_id);
        // BT_OFFLOAD_MGR_LOG_BYTES(data, data_length);
    	profile_mgr_rfcomm_data_tx((offload_instance_t *)offload, data_length, data);
    }
    break;
    
    }
}

/*===========================================================================
FUNCTION      socket_mgr_data_rx_rsp
===========================================================================*/
/**
 * @brief Send data receive response to the microapp.
 * @details This function sends a data receive response to the microapp for the specified socket ID.
 * @param[in] socket_id Socket ID.
 */
void socket_mgr_data_rx_rsp(uint64_t socket_id)
{
    const offload_instance_t *offload = offload_instance_find_by_socket_id(socket_id);
    if (offload == NULL)
    {
        OFFLOAD_MGR_LOGL("socket_mgr_data_rx_rsp : invalid data rx ind for socket id :");
		SOCKET_MGR_LOG_SOCKET_ID(socket_id);
        return;
    }

    switch (offload->offload_data.socket_data.socket_type)
    {
    case SOCKET_TYPE_LECOC:
    {
        profile_mgr_lecoc_data_rx_rsp(offload->offload_data.socket_data.pseudo_id);
    }
	break;
    case SOCKET_TYPE_RFCOMM:
    {
    	profile_mgr_rfcomm_data_rx_rsp(offload->offload_data.socket_data.pseudo_id);
    }
    break;
    }
}

void socket_mgr_send_data_tx_cfm_to_uapp(BmmSocketDataCfm *cfm)
{
    const offload_instance_t *offload = (offload_instance_t *)(cfm->context);
    if (offload == NULL)
    {
        /* invalid confirm received, log and return */
        OFFLOAD_MGR_LOGL("socket_mgr_send_data_tx_cfm : Invalid socket ");
        return;
    }
    /* header */
    endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
    header->opcode = UAPP_DATA_TX_CFM;
    header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(uapp_data_tx_cfm_t);
    header->endpoint_id = offload->ep_id.id;

    /* message */
    uapp_data_tx_cfm_t *tx_cfm = (uapp_data_tx_cfm_t *)header->data;
    tx_cfm->socket_id = offload->offload_data.socket_data.socket_id;
    tx_cfm->status = cfm->resultCode;
    /* send message to endpoint */
    endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
}

void socket_mgr_send_data_rx_ind_to_uapp(BmmSocketDataInd *ind)
{
    /* log the data received */
    OFFLOAD_MGR_LOGL("microstack_lecoc_cb : Data rx on connid : %d", ind->connId);
    // BT_OFFLOAD_MGR_LOG_BYTES(ind->data, ind->dataLen);

    /* find the socket with the pseudo id */
    const offload_instance_t *offload = offload_instance_find_by_pseudo_id(ind->connId);
    if (offload == NULL)
    {
        /* rx indication received on invalid socket, log and return */
        OFFLOAD_MGR_LOGE("Data rx indication on invalid conn id : %d", ind->connId);
        return;
    }
    /** send the rx indication to app manager */
    /* header */
    endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
    header->opcode = UAPP_DATA_RX_IND;
    header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(uapp_data_rx_ind_t) + ind->dataLen;
    header->endpoint_id = offload->ep_id.id;

    /* msg */
    uapp_data_rx_ind_t *rx_ind = (uapp_data_rx_ind_t *)header->data;

    rx_ind->socket_id = offload->offload_data.socket_data.socket_id;
    rx_ind->data_len = ind->dataLen;
    memcpy(rx_ind->data, ind->data, ind->dataLen);

    endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
}

/*===========================================================================
FUNCTION      socket_mgr_handle_open_socket
===========================================================================*/
/**
 * @brief Open a socket based on the event data.
 * @details This function processes an event to open a socket,
 *          using the provided event data and length.
 * @param[in] evt Pointer to the event data.
 * @param[in] len Length of the event data.
 */
void socket_mgr_handle_open_socket(uint8_t *evt, size_t len)
{
    SM_StateContext *sm_state_ctx = NULL;
    offload_instance_t *offload = NULL;
    google_offload_proto_socket_open socket_open = google_offload_proto_socket_open_init_default;
    bool status = false;

    OFFLOAD_MGR_LOGL("offload_mgr_sm_open_socket:\n");

    status = decode_socket_open(evt, len, &socket_open);
    if (!status)
    {
        /* send failure */
        /* can't send failure, don't have socket id of the failed socket */
        return;
    }

    /* TBD : check if the socket with socketid exists */

    /* get the Idle socket */
    offload = offload_mgr_sm_find_idle_offload_instance();
    if (offload == NULL)
    {
        OFFLOAD_MGR_LOGE("No more sockets");
        /* send failure */
		google_offload_proto_socket_open_rsp socket_open_rsp = {
			.sock_id = socket_open.ctx.socket_id,
			.status = status,
		};
		char *reason = "unknown";
		encode_socket_open_rsp(BT_OFFLOAD_OPEN_SOCKET, &socket_open_rsp, (uint8_t *)reason);
		Offload_Mgr_SendDataToBTSocketHal(offload_mgr_transport_shim_buf,
										  offload_mgr_transport_shim_encoded_buf_len);
        return;
    }

    /* most imp : fill the type */
    /* ids are already filled so, no need to fill again */
    offload->offload_instance_type = OFFLOAD_INSTANCE_TYPE_SOCKET;
    /* fill socket data */
    // socket_mgr_store_socket_data(offload, &socket_open);    /* todo map this to client fns */

    /* proto to open socket */
    offload->state_ctx->data = &socket_open;
    offload_mgr_sm_handle_offload_evt(offload);
}


/*===========================================================================
FUNCTION      socket_mgr_handle_close_socket
===========================================================================*/
/**
 * @brief Close a socket based on the event data.
 * @details This static function processes an event to close a socket,
 *          using the provided event data and length.
 * @param[in] evt Pointer to the event data.
 * @param[in] len Length of the event data.
 */
void socket_mgr_handle_close_socket(uint8_t *evt, size_t len)
{
    bool status = false;
    socket_id_t socket_id;
    SM_StateContext *sm_state_ctx = NULL;
    const offload_instance_t *offload = NULL;

    status = decode_socket_close(evt, len, &socket_id);
    if (!status)
    {
        /* TBD: send a close ind back */
        OFFLOAD_MGR_LOGE("offload_mgr_close_socket : unable to decode proto msg");
        return;
    }
    /* get the socket */
    offload = offload_instance_find_by_socket_id(socket_id);
    if (offload == NULL)
    {
        /* Do nothing */
        OFFLOAD_MGR_LOGE("offload_mgr_close_socket : No such socket id");
        return;
    }

    offload->state_ctx->data = &socket_id;
    offload_mgr_sm_handle_unoffload_evt((offload_instance_t *)offload);
}

