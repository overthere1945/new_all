/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      sm.c
===========================================================================*/
/**
 * @file sm.c
 * @brief State Machine Driver file.
 *
 * @details This file contains the implementation of the state machine driver.
 *
 * @note
 * 1. States and events are supported only up to 255.
 */

/*===========================================================================
                   INCLUDE FILES
===========================================================================*/
#include "sm.h"

/*===========================================================================
                    FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
FUNCTION      sm_event_based_state_transition
===========================================================================*/
/**

  function to init the memory and data structures required for Lpai Transport

  @param[in]    state_ctx   state_ctx of the state machine to be driven.
  @param[in]    evt         evt posted on state machine.

  @return       true if successful,
                false otherwise

  @sideeffects  None.
*/
/*=========================================================================*/
bool sm_event_based_state_transition(SM_StateContext *state_ctx, SM_EventId evt);

/*===========================================================================
FUNCTION      sm_execute_state_entry_function
===========================================================================*/
/**
  @brief triggers the entry function of the corresponding state.

  @param[in]    state_ctx       state_ctx of the state machine to be driven.

  @return       void

  @sideeffects  None.
*/
/*=========================================================================*/
void sm_execute_state_entry_function(SM_StateContext *state_ctx);

/*===========================================================================
FUNCTION      sm_execute_event_err_function
===========================================================================*/
/**
  @brief Entry function to be triggered on error in particular state.

  @param[in]    state_ctx       The state machine context.
  @param[in]    event           The event to be processed.

  @return       void.

  @sideeffects  None.
*/
/*=========================================================================*/
void sm_execute_event_err_function(SM_StateContext *state_ctx, SM_EventId event);

bool sm_event_based_state_transition(SM_StateContext *state_ctx, SM_EventId evt)
{
    const SM_StateTransition *sm_tbl;
    uint8_t ind;
    bool entry_found = false;

    /* Perform parameter validation */
    if ((state_ctx == NULL) ||
        (state_ctx->state_tbl == NULL) ||
        (state_ctx->state_transition_cnt == 0))
    {
        return false;
    }

    sm_tbl = state_ctx->state_tbl;
    for (ind = 0; ind < state_ctx->state_transition_cnt; ind++)
    {
        /* Find matching state-event pair */
        if ((sm_tbl[ind].curr_state == state_ctx->curr_state) && (sm_tbl[ind].evt == evt))
        {
            entry_found = true;
            break;
        }

        if (sm_tbl[ind].curr_state == SMAnyState &&
            (sm_tbl[ind].evt == evt))
        {
            state_ctx->prev_state = state_ctx->curr_state;
            state_ctx->curr_state = SMAnyState;
            entry_found = true;
            break;
        }
    }
    if (entry_found)
    {
        if (state_ctx->curr_state != SMAnyState)
        {
            state_ctx->prev_state = state_ctx->curr_state;
        }
        state_ctx->curr_state = sm_tbl[ind].next_state;
    }
    return entry_found;
}

bool sm_process_event(SM_StateContext *ctx, SM_EventId event)
{
    bool result;
    result = sm_event_based_state_transition(ctx, event);
    if (result)
    {
        sm_execute_state_entry_function(ctx);
    }
    else
    {
        sm_execute_event_err_function(ctx, event);
    }
    return result;
}

void sm_execute_event_err_function(SM_StateContext *state_ctx, SM_EventId event)
{
    /* Perform parameter validation */
    if ((state_ctx == NULL) ||
        (state_ctx->state_tbl == NULL) ||
        (state_ctx->state_transition_cnt == 0))
    {
        return;
    }

    if ((state_ctx->err_func != NULL))
    {
        state_ctx->err_func(event);
    }
}

/**
    Execute_state_entry_function
*/
void sm_execute_state_entry_function(SM_StateContext *state_ctx)
{
    /* Perform parameter validation */
    if ((state_ctx == NULL) ||
        (state_ctx->state_tbl == NULL) ||
        (state_ctx->state_transition_cnt == 0))
    {
        return;
    }

    if ((state_ctx->entry_func != NULL))
    {
        state_ctx->entry_func(state_ctx->curr_state);
    }
}
/*===========================================================================
FUNCTION      sm_execute_event_err_function
===========================================================================*/
/**
  @brief Entry function to be triggered on error in particular state.

  @param[in]    state_ctx       The state machine context.
  @param[in]    event           The event to be processed.

  @return       void.

  @sideeffects  None.
*/
/*=========================================================================*/
void sm_execute_event_err_function_with_cb(SM_StateContext *state_ctx, SM_EventId event, void *routedMsg)
{
    /* Perform parameter validation */
    if ((state_ctx == NULL) ||
        (state_ctx->state_tbl == NULL) ||
        (state_ctx->state_transition_cnt == 0))
    {
        return;
    }

    if ((state_ctx->err_func2 != NULL))
    {
        state_ctx->err_func2(event, routedMsg);
    }
}

/**
    Execute_state_entry_function
*/
void sm_execute_state_entry_function_with_cb(SM_StateContext *state_ctx, void *routedMsg)
{
    /* Perform parameter validation */
    if ((state_ctx == NULL) ||
        (state_ctx->state_tbl == NULL) ||
        (state_ctx->state_transition_cnt == 0))
    {
        return;
    }

    if ((state_ctx->entry_func2 != NULL))
    {
        state_ctx->entry_func2(state_ctx->curr_state, routedMsg);
    }
}

/**
    Internal SSBT states must only be changed from
    SS_BT_Process_Event() function
*/
bool sm_process_evt_with_cb(SM_StateContext *ctx, SM_EventId event, void *routedMsg)
{
    bool result;
    result = sm_event_based_state_transition(ctx, event);
    if (result)
    {
        sm_execute_state_entry_function_with_cb(ctx, routedMsg);
    }
    else
    {
        sm_execute_event_err_function_with_cb(ctx, event, routedMsg);
    }
    return result;
}

void sm_set_current_state(SM_StateContext *thisctx, SM_StateId state, void *routedMsg)
{
    thisctx->curr_state = state;
    sm_execute_state_entry_function(thisctx);
    sm_execute_state_entry_function_with_cb(thisctx, routedMsg);
}

SM_StateId sm_get_current_state(SM_StateContext *thisctx)
{
    return thisctx->curr_state;
}

SM_StateId sm_get_prev_state(SM_StateContext *thisctx)
{
    return thisctx->prev_state;
}
