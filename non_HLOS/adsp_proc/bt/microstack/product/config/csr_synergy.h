#ifndef _CSR_SYNERGY_H
#define _CSR_SYNERGY_H
/******************************************************************************
 Copyright (c) 2020 - 2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/


#define CSR_FUNCATTR_NORETURN(x) x

/* Versioning */
#define CSR_VERSION_NUMBER(major,minor,fix) ((major * 10000) + (minor * 100) + fix)

/* Overall configuration */
#ifndef CSR_ENABLE_SHUTDOWN
#define CSR_ENABLE_SHUTDOWN
#endif

#ifndef CSR_EXCEPTION_HANDLER
#define CSR_EXCEPTION_HANDLER
#endif

#ifndef CSR_EXCEPTION_PANIC
#define CSR_EXCEPTION_PANIC 1
#endif

#ifndef CSR_CHIP_MANAGER_TEST_ENABLE
/* #undef CSR_CHIP_MANAGER_TEST_ENABLE */
#endif

#ifndef CSR_CHIP_MANAGER_QUERY_ENABLE
/* #undef CSR_CHIP_MANAGER_QUERY_ENABLE */
#endif

#ifndef CSR_CHIP_MANAGER_ENABLE
/* #undef CSR_CHIP_MANAGER_ENABLE */
#endif

#ifndef CSR_BUILD_DEBUG
/* #undef CSR_BUILD_DEBUG */
#endif

#ifndef CSR_INSTRUMENTED_PROFILING_SERVICE
/* #undef CSR_INSTRUMENTED_PROFILING_SERVICE */
#endif

#ifndef CSR_LOG_ENABLE
/* #undef CSR_LOG_ENABLE */
#endif

#ifndef CSR_USE_QCA_CHIP
#define CSR_USE_QCA_CHIP
#endif

#ifndef CSR_QCA_CHIP_CTL_LOG_ENABLE
/* #undef CSR_QCA_CHIP_CTL_LOG_ENABLE */
#endif

#ifndef CSR_QCA_CHIP_32BIT_SOC_SUPPORT
#define CSR_QCA_CHIP_32BIT_SOC_SUPPORT
#endif

#ifndef CSR_ASYNC_LOG_TRANSPORT
/* #undef CSR_ASYNC_LOG_TRANSPORT */
#endif

#ifndef CSR_AMP_ENABLE
/* #undef CSR_AMP_ENABLE */
#endif

#ifndef CSR_BT_LE_ENABLE
#define CSR_BT_LE_ENABLE
#endif

#define CSR_TARGET_PRODUCT_WEARABLE

#define UTIMER_EVENT_WAIT

#define MICROSTACK_ENABLE_ISLAND_MODE

#define IPC 1
#define UART 2
#define I3C 3

#define CSR_FRW_BUILDSYSTEM_AVAILABLE
#ifdef CSR_FRW_BUILDSYSTEM_AVAILABLE
#include "csr_frw_config.h"
#endif

#ifdef CONFIG_BT_FULL_STACK
#define CSR_BT_SLATE_FULL_CONFIG
#include "csr_bt_config_full.h"
#else
#include "csr_bt_config.h"
#endif

/* Do not edit this area - Start */
#ifdef CSR_BUILD_DEBUG
#ifndef MBLK_DEBUG
#define MBLK_DEBUG
#endif
#endif

#ifdef CSR_ENABLE_SHUTDOWN
#ifndef ENABLE_SHUTDOWN
#define ENABLE_SHUTDOWN
#endif
#endif

#ifndef CSR_EXCEPTION_HANDLER
#ifndef EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE
#define EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE
#endif
#endif

/* Do not edit this area - End */

void CsrSupressEmptyCompilationUnit(void);

#endif /* _CSR_SYNERGY_H */
