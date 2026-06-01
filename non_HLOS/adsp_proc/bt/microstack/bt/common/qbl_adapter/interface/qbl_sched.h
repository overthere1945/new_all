#ifndef SCHED_H__
#define SCHED_H__
/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/
#include "csr_sched.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HYDRA) && !defined(CAA)
#define INVALID_PHANDLE CSR_SCHED_QID_INVALID

typedef CsrSchedTid tid;
typedef CsrSchedTid tid_t;
typedef CsrSchedQid qid;
typedef CsrSchedQid qid_t;
#define SDP_L2CAPQUEUE SDP_L2CAP_IFACEQUEUE

#define put_message CsrSchedMessagePut
#define get_message(x,y,z) CsrSchedMessageGet(y,z)

#endif

#ifdef __cplusplus
}
#endif

#endif

