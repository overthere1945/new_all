/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2026 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      wlan_gpio.h
===========================================================================*/
/**
 * @file wlan_gpio.h
 * @brief Header file for WLAN GPIO operations.
 *
 * @details This file contains the definitions and declarations for WLAN GPIO
 *          time synchronization operations using GPIO 78.
 */

#ifndef WLAN_GPIO_H
#define WLAN_GPIO_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include "lpai_transport.h"

/*===========================================================================
                      MACRO DEFINITIONS
===========================================================================*/
/**
 * @brief WLAN GPIO ID.
 * 
 * GPIO 78 is used for WLAN time synchronization.
 * This is hardcoded in the implementation and not exposed in the protocol.
 */
#define WLAN_GPIO_ID (78U)

/**
 * @brief Command types sent from HOST to ADSP.
 */
typedef enum {
    WLAN_GPIO_CMD_REGISTER = 0x01,      /**< Register GPIO command */
    WLAN_GPIO_CMD_DEREGISTER = 0x02,    /**< Deregister GPIO command */
} wlan_gpio_cmd_type_t;

/**
 * @brief Response types sent from ADSP to HOST.
 */
typedef enum {
    WLAN_GPIO_RSP_REGISTER = 0x01,      /**< Register response */
    WLAN_GPIO_RSP_DEREGISTER = 0x02,    /**< Deregister response */
} wlan_gpio_rsp_type_t;

/**
 * @brief Status codes for WLAN GPIO operations.
 */
typedef enum {
    WLAN_GPIO_STATUS_SUCCESS = 0x00,           /**< Operation successful */
    WLAN_GPIO_STATUS_INVALID_PARAMS = 0x01,    /**< Invalid parameters */
    WLAN_GPIO_STATUS_ALREADY_REGISTERED = 0x02, /**< GPIO already registered */
    WLAN_GPIO_STATUS_NOT_REGISTERED = 0x03,    /**< GPIO not registered */
} wlan_gpio_status_t;

/**
 * @brief Request structure for GPIO operations.
 * 
 * Used for both REGISTER and DEREGISTER commands.
 * The cmd_type field determines which operation to perform.
 * GPIO 78 is hardcoded in the implementation.
 */
typedef struct {
    wlan_gpio_cmd_type_t cmd_type;  /**< Command type from HOST */
} wlan_gpio_request_t;

/**
 * @brief Response message structure.
 */
typedef struct {
    wlan_gpio_rsp_type_t rsp_type;  /**< Response type to HOST */
    wlan_gpio_status_t status;      /**< Status code */
} wlan_gpio_response_t;

/*===========================================================================
                    GLOBAL FUNCTION DECLARATIONS
===========================================================================*/
/**
 * @brief Handles WLAN GPIO messages.
 *
 * @param[in] data Pointer to the message data.
 * @param[in] size Size of the message data.
 */
void wlan_gpio_handle_message(uint8_t *data, size_t size);

/**
 * @brief Sends a response message for WLAN GPIO operations.
 *
 * @param[in] rsp_type Response type to send.
 * @param[in] status Status code.
 */
void wlan_gpio_send_response(wlan_gpio_rsp_type_t rsp_type, wlan_gpio_status_t status);

/**
 * @brief Convenience function to send data to WLAN GPIO channel.
 *
 * @param[in] ptr Pointer to data.
 * @param[in] size Size of the data.
 * @return glink_err_type Return code from glink.
 */
static inline glink_err_type Offload_Mgr_SendDataToWlanGpio(const void *ptr, size_t size) 
{
    return LPAI_Transport_Send_Data_On_Chnl(LPAI_TRANSPORT_HLOS_LINK_IDX, 
               LPAI_TRANSPORT_HLOS_WLAN_GPIO_CHNL_IDX, ptr, size);
}

#endif /* WLAN_GPIO_H */
