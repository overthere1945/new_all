
/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      endpt_mgr.h
===========================================================================*/
/**
 * @file endpt_mgr.h
 * @brief Exposes public interface for endpoint manager.
 *
 * @details This file provides the public interface for managing endpoints.
 */

#ifndef ENDPT_MGR_H
#define ENDPT_MGR_H

/*============================================================================*
                           INCLUDE FILES
*============================================================================*/
#include "socket_mgr.h" 

/*============================================================================*
                           MACRO DEFINITIONS
*============================================================================*/
/**
 * @define ENDPT_MGR_HUB_ID_AWM
 * @brief Hub ID for endpoint manager M55.
 */
#define ENDPT_MGR_HUB_ID_AWM        ((uint64_t)0xFBFBFBFBFBFBFB0A)

/*============================================================================*
                           EXTERN VARIABLES
*============================================================================*/
extern uint64_t endpt_mgr_hub_id_awm;

/*============================================================================*
                           GLOBAL FUNCTION DECLARATIONS
*============================================================================*/

/*===========================================================================
FUNCTION      endpt_mgr_rpc_encode_msg
===========================================================================*/
/**
  API used to send the data to the endpoint. 

  @param[in]    hub_id          hub of the endpoint to which the data is to be sent. 
  @param[in]    data            The pointer pointing to data(Expected to be encoded by
                                using endpt_mgr_rpc_encode_msg api before sending). 
  @param[in]    endpt           Endpoint to which the data is to be sent.
  @param[in]    data_len        Length of the data to be sent.
  @return       None.          
  @sideeffects  None.
*/
void endpt_mgr_send_msg_to_endpoint(uint64_t *hub_id, void *data, int data_len);


/*===========================================================================
FUNCTION      endpt_mgr_init
===========================================================================*/
/**
  Initialize endpt mgr.
  @param[in]    None. 
  @return       None.          
  @sideeffects  None.
*/
void endpt_mgr_init(void);

/*===========================================================================
FUNCTION      endpt_mgr_deinit
===========================================================================*/
/**
  Deinitialize endpt mgr.
  @param[in]    None. 
  @return       None.          
  @sideeffects  None.
*/
void endpt_mgr_deinit(void);


/*===========================================================================
FUNCTION      endpt_mgr_handle_awm_evt
===========================================================================*/
/**
  AWM events handler API.

  @param[in]    evt            event data
  @param[in]    len            length of the data received
  @return       None.          
  @sideeffects  None.
*/
void endpt_mgr_handle_awm_evt(uint8_t *evt, size_t len);

/*===========================================================================
FUNCTION      endpt_mgr_handle_context_hub_evt
===========================================================================*/
/**
  Context Hub events handler API. 
  @param[in]    opcode         opcode
  @param[in]    evt            event data
  @param[in]    len            length of the data received
  @return       None.          
  @sideeffects  None.
*/
void endpt_mgr_handle_context_hub_evt(uint16_t opcode, uint8_t *evt, size_t len);

#endif /* ENDPT_MGR_H */