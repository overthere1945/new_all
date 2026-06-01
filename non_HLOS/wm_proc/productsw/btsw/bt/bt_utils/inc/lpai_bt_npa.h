/**************************************************************************
 * @file     lpai_bt_npa.h
 * @brief    LPAI BT Island manager header file.
 * 
 * This file contains the declarations for all the island manager API exposed,
 * to BT Apps 
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_NPA_H
#define LPAI_BT_NPA_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stddef.h>

/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

/******************************************************************************
 * @enum btsw_island_status
 * @brief island status err codes.
 ******************************************************************************/
typedef enum {
    BTSW_ISLAND_SUCCESS = 0,  /**< success case. */
    BTSW_ISLAND_ERR,          /**< error case. */
} btsw_island_status;


/**
 * @brief Method to initialize island npa used under BTSW
 * @param[in]  None
 * @return int
 */
int btsw_island_npa_init();

/**
 * @brief Method to exit island for a BTSW usecase
 * @param[in]  None
 * @return int
 */
int btsw_vote_island_exit();

/**
 * @brief Method to enter island for a BTSW usecase
 * @param[in]  None
 * @return int
 */
int btsw_vote_island_entry();


#endif /**LPAI_BT_NPA_H */