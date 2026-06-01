/*****************************************************************************
Copyright (c) 2018-2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
*****************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_macro.h"
#include "csr_util.h"
#include "csr_log_text_2.h"
#include "csr_log_gsp.h"
#include "csr_hci_lib.h"
#include "csr_transport.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_message_queue.h"
#include "csr_tm_qc_hcivs_lib.h"
#include "csr_tm_bluecore_sef.h"
#include "hci_cmd_arb.h"


#define CSR_HCI_STD_HEADER_SIZE                          0x03

CsrUint8 *csrGetQcBinData(CsrQcBin *binVar, CsrUint32 *length)
{
    CsrUint8 *payload = NULL;
    CsrUint32 payloadLength;
    
    if ((binVar->currentIndex + MAX_LEN_BIN_FILE_SEGMENT) >= binVar->payloadLength)
    {
        payloadLength = binVar->payloadLength - binVar->currentIndex;
    }
    else
    {
        payloadLength = MAX_LEN_BIN_FILE_SEGMENT;
    }
    
    if (payloadLength > 0)
    {
        payload = &(binVar->payload[binVar->currentIndex]);
    }
    
    *length = payloadLength;
    
    return payload;
}

void csrSendQcHciVsData(CsrUint8 *payload, CsrUint16 payloadLength)
{
    CsrMblk *data;
    data = CsrMblkDataCreate(payload, payloadLength, TRUE);
//#if (CSR_HOST_PLATFORM == Q6_HOST) 
#ifndef CSR_TARGET_PRODUCT_WEARABLE
    CsrHciVendorSpecificCommandReqSend(TRANSPORT_CHANNEL_QC_HCIVS, data);
#else 
    HciCommandArbVSCmdSend(HCI_CMD_SENDER_MICRO_STACK, data);
#endif
}

/* TBD: Check if this can be done via HCI api's itself*/
CsrUint8 *CsrHciCommandBuild(CsrUint16 opCode,
    CsrUint8 *payload,
    CsrUint8 payloadLength,
    CsrUint16 *qcmdLength)
{
    CsrUint8 *qcmd;
    CsrUint16 totalLen = CSR_HCI_STD_HEADER_SIZE;
    CsrUint16 count = 0;
    
    if (payload && (payloadLength > 0))
    {
        totalLen = (CsrUint16) (totalLen + payloadLength);
    }
    else
    {
        payloadLength = 0;
    }
    
    qcmd = (CsrUint8 *) CsrPmemZalloc(totalLen);
    
    qcmd[count++] = (CsrUint8)(opCode & 0xFF);
    qcmd[count++] = (CsrUint8)((opCode>>8) & 0xFF);
    qcmd[count++] = (CsrUint8)(payloadLength);
    
    if (payloadLength > 0)
    {
        SynMemCpyS(&(qcmd[count]), payloadLength, payload, payloadLength);
    }
    
    *qcmdLength = totalLen;
    
    return qcmd;
}

CsrUint8 *CsrQcHciVsCommandBuild(CsrUint16 opCode,
    CsrUint8 *payload,
    CsrUint8 payloadLength,
    CsrUint16 *qcmdLength)
{
    CsrUint8 *qcmd;
    CsrUint16 totalLen = CSR_QC_HCIVS_STD_HEADER_SIZE;
    CsrUint16 count = 0;
    
    if (payload && (payloadLength > 0))
    {
        totalLen = (CsrUint16) (totalLen + payloadLength);
    }
    else
    {
        payloadLength = 0;
    }
    
    qcmd = (CsrUint8 *) CsrPmemZalloc(totalLen);
    
    qcmd[count++] = (CsrUint8)(opCode & 0xFF);
    qcmd[count++] = (CsrUint8)((opCode>>8) & 0xFF);
    qcmd[count++] = (CsrUint8)(payloadLength);
    
    if (payloadLength > 0)
    {
        SynMemCpyS(&(qcmd[count]), payloadLength, payload, payloadLength);
    }
    
    *qcmdLength = totalLen;
    
    return qcmd;
}

CsrUint8 *CsrQcHciVsDownloadCmdBuild(CsrUint16 opCode, 
    CsrUint8 reqCmd,
    CsrUint8 *payload,
    CsrUint8 payloadLength,
    CsrUint16 *qcmdLength)
{
    CsrUint8 *qcmd;
    CsrUint16 totalLen = CSR_QC_HCIVS_STD_HEADER_SIZE;
    CsrUint16 count = 0;
    
    if (payloadLength > MAX_LEN_BIN_FILE_SEGMENT)
    {
        CSR_LOG_TEXT_ERROR((CsrTmBluecoreLto, 0,
                            "Invalid parameter payloadLength: %d", payloadLength));
        return NULL;
    }
    
    totalLen = (CsrUint16) (totalLen + payloadLength + 2);
    qcmd = (CsrUint8 *) CsrPmemZalloc(totalLen);
    
    qcmd[count++] = (CsrUint8)(opCode & 0xFF);
    qcmd[count++] = (CsrUint8)((opCode>>8) & 0xFF);
    qcmd[count++] = (CsrUint8)(payloadLength + 2);
    qcmd[count++] = (CsrUint8)(reqCmd & 0xFF);
    qcmd[count++] = (CsrUint8)(payloadLength);
    
    if (payloadLength > 0)
    {
        SynMemCpyS(&(qcmd[count]), payloadLength, payload, payloadLength);
    }
    
    *qcmdLength = totalLen;
    
    return qcmd;
}

