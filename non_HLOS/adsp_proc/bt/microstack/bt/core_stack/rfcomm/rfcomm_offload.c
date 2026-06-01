/*******************************************************************************
Copyright (C) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "rfcomm_private.h"

#ifdef INSTALL_SOCKET_OFFLOAD
#include "socket_offload.h"

/******************************************************************************
  rfcomm_offload_dlci : Function to offload RFCOMM DLCI
******************************************************************************/
uint16_t rfcomm_offload_dlci(uint8_t dlci,
                         uint16_t tx_credits,
                         uint16_t max_frame_size,
                         uint16_t flags,
                         l2ca_cid_t cid,
                         uint16_t l2cap_mtu,
                         phandle_t phandle)
{
    RFC_CTRL_PARAMS_T rfc_params = {NULL, NULL, NULL, NULL, 0};
    RFC_CHAN_T *p_dlci;
    RFC_CHAN_T *p_mux;
    bool_t initiator = (flags & 0x01);

    rfc_params.rfc_ctrl = &rfc_ctrl_allocation;

    /* Get the Mux channel associated with the CID */
    rfc_get_mux_by_cid(&rfc_params, cid);

    if (NULL == rfc_params.p_mux)
    {
        rfc_mux_new(&rfc_params,
                    phandle,
                    NULL,
                    0, 
                    initiator);

        p_mux = rfc_params.rfc_ctrl->mux_list;
        p_mux->state = RFC_ST_CONNECTED;
        p_mux->info.mux.cid = cid;
        p_mux->info.mux.l2cap_mtu = l2cap_mtu;
    }

    rfc_find_chan_by_dlci(&rfc_params, dlci);
    if (rfc_params.p_dlc != NULL)
    {
        BLUESTACK_PANIC(PANIC_OFFLOAD_RFCOMM_DLCI_EXISTS);
    }

    rfc_dlc_new(&rfc_params, dlci, phandle, NULL, initiator, 0);
    p_dlci = rfc_params.p_dlc;
    p_dlci->state = RFC_ST_CONNECTED;
    p_dlci->info.dlc.rx_credits = 0;
    p_dlci->info.dlc.tx_credits = tx_credits;
    p_dlci->flags |=  (RFC_INBOUND_MSC_COMPLETE | RFC_OUTBOUND_MSC_COMPLETE);    
    p_dlci->info.dlc.allocated_rx_credits = 0;
    p_dlci->info.dlc.max_frame_size = max_frame_size;

    return p_dlci->info.dlc.conn_id;
}

/******************************************************************************
  rfcomm_disconnect_dlci : Function to disconnect RFCOMM DLCI
******************************************************************************/
void rfcomm_disconnect_dlci(uint16_t conn_id)
{
    RFC_CTRL_PARAMS_T rfc_params = {NULL, NULL, NULL, NULL, 0};

    rfc_params.rfc_ctrl = &rfc_ctrl_allocation;

    rfc_find_chan_by_id(&rfc_params, conn_id);
    if (rfc_params.p_dlc != NULL)
    {
        rfc_release_dlc(&rfc_params);
    }
}

#endif
