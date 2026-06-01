/*=============================================================================
  @file bt_state_mgr.h
=============================================================================*/
/**
 * @file bt_state_mgr.h
 * @brief Header file containing BT state management implementations for BT PAL.
 *
 * @details 
 * This file includes the implementations for managing the state of Bluetooth (BT) in the PAL (Platform Abstraction Layer).
 *
*******************************************************************************
* Copyright (c) 2012-2025 Qualcomm Technologies, Inc.  All Rights Reserved
* Qualcomm Technologies Confidential and Proprietary.
********************************************************************************/
#ifndef BT_STATE_MGR_H
#define BT_STATE_MGR_H
/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "offload_mgr_transport_shim.h"
#include "bt_utils.h"
#include "bt_main.h"
#include "bmm_lib.h"
#include "offload_mgr_log.h"

/*=============================================================================
                        TYPE DEFINITIONS
=============================================================================*/
/**
 * @enum bt_off_mode_t
 * @brief Enum for bt_off_modes
 */
typedef enum {
    BT_OFF_INIT = 0,                           /** Initial state */                            
    BT_OFF_USER_OFFLOAD_MODE = BT_OFF_INIT,    /** Offload mode */
    BT_OFF_USER_PASSTHROUGH_MODE = 1,          /** Passthrough mode */
    BT_OFF_SSR = 2,                            /** SSR case */
    BT_OFF_FMD = 3,                            /** Find My Device feature */
    BT_OFF_MAX = 4                             /** MAX value for boundary checks */
} bt_off_mode_t;

/**
 * @brief Type definition for MicroStack modes.
 */
typedef uint16_t btMode_t;

/**
 * @brief Type definition for BT states.
 */
typedef uint16_t btState_t;

/**
 * @enum bt_on_mode_t
 * @brief BT state manager modes for BT On state.
 */
typedef enum 
{
    BT_STATE_MGR_OFFLD_MODE,            /**< Offload mode for BT state manager */
    BT_STATE_MGR_PASSTHROUGH_MODE,      /**< Passthrough mode for BT state manager */
    BT_STATE_MGR_OFFLD_MODE_SSR,        /**< SSR during Offload mode for BT state manager. */
    BT_STATE_MGR_PASSTHROUGH_MODE_SSR,  /**< SSR during Passthrough mode for BT state manager */
    BT_STATE_MGR_DEFAULT_MODE = 0xFF,   /**< Default mode for BT state manager. */
} bt_on_mode_t;

/*=============================================================================
                        MACRO DEFINITIONS
=============================================================================*/
/**
 * @brief BT enable state.
 */
#define BT_ENABLE   (btState_t)0x0F01
/**
 * @brief BT disable state.
 */
#define BT_DISABLE  (btState_t)0x0F02

/*=============================================================================
                    GLOBAL FUNCTION DECLARATIONS
=============================================================================*/
/**
 * @brief Handles BT ON event.
 *
 * @param evt Event data.
 */
 void bt_state_on_handle(uint8_t *evt);

 /**
  * @brief Handles BT OFF event.
  *
  * @param evt Event data.
  */
 void bt_state_off_handle(uint8_t *evt);
 
 /**
  * @brief Initializes the BT state manager.
  */
 void bt_state_mgr_init(void);
 
 /**
  * @brief Deinitializes the BT state manager.
  */
 void bt_state_mgr_deinit(void);
 
 /**
  * @brief Stops the MicroStack in BT state manager.
  */
 void bt_state_mgr_microstack_stop(void);

 /**
  * @brief Function to handle SSR scenarios due to HAL crashes. 
  * This function checks and trigger BT-off if not triggered.
  */
 void bt_state_check_and_mock_off(void);

#endif /* BT_STATE_MGR_H */
