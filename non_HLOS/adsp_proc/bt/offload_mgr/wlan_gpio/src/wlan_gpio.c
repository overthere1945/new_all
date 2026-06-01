/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2026 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      wlan_gpio.c
===========================================================================*/
/**
 * @file wlan_gpio.c
 * @brief Implementation of WLAN GPIO operations.
 *
 * @details This file contains the implementation of WLAN GPIO time synchronization
 *          operations using GPIO 78 with uGPIOInt API integration.
 */

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "uGPIOInt.h"
#include "wlan_gpio.h"
#include "uGPIOInt.h"
#include "bt_pal_heap.h"
#include "offload_mgr_log.h"
#include "qurt_island.h"
#include "uSleep_mode_trans.h"
#include "bt_pal_npa.h"

/*===========================================================================
                      MACRO DEFINITIONS
===========================================================================*/
#define WLAN_GPIO_LOGE(fmt, ...) OFFLOAD_MGR_LOGE(fmt, ##__VA_ARGS__)
#define WLAN_GPIO_LOGM(fmt, ...) OFFLOAD_MGR_LOGM(fmt, ##__VA_ARGS__)
#define WLAN_GPIO_LOGL(fmt, ...) OFFLOAD_MGR_LOGL(fmt, ##__VA_ARGS__)
#define WLAN_GPIO_LOGH(fmt, ...) OFFLOAD_MGR_LOGH(fmt, ##__VA_ARGS__)

/*===========================================================================
                    GLOBAL DATA DECLARATIONS
===========================================================================*/
/**
 * @brief Flag to track if GPIO is registered.
 */
static bool gpio_registered = false;

/*===========================================================================
                    LOCAL FUNCTION DEFINITIONS
===========================================================================*/
/**
 * @brief GPIO interrupt callback for WLAN time sync events.
 * 
 * This callback is invoked when GPIO 78 triggers (rising edge).
 * Hardware timestamp is captured automatically by the GPIO hardware.
 * 
 * @param param Parameter passed by the interrupt system (unused).
 */
static void wlan_gpio_isr_callback(uint32_t param)
{
    WLAN_GPIO_LOGH("GPIO%d IRQ", WLAN_GPIO_ID);
}

/**
 * @brief Registers GPIO interrupt using uGPIOInt API.
 * 
 * Configures GPIO 78 for rising edge interrupt with hardware timestamp
 * capture and island mode support.
 * 
 * @return Status code indicating success or failure.
 */
static wlan_gpio_status_t wlan_gpio_register_isr(void)
{
    int32_t result;

    result = uGPIOInt_RegisterInterrupt(
                WLAN_GPIO_ID,                         
                UGPIOINT_TRIGGER_RISING,              
                (uGPIOINTISR)wlan_gpio_isr_callback,  
                NULL,                                 
                (UGPIOINTF_TIMESTAMP_EN | UGPIOINTF_ISLAND));

    if (result != UGPIOINT_SUCCESS) {
        WLAN_GPIO_LOGE("GPIO%d reg fail:%d", WLAN_GPIO_ID, result);
        return WLAN_GPIO_STATUS_INVALID_PARAMS;
    }

    WLAN_GPIO_LOGH("GPIO%d reg OK", WLAN_GPIO_ID);
    return WLAN_GPIO_STATUS_SUCCESS;
}

/**
 * @brief Deregisters GPIO interrupt using uGPIOInt API.
 * 
 * Removes the interrupt handler and disables GPIO 78.
 * 
 * @return Status code indicating success or failure.
 */
static wlan_gpio_status_t wlan_gpio_deregister_isr(void)
{
    int32_t result;

    result = uGPIOInt_DeregisterInterrupt(WLAN_GPIO_ID);

    if (result != UGPIOINT_SUCCESS) {
        WLAN_GPIO_LOGE("GPIO%d dereg fail:%d", WLAN_GPIO_ID, result);
        return WLAN_GPIO_STATUS_INVALID_PARAMS;
    }

    WLAN_GPIO_LOGH("GPIO%d dereg OK", WLAN_GPIO_ID);
    return WLAN_GPIO_STATUS_SUCCESS;
}

/**
 * @brief Handles WLAN GPIO register command.
 * 
 * Validates registration state and registers GPIO 78 if not already registered.
 * 
 * @param data Pointer to the command data.
 * @param size Size of the command data.
 */
static void wlan_gpio_handle_register_cmd(uint8_t *data, size_t size)
{
    wlan_gpio_status_t status;

    WLAN_GPIO_LOGH("GPIO%d reg cmd", WLAN_GPIO_ID);

    if (gpio_registered) {
        WLAN_GPIO_LOGE("GPIO%d already reg", WLAN_GPIO_ID);
        wlan_gpio_send_response(WLAN_GPIO_RSP_REGISTER, 
                              WLAN_GPIO_STATUS_ALREADY_REGISTERED);
        return;
    }

    /**If in Island Mode , Exit it and restrict Island Entry */
    if (qurt_island_get_status())
    {
        uSleep_exit();
    }
    bt_pal_mgr_restrict_island();

    status = wlan_gpio_register_isr();

    bt_pal_mgr_allow_island();

    if (status == WLAN_GPIO_STATUS_SUCCESS) {
        gpio_registered = true;
    }

    wlan_gpio_send_response(WLAN_GPIO_RSP_REGISTER, status);
}

/**
 * @brief Handles WLAN GPIO deregister command.
 * 
 * Validates registration state and deregisters GPIO 78 if currently registered.
 * 
 * @param data Pointer to the command data.
 * @param size Size of the command data.
 */
static void wlan_gpio_handle_deregister_cmd(uint8_t *data, size_t size)
{
    wlan_gpio_status_t status;

    WLAN_GPIO_LOGH("GPIO%d dereg cmd", WLAN_GPIO_ID);

    if (!gpio_registered) {
        WLAN_GPIO_LOGE("GPIO%d not reg", WLAN_GPIO_ID);
        wlan_gpio_send_response(WLAN_GPIO_RSP_DEREGISTER, 
                              WLAN_GPIO_STATUS_NOT_REGISTERED);
        return;
    }

    /**If in Island Mode , Exit it and restrict Island Entry */
    if (qurt_island_get_status())
    {
        uSleep_exit();
    }
    bt_pal_mgr_restrict_island();

    status = wlan_gpio_deregister_isr();

    bt_pal_mgr_allow_island();

    if (status == WLAN_GPIO_STATUS_SUCCESS) {
        gpio_registered = false;
    }

    wlan_gpio_send_response(WLAN_GPIO_RSP_DEREGISTER, status);
}

/*===========================================================================
                    GLOBAL FUNCTION DEFINITIONS
===========================================================================*/
/**
 * @brief Handles WLAN GPIO messages.
 * 
 * Main entry point for processing WLAN GPIO messages from APPS processor.
 * Routes messages to appropriate handlers based on message type.
 * 
 * @param data Pointer to the message data.
 * @param size Size of the message data.
 */
void wlan_gpio_handle_message(uint8_t *data, size_t size)
{
    if (data == NULL || size < 1) {
        WLAN_GPIO_LOGE("Invalid msg: data=%p sz=%zu", data, size);
        return;
    }

    wlan_gpio_cmd_type_t cmd_type = (wlan_gpio_cmd_type_t)data[0];

    WLAN_GPIO_LOGH("Msg: cmd=%d sz=%zu", cmd_type, size);

    switch (cmd_type) {
        case WLAN_GPIO_CMD_REGISTER:
            if (size >= sizeof(wlan_gpio_request_t)) {
                wlan_gpio_handle_register_cmd(data, size);
            } else {
                WLAN_GPIO_LOGE("Invalid reg cmd sz:%zu exp:%zu", 
                             size, sizeof(wlan_gpio_request_t));
                wlan_gpio_send_response(WLAN_GPIO_RSP_REGISTER, 
                                      WLAN_GPIO_STATUS_INVALID_PARAMS);
            }
            break;

        case WLAN_GPIO_CMD_DEREGISTER:
            if (size >= sizeof(wlan_gpio_request_t)) {
                wlan_gpio_handle_deregister_cmd(data, size);
            } else {
                WLAN_GPIO_LOGE("Invalid dereg cmd sz:%zu exp:%zu", 
                             size, sizeof(wlan_gpio_request_t));
                wlan_gpio_send_response(WLAN_GPIO_RSP_DEREGISTER, 
                                      WLAN_GPIO_STATUS_INVALID_PARAMS);
            }
            break;

        default:
            WLAN_GPIO_LOGE("Unknown cmd:%d", cmd_type);
            /* Unknown cmd: use reg rsp as default */
            wlan_gpio_send_response(WLAN_GPIO_RSP_REGISTER, WLAN_GPIO_STATUS_INVALID_PARAMS);
            break;
    }
}

/**
 * @brief Sends a response message for WLAN GPIO operations.
 * 
 * Constructs and sends a response message back to APPS processor via GLINK.
 * 
 * @param rsp_type Response type to send.
 * @param status Status code.
 */
void wlan_gpio_send_response(wlan_gpio_rsp_type_t rsp_type, wlan_gpio_status_t status)
{
    wlan_gpio_response_t response;
    response.rsp_type = rsp_type;
    response.status = status;

    WLAN_GPIO_LOGH("Rsp: type=%d sts=%d", rsp_type, status);

    Offload_Mgr_SendDataToWlanGpio(&response, sizeof(response));
}
