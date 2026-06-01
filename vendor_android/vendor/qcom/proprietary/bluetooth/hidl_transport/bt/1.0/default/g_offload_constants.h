/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

//#ifdef PASSTHROUGH_ENABLED

#define HCI_GRP_VENDOR_SPECIFIC (0x3F << 10) /* 0xFC00 */
#define HCI_VS_TXP_CMD_OPCODE (0x003A | HCI_GRP_VENDOR_SPECIFIC)  /* 0xFC3A */
#define UART_TXP_FLOW_OFF 0x01
#define UART_TXP_FLOW_OFF_RSP_LENGTH 7
#define UART_TXP_SWITCH_TO_IPC 0x03
#define HCI_PERI_BT_SPI_TXP_SWITCH_SUB_OPCODE 0x09
#define HCI_PERI_BT_SPI_TXP_SWITCH_LENGTH 0x09
#define TRANSPORT_ID_SPI 0x02
#define TRANSPORT_ID_UART 0x01
#define STATUS_SUCCESS 0
#define HCI_ARBITER_GLINK_NODE "/dev/glink_pkt_hci_arbiter_chnl"
#define SOCKET_OFFLOAD_GLINK_NODE "/dev/glink_pkt_bt_socket_hal_chnl"
#define GATT_OFFLOAD_GLINK_NODE "/dev/glink_pkt_bt_gatt_offload_chnl"
#define MSG_SIZE_MIN 4
#define MSG_SIZE_MAX 4096
#define PACKET_ENCODED 0x0000
#define PACKET_NOT_ENCODED 0x0001
#define PASSTHROUGH_ENABLE_DISABLE_TIMEOUT (2000)

// Packet Types
#define PASSTHROUGH_ENABLE 0x0F01
#define PASSTHROUGH_DISABLE 0x0F02
#define PASSTHROUGH_ENABLE_DONE_CB 0x0F01
#define PASSTHROUGH_DISABLE_DONE_CB 0x0F02
#define PASSTHROUGH_MODE 0x0001

#define SSR_MODE 0x0002
#define FMD_MODE 0x0003

// Lengths
#define EXTRA_LENGTH_FOR_TOTAL_PACKET_GLINK 7
#define EXTRA_LENGTH_FOR_PAYLOAD_GLINK 1
#define PAYLOAD_ON_GLINK_OFFSET 7
//#define PROTO_NO_PAYLOAD_LENGTH 6
#define PROTO_HEADER_LENGTH 6

#ifdef GOOGLE_OFFLOAD_ENABLED
#define SOCKET_OFFLOAD_GLINK_NODE "/dev/glink_pkt_bt_socket_hal_chnl"
#define OFFLOAD_MODE 0x0000
#endif
#ifdef GATT_OFFLOAD_ENABLED
#define GATT_OFFLOAD_GLINK_NODE "/dev/glink_pkt_bt_gatt_offload_chnl"
#endif

#if defined(PASSTHROUGH_ENABLED) || defined(GOOGLE_OFFLOAD_ENABLED)
#define MSG_SIZE_MAX 4096
#define MSG_SIZE_MIN 4
#endif
//#endif
