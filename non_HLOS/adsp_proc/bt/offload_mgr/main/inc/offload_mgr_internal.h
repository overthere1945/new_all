/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_internal.h
===========================================================================*/
/**
 * @file offload_mgr_internal.h
 * @brief Internal functions used by offload mgr. 
 *
 * @details This file includes internal functions used by offload mgr. 
 */

#ifndef OFFLOAD_MGR_INTERNAL_H
#define OFFLOAD_MGR_INTERNAL_H

/*===========================================================================
                             INCLUDE FILES
===========================================================================*/
#include "offload_mgr_sm.h"
#include "offload_instance.h"
#include "offload_mgr_log.h"


/*===========================================================================
                        GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

offload_instance_t *offload_mgr_sm_find_idle_offload_instance(void);
void offload_mgr_sm_handle_offload_evt(offload_instance_t *offload);
void offload_mgr_sm_handle_unoffload_evt(offload_instance_t *offload);
void offload_mgr_sm_handle_uapp_offload_evt(offload_instance_t *offload, int status);
void offload_mgr_sm_handle_microstack_offload_evt(offload_instance_t *offload, int status);
void offload_mgr_sm_handle_microstack_unoffload_evt(offload_instance_t *offload, int status);
void offload_mgr_sm_handle_uapp_unoffload_evt(offload_instance_t *offload, int status);
#endif  /* OFFLOAD_MGR_INTERNAL_H */
