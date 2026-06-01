/*******************************************************************************
Copyright (C) 2008 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

\brief  This file handles L2CAP connections, there is a one-to-one
        mapping between a L2CAP connection and an ACL.
*******************************************************************************/

#include "l2cap_private.h"
#ifdef GATT_SEQUENCING
#include "hci_att_arb.h"
#include "hci_att_arb_prim.h"
#endif /* GATT_SEQUENCING */

static bool_t CH_CidIsValid(L2CAP_CHCB_T *chcb, l2ca_cid_t cid);


static uint8_t CH_SendACLDataPacket(
        TXQUEUE_T *queue,
        uint16_t handle_plus_flags,
        L2CAP_CH_DATA_SIZES_T *sizes,
        TXQE_T *txqe)
{
    MBLK_T *head = NULL, *tail = NULL, *mblk = NULL;

#ifdef INSTALL_SOCKET_OFFLOAD
    if (txqe->type == L2CAP_FRAMETYPE_RAW)
    {
        uint8_t map[5];

        CsrMblkCopyToMemory(txqe->mblk, txqe->sent, 5, &map[0]);
        handle_plus_flags = UINT16_R(&map[0], 1);
        sizes->body = (UINT16_R(&map[0], 3)) + 5; /* ACL len + packet type + ACL hdr */
    }
#endif

    if (sizes->body == 0 ||
            (mblk = mblk_duplicate_region(txqe->mblk,
                                          txqe->sent,
                                          sizes->body)) != NULL)
    {
        if (sizes->header == 0 ||
                (head = mblk_data_create(pcopy(txqe->header, sizes->header),
                                         sizes->header,
                                         TRUE)) != NULL)
        {
            mblk = mblk_add_head(head, mblk);

            if (sizes->trailer == 0 ||
                (tail = mblk_data_create(
                    pcopy(txqe->trailer + txqe->trailersent, sizes->trailer),
                          sizes->trailer,
                          TRUE)) != NULL)
            {
                mblk = mblk_add_tail(tail, mblk);

                QBL_UNUSED(queue);

#ifdef INSTALL_SOCKET_OFFLOAD
                if (txqe->type == L2CAP_FRAMETYPE_RAW)
                {
                    if ((handle_plus_flags & HCI_PKT_BOUNDARY_MASK) != HCI_PKT_BOUNDARY_FLAG_CONT)
                    {
                        L2CAP_DEBUG("First fragment Flags 0x%x Len %d ", 
                            handle_plus_flags, sizes->body);
                    }
                    else
                    {
                        L2CAP_DEBUG("Continue Fragment Flags 0x%x Len %d ", 
                            handle_plus_flags, sizes->body);
                    }

                    handle_plus_flags = ACL_SOURCE_PRIMARY_STACK;
                }
#endif

                CsrBtHcishimAclDataReq(L2CA_AMP_CONTROLLER_BT,
                                       0,
                                       handle_plus_flags,
                                       mblk,
                                       INVALID_PHANDLE);

                return HCI_SUCCESS;
            }
        }
        mblk_destroy(mblk);
    }

    return HCI_ERROR_MEMORY_FULL;
}


#ifdef INSTALL_L2CAP_LECOC_CB
/*! \brief Count the number of LE L2CAP frames, queued for transmission on
           connection oriented channels.

    Traverse fully the Credit Based flow control transmit queue corresponding
    to the channel, so as to find the number of LE L2CAP frames that are
    pending to be transmitted across HCI.

    \param cidcb Channel Control Block.
    \returns Number of queued Tx frames on the specified channel identifier.
*/
uint16_t CH_getQueuedCnt(CIDCB_T *cidcb)
{
    uint16_t queuedCnt = 0;

    if(cidcb->cb_flow != NULL)
    {
        FLOWPKT_T *node = cidcb->cb_flow->tx_head;
        while(node != NULL)
        {
            queuedCnt++;
            node = node->next;
        }
    }

    return queuedCnt;
}

/*! \brief Check whether its data frame on L2CAP channel with Credit Based
           flow control.

    \param cidcb Channel Control block.
    \param type Data frame type.
    \returns TRUE : If its a data frame on L2CAP channel with Credit Based
                    flow control
             FALSE: Otherwise.
*/
bool_t CH_isCBFC_dataFrame(CIDCB_T *cidcb, uint32_t type)
{
    return (cidcb != NULL && CID_IsCBFlowControl(cidcb) &&
            (type == L2CAP_FRAMETYPE_NORMAL));
}

/* Just a wrapper export */
void CH_CBDataTxAppend(TXQUEUE_T *queue, TXQE_T *txqe, uint8_t priority)
{
    CH_DataTxAppend(queue, txqe, priority);
}
#endif

/*! \brief Sends head of transmit queue element over ACL

    This function will transmit the tx_queue head over the ACL. If a
    transmission is already pending (due to HCI fragmentation), we
    continue where we left. It is crucial that this function is as
    fast as possible because we are guaranteed to have the necessary
    credits so link is just waiting to be saturated! :-D

    \param chcb Pointer to CHCB instance.
    \param queue The Queue
*/
static bool_t CH_DataTx(L2CAP_CHCB_T *chcb,
                        TXQUEUE_T *queue)
{
    TXQE_T *txqe;
    mblk_size_t mblk_size;
    uint16_t handle_plus_flags;
    uint16_t packet_boundary_flag = HCI_PKT_BOUNDARY_FLAG_CONT;
    L2CAP_CH_DATA_SIZES_T data_sizes;

    /* Sanity */
    if(queue->tx_current == NULL)
    {
        return FALSE;
    }

    patch_fn(l2cap_shared_patchpoint);
    
    /* Prepare various bits */
    txqe = queue->tx_current;

    handle_plus_flags = CH_GET_HANDLE(chcb);

    mblk_size = ((txqe->mblk != NULL)
                 ? (mblk_size_t)mblk_get_length(txqe->mblk)
                 : (mblk_size_t)0);

    /* First packet to go out. Setup flushable/non-flushable boundary
     * flag on-host: Disable flush for non-2.1 controllers and
     * infinite channels. On-chip, radioio will know what to do */
    data_sizes.header = 0;
    if(txqe->sent == 0)
    {
        data_sizes.header  = txqe->headersize;

#ifdef BUILD_FOR_HOST
        if(!(CH_IS_CONNECTIONLESS(chcb)) &&
           LYMCB_TEST(LYM_Packet_Boundary_Flag_Supported) &&
           (txqe->flush == FLUSH_INFINITE_TIMEOUT))
        {
            packet_boundary_flag = HCI_PKT_BOUNDARY_FLAG_START_NONFLUSH;
        }
        else
#endif
        {
            packet_boundary_flag = HCI_PKT_BOUNDARY_FLAG_START_FLUSH;
        }
    }

    /* Send HCI packets */
    do
    {
        uint16_t credits;

        data_sizes.body = mblk_size - txqe->sent;
        data_sizes.trailer = txqe->trailersize - txqe->trailersent;
   
        /* Request credits. Exit if none to be had. */
        if ((credits = dm_amp_getdatacredits(queue, &data_sizes, 
                               CH_GET_ACL_TYPE(chcb))) == 0)
        {
            L2CAP_INFO("CH_DataTx credits not available"); 
        
            return FALSE;
        }

        txqe->credits += credits;

        switch (CH_SendACLDataPacket(
                    queue,
                    (uint16_t)(handle_plus_flags | packet_boundary_flag),
                    &data_sizes,
                    txqe))
        {
            case HCI_ERROR_MEMORY_FULL:
                return FALSE;
        }

        /* Increment offsets */
        txqe->sent += data_sizes.body;
        txqe->trailersent += data_sizes.trailer;

        /* All subsequent packets will be continuations without any
         * L2CAP headers */
        packet_boundary_flag = HCI_PKT_BOUNDARY_FLAG_CONT;
        data_sizes.header = 0;

        L2CAP_DEBUG("CH_DataTx txqe->sent %d Mblk size %d body %d", 
            txqe->sent, mblk_size, data_sizes.body);
    }
    while ((txqe->sent != mblk_size) ||
           (txqe->trailersent != txqe->trailersize));

    /* Free data now. In case of F&EC backup data, the refcount will
     * make sure that the data is still around */
    mblk_destroy(txqe->mblk);
    txqe->mblk = NULL;

    /* Move TXQE to the end of the done queue and clear the tx_current
     * element so we can start next transmission. This MUST be the
     * last thing we do as we reuse the txqe variable! */
    if(queue->tx_done == NULL)
    {
        queue->tx_done = queue->tx_current;
    }
    else
    {
        for(txqe = queue->tx_done; txqe->next != NULL; txqe = txqe->next)
        {
            /* nop */
        }
        txqe->next = queue->tx_current;
    }
    queue->tx_current = NULL;

    L2CAP_DEBUG("CH_DataTx tx_current set to NULL");

    return TRUE;
}

/*! \brief Transmission complete

    When HCI sends us NCPs indications, we scan the list of
    transmitted elements and generate data write or do callbacks so
    users can know that data they've sent has actually been shovelled
    out.

    \param queue Pointer to queue
    \param ncp Number of HCI packets sent
*/
static void CH_DataSendComplete(TXQUEUE_T *queue, uint16_t ncp)
{
    TXQE_T *txqe;
    uint16_t consume;
    uint16_t ncpPrimaryStack = 0;
        
    txqe = queue->tx_done;
    
    if (ncp > queue->credits_taken)
    {
        /* This should never happen. */
#ifdef L2CAP_HCI_DATA_CREDIT_CHECKS_PANIC
        BLUESTACK_PANIC(PANIC_L2CAP_HCI_DATA_CREDITS_INCONSISTENT);
#else
        BLUESTACK_WARNING(CONTROL_ERR);
#endif

        queue->credits_taken = 0;
    }
    else
        queue->credits_taken -= ncp;

    /* Make sure that there is always one credit reserved just for us. */
#ifdef SUPPORT_SEPARATE_LE_BUFFERS
    if(txqe != NULL)
    {
        L2CAP_CHCB_T *chcb = CH_GET_CHCB_FROM_QUEUE(queue);
        dm_amp_reserve_credit(queue, CH_GET_ACL_TYPE(chcb));
    }
#else
    dm_amp_reserve_credit(queue, DM_AMP_ACL_TYPE_BR_EDR);
#endif

    while((ncp > 0) && (queue->tx_done != NULL))
    {
        /* Cache element */
        txqe = queue->tx_done;

        /* Consume credits - make sure we don't underflow */
        consume = MIN(txqe->credits, ncp);
        txqe->credits -= consume;
        ncp -= consume;

        if (txqe->type == L2CAP_FRAMETYPE_RAW)
            ncpPrimaryStack += consume;

        /* Has element been completely sent? */
        if(txqe->credits == 0)
        {
            CIDCB_T *cidcb;
            cidcb = CIDME_GetCidcb(txqe->cid);
            
            /* Invoke special completion functions or send confirm -
             * only makes sense if the CIDCB is assigned */
            if(cidcb != NULL)
            {
                switch(txqe->type)
                {
                    case L2CAP_FRAMETYPE_NORMAL:
                        /* Confirm (function is cidcb-NULL safe) */
                        L2CA_DataWriteCfm(cidcb,
                                          txqe->context,
                                          txqe->sent,
                                          L2CA_DATA_SUCCESS);
                        break;
                                                
                    default:
                        /* No completion */
                        break;
                }
                
                /* Send any delayed confirms for flushed packets. */
                CH_EmptyAbortQueue(cidcb,
                                   &txqe->aborted,
                                   L2CA_DATA_LOCAL_ABORTED);
            }
            
            /* Remove TXQE from list and free it */
            queue->tx_done = txqe->next;
            pfree(txqe);
            
        }
    }

    /* Check current element too */
    txqe = queue->tx_current;
    if((ncp > 0) && (txqe != NULL))
    {
        consume = MIN(txqe->credits, ncp);
        txqe->credits -= consume;
        ncp -= consume;
        
        if (txqe->type == L2CAP_FRAMETYPE_RAW)
            ncpPrimaryStack += consume;
    }

    if (ncpPrimaryStack != 0)
    {
        L2CAP_CHCB_T *chcb = CH_GET_CHCB_FROM_QUEUE(queue);
        HciArbSendNocpToPrimaryStack(CH_GET_HANDLE(chcb), ncpPrimaryStack);
    }
        
#ifdef L2CAP_HCI_DATA_CREDIT_CHECKS
    /* ACL Data credit debugging.
       We should never get back more credits than we have consumed. */
    if (ncp != 0)
    {
#ifdef L2CAP_HCI_DATA_CREDIT_CHECKS_PANIC
        BLUESTACK_PANIC(PANIC_L2CAP_HCI_DATA_CREDITS_INCONSISTENT);
#else
        BLUESTACK_WARNING(CONTROL_ERR);
#endif
    }
#endif
}

#ifdef L2CAP_HCI_DATA_CREDIT_SLOW_CHECKS

/*! \brief Count number of outstanding credits for given chcb.
 
    \param chcb Pointer to Connection Handle Control Block.
    \returns Number of outstanding credits.
*/
uint16_t CH_UsedDataCredits(L2CAP_CHCB_T *chcb)
{
    TXQE_T *qe;
    uint16_t credits = 0;

    if (chcb != NULL)
    {
        if (chcb->queue.tx_current != NULL)
            credits = chcb->queue.tx_current->credits;

        for (qe = chcb->queue.tx_done; qe != NULL; qe = qe->next)
            credits += qe->credits;

        if (credits != chcb->queue.credits_taken)
#ifdef L2CAP_HCI_DATA_CREDIT_CHECKS_PANIC            
            BLUESTACK_PANIC(PANIC_L2CAP_HCI_DATA_CREDITS_INCONSISTENT);
#else
        {
            L2CAP_DEBUG("CH_UsedDataCredits Credit mismatch credits %d credits taken %d", 
                credits, chcb->queue.credits_taken);
        }
#endif

        credits += chcb->queue.reserved_credit;
    }

    return credits;
}
#endif



/*! \brief Send queued data/signals

    Send any queued signals or data, signals take preference over
    data.  If there is no data to be sent a check is made to see if
    the connection is idle.

    It's safe for everybody to call this function with a valid CHCB.

    \param chcb Pointer to CHCB instance.
    \param queue Queue
    \param priority Priority
    \param from_nhcp
*/
void CH_DataSendQueued(L2CAP_CHCB_T *chcb,
                       TXQUEUE_T *queue,
                       uint8_t priority,
                       bool_t from_nhcp)
{
    TXQE_T *txqe;

    /* Make sure that the channel is in a valid state and that we are not
     * already running. As we are dealing with callbacks on a queue it's
     * necessary to avoid flooding the call stack with re-scheduling.
     * The simplest solution is a lock - queue->tx_active, which will
     * work because we're running in the background. */
    if((!CH_IS_ULP(chcb) && dm_credit_cb.fc.max_acl_data_packet_length == 0) ||
#ifdef SUPPORT_SEPARATE_LE_BUFFERS
        (CH_IS_ULP(chcb) && dm_credit_cb.le_fc.max_acl_data_packet_length == 0) ||
#endif 
       !CH_IS_CONNECTED(chcb) || 
       queue->tx_active)
    {
        L2CAP_DEBUG("CH_DataSendQueued tx_active active");
        return;
    }
    L2CAP_DEBUG("CH_DataSendQueued priority %d from nocp %d", priority, from_nhcp);

    queue->tx_active = TRUE;

    /* Loop while we still have credits we can use */
    for(;;)
    {
        /* Check if there is any data waiting for credits */
        if(queue->tx_current != NULL)
        {
            CIDCB_T *cidcb = CIDME_GetCidcb(queue->tx_current->cid);

            L2CAP_DEBUG("CH_DataSendQueued tx_current not NULL");

            /* Resume sending data if it is of sufficient priority */
            if ((!from_nhcp && 
                 cidcb != NULL && 
                 cidcb->priority > priority)
             || (!CH_DataTx(chcb, queue)))
            {
                /* Channel is of insufficient priority, or it is not ready
                 * for more, so break out until we receive more credits */
                break;
            }
            continue;
        }

        /* Assign current TXQE element, but don't transfer it completely */
        txqe = queue->tx_queue[priority];

        if(txqe != NULL)
        {
            L2CAP_DEBUG("CH_DataSendQueued txqe not NULL");
            /* Move queue forward as we now know that element is valid */
            queue->tx_queue[priority] = txqe->next;
            queue->tx_current = txqe;
            txqe->next = NULL;
           
            /* Does this transmitter require a pre-send callback? */
            if(txqe->headersize == 0)
            {
                switch(txqe->type)
                {
                    case L2CAP_FRAMETYPE_SIGNAL:
                        /* Pre-send callback for signals */
                        SIGH_SignalPresend(chcb, txqe);
                        break;

                    default:
                        /* No callbacks */
                        break;
                }
            }

            /* Queue element already setup, so just call
             * transmitter */
            if((txqe->headersize != 0) || (txqe->mblk != NULL))
            {
                if(!CH_DataTx(chcb, queue))
                {
                    /* Channel got busy, so bail out */
                    break;
                }
            }
            else
            {
                /* Callback didn't have any data, so just remove the
                 * current element. Queue has already been moved
                 * forward */
                CH_EmptyAbortQueue(NULL, &txqe->aborted, L2CA_DATA_SILENT);
                pfree(txqe);
                queue->tx_current = NULL;
            }
            continue;
        }
        else
        {
            /* Queue is empty */
            queue->tx_queue[priority] = NULL;
            queue->tx_current = NULL;
        }

        /* No signals or data queued, so we have finished. Note that
         * both flow control and the standard data sender above
         * restarts the while() loop. */
        break;
    }

    queue->tx_active = FALSE;
}

/*! \brief Fill out standard header in txqe

    Fill out the header and headersize fields for a txqe. We always
    fill out the CID and length fields according to the spec. Any
    further header will be copied in case parameters are given.

    \param txqe Tx queue element to modify
    \param cid CID to copy into header
    \param mblksize Size of payload (without headers!)
    \param head Pointer to extra header fields
    \param headsize Length of extra headers in octets
*/
void CH_DataAddHeader(TXQE_T *txqe,
                      uint16_t cid,
                      uint16_t mblksize,
                      uint8_t *head,
                      uint8_t headsize)
{
    uint8_t *dataptr;
    txqe->headersize = L2CAP_SIZEOF_CID_HEADER + headsize;

    /* Fill common header */
    dataptr = txqe->header;
    write_uints(&dataptr,
                URW_FORMAT(uint16_t, 2, UNDEFINED, 0, UNDEFINED, 0),
                mblksize + headsize,
                cid);

    /* Copy in optional header fields */
    qbl_memscpy(dataptr, headsize, head, headsize);
}

/*! \brief Add tx elememt to tail of transmission queue.

    \param queue Tx Queue of signal & data PDUs.
    \param txqe Tx element appended to Tx Queue.
    \param priority Priority associated with Tx Queue.
*/
void CH_DataTxAppend(TXQUEUE_T *queue,
                     TXQE_T *txqe,
                     uint8_t priority)
{
    /* Insert into tx_queue tail */
    if(queue->tx_queue[priority] == NULL)
    {
        queue->tx_queue[priority] = txqe;
    }
    else
    {
        TXQE_T *tail;
        for(tail = queue->tx_queue[priority]; tail->next != NULL; tail = tail->next)
        {
            /* nop */
        }
        tail->next = txqe;
    }
}

/*! \brief Add a CID-style data element to the queue

    Add data to the (basic) transmit queue, possibly transmitting it
    in the same go. This function should only be used by the normal
    (ie. basic) transmit path.

    We need to be able to jump the normal transmit queue for
    normal data elements (the duplicate request signal handling
    comes spring to mind). This function allows both normal
    prepend and append

    \param cidcb Pointer to channel control block.
    \param context Opaque value to be returned in _CFM.
    \param mblk Pointer to data MBLK.
    \returns TRUE if successful, otherwise FALSE.
*/
bool_t CH_DataTxBasic(CIDCB_T *cidcb,
                    context_t context,
                    MBLK_T *mblk)
{
    TXQE_T *txqe;
    TXQUEUE_T *queue;
    FRAMETYPE_T frame_type = L2CAP_FRAMETYPE_NORMAL;
    uint8_t *psm_len = NULL;
    uint8_t size = 0;
    L2CAP_CHCB_T *chcb = cidcb->chcb;
#ifdef GATT_SEQUENCING
    l2ca_cid_t l2cap_cid;
#endif /* GATT_SEQUENCING */

    txqe = (TXQE_T*)zpmalloc(sizeof(TXQE_T) + L2CAP_SIZEOF_CID_HEADER + size);
    txqe->type = frame_type;
    txqe->cid = cidcb->local_cid;
    txqe->context = context;
    txqe->mblk = mblk;
    txqe->flush = cidcb->local_flush_to;
    CH_DataAddHeader(txqe, cidcb->remote_cid, mblk_get_length(mblk),
                        psm_len, size);

    queue = &(chcb->queue);

#ifdef GATT_SEQUENCING
    l2cap_cid = cidcb->remote_cid & 0x0000FFFF;

    /* check if the message is ATT message. */
    if(l2cap_cid == L2CA_CID_ATTRIBUTE_PROTOCOL)
    {
        HciAttArbAttMsgSend(HCI_ATT_SENDER_MICRO_STACK, txqe, chcb, cidcb->priority, FALSE);
    }
    else
#endif /* GATT_SEQUENCING */
    {
        CH_DataTxAppend(queue, txqe, cidcb->priority);
        CH_DataSendQueued(chcb, queue, cidcb->priority, FALSE);
    }

    return TRUE;
}


/*! \brief Add signal-style callback element to head of queue

    Signals need to take priority over all other data packets, so we
    place a callback on the head of the basic queue. Using the
    callback avoids a runtime check on the queue loop.
*/
void CH_DataTxSignalCallback(L2CAP_CHCB_T *chcb)
{
    TXQE_T *txqe;
    TXQUEUE_T *queue = &(chcb->queue);

    /* Don't flood the tx-queue */
    if(!chcb->signal_scheduled)
    {
        txqe = (TXQE_T*)zpmalloc(sizeof(TXQE_T) + L2CAP_SIZEOF_CID_HEADER);
        txqe->type = L2CAP_FRAMETYPE_SIGNAL;
        
        /* Insert as new head so signals take priority */
        txqe->next = queue->tx_queue[L2CA_SIGNALLING_PRIORITY];
        queue->tx_queue[L2CA_SIGNALLING_PRIORITY] = txqe;
        chcb->signal_scheduled = TRUE;

        CH_DataSendQueued(chcb, queue, L2CA_SIGNALLING_PRIORITY, FALSE);
    }
}


/*! \brief Check validity of CID

    Check if the CID is valid for the transport on which its
    received.

    Returns TRUE if valid, FALSE otherwise.
*/
static bool_t CH_CidIsValid(L2CAP_CHCB_T *chcb, l2ca_cid_t cid)
{
    bool_t valid = FALSE;

#ifndef DISABLE_DM_BREDR
    /* Transport is BREDR */
    if(!CH_IS_ULP(chcb))
    {
        switch (cid)
        {
            case L2CA_CID_SIGNAL:
            case L2CA_CID_CONNECTIONLESS:
            case L2CA_CID_AMP_MANAGER:
            case L2CA_CID_BREDR_SECURITY_MANAGER:
            case L2CA_CID_AMP_TEST_MANGAGER:
                valid = TRUE;
                break;

            default:
                /* Not within dynamic range then discard */
                if(cid >= L2CA_CID_DYNAMIC_FIRST)
                {
                    valid = TRUE;
                }
                break;
        }
    }
    /* LE */
    else 
#endif
#ifdef INSTALL_ULP
    /* Transport is LE */
    if(CH_IS_ULP(chcb))
    {
        switch (cid)
        {
            case L2CA_CID_LE_SIGNAL:
            case L2CA_CID_ATTRIBUTE_PROTOCOL:
            case L2CA_CID_SECURITY_MANAGER:
                valid = TRUE;
                break;

            default:
                /* Not within dynamic range then discard */
                if(cid >= L2CA_LE_CID_DYNAMIC_FIRST && cid <= L2CA_LE_CID_DYNAMIC_LAST)
                {
                    valid = TRUE;;
                }
                break;
        }
    }
#endif
    return valid;
}


/*! \brief Receive signals/data packets.

    This function is called to handle receive L2CAP signals/data.

    \param chcb Pointer to CHCB instance.
    \param mblk Data primitive with MBLK containing signals or data PDU.
    \param bc TRUE if received data is broadcast.
*/
void CH_DataRx(L2CAP_CHCB_T *chcb, MBLK_T *mblk, bool_t bc)
{
    mblk_size_t mblk_size;
    uint16_t payload_size;
    l2ca_cid_t cid = L2CA_CID_INVALID;
    uint8_t header[L2CAP_SIZEOF_CID_HEADER];
    CIDCB_T *cidcb = NULL;

    /* Make sure that we can read in the complete header */
    if(mblk_read_head(&mblk, header, L2CAP_SIZEOF_CID_HEADER) == L2CAP_SIZEOF_CID_HEADER)
    {
        /* Get size of payload */
        payload_size = UINT16_R(header, L2CAP_PKT_POS_LENGTH);

        /* Get the CID if we can */
        cid = UINT16_R(header, L2CAP_PKT_POS_CID);

        /* Fetch the CIDCB structure now. If it doesn't exist, we'll
         * handle it further down. Signals don't care about this  */
        cidcb = CIDME_GetCidcbWithHandle(chcb, cid);

        /* Get MBLK data size */
        mblk_size = mblk_get_length(mblk);

        /* Check the payload size is correct, receiving a partial
         * L2CAP packet is expected behaviour when the remote device
         * is using flush timeout */

        /* Make sure that size matches and that only connectionless L2CAP
           is sent via broadcast. */
        if(payload_size == 0)
        {
            /* An L2CAP packet with no data will not be useful.
             * Do nothing and let it get discarded */
        }
        else if(payload_size == mblk_size && (cid == L2CA_CID_CONNECTIONLESS || !bc))
        {
            /* Check if CID is valid on the transport received */
            if(CH_CidIsValid(chcb, cid))
            {
                /* Route by cid. */
                switch (cid)
                {
#ifdef INSTALL_ULP
                    case L2CA_CID_LE_SIGNAL:
                        /* Low energy signalling channel */
                        if (payload_size >= L2CAP_SIZEOF_SIGNAL_HEADER)
                            SIGH_LeSignalReceive(chcb, &mblk, payload_size);
                        break;
#endif

                    default:
                    {
                        if (cidcb == NULL &&
                            cid < L2CA_CID_DYNAMIC_FIRST)
                        {
                            cidcb = CIDME_GetCidcbRemoteWithHandle(chcb, cid);
                        }

                        /* Data packet. */
                        if (cidcb != NULL)
                        {
                            /* Pass data to CIDCB return to avoid freeing the MBLK*/
                            if (CID_DataReadInd(cidcb, mblk, payload_size, header))
                                return;
                        }
                        break;
                    }
                } /* switch CID */
            }
        } /* payload size ok */
        else
        {

        }
    }

    /* Destroy MBLK */
    mblk_destroy(mblk);
}

/*! \brief Free a tx queue element, possibly with ack

    Helper function to free, and possible acknowledge, a tx queue
    element.

    \param txqe Pointer to TX queue element.
    \param result failure result code or L2CA_DATA_SILENT to send nothing.
*/
void CH_FlushTxqe(TXQE_T *txqe, l2ca_data_result_t result)
{
    CIDCB_T *cidcb;

    cidcb = CIDME_GetCidcb(txqe->cid);

    /* Any send acknowledgments if there's a CIDCB and allowed to do
     * so */
    if(cidcb != NULL && result != L2CA_DATA_SILENT)
    {
        switch (txqe->type)
        {
            case L2CAP_FRAMETYPE_NORMAL:
            case L2CAP_FRAMETYPE_CONNECTIONLESS:
                /* Cfm function is cidcb-NULL-safe */
                L2CA_DataWriteCfm(cidcb, txqe->context, txqe->sent, result);
                break;

            default:
                break;
        }
    }

    CH_EmptyAbortQueue(cidcb, &txqe->aborted, result);
    mblk_destroy(txqe->mblk);
    pfree(txqe);
}

/*! \brief Empty data queue of packets from specified channel.

    This function will empty the data queue of packets from the specified channel,
    the packets will be destroyed.

    \param queue Pointer to queue
    \param cid The channel ID for the packets to destroy. L2CA_CID_INVALID
               can be used to target all TXQEs
    \param result failure result code or L2CA_DATA_SILENT to send nothing.
    \return Number of credits salvaged from queue->tx_current
*/
uint16_t CH_FlushPendingQueueWithCid(TXQUEUE_T *queue,
                                     l2ca_cid_t cid,
                                     l2ca_data_result_t result)
{
    TXQE_T *elp, **elpp, **q, **end;
    uint16_t credits = 0;

    /* Special case for the current tx element */
    if(queue->tx_current != NULL)
    {
        if((queue->tx_current->cid == cid) || (cid == L2CA_CID_INVALID))
        {
            credits = queue->tx_current->credits;
            CH_FlushTxqe(queue->tx_current, result);
            queue->tx_current = NULL;
        }
    }

    /* Traverse the transmit queues */
    for (q = queue->tx_queue, end = q + L2CAP_MAX_TX_QUEUES; q != end; ++q)
    {
        for (elpp = q; (elp = *elpp) != NULL; /* empty */ )
        {
            if (cid == L2CA_CID_INVALID || elp->cid == cid)
            {
                *elpp = elp->next;
                CH_FlushTxqe(elp, result);
            }
            else
                elpp = &elp->next;
        }
    }

    return credits;
}  

/*! \brief Empty the "done" queue
  
    This function will flush the "tx_done" queue for the specified
    queue object. Function will return the number of credits freed.

    \param queue Pointer to queue
    \param result failure result code or L2CA_DATA_SILENT to send nothing.
*/
uint16_t CH_FlushDoneQueue(TXQUEUE_T *queue, l2ca_data_result_t result)
{
    TXQE_T *txqe;
    uint16_t credits = 0;

    while(queue->tx_done != NULL)
    {
        /* Get head element and move to next */
        txqe = queue->tx_done;
        credits += txqe->credits;
        queue->tx_done = txqe->next;
        CH_FlushTxqe(txqe, result);
    }
    return credits;
}

/*! \brief Flush data related to an L2CAP channel in CHCB

    This function will flush out any pending queued data in the CHCB instance
    for the specified CIDCB instance. Upper layer is informed of any
    data that was partially sent.

    \param chcb Pointer to CHCB instance.
    \param cidcb Pointer to CIDCB instance.
    \param result failure result code or L2CA_DATA_SILENT to send nothing.
*/
void CH_FlushCidcbPendingTxDataQueue(L2CAP_CHCB_T *chcb,
                                   CIDCB_T *cidcb,
                                   l2ca_data_result_t result)
{
    /* Empty the data queue of packet belonging to the CID */
    CH_GET_ACL(chcb)->pending_credits
            += CH_FlushPendingQueueWithCid(&(chcb->queue),
                                           cidcb->local_cid,
                                           result);
}

/*! \brief Flush signals related to an L2CAP channel in CHCB

    This function will flush out any queued L2CAP signals related to CIDCB in 
    pending queue or in sent queue (signals for which response is awaited).

    \param chcb Pointer to CHCB instance.
    \param cidcb Pointer to CIDCB instance.
*/
void CH_FlushCidcbTxSignalQueue(L2CAP_CHCB_T *chcb,
                              CIDCB_T *cidcb)
{
    /* Empty the signal queues of signals belonging to this CID */
    SIGH_SignalQueueEmptyWithCid(&chcb->signal_queue,
                                 cidcb->offload_cid);
    SIGH_SignalQueueEmptyWithCid(&chcb->signal_pending,
                                 cidcb->offload_cid);
}


/*! \brief Flush CHCB of queued data and signals.

    This is the main CHCB cleanup function.

    This function frees any queued signals and data on the specified
    CHCB. Upper layer is informed of any data that was partially sent
    unless result == L2CA_DATA_SILENT.

    \param chcb Pointer to CHCB instance.
    \param result failure result code or L2CA_DATA_SILENT to send nothing.
*/
uint16_t CH_FlushChcb(L2CAP_CHCB_T *chcb, l2ca_data_result_t result)
{
    BD_ADDR_T *addr = NULL;
    TXQUEUE_T *queue;
    uint16_t credits;
    uint8_t i; /* counter index for running loops in this function */

    SIGH_SignalQueueEmpty(&chcb->signal_pending, addr);
    SIGH_SignalQueueEmpty(&chcb->signal_queue, addr);

    /* Flush the basic radio transmit queue */
    queue = &(chcb->queue);
    credits = CH_FlushPendingQueueWithCid(queue,
                                          L2CA_CID_INVALID,
                                          result)
            + CH_FlushDoneQueue(queue, result);

    /* Flush the AMP queues */
#ifdef INSTALL_AMP_SUPPORT
    for(queue = chcb->amp_queues; queue != NULL; queue = queue->next_ptr)
    {
        (void)CH_FlushPendingQueueWithCid(queue,
                                          L2CA_CID_INVALID,
                                          result);
        (void)CH_FlushDoneQueue(queue, result);
    }
#endif

    /* Free duplicate signal detection block */
    mblk_destroy(chcb->signal_dup_record.sig_data);
    chcb->signal_dup_record.sig_data = NULL;

    for(i = 0; i < L2CAP_NUM_DUPLICATE_SIGNAL_RECORD; i++)
    {
        chcb->signal_dup_record.sig_attr[i].type = 0;
        chcb->signal_dup_record.sig_attr[i].identifier = 0;
        chcb->signal_dup_record.sig_attr[i].length = 0;
    }
    chcb->signal_dup_record.last_req = 0;


#ifdef BUILD_FOR_HOST
    /* Free ACL reassembly buffer */
    mblk_destroy(chcb->reassem.mblk);
    chcb->reassem.length = 0;
    chcb->reassem.offset = 0;
#endif

    /* Return number of credits freed */
    return credits;
}


/*! \brief Destroy CHCB instance.

    This function destroys a CHCB instance, all CIDS on the CHCB are closed
    down.

    \param chcb Pointer to CHCB instance.
    \param status Close status to report to CIDs.
*/
static uint16_t CH_Destroy(L2CAP_CHCB_T *chcb, uint16_t status)
{
    uint16_t reserved_credit;
    CIDCB_T *cidcb;

    L2CAP_INFO("CH_Destroy");

    /* Notify CIDs connection has gone, CID may remove itself from list */
    /* Use double indirection to detect and cope with CIDCB destruction */
    while (chcb->cidcb_list != NULL)
    {
        cidcb = chcb->cidcb_list->next_ptr;
        CID_DisconnectCleanup(chcb->cidcb_list);

        if (chcb->cidcb_list->remote_cid == L2CA_CID_ATTRIBUTE_PROTOCOL)
        {
            L2CAP_INFO("ATT CID exists. Send disconnect ind");
            L2CA_DisconnectInd(chcb->cidcb_list, 0, L2CA_DISCONNECT_NORMAL);
        }
        CH_CleanupCidcbTx(chcb, chcb->cidcb_list);
    
        CIDME_FreeCidcb(chcb->cidcb_list);
        chcb->cidcb_list = cidcb;
    }

    mblk_destroy(chcb->data);

    /* Flush any data/signals */
    reserved_credit = chcb->queue.reserved_credit;
    chcb->queue.reserved_credit = 0;

#ifdef INSTALL_DLE
    timer_cancel(&chcb->tx_max_octet_id);
#endif /* INSTALL_DLE */
#ifdef GATT_SEQUENCING
    HciAttArbClearOffloadedGATTAclHandle(chcb);
#endif /* GATT_SEQUENCING */

    return CH_FlushChcb(chcb, L2CA_DATA_LINK_TERMINATED) + reserved_credit;
}


/*! \brief Disconnect indication.

    Handle disconnect indication from the device manager.

    \param p_acl Pointer to ACL
    \param reason Reason
*/
uint16_t CH_DisconnectInd(DM_ACL_T *p_acl, uint16_t reason)
{
    /* Destroy this instance */
    return CH_Destroy(CH_GET_CHCB(p_acl), reason);
}


/*! \brief Clean up CIDCB's pending Tx data and signal

    Flushs any Tx pending/sent signals and pending data for the CIDCB.

    \param chcb Pointer to CHCB instance.
    \param cidcb Pointer to CIDCB instance.
*/
void CH_CleanupCidcbTx(L2CAP_CHCB_T *chcb, struct cidtag *cidcb)
{
    if (CH_IS_CONNECTED(chcb))
    {
        /* Flush data for this CIDCB (if required) */
        CH_FlushCidcbPendingTxDataQueue(chcb, cidcb, L2CA_DATA_LINK_TERMINATED);

        /* Flush pending and sent signal queue */
        CH_FlushCidcbTxSignalQueue(chcb, cidcb);
    }
}

/*! \brief NCPs received from HCI

    Indication that packets have been sent across the air.

    \param chcb Pointer to l2cap connection record.
    \param completed_packets number of completed packets.
*/
void CH_CompletedPackets(L2CAP_CHCB_T *chcb, uint16_t completed_packets)
{
    CH_DataSendComplete(&(chcb->queue), completed_packets);
}

/*! \brief Empty the abort queue, possibly sending L2CA_DATAWRITE_CFMs
           to application.

    \param cidcb Pointer to channel control block.
    \param abort_queue Pointer to pointer to queue.
    \param result Set to L2CA_DATA_SILENT to suppress messages to application.
*/
#ifdef INSTALL_L2CAP_DATA_ABORT_SUPPORT
void CH_EmptyAbortQueue(CIDCB_T *cidcb,
                        L2CAP_ABORTED_PACKET_T **abort_queue,
                        l2ca_data_result_t result)
{
    L2CAP_ABORTED_PACKET_T *pkt;

    QBL_UNUSED(cidcb);

    while ((pkt = *abort_queue) != NULL)
    {
        *abort_queue = pkt->next;

        if (result == L2CA_DATA_SILENT)
            pfree(pkt);
        else
            L2CA_PrimSend(&pkt->u.common);
    }
}
#endif /* INSTALL_L2CAP_DATA_ABORT_SUPPORT */


