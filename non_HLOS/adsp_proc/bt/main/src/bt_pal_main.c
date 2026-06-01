/*=============================================================================
  @file bt_pal_main.c

  Main File for the BT PAL layer

*******************************************************************************
* Copyright (c) 2012-2025 Qualcomm Technologies, Inc.  All Rights Reserved
* Qualcomm Technologies Confidential and Proprietary.
********************************************************************************/


/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "stdlib.h"
#include "Diag_LSM.h"
#include "offload_fw.h"
#include "bt_pal_npa.h"
#include "qurt_island.h"
#include "uSleep_mode_trans.h"

#ifdef ENABLE_Q6_QCLI
#include "qcli.h"
#endif /* ENABLE_Q6_QCLI */


/*=============================================================================
                        MACRO DEFINITIONS
=============================================================================*/
/**
 * @brief F3 trace buffer size for Big Image(128KB)
 */
#define BT_F3_TRACE_SIZE (0x20000U)

/*=============================================================================
                    GLOBAL DATA DECLARATIONS
=============================================================================*/
/**
 * @brief F3 trace buffer for Big Image
 */
static uint8_t bt_f3_trace_buffer[BT_F3_TRACE_SIZE] = {0};


/*===========================================================================
                    LOCAL FUNCTION DEFINITIONS
===========================================================================*/
/*===========================================================================
FUNCTION      bt_pal_trace_f3_buff_init
===========================================================================*/
/**
 * @brief Initializes f3 trace buffer for Big Image.
 * 
 * @note TBD: revisit during PDR implementation
 */
static void bt_pal_trace_f3_buff_init(void)
{
    diag_lsm_f3_trace_init(bt_f3_trace_buffer, BT_F3_TRACE_SIZE);
}

/*=============================================================================
                    GLOBAL FUNCTION DEFINITIONS
=============================================================================*/
/*===========================================================================
FUNCTION      bt_pal_Init
===========================================================================*/
/**
 * @brief Main file for the BT_PAL layer
 *
 * @param evt Event data.
 */
void bt_pal_Init(void)
{
    /* F3 trace buffer init */
    bt_pal_trace_f3_buff_init();

    /*NPA Client Creation*/
    bt_pal_island_mgr_init();
    
    /**Exit Island if in Island Mode */
    if (qurt_island_get_status())
    {
        uSleep_exit();
    }

    /**Restrict Island Entry for Task Initializations*/
    bt_pal_mgr_restrict_island();
    
    /* Enable Offload Framework */
    Offload_Fw_Init();


#ifdef ENABLE_Q6_QCLI
    Qcli_App_Init();
#endif /* ENABLE_Q6_QCLI */

    /**Allow for Island Entry once Task Initialization is done*/
    bt_pal_mgr_allow_island();
}
