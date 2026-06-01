/*******************************************************************************

Copyright (C) 2019-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/
#ifndef _EXTERNAL_LOGGING_H_
#define _EXTERNAL_LOGGING_H_

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if defined (BUILD_FOR_HOST) && !defined (BLUESTACK_HOST_IS_APPS)
#include "csr_bt_core_stack_log.h"
#include "csr_bt_panic.h"
#else
#include "csr_bt_panic.h"
#endif


/*****************************************
**      Macro showing source line        *
******************************************/

#if defined (BUILD_FOR_HOST) && !defined (BLUESTACK_HOST_IS_APPS)
/* Synergy provides definition for following macros in the header files csr_bt_core_stack_log.h and csr_bt_panic.h respectively
 *
 * BLUESTACK_WARNING(error_code);
 *
 * BLUESTACK_PANIC(panic_code);
**/
#define BLUESTACK_BREAK_IF_PANIC_RETURNS break;
#elif defined COMPILER_NORCROFT
#define BLUESTACK_WARNING(error_code)
#define BLUESTACK_PANIC(panic_code) NOTREACHED
#define BLUESTACK_BREAK_IF_PANIC_RETURNS
#else
#define BLUESTACK_WARNING(error_code)
#if !defined(CAA) /* Redefinition */
#define BLUESTACK_PANIC(panic_code) BLUESTACK_PANIC(panic_code)
#endif
#define BLUESTACK_BREAK_IF_PANIC_RETURNS
#endif /* BUILD_FOR_HOST && !BLUESTACK_HOST_IS_APPS / COMPILER_NORCROFT */

#ifdef INSTALL_L2CAP_DEBUG
#if defined (BUILD_FOR_HOST) && !defined (BLUESTACK_HOST_IS_APPS)
/* Synergy provides definition for following macros in the header file csr_bt_core_stack_log.h
 *
 * DEBOUT(error_code);
 *
 * DEBDRP(error_code);
**/
#else
#define DEBOUT(x)  { fault((faulCsrSchedTid)(x)); }
#define DEBDRP(x)  { fault((faulCsrSchedTid)(x)); }
#endif
#else
#ifndef DEBOUT
#define DEBOUT(x)
#endif
#ifndef DEBDRP
#define DEBDRP(x)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
