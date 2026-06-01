#ifndef QBL_TIME_H__
#define QBL_TIME_H__
/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HYDRA)

#include "hal/haltime.h"

#elif !defined(CAA)

#define SECOND CSR_SCHED_SECOND
#define MILLISECOND CSR_SCHED_MILLISECOND
#define MINUTE CSR_SCHED_MINUTE

#define timed_event_in CsrSchedTimerSet
#define cancel_timed_event CsrSchedTimerCancel
#define get_time() CsrTimeGet(NULL)
#define time_eq CsrTimeEq
#define time_gt CsrTimeGt
#define time_sub CsrTimeSub
#endif

#ifdef __cplusplus
}
#endif

#endif

