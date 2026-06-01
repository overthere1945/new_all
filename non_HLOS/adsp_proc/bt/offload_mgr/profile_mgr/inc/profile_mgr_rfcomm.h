/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      profile_mgr_rfcomm.h
===========================================================================*/
/**
 * @file profile_mgr_rfcomm.h
 * @brief Public header file for RFCOMM profile.
 *
 * @details This file contains the public API definitions for the RFCOMM profile.
 */
#ifndef PROFILE_MGR_RFCOMM_H
#define PROFILE_MGR_RFCOMM_H

#include "bmm_lib.h"
#include "bt_main.h"
#include "socket_mgr.pb.h"

typedef struct offload_instance offload_instance_t;
typedef uint64_t socket_id_t;

/*===========================================================================
STRUCTURE        rfcomm_data_t
===========================================================================*/
/**
 * Structure to maintain RFCOMM configuration data.
 *
 * @param psm Protocol/Service Multiplexer.
 * @param localMtu Local Maximum Transmission Unit.
 * @param remoteMtu Remote Maximum Transmission Unit.
 * @param localCid Local Maximum Packet Size.
 * @param remoteCid Remote Maximum Packet Size.
 * @param maxFrameSize Maximum Frame Size
 */
typedef struct rfcomm_data  {
    uint32_t localMtu;
    uint32_t remoteMtu;
    uint32_t localCid;
    uint32_t remoteCid;
    uint32_t  maxFrameSize;
}rfcomm_data_t;

/*===========================================================================
FUNCTION      profile_mgr_rfcomm_socket_close
===========================================================================*/
/**
 * @brief Closes a socket for the RFCOMM profile.
 *
 * This function takes a BmmConnId as input and closes the corresponding socket for the RFCOMM profile.
 *
 * @param[in]    connId  The BmmConnId of the socket to be closed.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_rfcomm_socket_close(BmmConnId connId);

/*===========================================================================
FUNCTION      profile_mgr_rfcomm_socket_open
===========================================================================*/
/**
 * @brief Opens a socket for the RFCOMM profile.
 *
 * This function takes a pointer to a google_offload_proto_socket_open structure and an offload instance as input,
 * and opens a socket for the RFCOMM profile.
 *
 * @param[in]    socket_open  Pointer to the google_offload_proto_socket_open structure.
 * @param[in]    offload      Pointer to the offload instance.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_rfcomm_socket_open(google_offload_proto_socket_open *socket_open, offload_instance_t *offload);

/*===========================================================================
FUNCTION      profile_mgr_rfcomm_microapp_socket_open_cb
===========================================================================*/
/**
 * @brief Callback function for microapp socket open for the RFCOMM profile.
 *
 * This function takes a pointer to an offload instance as input and performs the necessary actions
 * when a microapp socket is opened for the RFCOMM profile.
 *
 * @param[in]    offload  Pointer to the offload instance.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_rfcomm_microapp_socket_open_cb(const offload_instance_t *offload);

/*===========================================================================
FUNCTION      profile_mgr_rfcomm_data_tx
===========================================================================*/
/**
 * @brief Transmits data for the RFCOMM profile.
 *
 * This function takes an offload instance, data length, and a pointer to the data as input,
 * and transmits the data for the RFCOMM profile.
 *
 * @param[in]    offload      Pointer to the offload instance.
 * @param[in]    data_length  The length of the data to be transmitted.
 * @param[in]    data         Pointer to the data to be transmitted.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_rfcomm_data_tx(offload_instance_t *offload, int data_length, void *data);

/*===========================================================================
FUNCTION      profile_mgr_rfcomm_data_rx_rsp
===========================================================================*/
/**
 * @brief Sends a response for received data for the RFCOMM profile.
 *
 * This function takes a pseudo ID as input and sends a response for the received data for the RFCOMM profile.
 *
 * @param[in]    pseudo_id  The pseudo ID of the received data.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_rfcomm_data_rx_rsp(uint32_t pseudo_id);

/*===========================================================================
FUNCTION      profile_mgr_rfcomm_init
===========================================================================*/
/**
 * @brief Initializes the RFCOMM profile manager.
 *
 * This function initializes the RFCOMM profile manager and performs any necessary setup.
 *
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_rfcomm_init(void);

/*===========================================================================
FUNCTION      profile_mgr_rfcomm_deinit
===========================================================================*/
/**
 * @brief Deinitializes the RFCOMM profile manager.
 *
 * This function deinitializes the RFCOMM profile manager and performs any necessary cleanup.
 *
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_rfcomm_deinit(void);

#endif /* PROFILE_MGR_RFCOMM_H*/
