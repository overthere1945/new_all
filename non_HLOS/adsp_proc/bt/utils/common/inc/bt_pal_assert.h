#ifndef BT_PAL_ASSERT_H
#define BT_PAL_ASSERT_H

#include "err.h" // Core error handling mechanism (for ERR_FATAL)
#include "msg.h" // Often required by ERR_FATAL for logging

/**
 * @brief Triggers a fatal error for the Bluetooth PAL layer with a custom message and arguments.
 *
 * This macro should be called when an unrecoverable error or an unexpected critical state
 * occurs within the Bluetooth PAL layer. It logs a custom format string along with up to
 * three 32-bit arguments, and then halts system execution.
 *
 *
 * @param fmt       A printf-style format string for the error message.
 * @param arg1      Optional argument for the format string (will be cast to uint32_t).
 * @param arg2      Optional argument for the format string (will be cast to uint32_t).
 * @param arg3      Optional argument for the format string (will be cast to uint32_t).
 *
 * @note This macro directly calls `ERR_FATAL` which leads to system reset/crash.
 * It does not return control to the caller.
 */
#define BT_PAL_ASSERT_FATAL(fmt, arg1, arg2, arg3) \
    do {                                                                                                 \
        ERR_FATAL(fmt, (uint32_t)(arg1), (uint32_t)(arg2), (uint32_t)(arg3)); /* KLOCWORK: IGNORE NULL_POINTER */ \
    } while (0)

/**
 * @brief Triggers a fatal error for the Bluetooth PAL layer with a custom message and no arguments.
 *
 * This is a convenience macro for situations where you want a custom message to describe
 * a fatal error but don't need additional arguments to provide context.
 *
 * @param fmt       A printf-style format string for the error message.
 */
#define BT_PAL_ASSERT_FATAL_NO_ARGS(fmt) \
    do { \
        ERR_FATAL_NO_ARG(fmt); /* KLOCWORK: IGNORE NULL_POINTER */ \
    } while (0)

#endif // BT_PAL_ASSERT_H