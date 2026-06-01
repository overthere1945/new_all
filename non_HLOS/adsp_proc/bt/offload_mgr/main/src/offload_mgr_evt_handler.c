/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_evt_handler.c
===========================================================================*/
/**
 * @file offload_mgr_evt_handler.c
 * @brief entry file to handle offload and unoffload events from socket/GATT HAL. 
 * @details This file contains the implementation of the offload manager,
 *          which handles offload and unoffload events from socket/GATT HAL.
 */
/*============================================================================*
                           INCLUDE FILES
*============================================================================*/
#include "offload_mgr_sm.h"
#include "offload_mgr_client_interface.h"
#include "offload_mgr_internal.h"
#include "socket_mgr.h"
#include "profile_mgr_gatt.h"

/*===========================================================================
FUNCTION      offload_mgr_handle_offload_request
===========================================================================*/
/**
 * @brief Handles offload requests based on the provided opcode.
 * @details Dispatches the offload event to the appropriate offload client depending on the opcode.
 * @param[in] opcode Opcode specifying the offload operation.
 * @param[in] evt Pointer to the event data.
 * @param[in] len Length of the event data.
 */
static void offload_mgr_handle_offload_request(uint8_t *evt, size_t len, uint16_t opcode)
{
    if (opcode == BT_OFFLOAD_OPEN_SOCKET)
    {
        (void)socket_mgr_handle_open_socket(evt, len);
    }
    else if (opcode == BT_OFFLOAD_GATT_REGISTER_SERVICE)
    {
        (void)profile_mgr_gatt_hal_offload(evt, len);
    }
}

/*===========================================================================
FUNCTION      offload_mgr_handle_unoffload_request
===========================================================================*/
/**
 * @brief Handles unoffload requests based on the provided opcode.
 * @details Dispatches the unoffload event to the appropriate offload client depending on the opcode.
 * @param[in] opcode Opcode specifying the unoffload operation.
 * @param[in] evt Pointer to the event data.
 * @param[in] len Length of the event data.
 */

static void offload_mgr_handle_unoffload_request(uint8_t *evt, size_t len, uint16_t opcode)
{
    if (opcode == BT_OFFLOAD_CLOSE_SOCKET)
    {
        (void)socket_mgr_handle_close_socket(evt, len);
    }
    else if (opcode == BT_OFFLOAD_GATT_UNREGISTER_SERVICE)
    {
        (void)profile_mgr_gatt_hal_unoffload(evt, len);
    }
}

/*===========================================================================
FUNCTION      offload_mgr_handle_multiple_unoffload_request
===========================================================================*/
/**
 * @brief Handles multiple unoffload requests for GATT services.
 * @details Clears multiple GATT services based on the event data.
 * @param[in] evt Pointer to the event data.
 * @param[in] len Length of the event data.
 */
static void offload_mgr_handle_multiple_unoffload_request(uint8_t *evt, size_t len)
{
    (void)profile_mgr_gatt_hal_clear_service(evt, len);
}

/*===========================================================================
FUNCTION      offload_mgr_handle_offload_get_caps
===========================================================================*/
/**
 * @brief Handles offload capability requests based on the provided opcode.
 * @details Dispatches the capability request to the appropriate handler depending on the opcode.
 * @param[in] opcode Opcode specifying the capability request.
 */
static void offload_mgr_handle_offload_get_caps(uint16_t opcode)
{
    if (opcode == BT_OFFLOAD_SOCKET_CAPS)
    {
        (void)socket_mgr_get_socket_capabilities();
    } 
    else if (opcode == BT_OFFLOAD_GATT_GET_CAPABILITIES)
    {
        (void)profile_mgr_gatt_hal_get_caps();
    }
}


/*===========================================================================
FUNCTION      offload_mgr_evt_handler
===========================================================================*/
/**
 * @brief Handle events from Socket and GATT HAL.
 * @details This function handles events based on the opcode.
 * @param[in] opcode Opcode of the event.
 * @param[in] evt Pointer to the event data.
 * @param[in] len Length of the event data.
 */

void offload_mgr_evt_handler(uint16_t opcode, uint8_t *evt, size_t len)
{
    switch (opcode)
    {
    /* proto messages coming from hlos */
    case BT_OFFLOAD_OPEN_SOCKET:
    case BT_OFFLOAD_GATT_REGISTER_SERVICE:
    {
        (void)offload_mgr_handle_offload_request(evt, len, opcode);
        break;
    }
    case BT_OFFLOAD_CLOSE_SOCKET:
    case BT_OFFLOAD_GATT_UNREGISTER_SERVICE:
    {
        (void)offload_mgr_handle_unoffload_request(evt, len, opcode);
        break;
    }
    case BT_OFFLOAD_SOCKET_CAPS:
    case BT_OFFLOAD_GATT_GET_CAPABILITIES:
    {
        (void)offload_mgr_handle_offload_get_caps(opcode);
        break;
    }
    case BT_OFFLOAD_GATT_CLEAR_SERVICES:
    {
        (void)offload_mgr_handle_multiple_unoffload_request(evt, len);
        break;
    }
    default:
    {
        OFFLOAD_MGR_LOGE("offload_mgr_sm_evt_handler : Invalid opcode : %d", opcode);
        break;
    }
    }
}

/*===========================================================================
FUNCTION      offload_mgr_profile_unoffload_cfm_cb
===========================================================================*/
/**
 * @brief To be called by socket/session manager when the profile unoffload operation is confirmed.
 * @details This function is called when the profile unoffload operation is confirmed.
 * @param[in] id Offload instance ID.
 * @param[in] status Status of the unoffload operation.
 */
void offload_mgr_profile_unoffload_cfm_cb(offload_instance_id_t id, int status) {
    const offload_instance_t *offload = offload_instance_find_by_id(id);
    if (offload == NULL) {
        OFFLOAD_MGR_LOGE("offload_mgr_profile_unoffload_cfm_cb : No such offload instance");
        return;
    }
    offload_mgr_sm_handle_microstack_unoffload_evt((offload_instance_t *)offload, status);

}

/*===========================================================================
FUNCTION      offload_mgr_profile_offload_cfm_cb
===========================================================================*/
/**
 * @brief  To be called by socket/session manager when Profile offload is complete.
 * @details This function is called when the profile offload operation is confirmed.
 * @param[in] id Offload instance ID.
 * @param[in] status Status of the offload operation.
 */
void offload_mgr_profile_offload_cfm_cb(offload_instance_id_t id, int status) {
    const offload_instance_t *offload = offload_instance_find_by_id(id);
    if (offload == NULL) {
        OFFLOAD_MGR_LOGE("offload_mgr_profile_offload_cfm_cb : No such offload instance");
        return;
    }
    offload_mgr_sm_handle_microstack_offload_evt((offload_instance_t *)offload, status);
}

/*===========================================================================
FUNCTION      offload_mgr_uapp_offload_rsp_cb
===========================================================================*/
/**
 * @brief To be called by endpoint manager when the uapp offload operation is complete. 
 * @details This function is called when the uapp offload operation is complete.
 * @param[in] id Offload instance ID.
 * @param[in] status Status of the offload operation.
 */
void offload_mgr_uapp_offload_rsp_cb(offload_instance_id_t id, uint8_t status) {
    const offload_instance_t *offload = offload_instance_find_by_id(id);
    if (offload == NULL) {
        OFFLOAD_MGR_LOGE("offload_mgr_uapp_offload_rsp_cb : No such offload instance");
        return;
    }
    offload_mgr_sm_handle_uapp_offload_evt((offload_instance_t *)offload, status);
}

/*===========================================================================
FUNCTION      offload_mgr_uapp_unoffload_rsp_cb
===========================================================================*/
/**
 * @brief To be called by endpoint manager when the unoffload operation to microapps is complete. 
 * @details This function is called when the uapp unoffload operation is responded.
 * @param[in] id Offload instance ID.
 * @param[in] status Status of the unoffload operation.
 */
void offload_mgr_uapp_unoffload_rsp_cb(offload_instance_id_t id, uint8_t status) {
    const offload_instance_t *offload = offload_instance_find_by_id(id);
    if (offload == NULL) {
        OFFLOAD_MGR_LOGE("offload_mgr_uapp_unoffload_rsp_cb : No such offload instance");
        return;
    }
    offload_mgr_sm_handle_uapp_unoffload_evt(offload, status);
}

