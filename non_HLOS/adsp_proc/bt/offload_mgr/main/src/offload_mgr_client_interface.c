/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_client_interface.c
===========================================================================*/
/**
 * @file offload_mgr_client_interface.c
 * @brief Offload manager client interface file, to init and interface all clients
 *        with offload manager.
 *
 * @details 1. Socket manager, profile manager, microstack and endpoint manager
 *          are clients to offload manager.
 */

/*===========================================================================
                            INCLUDE FILES
===========================================================================*/
#include "bmm_lib.h"
#include "bt_main.h"
#include "socket_mgr.h"
#include "endpt_mgr.h"
#include "offload_mgr_client_interface.h"
#include "offload_mgr_transport_shim.h"
#include "offload_mgr_sm.h"
#include "offload_instance.h"
#include "profile_mgr_gatt.h"
#include "bt_utils.h"

/**
 * @brief max number of callbacks that could be held by app_callbacks array
 */
#define MICROSTACK_APP_MAX_CBS 6
/*===========================================================================
                            TYPE DEFINITIONS
===========================================================================*/

/**
 * @brief structure to hold the mapping of callback and handle of the application.
 */
typedef struct
{
    BtAppHandle handle;
    BtHostAppCallback app_cb;
} microstack_app_callback_t;

offload_mgr_client_fn_t offload_mgr_clients[2];
/*===========================================================================
                            GLOBAL VARIABLES DECLARATIONS
===========================================================================*/
/**
 * @var app_callbacks
 * @brief Maintains the app_handle to callback mappings.
 *
 * @details This variable is used to store the context for the app callbacks registered with microstack,
 *          including the mappings between application handles and their corresponding callbacks.
 */
static microstack_app_callback_t app_callbacks[MICROSTACK_APP_MAX_CBS];

/*===========================================================================
FUNCTION      microstack_cb
===========================================================================*/
/**
 * @brief Callback to handle Microstack events.
 *
 * @param handle The handle for the Bluetooth application.
 * @param eventClass The class of the event.
 * @param message Pointer to the message.
 *
 * @details This function allocates memory for a Microstack message, populates it with the
 *          provided handle, event class, and message, and posts the event to the offload manager.
 */
static void microstack_cb(BtAppHandle handle, BtEventClass eventClass, void *message)
{
    microstack_msg_t *microstack_msg = bt_pal_malloc(sizeof(microstack_msg_t));
    if (microstack_msg == NULL)
    {
        BT_PAL_ASSERT_FATAL("microstack_cb: Failed to process microstack msg hndl: %x, evtclass: %x, msg: %x", handle, eventClass, message);
        return;
    }
    microstack_msg->handle = handle;
    microstack_msg->eventClass = eventClass;
    microstack_msg->message = message;

    Offload_Mgr_Post_Microstack_Evt((uint8_t *)microstack_msg, sizeof(microstack_msg_t));
}

/*===========================================================================
FUNCTION      map_handle_to_callback
===========================================================================*/
/**
 * @brief map handle to callback of the application.
 *
 * @param handle handle provided by the application
 * @param app_cb callback to be triggered on events on the handle.
 */
static void map_handle_to_callback(BtAppHandle handle, BtHostAppCallback app_cb)
{
    for (uint8_t i = 0; i < MICROSTACK_APP_MAX_CBS; i++)
    {
        if (app_callbacks[i].handle == 0)
        {
            app_callbacks[i].app_cb = app_cb;
            app_callbacks[i].handle = handle;
            return;
        }
    }
    /* this means the app callbacks array is exausted, increase the size of the array */
    OFFLOAD_MGR_ASSERT_FATAL(false);
}

/*===========================================================================
FUNCTION      remove_handle
===========================================================================*/
/**
 * @brief remove handle from list. 
 *
 * @param handle handle provided by the application
 */
static void remove_handle(BtAppHandle handle)
{
    for (uint8_t i = 0; i < MICROSTACK_APP_MAX_CBS; i++)
    {
        if (app_callbacks[i].handle == handle)
        {
            memset(app_callbacks + i, 0, sizeof(microstack_app_callback_t));
            return;
        }
    }
    /* this means the app callbacks array is exausted, increase the size of the array */
    OFFLOAD_MGR_ASSERT_FATAL(false);
}

/*===========================================================================
FUNCTION      get_callback_from_handle
===========================================================================*/
/**
 * @brief Get the callback from handle object
 *
 * @param handle handle of the application
 * @return BtHostAppCallback callback registered by the application.
 */
static BtHostAppCallback get_callback_from_handle(BtAppHandle handle)
{
    for (uint8_t i = 0; i < MICROSTACK_APP_MAX_CBS; i++)
    {
        if (app_callbacks[i].handle == handle)
        {
            return app_callbacks[i].app_cb;
        }
    }
    return NULL;
}

bool microstack_register_app_callback(BtAppHandle handle, BtHostAppCallback app_cb)
{
    /* check if the handle is already registered */
    if (get_callback_from_handle(handle) != NULL)
    {
        return false;
    }
    map_handle_to_callback(handle, app_cb);
    return MicroStackRegisterAppCb(handle, microstack_cb);
}

bool microstack_deregister_app_callback(BtAppHandle handle)
{
    /* check if the handle is already registered */
    if (get_callback_from_handle(handle) == NULL)
    {
        return false;
    }
    remove_handle(handle);
    
    MicroStackDeregisterAppCb(handle);
    return true;
}

void offload_mgr_client_interface_init(bool mode)
{
    memset(app_callbacks, 0, sizeof(app_callbacks));
    if (mode == BTHOST_CONFIG_OFFLOAD_MODE_MASK)
    {
        socket_mgr_init();
        gatt_session_mgr_init();
        endpt_mgr_init();
        bt_utils_init();
    }
}

void offload_mgr_client_interface_deinit(bool mode)
{
    if (mode == BTHOST_CONFIG_OFFLOAD_MODE_MASK)
    {
        socket_mgr_deinit();
        gatt_session_mgr_deinit();
        endpt_mgr_deinit();
        bt_utils_deinit();
    }
    memset(app_callbacks, 0, sizeof(app_callbacks));
}

void offload_mgr_microstack_msg_handler(BtAppHandle handle, BtEventClass eventClass, void *message)
{
    CsrPrim *type = message;
    /* call the app callback */
    BtHostAppCallback app_cb = get_callback_from_handle(handle);
    if (app_cb != NULL)
    {
        app_cb(handle, eventClass, message);
        MicroStackFreePrimitive(eventClass, message);

    }
    else
    {
        /* this is unexpected */
        OFFLOAD_MGR_ASSERT_FATAL(false);
    }
}


void offload_mgr_client_register(offload_mgr_client_type_t client_type, offload_mgr_client_fn_t *offload_mgr_client_fns) {
    if(offload_mgr_client_fns != NULL)
        offload_mgr_clients[client_type] = *offload_mgr_client_fns;
    else {
        OFFLOAD_MGR_LOGE("offload_mgr_client_register : offload_mgr_client_fns are NULL");
    }
}

void offload_mgr_client_deregister(offload_mgr_client_type_t client_type) {
    memset(&offload_mgr_clients[client_type], 0, sizeof(offload_mgr_client_fn_t));
}

offload_mgr_client_fn_t* offload_mgr_get_client_fns_by_offload_type(offload_instance_type_t offload_type)
{
    return &offload_mgr_clients[offload_type];
}
