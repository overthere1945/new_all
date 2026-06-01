/******************************************************************************
 Copyright (c) 2016-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_util.h"
#include "csr_log_text_2.h"
#include "csr_message_queue.h"
#include "csr_tm_qc_hcivs_lib.h"
#include "csr_tm_qc_hcivs_util.h"
#include "csr_pmem.h"
#include "csr_hci_lib.h"
#include "csr_result.h"
#include "csr_tm_bluecore_sef.h"
#include "csr_panic.h"

void CsrQcSetBaudrateReqSend(CsrUint8 baud)
{
    CsrUint8 baudrate = baud;
    CsrUint16 qcmdLength;
    CsrUint8 *qcmd;

    /*CSR_LOG_TEXT_CONDITIONAL_ERROR(qcmdInst->state != CSR_QCMD_STATE_ACTIVATING_TRANSPORT,
        (CsrTmBluecoreLto, 0, "Unexpected CSR_QCMD_SET_BAUDRATE_REQ"));*/

    qcmd = CsrQcHciVsCommandBuild(CSR_QSOC_SET_BAUD_RATE_OPCODE, &baudrate, 0x01, &qcmdLength);
    csrSendQcHciVsData(qcmd, qcmdLength);
}

void CsrQcGetPatchVerReqSend(void)
{
    CsrUint8 patchVerCmd[] = {CSR_QSOC_PATCH_VER_REQ};
    CsrUint16 qcmdLength;
    CsrUint8 *qcmd;

    /*CSR_LOG_TEXT_CONDITIONAL_ERROR(qcmdInst->state != CSR_QCMD_STATE_ACTIVATING_TRANSPORT,
        (CsrTmBluecoreLto, 0, "Unexpected CSR_QCMD_GET_PATCH_VERSION_REQ"));*/

    qcmd = CsrQcHciVsCommandBuild(CSR_QSOC_EDL_CMD_OPCODE, patchVerCmd, 0x01, &qcmdLength);
    csrSendQcHciVsData(qcmd, qcmdLength);
}

void CsrQcHciResetReqSend(void)
{
    CsrUint16 qcmdLength;
    CsrUint8 *qcmd;

    qcmd = CsrHciCommandBuild(CSR_HCI_RESET_OPCODE, NULL, 0x00, &qcmdLength);
    CsrHciCommandReqSend(qcmdLength, qcmd);
}

#define FIRMWARE_LOG_OVER_HCI_DISABLE    0x00
#define FIRMWARE_LOG_OVER_HCI_ENABLE     0x01
#define HOST_LOG_CTRL_SUBID              0x00
#define HOST_LOG_CTRL_PARAM_LEN          0x02

void CsrQcControllerLogReqSend(CsrBool logEnable)
{
    CsrUint8 value[2] = {HOST_LOG_CTRL_SUBID, FIRMWARE_LOG_OVER_HCI_DISABLE};
    CsrUint16 qcmdLength;
    CsrUint8 *qcmd;
    /*CSR_LOG_TEXT_CONDITIONAL_ERROR(qcmdInst->state != CSR_QCMD_STATE_ACTIVATING_TRANSPORT,
        (CsrTmBluecoreLto, 0, "Unexpected CSR_QCMD_CONTROLLER_LOG_REQ"));*/
    if(logEnable)
    {
        value[1] = FIRMWARE_LOG_OVER_HCI_ENABLE;
    }
    qcmd = CsrQcHciVsCommandBuild(CSR_QSOC_HOST_CTL_LOG_OPCODE, 
                               value,
                               HOST_LOG_CTRL_PARAM_LEN,
                               &qcmdLength);
    csrSendQcHciVsData(qcmd, qcmdLength);
}

CsrBool CsrQcNvmChunkReqSend(CsrQcBin *qcBinVar)
{
    CsrUint32 payloadLen=0;
    CsrUint8 *payload;
    CsrBool status = FALSE;
    payload = csrGetQcBinData(qcBinVar, &payloadLen);
    if (payload)
    {
        CsrUint8 *qcmd;
        CsrUint16 qcmdLen;
        qcmd = CsrQcHciVsDownloadCmdBuild(CSR_QSOC_EDL_CMD_OPCODE, 
                                       CSR_TLV_DOWNLOAD_REQ,
                                       payload, 
                                       (CsrUint8)payloadLen,
                                       &qcmdLen);

        csrSendQcHciVsData(qcmd, qcmdLen);
        qcBinVar->currentIndex += payloadLen;
        status = TRUE;
    }
    return status;
}

void CsrQcReadDebugControlTagReqSend(void)
{
    CsrUint8 read_debug_control_opcode[] = {NVM_ACCESS_COMMAND_TYPE_GET, NVM_ACCESS_TAG_NUMBER_DEBUG_CONTROL};
    CsrUint16 qcmdLength;
    CsrUint8 *qcmd;

    qcmd = CsrQcHciVsCommandBuild
            (
                CSR_QSOC_NVM_ACCESS_OPCODE, 
                read_debug_control_opcode, 
                sizeof(read_debug_control_opcode), 
                &qcmdLength
            );
    csrSendQcHciVsData(qcmd, qcmdLength);
}

void CsrQcWriteDebugControlTagReqSend(CsrUint8 *debugControl, CsrUint8 length)
{
    CsrUint8 *qcmd;
    CsrUint16 qcmdLength;
    CsrUint8 *payload = CsrPmemZalloc(NVM_ACCESS_HEADER_SIZE + length);

    payload[0] = NVM_ACCESS_COMMAND_TYPE_SET;
    payload[1] = NVM_ACCESS_TAG_NUMBER_DEBUG_CONTROL;
    payload[2] = length;

    SynMemCpyS(&payload[NVM_ACCESS_HEADER_SIZE],
               length,
               debugControl,
               length);

    qcmd = CsrQcHciVsCommandBuild( CSR_QSOC_NVM_ACCESS_OPCODE,
                                   payload,
                                   NVM_ACCESS_HEADER_SIZE + length,
                                  &qcmdLength);
    csrSendQcHciVsData(qcmd, qcmdLength);

    CsrPmemFree(payload);
}

void CsrTmQcPanic(CsrUint32 p)
{
#ifdef CSR_TARGET_PRODUCT_WEARABLE
    p |= CSR_PANIC_FW_TM_BLUECORE_BASE;
#endif
    CsrPanic(CSR_TECH_FW, p, "QC TM panic");
}
