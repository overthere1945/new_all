#ifndef CSR_LOG_TEXT_2_H__
#define CSR_LOG_TEXT_2_H__
/******************************************************************************
Copyright (c) 2011-2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #6 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_util.h"
#include "csr_log_configure.h"
#ifdef MICROSTACK_ENABLE_ISLAND_MODE
#include "micro_msg_diag_service.h"
#endif
#ifdef CSR_TARGET_PRODUCT_VM
#include "platform/csr_hydra_log.h"
#else
/* Note:- Synergy legacy code has dependency on csr_log_text.h file. 
 *        Should be removed once dependency is sorted out */
#include "csr_log_text.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Exception Panic Macros */
#define CSR_EXCEPTION_PANIC_NONE      0
#define CSR_EXCEPTION_PANIC_CRITICAL  1
#define CSR_EXCEPTION_PANIC_ERROR     2
#define CSR_EXCEPTION_PANIC_WARNING   3

/* Warning Exception Panic */
#if !defined(EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE) &&       \
    CSR_EXCEPTION_PANIC >= CSR_EXCEPTION_PANIC_WARNING
#define CSR_WARNING_PANIC()    \
    CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_EXCEPTION, "Panic due to a warning exception")
#else
#define CSR_WARNING_PANIC()
#endif

/* Error Exception Panic */
#if !defined(EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE) &&       \
    CSR_EXCEPTION_PANIC >= CSR_EXCEPTION_PANIC_ERROR
#define CSR_ERROR_PANIC()   \
    CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_EXCEPTION, "Panic due to an error exception")
#else
#define CSR_ERROR_PANIC()
#endif

/* Critical Exception Panic */
#if !defined(EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE) &&       \
    CSR_EXCEPTION_PANIC >= CSR_EXCEPTION_PANIC_CRITICAL
#define CSR_CRITICAL_PANIC()    \
    CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_EXCEPTION, "Panic due to a critical exception")
#else
#define CSR_CRITICAL_PANIC()
#endif

/* Undefine the macros from csr_log_text.h */
#undef CSR_LOG_TEXT_CRITICAL
#undef CSR_LOG_TEXT_CONDITIONAL_CRITICAL
#undef CSR_LOG_TEXT_BUFFER_CRITICAL
#undef CSR_LOG_TEXT_BUFFER_CONDITIONAL_CRITICAL
#undef CSR_LOG_TEXT_ERROR
#undef CSR_LOG_TEXT_CONDITIONAL_ERROR
#undef CSR_LOG_TEXT_BUFFER_ERROR
#undef CSR_LOG_TEXT_BUFFER_CONDITIONAL_ERROR
#undef CSR_LOG_TEXT_WARNING
#undef CSR_LOG_TEXT_CONDITIONAL_WARNING
#undef CSR_LOG_TEXT_BUFFER_WARNING
#undef CSR_LOG_TEXT_BUFFER_CONDITIONAL_WARNING
#undef CSR_LOG_TEXT_INFO
#undef CSR_LOG_TEXT_CONDITIONAL_INFO
#undef CSR_LOG_TEXT_BUFFER_INFO
#undef CSR_LOG_TEXT_BUFFER_CONDITIONAL_INFO
#undef CSR_LOG_TEXT_DEBUG
#undef CSR_LOG_TEXT_CONDITIONAL_DEBUG
#undef CSR_LOG_TEXT_BUFFER_DEBUG
#undef CSR_LOG_TEXT_BUFFER_CONDITIONAL_DEBUG
#undef CSR_LOG_TEXT_ASSERT
#undef CSR_LOG_TEXT_UNHANDLED_PRIMITIVE

/* Log Text Handle */
typedef struct CsrLogTextHandle CsrLogTextHandle;

#define CSR_LOG_ENABLE_REGISTRATION
#define CSR_LOG_ENABLE_STATE_TRANSITION

#if defined(CSR_TARGET_PRODUCT_VM) 
#undef CSR_LOG_ENABLE_REGISTRATION
#undef CSR_LOG_ENABLE_STATE_TRANSITION
#endif

#if defined(CSR_LOG_ENABLE) && defined(CSR_TARGET_PRODUCT_WEARABLE_PLATFORM)
#include "syn_qurt_log.h"
#undef CSR_LOG_ENABLE_STATE_TRANSITION
#undef CSR_LOG_ENABLE_REGISTRATION
#endif

/* Use CSR_LOG_TEXT_REGISTER to register a component before using the text
   logging interface from that component. Initially the handle pointer must be
   initialised to NULL, for the registration to take effect. On return the
   handle pointer will be set to a non-NULL value, and subsequent registrations
   with the same handle pointer will have no effect as the macro will do nothing
   if the handle pointer is not NULL. This allows the call to be placed in code
   paths that may be executed multiple times without causing multiple
   registrations. When the log system is deinitialised, all (registered) handle
   pointers will automatically be reset to NULL. */
#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)

void CsrLogTextRegister2(CsrLogTextHandle **handle,
                         const CsrCharString *originName,
                         CsrUint16 subOriginsCount,
                         const CsrCharString *subOrigins[]);
#define CSR_LOG_TEXT_REGISTER(handle, taskName, subOriginsCount, subOrigins)    \
    do                                                                          \
    {                                                                           \
        if (*(handle) == NULL)                                                  \
        {                                                                       \
            CsrLogTextRegister2(handle, taskName, subOriginsCount, subOrigins); \
        }                                                                       \
    } while(0)
#define CSR_LOG_TEXT_HANDLE_DEFINE(name)    \
    CsrLogTextHandle * name = NULL
#define CSR_LOG_TEXT_HANDLE_DECLARE(name)   \
    extern CsrLogTextHandle * name
#else
#define CSR_LOG_TEXT_REGISTER(handle, taskName, subOriginsCount, subOrigins)
#define CSR_LOG_TEXT_HANDLE_DEFINE(name)    \
    CsrLogTextHandle * name
#define CSR_LOG_TEXT_HANDLE_DECLARE(name)   \
    extern CsrLogTextHandle * name
#endif /* defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION) */

#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextGeneric2(level, handle, subOrigin, ...)                               SYNERGY_DEBUG_LOG(level, handle, subOrigin, __VA_ARGS__)
#define CsrLogTextBufferGeneric2(level, handle, subOrigin, bufferLength, buffer, ...)   SYNERGY_DEBUG_LOG(level, handle, subOrigin, __VA_ARGS__)
#endif

#if defined(CSR_LOG_ENABLE) && defined(CSR_TARGET_PRODUCT_WEARABLE_PLATFORM)

#if (CSR_HOST_PLATFORM == QCC5100_HOST)
#include "qapi_debug_common.h"
#elif (CSR_HOST_PLATFORM == Q6_HOST)
#include "msg_diag_service.h"
#include "diag.h"
#endif

/* Remove this: This is a workaround added to overcome build error seen where 
    MSG_1->XX_MSG_V2_CONST->msg_file refers to __MODULE__ instead __FILENAME__.
    This has to be removed when this is fixed */
#define __MODULE__

/* Enable this macro to route BT Host debug messages to QCLI */
/* #define SYN_MAP_LOG_TO_CUSTOM */
#undef MSG_0
#define MSG_0(...)  MSG(__VA_ARGS__)

/* Construction Macros */
#define SLIDE_SEQUENCE_FORWARD_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, N,...) N
//#define REVERSE_SEQUENCE_N()                9,8,7,6,5,4,3,2,1,0
/* Note: there are two spaces left after "_1, , " , which is to construct "MSG" 
    for zero arguments i.e. MSG(ssid, mask, fmt). Otherwise we would want to contruct MSG_1, MSG_2, etc... */
#define REVERSE_SEQUENCE_N()                        _8,_7,_6,_5,_4,_3,_2,_1, , 
#define NUMBER_OF_ARGS(...)                         SLIDE_SEQUENCE_FORWARD_N(__VA_ARGS__)
#define COUNT_ARGS(...)                             NUMBER_OF_ARGS(__VA_ARGS__,REVERSE_SEQUENCE_N())

#define TOKENPASTE(x,y)                             x##y
#ifdef SYN_MAP_LOG_TO_CUSTOM
#define TOKENIZE_MSG(_N)                            TOKENPASTE(DMSG, _N)
#define SYN_LOG_HEX_DUMP(len, data)                 DMSG_HEXDUMP(len, data)
#else
#if (CSR_HOST_PLATFORM == QCC5100_HOST)
#define TOKENIZE_MSG(_N)                            TOKENPASTE(QAPI_MSG, _N)
#else
#ifdef MICROSTACK_ENABLE_ISLAND_MODE
#define TOKENIZE_MSG(_N)                            MICRO_MSG
#else
#define TOKENIZE_MSG(_N)                            TOKENPASTE(MSG, _N)
#endif
#endif
#define SYN_LOG_HEX_DUMP(len, data)
#endif
#define CREATE_MSG(...)                             TOKENIZE_MSG(COUNT_ARGS(__VA_ARGS__))
#define CONSTRUCT_MSG(...)                          CREATE_MSG(__VA_ARGS__)(MSG_SSID_BT, MSG_LEGACY_LOW, __VA_ARGS__)
#define CONSTRUCT_LOW_MSG(...)                      CREATE_MSG(__VA_ARGS__)(MSG_SSID_BT, MSG_LEGACY_LOW, __VA_ARGS__)
#define CONSTRUCT_MED_MSG(...)                      CREATE_MSG(__VA_ARGS__)(MSG_SSID_BT, MSG_LEGACY_MED, __VA_ARGS__)
#define CONSTRUCT_HIGH_MSG(...)                     CREATE_MSG(__VA_ARGS__)(MSG_SSID_BT, MSG_LEGACY_HIGH, __VA_ARGS__)
#define CONSTRUCT_ERROR_MSG(...)                    CREATE_MSG(__VA_ARGS__)(MSG_SSID_BT, MSG_LEGACY_ERROR, __VA_ARGS__)
#define CONSTRUCT_FATAL_MSG(...)                    CREATE_MSG(__VA_ARGS__)(MSG_SSID_BT, MSG_LEGACY_FATAL, __VA_ARGS__)

#define SYNERGY_LOG_LOW(x, y, ...)                  CONSTRUCT_LOW_MSG(#x #y " "  __VA_ARGS__)
#define SYNERGY_LOG_MED(x, y, ...)                  CONSTRUCT_MED_MSG(#x #y " "  __VA_ARGS__)
#define SYNERGY_LOG_HIGH(x, y, ...)                 CONSTRUCT_HIGH_MSG(#x #y " "  __VA_ARGS__)
#define SYNERGY_LOG_ERROR(x, y, ...)                CONSTRUCT_ERROR_MSG(#x #y " " __VA_ARGS__)
#define SYNERGY_LOG_FATAL(x, y, ...)                CONSTRUCT_FATAL_MSG(#x #y " " __VA_ARGS__)

#define CSR_LOG_BGINT_NAME(bgname)                  CONSTRUCT_MSG("Bg Int Name " bgname)
#define CSR_LOG_BGINT_REG(irq)                      CONSTRUCT_MSG("Bg Int Register %d" , irq);
#define CSR_LOG_BGINT_START(irq)                    CONSTRUCT_MSG("Bg Int Start %d", irq)
#define CSR_LOG_BGINT_DONE(irq)                     CONSTRUCT_MSG("Bg Int Done %d", irq)
#define CSR_LOG_BGINT_UNREG(irq)                    CONSTRUCT_MSG("Bg Int Unregister %d", irq)
#ifndef SYN_MAP_LOG_TO_CUSTOM
#define CSR_LOG_BGINT_SET(irq)                      CONSTRUCT_MSG("Bg Int Set %d", irq)
#else
/* In custom logging generally the log apis would be mapped to platform provide 
    uart/console print apis involves hardware, this might be a problem if synergy logging is 
    invoked via a ISR routine causing undefined behavior. */
#define CSR_LOG_BGINT_SET(irq)
#endif /* ifndef SYN_MAP_LOG_TO_CUSTOM */
void SynLogSchedMsg(CsrBool isPut, CsrSchedQid src, CsrSchedQid dst, CsrUint16 mi, void *mv);
#define CSR_LOG_PUT_MSG(dst, mi, mv)                SynLogSchedMsg(TRUE, CsrSchedTaskQueueGet(), dst, mi, mv)
#define CSR_LOG_GET_MSG(src, dst, mi, mv)           SynLogSchedMsg(FALSE, src, dst, mi, mv)
#define CSR_LOG_TIMER_IN(tid, requested_delay)      CONSTRUCT_MSG("Timer In: %x: %d: %d", CsrSchedTaskQueueGet(), tid, requested_delay)
#define CSR_LOG_TIMER_FIRE(task, tid)               CONSTRUCT_MSG("Timer Fire: %x: %d", task, tid)
#define CSR_LOG_TIMER_DONE(task, tid)               CONSTRUCT_MSG("Timer Done: %x: %d", task, tid)
#define CSR_LOG_TIMER_CANCEL(tid)                   CONSTRUCT_MSG("Timer Cancel: %x: %d", CsrSchedTaskQueueGet(), tid)
#define CSR_LOG_FSM(...)                            CONSTRUCT_MSG(__VA_ARGS__)
void SynLogBluestackWarning(CsrUint32 error_code, CsrUint32 line);
void SynLogBluestackDebout(CsrUint32 error_code, CsrUint32 line);
void SynLogEnableSnoop(CsrBool enable);
/* configures the flags to enable or disable acl rx and enhanced logging */
void SynLogConfigureSnoop(CsrUint16 flag); 

#define CsrLogTextFsm(...)                          SYNERGY_LOG_HIGH(__VA_ARGS__)
#define CSR_LOG_TEXT_FSM(handle_subOrigin_formatString_varargs)  \
                            CsrLogTextFsm handle_subOrigin_formatString_varargs

#define SYNERGY_LOG_DOWNSTREAM_EXT_MSG(mi, type)    CONSTRUCT_MED_MSG("==> group: 0x%04X, id: 0x%04X ", mi, type)
#define SYNERGY_LOG_UPSTREAM_EXT_MSG(mi, type)      CONSTRUCT_MED_MSG("<== group: 0x%04X, id: 0x%04X ", mi, type)
#else
#define CSR_LOG_BGINT_NAME(bgname)
#define CSR_LOG_BGINT_REG(irq)
#define CSR_LOG_BGINT_START(irq)
#define CSR_LOG_BGINT_DONE(irq)
#define CSR_LOG_BGINT_UNREG(irq)
#define CSR_LOG_BGINT_SET(irq)
#define SynLogSchedMsg(isPut, dst, mi, mv)
#define CSR_LOG_PUT_MSG(dst, mi, mv)
#define CSR_LOG_GET_MSG(src, dst, mi, mv)
#define CSR_LOG_TIMER_IN(tid, requested_delay)
#define CSR_LOG_TIMER_FIRE(task, tid)
#define CSR_LOG_TIMER_DONE(task, tid)
#define CSR_LOG_TIMER_CANCEL(tid)
#define SynLogBluestackWarning(error_code, line)
#define SynLogBluestackDebout(error_code, line)
#define SynLogEnableSnoop(enable)
#define SYNERGY_LOG_DOWNSTREAM_EXT_MSG()
#define SYNERGY_LOG_UPSTREAM_EXT_MSG(mi, type)
#define SYN_LOG_HEX_DUMP(len, data)
#endif

/* CRITICAL: Conditions that are threatening to the integrity/stability of the
   system as a whole. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_CRITICAL_DISABLE)
#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextCritical2(...)            CsrLogTextGeneric2(SYNERGY_DEBUG_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#define CsrLogTextBufferCritical2(...)      CsrLogTextBufferGeneric2(SYNERGY_DEBUG_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#elif defined(CSR_TARGET_PRODUCT_WEARABLE_PLATFORM)
#define CsrLogTextCritical2(...)            SYNERGY_LOG_FATAL(__VA_ARGS__)
#else
void CsrLogTextCritical2(CsrLogTextHandle *handle,
                         CsrUint16 subOrigin,
                         const CsrCharString *formatString, ...);
void CsrLogTextBufferCritical2(CsrLogTextHandle *handle,
                               CsrUint16 subOrigin,
                               CsrSize bufferLength,
                               const void *buffer,
                               const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_CRITICAL(handle_subOrigin_formatString_varargs)    \
    do                                                                  \
    {                                                                   \
        CsrLogTextCritical2 handle_subOrigin_formatString_varargs;      \
        CSR_CRITICAL_PANIC();                                           \
    } while (0)
#define CSR_LOG_TEXT_CONDITIONAL_CRITICAL(condition, logtextargs)       \
    do                                                                  \
    {                                                                   \
        if (condition)                                                  \
        {                                                               \
            CSR_LOG_TEXT_CRITICAL(logtextargs);                         \
        }                                                               \
    } while(0)
#ifndef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
#define CSR_LOG_TEXT_BUFFER_CRITICAL(handle_subOrigin_length_buffer_formatString_varargs)  \
    CsrLogTextBufferCritical2 handle_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_CRITICAL(condition, logtextbufferargs)  \
    do                                                                          \
    {                                                                           \
        if (condition)                                                          \
        {                                                                       \
            CSR_LOG_TEXT_BUFFER_CRITICAL(logtextbufferargs);                    \
        }                                                                       \
    } while(0)
#endif /* CSR_TARGET_PRODUCT_WEARABLE_PLATFORM */
#else
#define CSR_LOG_TEXT_CRITICAL(handle_subOrigin_formatString_varargs)   \
    CSR_CRITICAL_PANIC()
#define CSR_LOG_TEXT_CONDITIONAL_CRITICAL(condition, logtextargs)      \
    do                                                                 \
    {                                                                  \
        if (condition)                                                 \
        {                                                              \
            CSR_CRITICAL_PANIC();                                      \
        }                                                              \
    } while(0)
#define CSR_LOG_TEXT_BUFFER_CRITICAL(handle_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_CRITICAL(condition, logtextbufferargs)
#endif

/* ERROR: Malfunction of a component rendering it unable to operate correctly,
   causing lack of functionality but not loss of system integrity/stability. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_ERROR_DISABLE)
#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextError2(...)               CsrLogTextGeneric2(SYNERGY_DEBUG_LOG_LEVEL_ERROR, __VA_ARGS__)
#define CsrLogTextBufferError2(...)         CsrLogTextBufferGeneric2(SYNERGY_DEBUG_LOG_LEVEL_ERROR, __VA_ARGS__)
#elif defined(CSR_TARGET_PRODUCT_WEARABLE_PLATFORM)
#define CsrLogTextError2(...)               SYNERGY_LOG_ERROR(__VA_ARGS__)
#else
void CsrLogTextError2(CsrLogTextHandle *handle,
                      CsrUint16 subOrigin,
                      const CsrCharString *formatString, ...);
void CsrLogTextBufferError2(CsrLogTextHandle *handle,
                            CsrUint16 subOrigin,
                            CsrSize bufferLength,
                            const void *buffer,
                            const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_ERROR(handle_subOrigin_formatString_varargs)   \
    do                                                              \
    {                                                               \
        CsrLogTextError2 handle_subOrigin_formatString_varargs;     \
        CSR_ERROR_PANIC();                                          \
    } while (0)
#define CSR_LOG_TEXT_CONDITIONAL_ERROR(condition, logtextargs)      \
    do                                                              \
    {                                                               \
        if (condition)                                              \
        {                                                           \
            CSR_LOG_TEXT_ERROR(logtextargs);                        \
        }                                                           \
    } while(0)
#ifndef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
#define CSR_LOG_TEXT_BUFFER_ERROR(handle_subOrigin_length_buffer_formatString_varargs)  \
    CsrLogTextBufferError2 handle_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_ERROR(condition, logtextbufferargs)   \
    do                                                                        \
    {                                                                         \
        if (condition)                                                        \
        {                                                                     \
            CSR_LOG_TEXT_BUFFER_ERROR(logtextbufferargs);                     \
        }                                                                     \
    } while(0)
#endif /* CSR_TARGET_PRODUCT_WEARABLE_PLATFORM */

#else
#define CSR_LOG_TEXT_ERROR(handle_subOrigin_formatString_varargs)  \
    CSR_ERROR_PANIC()
#define CSR_LOG_TEXT_CONDITIONAL_ERROR(condition, logtextargs)     \
    do                                                             \
    {                                                              \
        if (condition)                                             \
        {                                                          \
            CSR_ERROR_PANIC();                                     \
        }                                                          \
    } while(0)
#define CSR_LOG_TEXT_BUFFER_ERROR(handle_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_ERROR(condition, logtextbufferargs)
#endif

/* WARNING: Conditions that are unexpected and indicative of possible problems
   or violations of specifications, where the result of such deviations does not
   lead to malfunction of the component. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_WARNING_DISABLE)
#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextWarning2(...)             CsrLogTextGeneric2(SYNERGY_DEBUG_LOG_LEVEL_WARN, __VA_ARGS__)
#define CsrLogTextBufferWarning2(...)       CsrLogTextBufferGeneric2(SYNERGY_DEBUG_LOG_LEVEL_WARN, __VA_ARGS__)
#elif defined(CSR_TARGET_PRODUCT_WEARABLE_PLATFORM)
#define CsrLogTextWarning2(...)             SYNERGY_LOG_HIGH(__VA_ARGS__)
#else
void CsrLogTextWarning2(CsrLogTextHandle *handle,
                        CsrUint16 subOrigin,
                        const CsrCharString *formatString, ...);
void CsrLogTextBufferWarning2(CsrLogTextHandle *handle,
                              CsrUint16 subOrigin,
                              CsrSize bufferLength,
                              const void *buffer,
                              const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_WARNING(handle_subOrigin_formatString_varargs) \
    do                                                              \
    {                                                               \
        CsrLogTextWarning2 handle_subOrigin_formatString_varargs;   \
        CSR_WARNING_PANIC();                                        \
    } while (0)
#define CSR_LOG_TEXT_CONDITIONAL_WARNING(condition, logtextargs)    \
    do                                                              \
    {                                                               \
        if (condition)                                              \
        {                                                           \
            CSR_LOG_TEXT_WARNING(logtextargs);                      \
        }                                                           \
    } while(0)
#ifndef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
#define CSR_LOG_TEXT_BUFFER_WARNING(handle_subOrigin_length_buffer_formatString_varargs)  \
    CsrLogTextBufferWarning2 handle_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_WARNING(condition, logtextbufferargs) \
    do                                                                        \
    {                                                                         \
        if (condition)                                                        \
        {                                                                     \
            CSR_LOG_TEXT_BUFFER_WARNING(logtextbufferargs);                   \
        }                                                                     \
    } while(0)
#endif /* CSR_TARGET_PRODUCT_WEARABLE_PLATFORM */

#else
#define CSR_LOG_TEXT_WARNING(handle_subOrigin_formatString_varargs)  \
    CSR_WARNING_PANIC()
#define CSR_LOG_TEXT_CONDITIONAL_WARNING(condition, logtextargs)     \
    do                                                               \
    {                                                                \
        if (condition)                                               \
        {                                                            \
            CSR_WARNING_PANIC();                                     \
        }                                                            \
    } while(0)
#define CSR_LOG_TEXT_BUFFER_WARNING(handle_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_WARNING(condition, logtextbufferargs)
#endif

/* INFO: Important events that may aid in determining the conditions under which
   the more severe conditions are encountered. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_INFO_DISABLE)
#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextInfo2(...)                CsrLogTextGeneric2(SYNERGY_DEBUG_LOG_LEVEL_INFO, __VA_ARGS__)
#define CsrLogTextBufferInfo2(...)          CsrLogTextBufferGeneric2(SYNERGY_DEBUG_LOG_LEVEL_INFO, __VA_ARGS__)
#elif defined(CSR_TARGET_PRODUCT_WEARABLE_PLATFORM)
#define CsrLogTextInfo2(...)                SYNERGY_LOG_HIGH(__VA_ARGS__)
#else
void CsrLogTextInfo2(CsrLogTextHandle *handle,
                     CsrUint16 subOrigin,
                     const CsrCharString *formatString, ...);
void CsrLogTextBufferInfo2(CsrLogTextHandle *handle,
                           CsrUint16 subOrigin,
                           CsrSize bufferLength,
                           const void *buffer,
                           const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_INFO(handle_subOrigin_formatString_varargs)  \
    CsrLogTextInfo2 handle_subOrigin_formatString_varargs
#define CSR_LOG_TEXT_CONDITIONAL_INFO(condition, logtextargs)                 \
    do                                                                        \
    {                                                                         \
        if (condition)                                                        \
        {                                                                     \
            CSR_LOG_TEXT_INFO(logtextargs);                                   \
        }                                                                     \
    } while(0)
#ifndef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
#define CSR_LOG_TEXT_BUFFER_INFO(handle_subOrigin_length_buffer_formatString_varargs)  \
    CsrLogTextBufferInfo2 handle_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_INFO(condition, logtextbufferargs)    \
    do                                                                        \
    {                                                                         \
        if (condition)                                                        \
        {                                                                     \
            CSR_LOG_TEXT_BUFFER_INFO(logtextbufferargs);                      \
        }                                                                     \
    } while(0)
#endif /* CSR_TARGET_PRODUCT_WEARABLE_PLATFORM */

#else
#define CSR_LOG_TEXT_INFO(handle_subOrigin_formatString_varargs)
#define CSR_LOG_TEXT_CONDITIONAL_INFO(condition, logtextargs)
#define CSR_LOG_TEXT_BUFFER_INFO(handle_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_INFO(condition, logtextbufferargs)
#endif

/* DEBUG: Similar to INFO, but dedicated to events that occur more frequently. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_DEBUG_DISABLE)
#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextDebug2(...)               CsrLogTextGeneric2(SYNERGY_DEBUG_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define CsrLogTextBufferDebug2(...)         CsrLogTextBufferGeneric2(SYNERGY_DEBUG_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#elif defined(CSR_TARGET_PRODUCT_WEARABLE_PLATFORM)
#define CsrLogTextDebug2(...)               SYNERGY_LOG_LOW(__VA_ARGS__)
#else
void CsrLogTextDebug2(CsrLogTextHandle *handle,
                      CsrUint16 subOrigin,
                      const CsrCharString *formatString, ...);
void CsrLogTextBufferDebug2(CsrLogTextHandle *handle,
                            CsrUint16 subOrigin,
                            CsrSize bufferLength,
                            const void *buffer,
                            const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_DEBUG(handle_subOrigin_formatString_varargs)  \
    CsrLogTextDebug2 handle_subOrigin_formatString_varargs
#define CSR_LOG_TEXT_CONDITIONAL_DEBUG(condition, logtextargs)     \
    do                                                             \
    {                                                              \
        if (condition)                                             \
        {                                                          \
            CSR_LOG_TEXT_DEBUG(logtextargs);                       \
        }                                                          \
    } while(0)
#ifndef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
#define CSR_LOG_TEXT_BUFFER_DEBUG(handle_subOrigin_length_buffer_formatString_varargs)  \
    CsrLogTextBufferDebug2 handle_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_DEBUG(condition, logtextbufferargs)   \
    do                                                                        \
    {                                                                         \
        if (condition)                                                        \
        {                                                                     \
            CSR_LOG_TEXT_BUFFER_DEBUG(logtextbufferargs);                     \
        }                                                                     \
    } while(0)
#endif /* CSR_TARGET_PRODUCT_WEARABLE_PLATFORM */

#else
#define CSR_LOG_TEXT_DEBUG(handle_subOrigin_formatString_varargs)
#define CSR_LOG_TEXT_CONDITIONAL_DEBUG(condition, logtextargs)
#define CSR_LOG_TEXT_BUFFER_DEBUG(handle_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_DEBUG(condition, logtextbufferargs)
#endif

#ifdef CSR_TARGET_PRODUCT_VM

/* CSR_LOG_TEXT_ASSERT (CRITICAL) */
#define CSR_LOG_TEXT_ASSERT(origin,                                           \
                            suborigin,                                        \
                            condition)                                        \
    do {                                                                      \
        SYNERGY_HYDRA_LOG_STRING(cond, SYNERGY_FMT(#condition, bonus_arg));   \
        SYNERGY_HYDRA_LOG_STRING(the_file, SYNERGY_FMT(__FILE__, bonus_arg)); \
        CSR_LOG_TEXT_CONDITIONAL_CRITICAL(!(condition),                       \
                                          (origin,                            \
                                          suborigin,                          \
                                          "Assertion \"%s\" failed at %s:%u", \
                                          cond,                               \
                                          the_file,                           \
                                          __LINE__));                         \
    } while(0)

/* CSR_LOG_TEXT_UNHANDLED_PRIM (CRITICAL) */
#define CSR_LOG_TEXT_UNHANDLED_PRIMITIVE(origin,                              \
                                         suborigin,                           \
                                         primClass,                           \
                                         primType)                            \
    do {                                                                      \
        SYNERGY_HYDRA_LOG_STRING(the_file, SYNERGY_FMT(__FILE__, bonus_arg)); \
        CSR_LOG_TEXT_CRITICAL((origin,                                        \
                              suborigin,                                      \
                              "Unhandled primitive 0x%04X:0x%04X at %s:%u",   \
                              primClass,                                      \
                              primType,                                       \
                              the_file,                                       \
                              __LINE__));                                     \
    } while(0)

#elif defined(CSR_TARGET_PRODUCT_WEARABLE_PLATFORM)
#define CSR_LOG_TEXT_ASSERT(origin,                                           \
                            suborigin,                                        \
                            condition)                                        \
    CSR_LOG_TEXT_CONDITIONAL_CRITICAL(!(condition),                           \
                                      (origin,                                \
                                      suborigin,                              \
                                      "Condition Asserted"))

#else

/* CSR_LOG_TEXT_ASSERT (CRITICAL) */
#define CSR_LOG_TEXT_ASSERT(origin,                                           \
                            suborigin,                                        \
                            condition)                                        \
    CSR_LOG_TEXT_CONDITIONAL_CRITICAL(!(condition),                           \
                                      (origin,                                \
                                      suborigin,                              \
                                      "Assertion \"%s\" failed at %s:%u",     \
                                      #condition,                             \
                                      __FILE__,                               \
                                      __LINE__))

/* CSR_LOG_TEXT_UNHANDLED_PRIM (CRITICAL) */
#define CSR_LOG_TEXT_UNHANDLED_PRIMITIVE(origin,                              \
                                         suborigin,                           \
                                         primClass,                           \
                                         primType)                            \
    CSR_LOG_TEXT_CRITICAL((origin,                                            \
                          suborigin,                                          \
                          "Unhandled primitive 0x%04X:0x%04X at %s:%u",       \
                          primClass,                                          \
                          primType,                                           \
                          __FILE__,                                           \
                          __LINE__))

#endif /* CSR_TARGET_PRODUCT_VM */

/*---------------------------------*/
/*    State transition logging     */
/*---------------------------------*/
typedef CsrUint8 bitmask8_t;
typedef CsrUint16 bitmask16_t;
typedef CsrUint32 bitmask32_t;

#define CSR_LOG_STATE_TRANSITION_MASK_FSM_NAME          (0x001)
#define CSR_LOG_STATE_TRANSITION_MASK_NEXT_STATE        (0x002)
#define CSR_LOG_STATE_TRANSITION_MASK_NEXT_STATE_STR    (0x004)
#define CSR_LOG_STATE_TRANSITION_MASK_PREV_STATE        (0x008)
#define CSR_LOG_STATE_TRANSITION_MASK_PREV_STATE_STR    (0x010)
#define CSR_LOG_STATE_TRANSITION_MASK_EVENT             (0x020)
#define CSR_LOG_STATE_TRANSITION_MASK_EVENT_STR         (0x040)

#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_STATE_TRANSITION)
void CsrLogStateTransition(CsrLogTextHandle *logTextHandle,
                           CsrUint16 subOrigin,
                           bitmask16_t mask,
                           CsrUint32 identifier,
                           const CsrCharString *fsm_name,
                           CsrUint32 prev_state,
                           const CsrCharString *prev_state_str,
                           CsrUint32 in_event,
                           const CsrCharString *in_event_str,
                           CsrUint32 next_state,
                           const CsrCharString *next_state_str,
                           CsrUint32 line,
                           const CsrCharString *file);
#else
#define CsrLogStateTransition(logTextHandle,        \
                              subOrigin,            \
                              mask,                 \
                              identifier,           \
                              fsmName,              \
                              prevState,            \
                              prevStateStr,         \
                              inEvent,              \
                              inEventStr,           \
                              nextState,            \
                              nextStateStr,         \
                              line,                 \
                              file)
#endif

/*---------------------------------*/
/*       Exception logging         */
/*---------------------------------*/
#ifdef CSR_TARGET_PRODUCT_VM

#define CsrGeneralException(theHandle,                                                              \
                            theSubOrigin,                                                           \
                            theEventClass,                                                          \
                            thePrimType,                                                            \
                            theState,                                                               \
                            theMessage)                                                             \
    do {                                                                                            \
        SYNERGY_HYDRA_LOG_STRING(the_msg, SYNERGY_FMT(theMessage, bonus_arg));                      \
        SYNERGY_HYDRA_LOG_STRING(the_file, SYNERGY_FMT(__FILE__, bonus_arg));                       \
        CSR_LOG_TEXT_WARNING((theHandle,                                                            \
                             theSubOrigin,                                                          \
                             "Exception: EventClass=0x%X, PrimType=0x%X, State=0x%X, %s [%s:%d]",   \
                             theEventClass,                                                         \
                             thePrimType,                                                           \
                             theState,                                                              \
                             the_msg, the_file, __LINE__));                                         \
    } while(0)
#elif defined(CSR_TARGET_PRODUCT_WEARABLE_PLATFORM)
#define CsrGeneralException(theHandle,                                                              \
                            theSubOrigin,                                                           \
                            theEventClass,                                                          \
                            thePrimType,                                                            \
                            theState,                                                               \
                            theMessage)                                                             \
    do {                                                                                            \
        CSR_LOG_TEXT_WARNING((theHandle,                                                            \
                             theSubOrigin,                                                          \
                             "EXP: EVC=%X PT=%X ST=%X " #theMessage, \
                             theEventClass,                                                         \
                             thePrimType,                                                           \
                             theState));                                                             \
    } while(0)


#else /* CSR_TARGET_PRODUCT_VM */

#define CsrGeneralException(theHandle,                                       \
                            theSubOrigin,                                    \
                            theEventClass,                                   \
                            thePrimType,                                     \
                            theState,                                        \
                            theMessage)                                      \
    CSR_LOG_TEXT_WARNING((theHandle,                                         \
                         theSubOrigin,                                       \
                         "Exception: EventClass=0x%X, PrimType=0x%X, "       \
                         "State=0x%X, %s [%s:%u]",                           \
                         theEventClass,                                      \
                         thePrimType,                                        \
                         theState,                                           \
                         theMessage ? theMessage : "",                       \
                         CsrGetBaseName(__FILE__),                           \
                         __LINE__))

#endif /* CSR_TARGET_PRODUCT_VM */

#ifdef __cplusplus
}
#endif

#endif
