/******************************************************************************
 Copyright (c) 2017-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_pmem.h"
#include "csr_sched.h"
#include "csr_util.h"
#include "csr_types.h"
#include "csr_hci_common.h"
#include "csr_hci_lib.h"
#include "csr_macro.h"
#include "csr_mblk.h"
#include "csr_message_queue.h"
#include "csr_transport.h"
#include "csr_qvsc_handler.h"
#include "hci_cmd_arb.h"

/* Sends vendor specific command */
void CsrQvscSendCommand(CsrUint16 ocf, void *payload, CsrUint8 payloadLength)
{
    CsrMblk *vsCmd;
    CsrUint8 *qvscHdr;

    vsCmd = CsrMblkMallocCreate((void **)&qvscHdr, CSR_QVSC_HEADER_SIZE);

    /* Added to address KW issue */
    if (qvscHdr == NULL)
    {
        return;
    }

    qvscHdr[0] = CSR_LSB16(ocf);
    qvscHdr[1] = ((CSR_MSB16(ocf) & 0x03) | CSR_HCI_MANUFACTURER_EXTENSION_MASK);

    if (payload)
    {
        CsrMblk *tail;

        if (payloadLength)
        {
            tail = CsrMblkDataCreate(payload,
                                     payloadLength,
                                     TRUE);
        }
        else
        {
            tail = (CsrMblk *) payload;
            payloadLength = (CsrUint8) CsrMblkGetLength(tail);
        }
        vsCmd = CsrMblkJoinChain(vsCmd, tail);
    }
    else
    {
        payloadLength = 0;
    }

    qvscHdr[2] = payloadLength;

    HciCommandArbVSCmdSend(HCI_CMD_SENDER_MICRO_STACK, vsCmd);


}


void CsrQvscRestore(CsrQvscInstanceData *qvscInst)
{
    CsrUint16 event;

    CSR_QVSC_CHANGE_STATE(qvscInst, CSR_QVSC_STATE_IDLE);
    qvscInst->phandle = CSR_SCHED_QID_INVALID;

    CsrPmemFree(qvscInst->tlvData.payload);
    CsrMemSet(&qvscInst->tlvData, 0, sizeof(qvscInst->tlvData));

    if (CsrMessageQueuePop(&qvscInst->messageQueue, &event, &qvscInst->recvMsgP))
    {
        CsrQvscPrimHandler(qvscInst);
    }
}

/* Generates a new subscription ID */
CsrQvscSubscrptionId CsrQvscGenerateSubscriptionId(CsrQvscInstanceData *qvscInst)
{
    CsrQvscSubscrptionId id;

    for (id = qvscInst->subsIdCounter + 1; id != qvscInst->subsIdCounter; id++)
    {
        if (id != CSR_QVSC_SUBSCRIPTION_ID_INVALID &&
            !CSR_QVSC_FIND_SUBS_ELEM_BY_ID(qvscInst->subsList, &id))
        {
            /* Found Unallocated id */
            qvscInst->subsIdCounter = id;
            return (id);
        }
    }

    return (CSR_QVSC_SUBSCRIPTION_ID_INVALID);
}

void CsrQvscSubsListDeinit(CsrCmnListElm_t *elem)
{
    /* CsrPmemFree local pointers in the CsrQvscSubsList Element.
     * This function is called every time a element is removed from the
     * CsrQvscSubsList */
    CsrQvscSubsListElement *sElem = (CsrQvscSubsListElement *) elem;
    CsrPmemFree(sElem->pattern);
}

CsrBool CsrQvscFindSubsElemByHandleAndId(CsrCmnListElm_t *elem, void *data)
{ 
    CsrQvscUnsubscribeReq *prim = (CsrQvscUnsubscribeReq *) data;
    CsrQvscSubsListElement *element = (CsrQvscSubsListElement *) elem;

    if (element->phandle == prim->phandle)
    {
        if (prim->subscriptionId == CSR_QVSC_SUBSCRIPTION_ID_INVALID ||
            element->subsId == prim->subscriptionId)
        {
            /* Element with phandle ID exists*/
            return (TRUE);
        }
    }

    return (FALSE);
}

CsrBool CsrQvscFindSubsElemById(CsrCmnListElm_t *elem, void *id)
{ 
    CsrQvscSubscrptionId subsId =  *((CsrQvscSubscrptionId *) id);
    CsrQvscSubsListElement *element = (CsrQvscSubsListElement *) elem;

    if (element->subsId == subsId)
    {
        /* Element with Subscription ID exists*/
        return (TRUE);
    }

    return (FALSE);
}

CsrBool CsrQvscFindSubsElemByData(CsrCmnListElm_t *elem, void *data)
{ 
    CsrQvscSubscribeReq *prim = (CsrQvscSubscribeReq *) data;
    CsrQvscSubsListElement *element = (CsrQvscSubsListElement *) elem;

    if (element->phandle == prim->phandle &&
        element->patternLen == prim->patternLen)
    {
        if (CsrMemCmp(element->pattern, prim->pattern, prim->patternLen) == 0)
        { 
            /* Element with data pattern, length and phandle already exists */
            return (TRUE);
        }
    }

    return (FALSE);
}

void CsrQvscFindSubsAndSendInd(CsrCmnListElm_t *elem, void *data)
{ 
    CsrQvscUnsolicitedHciEvent *event = (CsrQvscUnsolicitedHciEvent *) data;
    CsrQvscSubsListElement *element = (CsrQvscSubsListElement *)elem;

    if (element->patternLen <= event->dataLen)
    {
        /* Data length has matched */
        if (CsrMemCmp(element->pattern, event->data, element->patternLen) == 0)
        {
            /* Data has matched, Send indication */
            CsrQvscEventIndSend(element->phandle,
                                element->subsId,
                                event->dataLen, 
                                CsrMemDup(event->data, event->dataLen));
            event->processed = TRUE;
        }
    }
}


