/******************************************************************************
 Copyright (c) 2016-2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_h4_rx.h"

#include "csr_pmem.h"
#include "csr_serial_com.h"
#include "csr_time.h"
#include "csr_util.h"
#include "csr_transport.h"
#include "csr_tm_bluecore_transport.h"
#include "csr_h4.h"


#if CSR_H4_PKT_READ_TIMEOUT
#define CSR_H4_RX_SYNC_LOSS_TIMEOUT  (CSR_H4_PKT_READ_TIMEOUT * CSR_SCHED_SECOND)

static void csrH4RxPacketReadTimeout(CsrUint16 type, void *opaque)
{
    CsrH4RxInstance *rx = (CsrH4RxInstance *) opaque;
    rx->pktReadTimer = CSR_SCHED_TID_INVALID;
    CsrH4SyncLossHandler((CsrUint8) type);
}
#endif

/***********************************************************************
 Receives a full RX package from the lowest part of the H4
 driver. At this point the package is assembled and the channel
 is decoded. If the package is not a HCI-type this function
 must further decode the message to determine the channel and
 assemble fragmented messages (e.g. BCCMD messages).
 ***********************************************************************/
static CsrUint16 csrH4RxMsgPut(void *transport,
                               CsrUint8 h4Hdr,
                               CsrUint8 *data,
                               CsrUint16 len)
{
    MessageStructure msg;

    switch (h4Hdr)
    {
        case CSR_HCI_HDR_ACL:
            msg.chan = TRANSPORT_CHANNEL_ACL;
            break;

        case CSR_HCI_HDR_EVT:
            msg.chan = TRANSPORT_CHANNEL_HCI;
            break;

        case CSR_HCI_HDR_SCO:
            msg.chan = TRANSPORT_CHANNEL_SCO;
            break;

        case CSR_HCI_HDR_ISO:
            msg.chan = TRANSPORT_CHANNEL_ISO;
            break;

        case CSR_PERI_HDR_EVT:
            msg.chan = TRANSPORT_CHANNEL_PERI;
            break;

        default:
            /* This should never happen so do something dramatic */
            CSR_LOG_TEXT_CRITICAL((CsrH4Lto, CSR_H4_LTSO_RX, "Unknown H4 header: %d", h4Hdr));
            return (0);
            break;
    }

    msg.buf = data;
    msg.buflen = len;
    msg.dex = 0;

    CsrTmBlueCoreTransportMsgRx(transport, &msg);

    CsrPmemFree(msg.buf);

    return (len);
}

static CsrUint16 csrH4RxGetHciHeaderSize(CsrUint8 h4Header)
{
    CsrUint16 headerSize;

    switch (h4Header)
    {
        case CSR_HCI_HDR_ACL:
            headerSize = CSR_HCI_ACL_HDR_SIZE;
            break;
        case CSR_HCI_HDR_SCO:
            headerSize = CSR_HCI_SCO_HDR_SIZE;
            break;
        case CSR_HCI_HDR_EVT:
            headerSize = CSR_HCI_EVT_HDR_SIZE;
            break;
        case CSR_HCI_HDR_ISO:
            headerSize = CSR_HCI_ISO_HDR_SIZE;
            break;
        case CSR_PERI_HDR_EVT:
            headerSize = CSR_PERI_EVT_HDR_SIZE;
            break;
        default:
            headerSize = 0;
            break;
    }
    return (headerSize);
}

static CsrUint16 csrH4RxGetHciBodySize(CsrUint8 h4Header, CsrUint8 *hciHeader)
{
    CsrUint16 hciPayloadSize;

    switch (h4Header)
    {
        case CSR_HCI_HDR_ACL:
            hciPayloadSize = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(&hciHeader[CSR_H4_ACL_LEN_FIELD_IDX]);
            break;
        case CSR_HCI_HDR_SCO:
            hciPayloadSize = hciHeader[CSR_H4_SCO_LEN_FIELD_IDX];
            break;
        case CSR_HCI_HDR_EVT:
            hciPayloadSize = hciHeader[CSR_H4_EVT_LEN_FIELD_IDX];
            break;
        case CSR_HCI_HDR_ISO:
            hciPayloadSize = hciHeader[CSR_H4_ISO_LEN_FIELD_IDX] + ((hciHeader[CSR_H4_ISO_LEN_FIELD_IDX + 1] & 0x3F) << 8);
            break;
        case CSR_PERI_HDR_EVT:
            hciPayloadSize = hciHeader[CSR_H4_PERI_EVT_LEN_FIELD_IDX];
            break;
        default:
            hciPayloadSize = 0;
            break;
    }

    return (hciPayloadSize);
}

static void csrH4RxStateInit(CsrH4RxInstance* rx)
{
    CSR_H4_RX_STATE_SET(rx, CSR_H4_RX_STATE_PKT_HDR);
    rx->bytesToRead = CSR_H4_PKT_HDR_SIZE;
    rx->writePtr = &(rx->h4Header);
}

static void csrH4RxExtDataHandler(CsrH4RxInstance* rx)
{
    CSR_LOG_TEXT_DEBUG((CsrH4Lto, CSR_H4_LTSO_RX, "Unknown H4 header: 0x%02X", rx->h4Header));

    rx->bytesToRead = CsrH4ExtDataHandler(&(rx->writePtr));
    if (rx->bytesToRead)
    {
        /* Protocol handler wants to read more data */
        CSR_H4_RX_STATE_SET(rx, CSR_H4_RX_STATE_EXT_DATA);
    }
    else
    {
        csrH4RxStateInit(rx);
    }
}

static void csrH4RxHciBodyHandler(CsrH4RxInstance* rx)
{
    /* Complete HCI packet read. Pass it up. Reset state. */
    csrH4RxMsgPut(CSR_H4_INST_FROM_RX(rx)->transport,
                  rx->h4Header,
                  rx->hciPacket,
                  rx->hciPacketLength);

    rx->hciPacket = NULL;

    CSR_H4_RX_HCI(rx);

#if CSR_H4_PKT_READ_TIMEOUT
    CSR_H4_STOP_TIMER(rx->pktReadTimer);
#endif

    csrH4RxStateInit(rx);
}

static void csrH4RxHciHdrHandler(CsrH4RxInstance* rx)
{
    CsrUint16 hciHeaderSize = csrH4RxGetHciHeaderSize(rx->h4Header);
    CsrUint16 hciPayloadSize = csrH4RxGetHciBodySize(rx->h4Header, rx->hciHeader);

    rx->hciPacketLength = hciHeaderSize + hciPayloadSize;
    rx->hciPacket = CsrPmemAlloc(rx->hciPacketLength);
    SynMemCpyS(rx->hciPacket, rx->hciPacketLength, rx->hciHeader, hciHeaderSize);

    if (hciPayloadSize)
    {
        rx->bytesToRead = hciPayloadSize;
        rx->writePtr = rx->hciPacket + hciHeaderSize;
        CSR_H4_RX_STATE_SET(rx, CSR_H4_RX_STATE_HCI_BODY);
    }
    else
    { /* Send immediately if zero-length HCI packet */
        csrH4RxHciBodyHandler(rx);
    }
}

static void csrH4RxPktHdrHandler(CsrH4RxInstance* rx)
{
    rx->bytesToRead = csrH4RxGetHciHeaderSize(rx->h4Header);
    if (rx->bytesToRead)
    {
        CSR_H4_RX_STATE_SET(rx, CSR_H4_RX_STATE_HCI_HDR);
        rx->writePtr = rx->hciHeader;

#if CSR_H4_PKT_READ_TIMEOUT
        CSR_H4_RESTART_TIMER(rx->pktReadTimer,
                             CSR_H4_RX_SYNC_LOSS_TIMEOUT,
                             csrH4RxPacketReadTimeout,
                             CSR_H4_SYNC_LOSS_TYPE_READ_TIMEOUT,
                             (void *) (rx));
#endif
    }
    else
    {
        /* Must be unknown H4 packet. Exception */
        csrH4RxExtDataHandler(rx);
    }
}

static void csrH4RxPacketReassembly(void *context)
{
    CsrUint16 bytesRead;
    CsrH4Instance *h4Inst = (CsrH4Instance *) context;
    CsrH4RxInstance *rx = &h4Inst->rx;

    bytesRead = (CsrUint16) CsrUartDrvLowLevelTransportRx(h4Inst->uartHandle,
                                                          rx->bytesToRead,
                                                          rx->writePtr);
    rx->bytesToRead -= bytesRead;

    if (rx->bytesToRead)
    { /* More data to be read for current state.
     If and when more data is available with UART, it would set RX BgInt. */
        rx->writePtr += bytesRead;
    }
    else
    { /* All data for current state has been read. Move to next state */
        switch (rx->state)
        {
            case CSR_H4_RX_STATE_PKT_HDR: /* New packet */
                csrH4RxPktHdrHandler(rx);
                break;

            case CSR_H4_RX_STATE_HCI_HDR:
                csrH4RxHciHdrHandler(rx);
                break;

            case CSR_H4_RX_STATE_HCI_BODY:
                csrH4RxHciBodyHandler(rx);
                break;

            case CSR_H4_RX_STATE_EXT_DATA:
                csrH4RxExtDataHandler(rx);
                break;

            default:
                /* State machine in unknown state - This should never happen */
                /* If this actually happens a _very serious_ error occurred! */
                CSR_LOG_TEXT_ERROR((CsrH4Lto, CSR_H4_LTSO_RX, "Unknown state=%d", rx->state));
                CsrH4Panic(CSR_PANIC_FW_H4_CORRUPTION);
                break;
        }
    }
}

void CsrH4RxInit(CsrH4RxInstance *rx)
{
    CsrMemSet(rx, 0, sizeof(*rx));
    csrH4RxStateInit(rx);
    rx->bgintReassemble = CsrSchedBgintReg(csrH4RxPacketReassembly,
                                           CSR_H4_INST_FROM_RX(rx),
                                           "H4 Reassemble");
}

void CsrH4RxDeinit(CsrH4RxInstance *rx)
{
    rx->writePtr = NULL;

    CsrSchedBgintUnreg(rx->bgintReassemble);
    rx->bgintReassemble = CSR_SCHED_BGINT_INVALID;

    CsrPmemFree(rx->hciPacket);
    rx->hciPacket = NULL;

#if CSR_H4_PKT_READ_TIMEOUT
    CSR_H4_STOP_TIMER(rx->pktReadTimer);
#endif
}

