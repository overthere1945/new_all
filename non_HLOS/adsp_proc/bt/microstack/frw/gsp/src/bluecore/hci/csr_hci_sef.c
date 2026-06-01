/******************************************************************************
Copyright (c) 2009-2026 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #6 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_macro.h"
#include "csr_hci_sef.h"
#include "csr_hci_prim.h"
#include "csr_hci_task.h"
#include "csr_hci_util.h"
#include "csr_hci_handler.h"
#include "csr_pmem.h"
#include "csr_transport.h"
#include "csr_tm_bluecore_lib.h"
#include "csr_tm_bluecore_private_lib.h"
#include "csr_tm_bluecore_transport.h"
#include "csr_hci_private_lib.h"
#include "csr_hci_sco.h"
#include "csr_log_gsp.h"
#include "csr_log_text_2.h"
#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
#include "csr_ips.h"
#include "csr_sched_ips.h"
#endif
#include "csr_hci_iso.h"
#include "hci_arbiter_private.h"


#ifdef CSR_USE_QCA_CHIP
/* OCF=0x00 for CSR chipsets.
   OCF=0x07 for BCCMD/HQ on QCA chipsets.
*/
#define HCI_BCCMD_HQ_VENDOR_OPCODE_LSB 0x07
#else
#define HCI_BCCMD_HQ_VENDOR_OPCODE_LSB 0x00
#endif /* CSR_USE_QCA_CHIP */

/* CSR_HCI internal transport manager interface functions */
static void *thdl = NULL;
static CsrHciInstData *globalInst;

#ifdef CSR_TARGET_PRODUCT_WEARABLE
CsrUint8 txBlock;
#endif

#ifndef CSR_USE_QCA_CHIP
static CsrBool checkHwErrSyncLoss(CsrUint8 *payload)
{
    /*Check for H4 link Sync failure.*/
    if(((payload[0] == CSR_HCI_EV_HARDWARE_ERROR) &&
        (payload[1] == CSR_HCI_EV_PARAM_LEN_H4_LINK_SYNC_FAILURE) &&
        (payload[2] == CSR_HCI_EV_H4_LINK_SYNC_FAILURE)))
    {
        return TRUE;
    }
    
    return FALSE;
}
#endif


#ifndef CSR_TARGET_PRODUCT_WEARABLE

#ifdef CSR_USE_QCA_CHIP
static CsrBool hciCheckHciResetCmdComplete(CsrUint8 *payload,CsrUint16 length)
{
    if( length >= 6 )
    {
        /*check if Command complete event received from chip for the HCI_RESET sent.*/
        if ((payload[0] == CSR_HCI_EV_COMMAND_COMPLETE) &&
            (payload[2] == CSR_HCI_CMD_PACKET) &&
            (payload[3] == CSR_HCI_CMD_RESET) &&
            (payload[4] == (CSR_HCI_OGF << 2)))
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif
#endif

#ifdef CSR_TARGET_PRODUCT_WEARABLE
void CsrHciBlockTransmission(void)
{
    txBlock = 1;
}
#endif

static void csrHciPrepareTransport(CsrUint8 ch, CsrUint8 type, CsrUint8 *msg, CsrUint32 msg_len)
{
#ifdef CSR_TARGET_PRODUCT_WEARABLE
    if (txBlock)
    {
        CsrPmemFree(msg);
        return;
    }
#endif
    {
        TXMSG *txQMsg;

        txQMsg = CsrPmemAlloc(sizeof(TXMSG));
        txQMsg->chan = ch;
        txQMsg->seq = type;

        txQMsg->m.buf = msg;
        txQMsg->m.buflen = msg_len;
        txQMsg->m.dex = 0;

        CSR_LOG_BCI(ch, FALSE, msg_len, msg);

        CsrTmBlueCoreTransportMsgTx(thdl, txQMsg);        /*    transport is always able to send or queue the message */
    }
}

//#if (CSR_HOST_PLATFORM == Q6_HOST) 
#ifndef CSR_TARGET_PRODUCT_WEARABLE

#ifdef CSR_USE_QCA_CHIP
void csrHciHandleQcaVendorEvents(CsrHciInstData *inst, CsrMblk **mblk)
{
#ifndef EXCLUDE_CSR_BCCMD_MODULE
    CsrUint8 *mapped_data;
    CsrUint8 vendorEventClass;
    CsrUint8 data[4];

    /* QCA BCCMD : 0xFF | len | Vendor EventClass 0x07  | Channel/Frag | <BCCMD> */
    /* QCA VS CMD: 0xFF | len | Vendor EventClass 0xNN  | <QCA VS Packet> */

    /* mblk here will be pointing from lenght field, so at offset is where the
        vendor event class would need to be fetched */
    mapped_data = CsrMblkMap(*mblk, 1, 1);
    vendorEventClass = mapped_data[0];
    CsrMblkUnmap(*mblk, mapped_data);

    /* BCCMD */
    if (HCI_BCCMD_HQ_VENDOR_OPCODE_LSB == vendorEventClass)
    {
        if (3 == CsrMblkReadHead(mblk, data, 3))
        {
            CsrUint8 channel, fragment;
            channel = data[2] & CHANNEL_ID_MASK;
            fragment = data[2] & ~CHANNEL_ID_MASK;
        
            CsrHciReassembleVendorSpecificEvents(inst,
                                                 channel,
                                                 fragment,
                                                 CSR_HCI_EV_VENDOR_SPECIFIC,
                                                 *mblk);
            *mblk = NULL;
        }
        else
        {
            CSR_LOG_TEXT_WARNING((LHci, 0,
                                  "csrHciIncomingDataProcess for BCCMD class failed to read HCI vendor specific event header out of mblk"));
        }
    }
    else
#endif /* !EXCLUDE_CSR_BCCMD_MODULE */
    {
        /* vendor specific command defined by QCA chip is quiet different 
           with bluecore chip, no fragment bit and channel bit are involved. */
        CsrHciReassembleVendorSpecificEvents(inst,
                                             TRANSPORT_CHANNEL_QC_HCIVS,
                                             CSR_HCI_UNFRAGMENTED,
                                             CSR_HCI_EV_VENDOR_SPECIFIC,
                                             *mblk);
        *mblk = NULL;
    }
}
#endif /* CSR_USE_QCA_CHIP */


static void csrHciIncomingDataProcess(CsrHciInstData *inst, CsrUint8 chan,
    CsrMblk *mblk)
{
    CsrUint8 data[4];

    switch (chan)
    {
        case TRANSPORT_CHANNEL_ACL:
        {
            if (CsrMblkReadHead(&mblk, data, CSR_HCI_ACL_HDR_SIZE) == CSR_HCI_ACL_HDR_SIZE)
            {
                CsrUint16 handlePlusFlags, handle, length;
                CsrSchedQid queueId;

                CsrHciExtractAclHeader(data, &handlePlusFlags, &length);

                handle = handlePlusFlags & CSR_HCI_ACL_HANDLE_MASK;

                if ((queueId = CsrHciCheckHandlers(inst->hciAclHandler, handle)) != CSR_SCHED_QID_INVALID) /* This specifical aclhandle was registred for by a higher layer */
                {
#ifdef CSR_BLUECORE_ONOFF
                    if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                    {
                        CsrHciSendAclDataInd(handlePlusFlags, queueId, mblk, FALSE);
                        mblk = NULL;
                    }
                }
                else if (inst->hciAclMainHandler != CSR_SCHED_QID_INVALID) /* This is not registred for specificly but instead generally by a higher layer using aclHandle==0xFFFF in CSR_HCI_ACL_REGISTER_HANDLER_REQ */
                {
#ifdef CSR_BLUECORE_ONOFF
                    if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                    {
                        CsrHciSendAclDataInd(handlePlusFlags, inst->hciAclMainHandler, mblk, TRUE);
                        mblk = NULL;
                    }
                }
                else
                {
                    CSR_LOG_TEXT_WARNING((LHci, 0, "ACL data dst not found"));
                }
            }
            else
            {
                CSR_LOG_TEXT_WARNING((LHci, 0, "Read ACL header Fail"));
            }

            break;
        }

#ifndef CSR_TARGET_PRODUCT_WEARABLE
        case TRANSPORT_CHANNEL_SCO:
        {
            if (CsrMblkReadHead(&mblk, data, CSR_HCI_SCO_HDR_SIZE) == CSR_HCI_SCO_HDR_SIZE)
            {
                CsrUint16 handlePlusFlags, handle, length;
                CsrSchedQid queueId;

                CsrHciExtractScoHeader(data, &handlePlusFlags, &length);

                handle = handlePlusFlags & CSR_HCI_SCO_HANDLE_MASK;

                if ((queueId = CsrHciCheckHandlers(inst->hciScoHandler, handle)) != CSR_SCHED_QID_INVALID) /* This specifical aclhandle was registred for by a higher layer */
                {
#ifdef CSR_BLUECORE_ONOFF
                    if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                    {
                        CsrHciSendScoDataInd(handlePlusFlags, queueId, mblk);
                        mblk = NULL;
                    }
                }
                else
                {
                    CSR_LOG_TEXT_WARNING((LHci, 0,
                                          "csrHciIncomingDataProcess we received SCO data but have nowhere to send it"));
                }
            }
            else
            {
                CSR_LOG_TEXT_WARNING((LHci, 0,
                                      "csrHciIncomingDataProcess failed to read SCO header out of mblk"));
            }
            break;
        }
#endif /* #ifndef CSR_TARGET_PRODUCT_WEARABLE */
        case TRANSPORT_CHANNEL_HCI:
        {
            if (CsrMblkReadHead(&mblk, data, CSR_HCI_EVENT_CODE_SIZE) == CSR_HCI_EVENT_CODE_SIZE)
            {
#ifdef CSR_USE_QCA_CHIP
                csrHciListType *list = inst->hciVendorEventHandler;
                CsrSchedQid queueId = CsrHciCheckHandlers(list, TRANSPORT_CHANNEL_QC_HCIVS);

                if ((*data == 0xFF) && ((inst->state == CSR_HCI_STATE_IDLE) || (queueId != CSR_SCHED_QID_INVALID))) /* Vendor specific event */
#else
                if (*data == 0xFF) /* Vendor specific event */
#endif/*CSR_USE_QCA_CHIP*/
                {
#ifdef CSR_USE_QCA_CHIP
                    csrHciHandleQcaVendorEvents(inst, &mblk);
#else
                    if (CsrMblkReadHead(&mblk, data, 2) == 2)
                    {
                        CsrUint8 channel, fragment;
                        channel = data[1] & CHANNEL_ID_MASK;
                        fragment = data[1] & ~CHANNEL_ID_MASK;

                        CsrHciReassembleVendorSpecificEvents(inst,
                                                             channel,
                                                             fragment,
                                                             CSR_HCI_EV_VENDOR_SPECIFIC,
                                                             mblk);
                        mblk = NULL;
                    }
                    else
                    {
                        CSR_LOG_TEXT_WARNING((LHci, 0,
                                              "csrHciIncomingDataProcess failed to read HCI vendor specific event header out of mblk"));
                    }
#endif/*CSR_USE_QCA_CHIP*/
                }
                else /* Normal HCI event forward to hciEventHandler (unless its a NOP event in which case it goes to the TM */
                {
                    CsrUint16 length;
                    CsrUint8 *payload;
                    length = CsrMblkGetLength(mblk);

#ifdef CSR_USE_QCA_CHIP
                    /* Check if the event type is command complete */
                    if (*data == CSR_HCI_EV_COMMAND_COMPLETE)
                    {
                        csrHciListType *list = inst->hciVendorEventHandler;
                        CsrSchedQid queueId = CsrHciCheckHandlers(list, TRANSPORT_CHANNEL_QC_HCIVS);
                        if ((inst->state == CSR_HCI_STATE_IDLE) ||  (queueId != CSR_SCHED_QID_INVALID))
                        {
                            CsrUint8 *mapped_data;
                            CsrUint8 ogf;

                            /* QCA Command complete : 0x0E | len | credit | opcode lsb | opcode msb |  */

                            /* mblk here will be pointing from lenght field, so at offset 3 is where the
                                ogf would need to be fetched */
                            mapped_data = CsrMblkMap(mblk, 3, 1);
                            ogf = mapped_data[0] >> 2;
                            CsrMblkUnmap(mblk, mapped_data);
                            if (ogf == 0x3f)
                            {
                                CsrHciReassembleVendorSpecificEvents(inst,
                                                                     TRANSPORT_CHANNEL_QC_HCIVS,
                                                                     CSR_HCI_UNFRAGMENTED,
                                                                     CSR_HCI_EV_COMMAND_COMPLETE,
                                                                     mblk);
                                mblk = NULL;
                                break;
                            }
                        }
                    }
#ifndef CSR_TARGET_PRODUCT_WEARABLE
                    else if (*data == CSR_HCI_EV_NUM_OF_COMP_PKTS)
                    {
                        CsrUint8 *evt, *pkt;
                        pkt = CsrPmemAlloc(length + 1);
                        evt = CsrMblkMap(mblk, 0, length);
                        pkt[0] = *data;
                        SynMemCpyS(pkt+1, length, evt, length);
                        CsrMblkUnmap(mblk, evt);
                        CsrHciIsoHandleNumOfCompPkts(pkt);
                    }
#endif
#endif
                    payload = CsrPmemAlloc(length + 1);
                    /* Reinsert the event code before forwarding it all upwards */
                    payload[0] = data[0];

                    CsrMblkCopyToMemory(mblk, 0, length, &payload[1]);

#ifndef CSR_USE_QCA_CHIP
                    /* For QC chips - 
                       1) NOP indication is not sent from controller to host 
                          after a reset
                       2) When a Transport sync loss occurs on QC controller,
                          it doesn't send sync loss error*/
                    if (((payload[0] == CSR_HCI_EV_COMMAND_STATUS) && ((length + 1) >= 6) && (payload[4] == 0x00) && (payload[5] == 0x00)) ||
                        ((payload[0] == CSR_HCI_EV_COMMAND_COMPLETE) && ((length + 1) >= 5) && (payload[3] == 0x00) && (payload[4] == 0x00)))
                    { /* A HCI NOP message is received - Note: length check uses (length+1) since length is computed from data excluding the event code */
                        CsrHciSendTmNopInd(inst, payload, (CsrUint16) (length + 1));
                    }
                    else if (checkHwErrSyncLoss(payload))
                    {
                        /* H4 link Sync failure.*/
                        CSR_LOG_TEXT_INFO((LHci, 0, "Received Sync loss from Controller"));
                        CsrPmemFree(payload);
                        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_H4_SYNC_LOST, "Chip H4 Link Sync Failure");
                    }
                    else
#endif
                    {
#ifdef CSR_USE_QCA_CHIP /*Block CC till bootstrap*/
                        if ((inst->state == CSR_HCI_STATE_IDLE) && 
                           ((payload[0] == CSR_HCI_EV_COMMAND_COMPLETE) && 
                             ((length + 1) >= 5) &&
                             (payload[3] == 0x00) &&
                             (payload[4] == 0x00)))
                        { /* Don't send Command complete packets above till bootstrapping is complete
                             Note: length check uses (length+1) since length is 
                             computed from data excluding the event code */
                            CSR_LOG_TEXT_INFO((LHci, 0, "Rcvd CC from in IDLE "));
                            /*CsrPmemFree(payload);*/
                        }
                        else if (inst->hciEventHandler != CSR_SCHED_QID_INVALID)
#else /* CSR_USE_QCA_CHIP */
                        if (inst->hciEventHandler != CSR_SCHED_QID_INVALID) /* Block CC till bootstrap */
#endif /* CSR_USE_QCA_CHIP */
                        {
#ifdef CSR_BLUECORE_ONOFF
                            if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                            {
#ifdef CSR_USE_QCA_CHIP
                                if ((inst->state == CSR_HCI_STATE_IDLE) && 
                                     hciCheckHciResetCmdComplete(payload,(length+1)))
                                {
                                    /* Command complete for HCI RESET is received. 
                                    Notify TM that QCA chip is ready. */
                                    CsrHciSendTmResetInd(inst);
                                }
                                else
                                {
                                    CsrHciSendEventInd(inst->hciEventHandler, payload, (CsrUint16) (length + 1));
                                    payload = NULL;
                                }
#else /* CSR_USE_QCA_CHIP */
                                CsrHciSendEventInd(inst->hciEventHandler, payload, (CsrUint16) (length + 1));
                                payload = NULL;
#endif /* CSR_USE_QCA_CHIP */
                            }
                        }
                        else
                        {
                            CSR_LOG_TEXT_WARNING((LHci, 0, "HCI event dst not found"));
                        }
                        CsrPmemFree(payload);
                    }
                }
            }
            else
            {
                CSR_LOG_TEXT_WARNING((LHci, 0, "Read HCI Event header Fail"));
            }
            break;
        }
        case TRANSPORT_CHANNEL_PERI:
        {
            CsrUint16 length;
            CsrUint8 *payload;
            length = CsrMblkGetLength(mblk);
            payload = CsrPmemAlloc(length);

            CsrMblkCopyToMemory(mblk, 0, length, &payload[0]);

            CsrPeriSendEventInd(inst->periEventHandler, payload, (CsrUint16) (length));
            payload = NULL;
            break;
        }
#ifndef CSR_USE_QCA_CHIP
        case TRANSPORT_CHANNEL_BCCMD:
        case TRANSPORT_CHANNEL_HQ:
        case TRANSPORT_CHANNEL_DM:
        case TRANSPORT_CHANNEL_L2CAP:
        case TRANSPORT_CHANNEL_RFCOMM:
        case TRANSPORT_CHANNEL_SDP:
        case TRANSPORT_CHANNEL_DFU:
        case TRANSPORT_CHANNEL_VM:
        { /* Can only be sent here by BCSP */
            CsrSchedQid queueId = CsrHciCheckHandlers(inst->hciVendorEventHandler, chan);

            if (queueId != CSR_SCHED_QID_INVALID) /* This specific channel was registred to receive data on by a higher layer */
            {
#ifdef CSR_BLUECORE_ONOFF
                if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                {
                    CsrHciSendVendorSpecificEventInd(queueId, chan, mblk);
                    mblk = NULL;
                }
            }
            else
            {
                CSR_LOG_TEXT_WARNING((LHci, 0,
                                      "csrHciIncomingDataProcess we received incoming data on a BCSP channel but we have nowhere to send it"));
            }
            break;
        }
#endif /* !CSR_USE_QCA_CHIP */
        default:
        {
            CSR_LOG_TEXT_WARNING((LHci, 0, "unexpected data rcvd on channel %x", chan));
            break;
        }
    }

    if (mblk != NULL)
    {
        CsrMblkDestroy(mblk);
    }
}

#else
static void csrHciIncomingDataProcess(CsrHciInstData *inst, CsrUint8 chan,
    CsrMblk *mblk)
{
    CsrUint8 data[4];

    switch (chan)
    {
        case TRANSPORT_CHANNEL_ACL:
        {
            if (CsrMblkReadHead(&mblk, data, CSR_HCI_ACL_HDR_SIZE) == CSR_HCI_ACL_HDR_SIZE)
            {
                CsrUint16 handlePlusFlags, handle, length;
                CsrSchedQid queueId;

                CsrHciExtractAclHeader(data, &handlePlusFlags, &length);

                handle = handlePlusFlags & CSR_HCI_ACL_HANDLE_MASK;

                if ((queueId = CsrHciCheckHandlers(inst->hciAclHandler, handle)) != CSR_SCHED_QID_INVALID) /* This specifical aclhandle was registred for by a higher layer */
                {
#ifdef CSR_BLUECORE_ONOFF
                    if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                    {
                        CsrHciSendAclDataInd(handlePlusFlags, queueId, mblk, FALSE);
                        mblk = NULL;
                    }
                }
                else if (inst->hciAclMainHandler != CSR_SCHED_QID_INVALID) /* This is not registred for specificly but instead generally by a higher layer using aclHandle==0xFFFF in CSR_HCI_ACL_REGISTER_HANDLER_REQ */
                {
#ifdef CSR_BLUECORE_ONOFF
                    if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                    {
                        CsrHciSendAclDataInd(handlePlusFlags, inst->hciAclMainHandler, mblk, TRUE);
                        mblk = NULL;
                    }
                }
                else
                {
                    CSR_LOG_TEXT_WARNING((LHci, 0, "ACL data dst not found"));
                }
            }
            else
            {
                CSR_LOG_TEXT_WARNING((LHci, 0, "Read ACL header Fail"));
            }

            break;
        }

        case TRANSPORT_CHANNEL_HCI:
        {
            if (CsrMblkReadHead(&mblk, data, CSR_HCI_EVENT_CODE_SIZE) == CSR_HCI_EVENT_CODE_SIZE)
            {
                CsrUint16 length;
                CsrUint8 *payload;
                length = CsrMblkGetLength(mblk);

#ifdef CSR_USE_QCA_CHIP
                /* Check if the event type is command complete */
                if (*data == CSR_HCI_EV_COMMAND_COMPLETE)
                {
                    csrHciListType *list = inst->hciVendorEventHandler;
                    CsrSchedQid queueId = CsrHciCheckHandlers(list, TRANSPORT_CHANNEL_QC_HCIVS);
                    if ((inst->state == CSR_HCI_STATE_IDLE) ||  (queueId != CSR_SCHED_QID_INVALID))
                    {
                        CsrUint8 *mapped_data;
                        CsrUint8 ogf;

                        /* QCA Command complete : 0x0E | len | credit | opcode lsb | opcode msb |  */

                        /* mblk here will be pointing from lenght field, so at offset 3 is where the
                            ogf would need to be fetched */
                        mapped_data = CsrMblkMap(mblk, 3, 1);
                        ogf = mapped_data[0] >> 2;
                        CsrMblkUnmap(mblk, mapped_data);
                        if (ogf == 0x3f)
                        {
                            CsrHciReassembleVendorSpecificEvents(inst,
                                                                 TRANSPORT_CHANNEL_QC_HCIVS,
                                                                 CSR_HCI_UNFRAGMENTED,
                                                                 CSR_HCI_EV_COMMAND_COMPLETE,
                                                                 mblk);
                            mblk = NULL;
                            break;
                        }
                    }
                }
#endif
                payload = CsrPmemAlloc(length + 1);
                /* Reinsert the event code before forwarding it all upwards */
                payload[0] = data[0];

                CsrMblkCopyToMemory(mblk, 0, length, &payload[1]);

                if (inst->hciEventHandler != CSR_SCHED_QID_INVALID) /* Block CC till bootstrap */
                {
#ifdef CSR_BLUECORE_ONOFF
                    if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                    {
                        CsrHciSendEventInd(inst->hciEventHandler, payload, (CsrUint16) (length + 1));
                        payload = NULL;
                    }
                }
                else
                {
                    CSR_LOG_TEXT_WARNING((LHci, 0, "HCI event dst not found"));
                }
                CsrPmemFree(payload);
            }
            else
            {
                CSR_LOG_TEXT_WARNING((LHci, 0, "Read HCI Event header Fail"));
            }
            break;
        }
        case TRANSPORT_CHANNEL_PERI:
        {
            CsrUint16 length;
            CsrUint8 *payload;
            length = CsrMblkGetLength(mblk);
            payload = CsrPmemAlloc(length);

            CsrMblkCopyToMemory(mblk, 0, length, &payload[0]);

            CsrPeriSendEventInd(inst->periEventHandler, payload, (CsrUint16) (length));
            payload = NULL;
            break;
        }
        default:
        {
            CSR_LOG_TEXT_WARNING((LHci, 0, "unexpected data rcvd on channel %x", chan));
            break;
        }
    }

    if (mblk != NULL)
    {
        CsrMblkDestroy(mblk);
    }
}
#endif

/***********************************************************************
CsrHciSendScoData
***********************************************************************/
void CsrHciSendScoData(CsrUint8 *theData)
{
    CSR_LOG_BCI(TRANSPORT_CHANNEL_SCO, FALSE, ((CsrUint8) theData[2]) + 3, theData);

    CsrTmBlueCoreTransportScoTx(thdl, theData);
}

#define ISO_HDR_SIZE 4

/***********************************************************************
CsrHciSendIsoPacket
***********************************************************************/
void CsrHciSendIsoPacket(CsrUint8 *theData)
{
    CsrUint16 isoDataLoadLength;

    isoDataLoadLength = theData[3] + ((theData[4] & 0x3F) << 8);
    CSR_LOG_BCI(TRANSPORT_CHANNEL_ACL /*TRANSPORT_CHANNEL_ISO*/, FALSE, isoDataLoadLength + ISO_HDR_SIZE, theData+1);

    CsrTmBlueCoreTransportIsoTx(thdl, theData);
}

static void csrHciTransportMsgRx(void *arg)
{
    MessageStructure *src;

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
    /* The purpose of this instrumentation is to attribute the time spent in
       this function to the HCI task rather than the calling context (usually
       the transport protocol driver). */
    CsrUint32 *previousMeasurements;
    previousMeasurements = CsrIpsInsert(CSR_IPS_CONTEXT_CURRENT, CsrSchedIpsMeasurementsGet(CSR_SCHED_IPS_CONTEXT_TASK(CSR_HCI_IFACEQUEUE)));
#endif

    src = (MessageStructure *) arg;

    CSR_LOG_BCI(src->chan, TRUE, src->buflen, src->buf);

    if (src->buflen > 0)
    {
        void *data;
        CsrMblk *mblk;

#ifndef CSR_TARGET_PRODUCT_WEARABLE
        if (src->chan == TRANSPORT_CHANNEL_SCO)
        {
            if (CsrHciLookForScoHandle(src->buf)) /* We still support function based SCO */
            {
                return;
            }
        }
        else if (src->chan == TRANSPORT_CHANNEL_ISO)
        {
            if (CsrHciLookForIsoHandle(src->buf))
            {
                return;
            }
        }
#endif

    /* Drop the enhanced logging PDUs after logging, not forwarded to stack */
    if ((src->chan == TRANSPORT_CHANNEL_ACL) && (CSR_HCI_GET_XAP_UINT16(src->buf) == BTSS_ENH_LOG_CONN_HANDLE))
    {
        return;
    }


#ifdef ENABLE_EVENT_COUNTER_REPORTING
        if(src->chan == TRANSPORT_CHANNEL_ACL)
        {
            CsrUint16 connHandle = CSR_HCI_GET_XAP_UINT16(src->buf) & CSR_HCI_ACL_HANDLE_MASK;
            extern CsrUint32 BtHostEventCounterGetCounterForGivenConnHandle(CsrUint16 connHandle);
            CsrUint32  metaData = BtHostEventCounterGetCounterForGivenConnHandle(connHandle);
            mblk = CsrMblkMallocCreateMeta(&data, (CsrMblkSize) src->buflen, (void *)&metaData, sizeof(CsrUint32));
        }
        else
#endif

        {
            mblk = CsrMblkMallocCreate(&data, (CsrMblkSize) src->buflen);
        }

        /* Added to address KW issue */
        if (data == NULL)
        {
            return;
        }

        SynMemCpyS(data, (CsrMblkSize) src->buflen, src->buf, (CsrMblkSize) src->buflen);

        csrHciIncomingDataProcess(globalInst, src->chan, mblk);
    }

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
    CsrIpsInsert(CSR_IPS_CONTEXT_CURRENT, previousMeasurements);
#endif
}

void *CsrHciTransportCreate(void *type)
{
    thdl = type;

    CsrTmBlueCoreTransportRegisterMsgRx(type, csrHciTransportMsgRx);
    return type;
}

void CsrHciInstanceRegister(CsrHciInstData *inst)
{
    globalInst = inst;
}

/* CSR_HCI downstream handler functions                                                */
void CsrHciRegisterEventHandlerReqHandler(CsrHciInstData *inst)
{
    CsrHciRegisterEventHandlerReq *prim = inst->recvMsgP;

    inst->hciEventHandler = prim->queueId;

    CsrHciSendRegisterEventHandlerCfm(prim->queueId);
}

void CsrPeriRegisterEventHandlerReqHandler(CsrHciInstData *inst)
{
    CsrPeriRegisterEventHandlerReq *prim = inst->recvMsgP;

    inst->periEventHandler = prim->queueId;

    CsrPeriSendRegisterEventHandlerCfm(prim->queueId);
}

void CsrHciRegisterAclHandlerReqHandler(CsrHciInstData *inst)
{
    CsrHciRegisterAclHandlerReq *prim = inst->recvMsgP;

    if (prim->aclHandle == 0xFFFF)
    {
        inst->hciAclMainHandler = prim->queueId;
    }
    else
    {
        CsrHciRegisterHandle(&(inst->hciAclHandler), prim->aclHandle, prim->queueId);
    }

    CsrHciSendRegisterAclHandlerCfm(prim->queueId, prim->aclHandle);
}

void CsrHciUnregisterAclHandlerReqHandler(CsrHciInstData *inst)
{
    CsrHciUnregisterAclHandlerReq *prim = inst->recvMsgP;

    if (prim->aclHandle == 0xFFFF)
    {
        inst->hciAclMainHandler = CSR_SCHED_QID_INVALID;
    }
    else
    {
        CsrHciUnregisterHandle(&(inst->hciAclHandler), prim->aclHandle, prim->queueId);
    }

    CsrHciSendUnregisterAclHandlerCfm(prim->queueId, prim->aclHandle);
}

#ifndef CSR_TARGET_PRODUCT_WEARABLE
void CsrHciRegisterScoHandlerReqHandler(CsrHciInstData *inst)
{
    CsrHciRegisterScoHandlerReq *prim = inst->recvMsgP;

    CsrHciRegisterHandle(&(inst->hciScoHandler), prim->scoHandle, prim->queueId);

    CsrHciSendRegisterScoHandlerCfm(prim->queueId, prim->scoHandle);
}

void CsrHciUnregisterScoHandlerReqHandler(CsrHciInstData *inst)
{
    CsrHciUnregisterScoHandlerReq *prim = inst->recvMsgP;

    CsrHciUnregisterHandle(&(inst->hciScoHandler), prim->scoHandle, prim->queueId);

    CsrHciSendUnregisterScoHandlerCfm(prim->queueId, prim->scoHandle);
}
#endif

void CsrHciRegisterVendorSpecificEventHandlerReqHandler(CsrHciInstData *inst)
{
    CsrHciRegisterVendorSpecificEventHandlerReq *prim = inst->recvMsgP;

#ifndef CSR_BLUECORE_ONOFF
    if (inst->state == CSR_HCI_STATE_IDLE) /* In idle state we only allow BCCMD and HQ to register because they need to bootstrap and receive error events from chip */
    {
        switch (prim->channel)
        {
#ifndef EXCLUDE_CSR_BCCMD_MODULE
            case TRANSPORT_CHANNEL_BCCMD:
            case TRANSPORT_CHANNEL_HQ:
#endif /* !EXCLUDE_CSR_BCCMD_MODULE */
#ifdef CSR_USE_QCA_CHIP
            case TRANSPORT_CHANNEL_QC_HCIVS:
#endif /* CSR_USE_QCA_CHIP */
                /* proceed */
                break;
            default:
                CsrHciSaveMessage(inst);
                return;
        }
    }
#endif /* CSR_BLUECORE_ONOFF */

    CsrHciRegisterHandle(&(inst->hciVendorEventHandler), prim->channel, prim->queueId);

    CsrHciSendRegisterVendorSpecificEventHandlerCfm(prim->queueId, prim->channel);
}

void CsrHciUnregisterVendorSpecificEventHandlerReqHandler(CsrHciInstData *inst)
{
    CsrHciUnregisterVendorSpecificEventHandlerReq *prim = inst->recvMsgP;

    CsrHciUnregisterHandle(&(inst->hciVendorEventHandler), prim->channel, prim->queueId);

    CsrHciSendUnregisterVendorSpecificEventHandlerCfm(prim->queueId, prim->channel);
}

void CsrHciCommandReqHandler(CsrHciInstData *inst)
{
    CsrHciCommandReq *prim = inst->recvMsgP;

#ifdef CSR_BLUECORE_ONOFF
    if (CSR_TM_BLUECORE_TRANSPORT_STARTED(thdl))
#endif
    {
        csrHciPrepareTransport(TRANSPORT_CHANNEL_HCI, TRANSPORT_RELIABLE_CHANNEL, prim->payload, prim->payloadLength);
    }
#ifdef CSR_BLUECORE_ONOFF
    else
    {
        CsrPmemFree(prim->payload);
    }
#endif
}

void CsrPeriCommandReqHandler(CsrHciInstData *inst)
{
    CsrPeriCommandReq *prim = inst->recvMsgP;

#ifdef CSR_BLUECORE_ONOFF
    if (CSR_TM_BLUECORE_TRANSPORT_STARTED(thdl))
#endif
    {
        csrHciPrepareTransport(TRANSPORT_CHANNEL_PERI, TRANSPORT_RELIABLE_CHANNEL, prim->payload, prim->payloadLength);
    }
#ifdef CSR_BLUECORE_ONOFF
    else
    {
        CsrPmemFree(prim->payload);
    }
#endif
}

void CsrHciAclDataReqHandler(CsrHciInstData *inst)
{
    CsrHciAclDataReq *prim = inst->recvMsgP;

#ifdef CSR_BLUECORE_ONOFF
    if (CSR_TM_BLUECORE_TRANSPORT_STARTED(thdl))
#endif
    {
        CsrUint16 length = CsrMblkGetLength(prim->data);
        if (prim->handlePlusFlags == ACL_SOURCE_PRIMARY_STACK)
        {            
            CsrUint8 *payload = CsrPmemAlloc(length-1);

            (void) CsrMblkCopyToMemory(prim->data, 1, (length-1), payload);
            csrHciPrepareTransport(TRANSPORT_CHANNEL_ACL, TRANSPORT_CHANNEL_ACL_PRIMARY_STACK, payload, length-1);
        }
        else
        {
            CsrUint8 *payload = CsrPmemAlloc(length + CSR_HCI_ACL_HDR_SIZE);
            (void) CsrMblkCopyToMemory(prim->data, 0, length, &(payload[CSR_HCI_ACL_HDR_SIZE]));
            CsrHciInsertAclHeader(payload, prim->handlePlusFlags, length);
            csrHciPrepareTransport(TRANSPORT_CHANNEL_ACL, TRANSPORT_CHANNEL_ACL, payload, length + CSR_HCI_ACL_HDR_SIZE);
        }
    }

    CsrMblkDestroy(prim->data);
    prim->data = NULL;
}

#ifndef CSR_TARGET_PRODUCT_WEARABLE
void CsrHciScoDataReqHandler(CsrHciInstData *inst)
{
    CsrHciScoDataReq *prim = inst->recvMsgP;

#ifdef CSR_BLUECORE_ONOFF
    if (CSR_TM_BLUECORE_TRANSPORT_STARTED(thdl))
#endif
    {
        CsrUint16 length = CsrMblkGetLength(prim->data);
        CsrUint8 *payload = CsrPmemAlloc(length + CSR_HCI_SCO_HDR_SIZE);
        (void) CsrMblkCopyToMemory(prim->data, 0, length, &(payload[CSR_HCI_SCO_HDR_SIZE]));
        CsrHciInsertScoHeader(payload, prim->handlePlusFlags, length);
        csrHciPrepareTransport(TRANSPORT_CHANNEL_SCO, TRANSPORT_RELIABLE_CHANNEL, payload, length + CSR_HCI_SCO_HDR_SIZE);
    }

    CsrMblkDestroy(prim->data);
    prim->data = NULL;
}
#endif

void CsrHciVendorSpecificCommandReqHandler(CsrHciInstData *inst)
{
    CsrHciVendorSpecificCommandReq *prim = inst->recvMsgP;
    CsrUint16 length;

#ifdef CSR_BLUECORE_ONOFF
    if (!CSR_TM_BLUECORE_TRANSPORT_STARTED(thdl))
    {
        /* Discard everything when transport is not started */
        CsrMblkDestroy(prim->data);
        prim->data = NULL;
        return;
    }
#else
    if (inst->state == CSR_HCI_STATE_IDLE) /* In idle state we only allow BCCMD and HQ to register because they need to bootstrap and receive error events from chip */
    {
        switch (prim->channel)
        {
#ifndef EXCLUDE_CSR_BCCMD_MODULE
            case TRANSPORT_CHANNEL_BCCMD:
            case TRANSPORT_CHANNEL_HQ:
#endif /* !EXCLUDE_CSR_BCCMD_MODULE */
#ifdef CSR_USE_QCA_CHIP
            case TRANSPORT_CHANNEL_QC_HCIVS:
#endif /* CSR_USE_QCA_CHIP */
                /* proceed */
                break;
            default:
                CsrHciSaveMessage(inst);
                return;
        }
    }
#endif

    length = CsrMblkGetLength(prim->data);

    if ((prim->channel < 16) && (CsrTmBlueCoreTransportQuery(thdl) == TRANSPORT_TYPE_BCSP))
    {
        CsrUint8 *payload;
        /* On BCSP channels 0-15 are mapped as logical channels in the transport protocol so we do not send those as vendor specific HCI cmds */
        payload = CsrPmemAlloc(length);

        (void) CsrMblkCopyToMemory(prim->data, 0, length, payload);

        csrHciPrepareTransport(prim->channel, TRANSPORT_RELIABLE_CHANNEL, payload, length);
    }
    else if (prim->channel <= 63) /* On all other transports than BCSP, the channels 0-15 are sent as vendor specific HCI commands,
         and for all transports incl BCSP the channels 16-63 are always routed as vendor specific HCI commands */
    {
        CsrUint8 *hciExtCmd;
        CsrUint16 count = 0;

        if (length <= CSR_HCI_MAX_HCI_CMD_PAYLOAD_SIZE) /* MAX HCI cmd packet size */
        {
            CsrUint8 hciVendorOpcodeLsb = 0x00;

#ifdef CSR_USE_QCA_CHIP
#ifndef EXCLUDE_CSR_BCCMD_MODULE
            if (prim->channel == TRANSPORT_CHANNEL_BCCMD ||
                prim->channel == TRANSPORT_CHANNEL_HQ)
            {
                hciVendorOpcodeLsb = HCI_BCCMD_HQ_VENDOR_OPCODE_LSB;
            }
            else
#endif /* !EXCLUDE_CSR_BCCMD_MODULE */
            if (prim->channel == TRANSPORT_CHANNEL_QC_HCIVS)
            {
                /* For QCA defined vendor specific command, no fragment bit 
                    and channel bit are involved. */
                hciExtCmd = (CsrUint8 *) CsrPmemAlloc(length);
                (void) CsrMblkCopyToMemory(prim->data, 0, length, hciExtCmd);
                csrHciPrepareTransport(TRANSPORT_CHANNEL_HCI, TRANSPORT_RELIABLE_CHANNEL, hciExtCmd, length);
            }
            else
#endif /* CSR_USE_QCA_CHIP */
            {
                hciExtCmd = (CsrUint8 *) CsrPmemAlloc(length + CSR_HCI_COMMAND_VENDOR_HDR_SIZE);
                hciExtCmd[0] = hciVendorOpcodeLsb;
                hciExtCmd[1] = 0xFC;
                hciExtCmd[2] = length + 1;
                hciExtCmd[3] = CSR_HCI_UNFRAGMENTED | prim->channel; /* unfragmented packet, tunneled channel id */

                (void) CsrMblkCopyToMemory(prim->data, 0, length, &hciExtCmd[CSR_HCI_COMMAND_VENDOR_HDR_SIZE]);

#ifdef CSR_LOG_ENABLE
                if (CsrHciChannelLog(prim->channel))
                {
                    CSR_LOG_BCI(prim->channel,
                                FALSE,
                                length,
                                &hciExtCmd[CSR_HCI_COMMAND_VENDOR_HDR_SIZE]);
                }
#endif
                csrHciPrepareTransport(TRANSPORT_CHANNEL_HCI, TRANSPORT_RELIABLE_CHANNEL, hciExtCmd, length + CSR_HCI_COMMAND_VENDOR_HDR_SIZE);
            }
        }
        else    /* fragmentation is required, packet too big */
        {
            CsrBool firstFragment = TRUE;
            CSR_LOG_TEXT_CONDITIONAL_ERROR((prim->channel == TRANSPORT_CHANNEL_QC_HCIVS),
                                           (LHci, 0,
                                           "payload length exceeds CSR_HCI_MAX_HCI_CMD_PAYLOAD_SIZE"));
            while (length > 0)
            {
                if (length > CSR_HCI_MAX_HCI_CMD_PAYLOAD_SIZE)
                {
                    hciExtCmd = (CsrUint8 *) CsrPmemAlloc(CSR_HCI_MAX_HCI_CMD_PAYLOAD_SIZE + CSR_HCI_COMMAND_VENDOR_HDR_SIZE);
                    hciExtCmd[0] = 0x00;
                    hciExtCmd[1] = 0xFC;
                    hciExtCmd[2] = CSR_HCI_MAX_HCI_CMD_PAYLOAD_SIZE + 1;

                    if (firstFragment)
                    {
                        hciExtCmd[3] = CSR_HCI_FRAGMENT_START | prim->channel; /* first fragment packet, tunneled channel id */
                        firstFragment = FALSE;
                        count = 0;
                    }
                    else
                    {
                        hciExtCmd[3] = prim->channel; /* continuation packet, tunneled channel id */
                    }

                    (void) CsrMblkCopyToMemory(prim->data, count, length, &hciExtCmd[CSR_HCI_COMMAND_VENDOR_HDR_SIZE]);

#ifdef CSR_LOG_ENABLE
                    if (CsrHciChannelLog(prim->channel))
                    {
                        CSR_LOG_BCI(prim->channel,
                                    FALSE,
                                    length,
                                    &hciExtCmd[CSR_HCI_COMMAND_VENDOR_HDR_SIZE]);
                    }
#endif
                    csrHciPrepareTransport(TRANSPORT_CHANNEL_HCI, TRANSPORT_RELIABLE_CHANNEL, hciExtCmd, length + CSR_HCI_COMMAND_VENDOR_HDR_SIZE);

                    length -= CSR_HCI_MAX_HCI_CMD_PAYLOAD_SIZE;
                    count += CSR_HCI_MAX_HCI_CMD_PAYLOAD_SIZE;
                }
                else
                {
                    hciExtCmd = (CsrUint8 *) CsrPmemAlloc(length + CSR_HCI_COMMAND_VENDOR_HDR_SIZE);
                    hciExtCmd[0] = 0x00;
                    hciExtCmd[1] = 0xFC;
                    hciExtCmd[2] = length + 1;
                    hciExtCmd[3] = CSR_HCI_FRAGMENT_END | prim->channel; /*  last fragmented packet, tunneled channel id */

                    (void) CsrMblkCopyToMemory(prim->data, count, length, &hciExtCmd[CSR_HCI_COMMAND_VENDOR_HDR_SIZE]);

#ifdef CSR_LOG_ENABLE
                    if (CsrHciChannelLog(prim->channel))
                    {
                        CSR_LOG_BCI(prim->channel,
                                    FALSE,
                                    length,
                                    &hciExtCmd[CSR_HCI_COMMAND_VENDOR_HDR_SIZE]);
                    }
#endif
                    csrHciPrepareTransport(TRANSPORT_CHANNEL_HCI, TRANSPORT_RELIABLE_CHANNEL, hciExtCmd, length + CSR_HCI_COMMAND_VENDOR_HDR_SIZE);

                    length = 0;
                }
            }
        }
    }
    else
    {
        CSR_LOG_TEXT_WARNING((LHci, 0, "VSC send data on ch no > 63 is illegal"));
    }

    CsrMblkDestroy(prim->data);
    prim->data = NULL;
}

void CsrHciResetTransportReqHandler(CsrHciInstData *inst)
{
    CsrHciResetTransportReq *prim = inst->recvMsgP;
    CsrBool restarted = CsrTmBlueCoreTransportDriverRestart(thdl, prim->baudId);

    CsrHciSendResetTransportCfm(prim->queueId, restarted);
}

void CsrHciSendEventIndToRegisteredHciEventHandler(CsrUint8 *payload, CsrUint16 length)
{
    CsrHciSendEventInd(globalInst->hciEventHandler, payload, length);
}

#if defined(CSR_CHIP_MANAGER_TEST_ENABLE) && !defined(CSR_USE_QCA_CHIP)
/* This function is needed to send a COLD RESET without using the queues in the bccmd task,
   when testing the chip manager. (implemented in csr_hci_sef.c) */
void CsrBcCmTestHciDriverSendBcCommand(CsrUint8 *msg, CsrUint32 msg_len)
{
    csrHciPrepareTransport(TRANSPORT_CHANNEL_BCCMD, TRANSPORT_RELIABLE_CHANNEL, msg, msg_len);
}
#endif /* CSR_CHIP_MANAGER_TEST_ENABLE && !CSR_USE_QCA_CHIP */

