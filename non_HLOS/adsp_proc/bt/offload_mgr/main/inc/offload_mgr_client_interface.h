/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_client_interface.h
===========================================================================*/
/**
 * @file offload_mgr_client_interface.h
 * @brief Offload manager client interface file, to init and interface all clients
 *        with offload manager. 
 *
 * @details 1. Socket manager, session manager and endpt manager
 *          are clients to offload manager. 
 */

#ifndef OFFLOAD_MGR_CLIENT_INTERFACE_H
#define OFFLOAD_MGR_CLIENT_INTERFACE_H

/*===========================================================================
                            INCLUDE FILES
===========================================================================*/
#include "bmm_lib.h"
#include "bt_main.h"
#include "offload_instance.h"

/**
 * @struct microstack_msg_t
 * @brief Structure for microstack messages in the profile manager.
 *
 * @details This structure contains the handle, event class, and message for microstack messages.
 */
typedef struct {
  BtAppHandle handle;      /**< Handle for the Bluetooth application */
  BtEventClass eventClass; /**< Class of the event */
  void *message;           /**< Pointer to the message */
} microstack_msg_t;

typedef offload_instance_type_t offload_mgr_client_type_t;

/*===========================================================================
STRUCTURE      offload_mgr_client_fn_t
===========================================================================*/
/**
 * @brief client function pointers to be used by offload mgr. 
 * @details currently the clients are socket mgr and session mgr. 
 */
typedef struct offload_mgr_client_fn
{
  void (*offload_req_to_profile)(void *, void *);                     /**< Function pointer to handle offload request */
  void (*unoffload_req_to_profile)(void *data, void *);               /**< Function pointer to handle unoffload request */
  void (*offload_req_to_microapp)(void *data);                        /** Function pointer to handle offload request to microapp */
  void (*unoffload_req_to_microapp)(void *data);                      /**< Function pointer to handle unoffload request to microapp */
  void (*store_offload_data)(void *data, void *);                     /**< Function pointer to store offload data */
  void (*clear_offload_data)(void *data);                             /**< Function pointer to erase offload data */
  void (*send_offload_resp_to_hal)(void *data, int status);           /** < Function pointer to send offload response to hal */
  void (*send_unoffload_resp_to_hal)(void *data, int status);         /** < Function pointer to send unoffload response to hal */
  void (*offload_to_microapp_success_cb)(void *data, int status);     /** <called when the offload to microapps is completed */
  void (*unoffload_from_microapp_success_cb)(void *data, int status); /** <called when the unoffload from microapps is completed */
} offload_mgr_client_fn_t;

/*===========================================================================
                        GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
FUNCTION        microstack_register_app_callback
===========================================================================*/
/**
  @brief Registers a microstack callback.
  @param[in]    handle    Handle to the application registered with microstack. 
  @param[in]    appCb     Callback function registered by application. 
  @return         true if registration is successful, false otherwise.
  @sideeffects  None.
*/
bool microstack_register_app_callback(BtAppHandle handle, BtHostAppCallback app_cb);

/*===========================================================================
FUNCTION        microstack_deregister_app_callback
===========================================================================*/
/**
  @brief Registers a microstack callback.
  @param[in]    handle    Handle to the application registered with microstack. 
  @return         true if registration is successful, false otherwise.
  @sideeffects  None.
*/
bool microstack_deregister_app_callback(BtAppHandle handle);

/*===========================================================================
FUNCTION        offload_mgr_client_interface_init
===========================================================================*/
/**
 * @brief inits all the clients to offload manager. 
 * 
 * @param mode could be one of BTHOST_CONFIG_OFFLOAD_MODE_MASK OR 
 *             MICROSTACK_CONFIG_PASSTHROUGH_MODE_MASK
 */
void offload_mgr_client_interface_init(bool mode);


/*===========================================================================
FUNCTION        offload_mgr_client_interface_deinit
===========================================================================*/
/**
 * @brief deinits all the clients to offload manager. 
 * 
 * @param mode could be one of BTHOST_CONFIG_OFFLOAD_MODE_MASK OR 
 *             MICROSTACK_CONFIG_PASSTHROUGH_MODE_MASK
 */
void offload_mgr_client_interface_deinit(bool mode);


/*===========================================================================
FUNCTION      offload_mgr_microstack_msg_handler
===========================================================================*/
/**
 * @brief Handles microstack messages in the profile manager.
 *
 * @param[in]    handle       Handle to the Bluetooth application.
 * @param[in]    eventClass   Class of the event.
 * @param[in]    message      Pointer to the message data.
 * @return       void.
 * @sideeffects  None.
 */
void offload_mgr_microstack_msg_handler(BtAppHandle handle, BtEventClass eventClass, void *message);

/*===========================================================================
FUNCTION      offload_mgr_client_register
===========================================================================*/
/**
 * @brief Registers a client with the offload manager.
 *
 * @param[in]    client_type       Type of the client to be registered.
 * @param[in]    offload_mgr_client_fns  Pointer to the client function structure.
 * @return       void.
 * @sideeffects  None.
 */
void offload_mgr_client_register(offload_mgr_client_type_t client_type, offload_mgr_client_fn_t *offload_mgr_client_fns);

/*===========================================================================
FUNCTION      offload_mgr_client_deregister
===========================================================================*/
/**
 * @brief Deregisters a client from the offload manager.
 *
 * @param[in]    client_type       Type of the client to be deregistered.
 * @return       void.
 * @sideeffects  None.
 */
void offload_mgr_client_deregister(offload_mgr_client_type_t client_type);

/*===========================================================================
FUNCTION      offload_mgr_get_client_fns_by_offload_type
===========================================================================*/
/**
 * @brief Retrieves the client function structure based on the offload type.
 *
 * @param[in]    offload_type      Type of offload for which the client functions are required.
 * @return       Pointer to the client function structure, or NULL if not found.
 * @sideeffects  None.
 */
offload_mgr_client_fn_t *offload_mgr_get_client_fns_by_offload_type(offload_instance_type_t offload_type);




#endif