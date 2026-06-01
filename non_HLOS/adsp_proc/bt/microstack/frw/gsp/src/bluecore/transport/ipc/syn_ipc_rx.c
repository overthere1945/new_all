/******************************************************************************
 Copyright (c) 2020-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "syn_ipc_rx.h"
#include "syn_ipc.h"

#include "csr_pmem.h"
#include "syn_ipc_drv.h"
#include "csr_time.h"
#include "csr_util.h"
#include "csr_transport.h"
#include "csr_tm_bluecore_transport.h"
#include "syn_ipc_internal.h"

static void synIpcRxPacketReassembly(void *context);

typedef struct {
    synIpcExtension ipcRxExt;
    synIpcExtension *pRxExt;
    SynIpcCustomRxCallback customRxCb;
    SynIpcRxInstance *synIpcRx;
} rxInstance;

rxInstance rxInst = {{NULL,0}, NULL, NULL, NULL};

#if defined (CSR_PLATFORM_WINDOWS) || defined (CSR_PLATFORM_LINUX)
#if (SYN_BT_HOST_TRANSPORT == IPC)

#include "syn_qapi_ipc_x86.h"

extern uint8_t *btHciGetBufCb(void *arg, const qapi_ipc_msg_t *ipcMsg);
extern void btHciRxMsgCb(void *arg, const qapi_ipc_msg_t * ipcMsg, const uint8 *payload);

void SynIpcSendToHciTest(SynIpcHciPktType hciPktType, CsrUint8 *data, CsrUint32 dataLen)
{
    qapi_ipc_msg_t qmsg;
    qmsg.msg_len = dataLen + 1;

    qmsg.payload = btHciGetBufCb(NULL, &qmsg);
    qmsg.payload[0] = hciPktType;
    SynMemCpyS(qmsg.payload + 1, dataLen, data, dataLen);
    btHciRxMsgCb(NULL, NULL, qmsg.payload);
}
#endif /* (SYN_BT_HOST_TRANSPORT == IPC) */
#endif /* (CSR_PLATFORM_WINDOWS) || defined (CSR_PLATFORM_LINUX) */

void SynIpcSendToHci(SynIpcHciPktType hciPktType, CsrUint8 *data, CsrUint32 dataLen)
{
    SynIpcInstance *ipcInst = SYN_IPC_INST_FROM_RX(rxInst.synIpcRx);
    MessageStructure msg;

    if (hciPktType == SYN_IPC_HCI_ACL)
    {
        msg.chan = TRANSPORT_CHANNEL_ACL;
    }
    else if (hciPktType == SYN_IPC_HCI_EVT)
    {
        msg.chan = TRANSPORT_CHANNEL_HCI;
    }
    else
    {
        return;
    }
    msg.buf = data;
    msg.buflen = dataLen;
    msg.dex = 0;

    CsrTmBlueCoreTransportMsgRx(ipcInst->transport, &msg);
}

/***********************************************************************
 Receives a full RX package from the lowest part of the IPC
 driver. At this point the package is assembled and the channel
 is decoded.
 ***********************************************************************/
static CsrBool synIpcRxReadCallback(void *context, CsrUint8 *packet, CsrUint32 size)
{
    SynIpcHciPktType hciPktType;
    SynIpcMsgHci hci;
    SynIpcResult result;
    SynIpcMsgCustom custMsg;
    CsrUint8 *pmem = packet;

    CSR_UNUSED(context);    

    if (!packet)
    {
        return FALSE;
    }

#ifdef SYN_IPC_DEBUG_ADD_RX_WATER_MARK
    pmem = pmem + 2;
    size = size - 2;
#endif

    hciPktType = pmem[0];
    if (hciPktType == SYN_IPC_HCI_PKT_CUSTOM)
    {
        if (!rxInst.customRxCb)
        {
            CSR_LOG_TEXT_CRITICAL((SynIpcLto, SYN_IPC_LTSO_RX, "!CustRxCb"));
        }

        custMsg.msgId = pmem[1];
        if (size > 2)
        {
            custMsg.data = &pmem[2];
            custMsg.dataLen = size - 2;
        }
        else
        {
            custMsg.data = NULL;
            custMsg.dataLen = 0;
        }
        rxInst.customRxCb(SYN_IPC_MSG_CUSTOM, &custMsg);
        /* Custom events will never cause locking of the buffers
            so return TRUE */
        return TRUE;
    }

    /* Extension call */
    if (rxInst.pRxExt)
    {
        if (((hciPktType == SYN_IPC_HCI_EVT) && (rxInst.pRxExt->pktTypeReq & SYN_IPC_HCI_PKT_EVT)) || 
            ((hciPktType == SYN_IPC_HCI_ACL) && (rxInst.pRxExt->pktTypeReq & SYN_IPC_HCI_PKT_ACL)))
        {
            hci.data = pmem + 1;
            hci.dataLen = size - 1;
            hci.pktType = hciPktType;
            result = rxInst.pRxExt->cb(SYN_IPC_MSG_HCI, &hci);
            if (result == SYN_IPC_RESULT_CONSUMED)
            {
                /* This is an indication to lock the buffers */
                return FALSE;
            }
            else if (result == SYN_IPC_RESULT_FTM_MODE)
            {
                return TRUE;
            }
            else
            {
                /* This packet was ignored by extension module, let 
                    this packet go to tm */
            }
        }
        else
        {
            /* Extension module is not keen on the type of packet received,
                let the message go the tm module */
        }
    }
    else
    {
        /* No extension module registered, let the message go the tm module */
    }

    /* pmem holds the hci packet type in the 1st location this needs to be skipped
        as packet type is separately handled */
    SynIpcSendToHci(hciPktType, pmem + 1, size - 1);

    return TRUE;
}

static void synIpcRxPacketReassembly(void *context)
{
    SynIpcInstance *ipcInst = context;
    CsrUint8 spacePercent;
    SynIpcMsgDriver driver;

    SynIpcDrvLowLevelTransportRx(ipcInst->ipcDrvHandle, ipcInst, synIpcRxReadCallback);
    spacePercent = SynIpcMemSpacePercent();
    if (spacePercent >= SYN_IPC_MAX_SPACE_PERCENT)
    {
        driver.id = SYN_IPC_MSG_DRIVER_RECEIVE_MEMORY_FULL;
        rxInst.customRxCb(SYN_IPC_MSG_DRIVER, &driver);
    }
}

static void SynIpcCustomInit(void)
{
    SYN_IPC_INST_FROM_RX(rxInst.synIpcRx)->ipcDrvCustomHandle = SynIpcDrvOpen(SYN_IPC_TRANSPORT_CUSTOM);
}

void SynIpcRxInit(SynIpcRxInstance *rx)
{
    CsrMemSet(rx, 0, sizeof(*rx));

    rxInst.synIpcRx = rx;
    if (rxInst.customRxCb)
    {
        SynIpcCustomInit();
    }
    rx->bgintReassemble = CsrSchedBgintReg(synIpcRxPacketReassembly,
                                           SYN_IPC_INST_FROM_RX(rx),
                                           "IPC Reassemble");
}

void SynIpcRxDeinit(SynIpcRxInstance *rx)
{
    CsrSchedBgintUnreg(rx->bgintReassemble);
    rx->bgintReassemble = CSR_SCHED_BGINT_INVALID;
    CsrMemSet(&rxInst, 0, sizeof(rxInst));
}

CsrBool SynIpcRegisterRxExtension(SynIpcRxExtensionCallback cb, SynIpcHciPktTypeReq pktTypeReq)
{
    SynIpcHciPktTypeReq mask = SYN_IPC_HCI_PKT_EVT | SYN_IPC_HCI_PKT_ACL;
    if (!cb)
    {
        return FALSE;
    }
    if (!(pktTypeReq & mask))
    {
        return FALSE;
    }
    rxInst.ipcRxExt.cb = cb;
    rxInst.ipcRxExt.pktTypeReq = pktTypeReq;

    rxInst.pRxExt = &(rxInst.ipcRxExt);

    return TRUE;
}

void SynIpcRegisterCustom(SynIpcCustomRxCallback rxCb)
{
    if (rxCb)
    {
        rxInst.customRxCb = rxCb;
    }
}

