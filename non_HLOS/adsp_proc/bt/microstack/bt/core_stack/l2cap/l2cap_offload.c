/*******************************************************************************
Copyright (C) 2024 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "l2cap_private.h"

#ifdef INSTALL_SOCKET_OFFLOAD

#include "socket_offload.h"
#ifdef GATT_SEQUENCING
#include "hci_att_arb.h"
#include "hci_att_arb_prim.h"
#endif /* GATT_SEQUENCING */


/******************************************************************************
  L2CAOFFLOAD_RfcommConn : Update L2CAP conn for RFCOMM conn
******************************************************************************/
l2ca_cid_t L2CAOFFLOAD_RfcommConn(hci_connection_handle_t hci_handle,
                                            uint16_t  local_cid,
                                            uint16_t  remote_cid,
                                            uint16_t  local_mtu,
                                            uint16_t  remote_mtu,
                                            context_t context,
                                            phandle_t phandle)
{
    CIDCB_T *cidcb;
    L2CAP_CHCB_T *chcb;

    if ((chcb = CHME_GetConHandle(hci_handle)) == NULL)
    {
        BLUESTACK_PANIC(PANIC_DM_ACL_SANITY_FAILED);    
        return L2CA_CID_INVALID;
    }

    cidcb = CIDME_GetCidcbRemoteWithHandle(chcb, remote_cid);

    if (cidcb == NULL)
    {
        cidcb = CIDME_AllocCidCb(PROTOCOL_RFCOMM);

        CIDME_InitCidcb(cidcb, chcb, remote_cid, NULL, RFCOMM_PSM, context);

        cidcb->offload_cid = local_cid;
        cidcb->p_handle    = phandle;
        cidcb->local_mtu   = local_mtu;
        cidcb->remote_mtu  = remote_mtu;
        cidcb->state       = CID_ST_OPEN;

        cidcb->next_ptr  = chcb->cidcb_list;
        chcb->cidcb_list = cidcb;
    }
    return cidcb->local_cid;
}

/******************************************************************************
  L2CAOFFLOAD_LecocConn : Update L2CAP conn for LECOC conn
******************************************************************************/
l2ca_cid_t L2CAOFFLOAD_LecocConn(hci_connection_handle_t hci_handle,
                                 uint16_t  local_cid,
                                 uint16_t  remote_cid,
                                 uint16_t  local_mtu,
                                 uint16_t  remote_mtu,
                                 uint16_t  local_mps,
                                 uint16_t  remote_mps,
                                 uint16_t  initial_rx_credits,
                                 uint16_t  initial_tx_credits,
                                 context_t context,
                                 phandle_t phandle)
{
    CIDCB_T *cidcb;
    L2CAP_CHCB_T *chcb;
    CB_FLOWINFO_T *cb_flow;

    if ((chcb = CHME_GetConHandle(hci_handle)) == NULL)
    {
        BLUESTACK_PANIC(PANIC_DM_ACL_SANITY_FAILED);    
        return L2CA_CID_INVALID;
    }

    cidcb = CIDME_AllocCidCb(PROTOCOL_LECOC);
    cb_flow = zpnew(CB_FLOWINFO_T);

    CIDME_InitCidcb(cidcb, chcb, remote_cid, NULL, L2CA_PSM_INVALID, context);

    cidcb->offload_cid = local_cid;
    cidcb->p_handle    = phandle;
    cidcb->local_mtu   = local_mtu;
    cidcb->remote_mtu  = remote_mtu;
    cidcb->state       = CID_ST_OPEN;

    cb_flow->rx_credits     = initial_rx_credits;
    cb_flow->tot_rx_credits = initial_rx_credits;
    cb_flow->app_rx_credits = initial_rx_credits;
    cb_flow->local_mps      = local_mps;
    cb_flow->tx_credits     = initial_tx_credits;
    cb_flow->remote_mps     = remote_mps - L2CAP_SIZEOF_CID_HEADER;
    cidcb->cb_flow          = cb_flow;

    cidcb->next_ptr  = chcb->cidcb_list;
    chcb->cidcb_list = cidcb;

    dm_amp_reserve_credit(&(chcb->queue), DM_AMP_ACL_TYPE_LE);

    return cidcb->local_cid;
}

/******************************************************************************
  L2CAOFFLOAD_AttConn : Update L2CAP conn context for ATT fixed channel
******************************************************************************/
l2ca_cid_t L2CAOFFLOAD_AttConn(hci_connection_handle_t hci_handle, 
                               uint16_t mtu,
                               context_t context)
{
    CIDCB_T *cidcb;
    L2CAP_CHCB_T *chcb;
    phandle_t phandle = QUEUEID_TO_PHANDLE(ATT_IFACEQUEUE);

    if ((chcb = CHME_GetConHandle(hci_handle)) == NULL)
    {
        BLUESTACK_PANIC(PANIC_DM_ACL_SANITY_FAILED);
        return L2CA_CID_INVALID;
    }

    cidcb = CIDME_GetCidcbRemoteWithHandle(chcb, L2CA_CID_ATTRIBUTE_PROTOCOL);

    if (cidcb == NULL)
    {
        cidcb = CIDME_AllocCidCb(PROTOCOL_ATT);

        CIDME_InitCidcb(cidcb, chcb, L2CA_CID_ATTRIBUTE_PROTOCOL, NULL, L2CA_PSM_INVALID, context);

        cidcb->offload_cid = L2CA_CID_ATTRIBUTE_PROTOCOL;
        cidcb->p_handle    = phandle;
        cidcb->local_mtu   = mtu;
        cidcb->remote_mtu  = mtu;
        cidcb->state       = CID_ST_OPEN_FIXED;

        cidcb->next_ptr  = chcb->cidcb_list;
        chcb->cidcb_list = cidcb;

        dm_amp_reserve_credit(&(chcb->queue), DM_AMP_ACL_TYPE_LE);
    }

    return cidcb->local_cid;
}


/******************************************************************************
  L2CAOFFLOAD_DisconnectCid : Disconnect CID in L2CAP
******************************************************************************/
void L2CAOFFLOAD_DisconnectCid(hci_connection_handle_t hci_handle, uint16_t remote_cid)
{
    CIDCB_T *cidcb;
    L2CAP_CHCB_T *chcb;

    if (((chcb = CHME_GetConHandle(hci_handle)) != NULL) &&
        ((cidcb = CIDME_GetCidcbRemoteWithHandle(chcb, remote_cid)) != NULL))        
    {
        CID_DisconnectCleanup(cidcb);

        CH_CleanupCidcbTx(chcb, cidcb);
    
        CIDME_FreeCidcb(cidcb);
    }
}

void L2CAINT_PassthroughDataReq(uint16_t length, void *p_data)
{
    L2CA_INTERNAL_PASSTHROUGH_DATA_REQ_T *prim = 
        pnew(L2CA_INTERNAL_PASSTHROUGH_DATA_REQ_T);

    prim->type   = L2CA_INTERNAL_PASSTHROUGH_DATA_REQ;
    prim->length = length;
    prim->data   = p_data;
    L2CA_PutIntMsg(prim);
}


L2CAP_CHCB_T * L2CA_ValidatePassthroughDataReq(
        L2CA_INTERNAL_PASSTHROUGH_DATA_REQ_T *req, 
        uint16_t *handle_plus_flags,
        uint16_t * l2ca_seg_len)
{
    uint8_t *map;
    L2CAP_CHCB_T *chcb=NULL;
    uint16_t mblk_len, acl_len;

    /* Currently only MBLK is handled */
    if ((req->length != 0) || (req->data == NULL))      
    {
        BLUESTACK_PANIC(PANIC_OFFLOAD_INVALID_PARAMS);
        return NULL;
    }

    if ((mblk_len = mblk_get_length(req->data)) < 5)
    {
        L2CAP_WARNING("Invalid length < ACL header");
        return NULL;
    }

    map = mblk_map(req->data, 0, 5);
    *handle_plus_flags = UINT16_R(map, 1);
    *l2ca_seg_len = UINT16_R(map, 3);
    acl_len = UINT16_R(map, 3) + 5; /* includes packet type and ACL hdr */
    mblk_unmap(req->data, map);

    if (mblk_len != acl_len)
    {
        L2CAP_WARNING("ACL Data dropped -Mblk len %d, Acl len %d", mblk_len, acl_len);
        return NULL;
    }

    chcb = CHME_GetConHandle(*handle_plus_flags & HCI_CONNECTION_HANDLE_MASK);

    if (chcb == NULL)
    {
        L2CAP_WARNING("ACL Data dropped - ACL doesnt exist");
        return NULL;
    }

    if ((*handle_plus_flags & HCI_PKT_BOUNDARY_MASK) != HCI_PKT_BOUNDARY_FLAG_CONT)
    {
        /* first fragment */
        *l2ca_seg_len -= 4; /* L2CAP header */

        L2CAP_DEBUG("First frag L2CAP Len %d Flags 0x%x", *l2ca_seg_len, *handle_plus_flags);

        if (chcb->data != NULL)
        {
            L2CAP_WARNING("Invalid first fragment received");
            mblk_destroy(chcb->data);
            chcb->data = NULL;
        }
    }
    else
    {
        /* Continue fragment */
        L2CAP_DEBUG("Continue frag L2CAP Len %d Flags 0x%x", *l2ca_seg_len, *handle_plus_flags);

        if ((chcb->data == NULL) || (chcb->bytes_pending == 0))
        {
            BLUESTACK_PANIC(PANIC_OFFLOAD_INVALID_CONTINUE_FRAG);
        }
    }
    return chcb;
}

void L2CAINT_ProcessPassthroughDataReq(L2CA_INTERNAL_PASSTHROUGH_DATA_REQ_T *req)
{
    uint8_t *map;
    L2CAP_CHCB_T *chcb;
    uint16_t handle_plus_flags, l2ca_seg_len;
#ifdef GATT_SEQUENCING
    l2ca_cid_t l2cap_cid = L2CA_CID_INVALID;
#endif /* GATT_SEQUENCING */

    chcb = L2CA_ValidatePassthroughDataReq(req, &handle_plus_flags, &l2ca_seg_len);
    if (chcb == NULL)
    {
        mblk_destroy(req->data);
        req->data = NULL;
        return;
    }

    if ((handle_plus_flags & HCI_PKT_BOUNDARY_MASK) != HCI_PKT_BOUNDARY_FLAG_CONT)
    {
        /* first fragment */
        chcb->data = req->data;
        
        map = mblk_map(req->data, 5, 2);
        chcb->bytes_pending = UINT16_R(map, 0); /* Total L2CAP len */
        mblk_unmap(req->data, map);

        L2CAP_DEBUG("Total Len %d ", chcb->bytes_pending);
    }
    else
    {
        /* Continue fragment */
        chcb->data = mblk_add_tail(req->data, chcb->data);
    }

    chcb->bytes_pending -= l2ca_seg_len;

    if (chcb->bytes_pending == 0)
    {
        TXQUEUE_T *queue;
        TXQE_T *txqe;

        txqe = (TXQE_T*)zpmalloc(sizeof(TXQE_T));
        txqe->type = L2CAP_FRAMETYPE_RAW;
        txqe->mblk = chcb->data;
        txqe->cid  = L2CA_CID_INVALID;

        chcb->data = NULL;

        /* Stuff data into queue */
        queue = &(chcb->queue);

        L2CAP_DEBUG("Processing buffered PDUs");

#ifdef GATT_SEQUENCING
        if(txqe->mblk)
        {
            map = mblk_map(txqe->mblk, 7, 2);
            l2cap_cid = UINT16_R(map, 0);
            mblk_unmap(txqe->mblk, map);
        }

         /* check if the message is ATT message. */
        if(l2cap_cid == L2CA_CID_ATTRIBUTE_PROTOCOL)
        {
            HciAttArbAttMsgSend(HCI_ATT_SENDER_PRIMARY_STACK, txqe, chcb, L2CA_PRIMARY_STACK_PRIORITY, FALSE);
        }
        else
#endif /* GATT_SEQUENCING */
        {
            CH_DataTxAppend(queue, txqe, L2CA_PRIMARY_STACK_PRIORITY);
            CH_DataSendQueued(chcb, queue, L2CA_PRIMARY_STACK_PRIORITY, FALSE);
        }
    }
    else if (chcb->bytes_pending < 0)
    {
        L2CAP_INFO("Invalid length");
        L2CAP_PANIC(PANIC_OFFLOAD_INVALID_PARAMS, "Offload Invalid Params");
    }
    else
    {
        L2CAP_DEBUG("Bytes pending %d ", chcb->bytes_pending);
    }
    req->data = NULL;
}


#endif /* INSTALL_SOCKET_OFFLOAD */
