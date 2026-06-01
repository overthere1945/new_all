/******************************************************************************
 Copyright (c) 2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
******************************************************************************/

#include "syn_i3c_internal.h"

#include "csr_pmem.h"
#include "csr_sched.h"
#include "csr_time.h"
#include "csr_transport.h"
#include "csr_tm_bluecore_transport.h"
#include "bt_transport_api.h"

static SynI3cInstance synI3cInst;

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(SynI3cLto);

#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)
static const CsrCharString *synI3cSubOrigins[] =
{
    "General",
    "TX",
    "RX",
};
#endif

/* Callback for receiving HCI packets from I3C transport */
static void synI3cRxCallback(const uint8_t *packet, uint32_t len)
{
    MessageStructure msg;
    
    if (!packet || len == 0)
    {
        CSR_LOG_TEXT_WARNING((SynI3cLto, SYN_I3C_LTSO_RX, "synI3cRxCallback: Invalid parameters - packet=%p, len=%u", packet, len));
        return;
    }
    
    /* The first byte of the packet indicates the channel type (HCI packet type) */
    if (packet[0] == CSR_HCI_HDR_ACL)
    {
        msg.chan = TRANSPORT_CHANNEL_ACL;
    }
    else if (packet[0] == CSR_HCI_HDR_EVT)
    {
        msg.chan = TRANSPORT_CHANNEL_HCI;
    }
    else if (packet[0] == CSR_HCI_HDR_SCO)
    {
        msg.chan = TRANSPORT_CHANNEL_SCO;
    }
    else if (packet[0] == CSR_HCI_HDR_ISO)
    {
        msg.chan = TRANSPORT_CHANNEL_ISO;
    }
    else if (packet[0] == CSR_PERI_HDR_EVT)
    {
        msg.chan = TRANSPORT_CHANNEL_PERI;
    }
    else if (packet[0] == CSR_HCI_HDR_CMD)
    {
        CSR_LOG_TEXT_WARNING((SynI3cLto, SYN_I3C_LTSO_RX, "synI3cRxCallback: Unexpected CMD from controller"));
        return;
    }
    else if (packet[0] == CSR_PERI_HDR_CMD)
    {
        CSR_LOG_TEXT_WARNING((SynI3cLto, SYN_I3C_LTSO_RX, "synI3cRxCallback: Unexpected PERI CMD from controller"));
        return;
    }
    else
    {
        CSR_LOG_TEXT_WARNING((SynI3cLto, SYN_I3C_LTSO_RX, "synI3cRxCallback: Unknown pkt type 0x%x", packet[0]));
        return;
    }
    
    /* Skip the packet type byte and point to actual HCI data */
    msg.buf = (CsrUint8 *)&packet[1];
    msg.buflen = len - 1;
    msg.dex = 0;
    
    /* Forward to HCI layer using the proper transport function */
    CsrTmBlueCoreTransportMsgRx(synI3cInst.transport, &msg);
}

/* Free callback for platform to free packets after transmission */
static void synI3cFreeCallback(void *ptr)
{
    if (ptr != NULL)
    {
        CsrPmemFree(ptr);
    }
}

static CsrBool synI3cStart(void)
{
    CsrBool result = FALSE;
    bt_transport_config_t config;

    CSR_LOG_TEXT_INFO((SynI3cLto, SYN_I3C_LTSO_GEN, "synI3cStart: state=%x", synI3cInst.state));

    if (synI3cInst.state == SYN_I3C_STATE_STOPPED)
    {
        /* Configure and start I3C transport using bt_transport_api.h */
        config.rx_cb = synI3cRxCallback;
        config.free_cb = synI3cFreeCallback;
        
        if (start_bt_transport(&config))
        {
            synI3cInst.state = SYN_I3C_STATE_STARTED;
            result = TRUE;
            CSR_LOG_TEXT_INFO((SynI3cLto, SYN_I3C_LTSO_GEN, "synI3cStart: Started successfully"));
        }
        else
        {
            CSR_LOG_TEXT_CRITICAL((SynI3cLto, SYN_I3C_LTSO_GEN, "synI3cStart: start_bt_transport failed"));
        }
    }
    else if (synI3cInst.state == SYN_I3C_STATE_STARTED)
    {
        result = TRUE;
    }

    return result;
}

static CsrBool synI3cStop(void)
{
    if (synI3cInst.state == SYN_I3C_STATE_STARTED)
    {
        if (!stop_bt_transport())
        {
            CSR_LOG_TEXT_CRITICAL((SynI3cLto, SYN_I3C_LTSO_GEN, "synI3cStop: stop_bt_transport failed"));
            return FALSE;
        }
        else
        {
            synI3cInst.state = SYN_I3C_STATE_STOPPED;
            CSR_LOG_TEXT_INFO((SynI3cLto, SYN_I3C_LTSO_GEN, "synI3cStop: Stopped successfully"));
        }
    }
    
    return TRUE;
}

static void synI3cMsgTx(void *arg)
{
    SynI3cTxSendMsg(arg);
}

static struct CsrTmBlueCoreTransport _synI3cTransport =
{
    synI3cStart,
    synI3cStop,
    NULL,
    synI3cMsgTx,
    /* Receive callback; Registered by HCI */
    NULL,
    NULL,
    /* driverRestart - Not required for I3C */
    NULL,
    /* restart - Not required for I3C */
    NULL,
    NULL,
    FALSE,
    NULL,
};

void CsrTmBlueCoreI3cInit(void **gash)
{
    CSR_LOG_TEXT_REGISTER(&SynI3cLto,
                          "I3C",
                          CSR_ARRAY_SIZE(synI3cSubOrigins),
                          synI3cSubOrigins);

    synI3cInst.transport = &_synI3cTransport;
    CsrTmBlueCoreTransportInit(gash, synI3cInst.transport);
}
