
/*************************************************************************
 * @file     lpai_bt_npa.c
 * @brief    LPAI BT Island Manager source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "lpai_bt_npa.h"
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include "ee_modechange.h"
#include "npa.h"

/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/

// 1. Define the event object
K_EVENT_DEFINE(island_signal);

// Define bitmasks for your signals
#define SIGNAL_ISLAND_EXIT  (1 << 0)

/**
 * @def NPA_CLIENT_NAME
 * @brief Define the the NPA client name for Island usecases
 *
 * npa client name for BTSW
 */
#define NPA_CLIENT_NAME  "btsw_npa_client"

// Define constants for NPA resources and client naming
#define ISLAND_RESOURCE "/soc/ee_modechange"


/*===========================================================================
                        STATIC/GLOBAL DATA DEFINITIONS
===========================================================================*/

/**
 *  @def btsw_npa_client
 *  @brief npa client for BTSW
 *
 *  Token used to control island entry/exit based on BTSW usecases
 */
npa_client_handle btsw_npa_client = NULL;

/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/


npa_client_handle btsw_island_create_client(const char *client_name)
{
    if ((client_name == NULL) || (strlen(client_name) == 0))
    {
        printk("Invalid client name provided: %p (NULL or empty string).\n", client_name);
        return NULL;
    }
    npa_client_handle handle = npa_create_sync_client(
        ISLAND_RESOURCE, client_name, NPA_CLIENT_REQUIRED);
    if (NULL == handle)
    {
        printk("Failed to create NPA client for island mode resource '%s' with name '%s'.",
            ISLAND_RESOURCE, client_name);
        return NULL;
    }

    printk("NPA client '%s' created successfully.\n", client_name);
    return handle;
}

/**
 * @brief Callback function invoked when the island mode changes.
 *        Signals waiting clients if the system transitions to non-island mode.
 *
 * @param status True if the system is in non-island mode, false otherwise (i.e., island mode).
 */
static void btsw_island_change_cb(bool status)
{
    if (status) //enter non-island mode
    {
        k_event_post(&island_signal, SIGNAL_ISLAND_EXIT);
    }
}

/**
 * @brief Method to initialize island npa used under BTSW
 * @param[in]  None
 * @return int
 */
int btsw_island_npa_init()
{
    btsw_npa_client = btsw_island_create_client(NPA_CLIENT_NAME);
    if (btsw_npa_client == NULL)
    {
        printk("btsw_npa_client creation failed:\n");
        return BTSW_ISLAND_ERR;
    }

    // Register the callback for island mode changes
    (void)ee_modechange_registercb(btsw_island_change_cb);

    return BTSW_ISLAND_SUCCESS;
}

/**
 * @brief Method to exit island for a BTSW usecase
 * @param[in]  btsw_island_status
 * @return int
 */
int btsw_vote_island_exit()
{
    if (btsw_npa_client == NULL)
    {
        printk("Invalid NPA client handle for island exit vote (NULL).\n");
        return BTSW_ISLAND_ERR;
    }

    if (exit_island_mode(&btsw_npa_client))
    {
        printk("Failed to exit island mode for client %p", (void *)btsw_npa_client);
        return BTSW_ISLAND_ERR;
    }

    int current_state = get_current_state(); // 0 for ISLAND, 1 for NON_ISLAND
    printk("%s: current state = %d (0=ISLAND, 1=NON_ISLAND)\n", __func__, current_state);

    if (!current_state)
    {
        // Clear the event manually if 'reset' was false above
        k_event_clear(&island_signal, SIGNAL_ISLAND_EXIT);

        uint32_t events = k_event_wait(&island_signal, 
                                        SIGNAL_ISLAND_EXIT, 
                                        false,        // true = reset bits after wait
                                        K_FOREVER);   // Wait indefinitely
    }

    printk("btsw island exit complete\n");
    return BTSW_ISLAND_SUCCESS;
}

/**
 * @brief Method to enter island for a BTSW usecase
 * @param[in]  None
 * @return int
 */
int btsw_vote_island_entry()
{
    if (btsw_npa_client == NULL)
    {
        printk("Client handle (NULL).\n");
        return BTSW_ISLAND_ERR;
    }

    // Allow island mode re-entry
    if (enter_island_mode(&btsw_npa_client))
    {
        printk("island mode entry failed hdl%p.\n", (void *)btsw_npa_client);
        return BTSW_ISLAND_ERR;
    }

    printk("allow island mode for cl_hdl %p.\n", (void *)btsw_npa_client);
    return BTSW_ISLAND_SUCCESS;
}

// Zephyr system initialization macro to call btsw_island_npa_init.
// APPLICATION priority ensures it runs post KERNEL and before most user threads.
SYS_INIT(btsw_island_npa_init, APPLICATION, 90);