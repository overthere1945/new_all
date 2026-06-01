/**************************************************************************
 * @file     lpai_bt_app_mgr_rpc.h
 * @brief    LPAI BT APP Manager RPC header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef APP_MGR_RPC
#define APP_MGR_RPC
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif


#define APP_MGR_RPC_HDR_SIZE sizeof(app_mgr_rpc_header_t)
extern uint8_t app_mgr_transport_buf[1024];

/**
 * @struct gatt_mgr_rpc_header
 * @brief Structure for RPC header.
 */
typedef struct app_mgr_rpc_header {
    uint16_t command_type;    /**< Type of command */
    uint16_t opcode;          /**< Operation code */
    uint16_t message_format;  /**< Format of the message */
    uint16_t data_len;        /**< Length of the data */
    uint64_t endpoint_id;     /**< ID of the endpoint */
    uint8_t data[];           /**< Data payload */
} app_mgr_rpc_header_t;

app_mgr_rpc_header_t *
app_mgr_rpc_make_header(uint16_t command_type, uint16_t opcode, uint16_t message_format,
                          uint64_t ep_id, int extra_size, int *out_size);



app_mgr_rpc_header_t *app_mgr_rpc_get_default_header(void);
void *app_mgr_rpc_get_empty_msg(void);
int app_mgr_rpc_send_endpt_msg_adsp(uint64_t endPointId, uint16_t opcode , uint16_t dataLen, bool proto_enabled);
//int lpai_bt_appmgr_send_endpt_discovery_response_adsp(uint16_t dataLen , const void *appDataBuf);
//int lpai_bt_appmgr_send_microapp_msg_adsp(uint16_t opcode , uint16_t dataLen , void *message);
void app_mgr_rpc_adsp_evt_handler(const uint8_t* dataBuff, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
