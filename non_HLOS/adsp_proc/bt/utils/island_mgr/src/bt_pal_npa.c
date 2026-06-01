/*=============================================================================
  @file bt_pal_npa.c

  This file contains island manager implementations of BT PAL.

*******************************************************************************
* Copyright (c) 2012-2025 Qualcomm Technologies, Inc.  All Rights Reserved
* Qualcomm Technologies Confidential and Proprietary.
********************************************************************************/
/*===========================================================================
                      INCLUDE FILES
===========================================================================*/
#include "npa.h"
#include "uSleep_mode_trans.h"
#include "uSleep_islands.h"
#include "stdint.h"
#include "island_user.h"
#include "bt_pal_log.h"


/*===========================================================================
                        PUBLIC/GLOBAL DATA DEFINITIONS
===========================================================================*/
npa_client_handle uSleepTestClient = NULL;
uint16_t bt_island_entry_ctr = 0;
uint16_t bt_island_exit_ctr = 0;
uSleep_notification_cb_handle cb_handle = NULL;
int16_t current_island_state = -1;


/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/
/**
 * @brief Callback Function which is invoked each a system state transition occurs
 * @param[in] state  Current Islanad Transition change in the system
 * @return  None
 */
static void island_notification_cb(uSleep_state_notification state)
{
    current_island_state = state;
    if (USLEEP_STATE_EXIT == state)
    {
        bt_island_exit_ctr++;
        //BT_PAL_LOGH("Island Exit Done ! \n");
    }
    else if (USLEEP_STATE_ENTER == state)
    {
        bt_island_entry_ctr++;
        //BT_PAL_LOGH("Island Entry Done ! \n");
    }
}


/**
 * uSleepNode_createTestClient
 *
 * @brief Creates a test client to control uSleep entry for debug or
 *        bringup purposes.
 *
 * @note Variables must be initially set at cold boot time.
 *       Changing after that, during run-time, will have no effect.
 */
static void uSleepNode_createTestClient(void)
{
  /* Check if test clients should be created */
  uSleepTestClient = npa_create_sync_client(USLEEP_NODE_NAME,
                                              "btPalTest",
                                              NPA_CLIENT_REQUIRED);

  if(NULL != uSleepTestClient)
  {
    cb_handle = uSleep_registerNotificationCallbackEx(USLEEP_ALL_ISLANDS, 150, 150, island_notification_cb);
  }

  return;
}


/**
 * @brief Define the Callback Funtion which is invoked when a npa intended resource is available
 * @return  None
 */
static void MyResourceAvailableCb( void *context,
                                   unsigned int  event_type, 
                                   void *data,       
                                   unsigned int  data_size )
{
  // context is the user data that was provided when registering for the
  // callback
  uSleepNode_createTestClient();
}


/**
 * @brief Initialization Function for BT Island Manager
 * @param[in] None
 * @return    None
 */
void bt_pal_island_mgr_init (void)
{
  // Wait for the USLEEP_NODE_NAME resource to be available.
  npa_resource_available_cb(  USLEEP_NODE_NAME,
                             MyResourceAvailableCb,
                             NULL);

}


/**
 * @brief Function to Allow Island Entry using BT PAL Client
 * @param[in] None
 * @return    None
 */
void bt_pal_mgr_allow_island(void)
{
  npa_issue_required_request(uSleepTestClient, USLEEP_VOTE(USLEEP_ALL_ISLANDS, USLEEP_CLIENT_ALLOW_ISLAND));
}


/**
 * @brief Function to Restrict Island Entry using BT PAL Client
 * @param[in] None
 * @return    None
 */
void bt_pal_mgr_restrict_island(void)
{
  npa_issue_required_request(uSleepTestClient, USLEEP_VOTE(USLEEP_ALL_ISLANDS, USLEEP_CLIENT_RESTRICT_ISLAND));
}
