/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      socket_mgr.h
===========================================================================*/
/**
 * @file socket_mgr.h
 * @brief implementation of socket manager. 
 *
 * @details socket manager is responsible for managing sockets and communication with socket HAL. 
 */

#ifndef SOCKET_MGR_H
#define SOCKET_MGR_H

/*===========================================================================
                    INCLUDE FILES
===========================================================================*/
#include "sm.h"
#include "bmm_lib.h"
#include "bt_main.h"
#include "offload_mgr_log.h"

#include "offload_mgr_sm.h"
#include "offload_instance.h"
#include "socket.h"
#include "socket_mgr.h"
#include "offload_mgr_msg_utils.h"
#include "socket_mgr.pb.h"
#include "endpt_mgr.h"
#include "offload_mgr_transport_shim.h"
#include "offload_mgr_internal.h"

/*============================================================================*
                                MACRO DEFINITIONS
*============================================================================*/
/**
 * @brief   Initial credits for lecoc communication
 * @note    value 10 is experimental, need to change based on requirements.
 */
#define SOCKET_MGR_LECOC_MAX_INITIAL_CREDITS        10

/**
 * @brief   MAX LECOC MTU value for offload lecoc communication.
 */
#define SOCKET_MGR_LECOC_MAX_LECOC_MTU              986

/**
 * @brief   Initial credits for rfcomm communication
 * @note    value 5 is experimental, need to change based on requirements.
 */
#define SOCKET_MGR_RFCOMM_MAX_INITIAL_CREDITS       5

/**
 * @brief   MAX RFCCOMM Frame size for offload rfcomm communication
 */
#define SOCKET_MGR_RFCOMM_MAX_FRAME_SIZE            990

/*===========================================================================
                    TYPE DEFINITIONS
===========================================================================*/
typedef enum {
  SOCKET_MGR_SOCKET_OPEN_SUCCESS = 0, 
  SOCKET_MGR_SOCKET_OPEN_FAILED = 1,
}socket_mgr_socket_open_status_t;

/*===========================================================================
                  LOCAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
FUNCTION      decode_socket_close
===========================================================================*/
/**
* @brief Decodes a socket close event.
*
* @param evt Pointer to the event data.
* @param len Length of the event data.
* @param socketId Pointer to the socket ID.
* @return true if decoding is successful, false otherwise.
*/
bool decode_socket_close(uint8_t *evt, size_t len, socket_id_t *socketId);

/*===========================================================================
FUNCTION      decode_socket_open
===========================================================================*/
/**
* @brief Decodes a socket open event.
*
* @param evt Pointer to the event data.
* @param len Length of the event data.
* @param socket_open Pointer to the socket open structure.
* @return true if decoding is successful, false otherwise.
*/
bool decode_socket_open(uint8_t *evt, size_t len, google_offload_proto_socket_open *socket_open);

/*===========================================================================
FUNCTION      encode_socket_close_ind
===========================================================================*/
/**
* @brief Encodes a socket close indication.
*
* @param opcode Opcode for the socket close indication.
* @param socket_close_ind Pointer to the socket close indication structure.
* @return true if encoding is successful, false otherwise.
*/
bool encode_socket_close_ind(uint16_t opcode, google_offload_proto_socket_close_ind *socket_close_ind);

/*===========================================================================
FUNCTION      encode_socket_close_rsp
===========================================================================*/
/**
* @brief Encodes a socket close response.
*
* @param opcode Opcode for the socket close response.
* @param socket_close_rsp Pointer to the socket close response structure.
* @param reason Pointer to the reason for socket closure.
* @return true if encoding is successful, false otherwise.
*/
bool encode_socket_close_rsp(uint16_t opcode, google_offload_proto_socket_close_rsp *socket_close_rsp, uint8_t *reason);

/*===========================================================================
FUNCTION      encode_socket_open_rsp
===========================================================================*/
/**
* @brief Encodes a socket open response.
*
* @param opcode Opcode for the socket open response.
* @param socket_open_rsp Pointer to the socket open response structure.
* @param reason Pointer to the reason for socket opening.
* @return true if encoding is successful, false otherwise.
*/
bool encode_socket_open_rsp(uint16_t opcode, google_offload_proto_socket_open_rsp *socket_open_rsp, uint8_t *reason);

/*===========================================================================
FUNCTION      encode_socket_capabilities
===========================================================================*/
/**
* @brief Encodes socket capabilities.
*
* @param opcode Opcode for the socket capabilities.
* @param socket_caps Pointer to the socket capabilities structure.
* @return true if encoding is successful, false otherwise.
*/
bool encode_socket_capabilities(uint16_t opcode, google_offload_proto_SocketCapabilities *socket_caps);

/*===========================================================================
FUNCTION      socket_mgr_get_socket_capabilities
===========================================================================*/
/**
 * @brief Get socket capabilities.
 * @details This function retrieves the socket capabilities and encodes them 
 *          into a Google offload protocol socket capabilities structure.
 */
void socket_mgr_get_socket_capabilities(void);

/*===========================================================================
FUNCTION      socket_mgr_init
===========================================================================*/
/**
 * @brief Initialize socket manager.
 * @details This function initializes the socket manager and registers the client functions.
 * 1. these registered functions are triggered from offload manager state machine.. 
 * 2. initialize profiles lecoc and rfcomm
 */
void socket_mgr_init(void);


/*===========================================================================
FUNCTION      socket_mgr_deinit
===========================================================================*/
/**
 * @brief Deinitialize socket manager.
 * @details This function deinitializes the socket manager
 */
void socket_mgr_deinit(void);


/*===========================================================================
FUNCTION      socket_mgr_send_offload_resp_to_hal
===========================================================================*/
/**
 * @brief Send offload response to HAL.
 * @details This function sends an open socket response to the HAL.
 * @param[in] data Pointer to the offload_instance_t structure.
 * @param[in] status Status of the offload operation.
 */
void socket_mgr_send_offload_resp_to_hal(void *data, int status);

/*===========================================================================
FUNCTION      socket_mgr_send_unoffload_resp_to_hal
===========================================================================*/
/**
 * @brief Send unoffload response to HAL.
 * @details This function sends a close socket response to the HAL.
 * @param[in] data Pointer to the offload_instance_t structure.
 * @param[in] status Status of the unoffload operation.
 */
void socket_mgr_send_unoffload_resp_to_hal(void *data, int status);

/*===========================================================================
FUNCTION      socket_mgr_data_tx
===========================================================================*/
/**
 * @brief Send data to the microapp.
 * @details This function sends data to the microapp for the specified socket ID.
 * @param[in] socket_id Socket ID.
 * @param[in] data Pointer to the data to be sent.
 * @param[in] data_length Length of the data to be sent.
 */
void socket_mgr_data_tx(uint64_t socket_id, void *data, int data_length);

/*===========================================================================
FUNCTION      socket_mgr_data_rx_rsp
===========================================================================*/
/**
 * @brief Send data receive response to the microapp.
 * @details This function sends a data receive response to the microapp for the specified socket ID.
 * @param[in] socket_id Socket ID.
 */
void socket_mgr_data_rx_rsp(uint64_t socket_id);

/*===========================================================================
FUNCTION      socket_mgr_microstack_offload_cfm
===========================================================================*/
/**
 * @brief Microstack offload confirmation.
 * @details This function is called when the microstack offload operation is confirmed.
 * @param[in] message Pointer to the confirmation message.
 */
void socket_mgr_microstack_offload_cfm(void *message);

/*===========================================================================
FUNCTION      socket_mgr_microstack_unoffload_cfm
===========================================================================*/
/**
 * @brief Microstack unoffload confirmation.
 * @details This function is called when the microstack unoffload operation is confirmed.
 * @param[in] message Pointer to the confirmation message.
 */
void socket_mgr_microstack_unoffload_cfm(void *message);

/*===========================================================================
FUNCTION      socket_mgr_send_microstack_socket_close_ind_to_hal
===========================================================================*/
/**
 * @brief Send microstack socket close indication to HAL.
 * @details This function sends a socket close indication to the HAL.
 * @param[in] socket_id Socket ID.
 * @param[in] status Status of the socket close operation.
 */
void socket_mgr_send_microstack_socket_close_ind_to_hal(socket_id_t socket_id, int status);

/*===========================================================================
FUNCTION      socket_mgr_send_data_tx_cfm_to_uapp
===========================================================================*/
/**
 * @brief Send data transmit confirmation.
 * @details This function sends a confirmation for a data transmit operation.
 * @param[in] cfm Pointer to the BmmSocketDataCfm structure containing the confirmation data.
 */

void socket_mgr_send_data_tx_cfm_to_uapp(BmmSocketDataCfm *cfm);

/*===========================================================================
FUNCTION      socket_mgr_send_data_rx_ind_to_uapp
===========================================================================*/
/**
 * @brief Send data receive indication.
 * @details This function sends an indication for a data receive operation.
 * @param[in] ind Pointer to the BmmSocketDataInd structure containing the indication data.
 */
void socket_mgr_send_data_rx_ind_to_uapp(BmmSocketDataInd *ind);

/*===========================================================================
FUNCTION      socket_mgr_handle_open_socket
===========================================================================*/
/**
 * @brief Open a socket based on the event data.
 * @details This function processes an event to open a socket,
 *          using the provided event data and length.
 * @param[in] evt Pointer to the event data.
 * @param[in] len Length of the event data.
 */
void socket_mgr_handle_open_socket(uint8_t *evt, size_t len);

/*===========================================================================
FUNCTION      socket_mgr_handle_close_socket
===========================================================================*/
/**
 * @brief Close a socket based on the event data.
 * @details This static function processes an event to close a socket,
 *          using the provided event data and length.
 * @param[in] evt Pointer to the event data.
 * @param[in] len Length of the event data.
 */
void socket_mgr_handle_close_socket(uint8_t *evt, size_t len);
#endif /* SOCKET_MGR_H */