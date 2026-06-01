/******************************************************************************
 Copyright (c) 2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
******************************************************************************/

#include "syn_i3c_internal.h"
#include "csr_pmem.h"
#include "csr_transport.h"
#include "csr_panic.h"
#include "bt_transport_api.h"

/* Log Text Handle (defined in syn_i3c.c) */
CSR_LOG_TEXT_HANDLE_DECLARE(SynI3cLto);

/* Converts the transport channel into HCI channel */
static CsrUint8 synI3cTxGetHciChannel(CsrUint8 transportChannel)
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

    return hciChannel;
}

void SynI3cTxSendMsg(void *msg)
{
    TXMSG *tMsg = (TXMSG *)msg;
    MessageStructure *m = &(tMsg->m);
    CsrUint8 packetType;
    CsrBool status;

    if (!m->buf || m->buflen == 0)
    {
        CSR_LOG_TEXT_WARNING((SynI3cLto, SYN_I3C_LTSO_TX, "SynI3cTxSendMsg: Invalid packet - buf=%p, buflen=%u", m->buf, m->buflen));
        if (m->buf)
        {
            CsrPmemFree(m->buf);
        }
        CsrPmemFree(tMsg);
        return;
    }

    /* Get HCI packet type from transport channel */
    packetType = synI3cTxGetHciChannel(tMsg->chan);

    /* Send via I3C transport API with ownership transfer */
    /* The platform will free m->buf when transmission is complete */
    status = send_hci_packet(packetType, m->buf, m->buflen);
    
    if (!status)
    {
        /* send_hci_packet failed - this is a critical error */
        /* Ownership was not transferred, so packet is still ours but we panic instead of freeing */
        CSR_LOG_TEXT_CRITICAL((SynI3cLto, SYN_I3C_LTSO_TX, "SynI3cTxSendMsg: send_hci_packet failed - PANIC"));
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "I3C TX send_hci_packet failed");
    }
    
    /* Note: m->buf ownership has been transferred to platform (on success) */
    /* We don't free it here as platform will free it via free_cb when transmission completes */

    /* Free the TXMSG structure */
    CsrPmemFree(tMsg);
}
