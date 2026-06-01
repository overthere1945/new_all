/******************************************************************************
Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #5 $
******************************************************************************/

#ifndef SYN_QURT_LOG_H__
#define SYN_QURT_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* #define SYN_LOG_EXT_PRIM */

/* Undefine this macro to route the logs to QCLI */
#define DUMP_BT_SNOOP_FMT_TO_DBG_UART

void DMSG(uint16_t ssid, uint16_t mask, char* fmt);
void DMSG_1(uint16_t ssid, uint16_t mask, char* fmt, int x1);
void DMSG_2(uint16_t ssid, uint16_t mask, char* fmt, int x1, int x2);
void DMSG_3(uint16_t ssid, uint16_t mask, char* fmt, int x1, int x2, int x3);
void DMSG_4(uint16_t ssid, uint16_t mask, char* fmt, int x1, int x2, int x3, int x4);

#ifdef SYN_QURT_LOG_EXT
void DMSG_5(uint16_t ssid, uint16_t mask, char* fmt, int x1, int x2, int x3, int x4, int x5);
void DMSG_6(uint16_t ssid, uint16_t mask, char* fmt, int x1, int x2, int x3, int x4, int x5, int x6);
void DMSG_7(uint16_t ssid, uint16_t mask, char* fmt, int x1, int x2, int x3, int x4, int x5, int x6, int x7);
void DMSG_8(uint16_t ssid, uint16_t mask, char* fmt, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8);
void DMSG_9(uint16_t ssid, uint16_t mask, char* fmt, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8, int x9);
#else
#define DMSG_5(ssid, mask, fmt, x1, x2, x3, x4, x5)
#define DMSG_6(ssid, mask, fmt, x1, x2, x3, x4, x5, x6)
#define DMSG_7(ssid, mask, fmt, x1, x2, x3, x4, x5, x6, x7)
#define DMSG_8(ssid, mask, fmt, x1, x2, x3, x4, x5, x6, x7, x8)
#define DMSG_9(ssid, mask, fmt, x1, x2, x3, x4, x5, x6, x7, x8, x9)
#endif 

void DMSG_HEXDUMP(uint16_t len, uint8_t *buffer);

/* SYN LOGGING HCI PKT FLAG - Bit2:(ACL_ENH_RX) Bit3:(ACL_RX_LEN) 
 * the other bits will be ignored */
#define SYN_HCI_ACL_RX_ENH_PKT_FLAG         (uint16_t)0x02
#define SYN_HCI_ACL_RX_INC_FULL_PAYLOAD_LEN (uint16_t)0x04

#ifdef SYN_DEBUG_LOGS_BUFFER_ENABLE
void SynStoreLogInfo(CsrUint8 isPut, CsrPrim msgType, CsrUint16 mi, CsrSchedQid src, CsrSchedQid dst);
void SynLogInit(void);
void SynLogDeinit(void);
#else
#define SynStoreLogInfo(isPut, msgType, mi, src, dst)
#define SynLogInit()
#define SynLogDeinit()
#endif

#ifdef __cplusplus
}
#endif

#endif /* SYN_QURT_LOG_H__ */
