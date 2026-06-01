/******************************************************************************
 Copyright (c) 2016-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_h4_tx.h"

#include "csr_pmem.h"
#include "csr_serial_com.h"
#include "csr_util.h"
#include "csr_queue_lib.h"
#include "csr_h4.h"

/* Releases the first message from the queue */
static void csrH4TxReleaseMsg(TXMSG **sendQ)
{
    TXMSG* tMsg = CsrQueueMsgTake(sendQ);

    if (tMsg != NULL)
    {
        CsrPmemFree(tMsg->m.buf);
        tMsg->m.buflen = 0;
        tMsg->m.dex = 0;
        CsrPmemFree(tMsg);
    }
}

/* Converts the transport channel into HCI channel */
static CsrUint8 csrH4TxGetHciChannel(CsrUint8 transportChannel)
{
    CsrUint8 hciChannel;

    switch (transportChannel)
    {
        case TRANSPORT_CHANNEL_ACL:
            hciChannel = CSR_HCI_HDR_ACL;
            break;
        case TRANSPORT_CHANNEL_SCO:
            hciChannel = CSR_HCI_HDR_SCO;
            break;
        case TRANSPORT_CHANNEL_HCI:
            hciChannel = CSR_HCI_HDR_CMD;
            break;
        case TRANSPORT_CHANNEL_ISO:
            hciChannel = CSR_HCI_HDR_ISO;
            break;
        case TRANSPORT_CHANNEL_PERI:
            hciChannel = CSR_PERI_HDR_CMD;
            break;
        default:
            hciChannel = transportChannel;
            break;
    }

    return (hciChannel);
}

static void csrH4TxOverflowWaitEnd(CsrUint16 tmp, void *opaque)
{
    CsrH4TxInstance *tx = (CsrH4TxInstance *) opaque;

    tx->overflowWaitTimer = CSR_SCHED_TID_INVALID;
    CsrH4TxStartTransmit(tx);

    CSR_UNUSED(tmp);
}

#define MIN_WAIT_TIME                   (10 * CSR_SCHED_MILLISECOND)
static void csrH4TxOverflowWaitStart(CsrH4TxInstance *tx, CsrUint16 len)
{
    CsrTime txWaitTime;

    /* UART buffer is full. Give it a little peace and quite to send something */
    if (tx->overflowWaitTimer != CSR_SCHED_TID_INVALID)
    {
        CsrSchedTimerCancel(tx->overflowWaitTimer, 0, NULL);
    }

    /* Wait time is twice the theoretical time taken for the set baud rate */
    txWaitTime = len * tx->timePerByte + MIN_WAIT_TIME;
    tx->overflowWaitTimer = CsrSchedTimerSet(txWaitTime,
                                             csrH4TxOverflowWaitEnd,
                                             0,
                                             tx);
}

/****************************************************************************
 Send Tx buffer to Uart Driver to be transmitted over UART
 Starts timer to trigger sending pending bytes if all data could not be sent.
 Returns number of bytes pending.
 *****************************************************************************/
static CsrUint32 csrH4TxSendBytes(CsrH4TxInstance *tx,
                                  const CsrUint8 *data,
                                  CsrUint32 dataLen)
{
    if (data && dataLen)
    {
        CsrUint32 bytesSent, bytesRemaining;

#ifdef CSR_TARGET_PRODUCT_WEARABLE
        if (txBlock)
        {
            CSR_LOG_TEXT_INFO((CsrH4Lto, CSR_H4_LTSO_TX, "Tx blocked (BTSS crashed or BT OFF scenario)"));
            return dataLen;
        }
#endif

    /* Put bytes onto the TX buffer */
        if (!CsrUartDrvTx(CSR_H4_INST_FROM_TX(tx)->uartHandle,
                          data,
                          dataLen,
                          &bytesSent))
        {
            CSR_LOG_TEXT_INFO((CsrH4Lto, CSR_H4_LTSO_TX, "Overrun"));
        }

        bytesRemaining = dataLen - bytesSent;
        if (bytesRemaining)
        {
            csrH4TxOverflowWaitStart(tx, bytesRemaining);
        }
        return (bytesRemaining);
    }
    else
    {
        return (0);
    }
}

static CsrBool csrH4TxSendPacket(CsrH4TxInstance *tx, TXMSG *tMsg)
{
    CsrBool result = FALSE;

    if (tx->state == CSR_H4_TX_STATE_HDR)
    {
        CsrUint8 hciChannel = csrH4TxGetHciChannel(tMsg->chan);

        if (hciChannel == CSR_HCI_HDR_ISO)
        {
            CSR_H4_TX_STATE_SET(tx, CSR_H4_TX_STATE_BODY);
        }
        else if (!csrH4TxSendBytes(tx, &hciChannel, CSR_H4_PKT_HDR_SIZE))
        {
            CSR_H4_TX_STATE_SET(tx, CSR_H4_TX_STATE_BODY);
        }
    }

    if (tx->state == CSR_H4_TX_STATE_BODY)
    {
        MessageStructure *msg = &(tMsg->m);
        CsrUint16 bufLen, bytesRemaining;
        CsrUint8 *buf;

        buf = msg->buf + msg->dex;
        bufLen = msg->buflen - msg->dex;

        bytesRemaining = (CsrUint16) csrH4TxSendBytes(tx, buf, bufLen);

        if (bytesRemaining)
        {
            msg->dex = msg->buflen - bytesRemaining;
        }
        else
        {
            CSR_H4_TX_STATE_SET(tx, CSR_H4_TX_STATE_HDR);

            result = TRUE;
        }
    }

    return (result);
}

#define OVERFLOW_WAITING(_tx)   ((_tx)->overflowWaitTimer != CSR_SCHED_TID_INVALID)

/***********************************************************************
 On arrival of a background interrupt send data to the UART
 driver. First finish send already started messages, then send
 SCO data if any and finally send everything else.
 ***********************************************************************/
static void csrH4TxBgintHandler(void *opaque)
{
    CsrH4TxInstance *tx = (CsrH4TxInstance *) opaque;

    if (tx->msgQ)
    {
        if (csrH4TxSendPacket(tx, tx->msgQ))
        { /* Packet is sent completely. Remove it from queue */
            csrH4TxReleaseMsg(&(tx->msgQ));

            /* More packets available */
            if (tx->msgQ)
            {
#ifdef CSR_H4_EXTENSION
                if (CSR_H4_IS_HCI_PACKET(tx->msgQ))
                { /* Inform extension module that there HCI data to send */
                    CSR_H4_TX_QUEUE_READY(tx);
                }
                else /* Extension packets, send it immediately  */
#endif
                {
                    CsrH4TxStartTransmit(tx);
                }
            }
            else
            {
                CSR_H4_TX_QUEUE_EMPTY(tx);
            }
        }
    }
    else
    {
        CSR_LOG_TEXT_INFO((CsrH4Lto, CSR_H4_LTSO_TX, "No message to send"));
        CSR_H4_TX_QUEUE_EMPTY(tx);
    }
}

static void csrH4TxIsrBgintHandler(void *opaque)
{
    CsrH4TxInstance *tx = (CsrH4TxInstance *) opaque;

    if (tx->overflowWaitTimer != CSR_SCHED_TID_INVALID)
    {
        CsrSchedTimerCancel(tx->overflowWaitTimer, 0, NULL);
        tx->overflowWaitTimer = CSR_SCHED_TID_INVALID;
        CsrH4TxStartTransmit(tx);
    }
}

#ifdef CSR_H4_EXTENSION
/* Adds the message in the front of the queue but behind
 * the messages from same channel or any partially sent packet. */
static void csrH4TxSendPriorityMsg(CsrH4TxInstance *tx, TXMSG *msg)
{
    TXMSG *p = NULL, *q = tx->msgQ;

    /* Over flow wait timer can be used to determine if
     * there is a partially transmitted packet in the message queue */
    if (OVERFLOW_WAITING(tx))
    { /* There is a partially sent packet in the queue.
         We can't add the new packet at the head.
         Remove partially sent packet. This will be added back later as head. */
        p = CsrQueueMsgTake(&q);
        if (p)
        {
            p->next = NULL;
        }
    }
    else
    { /* Trigger new transmission. New Packet is be added to the message queue below */
        CsrH4TxStartTransmit(tx);
    }

    /* Make sure that packets for same channel are sequential. */
    while (q && q->chan == msg->chan)
    {
        TXMSG *t = CsrQueueMsgTake(&q);
        CsrQueueMsgStore(&p, t);
    }

    /* Add tail behind prioritised message */
    if (q)
    {
        CsrQueueMsgAdd(&msg, q);
    }

    if (p)
    {
        CsrQueueMsgAdd(&p, msg);
        tx->msgQ = p;
    }
    else
    {
        tx->msgQ = msg;
    }

    CSR_LOG_TEXT_DEBUG((CsrH4Lto, CSR_H4_LTSO_TX, "Send Priority"));
}
#endif /* CSR_H4_EXTENSION */

/***********************************************************************
 Set up a message to be send to the UART driver and request a
 background interrupt
 ***********************************************************************/
CsrBool CsrH4TxSendMsg(CsrH4TxInstance *tx, TXMSG *msg)
{
    CsrBool success = TRUE;

    if (CSR_H4_IS_HCI_PACKET(msg))
    {
        if (tx->msgQ)
        {
            /* Transmission is already in progress.
             * This packet will be picked up in the process. */
            CSR_LOG_TEXT_DEBUG((CsrH4Lto, CSR_H4_LTSO_TX, "Message enqueued"));
        }
        else
        {
            CSR_H4_TX_QUEUE_READY(tx);
        }

        CsrQueueMsgStore(&(tx->msgQ), msg);
    }
    else
    {
#ifdef CSR_H4_EXTENSION
        csrH4TxSendPriorityMsg(tx, msg);
#else
        success = FALSE;

        CSR_LOG_TEXT_WARNING((CsrH4Lto,
                              CSR_H4_LTSO_TX,
                              "Unknown channel: 0x%02X",
                              msg->chan));

        CsrPmemFree(msg->m.buf);
        CsrPmemFree(msg);
#endif
    }

    return (success);
}

/* Triggers the transmission. */
void CsrH4TxStartTransmit(CsrH4TxInstance* tx)
{
    CsrSchedBgintSet(tx->bgint);
}

void CsrH4TxInit(CsrH4TxInstance *tx)
{
    CsrMemSet(tx, 0, sizeof(*tx));
    tx->bgint = CsrSchedBgintReg(csrH4TxBgintHandler, tx, "H4 Pump");
    tx->bgintIsr = CsrSchedBgintReg(csrH4TxIsrBgintHandler, tx, "H4 Tx done");
}

void CsrH4TxDeinit(CsrH4TxInstance *tx)
{
    CsrSchedBgintUnreg(tx->bgint);
    CsrSchedBgintUnreg(tx->bgintIsr);
    tx->bgint = CSR_SCHED_BGINT_INVALID;

    while (tx->msgQ)
    {
        csrH4TxReleaseMsg(&(tx->msgQ));
    }

    CsrSchedTimerCancel(tx->overflowWaitTimer, 0, NULL);
    tx->overflowWaitTimer = CSR_SCHED_TID_INVALID;
}

