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
 * @details This file is responsible for:
 *          1. Encoding and decoding of message formats used for communication with AWM over glink.
 */

/*============================================================================*
                           INCLUDE FILES
*============================================================================*/
#include "endpt_mgr_rpc.h"
#include "endpt_mgr_internal.h"

endpt_mgr_rpc_header_t *
endpt_mgr_rpc_make_header(uint16_t command_type, uint16_t opcode, uint16_t message_format,
                          uint64_t ep_id, int extra_size, int *out_size)
{
    int packet_size = ENDPT_MGR_RPC_HDR_SIZE + extra_size;
    endpt_mgr_rpc_header_t *header;

    *out_size = packet_size;
    header = (endpt_mgr_rpc_header_t *)offload_mgr_transport_shim_buf;

    header->command_type = command_type;
    header->opcode = opcode;
    header->message_format = message_format;
    header->endpoint_id = ep_id;

    return header;
}

static endpt_mgr_rpc_header_t endpt_mgr_rpc_header_init_default(void)
{
    return (endpt_mgr_rpc_header_t){OFFLOAD_APP_CMD, 0, MESSAGE_FORMAT_RAW, 0, ENDPT_MGR_HUB_ID_AWM};
}

endpt_mgr_rpc_header_t *endpt_mgr_rpc_get_default_header(void)
{
    endpt_mgr_rpc_header_t *header = (endpt_mgr_rpc_header_t *)offload_mgr_transport_shim_buf;
    *header = endpt_mgr_rpc_header_init_default();
    return header;
}
