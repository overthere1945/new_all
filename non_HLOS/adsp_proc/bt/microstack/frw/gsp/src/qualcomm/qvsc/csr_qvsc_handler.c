/******************************************************************************
 Copyright (c) 2016-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_qvsc_handler.h"

#include "csr_pmem.h"
#include "csr_hci_lib.h"
#include "csr_hci_prim.h"
#include "csr_prim_defs.h"
#include "csr_qvsc_task.h"
#include "csr_tm_bluecore_lib.h"
#include "csr_tm_bluecore_prim.h"
#include "csr_transport.h"
#ifdef CSR_FRW_INSTALL_TRANS_RECOVERY
#include "csr_qvsc_lib.h"
#endif

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(CsrQvscLto);
#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)
static const CsrCharString *subOrigins[] =
{
    "STATE_CHANGE",
};
#endif

#ifdef CSR_FRW_INSTALL_TRANS_RECOVERY
static void csrQvscStartTransSlugTimer(CsrQvscInstanceData* qvscInst);
static void csrQvscTransSlugTimeoutHandler(CsrUint16 mi, void* mv);
static void csrQvscTransNoRspTimeoutHandler(CsrUint16 mi, void* mv);
#endif /*CSR_FRW_INSTALL_TRANS_RECOVERY*/

static void csrQvscTmBlueCoreActivateTransportCfmHandler(CsrQvscInstanceData *qvscInst)
{
    CsrTmBluecoreActivateTransportCfm *prim = qvscInst->recvMsgP;

    if (prim->result == CSR_RESULT_SUCCESS)
    {
//#if (CSR_HOST_PLATFORM == Q6_HOST) 
#ifndef CSR_TARGET_PRODUCT_WEARABLE
        CsrHciRegisterVendorSpecificEventHandlerReqSend(CSR_QVSC_IFACEQUEUE, TRANSPORT_CHANNEL_QC_HCIVS);
#else
        CsrPmemFree(qvscInst->recvMsgP);
        qvscInst->recvMsgP = NULL;
        
        CsrQvscRestore(qvscInst);

#endif
    }
    else
    {
        CSR_LOG_TEXT_ERROR((CsrQvscLto,
                            CSR_QVSC_LTSO_GENERAL,
                            "CSR_TM_BLUECORE_ACTIVATE_TRANSPORT_CFM failed: %u",
                            prim->result));
    }
}

void CsrQvscInit(void **gash)
{
    CsrQvscInstanceData *qvscInst;

    CSR_LOG_TEXT_REGISTER(&CsrQvscLto,
                          "QVSC",
                          CSR_ARRAY_SIZE(subOrigins),
                          subOrigins);

    qvscInst = (void *) CsrPmemZalloc(sizeof(*qvscInst));
    qvscInst->phandle = CSR_SCHED_QID_INVALID;
    qvscInst->messageQueue = NULL;
    qvscInst->recvMsgP = NULL;
    qvscInst->subsIdCounter = CSR_QVSC_SUBSCRIPTION_ID_INVALID;
#ifdef CSR_FRW_INSTALL_TRANS_RECOVERY
    qvscInst->timerID = CSR_SCHED_TID_INVALID;
#endif
    
    CSR_QVSC_CHANGE_STATE(qvscInst, CSR_QVSC_STATE_NULL);

    CsrCmnListInit(&qvscInst->subsList,
                   0,
                   NULL,
                   CsrQvscSubsListDeinit);

    *gash = qvscInst;

    CsrTmBlueCoreActivateTransportReqSend(CSR_QVSC_IFACEQUEUE);
}

void CsrQvscHandler(void **gash)
{
    CsrUint16 eventClass;
    CsrPrim *primType;
    CsrQvscInstanceData *qvscInst = *gash;

    CsrSchedMessageGet(&eventClass, &(qvscInst->recvMsgP));
    primType = qvscInst->recvMsgP;

    switch (eventClass)
    {
        case CSR_QVSC_PRIM:
#ifdef CSR_FRW_INSTALL_TRANS_RECOVERY
            if (*primType == CSR_QVSC_CFM)
            {
               /*QVSC response has come so we need to cancel the 2sec timer,
                * which will be taken care of in csrQvscStartTransSlugTimer().*/
               csrQvscStartTransSlugTimer(qvscInst);
            }
            else
#endif /*CSR_FRW_INSTALL_TRANS_RECOVERY*/
            {
               CsrQvscPrimHandler(qvscInst);
            }
            break;

        case CSR_HCI_PRIM:
            if (*primType == CSR_HCI_VENDOR_SPECIFIC_EVENT_IND)
            {
                CsrQvscHciVendorSpecificEventIndHandler(qvscInst);
            }
            else if (*primType == CSR_HCI_REGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_CFM)
            {
                CsrPmemFree(qvscInst->recvMsgP);
                qvscInst->recvMsgP = NULL;

                CsrQvscRestore(qvscInst);
            }
            else
            {
                CsrGeneralException(CsrQvscLto,
                                    CSR_QVSC_LTSO_GENERAL,
                                    eventClass,
                                    *primType,
                                    qvscInst->state,
                                    NULL);
            }
            CsrHciFreeUpstreamMessageContents(eventClass, qvscInst->recvMsgP);
            break;

        case CSR_TM_BLUECORE_PRIM:
            if (*primType == CSR_TM_BLUECORE_ACTIVATE_TRANSPORT_CFM)
            {
                csrQvscTmBlueCoreActivateTransportCfmHandler(qvscInst);
#ifdef CSR_FRW_INSTALL_TRANS_RECOVERY
                csrQvscStartTransSlugTimer(qvscInst);
#endif /*CSR_FRW_INSTALL_TRANS_RECOVERY*/
            }
            else
            {
                CsrGeneralException(CsrQvscLto,
                                    CSR_QVSC_LTSO_GENERAL,
                                    eventClass,
                                    *primType,
                                    qvscInst->state,
                                    NULL);
            }
            CsrTmBluecoreFreeUpstreamMessageContents(eventClass, qvscInst->recvMsgP);
            break;

        case CSR_SCHED_PRIM:
            break;

        default:
            CsrGeneralException(CsrQvscLto,
                                CSR_QVSC_LTSO_GENERAL,
                                eventClass,
                                *primType,
                                qvscInst->state,
                                NULL);
            break;
    }

    CsrPmemFree(qvscInst->recvMsgP);
    qvscInst->recvMsgP = NULL;
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************
    This function is called by the scheduler to perform a graceful shutdown
    of a scheduler task.
    This function must:
    1)    empty the input message queue and free any allocated memory in the
        messages.
    2)    free any instance data that may be allocated.
****************************************************************************/
void CsrQvscDeinit(void **gash)
{
    CsrUint16 eventClass;
    CsrQvscInstanceData *qvscInst = *gash;

    while (CsrMessageQueuePop(&qvscInst->messageQueue,
                              &eventClass,
                              &(qvscInst->recvMsgP)) ||
           CsrSchedMessageGet(&eventClass, &(qvscInst->recvMsgP)))
    {
        switch (eventClass)
        {
            case CSR_QVSC_PRIM:
            {
                CsrQvscFreeDownstreamMessageContents(CSR_QVSC_PRIM,
                                                     qvscInst->recvMsgP);
                CsrQvscPrivateFreeDownstreamMessageContents(CSR_QVSC_PRIVATE_PRIM,
                                                            qvscInst->recvMsgP);
                break;
            }
            case CSR_HCI_PRIM:
            {
                CsrHciFreeUpstreamMessageContents(CSR_HCI_PRIM,
                                                  qvscInst->recvMsgP);
                break;
            }
            case CSR_TM_BLUECORE_PRIM:
            {
                /* Free function not available (no contents in any received messages) */
                break;
            }
            default:
                break;
        }
        CsrPmemFree(qvscInst->recvMsgP);
    }
#ifdef CSR_FRW_INSTALL_TRANS_RECOVERY
    CsrSchedTimerCancel(qvscInst->timerID, NULL, NULL);
#endif
    CsrPmemFree(qvscInst->tlvData.payload);
    CsrCmnListDeinit(&qvscInst->subsList);
    CsrPmemFree(qvscInst);
}
#endif /* ENABLE_SHUTDOWN */

#ifdef CSR_FRW_INSTALL_TRANS_RECOVERY
static void csrQvscStartTransSlugTimer(CsrQvscInstanceData *qvscInst)
{
    CsrSchedTimerCancel(qvscInst->timerID, NULL, NULL);
    qvscInst->timerID = CsrSchedTimerSet(CSR_FRW_TRANS_PROBE_TIME * CSR_SCHED_MILLISECOND, csrQvscTransSlugTimeoutHandler, 0, (void *)qvscInst);
}

static void csrQvscTransSlugTimeoutHandler(CsrUint16 mi, void *mv)
{
    CsrQvscInstanceData *qvscInst = mv;
    CsrUint8 *payload = (CsrUint8*)CsrPmemAlloc(1);
    payload[0] = 0x03;

    CsrQvscReqSend(CsrSchedTaskQueueGet(), 0x00, 1, payload);
    qvscInst->timerID = CsrSchedTimerSet(CSR_FRW_TRANS_PANIC_ON_NO_RSP_TIME * CSR_SCHED_MILLISECOND, csrQvscTransNoRspTimeoutHandler, mi, mv);
}

static void csrQvscTransNoRspTimeoutHandler(CsrUint16 mi, void *mv)
{
    CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_TRANS_UNRESPONSIVE, "Transport unresponsive");
}

#endif /*CSR_FRW_INSTALL_TRANS_RECOVERY*/
