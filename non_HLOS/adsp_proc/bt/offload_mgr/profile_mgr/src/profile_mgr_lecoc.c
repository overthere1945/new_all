/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      profile_mgr_lecoc.c
===========================================================================*/
/**
 * @file profile_mgr_lecoc.c
 * @brief Handles LECOC profile communication with Microstack.
 *
 * @details This file primarily handles:
 *          1. LECOC profile communication with Microstack.
 *             - Open LECOC socket
 *             - Close LECOC socket
 *             - Data transmission/reception
 *
 * add hyungchul :
 * - ADSP LECoC path debug log 추가.
 * - Qualcomm LECoC Micro App API 문서의 DATA_IND / DATA_IND_RESP 흐름에 맞게
 *   BMM_SOCKET_DATA_IND 수신 시 즉시 BmmSocketDataRspSend()를 호출하지 않고,
 *   AWM에서 UAPP_DATA_RX_RES가 올 때 endpt_mgr -> socket_mgr_data_rx_rsp() 경로로 응답하도록 변경.
 */

/*============================================================================*
                                INCLUDE FILES
*============================================================================*/
#include "profile_mgr_lecoc.h"
#include "offload_mgr_log.h"
#include "offload_mgr_client_interface.h"
#include "offload_instance.h"
#include "socket_mgr.h"

/*============================================================================*
                                MACRO DEFINITIONS
*============================================================================*/
#define PM_LECOC_APP_HANDLE 0x7812

// add hyungchul start
#define PM_LECOC_MAX_PENDING_RX_CONTEXTS 4

typedef struct pm_lecoc_rx_context_entry
{
    bool valid;
    BmmConnId connId;
    BmmContext context;
} pm_lecoc_rx_context_entry_t;

static pm_lecoc_rx_context_entry_t g_pm_lecoc_rx_context_table[PM_LECOC_MAX_PENDING_RX_CONTEXTS];

static void pm_lecoc_store_rx_context(BmmConnId connId, BmmContext context)
{
    for (uint8_t idx = 0; idx < PM_LECOC_MAX_PENDING_RX_CONTEXTS; idx++)
    {
        if ((g_pm_lecoc_rx_context_table[idx].valid == true) &&
            (g_pm_lecoc_rx_context_table[idx].connId == connId))
        {
            g_pm_lecoc_rx_context_table[idx].context = context;
            OFFLOAD_MGR_LOGM("change(add)-hyungchul-20260512-0001: update LECoC RX context connId=%d slot=%d", connId, idx);
            return;
        }
    }

    for (uint8_t idx = 0; idx < PM_LECOC_MAX_PENDING_RX_CONTEXTS; idx++)
    {
        if (g_pm_lecoc_rx_context_table[idx].valid == false)
        {
            g_pm_lecoc_rx_context_table[idx].valid = true;
            g_pm_lecoc_rx_context_table[idx].connId = connId;
            g_pm_lecoc_rx_context_table[idx].context = context;
            OFFLOAD_MGR_LOGM("change(add)-hyungchul-20260512-0001: store LECoC RX context connId=%d slot=%d", connId, idx);
            return;
        }
    }

    g_pm_lecoc_rx_context_table[0].valid = true;
    g_pm_lecoc_rx_context_table[0].connId = connId;
    g_pm_lecoc_rx_context_table[0].context = context;
    OFFLOAD_MGR_LOGE("change(add)-hyungchul-20260512-0001: RX context table full, overwrite slot0 connId=%d", connId);
}

static BmmContext pm_lecoc_take_rx_context(BmmConnId connId)
{
    for (uint8_t idx = 0; idx < PM_LECOC_MAX_PENDING_RX_CONTEXTS; idx++)
    {
        if ((g_pm_lecoc_rx_context_table[idx].valid == true) &&
            (g_pm_lecoc_rx_context_table[idx].connId == connId))
        {
            BmmContext context = g_pm_lecoc_rx_context_table[idx].context;
            g_pm_lecoc_rx_context_table[idx].valid = false;
            g_pm_lecoc_rx_context_table[idx].connId = 0;
            g_pm_lecoc_rx_context_table[idx].context = (BmmContext)NULL;
            OFFLOAD_MGR_LOGM("change(add)-hyungchul-20260512-0001: take LECoC RX context connId=%d slot=%d", connId, idx);
            return context;
        }
    }

    OFFLOAD_MGR_LOGE("change(add)-hyungchul-20260512-0001: RX context not found for connId=%d, use NULL", connId);
    return (BmmContext)NULL;
}
// end hyungchul

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
static void microstack_lecoc_cb(BtAppHandle handle, BtEventClass eventClass, void *message);

void profile_mgr_lecoc_init(void)
{
    OFFLOAD_MGR_LOGM("add hyungchul : profile_mgr_lecoc_init register handle=0x%x", PM_LECOC_APP_HANDLE);
    microstack_register_app_callback(PM_LECOC_APP_HANDLE, microstack_lecoc_cb);
}

void profile_mgr_lecoc_deinit(void)
{
    OFFLOAD_MGR_LOGM("add hyungchul : profile_mgr_lecoc_deinit deregister handle=0x%x", PM_LECOC_APP_HANDLE);
    microstack_deregister_app_callback(PM_LECOC_APP_HANDLE);

	// add hyungchul
    for (uint8_t idx = 0; idx < PM_LECOC_MAX_PENDING_RX_CONTEXTS; idx++)
    {
        g_pm_lecoc_rx_context_table[idx].valid = false;
        g_pm_lecoc_rx_context_table[idx].connId = 0;
        g_pm_lecoc_rx_context_table[idx].context = (BmmContext)NULL;
    }
}

static void store_profile_data(google_offload_proto_socket_open *socket_open, offload_instance_t *offload)
{
    offload->offload_data.socket_data.profile_data.lecoc_data.remoteMtu = socket_open->ctx.channel_info.lecoc_channel_info.remoteMtu;
    offload->offload_data.socket_data.profile_data.lecoc_data.remoteMps = socket_open->ctx.channel_info.lecoc_channel_info.remoteMps;
    offload->offload_data.socket_data.profile_data.lecoc_data.localMtu = socket_open->ctx.channel_info.lecoc_channel_info.remoteMps;
    offload->offload_data.socket_data.profile_data.lecoc_data.localMtu = socket_open->ctx.channel_info.lecoc_channel_info.remoteMps;
    offload->offload_data.socket_data.profile_data.lecoc_data.psm = socket_open->ctx.channel_info.lecoc_channel_info.psm;

    OFFLOAD_MGR_LOGM("add hyungchul : store_profile_data LECoC remoteMtu=%d remoteMps=%d psm=%d",
                     socket_open->ctx.channel_info.lecoc_channel_info.remoteMtu,
                     socket_open->ctx.channel_info.lecoc_channel_info.remoteMps,
                     socket_open->ctx.channel_info.lecoc_channel_info.psm);
}

void profile_mgr_lecoc_socket_open(google_offload_proto_socket_open *socket_open, offload_instance_t *offload)
{
    google_offload_proto_LECOCChannelInfo *channel_info = &(socket_open->ctx.channel_info.lecoc_channel_info);

    OFFLOAD_MGR_LOGM("add hyungchul : profile_mgr_lecoc_socket_open enter");
    OFFLOAD_MGR_LOGM("add hyungchul : socket_id lower32=%d acl=%d localCid=%d remoteCid=%d psm=%d localMtu=%d remoteMtu=%d localMps=%d remoteMps=%d rxCredits=%d txCredits=%d",
                     (uint32_t)socket_open->ctx.socket_id,
                     socket_open->ctx.aclConnectionHandle,
                     channel_info->localCid,
                     channel_info->remoteCid,
                     channel_info->psm,
                     channel_info->localMtu,
                     channel_info->remoteMtu,
                     channel_info->localMps,
                     channel_info->remoteMps,
                     channel_info->initialRxCredits,
                     channel_info->initialTxCredits);

    store_profile_data(socket_open, offload);

    OFFLOAD_MGR_LOGM("add hyungchul : call BmmSocketLecocOffloadReqSend");

    BmmSocketLecocOffloadReqSend(socket_open->ctx.socket_id,
                                 PM_LECOC_APP_HANDLE,
                                 socket_open->ctx.aclConnectionHandle,
                                 channel_info->localCid,
                                 channel_info->remoteCid,
                                 channel_info->localMtu,
                                 channel_info->remoteMtu,
                                 channel_info->localMps,
                                 channel_info->remoteMps,
                                 channel_info->initialRxCredits,
                                 channel_info->initialTxCredits,
                                 (BmmContext)offload);
}

void profile_mgr_lecoc_socket_close(BmmConnId connId)
{
    OFFLOAD_MGR_LOGM("add hyungchul : profile_mgr_lecoc_socket_close connId=%d", connId);
    BmmSocketCloseReqSend(connId, PM_LECOC_APP_HANDLE);
}

void profile_mgr_lecoc_data_tx(offload_instance_t *offload, int data_length, void *data)
{
    /* malloc the data to pass to microstack
       Note : 1. the data has to be malloced again as the microstack will try to free this block.
               because this *data ptr is part of the already malloced memory,
               and since the *data does not point to start of the memory malloced,
               this will not work.
               the free on this block will not work.

               2. Hence the data has to be malloced again. */

    void *data_ptr = bt_pal_malloc(data_length);
    if (data_ptr == NULL)
    {
        BT_PAL_ASSERT_FATAL("profile_mgr_lecoc_data_tx: malloc failed len:%d", data_length, 0, 0);
        return;
    }

    OFFLOAD_MGR_LOGM("add hyungchul : profile_mgr_lecoc_data_tx pseudo_id=%d len=%d",
                     offload->offload_data.socket_data.pseudo_id,
                     data_length);

    memcpy(data_ptr, data, data_length);
    BmmSocketDataReqSend(offload->offload_data.socket_data.pseudo_id,
                         PM_LECOC_APP_HANDLE,
                         data_length,
                         data_ptr,
                         (BmmContext)offload);
}

void profile_mgr_lecoc_data_rx_rsp(uint32_t pseudo_id)
{
    BmmContext context = pm_lecoc_take_rx_context((BmmConnId)pseudo_id);

    OFFLOAD_MGR_LOGM("add hyungchul: profile_mgr_lecoc_data_rx_rsp pseudo_id=%d context=0x%x",
                     pseudo_id,
                     (uint32_t)context);

    BmmSocketDataRspSend(pseudo_id, context);
}

static void microstack_lecoc_cb(BtAppHandle handle, BtEventClass eventClass, void *message)
{
    CsrPrim type = *((CsrPrim *)message);
    OFFLOAD_MGR_LOGM("add hyungchul : microstack_lecoc_cb type=%d handle=0x%x eventClass=%d", type, handle, eventClass);

    switch (type)
    {
    case BMM_SOCKET_DATA_CFM:
    {
        OFFLOAD_MGR_LOGM("add hyungchul : BMM_SOCKET_DATA_CFM");
        socket_mgr_send_data_tx_cfm_to_uapp(message);
    }
    break;

    case BMM_SOCKET_DATA_IND:
    {
        BmmSocketDataInd *ind = (BmmSocketDataInd *)message;
        OFFLOAD_MGR_LOGM("add hyungchul : BMM_SOCKET_DATA_IND connId=%d len=%d send to AWM and wait UAPP_DATA_RX_RES",
                         ind->connId,
                         ind->dataLen);

        pm_lecoc_store_rx_context(ind->connId, ind->context);
        socket_mgr_send_data_rx_ind_to_uapp(message);

        /* change(mod)-hyungchul :
         * Qualcomm LECoC Micro App API 문서 기준 DATA_IND 후 Micro App이 DATA_IND_RESP를 보내야 한다.
         * 기존 코드는 여기서 즉시 BmmSocketDataRspSend()를 호출했으나,
         * 이제 AWM이 UAPP_DATA_RX_RES를 보내면 endpt_mgr -> socket_mgr_data_rx_rsp()에서 응답한다.
         */
//        BmmSocketDataRspSend(((BmmSocketDataInd *)message)->connId, ((BmmSocketDataInd *)message)->context);
    }
    break;

    case BMM_SOCKET_LECOC_OFFLOAD_CFM:
    {
        OFFLOAD_MGR_LOGM("add hyungchul : BMM_SOCKET_LECOC_OFFLOAD_CFM");
		/* nothing to do here */
        socket_mgr_microstack_offload_cfm(message);
    }
    break;

    case BMM_SOCKET_CLOSE_CFM:
    {
        OFFLOAD_MGR_LOGM("add hyungchul : BMM_SOCKET_CLOSE_CFM");
        socket_mgr_microstack_unoffload_cfm(message);
    }
    break;

    case BMM_SOCKET_SET_PARAMS_CFM:
    {
        BmmSocketSetParamsCfm *cfm = (BmmSocketSetParamsCfm *)message;
        OFFLOAD_MGR_LOGM("BMM_SOCKET_SET_PARAMS_CFM : for conn id %d type %d resultCode : %d", cfm->connId, cfm->type, cfm->resultCode);
    }
    break;

    default:
    {
        OFFLOAD_MGR_LOGE("add hyungchul : unexpected microstack_lecoc_cb type=%d", type);
		/* should never come here */
        OFFLOAD_MGR_ASSERT_FATAL(0);
    }
    }
}

void profile_mgr_lecoc_microapp_socket_open_cb(const offload_instance_t *offload)
{
    OFFLOAD_MGR_LOGM("add hyungchul : profile_mgr_lecoc_microapp_socket_open_cb pseudo_id=%d",
                     offload->offload_data.socket_data.pseudo_id);
	/* set paramters for the opened socket */
    BmmSocketSetParamsReqSend(offload->offload_data.socket_data.pseudo_id,
                              PM_LECOC_APP_HANDLE,
                              BMM_SOCK_PREFERRED_RX_CREDITS,
                              SOCKET_MGR_LECOC_MAX_INITIAL_CREDITS, /* TBD */
                              NULL);
}
