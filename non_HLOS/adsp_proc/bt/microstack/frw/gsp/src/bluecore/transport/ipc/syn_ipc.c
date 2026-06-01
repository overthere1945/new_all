/******************************************************************************
 Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "syn_ipc_internal.h"

#include "csr_pmem.h"
#include "csr_sched.h"
#include "syn_ipc_drv.h"
#include "csr_time.h"
#include "csr_transport.h"
#include "csr_tm_bluecore_transport.h"

static SynIpcInstance synIpcInst;

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(SynIpcLto);

#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)
static const CsrCharString *synIpcSubOrigins[] =
{
  "General",
  "TX",
  "RX",
};
#endif

static void synIpcInitialize(void)
{
    if (synIpcInst.state == SYN_IPC_STATE_UNINITIALIZED)
    {
        synIpcInst.state = SYN_IPC_STATE_INITIALIZED;
        SynIpcRxInit(&synIpcInst.rx);
        SynIpcTxInit(&synIpcInst.tx);
        SynIpcDrvRegister(synIpcInst.ipcDrvHandle,
                           synIpcInst.rx.bgintReassemble,
                           synIpcInst.tx.bgint,
                           synIpcInst.tx.bgintcustom);

        CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "InitCmp"));
    }
}

static void synIpcDeinitialize(void)
{
    if (synIpcInst.state == SYN_IPC_STATE_INITIALIZED)
    {
        synIpcInst.state = SYN_IPC_STATE_UNINITIALIZED;
        SynIpcRxDeinit(&synIpcInst.rx);
        SynIpcTxDeinit(&synIpcInst.tx);

        CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "DeinitCmp"));
    }
}

static CsrBool synIpcStart(void)
{
    CsrBool result = FALSE;

    synIpcInitialize();

    CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "st %x", synIpcInst.state));

    if (synIpcInst.state == SYN_IPC_STATE_INITIALIZED)
    {
        if (SynIpcDrvStart(synIpcInst.ipcDrvHandle))
        {
            synIpcInst.state = SYN_IPC_STATE_STARTED;

            result = TRUE;
        }
        else
        {
            CSR_LOG_TEXT_CRITICAL((SynIpcLto, SYN_IPC_LTSO_GEN, "StartFail"));
        }
    }
    else if (synIpcInst.state == SYN_IPC_STATE_STARTED)
    {
        result = TRUE;
    }

    return (result);
}

static CsrBool synIpcStopCommon(void)
{
    if (synIpcInst.ipcDrvHandle)
    {
        if (!SynIpcDrvStop(synIpcInst.ipcDrvHandle))
        {
            CSR_LOG_TEXT_ERROR((SynIpcLto, SYN_IPC_LTSO_GEN, "StopFail"));
        }
        synIpcInst.ipcDrvHandle = NULL;
    }
    if (synIpcInst.ipcDrvCustomHandle)
    {
        if (!SynIpcDrvStop(synIpcInst.ipcDrvCustomHandle))
        {
            CSR_LOG_TEXT_ERROR((SynIpcLto, SYN_IPC_LTSO_GEN, "CustStopFail"));
        }
        synIpcInst.ipcDrvCustomHandle = NULL;
    }

    return TRUE;
}

static CsrBool synIpcStop(void)
{
    if (synIpcInst.state < SYN_IPC_STATE_STARTED || synIpcStopCommon())
    {
        CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "Stopped"));
        synIpcInst.state = SYN_IPC_STATE_INITIALIZED;

        synIpcDeinitialize();
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

static void synIpcMsgTx(void *arg)
{
    SynIpcTxSendMsg(&(synIpcInst.tx), arg);
}

static struct CsrTmBlueCoreTransport _synIpcTransport =
{
  synIpcStart,
  synIpcStop,
  NULL,
  synIpcMsgTx,
  /* Receive callback; Registered by HCI */
  NULL,
  NULL,
  /* driverRestart -
     For qca chips driver restart is invoked when baudrate is changed during bootstrapping
     through CsrHciResetTransportReqSend()->CsrTmBlueCoreTransportDriverRestart.
     Driver restart functionality is not required for IPC.
   */
  NULL,
  /* restart - This is invoked as part of 
     CsrTmBluecoreResetIndSend()/CSR_TM_BLUECORE_RESET_IND->csrTmBlueCoreResetIndHandler()->restart()
     But this will is only for bcsp, h4ds, usb. Not for qca h4+ibs or ipc.
   */  
  NULL,
  NULL,
  FALSE,
  NULL,
};

void CsrTmBlueCoreIpcInit(void **gash)
{
    CSR_LOG_TEXT_REGISTER(&SynIpcLto,
                          "IPC",
                          CSR_ARRAY_SIZE(synIpcSubOrigins),
                          synIpcSubOrigins);

    synIpcInst.transport = &_synIpcTransport;
    CsrTmBlueCoreTransportInit(gash, synIpcInst.transport);
}

/* Note: This has to be invoked from the csr_bt_tasks.c, after having 
    initialized the ipc com driver */
void CsrTmBlueCoreRegisterIpcDrvHandle(void *handle)
{
    synIpcInst.ipcDrvHandle = handle;
}
