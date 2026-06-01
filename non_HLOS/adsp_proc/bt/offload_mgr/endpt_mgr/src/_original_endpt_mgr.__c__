/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      endpt_mgr_rpc.c
===========================================================================*/
/**
 * @file endpt_mgr_rpc.c
 * @brief Handles the encoding and decoding of message formats used to send and receive data to AWM over glink.
 *
 * @details Assumption: All the messages reaching endpt_mgr over glink are coming from
 *          M55 (AWM) and context hub HAL.
 */

/*============================================================================*
                           INCLUDE FILES
*============================================================================*/
#include "endpt_mgr_internal.h"
#include "offload_mgr_proto_utils.h"
#include "profile_mgr_le_adv.h"
#include "profile_mgr_lecoc.h"
#include "profile_mgr_rfcomm.h"
#include "offload_mgr_sm.h"
#include "offload_instance.h"
#include "profile_mgr_le_scan.h"
#include "bt_utils.h"

uint64_t endpt_mgr_hub_id_awm = ENDPT_MGR_HUB_ID_AWM;

void endpt_mgr_init(void)
{
}

void endpt_mgr_deinit(void)
{
}

void endpt_mgr_handle_offload_app_evt(endpt_mgr_rpc_header_t *rpc_msg)
{
    switch (rpc_msg->opcode)
    {
    case UAPP_OPEN_SOCKET_RES:
    {
        uapp_socket_open_rsp_t *rsp;
        rsp = (uapp_socket_open_rsp_t *)rpc_msg->data;

        OFFLOAD_MGR_LOGM("endpt_mgr_handle_offload_app_evt : received UAPP_OPEN_SOCKET_RES");
        SOCKET_MGR_LOG_SOCKET_ID(rsp->socket_id);

        offload_instance_id_t id = offload_instance_find_id_by_socket_id(rsp->socket_id);
        offload_mgr_uapp_offload_rsp_cb(id, rsp->status);
    }
    break;
    case UAPP_DATA_TX_REQ:
    {
        uapp_data_tx_req_t *req;
        req = (uapp_data_tx_req_t *)rpc_msg->data;

        OFFLOAD_MGR_LOGL("endpt_mgr_handle_offload_app_evt : received UAPP_DATA_TX_REQ");
        SOCKET_MGR_LOG_SOCKET_ID(req->socket_id);

        socket_mgr_data_tx(req->socket_id, (uint8_t *)req->data, req->data_len);
    }
    break;
    case UAPP_DATA_RX_RES:
    {
        // uapp_data_rx_rsp_t *data_rx_rsp;
        // data_rx_rsp = (uapp_data_rx_rsp_t *)rpc_msg->data;
 
        // OFFLOAD_MGR_LOGL("endpt_mgr_handle_offload_app_evt : received UAPP_DATA_RX_RES");
        // SOCKET_MGR_LOG_SOCKET_ID(data_rx_rsp->socket_id);
 
        // socket_mgr_data_rx_rsp(data_rx_rsp->socket_id);
    }
    break;
    case UAPP_ENDPT_DISC_RES:
    {
        /* fill the endpt discovery response and send on context hub channel */
        OFFLOAD_MGR_LOGM("endpt_mgr_handle_offload_app_evt : endpt discovery responses received : \nSending the endpt discovery response to context hub");
        memcpy(offload_mgr_transport_shim_buf + OFFLOAD_MGR_TRANSPORT_SHIM_HDR_LEN, rpc_msg->data, rpc_msg->data_len);
        Offload_Mgr_Update_Header(offload_mgr_transport_shim_buf, UAPP_ENDPT_DISC_RES, rpc_msg->data_len);
        Offload_Mgr_SendDataToContextHubHal((void *)offload_mgr_transport_shim_buf, offload_mgr_transport_shim_encoded_buf_len);
    }
    break;
    case UAPP_SOCKET_CLOSE_IND: 
    {
        uapp_socket_close_ind_t *close_ind;
        close_ind = (uapp_socket_close_ind_t *)rpc_msg->data;

        OFFLOAD_MGR_LOGM("endpt_mgr_handle_offload_app_evt : received UAPP_SOCKET_CLOSE_IND");
        SOCKET_MGR_LOG_SOCKET_ID(close_ind->socket_id);

        /* send the close ind to hal */
        socket_mgr_send_microstack_socket_close_ind_to_hal(close_ind->socket_id, close_ind->reason);
    }
    break;
    case UAPP_SOCKET_CLOSE_RES:
    {
        uapp_socket_close_rsp_t *rsp;
        rsp = (uapp_socket_close_rsp_t *)rpc_msg->data;
        OFFLOAD_MGR_LOGM("endpt_mgr_handle_offload_app_evt : received UAPP_SOCKET_CLOSE_RES");
        SOCKET_MGR_LOG_SOCKET_ID(rsp->socket_id);

        offload_instance_id_t id = offload_instance_find_id_by_socket_id(rsp->socket_id);
        offload_mgr_uapp_unoffload_rsp_cb(id, 0); /* always sending success */
    }
    break;
    case UAPP_GATT_APP_REG_REQ:
    {
        profile_mgr_gatt_uapp_register(rpc_msg->endpoint_id);
    }
    break;
    case UAPP_GATT_APP_UNREG_REQ:
    {
        profile_mgr_gatt_uapp_unregister(rpc_msg->endpoint_id);
    }
    break;
    case UAPP_GATT_SESSION_REGISTERED_RESP:
    {
        uapp_gatt_session_registered_rsp_t *rsp = (uapp_gatt_session_registered_rsp_t *)rpc_msg->data;
        OFFLOAD_MGR_LOGL("UAPP_GATT_SESSION_REGISTERED_RESP sessionId %d status %d", rsp->session_id, rsp->status);

        offload_instance_id_t id = offload_instance_find_id_by_gatt_session_id(rsp->session_id);
        if (id < MAX_OFFLOAD_INSTANCES)
        {
            offload_mgr_uapp_offload_rsp_cb(id, rsp->status);
        } 
    }
    break;
    case UAPP_GATT_SESSION_UNREGISTERED_IND:
    case UAPP_GATT_SESSION_UNREGISTERED_RESP:
    {
        uapp_gatt_session_unregister_ind_t *resp = (uapp_gatt_session_unregister_ind_t *)rpc_msg->data;
        
        OFFLOAD_MGR_LOGL("UAPP_GATT_SESSION_UNREGISTERED_IND sessionId %d", resp->session_id);
        offload_instance_id_t id = offload_instance_find_id_by_gatt_session_id(resp->session_id);
        if (id < MAX_OFFLOAD_INSTANCES)
        {
            offload_mgr_uapp_unoffload_rsp_cb(id, 0);
        } 
    }
    break;

    case UAPP_GATT_CLIENT_READ_CHAR_REQ:
    case UAPP_GATT_CLIENT_WRITE_CHAR_REQ:
    case UAPP_GATT_CLIENT_WRITE_CHAR_CMD:
    case UAPP_GATT_SERVER_READ_CHAR_IND_RESP:
    case UAPP_GATT_SERVER_WRITE_CHAR_IND_RESP:
    case UAPP_GATT_SERVER_NOTIFICATION_REQ:
    {
        profile_mgr_gatt_uapp_char_evt(rpc_msg->endpoint_id, rpc_msg->opcode, rpc_msg->data, rpc_msg->data_len);
    }
    break;
    default:
    {
        OFFLOAD_MGR_FATAL("Invalid opcode received from offload app : %d", rpc_msg->opcode);
    }
    }
}

void endpt_mgr_handle_non_offload_app_evt(void *rpc_msg)
{
    endpt_mgr_rpc_header_t *msg = (endpt_mgr_rpc_header_t *)rpc_msg;
    OFFLOAD_MGR_LOGL("Handle Non OffLoad App Event : Msg Opcode : %d\n",msg->opcode);
    if(msg->opcode >= LE_ADV_START_APP && msg->opcode <= LE_ADV_CMD_MAX)
    {
       le_adv_cmd_handler(msg->opcode,msg->data_len, (void *)((endpt_mgr_rpc_header_t *)rpc_msg)->data);
    }
    if(msg->opcode >= LE_SCAN_START_APP && msg->opcode <= LE_SCAN_CMD_MAX)
    {
        le_scan_cmd_handler(msg->opcode,msg->data_len, (void *)((endpt_mgr_rpc_header_t *)rpc_msg)->data);
    }
    if(msg->opcode >= BT_UTILS_CMD_START && msg->opcode <= BT_UTILS_CMD_MAX)
    {
        bt_utils_cmd_handler(msg->opcode,msg->data_len, (void *)((endpt_mgr_rpc_header_t *)rpc_msg)->data); 
    }
}

void endpt_mgr_handle_awm_evt(uint8_t *evt, size_t len)
{
    endpt_mgr_rpc_header_t *rpc_msg = (endpt_mgr_rpc_header_t *)evt;
    OFFLOAD_MGR_LOGM("endpt_mgr_handle_evt : received length : %d\n", len);
    // ENDPT_MGR_LOG_RPC_MSG(rpc_msg);

    if (rpc_msg->command_type == OFFLOAD_APP_CMD)
    {
        endpt_mgr_handle_offload_app_evt(rpc_msg);
    }
    else
    {
        endpt_mgr_handle_non_offload_app_evt(rpc_msg);
    }
}

void endpt_mgr_handle_context_hub_evt(uint16_t opcode, uint8_t *evt, size_t len)
{
    /* header */
    if (opcode == UAPP_ENDPT_DISC_REQ)
    {
        endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
        header->opcode = UAPP_ENDPT_DISC_REQ;
        header->data_len = ENDPT_MGR_RPC_HDR_SIZE;
        /* no message body */

        endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
    }
}

void endpt_mgr_send_msg_to_endpoint(uint64_t *hub_id, void *data, int data_len)
{
    if (*hub_id == ENDPT_MGR_HUB_ID_AWM)
    {
        OFFLOAD_MGR_LOGL("hub id matched to awm : Sending msg ... ");
        Offload_Mgr_SendDataToAppMgr(data, data_len);
    }
    else
    {
        /* ignore message */
        OFFLOAD_MGR_LOGE("Hub id not matching %d, Ignoring the message", ENDPT_MGR_HUB_ID_AWM);
    }
}
