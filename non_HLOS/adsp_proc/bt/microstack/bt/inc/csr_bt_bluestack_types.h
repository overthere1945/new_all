#ifndef CSR_BT_BLUESTACK_TYPES_H__
#define CSR_BT_BLUESTACK_TYPES_H__
/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_sched.h"
#include "csr_macro.h"
#include "inc_dir.h"
#include "csr_mblk.h"
#include "csr_pmem.h"
#include "csr_bt_panic.h"
#include "mblk.h"
#include "qbl_fault.h"
#include "qbl_adapter_logging.h"
#include "qbl_adapter_memory.h"
#include "qbl_adapter_panic.h"
#include "qbl_time.h"
#include "qbl_sched.h"
#include "qbl_macros.h"



#ifdef HYDRA

#include "types.h"

#else

#ifdef __cplusplus
extern "C" {
#endif


/* BlueStack support types */
#define PARAM_UNUSED CSR_UNUSED
typedef CsrSchedQid phandle_t;
typedef CsrTime TIME;
typedef CsrUint32 context_t;

#ifndef PHANDLE_TO_QUEUEID
#define PHANDLE_TO_QUEUEID(phdl) (phdl)
#endif

#ifndef QUEUEID_TO_PHANDLE
#define QUEUEID_TO_PHANDLE(qid) (qid)
#endif

/* Compile time assert
 *
 * Evaluate an expression at compile time and force the compilation to
 * abort if the expression is false. expr is the expression to
 * evaluate. msg is a symbol name to try to get into the error message
 * (works only on some compilers).
 */
#define COMPILE_TIME_ASSERT(expr, msg) struct compile_time_assert_ ## msg { \
    int compile_time_assert_ ## msg [1 - (!(expr))*2]; \
}

typedef CsrBool Bool;
typedef Bool bool_t;
#if !defined(CSR_TARGET_PRODUCT_VM)
#ifndef bool
typedef CsrBool bool;
#endif
#endif

typedef CsrUint8 uint8_t;
typedef CsrUint16 uint16_t;
typedef CsrUint32 uint32_t;
typedef CsrUint24 uint24_t;

typedef CsrInt8 int8_t;
typedef CsrInt16 int16_t;
typedef CsrInt32 int32_t;
/* typedef CsrInt24 int24_t; */

#if !defined(CSR_TARGET_PRODUCT_VM)
#ifndef _UINT8_DEFINED
typedef uint8_t uint8;
#define _UINT8_DEFINED
#endif

#ifndef _INT8_DEFINED
typedef int8_t  int8;
#define _INT8_DEFINED
#endif

#ifndef _UINT16_DEFINED
typedef uint16_t uint16;
#define _UINT16_DEFINED
#endif

#ifndef _INT16_DEFINED
typedef int16_t int16;
#define _INT16_DEFINED
#endif

#ifndef _UINT32_DEFINED
typedef uint32_t uint32;
#define _UINT32_DEFINED
#endif

#ifndef _INT32_DEFINED
typedef int32_t int32;
#define _INT32_DEFINED
#endif

typedef int32_t int24_t;
typedef uint24_t uint24;

typedef CsrSize size_t;
#endif

typedef CsrMblk MBLK_T;

#define pdufmt_el_CsrUint8 pdufmt_el_uint8_t
#define pdufmt_el_CsrUint16 pdufmt_el_uint16_t
#define pdufmt_el_CsrUint24 pdufmt_el_uint24_t
#define pdufmt_el_CsrUint32 pdufmt_el_uint32_t

#define pdufmt_el_CsrInt8 pdufmt_el_int8_t
#define pdufmt_el_CsrInt16 pdufmt_el_int16_t
#define pdufmt_el_CsrInt24 pdufmt_el_int24_t
#define pdufmt_el_CsrInt32 pdufmt_el_int32_t

#define URW_TYPE_CsrUint8  URW_TYPE_uint8_t
#define URW_TYPE_CsrUint16 URW_TYPE_uint16_t
#define URW_TYPE_CsrUint32 URW_TYPE_uint32_t

#define unused CSR_UNUSED

#ifdef __cplusplus
}
#endif

#endif /* !HYDRA */

#endif
