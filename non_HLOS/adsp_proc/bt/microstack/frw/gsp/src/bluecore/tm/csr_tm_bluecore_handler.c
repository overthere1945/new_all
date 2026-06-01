/******************************************************************************
Copyright (c) 2008-2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_tm_bluecore_handler.h"
#include "csr_tm_bluecore_prim.h"
#include "csr_tm_bluecore_lib.h"
#include "csr_tm_bluecore_sef.h"
#include "csr_tm_bluecore_transport.h"
#include "csr_tm_qc_hcivs_lib.h"
#include "csr_tm_bluecore_private_lib.h"

#ifndef CSR_USE_QCA_CHIP
#include "csr_bccmd_lib.h"
#endif

#include "csr_hci_private_lib.h"
#include "csr_log_text_2.h"

#if defined(CSR_CHIP_MANAGER_ENABLE) && defined(CSR_BLUECORE_ONOFF)
#error CSR_CHIP_MANAGER_ENABLE and CSR_BLUECORE_ONOFF are mutually exclusive
#endif

#ifdef PATCH_NVM_BUFFER_DOWNLOAD
CsrTmBlueCorePatchNvmData PatchNvmInst;

static void setBDAddress()
{
    CsrUint8 *qcmd;
    CsrUint16 qcmdLength;
    CsrUint8 addrLen = 6;
    CsrUint8 *payload = CsrPmemZalloc(NVM_ACCESS_HEADER_SIZE + addrLen);

    payload[0] = NVM_ACCESS_COMMAND_TYPE_SET;
    payload[1] = 0x02;
    payload[2] = addrLen;

    payload[3] = (PatchNvmInst.btDeviceAddr.nap >> 8) & 0xFF;
    payload[4] = PatchNvmInst.btDeviceAddr.nap & 0xFF;
    payload[5] = PatchNvmInst.btDeviceAddr.uap;
    payload[6] = (PatchNvmInst.btDeviceAddr.lap >> 16) & 0xFF;
    payload[7] = (PatchNvmInst.btDeviceAddr.lap >> 8) & 0xFF;
    payload[8] = PatchNvmInst.btDeviceAddr.lap & 0xFF;

    qcmd = CsrQcHciVsCommandBuild(CSR_QSOC_NVM_ACCESS_OPCODE,
                                  payload,
                                  NVM_ACCESS_HEADER_SIZE + addrLen,
                                  &qcmdLength);
    csrSendQcHciVsData(qcmd, qcmdLength);

    CsrPmemFree(payload);
}

static CsrUint8 tlvSegmentDownload(CsrTmBlueCoreTlvData* tlvData)
{
    CsrUint8 *payload = tlvData->payload;
    CsrUint8 payloadLength = 0;
    CsrUint16 remainingLength = (tlvData->length) - tlvData->currentIndex;

    if ((tlvData->currentIndex < (tlvData->length)) && payload)
    {
        CsrUint8 *qcmd;
        CsrUint16 qcmdLength;

        payloadLength = (CsrUint8) CSRMIN(remainingLength, TLV_SEGMENT_LENGTH_MAX);

        qcmd = CsrQcHciVsDownloadCmdBuild(CSR_QSOC_EDL_CMD_OPCODE,
                                          CSR_TLV_DOWNLOAD_REQ,
                                          &payload[tlvData->currentIndex],
                                          (CsrUint8)payloadLength,
                                          &qcmdLength);
        csrSendQcHciVsData(qcmd, qcmdLength);
        tlvData->currentIndex += payloadLength;
    }
    return payloadLength;
}

static void patchNvmCallbackError(CsrTmBlueCoreTlvData* tlvData)
{
    if (tlvData->bufferToBeFreed && tlvData->payload)
    {
          CsrPmemFree(tlvData->payload);
    }
    tlvData->payload = NULL;

    if (PatchNvmInst.state == PATCH_DOWNLOAD_STATE)
    {
        PatchNvmInst.patchCb(DOWNLOAD_ERROR, tlvData->index, &tlvData->length, &tlvData->payload, &tlvData->bufferToBeFreed, 0x01);
    }
    else if (PatchNvmInst.state == NVM_DOWNLOAD_STATE)
    {
         PatchNvmInst.nvmCb(DOWNLOAD_ERROR, tlvData->index, &tlvData->length, &tlvData->payload, &tlvData->bufferToBeFreed, 0x01);
    }

    PatchNvmInst.state = IDLE_DOWNLOAD_STATE;
    tlvData->index = 0;
    tlvData->currentIndex = 0;
}

static CsrBool patchNvmLengthValidation(CsrTmBlueCoreTlvData* tlvData)
{
    CsrSize length;

    if (PatchNvmInst.state == PATCH_DOWNLOAD_STATE)
    {
        length = PatchNvmInst.patchLength;
    }
    else
    {
        length = PatchNvmInst.nvmLength;
    }

    if ((tlvData->length + tlvData->index) != length)
    {
        if (((tlvData->length) > TLV_SEGMENT_LENGTH_MAX) && ((tlvData->length % TLV_SEGMENT_LENGTH_MAX) != 0))
        {
            patchNvmCallbackError(tlvData);
            return FALSE;
        }
    }
    return TRUE;
}

static void handlePatchNvmOpcode(download_callback_op op, CsrTmBlueCoreTlvData* tlvData)
{
    if (tlvData->bufferToBeFreed && tlvData->payload)
    {
        CsrPmemFree(tlvData->payload);
    }
    tlvData->payload = NULL;

    switch (op)
    {
        case PATCH_DATA_GET:
        {
            tlvData->index += tlvData->currentIndex;
            tlvData->currentIndex = 0;

            if (tlvData->index == PatchNvmInst.patchLength)
            { /* Patch download sent for complete segments, wait for command complete to proceed */
                return;
            }

            if (PatchNvmInst.patchCb)
            {
                PatchNvmInst.patchCb(PATCH_DATA_GET, tlvData->index, &tlvData->length, &tlvData->payload, &tlvData->bufferToBeFreed, 0x00);
            }

            if (tlvData->payload)
            {
                if (patchNvmLengthValidation(tlvData))
                {
                    tlvSegmentDownload(tlvData);

                    /* post a msg to self to continue the patch download process */
                    CsrTmBluecorePatchSegmentDownloadReqSend(CSR_TM_BLUECORE_IFACEQUEUE);
                }
            }
            else
            {
                /* NULL payload sent by app */
                CSR_LOG_TEXT_ERROR((CsrTmBluecoreLto, 0, "Received Patch data is NULL"));
            }
        }
        break;
        case NVM_DATA_GET:
        {
            tlvData->currentIndex = 0;

            if (PatchNvmInst.nvmCb)
            {
                PatchNvmInst.nvmCb(NVM_DATA_GET, tlvData->index, &tlvData->length, &tlvData->payload, &tlvData->bufferToBeFreed, 0x00);
            }

            if (tlvData->payload)
            {
                if (patchNvmLengthValidation(tlvData))
                {
                    tlvSegmentDownload(tlvData);
                }
            }
            else
            {
                /* NULL payload sent by app */
                CSR_LOG_TEXT_ERROR((CsrTmBluecoreLto, 0, "Received NVM data is NULL"));
            }
        }
        break;
        case PATCH_DOWNLOAD_COMPLETE:
        {
            PatchNvmInst.patchCb(PATCH_DOWNLOAD_COMPLETE, tlvData->index, &tlvData->length, &tlvData->payload, &tlvData->bufferToBeFreed, 0x00);
            CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0, "Patch Download complete"));

            tlvData->index = 0;
            tlvData->currentIndex = 0;

            if (PatchNvmInst.nvmLength > 0)
            {
                PatchNvmInst.state = NVM_DOWNLOAD_STATE;
                CsrTmBlueCoreStartPatchNvmDownload();
            }
            else
            {
                PatchNvmInst.state = IDLE_DOWNLOAD_STATE;
                CsrQcHciResetReqSend();
            }
        }
        break;
        case NVM_DOWNLOAD_COMPLETE:
        {
            PatchNvmInst.nvmCb(NVM_DOWNLOAD_COMPLETE, tlvData->index, &tlvData->length, &tlvData->payload, &tlvData->bufferToBeFreed, 0x00);
            CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0, "NVM Download complete"));

            PatchNvmInst.state = IDLE_DOWNLOAD_STATE;
            tlvData->index = 0;
            tlvData->currentIndex = 0;

            if (PatchNvmInst.btDeviceAddr.lap || PatchNvmInst.btDeviceAddr.nap || PatchNvmInst.btDeviceAddr.uap)
            {
                /* Set Bluetooth address */
                setBDAddress();
            }
            else
            {
               CsrQcHciResetReqSend();
            }
        }
        break;
        default:
            break;
    }
}

static void handlePatchDownload(CsrTmBlueCoreTlvData* tlvData)
{
    if (tlvData->index < PatchNvmInst.patchLength)
    {
        if (tlvData->currentIndex == 0 || tlvData->currentIndex == tlvData->length)
        { /* First time patch download or current segment download complete case */
            handlePatchNvmOpcode(PATCH_DATA_GET, tlvData);
        }
        else
        {
            tlvSegmentDownload(tlvData);

            /* post a msg to self to continue the patch download process */
            CsrTmBluecorePatchSegmentDownloadReqSend(CSR_TM_BLUECORE_IFACEQUEUE);
        }
    }
}

static void handleNvmDownload(CsrTmBlueCoreTlvData* tlvData)
{
    if (tlvData->index < PatchNvmInst.nvmLength)
    {
        if (!(tlvData->currentIndex))
        { /* First time NVM download case */
            handlePatchNvmOpcode(NVM_DATA_GET, tlvData);
        }
        else
        {
            CsrUint8 segmentlength = tlvSegmentDownload(tlvData);

            if (segmentlength)
            {
                /* download pending for current segment */
            }
            else
            {
                tlvData->index += tlvData->currentIndex;

                if (tlvData->index == PatchNvmInst.nvmLength)
                { /* NVM Download complete */
                    handlePatchNvmOpcode(NVM_DOWNLOAD_COMPLETE, tlvData);
                }
                else
                {
                    handlePatchNvmOpcode(NVM_DATA_GET, tlvData);
                }
            }
        }
    }
}

static void handlePatchNvmDownload(CsrTmBlueCoreTlvData* tlvData)
{
    switch (PatchNvmInst.state)
    {
        case PATCH_DOWNLOAD_STATE:
        {
            handlePatchDownload(tlvData);
        }
        break;

        case NVM_DOWNLOAD_STATE:
        {
            handleNvmDownload(tlvData);
        }
        break;
        default:
            break;
    }
}

void CsrTmBlueCoreHandlePatchNvmDownloadCfm(CsrResult result)
{
    CsrTmBlueCoreTlvData* tlvData = &PatchNvmInst.tlvData;

    switch (PatchNvmInst.state)
    {
        case PATCH_DOWNLOAD_STATE:
        {
            if (result == CSR_QC_HCI_VS_RESULT_SUCCESS)
            {
                handlePatchNvmOpcode(PATCH_DOWNLOAD_COMPLETE, tlvData);
            }
            else
            {
                CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0,
                                   "Patch download error received from BTSS (Reason: %u)", result));
                patchNvmCallbackError(tlvData);
            }
        }
        break;
        case NVM_DOWNLOAD_STATE:
        {
            if (result == CSR_QC_HCI_VS_RESULT_SUCCESS)
            {
                handlePatchNvmDownload(tlvData);
            }
            else
            {
                CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0,
                                   "NVM download error received from BTSS (Reason: %u)", result));
                patchNvmCallbackError(tlvData);
            }
        }
        break;
        default:
            break;
    }
}

void CsrTmBlueCoreInitPatchNvmData(void *data)
{
    CsrTmBlueCorePatchNvmData *pData = (CsrTmBlueCorePatchNvmData *)data;

    if (pData)
    {
        PatchNvmInst.patchLength = pData->patchLength;
        PatchNvmInst.nvmLength = pData->nvmLength;
        PatchNvmInst.btDeviceAddr.lap = pData->btDeviceAddr.lap;
        PatchNvmInst.btDeviceAddr.uap = pData->btDeviceAddr.uap;
        PatchNvmInst.btDeviceAddr.nap = pData->btDeviceAddr.nap;
        PatchNvmInst.patchCb = pData->patchCb;
        PatchNvmInst.nvmCb = pData->nvmCb;

        PatchNvmInst.tlvData.index = 0;
        PatchNvmInst.tlvData.currentIndex = 0;
        PatchNvmInst.tlvData.length = 0;
        PatchNvmInst.tlvData.payload = NULL;
        PatchNvmInst.tlvData.bufferToBeFreed = 0;

        /* Initialise state to start the process */
        PatchNvmInst.state = PATCH_DOWNLOAD_STATE;
    }
}

void CsrTmBlueCoreStartPatchNvmDownload()
{
    CsrTmBlueCoreTlvData* tlvData = &PatchNvmInst.tlvData;

    handlePatchNvmDownload(tlvData);
}

void CsrTmBlueCoreSendBootstrapComplete()
{
    CsrTmBlueCoreTlvData* tlvData = &PatchNvmInst.tlvData;

    CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0, "Bootstrap complete complete"));

    PatchNvmInst.patchCb(BOOTSTRAP_COMPLETE, tlvData->index, &tlvData->length, &tlvData->payload, &tlvData->bufferToBeFreed, 0x00);
}
#endif

void CsrTmBlueCoreHandler(void **gash)
{
    CsrTmBlueCoreInstanceData *tmBcInst = *gash;
    CsrUint16 event;

    if (!tmBcInst)
    {
        CSR_LOG_TEXT_ERROR((CsrTmBluecoreLto, 0, "tmBcInst is NULL"));
        return;
    }

    CsrSchedMessageGet(&event, &tmBcInst->recvMsgP);

    switch (event)
    {
        case CSR_TM_BLUECORE_PRIM:
            CsrTmBlueCoreTmBlueCoreMsgHandler(tmBcInst);
            break;
#ifndef CSR_USE_QCA_CHIP
        /* BCCMD is used in TM only for BlueCore bootstrapping */
        case CSR_BCCMD_PRIM:
            CsrTmBlueCoreBccmdMsgHandler(tmBcInst);
            break;
#ifdef CSR_USE_BCSP_HTRANS
        case CSR_SSD_PRIM:
        {
            CsrTmBlueCoreSsdMsgHandler(tmBcInst);
            break;
        }
#endif /* CSR_USE_BCSP_HTRANS */
#endif /* !CSR_USE_QCA_CHIP */

        case CSR_HCI_PRIM:
            CsrTmBlueCoreHciMsgHandler(tmBcInst);
            break;
        default:
            CsrGeneralException(CsrTmBluecoreLto,
                                0,
                                event,
                                *(CsrPrim * ) tmBcInst->recvMsgP,
                                tmBcInst->state,
                                NULL);
            break;
    }

    CsrPmemFree(tmBcInst->recvMsgP);
    tmBcInst->recvMsgP = NULL;
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************
    This function is called by the scheduler to perform a graceful shutdown
    of a scheduler task.
    This function must:
    1)    empty the input message queue and free any allocated memory in the
          messages.
    2)    free any instance data that may be allocated.
****************************************************************************/
void CsrTmBlueCoreDeinit(void **gash)
{
    CsrTmBlueCoreInstanceData *tmBcInst = *gash;
    CsrUint16 event;
    void *message;
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
    CsrTmBlueCoreResetFtmMode();

    CsrMemSet(&PatchNvmInst,
              0x00,
              sizeof(PatchNvmInst));
#endif
#ifdef CSR_BLUECORE_ONOFF
    if (CSR_TM_BLUECORE_TRANSPORT_STARTED(tmBcInst->blueCoreTransportHandle))
    {
        CsrTmBlueCoreTransportStop(tmBcInst->blueCoreTransportHandle);
    }
    CsrCmnListDeinit(&tmBcInst->transportDelegates);
#else
    CsrTmBlueCoreTransportStop(tmBcInst->blueCoreTransportHandle);
    CsrCmnListDeinit(&tmBcInst->transportActivators);
#endif
#ifdef CSR_CHIP_MANAGER_ENABLE
    CsrCmnListDeinit(&tmBcInst->cmStatusSubscribers);
    CsrCmnListDeinit(&tmBcInst->cmReplayersRegistered);
    CsrCmnListDeinit(&tmBcInst->cmReplayersStarted);
#endif

#ifdef CSR_BLUECORE_ONOFF
    while (CsrSchedMessageGet(&event, &message) ||
           CsrMessageQueuePop(&tmBcInst->saveQueue, &event, &message))
#else
    while (CsrSchedMessageGet(&event, &message))
#endif
    {
        switch (event)
        {
#ifndef CSR_USE_QCA_CHIP
#ifndef EXCLUDE_CSR_BCCMD_MODULE
            case CSR_BCCMD_PRIM:
            {
                CsrBccmdFreeUpstreamMessageContents(CSR_BCCMD_PRIM, message);
                break;
            }
#endif /* !EXCLUDE_CSR_BCCMD_MODULE */
#endif /* !CSR_USE_QCA_CHIP */
            case CSR_HCI_PRIVATE_PRIM:
            {
                CsrHciPrivateFreeUpstreamMessageContents(CSR_HCI_PRIVATE_PRIM, message);
                break;
            }
        }
        CsrPmemFree(message);
    }
#ifdef CSR_USE_QCA_CHIP
    if(tmBcInst->qcBinVar)
    {
        if(tmBcInst->qcBinVar->payload)
        {
            /*Free the memory allocated for patch download*/
            CsrPmemFree(tmBcInst->qcBinVar->payload);
        }
        CsrPmemFree(tmBcInst->qcBinVar);
    }
#endif
    CsrPmemFree(tmBcInst);
}

#endif /* ENABLE_SHUTDOWN */
