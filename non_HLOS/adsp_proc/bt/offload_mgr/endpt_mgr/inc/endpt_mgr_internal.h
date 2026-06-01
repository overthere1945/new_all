/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      endpt_mgr_internal.h
===========================================================================*/
/**
 * @file endpt_mgr_internal.h
 * @brief Handles the communication with endpoints on AWM.
 *
 * @details This file contains functions and definitions for managing communication 
 *          with endpoints on AWM.
 */

#ifndef ENDPT_MGR_INTERNAL_H
#define ENDPT_MGR_INTERNAL_H

/*============================================================================*
                           INCLUDE FILES
*============================================================================*/
#include "endpt_mgr.h"
#include "endpt_mgr_rpc.h"
#include "offload_mgr_transport_shim.h"
#include "offload_mgr_log.h"

/*============================================================================*
                           MACRO DEFINITIONS
*============================================================================*/

/*============================================================================*
                           LOCAL FUNCTION DECLARATIONS
*============================================================================*/

/*===========================================================================
FUNCTION      endpt_mgr_open_socket_rsp_cb
===========================================================================*/
/**
 * @brief Callback for socket open response.
 *
 * @param[in]    socket_id    ID of the socket.
 * @param[in]    status       Status of the socket open operation.
 * @return       void.
 * @sideeffects  None.
 */
void endpt_mgr_open_socket_rsp_cb(uint64_t socket_id, uint8_t status);

/*===========================================================================
FUNCTION      endpt_mgr_data_tx_req_cb
===========================================================================*/
/**
 * @brief Callback for data transmission request.
 *
 * @param[in]    socket_id     ID of the socket.
 * @param[in]    data          Pointer to the data to be transmitted.
 * @param[in]    data_length   Length of the data to be transmitted.
 * @return       void.
 * @sideeffects  None.
 */
void endpt_mgr_data_tx_req_cb(uint64_t socket_id, uint8_t *data, int data_length);

/*===========================================================================
FUNCTION      endpt_mgr_data_rx_rsp_cb
===========================================================================*/
/**
 * @brief Callback for data reception response.
 *
 * @param[in]    socket_id    ID of the socket.
 * @return       void.
 * @sideeffects  None.
 */
void endpt_mgr_data_rx_rsp_cb(uint64_t socket_id);

#endif /* ENDPT_MGR_INTERNAL_H */
