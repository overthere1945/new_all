/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "csr_synergy.h"

#ifdef INCLUDE_HCI_ARBITER
#include "hci_arbiter_lib.h"
#include "csr_log_text_2.h"
#include "csr_hci_lib.h"
#include "l2cap_private.h"
#include "hci_arbiter_private.h"
#include "hci_cmd_arb.h"

void HciArbiterSendMsgToBmm(void *msg)
{
    CsrMsgTransport(BMM_IFACEQUEUE, HCI_ARBITER_PRIM, msg);
}

/******************************************************************************
 * API to register callback for HCI message.
 ******************************************************************************/
void HciArbiterRegisterHciMsgCb(HCI_ARBITER_MSG_CB hciCb)
{
    if (hciCb)
    {
        HciArbRegisterCb(hciCb);
    }
    else
    {
        HCI_PANIC(BT_OFFLOAD_PANIC_HCI_ARB_INV_PARAM, "NULL HCI CB");
    }
}

/******************************************************************************
 * API to send HCI Message from primary stack. 
 ******************************************************************************/
void HciArbiterSendHciMsg(uint16 length, uint8 *payload)
{
    if ((payload == NULL) || (length <= 1))
    {
        HCI_PANIC(BT_OFFLOAD_PANIC_HCI_ARB_INV_PARAM, "Invalid Params");
    }
    else if (payload[0] == HCI_ARB_HCI_CMD_TYPE)
    {
        uint16 len;
        uint8 * data;

#ifdef CONFIG_BT_TESTER
    if (HciArbiterSkipTestCmd(length, payload))
    {
        return;
    }
#endif

        len  = (length-1);
        data = CsrPmemAlloc(len);
        SynMemCpyS(data, len, &payload[1], len);

        if (HciArbIsPassthroughMode())
        {
            CsrHciCommandReqSend(len, data);
        }
        else
        {
            HciCommandArbCmdSend(HCI_CMD_SENDER_PRIMARY_STACK, len, data);
        }
        
        CsrPmemFree(payload);
    }
    else if (payload[0] == HCI_ARB_HCI_ACL_TYPE)
    {
        CsrMblk *mblk = CsrMblkDataCreate(payload, length, TRUE);

        if (HciArbIsPassthroughMode())
        {
            CsrHciAclDataReqSend(ACL_SOURCE_PRIMARY_STACK, mblk);
        }
#ifdef INSTALL_SOCKET_OFFLOAD
        else
        {
            L2CAINT_PassthroughDataReq(0, mblk);
        }
#endif
    }
    else if (payload[0] == HCI_ARB_PERI_CMD_TYPE)
    {
        uint16 len    = (length-1);
        uint8 * data  = CsrPmemAlloc(len);

        SynMemCpyS(data, len, &payload[1], len);
        HciPeriCommandArbCmdSend(HCI_CMD_SENDER_PRIMARY_STACK, len, data);
        CsrPmemFree(payload);
    }
    else if (payload[0] == HCI_ARB_PERI_DATA_TYPE)
    {

    }
    else
    {
        HCI_PANIC(BT_OFFLOAD_PANIC_HCI_ARB_INV_HCI_MSG, "Invalid HCI msg"); 
    }
}

/* These are internal messages */

void HciArbiterBtOffIndSend(HciArbiterResultCode status)
{
    HciArbiterBtOffInd *prim = zpnew(HciArbiterBtOffInd);

    prim->type        = HCI_ARBITER_BT_OFF_IND;
    prim->status      = status;

    HciArbiterSendMsgToBmm(prim);
}

void HciArbiterBtssCrashIndSend(HciArbiterBtssErrorCode reason)
{
    HciArbiterBtssCrashInd *prim = zpnew(HciArbiterBtssCrashInd);

    prim->type        = HCI_ARBITER_BTSS_CRASH_IND;
    prim->reason      = reason;

    HciArbiterSendMsgToBmm(prim);
}

#endif

