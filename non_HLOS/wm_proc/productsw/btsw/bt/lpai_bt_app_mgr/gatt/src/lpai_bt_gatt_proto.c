/**************************************************************************
 * @file     lpai_bt_app_mgr.h
 * @brief    LPAI BT APP Manager header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#include "lpai_bt_gatt_proto.h"
/**
 * @brief Callback for decoding GATT characteristic lists using nanopb.
 *
 * @param stream Protobuf stream context.
 * @param field  Field context.
 * @param arg    Pointer to char_list_holder_t context.
 * @return true if decoding succeeded, false otherwise.
 */
bool char_list_decode_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    char_list_holder_t *holder = (char_list_holder_t *)(*arg);
    if (holder->num_chars >= GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS) {
        printk("char_list_decode_callback: num chars exceeded\n");
        return false;
    }

    /* Decode one element */
    GattCharacteristic gatt_char = GattCharacteristic_init_zero;
    uint8_t char_uuid[GATT_OFFLOAD_UUID_BYTE_LEN] = {0};
    gatt_char.uuid.uuid.arg = &char_uuid;
    gatt_char.uuid.uuid.funcs.decode = decode_bytes;
    if (!pb_decode(stream, GattCharacteristic_fields, &gatt_char)) {
        printk("char_list_decode_callback: Decode failed: %s\n", PB_GET_ERROR(stream));
        return false;
    }

    holder->chars[holder->num_chars].properties = gatt_char.properties;
    holder->chars[holder->num_chars].valueHandle = gatt_char.valueHandle;
    memscpy(&holder->chars[holder->num_chars].uuid[0], GATT_OFFLOAD_UUID_BYTE_LEN, &char_uuid[0], GATT_OFFLOAD_UUID_BYTE_LEN);

    holder->num_chars++;
    return true;
}

/**
 * @brief Decode and process a GATT "register service" request from application layer.
 *
 * @param buffer            Raw request buffer.
 * @param buf_len           Buffer length.
 * @param service           Decoded structure output.
 * @param service_uuid_holder UUID buffer for output.
 * @param char_list_holder  Pointer to decoded characteristics data.
 * @return true if decoding succeeded, false otherwise.
 */

bool decode_gatt_register_service(
    const uint8_t *buffer, size_t buf_len,
    gatt_register_service *service,
    uint8_t *service_uuid_holder,
    char_list_holder_t *char_list_holder)
{
    /* Assign uuid callback/context */
    service->uuid.uuid.funcs.decode = decode_bytes;
    service->uuid.uuid.arg = service_uuid_holder;

    /* Assign gattCharacteristic list callback/context */
    service->gattCharacteristic.funcs.decode = &char_list_decode_callback;
    service->gattCharacteristic.arg = char_list_holder;
    char_list_holder->num_chars = 0; // Initialize

    /* Decode */
    pb_istream_t stream = pb_istream_from_buffer(buffer, buf_len);
    if (!pb_decode(&stream, gatt_register_service_fields, service))
    {
        printk("Decode failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
#ifdef DEBUG_BUILD_LOG
    log_register_service(service, service_uuid_holder, char_list_holder);
#endif
    return true;
}