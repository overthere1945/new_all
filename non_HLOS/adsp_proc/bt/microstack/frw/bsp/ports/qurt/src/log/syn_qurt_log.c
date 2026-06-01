/******************************************************************************
Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #17 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_util.h"
#include "csr_log_text_2.h"
#include "csr_transport.h"
#include "csr_bt_profiles.h"
#include "csr_bt_tasks.h"
#include "syn_qurt_log.h"
#include "csr_time.h"
#ifdef SYN_LOG_EXT_PRIM
#include "csr_bt_hf_prim.h"
#include "csr_bt_hfg_prim.h"
#include "csr_bt_av_prim.h"
#include "csr_bt_avrcp_prim.h"
#endif
#if (CSR_HOST_PLATFORM == QCC5100_HOST)
#include "qapi_dbg_log.h"
#endif
#include "csr_log_gsp.h"

#ifdef SYN_DEBUG_LOGS_BUFFER_ENABLE
#include "csr_framework_ext.h"
#endif

/* Macros used for snoop file formatting */
#define REC_HDR_FIELD_WIDTH_4           4
#define REC_HDR_FIELD_WIDTH_8           8
#define PACKET_REC_HDR_SIZE             24
#define BIT_MASK_64BIT                  0x00000000000000FF
#define BIT_MASK_64BIT_2                0xFFFFFFFFFFFFFFFF
#define BIT_MASK_64BIT_3                0xFFFFFFFF00000000
#define LOW_EPOCH_TIME                  0x4A676000
#define HIGH_EPOCH_TIME                 0x00E03AB4

/* HCI PKT FLAG - Bit0:(0=Tx,1=Rx) Bit1:(0=ACL data, 1=Cmd/Evt) */
#define HCI_EVT_PKT_FLAG                0x03
#define HCI_ACL_RX_PKT_FLAG             0x01
#define HCI_CMD_PKT_FLAG                0x02
#define HCI_ACL_TX_PKT_FLAG             0x00

#ifdef DUMP_BT_SNOOP_FMT_TO_DBG_UART
#define MAX_PAYLOAD_LEN_TO_INCLUDE      550
#else
#define MAX_PAYLOAD_LEN_TO_INCLUDE      100
#endif


#ifdef CSR_LOG_ENABLE

#include "log_codes.h"
#include "log.h"

typedef struct
{
  log_hdr_type hdr;
  CsrUint8 data[1];
} btLogPktStT;

static CsrUint16 snoopFlags = SYN_HCI_ACL_RX_ENH_PKT_FLAG;
static CsrBool snoopEnabled = FALSE;

#ifdef SYN_DEBUG_LOGS_BUFFER_ENABLE

#define SYN_QURT_MAX_MSG_ENTRIES   100
typedef struct 
{
    struct
    {
        CsrUint8 index;
        CsrUint8 isPut;
        CsrPrim msgType;
        CsrUint16 mi;
        CsrSchedQid src;
        CsrSchedQid dst;
    }msg[SYN_QURT_MAX_MSG_ENTRIES];
    CsrUint16 wrIndex;
    CsrMutexHandle logMutex;
}SynMsgLog;

SynMsgLog gMsgLog;

void SynStoreLogInfo(CsrUint8 isPut, CsrPrim msgType, CsrUint16 mi, CsrSchedQid src, CsrSchedQid dst)
{
    CsrMutexLock(&gMsgLog.logMutex);
    gMsgLog.msg[gMsgLog.wrIndex].index   = gMsgLog.wrIndex;
    gMsgLog.msg[gMsgLog.wrIndex].isPut   = isPut;
    gMsgLog.msg[gMsgLog.wrIndex].msgType = msgType;
    gMsgLog.msg[gMsgLog.wrIndex].mi      = mi;
    gMsgLog.msg[gMsgLog.wrIndex].src     = src;
    gMsgLog.msg[gMsgLog.wrIndex].dst     = dst;
    gMsgLog.wrIndex = (gMsgLog.wrIndex +1 ) % SYN_QURT_MAX_MSG_ENTRIES;   
    CsrMutexUnlock(&gMsgLog.logMutex);
}

void SynLogInit(void)
{
    CsrMutexCreate(&gMsgLog.logMutex);
}

void SynLogDeinit(void)
{
    CsrMutexDestroy(&gMsgLog.logMutex);
}
#endif

CsrBool CsrLogTaskIsFiltered(CsrSchedQid taskId, CsrLogLevelTask level)
{
    CSR_UNUSED(taskId);
    CSR_UNUSED(level);
    return FALSE;
}

void CsrLogMessageQueuePush(CsrUint16 prim_type, const void *ptr)
{
    CsrUint16 msgType;
    if (ptr)
    {
        msgType = *((CsrUint16 *) ptr);
        CONSTRUCT_MSG("Save to Queue: %x, %x", prim_type, msgType);
    }
}

void CsrLogMessageQueuePop(CsrUint16 prim_type, const void *ptr)
{
    CsrUint16 msgType;
    if (ptr)
    {
        msgType = *((CsrUint16 *) ptr);
        CONSTRUCT_MSG("Pop from Queue: %x, %x", prim_type, msgType);
    }
}

void SynLogEnableSnoop(CsrBool enable)
{
    snoopEnabled = enable;
}

void SynLogConfigureSnoop(CsrUint16 flag)
{
    snoopFlags = flag;
}

#ifdef STANDALONE_DUT_PLUS
static CsrUint8 *getSnoopRecordMemory(CsrUint16 size)
{
#ifdef DUMP_BT_SNOOP_FMT_TO_DBG_UART
    void *p = NULL;
    qapi_BTSnoopLogRsp_t rsp ;
    rsp = qapi_BTSnoopLog_Alloc(&p, size);
    if (rsp != QAPI_BT_SNOOP_RSP_SUCCESS)
    {
        CONSTRUCT_HIGH_MSG("qapi_BTSnoopLog_Alloc: %x", rsp);
    }
    return p;
#else
    return CsrPmemAlloc(size);
#endif
}

static void logBTSnoopRecord(CsrUint8 * snoopRecMem, CsrSize payloadLength)
{
#ifdef DUMP_BT_SNOOP_FMT_TO_DBG_UART
    /* Commit to shared memory for snoop */
    qapi_BTSnoopLogRsp_t rsp = qapi_BTSnoopLog_Commit(snoopRecMem, payloadLength + PACKET_REC_HDR_SIZE);
    if (rsp != QAPI_BT_SNOOP_RSP_SUCCESS)
    {
        CONSTRUCT_HIGH_MSG("qapi_BTSnoopLog_Commit: %x", rsp);
    }
#else
    /* Else print to QCLI */
    CONSTRUCT_HIGH_MSG("SNOOP: %d", payloadLength + 24);
    DMSG_HEXDUMP(payloadLength + PACKET_REC_HDR_SIZE, (uint8_t*)snoopRecMem);
    CsrPmemFree(snoopRecMem);
#endif
}

void CsrLogBciSnoop(log_code_type code,
                    CsrSize payloadLength,
                    const void *payload)
{
    CsrUint8 pktFlag = 0x00;
    uint64_t timestamp = 0x00;
    CsrTime now;
    CsrUint16 logHandle = 0x00;
    CsrUint8 * snoopRecMem = NULL;
    CsrUint8 iter = 0;
    CsrSize actualLength = payloadLength;

    if(!(snoopFlags & SYN_HCI_ACL_RX_INC_FULL_PAYLOAD_LEN) &&
        ((code == LOG_BT_HCI_RX_ACL_C || code == LOG_BT_HCI_TX_ACL_C) &&
        payloadLength > MAX_PAYLOAD_LEN_TO_INCLUDE))
    {
        payloadLength = MAX_PAYLOAD_LEN_TO_INCLUDE;
    }

    now = CsrTimeGet(NULL);

    {
        switch (code)
        {
            case LOG_BT_HCI_EV_C:
                pktFlag = HCI_EVT_PKT_FLAG;
                break;
            case LOG_BT_HCI_RX_ACL_C:
            {
                if(payloadLength >= 2)
                {
                    logHandle = ((((uint8_t*)payload)[1] << 8) | ((uint8_t*)payload)[0]);
                }

                if((logHandle == BTSS_ENH_LOG_CONN_HANDLE) && !(snoopFlags & SYN_HCI_ACL_RX_ENH_PKT_FLAG))
                    return;

                pktFlag = HCI_ACL_RX_PKT_FLAG;
                break;
            }
            case LOG_BT_HCI_CMD_C:
                pktFlag = HCI_CMD_PKT_FLAG;
                break;
            case LOG_BT_HCI_TX_ACL_C:
                pktFlag = HCI_ACL_TX_PKT_FLAG;
                break;
            default:
                break;
        }
    
        snoopRecMem = getSnoopRecordMemory(payloadLength + PACKET_REC_HDR_SIZE);
        if(snoopRecMem != NULL)
        {
            /* Form snoop record, start with original length */
            snoopRecMem[iter + 3] = actualLength & 0xFF;
            snoopRecMem[iter + 2] = (actualLength >> 8) & 0xFF;
            snoopRecMem[iter + 1] = (actualLength >> 16) & 0xFF;
            snoopRecMem[iter] = (actualLength >> 24) & 0xFF;
            iter += REC_HDR_FIELD_WIDTH_4;

            /* Included length */
            snoopRecMem[iter + 3] = payloadLength & 0xFF;
            snoopRecMem[iter + 2] = (payloadLength >> 8) & 0xFF;
            snoopRecMem[iter + 1] = (payloadLength >> 16) & 0xFF;
            snoopRecMem[iter] = (payloadLength >> 24) & 0xFF;
            iter += REC_HDR_FIELD_WIDTH_4;

            /* Packet flag */
            snoopRecMem[iter + 3] = pktFlag & 0xFF;
            snoopRecMem[iter + 2] = 0x00;
            snoopRecMem[iter + 1] = 0x00;
            snoopRecMem[iter] = 0x00;
            iter += REC_HDR_FIELD_WIDTH_4;

            /* Set cumulative drop to all zeros */
            snoopRecMem[iter + 3] = 0x00;
            snoopRecMem[iter + 2] = 0x00;
            snoopRecMem[iter + 1] = 0x00;
            snoopRecMem[iter] = 0x00;
            iter += REC_HDR_FIELD_WIDTH_4;

            timestamp = 0x00;
            timestamp += now;
            timestamp += (((HIGH_EPOCH_TIME & BIT_MASK_64BIT_2) << 32) & BIT_MASK_64BIT_3) | \
                (LOW_EPOCH_TIME);

            snoopRecMem[iter + 7] = timestamp & BIT_MASK_64BIT;
            snoopRecMem[iter + 6] = (timestamp >> 8) & BIT_MASK_64BIT;
            snoopRecMem[iter + 5] = (timestamp >> 16) & BIT_MASK_64BIT;
            snoopRecMem[iter + 4] = (timestamp >> 24) & BIT_MASK_64BIT;
            snoopRecMem[iter + 3] = (timestamp >> 32) & BIT_MASK_64BIT;
            snoopRecMem[iter + 2] = (timestamp >> 40) & BIT_MASK_64BIT;
            snoopRecMem[iter + 1] = (timestamp >> 48) & BIT_MASK_64BIT;
            snoopRecMem[iter + 0] = (timestamp >> 56) & BIT_MASK_64BIT;
            iter += REC_HDR_FIELD_WIDTH_8;
            /* Get the real payload bytes */
            SynMemCpyS(snoopRecMem + iter, payloadLength, payload, payloadLength);

            /* log the snoop record */
            logBTSnoopRecord(snoopRecMem, payloadLength);
        }
    }
}
#endif

void CsrLogBci(CsrUint8 channel,
        CsrBool received,
        CsrSize payloadLength,
        const void *payload)
{
    log_code_type code;

    if (!snoopEnabled || !payload)
    {
        return;
    }

    if (received)
    {
        if (channel == TRANSPORT_CHANNEL_HCI)
        {
            code = LOG_BT_HCI_EV_C;
        }
        else if (channel == TRANSPORT_CHANNEL_PERI)
        {
            // Do nothing for TRANSPORT_CHANNEL_PERI
            return;
        }
        else
        {
            code = LOG_BT_HCI_RX_ACL_C;
        }
    }
    else
    {
        if (channel == TRANSPORT_CHANNEL_HCI)
        {
            code = LOG_BT_HCI_CMD_C;
        }
        else if (channel == TRANSPORT_CHANNEL_PERI)
        {
            // Do nothing for TRANSPORT_CHANNEL_PERI
            return;
        }
        else
        {
            code = LOG_BT_HCI_TX_ACL_C;
        }
    }

#ifdef STANDALONE_DUT_PLUS
    {
        CsrLogBciSnoop(code, payloadLength, payload);
    }
#else
    {
        btLogPktStT *btLogPkt = NULL;
        CsrUint8 btLogPktHdrSize = sizeof(btLogPktStT) - 1;
    
        btLogPkt = log_alloc(code, btLogPktHdrSize + payloadLength);
        if (btLogPkt)
        {
            SynMemCpyS(btLogPkt->data, payloadLength, payload, payloadLength);
            log_commit(btLogPkt);
        }
    }
#endif /* STANDALONE_DUT_PLUS */
}

#ifdef SYN_LOG_EXT_PRIM
#ifndef EXCLUDE_CSR_BT_HF_MODULE
static void synLogPrimHf (void *prim)
{
    CsrPrim *type = prim;
    CsrUint8 i;

    switch (*type)
    {
        case CSR_BT_HF_ACTIVATE_REQ:
        {
            CsrBtHfActivateReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_ACTIVATE_REQ Handle %x MaxHFConn %x MaxHSConn %x MaxSimCon %x ",
                req->phandle, req->maxHFConnections, req->maxHSConnections, req->maxSimultaneousConnections);
            CONSTRUCT_HIGH_MSG("supportedFeatures %x hfConfigMask %x atResponseTime %x hfSupportedHfIndicatorsCount %x hfSupportedHfIndicatorsList:",
                req->supportedFeatures, req->hfConfig, req->atResponseTime, req->hfSupportedHfIndicatorsCount);
            for (i=0; i<req->hfSupportedHfIndicatorsCount; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->hfSupportedHfIndicators[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_HF_ACTIVATE_CFM:
        {
            CsrBtHfActivateCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_ACTIVATE_CFM resultCode %x resultSupplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_HF_DEACTIVATE_REQ:
        {
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_DEACTIVATE_REQ \n");
        }
        break;
        case CSR_BT_HF_DEACTIVATE_CFM:
        {
            CsrBtHfDeactivateCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_DEACTIVATE_CFM resultCode %x resultSupplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_HF_SERVICE_CONNECT_REQ:
        {
            CsrBtHfServiceConnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SERVICE_CONNECT_REQ deviceAddr lap %x uap %x nap %x connectionType %x ",
                req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap, req->connectionType);
            CONSTRUCT_HIGH_MSG("userAction %x\n", req->userAction);
        }
        break;
        case CSR_BT_HF_SERVICE_CONNECT_IND:
        {
            CsrBtHfServiceConnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SERVICE_CONNECT_IND connectionId %x supportedFeatures %x network %x remoteVersion %x ",
                ind->connectionId, ind->supportedFeatures, ind->network, ind->remoteVersion);
            CONSTRUCT_HIGH_MSG("deviceAddr lap %x uap %x nap %x connectionType %x ",
                ind->deviceAddr.lap, ind->deviceAddr.uap, ind->deviceAddr.nap, ind->connectionType);
            CONSTRUCT_HIGH_MSG("resultCode %x resultSupplier %x btConnId %x hfgSupportedHfIndicatorsCount %x \n",
                ind->resultCode, ind->resultSupplier, ind->btConnId, ind->hfgSupportedHfIndicatorsCount);
            for (i=0; i<ind->hfgSupportedHfIndicatorsCount; i++)
            {
                CONSTRUCT_HIGH_MSG("hfIndicatorID %x status %x\n", ind->hfgSupportedHfIndicators[i].hfIndicatorID, ind->hfgSupportedHfIndicators[i].status);
            }
        }
        break;
        case CSR_BT_HF_SERVICE_CONNECT_CFM:
        {
            CsrBtHfServiceConnectCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SERVICE_CONNECT_CFM connectionId %x supportedFeatures %x network %x remoteVersion %x ",
                cfm->connectionId, cfm->supportedFeatures, cfm->network, cfm->remoteVersion);
            CONSTRUCT_HIGH_MSG("deviceAddr lap %x uap %x nap %x connectionType %x ",
                cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap, cfm->connectionType);
            CONSTRUCT_HIGH_MSG("resultCode %x resultSupplier %x btConnId %x hfgSupportedHfIndicatorsCount %x \n",
                cfm->resultCode, cfm->resultSupplier, cfm->btConnId, cfm->hfgSupportedHfIndicatorsCount);
            for (i=0; i<cfm->hfgSupportedHfIndicatorsCount; i++)
            {
                CONSTRUCT_HIGH_MSG("hfIndicatorID %x status %x\n",
                    cfm->hfgSupportedHfIndicators[i].hfIndicatorID, cfm->hfgSupportedHfIndicators[i].status);
            }
        }
        break;
        case CSR_BT_HF_CANCEL_CONNECT_REQ:
        {
            CsrBtHfCancelConnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_CANCEL_CONNECT_REQ deviceAddr lap %x uap %x lap %x\n",
                req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap);
        }
        break;
        case CSR_BT_HF_DISCONNECT_REQ:
        {
            CsrBtHfDisconnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_DISCONNECT_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HF_DISCONNECT_IND:
        {
            CsrBtHfDisconnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_DISCONNECT_IND connectionId %x reasonCode %x reasonSupplier %x\n",
                ind->connectionId, ind->reasonCode, ind->reasonSupplier);
        }
        break;
        case CSR_BT_HF_DISCONNECT_CFM:
        {
            CsrBtHfDisconnectCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_DISCONNECT_CFM connectionId %x reasonCode %x reasonSupplier %x\n",
                cfm->connectionId, cfm->reasonCode, cfm->reasonSupplier);
        }
        break;
        case CSR_BT_HF_AUDIO_CONNECT_REQ:
        {
            CsrBtHfAudioConnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_AUDIO_CONNECT_REQ connectionId %x pcmSlot %x pcmRealloc %x audioParametersLength %x audioParameters:\n",
                req->connectionId, req->pcmSlot, req->pcmRealloc, req->audioParametersLength);
            for (i=0; i<req->audioParametersLength; i++)
            {
                CONSTRUCT_HIGH_MSG("packetType %x txBandwidth %x rxBandwidth %x maxLatency %x ",
                    req->audioParameters[i].packetType, req->audioParameters[i].txBandwidth, req->audioParameters[i].rxBandwidth, req->audioParameters[i].maxLatency);
                CONSTRUCT_HIGH_MSG("voiceSettings %x reTxEffort %x\n", req->audioParameters[i].voiceSettings, req->audioParameters[i].reTxEffort);
            }
        }
        break;
        case CSR_BT_HF_AUDIO_CONNECT_IND:
        {
            CsrBtHfAudioConnectInd *ind =prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_AUDIO_CONNECT_IND connectionId %x scoHandle %x pcmSlot %x linkType %x ",
                ind->connectionId, ind->scoHandle, ind->pcmSlot, ind->linkType);
            CONSTRUCT_HIGH_MSG("txInterval %x weSco %x rxPacketLength %x txPacketLength %x ",
                ind->txInterval, ind->weSco, ind->rxPacketLength, ind->txPacketLength);
            CONSTRUCT_HIGH_MSG("airMode %x qceCodecId %x resultCode %x resultSupplier %x\n",
                ind->airMode, ind->qceCodecId, ind->resultCode, ind->resultSupplier);
        }
        break;
        case CSR_BT_HF_AUDIO_CONNECT_CFM:
        {
            CsrBtHfAudioConnectCfm *cfm =prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_AUDIO_CONNECT_CFM connectionId %x scoHandle %x pcmSlot %x linkType %x ",
                cfm->connectionId, cfm->scoHandle, cfm->pcmSlot, cfm->linkType);
            CONSTRUCT_HIGH_MSG("txInterval %x weSco %x rxPacketLength %x txPacketLength %x ",
                cfm->txInterval, cfm->weSco, cfm->rxPacketLength, cfm->txPacketLength);
            CONSTRUCT_HIGH_MSG("airMode %x qceCodecId %x resultCode %x resultSupplier %x\n",
                cfm->airMode, cfm->qceCodecId, cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES:
        {
            CsrBtHfAudioAcceptConnectRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_AUDIO_ACCEPT_CONNECT_RES connectionId %x acceptResponse %x pcmSlot %x pcmReassign %x ",
                res->connectionId, res->acceptResponse, res->pcmSlot, res->pcmReassign);
            CONSTRUCT_HIGH_MSG("acceptParametersLength %x acceptParameters:\n", res->acceptParametersLength);
            for (i=0; i<res->acceptParametersLength; i++)
            {
                CONSTRUCT_HIGH_MSG("packetTypes %x txBandwidth %x rxBandwidth %x ",
                    res->acceptParameters[i].packetTypes, res->acceptParameters[i].txBandwidth, res->acceptParameters[i].rxBandwidth);
                CONSTRUCT_HIGH_MSG("maxLatency %x contentFormat %x reTxEffort %x\n",
                    res->acceptParameters[i].maxLatency, res->acceptParameters[i].contentFormat, res->acceptParameters[i].reTxEffort);
            }
        }
        break;
        case CSR_BT_HF_AUDIO_ACCEPT_CONNECT_IND:
        {
            CsrBtHfAudioAcceptConnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_AUDIO_ACCEPT_CONNECT_IND connectionId %x linkType %x\n",
                ind->connectionId, ind->linkType);
        }
        break;
        case CSR_BT_HF_AUDIO_DISCONNECT_REQ:
        {
            CsrBtHfAudioDisconnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_AUDIO_DISCONNECT_REQ connectionId %x scoHandle %x\n",
                req->connectionId, req->scoHandle);
        }
        break;
        case CSR_BT_HF_AUDIO_DISCONNECT_IND:
        {
            CsrBtHfAudioDisconnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_AUDIO_DISCONNECT_IND connectionId %x scoHandle %x resultCode %x  resultSupplier %x\n",
                ind->connectionId, ind->scoHandle, ind->reasonCode, ind->reasonSupplier);
        }
        break;
        case CSR_BT_HF_AUDIO_DISCONNECT_CFM:
        {
            CsrBtHfAudioDisconnectCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_AUDIO_DISCONNECT_CFM connectionId %x scoHandle %x resultCode %x  resultSupplier %x\n",
                cfm->connectionId, cfm->scoHandle, cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_HF_STATUS_INDICATOR_UPDATE_IND:
        {
            CsrBtHfStatusIndicatorUpdateInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_STATUS_INDICATOR_UPDATE_IND connectionId %x index %x value %x\n",
                ind->connectionId, ind->index, ind->value);
        }
        break;
        case CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ:
        {
            CsrBtHfGetAllStatusIndicatorsReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HF_GET_ALL_STATUS_INDICATORS_CFM:
        {
            CsrBtHfGetAllStatusIndicatorsCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_GET_ALL_STATUS_INDICATORS_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ:
        {
            CsrBtHfGetCurrentCallListReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HF_GET_CURRENT_CALL_LIST_IND:
        {
            CsrBtHfGetCurrentCallListInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_GET_CURRENT_CALL_LIST_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HF_GET_CURRENT_CALL_LIST_CFM:
        {
            CsrBtHfGetCurrentCallListCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_GET_CURRENT_CALL_LIST_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ:
        {
            CsrBtHfSetCallNotificationIndicationReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ connectionId %x enable%x\n",
                req->connectionId, req->enable);
        }
        break;
        case CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_CFM:
        {
            CsrBtHfSetCallNotificationIndicationCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ:
        {
            CsrBtHfSetCallWaitingNotificationReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ connectionId %x enable %x\n",
                req->connectionId, req->enable);
        }
        break;
        case CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_CFM:
        {
            CsrBtHfSetCallWaitingNotificationCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ:
        {
            CsrBtHfSetStatusIndicatorUpdateReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ connectionId %x enable %x\n",
                req->connectionId, req->enable);
        }
        break;
        case CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_CFM:
        {
            CsrBtHfSetStatusIndicatorUpdateCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_INBAND_RING_SETTING_CHANGED_IND:
        {
            CsrBtHfInbandRingSettingChangedInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_INBAND_RING_SETTING_CHANGED_IND connectionId %x inbandRingingActivated %x\n",
                ind->connectionId, ind->inbandRingingActivated);
        }
        break;
        case CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ:
        {
            CsrBtHfSpeakerGainStatusReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ connectionId %x gain %x\n",
                req->connectionId, req->gain);
        }
        break;
        case CSR_BT_HF_SPEAKER_GAIN_STATUS_CFM:
        {
            CsrBtHfSpeakerGainStatusCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SPEAKER_GAIN_STATUS_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_SPEAKER_GAIN_IND:
        {
            CsrBtHfSpeakerGainInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SPEAKER_GAIN_IND connectionId %x gain %x\n",
                ind->connectionId, ind->gain);
        }
        break;
        case CSR_BT_HF_MIC_GAIN_STATUS_REQ:
        {
            CsrBtHfMicGainStatusReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_MIC_GAIN_STATUS_REQ connectionId %x gain %x\n",
                req->connectionId, req->gain);
        }
        break;
        case CSR_BT_HF_MIC_GAIN_STATUS_CFM:
        {
            CsrBtHfMicGainStatusCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_MIC_GAIN_STATUS_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_MIC_GAIN_IND:
        {
            CsrBtHfMicGainInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_MIC_GAIN_IND connectionId %x gain %x\n", ind->connectionId, ind->gain);
        }
        break;
        case CSR_BT_HF_DIAL_REQ:
        {
            CsrBtHfDialReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_DIAL_REQ connectionId %x command %x\n", req->connectionId, req->command);
        }
        break;
        case CSR_BT_HF_DIAL_CFM:
        {
            CsrBtHfDialCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_DIAL_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_CALL_ANSWER_REQ:
        {
            CsrBtHfCallAnswerReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_CALL_ANSWER_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HF_CALL_ANSWER_CFM:
        {
            CsrBtHfCallAnswerCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_CALL_ANSWER_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_CALL_END_REQ:
        {
            CsrBtHfCallEndReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_CALL_END_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HF_CALL_END_CFM:
        {
            CsrBtHfCallEndCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_CALL_END_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_CALL_HANDLING_REQ:
        {
            CsrBtHfCallHandlingReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_CALL_HANDLING_REQ connectionId %x command %x index %x\n",
                req->connectionId, req->command, req->index);
        }
        break;
        case CSR_BT_HF_CALL_HANDLING_IND:
        {
            CsrBtHfCallHandlingInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_CALL_HANDLING_IND connectionId %x event %x\n", ind->connectionId, ind->event);
        }
        break;
        case CSR_BT_HF_CALL_HANDLING_CFM:
        {
            CsrBtHfCallHandlingCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_CALL_HANDLING_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_CALL_RINGING_IND:
        {
            CsrBtHfCallRingingInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_CALL_RINGING_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HF_CALL_NOTIFICATION_IND:
        {
            CsrBtHfCallNotificationInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_CALL_NOTIFICATION_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HF_CALL_WAITING_NOTIFICATION_IND:
        {
            CsrBtHfCallWaitingNotificationInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_CALL_WAITING_NOTIFICATION_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HF_SELECTED_CODEC_IND:
        {
            CsrBtHfSelectedCodecInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SELECTED_CODEC_IND connectionId %x codecToUse %x\n", ind->connectionId, ind->codecToUse);
        }
        break;
#if 0
        case CSR_BT_HF_CONFIG_LOW_POWER_REQ:
        {
            CsrBtHfConfigLowPowerReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_CONFIG_LOW_POWER_REQ connectionId %x mask %x\n",
                req->connectionId, req->mask);
        }
        break;
        case CSR_BT_HF_CONFIG_LOW_POWER_CFM:
        {
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_CONFIG_LOW_POWER_CFM\n");
        }
        break;
        case CSR_BT_HF_STATUS_LOW_POWER_IND:
        {
            CsrBtHfStatusLowPowerInd *req = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_STATUS_LOW_POWER_IND connectionId %x currentMode %x oldMode %x wantedMode %x ",
                req->connectionId, req->currentMode, req->oldMode, req->wantedMode);
            CONSTRUCT_HIGH_MSG("remoteReq %x resultCode %x resultSupplier %x\n",
                req->remoteReq, req->resultCode, req->resultSupplier);
        }
        break;
        case CSR_BT_HF_CONFIG_AUDIO_REQ:
        {
            CsrBtHfConfigAudioReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_CONFIG_AUDIO_REQ connectionId %x audioType %x audioSettingLen %x audioSetting:",
                req->connectionId, req->audioType, req->audioSettingLen);
            for (i=0; i<req->audioSettingLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->audioSetting[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_HF_CONFIG_AUDIO_CFM:
        {
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_CONFIG_AUDIO_CFM\n");
        }
        break;
        case CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ:
        {
            CsrBtHfGetCurrentOperatorSelectionReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ connectionId %x mode %x format %x forceResendingFormat %x\n",
                req->connectionId, req->mode, req->format, req->forceResendingFormat);
        }
        break;
        case CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_CFM:
        {
            CsrBtHfGetCurrentOperatorSelectionCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ:
        {
            CsrBtHfGetSubscriberNumberInformationReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_IND:
        {
            CsrBtHfGetSubscriberNumberInformationInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_CFM:
        {
            CsrBtHfGetSubscriberNumberInformationCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ:
        {
            CsrBtHfSetExtendedAgErrorResultCodeReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ connectionId %x enable %x\n",
                req->connectionId, req->enable);
        }
        break;
        case CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_CFM:
        {
            CsrBtHfSetExtendedAgErrorResultCodeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        case CSR_BT_HF_SET_ECHO_AND_NOISE_REQ:
        {
            CsrBtHfSetEchoAndNoiseReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SET_ECHO_AND_NOISE_REQ connectionId %x enable %x\n",
                req->connectionId, req->enable);
        }
        break;
        case CSR_BT_HF_SET_ECHO_AND_NOISE_CFM:
        {
            CsrBtHfSetEchoAndNoiseCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SET_ECHO_AND_NOISE_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_SET_VOICE_RECOGNITION_REQ:
        {
            CsrBtHfSetVoiceRecognitionReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SET_VOICE_RECOGNITION_REQ connectionId %x value %x\n",
                req->connectionId, req->value);
        }
        break;
        case CSR_BT_HF_SET_VOICE_RECOGNITION_CFM:
        {
            CsrBtHfSetVoiceRecognitionCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SET_VOICE_RECOGNITION_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_SET_VOICE_RECOGNITION_IND:
        {
            CsrBtHfSetVoiceRecognitionInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SET_VOICE_RECOGNITION_IND connectionId %x started %x vreState %x vreState %x ",
                ind->connectionId, ind->started, ind->vreState, ind->vreState);
            CONSTRUCT_HIGH_MSG("textType %x textOperation %x\n", ind->textType, ind->textOperation);
        }
        break;
        case CSR_BT_HF_BT_INPUT_REQ:
        {
            CsrBtHfBtInputReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_BT_INPUT_REQ connectionId %x dataRequest %x\n",
                req->connectionId, req->dataRequest);
        }
        break;
        case CSR_BT_HF_BT_INPUT_CFM:
        {
            CsrBtHfBtInputCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_BT_INPUT_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_GENERATE_DTMF_REQ:
        {
            CsrBtHfGenerateDtmfReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_GENERATE_DTMF_REQ connectionId %x dtmf %x\n",
                req->connectionId, req->dtmf);
        }
        break;
        case CSR_BT_HF_GENERATE_DTMF_CFM:
        {
            CsrBtHfGenerateDtmfCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_GENERATE_DTMF_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_AT_CMD_REQ:
        {
            CsrBtHfAtCmdReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_AT_CMD_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HF_AT_CMD_IND:
        {
            CsrBtHfAtCmdInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_AT_CMD_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HF_AT_CMD_CFM:
        {
            CsrBtHfAtCmdCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_AT_CMD_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_SECURITY_IN_REQ:
        {
            CsrBtHfSecurityInReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SECURITY_IN_REQ appHandle %x secLevel %x\n", req->appHandle, req->secLevel);
        }
        break;
        case CSR_BT_HF_SECURITY_IN_CFM:
        {
            CsrBtHfSecurityInCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SECURITY_IN_CFM resultCode %x resultSupplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_HF_SECURITY_OUT_REQ:
        {
            CsrBtHfSecurityOutReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SECURITY_OUT_REQ appHandle %x secLevel %x\n", req->appHandle, req->secLevel);
        }
        break;
        case CSR_BT_HF_SECURITY_OUT_CFM:
        {
            CsrBtHfSecurityOutCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SECURITY_OUT_CFM resultCode %x resultSupplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_HF_DEREGISTER_TIME_REQ:
        {
            CsrBtHfDeregisterTimeReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_DEREGISTER_TIME_REQ \n", req->waitSeconds);
        }
        break;
        case CSR_BT_HF_DEREGISTER_TIME_CFM:
        {
            CsrBtHfDeregisterTimeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_DEREGISTER_TIME_CFM result %x \n", cfm->result);
        }
        break;
        case CSR_BT_HF_INDICATOR_ACTIVATION_REQ:
        {
            CsrBtHfIndicatorActivationReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_INDICATOR_ACTIVATION_REQ connectionId %x indicatorBitMask %x\n",
                req->connectionId, req->indicatorBitMask);
        }
        break;
        case CSR_BT_HF_INDICATOR_ACTIVATION_CFM:
        {
            CsrBtHfIndicatorActivationCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_INDICATOR_ACTIVATION_CFM connectionId %x result %x\n",
                cfm->connectionId, cfm->result);
        }
        break;
        case CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ:
        {
            CsrBtHfUpdateSupportedCodecReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_UPDATE_SUPPORTED_CODEC_REQ codecMask %x enable %x sendUpdate %x\n",
                req->codecMask, req->enable, req->sendUpdate);
        }
        break;
        case CSR_BT_HF_UPDATE_SUPPORTED_CODEC_CFM:
        {
            CsrBtHfUpdateSupportedCodecCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_UPDATE_SUPPORTED_CODEC_CFM resultCode %x resultSupplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ:
        {
            CsrBtHfSetHfIndicatorValueReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ connectionId %x indId %x value %x\n",
                req->connectionId, req->indId, req->value);
        }
        break;
        case CSR_BT_HF_SET_HF_INDICATOR_VALUE_CFM:
        {
            CsrBtHfSetHfIndicatorValueCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_SET_HF_INDICATOR_VALUE_CFM connectionId %x cmeeResultCode %x\n",
                cfm->connectionId, cfm->cmeeResultCode);
        }
        break;
        case CSR_BT_HF_HF_INDICATOR_STATUS_IND:
        {
            CsrBtHfHfIndicatorStatusInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HF_HF_INDICATOR_STATUS_IND connectionId %x indId %x status %x\n",
                ind->connectionId, ind->indId, ind->status);
        }
        break;
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
        case CSR_BT_HF_UPDATE_QCE_CODEC_REQ:
        {
            CsrBtHfUpdateQceSupportReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HF_UPDATE_QCE_CODEC_REQ codecMask %x enable %x\n", req->codecMask, req->enable);
        }
        break;
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
#endif
        default:
            CONSTRUCT_HIGH_MSG("<== HF UNKNOWN MSG TYPE %x \n", *type);
       break;
    }    
}
#endif /* EXCLUDE_CSR_BT_HF_MODULE */

#ifndef EXCLUDE_CSR_BT_HFG_MODULE
static void synLogPrimHfg (void *prim)
{
    CsrPrim *type = prim;
    CsrUint8 i;

    switch (*type)
    {
        case CSR_BT_HFG_ACTIVATE_REQ:
        {
            CsrBtHfgActivateReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_ACTIVATE_REQ Handle phandle %x atMode %x numConnections %x supportedFeatures %x ",
                req->phandle, req->atMode, req->numConnections, req->supportedFeatures);
            CONSTRUCT_HIGH_MSG("callConfig %x hfgConfig %x hfgSupportedHfIndicatorsCount %x hfgSupportedHfIndicators:\n",
                req->callConfig, req->hfgConfig, req->hfgSupportedHfIndicatorsCount);
            for (i=0; i<req->hfgSupportedHfIndicatorsCount; i++)
            {
                CONSTRUCT_HIGH_MSG("hfIndicatorID %x status %x valueMax %x valueMin %x\n",
                    req->hfgSupportedHfIndicators[i].hfIndicatorID, req->hfgSupportedHfIndicators[i].status, req->hfgSupportedHfIndicators[i].valueMax, req->hfgSupportedHfIndicators[i].valueMin);
            }
        }
        break;
        case CSR_BT_HFG_DEACTIVATE_REQ:
        {
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_DEACTIVATE_REQ\n");
        }
        break;
        case CSR_BT_HFG_DEACTIVATE_CFM:
        {
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_DEACTIVATE_CFM\n");
        }
        break;
        case CSR_BT_HFG_SERVICE_CONNECT_REQ:
        {
            CsrBtHfgServiceConnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_SERVICE_CONNECT_REQ connectionType %x, deviceAddr: lap %x nap %x uap %x\n", req->connectionType, req->deviceAddr.lap, req->deviceAddr.nap, req->deviceAddr.uap);
        }
        break;
        case CSR_BT_HFG_SERVICE_CONNECT_IND:
        {
            CsrBtHfgServiceConnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_SERVICE_CONNECT_IND rc %x rs %x connectionId %x connectionType %x ", ind->resultCode, ind->resultSupplier, ind->connectionId, ind->connectionType);
            CONSTRUCT_HIGH_MSG("deviceAddr: lap %x nap %x uap %x, supportedFeatures %x ", ind->deviceAddr.lap, ind->deviceAddr.nap, ind->deviceAddr.uap, ind->supportedFeatures);
            CONSTRUCT_HIGH_MSG("remoteVersion %x btConnId %, hfSupportedHfIndicatorsCount %x hfSupportedHfIndicators:", ind->remoteVersion, ind->btConnId, ind->hfSupportedHfIndicatorsCount);
            for (i=0; i<ind->hfSupportedHfIndicatorsCount; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->hfSupportedHfIndicators[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_HFG_CANCEL_CONNECT_REQ:
        {
            CsrBtHfgCancelConnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_CANCEL_CONNECT_REQ deviceAddr: lap %x nap %x uap %x\n", req->deviceAddr.lap, req->deviceAddr.nap, req->deviceAddr.uap);
        }
        break;
        case CSR_BT_HFG_DISCONNECT_REQ:
        {
            CsrBtHfgDisconnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_DISCONNECT_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HFG_DISCONNECT_IND:
        {
            CsrBtHfgDisconnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_DISCONNECT_IND reasonCode %x reasonSupplier %x connectionId %x localTerminated %x ", ind->reasonCode, ind->reasonSupplier, ind->connectionId, ind->localTerminated);
            CONSTRUCT_HIGH_MSG("deviceAddr lap %x uap %x nap %x\n", ind->deviceAddr.lap, ind->deviceAddr.uap, ind->deviceAddr.nap);
        }
        break;
        case CSR_BT_HFG_AUDIO_CONNECT_REQ:
        {
            CsrBtHfgAudioConnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_AUDIO_CONNECT_REQ connectionId %x pcmSlot %x pcmRealloc %x qceCodecId %x\n", req->connectionId, req->pcmSlot, req->pcmRealloc, req->qceCodecId);
        }
        break;
        case CSR_BT_HFG_AUDIO_CONNECT_IND:
        {
            CsrBtHfgAudioConnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_AUDIO_CONNECT_IND rc %x rs %x connectionId %x scoHandle %x ", ind->resultCode, ind->resultSupplier, ind->connectionId, ind->scoHandle);
            CONSTRUCT_HIGH_MSG("pcmSlot %x linkType %x txInterval %x weSco %x ", ind->pcmSlot, ind->linkType, ind->txInterval, ind->weSco);
            CONSTRUCT_HIGH_MSG("rxPacketLength %x txPacketLength %x airMode %x\n", ind->rxPacketLength, ind->txPacketLength, ind->airMode);
        }
        break;
        case CSR_BT_HFG_AUDIO_CONNECT_CFM:
        {
            CsrBtHfgAudioConnectCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_AUDIO_CONNECT_CFM rc %x rs %x connectionId %x scoHandle %x ", cfm->resultCode, cfm->resultSupplier, cfm->connectionId, cfm->scoHandle);
            CONSTRUCT_HIGH_MSG("pcmSlot %x linkType %x txInterval %x weSco %x ", cfm->pcmSlot, cfm->linkType, cfm->txInterval, cfm->weSco);
            CONSTRUCT_HIGH_MSG("rxPacketLength %x txPacketLength %x airMode %x\n", cfm->rxPacketLength, cfm->txPacketLength, cfm->airMode);
        }
        break;
        case CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_IND:
        {
            CsrBtHfgAudioAcceptConnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_IND connectionId %x linkType %x\n", ind->connectionId, ind->linkType);
        }
        break;
        case CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES:
        {
            CsrBtHfgAudioAcceptConnectRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_AUDIO_ACCEPT_CONNECT_RES connectionId %x acceptResponse %x pcmSlot %x pcmReassign %x\n", res->connectionId, res->acceptResponse, res->pcmSlot, res->pcmReassign);
            if (res->acceptParameters != NULL)
            {
                CONSTRUCT_HIGH_MSG("packetTypes %x txBandwidth %x rxBandwidth %x maxLatency %x ", res->acceptParameters->packetTypes, res->acceptParameters->txBandwidth, res->acceptParameters->rxBandwidth, res->acceptParameters->maxLatency);
                CONSTRUCT_HIGH_MSG("contentFormat %x reTxEffort %x\n",  res->acceptParameters->contentFormat,  res->acceptParameters->reTxEffort);
            }
        }
        break;
        case CSR_BT_HFG_AUDIO_DISCONNECT_REQ:
        {
            CsrBtHfgAudioDisconnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_AUDIO_DISCONNECT_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HFG_AUDIO_DISCONNECT_IND:
        {
            CsrBtHfgAudioDisconnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_AUDIO_DISCONNECT_IND rc %x rs %x connectionId %x scoHandle %x\n", ind->reasonCode, ind->reasonSupplier, ind->connectionId, ind->scoHandle);
        }
        break;
        case CSR_BT_HFG_AUDIO_DISCONNECT_CFM:
        {
            CsrBtHfgAudioDisconnectCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_AUDIO_DISCONNECT_CFM rc %x rs %x connectionId %x scoHandle %x\n", cfm->resultCode, cfm->resultSupplier, cfm->connectionId, cfm->scoHandle);
        }
        break;
        case CSR_BT_HFG_SECURITY_IN_REQ:
        {
            CsrBtHfgSecurityInReq *req = prim;
            CONSTRUCT_HIGH_MSG("=-> CSR_BT_HFG_SECURITY_IN_REQ appHandle %x secLevel %x\n", req->appHandle, req->secLevel);
        }
        break;
        case CSR_BT_HFG_SECURITY_IN_CFM:
        {
            CsrBtHfgSecurityInCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_SECURITY_IN_CFM rc %x rs %x\n", cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_HFG_SECURITY_OUT_REQ:
        {
            CsrBtHfgSecurityOutReq *req = prim;
            CONSTRUCT_HIGH_MSG("=-> CSR_BT_HFG_SECURITY_OUT_REQ appHandle %x secLevel %x\n", req->appHandle, req->secLevel);
        }
        break;
        case CSR_BT_HFG_SECURITY_OUT_CFM:
        {
            CsrBtHfgSecurityOutCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_SECURITY_OUT_CFM rc %x rs %x\n", cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_HFG_CONFIG_SNIFF_REQ:
        {
            CsrBtHfgConfigSniffReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_CONFIG_SNIFF_REQ mask %x\n", req->mask);
        }
        break;
        case CSR_BT_HFG_STATUS_LP_IND:
        {
            CsrBtHfgStatusLpInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_STATUS_LP_IND rc %x rs %x connectionId %x currentMode %x ", ind->resultCode, ind->resultSupplier, ind->connectionId, ind->currentMode);
            CONSTRUCT_HIGH_MSG("oldMode %x wantedMode %x remoteReq %x\n", ind->oldMode, ind->wantedMode, ind->remoteReq);
        }
        break;
        case CSR_BT_HFG_CONFIG_AUDIO_REQ:
        {
            CsrBtHfgConfigAudioReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_CONFIG_AUDIO_REQ connectionId %x audioType %x audioSettingLen %x audioSetting:", req->connectionId, req->audioType, req->audioSettingLen);
            for (i=0; i<req->audioSettingLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->audioSetting[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_HFG_STATUS_AUDIO_IND:
        {
            CsrBtHfgStatusAudioInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_STATUS_AUDIO_IND connectionId %x audioType %x audioSettingLen %x audioSetting:", ind->connectionId, ind->audioType, ind->audioSettingLen);
            for (i=0; i<ind->audioSettingLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->audioSetting[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_HFG_RING_REQ:
        {
            CsrBtHfgRingReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_RING_REQ connectionId %x repetitionRate %x numOfRings %x numType %x\n", req->connectionId, req->repetitionRate, req->numOfRings, req->numType);
        }
        break;
        case CSR_BT_HFG_RING_CFM:
        {
            CsrBtHfgRingCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_RING_CFM connectionId %x\n", cfm->connectionId);
        }
        break;
        case CSR_BT_HFG_ANSWER_IND:
        {
            CsrBtHfgAnswerInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_ANSWER_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HFG_REJECT_IND:
        {
            CsrBtHfgRejectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_REJECT_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HFG_CALL_WAITING_REQ:
        {
            CsrBtHfgCallWaitingReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_CALL_WAITING_REQ connectionId %x type %x\n", req->connectionId, req->type);
        }
        break;
        case CSR_BT_HFG_CALL_HANDLING_IND:
        {
            CsrBtHfgCallHandlingInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_CALL_HANDLING_IND connectionId %x index %x value %x\n", ind->connectionId, ind->index, ind->value);
        }
        break;
        case CSR_BT_HFG_CALL_HANDLING_REQ:
        {
            CsrBtHfgCallHandlingReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_CALL_HANDLING_REQ connectionId %x btrh %x\n", req->connectionId, req->btrh);
        }
        break;
        case CSR_BT_HFG_CALL_HANDLING_RES:
        {
            CsrBtHfgCallHandlingRes *res = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_CALL_HANDLING_RES connectionId %x btrh %x cmeeCode %x\n", res->connectionId, res->btrh, res->cmeeCode);
        }
        break;
        case CSR_BT_HFG_DIAL_IND:
        {
            CsrBtHfgDialInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_DIAL_IND connectionId %x command %x\n", ind->connectionId, ind->command);
        }
        break;
        case CSR_BT_HFG_DIAL_RES:
        {
            CsrBtHfgDialRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_DIAL_RES connectionId %x cmeeCode %x\n", res->connectionId, res->cmeeCode);
        }
        break;
        case CSR_BT_HFG_SPEAKER_GAIN_REQ:
        {
            CsrBtHfgSpeakerGainReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_SPEAKER_GAIN_REQ connectionId %x gain %x\n", req->connectionId, req->gain);
        }
        break;
        case CSR_BT_HFG_SPEAKER_GAIN_IND:
        {
            CsrBtHfgSpeakerGainInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_SPEAKER_GAIN_IND connectionId %x gain %x\n", ind->connectionId, ind->gain);
        }
        break;
        case CSR_BT_HFG_MIC_GAIN_REQ:
        {
            CsrBtHfgMicGainReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_MIC_GAIN_REQ connectionId %x gain %x\n", req->connectionId, req->gain);
        }
        break;
        case CSR_BT_HFG_MIC_GAIN_IND:
        {
            CsrBtHfgMicGainInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_MIC_GAIN_IND connectionId %x gain %x\n", ind->connectionId, ind->gain);
        }
        break;
        case CSR_BT_HFG_AT_CMD_REQ:
        {
            CsrBtHfgAtCmdReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_AT_CMD_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_HFG_AT_CMD_IND:
        {
            CsrBtHfgAtCmdInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_AT_CMD_IND connectionId %x cmeeEnabled %x ", ind->connectionId, ind->cmeeEnabled);
            CONSTRUCT_HIGH_MSG("deviceAddr: lap %x nap %x uap %x\n", ind->deviceAddr.lap, ind->deviceAddr.nap, ind->deviceAddr.uap);
        }
        break;
        case CSR_BT_HFG_OPERATOR_IND:
        {
            CsrBtHfgOperatorInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_OPERATOR_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HFG_OPERATOR_RES:
        {
            CsrBtHfgOperatorRes *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_OPERATOR_RES connectionId %x mode %x cmeeCode %x\n", req->connectionId, req->mode, req->cmeeCode);
        }
        break;
        case CSR_BT_HFG_CALL_LIST_IND:
        {
            CsrBtHfgCallListInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_CALL_LIST_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HFG_CALL_LIST_RES:
        {
            CsrBtHfgCallListRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_CALL_LIST_RES connectionId %x final %x idx %x dir %x ", res->connectionId, res->final, res->idx, res->dir);
            CONSTRUCT_HIGH_MSG("stat %x mode %x mpy %x numType %x ", res->stat, res->mode, res->mpy, res->numType);
            CONSTRUCT_HIGH_MSG("cmeeCode %x\n", res->cmeeCode);
        }
        break;
        case CSR_BT_HFG_SUBSCRIBER_NUMBER_IND:
        {
            CsrBtHfgSubscriberNumberInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_SUBSCRIBER_NUMBER_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HFG_SUBSCRIBER_NUMBER_RES:
        {
            CsrBtHfgSubscriberNumberRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_SUBSCRIBER_NUMBER_RES connectionId %x final %x numType %x ", res->connectionId, res->final, res->numType);
            CONSTRUCT_HIGH_MSG("service %x cmeeCode %x\n", res->service, res->cmeeCode);
        }
        break;
        case CSR_BT_HFG_STATUS_INDICATOR_SET_REQ:
        {
            CsrBtHfgStatusIndicatorSetReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_STATUS_INDICATOR_SET_REQ connectionId %x indicator %x value %x\n", req->connectionId, req->indicator, req->value);
        }
        break;
        case CSR_BT_HFG_INBAND_RINGING_REQ:
        {
            CsrBtHfgInbandRingingReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_INBAND_RINGING_REQ connectionId %x inband %x\n", req->connectionId, req->inband);
        }
        break;
        case CSR_BT_HFG_GENERATE_DTMF_IND:
        {
            CsrBtHfgGenerateDtmfInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_GENERATE_DTMF_IND connectionId %x dtmf %x\n", ind->connectionId, ind->dtmf);
        }
        break;
        case CSR_BT_HFG_NOISE_ECHO_IND:
        {
            CsrBtHfgNoiseEchoInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_NOISE_ECHO_IND connectionId %x nrec %x\n", ind->connectionId, ind->nrec);
        }
        break;
        case CSR_BT_HFG_BT_INPUT_IND:
        {
            CsrBtHfgBtInputInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_BT_INPUT_IND connectionId %x request %x\n", ind->connectionId, ind->request);
        }
        break;
        case CSR_BT_HFG_BT_INPUT_RES:
        {
            CsrBtHfgBtInputRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_BT_INPUT_RES connectionId %x cmeeCode %x\n", res->connectionId, res->cmeeCode);
        }
        break;
        case CSR_BT_HFG_VOICE_RECOG_REQ:
        {
            CsrBtHfgVoiceRecogReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_VOICE_RECOG_REQ connectionId %x bvra %x vreState %x textType %x ", req->connectionId, req->bvra, req->vreState, req->textType);
            CONSTRUCT_HIGH_MSG("textOperation %x\n", req->textOperation);
        }
        break;
        case CSR_BT_HFG_VOICE_RECOG_IND:
        {
            CsrBtHfgVoiceRecogInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_VOICE_RECOG_IND connectionId %x bvra %x\n", ind->connectionId, ind->bvra);
        }
        break;
        case CSR_BT_HFG_ENHANCED_VOICE_RECOG_IND:
        {
            CsrBtHfgEnhancedVoiceRecogInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_ENHANCED_VOICE_RECOG_IND connectionId %x bvra %x\n", ind->connectionId, ind->bvra);
        }
        break;
        case CSR_BT_HFG_VOICE_RECOG_RES:
        {
            CsrBtHfgVoiceRecogRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_VOICE_RECOG_RES connectionId %x cmeeCode %x\n", res->connectionId, res->cmeeCode);
        }
        break;
        case CSR_BT_HFG_MANUAL_INDICATOR_IND:
        {
            CsrBtHfgManualIndicatorInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_MANUAL_INDICATOR_IND connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_HFG_MANUAL_INDICATOR_RES:
        {
            CsrBtHfgManualIndicatorRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_MANUAL_INDICATOR_RES connectionId %x indicatorsLength %x\n", res->connectionId, res->indicatorsLength);
        }
        break;
        case CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ:
        {
            CsrBtHfgConfigSingleAtcmdReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_CONFIG_SINGLE_ATCMD_REQ phandle %x idx %x sendToApp %x\n", req->phandle, req->idx, req->sendToApp);
        }
        break;
        case CSR_BT_HFG_CONFIG_SINGLE_ATCMD_CFM:
        {
            CsrBtHfgConfigSingleAtcmdCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_CONFIG_SINGLE_ATCMD_CFM result %x\n", cfm->result);
        }
        break;
        case CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ:
        {
            CsrBtHfgConfigAtcmdHandlingReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_CONFIG_ATCMD_HANDLING_REQ phandle %x bitwiseIndicatorsLength %x bitwiseIndicators: ", req->phandle, req->bitwiseIndicatorsLength);
            for (i=0; i<req->bitwiseIndicatorsLength; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->bitwiseIndicators[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_HFG_CONFIG_ATCMD_HANDLING_CFM:
        {
            CsrBtHfgConfigAtcmdHandlingCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_CONFIG_ATCMD_HANDLING_CFM result %x bitwiseIndicatorsLength %x bitwiseIndicators: ", cfm->result, cfm->bitwiseIndicatorsLength);
            for (i=0; i<cfm->bitwiseIndicatorsLength; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", cfm->bitwiseIndicators[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");

        }
        break;
        case CSR_BT_HFG_DEREGISTER_TIME_REQ:
        {
            CsrBtHfgDeregisterTimeReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_DEREGISTER_TIME_REQ waitSeconds %x\n", req->waitSeconds);
        }
        break;
        case CSR_BT_HFG_DEREGISTER_TIME_CFM:
        {
            CsrBtHfgDeregisterTimeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_DEREGISTER_TIME_CFM result %x\n", cfm->result);
        }
        break;
        case CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ:
        {
            CsrBtHfgSetHfIndicatorStatusReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_SET_HF_INDICATOR_STATUS_REQ connectionId %x indId %x status %x\n", req->connectionId, req->indId, req->status);
        }
        break;
        case CSR_BT_HFG_HF_INDICATOR_VALUE_IND:
        {
            CsrBtHfgHfIndicatorValueInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_HF_INDICATOR_VALUE_IND connectionId %x indId %x value %x\n", ind->connectionId, ind->indId, ind->value);
        }
        break;
#ifdef CSR_BT_HFG_ENABLE_SWB_SUPPORT
        case CSR_BT_HFG_SWB_RSP:
        {
            CsrBtHfgSwbRsp *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_HFG_SWB_RSP connectionId %x len %x\n", res->connectionId, res->len);
        }
        break;
#endif /* CSR_BT_HFG_ENABLE_SWB_SUPPORT */
        case CSR_BT_HFG_SELECTED_CODEC_IND:
        {
            CsrBtHfgSelectedCodecInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_HFG_SELECTED_CODEC_IND connectionId %x codecToUse %x\n", ind->connectionId, ind->codecToUse);
        }
        break;

        default:
            CONSTRUCT_HIGH_MSG("<== HFG UNKNOWN MSG TYPE %x \n", *type);
       break;
    }    
}
#endif /* EXCLUDE_CSR_BT_HFG_MODULE */

#ifndef EXCLUDE_CSR_BT_AV_MODULE
static void synLogPrimAv (void *prim)
{
    CsrPrim *type = prim;
    CsrUint8 i;

    switch (*type)
    {
        case CSR_BT_AV_ACTIVATE_REQ:
        {
            CsrBtAvActivateReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_ACTIVATE_REQ phandle %x localRole %x\n", req->phandle, req->localRole);
        }
        break;
        case CSR_BT_AV_ACTIVATE_CFM:
        {
            CsrBtAvActivateCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_ACTIVATE_CFM avResultCode %x avResultSupplier %x\n",
                cfm->avResultCode, cfm->avResultSupplier);
        }
        break;
        case CSR_BT_AV_DEACTIVATE_REQ:
        {
            CsrBtAvDeactivateReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_DEACTIVATE_REQ localRole %x\n", req->localRole);
        }
        break;
        case CSR_BT_AV_DEACTIVATE_CFM:
        {
            CsrBtAvDeactivateCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_DEACTIVATE_CFM avResultCode %x avResultSupplier %x\n",
                cfm->avResultCode, cfm->avResultSupplier);
        }
        break;
        case CSR_BT_AV_CONNECT_REQ:
        {
            CsrBtAvConnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_CONNECT_REQ phandle %x lap %x uap %x nap %x ",
                req->phandle, req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap);
            CONSTRUCT_HIGH_MSG("remoteRole %x localRole %x\n", req->remoteRole, req->localRole);
        }
        break;
        case CSR_BT_AV_CONNECT_CFM:
        {
            CsrBtAvConnectCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_CONNECT_CFM connectionId %x lap %x uap %x nap %x ",
                cfm->connectionId, cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap);
            CONSTRUCT_HIGH_MSG("avResultCode %x avResultSupplier %x btConnId %x\n", cfm->avResultCode, cfm->avResultSupplier, cfm->btConnId);
        }
        break;
        case CSR_BT_AV_CONNECT_IND:
        {
            CsrBtAvConnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_CONNECT_IND connectionId %x lap %x uap %x nap %x ",
                ind->connectionId, ind->deviceAddr.lap, ind->deviceAddr.uap, ind->deviceAddr.nap);
            CONSTRUCT_HIGH_MSG("btConnId %x\n", ind->btConnId);
        }
        break;
        case CSR_BT_AV_CANCEL_CONNECT_REQ:
        {
            CsrBtAvCancelConnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_CANCEL_CONNECT_REQ deviceAddr lap %x uap %x nap %x\n",
                req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap);
        }
        break;
        case CSR_BT_AV_DISCONNECT_REQ:
        {
            CsrBtAvDisconnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_DISCONNECT_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_AV_DISCONNECT_IND:
        {
            CsrBtAvDisconnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_DISCONNECT_IND connectionId %x localTerminated %x reasonCode %x reasonSupplier %x\n",
                ind->connectionId, ind->localTerminated, ind->reasonCode, ind->reasonSupplier);
        }
        break;
        case CSR_BT_AV_DISCOVER_REQ:
        {
            CsrBtAvDiscoverReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_DISCOVER_REQ connectionId %x tLabel %x\n",
                req->connectionId, req->tLabel);
        }
        break;
        case CSR_BT_AV_DISCOVER_IND:
        {
            CsrBtAvDiscoverInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_DISCOVER_IND connectionId %x tLabel %x\n", ind->connectionId, ind->tLabel);
        }
        break;
        case CSR_BT_AV_DISCOVER_RES:
        {
            CsrBtAvDiscoverRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_DISCOVER_RES connectionId %x tLabel %x seidInfoCount %x avResponse %x seidInfo:\n",
                res->connectionId, res->tLabel, res->seidInfoCount, res->avResponse);
            for (i=0; i<res->seidInfoCount; i++)
            {
                CONSTRUCT_HIGH_MSG("acpSeid %x inUse %x mediaType %x sepType %x\n",
                    res->seidInfo[i].acpSeid, res->seidInfo[i].inUse, res->seidInfo[i].mediaType, res->seidInfo[i].sepType);
            }
        }
        break;
        case CSR_BT_AV_DISCOVER_CFM:
        {
            CsrBtAvDiscoverCfm *cfm  = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_DISCOVER_CFM connectionId %x tLabel %x seidInfoCount %x avResponse %x ",
                cfm->connectionId, cfm->tLabel, cfm->seidInfoCount, cfm->avResultCode);
            CONSTRUCT_HIGH_MSG("avResultSupplier %x seidInfo:\n", cfm->avResultSupplier);
            for (i=0; i<cfm->seidInfoCount; i++)
            {
                CONSTRUCT_HIGH_MSG("acpSeid %x inUse %x mediaType %x sepType %x\n",
                    cfm->seidInfo[i].acpSeid, cfm->seidInfo[i].inUse, cfm->seidInfo[i].mediaType, cfm->seidInfo[i].sepType);
            }
        }
        break;
        case CSR_BT_AV_GET_CAPABILITIES_REQ:
        {
            CsrBtAvGetCapabilitiesReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_GET_CAPABILITIES_REQ connectionId %x tLabel %x acpSeid %x\n",
                req->connectionId, req->tLabel, req->acpSeid);
        }
        break;
        case CSR_BT_AV_GET_CAPABILITIES_IND:
        {
            CsrBtAvGetCapabilitiesInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_GET_CAPABILITIES_IND connectionId %x tLabel %x acpSeid %x\n",
                ind->connectionId, ind->tLabel, ind->acpSeid);
        }
        break;
        case CSR_BT_AV_GET_CAPABILITIES_RES:
        {
            CsrBtAvGetCapabilitiesRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_GET_CAPABILITIES_RES connectionId %x tLabel %x servCapLen %x avResponse %x servCapData:\n",
                res->connectionId, res->tLabel, res->servCapLen, res->avResponse);
            for (i=0; i<res->servCapLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", res->servCapData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_GET_CAPABILITIES_CFM:
        {
            CsrBtAvGetCapabilitiesCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_GET_CAPABILITIES_CFM connectionId %x tLabel %x servCapLen %x avResultCode %x",
                cfm->connectionId, cfm->tLabel, cfm->servCapLen, cfm->avResultCode);
            CONSTRUCT_HIGH_MSG("avResultSupplier %x servCapData:\n", cfm->avResultSupplier);
            for (i=0; i<cfm->servCapLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", cfm->servCapData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_GET_ALL_CAPABILITIES_IND:
        {
            CsrBtAvGetAllCapabilitiesInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_GET_ALL_CAPABILITIES_IND connectionId %x tLabel %x acpSeid %x\n",
                ind->connectionId, ind->tLabel, ind->acpSeid);
        }
        break;
        case CSR_BT_AV_GET_ALL_CAPABILITIES_RES:
        {
            CsrBtAvGetAllCapabilitiesRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_GET_ALL_CAPABILITIES_RES connectionId %x tLabel %x servCapLen %x avResponse %x servCapData:\n",
                res->connectionId, res->tLabel, res->servCapLen, res->avResponse);
            for (i=0; i<res->servCapLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", res->servCapData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_SET_CONFIGURATION_REQ:
        {
            CsrBtAvSetConfigurationReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SET_CONFIGURATION_REQ connectionId %x tLabel %x acpSeid %x intSeid %x ",
                req->connectionId, req->tLabel, req->acpSeid, req->intSeid);
            CONSTRUCT_HIGH_MSG("appServCapLen %x appServCapData:\n", req->appServCapLen);
            for (i=0; i<req->appServCapLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->appServCapData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_SET_CONFIGURATION_IND:
        {
            CsrBtAvSetConfigurationInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_SET_CONFIGURATION_IND connectionId %x tLabel %x shandle %x acpSeid %x ",
                ind->connectionId, ind->tLabel, ind->shandle, ind->acpSeid);
            CONSTRUCT_HIGH_MSG("intSeid %x servCapLen %x servCapData:\n", ind->intSeid, ind->servCapLen);
            for (i=0; i<ind->servCapLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->servCapData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_SET_CONFIGURATION_RES:
        {
            CsrBtAvSetConfigurationRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SET_CONFIGURATION_RES shandle %x tLabel %x servCategory %x avResponse %x\n",
                res->shandle, res->tLabel, res->servCategory, res->avResponse);
        }
        break;
        case CSR_BT_AV_SET_CONFIGURATION_CFM:
        {
            CsrBtAvSetConfigurationCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_SET_CONFIGURATION_CFM connectionId %x tLabel %x shandle %x servCategory %x ",
                cfm->connectionId, cfm->tLabel, cfm->shandle, cfm->servCategory);
            CONSTRUCT_HIGH_MSG("avResultCode %x avResultSupplier %x\n", cfm->avResultCode, cfm->avResultSupplier);
        }
        break;
        case CSR_BT_AV_GET_CONFIGURATION_REQ:
        {
            CsrBtAvGetConfigurationReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_GET_CONFIGURATION_REQ shandle %x tLabel %x\n", req->shandle, req->tLabel);
        }
        break;
        case CSR_BT_AV_GET_CONFIGURATION_IND:
        {
            CsrBtAvGetConfigurationInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_GET_CONFIGURATION_IND shandle %x tLabel %x\n", ind->shandle, ind->tLabel);
        }
        break;
        case CSR_BT_AV_GET_CONFIGURATION_CFM:
        {
            CsrBtAvGetConfigurationCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_GET_CONFIGURATION_CFM tLabel %x shandle %x servCapLen %x avResultCode %x ",
                cfm->tLabel, cfm->shandle, cfm->servCapLen, cfm->avResultCode);
            CONSTRUCT_HIGH_MSG("avResultSupplier %x servCapData:\n", cfm->avResultSupplier);
            for (i=0; i<cfm->servCapLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", cfm->servCapData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_GET_CONFIGURATION_RES:
        {
            CsrBtAvGetConfigurationRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_GET_CONFIGURATION_RES shandle %x tLabel %x servCapLen %x avResponse %x\n",
                res->shandle, res->tLabel, res->servCapLen, res->avResponse);
            for (i=0; i<res->servCapLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", res->servCapData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_RECONFIGURE_REQ:
        {
            CsrBtAvReconfigureReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_RECONFIGURE_REQ shandle %x tLabel %x servCapLen %x servCapData:\n",
                req->shandle, req->tLabel, req->servCapLen);
            for (i=0; i<req->servCapLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->servCapData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_RECONFIGURE_IND:
        {
            CsrBtAvReconfigureInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_RECONFIGURE_IND shandle %x tLabel %x servCapLen %x servCapData:\n",
                ind->shandle, ind->tLabel, ind->servCapLen);
            for (i=0; i<ind->servCapLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->servCapData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_RECONFIGURE_CFM:
        {
            CsrBtAvReconfigureCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_RECONFIGURE_CFM shandle %x tLabel %x servCategory %x avResultCode %x ",
                cfm->shandle, cfm->tLabel, cfm->servCategory, cfm->avResultCode);
            CONSTRUCT_HIGH_MSG("avResultSupplier %x", cfm->avResultSupplier);
        }
        break;
        case CSR_BT_AV_RECONFIGURE_RES:
        {
            CsrBtAvReconfigureRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_RECONFIGURE_RES shandle %x tLabel %x servCategory %x avResponse %x\n",
                res->shandle, res->tLabel, res->servCategory, res->avResponse);
        }
        break;
        case CSR_BT_AV_OPEN_REQ:
        {
            CsrBtAvOpenReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_OPEN_REQ shandle %x tLabel %x\n", req->shandle, req->tLabel);
        }
        break;
        case CSR_BT_AV_OPEN_IND:
        {
            CsrBtAvOpenInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_OPEN_IND shandle %x tLabel %x\n", ind->shandle, ind->tLabel);
        }
        break;
        case CSR_BT_AV_OPEN_RES:
        {
            CsrBtAvOpenRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_OPEN_RES shandle %x tLabel %x avResponse %x\n",
                res->shandle, res->tLabel, res->avResponse);
        }
        break;
        case CSR_BT_AV_OPEN_CFM:
        {
            CsrBtAvOpenCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_OPEN_CFM shandle %x tLabel %x avResultCode %x avResultSupplier %x\n",
                cfm->shandle, cfm->tLabel, cfm->avResultCode, cfm->avResultSupplier);
        }
        break;
        case CSR_BT_AV_START_REQ:
        {
            CsrBtAvStartReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_START_REQ tLabel %x listLength %x list:\n", req->tLabel, req->listLength);
            for (i=0; i<req->listLength; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->list[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_START_IND:
        {
            CsrBtAvStartInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_START_IND tLabel %x listLength %x list:\n", ind->tLabel, ind->listLength);
            for (i=0; i<ind->listLength; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->list[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_START_RES:
        {
            CsrBtAvStartRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_START_RES tLabel %x reject_shandle %x listLength %x avResponse %x list:\n",
                res->tLabel, res->reject_shandle, res->listLength, res->avResponse);
            for (i=0; i<res->listLength; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", res->list[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_START_CFM:
        {
            CsrBtAvStartCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_START_CFM connectionId %x tLabel %x reject_shandle %x avResultCode %x ",
                cfm->connectionId, cfm->tLabel, cfm->reject_shandle, cfm->avResultCode);
            CONSTRUCT_HIGH_MSG("avResultSupplier %x\n", cfm->avResultSupplier);
        }
        break;
        case CSR_BT_AV_CLOSE_REQ:
        {
            CsrBtAvCloseReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_CLOSE_REQ shandle %x tLabel %x\n", req->shandle, req->tLabel);
        }
        break;
        case CSR_BT_AV_CLOSE_IND:
        {
            CsrBtAvCloseInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_CLOSE_IND shandle %x tLabel %x\n", ind->shandle, ind->tLabel);
        }
        break;
        case CSR_BT_AV_CLOSE_RES:
        {
            CsrBtAvCloseRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_CLOSE_RES shandle %x tLabel %x avResponse %x\n",
                res->shandle, res->tLabel, res->avResponse);
        }
        break;
        case CSR_BT_AV_CLOSE_CFM:
        {
            CsrBtAvCloseCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_CLOSE_CFM tLabel %x shandle %x avResultCode %x avResultSupplier %x\n",
                cfm->tLabel, cfm->shandle, cfm->avResultCode, cfm->avResultSupplier);
        }
        break;
        case CSR_BT_AV_SUSPEND_REQ:
        {
            CsrBtAvSuspendReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SUSPEND_REQ tLabel %x listLength %x list:\n", req->tLabel, req->listLength);
            for (i=0; i<req->listLength; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->list[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_SUSPEND_IND:
        {
            CsrBtAvSuspendInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_SUSPEND_IND tLabel %x listLength %x list:\n", ind->tLabel, ind->listLength);
            for (i=0; i<ind->listLength; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->list[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_SUSPEND_CFM:
        {
            CsrBtAvSuspendCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_SUSPEND_CFM connectionId %x tLabel %x reject_shandle %x avResultCode %x ",
                cfm->connectionId, cfm->tLabel, cfm->reject_shandle, cfm->avResultCode);
            CONSTRUCT_HIGH_MSG("avResultSupplier %x\n", cfm->avResultSupplier);
        }
        break;
        case CSR_BT_AV_SUSPEND_RES:
        {
            CsrBtAvSuspendRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SUSPEND_RES tLabel %x reject_shandle %x listLength %x avResponse %x list:\n",
                res->tLabel, res->reject_shandle, res->listLength, res->avResponse);
            for (i=0; i<res->listLength; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", res->list[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_ABORT_REQ:
        {
            CsrBtAvAbortReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_ABORT_REQ shandle %x tLabel %x\n", req->shandle, req->tLabel);
        }
        break;
        case CSR_BT_AV_ABORT_IND:
        {
            CsrBtAvAbortInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_ABORT_IND shandle %x tLabel %x\n", ind->shandle, ind->tLabel);
        }
        break;
        case CSR_BT_AV_ABORT_RES:
        {
            CsrBtAvAbortRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_ABORT_RES shandle %x tLabel %x\n", res->shandle, res->tLabel);
        }
        break;
        case CSR_BT_AV_ABORT_CFM:
        {
            CsrBtAvAbortCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_ABORT_CFM shandle %x tLabel %x\n", cfm->shandle, cfm->tLabel);
        }
        break;
        case CSR_BT_AV_STATUS_IND:
        {
            CsrBtAvStatusInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_STATUS_IND connectionId %x statusType %x roleType %x appHandle %x\n",
                ind->connectionId, ind->statusType, ind->roleType, ind->appHandle);
        }
        break;
        case CSR_BT_AV_DELAY_REPORT_REQ:
        {
            CsrBtAvDelayReportReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_DELAY_REPORT_REQ tLabel %x delay %x shandle %x\n",
                req->tLabel, req->delay, req->shandle);
        }
        break;
        case CSR_BT_AV_DELAY_REPORT_IND:
        {
            CsrBtAvDelayReportInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_DELAY_REPORT_IND tLabel %x delay %x shandle %x\n", ind->tLabel, ind->delay, ind->shandle);
        }
        break;
        case CSR_BT_AV_DELAY_REPORT_RES:
        {
            CsrBtAvDelayReportRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_DELAY_REPORT_RES tLabel %x shandle %x avResponse %x\n",
                res->tLabel, res->shandle, res->avResponse);
        }
        break;
        case CSR_BT_AV_DELAY_REPORT_CFM:
        {
            CsrBtAvDelayReportCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_DELAY_REPORT_CFM tLabel %x shandle %x avResultCode %x avResultSupplier %x\n",
                cfm->tLabel, cfm->shandle, cfm->avResultCode, cfm->avResultSupplier);
        }
        break;
#if 0
        case CSR_BT_AV_REGISTER_STREAM_HANDLE_REQ:
        {
            CsrBtAvRegisterStreamHandleReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_REGISTER_STREAM_HANDLE_REQ streamHandle %x\n", req->streamHandle);
        }
        break;
        case CSR_BT_AV_REGISTER_STREAM_HANDLE_CFM:
        {
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_REGISTER_STREAM_HANDLE_CFM \n");
        }
        break;
        case CSR_BT_AV_STREAM_DATA_REQ:
        {
            CsrBtAvStreamDataReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_STREAM_DATA_REQ shandle %x hdr_type %x length %x context %x data:\n",
                req->shandle, req->hdr_type, req->length, req->context);
            for (i=0; i<req->length; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->data[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_STREAM_DATA_IND:
        {
            CsrBtAvStreamDataInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_STREAM_DATA_IND shandle %x pad %x pad2 %x length %x",
                ind->shandle, ind->pad, ind->pad2, ind->length);
            CONSTRUCT_HIGH_MSG("context %x Data:\n", ind->context);
            for (i=0; i<ind->length; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->data[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_QOS_IND:
        {
            CsrBtAvQosInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_QOS_IND shandle %x bufferStatus %x\n", ind->shandle, ind->bufferStatus);
        }
        break;
        case CSR_BT_AV_STREAM_MTU_SIZE_IND:
        {
            CsrBtAvStreamMtuSizeInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_STREAM_MTU_SIZE_IND shandle %x remoteMtuSize %x btConnId %x\n",
                ind->shandle, ind->remoteMtuSize, ind->btConnId);
        }
        break;
        case CSR_BT_AV_SECURITY_CONTROL_REQ:
        {
            CsrBtAvSecurityControlReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SECURITY_CONTROL_REQ shandle %x tLabel %x contProtMethodLen %x contProtMethodData:\n",
                req->shandle, req->tLabel, req->contProtMethodLen);
            for (i=0; i<req->contProtMethodLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->contProtMethodData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_SECURITY_CONTROL_IND:
        {
            CsrBtAvSecurityControlInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_SECURITY_CONTROL_IND shandle %x tLabel %x contProtMethodLen %x contProtMethodData:\n",
                ind->shandle, ind->tLabel, ind->contProtMethodLen);
            for (i=0; i<ind->contProtMethodLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->contProtMethodData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_SECURITY_CONTROL_RES:
        {
            CsrBtAvSecurityControlRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SECURITY_CONTROL_RES shandle %x tLabel %x contProtMethodLen %x avResponse %x contProtMethodData:\n",
                res->shandle, res->tLabel, res->contProtMethodLen, res->avResponse);
            for (i=0; i<res->contProtMethodLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", res->contProtMethodData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_SECURITY_CONTROL_CFM:
        {
            CsrBtAvSecurityControlCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_SECURITY_CONTROL_CFM shandle %x tLabel %x contProtMethodLen %x avResponse %x\n",
                cfm->shandle, cfm->tLabel, cfm->contProtMethodLen, cfm->avResultCode);
            CONSTRUCT_HIGH_MSG("avResultSupplier %x contProtMethodData:\n", cfm->avResultSupplier);
            for (i=0; i<cfm->contProtMethodLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", cfm->contProtMethodData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AV_SECURITY_IN_REQ:
        {
            CsrBtAvSecurityInReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SECURITY_IN_REQ appHandle %x secLevel %x\n",
                req->appHandle, req->secLevel);
        }
        break;
        case CSR_BT_AV_SECURITY_IN_CFM:
        {
            CsrBtAvSecurityInCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_SECURITY_IN_CFM resultCode %x resultSupplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AV_SECURITY_OUT_REQ:
        {
            CsrBtAvSecurityOutReq *req  = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SECURITY_OUT_REQ appHandle %x secLevel %x\n",
                req->appHandle, req->secLevel);
        }
        break;
        case CSR_BT_AV_SECURITY_OUT_CFM:
        {
            CsrBtAvSecurityOutCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_SECURITY_OUT_CFM resultCode %x resultSupplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AV_SET_QOS_INTERVAL_REQ:
        {
            CsrBtAvSetQosIntervalReq *req  = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SET_QOS_INTERVAL_REQ qosInterval %x\n", req->qosInterval);
        }
        break;
        case CSR_BT_AV_LP_NEG_CONFIG_REQ:
        {
            CsrBtAvLpNegConfigReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_LP_NEG_CONFIG_REQ enable %x\n", req->enable);
        }
        break;
        case CSR_BT_AV_GET_CHANNEL_INFO_REQ:
        {
            CsrBtAvGetChannelInfoReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_GET_CHANNEL_INFO_REQ btConnId %x\n", req->btConnId);
        }
        break;
        case CSR_BT_AV_GET_STREAM_CHANNEL_INFO_REQ:
        {
            CsrBtAvGetStreamChannelInfoReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_GET_STREAM_CHANNEL_INFO_REQ shandle %x\n", req->shandle);
        }
        break;
        case CSR_BT_AV_GET_CHANNEL_INFO_CFM:
        {
            CsrBtAvGetChannelInfoCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_GET_CHANNEL_INFO_CFM aclHandle %x remoteCid %x resultCode %x resultSupplier %x\n",
                cfm->aclHandle, cfm->remoteCid, cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AV_SET_STREAM_INFO_REQ:
        {
            CsrBtAvSetStreamInfoReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_SET_STREAM_INFO_REQ shandle %x codecLocation %x\n", req->shandle, req->sInfo.codecLocation);
        }
        break;
        case CSR_BT_AV_GET_MEDIA_CHANNEL_INFO_REQ:
        {
            CsrBtAvGetMediaChannelInfoReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AV_GET_MEDIA_CHANNEL_INFO_REQ shandle %x\n", req->shandle);
        }
        break;
        case CSR_BT_AV_GET_MEDIA_CHANNEL_INFO_CFM:
        {
            CsrBtAvGetMediaChannelInfoCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AV_GET_MEDIA_CHANNEL_INFO_CFM shandle %x aclHandle %x aclHandle %x localCid %x ",
                cfm->shandle, cfm->aclHandle, cfm->remoteCid, cfm->localCid);
            CONSTRUCT_HIGH_MSG("resultCode %x resultSupplier %x\n", cfm->resultCode, cfm->resultSupplier);
        }
        break;
#endif
        default:
            CONSTRUCT_HIGH_MSG("<== AV UNKNOWN MSG TYPE %x \n", *type);
        break;
    }
}
#endif /* EXCLUDE_CSR_BT_AV_MODULE */

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE
static void synLogPrimAvrcp(void *prim)
{
    CsrPrim *type = prim;
    CsrUint8 i;

    switch (*type)
    {
        case CSR_BT_AVRCP_CONFIG_REQ:
        {
            CsrBtAvrcpConfigReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_CONFIG_REQ phandle %x globalConfig %x mtu %x uidCount %x\n",
                req->phandle, req->globalConfig, req->mtu, req->uidCount);
            CONSTRUCT_HIGH_MSG("tgDetails: roleSupported %x roleConfig %x srAvrcpVersion %x tgDetails %x\n",
                req->tgDetails.roleSupported, req->tgDetails.roleConfig, req->tgDetails.srAvrcpVersion, req->tgDetails.srFeatures);
            CONSTRUCT_HIGH_MSG("ctDetails: roleSupported %x roleConfig %x srAvrcpVersion %x tgDetails %x\n",
                req->ctDetails.roleSupported, req->ctDetails.roleConfig, req->ctDetails.srAvrcpVersion, req->ctDetails.srFeatures);
        }
        break;
        case CSR_BT_AVRCP_CONFIG_CFM:
        {
            CsrBtAvrcpConfigCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_CONFIG_CFM resultCode %x resultSupplier %x\n", cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AVRCP_ACTIVATE_REQ:
        {
            CsrBtAvrcpActivateReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_ACTIVATE_REQ maxIncoming %x\n", req->maxIncoming);
        }
        break;
        case CSR_BT_AVRCP_ACTIVATE_CFM:
        {
            CsrBtAvrcpActivateCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_ACTIVATE_CFM resultCode %x resultSupplier %x\n", cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AVRCP_DEACTIVATE_REQ:
        {
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_DEACTIVATE_REQ\n");
        }
        break;
        case CSR_BT_AVRCP_DEACTIVATE_CFM:
        {
            CsrBtAvrcpDeactivateCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_DEACTIVATE_CFM resultCode %x resultSupplier %x\n", cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AVRCP_CONNECT_REQ:
        {
            CsrBtAvrcpConnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_CONNECT_REQ deviceAddr lap %x uap %x nap %x\n",
                req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap);
        }
        break;
        case CSR_BT_AVRCP_CONNECT_IND:
        {
            CsrBtAvrcpConnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_CONNECT_IND connectionId %x deviceAddr lap %x, uap %x nap %x ",
                ind->connectionId, ind->deviceAddr.lap, ind->deviceAddr.uap, ind->deviceAddr.nap);
            CONSTRUCT_HIGH_MSG("connectionId %x\n", ind->connectionId);
        }
        break;
        case CSR_BT_AVRCP_CONNECT_CFM:
        {
            CsrBtAvrcpConnectCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_CONNECT_CFM connectionId %x deviceAddr lap %x, uap %x nap %x ",
                cfm->connectionId, cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap);
            CONSTRUCT_HIGH_MSG("btConnId %x resultCode %x resultSupplier %x\n", cfm->btConnId, cfm->resultCode, cfm->resultSupplier);
            CONSTRUCT_HIGH_MSG("tgFeatures: roleSupported %x roleConfig %x srAvrcpVersion %x srFeatures %x\n",
                cfm->tgFeatures.roleSupported, cfm->tgFeatures.roleConfig, cfm->tgFeatures.srAvrcpVersion, cfm->tgFeatures.srFeatures);
            CONSTRUCT_HIGH_MSG("ctFeatures: roleSupported %x roleConfig %x srAvrcpVersion %x srFeatures %x\n",
                cfm->ctFeatures.roleSupported, cfm->ctFeatures.roleConfig, cfm->ctFeatures.srAvrcpVersion, cfm->ctFeatures.srFeatures);
        }
        break;
        case CSR_BT_AVRCP_CANCEL_CONNECT_REQ:
        {
            CsrBtAvrcpCancelConnectReq * req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_CANCEL_CONNECT_REQ deviceAddr lap %x uap %x nap %x\n",
                req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap);
        }
        break;
        case CSR_BT_AVRCP_DISCONNECT_REQ:
        {
            CsrBtAvrcpDisconnectReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_DISCONNECT_REQ connectionId %x\n", req->connectionId);
        }
        break;
        case CSR_BT_AVRCP_DISCONNECT_IND:
        {
            CsrBtAvrcpDisconnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_DISCONNECT_IND connectionId %x localTerminated %x reasonCode %x reasonSupplier %x\n",
                ind->connectionId, ind->localTerminated, ind->reasonCode, ind->reasonSupplier);
        }
        break;
        case CSR_BT_AVRCP_REMOTE_FEATURES_IND:
        {
            CsrBtAvrcpRemoteFeaturesInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_REMOTE_FEATURES_IND connectionId %x deviceAddr lap %x uap %x nap %x\n",
                ind->connectionId, ind->deviceAddr.lap, ind->deviceAddr.uap, ind->deviceAddr.nap);
            CONSTRUCT_HIGH_MSG("tgFeatures: roleSupported %x roleConfig %x srAvrcpVersion %x srFeatures %x\n",
                ind->tgFeatures.roleSupported, ind->tgFeatures.roleConfig, ind->tgFeatures.srAvrcpVersion, ind->tgFeatures.srFeatures);
            CONSTRUCT_HIGH_MSG("ctFeatures: roleSupported %x roleConfig %x srAvrcpVersion %x srFeatures %x\n",
                ind->ctFeatures.roleSupported, ind->ctFeatures.roleConfig, ind->ctFeatures.srAvrcpVersion, ind->ctFeatures.srFeatures);
        }
        break;
        case CSR_BT_AVRCP_TG_MP_REGISTER_REQ:
        {
            CsrBtAvrcpTgMpRegisterReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_MP_REGISTER_REQ playerHandle %x notificationMask %x configMask %x pasLen %x pas:\n",
                req->playerHandle, req->notificationMask, req->configMask, req->pasLen);
            for (i=0; i<req->pasLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->pas[i]);
            }
            CONSTRUCT_HIGH_MSG("\nmajorType %x subType %x\n", req->majorType, req->subType);
        }
        break;
        case CSR_BT_AVRCP_TG_MP_REGISTER_CFM:
        {
            CsrBtAvrcpTgMpRegisterCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_MP_REGISTER_CFM playerId %x resultCode %x resultSupplier %x\n",
                cfm->playerId, cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AVRCP_TG_MP_UNREGISTER_REQ:
        {
            CsrBtAvrcpTgMpUnregisterReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_MP_UNREGISTER_REQ phandle %x playerId %x\n", req->phandle, req->playerId);
        }
        break;
        case CSR_BT_AVRCP_TG_MP_UNREGISTER_CFM:
        {
            CsrBtAvrcpTgMpUnregisterCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_MP_UNREGISTER_CFM playerId %x resultCode %x resultSupplier %x\n",
                cfm->playerId, cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AVRCP_TG_NOTI_REQ:
        {
            CsrBtAvrcpTgNotiReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_NOTI_REQ playerId %x notiId %x notiData:", req->playerId, req->notiId);
            for (i=0; i<CSR_BT_AVRCP_TG_NOTI_MAX_SIZE; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", req->notiData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AVRCP_TG_NOTI_CFM:
        {
            CsrBtAvrcpTgNotiCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_NOTI_CFM playerId %x notiId %x resultCode %x resultSupplier %x\n",
                cfm->playerId, cfm->notiId, cfm->resultCode, cfm->resultSupplier);
        }
        case CSR_BT_AVRCP_TG_NOTI_IND:
        {
            CsrBtAvrcpTgNotiInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_NOTI_IND connectionId %x playerId %x notiId %x playbackInterval %x ",
                ind->connectionId, ind->playerId, ind->notiId, ind->playbackInterval);
            CONSTRUCT_HIGH_MSG("%x\n", ind->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_NOTI_RES:
        {
            CsrBtAvrcpTgNotiRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_NOTI_RES connectionId %x notiId %x status %x msgId %x notiData:",
                res->connectionId, res->notiId, res->status, res->msgId);
            for (i=0; i<CSR_BT_AVRCP_TG_NOTI_MAX_SIZE; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", res->notiData[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AVRCP_TG_PASS_THROUGH_IND:
        {
            CsrBtAvrcpTgPassThroughInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_PASS_THROUGH_IND connectionId %x playerId %x operationId %x state %x ",
                ind->connectionId, ind->playerId, ind->operationId, ind->state);
            CONSTRUCT_HIGH_MSG("msgId %x\n", ind->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_PASS_THROUGH_RES:
        {
            CsrBtAvrcpTgPassThroughRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_PASS_THROUGH_RES  connectionId %x status %x msgId %x\n",
                res->connectionId, res->status, res->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_PAS_CURRENT_IND:
        {
            CsrBtAvrcpTgPasCurrentInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_PAS_CURRENT_IND connectionId %x playerId %x msgId %x attIdCount %x attId:",
                ind->connectionId, ind->playerId, ind->msgId, ind->attIdCount);
            for (i=0; i<ind->attIdCount; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->attId[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AVRCP_TG_PAS_CURRENT_RES:
        {
            CsrBtAvrcpTgPasCurrentRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_PAS_CURRENT_RES connectionId %x msgId %x status %x attValPairCount %x\n",
                res->connectionId, res->msgId, res->status, res->attValPairCount);
            for (i=0; i<res->attValPairCount; i++)
            {
                CONSTRUCT_HIGH_MSG("attribId %x valueId %x\n", res->attValPair[i].attribId, res->attValPair[i].valueId);
            }
        }
        break;
        case CSR_BT_AVRCP_TG_SET_VOLUME_IND:
        {
            CsrBtAvrcpTgSetVolumeInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_SET_VOLUME_IND connectionId %x playerId %x volume %x msgId %x ",
                ind->connectionId, ind->playerId, ind->volume, ind->msgId);
            CONSTRUCT_HIGH_MSG("tLabel %x\n", ind->tLabel);
        }
        break;
        case CSR_BT_AVRCP_TG_SET_VOLUME_RES:
        {
            CsrBtAvrcpTgSetVolumeRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_SET_VOLUME_RES connectionId %x volume %x status %x msgId %x ",
                res->connectionId, res->volume, res->status, res->msgId);
            CONSTRUCT_HIGH_MSG("tLabel %x\n", res->tLabel);
        }
        break;
        case CSR_BT_AVRCP_TG_GET_PLAY_STATUS_IND:
        {
            CsrBtAvrcpTgGetPlayStatusInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_GET_PLAY_STATUS_IND connectionId %x playerId %x msgId %x\n",
                ind->connectionId, ind->playerId, ind->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_GET_PLAY_STATUS_RES:
        {
            CsrBtAvrcpTgGetPlayStatusRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_GET_PLAY_STATUS_RES connectionId %x songLength %x songPosition %x playStatus %x ", 
                res->connectionId, res->songLength, res->songPosition, res->playStatus);
            CONSTRUCT_HIGH_MSG("msgId %x status %x\n", res->msgId, res->status);
        }
        break;
        case CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_IND:
        {
            CsrBtAvrcpTgSetAddressedPlayerInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_IND connectionId %x playerId %x msgId %x\n", 
                ind->connectionId, ind->playerId, ind->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_RES:
        {
            CsrBtAvrcpTgSetAddressedPlayerRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_RES connectionId %x uidCounter %x msgId %x playerId %x ",
                res->connectionId, res->uidCounter, res->msgId, res->playerId);
            CONSTRUCT_HIGH_MSG("status %x\n", res->status);
        }
        break;
        case CSR_BT_AVRCP_TG_INFORM_BATTERY_STATUS_IND:
        {
            CsrBtAvrcpTgInformBatteryStatusInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_INFORM_BATTERY_STATUS_IND connectionId %x playerId %x batStatus %x\n",
                ind->connectionId, ind->playerId, ind->batStatus);
        }
        break;
#if 0
        case CSR_BT_AVRCP_SECURITY_IN_REQ:
        {
            CsrBtAvrcpSecurityInReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_SECURITY_IN_REQ phandle %x secLevel %x config %x\n", req->phandle, req->secLevel, req->config);
        }
        break;
        case CSR_BT_AVRCP_SECURITY_IN_CFM:
        {
            CsrBtAvrcpSecurityInCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_SECURITY_IN_CFM resultCode %x resultSupplier %x\n", cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AVRCP_SECURITY_OUT_REQ:
        {
            CsrBtAvrcpSecurityOutReq *req =prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_SECURITY_OUT_REQ phandle %x secLevel %x config %x\n",
                req->phandle, req->secLevel, req->config);
        }
        break;
        case CSR_BT_AVRCP_SECURITY_OUT_CFM:
        {
            CsrBtAvrcpSecurityOutCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_SECURITY_OUT_CFM resultCode %x resultSupplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AVRCP_TG_PAS_SET_REQ:
        {
            CsrBtAvrcpTgPasSetReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_PAS_SET_REQ phandle %x playerId %x attValPairCount %x\n",
                req->phandle, req->playerId, req->attValPairCount);
            for (i=0; i<req->attValPairCount; i++)
            {
                CONSTRUCT_HIGH_MSG("attribId %x valueId %x\n", req->attValPair[i].attribId, req->attValPair[i].valueId);
            }
        }
        break;
        case CSR_BT_AVRCP_TG_PAS_SET_CFM:
        {
            CsrBtAvrcpTgPasSetCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_PAS_SET_CFM playerId %x resultCode %x resultSupplier %x\n",
                cfm->playerId, cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AVRCP_TG_PAS_SET_IND:
        {
            CsrBtAvrcpTgPasSetInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_PAS_SET_IND connectionId %x playerId %x msgId %x attValPairCount %x\n",
                ind->connectionId, ind->playerId, ind->msgId, ind->attValPairCount);
            for (i=0; i<ind->attValPairCount; i++)
            {
                CONSTRUCT_HIGH_MSG("attribId %x valueId %x\n", ind->attValPair[i].attribId, ind->attValPair[i].valueId);
            }
        }
        break;
        case CSR_BT_AVRCP_TG_PAS_SET_RES:
        {
            CsrBtAvrcpTgPasSetRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_PAS_SET_RES connectionId %x msgId %x status %x\n",
                res->connectionId, res->msgId, res->status);
        }
        break;
        case CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_REQ:
        {
            CsrBtAvrcpTgSetAddressedPlayerReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_REQ phandle %x phandle %x uidCounter %x\n",
                req->phandle, req->playerId, req->uidCounter);
        }
        break;
        case CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_CFM:
        {
            CsrBtAvrcpTgSetAddressedPlayerCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_CFM playerId %x resultCode %x resultSupplier %x\n",
                cfm->playerId, cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_AVRCP_TG_PLAY_IND:
        {
            CsrBtAvrcpTgPlayInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_PLAY_IND connectionId %x playerId %x scope %x uidCounter %x ",
                ind->connectionId, ind->playerId, ind->scope, ind->uidCounter);
            CONSTRUCT_HIGH_MSG("msgId %x\n", ind->msgId);
        }
        break;

        case CSR_BT_AVRCP_TG_PLAY_RES:
        {
            CsrBtAvrcpTgPlayRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_PLAY_RES connectionId %x status %x scope %x msgId %x\n",
                res->connectionId, res->status, res->scope, res->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_SEARCH_IND:
        {
            CsrBtAvrcpTgSearchInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_SEARCH_IND connectionId %x playerId %x msgId %x\n", ind->connectionId, ind->playerId, ind->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_SEARCH_RES:
        {
            CsrBtAvrcpTgSearchRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_SEARCH_RES connectionId %x uidCounter %x numberOfItems %x status %x ",
                res->connectionId, res->uidCounter, res->numberOfItems, res->status);
            CONSTRUCT_HIGH_MSG("msgId %x\n", res->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_CHANGE_PATH_IND:
        {
            CsrBtAvrcpTgChangePathInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_CHANGE_PATH_IND connectionId %x playerId %x folderDir %x msgId %x\n",
                ind->connectionId, ind->playerId, ind->folderDir, ind->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_CHANGE_PATH_RES:
        {
            CsrBtAvrcpTgChangePathRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_CHANGE_PATH_RES connectionId %x itemsCount %x msgId %x status %x\n",
                res->connectionId, res->itemsCount, res->msgId, res->status);
        }
        break;
        case CSR_BT_AVRCP_TG_GET_FOLDER_ITEMS_IND:
        {
            CsrBtAvrcpTgGetFolderItemsInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_GET_FOLDER_ITEMS_IND connectionId %x playerId %x scope %x startItem %x\n",
                ind->connectionId, ind->playerId, ind->scope, ind->startItem);
            CONSTRUCT_HIGH_MSG("endItem %x attributeMask %x maxData %x msgId %x\n", ind->endItem, ind->attributeMask, ind->maxData, ind->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_GET_FOLDER_ITEMS_RES:
        {
            CsrBtAvrcpTgGetFolderItemsRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_GET_FOLDER_ITEMS_RES connectionId %x itemsCount %x uidCounter %x itemsLen %x ",
                res->connectionId, res->itemsCount, res->uidCounter, res->itemsLen);
            CONSTRUCT_HIGH_MSG("msgId %x status %x items:", res->msgId, res->status);
            for (i=0; i<res->itemsCount; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", res->items[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AVRCP_TG_GET_ATTRIBUTES_IND:
        {
            CsrBtAvrcpTgGetAttributesInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_GET_ATTRIBUTES_IND connectionId %x playerId %x scope %x ",
                ind->connectionId, ind->playerId, ind->scope);
            CONSTRUCT_HIGH_MSG("attributeMask %x maxData %x msgId %x uidCounter %x\n", ind->attributeMask, ind->maxData, ind->msgId, ind->uidCounter);
        }
        break;
        case CSR_BT_AVRCP_TG_GET_ATTRIBUTES_RES:
        {
            CsrBtAvrcpTgGetAttributesRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_GET_ATTRIBUTES_RES connectionId %x attribCount %x msgId %x attribDataLen %x attribData:\n",
                res->connectionId, res->attribCount, res->msgId, res->attribDataLen);
            for (i=0; i<res->attribDataLen; i++)
            {
                CONSTRUCT_HIGH_MSG ("%x ", res->attribData[i]);
            }
            CONSTRUCT_HIGH_MSG("\nstatus %x", res->status);
        }
        break;
        case CSR_BT_AVRCP_TG_SET_BROWSED_PLAYER_IND:
        {
            CsrBtAvrcpTgSetBrowsedPlayerInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_SET_BROWSED_PLAYER_IND connectionId %x phandle %x playerId %x msgId %x\n",
                ind->connectionId, ind->phandle, ind->playerId, ind->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_SET_BROWSED_PLAYER_RES:
        {
            CsrBtAvrcpTgSetBrowsedPlayerRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_SET_BROWSED_PLAYER_RES connectionId %x uidCounter %x itemsCount %x folderDepth %x\n",
                res->connectionId, res->uidCounter, res->itemsCount, res->folderDepth);
            CONSTRUCT_HIGH_MSG("msgId %x playerId %x status %x folderNamesLen %x folderNames:\n",
                res->msgId, res->playerId, res->status, res->folderNamesLen);
            for (i=0; i<res->folderNamesLen; i++)
            {
                CONSTRUCT_HIGH_MSG("%x", res->folderNames[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_AVRCP_TG_ADD_TO_NOW_PLAYING_IND:
        {
            CsrBtAvrcpTgAddToNowPlayingInd *ind  = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_ADD_TO_NOW_PLAYING_IND connectionId %x playerId %x uidCounter %x scope %x ",
                ind->connectionId, ind->playerId, ind->uidCounter, ind->scope);
            CONSTRUCT_HIGH_MSG("msgId %x\n", ind->msgId);
        }
        break;
        case CSR_BT_AVRCP_TG_ADD_TO_NOW_PLAYING_RES:
        {
            CsrBtAvrcpTgAddToNowPlayingRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_ADD_TO_NOW_PLAYING_RES connectionId %x status %x msgId %x\n", 
                res->connectionId, res->status, res->msgId);
        }
        break;  
        case CSR_BT_AVRCP_TG_GET_TOTAL_NUMBER_OF_ITEMS_IND:
        {
            CsrBtAvrcpTgGetTotalNumberOfItemsInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_GET_TOTAL_NUMBER_OF_ITEMS_IND connectionId %x playerId %x scope %x scope %x\n",
                ind->connectionId, ind->playerId, ind->scope, ind->scope);
        }
        break;
        case CSR_BT_AVRCP_TG_GET_TOTAL_NUMBER_OF_ITEMS_RES:
        {
            CsrBtAvrcpTgGetTotalNumberOfItemsRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_AVRCP_TG_GET_TOTAL_NUMBER_OF_ITEMS_RES connectionId %x noOfItems %x uidCounter %x msgId %x ",
                res->connectionId, res->noOfItems, res->uidCounter, res->msgId);
            CONSTRUCT_HIGH_MSG("status %x\n", res->status);
        }
        break;
        case CSR_BT_AVRCP_TG_INFORM_DISP_CHARSET_IND:
        {
            CsrBtAvrcpTgInformDispCharsetInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_AVRCP_TG_INFORM_DISP_CHARSET_IND connectionId %x playerId %x charsetCount %x charset:\n", 
                ind->connectionId, ind->playerId, ind->charsetCount);
            for (i=0; i<ind->charsetCount; i++)
            {
                CONSTRUCT_HIGH_MSG("%x ", ind->charset[i]);
            }
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
#endif
        default:
            CONSTRUCT_HIGH_MSG("<== AVRCP UNKNOWN MSG TYPE %x \n", *type);
        break;
    }
}
#endif /* EXCLUDE_CSR_BT_AVRCP_MODULE */

static void synLogPrimGatt (void *prim)
{
    CsrPrim *type = prim;
    CsrUint8 i;

    switch (*type)
    {
        case CSR_BT_GATT_REGISTER_REQ:
        {
            CsrBtGattRegisterReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_REGISTER_REQ Handle %x Context %x \n", req->pHandle, req->context);
        }
        break;

        case CSR_BT_GATT_REGISTER_CFM:
        {
            CsrBtGattRegisterCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_REGISTER_CFM GattId %x ResultCode %x Supp %x \n", 
                cfm->gattId, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_GATT_SET_EVENT_MASK_REQ:
        {
            CsrBtGattSetEventMaskReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_SET_EVENT_MASK_REQ GattId %x Mask %x \n", 
                req->gattId, req->eventMask);
        }
        break;

        case CSR_BT_GATT_SET_EVENT_MASK_CFM:
        {
        CsrBtGattSetEventMaskCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_SET_EVENT_MASK_CFM GattId %x ResultCode %x Supp %x \n", 
                cfm->gattId, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_GATT_UNREGISTER_REQ:
        {
            CsrBtGattUnregisterReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_UNREGISTER_REQ GattId %x \n", req->gattId);
        }
        break;

        case CSR_BT_GATT_UNREGISTER_CFM:
        {
            CsrBtGattUnregisterCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_UNREGISTER_CFM GattId %x ResultCode %x Supp %x \n", 
                cfm->gattId, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_GATT_DB_ALLOC_REQ:
        {
            CsrBtGattDbAllocReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_DB_ALLOC_REQ GattId %x Num %x PrefHandle %x\n", 
                req->gattId, req->numOfAttrHandles, req->preferredStartHandle);
        }
        break;

        case CSR_BT_GATT_DB_ALLOC_CFM:
        {
            CsrBtGattDbAllocCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DB_ALLOC_CFM GattId %x ResultCode %x Supplier %x Start %x ",
                       cfm->gattId, cfm->resultCode, cfm->resultSupplier, 
                       cfm->start);
            CONSTRUCT_HIGH_MSG("End %x Pref %x\n", cfm->end, cfm->preferredStartHandle);
        }
        break;

        case CSR_BT_GATT_DB_DEALLOC_REQ:
        {
            CsrBtGattDbDeallocReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_DB_DEALLOC_REQ GattId %x \n", req->gattId);
        }
        break;

        case CSR_BT_GATT_DB_DEALLOC_CFM:
        {
            CsrBtGattDbDeallocCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DB_DEALLOC_CFM GattId %x ResultCode %x Supp %x Start %x ",
                       cfm->gattId, cfm->resultCode, cfm->resultSupplier, cfm->start);
            CONSTRUCT_HIGH_MSG("End %x\n", cfm->end);
        }
        break;

        case CSR_BT_GATT_DB_REMOVE_REQ:
        {
            CsrBtGattDbRemoveReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_DB_REMOVE_REQ GattId %x Start %x End %x\n", 
                req->gattId, req->start, req->end);
        }
        break;

        case CSR_BT_GATT_DB_REMOVE_CFM:
        {
            CsrBtGattDbRemoveCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DB_REMOVE_CFM GattId %x ResultCode %x Supp %x NumAttr %x\n",
                       cfm->gattId, cfm->resultCode, cfm->resultSupplier, cfm->numOfAttr);
        }
        break;

        case CSR_BT_GATT_DB_ACCESS_RES:
        {
            CsrBtGattDbAccessRes *res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_DB_ACCESS_RES GattId %x BtConnId %x Handle %x Res %x ",
                       res->gattId, res->btConnId, res->attrHandle, res->responseCode);
            CONSTRUCT_HIGH_MSG("Len %x\n", res->valueLength);
        }
        break;

        case CSR_BT_GATT_EVENT_SEND_REQ:
            {
            CsrBtGattEventSendReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_EVENT_SEND_REQ GattId %x BtConnId %x Handle %x EndHandle %x ",
                       req->gattId, req->btConnId, req->attrHandle, req->endGroupHandle);
            CONSTRUCT_HIGH_MSG("Flags %x Len %x\n",req->flags, req->valueLength);
        }
        break;

        case CSR_BT_GATT_EVENT_SEND_CFM:
        {
            CsrBtGattEventSendCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_EVENT_SEND_CFM GattId %x ConnId %x ResultCode %x Supp %x\n",
                       cfm->gattId, cfm->btConnId, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_GATT_DISCOVER_SERVICES_REQ:
        {
            CsrBtGattDiscoverServicesReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_DISCOVER_SERVICES_REQ GattId %x BtConnId %x Len %x Uuid %x\n",
                       req->gattId, req->btConnId, req->uuid.length, req->uuid.uuid[0]);
        }
        break;

        case CSR_BT_GATT_DISCOVER_SERVICES_IND:
        {
            CsrBtGattDiscoverServicesInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DISCOVER_SERVICES_IND GattId %x ConnId %x Start %x End %x ",
                       ind->gattId, ind->btConnId, ind->startHandle, ind->endHandle);
            CONSTRUCT_HIGH_MSG("len %x Uuid %x\n", ind->uuid.length, ind->uuid.uuid[0]);
        }
        break;

        case CSR_BT_GATT_DISCOVER_SERVICES_CFM:
        {
            CsrBtGattDiscoverServicesCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DISCOVER_SERVICES_CFM GattId %x ConnId %x ResultCode %x Supp %x\n",
                       cfm->gattId, cfm->btConnId, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_REQ:
        {
            CsrBtGattDiscoverCharacReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_DISCOVER_CHARAC_REQ GattId %x BtConnId %x Len %x Uuid %x ",
                       req->gattId, req->btConnId, req->uuid.length, req->uuid.uuid[0]);
            CONSTRUCT_HIGH_MSG("start %x end %x\n", req->startHandle, req->endGroupHandle);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_IND:
        {
            CsrBtGattDiscoverCharacInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DISCOVER_CHARAC_IND GattId %x ConnId %x DecHandle %x Property %x ",
                       ind->gattId, ind->btConnId, ind->declarationHandle, ind->property);
            CONSTRUCT_HIGH_MSG("len %x Uuid %x Value Handle %x\n", ind->uuid.length, ind->uuid.uuid[0], ind->valueHandle);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            CsrBtGattDiscoverCharacCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DISCOVER_CHARAC_CFM GattId %x ConnId %x ResultCode %x Supp %x\n",
                       cfm->gattId, cfm->btConnId, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_REQ:
        {
            CsrBtGattDiscoverCharacDescriptorsReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_REQ GattId %x BtConnId %x start %x end %x\n",
                       req->gattId, req->btConnId, req->startHandle, req->endGroupHandle);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND:
        {
            CsrBtGattDiscoverCharacDescriptorsInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND GattId %x ConnId %x DecHandle %x len %x ",
                       ind->gattId, ind->btConnId, ind->descriptorHandle, ind->uuid.length);
            CONSTRUCT_HIGH_MSG("Uuid %x\n", ind->uuid.uuid[0]);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            CsrBtGattDiscoverCharacDescriptorsCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM GattId %x ConnId %x ResultCode %x Supp %x\n",
                       cfm->gattId, cfm->btConnId, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_GATT_READ_REQ:
        {
            CsrBtGattReadReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_READ_REQ GattId %x BtConnId %x Handle %x Offset %x ",
                       req->gattId, req->btConnId, req->handle, req->offset);
             CONSTRUCT_HIGH_MSG("Flags %x\n", req->flags);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            CsrBtGattReadCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_READ_CFM GattId %x ConnId %x ResultCode %x Supplier %x ",
                       cfm->gattId, cfm->btConnId, cfm->resultCode, cfm->resultSupplier);
            CONSTRUCT_HIGH_MSG("Handle %x len %x\n", cfm->handle, cfm->valueLength);
        }
        break;

        case CSR_BT_GATT_READ_BY_UUID_REQ:
        {
            CsrBtGattReadByUuidReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_READ_BY_UUID_REQ GattId %x BtConnId %x Start %x End %x ",
                       req->gattId, req->btConnId, req->startHandle, req->endGroupHandle);
            CONSTRUCT_HIGH_MSG("len %x Uuid %x\n", req->uuid.length, req->uuid.uuid[0]);
        }
        break;

        case CSR_BT_GATT_READ_BY_UUID_CFM:
        {
            CsrBtGattReadByUuidCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_READ_BY_UUID_CFM GattId %x ConnId %x ResultCode %x Supplier %x ",
                       cfm->gattId, cfm->btConnId, cfm->resultCode, cfm->resultSupplier);
            CONSTRUCT_HIGH_MSG("len %x uuid %x\n", cfm->uuid.length, cfm->uuid.uuid[0]);
        }
        break;

        case CSR_BT_GATT_WRITE_REQ:
        {
            CsrBtGattWriteReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_WRITE_REQ GattId %x BtConnId %x Flags %x Count %x \n",
                       req->gattId, req->btConnId, req->flags, req->attrWritePairsCount);
        }
        break;

        case CSR_BT_GATT_WRITE_CFM:
        {
            CsrBtGattWriteCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_WRITE_CFM GattId %x ConnId %x ResultCode %x Supplier %x ",
                       cfm->gattId, cfm->btConnId, cfm->resultCode, cfm->resultSupplier);
            CONSTRUCT_HIGH_MSG("handle %x \n", cfm->handle);
        }
        break;

        case CSR_BT_GATT_DB_ADD_REQ:
        {
            CsrBtGattDbAddReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_DB_ADD_REQ GattId %x \n", req->gattId);
        }
        break;

        case CSR_BT_GATT_FIND_INCL_SERVICES_REQ:
        {
            CsrBtGattFindInclServicesReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_FIND_INCL_SERVICES_REQ GattId %x btConnId %x startHandle %x endGroupHandle %x \n",
                       req->gattId, req->btConnId, req->startHandle, req->endGroupHandle);
        }
        break;

        case CSR_BT_GATT_CANCEL_REQ:
        {
            CsrBtGattCancelReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_GATT_CANCEL_REQ GattId %x btConnId %x \n",
                       req->gattId, req->btConnId);
        }
        break;

        case CSR_BT_GATT_DB_ADD_CFM:
        {
            CsrBtGattDbAddCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DB_ADD_CFM GattId %x resultCode %x resultSupplier %x\n",
                       cfm->gattId, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            CsrBtGattDbAccessReadInd *ind = prim;

            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DB_ACCESS_READ_IND GattId %x btConnId %x attrHandle %x offset %x ",
                       ind->gattId, ind->btConnId, ind->attrHandle, ind->offset);

            CONSTRUCT_HIGH_MSG("maxRspValueLength %x check %x connInfo %x ", ind->maxRspValueLength, ind->check, ind->connInfo);
            
            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x) \n",
                       ind->address.type, ind->address.addr.lap,
                        ind->address.addr.uap, ind->address.addr.nap);
        }
        break;

        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            CsrBtGattDbAccessWriteInd *ind = prim;

            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_DB_ACCESS_WRITE_IND GattId %x btConnId %x check %x connInfo %x ",
                       ind->gattId, ind->btConnId, ind->check, ind->connInfo);

            CONSTRUCT_HIGH_MSG("writeUnitCount %x attrHandle %x ", ind->writeUnitCount, ind->attrHandle);

            CONSTRUCT_HIGH_MSG("Address type %x lap %x uap %x nap %x \n",
                       ind->address.type, ind->address.addr.lap,
                        ind->address.addr.uap, ind->address.addr.nap);

            for(i = 0; i < ind->writeUnitCount; i++)
            {
                CONSTRUCT_HIGH_MSG("<== writeUnit[%d].attrHandle %x offset %x valueLength %x \n",
                       i, ind->writeUnit[i].attrHandle, ind->writeUnit[i].offset,
                       ind->writeUnit[i].valueLength);
                if(ind->writeUnit[i].value)
                {
                CONSTRUCT_HIGH_MSG("<== value %x \n",
                       ind->writeUnit[i].value[0]);
                }
            }
        }
        break;

        case CSR_BT_GATT_FIND_INCL_SERVICES_IND:
        {
            CsrBtGattFindInclServicesInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_FIND_INCL_SERVICES_IND GattId %x btConnId %x attrHandle %x startHandle %x ",
                        ind->gattId, ind->btConnId, ind->attrHandle, ind->startHandle);
            CONSTRUCT_HIGH_MSG("endGroupHandle %x uuid.length %x uuid.uuid[0] %x\n", 
                        ind->endGroupHandle, ind->uuid.length, ind->uuid.uuid[0]);
        }
        break;

        case CSR_BT_GATT_FIND_INCL_SERVICES_CFM:
        {
            CsrBtGattStdBtConnIdCfm *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_FIND_INCL_SERVICES_CFM GattId %x btConnId %x resultCode %x resultSupplier %x\n",
                       ind->gattId, ind->btConnId, ind->resultCode, ind->resultSupplier);
        }
        break;

        case CSR_BT_GATT_READ_BY_UUID_IND:
        {
            CsrBtGattReadByUuidInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_READ_BY_UUID_IND GattId %x btConnId %x valueHandle %x valueLength %x \n",
                       ind->gattId, ind->btConnId, ind->valueHandle, ind->valueLength);
            if(ind->value)
            {
                CONSTRUCT_HIGH_MSG("<== value[0] %x\n", ind->value[0]);
            }
        }
        break;

        case CSR_BT_GATT_CLIENT_INDICATION_IND:
        {
            CsrBtGattClientIndicationInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_CLIENT_INDICATION_IND GattId %x btConnId %x valueHandle %x valueLength %x \n",
                       ind->gattId, ind->btConnId, ind->valueHandle, ind->valueLength);
            if(ind->value)
            {
                CONSTRUCT_HIGH_MSG("<== value[0] %x\n", ind->value[0]);
            }
        }
        break;

        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            CsrBtGattClientIndicationInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_GATT_CLIENT_NOTIFICATION_IND GattId %x btConnId %x valueHandle %x valueLength %x \n",
                       ind->gattId, ind->btConnId, ind->valueHandle, ind->valueLength);
            if(ind->value)
            {
                CONSTRUCT_HIGH_MSG("<== value[0] %x\n", ind->value[0]);
            }

        }
        break;

        default:
            CONSTRUCT_HIGH_MSG("<== GATT UNKNOWN MSG TYPE %x \n", *type);
        break;
    }
}

static void synLogPrimCm(void *prim)
{
    CsrPrim *type = prim;
    CsrUint8 i;

    switch (*type)
    {
        case CSR_BT_CM_REGISTER_HANDLER_REQ:
        {
            CsrBtCmRegisterHandlerReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_REGISTER_HANDLER_REQ HandlerType %x Handle %x Flags %x\n", 
                req->handlerType, req->handle, req->flags);
        }
        break;

        case CSR_BT_CM_READ_LOCAL_BD_ADDR_REQ:
        {
            CsrBtCmReadLocalBdAddrReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_LOCAL_BD_ADDR_REQ\n");
        }
        break;

        case CSR_BT_CM_READ_LOCAL_BD_ADDR_CFM:
        {
            CsrBtCmReadLocalBdAddrCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_LOCAL_BD_ADDR_CFM lap %x uap %x nap %x \n", \
                cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SET_LOCAL_NAME_REQ:
        {
            CsrBtCmSetLocalNameReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SET_LOCAL_NAME_REQ\n");
        }
        break;

        case CSR_BT_CM_SET_LOCAL_NAME_CFM:
        {
            CsrBtCmSetLocalNameCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SET_LOCAL_NAME_CFM\n");
        }
        break;

        case CSR_BT_CM_READ_LOCAL_NAME_REQ:
        {
            CsrBtCmReadLocalNameReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_LOCAL_NAME_REQ\n");
        }
        break;

        case CSR_BT_CM_READ_LOCAL_NAME_CFM:
        {
            CsrBtCmReadLocalNameCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_LOCAL_NAME_CFM\n");
        }
        break;

        case CSR_BT_CM_SET_EVENT_MASK_REQ:
        {
            CsrBtCmSetEventMaskReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SET_EVENT_MASK_REQ Handle %x Mask %x Cond %x\n",
            req->phandle, req->eventMask, req->conditionMask);
        }
        break;

        case CSR_BT_CM_SET_EVENT_MASK_CFM:
        {
            CsrBtCmSetEventMaskCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SET_EVENT_MASK_CFM Mask %x\n", cfm->eventMask);
        }
        break;

        case CSR_BT_CM_READ_REMOTE_NAME_REQ:
        {
            CsrBtCmReadRemoteNameReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_REMOTE_NAME_REQ \
Handle %x Addr::lap %x uap %x nap %x \n",
                req->phandle, req->deviceAddr.lap, req->deviceAddr.uap,
                req->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_READ_REMOTE_NAME_CFM:
        {
            CsrBtCmReadRemoteNameCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_REMOTE_NAME_CFM \
lap %x uap %x nap %x ",cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap);
            CONSTRUCT_HIGH_MSG("Result %x Supplier %x\n",
            cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_READ_REMOTE_VERSION_REQ:
        {
            CsrBtCmReadRemoteVersionReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_REMOTE_VERSION_REQ \
Handle %x Transport %x ", req->appHandle, req->transportType);
            CONSTRUCT_HIGH_MSG("Addr::type %x lap %x uap %x nap %x  \n",
            req->addressType, req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap );
        }
        break;

        case CSR_BT_CM_READ_REMOTE_VERSION_CFM:
        {
            CsrBtCmReadRemoteVersionCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_REMOTE_VERSION_CFM \
Addr::lap %x uap %x nap %x Result %x ",
            cfm->deviceAddr.lap, cfm->deviceAddr.uap,
            cfm->deviceAddr.nap, cfm->resultCode);

            CONSTRUCT_HIGH_MSG("supplier %x manufacturerName %x \
lmpVersion %x lmpSubversion ",
            cfm->resultSupplier, cfm->manufacturerName,
            cfm->lmpVersion, cfm->lmpSubversion);

            CONSTRUCT_HIGH_MSG("transportType %x addressType %x \n",
            cfm->transportType, cfm->addressType);
        }
        break;

        case CSR_BT_CM_WRITE_COD_REQ:
        {
            CsrBtCmWriteCodReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_WRITE_COD_REQ \
Handle %x Flags  %x ", req->appHandle, req->updateFlags);
            CONSTRUCT_HIGH_MSG("Service %x Cod::Major %x Minor %x \n",
            req->serviceClassOfDevice, req->majorClassOfDevice, req->minorClassOfDevice);
        }
        break;

        case CSR_BT_CM_WRITE_COD_CFM:
        {
            CsrBtCmWriteCodCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_WRITE_COD_CFM Result %x Supplier %x\n",
            cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_READ_COD_REQ:
        {
            CsrBtCmReadCodReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_COD_REQ Handle %x \n",req->appHandle);
        }
        break;

        case CSR_BT_CM_READ_COD_CFM:
        {
            CsrBtCmReadCodCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_COD_CFM Result %x Supplier %x Cod = %x \n",
            cfm->resultCode, cfm->resultSupplier, cfm->classOfDevice);
        }
        break;
#if 0
        case CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_REQ:
        {
            CsrBtCmWriteLinkSupervTimeoutReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_REQ Handle %x lap %x uap %x nap %x",
            req->phandle, req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap);

            CONSTRUCT_HIGH_MSG(" timeout %x\n", req->timeout);
        }
        break;
        case CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_CFM:
        {
            CsrBtCmWriteLinkSupervTimeoutCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_CFM lap %x uap %x nap %x",
                cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap);

            CONSTRUCT_HIGH_MSG("result %x supplier %x \n", cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_READ_TX_POWER_LEVEL_REQ:
        {
            CsrBtCmReadTxPowerLevelReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_TX_POWER_LEVEL_REQ Handle %x lap %x uap %x nap %x
                addrtype %x transtype %x leveltype %x\n", req->appHandle, req->deviceAddr.lap, req->deviceAddr.uap,
                req->deviceAddr.nap, req->addressType, req->transportType, req->levelType);
        }
        break;
        case CSR_BT_CM_READ_TX_POWER_LEVEL_CFM:
        {
            CsrBtCmReadTxPowerLevelCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_TX_POWER_LEVEL_CFM lap %x uap %x nap %x result %x supplier %x \
                transportType %x addressType %x powerlevel %x \n", cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap,
                cfm->resultCode, cfm->resultSupplier, , cfm->transportType, cfm->addressType, cfm->powerLevel);
        }
        break;

        case CSR_BT_CM_GET_LINK_QUALITY_REQ:
        {
            CsrBtCmGetLinkQualityReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_GET_LINK_QUALITY_REQ Handle %x lap %x uap %x nap %x \n",
                req->appHandle, req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap);
        }
        break;
        case CSR_BT_CM_GET_LINK_QUALITY_CFM:
        {
            CsrBtCmGetLinkQualityCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_GET_LINK_QUALITY_CFM lap %x uap %x nap %x result %x supplier %x \
                linkquality %x \n", cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap,
                cfm->resultCode, cfm->resultSupplier, , cfm->linkQuality);
        }
        break;

        case CSR_BT_CM_READ_RSSI_REQ:
        {
            CsrBtCmReadRssiReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_RSSI_REQ Handle %x lap %x uap %x nap %x \
                addresstype %x transporttype %x\n",req->appHandle, req->deviceAddr.lap, req->deviceAddr.uap,
                req->deviceAddr.nap, req->addressType, req->transportType);
        }
        break;
        case CSR_BT_CM_READ_RSSI_CFM:
        {
            CsrBtCmReadRssiCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_RSSI_CFM lap %x uap %x nap %x result %x supplier %x \
            addresstype %x transporttype %x rssi %x \n", cfm->deviceAddr.lap, cfm->deviceAddr.uap,
                cfm->deviceAddr.nap, cfm->resultCode, cfm->resultSupplier, , cfm->addressType,
                cfm->transportType, cfm->rssi);
        }
        break;

        case CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_REQ:
        {
            CsrBtCmDmSetLinkBehaviourReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_REQ Handle %x lap %x uap %x nap %x \
                addrtype %x l2capretry %x\n", req->appHandle, req->addr.lap, req->addr.uap,
                req->addr.nap, req->addrType, req->l2capRetry);
        }
        break;
        case CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_CFM:
        {
            CsrBtCmDmSetLinkBehaviourCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_CFM status= %x lap %x uap %x nap %x \
                addrtype %x\n", cfm->status, cfm->addr.lap, cfm->addr.uap, cfm->addr.nap, cfm->addrType);
        }
        break;


        case CSR_BT_CM_SDC_SEARCH_REQ:
        {
            CsrBtCmSdcSearchReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SDC_SEARCH_REQ Handle %x lap %x uap %x nap %x \
                extendedsearchflag %x servicelistsize %x servicelist = %x\n", req->appHandle,
                req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap, req->extendedUuidSearch,
                req->serviceListSize, req->serviceList);
        }
        break;
        case CSR_BT_CM_SDC_SEARCH_IND:
        {
            CsrBtCmSdcSearchInd *ind = prim;

            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SDC_SEARCH_IND lap %x uap %x nap %x \
                localServerChannel %x service %x serviceHandleListCount %x\n", ind->deviceAddr.lap, ind->deviceAddr.uap, ind->deviceAddr.nap,
                ind->localServerChannel, ind->service, ind->serviceHandleListCount);
        }
        break;
        case CSR_BT_CM_SDC_SEARCH_CFM:
        {
            CsrBtCmSdcSearchCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SDC_SEARCH_CFM status= %x lap %x uap %x nap %x \
                localserverchannel %x\n", cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap,
                cfm->localServerChannel);
        }
        break;

        case CM_SDC_SERVICE_SEARCH_ATTR_REQ:
        {
            CmSdcServiceSearchAttrReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CM_SDC_SERVICE_SEARCH_ATTR_REQ Handle %x lap %x uap %x nap %x \
                extendedsearchflag %x localserverchannel %x uuidtype = %x\n", req->appHandle,
                req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap, req->extendedUuidSearch,
                req->localServerChannel, req->uuidType);

            if(req->svcSearchAttrInfoList && req->svcSearchAttrInfoList->serviceListSize > 0)
            {
                uint16 size = (req->uuidType == CSR_BT_CM_SDC_UUID32) ? 4 : ((req->uuidType == CSR_BT_CM_SDC_UUID128) ? 8 : 0);
                CONSTRUCT_HIGH_MSG("serviceListSize= %x \n",req->svcSearchAttrInfoList->serviceListSize);
                DMSG_HEXDUMP(size * req->svcSearchAttrInfoList->serviceListSize, req->svcSearchAttrInfoList->serviceList);

                if(req->svcSearchAttrInfoList->attrInfoList && req->svcSearchAttrInfoList->attrInfoList->noOfAttr > 0)
                {
                CONSTRUCT_HIGH_MSG("noOfAttr= %x \n", req->svcSearchAttrInfoList->attrInfoList->noOfAttr );
                DMSG_HEXDUMP(sizeof(uint16) * req->svcSearchAttrInfoList->attrInfoList->noOfAttr,
                    req->svcSearchAttrInfoList->attrInfoList->attrList);
                }
            }
        }
        break;
        case CM_SDC_SERVICE_SEARCH_ATTR_CFM:
        {
            CmSdcServiceSearchAttrCfm *cfm = prim;

            CONSTRUCT_HIGH_MSG("<== CM_SDC_SERVICE_SEARCH_ATTR_CFM status= %x lap %x uap %x nap %x \
                 localserverchannel %x servicehandle %x result %x supplier %x\n", cfm->deviceAddr.lap,
                 cfm->deviceAddr.uap, cfm->deviceAddr.nap, cfm->localServerChannel, cfm->serviceHandle,
                 cfm->resultCode, cfm->resultSupplier, cfm->attributeListSize);
        }
        break;

        case CSR_BT_CM_SDC_ATTRIBUTE_REQ:
        {
            CsrBtCmSdcAttributeReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SDC_ATTRIBUTE_REQ serviceHandle %x attributeId %x attrUpperRange\
                %x maxBytes %x\n", req->serviceHandle, req->attributeIdentifier,
                req->upperRangeAttributeIdentifier, req->maxBytesToReturn);
        }
        break;
        case CSR_BT_CM_SDC_ATTRIBUTE_CFM:
        {
            CsrBtCmSdcAttributeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SDC_ATTRIBUTE_CFM status %x lap %x uap %x nap %x \
                 localserverchannel %x\n", cfm->deviceAddr.lap, cfm->deviceAddr.uap, cfm->deviceAddr.nap,
                 cfm->localServerChannel, cfm->resultCode, cfm->resultSupplier, cfm->attributeListSize);
        }
        break;

        case CSR_BT_CM_SDC_CANCEL_SEARCH_REQ:
        {
            CsrBtCmSdcAttributeReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SDC_ATTRIBUTE_REQ serviceHandle %x attributeId %x attrUpperRange\
                  %x maxBytes %x\n", req->serviceHandle, req->attributeIdentifier,
                  req->upperRangeAttributeIdentifier, req->maxBytesToReturn);
        }
        break;

        case CSR_BT_CM_SDC_UUID128_SEARCH_REQ:
        {
            CsrBtCmSdcUuid128SearchReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SDC_UUID128_SEARCH_REQ Handle %x lap %x uap %x nap %x \
                  serviceLstSize %x\n", req->appHandle, req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap,
                  req->serviceListSize);
        }
        break;

        case CSR_BT_CM_SDC_UUID128_SEARCH_IND:
        {
            CsrBtCmSdcUuid128SearchInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SDC_UUID128_SEARCH_IND lap %x uap %x nap %x localServerChannel\
                  %x service %x serviceHandleListCount %x\n", ind->deviceAddr.lap, ind->deviceAddr.uap, ind->deviceAddr.nap,
                  ind->localServerChannel, ind->service, ind->serviceHandleListCount);
        }
        break;

        case CSR_BT_CM_SDC_CLOSE_REQ:
        {
            CsrBtCmSdcCloseReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SDC_CLOSE_REQ Handle %x \n", req->appHandle);
        }
        break;
        case CSR_BT_CM_SDC_CLOSE_IND:
        {
            CsrBtCmSdcCloseInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SDC_CLOSE_IND localServerChannel %x lap %x uap %x nap %x \
                  %x result %x supplier %x\n", ind->localServerChannel, ind->deviceAddr.lap, ind->deviceAddr.uap,
                  ind->deviceAddr.nap,ind->resultCode, ind->resultSupplier);
        }
        break;

        case CSR_BT_CM_SDS_REGISTER_REQ:
        {
            CsrBtCmSdsRegisterReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SDS_REGISTER_REQ Handle %x serviceRecordSize %x context = %x\n",
                req->appHandle, req->serviceRecordSize, req->context);
        }
        break;
        case CSR_BT_CM_SDS_REGISTER_CFM:
        {
            CsrBtCmSdsRegisterCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SDS_REGISTER_CFM serviceRecHandle %x \
                  result %x supplier %x\n", cfm->serviceRecHandle, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_SDS_UNREGISTER_REQ:
        {
            CsrBtCmSdsUnregisterReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SDS_UNREGISTER_REQ Handle %x serviceRecHandle %x context = %x\n",
                req->appHandle, req->serviceRecHandle, req->context);
        }
        break;
        case CSR_BT_CM_SDS_UNREGISTER_CFM:
        {
            CsrBtCmSdsUnregisterCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SDS_UNREGISTER_CFM serviceRecHandle %x \
                  result %x supplier %x\n", cfm->serviceRecHandle, cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_CM_WRITE_SCAN_ENABLE_REQ:
        {
            CsrBtCmWriteScanEnableReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_WRITE_SCAN_ENABLE_REQ \
            Handle %x Disable (inquiry %x page %x)\n",
            req->appHandle, req->disableInquiryScan, req->disablePageScan);
        }
        break;
        case CSR_BT_CM_WRITE_SCAN_ENABLE_CFM:
        {
            CsrBtCmWriteScanEnableCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_WRITE_SCAN_ENABLE_CFM Result %x Supplier %x\n",
               cfm->resultCode, cfm->resultSupplier);
            }
        break;

        case CSR_BT_CM_READ_SCAN_ENABLE_REQ:
        {
            CsrBtCmReadScanEnableReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_SCAN_ENABLE_REQ Handle = %x \n", req->appHandle);
        }
        break;

        case CSR_BT_CM_READ_SCAN_ENABLE_CFM:
        {
            CsrBtCmReadScanEnableCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_SCAN_ENABLE_CFM \
                Scannable  %x Result %x Supplier %x\n",
                cfm->scanEnable, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_ENABLE_DUT_MODE_REQ:
        {
            CsrBtCmEnableDutModeReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_ENABLE_DUT_MODE_REQ Handle %x\n", req->appHandle);
        }
        break;
        case CSR_BT_CM_ENABLE_DUT_MODE_CFM:
        {
            CsrBtCmEnableDutModeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_ENABLE_DUT_MODE_CFM \
                StepNumber %x Result %x Supplier %x\n",
                cfm->stepNumber, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_DISABLE_DUT_MODE_REQ:
        {
            CsrBtCmEnableDutModeReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_DISABLE_DUT_MODE_REQ Handle %x\n", req->appHandle);
        }
        break;
        case CSR_BT_CM_DISABLE_DUT_MODE_CFM:
        {
            CsrBtCmEnableDutModeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_DISABLE_DUT_MODE_CFM \
                StepNumber %x Result %x Supplier %x\n",
                cfm->stepNumber, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_WRITE_PAGE_TO_REQ:
        {
            CsrBtCmWritePageToReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_WRITE_PAGE_TO_REQ Handle %x PageTimeout %x\n",
                req->appHandle, req->pageTimeout);
        }
        break;
        case CSR_BT_CM_WRITE_PAGE_TO_CFM:
        {
            CsrBtCmWritePageToCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_WRITE_PAGE_TO_CFM Result %x Supplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_REQ:
        {
            CsrBtCmWritePagescanSettingsReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_REQ \
                Handle %x PageTimeout %x Interval %x Window %x\n",
                req->appHandle, req->interval, req->window);
        }
        break;
        case CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_CFM:
        {
            CsrBtCmWritePagescanSettingsCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_CFM \
                Result %x Supplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_WRITE_PAGESCAN_TYPE_REQ:
        {
            CsrBtCmWritePagescanTypeReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_WRITE_PAGESCAN_TYPE_REQ \
                Handle %x ScanType %x \n",
                req->appHandle, req->scanType);
        }
        break;
        case CSR_BT_CM_WRITE_PAGESCAN_TYPE_CFM:
        {
            CsrBtCmWritePagescanTypeCfm *cfm = prim;
                CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_WRITE_PAGESCAN_TYPE_CFM \
                Result %x Supplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_REQ:
        {
            CsrBtCmWriteInquiryscanSettingsReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_REQ \
                Handle %x Interval %x Window %x\n",
                req->appHandle, req->interval, req->window);
        }
        break;
        case CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_CFM:
        {
            CsrBtCmWriteInquiryscanSettingsCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_CFM \
                Result %x Supplier %x\n", cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_REQ:
        {
            CsrBtCmWriteInquiryscanTypeReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_REQ \
                Handle %x ScanType %x \n",
                req->appHandle, req->scanType);
        }
        break;
        case CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_CFM:
        {
            CsrBtCmWriteInquiryscanTypeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_CFM \
                Result %x Supplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_READ_REMOTE_EXT_FEATURES_REQ:
        {
            CsrBtCmReadRemoteExtFeaturesReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_REMOTE_EXT_FEATURES_REQ \
                Handle %x PageNum %x Addr::lap %x uap %x nap %x \n",
                req->appHandle, req->pageNum, req->bd_addr.lap,
                req->bd_addr.uap, req->bd_addr.nap);
        }
        break;
        case CSR_BT_CM_READ_REMOTE_EXT_FEATURES_CFM:
        {
            CsrBtCmReadRemoteExtFeaturesCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_REMOTE_EXT_FEATURES_CFM \
                Result %x Supplier %x PageNum %x MaxpageNum %x \
                lmp0 %x lmp1 %x lmp2 %x lpm3 %x ",
                cfm->resultCode, cfm->resultSupplier, cfm->pageNum,
                cfm->maxPageNum, cfm->extLmpFeatures[0],
                cfm->extLmpFeatures[1], cfm->extLmpFeatures[2],
                cfm->extLmpFeatures[3]);

             CONSTRUCT_HIGH_MSG("Addr::lap %x uap %x nap %x \n",
                cfm->bd_addr.lap, cfm->bd_addr.uap, cfm->bd_addr.nap);
        }
        break;

        case CSR_BT_CM_READ_LOCAL_EXT_FEATURES_REQ:
        {
            CsrBtCmReadLocalExtFeaturesReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_LOCAL_EXT_FEATURES_REQ \
                Handle %x PageNum%x \n", req->appHandle, req->pageNum);
        }
        break;
        case CSR_BT_CM_READ_LOCAL_EXT_FEATURES_CFM:
        {
            CsrBtCmReadLocalExtFeaturesCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_LOCAL_EXT_FEATURES_CFM \
                Result %x Supplier %x PageNum %x MaxpageNum %x \n",
                cfm->resultCode, cfm->resultSupplier, cfm->pageNum, cfm->maxPageNum);

            CONSTRUCT_HIGH_MSG("LmpFeatures ::");
            DMSG_HEXDUMP(sizeof(cfm->extLmpFeatures),cfm->extLmpFeatures);
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;

        case CSR_BT_CM_SET_AFH_CHANNEL_CLASS_REQ:
        {
            CsrBtCmSetAfhChannelClassReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SET_AFH_CHANNEL_CLASS_REQ Handle %x \n",
                req->appHandle);

            CONSTRUCT_HIGH_MSG("Map ::");
            DMSG_HEXDUMP(sizeof(req->map),req->map);
            CONSTRUCT_HIGH_MSG("\n");
        }
        break;
        case CSR_BT_CM_SET_AFH_CHANNEL_CLASS_CFM:
        {
            CsrBtCmSetAfhChannelClassCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SET_AFH_CHANNEL_CLASS_CFM \
                Result %x Supplier %x\n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_REQ:
        {
            CsrBtCmReadAfhChannelAssessmentModeReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_REQ \
                Handle %x \n", req->appHandle);
        }
        break;
        case CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_CFM:
        {
            CsrBtCmReadAfhChannelAssessmentModeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_CFM \
                ClassMode %x Result %x Supplier %x\n",
                cfm->classMode, cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_WRITE_AFH_CHANNEL_ASSESSMENT_MODE_REQ:
        {
            CsrBtCmWriteAfhChannelAssessmentModeReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_REQ \
                Handle %x ClassMode %x\n",
                req->appHandle, req->classMode);
        }
        break;
        case CSR_BT_CM_WRITE_AFH_CHANNEL_ASSESSMENT_MODE_CFM:
        {
            CsrBtCmWriteAfhChannelAssessmentModeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_CFM \
                Result %x Supplier %x\n", cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_REQ:
        {
            CsrBtCmReadAdvertisingChTxPowerReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_REQ \
                Handle %x Context %x\n",
                req->appHandle, req->context);
        }
        break;
        case CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_CFM:
        {
            CsrBtCmReadAdvertisingChTxPowerCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_CFM \
                Result %x Supplier %x TxPower %x context %x \n",
                cfm->resultCode, cfm->resultSupplier,
                cfm->txPower, cfm->context);
        }
        break;

        case CSR_BT_CM_LE_SIRK_OPERATION_REQ:
        {
            CsrBtCmLeSirkOperationReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_SIRK_OPERATION_REQ Handle %x transType %x type %x \
                lap %x uap %x nap %x flag %x\n", req->appHandle, req->tpAddrt.tp_type, req->tpAddrt.addrt.type,
                req->tpAddrt.addrt.addr.lap, req->tpAddrt.addrt.addr.lap,req->tpAddrt.addrt.addr.uap, req->flags);
        }
        break;

        case CSR_BT_CM_LE_SIRK_OPERATION_CFM:
        {
            CsrBtCmLeSirkOperationCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_SIRK_OPERATION_CFM result %x supplier %x \
                flag %x type %x idAdress (lap %x uap %x nap %x) rpa (lap %x uap %x nap %x)\n",
                cfm->resultCode, cfm->tpAddrt.tp_type, cfm->tpAddrt.addrt.type, cfm->tpAddrt.addrt.addr.lap,
                cfm->tpAddrt.addrt.addr.uap, cfm->tpAddrt.addrt.addr.nap);
        }
        break;
        case CSR_BT_CM_INQUIRY_REQ:
        {
            CsrBtCmInquiryReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_INQUIRY_REQ  \
               handle %x configMask %x inquiryAccessCode %x \
               inquiryTimeout %x inquiryTxPowerLevel %x \n",
               req->appHandle,req->configMask,req->inquiryAccessCode,
               req->inquiryTimeout, req->inquiryTxPowerLevel);
        }
        break;

        case CSR_BT_CM_INQUIRY_RESULT_IND:
        {
            CsrBtCmInquiryResultInd*ind= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_INQUIRY_RESULT_IND  \
                Addr::lap %x uap %x nap %x ",
                ind->deviceAddr.lap, ind->deviceAddr.uap,
                ind->deviceAddr.nap);
            CONSTRUCT_HIGH_MSG("Cod %x ClockOffset %x EirDataLen %x \
                PageScanMode %x PageScanRepMode %x Rssi %x Status %x \n",
                ind->classOfDevice, ind->clockOffset, ind->eirDataLength,
                ind->pageScanMode, ind->pageScanRepMode, ind->rssi,
                ind->status);
        }
        break;

        case CSR_BT_CM_INQUIRY_CFM:
        {
            CsrBtCmInquiryCfm*req= prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_INQUIRY_CFM  \
               result = %x supplier %x \n",
               req->resultCode,req->resultSupplier);
        }
        break;

        case CSR_BT_CM_CANCEL_INQUIRY_REQ:
        {
            CsrBtCmCancelInquiryReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_CANCEL_INQUIRY_REQ  \
              Handle = %x \n", req->phandle);
        }
        break;

        case CSR_BT_CM_LE_SCAN_REQ:
        {
            CsrBtCmLeScanReq *ind = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_SCAN_REQ Handle %x Context %x \
                Mode %x ScanType %x ScanInterval %x ScanWindow %x ScanFilterPolicy %x \
                filterDup %x ", ind->appHandle, ind->context, ind->mode, ind->scanType,
                ind->scanInterval, ind->scanWindow, ind->scanningFilterPolicy,
                ind->filterDuplicates);
            CONSTRUCT_HIGH_MSG("addressCount %x\n", ind->addressCount);
        }
        break;

        case CSR_BT_CM_LE_REPORT_IND:
        {
            CsrBtCmLeReportInd *ind = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_REPORT_IND Event Type %x DataLen %x \
                Rssi %x Addr (type %x lap %x uap %x nap %x) \n",
                ind->eventType, ind->lengthData, ind->rssi,
                ind->address.type, ind->address.addr.lap, ind->address.addr.uap,
                ind->address.addr.nap);
        }
        break;

        case CSR_BT_CM_LE_SCAN_CFM:
        {
            CsrBtCmLeScanCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_SCAN_CFM Handle %x Context %x Mode %x \
                WhiteListEnable %x Result %x, Supplier %x \n",
                cfm->context, cfm->scanMode, cfm->whiteListEnable,
                cfm->resultCode, cfm->resultSupplier);
        }
        break;
        case CSR_BT_CM_SM_BONDING_REQ:
        {
            CsrBtCmSmBondingReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_BONDING_REQ  \
               addresss lap %x uap %x nap %x \n",
               req->deviceAddr.lap, req->deviceAddr.uap, req->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SM_BONDING_CFM:
        {
            CsrBtCmSmBondingCfm*cfm= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_BONDING_CFM  \
               Handle = %x flags %x Status %x Addr type %x lap %x uap %x nap %x\n",
               cfm->phandle, cfm->flags, cfm->status, cfm->addrt.type,
               cfm->addrt.addr.lap, cfm->addrt.addr.lap, cfm->addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_BONDING_CANCEL_REQ:
        {
              CsrBtCmSmBondingCancelReq*req= prim;
              CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_BONDING_CANCEL_REQ  \
                 addrType = %x lap %x uap %x nap %x force %x \n",
                 req->addressType,req->deviceAddr.lap,req->deviceAddr.uap,
                 req->deviceAddr.nap, req->force);
        }
        break;
        case CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_REQ:
        {
            CsrBtCmExtAdvRegisterAppAdvSetReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_REQ Handle %x advHandle %x flags %x\n",
                req->appHandle, req->advHandle, req->flags);
        }
        break;

        case CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_CFM:
        {
            CsrBtCmExtAdvRegisterAppAdvSetCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_CFM result %x \n", cfm->resultCode);
        }
        break;

        case CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ:
        {
            CsrBtCmExtAdvUnregisterAppAdvSetReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ Handle %x advHandle %x \n",
                req->appHandle, req->advHandle);
        }
        break;

        case CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM:
        {
            CsrBtCmExtAdvUnregisterAppAdvSetCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM result %x \n", cfm->resultCode);
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_PARAMS_REQ:
        {
            CsrBtCmExtAdvSetParamsReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_SET_PARAMS_REQ Handle %x advHandle %x \
                advEventProperties %x primaryAdvIntervalMin %x primaryAdvIntervalMax %x \
                primaryAdvChannelMap %x  advFilterPolicy %x primaryAdvPhy %x ",
                req->appHandle,req->advHandle, req->advEventProperties,
                req->primaryAdvIntervalMin, req->primaryAdvIntervalMax,
                req->primaryAdvChannelMap, req->advFilterPolicy,req->primaryAdvPhy);

            CONSTRUCT_HIGH_MSG("secondaryAdvMaxSkip %x secondaryAdvPhy %x \
                advSid %x ownAddrType %x PeerAddress (type %x lap %x uap %x nap %x)\n",
                req->secondaryAdvMaxSkip, req->secondaryAdvPhy,
                req->advSid, req->ownAddrType, req->peerAddr.type, req->peerAddr.addr.lap,
                req->peerAddr.addr.uap, req->peerAddr.addr.nap);
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_PARAMS_CFM:
        {
            CsrBtCmExtAdvSetParamsCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM result %x advSid %x\n",
                cfm->resultCode, cfm->advSid);
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_DATA_REQ:
        {
            CsrBtCmExtAdvSetDataReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_SET_DATA_REQ Handle %x advHandle %x \
                operation %x fragPreference %x dataLength %x \n",
                req->appHandle, req->advHandle, req->operation,
                req->fragPreference, req->dataLength);
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_DATA_CFM:
        {
            CsrBtCmExtAdvSetDataCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_SET_DATA_CFM resultCode %x \n",
               cfm->resultCode);
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_REQ:
        {
            CsrBtCmExtAdvSetScanRespDataReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_REQ Handle %x advHandle %x \
                operation %x fragPreference %x dataLength %x \n",
                req->appHandle, req->advHandle, req->operation,
                req->fragPreference, req->dataLength);
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM:
        {
            CsrBtCmExtAdvSetScanRespDataCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM resultCode %x \n",
               cfm->resultCode);
        }
        break;

        case CSR_BT_CM_EXT_ADV_ENABLE_REQ:
        {
            CsrBtCmExtAdvEnableReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_ENABLE_REQ Handle %x advHandle %x \
                enable %x \n", req->appHandle, req->advHandle, req->enable);
        }
        break;

        case CSR_BT_CM_EXT_ADV_ENABLE_CFM:
        {
            CsrBtCmExtAdvEnableCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_ENABLE_CFM resultCode %x \n",
               cfm->resultCode);
        }
        break;

        case CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ:
        {
            CsrBtCmExtAdvReadMaxAdvDataLenReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ Handle %x \
                advHandle %x\n", req->appHandle, req->advHandle);
        }
        break;

        case CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM:
        {
            CsrBtCmExtAdvReadMaxAdvDataLenCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM \
            resultCode %x maxAdvData %x maxScanrespData %x\n",cfm->resultCode,
            cfm->maxAdvData,
            cfm->maxScanRespData);
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_REQ:
        {
            CsrBtCmExtAdvSetRandomAddrReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_REQ Handle %x \
                advHandle %x action %x randomAddr (lap %x uap %x nap %x)\n", req->appHandle,
                req->advHandle, req->action, req->randomAddr.lap, req->randomAddr.uap,
                req->randomAddr.nap );
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM:
        {
            CsrBtCmExtAdvSetRandomAddrCfm*req = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM result %x \
                advHandle %x randomAddr (lap %x uap %x nap %x)\n", req->resultCode, req->advHandle, req->randomAddr.lap,
                req->randomAddr.uap, req->randomAddr.nap );
        }
        break;

        case CSR_BT_CM_EXT_ADV_SETS_INFO_REQ:
        {
            CsrBtCmExtAdvSetsInfoReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_SETS_INFO_REQ Handle %x \n",
                req->appHandle);
        }
        break;
        case CSR_BT_CM_EXT_ADV_SETS_INFO_CFM:
        {
            CsrBtCmExtAdvSetsInfoCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_SETS_INFO_CFM flags %x \
                numAdvSets %x \n", cfm->flags, cfm->numAdvSets);
        }
        break;

        case CSR_BT_CM_EXT_ADV_MULTI_ENABLE_REQ:
        {
            CsrBtCmExtAdvMultiEnableReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_ADV_SETS_INFO_REQ Handle %x \
            enable %x numSets %x\n", req->appHandle, req->enable, req->numSets);
            if(req->numSets > 0)
            {
                for(uint8 index = 0; index < req->numSets; ++index)
                {
                CONSTRUCT_HIGH_MSG("advHandle %x maxEaEvents %x duration %x\n",
                    req->config[index].advHandle, req->config[index].maxEaEvents,
                    req->config[index].duration);
                }
            }
        }
        break;

        case CSR_BT_CM_EXT_ADV_MULTI_ENABLE_CFM:
        {
            CsrBtCmExtAdvMultiEnableCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_ADV_MULTI_ENABLE_CFM result %x \
                maxAdvSets %x advBits %x\n", cfm->resultCode, cfm->maxAdvSets, cfm->advBits);
        }
        break;

        case CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_REQ:
        {
            CsrBtCmExtScanGetGlobalParamsReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_REQ Handle %x\n",
                req->appHandle);
        }
        break;

        case CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_CFM:
        {
            CsrBtCmExtScanGetGlobalParamsCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_CFM resultCode %x \
                flags %x ownAddressType %x scanFilterPolicy %x scanPhys %x filterDup %x\n",
                cfm->resultCode, cfm->flags, cfm->own_address_type,
                cfm->scanning_filter_policy, cfm->scanning_phys, cfm->filter_duplicates);

            if(cfm->scanning_phys & CM_EXT_SCAN_LE_1M_PHY_BIT_MASK)
            {
                CONSTRUCT_HIGH_MSG("scan type %x window %x interval %x\n",
                cfm->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_type,
                cfm->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_window,
                cfm->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_interval);
            }
            if(cfm->scanning_phys & CM_EXT_SCAN_LE_2M_PHY_BIT_MASK)
            {
                CONSTRUCT_HIGH_MSG("scan type %x window %x interval %x\n",
                    cfm->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_type,
                cfm->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_window,
                    cfm->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_interval);
            }
        }
        break;

        case CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_REQ:
        {
            CsrBtCmExtScanSetGlobalParamsReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_REQ Handle %x, \
                filter_duplicates %x flags %x own_address_type %x scanning_filter_policy %x \
                scanning_phy %x, phys0 %x phys1 %x\n",
                req->appHandle, req->filter_duplicates, req->flags, req->own_address_type,
                req->scanning_filter_policy, req->scanning_phy);

            if(req->scanning_phy & CM_EXT_SCAN_LE_1M_PHY_BIT_MASK)
            {
                CONSTRUCT_HIGH_MSG("scan type %x window %x interval %x\n",
                req->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_type,
                req->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_window,
                req->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_interval);
            }
            if(req->scanning_phy & CM_EXT_SCAN_LE_2M_PHY_BIT_MASK)
            {
                CONSTRUCT_HIGH_MSG("scan type %x window %x interval %x\n",
                req->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_type,
                req->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_window,
                req->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_interval);
            }
        }
        break;

        case CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM:
        {
            CsrBtCmExtScanSetGlobalParamsCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM result %x \n",
                cfm->resultCode);
        }
        break;

        case CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_REQ:
        {
            CsrBtCmExtScanRegisterScannerReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_REQ Handle %x \
                flags %x adv_filter %x adv_filter_sub1 %x adv_filter_sub2 %x \
                ad_structure_filter %x ad_structure_filter_sub1 %x ", req->appHandle, req->flags, req->adv_filter,
                req->adv_filter_sub_field1, req->adv_filter_sub_field2, req->ad_structure_filter,
                req->ad_structure_filter_sub_field1);

            CONSTRUCT_HIGH_MSG("ad_structure_filter_sub2 %x reg_ad_types %x\n",
                req->ad_structure_filter_sub_field2, req->num_reg_ad_types);

            if(req->num_reg_ad_types > 0)
            {
                for (uint8 index = 0; index < req->num_reg_ad_types; index++)
                CONSTRUCT_HIGH_MSG("reg ad types %x \n",req->reg_ad_types[index]);
            }
        }
        break;

        case CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_CFM:
        {
            CsrBtCmExtScanRegisterScannerCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_CFM result %x \
                scanHandle %x\n", cfm->resultCode, cfm->scan_handle);
        }
        break;

        case CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_REQ:
        {
            CsrBtCmExtScanConfigureScannerReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_REQ Handle %x \
                scanHandle %x useGlobalParam %x scanningPhys %x\n", req->appHandle,
                req->scan_handle, req->use_only_global_params, req->scanning_phys);

            if(req->scanning_phys & CM_EXT_SCAN_LE_1M_PHY_BIT_MASK)
            {
                CONSTRUCT_HIGH_MSG("scan type %x window %x interval %x\n",
                 req->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_type,
                 req->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_window,
                 req->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_interval);
            }
            if(req->scanning_phys & CM_EXT_SCAN_LE_2M_PHY_BIT_MASK)
            {
             CONSTRUCT_HIGH_MSG("scan type %x window %x interval %x\n",
                 req->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_type,
                 req->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_window,
                 req->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_interval);
            }
        }
        break;

        case CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_CFM:
        {
             CsrBtCmExtScanConfigureScannerCfm*cfm = prim;

             CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_CFM result %x \n",
                cfm->resultCode);
        }
        break;

        case CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_REQ:
        {
            CsrBtCmExtScanEnableScannersReq*req = prim;

            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_REQ result %x \n",
                req->appHandle, req->enable, req->num_of_scanners);
            if(req->num_of_scanners > 0)
            {
                for (uint8 index = 0; index < req->num_of_scanners; index++)
                   CONSTRUCT_HIGH_MSG("scanHandle %x duration %x\n",req->scanners[index].scan_handle,
                   req->scanners[index].duration);
            }
        }
        break;

        case CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_CFM:
        {
            CsrBtCmExtScanEnableScannersCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_CFM result %x \n",
               cfm->resultCode);
        }
        break;

        case CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ:
        {
            CsrBtCmExtScanGetCtrlScanInfoReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ Handle %x \n",
             req->appHandle);
        }
        break;

        case CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM:
        {
            CsrBtCmExtScanGetCtrlScanInfoCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM result %x \
            duration %x numEnabledScanners %x scanningPhys %x \n",
            cfm->resultCode, cfm->duration, cfm->num_of_enabled_scanners, cfm->scanning_phys );

            if(cfm->scanning_phys & CM_EXT_SCAN_LE_1M_PHY_BIT_MASK)
            {
                 CONSTRUCT_HIGH_MSG("scan type %x window %x interval %x\n",
                 cfm->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_type,
                 cfm->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_window,
                 cfm->phys[CM_EXT_SCAN_LE_1M_PHY_BIT].scan_interval);
            }
            if(cfm->scanning_phys & CM_EXT_SCAN_LE_2M_PHY_BIT_MASK)
            {
                 CONSTRUCT_HIGH_MSG("scan type %x window %x interval %x\n",
                 cfm->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_type,
                 cfm->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_window,
                 cfm->phys[CM_EXT_SCAN_LE_2M_PHY_BIT].scan_interval);
            }
        }
        break;
#endif
        case CSR_BT_CM_LE_RECEIVER_TEST_REQ:
        {
            CsrBtCmLeReceiverTestReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_RECEIVER_TEST_REQ \
Handle %x RxFreq = %x\n",
                req->appHandle, req->rxFrequency);
        }
        break;

        case CSR_BT_CM_LE_RECEIVER_TEST_CFM:
        {
            CsrBtCmLeReceiverTestCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_RECEIVER_TEST_CFM \
Result %x Supplier %x \n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_LE_TRANSMITTER_TEST_REQ:
        {
            CsrBtCmLeTransmitterTestReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_TRANSMITTER_TEST_REQ \
Handle %x TxFreq = %x TestDataLen %x\n",
                req->appHandle, req->txFrequency, req->lengthOfTestData);
        }
        break;

        case CSR_BT_CM_LE_TRANSMITTER_TEST_CFM:
        {
            CsrBtCmLeTransmitterTestCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_TRANSMITTER_TEST_CFM Result %x Supplier %x \n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_LE_TEST_END_REQ:
        {
            CsrBtCmLeTestEndReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_TEST_END_REQ Handle %x \n",req->appHandle);
        }
        break;

        case CSR_BT_CM_LE_TEST_END_CFM:
        {
            CsrBtCmLeTestEndCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_TEST_END_CFM Result %x Supplier %x \
NumPackets %x \n", cfm->resultCode, cfm->resultSupplier, cfm->numberOfPackets);
        }
        break;

        case CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_REQ:
        {
            CsrBtCmLeSetOwnAddressTypeReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_REQ \
Handle %x ownAddressType %x\n",
            req->appHandle, req->ownAddressType);
        }
        break;

        case CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_CFM:
        {
            CsrBtCmLeSetOwnAddressTypeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_CFM \
Result %x Supplier %x \n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_REQ:
        {
            CsrBtCmLeSetPvtAddrTimeoutReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_REQ Handle %x Timeout %x\n",
                req->appHandle, req->timeout);
        }
        break;

        case CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_CFM:
        {
            CsrBtCmLeSetPvtAddrTimeoutCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_CFM Result %x Supplier %x \n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_LE_SET_STATIC_ADDRESS_REQ:
        {
            CsrBtCmLeSetStaticAddressReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_SET_STATIC_ADDRESS_REQ \
Handle %x Addr::lap %x uap %x nap %x\n",
                req->appHandle, req->staticAddress.lap, req->staticAddress.uap,
                req->staticAddress.nap);
        }
        break;

        case CSR_BT_CM_LE_SET_STATIC_ADDRESS_CFM:
        {
            CsrBtCmLeSetStaticAddressCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_SET_STATIC_ADDRESS_CFM \
Result %x Supplier %x \n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_LE_READ_RANDOM_ADDRESS_REQ:
        {
            CsrBtCmLeReadRandomAddressReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_READ_RANDOM_ADDRESS_REQ \
Handle %x Flag %x\n",   req->appHandle, req->flag);
            CONSTRUCT_HIGH_MSG("Addr::type %x lap %x uap %x nap %x \n",
                req->idAddress.type, req->idAddress.addr.lap,
                req->idAddress.addr.uap, req->idAddress.addr.nap);

        }
        break;

        case CSR_BT_CM_LE_READ_RANDOM_ADDRESS_CFM:
        {
            CsrBtCmLeReadRandomAddressCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_READ_RANDOM_ADDRESS_CFM \
Result %x Supplier %x Flag %x ",
                cfm->resultCode, cfm->resultSupplier, cfm->flag);

            CONSTRUCT_HIGH_MSG("idAdress (Type %x  lap %x uap %x nap %x) ",
                cfm->idAddress.type, cfm->idAddress.addr.lap,
                cfm->idAddress.addr.uap, cfm->idAddress.addr.nap);

            CONSTRUCT_HIGH_MSG("rpa (lap %x uap %x nap %x)\n",
                 cfm->rpa.lap, cfm->rpa.uap, cfm->rpa.nap);
        }
        break;

        case CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_REQ:
        {
            CsrBtCmLeSetDataRelatedAddressChangesReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_REQ \
Handle %x AdvHandle %x Flags %x ChangeReasons %x \n",
                req->appHandle, req->advHandle, req->flags, req->changeReasons);
            }
        break;

        case CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_CFM:
        {
            CsrBtCmLeSetDataRelatedAddressChangesCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_CFM \
Result %x Supplier %x \n", cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_REQ:
        {
            CsrBtCmLeSetDefaultSubrateReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_REQ \
Handle %x Subrate (Min %x Max %x) MaxLatency %x ",
                req->appHandle, req->subrate_min,
                req->subrate_max, req->max_latency);
            CONSTRUCT_HIGH_MSG("ContNum %x STO %x\n",
                req->continuation_num, req->supervision_timeout);
        }
        break;

        case CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_CFM:
        {
            CsrBtCmLeSetDefaultSubrateCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_CFM Result %x n", cfm->resultCode);
        }
        break;

        case CSR_BT_CM_LE_SUBRATE_CHANGE_REQ:
        {
            CsrBtCmLeSubrateChangeReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_SUBRATE_CHANGE_REQ Handle %x \
Subrate (Min %x Max %x) ", req->appHandle, req->address.type,
                req->subrate_min, req->subrate_max);
            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x) ", req->address.type,
                req->address.addr.lap, req->address.addr.uap, req->address.addr.nap);

            CONSTRUCT_HIGH_MSG("MaxLatency %x ContNum %x STO %x\n",
                req->max_latency, req->continuation_num, req->supervision_timeout);
        }
        break;

        case CSR_BT_CM_LE_SUBRATE_CHANGE_CFM:
        {
            CsrBtCmLeSubrateChangeCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_SUBRATE_CHANGE_CFM Result %x ", cfm->resultCode);

            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x)\n",cfm->address.type,
                    cfm->address.addr.lap, cfm->address.addr.uap, cfm->address.addr.nap);
        }
        break;

        case CSR_BT_CM_DM_POWER_SETTINGS_REQ:
        {
            CsrBtCmDmPowerSettingsReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_DM_POWER_SETTINGS_REQ \n");
        }
        break;

        case CSR_BT_CM_ACL_OPEN_REQ:
        {
            CsrBtCmAclOpenReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_ACL_OPEN_REQ Handle %x Flags %x ",
                req->appHandle,  req->flags);
            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x)\n",
                req->address.type, req->address.addr.lap, req->address.addr.uap,
                req->address.addr.nap);
        }
        break;

        case CSR_BT_CM_ACL_OPEN_CFM:
        {
            CsrBtCmAclOpenCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_ACL_OPEN_CFM \
Address (type %x lap %x uap %x nap %x) ", cfm->deviceAddr.type, cfm->deviceAddr.addr.lap, cfm->deviceAddr.addr.uap,
                cfm->deviceAddr.addr.nap);
            CONSTRUCT_HIGH_MSG("Status %x \n",cfm->status);
        }
        break;

        case CSR_BT_CM_ACL_CONNECT_IND:
        {
            CsrBtCmAclConnectInd *ind = prim;

            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_ACL_CONNECT_IND Address(lap %x uap %x nap %x) ",
                ind->deviceAddr.lap, ind->deviceAddr.uap, ind->deviceAddr.nap);

            CONSTRUCT_HIGH_MSG("Result %x Supplier %x Incoming %x Cod = %x \n",
                ind->resultCode, ind->resultSupplier, ind->incoming, ind->cod);
        }
        break;

        case CSR_BT_CM_ACL_DISCONNECT_IND:
        {
            CsrBtCmAclDisconnectInd *ind = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_ACL_DISCONNECT_IND Result %x Supplier %x ",
                ind->reasonCode, ind->reasonSupplier);
            CONSTRUCT_HIGH_MSG("Address (lap %x uap %x nap %x)\n",
                ind->deviceAddr.lap, ind->deviceAddr.uap, ind->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_LE_ADVERTISE_REQ:
        {
            CsrBtCmLeAdvertiseReq *req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_ADVERTISE_REQ Handle %x Context %x Mode %x \
ParamChange %x ", req->appHandle, req->context, req->mode);

               CONSTRUCT_HIGH_MSG("AdvDataLen %x ScanRspDataLen %x AdvIntMin %x AdvIntMax %x, ",
                req->advertisingDataLength,req->scanResponseDataLength,
                req->advIntervalMin, req->advIntervalMax);        

            CONSTRUCT_HIGH_MSG("AdvType %x AdvChannelMap %x AdvFilterPolicy %x \
WhitelistAddrCount %x", req->advertisingType,req->advertisingChannelMap, req->advertisingFilterPolicy,
                req->whitelistAddrCount);

            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x)\n",
                    req->address.type, req->address.addr.lap,
                    req->address.addr.uap, req->address.addr.nap);
        }
        break;

        case CSR_BT_CM_LE_ADVERTISE_CFM:
        {
            CsrBtCmLeAdvertiseCfm *cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_ADVERTISE_CFM Handle %x Context %x Mode %x \
whiteListEnable %x ", cfm->context, cfm->advMode, cfm->whiteListEnable);
            CONSTRUCT_HIGH_MSG("Result %x, Supplier %x \n",
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_LE_CONNPARAM_REQ:
        {
            CsrBtCmLeConnparamReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_CONNPARAM_REQ Handle %x ConnIntMax %x \
ConnIntMin %x ", req->appHandle, req->connIntervalMax, req->connIntervalMin);
            CONSTRUCT_HIGH_MSG("ConnLatency %x ConnLatencyMax %x ScanInterval %x ScanWindow %x ",
                req->connLatency, req->connLatencyMax,
                req->scanInterval, req->scanWindow);
            CONSTRUCT_HIGH_MSG("STO %x STOMax %x STOMin %x \n",
                req->supervisionTimeout, req->supervisionTimeoutMax,
                req->supervisionTimeoutMin);
        }
        break;

        case CSR_BT_CM_LE_CONNPARAM_CFM:
        {
             CsrBtCmLeConnparamCfm*cfm = prim;
             CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_CONNPARAM_CFM Result %x Supplier %x \n",
                 cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_LE_CONNPARAM_UPDATE_REQ:
        {
            CsrBtCmLeConnparamUpdateReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_CONNPARAM_REQ Handle %x ConnIntMax %x \
    ConnIntMin %x ", req->appHandle, req->connIntervalMax, req->connIntervalMin);
            CONSTRUCT_HIGH_MSG("ConnLatency %x SupervisionTO %x\n",
                req->connLatency, req->supervisionTimeout);
        }
        break;

        case CSR_BT_CM_LE_CONNPARAM_UPDATE_CMP_IND:
        {
             CsrBtCmLeConnparamUpdateCmpInd*ind = prim;
             CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_CONNPARAM_UPDATE_CMP_IND Result %x Supplier %x ", ind->resultCode, ind->resultSupplier);
             CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x)\n",
                ind->address.type, ind->address.addr.lap, ind->address.addr.uap,
                ind->address.addr.nap);
        }
        break;

        case CSR_BT_CM_LE_ACCEPT_CONNPARAM_UPDATE_RES:
        {
            CsrBtCmLeAcceptConnparamUpdateRes*res = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_ACCEPT_CONNPARAM_UPDATE_RES l2caSignalId %x\
    connIntervalMin %x connIntervalMax %x ",
                res->l2caSignalId, res->connIntervalMin, res->connIntervalMax);
            CONSTRUCT_HIGH_MSG("connLatency %x supervisionTimeout %x accept %x \n",
                res->connLatency, res->supervisionTimeout, res->accept);
        }
        break;

        case CSR_BT_CM_LE_WHITELIST_SET_REQ:
        {
            CsrBtCmLeWhitelistSetReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_LE_WHITELIST_SET_REQ Handle %x addressCount %x \n",
                req->appHandle, req->addressCount);
        }
        break;

        case CSR_BT_CM_LE_WHITELIST_SET_CFM:
        {
            CsrBtCmLeWhitelistSetCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_LE_WHITELIST_SET_CFM result %x supplier %x \n",
            cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_REQ:
        {
            CsrBtCmGetAdvScanCapabilitiesReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_REQ Handle %x \n",
                req->appHandle);
        }
        break;

        case CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_CFM:
        {
            CsrBtCmGetAdvScanCapabilitiesCfm*cfm = prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_CFM result %x availableApi %x \
    availableAdvSets %x stackReservedAdvSets %x ",
                cfm->resultCode, cfm->availableApi, cfm->availableAdvSets, cfm->stackReservedAdvSets);
            CONSTRUCT_HIGH_MSG("maxPeriodicSyncListSize %x supportedPhys %x maxPotentialSizeOfTxAdvData %x \
    maxPotentialSizeOfTxPeriodicAdvData %x ",
               cfm->maxPeriodicSyncListSize, cfm->supportedPhys, 
               cfm->maxPotentialSizeOfTxAdvData,cfm->maxPotentialSizeOfTxPeriodicAdvData);
            CONSTRUCT_HIGH_MSG("maxPotentialSizeOfRxAdvData %x maxPotentialSizeOfRxPeriodicAdvData %x \n",
                cfm->maxPotentialSizeOfRxAdvData,cfm->maxPotentialSizeOfRxPeriodicAdvData);
        }
        break;

        case CM_L2CA_TP_CONNECT_REQ:
        {
            CmL2caTpConnectReq*req = prim;
            CONSTRUCT_HIGH_MSG("==> CM_L2CA_TP_CONNECT_REQ Handle %x localPsm %x remotePsm %x \
    secLevel %x ", req->phandle, req->localPsm, req->remotePsm, req->secLevel);

            CONSTRUCT_HIGH_MSG("context %x minEncKeySize %x confTabCount transport %x ", 
                req->context, req->minEncKeySize, req->conftabCount, req->tpdAddrT.tp_type);

            CONSTRUCT_HIGH_MSG("Address(Type %x lap %x uap %x nap %x) \n",
                req->tpdAddrT.addrt.type, req->tpdAddrT.addrt.addr.lap,
                req->tpdAddrT.addrt.addr.uap, req->tpdAddrT.addrt.addr.nap);
        }
        break;

        case CM_L2CA_TP_CONNECT_CFM:
        {
            CsrBtCmL2caTpConnectCfm*cfm= prim;

            CONSTRUCT_HIGH_MSG("<== CM_L2CA_TP_CONNECT_CFM Handle %x \
connId %x localPsm %x Mtu %x ", cfm->btConnId, cfm->localPsm, cfm->mtu, cfm->localMtu);

            CONSTRUCT_HIGH_MSG("result %x supplier %x context %x transport %x ",
                cfm->resultCode, cfm->resultSupplier, cfm->context, cfm->tpdAddrT.tp_type);

            CONSTRUCT_HIGH_MSG("Address (Type %x lap %x uap %x nap %x) \n",
                  cfm->tpdAddrT.addrt.type, cfm->tpdAddrT.addrt.addr.lap, 
                  cfm->tpdAddrT.addrt.addr.uap, cfm->tpdAddrT.addrt.addr.nap);
        }
        break;

        case CM_L2CA_ADD_CREDIT_REQ:
        {
            CmL2caAddCreditReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CM_L2CA_ADD_CREDIT_REQ Handle %x \
connId %x context %x credits %x\n",
                req->phandle, req->btConnId, req->context, req->credits);
        }
        break;

        case CM_L2CA_ADD_CREDIT_CFM:
        {
            CmL2caAddCreditCfm*cfm= prim;
            CONSTRUCT_HIGH_MSG("<== CM_L2CA_ADD_CREDIT_CFM Handle %x \
connId %x context %x credits %x result %x supplier %x \n",
                cfm->btConnId, cfm->context, cfm->credits,
                cfm->resultCode, cfm->resultSupplier);
        }
        break;

        case CM_L2CA_TP_CONNECT_ACCEPT_RSP:
        {
            CmL2caTpConnectAcceptRsp*rsp= prim;
            CONSTRUCT_HIGH_MSG("==> CM_L2CA_TP_CONNECT_ACCEPT_RSP Handle %x \
accept %x connId %x localPsm %x ", rsp->phandle, rsp->accept, rsp->btConnId, rsp->localPsm);

            CONSTRUCT_HIGH_MSG("identifier %x confTabCount %x minEncKeySize %x transport %x ",
                rsp->identifier, rsp->conftabCount, rsp->minEncKeySize, rsp->tpdAddrT.tp_type);

            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x) \n", 
                rsp->tpdAddrT.addrt.type, rsp->tpdAddrT.addrt.addr.lap,
                rsp->tpdAddrT.addrt.addr.uap, rsp->tpdAddrT.addrt.addr.nap);
        }
        break;

        case CM_L2CA_TP_CONNECT_ACCEPT_IND:
        {
            CmL2caTpConnectAcceptInd*ind= prim;
            CONSTRUCT_HIGH_MSG("<== CM_L2CA_TP_CONNECT_ACCEPT_IND context %x \
connId %x localPsm %x identifier %x ", ind->context, ind->btConnId,ind->localPsm,
                ind->identifier);
            CONSTRUCT_HIGH_MSG("localControl %x mtu %x localMtu %x credits %x ", 
                ind->localControl, ind->mtu, ind->localMtu, ind->credits);        

            CONSTRUCT_HIGH_MSG("dataPriority %x transport %x flags %x ", 
                ind->dataPriority, ind->tpdAddrT.tp_type, ind->flags);
            
            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x)\n", 
                ind->tpdAddrT.addrt.type, ind->tpdAddrT.addrt.addr.lap, 
                ind->tpdAddrT.addrt.addr.uap, ind->tpdAddrT.addrt.addr.nap);
        }
        break;

        case CM_L2CA_TP_CONNECT_ACCEPT_CFM:
        {
            CmL2caTpConnectAcceptCfm*cfm= prim;
            CONSTRUCT_HIGH_MSG("<== CM_L2CA_TP_CONNECT_ACCEPT_CFM connId %x \
result %x supplier %x localPsm %x ", cfm->btConnId,  cfm->resultCode, cfm->resultSupplier,
                cfm->localPsm);
            
            CONSTRUCT_HIGH_MSG("remotePsm %x mtu %x localMtu %x transport %x ",
                cfm->remotePsm, cfm->mtu, cfm->localMtu, cfm->tpdAddrT.tp_type);

            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x ) ", cfm->tpdAddrT.addrt.type,
                       cfm->tpdAddrT.addrt.addr.lap, cfm->tpdAddrT.addrt.addr.uap,
                       cfm->tpdAddrT.addrt.addr.nap);

            CONSTRUCT_HIGH_MSG("context %x \n", cfm->context);
        }
        break;

        case CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ:
        {
            CmWriteScHostSupportOverrideReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ Handle %x \
lap %x uap %x nap %x ", req->appHandle, req->deviceAddr.lap, req->deviceAddr.uap,
                req->deviceAddr.nap);
            CONSTRUCT_HIGH_MSG("overrideAction %x\n", req->overrideAction);
        }
        break;

        case CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM:
        {
            CmWriteScHostSupportOverrideCfm*cfm= prim;
            CONSTRUCT_HIGH_MSG("<== CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM \
status %x lap %x uap %x nap %x ", cfm->status, cfm->deviceAddr.lap, cfm->deviceAddr.uap,
                cfm->deviceAddr.nap);

            CONSTRUCT_HIGH_MSG("hostSupportOverride %x\n", cfm->hostSupportOverride);
        }
        break;

        case CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ:
        {
            CmReadScHostSupportOverrideMaxBdAddrReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ Handle %x \n",
                req->appHandle);
        }
        break;

        case CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM:
        {
            CmReadScHostSupportOverrideMaxBdAddrCfm*cfm= prim;
            CONSTRUCT_HIGH_MSG("<== CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM status %x \
maxOverrideBdAddr %x\n", cfm->status, cfm->maxOverrideBdAddr);
        }
        break;

        case CM_DM_LE_SET_PHY_REQ:
        {
            CmDmLeSetPhyReq*req= prim;

            CONSTRUCT_HIGH_MSG("==> CM_DM_LE_SET_PHY_REQ Handle %x \
transport %x flags %x ",req->appHandle, req->tpAddr.tp_type, req->phyInfo.flags);

            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x) ",
                req->tpAddr.addrt.type, req->tpAddr.addrt.addr.lap, 
                req->tpAddr.addrt.addr.uap, req->tpAddr.addrt.addr.nap);

            CONSTRUCT_HIGH_MSG("maxRxRate %x maxTxRate %x minRxRate %x minTxRate %x \n",
                 req->phyInfo.maxRxRate, req->phyInfo.maxTxRate,
                 req->phyInfo.minRxRate, req->phyInfo.minTxRate);
        }
        break;

        case CM_DM_LE_SET_DEFAULT_PHY_REQ:
        {
            CmDmLeSetDefaultPhyReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CM_DM_LE_SET_DEFAULT_PHY_REQ Handle %x \
flags %x maxRxRate %x maxTxRate %x ",req->appHandle, req->phyInfo.flags,
                req->phyInfo.maxRxRate, req->phyInfo.maxTxRate);
            CONSTRUCT_HIGH_MSG("minRxRate %x minTxRate %x \n",
                req->phyInfo.minRxRate, req->phyInfo.minTxRate);
        }
        break;

        case CSR_BT_CM_SM_ADD_DEVICE_REQ:
        {
            CsrBtCmSmAddDeviceReq*req= prim;

            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_ADD_DEVICE_REQ address (type %x lap %x uap %x nap %x) ", 
                req->typedAddr.type, req->typedAddr.addr.lap,
                req->typedAddr.addr.uap, req->typedAddr.addr.nap);

            CONSTRUCT_HIGH_MSG(" trust %x ", req->trust);

            if(req->keys != NULL)
            {
                CONSTRUCT_HIGH_MSG("keysPresent %x encrKeySize %x securityReq %x \n",
                    req->keys->present, req->keys->encryption_key_size,
                    req->keys->security_requirements);
            }
            else
            {
                 CONSTRUCT_HIGH_MSG("\n");
            }
        }
        break;

        case CSR_BT_CM_SM_ADD_DEVICE_CFM:
        {
            CsrBtCmSmAddDeviceCfm*cfm= prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SM_ADD_DEVICE_CFM \
Handle %x status %x ", cfm->phandle, cfm->status);
            CONSTRUCT_HIGH_MSG("Address (type; %x lap %x uap %x nap %x) \n",
                cfm->addrt.type, cfm->addrt.addr.lap,
                cfm->addrt.addr.uap, cfm->addrt.addr.nap);        
        }
        break;

        case CSR_BT_CM_SM_REMOVE_DEVICE_REQ:
        {
            CsrBtCmSmRemoveDeviceReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_REMOVE_DEVICE_REQ  \
Address (type %x lap %x uap %x nap %x )\n", req->addressType,
                req->deviceAddr.lap, req->deviceAddr.uap,
                req->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SM_REMOVE_DEVICE_CFM:
        {
            CsrBtCmSmRemoveDeviceCfm*cfm= prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SM_REMOVE_DEVICE_CFM \
Handle %x status %x ", cfm->phandle, cfm->status);

            CONSTRUCT_HIGH_MSG("Addr (type %x lap %x uap %x nap %x) \n",
                cfm->addrt.type, cfm->addrt.addr.lap,
                cfm->addrt.addr.uap, cfm->addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_CANCEL_CONNECT_REQ:
        {
            CsrBtCmSmCancelConnectReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_CANCEL_CONNECT_REQ \
lap %x uap %x nap %x \n",req->deviceAddr.lap, req->deviceAddr.uap,
                req->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SM_ENCRYPTION_CHANGE_IND:
        {
            CsrBtCmSmEncryptionChangeInd*ind= prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SM_ENCRYPTION_CHANGE_IND Handle %x \
encrypted %x transport %x ", ind->phandle, ind->encrypted, ind->encrypt_type, ind->tp_addrt.tp_type);

            CONSTRUCT_HIGH_MSG("Addr (type %x lap %x uap %x nap %x) \n", 
                ind->tp_addrt.addrt.type, ind->tp_addrt.addrt.addr.lap,
                ind->tp_addrt.addrt.addr.uap, ind->tp_addrt.addrt.addr.nap);

        }
        break;

        case CSR_BT_CM_SM_SET_SEC_MODE_REQ:
        {
            CsrBtCmSmSetSecModeReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_SET_SEC_MODE_REQ  \
mode = %x mode3Enc %x \n", req->mode, req->mode3Enc);
        }
        break;

        case CSR_BT_CM_SM_SET_DEFAULT_SEC_LEVEL_REQ:
        {
            CsrBtCmSmSetDefaultSecLevelReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_SET_DEFAULT_SEC_LEVEL_REQ \
seclDefault = %x \n", req->seclDefault);
        }
        break;

        case CSR_BT_CM_SM_AUTHORISE_IND:
        {
            CsrBtCmSmAuthoriseInd*ind= prim;
            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SM_AUTHORISE_IND \
handle = %x channel %x protocol %x incoming %x ",
              ind->phandle, ind->cs.connection.service.channel,
              ind->cs.connection.service.protocol_id,
              ind->cs.incoming);
            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x)\n",
              ind->cs.connection.addrt.type,
              ind->cs.connection.addrt.addr.lap,
              ind->cs.connection.addrt.addr.uap,
              ind->cs.connection.addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_AUTHORISE_RES:
        {
            CsrBtCmSmAuthoriseRes*rsp= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_AUTHORISE_RES \
protocol = %x channel %x incoming %x auth %x ",
                rsp->protocolId, rsp->channel, rsp->incoming, rsp->authorisation);
            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x) \n",
                rsp->addressType, rsp->deviceAddr.lap,
                rsp->deviceAddr.uap, rsp->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SM_SECURITY_IND:
        {
            CsrBtCmSmSecurityInd*ind= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_SET_SEC_MODE_REQ \
Handle = %x status %x ConnectFlags %x Context %x ",
               ind->phandle,ind->status, ind->connection_flags,
               ind->context);
            CONSTRUCT_HIGH_MSG("SecReq %x ",ind->security_requirements);

            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x)\n",
               ind->addrt.type, ind->addrt.addr.lap, ind->addrt.addr.uap,
               ind->addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_LE_SECURITY_REQ:
        {
             CsrBtCmSmLeSecurityReq*req= prim;

             CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_LE_SECURITY_REQ \
Addr (type = %x lap %x uap %x nap %x) ",
                   req->addr.type,req->addr.addr.lap,req->addr.addr.uap,
                   req->addr.addr.nap);
             CONSTRUCT_HIGH_MSG("Context %x secRequired %x l2caFlags %x\n",
                req->context, req->securityRequirements, req->l2caConFlags);
        }
        break;

        case CSR_BT_CM_SM_IO_CAPABILITY_RESPONSE_IND:
        {
            CsrBtCmSmIoCapabilityResponseInd*ind= prim;

            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_IO_CAPABILITY_RESPONSE_IND  \
Handle %x ioCap = %x authReq %x oobDataPresent %x ", ind->phandle, ind->io_capability,
               ind->authentication_requirements, ind->oob_data_present);

            CONSTRUCT_HIGH_MSG("Flags %x keyDist %x transport %x ", 
                       ind->flags, ind->key_distribution, ind->tp_addrt.tp_type);

            CONSTRUCT_HIGH_MSG("Addr (type %x lap %x uap %x nap %x) \n",
                ind->tp_addrt.addrt.type, ind->tp_addrt.addrt.addr.lap,
                ind->tp_addrt.addrt.addr.uap, ind->tp_addrt.addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_IND:
        {
            CsrBtCmSmIoCapabilityRequestInd*ind= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_IND  \
phandle= %x dev_class %x flags %x Transport %x ",
               ind->phandle, ind->dev_class, ind->flags, ind->tp_addrt.tp_type);

            CONSTRUCT_HIGH_MSG(" Address (type %x lap %x uap %x nap %x)",
                ind->tp_addrt.addrt.type, ind->tp_addrt.addrt.addr.lap, 
                ind->tp_addrt.addrt.addr.uap, ind->tp_addrt.addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_RES:
        {
            CsrBtCmSmIoCapabilityRequestRes*rsp= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_RES  \
ioCap = %x authReq %x oobDataPresent %x keyDist %x ",
               rsp->ioCapability, rsp->authenticationRequirements,rsp->oobDataPresent,
               rsp->keyDistribution);

            CONSTRUCT_HIGH_MSG("Addr (type %x lap %x uap %x nap %x) ",
               rsp->transportType, rsp->addressType,rsp->deviceAddr.lap,
               rsp->deviceAddr.uap, rsp->deviceAddr.nap);

            CONSTRUCT_HIGH_MSG("Transport %x \n", rsp->transportType);
        }
        break;

        case CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_NEG_RES:
        {
            CsrBtCmSmIoCapabilityRequestNegRes*rsp= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_NEG_RES reason %x \
transport  %x ", rsp->reason, rsp->transportType);

            CONSTRUCT_HIGH_MSG("Addr (type %x lap %x uap %x nap %x) \n",
               rsp->addressType, rsp->deviceAddr.lap,
               rsp->deviceAddr.uap, rsp->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_RES:
        {
            CsrBtCmSmUserConfirmationRequestRes*rsp= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_RES reason %x \
transport  %x ", rsp->reason, rsp->transportType);

            CONSTRUCT_HIGH_MSG("Addr (type %x lap %x uap %x nap %x) \n",
                rsp->addressType,rsp->deviceAddr.lap,
                rsp->deviceAddr.uap, rsp->deviceAddr.nap);
        }
            break;
            case CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_NEG_RES:
        {
            CsrBtCmSmUserConfirmationRequestNegRes*rsp= prim;

            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_NEG_RES reason %x \
transport  %x ", rsp->reason, rsp->transportType);
            CONSTRUCT_HIGH_MSG("Addr (type %x lap %x uap %x nap %x) \n",
                rsp->addressType,rsp->deviceAddr.lap,
                rsp->deviceAddr.uap, rsp->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_IND:
        {
            CsrBtCmSmUserConfirmationRequestInd*ind= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_IND Handle %x \
dev_class  %x numeric_value %x io_cap_remote %x ",
              ind->phandle, ind->dev_class, ind->numeric_value,ind->io_cap_remote);

            CONSTRUCT_HIGH_MSG("io_cap_local %x flags %x Transport %x ",
              ind->io_cap_local, ind->flags, ind->tp_addrt.tp_type);

            CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x)\n",
              ind->tp_addrt.addrt.type,ind->tp_addrt.addrt.addr.lap, 
              ind->tp_addrt.addrt.addr.uap, ind->tp_addrt.addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_USER_PASSKEY_REQUEST_RES:
        {
            CsrBtCmSmUserPasskeyRequestRes*rsp= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_USER_PASSKEY_REQUEST_RES transport  %x \
numvalue = %x ", rsp->transportType, rsp->numericValue);
            CONSTRUCT_HIGH_MSG("Addr(type %x lap %x uap %x nap %x)\n",
                rsp->addressType,rsp->deviceAddr.lap,
                rsp->deviceAddr.uap, rsp->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SM_USER_PASSKEY_REQUEST_NEG_RES:
        {
            CsrBtCmSmUserPasskeyRequestNegRes*rsp= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_USER_PASSKEY_REQUEST_NEG_RES transport  %x \
numvalue = %x\n", rsp->transportType, rsp->numericValue);

            CONSTRUCT_HIGH_MSG("Addr (type %x lap %x uap %x nap %x) \n",
                rsp->addressType,rsp->deviceAddr.lap,
                rsp->deviceAddr.uap, rsp->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SM_USER_PASSKEY_REQUEST_IND:
        {
            CsrBtCmSmUserPasskeyRequestInd*ind= prim;

            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SM_USER_PASSKEY_REQUEST_IND \
Handle %x devClass %x Flags %x Transport %x ", ind->phandle , ind->dev_class, 
                    ind->flags, ind->tp_addrt.tp_type);

            CONSTRUCT_HIGH_MSG("Addr:: (type %x lap %x uap %x nap %x) \n",
                ind->tp_addrt.addrt.type,ind->tp_addrt.addrt.addr.lap,
                ind->tp_addrt.addrt.addr.uap, ind->tp_addrt.addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_USER_PASSKEY_NOTIFICATION_IND:
        {
            CsrBtCmSmUserPasskeyNotificationInd*ind = prim;

            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_USER_PASSKEY_NOTIFICATION_IND \
Handle %x devClass %x Flags %x Transport %x ",
                ind->phandle , ind->dev_class, ind->flags, ind->tp_addrt.tp_type);
            
            CONSTRUCT_HIGH_MSG("Addr:: (type %x lap %x uap %x nap %x)\n",
                ind->tp_addrt.addrt.type,ind->tp_addrt.addrt.addr.lap,
                ind->tp_addrt.addrt.addr.uap, ind->tp_addrt.addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_REQ:
        {
            CsrBtCmSmReadLocalOobDataReq*req= prim;
            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_REQ  \
Transport = %x \n", req->transportType);
        }
        break;

        case CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_CFM:
        {
            CsrBtCmSmReadLocalOobDataCfm*cfm= prim;

            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_CFM  \
Handle = %x Transport %x Status %x OobPresent %x \n",
               cfm->phandle, cfm->tp_type,
               cfm->status, cfm->oob_data_present);

            if(cfm->oob_data_present)
            {
                /*DMSG_HEXDUMP(SIZE_OOB_DATA* 2, cfm->oob_hash_c);
                DMSG_HEXDUMP(SIZE_OOB_DATA* 2, cfm->oob_rand_r);*/
            }
        }
        break;

        case CSR_BT_CM_SM_SEND_KEYPRESS_NOTIFICATION_REQ:
        {
            CsrBtCmSmSendKeypressNotificationReq*req= prim;

            CONSTRUCT_HIGH_MSG("==> CSR_BT_CM_SM_SEND_KEYPRESS_NOTIFICATION_REQ  \
Transport %x NotiType %x ", req->transportType, req->notificationType);

            CONSTRUCT_HIGH_MSG("Addr (type %x lap %x uap %x nap %x)\n",
               req->addressType,req->deviceAddr.lap,
               req->deviceAddr.uap, req->deviceAddr.nap);
        }
        break;

        case CSR_BT_CM_SM_KEYPRESS_NOTIFICATION_IND:
        {
              CsrBtCmSmKeypressNotificationInd*req= prim;

              CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SM_KEYPRESS_NOTIFICATION_IND  \
handle = %x notiType %x flags %x transport %x ",
                  req->phandle,req->notification_type, req->flags,
                  req->tp_addrt.tp_type);

              CONSTRUCT_HIGH_MSG("Address (type %x lap %x uap %x nap %x)\n",
                req->tp_addrt.addrt.type, req->tp_addrt.addrt.addr.lap, 
                req->tp_addrt.addrt.addr.uap, req->tp_addrt.addrt.addr.nap);
        }
        break;

        case CSR_BT_CM_SM_PIN_REQUEST_IND:
        {
            CsrBtCmSmPinRequestInd*ind= prim;

            CONSTRUCT_HIGH_MSG("<== CSR_BT_CM_SM_PIN_REQUEST_IND \
handle = %x devClass %x initiator %x ",
                 ind->phandle, ind->dev_class, ind->initiator);
            CONSTRUCT_HIGH_MSG("Address type %x lap %x uap %x nap %x\n",
                 ind->addrt.type, ind->addrt.addr.lap,
                 ind->addrt.addr.uap, ind->addrt.addr.nap);
        }
        break;

        default:
            CONSTRUCT_HIGH_MSG("<== CM UNKNOWN MSG TYPE %x \n", *type);
        break;
    }
}

void SynLogPrim(CsrUint16 mi, void *prim)
{
    switch(mi)
    {
        case CSR_BT_CM_PRIM:
        {
            synLogPrimCm(prim);
        }
        break;
        case CSR_BT_GATT_PRIM:
        {
            synLogPrimGatt(prim);
        }
        break;

#ifndef EXCLUDE_CSR_BT_HF_MODULE
        case CSR_BT_HF_PRIM:
        {
            synLogPrimHf(prim);
        }
        break;
#endif /* EXCLUDE_CSR_BT_HF_MODULE */

#ifndef EXCLUDE_CSR_BT_HFG_MODULE
        case CSR_BT_HFG_PRIM:
        {
            synLogPrimHfg(prim);
        }
#endif /* EXCLUDE_CSR_BT_HFG_MODULE */
        break;

#ifndef EXCLUDE_CSR_BT_AV_MODULE
        case CSR_BT_AV_PRIM:
        {
            synLogPrimAv(prim);
        }
        break;
#endif /* EXCLUDE_CSR_BT_AV_MODULE */

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE
        case CSR_BT_AVRCP_PRIM:
        {
            synLogPrimAvrcp(prim);
        }
        break;
#endif /* EXCLUDE_CSR_BT_AVRCP_MODULE */

        default:
            break;
    }
}
#endif

void SynLogSchedMsg(CsrBool isPut, CsrSchedQid src, CsrSchedQid dst, CsrUint16 mi, void *mv)
{
    CsrPrim type = mv?(*(CsrPrim *)mv):0xffff;

    SynStoreLogInfo(isPut, type, mi, src, dst);

    if (isPut)
    {
        if ((mi == CSR_BT_GATT_PRIM) && (dst == CSR_BT_GATT_IFACEQUEUE))
        {
#ifdef SYN_LOG_EXT_PRIM
            SynLogPrim(mi, mv);
#else
            SYNERGY_LOG_DOWNSTREAM_EXT_MSG(mi, type);
#endif
        }
        else if (mi == CSR_BT_GATT_PRIM)
        {
#ifdef SYN_LOG_EXT_PRIM
            SynLogPrim(mi, mv);
#else
            SYNERGY_LOG_UPSTREAM_EXT_MSG(mi, type);
#endif
        }
        else
        {
            CONSTRUCT_MSG("Put Msg src: %x dst: %x, mi: %x, mv: %x ", src, dst, mi, type);
        }
    }
    else
    {
        CONSTRUCT_MSG("Get Msg src: %x dst: %x, mi: %x, mv: %x ", src, dst, mi, type);
    }
}

void SynLogBluestackWarning(CsrUint32 error_code, CsrUint32 line)
{
    CONSTRUCT_HIGH_MSG("Bluestack Warning: %X line: %x", error_code, line);
}
void SynLogBluestackDebout(CsrUint32 error_code, CsrUint32 line)
{
    CONSTRUCT_HIGH_MSG("Bluestack Debout: %X", error_code);
}

#endif /* CSR_LOG_ENABLE */
