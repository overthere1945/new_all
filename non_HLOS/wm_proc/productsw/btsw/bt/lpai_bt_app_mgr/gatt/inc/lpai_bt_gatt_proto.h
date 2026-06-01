/**************************************************************************
 * @file     lpai_bt_app_mgr.h
 * @brief    LPAI BT APP Manager header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef GATT_MGR_PROTO_H
#define GATT_MGR_PROTO_H

#include "lpai_bt_app_mgr_rpc.h"
#include "lpai_bt_gatt_msgs.h"
#include <stdint.h>
#include <stddef.h>
#include "lpai_bt_gatt.pb.h"
#include "bt_utilities.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @def GATT_OFFLOAD_UUID_BYTE_LEN
 * @brief Number of bytes in a UUID.
 */
#define GATT_OFFLOAD_UUID_BYTE_LEN 16

/**
 * @def GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS
 * @brief Maximum number of characteristics per session.
 */
#define GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS 3

/**
 * @struct gatt_char_holder_t
 * @brief Structure holding the properties and UUID of a GATT characteristic.
 * It is used to during proto decoding of Glink message.
 */
typedef struct gatt_char_holder
{
    uint8_t uuid[GATT_OFFLOAD_UUID_BYTE_LEN]; /**< 128-bit UUID for characteristic */
    int32_t properties;                       /**< Properties of characteristic */
    int32_t valueHandle;                      /**< Value handle */
} gatt_char_holder_t;

/**
 * @struct char_list_holder_t
 * @brief Structure holding a list of GATT characteristics.
 * It is used to during proto decoding of Glink message.
 */
typedef struct
{
    gatt_char_holder_t chars[GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS]; /**< Array of characteristics */
    uint8_t num_chars;                                              /**< Number of characteristics */
} char_list_holder_t;

bool char_list_decode_callback(pb_istream_t *stream, const pb_field_t *field, void **arg);

bool decode_gatt_register_service(
    const uint8_t *buffer, size_t buf_len,
    gatt_register_service *service,
    uint8_t *service_uuid_holder,
    char_list_holder_t *char_list_holder);

#ifdef __cplusplus
}
#endif
#endif
