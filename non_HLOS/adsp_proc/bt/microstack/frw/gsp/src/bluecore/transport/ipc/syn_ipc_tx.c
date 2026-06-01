/******************************************************************************
 Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "syn_ipc_tx.h"

#include "csr_pmem.h"
#include "csr_serial_com.h"
#include "csr_util.h"
#include "csr_queue_lib.h"
#include "syn_ipc.h"
#include "syn_ipc_drv.h"
#include "syn_ipc_internal.h"

typedef struct {
    synIpcExtension ipcTxExt;
    synIpcExtension *pTxExt;
    SynIpcTxInstance *synIpcTx;
} txInstance;

txInstance txInst = {{NULL, 0}, NULL, NULL};

static void synIpcTxStartTransmit(SynIpcTxInstance* tx);
static void synIpcTxCustomStartTransmit(SynIpcTxInstance* tx);
static void synIpcTxCustomBgintHandler(void *opaque);

/* Releases the first message from the queue */
static void synIpcTxReleaseMsg(TXMSG **sendQ)
{
    TXMSG* tMsg = CsrQueueMsgTake(sendQ);

    if (tMsg)
    {
        CsrPmemFree(tMsg->m.buf);
        CsrPmemFree(tMsg);
    }
}

/* Converts the transport channel into HCI channel */
static CsrUint8 synIpcTxGetHciChannel(CsrUint8 transportChannel)
{
    CsrUint8 hciChannel = SYN_IPC_HCI_CMD;

    switch (transportChannel)
    {
        case TRANSPORT_CHANNEL_ACL:
            hciChannel = SYN_IPC_HCI_ACL;
            break;
        case TRANSPORT_CHANNEL_HCI:
            hciChannel = SYN_IPC_HCI_CMD;
            break;
        default:
            CSR_LOG_TEXT_CRITICAL((SynIpcLto, SYN_IPC_LTSO_TX, "unknown channel type"));
            break;
    }

    return (hciChannel);
}

static CsrBool synIpcTxSendHciPacket(SynIpcTxInstance *tx, TXMSG *tMsg)
{
    CsrUint16 hciPktLen;
    MessageStructure *msg = &(tMsg->m);
    CsrUint8* hciPkt;
    CsrBool status;

    /* + 1 is for hci pkt type */
    hciPktLen = (CsrUint16) (msg->buflen + 1);
    hciPkt = SynIpcDrvAlloc(hciPktLen);
    if (!hciPkt)
    {
        CSR_LOG_TEXT_WARNING((SynIpcLto, SYN_IPC_LTSO_TX, "TX Allocation Failed"));
        return FALSE;
    }

    hciPkt[0] = synIpcTxGetHciChannel(tMsg->chan);
    SynMemCpyS(hciPkt+1, msg->buflen, msg->buf, msg->buflen);

    status = SynIpcDrvTx(SYN_IPC_INST_FROM_TX(tx)->ipcDrvHandle, SYN_IPC_MSG_BT_HCI, hciPkt, hciPktLen);

    return status;
}

/* Need to check if there will be cases where partial data is copied
then we need to bring the offset based send. this should never happen */

/***********************************************************************
 On arrival of a background interrupt send data to the IPC
 driver.
 ***********************************************************************/
static void synIpcTxHciBgintHandler(void *opaque)
{
    SynIpcTxInstance *tx = opaque;

    if (tx->hciQ)
    {
        if (synIpcTxSendHciPacket(tx, tx->hciQ))
        { /* Packet is sent completely. Remove it from queue */
            synIpcTxReleaseMsg(&(tx->hciQ));

            if (tx->hciQ)
            {
                /* More packets available */
                /* Do not schedule the tx background here, instead wait for
                    ipc to acknowledge (ACK) the completion of previous 
                    transfer. */
                ;
            }
            else
            {
                if (tx->drainHci)
                {
                    tx->drainHci = FALSE;
                    synIpcTxCustomStartTransmit(tx);
                }
            }
        }
        else
        {
            /* Failed to send the packet, setting the bg int 
                might help retry sending it again */
            CSR_LOG_TEXT_WARNING((SynIpcLto, SYN_IPC_LTSO_TX, "Set HCI TX BG Int for retry"));
            synIpcTxStartTransmit(tx);
        }
    }
    else
    {
        if (tx->drainHci)
        {
            tx->drainHci = FALSE;
            synIpcTxCustomStartTransmit(tx);
        }
        CSR_LOG_TEXT_DEBUG((SynIpcLto, SYN_IPC_LTSO_TX, "No message to send"));
    }
}

/***********************************************************************
 Set up a message to be send to the IPC driver and request a
 background interrupt
 ***********************************************************************/
CsrBool SynIpcTxSendMsg(SynIpcTxInstance *tx, TXMSG *msg)
{
    CsrBool success = TRUE;
    SynIpcMsgHci hci;
    SynIpcResult result;

    /* Extension call */
    if (txInst.pTxExt)
    {
        if ((msg->chan == TRANSPORT_CHANNEL_HCI) && 
                (txInst.pTxExt->pktTypeReq & SYN_IPC_HCI_PKT_CMD))
        {
            hci.data = msg->m.buf;
            hci.dataLen = msg->m.buflen;
            hci.pktType = SYN_IPC_HCI_CMD;
            result = txInst.pTxExt->cb(SYN_IPC_MSG_HCI, &hci);
            if (result == SYN_IPC_RESULT_CONSUMED)
            {
                CsrPmemFree(msg->m.buf);
                CsrPmemFree(msg);
                return success;
            }
        }
    }

    if (SYN_IPC_IS_HCI_PACKET(msg))
    {
        if (tx->hciQ)
        {
            /* Transmission is already in progress.
             * This packet will be picked up in the process.
             */
            CSR_LOG_TEXT_DEBUG((SynIpcLto, SYN_IPC_LTSO_TX, "Message enqueued"));
            CsrQueueMsgStore(&(tx->hciQ), msg);
        }
        else
        {
            CsrQueueMsgStore(&(tx->hciQ), msg);
            synIpcTxStartTransmit(tx);
        }
    }
    else
    {
        success = FALSE;

        CSR_LOG_TEXT_WARNING((SynIpcLto,
                              SYN_IPC_LTSO_TX,
                              "unknown channel type %X",
                              msg->chan));

        CsrPmemFree(msg->m.buf);
        CsrPmemFree(msg);
    }

    return (success);
}

/* Triggers the transmission. */
static void synIpcTxStartTransmit(SynIpcTxInstance* tx)
{
    CsrSchedBgintSet(tx->bgint);
}

void SynIpcTxInit(SynIpcTxInstance *tx)
{
    CsrMemSet(tx, 0, sizeof(*tx));

    txInst.synIpcTx = tx;
    tx->bgint = CsrSchedBgintReg(synIpcTxHciBgintHandler, tx, "IPC Pump");
    tx->bgintcustom = CsrSchedBgintReg(synIpcTxCustomBgintHandler, tx, "IPC Custom");
    tx->drainHci = FALSE;
}

void SynIpcTxDeinit(SynIpcTxInstance *tx)
{
    CsrSchedBgintUnreg(tx->bgint);
    CsrSchedBgintUnreg(tx->bgintcustom);
    tx->bgint = CSR_SCHED_BGINT_INVALID;
    tx->bgintcustom = CSR_SCHED_BGINT_INVALID;
    tx->drainHci = FALSE;

    while (tx->hciQ)
    {
        synIpcTxReleaseMsg(&(tx->hciQ));
    }

    while (tx->customQ)
    {
        synIpcTxReleaseMsg(&(tx->customQ));
    }
    CsrMemSet(&txInst, 0, sizeof(txInst));
}

CsrBool SynIpcRegisterTxExtension(SynIpcTxExtensionCallback cb, SynIpcHciPktTypeReq pktTypeReq)
{
    if (!cb)
    {
        return FALSE;
    }
    if (!(pktTypeReq & SYN_IPC_HCI_PKT_CMD))
    {
        return FALSE;
    }
    txInst.ipcTxExt.cb = cb;
    txInst.ipcTxExt.pktTypeReq = pktTypeReq;

    txInst.pTxExt = &(txInst.ipcTxExt);

    return TRUE;
}

static void synIpcTxCustomStartTransmit(SynIpcTxInstance* tx)
{
    CsrSchedBgintSet(tx->bgintcustom);
}

static void synIpcTxCustomReleaseMsg(TXMSG **customSendQ)
{
    TXMSG* tMsg = CsrQueueMsgTake(customSendQ);
    /* seq is reused to indicate if the memory has to be freed or not */
    CsrBool freeData;

    if (tMsg)
    {
        freeData = (CsrBool)tMsg->seq;
        
        if (freeData)
        {
            CsrPmemFree(tMsg->m.buf);
        }
        
        CsrPmemFree(tMsg);
        return;
    }
}

static CsrBool synIpcTxCustomSendPacket(SynIpcTxInstance *tx, TXMSG *tMsg)
{
    CsrUint8* pkt = NULL;
    CsrBool status;
    MessageStructure *msg = &(tMsg->m);

    if (msg->buflen)
    {
        pkt = SynIpcDrvAlloc(msg->buflen);
        if (!pkt)
        {
            CSR_LOG_TEXT_WARNING((SynIpcLto, SYN_IPC_LTSO_TX, "Custom Alloc Fail"));
            return FALSE;
        }
        SynMemCpyS(pkt, msg->buflen, msg->buf, msg->buflen);
    }

    status = SynIpcDrvTx(SYN_IPC_INST_FROM_TX(tx)->ipcDrvCustomHandle, (SynIpcMsgId)msg->dex, pkt, msg->buflen);
    if (status != TRUE)
    {
        /* If SynIpcDrvAlloc() has passed then send should never fail.
            probably we should panic.
        */
        CSR_LOG_TEXT_WARNING((SynIpcLto, SYN_IPC_LTSO_TX, "Custom TX Send failed"));
    }

    return status;
}

static void synIpcTxCustomBgintHandler(void *opaque)
{
    SynIpcTxInstance *tx = opaque;

    if (tx->drainHci)
    {
        /* Wait for the hci commands to be sent. */
        return;
    }

    if (tx->customQ)
    {
        if (synIpcTxCustomSendPacket(tx, tx->customQ))
        { 
            /* Packet is sent completely. Remove it from queue */
            synIpcTxCustomReleaseMsg(&(tx->customQ));

            /* More packets available, but this will be scheduled from the 
                IPC Ack received in Message Rx Callback for the previsouly
                transmitted custom message */
        }
        else
        {
            /* Failed to send the packet, setting the bg int 
                might help retry sending it again */
            CSR_LOG_TEXT_WARNING((SynIpcLto, SYN_IPC_LTSO_TX, "Set Custom TX BG Int for retry"));
            synIpcTxCustomStartTransmit(tx);
        }
    }
    else
    {
        CSR_LOG_TEXT_DEBUG((SynIpcLto, SYN_IPC_LTSO_TX, "No custom message to send"));
    }
}


static CsrBool synIpcTxCustomAddToTransmitQueue(SynIpcTxInstance *tx, TXMSG *msg, CsrBool drainHci)
{
    if (drainHci && tx->hciQ)
    {
        tx->drainHci = TRUE;
    }

    if (tx->customQ)
    {
        /* Transmission of custom packet is in progress.
         * This packet will be picked up in the process.
         */
        CSR_LOG_TEXT_DEBUG((SynIpcLto, SYN_IPC_LTSO_TX, "Custom Message enqueued"));
        CsrQueueMsgStore(&(tx->customQ), msg);
    }
    else
    {
        CsrQueueMsgStore(&(tx->customQ), msg);
        if (tx->drainHci)
        {
            /* Wait for the hci commands to be sent */
        }
        else
        {
            synIpcTxCustomStartTransmit(tx);
        }
    }

    return TRUE;
}

/* drainHci - provides a mechanism for custom messages to be sent when hci messages are 
    waiting in the tx queue. This is required in cases where hci messages have to be flushed
    before sending custom messages, example in case of switching the transport to uart 
    (back to atherton) all the hci messages have to be sent to bt ss.
    */
void SynIpcCustomMessageSend(SynIpcMsgId msgId, CsrUint8 *data, CsrUint16 dataLen, CsrBool drainHci, CsrBool freeMemory)
{
    TXMSG *txQMsg;
    SynIpcInstance *inst = SYN_IPC_INST_FROM_TX(txInst.synIpcTx);

    txQMsg = CsrPmemAlloc(sizeof(TXMSG));
    txQMsg->chan = TRANSPORT_CHANNEL_IPC_CUSTOM;
    txQMsg->seq = (CsrUint8)freeMemory;

    txQMsg->m.buf = data;
    txQMsg->m.buflen = dataLen;
    txQMsg->m.dex = msgId;

    CSR_LOG_TEXT_DEBUG((SynIpcLto, SYN_IPC_LTSO_TX, "Send custom IPC %x", msgId));

    synIpcTxCustomAddToTransmitQueue(&inst->tx, txQMsg, drainHci);
}

