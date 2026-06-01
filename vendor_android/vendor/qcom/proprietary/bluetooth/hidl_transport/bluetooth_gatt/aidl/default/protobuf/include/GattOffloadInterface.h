/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

/*Maintaining MSG-IDs B/w Offload Manager/Q6 and AP*/
#define BT_LE_GATT_OFFLOAD                       0x0F10

#define BT_LE_GATT_REGISTER_SERVICE              0x0F10								         
#define BT_LE_GATT_UNREGISTER_SERVICE            0x0F11						         
#define BT_LE_GATT_CLEAR_SERVICES                0x0F12								         
#define BT_LE_GET_GATT_CAPABILITIES              0x0F13
#define BT_LE_GATT_REGISTER_SERVICE_COMPLETE     BT_LE_GATT_REGISTER_SERVICE
#define BT_LE_GATT_UNREGISTER_SERVICE_COMPLETE   BT_LE_GATT_UNREGISTER_SERVICE
#define BT_LE_GATT_CLEAR_SERVICES_COMPLETE       BT_LE_GATT_CLEAR_SERVICES
#define BT_LE_GATT_ERROR_REPORT                  0x0F14
#define BT_LE_GET_GATT_CAPABILITIES_RSP          BT_LE_GET_GATT_CAPABILITIES