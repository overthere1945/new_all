/**************************************************************************
 * @file     lpai_bt_state_mgr.c
 * @brief    LPAI BT State Manager Source file.
 * 			 It contains handling for BT ON and OFF Sequences
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
******************************************************************************/


#include <stdbool.h>
#include "lpai_bt_state_mgr.h"
#include "lpai_bt_le_adv.h"
#include "lpai_bt_le_scan.h"
#include "lpai_bt_rfcomm_app.h"

extern appMgrContext_t appMgrCtx;

/**
 * @breif Method to perform unregistration of microapps once BT Off Status is received from ADSP
 * @param[in] None
 * @return None
 */
void handle_bt_off()
{
	printk("Deinit All Profiles \n");
	le_adv_deinit();
	le_scan_deinit();
	lecoc_app_deinit();
	rfcomm_app_deinit();
#ifdef CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER
    extern void gatt_app_deinit();
    gatt_app_deinit();
    extern void ancs_deinit();
    ancs_deinit();
#endif
}

bt_status_t lpai_bt_get_status(void)
{
    return appMgrCtx.btStatus;
}



void lpai_bt_status_evt(bt_status_t state)
{
    /*Store the Bt State for future use*/
    appMgrCtx.btStatus = state;
    printk("BT Status Received is %d\n", state);
    if(state == BT_STATUS_ON)
    {
        printk("BT Turned ON ! \n");
        #ifdef CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER
            extern void gatt_mgr_handle_bt_on_evt();
            gatt_mgr_handle_bt_on_evt();
        #endif
    }
    else
    {
        printk("BT Turned OFF ! \n");
        handle_bt_off();
    }
    
}