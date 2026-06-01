/******************************************************************************
Copyright (c) 2016-2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_util.h"
#include "csr_log_text_2.h"
#include "csr_hci_lib.h"
#include "csr_mblk.h"
#include "csr_transport.h"
#include "csr_message_queue.h"
#include "csr_tm_qc_hcivs_lib.h"
#include "csr_tm_qc_hcivs_util.h"
#include "csr_tm_bluecore_handler.h"
#include "csr_tm_bluecore_bootstrap.h"
#include "csr_tm_bluecore_transport.h"
#include "csr_tm_bluecore_prim.h"
#include "csr_tm_bluecore_task.h"
#include "csr_serial_com.h"
#include "csr_tm_bluecore_sef.h"
#include "csr_hci_private_prim.h"
#include "csr_hci_prim.h"
#include "csr_hci_common.h"

#define CSR_TM_QC_UART_HANDLE                                           NULL
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
static CsrBool ftmMode = FALSE;

void CsrTmBlueCoreSetFtmMode()
{
    ftmMode = TRUE;
}

void CsrTmBlueCoreResetFtmMode()
{
    ftmMode = FALSE;
}
static void csrTmGetPatchVerRspHandler(CsrTmBlueCoreInstanceData *tmBcInst,
                                    CsrUint8 const* payload)
{
    /* payload pointing to build version, just extract it */
    CsrUint16 buildVer = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(payload);

#ifdef CSR_QCA_CHIP_32BIT_SOC_SUPPORT
    CsrUint32 socVer = CSR_GET_UINT32_FROM_LITTLE_ENDIAN(payload + CSR_QC_HCIVS_SOC_VER_OFFSET);
#endif
    CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0,
                      "SOC Version: 0x%04x PatchBuild Version: 0x%02x",
                       socVer, buildVer));
}
#endif
#if !defined(CSR_TARGET_PRODUCT_WEARABLE) || defined(CSR_TARGET_PRODUCT_WEARABLE_X86)
#ifdef CSR_LOG_ENABLE
static CsrCharString logTextErrorString [MAX_NUMBER_OF_DOWNLOAD_STATES][20] =
{
    "DOWNLOAD_IDLE",
    "M0_DOWNLOAD",
    "AUP_DOWNLOAD",
    "NVM_DOWNLOAD"
};
#endif

static void csrNvmDownloadStart(CsrTmBlueCoreInstanceData *tmBcInst);
static void qcPatchDownloadHandler(CsrTmBlueCoreInstanceData *tmBcInst);

static void csrTmQcBaudrateRespTimeout(CsrUint16 mi, void *mv)
{
#ifdef CSR_QCA_BAUD_NO_RSP_SKIP_PANIC
    CsrTmBlueCoreInstanceData* tmBcInst = (CsrTmBlueCoreInstanceData*)mv;

    tmBcInst->state = CSR_TM_BLUECORE_STATE_ACTIVATING;
    CsrQcGetPatchVerReqSend();
#else
    CsrTmQcPanic(SET_BAUD_FAILURE);
    CSR_UNUSED(mv);
#endif
    CSR_UNUSED(mi);
}

/*After transport restart the chip is supposed to respond to set baud on the 
new baud.But if the chip response is not received/missed then we timeout and
assert/panic*/
#define QC_BAUDRATE_RESP_TIMEOUT    150000

static void csrQcTlvPatchDownload(CsrQcBin *binVar)
{
    CsrUint8 *payload;
    CsrUint32 payloadLength;
    if (binVar->type == BINARY_FILE_TYPE_TLV || binVar->type == BINARY_FILE_TYPE_AUP)
    {
        while (binVar->currentIndex < binVar->payloadLength)
        {
            payload = csrGetQcBinData(binVar, &payloadLength);
            if (payload)
            {
                CsrUint8 *qcmd;
                CsrUint16 qcmdLength;
                qcmd = CsrQcHciVsDownloadCmdBuild(CSR_QSOC_EDL_CMD_OPCODE, 
                                               CSR_TLV_DOWNLOAD_REQ,
                                               payload, 
                                               (CsrUint8)payloadLength,
                                               &qcmdLength);
                csrSendQcHciVsData(qcmd, qcmdLength);
                binVar->currentIndex += payloadLength;
            }
        }
    }
}

CsrBool CsrQcReadAndUpdateNvmFile(CsrUint8 **payload, CsrUint32 *payloadLen)
{
    CsrSize size = 0;
    CsrBool status = FALSE;

    *payload = CsrTmBluecoreReadBinFile(BINARY_FILE_TYPE_NVM, &size);
    if (*payload != NULL)
    {
        if (CsrUpdateNvmTags(*payload))
        {
            status = TRUE;
        }
    }
    else
    {
        CSR_LOG_TEXT_ERROR((CsrTmBluecoreLto, 0, "Invalid binary file"));
    }
    *payloadLen = (CsrUint32) size;

    return status;
}

static CsrBool csrTmQcBaudrateChange(CsrTmBlueCoreInstanceData *tmBcInst)
{
    CsrUint32 currentBaudrate = CsrUartDrvGetBaudrate(CSR_TM_QC_UART_HANDLE);
    CsrUint32 resetBaudrate = CsrTmGetResetBaudrate();

    if (currentBaudrate != resetBaudrate)
    {
        CsrUint8 resetBaudId = CsrUartDrvGetBaudIdFromBaudrate(resetBaudrate);

        if (resetBaudId == CSR_UART_BAUD_ID_INVALID)
        {
            return (FALSE);
        }

        CsrUartDrvRtsSet(CSR_TM_QC_UART_HANDLE, FALSE);
        /* Notify BT chip to set specific baud rate  */
        CsrQcSetBaudrateReqSend(CsrQsocBaudrateMap(resetBaudrate));

        /* After setbaudreq reaches the chip, reset the transport driver for new baud rate settings.*/
        CsrHciResetTransportReqSend(CSR_TM_BLUECORE_IFACEQUEUE, resetBaudId);

        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

static void qcBinSendCfmHandler(CsrTmBlueCoreInstanceData *tmBcInst)
{
    if (CsrTmQcGetFirmwareLogSetting())
    {
        CsrQcControllerLogReqSend(TRUE);
    }
    else
    {
        CsrQcHciResetReqSend();
    }
}

static void csrQcInitiatePatchDownload (CsrTmBlueCoreInstanceData *tmBcInst, CsrUint8 downloadType)
{
    CsrSize payloadLength = 0;
    CsrUint8 *payload = CsrTmBluecoreReadBinFile(downloadType, &payloadLength);

    if (payload != NULL)
    {
        CsrQcBin *binVar = CsrPmemAlloc(sizeof(CsrQcBin));

        binVar->payload = payload;
        binVar->payloadLength = (CsrUint32) payloadLength;
        binVar->currentIndex = 0;
        binVar->type = downloadType;

        CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0, "%s: downloading binary file", logTextErrorString[tmBcInst->qcPatchDownloadState]));

        csrQcTlvPatchDownload(binVar);

        /*Free the memory allocated for patch download*/
        CsrPmemFree(binVar->payload);
        CsrPmemFree(binVar);
    }
    else
    {
        /* this is the failure case - hence we will not be receiving any event from the firmware - so start the next download */
        CSR_LOG_TEXT_ERROR((CsrTmBluecoreLto, 0, "Invalid binary file for download type:  %s", logTextErrorString[tmBcInst->qcPatchDownloadState]));
        qcPatchDownloadHandler(tmBcInst);
    }
}

static void qcUpdatePatchDownloadState(CsrTmBlueCoreInstanceData *tmBcInst)
{
    /* Moving on to next state */
    tmBcInst->qcPatchDownloadState = tmBcInst->qcPatchDownloadState < NVM_DOWNLOAD ?
                                     tmBcInst->qcPatchDownloadState + 1:
                                     NVM_DOWNLOAD;
}

static void qcPatchDownloadHandler (CsrTmBlueCoreInstanceData *tmBcInst)
{
    qcUpdatePatchDownloadState (tmBcInst);

    switch (tmBcInst->qcPatchDownloadState)
    {
        case M0_DOWNLOAD:
            /* initiating M0 patch download */
            csrQcInitiatePatchDownload (tmBcInst, BINARY_FILE_TYPE_TLV);
            break;
        case AUP_DOWNLOAD:
            /* initiating AUP patch download */
            csrQcInitiatePatchDownload (tmBcInst, BINARY_FILE_TYPE_AUP);
            break;
        case NVM_DOWNLOAD:
            /* initiating NVM patch download */
            csrNvmDownloadStart (tmBcInst);
            break;
        default:
            break;
    }
}

static void CsrTmQcDownloadPanic(CsrTmBlueCoreInstanceData *tmBcInst)
{
    switch(tmBcInst->qcPatchDownloadState)
    {
        case M0_DOWNLOAD:
            CsrTmQcPanic(PATCH_M0_DOWNLOAD_FAILURE);
            break;
        case AUP_DOWNLOAD:
            CsrTmQcPanic(PATCH_AUP_DOWNLOAD_FAILURE);
            break;
        case NVM_DOWNLOAD:
            CsrTmQcPanic(READ_UPDATE_NVM_FAILURE);
            break;
        default:
            break;
    }
}

/*Start dumping the patch file(tlv extension) after receiving patch version details.
QC Chips enter a mode where it receives all patch chunk without any event or command
complete response. Only for the last chunk of patch there will be a event response*/
static void qcGetPatchVerRspHandler(CsrTmBlueCoreInstanceData *tmBcInst,
                                    CsrUint8 const* payload)
{
    /* payload pointing to build version, just extract it */
    CsrUint16 buildVer = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(payload);

#ifdef CSR_QCA_CHIP_32BIT_SOC_SUPPORT
    CsrUint32 socVer = CSR_GET_UINT32_FROM_LITTLE_ENDIAN(payload + CSR_QC_HCIVS_SOC_VER_OFFSET);
    if (CsrHciGetPatchNvmFileNameEx(socVer, buildVer))
#else
    CsrUint16 socVer = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(payload + CSR_QC_HCIVS_SOC_VER_OFFSET);
    if (CsrHciGetPatchNvmFileName(socVer, buildVer))
#endif
    {
        CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0,
                          "SOC Version: 0x%04x PatchBuild Version: 0x%02x",
                          socVer, buildVer));
        /* Attempting to download M0 patch file */
        qcPatchDownloadHandler(tmBcInst);
    }
    else
    {
        CSR_LOG_TEXT_ERROR((CsrTmBluecoreLto, 0,
                           "Unknown SOC Version: 0x%04x or PatchBuild Version: 0x%02x mismatch",
                           socVer, buildVer));
    }
}

static void qcSetBaudrateCfmHandler(CsrTmBlueCoreInstanceData *tmBcInst, CsrResult status)
{

    if(tmBcInst->timerId != CSR_SCHED_TID_INVALID)
    {
        CsrSchedTimerCancel(tmBcInst->timerId, NULL, NULL);
        tmBcInst->timerId = CSR_SCHED_TID_INVALID;
    }

    if (status == CSR_RESULT_SUCCESS)
    {
        tmBcInst->state = CSR_TM_BLUECORE_STATE_ACTIVATING;
        CsrQcGetPatchVerReqSend();
    }
    else
    {
        CsrTmQcPanic(SET_BAUD_FAILURE);
    }
}

static void csrNvmDownloadStart(CsrTmBlueCoreInstanceData *tmBcInst)
{
    if(!tmBcInst->qcBinVar)
    {
        tmBcInst->qcBinVar = CsrPmemAlloc(sizeof(CsrQcBin));
        tmBcInst->qcBinVar->currentIndex = 0;
        tmBcInst->qcBinVar->type = BINARY_FILE_TYPE_NVM;

        CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0, "%s: downloading binary file", logTextErrorString[tmBcInst->qcPatchDownloadState]));
        if(!CsrQcReadAndUpdateNvmFile(&tmBcInst->qcBinVar->payload, &tmBcInst->qcBinVar->payloadLength))
        {
            CsrTmQcPanic(READ_UPDATE_NVM_FAILURE);
        }
    }
    /* CsrQcNvmChunkReqSend returns FALSE when there is no
       more data i.e.eof */
    if(!CsrQcNvmChunkReqSend(tmBcInst->qcBinVar))
    {
        /*Free the memory allocated for patch download*/
        CsrPmemFree(tmBcInst->qcBinVar->payload);
        CsrPmemFree(tmBcInst->qcBinVar);
        tmBcInst->qcBinVar = NULL;
        qcBinSendCfmHandler(tmBcInst);
    }
}

static void csrTmQcReadDebugControlTagResHandler(CsrTmBlueCoreInstanceData *tmBcInst,
                                                 CsrUint8 *debugControl,
                                                 CsrUint8 length)
{
    debugControl[0] |= DEBUG_CONTROL_INHIBIT_NOP_BIT4;
    CsrQcWriteDebugControlTagReqSend(debugControl, length);
}

static void csrTmQcWriteDebugControlTagResHandler(CsrTmBlueCoreInstanceData *tmBcInst)
{
    CsrHciUnregisterVendorSpecificEventHandlerReqSend(CSR_TM_BLUECORE_IFACEQUEUE, TRANSPORT_CHANNEL_QC_HCIVS);
}

static void csrTmQcHandleVsEvent(CsrTmBlueCoreInstanceData *tmBcInst,
                                 CsrUint8 *data,
                                 CsrUint16 dataLen)
{
    CsrResult status = CSR_QC_HCI_VS_RESULT_ERROR;

    if (CsrTmQcGetFirmwareLogSetting() && (dataLen == 2))
    {
        /* Response for firmware hci log enable(HCI_VS_HOST_LOG_CTRL_SUBID)
        has arrived*/
        if((data[0] == EDL_FW_HCI_RES_PARAM_LEN) &&
            (data[1] == EDL_FW_HCI_REQ_RES_EVT))
        {
            CsrQcHciResetReqSend();
        }
        return;
    }

    switch(data[CMD_RSP_OFFSET])
    {
        case EDL_CMD_REQ_RES_EVT:
        {
            CSR_LOG_ASSERT(dataLen >= CSR_QC_HCIVS_RESP_MIN_SIZE);
            switch(data[RES_TYPE_OFFSET])
            {
                case EDL_APP_VER_RES_EVT:
                    /* Initializing the download state - for the download procedure to start */
                    tmBcInst->qcPatchDownloadState = DOWNLOAD_IDLE;
                    qcGetPatchVerRspHandler(tmBcInst, data + CSR_QC_HCIVS_BUILD_VER_OFFSET);
                    break;
                case EDL_TLV_DNLD_RES_EVT:
                    status = data[CSR_QC_HCIVS_TLV_DOWNLOAD_RES_STATUS_OFFSET];

                    if (status == CSR_QC_HCI_VS_RESULT_SUCCESS)
                    {
                        qcPatchDownloadHandler (tmBcInst);
                    }
                    else
                    {
                        /* Patch Download Failed */
                        CsrTmQcDownloadPanic(tmBcInst);
                    }
                    break;
                default:
                    break;
            }

            break;
        }
        case CSR_QSOC_SET_BAUD_RATE_RESP:
        {
            status = data[CSR_QC_HCIVS_SET_BAUD_RES_STATUS_OFFSET];
            qcSetBaudrateCfmHandler(tmBcInst, status^0x01);
            break;
        }
        case VENDOR_EVENT_CLASS_NVM_ACCESS:
        {
            switch(data[NVM_ACCESS_COMMAND_TYPE_OFFSET])
            {
                case NVM_ACCESS_COMMAND_TYPE_GET:
                {
                    if (data[NVM_ACCESS_TAG_NUMBER_OFFSET] == NVM_ACCESS_TAG_NUMBER_DEBUG_CONTROL)
                    {
                        csrTmQcReadDebugControlTagResHandler(tmBcInst,
                                                             &data[NVM_ACCESS_TAG_VALUE_OFFSET],
                                                             dataLen - NVM_ACCESS_TAG_VALUE_OFFSET);
                    }
                    else
                    {
                        CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto, 0, 
                                             "NVM Access Get Response for unknown Tag Number (%u)", 
                                             data[NVM_ACCESS_TAG_NUMBER_OFFSET]));
                    }
                    break;
                }
                case NVM_ACCESS_COMMAND_TYPE_SET:
                {
                    if (data[NVM_ACCESS_TAG_NUMBER_OFFSET] == NVM_ACCESS_TAG_NUMBER_DEBUG_CONTROL)
                    {
                        csrTmQcWriteDebugControlTagResHandler(tmBcInst);
                    }
                    else
                    {
                        CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto, 0, 
                                             "NVM Access Set Response for unknown Tag Number (%u)", 
                                             data[NVM_ACCESS_TAG_NUMBER_OFFSET]));
                    }
                    break;
                }
                default:
                {
                    CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto, 0, 
                                         "NVM Access unknown command type (%u)", 
                                         data[NVM_ACCESS_COMMAND_TYPE_OFFSET]));
                    break;
                }
            }
            break;
        }

        default:
        {
            CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto, 0, 
                                 "Unknown command response received (%u)", 
                                 data[CMD_RSP_OFFSET]));
            break;
        }
    }
}
#endif

#if defined(CSR_QCA_CHIP_32BIT_SOC_SUPPORT)
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
static void csrTmQcHandleVsCommandComplete(CsrTmBlueCoreInstanceData *tmBcInst,
                                           CsrUint8 *data,
                                           CsrUint16 dataLen)
{
    CsrUint16 opcode = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(data + CSR_QC_HCIVS_CC_OPCODE_OFFSET);

    switch(opcode)
    {
        case CSR_QSOC_EDL_CMD_OPCODE:
        {
            if (data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET] == CSR_QSOC_PATCH_VER_REQ)
            {
                csrTmGetPatchVerRspHandler(tmBcInst, data + CSR_QC_HCIVS_CC_BUILD_VER_OFFSET);
                CsrTmBlueCoreStartPatchNvmDownload();
            }
            else if (data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET] == CSR_TLV_DOWNLOAD_REQ)
            {

                CsrTmBlueCoreHandlePatchNvmDownloadCfm(data[CSR_QC_HCIVS_CC_STATUS_OFFSET]);
            }
            else
            {
                CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto,
                                     0,
                                     "EDL command unknown sub-opcode (%u)",
                                     data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET]));
            }
            break;
        }

        case CSR_QSOC_NVM_ACCESS_OPCODE:
        {
            if (data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET] == NVM_ACCESS_COMMAND_TYPE_SET)
            {
                if (data[CSR_QC_HCIVS_CC_STATUS_OFFSET] == CSR_QC_HCI_VS_RESULT_SUCCESS)
                { /* Success case */
                    CsrQcHciResetReqSend();
                }
                else
                {
                    CsrTmQcPanic(SET_ADDRESS_FAILURE);
                }
            }
            else
            {
                CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto,
                                     0,
                                     "NVM Access unknown sub-opcode (%u)",
                                     data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET]));
            }
            break;
        }
        default:
        {
            CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto,
                                 0,
                                 "Unknown command opcode received (%u)",
                                 opcode));
            break;
        }
    }
}
#else
#if !defined(CSR_TARGET_PRODUCT_WEARABLE) || defined(CSR_TARGET_PRODUCT_WEARABLE_X86)
static void csrTmQcHandleVsCommandComplete(CsrTmBlueCoreInstanceData *tmBcInst,
                                           CsrUint8 *data,
                                           CsrUint16 dataLen)
{
    CsrUint16 opcode = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(data + CSR_QC_HCIVS_CC_OPCODE_OFFSET);

    switch(opcode)
    {
        case CSR_QSOC_EDL_CMD_OPCODE:
        {
            if (data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET] == CSR_QSOC_PATCH_VER_REQ)
            {
                tmBcInst->qcPatchDownloadState = DOWNLOAD_IDLE;
                qcGetPatchVerRspHandler(tmBcInst, data + CSR_QC_HCIVS_CC_BUILD_VER_OFFSET);
            }
            else if (data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET] == CSR_TLV_DOWNLOAD_REQ)
            {
                /* NVM download is in progress, just call the handler */
                qcPatchDownloadHandler(tmBcInst);
            }
            else
            {
                CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto,
                                     0,
                                     "EDL command unknown sub-opcode (%u)",
                                     data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET]));
            }
            break;
        }

        case CSR_QSOC_NVM_ACCESS_OPCODE:
        {
            if (data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET] == NVM_ACCESS_COMMAND_TYPE_GET)
            {
                csrTmQcReadDebugControlTagResHandler(tmBcInst,
                                                     &data[CSR_QC_HCIVS_CC_NVM_ACCESS_TAG_VALUE_OFFSET],
                                                     dataLen - CSR_QC_HCIVS_CC_NVM_ACCESS_TAG_VALUE_OFFSET);
            }
            else if (data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET] == NVM_ACCESS_COMMAND_TYPE_SET)
            {
                csrTmQcWriteDebugControlTagResHandler(tmBcInst);
            }
            else
            {
                CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto,
                                     0,
                                     "NVM Access unknown sub-opcode (%u)",
                                     data[CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET]));
            }
            break;
        }
        case CSR_QSOC_SET_BAUD_RATE_OPCODE:
        {
            CsrResult status = data[CSR_QC_HCIVS_CC_SET_BAUD_RES_STATUS_OFFSET];
            qcSetBaudrateCfmHandler(tmBcInst, status ^ 0x01);
            break;
        }
        default:
        {
            CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto,
                                 0,
                                 "Unknown command opcode received (%u)", 
                                 opcode));
            break;
        }
    }
}
#endif
#endif
#endif

static void CsrTmQcHciVsEvtIndHandler(CsrTmBlueCoreInstanceData *tmBcInst)
{
    CsrHciVendorSpecificEventInd *prim = tmBcInst->recvMsgP;
    CsrUint16 dataLen = CsrMblkGetLength(prim->data);
    CsrUint8 * data = CsrMblkMap(prim->data, 0, dataLen);

    CSR_LOG_ASSERT(data == NULL);

    if (prim->event == CSR_HCI_EV_VENDOR_SPECIFIC)
    {
#if !defined(CSR_TARGET_PRODUCT_WEARABLE) || defined(CSR_TARGET_PRODUCT_WEARABLE_X86)
        csrTmQcHandleVsEvent(tmBcInst,
                             data,
                             dataLen);
#endif
    }
#if defined(CSR_QCA_CHIP_32BIT_SOC_SUPPORT)
#if defined(PATCH_NVM_BUFFER_DOWNLOAD) || !defined(CSR_TARGET_PRODUCT_WEARABLE) || defined(CSR_TARGET_PRODUCT_WEARABLE_X86)
    else if (prim->event == CSR_HCI_EV_COMMAND_COMPLETE)
    {
        csrTmQcHandleVsCommandComplete(tmBcInst,
                                       data,
                                       dataLen);
    }
#endif
#endif
    else
    {
        CSR_LOG_TEXT_WARNING((CsrTmBluecoreLto, 0, "Received Event: %d Is Not Handled", prim->event));
    }

    CsrMblkUnmap(prim->data, data);
    CsrMblkDestroy(prim->data);
}

static void csrTmQcHciResetIndHandler(CsrTmBlueCoreInstanceData *tmBcInst)
{
    CsrHciResetInd *prim = (CsrHciResetInd *) tmBcInst->recvMsgP;
    CSR_LOG_TEXT_INFO((CsrTmBluecoreLto, 0, "Received RESET IND (state: %u)", tmBcInst->state));
    tmBcInst->nopHandler = prim->phandle;
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
    CsrHciUnregisterVendorSpecificEventHandlerReqSend(CSR_TM_BLUECORE_IFACEQUEUE, TRANSPORT_CHANNEL_QC_HCIVS);
#else
    switch (tmBcInst->state)
    {
        case CSR_TM_BLUECORE_STATE_ACTIVATING:
            /* Read the Tag 0x26 (tag38) which includes the Inhibit NOP: bit4 in byte 0 - 1 bit */
            CsrQcReadDebugControlTagReqSend();
            break;
        default:
            break;
    }
#endif
}

static void CsrTmQcHciUnRegisterVsEvtCfmHandler(CsrTmBlueCoreInstanceData *tmBcInst)
{
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
    if (!ftmMode)
    {
        CsrTmBlueCoreActivate(tmBcInst);
    }
    CsrTmBlueCoreSendBootstrapComplete();
#else
    CsrTmBlueCoreActivate(tmBcInst);
#endif
}
#if !defined(CSR_TARGET_PRODUCT_WEARABLE) || defined(CSR_TARGET_PRODUCT_WEARABLE_X86)
static void CsrTmQcHciResetTransportCfmHandler(CsrTmBlueCoreInstanceData *tmBcInst)
{
    CsrHciResetTransportCfm *cfm = (CsrHciResetTransportCfm *) tmBcInst->recvMsgP;

    tmBcInst->state = CSR_TM_BLUECORE_STATE_RESTARTING;
    tmBcInst->numberOfForcedReset += 1;

    if (cfm->restarted)
    {
        CsrUartDrvRtsSet(CSR_TM_QC_UART_HANDLE, TRUE);
        
        tmBcInst->timerId = CsrSchedTimerSet(QC_BAUDRATE_RESP_TIMEOUT,
                                             csrTmQcBaudrateRespTimeout,
                                             0,
                                             tmBcInst);
    }
    else
    {
        CSR_LOG_TEXT_ERROR((CsrTmBluecoreLto, 0, "Transport restart failed"));
    }
}
#endif
static void CsrTmQcHciRegisterVsEvtCfmHandler(CsrTmBlueCoreInstanceData *tmBcInst)
{
#if !defined(CSR_TARGET_PRODUCT_WEARABLE) || defined(CSR_TARGET_PRODUCT_WEARABLE_X86)
    if (csrTmQcBaudrateChange(tmBcInst))
    {
        /* Baudrate change initiated.
         * Wait for reset transport confirm from HCI */
    }
    else
#endif
    {
        tmBcInst->state = CSR_TM_BLUECORE_STATE_ACTIVATING;
        CsrQcGetPatchVerReqSend();
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
        if (ftmMode)
        { /* For FTM mode, DM task is not initialised so Register with HCI to receive HCI reset ind */
            CsrHciRegisterEventHandlerReqSend(CSR_TM_BLUECORE_IFACEQUEUE);
        }
#endif
    }
}

void CsrTmQcMsgHandler(CsrTmBlueCoreInstanceData *tmBcInst)
{
    CsrPrim *primType = (CsrPrim *) tmBcInst->recvMsgP;

    switch (*primType)
    {
        case CSR_HCI_RESET_IND:
            csrTmQcHciResetIndHandler(tmBcInst);
            break;
        case CSR_HCI_VENDOR_SPECIFIC_EVENT_IND:
            CsrTmQcHciVsEvtIndHandler(tmBcInst);
            break;
        case CSR_HCI_REGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_CFM:
            CsrTmQcHciRegisterVsEvtCfmHandler(tmBcInst);
            break;
        case CSR_HCI_UNREGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_CFM:
            CsrTmQcHciUnRegisterVsEvtCfmHandler(tmBcInst);
            break;
#if !defined(CSR_TARGET_PRODUCT_WEARABLE) || defined(CSR_TARGET_PRODUCT_WEARABLE_X86)
        case CSR_HCI_RESET_TRANSPORT_CFM:
            CsrTmQcHciResetTransportCfmHandler(tmBcInst);
            break;
#endif
        case CSR_HCI_REGISTER_EVENT_HANDLER_CFM:
            /* ignore */
            break;
        default:
            CsrGeneralException(CsrTmBluecoreLto,
                                0,
                                CSR_HCI_PRIM,
                                *primType,
                                tmBcInst->state,
                                NULL);
            break;
    }
}
