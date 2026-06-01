/******************************************************************************
 Copyright (c) 2021 - 2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "csr_synergy.h"

#include "csr_sched_init.h"
#include "csr_tm_bluecore_task.h"
#include "csr_hci_task.h"
#include "bt_private.h"
#include "csr_bt_tasks.h"

#include "csr_tm_bluecore_ipc.h"
#if (SYN_BT_HOST_TRANSPORT == IPC)
#include "syn_ipc_drv.h"
#endif
#if (SYN_BT_HOST_TRANSPORT == I3C)
#include "syn_tm_bluecore_i3c.h"
#endif
#include "csr_log_text_2.h"

#include "csr_qvsc_task.h"
#include "bt_main.h"
#include "mblk.h"
#include "hci.h"

#include "csr_hci_common.h"
#include "csr_sched_init.h"
#include "csr_framework_ext.h"

/* Synergy Framework */
CsrSchedQid CSR_HCI_IFACEQUEUE;
CsrSchedQid CSR_TM_BLUECORE_IFACEQUEUE;
CsrSchedQid CSR_QVSC_IFACEQUEUE;

/* Synergy Bluetooth */
CsrSchedQid ADV_SCAN_IFACEQUEUE;
CsrSchedQid HCI_CMD_ARB_IFACEQUEUE;
CsrSchedQid HCI_ATT_ARB_IFACEQUEUE;
CsrSchedQid DM_HCI_IFACEQUEUE;
CsrSchedQid L2CAP_IFACEQUEUE;
CsrSchedQid RFCOMM_IFACEQUEUE;
CsrSchedQid ATT_IFACEQUEUE;
CsrSchedQid CSR_BT_GATT_IFACEQUEUE;
CsrSchedQid CSR_BT_CM_IFACEQUEUE;
CsrSchedQid BMM_IFACEQUEUE;

#ifdef CONFIG_BT_TESTER
CsrSchedQid TESTQUEUE;
#endif

static void * hciTransport = NULL;

static void csrSchedFrwTaskInit(void *data)
{
#if (SYN_BT_HOST_TRANSPORT == IPC)
    CsrSchedRegisterTask(&CSR_TM_BLUECORE_IFACEQUEUE, CsrTmBlueCoreIpcInit, CSR_TM_BLUECORE_DEINIT, CSR_TM_BLUECORE_HANDLER, "CSR_TM_BLUECORE", data, ID_STACK);
#elif (SYN_BT_HOST_TRANSPORT == UART)
    CsrSchedRegisterTask(&CSR_TM_BLUECORE_IFACEQUEUE, CsrTmBlueCoreH4IbsInit, CSR_TM_BLUECORE_DEINIT, CSR_TM_BLUECORE_HANDLER, "CSR_TM_BLUECORE", data, ID_STACK);
#elif (SYN_BT_HOST_TRANSPORT == I3C)
    CsrSchedRegisterTask(&CSR_TM_BLUECORE_IFACEQUEUE, CsrTmBlueCoreI3cInit, CSR_TM_BLUECORE_DEINIT, CSR_TM_BLUECORE_HANDLER, "CSR_TM_BLUECORE", data, ID_STACK);
#endif
    CsrSchedRegisterTask(&CSR_HCI_IFACEQUEUE, CSR_HCI_INIT, CSR_HCI_DEINIT, CSR_HCI_HANDLER, "CSR_HCI", data, ID_STACK);
    CsrSchedRegisterTask(&CSR_QVSC_IFACEQUEUE, CSR_QVSC_INIT, CSR_QVSC_DEINIT, CSR_QVSC_HANDLER, "QVSC", data, ID_STACK);
}

static void csrSchedBtTaskInit(void *data)
{
    CsrSchedRegisterTask(&ADV_SCAN_IFACEQUEUE, ADV_SCAN_INIT, ADV_SCAN_DEINIT, ADV_SCAN_TASK, "BT_ADV_SCAN", data, ID_STACK);
    CsrSchedRegisterTask(&HCI_CMD_ARB_IFACEQUEUE, HCI_CMD_ARB_INIT, HCI_CMD_ARB_DEINIT, HCI_CMD_ARB_TASK, "BT_HCI_CMD_ARB", data, ID_STACK);
#ifdef GATT_SEQUENCING
    CsrSchedRegisterTask(&HCI_ATT_ARB_IFACEQUEUE, HCI_ATT_ARB_INIT, HCI_ATT_ARB_DEINIT, HCI_ATT_ARB_TASK, "BT_ATT_CMD_ARB", data, ID_STACK);
#endif /* GATT_SEQUENCING */
    CsrSchedRegisterTask(&DM_HCI_IFACEQUEUE, DM_HCI_INIT, DM_HCI_DEINIT, DM_HCI_TASK, "BT_DM_HCI", data, ID_STACK);
    CsrSchedRegisterTask(&L2CAP_IFACEQUEUE, L2CAP_INIT, L2CAP_DEINIT, L2CAP_TASK, "BT_L2CAP", data, ID_STACK);
    CsrSchedRegisterTask(&RFCOMM_IFACEQUEUE, RFCOMM_INIT, RFCOMM_DEINIT, RFCOMM_TASK, "BT_RFCOMM", data, ID_STACK);

    CsrSchedRegisterTask(&BMM_IFACEQUEUE, BMM_INIT, BMM_DEINIT, BMM_TASK, "BMM", data, ID_STACK);
#ifdef GATT_OFFLOAD
    CsrSchedRegisterTask(&ATT_IFACEQUEUE, ATT_INIT, ATT_DEINIT, ATT_TASK, "BT_ATT", data, ID_STACK);
    CsrSchedRegisterTask(&CSR_BT_GATT_IFACEQUEUE, CSR_BT_GATT_INIT, CSR_BT_GATT_DEINIT, CSR_BT_GATT_HANDLER, "BT_GATT", data, ID_STACK);
#endif

#ifdef CONFIG_BT_TESTER
    CsrSchedRegisterTask(&TESTQUEUE, CSR_BT_TEST_INIT, CSR_BT_TEST_DEINIT, CSR_BT_TEST_HANDLER, "CSR_BT_TEST", data, ID_STACK);
#endif
}

/******************************************************************************
  CsrSchedTaskInit: Function to initialise BT scheduler tasks/modules
                    This is called from CsrSchedInit()
******************************************************************************/
void CsrSchedTaskInit(void *data)
{
    csrSchedFrwTaskInit(data);
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
    if (!(BtHostIsFtmModeEnabled()))
    {
        csrSchedBtTaskInit(data);
    }
#else
    csrSchedBtTaskInit(data);
#endif
}

/******************************************************************************
  BtTransportInitialise: Function to initialise the HCI transport
******************************************************************************/
void BtTransportInitialise(void)
{
#if (SYN_BT_HOST_TRANSPORT == IPC)
    hciTransport = SynIpcDrvOpen(SYN_IPC_TRANSPORT_HCI);
    CsrTmBlueCoreRegisterIpcDrvHandle(hciTransport);
#endif    
}

/******************************************************************************
  BtTransportDeinitialise: Function to close the HCI transport
******************************************************************************/
void BtTransportDeinitialise(void)
{
    if (hciTransport != NULL)
    {
#if (SYN_BT_HOST_TRANSPORT == IPC)
        SynIpcDrvClose(hciTransport);
#endif
        hciTransport = NULL;
    }
}

/******************************************************************************
  BtHostPlatformCallBack: This is the callback function where platform specific
  functionality can be added for upstream messages
******************************************************************************/
CsrBool BtHostPlatformCallBack(CsrUint16 q, CsrUint16 mi, void *mv)
{
    CsrBool status = TRUE;
    CSR_UNUSED(q);

    {
        /* Disconnect is locally triggered in tracker to active mode scenario 
         * to clear all connections locally. Consume the disconnect ind internally 
         * and dont send it to application */
        CsrPrim type = mv ? (*(CsrPrim *)mv) : 0xffff;

        switch (mi)
        {

            case CSR_BT_GATT_PRIM:
            {
                if (type == CSR_BT_GATT_DISCONNECT_IND)
                {
                    status = FALSE;
                    MicroStackFreePrimitive(mi, mv);
                }
                break;
            }
        }
    }
    return status;
}
