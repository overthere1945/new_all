/*******************************************************************************

Copyright (C) 2009 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef _RFCOMM_H_
#define _RFCOMM_H_


#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include INC_DIR(mblk,mblk.h)

#ifdef __cplusplus
extern "C" {
#endif


void rfc_init(void **control_data);
#ifdef ENABLE_SHUTDOWN
void rfc_deinit(void **control_data);
#endif
void rfc_iface_handler(void **control_data);

typedef uint8_t  fsm_state_t;
#define RFC_ST_DISCONNECTED          ((fsm_state_t) 0)
#define RFC_ST_CONNECTED             ((fsm_state_t) 1)
#define RFC_ST_DISCONNECTING         ((fsm_state_t) 2)


#ifdef __cplusplus
}
#endif

#define RFCOMM_LOG_ENABLE

#if defined(RFCOMM_LOG_ENABLE)
#define RFC_LOG_INFO(...) CSR_LOG_TEXT_INFO((RFCOMM , 0, __VA_ARGS__))
#define RFC_LOG_DEBUG(...) CSR_LOG_TEXT_INFO((RFCOMM , 0, __VA_ARGS__))
#define RFC_LOG_WARNING(...)  CSR_LOG_TEXT_WARNING((RFCOMM , 0, __VA_ARGS__))
#define RFC_LOG_ERROR(...)  CSR_LOG_TEXT_ERROR((RFCOMM , 0, __VA_ARGS__))
#define RFC_PANIC(code, str)  {CsrPanic(CSR_TECH_BT, code, str);} 
#else
#define RFC_LOG_INFO(...)
#define RFC_LOG_DEBUG(...)
#define RFC_LOG_WARNING(...)
#define RFC_LOG_ERROR(...)
#define RFC_PANIC(...)  {CsrPanic(CSR_TECH_BT, code, str);} 
#endif

#endif /* _RFCOMM_H_ */
