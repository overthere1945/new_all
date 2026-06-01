/******************************************************************************
Copyright (c) 2009-2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_hci_prim.h"
#include "csr_hci_lib.h"
#include "csr_hci_util.h"
#include "csr_hci_handler.h"
#include "csr_pmem.h"
#include "csr_transport.h"
#include "csr_hci_private_prim.h"
#include "csr_tm_bluecore_task.h"
#include "csr_hci_handler.h"
#include "csr_hci_sef.h"

#include "csr_log_text_2.h"
#include "csr_log_gsp.h"

void CsrHciRegisterHandle(csrHciListType **main_list, CsrUint16 handle, CsrSchedQid queueId)
{
    csrHciListType *newEntry;

    newEntry = CsrHciGetHandler(*main_list, handle);
    if (newEntry != NULL)
    {
        /* Reregister handle from one queue to another not allowed */
        CSR_LOG_TEXT_CONDITIONAL_ERROR(newEntry->queueId != queueId,
            (LHci, 0, "Reregister handle %u from diff Q disallowed", handle));
    }
    else
    {
        newEntry = CsrPmemAlloc(sizeof(csrHciListType));
        newEntry->handle = handle;
        newEntry->queueId = queueId;
        newEntry->data = NULL;
        newEntry->next = *main_list;
        *main_list = newEntry;
    }
}

void CsrHciUnregisterHandle(csrHciListType **main_list, CsrUint16 handle, CsrSchedQid queueId)
{
    csrHciListType *list = *main_list, *previous;

    for (previous = list; list; list = list->next)
    {
        if (handle == list->handle)
        {
            if (queueId != list->queueId)
            {
                CSR_LOG_TEXT_ERROR((LHci, 0,
                    "Unregister handle %u from another queue not allowed)", handle));
            }
            else
            {
                if (list == *main_list)
                {
                    *main_list = list->next;
                }
                else
                {
                    previous->next = list->next;
                }
                CsrPmemFree(list);
            }
            break;
        }
        previous = list;
    }
}

void CsrHciUnregisterAllHandles(csrHciListType **main_list)
{
    while (*main_list)
    {
        csrHciListType *next = (*main_list)->next;
        CsrMblkDestroy((*main_list)->data);
        CsrPmemFree(*main_list);
        *main_list = next;
    }
}

void CsrHciSendRegisterEventHandlerCfm(CsrSchedQid phandle)
{
    CsrHciRegisterEventHandlerCfm *prim = CsrPmemAlloc(sizeof(CsrHciRegisterEventHandlerCfm));

    prim->type = CSR_HCI_REGISTER_EVENT_HANDLER_CFM;
    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

void CsrPeriSendRegisterEventHandlerCfm(CsrSchedQid phandle)
{
    CsrPeriRegisterEventHandlerCfm *prim = CsrPmemAlloc(sizeof(CsrPeriRegisterEventHandlerCfm));

    prim->type = CSR_PERI_REGISTER_EVENT_HANDLER_CFM;
    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

void CsrHciSendRegisterAclHandlerCfm(CsrSchedQid phandle, CsrUint16 aclHandle)
{
    CsrHciRegisterAclHandlerCfm *prim = CsrPmemAlloc(sizeof(CsrHciRegisterAclHandlerCfm));

    prim->type = CSR_HCI_REGISTER_ACL_HANDLER_CFM;
    prim->aclHandle = aclHandle;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

void CsrHciSendUnregisterAclHandlerCfm(CsrSchedQid phandle, CsrUint16 aclHandle)
{
    CsrHciUnregisterAclHandlerCfm *prim = CsrPmemAlloc(sizeof(CsrHciUnregisterAclHandlerCfm));

    prim->type = CSR_HCI_UNREGISTER_ACL_HANDLER_CFM;
    prim->aclHandle = aclHandle;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

#ifndef CSR_TARGET_PRODUCT_WEARABLE
void CsrHciSendRegisterScoHandlerCfm(CsrSchedQid phandle, CsrUint16 scoHandle)
{
    CsrHciRegisterScoHandlerCfm *prim = CsrPmemAlloc(sizeof(CsrHciRegisterScoHandlerCfm));

    prim->type = CSR_HCI_REGISTER_SCO_HANDLER_CFM;
    prim->scoHandle = scoHandle;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

void CsrHciSendUnregisterScoHandlerCfm(CsrSchedQid phandle, CsrUint16 scoHandle)
{
    CsrHciUnregisterScoHandlerCfm *prim = CsrPmemAlloc(sizeof(CsrHciUnregisterScoHandlerCfm));

    prim->type = CSR_HCI_UNREGISTER_SCO_HANDLER_CFM;
    prim->scoHandle = scoHandle;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}
#endif

void CsrHciSendRegisterVendorSpecificEventHandlerCfm(CsrSchedQid phandle, CsrUint8 channel)
{
    CsrHciRegisterVendorSpecificEventHandlerCfm *prim = CsrPmemAlloc(sizeof(CsrHciRegisterVendorSpecificEventHandlerCfm));

    prim->type = CSR_HCI_REGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_CFM;
    prim->channel = channel;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

void CsrHciSendUnregisterVendorSpecificEventHandlerCfm(CsrSchedQid phandle, CsrUint8 channel)
{
    CsrHciUnregisterVendorSpecificEventHandlerCfm *prim = CsrPmemAlloc(sizeof(CsrHciUnregisterVendorSpecificEventHandlerCfm));

    prim->type = CSR_HCI_UNREGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_CFM;
    prim->channel = channel;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

void CsrHciSendAclDataInd(CsrUint16 handlePlusFlags, CsrSchedQid phandle, CsrMblk *data, CsrBool functionPotential)
{
    CsrHciAclDataInd *prim = CsrPmemAlloc(sizeof(CsrHciAclDataInd));

    prim->type = CSR_HCI_ACL_DATA_IND;
    prim->handlePlusFlags = handlePlusFlags;
    prim->data = data;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

#ifndef CSR_TARGET_PRODUCT_WEARABLE
void CsrHciSendScoDataInd(CsrUint16 handlePlusFlags, CsrSchedQid phandle, CsrMblk *data)
{
    CsrHciScoDataInd *prim = CsrPmemAlloc(sizeof(CsrHciScoDataInd));

    prim->type = CSR_HCI_SCO_DATA_IND;
    prim->handlePlusFlags = handlePlusFlags;
    prim->data = data;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}
#endif

void CsrHciSendEventInd(CsrSchedQid phandle, CsrUint8 *payload, CsrUint16 length)
{
    CsrHciEventInd *prim = CsrPmemAlloc(sizeof(CsrHciEventInd));

    prim->type = CSR_HCI_EVENT_IND;
    prim->payload = payload;
    prim->payloadLength = length;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

void CsrPeriSendEventInd(CsrSchedQid phandle, CsrUint8 *payload, CsrUint16 length)
{
    CsrPeriEventInd *prim = CsrPmemAlloc(sizeof(CsrPeriEventInd));

    prim->type = CSR_PERI_EVENT_IND;
    prim->payload = payload;
    prim->payloadLength = length;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

void CsrHciSendVendorSpecificEventIndExt(CsrSchedQid phandle,
                                         CsrUint8 channel,
                                         CsrUint8 event,
                                         CsrMblk* data)
{
    CsrHciVendorSpecificEventInd* prim = CsrPmemZalloc(sizeof(CsrHciVendorSpecificEventInd));

    prim->type = CSR_HCI_VENDOR_SPECIFIC_EVENT_IND;
    prim->channel = channel;
    prim->data = data;
    prim->event = event;

#ifdef CSR_LOG_ENABLE
    if (CsrHciChannelLog(channel))
    {
        CsrUint16 length = CsrMblkGetLength(prim->data);
        CsrUint8 *mblkData = CsrMblkMap(prim->data, 0, length);

        CSR_LOG_BCI(prim->channel, FALSE, length, mblkData);

        CsrMblkUnmap(prim->data, mblkData);
    }
#endif

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

void CsrHciSendVendorSpecificEventInd(CsrSchedQid phandle, CsrUint8 channel, CsrMblk *data)
{
    CsrHciSendVendorSpecificEventIndExt(phandle,
                                        channel,
                                        CSR_HCI_EV_VENDOR_SPECIFIC,
                                        data);
}

void CsrHciSendResetTransportCfm(CsrSchedQid phandle, CsrBool restarted)
{
    CsrHciResetTransportCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_HCI_RESET_TRANSPORT_CFM;
    prim->restarted = restarted;

    CsrSchedMessagePut(phandle, CSR_HCI_PRIM, prim);
}

csrHciListType *CsrHciGetHandler(csrHciListType *list, CsrUint16 handle)
{
    /* check if we a registration for this handle */
    for (; list; list = list->next)
    {
        if (list->handle == handle) /* handle in list */
        {
            return list;
        }
    }

    return NULL;
}

CsrSchedQid CsrHciCheckHandlers(csrHciListType *list, CsrUint16 handle)
{
    /* check if we a registration for this handle */
    for (; list; list = list->next)
    {
        if (list->handle == handle) /* handle in list */
        {
            return list->queueId;
        }
    }

    return CSR_SCHED_QID_INVALID;
}

void CsrHciInsertAclHeader(CsrUint8 *payload, CsrUint16 handlePlusFlags, CsrUint16 length)
{
    CSR_HCI_SET_XAP_UINT16(payload, handlePlusFlags);
    CSR_HCI_SET_XAP_UINT16(&(payload[2]), length);
}

#ifndef CSR_TARGET_PRODUCT_WEARABLE
void CsrHciInsertScoHeader(CsrUint8 *payload, CsrUint16 handlePlusFlags, CsrUint16 length)
{
    CSR_HCI_SET_XAP_UINT16(payload, handlePlusFlags);
    payload[2] = (CsrUint8)length;
}
#endif

void CsrHciExtractAclHeader(CsrUint8 *payload, CsrUint16 *handlePlusFlags, CsrUint16 *length)
{
    *handlePlusFlags = CSR_HCI_GET_XAP_UINT16(payload);
    *length = CSR_HCI_GET_XAP_UINT16(&payload[2]);
}

#ifndef CSR_TARGET_PRODUCT_WEARABLE
void CsrHciExtractScoHeader(CsrUint8 *payload, CsrUint16 *handlePlusFlags, CsrUint16 *length)
{
    *handlePlusFlags = CSR_HCI_GET_XAP_UINT16(payload);
    *length = payload[2];
}
#endif

CsrBool CsrHciChannelLog(CsrUint8 channel)
{
#ifndef EXCLUDE_CSR_BCCMD_MODULE
    if (channel == TRANSPORT_CHANNEL_BCCMD || channel == TRANSPORT_CHANNEL_HQ)
    {
        return (TRUE);
    }
    else
#endif /* !EXCLUDE_CSR_BCCMD_MODULE */
#ifndef CSR_USE_QCA_CHIP
    if (channel == TRANSPORT_CHANNEL_VM)
    {
        return (TRUE);
    }
    else
#endif /* CSR_USE_QCA_CHIP */
    {
        CSR_UNUSED(channel);
        return (FALSE);
    }
}

#ifndef CSR_BLUECORE_ONOFF
void CsrHciSaveMessage(CsrHciInstData *inst)
{
    CsrMessageQueuePush(&inst->saveQueue, CSR_HCI_PRIM, inst->recvMsgP);
    inst->recvMsgP = NULL;
}

void CsrHciRestoreSavedMessages(CsrHciInstData *inst)
{
    inst->state = CSR_HCI_STATE_ACTIVATED;
    if (inst->saveQueue != NULL)
    {
        inst->restoreFlag = TRUE;
        CsrSchedMessagePut(CSR_HCI_IFACEQUEUE, CSR_HCI_PRIM, NULL);
    }
}

#ifdef ENABLE_SHUTDOWN
void CsrHciDiscardSavedMessages(CsrHciInstData *inst)
{
    void *message;
    CsrUint16 event;
    while (CsrMessageQueuePop(&inst->saveQueue, &event, &message))
    {
        CSR_LOG_TEXT_ASSERT(LHci, 0, event == CSR_HCI_PRIM);
        CsrHciFreeDownstreamMessageContents(CSR_HCI_PRIM, message);
        CsrPmemFree(message);
    }
}

#endif /* ENABLE_SHUTDOWN */
#endif /* !CSR_BLUECORE_ONOFF */

void CsrHciReassembleVendorSpecificEvents(CsrHciInstData *inst,
                                          CsrUint8 channel,
                                          CsrUint8 fragment,
                                          CsrUint8 event,
                                          CsrMblk *data)
{
    csrHciListType *list = inst->hciVendorEventHandler;
    CsrSchedQid queueId = CsrHciCheckHandlers(list, channel);

    if (queueId != CSR_SCHED_QID_INVALID) /* This specific channel was registred to receive data on by a higher layer */
    {
        switch (fragment)
        {
            case CSR_HCI_UNFRAGMENTED:
            {
#ifdef CSR_BLUECORE_ONOFF
                if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                {
                    CsrHciSendVendorSpecificEventIndExt(queueId, channel, event, data);
                }
#ifdef CSR_BLUECORE_ONOFF
                else
                {
                    CsrMblkDestroy(data);
                }
#endif
                break;
            }
//#if (CSR_HOST_PLATFORM == Q6_HOST) 
#ifndef CSR_TARGET_PRODUCT_WEARABLE

            case CSR_HCI_FRAGMENT_START:
            {
                csrHciListType *channelHandler = CsrHciGetHandler(list, channel);
                if (channelHandler)
                {
                    channelHandler->data = data;
                }
                else
                {
                    CsrMblkDestroy(data);
                }
                break;
            }
            case CSR_HCI_FRAGMENT_END:
            {
                csrHciListType *channelHandler = CsrHciGetHandler(list, channel);
                if (channelHandler)
                {
                    CsrMblkAddTail(data, channelHandler->data);

#ifdef CSR_BLUECORE_ONOFF
                    if (inst->state != CSR_HCI_STATE_DEACTIVATING)
#endif
                    {
                        CsrHciSendVendorSpecificEventInd(queueId, channel, channelHandler->data);
                    }
#ifdef CSR_BLUECORE_ONOFF
                    else
                    {
                        CsrMblkDestroy(channelHandler->data);
                    }
#endif
                    channelHandler->data = NULL;
                }
                else
                {
                    CsrMblkDestroy(data);
                }
                break;
            }
#endif /* CSR_TARGET_PRODUCT_WEARABLE */
            default:
            { /* Continuation */
                csrHciListType *channelHandler = CsrHciGetHandler(list, channel);
                if (channelHandler)
                {
                    CsrMblkAddTail(data, channelHandler->data);
                }
                else
                {
                    CsrMblkDestroy(data);
                }
                break;
            }
        }
    }
    else
    {
        CsrMblkDestroy(data);
        /* we received an HCI Vendor Specific Event but have nowhere to send it */
        CSR_LOG_TEXT_WARNING((LHci, 0, "ReassmbleVSCEvents dst not found"));
    }
}


#ifndef CSR_USE_QCA_CHIP
void CsrHciSendTmNopInd(CsrHciInstData *inst, CsrUint8 *payload, CsrUint16 payloadLength)
{
        CsrHciNopInd *prim = CsrPmemAlloc(sizeof(CsrHciNopInd));
        prim->type = CSR_HCI_NOP_IND;
    prim->phandle = inst->hciEventHandler;
        prim->payload = payload;
        prim->payloadLength = payloadLength;
        CsrSchedMessagePut(CSR_TM_BLUECORE_IFACEQUEUE, CSR_HCI_PRIM, prim);
    }

#else

void CsrHciSendTmResetInd(CsrHciInstData *inst)
{
        CsrHciResetInd *prim = CsrPmemAlloc(sizeof(CsrHciResetInd));
        prim->type = CSR_HCI_RESET_IND;
    prim->phandle = inst->hciEventHandler;
        CsrSchedMessagePut(CSR_TM_BLUECORE_IFACEQUEUE, CSR_HCI_PRIM, prim);
    }
#endif /* CSR_USE_QCA_CHIP */
