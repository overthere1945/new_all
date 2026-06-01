/******************************************************************************
 Copyright (c) 2020 - 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#ifndef CSR_FRAMEWORK_EXT_TYPES_H__
#define CSR_FRAMEWORK_EXT_TYPES_H__

#include "csr_synergy.h"
#include "csr_types.h"

#include "qurt.h"

#ifdef UTIMER_EVENT_WAIT
#include "utimer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    qurt_signal_t signal;
#ifdef UTIMER_EVENT_WAIT
    utimer_type timer;
#endif
} CsrEventHandle;

typedef qurt_mutex_t CsrMutexHandle;
typedef qurt_thread_t CsrThreadHandle;


#define CSR_HAVE_TIMER
typedef CsrUint32 CsrTimerHandle;

#ifdef __cplusplus
}
#endif

#endif
