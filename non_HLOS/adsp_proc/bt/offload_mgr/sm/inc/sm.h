/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      sm.h
===========================================================================*/
/**
 * @file sm.h
 * @brief State machine driver APIs.
 *
 * @details This file contains the API definitions for the state machine driver.
 *
 * @note 
 * 1. States and events are supported only up to 255.
 */

#ifndef SM_H
#define SM_H

/*============================================================================*
                                INCLUDE FILES
*============================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/*===========================================================================
                           TYPE DEFINITIONS
===========================================================================*/
typedef uint8_t SM_StateId;
typedef uint8_t SM_EventId;

enum {
    SMAnyState = 0
};

/*===========================================================================
FUNCTION      SMStateEntryFunction
===========================================================================*/
/**
  @brief The callback entry function to be triggered on state machine movement

  @param[in]    state           Current state of the machine

  @return       void. 

  @sideeffects  None.
*/
/*=========================================================================*/
typedef void ( *SMStateEntryFunction)(SM_StateId state);

/*===========================================================================
FUNCTION      SMStateErrorFunction
===========================================================================*/
/**
  @brief The callback error function to be triggered on state machine movement

  @param[in]    evt           The last event posted on machine. 

  @return       void. 

  @sideeffects  None.
*/
/*=========================================================================*/
typedef void ( *SMStateErrorFunction)(SM_EventId evt);

/*===========================================================================
FUNCTION      SMStateExitFunction
===========================================================================*/
/**
  @brief The callback exit function to be triggered on state machine movement
         on Exit from the current state

  @param[in]    void

  @return       void. 

  @sideeffects  None.
*/
/*=========================================================================*/
typedef void ( *SMStateExitFunction)(void);

/*===========================================================================
FUNCTION      SMStateEntryFunctionroutedMsg
===========================================================================*/
/**
  @brief The callback entry function to be triggered on state machine movement

  @param[in]    void

  @return       void. 

  @sideeffects  None.
*/
/*=========================================================================*/
typedef void ( *SMStateEntryFunctionroutedMsg)(SM_StateId state, void *routedMsg);
typedef void ( *SMStateErrorFunctionroutedMsg)(SM_EventId evt, void *routedMsg);

typedef struct
{
    uint8_t state_id;
    SMStateEntryFunction      entry_func;
} SM_State;

typedef struct
{
    SM_StateId   curr_state;
    SM_EventId     evt;
    SM_StateId   next_state;
} SM_StateTransition;

/** @brief Function pointer for handling SM events */
typedef void (*sm_routed_msg_cleanup_t)(void *routedMsg);

typedef struct
{
    SM_StateId      prev_state;
    SM_StateId      curr_state;
    uint8_t           state_cnt;
    uint8_t           state_transition_cnt;
    const SM_StateTransition * state_tbl;
    SMStateEntryFunction entry_func;
    SMStateErrorFunction err_func;
    SMStateErrorFunctionroutedMsg err_func2;
    SMStateEntryFunctionroutedMsg entry_func2;
}SM_StateContext;


/*===========================================================================
                    GLOBAL FUNCTION DEFINITIONS
===========================================================================*/


/*===========================================================================
FUNCTION      sm_process_evt
===========================================================================*/
/**
  @brief Post and process the event on the state machine. 

  @param[in]    ctx             The state machine context.
  @param[in]    event           The event to be processed.

  @return       true if the event is processed properly, false otherwise.

  @sideeffects  None.
*/
/*=========================================================================*/
bool sm_process_evt(SM_StateContext * ctx, SM_EventId event);

/*===========================================================================
FUNCTION      sm_process_evt_with_cb
===========================================================================*/
/**
  @brief Post and process the event on the state machine.

  @param[in]    ctx             The state machine context.
  @param[in]    event           The event to be processed.
  @param[in]    routedMsg       The routedMsg will be returned in the 
                                SMStateEntryFunctionroutedMsg

  @return       true if the event is processed properly, false otherwise.

  @sideeffects  None.
*/
/*=========================================================================*/
bool sm_process_evt_with_cb(SM_StateContext * ctx, SM_EventId event, void *routedMsg);

/*===========================================================================
FUNCTION      sm_set_current_state
===========================================================================*/
/**
  @brief Entry function to be triggered on 

  @param[in]    thisctx             The state machine context.
  @param[in]    state               The state to be set. 
  @param[in]    routedMsg           The routedMsg will be returned in the 
                                    SMStateEntryFunctionroutedMsg

  @return       void

  @sideeffects  None.
*/
/*=========================================================================*/
void sm_set_current_state(SM_StateContext * thisctx, SM_StateId state, void *routedMsg);

/*===========================================================================
FUNCTION      sm_get_current_state
===========================================================================*/
/**
  @brief API to get the current state of the machine

  @param[in]    thisctx             The state machine context.

  @return       current state of the machine

  @sideeffects  None.
*/
/*=========================================================================*/
SM_StateId sm_get_current_state(SM_StateContext * thisctx);

/*===========================================================================
FUNCTION      sm_get_prev_state
===========================================================================*/
/**
  @brief API to get the previous state of the machine

  @param[in]    thisctx             The state machine context.

  @return       current state of the machine

  @sideeffects  None.
*/
/*=========================================================================*/
SM_StateId sm_get_prev_state(SM_StateContext * thisctx);

#endif /* SM_H */
