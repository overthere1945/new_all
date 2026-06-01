/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      profile_mgr_rfcomm.c
===========================================================================*/
/**
 * @file profile_mgr_rfcomm.c
 * @brief Handles RFCOMM profile communication with Microstack.
 *
 * @details This file primarily handles:
 *          1. RFCOMM profile communication with Microstack.
 *             - Open RFCOMM socket
 *             - Close RFCOMM socket
 *             - Data transmission/reception
 */

/*============================================================================*
                                INCLUDE FILES
*============================================================================*/

#include "profile_mgr_rfcomm.h"
#include "offload_mgr_log.h"
#include "offload_mgr_client_interface.h"
#include "offload_instance.h"
#include "socket_mgr.h"

/*============================================================================*
                                MACRO DEFINITIONS
*============================================================================*/
#define PM_RFCOMM_APP_HANDLE 0x7813
/* value */
//#typedef google_offload_proto_RFCOMMChannelInfo rfcommChannelInfo_t;

/*============================================================================*
                                FUNCTION DEFINITIONS
*============================================================================*/


/*===========================================================================
FUNCTION    microstack_lecoc_cb
===========================================================================*/
/**
 * @brief Callback function for handling microstack events.
 *
 * This function is called when a microstack event occurs. It processes the event
 * based on the event class and message provided.
 *
 * @param handle The handle to the Bluetooth application.
 * @param eventClass The class of the event that occurred.
 * @param message Pointer to the message associated with the event.
 * @note refer microstack docs for more details about this callback
 */
static void microstack_rfcomm_cb(BtAppHandle handle, BtEventClass eventClass, void *message);

void profile_mgr_rfcomm_init(void) {
    microstack_register_app_callback(PM_RFCOMM_APP_HANDLE, microstack_rfcomm_cb);
}

void profile_mgr_rfcomm_deinit(void)
{
    microstack_deregister_app_callback(PM_RFCOMM_APP_HANDLE);
}

static void store_profile_data(google_offload_proto_socket_open *socket_open, offload_instance_t *offload)
{
	offload->offload_data.socket_data.profile_data.rfcomm_data.remoteMtu = socket_open->ctx.channel_info.rfcomm_channel_info.remotemtu;
	offload->offload_data.socket_data.profile_data.rfcomm_data.localMtu = socket_open->ctx.channel_info.rfcomm_channel_info.localmtu;
	offload->offload_data.socket_data.profile_data.rfcomm_data.localCid = socket_open->ctx.channel_info.rfcomm_channel_info.localCid;
	offload->offload_data.socket_data.profile_data.rfcomm_data.remoteCid = socket_open->ctx.channel_info.rfcomm_channel_info.remoteCid;
	offload->offload_data.socket_data.profile_data.rfcomm_data.maxFrameSize = socket_open->ctx.channel_info.rfcomm_channel_info.maxFrameSize;

}

void profile_mgr_rfcomm_socket_open(google_offload_proto_socket_open* socket_open, offload_instance_t *offload) {
    google_offload_proto_RFCOMMChannelInfo *channel_info = &(socket_open->ctx.channel_info.rfcomm_channel_info);
    store_profile_data(socket_open, offload);
    BmmSocketRfcommOffloadReqSend(socket_open->ctx.socket_id,
                                    PM_RFCOMM_APP_HANDLE,
                                    socket_open->ctx.aclConnectionHandle, 
                                    channel_info->localCid,
                                    channel_info->remoteCid,
                                    channel_info->localmtu,
                                    channel_info->remotemtu,
                                    channel_info->dlci,
                                    channel_info->initialRxCredits,
                                    channel_info->initialTxCredits,
                                    channel_info->maxFrameSize,
                                    channel_info->muxInitiatorFlag,
                                    (BmmContext)offload);
}

void profile_mgr_rfcomm_socket_close(BmmConnId connId) {
    BmmSocketCloseReqSend(connId, PM_RFCOMM_APP_HANDLE);
}

void profile_mgr_rfcomm_data_tx(offload_instance_t *offload, int data_length, void *data) {
    void *data_ptr = bt_pal_malloc(data_length);
    if (data_ptr == NULL)
    {
        BT_PAL_ASSERT_FATAL("profile_mgr_rfcomm_data_tx: malloc failed len:%d", data_length, 0, 0);
        return;
    }
    memcpy(data_ptr, data, data_length);
    BmmSocketDataReqSend(offload->offload_data.socket_data.pseudo_id,
                            PM_RFCOMM_APP_HANDLE,    
                            data_length,
                            data_ptr,
                            (BmmContext)offload);
}

void profile_mgr_rfcomm_data_rx_rsp(uint32_t pseudo_id) {
    BmmSocketDataRspSend(pseudo_id, (BmmContext)NULL);
}

static void microstack_rfcomm_cb(BtAppHandle handle, BtEventClass eventClass, void *message) {
    CsrPrim type = *((CsrPrim* )message);
    switch (type) {
        case BMM_SOCKET_DATA_CFM:
        {
            socket_mgr_send_data_tx_cfm_to_uapp(message);
        }
        break;

        case BMM_SOCKET_DATA_IND: 
        {
            socket_mgr_send_data_rx_ind_to_uapp(message);
            BmmSocketDataRspSend(((BmmSocketDataInd *)message)->connId, ((BmmSocketDataInd *)message)->context);
        }
        break;

        case BMM_SOCKET_RFCOMM_OFFLOAD_CFM:
        {
            /* nothing to do here */
            socket_mgr_microstack_offload_cfm(message);
        }
        break;
        case BMM_SOCKET_CLOSE_CFM:
        {
            socket_mgr_microstack_unoffload_cfm(message);
        }
        break;
        case BMM_SOCKET_SET_PARAMS_CFM: 
        {
            BmmSocketSetParamsCfm *cfm = (BmmSocketSetParamsCfm *)message;
            OFFLOAD_MGR_LOGM("BMM_SOCKET_SET_PARAMS_CFM : for conn id %d type %d resultCode : %d", cfm->connId, cfm->type, cfm->resultCode);
        }
        break;

    }
}

void profile_mgr_rfcomm_microapp_socket_open_cb(const offload_instance_t *offload) {
    /* set paramters for the opened socket */
    BmmSocketSetParamsReqSend(offload->offload_data.socket_data.pseudo_id, 
        PM_RFCOMM_APP_HANDLE,
        BMM_SOCK_PREFERRED_RX_CREDITS,
        SOCKET_MGR_RFCOMM_MAX_INITIAL_CREDITS, /* TBD */
        NULL);
}
