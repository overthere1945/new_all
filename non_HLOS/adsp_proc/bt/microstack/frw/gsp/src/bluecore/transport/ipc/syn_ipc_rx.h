/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef SYN_IPC_RX_H__
#define SYN_IPC_RX_H__

#include "csr_sched.h"
#include "csr_types.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    CsrSchedBgint bgintReassemble;
} SynIpcRxInstance;

void SynIpcRxInit(SynIpcRxInstance *rx);
void SynIpcRxDeinit(SynIpcRxInstance *rx);

#ifdef __cplusplus
}
#endif

#endif
