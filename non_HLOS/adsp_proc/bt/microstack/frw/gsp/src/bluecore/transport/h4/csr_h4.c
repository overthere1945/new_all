/******************************************************************************
 Copyright (c) 2016-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_h4.h"

#include "csr_pmem.h"
#include "csr_sched.h"
#include "csr_serial_com.h"
//#include "platform/csr_serial_init.h"
#include "csr_time.h"
#include "csr_transport.h"
#include "csr_tm_bluecore_transport.h"

static CsrUint32 bluecorePreBootstrapBaudrate = 3000000;

#define BLUECORE_UART_DEVICE "COM1"

/* Calculates time in microseconds to transmit one byte at give baud rate */
#define TIME_PER_BYTE(_baud)    (((CSR_SCHED_SECOND * 8) / (_baud)) + 1)

static CsrH4Instance csrH4Inst;

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(CsrH4Lto);

#ifdef CSR_LOG_ENABLE
/*
static const CsrCharString *h4subOrigins[] =
{
  "General",
  "TX",
  "RX",
};
*/
#endif

#if CSR_H4_ALLOW_STRAY_BYTES
/* Some of the QCA platforms generate a stray byte while applying patch at high
 * baudrates. Usually a stray byte represents a sync loss, but in this case such
 * a stray byte must be ignored.
 * This function helps determine if a stray byte has to be ignored or not,
 * based on user specified number (CSR_H4_ALLOW_STRAY_BYTES). */
static CsrUint32 csrH4IgnoreStrayByte(void)
{
    CsrUint32 numAllowedStrayBytes = CSR_H4_ALLOW_STRAY_BYTES - csrH4Inst.strayByteCount;

    if (numAllowedStrayBytes)
    {
        csrH4Inst.strayByteCount++;
    }

    return (numAllowedStrayBytes);
}
#endif /* CSR_H4_ALLOW_STRAY_BYTES */

static void csrH4Initialize(void)
{
    if (csrH4Inst.state == CSR_H4_STATE_UNINITIALIZED)
    {
        csrH4Inst.state = CSR_H4_STATE_INITIALIZED;
        CsrH4RxInit(&csrH4Inst.rx);
        CsrH4TxInit(&csrH4Inst.tx);
        CsrUartDrvRegister(csrH4Inst.uartHandle,
                           NULL,
                           csrH4Inst.rx.bgintReassemble);
        CsrUartDrvRegisterTx(csrH4Inst.uartHandle,
                             csrH4Inst.tx.bgintIsr);

        CSR_LOG_TEXT_INFO((CsrH4Lto, CSR_H4_LTSO_GEN, "Initialized"));
    }
}

static void csrH4Deinitialize(void)
{
    if (csrH4Inst.state == CSR_H4_STATE_INITIALIZED)
    {
        csrH4Inst.state = CSR_H4_STATE_UNINITIALIZED;
        CsrH4RxDeinit(&csrH4Inst.rx);
        CsrH4TxDeinit(&csrH4Inst.tx);

#ifdef CSR_H4_EXTENSION
        CsrPmemFree(csrH4Inst.extInst->prtInst);
        csrH4Inst.extInst->prtInst = NULL;

        CsrPmemFree(csrH4Inst.extInst);
        csrH4Inst.extInst = NULL;
#endif

        CSR_LOG_TEXT_INFO((CsrH4Lto, CSR_H4_LTSO_GEN, "Deinitialized"));
    }
}

static CsrBool csrH4StartCommon(CsrUint8 baudId)
{
    CsrBool result = FALSE;

    if (csrH4Inst.state == CSR_H4_STATE_UNINITIALIZED)
    {
        CsrCharString *port = BLUECORE_UART_DEVICE;
        CsrUint8 token = 0xC0;

        csrH4Inst.uartHandle = CsrUartDrvOpen(port,
                                            &bluecorePreBootstrapBaudrate,
                                            3,
                                            0,
                                            1,
                                            TRUE,
                                            &token);

        if (CsrUartDrvStart(csrH4Inst.uartHandle, baudId))
        {
            CsrUint32 baudRate = CsrUartDrvGetBaudrate(csrH4Inst.uartHandle);

            csrH4Initialize();

            csrH4Inst.tx.timePerByte = (2 * TIME_PER_BYTE(baudRate));

            csrH4Inst.state = CSR_H4_STATE_STARTED;

            CSR_LOG_TEXT_INFO((CsrH4Lto, CSR_H4_LTSO_GEN, "Started at baudrate=%d", baudRate));

            result = TRUE;
        }
        else
        {
            CSR_LOG_TEXT_CRITICAL((CsrH4Lto, CSR_H4_LTSO_GEN, "Start failed"));
        }
    }
    else if (csrH4Inst.state == CSR_H4_STATE_STARTED)
    {
        result = TRUE;
    }

    return (result);
}

static CsrBool csrH4Start(void)
{
    return (csrH4StartCommon(CSR_UART_BAUD_RATE_IGNORE));
}

static CsrBool csrH4Stop(void)
{
    if (csrH4Inst.state < CSR_H4_STATE_STARTED || CsrUartDrvStop(csrH4Inst.uartHandle))
    {
        CSR_LOG_TEXT_INFO((CsrH4Lto, CSR_H4_LTSO_GEN, "Stopped"));
        csrH4Inst.state = CSR_H4_STATE_INITIALIZED;

        csrH4Deinitialize();
        CsrUartDrvClose(csrH4Inst.uartHandle);
        return (TRUE);
    }
    else
    {
        CSR_LOG_TEXT_ERROR((CsrH4Lto, CSR_H4_LTSO_GEN, "Stop failed"));
        return (FALSE);
    }
}

static CsrUint16 csrH4Query(void)
{
    return (CSR_H4_GET_TRANSPORT_TYPE(&(csrH4Inst)));
}

static void csrH4MsgTx(void *arg)
{
    CsrH4TxSendMsg(&(csrH4Inst.tx), arg);
}

static void csrH4ScoTx(void *arg)
{
    TXMSG *txQMsg;

    txQMsg = CsrPmemAlloc(sizeof(*txQMsg));
    txQMsg->chan = TRANSPORT_CHANNEL_SCO;
    txQMsg->seq = 0;

    txQMsg->m.buf = arg;
    txQMsg->m.buflen = ((CsrUint8 *) arg)[CSR_H4_SCO_LEN_FIELD_IDX]
                       + CSR_HCI_SCO_HDR_SIZE;
    txQMsg->m.dex = 0;

    CsrH4TxSendMsg(&(csrH4Inst.tx), txQMsg);
}

static void csrH4IsoTx(void *arg)
{
    TXMSG *txQMsg;
    CsrUint8 *pkt = arg;
    CsrUint16 isoDataLoadLength;

    isoDataLoadLength = pkt[3] + ((pkt[4] & 0x3F) << 8);

    txQMsg = CsrPmemAlloc(sizeof(*txQMsg));
    txQMsg->chan = TRANSPORT_CHANNEL_ISO;
    txQMsg->seq = 0;

    txQMsg->m.buf = arg;
    txQMsg->m.buflen = isoDataLoadLength + CSR_HCI_ISO_HDR_SIZE + 1;
    txQMsg->m.dex = 0;

    CsrH4TxSendMsg(&(csrH4Inst.tx), txQMsg);

}

static CsrBool csrH4DriverRestart(CsrUint8 baudId)
{
    CsrBool result = FALSE;

    CSR_LOG_TEXT_INFO((CsrH4Lto, CSR_H4_LTSO_GEN, "Restarting"));
    if (csrH4Stop())
    {
        result = csrH4StartCommon(baudId);
    }
    return (result);
}

static void csrH4Restart(void)
{
    if (csrH4DriverRestart(CSR_UART_BAUD_RATE_IGNORE))
    {
        csrH4Inst.state = CSR_H4_STATE_RESTARTED;
    }
}

static void csrH4Close(void)
{
}

static struct CsrTmBlueCoreTransport _csrH4Transport =
{
  csrH4Start,
  csrH4Stop,
  csrH4Query,
  csrH4MsgTx,
  NULL,                 /* Receive callback; Registered by HCI */
  csrH4ScoTx,
  csrH4DriverRestart,
  csrH4Restart,
  csrH4Close,
  FALSE,
  csrH4IsoTx,
};

void CsrTmBlueCoreH4Init(void **gash)
{
    CSR_LOG_TEXT_REGISTER(&CsrH4Lto,
                          "H4",
                          CSR_ARRAY_SIZE(h4subOrigins),
                          h4subOrigins);

    csrH4Inst.transport = &_csrH4Transport;
    CsrTmBlueCoreTransportInit(gash, csrH4Inst.transport);
}

void CsrTmBlueCoreRegisterUartHandleH4(void *handle)
{
    csrH4Inst.uartHandle = handle;
}

CsrUint16 CsrH4ExtDataHandler(CsrUint8 * const *writePtr)
{
#ifdef CSR_H4_EXTENSION
    CsrH4ExtInstance *extInst = csrH4Inst.extInst;

    if (extInst && extInst->rxHandler)
    {
#if CSR_H4_ALLOW_STRAY_BYTES
        /* The stray byte generated by some QCA chips on patch download may
         * resemble an IBS message (extension protocol).
         * Such a stray byte may prematurely activate IBS. If TX idle timeout
         * is small enough for IBS to sleep during bootstrap, it may never come
         * out of sleep. */
        if (extInst->transportType == TRANSPORT_TYPE_H4IBS &&
            csrH4IgnoreStrayByte() == CSR_H4_ALLOW_STRAY_BYTES)
        {
            /* Ignore the first unknown byte if ignoring stray bytes are allowed,
             * irrespective of the value.
             * IBS protocol is robust enough to recover if a genuine IBS message from
             * chip is ignored as stray byte. */
            CSR_LOG_TEXT_ERROR((CsrH4Lto, CSR_H4_LTSO_RX, "Stray byte ignored"));
        }
        else
#endif /* CSR_H4_ALLOW_STRAY_BYTES */
        {
            return (extInst->rxHandler(extInst->prtInst, writePtr));
        }
    }
    else
#else /* CSR_H4_EXTENSION */
    CSR_UNUSED(writePtr);
#endif /* CSR_H4_EXTENSION */
    {
        CsrH4SyncLossHandler(CSR_H4_SYNC_LOSS_TYPE_INVALID_HDR);
    }
    return (0);
}

void CsrH4SyncLossHandler(CsrH4SyncLoss reason)
{
#if CSR_H4_ALLOW_STRAY_BYTES
    /* Some QCA chips may generate stray byte while applying patch at high baudrates.
     * Below logic allow H4 to ignore such stray byte. */
    if (reason == CSR_H4_SYNC_LOSS_TYPE_INVALID_HDR && csrH4IgnoreStrayByte())
    {
        CSR_LOG_TEXT_ERROR((CsrH4Lto, CSR_H4_LTSO_RX, "Stray byte ignored"));
    }
    else
#endif
    {
#ifdef CSR_TARGET_PRODUCT_WEARABLE
        CsrPanic(CSR_TECH_FW, (CSR_PANIC_FW_H4_SYNC_LOST * 10) + reason, "h4 panic");
#else
        CSR_LOG_TEXT_ERROR((CsrH4Lto, CSR_H4_LTSO_RX, "Sync loss: reason=%d", reason));
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_H4_SYNC_LOST, "h4 panic");
#endif
    }
}

#ifdef CSR_H4_EXTENSION
CsrBool CsrH4SetPowerState(CsrBool powerState)
{
    if (CsrUartDrvSetPowerMode(csrH4Inst.uartHandle, powerState))
    {
        return (TRUE);
    }
    else
    {
        CSR_LOG_TEXT_ERROR((CsrH4Lto,
                            CSR_H4_LTSO_GEN,
                            "UART power state not changed: %d",
                            powerState));
        return (FALSE);
    }
}

void CsrH4SendExtData(TXMSG *msg)
{
    csrH4MsgTx(msg);
}

void CsrH4StartTransmit(void)
{
    CsrH4TxStartTransmit(&(csrH4Inst.tx));
}

void CsrH4RegisterExtension(CsrUint16 transportType,
                            void *inst,
                            CsrH4RxExtDataHandler rxHandler,
                            CsrH4StatusHandler statusHandler)
{
    CsrH4ExtInstance *extInst = CsrPmemZalloc(sizeof(*extInst));

    extInst->transportType = transportType;
    extInst->rxHandler = rxHandler;
    extInst->statusHandler = statusHandler;
    extInst->prtInst = inst;

    csrH4Inst.extInst = extInst;

    CSR_LOG_TEXT_INFO((CsrH4Lto,
                       CSR_H4_LTSO_GEN,
                       "Registered transport type %d",
                       transportType));
}
#endif /* CSR_H4_EXTENSION */

