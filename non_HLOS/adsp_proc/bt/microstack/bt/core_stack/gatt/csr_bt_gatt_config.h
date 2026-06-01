#ifndef _CSR_BT_GATT_CONFIG_H_
#define _CSR_BT_GATT_CONFIG_H_
/******************************************************************************
 Copyright (c) 2010-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #7 $
******************************************************************************/

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EXCLUDE_CSR_BT_SC_MODULE
#undef CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
    /* Privacy might be enabled for the product but if sc is not used
        then this support from gatt requires to be excluded */
#if !defined(CSR_TARGET_PRODUCT_VM) && !defined(CSR_TARGET_PRODUCT_WEARABLE)
#undef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
#endif
#else
    /* Enable read remote le name feature only if SC is enabled
        as this feature is only used by sc */
#define CSR_BT_GATT_INSTALL_READ_REMOTE_LE_NAME
#endif

#if defined(CSR_TARGET_PRODUCT_VM)
    /* Enable GATT Server Handle translation in GATT Layer
     * It translates the handle value from actual value to Zero index based 
     * value before sending the events or messages to GATT Service libraries.
     * It also translates the Zero index based handle value to actual handle 
     * value in the events or messages received from the GATT Service libraries */
#define CSR_BT_GATT_INSTALL_SERVER_HANDLE_RESOLUTION

    /* Enable New Long Write offset method in GATT layer
     * It Stores Array of pointer in GATT layer instead of creating a big chunk for 
     * All the prepare Write Req. */
#define CSR_BT_GATT_INSTALL_SERVER_LONG_WRITE_OFFSET
    /* Enable New Long Read offset method in GATT layer
     * It Stores Array of pointer in GATT layer instead of creating a big chunk for 
     * All the stored blob Req. */
#define CSR_BT_GATT_INSTALL_CLIENT_LONG_READ_OFFSET
#endif /* CSR_TARGET_PRODUCT_VM */

#if defined(CSR_TARGET_PRODUCT_IOT)
/* #define CSR_BT_GATT_INSTALL_ATT_LE_FIX_CID_ATTACH */
#define CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
#elif defined(CSR_TARGET_PRODUCT_VM) || defined(CSR_TARGET_PRODUCT_AUTO)
#define CSR_BT_GATT_INSTALL_SERVER_GAP_SERVICE
#define CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
#define CSR_BT_INSTALL_GATT_BREDR
#elif  defined(CSR_TARGET_PRODUCT_WEARABLE)
#define CSR_BT_GATT_INSTALL_CLIENT_SERVICE_REGISTRATION
#define CSR_BT_GATT_INSTALL_SERVER_GAP_SERVICE
#else
/* Legacy features - full-le build */
#define CSR_BT_GATT_INSTALL_SERVER_GAP_SERVICE
#endif

#ifdef __cplusplus
}
#endif

#endif

