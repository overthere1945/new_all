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
#include "dm_acl.h"
#include "dm_hci_interface.h"
#include "csr_hci_common.h"
#include "csr_bt_common.h"


#define HCI_ARB_BT_ENH_LOG_CONN_HANDLE          0xEDC 
#define HCI_ARB_ACL_HDR_SIZE                    5  /* 4 hdr size 1 packet ind*/
#define HCI_ARB_MIN_HDR                        0x0C
#define HCI_ARB_MIN_LECOC_HDR                  0x04
#define HCI_ARB_MIN_LECOC_CREDIT_HDR           0x0C
#define HCI_ARB_MIN_RFCOMM_HDR                 0x06
#define HCI_ARB_RFC_FRAME_TYPE_POS              5
#define HCI_ARB_CID_POS                         2
#define HCI_ARB_L2CAP_TYPE_POS                  4
#define HCI_ARB_LECOC_CREDIT_CID_POS            8
#define HCI_ARB_LE_L2CAP_SIGNALLING_CID         0x0005
#define HCI_ARB_L2CAP_FLOW_CONTROL_CREDIT       0x16

#define HCI_ARB_MIN_L2CAP_HDR                  0x04
#define HCI_ARB_MIN_GATT_HDR                   0x05
#define HCI_ARB_MIN_ATT_HDR_HDL                0x07
#define HCI_ARB_ATT_OPCODE_POS                  4
#define HCI_ARB_ATT_HANDLE_POS                  5
#define HCI_ARB_ATT_OP_RSP_MASK                0x01 /* bit 0 is set in responses */

typedef enum {
    HCI_ARB_ATT_OP_ERROR_RSP               = 0x01,
    HCI_ARB_ATT_OP_EXCHANGE_MTU_REQ,        /* 02 */
    HCI_ARB_ATT_OP_EXCHANGE_MTU_RSP,        /* 03 */
    HCI_ARB_ATT_OP_FIND_INFO_REQ,           /* 04 */
    HCI_ARB_ATT_OP_FIND_INFO_RSP,           /* 05 */
    HCI_ARB_ATT_OP_FIND_BY_TYPE_VALUE_REQ,  /* 06 */
    HCI_ARB_ATT_OP_FIND_BY_TYPE_VALUE_RSP,  /* 07 */
    HCI_ARB_ATT_OP_READ_BY_TYPE_REQ,        /* 08 */
    HCI_ARB_ATT_OP_READ_BY_TYPE_RSP,        /* 09 */
    HCI_ARB_ATT_OP_READ_REQ,                /* 0a */
    HCI_ARB_ATT_OP_READ_RSP,                /* 0b */
    HCI_ARB_ATT_OP_READ_BLOB_REQ,           /* 0c */
    HCI_ARB_ATT_OP_READ_BLOB_RSP,           /* 0d */
    HCI_ARB_ATT_OP_READ_MULTI_REQ,          /* 0e */
    HCI_ARB_ATT_OP_READ_MULTI_RSP,          /* 0f */
    HCI_ARB_ATT_OP_READ_BY_GROUP_TYPE_REQ,  /* 10 */
    HCI_ARB_ATT_OP_READ_BY_GROUP_TYPE_RSP,  /* 11 */
    HCI_ARB_ATT_OP_WRITE_REQ,               /* 12 */   
    HCI_ARB_ATT_OP_WRITE_RSP,               /* 13 */
    HCI_ARB_ATT_OP_WRITE_CMD               = 0x52,    
    HCI_ARB_ATT_OP_PREPARE_WRITE_REQ       = 0x16,
    HCI_ARB_ATT_OP_PREPARE_WRITE_RSP,       /* 17 */
    HCI_ARB_ATT_OP_EXECUTE_WRITE_REQ,       /* 18 */
    HCI_ARB_ATT_OP_EXECUTE_WRITE_RSP,       /* 19 */
    HCI_ARB_ATT_OP_HANDLE_VALUE_NOT        = 0x1b,
    HCI_ARB_ATT_OP_HANDLE_VALUE_IND        = 0x1d,
    HCI_ARB_ATT_OP_HANDLE_VALUE_CFM,        /* 1e */
    HCI_ARB_ATT_OP_READ_MULTI_VAR_REQ      = 0x20,
    HCI_ARB_ATT_OP_READ_MULTI_VAR_RSP,      /* 21 */
    HCI_ARB_ATT_OP_MULTI_HANDLE_VALUE_NOT  = 0x23,

    HCI_ARB_ATT_OP_LAST
} HciArbAttOpcode;


/* In UIH frames P/F bit should be 0 for non credit based flow control and one
   for credit based flow control.
*/ 
#define HCI_ARB_RFC_UIH         0xEF    /* No credit based flow control */
#define HCI_ARB_RFC_UIH_PF      0xFF    /* Credit based flow control */


#define HCI_ARB_IS_FIRST_FRAGMENT(flags) \
 ((flags & HCI_PKT_BOUNDARY_MASK) != HCI_PKT_BOUNDARY_FLAG_CONT)


bool HciArbIsRfcommOffloadedForAclData(HciArbAclInst *aclInst, 
                                       uint8 *hdr, uint16 hdrLen, uint16 cid)
{
    HciArbRxFilter *rxFilter;
    bool offloaded = FALSE;

    if (hdrLen < HCI_ARB_MIN_RFCOMM_HDR)
        return offloaded;

    for (rxFilter = aclInst->rxFilter; rxFilter; rxFilter = rxFilter->next)
    {
        uint8 frameType = hdr[HCI_ARB_RFC_FRAME_TYPE_POS];
        uint8 dlci = (hdr[4] >> 2);

        if ((rxFilter->localCid == cid) &&
            (rxFilter->proto.rfcomm != NULL) &&
            (rxFilter->proto.rfcomm->dlci == dlci) &&
            ((frameType == HCI_ARB_RFC_UIH) || (frameType == HCI_ARB_RFC_UIH_PF)))
        {
            /* Only UIH frame received on DLCI processed */
            offloaded = TRUE;
            break;
        }
    }

    return offloaded;
}


bool HciArbIsLecocOffloadedForAclData(HciArbAclInst *aclInst, 
                                      uint8 *hdr, uint16 hdrLen, uint16 cid)
{
    HciArbRxFilter *rxFilter;
    bool offloaded = FALSE;

    if (hdrLen < HCI_ARB_MIN_LECOC_HDR)
        return FALSE;

    /* Get the CID if received packet is l2cap flow control credit ind */
    if (cid == HCI_ARB_LE_L2CAP_SIGNALLING_CID)
    {
        if ((hdrLen == HCI_ARB_MIN_LECOC_CREDIT_HDR) &&
            (hdr[HCI_ARB_L2CAP_TYPE_POS] == HCI_ARB_L2CAP_FLOW_CONTROL_CREDIT))
        {
            uint16 remoteCid = HCI_ARB_GET_UINT16(&hdr[HCI_ARB_LECOC_CREDIT_CID_POS]);

            for (rxFilter = aclInst->rxFilter; rxFilter; rxFilter = rxFilter->next)
            {
                if ((rxFilter->protocol == HCI_ARB_LECOC_PROTOCOL) &&
                    (rxFilter->proto.lecoc != NULL) &&
                    (rxFilter->proto.lecoc->remoteCid == remoteCid))
                {
                    return TRUE;
                }
            }
        }
        return FALSE;
    }

    for (rxFilter = aclInst->rxFilter; rxFilter; rxFilter = rxFilter->next)
    {
        if ((rxFilter->protocol == HCI_ARB_LECOC_PROTOCOL) && 
            (rxFilter->localCid == cid))
        {
            offloaded = TRUE;
            break;
        }
    }
    return offloaded;
}

bool HciArbIsGattHandleOffloaded(HciArbAclInst *aclInst, 
                                 HciArbHciHandle handle, HciArbGattRole role)
{
    HciArbRxFilter *rxFilter;

    for (rxFilter = aclInst->rxFilter; rxFilter; rxFilter = rxFilter->next)
    {
        HCI_LOG_DEBUG("Proto %x ", rxFilter->protocol);
        if ((rxFilter->protocol == HCI_ARB_GATT_PROTOCOL) &&
            (rxFilter->proto.gatt != NULL))
        {
            HCI_LOG_DEBUG("GATT Role %d Num %d", rxFilter->proto.gatt->role, 
                          rxFilter->proto.gatt->numHandles);        
        }
        if ((rxFilter->protocol == HCI_ARB_GATT_PROTOCOL) &&
            (rxFilter->proto.gatt != NULL) &&
            (rxFilter->proto.gatt->role == role))
        {
            uint16 i;
            HciArbGattCtx *gatt = rxFilter->proto.gatt;

            for (i = 0; i < gatt->numHandles; i++)
            {
                HCI_LOG_DEBUG("Handle 0x%x Attr Handle 0x%x", handle, gatt->attrHandle[i]);
                if (handle == gatt->attrHandle[i])
                {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

bool HciArbIsGattRspOffloaded(HciArbHciHandle hciHandle, HciArbL2capCid cid,
                              HciArbGattRole role)
{
    HciArbStack sender = HCI_ARB_PRIMARY_STACK;
    bool offloaded;

    if (gHciArb.attSeqCb)
    {
        sender = gHciArb.attSeqCb(hciHandle, role, cid);
    }
    offloaded = (sender == HCI_ARB_PRIMARY_STACK) ? FALSE : TRUE;
    HCI_LOG_INFO("ATT response offloaded %x", offloaded);
    return offloaded;
}

bool HciArbIsGattOffloadedForAclData(HciArbAclInst *aclInst, uint8 *hdr,
                                     uint16 hdrLen, uint16 cid)
{
    bool offloaded = FALSE;
    HciArbHciHandle handle;
    HciArbAttOpcode op;

    if (hdrLen < HCI_ARB_MIN_GATT_HDR)
        return FALSE;

    op = hdr[HCI_ARB_ATT_OPCODE_POS];

    HCI_LOG_DEBUG("ATT opcode 0x%x", op);

    switch (op)
    {
        case HCI_ARB_ATT_OP_HANDLE_VALUE_NOT:
        {
            if (hdrLen > HCI_ARB_MIN_ATT_HDR_HDL)
            {
                handle = HCI_ARB_GET_UINT16(&hdr[HCI_ARB_ATT_HANDLE_POS]);
                HCI_LOG_DEBUG("Notification Handle 0x%x", handle);
                offloaded = HciArbIsGattHandleOffloaded(aclInst, handle, 
                                                        HCI_ARB_GATT_CLIENT);
            }
            break;
        }

        case HCI_ARB_ATT_OP_WRITE_REQ:
        case HCI_ARB_ATT_OP_WRITE_CMD:
        case HCI_ARB_ATT_OP_READ_REQ:
        case HCI_ARB_ATT_OP_READ_BLOB_REQ:
        case HCI_ARB_ATT_OP_PREPARE_WRITE_REQ:
        {
            if (hdrLen >= HCI_ARB_MIN_ATT_HDR_HDL)
            {
                handle = HCI_ARB_GET_UINT16(&hdr[HCI_ARB_ATT_HANDLE_POS]);
                offloaded = HciArbIsGattHandleOffloaded(aclInst, handle,
                                                        HCI_ARB_GATT_SERVER);
                if (op == HCI_ARB_ATT_OP_PREPARE_WRITE_REQ)
                {
                    aclInst->prepWriteOffloaded = offloaded;
                }
            }
            break;
        }

        case HCI_ARB_ATT_OP_EXECUTE_WRITE_REQ:
        {
            offloaded = aclInst->prepWriteOffloaded;
            break;
        }

        case HCI_ARB_ATT_OP_HANDLE_VALUE_CFM:
        {
            offloaded = HciArbIsGattRspOffloaded(aclInst->hciHandle,
                                                 cid, HCI_ARB_GATT_SERVER);
        }
        break;

        case HCI_ARB_ATT_OP_MULTI_HANDLE_VALUE_NOT:
        {

        }
        break;

        case HCI_ARB_ATT_OP_HANDLE_VALUE_IND:
        {
            if (hdrLen >= HCI_ARB_MIN_ATT_HDR_HDL)
            {
                handle = HCI_ARB_GET_UINT16(&hdr[HCI_ARB_ATT_HANDLE_POS]);
                offloaded = HciArbIsGattHandleOffloaded(aclInst, handle,
                                                        HCI_ARB_GATT_CLIENT);
            }
        }
        break;

        default:
        {
            if (op & HCI_ARB_ATT_OP_RSP_MASK)
            {
                offloaded = HciArbIsGattRspOffloaded(aclInst->hciHandle, cid,
                                                     HCI_ARB_GATT_CLIENT);
            }
            break;
        }
    }
    return offloaded;
}

HciArbStack HciArbFindStackForAclRx(HciArbAclInst *aclInst, CsrHciAclDataInd *prim)
{
    uint16 aclLen = CsrMblkGetLength(prim->data);
    HciArbStack destStack = HCI_ARB_PRIMARY_STACK;

    if ((aclInst != NULL) && 
        (aclLen >= HCI_ARB_MIN_L2CAP_HDR))
    {
        uint16 hdrLen = CSRMIN(aclLen, HCI_ARB_MIN_HDR);
        uint8 *hdr = CsrMblkMap(prim->data, 0, hdrLen);
        uint16 cid = HCI_ARB_GET_UINT16(&hdr[HCI_ARB_CID_POS]);

        /* Atleast one offload socket exists */
        if (aclInst->transport == BREDR_ACL)
        {
            if ((aclInst->rxFilter != NULL) &&
                HciArbIsRfcommOffloadedForAclData(aclInst, hdr, hdrLen, cid))
            {
                destStack = HCI_ARB_MICRO_STACK;
                HCI_LOG_DEBUG("RFCOMM PDU for Micro stack CID 0x%x", cid);
            }
        }
        else
        {
            /* LE transport */
            if (cid == HCI_ARB_ATT_CID)
            {
                if (HciArbIsGattOffloadedForAclData(aclInst, hdr, hdrLen, cid))
                {
                    destStack = HCI_ARB_MICRO_STACK;
                    HCI_LOG_DEBUG("GATT PDU for Micro stack CID 0x%x", cid);
                }
            }
            else if ((aclInst->rxFilter != NULL) &&
                     HciArbIsLecocOffloadedForAclData(aclInst, hdr, hdrLen, cid))
            {
                destStack = HCI_ARB_MICRO_STACK;
                HCI_LOG_DEBUG("LECOC PDU for Micro stack CID 0x%x", cid);
            }
        }
        CsrMblkUnmap(prim->data, hdr);
    }
    return destStack;
}


void HciArbProcessAclData(CsrHciAclDataInd *prim)
{
    HciArbStack destStack = HCI_ARB_PRIMARY_STACK;

    if (!HciArbIsPassthroughMode())
    {
        HciArbHciHandle hciHandle = prim->handlePlusFlags & CSR_HCI_ACL_HANDLE_MASK;
        HciArbAclInst *aclInst = HciArbGetInstFromHandle(hciHandle);

        HCI_LOG_DEBUG("ACL Data Rx Hci Handle 0x%x", hciHandle);

        if (hciHandle == HCI_ARB_BT_ENH_LOG_CONN_HANDLE)
        {
            destStack = HCI_ARB_MICRO_STACK;
        }
        else if (HCI_ARB_IS_FIRST_FRAGMENT(prim->handlePlusFlags))
        {
            /* Start fragment */
            destStack = HciArbFindStackForAclRx(aclInst, prim);

            HciArbSetFirstFragmentStack(aclInst, destStack);
        }
        else
        {
            /* Continue fragment */
            destStack = HciArbGetFirstFragmentStack(aclInst);
        }
    }

    if (destStack == HCI_ARB_MICRO_STACK)
    {
        bool consumed = dm_hci_l2cap_data(prim->handlePlusFlags, prim->data);

        if (!consumed)
        {
            CsrMblkDestroy(prim->data);
        }
    }
    else
    {
        uint16 payloadLen = CsrMblkGetLength(prim->data);
        uint16 len = HCI_ARB_ACL_HDR_SIZE + payloadLen;
        uint8 *payload = CsrPmemAlloc(len);
        uint8 *buff = payload;

        write_uint8(&buff, HCI_ARB_HCI_ACL_TYPE);
        write_uint16(&buff, prim->handlePlusFlags);
        write_uint16(&buff, payloadLen);

        (void) CsrMblkCopyToMemory(prim->data, 0, payloadLen, buff);

        HciArbSendHciMsgToPrimaryStack(len, payload);

        CsrMblkDestroy(prim->data);
    }
  
    prim->data = NULL; /* payload has been consumed */          
}
#endif



