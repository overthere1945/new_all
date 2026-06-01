/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      profile_mgr_lecoc.h
===========================================================================*/
/**
 * @file profile_mgr_lecoc.h
 * @brief Public header file for LECOC profile.
 *
 * @details This file contains the public API definitions for the LECOC profile.
 */

#ifndef PROFILE_MGR_LECOC_H
#define PROFILE_MGR_LECOC_H

#include "bmm_lib.h"
#include "bt_main.h"
#include "socket_mgr.pb.h"

/*===========================================================================
                        TYPE DEFINITINOS
===========================================================================*/
typedef struct offload_instance offload_instance_t;
typedef uint64_t socket_id_t;


/*===========================================================================
STRUCTURE        lecoc_data_t
===========================================================================*/
/**
 * Structure to maintain LECOC configuration data.
 *
 * @param psm Protocol/Service Multiplexer.
 * @param localMtu Local Maximum Transmission Unit.
 * @param remoteMtu Remote Maximum Transmission Unit.
 * @param localMps Local Maximum Packet Size.
 * @param remoteMps Remote Maximum Packet Size.
 */
typedef struct lecoc_data  {
    uint32_t psm;
    uint32_t localMtu;
    uint32_t remoteMtu;
    uint32_t localMps;
    uint32_t remoteMps;
}lecoc_data_t;

/*===========================================================================
FUNCTION      profile_mgr_lecoc_socket_open
===========================================================================*/
/**
 * @brief Opens a socket for the LECOC profile.
 *
 * This function takes a pointer to a google_offload_proto_socket_open structure and an offload instance as input,
 * and opens a socket for the LECOC profile.
 *
 * @param[in]    socket_open  Pointer to the google_offload_proto_socket_open structure.
 * @param[in]    offload      Pointer to the offload instance.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_lecoc_socket_open(google_offload_proto_socket_open *socket_open, offload_instance_t *offload);

/*===========================================================================
FUNCTION      profile_mgr_lecoc_socket_close
===========================================================================*/
/**
 * @brief Closes a socket for the LECOC profile.
 *
 * This function takes a BmmConnId as input and closes the corresponding socket for the LECOC profile.
 *
 * @param[in]    connId  The BmmConnId of the socket to be closed.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_lecoc_socket_close(BmmConnId connId);

/*===========================================================================
FUNCTION      profile_mgr_lecoc_data_tx
===========================================================================*/
/**
 * @brief Transmits data for the LECOC profile.
 *
 * This function takes an offload instance, data length, and a pointer to the data as input,
 * and transmits the data for the LECOC profile.
 *
 * @param[in]    offload      Pointer to the offload instance.
 * @param[in]    data_length  The length of the data to be transmitted.
 * @param[in]    data         Pointer to the data to be transmitted.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_lecoc_data_tx(offload_instance_t *offload, int data_length, void *data);

/*===========================================================================
FUNCTION      profile_mgr_lecoc_data_rx_rsp
===========================================================================*/
/**
 * @brief Sends a response for received data for the LECOC profile.
 *
 * This function takes a pseudo ID as input and sends a response for the received data for the LECOC profile.
 *
 * @param[in]    pseudo_id  The pseudo ID of the received data.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_lecoc_data_rx_rsp(uint32_t pseudo_id);

/*===========================================================================
FUNCTION      profile_mgr_lecoc_microapp_socket_open_cb
===========================================================================*/
/**
 * @brief Callback function for microapp socket open for the LECOC profile.
 *
 * This function takes a pointer to an offload instance as input and performs the necessary actions
 * when a microapp socket is opened for the LECOC profile.
 *
 * @param[in]    offload  Pointer to the offload instance.
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_lecoc_microapp_socket_open_cb(const offload_instance_t *offload);

/*===========================================================================
FUNCTION      profile_mgr_lecoc_init
===========================================================================*/
/**
 * @brief Initializes the LECOC profile manager.
 *
 * This function initializes the LECOC profile manager and performs any necessary setup.
 *
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_lecoc_init(void);

/*===========================================================================
FUNCTION      profile_mgr_lecoc_deinit
===========================================================================*/
/**
 * @brief Deinitializes the LECOC profile manager.
 *
 * This function deinitializes the LECOC profile manager and performs any necessary cleanup.
 *
 * @return       void.
 * @sideeffects  None.
 */
void profile_mgr_lecoc_deinit(void);

#endif /* PROFILE_MGR_LECOC_H*/