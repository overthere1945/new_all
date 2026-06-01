/******************************************************************************
 Copyright (c) 2016-2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_qvsc_lib.h"

#include "csr_qvsc_task.h"
#include "csr_msg_transport.h"
#include "csr_pmem.h"
#include "csr_sched.h"


void CsrQvscMsgTransport(void *msg)
{
    CsrMsgTransport(CSR_QVSC_IFACEQUEUE, CSR_QVSC_PRIM, msg);
}

void CsrQvscReqSend(CsrSchedQid phandle,
                    CsrUint16 ocf,
                    CsrUint16 payloadLength,
                    CsrUint8 *payload)
{
    CsrQvscReq *prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_QVSC_REQ;
    prim->phandle = phandle;
    prim->ocf = ocf;

    if (payload && payloadLength)
    {
        prim->payload = CsrMblkDataCreate(payload,
                                          payloadLength,
                                          TRUE);
    }
    else
    {
        prim->payload = NULL;
    }

    CsrQvscMsgTransport(prim);
}

void CsrQvscMblkReqSend(CsrSchedQid phandle, CsrUint16 ocf, CsrMblk *payload)
{
    CsrQvscReq *prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_QVSC_REQ;
    prim->phandle = phandle;
    prim->ocf = ocf;
    prim->payload = payload;
    CsrQvscMsgTransport(prim);
}

void CsrQvscSubscribeReqSend(CsrSchedQid phandle,
                             CsrUint8 patternLen,  
                             CsrUint8 *pattern)
{
    CsrQvscSubscribeReq *prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_QVSC_SUBSCRIBE_REQ;
    prim->phandle = phandle;
    prim->patternLen = patternLen;
    prim->pattern = pattern;
    CsrQvscMsgTransport(prim);
}

void CsrQvscUnsubscribeReqSend(CsrSchedQid phandle, 
                               CsrQvscSubscrptionId subscriptionId)
{
    CsrQvscUnsubscribeReq *prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = CSR_QVSC_UNSUBSCRIBE_REQ;
    prim->phandle = phandle;
    prim->subscriptionId = subscriptionId;
    CsrQvscMsgTransport(prim);
}

