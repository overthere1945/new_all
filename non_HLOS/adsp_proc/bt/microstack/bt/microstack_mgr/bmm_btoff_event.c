/******************************************************************************
Copyright (c) 2025 - 2026 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "bmm_private.h"
#include "bt_private.h"
#include "hci_arbiter_lib.h"
#include "csr_hci_peri_util.h"
#include "csr_sched_init.h"
#if (SYN_BT_HOST_TRANSPORT == IPC)
#include "syn_ipc_drv.h"
#endif
uint16    retryCount = 0;

CsrSchedTid schedTaskWaitTimer;
#define SCHED_TASK_WAIT_TIME                   (50 * CSR_SCHED_MILLISECOND)
#define BMM_MAX_STOP_RETRY_ATTEMPTS             2

static void schedTaskWaitTimerFire(uint16 tmp, void *data)
{
    schedTaskWaitTimer = CSR_SCHED_TID_INVALID;

    if (CsrSchedIsQueueEmpty() || retryCount > BMM_MAX_STOP_RETRY_ATTEMPTS)
    {
        BmmPrepareStopCfmSend(gBmm.prepareStopHandle, BMM_RESULT_SUCCESS);
        retryCount = 0;
    }
    else
    {
        BmmStartSchedTaskWaitTimer();
    }

    CSR_UNUSED(tmp);
}

void BmmStartSchedTaskWaitTimer(void)
{
    retryCount++;

    /* Start a timer and send cfm on its expiry */
    schedTaskWaitTimer = CsrSchedTimerSet(SCHED_TASK_WAIT_TIME,
                                          schedTaskWaitTimerFire,
                                          0,
                                          NULL);
}

void BmmHciArbiterBtOffIndHandler(HciArbiterBtOffInd *prim)
{
    BMM_INFO("Bmm Bt Off,status 0x%x", prim->status);

    BmmStartSchedTaskWaitTimer();
}

void BmmHciArbiterBtssCrashIndHandler(HciArbiterBtssCrashInd *prim)
{
    gBmm.errorReason = prim->reason;

    BMM_INFO("Bmm Btss Crash,reason 0x%x", prim->reason);

    BmmPropagateEvent(BMM_EVENT_MASK_SUBSCRIBE_BTSS_ERROR);
}


void BmmPrepareStopReqHandler(BmmPrepareStopReq * prim)
{
    BMM_INFO("Prepare stop,reason %d", prim->reason);

    gBmm.prepareStopHandle = prim->pHandle;
#ifndef CONFIG_BT_TESTER
    if (schedInstance != NULL)
    {
#if (SYN_BT_HOST_TRANSPORT == IPC)
        SynIpcDrvSetBtRadioOnState(FALSE);
#endif
        HciArbiterHostCleanup();

#if (SYN_BT_HOST_TRANSPORT == UART)
        if (prim->reason == BMM_STOP_REASON_USER)
        {
            SendPeriBtOffCommand();
        }
        else
#endif
        {
            BmmStartSchedTaskWaitTimer();
        }
    }
#else
    BmmPrepareStopCfmSend(gBmm.prepareStopHandle, BMM_RESULT_SUCCESS);
#endif
}
