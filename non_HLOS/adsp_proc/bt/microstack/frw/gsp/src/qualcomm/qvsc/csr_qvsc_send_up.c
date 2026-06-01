/******************************************************************************
 Copyright (c) 2017-2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
 ******************************************************************************/

#include "csr_pmem.h"
#include "csr_sched.h"
#include "csr_types.h"
#include "csr_result.h"
#include "csr_qvsc_prim.h"
#include "csr_qvsc_private_prim.h"

void CsrQvscCfmSend(CsrSchedQid phandle,
                    CsrUint8 *payload,
                    CsrUint16 payloadLength)
{
    CsrQvscCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_QVSC_CFM;
    prim->payloadLength = payloadLength;
    prim->payload = payload;

    CsrSchedMessagePut(phandle, CSR_QVSC_PRIM, prim);
}

void CsrQvscSubscribeCfmSend(CsrSchedQid phandle,
                             CsrQvscSubscrptionId subsId,
                             CsrResult result)
{
    CsrQvscSubscribeCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_QVSC_SUBSCRIBE_CFM;
    prim->subscriptionId = subsId;
    prim->result = result;

    CsrSchedMessagePut(phandle, CSR_QVSC_PRIM, prim);
}

void CsrQvscUnsubscribeCfmSend(CsrSchedQid phandle, CsrResult result)
{
    CsrQvscUnsubscribeCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_QVSC_UNSUBSCRIBE_CFM;
    prim->result = result;

    CsrSchedMessagePut(phandle, CSR_QVSC_PRIM, prim);
}

void CsrQvscEventIndSend(CsrSchedQid phandle,
                         CsrQvscSubscrptionId subscriptionId,
                         CsrUint8 eventLength, 
                         CsrUint8* event)
{
    CsrQvscEventInd *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_QVSC_EVENT_IND;
    prim->subscriptionId = subscriptionId;
    prim->eventLength = eventLength;
    prim->event = event;
        
    CsrSchedMessagePut(phandle, CSR_QVSC_PRIM, prim);
}

void CsrQvscTlvDownloadCfmSend(CsrSchedQid phandle, CsrResult result)
{
    CsrQvscTlvDownloadCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_QVSC_KCS_DOWNLOAD_CFM;
    prim->result = result;

    CsrSchedMessagePut(phandle, CSR_QVSC_PRIM, prim);
}

#ifdef CSR_DSPM_KCS_DOWNLOAD
void CsrQvscKcsDownloadCfmSend(CsrSchedQid phandle,
                               CsrResult result,
                               CsrUint16 kcsBundleId)
{
    CsrQvscKcsDownloadCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_QVSC_KCS_DOWNLOAD_CFM;
    prim->result = result;
    prim->kcsBundleId = kcsBundleId;

    CsrSchedMessagePut(phandle, CSR_QVSC_PRIM, prim);
}

void CsrQvscKcsRemoveCfmSend(CsrSchedQid phandle, CsrResult result)
{
    CsrQvscKcsRemoveCfm *prim = CsrPmemAlloc(sizeof(*prim));

    prim->type = CSR_QVSC_KCS_REMOVE_CFM;
    prim->result = result;

    CsrSchedMessagePut(phandle, CSR_QVSC_PRIM, prim);
}
#endif /* CSR_DSPM_KCS_DOWNLOAD */
