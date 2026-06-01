/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2026 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_transport_shim.c
===========================================================================*/
/**
 * @file offload_mgr_transport_shim.c
 * @brief Shim layer to support interfacing between Offload manager APIs and Transport.
 *
 * @details This file contains the implementation of the shim layer to facilitate
 *          communication between offload manager APIs and the transport layer.
 */

/*============================================================================*
                                Include Files
*============================================================================*/
#include "bt_pal_heap.h"
#include "offload_mgr_transport_shim.h"
#include "offload_mgr_thread.h"
#include "hci_arbiter_lib.h"
#include "bt_state_mgr.h"
#include "bt_utils.h"
#include "offload_mgr_log.h"
#include "offload_mgr_proto_utils.h"

/*=============================================================================
                        MACRO DEFINITIONS
=============================================================================*/

/**
 * @brief Offset to the payload received in a HCI packet from HLOS.
 *
 * @note first 6 bytes are reserved for HEADER
 */
#define BT_HCI_PKT_PAYLOAD_OFFSET  (0x6U)

/*=============================================================================
                    GLOBAL DATA DECLARATIONS
=============================================================================*/
/**
 * @brief function pointer to post HCI message to HCI arbiter
 */
void (*BT_Offload_Post_HciArb_HciMsg)(uint16_t len, uint8_t *pkt);

/**
 * @brief Buffer for transport shim.
 */
uint8_t offload_mgr_transport_shim_buf[OFFLOAD_MGR_TRANSPORT_SHIM_BUF_LEN] = {0};

/**
 * @brief Length of the encoded buffer.
 */
size_t offload_mgr_transport_shim_encoded_buf_len = 0;

/*===========================================================================
                    LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
FUNCTION      get_awm_msg_from_chnl
===========================================================================*/
/**
 * @brief Gets AWM message from channel ID.
 *
 * @param channel_id The channel ID.
 * @return The AWM message.
 */
static uint8_t get_awm_msg_from_chnl(uint8_t channel_id)
{
    switch (channel_id)
    {
    case LPAI_TRANSPORT_AWM_APP_MGR_CHNL_IDX:
    {
        return OFFLOAD_MGR_MSG_GLINK_APP_MANAGER_RX;
    }
    break;
    }
    BT_PAL_ASSERT_FATAL("invalid channel id : %d", channel_id, 0, 0);
    return 0;
}

/*===========================================================================
FUNCTION      get_hlos_msg_from_chnl
===========================================================================*/
/**
 * @brief Gets HLOS message from channel ID.
 *
 * @param channel_id The channel ID.
 * @return The HLOS message.
 */
static uint8_t get_hlos_msg_from_chnl(uint8_t channel_id)
{
    switch (channel_id)
    {
    case LPAI_TRANSPORT_HLOS_BT_SOCKET_CHNL_IDX:
    {
        return OFFLOAD_MGR_MSG_GLINK_SOCKET_OFFLOAD_RX;
    }
    break;
    case LPAI_TRANSPORT_HLOS_BT_GATT_CHNL_IDX:
    {
        return OFFLOAD_MGR_MSG_GLINK_GATT_OFFLOAD_RX;
    }
    break;
    case LPAI_TRANSPORT_HLOS_CONTEXT_HUB_CHNL_IDX:
    {
        return OFFLOAD_MGR_MSG_GLINK_CONTEXT_HUB_RX;
    }
    break;
    case LPAI_TRANSPORT_HLOS_HCI_CHNL_IDX:
    {
        return 0; /* Handled separately */
    }
    break;
#ifdef ENABLE_WLAN_TSF_GPIO
    case LPAI_TRANSPORT_HLOS_WLAN_GPIO_CHNL_IDX:
    {
        return OFFLOAD_MGR_MSG_GLINK_WLAN_GPIO_RX;
    }
    break;
#endif
    }
    BT_PAL_ASSERT_FATAL("invalid channel id : %d", channel_id, 0, 0);
    return 0;
}

/*===========================================================================
FUNCTION      get_msg_type_from_link_and_chnl
===========================================================================*/
/**
 * @brief Gets message type from link and channel ID.
 *
 * @param link_id The link ID.
 * @param channel_id The channel ID.
 * @return The message type.
 */
static uint8_t get_msg_type_from_link_and_chnl(uint8_t link_id, uint8_t channel_id)
{
    switch (link_id)
    {
    case LPAI_TRANSPORT_AWM_LINK_IDX:
    {
        return get_awm_msg_from_chnl(channel_id);
    }
    break;
    case LPAI_TRANSPORT_HLOS_LINK_IDX:
    {
        return get_hlos_msg_from_chnl(channel_id);
    }
    break;
    }
    BT_PAL_ASSERT_FATAL("invalid link id : %d", link_id, 0, 0);
    return 0;
}

/*===========================================================================
FUNCTION      BT_Offload_HciMsgCb
===========================================================================*/
/**
 * @brief Callback function for HCI message.
 *
 * @param length Length of the HCI message.
 * @param data Pointer to the HCI message data.
 */
void BT_Offload_HciMsgCb(uint16_t length, uint8_t *data)
{
    /* case 1: API Call to dummy Arbiter */
    uint16_t hci_arb_pkt_len = 2 + 2 + 2 + length; /* pkt_type + enc_dec + length + hci packet */

    offload_mgr_transport_shim_buf[0] = data[0];                 // opcode_byte_0
    offload_mgr_transport_shim_buf[1] = 0;                       // opcode_byte_1
    offload_mgr_transport_shim_buf[2] = 0;                       // enc_dec_0
    offload_mgr_transport_shim_buf[3] = 0;                       // enc_dec_1
    offload_mgr_transport_shim_buf[4] = (uint8_t)(length);       // len_0
    offload_mgr_transport_shim_buf[5] = (uint8_t)(length >> 8u); // len_1

    // memscpy(&offload_mgr_transport_shim_buf[3], length, data, length);
    memcpy(&offload_mgr_transport_shim_buf[6], data, length);
    Offload_Mgr_SendDataToHCI(offload_mgr_transport_shim_buf, hci_arb_pkt_len);

    // OFFLOAD_MGR_LOGL("BT_Offload_Manager_valid_hci_event\n");
    bt_pal_free(data);
}

/*=============================================================================
                    GLOBAL FUNCTION DEFINITIONS
=============================================================================*/
/*===========================================================================
FUNCTION      Offload_Mgr_Post_Transport_Event
===========================================================================*/
/**
 * @brief Posts transport event to Offload Manager.
 */
void Offload_Mgr_Post_Transport_Event(uint8_t link_id, uint8_t channel_id, const uint8_t *blob, size_t len)
{
    uint8_t msg_type = get_msg_type_from_link_and_chnl(link_id, channel_id);
    Offload_Mgr_Post_Event(msg_type, (uint8_t *)blob, len);
}

/*===========================================================================
FUNCTION      Offload_Mgr_Post_arbiter_event
===========================================================================*/
/**
 * @brief Posts arbiter event to Offload Manager.
 */
void Offload_Mgr_Post_arbiter_event(uint8_t link_id, uint8_t channel_id, const uint8 *blob, size_t len)
{

    uint16_t hdr = FETCH_2_BYTE_LITTLE_ENDIAN_FROM_ARR(blob);
    OFFLOAD_MGR_LOGH("Offload_Mgr_Post_arbiter_event: hdr:%d len:%d\n", hdr, len);

    //for (uint8_t i = 0; i < 6U; i++)
    //{
    //     RPC_PRINTF("Offload_Mgr_Task_Handler: hdr:%d\n",blob[i]);
    // }

    if ((hdr == BT_ENABLE || hdr == BT_DISABLE))
    {
        Offload_Mgr_Post_Event(OFFLOAD_MGR_MSG_BT_STATE_EVT_RX, (uint8_t *)blob, len);
    }
    else if (BT_Offload_Post_HciArb_HciMsg != NULL)
    {
        uint16_t bt_evt_len = FETCH_2_BYTE_LITTLE_ENDIAN_FROM_ARR(blob + 4);

        /* TBD: @vbandi, talk to microstack team for removing the bt_pal_malloc and bt_pal_free logic */
        uint8_t *hci_pkt = bt_pal_malloc(bt_evt_len);
        if (hci_pkt == NULL)
        {
            BT_PAL_ASSERT_FATAL("Offload_Mgr_Post_arbiter_event: Failed to rx HCI_arbiter msg len '%d'", bt_evt_len, 0, 0);
            return;
        }
        memcpy(hci_pkt, &blob[BT_HCI_PKT_PAYLOAD_OFFSET], bt_evt_len);

        BT_Offload_Post_HciArb_HciMsg(bt_evt_len,(uint8_t *)hci_pkt);
        bt_pal_free((void *)blob);
    }
    else
    {
        bt_pal_free((void *)blob);
    }
}

/*===========================================================================
FUNCTION      Offload_Mgr_Post_Microstack_Evt
===========================================================================*/
/**
 * @brief Posts Microstack event to Offload Manager.
 */
void Offload_Mgr_Post_Microstack_Evt(uint8_t *blob, uint16_t len)
{
    Offload_Mgr_Post_Event(OFFLOAD_MGR_MSG_MICROSTACK_RX, blob, len);
}

/*===========================================================================
FUNCTION      BT_Offload_HciArbiter_Init
===========================================================================*/
/**
 * @brief Initializes the HCI Arbiter.
 */
void BT_Offload_HciArbiter_Init(void)
{
    HciArbiterRegisterHciMsgCb(BT_Offload_HciMsgCb);
    BT_Offload_Post_HciArb_HciMsg = HciArbiterSendHciMsg;
}

/*===========================================================================
FUNCTION      BT_Offload_HciArbiter_DeInit
===========================================================================*/
/**
 * @brief De-Initializes the HCI Arbiter.
 */
void BT_Offload_HciArbiter_DeInit(void)
{
    if (BT_Offload_Post_HciArb_HciMsg != NULL)
    {
        BT_Offload_Post_HciArb_HciMsg = NULL;
    }
}

void Offload_Mgr_Update_Header(uint8_t *out_buf, uint16_t opcode, uint16_t length)
{
    /* opcode */
    out_buf[0] = LE_FIRST_BYTE(opcode);
    out_buf[1] = LE_SECOND_BYTE(opcode);

    /* proto encoded  : for now all of them are encoded */
    out_buf[2] = out_buf[3] = 0;

    /* length */
    out_buf[4] = LE_FIRST_BYTE(length);
    out_buf[5] = LE_SECOND_BYTE(length);

    offload_mgr_transport_shim_encoded_buf_len = length + OFFLOAD_MGR_TRANSPORT_SHIM_HDR_LEN;
}
