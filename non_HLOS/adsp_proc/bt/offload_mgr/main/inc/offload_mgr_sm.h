/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_sm.h
===========================================================================*/
/**
 * @file offload_mgr_sm.h
 * @brief Offload Manager State Manager. 
 *
 * @details This file handles the offload states based on the following events : 
 * 1. Offload to microstack
 * 2. Offload to microapps
 * 3. Unoffload to microapps
 * 4. Unoffload to microstack
 */

#ifndef OFFLOAD_MGR_H
#define OFFLOAD_MGR_H

/*===========================================================================
                    INCLUDE FILES
===========================================================================*/
#include "sm.h"
#include "bmm_lib.h"
#include "bt_main.h"
#include "offload_mgr_log.h"

/*===========================================================================
                    TYPE DEFINITIONS
===========================================================================*/
typedef uint8_t offload_instance_id_t;
/**
 * @enum offload_evt_t
 * @brief Enumeration for offload events.
 *
 * This enumeration defines various events related to offload operations.
 */
typedef enum
{
  OFFLOAD_REQ = 0,
  OFFLOAD_TO_MICROSTACK_SUCCESS,
  OFFLOAD_TO_MICROSTACK_FAILURE,
  UNOFFLOAD_REQ,
  OFFLOAD_TO_MICROAPP_SUCCESS,
  OFFLOAD_TO_MICROAPP_FAILURE,
  UNOFFLOAD_FROM_MICROSTACK_SUCCESS,
  UNOFFLOAD_FROM_MICROAPP_SUCCESS,
  OFFLOAD_INSTANCE_FREE,
} offload_evt_t;


/*===========================================================================
STRUCTURE      offload_mgr_state_ctx_t
===========================================================================*/
/**
  @brief Structure for offload manager state context.

  @details This structure holds the state context and additional data needed for offload operations.

  @var sm_state_ctx    State context for the offload manager.
  @var data            Pointer to extra data needed to process the operation on the offload.
*/
typedef struct offload_mgr_sm_state_ctx
{
  SM_StateContext sm_state_ctx;
  void *data;
} offload_mgr_sm_state_ctx_t;

/*===========================================================================
                    GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
FUNCTION      offload_mgr_evt_handler
===========================================================================*/
/**
  @brief Handles offload manager events.

  @param[in]    opcode    Opcode of the event.
  @param[in]    evt       Pointer to the event data.
  @param[in]    len       Length of the event data.
  @return       void.
  @sideeffects  None.
*/
void offload_mgr_evt_handler(uint16_t opcode, uint8_t *evt, size_t len);

/*===========================================================================
FUNCTION      offload_mgr_unoffload_cfm_cb
===========================================================================*/
/**
  @brief Callback for unoffload confirmation.

  @param[in]    id    offload instance id
  @param[in]    status   Status of the unoffload operation.

  @return       void.

  @sideeffects  None.
*/
void offload_mgr_profile_unoffload_cfm_cb(offload_instance_id_t id, int status);


/*===========================================================================
FUNCTION      offload_mgr_offload_cfm_cb
===========================================================================*/
/**
  @brief Callback for offload confirmation.

  @param[in]    id offload instance id
  @param[in]    status

  @return       void.

  @sideeffects  None.
*/
void offload_mgr_profile_offload_cfm_cb(offload_instance_id_t id, int status);

/*===========================================================================
FUNCTION      offload_mgr_offload_to_uapp_rsp_cb
===========================================================================*/
/**
  @brief callback called by endpoint manager when offload with microapp response
         is received.

  @param[in]    id    offload instance id
  @param[in]    status status of the offload to microapp

  @return       void.

  @sideeffects  None.
*/
void offload_mgr_uapp_offload_rsp_cb(offload_instance_id_t id, uint8_t status);

/*===========================================================================
FUNCTION      offload_mgr_uapp_unoffload_rsp_cb
===========================================================================*/
/**
 * @brief Uapp unoffload response callback.
 * @details This function is called when the uapp unoffload operation is responded.
 * @param[in] id Offload instance ID.
 * @param[in] status Status of the unoffload operation.
 */
void offload_mgr_uapp_unoffload_rsp_cb(offload_instance_id_t id, uint8_t status);

/*===========================================================================
FUNCTION      offload_mgr_sm_init
===========================================================================*/
/**
 * @brief Initialize offload mgr state machine.
 * @details This api will initialize the offload mgr state machine. 
 */
void offload_mgr_sm_init(void);

/*===========================================================================
FUNCTION      offload_mgr_sm_deinit
===========================================================================*/
/**
 * @brief Deinitialize offload mgr state machine. 
 * @details This function deinitialize the offload mgr state machine. 
 */
void offload_mgr_sm_deinit(void);

#endif /* OFFLOAD_MGR_H */
