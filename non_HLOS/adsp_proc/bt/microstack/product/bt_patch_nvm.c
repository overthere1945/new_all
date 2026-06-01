/******************************************************************************
 Copyright (c) 2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "csr_synergy.h"
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
#include "csr_sched_init.h"
#include "csr_tm_bluecore_task.h"
#include "csr_hci_task.h"
#include "bt_private.h"
#include "csr_bt_tasks.h"
#include "bt_main.h"
#include "csr_hci_lib.h"
#include "csr_tm_bluecore_private_lib.h"
#include "syn_ipc_drv.h"
#include "csr_transport.h"
#include "csr_log_gsp.h"

BtHostPatchNvmData BtHostPatchNvmInst;

BT_HOST_FTM_HCI_CLIENT_CB ftmClientCb = NULL;

void BtHostResetPatchNvmInst()
{
    CsrMemSet(&BtHostPatchNvmInst, 0x00, sizeof(BtHostPatchNvmInst));
    ftmClientCb = NULL;
}

boolean BtHostSendHciCmdFtmMode(uint16_t length, uint8_t *payload)
{
    if (payload && ftmClientCb)
    {
        CsrHciCommandReqSend(length, payload);
        return TRUE;
    }
    return FALSE;
}

boolean BtHostSetFtmMode(BT_HOST_FTM_HCI_CLIENT_CB clientCb)
{
    if (clientCb)
    {
        CsrTmBlueCoreSetFtmMode();
        ftmClientCb = clientCb;
        return TRUE;
    }

    CSR_LOG_TEXT_INFO((BT_API, API_LTSO_GEN, "Invalid Callback function sent in FTM Mode"));
    return FALSE;
}

SynIpcResult btHostFtmModeIpcRxExtensionCallback(SynIpcMsgType   type,
                                                 void            *event)
{
    if (type == SYN_IPC_MSG_HCI)
    {
        SynIpcMsgHci *eventHci = event;

        if (eventHci->pktType == SYN_IPC_HCI_EVT)
        {
            uint16_t dataLen = (uint16_t)eventHci->dataLen;
            CsrUint8  *data = CsrPmemZalloc(dataLen);

            SynMemCpyS(data, dataLen, eventHci->data, eventHci->dataLen);

            if (ftmClientCb)
            {
                ftmClientCb(dataLen, data);
            }
            
            CSR_LOG_BCI(TRANSPORT_CHANNEL_HCI, TRUE, eventHci->dataLen, eventHci->data);
        }
        else
        {
            CSR_LOG_BCI(TRANSPORT_CHANNEL_ACL, TRUE, eventHci->dataLen, eventHci->data);
        }
    }

    return SYN_IPC_RESULT_FTM_MODE;
}

boolean btHostPatchCb(bootstrap_callback_op operation, size_t offset, size_t *bufferLength, uint8_t **buffer, uint8_t *bufferToBeFreed, patchNvmStatus status)
{
    if (operation == OP_BT_BOOTSTRAP_COMPLETE)
    {
        CsrTime endTimeLow;
        CsrTime endTimeHigh;

        endTimeLow = CsrTimeGet(&endTimeHigh);

        patchNvmDownloadTime.totalTime = ((((CsrUint64)endTimeHigh) << 32) | ((CsrUint64)(endTimeLow)))-
                              ((((CsrUint64)patchNvmDownloadTime.startTimeHigh) << 32) | ((CsrUint64)(patchNvmDownloadTime.startTimeLow)));

        /* Convert micro second to milli second */
        patchNvmDownloadTime.totalTime /= 1000;
        CSR_LOG_TEXT_INFO((BT_API, API_LTSO_GEN, "Bootstrap complete : Patch/NVM Download time : %d", patchNvmDownloadTime.totalTime));

        if (ftmClientCb)
        {
            SynIpcRegisterRxExtension(btHostFtmModeIpcRxExtensionCallback, SYN_IPC_HCI_PKT_EVT | SYN_IPC_HCI_PKT_ACL);
        }
    }

    if (BtHostPatchNvmInst.patchCb)
    {
        return BtHostPatchNvmInst.patchCb(operation, offset, bufferLength, buffer, bufferToBeFreed, status);
    }
    return FALSE;
}

boolean btHostNvmCb(bootstrap_callback_op operation, size_t offset, size_t *bufferLength, uint8_t **buffer, uint8_t *bufferToBeFreed, patchNvmStatus status)
{
    if (BtHostPatchNvmInst.nvmCb)
    {
        return BtHostPatchNvmInst.nvmCb(operation, offset, bufferLength, buffer, bufferToBeFreed, status);
    }
    return FALSE;
}

boolean BtHostRegisterDownloadCB(download_type type, size_t length, BT_HOST_PATCH_NVM_CB patchNvmCb)
{
    if (patchNvmCb && length != 0)
    {
        if (type == PATCH_DOWNLOAD_TYPE)
        {
            BtHostPatchNvmInst.patchLength = length;
            BtHostPatchNvmInst.patchCb = patchNvmCb;
        }
        else if (type == NVM_DOWNLOAD_TYPE)
        {
            BtHostPatchNvmInst.nvmLength = length;
            BtHostPatchNvmInst.nvmCb = patchNvmCb;
        }
        else
        {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

boolean BtHostSetBDAddress(BT_BD_ADDR btDeviceAddr)
{
    if (btDeviceAddr.lap || btDeviceAddr.nap || btDeviceAddr.uap)
    {
        BtHostPatchNvmInst.btDeviceAddr = btDeviceAddr;
        return TRUE;
    }
    return FALSE;
}

boolean BtHostIsFtmModeEnabled()
{
    if (ftmClientCb)
    {
        return TRUE;
    }
    return FALSE;
}

void BtHostInitPatchNvmDownload()
{
    BtHostPatchNvmData patchNvmInst = BtHostPatchNvmInst;

    patchNvmInst.patchCb = btHostPatchCb;
    patchNvmInst.nvmCb = btHostNvmCb;

    CsrTmBlueCoreInitPatchNvmData(&patchNvmInst);
}
#endif
