/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_sm.c
===========================================================================*/
/**
 * @file offload_mgr_sm.c
 * @brief Handles socket open and close procedures.
 *
 * @details This file primarily handles : 
 * 1. offload with microstack and microapp
 * 2. unoffload with microstack and microapp
 * 3. since both of these operations are common for socket and session, hence are handled in this file. 
 *
 * Below is the plantuml for the statetransitions: 
 * 
 * @startuml
 * 
 * [*] --> Idle
 * 
 * Idle --> OffloadReq : OFFLOAD_REQ
 * 
 * OffloadReq --> OffloadToMicrostackSuccess : OFFLOAD_TO_MICROSTACK_SUCCESS
 * OffloadReq --> OffloadFailure : OFFLOAD_TO_MICROSTACK_FAILURE
 * OffloadReq --> UnOffloadReqToMicrostack : UNOFFLOAD_REQ
 * 
 * OffloadToMicroappSuccess --> UnOffloadReqFromHAL : UNOFFLOAD_REQ
 * OffloadToMicroappSuccess --> UnOffloadReqToMicrostack : UNOFFLOAD_FROM_MICROAPP_SUCCESS
 * 
 * OffloadToMicroappFailure --> OffloadFailure : UNOFFLOAD_FROM_MICROSTACK_SUCCESS
 * 
 * UnOffloadReqToMicrostack --> UnoffloadSuccess : UNOFFLOAD_FROM_MICROSTACK_SUCCESS
 * UnOffloadReqFromHAL --> UnoffloadSuccess : UNOFFLOAD_FROM_MICROSTACK_SUCCESS
 * 
 * OffloadToMicrostackSuccess --> UnOffloadReqToMicrostack : UNOFFLOAD_REQ
 * OffloadToMicrostackSuccess --> OffloadToMicroappSuccess : OFFLOAD_TO_MICROAPP_SUCCESS
 * OffloadToMicrostackSuccess --> OffloadToMicroappFailure : OFFLOAD_TO_MICROAPP_FAILURE
 * 
 * UnoffloadSuccess --> Idle : OFFLOAD_INSTANCE_FREE
 * OffloadFailure --> Idle : OFFLOAD_INSTANCE_FREE
 * 
 * @enduml
 */

/** @note  only for testing */
// #define MOCK_APP_MANAGER
#define SAFE_CALL(func_ptr, ...) \
    do { \
        if (func_ptr != NULL) { \
            func_ptr(__VA_ARGS__); \
        } \
    } while (0)


/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "sm.h"
#include "offload_mgr_internal.h"
#include <string.h>
#include "offload_mgr_transport_shim.h"
#include "offload_mgr_sm.h"
#include "offload_mgr_client_interface.h"

/*===========================================================================
                        EXTERN VARIABLES
===========================================================================*/
extern offload_instance_t offload_instances[MAX_OFFLOAD_INSTANCES];

/*===========================================================================
TABLE      offload_mgr_state_table
===========================================================================*/
/**
@brief State transition table for the offload manager.
@details This table defines the state transitions for the offload manager based on various events.

@table
| Current State                  | Event                              | Next State                 |
|-------------------------------|------------------------------------|-----------------------------|
| Idle                           | OFFLOAD_REQ                        | OffloadReq                 |
| OffloadReq                     | OFFLOAD_TO_MICROSTACK_SUCCESS      | OffloadToMicrostackSuccess |
| OffloadReq                     | OFFLOAD_TO_MICROSTACK_FAILURE      | OffloadFailure             |
| OffloadReq                     | UNOFFLOAD_REQ                      | UnOffloadReqToMicrostack   |
| OffloadToMicroappSuccess       | UNOFFLOAD_REQ                      | UnOffloadReqFromHAL        |
| OffloadToMicroappSuccess       | UNOFFLOAD_FROM_MICROAPP_SUCCESS    | UnOffloadReqToMicrostack   |
| OffloadToMicroappFailure       | UNOFFLOAD_FROM_MICROSTACK_SUCCESS  | OffloadFailure             |
| UnOffloadReqToMicrostack       | UNOFFLOAD_FROM_MICROSTACK_SUCCESS  | UnoffloadSuccess           |
| UnOffloadReqFromHAL            | UNOFFLOAD_FROM_MICROSTACK_SUCCESS  | UnoffloadSuccess           |
| OffloadToMicrostackSuccess     | UNOFFLOAD_REQ                      | UnOffloadReqToMicrostack   |
| OffloadToMicrostackSuccess     | OFFLOAD_TO_MICROAPP_SUCCESS        | OffloadToMicroappSuccess   |
| OffloadToMicrostackSuccess     | OFFLOAD_TO_MICROAPP_FAILURE        | OffloadToMicroappFailure   |
| UnoffloadSuccess               | OFFLOAD_INSTANCE_FREE              | Idle                       |
| OffloadFailure                 | OFFLOAD_INSTANCE_FREE              | Idle                       |
@endtable
*/
SM_StateTransition offload_mgr_state_table[] =
    {
        /* Current state                Event                            Next State            */
        /* Idle state */
        {Idle, OFFLOAD_REQ, OffloadReq},

        /* OffloadReq state */
        {OffloadReq, OFFLOAD_TO_MICROSTACK_SUCCESS, OffloadToMicrostackSuccess},
        {OffloadReq, OFFLOAD_TO_MICROSTACK_FAILURE, OffloadFailure},
        {OffloadReq, UNOFFLOAD_REQ, UnOffloadReqToMicrostack},

        {OffloadToMicroappSuccess, UNOFFLOAD_REQ, UnOffloadReqFromHAL},
        {UnOffloadReqFromHAL, UNOFFLOAD_FROM_MICROAPP_SUCCESS, UnOffloadReqToMicrostack},
        {OffloadToMicroappSuccess, UNOFFLOAD_FROM_MICROAPP_SUCCESS, UnOffloadReqToMicrostack}, /* can occur in case microapp goes bad */

        /* OffloadToMicroappFailure state */
        {OffloadToMicroappFailure, UNOFFLOAD_FROM_MICROSTACK_SUCCESS, OffloadFailure},

        /* UnOffloadReqToMicrostack state */
        {UnOffloadReqToMicrostack, UNOFFLOAD_FROM_MICROSTACK_SUCCESS, UnoffloadSuccess},

        /* OffloadToMicrostackSuccess */
        {OffloadToMicrostackSuccess, UNOFFLOAD_REQ, UnOffloadReqToMicrostack},
        {OffloadToMicrostackSuccess, OFFLOAD_TO_MICROAPP_SUCCESS, OffloadToMicroappSuccess},
        {OffloadToMicrostackSuccess, OFFLOAD_TO_MICROAPP_FAILURE, OffloadToMicroappFailure},

        /* Final states */
        {UnoffloadSuccess, OFFLOAD_INSTANCE_FREE, Idle},
        {OffloadFailure, OFFLOAD_INSTANCE_FREE, Idle},
};

/**
 * @brief Global array representing the state context for offload instances.
 *
 * @details
 * 1. Each element of this array represents a unique socket and its current state.
 * 2. For uniformity, each element from this array is one-to-one mapped with the element from the base sockets array in socket.c.
 * 3. This cascading is done to avoid sharing unnecessary information with other modules.
 */
static offload_mgr_sm_state_ctx_t offload_mgr_sm_state_ctx[MAX_OFFLOAD_INSTANCES];

/*===========================================================================
                        STATIC FUNCTION DECLARATIONS
===========================================================================*/
static void offload_mgr_sm_state_entry_function(SM_StateId state, void *buf);
static void offload_mgr_sm_state_error_function(uint8_t evt, void *buf);

/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
FUNCTION      offload_mgr_sm_find_idle_offload_instance
===========================================================================*/
/**
 * @brief Find an idle offload_instance.
 * @details This static method searches for an idle offload_instance.
 * @return Pointer to the offload_instance_t structure if an idle offload_instance is found, otherwise NULL.
 */
offload_instance_t *offload_mgr_sm_find_idle_offload_instance(void)
{
    for (uint8_t i = 0; i < MAX_OFFLOAD_INSTANCES; i++)
    {
        if (offload_instances[i].offload_state == Idle)
        {
            return &offload_instances[i];
        }
    }
    return NULL;
}

/*===========================================================================
FUNCTION      offload_mgr_instance_sm_init
===========================================================================*/
/**
 * @brief Initialize the state machine for the given offload_instance state context.
 * @details This function initializes the state machine for the specified offload_instance,
 *           setting up the necessary context for state management.
 * @param[in] offload_instance Pointer to the offload_instance_t structure representing the offload_instance state context to initialize.
 */
void offload_mgr_instance_sm_init(offload_instance_t *offload_instance)
{
    SM_StateContext *sm_state_ctx;

    sm_state_ctx = &(offload_instance->state_ctx->sm_state_ctx);
    sm_state_ctx->state_tbl = offload_mgr_state_table;
    sm_state_ctx->state_transition_cnt = sizeof(offload_mgr_state_table) / sizeof(SM_StateTransition);
    sm_state_ctx->entry_func2 = offload_mgr_sm_state_entry_function;
    sm_state_ctx->err_func2 = offload_mgr_sm_state_error_function;

    sm_set_current_state(sm_state_ctx, Idle, offload_instance);
}

/*===========================================================================
FUNCTION      offload_mgr_sm_init
===========================================================================*/
/**
 * @brief Initialize the state machine.
 * @details This function initializes the state machine, setting up the necessary context for state management.
 */
void offload_mgr_sm_init(void)
{
    memset(offload_instances, 0, sizeof(offload_instances));
    memset(offload_mgr_sm_state_ctx, 0, sizeof(offload_mgr_sm_state_ctx));
    /**
     * 1. offload_mgr_sm_ctx and offload_instances are one to one mapped
     * 2. this is done to avoid mapping issues.
     * 3. check if can be done in better way..
     */
    for (uint8_t i = 0; i < MAX_OFFLOAD_INSTANCES; i++)
    {
        offload_instances[i].state_ctx = &offload_mgr_sm_state_ctx[i];
        offload_instances[i].offload_instance_id = i + 1;

        /* init the offload_instance state machine */
        offload_mgr_instance_sm_init(&offload_instances[i]);
    }
}

/*===========================================================================
FUNCTION      offload_mgr_sm_init
===========================================================================*/
/**
 * @brief Initialize the state machine.
 * @details This function initializes the state machine, setting up the necessary context for state management.
 */
void offload_mgr_sm_deinit(void)
{
    memset(offload_instances, 0, sizeof(offload_instances));
    memset(offload_mgr_sm_state_ctx, 0, sizeof(offload_mgr_sm_state_ctx));
    
    for (uint8_t i = 0; i < MAX_OFFLOAD_INSTANCES; i++)
    {
        offload_instances[i].state_ctx = &offload_mgr_sm_state_ctx[i];
        offload_instances[i].offload_instance_id = i + 1;

        /* init the offload_instance state machine */
        offload_mgr_instance_sm_init(&offload_instances[i]);
    }
}


/*===========================================================================
FUNCTION      offload_mgr_sm_handle_offload_evt
===========================================================================*/
/**
 * @brief Handle offload event.
 * @details This function handles the offload event for the given offload_instance.
 * @param[in] offload Pointer to the offload_instance_t structure.
 */
void offload_mgr_sm_handle_offload_evt(offload_instance_t *offload) {
    SM_StateContext *sm_state_ctx = NULL;

    if(offload == NULL) {
        OFFLOAD_MGR_FATAL("offload_mgr_sm_handle_offload_evt : offload is NULL\n");
        return;
    }
    /* start state machine*/
    sm_state_ctx = &(offload->state_ctx->sm_state_ctx);
    sm_process_evt_with_cb(sm_state_ctx, OFFLOAD_REQ, offload);
}

/*===========================================================================
FUNCTION      offload_mgr_sm_handle_unoffload_evt
===========================================================================*/
/**
 * @brief Handle unoffload event.
 * @details This function handles the unoffload event for the given offload_instance.
 * @param[in] offload Pointer to the offload_instance_t structure.
 */
void offload_mgr_sm_handle_unoffload_evt(offload_instance_t *offload) {
    SM_StateContext *sm_state_ctx = NULL;

    if(offload == NULL) {
        OFFLOAD_MGR_FATAL("offload_mgr_sm_handle_unoffload_evt : offload is NULL\n");
        return;
    }
    /* posto to state machine */
    sm_state_ctx = &(offload->state_ctx->sm_state_ctx);
    sm_process_evt_with_cb(sm_state_ctx, UNOFFLOAD_REQ, offload);
}

/*===========================================================================
FUNCTION      offload_mgr_sm_handle_uapp_offload_evt
===========================================================================*/
/**
 * @brief Handle uapp offload event.
 * @details This function handles the uapp offload event for the given offload_instance.
 * @param[in] offload Pointer to the offload_instance_t structure.
 * @param[in] status Status of the offload operation.
 */
void offload_mgr_sm_handle_uapp_offload_evt(offload_instance_t *offload, int status) {
    
    SM_StateContext *state_ctx = NULL;

    if(offload == NULL) {
        OFFLOAD_MGR_FATAL("offload_mgr_sm_handle_uapp_offload_evt : offload is NULL\n");
    }

    state_ctx = &(offload->state_ctx->sm_state_ctx);
    if (status == 0)
    {
        sm_process_evt_with_cb(state_ctx, OFFLOAD_TO_MICROAPP_SUCCESS, offload);
    }
    else
    {
        sm_process_evt_with_cb(state_ctx, OFFLOAD_TO_MICROAPP_FAILURE, offload);
    }
}

/*===========================================================================
FUNCTION      offload_mgr_sm_handle_uapp_unoffload_evt
===========================================================================*/
/**
 * @brief Handle uapp unoffload event.
 * @details This function handles the uapp unoffload event for the given offload_instance.
 * @param[in] offload Pointer to the offload_instance_t structure.
 * @param[in] status Status of the unoffload operation.
 */
void offload_mgr_sm_handle_uapp_unoffload_evt(offload_instance_t *offload, int status) {
    
    SM_StateContext *state_ctx = NULL;

    if(offload == NULL) {
        OFFLOAD_MGR_FATAL("offload_mgr_sm_handle_uapp_unoffload_evt : offload is NULL\n");
    }

    state_ctx = &(offload->state_ctx->sm_state_ctx);
    if (status == 0)
    {
        sm_process_evt_with_cb(state_ctx, UNOFFLOAD_FROM_MICROAPP_SUCCESS, offload);
    }
    else
    {
        // sm_process_evt_with_cb(state_ctx, OFFLOAD_TO_MICROAPP_FAILURE, offload);
        /* not implemented */
        OFFLOAD_MGR_FATAL("offload_mgr_sm_handle_uapp_unoffload_evt : not implemented\n");
    }
}


/*===========================================================================
FUNCTION      offload_mgr_sm_state_entry_function
===========================================================================*/
/**
 * @brief State machine state entry function.
 * @details This static function handles the entry actions
 *          for a given state in the socket manager state machine.
 * @param[in] state The state ID of the state being entered.
 * @param[in] buf Pointer to the buffer containing state-specific data.
 */
static void offload_mgr_sm_state_entry_function(SM_StateId state, void *buf)
{
    SM_StateContext *state_ctx = NULL;
    offload_instance_t *offload = buf;
    OFFLOAD_MGR_ASSERT_FATAL(offload != NULL);

    offload_mgr_client_fn_t *client_fns = offload_mgr_get_client_fns_by_offload_type(offload->offload_instance_type);

    OFFLOAD_MGR_ASSERT_FATAL(client_fns != NULL);

    /* update the offload state */
    offload->offload_state = state;
    state_ctx = &(offload->state_ctx->sm_state_ctx);
    OFFLOAD_MGR_LOGM("offload_mgr_sm_state_entry_function state : %d", state);

    switch (state)
    {
    case Idle:
    {
        SAFE_CALL(client_fns->clear_offload_data, offload);
        /**
         * @note Cleaning of offload involves setting its state to idle.
         * Nothing to do here since the state is already set to idle
         * at the start
         */
    }
    break;
    case OffloadReq:
    {
        SAFE_CALL(client_fns->store_offload_data, offload, offload->state_ctx->data);
        SAFE_CALL(client_fns->offload_req_to_profile, offload, offload->state_ctx->data);
    }
    break;
    case OffloadToMicrostackSuccess:
    {
#ifdef MOCK_APP_MANAGER
        sm_process_evt_with_cb(state_ctx, OFFLOAD_TO_MICROAPP_SUCCESS, offload);
#else
        SAFE_CALL(client_fns->offload_req_to_microapp, offload);
#endif
    }
    break;
    case OffloadFailure:
    {
        /**
         * 1. send the socket open failure to hal
         * 2. clean the socket and make it idle
         */

        /** 1 */
        if(client_fns->send_offload_resp_to_hal)
        {
            int status = 1; /* todo : failure code */
            SAFE_CALL(client_fns->send_offload_resp_to_hal, offload, status);
        }

        /** 2 */
        sm_process_evt_with_cb(state_ctx, OFFLOAD_INSTANCE_FREE, offload);
    }
    break;
    case UnOffloadReqToMicrostack:
    {
        /* send socket close to microstack */
        SAFE_CALL(client_fns->unoffload_req_to_profile, offload, NULL);
    }
    break;
    case OffloadToMicroappSuccess:
    {
        /* send offload response to apss */
	    SAFE_CALL(client_fns->offload_to_microapp_success_cb, offload, 0);
        
	    /* send response to HAL */
        SAFE_CALL(client_fns->send_offload_resp_to_hal, offload, 0);
    }
    break;
    case OffloadToMicroappFailure:
    {
        /* send unoffload to microstack */
        SAFE_CALL(client_fns->unoffload_req_to_profile, offload, NULL);
    }
    break;
    case UnoffloadSuccess:
    {
        /* 1. send unoffload response to hal */
        SAFE_CALL(client_fns->send_unoffload_resp_to_hal, offload, 0);

        /* 2. clean the instance */
        sm_process_evt_with_cb(state_ctx, OFFLOAD_INSTANCE_FREE, offload);
    }
    break;
    case UnOffloadReqFromHAL:
    {
        /* send unoffload req to microapp */
        SAFE_CALL(client_fns->unoffload_req_to_microapp, offload);
    }
    break;
    default:
        break;
    }
}

/*===========================================================================
FUNCTION      offload_mgr_sm_state_error_function
===========================================================================*/
/**
 * @brief State machine state error function.
 * @details This static function handles error events in the socket manager
 *          state machine.
 * @param[in] evt The event ID that triggered the error.
 * @param[in] buf Pointer to the buffer containing event-specific data.
 */
static void offload_mgr_sm_state_error_function(uint8_t evt, void *buf)
{
    offload_instance_t *offload = buf;
    SM_StateContext *state_ctx = &(offload->state_ctx->sm_state_ctx);
    OFFLOAD_MGR_LOGE("offload_mgr_sm_state_error_function state : %d event : %d",
                       sm_get_current_state(state_ctx), evt);
}

/*===========================================================================
FUNCTION      offload_mgr_sm_handle_microstack_offload_evt
===========================================================================*/
/**
 * @brief Handle microstack offload event.
 * @details This function handles the microstack offload event for the given offload_instance.
 * @param[in] offload Pointer to the offload_instance_t structure.
 * @param[in] status Status of the offload operation.
 */
void offload_mgr_sm_handle_microstack_offload_evt(offload_instance_t *offload, int status) {
    SM_StateContext *state_ctx = &(offload->state_ctx->sm_state_ctx);

    OFFLOAD_MGR_ASSERT_FATAL(offload != NULL);
    if (status == 0) {
        sm_process_evt_with_cb(state_ctx, OFFLOAD_TO_MICROSTACK_SUCCESS, offload);
    }
    else {
        sm_process_evt_with_cb(state_ctx, OFFLOAD_TO_MICROSTACK_FAILURE, offload);
    }
}

/*===========================================================================
FUNCTION      offload_mgr_sm_handle_microstack_unoffload_evt
===========================================================================*/
/**
 * @brief Handle microstack unoffload event.
 * @details This function handles the microstack unoffload event for the given offload_instance.
 * @param[in] offload Pointer to the offload_instance_t structure.
 * @param[in] status Status of the unoffload operation.
 */
void offload_mgr_sm_handle_microstack_unoffload_evt(offload_instance_t *offload, int status) {
    SM_StateContext *state_ctx = &(offload->state_ctx->sm_state_ctx);

    OFFLOAD_MGR_ASSERT_FATAL(offload != NULL);
    if (status == 0) {
        sm_process_evt_with_cb(state_ctx, UNOFFLOAD_FROM_MICROSTACK_SUCCESS, offload);
    }
    else {
        /* currently don't have event */
        /* tbd : do we need failure here ? */
    }
}

