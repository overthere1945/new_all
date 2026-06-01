#ifndef CSR_LOG_TEXT_H__
#define CSR_LOG_TEXT_H__
/*****************************************************************************
 Copyright (c) 2009-2020, The Linux Foundation.
 All rights reserved.
*****************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_log_configure.h"

#ifdef CSR_TARGET_PRODUCT_VM
#include "platform/csr_hydra_log.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CsrLogSubOrigin
{
    CsrUint16            subOriginNumber;  /* Id of the given SubOrigin */
    const CsrCharString *subOriginName;    /* Prefix Text for this SubOrigin */
} CsrLogSubOrigin;

/* Register a task which is going to use the CSR_LOG_TEXT_* interface */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_TARGET_PRODUCT_VM)
void CsrLogTextRegister(CsrLogTextTaskId taskId, const CsrCharString *taskName, CsrUint16 subOriginsLength, const CsrLogSubOrigin *subOrigins);
#else
#define CsrLogTextRegister(taskId, taskName, subOriginsLength, subOrigins)
#endif

#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextGeneric(level, taskId, subOrigin, ...)                               SYNERGY_DEBUG_LOG(level, handle, subOrigin, __VA_ARGS__)
#define CsrLogTextBufferGeneric(level, taskId, subOrigin, bufferLength, buffer, ...)   SYNERGY_DEBUG_LOG(level, handle, subOrigin, __VA_ARGS__)
#endif

/* CRITICAL: Conditions that are threatening to the integrity/stability of the
   system as a whole. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_CRITICAL_DISABLE)

#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextCritical(...)             CsrLogTextGeneric(SYNERGY_DEBUG_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#define CsrLogTextBufferCritical(...)       CsrLogTextBufferGeneric(SYNERGY_DEBUG_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#else
void CsrLogTextCritical(CsrLogTextTaskId taskId, CsrUint16 subOrigin, const CsrCharString *formatString, ...);
void CsrLogTextBufferCritical(CsrLogTextTaskId taskId, CsrUint16 subOrigin, CsrSize bufferLength, const void *buffer, const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_CRITICAL(taskId_subOrigin_formatString_varargs) CsrLogTextCritical taskId_subOrigin_formatString_varargs
#define CSR_LOG_TEXT_CONDITIONAL_CRITICAL(condition, logtextargs) {if (condition) {CSR_LOG_TEXT_CRITICAL(logtextargs);}}
#define CSR_LOG_TEXT_BUFFER_CRITICAL(taskId_subOrigin_length_buffer_formatString_varargs) CsrLogTextBufferCritical taskId_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_CRITICAL(condition, logtextbufferargs) {if (condition) {CSR_LOG_TEXT_BUFFER_CRITICAL(logtextbufferargs);}}

#else /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_CRITICAL_DISABLE) */

#define CSR_LOG_TEXT_CRITICAL(taskId_subOrigin_formatString_varargs)
#define CSR_LOG_TEXT_CONDITIONAL_CRITICAL(condition, logtextargs)
#define CSR_LOG_TEXT_BUFFER_CRITICAL(taskId_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_CRITICAL(condition, logtextbufferargs)

#endif /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_CRITICAL_DISABLE) */

/* ERROR: Malfunction of a component rendering it unable to operate correctly,
   causing lack of functionality but not loss of system integrity/stability. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_ERROR_DISABLE)

#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextError(...)                CsrLogTextGeneric(SYNERGY_DEBUG_LOG_LEVEL_ERROR, __VA_ARGS__)
#define CsrLogTextBufferError(...)          CsrLogTextBufferGeneric(SYNERGY_DEBUG_LOG_LEVEL_ERROR, __VA_ARGS__)
#else
void CsrLogTextError(CsrLogTextTaskId taskId, CsrUint16 subOrigin, const CsrCharString *formatString, ...);
void CsrLogTextBufferError(CsrLogTextTaskId taskId, CsrUint16 subOrigin, CsrSize bufferLength, const void *buffer, const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_ERROR(taskId_subOrigin_formatString_varargs) CsrLogTextError taskId_subOrigin_formatString_varargs
#define CSR_LOG_TEXT_CONDITIONAL_ERROR(condition, logtextargs) {if (condition) {CSR_LOG_TEXT_ERROR(logtextargs);}}
#define CSR_LOG_TEXT_BUFFER_ERROR(taskId_subOrigin_length_buffer_formatString_varargs) CsrLogTextBufferError taskId_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_ERROR(condition, logtextbufferargs) {if (condition) {CSR_LOG_TEXT_BUFFER_ERROR(logtextbufferargs);}}

#else /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_ERROR_DISABLE) */

#define CSR_LOG_TEXT_ERROR(taskId_subOrigin_formatString_varargs)
#define CSR_LOG_TEXT_CONDITIONAL_ERROR(condition, logtextargs)
#define CSR_LOG_TEXT_BUFFER_ERROR(taskId_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_ERROR(condition, logtextbufferargs)

#endif /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_ERROR_DISABLE) */

/* WARNING: Conditions that are unexpected and indicative of possible problems
   or violations of specifications, where the result of such deviations does not
   lead to malfunction of the component. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_WARNING_DISABLE)

#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextWarning(...)              CsrLogTextGeneric(SYNERGY_DEBUG_LOG_LEVEL_WARN, __VA_ARGS__)
#define CsrLogTextBufferWarning(...)        CsrLogTextBufferGeneric(SYNERGY_DEBUG_LOG_LEVEL_WARN, __VA_ARGS__)
#else
void CsrLogTextWarning(CsrLogTextTaskId taskId, CsrUint16 subOrigin, const CsrCharString *formatString, ...);
void CsrLogTextBufferWarning(CsrLogTextTaskId taskId, CsrUint16 subOrigin, CsrSize bufferLength, const void *buffer, const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_WARNING(taskId_subOrigin_formatString_varargs) CsrLogTextWarning taskId_subOrigin_formatString_varargs
#define CSR_LOG_TEXT_CONDITIONAL_WARNING(condition, logtextargs) {if (condition) {CSR_LOG_TEXT_WARNING(logtextargs);}}
#define CSR_LOG_TEXT_BUFFER_WARNING(taskId_subOrigin_length_buffer_formatString_varargs) CsrLogTextBufferWarning taskId_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_WARNING(condition, logtextbufferargs) {if (condition) {CSR_LOG_TEXT_BUFFER_WARNING(logtextbufferargs);}}

#else /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_WARNING_DISABLE) */

#define CSR_LOG_TEXT_WARNING(taskId_subOrigin_formatString_varargs)
#define CSR_LOG_TEXT_CONDITIONAL_WARNING(condition, logtextargs)
#define CSR_LOG_TEXT_BUFFER_WARNING(taskId_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_WARNING(condition, logtextbufferargs)

#endif /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_WARNING_DISABLE) */

/* INFO: Important events that may aid in determining the conditions under which
   the more severe conditions are encountered. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_INFO_DISABLE)

#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextInfo(...)              CsrLogTextGeneric(SYNERGY_DEBUG_LOG_LEVEL_INFO, __VA_ARGS__)
#define CsrLogTextBufferInfo(...)        CsrLogTextBufferGeneric(SYNERGY_DEBUG_LOG_LEVEL_INFO, __VA_ARGS__)
#else
void CsrLogTextInfo(CsrLogTextTaskId taskId, CsrUint16 subOrigin, const CsrCharString *formatString, ...);
void CsrLogTextBufferInfo(CsrLogTextTaskId taskId, CsrUint16 subOrigin, CsrSize bufferLength, const void *buffer, const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_INFO(taskId_subOrigin_formatString_varargs) CsrLogTextInfo taskId_subOrigin_formatString_varargs
#define CSR_LOG_TEXT_CONDITIONAL_INFO(condition, logtextargs) {if (condition) {CSR_LOG_TEXT_INFO(logtextargs);}}
#define CSR_LOG_TEXT_BUFFER_INFO(taskId_subOrigin_length_buffer_formatString_varargs) CsrLogTextBufferInfo taskId_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_INFO(condition, logtextbufferargs) {if (condition) {CSR_LOG_TEXT_BUFFER_INFO(logtextbufferargs);}}

#else /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_INFO_DISABLE) */

#define CSR_LOG_TEXT_INFO(taskId_subOrigin_formatString_varargs)
#define CSR_LOG_TEXT_CONDITIONAL_INFO(condition, logtextargs)
#define CSR_LOG_TEXT_BUFFER_INFO(taskId_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_INFO(condition, logtextbufferargs)

#endif /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_INFO_DISABLE) */

/* DEBUG: Similar to INFO, but dedicated to events that occur more frequently. */
#if defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_DEBUG_DISABLE)

#ifdef CSR_TARGET_PRODUCT_VM
#define CsrLogTextDebug(...)             CsrLogTextGeneric(SYNERGY_DEBUG_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define CsrLogTextBufferDebug(...)       CsrLogTextBufferGeneric(SYNERGY_DEBUG_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#else
void CsrLogTextDebug(CsrLogTextTaskId taskId, CsrUint16 subOrigin, const CsrCharString *formatString, ...);
void CsrLogTextBufferDebug(CsrLogTextTaskId taskId, CsrUint16 subOrigin, CsrSize bufferLength, const void *buffer, const CsrCharString *formatString, ...);
#endif
#define CSR_LOG_TEXT_DEBUG(taskId_subOrigin_formatString_varargs) CsrLogTextDebug taskId_subOrigin_formatString_varargs
#define CSR_LOG_TEXT_CONDITIONAL_DEBUG(condition, logtextargs) {if (condition) {CSR_LOG_TEXT_DEBUG(logtextargs);}}
#define CSR_LOG_TEXT_BUFFER_DEBUG(taskId_subOrigin_length_buffer_formatString_varargs) CsrLogTextBufferDebug taskId_subOrigin_length_buffer_formatString_varargs
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_DEBUG(condition, logtextbufferargs) {if (condition) {CSR_LOG_TEXT_BUFFER_DEBUG(logtextbufferargs);}}

#else /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_DEBUG_DISABLE) */

#define CSR_LOG_TEXT_DEBUG(taskId_subOrigin_formatString_varargs)
#define CSR_LOG_TEXT_CONDITIONAL_DEBUG(condition, logtextargs)
#define CSR_LOG_TEXT_BUFFER_DEBUG(taskId_subOrigin_length_buffer_formatString_varargs)
#define CSR_LOG_TEXT_BUFFER_CONDITIONAL_DEBUG(condition, logtextbufferargs)

#endif /* defined(CSR_LOG_ENABLE) && !defined(CSR_LOG_LEVEL_TEXT_DEBUG_DISABLE) */

/* CSR_LOG_TEXT_ASSERT (CRITICAL) */
#ifdef CSR_LOG_ENABLE

#ifdef CSR_TARGET_PRODUCT_VM
#define CSR_LOG_TEXT_ASSERT(origin, suborigin, condition)                                                           \
    {if (!(condition)) {                                                                                            \
        SYNERGY_HYDRA_LOG_STRING(cond, SYNERGY_FMT(#condition, bonus_arg));                                         \
        SYNERGY_HYDRA_LOG_STRING(the_file, SYNERGY_FMT(__FILE__, bonus_arg));                                       \
        CSR_LOG_TEXT_CRITICAL((origin, suborigin, "Assertion \"%s\" failed at %s:%u", cond, the_file, __LINE__));   \
    }}
#else
#define CSR_LOG_TEXT_ASSERT(origin, suborigin, condition) \
    {if (!(condition)) {CSR_LOG_TEXT_CRITICAL((origin, suborigin, "Assertion \"%s\" failed at %s:%u", #condition, __FILE__, __LINE__));}}
#endif

#else /* CSR_LOG_ENABLE */

#define CSR_LOG_TEXT_ASSERT(origin, suborigin, condition)

#endif /* CSR_LOG_ENABLE */

/* CSR_LOG_TEXT_UNHANDLED_PRIM (CRITICAL) */
#ifdef CSR_LOG_ENABLE

#ifdef CSR_TARGET_PRODUCT_VM
#define CSR_LOG_TEXT_UNHANDLED_PRIMITIVE(origin, suborigin, primClass, primType)    \
    SYNERGY_HYDRA_LOG_STRING(the_file, SYNERGY_FMT(__FILE__, bonus_arg));           \
    CSR_LOG_TEXT_CRITICAL((origin, suborigin, "Unhandled primitive 0x%04X:0x%04X at %s:%u", primClass, primType, the_file, __LINE__))
#else
#define CSR_LOG_TEXT_UNHANDLED_PRIMITIVE(origin, suborigin, primClass, primType) \
    CSR_LOG_TEXT_CRITICAL((origin, suborigin, "Unhandled primitive 0x%04X:0x%04X at %s:%u", primClass, primType, __FILE__, __LINE__))
#endif

#else /* CSR_LOG_ENABLE */

#define CSR_LOG_TEXT_UNHANDLED_PRIMITIVE(origin, suborigin, primClass, primType)

#endif /* CSR_LOG_ENABLE */

#ifdef __cplusplus
}
#endif

#endif
