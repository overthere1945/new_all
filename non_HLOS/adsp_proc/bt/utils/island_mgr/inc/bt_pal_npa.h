/*=============================================================================
  @file bt_pal_npa.h

  This file contains island manager implementations of BT PAL..

*******************************************************************************
* Copyright (c) 2012-2025 Qualcomm Technologies, Inc.  All Rights Reserved
* Qualcomm Technologies Confidential and Proprietary.
********************************************************************************/
#ifndef BT_PAL_NPA_H
#define BT_PAL_NPA_H

/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
/**
 * @brief Initialization Function for BT Island Manager
 * @param[in] None
 * @return    None
 */
void bt_pal_island_mgr_init (void);


/**
 * @brief Function to Allow Island Entry using BT PAL Client
 * @param[in] None
 * @return    None
 */
void bt_pal_mgr_allow_island(void);


/**
 * @brief Function to Restrict Island Entry using BT PAL Client
 * @param[in] None
 * @return    None
 */
void bt_pal_mgr_restrict_island(void);



#endif /* BT_PAL_NPA_H */