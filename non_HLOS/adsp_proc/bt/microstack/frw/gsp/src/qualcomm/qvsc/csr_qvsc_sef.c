/******************************************************************************
 Copyright (c) 2016-2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_pmem.h"
#include "csr_sched.h"
#include "csr_types.h"
#include "csr_log_text_2.h"
#include "csr_message_queue.h"
#include "csr_prim_defs.h"
#include "csr_qvsc_prim.h"
#include "csr_qvsc_private_prim.h"
#include "csr_qvsc_handler.h"


static CsrBool csrQvscReqHandler(CsrQvscInstanceData *qvscInst, void *msg)
{
    if (qvscInst->state == CSR_QVSC_STATE_IDLE)
    {
        CsrQvscReq *prim = msg;

        if (prim->phandle != CSR_SCHED_QID_INVALID)
        { /* Application expects a response */
            qvscInst->phandle = prim->phandle;
            CSR_QVSC_CHANGE_STATE(qvscInst, CSR_QVSC_STATE_REQ);
        }

        CsrQvscSendCommand(prim->ocf, (void *) prim->payload, 0);
        prim->payload = NULL;
        return (TRUE);
    }
    else
    { /* Not activated */
        return (FALSE);
    }
}

static CsrBool csrQvscSubscribeReqHandler(CsrQvscInstanceData *qvscInst, 
                                          void *msg)
{
    CsrQvscSubscribeReq *prim = msg;
        
    if (prim->phandle != CSR_SCHED_QID_INVALID)
    {
        CsrResult result = CSR_RESULT_FAILURE;
        CsrQvscSubscrptionId subsId = CSR_QVSC_SUBSCRIPTION_ID_INVALID;

        if (prim->pattern && prim->patternLen)
        {
            CsrQvscSubsListElement *elem;

            elem = CSR_QVSC_FIND_SUBS_ELEM_BY_DATA(qvscInst->subsList, prim);

            if (elem)
            {
                /* Subscription already exists, Return existing subscription ID. */
                subsId = elem->subsId;
                result = CSR_RESULT_SUCCESS;
            }
            else
            {
                /* Subscription does not exist, Add subscription entry. */
                subsId = CsrQvscGenerateSubscriptionId(qvscInst);
                if (subsId != CSR_QVSC_SUBSCRIPTION_ID_INVALID)
                {
                    elem = CSR_QVSC_ADD_SUBS_ELEM(qvscInst->subsList);
                    elem->phandle = prim->phandle;
                    elem->patternLen = prim->patternLen;
                    elem->pattern = prim->pattern;
                    elem->subsId = subsId;
                    /* pattern data is used in subscription list */
                    prim->pattern = NULL;
                    result = CSR_RESULT_SUCCESS;
                }
            }
        }

        CsrQvscSubscribeCfmSend(prim->phandle, subsId, result); 
    }
    else
    {
        CSR_LOG_TEXT_ERROR((CsrQvscLto,
                            CSR_QVSC_LTSO_GENERAL,
                            "Invalid phandle, prim: 0x%04X",
                            *((CsrQvscPrim *) prim)));
    }
    
    return (TRUE);
}

static CsrBool csrQvscUnsubscribeReqHandler(CsrQvscInstanceData *qvscInst, 
                                            void *msg)
{
    CsrQvscUnsubscribeReq *prim = msg;
    CsrResult result = CSR_RESULT_FAILURE;

    if (prim->phandle != CSR_SCHED_QID_INVALID)
    {
        if (prim->subscriptionId != CSR_QVSC_SUBSCRIPTION_ID_INVALID)
        {
            CSR_QVSC_REMOVE_SUBS_ELEM(qvscInst->subsList, prim);
            result = CSR_RESULT_SUCCESS;  
        }

        CsrQvscUnsubscribeCfmSend(prim->phandle, result);
    }
    else
    {
        CSR_LOG_TEXT_ERROR((CsrQvscLto,
                            CSR_QVSC_LTSO_GENERAL,
                            "Invalid phandle, prim: 0x%04X",
                            *((CsrQvscPrim *) prim)));
    }

    return (TRUE);
}

void CsrQvscPrimHandler(CsrQvscInstanceData *qvscInst)
{
    CsrBool processed;
    CsrQvscPrim *prim = qvscInst->recvMsgP;

    switch (*prim)
    {
        case CSR_QVSC_REQ:
            processed = csrQvscReqHandler(qvscInst, qvscInst->recvMsgP);
            break;

        case CSR_QVSC_SUBSCRIBE_REQ:
            processed = csrQvscSubscribeReqHandler(qvscInst, qvscInst->recvMsgP);
            break;

        case CSR_QVSC_UNSUBSCRIBE_REQ:
            processed = csrQvscUnsubscribeReqHandler(qvscInst, qvscInst->recvMsgP);
            break;

        case CSR_QVSC_TLV_DOWNLOAD_REQ:
            processed = CsrQvscTlvDownloadReqHandler(qvscInst, qvscInst->recvMsgP);
            break;

#ifdef CSR_DSPM_KCS_DOWNLOAD
        case CSR_QVSC_KCS_DOWNLOAD_REQ:
            processed = CsrQvscKcsDownloadReqHandler(qvscInst, qvscInst->recvMsgP);
            break;

        case CSR_QVSC_KCS_REMOVE_REQ:
            processed = CsrQvscKcsRemoveReqHandler(qvscInst, qvscInst->recvMsgP);
            break;
#endif /* CSR_DSPM_KCS_DOWNLOAD */

        default:
            processed = FALSE;
            CSR_LOG_TEXT_WARNING((CsrQvscLto,
                                  CSR_QVSC_LTSO_GENERAL,
                                  "Unknown primitive: 0x%04X",
                                  *((CsrQvscPrim *) prim)));
            break;
    }

    if (processed)
    {
        CsrQvscFreeDownstreamMessageContents(CSR_QVSC_PRIM, qvscInst->recvMsgP);
        CsrQvscPrivateFreeDownstreamMessageContents(CSR_QVSC_PRIVATE_PRIM, qvscInst->recvMsgP);
    }
    else
    {
        CsrMessageQueuePush(&qvscInst->messageQueue,
                            CSR_QVSC_PRIM,
                            qvscInst->recvMsgP);
        qvscInst->recvMsgP = NULL;
    }
}
