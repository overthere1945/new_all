/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "csr_synergy.h"

#ifdef INCLUDE_HCI_ARBITER
#include "hci_arbiter_private.h"
#include "csr_log_text_2.h"
#include "csr_hci_lib.h"
#include "dm_hci_interface.h"
#include "csr_bt_hci_convert.h"
#include "csr_hci_common.h"
#include "csr_hci_util.h"
#include "dm_acl.h"
#ifdef GATT_SEQUENCING
#include "hci_att_arb.h"
#endif /* GATT_SEQUENCING */
HciArbInst gHciArb;

#ifdef CONFIG_BT_TESTER
extern CsrUint8 hciReset;
#endif

void HciArbInit(bool isPassthrough)
{
    HCI_LOG_INFO("HCI Arb Init isPassthrough %d", isPassthrough);

    gHciArb.passthroughMode = isPassthrough;
    gHciArb.filterIdCounter = 1;

    gHciArb.debug.appCtxRunning = FALSE;
    gHciArb.debug.length = 0;
    gHciArb.debug.count++;

#ifdef CONFIG_BT_TESTER
    hciReset = 0;
#endif

#ifdef GATT_SEQUENCING
    HciArbRegisterAttSequencingCb(HciAttArbGetDestinationStack);
#endif /* GATT_SEQUENCING */
}

void HciArbDeinit(void)
{
    HCI_LOG_INFO("HCI Arb Deinit");
}

void HciArbRegisterCb(HCI_ARBITER_MSG_CB cb)
{
    gHciArb.hciMsgCb = cb;
}

bool HciArbIsPassthroughMode(void)
{
    return gHciArb.passthroughMode;
}

HciArbAclInst *HciArbGetInstFromHandle(HciArbHciHandle hciHandle)
{
    return dm_acl_arb_find_by_handle(hciHandle);
}

CsrBtTypedAddr HciArbGetBtAddrFromHandle(HciArbHciHandle hciHandle)
{
    CsrBtTypedAddr addrt;
    HciArbAclInst * aclInst = HciArbGetInstFromHandle(hciHandle);
    CsrBtAddrZero(&addrt);

    if (aclInst != NULL)
    {
        addrt = aclInst->addrt;
    }
    return addrt;
}

void HciArbSetFirstFragmentStack(HciArbAclInst *aclInst, HciArbStack stack)
{
    if (aclInst != NULL)
        aclInst->firstFragStack = stack;
}

HciArbStack HciArbGetFirstFragmentStack(HciArbAclInst *aclInst)
{
    return (aclInst != NULL) ? aclInst->firstFragStack : HCI_ARB_PRIMARY_STACK;
}


CsrBtResultCode HciArbValidateConnReq(HciArbHciHandle hciHandle)
{
    CsrBtResultCode result = HCI_ARB_RESULT_SUCCESS;

    if (HciArbIsPassthroughMode())
    {
        result = HCI_ARB_RESULT_INVALID_OPERATION;
    }
    else if(HciArbGetInstFromHandle(hciHandle) == NULL)
    {
        result = HCI_ARB_RESULT_ACL_DOES_NOT_EXIST;
    }
    return result;
}


HciArbFilterId HciArbGenerateFilterId(HciArbProtocol protocol, 
                                      HciArbHciHandle handle)
{
    HciArbFilterId filterId = (gHciArb.filterIdCounter << 16);
    filterId |= (HciArbFilterId)((protocol << 24) | (handle));
    gHciArb.filterIdCounter = 
        (gHciArb.filterIdCounter + 1) % HCI_ARB_MAX_FILTERID_COUNTER;

    HCI_LOG_INFO("Filter Id generated 0x%x ", filterId);    
    return filterId;
}

void HciArbCleanupFilter(HciArbRxFilter *prxFilter)
{
    if (prxFilter->protocol == HCI_ARB_RFCOMM_PROTOCOL)
    {
        HCI_LOG_INFO("RFCOMM Filter disabled ");
        pfree(prxFilter->proto.rfcomm);
    }
    else if (prxFilter->protocol == HCI_ARB_LECOC_PROTOCOL)
    {
        HCI_LOG_INFO("LECOC Filter disabled ");
        pfree(prxFilter->proto.lecoc);
    }
    else if (prxFilter->protocol == HCI_ARB_GATT_PROTOCOL)
    {
        HCI_LOG_INFO("GATT Filter disabled ");
        pfree(prxFilter->proto.gatt);
    }
}

CsrBtResultCode HciArbEnableRfcommFilter(HciArbHciHandle hciHandle,
                                         HciArbL2capCid cid,
                                         uint8 dlci,
                                         HciArbFilterId *filterId)
{
    CsrBtResultCode result = HciArbValidateConnReq(hciHandle);
    HciArbAclInst *aclInst = HciArbGetInstFromHandle(hciHandle);

    if ((result == HCI_ARB_RESULT_SUCCESS) && (aclInst != NULL))
    {
        HciArbRxFilter *rxFilter = zpnew(HciArbRxFilter);

        rxFilter->protocol = HCI_ARB_RFCOMM_PROTOCOL;
        *filterId = HciArbGenerateFilterId(HCI_ARB_RFCOMM_PROTOCOL, hciHandle);
        
        rxFilter->filterId = *filterId;
        rxFilter->localCid = cid;

        rxFilter->proto.rfcomm = zpnew(HciArbRfcommCtx);
        rxFilter->proto.rfcomm->dlci = dlci;
        rxFilter->next = aclInst->rxFilter;

        aclInst->rxFilter = rxFilter;
    }
    HCI_LOG_INFO("Enable RFCOMM Filter Id 0x%x Result 0x%x", *filterId, result);
    return result;
}


CsrBtResultCode HciArbEnableLecocFilter(HciArbHciHandle hciHandle,
                                        HciArbL2capCid localCid,
                                        HciArbL2capCid remoteCid,
                                        HciArbFilterId *filterId)
{
    CsrBtResultCode result = HciArbValidateConnReq(hciHandle);
    HciArbAclInst *aclInst = HciArbGetInstFromHandle(hciHandle);

    if ((result == HCI_ARB_RESULT_SUCCESS) && (aclInst != NULL))
    {
        HciArbRxFilter *rxFilter = zpnew(HciArbRxFilter);

        rxFilter->protocol = HCI_ARB_LECOC_PROTOCOL;
        *filterId = HciArbGenerateFilterId(HCI_ARB_LECOC_PROTOCOL, hciHandle);
        rxFilter->filterId = *filterId;
        rxFilter->localCid = localCid;

        rxFilter->proto.lecoc = zpnew(HciArbLecocCtx);
        rxFilter->proto.lecoc->remoteCid = remoteCid;
        rxFilter->next     = aclInst->rxFilter;

        aclInst->rxFilter = rxFilter;
    }
    HCI_LOG_INFO("Enable LECOC Filter Id 0x%x Result 0x%x", *filterId, result);

    return result;
}

CsrBtResultCode HciArbEnableGattFilter(HciArbHciHandle hciHandle,
                                       HciArbGattCtx *ctx,
                                       HciArbFilterId *filterId)
{
    CsrBtResultCode result = HciArbValidateConnReq(hciHandle);
    HciArbAclInst *aclInst = HciArbGetInstFromHandle(hciHandle);

    HCI_LOG_INFO("Enable GATT Filter Hci Handle 0x%x, Role 0x%x, Num Hdls %d", 
                      hciHandle, ctx->role, ctx->numHandles);

    if ((result == HCI_ARB_RESULT_SUCCESS) && (aclInst != NULL))
    {
        HciArbRxFilter *rxFilter = zpnew(HciArbRxFilter);

        rxFilter->protocol = HCI_ARB_GATT_PROTOCOL;
        *filterId = HciArbGenerateFilterId(HCI_ARB_GATT_PROTOCOL, hciHandle);
        rxFilter->filterId = *filterId;
        rxFilter->localCid = HCI_ARB_ATT_CID;

        rxFilter->proto.gatt = ctx;
        rxFilter->next = aclInst->rxFilter;

        aclInst->rxFilter = rxFilter;
    }
    else
    {
        pfree(ctx);
    }
    HCI_LOG_INFO("Enable GATT Filter Id 0x%x Result 0x%x", *filterId, result);
    return result;
}

void HciArbDisableFilter(HciArbFilterId filterId)
{
    HciArbRxFilter *prxFilter;
    HciArbRxFilter **pprxFilter;
    HciArbHciHandle hciHandle = HCI_ARB_GET_HCI_HANDLE_FROM_FILTER_ID(filterId);
    HciArbProtocol protocol = HCI_ARB_GET_PROTOCOL_FROM_FILTER_ID(filterId);
    HciArbAclInst *aclInst = HciArbGetInstFromHandle(hciHandle);

    HCI_LOG_INFO("Disable Filter 0x%x", filterId);
    if (aclInst == NULL)
    {
        return;
    }

    for (pprxFilter = &aclInst->rxFilter;
         (prxFilter = *pprxFilter) != NULL;
         pprxFilter = &prxFilter->next)
    {
        if ((prxFilter->filterId == filterId) &&
            (prxFilter->protocol == protocol))
        {
            *pprxFilter = prxFilter->next;

            HciArbCleanupFilter(prxFilter);

            pfree(prxFilter);
            break;
        }
    }
}

void HciArbSendHciMsgToPrimaryStack(uint16 length, uint8 *data)
{
    uint16 i;
    HCI_LOG_DEBUG("Sending HCI Msg to Primary stack Len %d", length);
    for (i=0; i < length && i < 20; i++)
    {
        HCI_LOG_DEBUG("%02X", data[i]);
    }

    if ((gHciArb.hciMsgCb) && (data != NULL))
    {
        uint16 lenCopy = CSRMIN(length, HCI_ARB_DEBUG_DATA_LEN);
    
        gHciArb.debug.length = length;
        SynMemCpyS(&gHciArb.debug.data[0], HCI_ARB_DEBUG_DATA_LEN, data, lenCopy);
        gHciArb.debug.appCtxRunning = TRUE;
        gHciArb.hciMsgCb(length, data);
        gHciArb.debug.appCtxRunning = FALSE;
    }
}

void HciArbSendNocpToPrimaryStack(HciArbHciHandle handle, uint16 ncp)
{
    uint8 nocp[]={0x04, 0x13, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00};
    uint8 *evt = CsrPmemAlloc(sizeof(nocp));

    HCI_LOG_DEBUG("Sending NOCP to Primary stack. Handle 0x%x Num %d", handle, ncp);

    HCI_ARB_SET_UINT16(&nocp[4], handle);

    HCI_ARB_SET_UINT16(&nocp[6], ncp);

    SynMemCpyS(evt, sizeof(nocp), nocp, sizeof(nocp));

    HciArbSendHciMsgToPrimaryStack(sizeof(nocp), evt);
}

void * HciArbAclConnected(HciArbHciHandle hciHandle,
                          PHYSICAL_TRANSPORT_T transport,
                          CsrBtTypedAddr addrt)
{
    HciArbAclInst *aclInst = zpnew(HciArbAclInst);

    if (aclInst != NULL)
    {
        aclInst->hciHandle = hciHandle;
        aclInst->transport = transport;
        aclInst->firstFragStack = HCI_ARB_PRIMARY_STACK;
        aclInst->prepWriteOffloaded = FALSE;
        aclInst->addrt = addrt;
        HCI_LOG_INFO("ACL connected Handle 0x%x Transport 0x%x", hciHandle, transport);
    }
    return aclInst;
}

void HciArbAclDisconnected(void *context)
{
    HciArbRxFilter *prxFilter;
    HciArbRxFilter **pprxFilter;
    HciArbAclInst *aclInst = context;

    if (aclInst == NULL)
    {
        HCI_LOG_INFO("ACL instance not found 0x%x", aclInst);
        return;
    }

    HCI_LOG_INFO("ACL disconnected Handle 0x%x", aclInst->hciHandle);

    for (pprxFilter = &aclInst->rxFilter; (prxFilter = *pprxFilter) != NULL; /* empty */)
    {
        /* Remove entry from list */
        *pprxFilter = prxFilter->next;

        HciArbCleanupFilter(prxFilter);

        pfree(prxFilter);
    }

    pfree(aclInst);
}

void HciArbRegisterAttSequencingCb(HCI_ARBITER_ATT_CB attCb)
{
    if (attCb)
    {
        gHciArb.attSeqCb = attCb;
    }
    else
    {
        HCI_PANIC(BT_OFFLOAD_PANIC_HCI_ARB_INV_PARAM, "NULL ATT CB");
    }
}

void HciArbiterHostCleanup(void)
{
    /* Cleanup as part of stack stop */
}

#endif
