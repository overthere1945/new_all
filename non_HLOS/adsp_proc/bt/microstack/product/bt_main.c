/******************************************************************************
 Copyright (c) 2020 - 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "csr_synergy.h"
#include "csr_sched_init.h"
#include "bt_private.h"
#include "bt_main.h"
#include "csr_framework_ext.h"
#include "hci_arbiter_int.h"
#if (SYN_BT_HOST_TRANSPORT == IPC)
#include "syn_ipc_drv.h"
#endif
#include "csr_hci_lib.h"
#include "hci_arbiter_private.h"

void *schedInstance = NULL;
static CsrMutexHandle btAppCbMutex;
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
BtHostPatchNvmDownloadTime patchNvmDownloadTime;
#endif

#define SCHED_INST_ID               0
#define MICROSTACK_STACK_SIZE       4096

/******************************************************************************
  MicroStackStart: API to start Micro stack
******************************************************************************/
qurt_thread_t MicroStackStart(const uint8_t *threadName,
                              uint16_t priority,
                              MicrostackConfig config)
{
    CsrThreadHandle handle = 0;

    CSR_LOG_TEXT_INFO((BT_API, API_LTSO_GEN, "MicroStackStart %d", config));

    if (schedInstance == NULL)
    {
#if (CSR_HOST_PLATFORM == Q6_HOST)
        SynLogEnableSnoop(TRUE);
#endif
        HciArbInit(config & MICROSTACK_CONFIG_PASSTHROUGH_MODE_MASK);

#if (SYN_BT_HOST_TRANSPORT == IPC)
        BtTransportInitialise();
        SynIpcDrvSetBtRadioOnState(TRUE);
#endif

        /* Initialise application callback mutex */
        CsrMutexCreate(&btAppCbMutex);

        SynLogInit();

#ifdef PATCH_NVM_BUFFER_DOWNLOAD
        BtHostInitPatchNvmDownload();
        patchNvmDownloadTime.startTimeLow = CsrTimeGet(&(patchNvmDownloadTime.startTimeHigh)); /* time in micro sec */
#endif
        /* Init Scheduler */
        schedInstance = CsrSchedInit(SCHED_INST_ID, priority, MICROSTACK_STACK_SIZE);

        BtHostExtCbInit(NULL);

        /* Set thread name and use it during thread creation */
        CsrSchedThreadNameSet((CsrCharString *)threadName, SCHED_INST_ID);

        /* Start scheduler */
        CsrSched(schedInstance);

        handle = CsrSchedThreadHandleGet(SCHED_INST_ID);
    }
    return handle;
}


/******************************************************************************
  MicroStackStop: API to stop the Micro stack
******************************************************************************/
void MicroStackStop(void)
{
    CSR_LOG_TEXT_INFO((BT_API, API_LTSO_GEN, "MicroStackStop"));
#ifndef CONFIG_BT_TESTER
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
    BtHostResetPatchNvmInst();
#endif
    if (schedInstance != NULL)
    {
        CsrSchedStop();

        /* Deinit Scheduler */
        CsrSchedDeinit(schedInstance);
        BtTransportDeinitialise();

        schedInstance = NULL;

        SynLogDeinit();
        
        /* Destroy application callback mutex */
        CsrMutexDestroy(&btAppCbMutex);
    }
#endif
}


/******************************************************************************
  MicroStackConfigureHciLogging: API to configure HCI logging
  Refer to the BTHOST_HCI_XXX options to control HCI logging configurations
******************************************************************************/
void MicroStackConfigureHciLogging(uint16_t configFlags)
{
#ifdef CSR_LOG_ENABLE
    if(configFlags & BTHOST_HCI_ENABLE_SNOOP_LOGGING)
    {
        SynLogEnableSnoop(TRUE);
    }
    else 
    {
        SynLogEnableSnoop(FALSE);
    }

#ifdef STANDALONE_DUT_PLUS
    SynLogConfigureSnoop(configFlags);
#endif /* STANDALONE_DUT_PLUS */

#else
    CSR_UNUSED(configFlags);
#endif
}


/******************************************************************************
  MicroStackRegisterAppCb: Registers the callback function for app handle
******************************************************************************/
boolean MicroStackRegisterAppCb(BtAppHandle handle, BtHostAppCallback appCb)
{
    boolean result; 
    CSR_LOG_TEXT_INFO((BT_API, API_LTSO_GEN, "App Register %x", handle));

    CsrMutexLock(&btAppCbMutex);

    result = BtHostRegisterAppHandleInfo(handle, appCb);

    CsrMutexUnlock(&btAppCbMutex);  

    return result;
}

/******************************************************************************
  MicroStackDeregisterAppCb: Deregisters the callback function for app handle
******************************************************************************/
void MicroStackDeregisterAppCb(BtAppHandle handle)
{
    CSR_LOG_TEXT_INFO((BT_API, API_LTSO_GEN, "App Deregister %x", handle));

    CsrMutexLock(&btAppCbMutex);

    BtHostRemoveAppHandleInfo(handle);

    CsrMutexUnlock(&btAppCbMutex);
}

/******************************************************************************
  MicroStackFreePrimitive: Application calls this API once it has processed the 
  upstream message, corresponding memory for the upstream message is freed. 
******************************************************************************/
void MicroStackFreePrimitive(BtEventClass eventClass, void *message)
{
    CsrBtFreeUpstreamMessageContents(eventClass, message);
    CsrPmemFree(message);
}

