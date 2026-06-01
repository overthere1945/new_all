/*******************************************************************************

Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.


*******************************************************************************/

#include "l2cap_private.h"


#define L2CAP_CMD_RSP_MASK         0x01 /* bit 0 is set in responses */

typedef struct
{
    uint8_t id;                     /* Signal identifier */
    uint8_t  type;                  /* Signal opcode */
    uint16_t length;                /* in-packet length (don't trust this!) */
    uint16_t length_with_header;    /* Signal size, including L2CAP header */
    const uint8_t *data;            /* Signal data. */
} L2CAP_SIGNAL_INFO_T;

/* Local prototypes */



/*! \brief Detect a duplicate request signal

    If the peer sends us a request signal which is present
    in our past signals records, then it is treated as a duplicate
    request otherwise a new request. If there is a response
    present for duplicate request, send it else ignore it.
*/
static bool_t SIGH_DuplicateDetect(L2CAP_CHCB_T *chcb, L2CAP_SIGNAL_INFO_T *sig)
{
    uint8_t i; /* counter index for running loops in this function */

    for(i = 0; i < L2CAP_NUM_DUPLICATE_SIGNAL_RECORD; i++)
    {
        /* Not a duplicate if any of: type; length; or identifier differ.
           Previously we only checked the identifier, but we need to work
           around the bad behaviour of some headsets. See B-66245.

           A note - we need to resolve dg 934546 to complete matching length concept.
           The issue is - when a response is sent, we match only the signal's ID and type
           from the record queue since length is not present in response signal.
         */
        if(chcb->signal_dup_record.sig_attr[i].identifier == sig->id &&
           chcb->signal_dup_record.sig_attr[i].type == sig->type &&
           chcb->signal_dup_record.sig_attr[i].length == sig->length_with_header)
           break;
    }

    if(i == L2CAP_NUM_DUPLICATE_SIGNAL_RECORD)
        return FALSE;

    /* If duplicate request matches with last request and
       we have a response then resend it. Otherwise ignore signal. */
    if (i == chcb->signal_dup_record.last_req &&
        chcb->signal_dup_record.sig_data != NULL)
    {
        /* Create a new TXQE with the raw response signal */
        TXQE_T *txqe;

        txqe = xzpmalloc(sizeof(TXQE_T) + L2CAP_SIZEOF_CID_HEADER);
        if (txqe != NULL)
        {
            mblk_size_t mblk_size;

            /* Use an MBLK duplicate. This both saves a bit of memory
             * and avoids a horrible race hazard if we should happen
             * to assign a new sig_data before the old one was sent.
             * If we can't create a duplicate then don't bother sending
             * anything at all. */
            mblk_size = mblk_get_length(chcb->signal_dup_record.sig_data);
            txqe->mblk = mblk_duplicate_create(chcb->signal_dup_record.sig_data,
                                               0,
                                               mblk_size);
            if (txqe->mblk != NULL)
            {
                /* Everything has been allocated. Fill out the final
                 * details and stuff it onto the head of the
                 * basic-radio transmit queue */
                CH_DataAddHeader(txqe,
                                 (uint16_t)CH_GET_SIGNAL_CID(chcb),
                                 mblk_size,
                                 NULL,
                                 0);
                txqe->type = L2CAP_FRAMETYPE_NORMAL;
                txqe->flush = FLUSH_INFINITE_TIMEOUT;
                txqe->next = chcb->queue.tx_queue[L2CA_SIGNALLING_PRIORITY];
                chcb->queue.tx_queue[L2CA_SIGNALLING_PRIORITY] = txqe;
                CH_DataSendQueued(chcb, &chcb->queue, L2CA_SIGNALLING_PRIORITY, FALSE);
            }
            else
            {
                pfree(txqe);
            }
        }
    }

    return TRUE;
}


/*! \brief Obtain Signal ID

    This function returns a unique signal ID.

    The spec says:
    A different Identifier must be used for each original command.
    Identifiers should not be recycled until a period of 360
    seconds has elapsed from the initial transmission of the
    command using the identifier.

    The latter part of this statement is ambiguous.  There is no indication of
    what should be done in the eventuality of needing to re-use a signal_id
    less than 360 secs after sending it the first time!

    So, for a particular channel, the first signal will be ID=1, the next ID=2
    and so on, wrapping around from 255 to 1. If a signal is pending with the
    ID that was chosen, then increment on to the next one.

    \param chcb Pointer to CHCB instance.
    \return A new signal ID or zero if none available.
*/
static uint8_t SIGH_SignalIdObtain(L2CAP_CHCB_T *chcb)
{
    SIG_SIGNAL_T *sig_ptr;

    /* Increment signal ID, handle wrap around */
    chcb->signal_id = (chcb->signal_id + 1) & 0xFF;
    if (chcb->signal_id == 0)
        chcb->signal_id = 1;

    /* Check this signal ID isn't used on the pending signal queue */
    for (sig_ptr = chcb->signal_pending; sig_ptr != NULL; sig_ptr = sig_ptr->next_ptr)
    {
        /* Check if this signal has the same signal ID */
        if (sig_ptr->signal_id == chcb->signal_id)
        {
            /* Increment signal ID, handle wrap around */
            chcb->signal_id = (chcb->signal_id + 1) & 0xFF;
            if (chcb->signal_id == 0)
                chcb->signal_id = 1;
        }
    }

    /* Return unused signal ID */
    return chcb->signal_id;
}

/*! \brief Destroy signal

    This function frees the memory for the specified signal, the
    signal must not be on any queue when it is freed.  Any pending
    timer will be cancelled.

    \param sig_ptr Pointer to signal to destroy.
*/
static void SIGH_SignalDestroy(SIG_SIGNAL_T *sig_ptr)
{
    timer_cancel(&sig_ptr->timer_id);
    mblk_destroy(sig_ptr->signal);
    pfree(sig_ptr);
}


/*! \brief Empty specified signal queue.

    This function will empty the specified signal queue of all signals, the
    signals will be destroyed.

    \param sig_list Pointer to signal list to empty, the list pointers will be
           updated automatically.
    \param p_bd_addr BD_ADDR for the CHCB to which these queues exist. If this
           is NULL then we are not allowed to send messages to the upper layers
*/
void SIGH_SignalQueueEmpty(SIG_SIGNAL_T **sig_list,
                           const BD_ADDR_T *const p_bd_addr)
{
    SIG_SIGNAL_T *sig_ptr;
    SIG_SIGNAL_T *sig_free_ptr;

    CSR_UNUSED(p_bd_addr);

    /* Flush the pending queue and stop any timers */
    for (sig_ptr = *sig_list; sig_ptr != NULL;)
    {
        sig_free_ptr = sig_ptr;        

        /* Move to next signal in list */
        sig_ptr = sig_ptr->next_ptr;

        /* Free this signal */
        SIGH_SignalDestroy(sig_free_ptr);
    }

    /* Reset list */
    *sig_list = NULL;
}


#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Empty specified signal queue of signals for specified channel.

    This function will empty the specified signal queue of signals
    from the specified channel, the signals will be destroyed. Note
    that we don't need to check for ECHO/GETINFO as they are never
    tied to CIDCBs.

    \param sig_list Pointer to signal list to empty, the list pointers will be updated
                    automatically.
    \param cid The channel ID for the signals to destroy.
*/
void SIGH_SignalQueueEmptyWithCid(SIG_SIGNAL_T **sig_list,
                                  l2ca_cid_t cid)
{
    SIG_SIGNAL_T *sig_ptr;
    SIG_SIGNAL_T **sig_ptr_ptr;

    /* Flush the queue and stop any timers */
    for (sig_ptr_ptr = sig_list; (sig_ptr = *sig_ptr_ptr) != NULL;)
    {
        if(sig_ptr->local_cid == cid)
        {
            /* Update pointer to next pointer */
            *sig_ptr_ptr = sig_ptr->next_ptr;

            /* Free this signal */
            SIGH_SignalDestroy(sig_ptr);
            sig_ptr = NULL;
        }
        else
        {
            sig_ptr_ptr = &sig_ptr->next_ptr;
        }
    }
}

#endif 

/*! \brief Add signal to queue.

    This function will add a signal to the specified queue, the signal will be
    added at the end of the queue.

    \param sig_ptr_ptr Pointer to signal queue to add signal to.
    \param sig_add_ptr Pointer to signal to add to queue.
*/
static void SIGH_SignalQueueAdd(SIG_SIGNAL_T **sig_ptr_ptr, SIG_SIGNAL_T *sig_add_ptr)
{
    SIG_SIGNAL_T *sig_ptr;

    /* Find last signal in pending list */
    for(;
        (sig_ptr = *sig_ptr_ptr) != NULL;
        sig_ptr_ptr = &sig_ptr->next_ptr)
    {
        /* NOP */
    }

    /* Set next pointer in last entry to new signal */
    *sig_ptr_ptr = sig_add_ptr;

    /* Clear next pointer in next signal */
    sig_add_ptr->next_ptr = NULL;
}



#ifdef INSTALL_ULP
#ifdef INSTALL_L2CAP_LECOC_CB

/*! \brief LE credit based flow control credit signal.

    This function handle LE credit based flow control credit signal. It extracts
    Tx credits provided to local L2CAP device which is equivalent to the Rx
    credits relinquished by peer L2CAP device. Based on these Tx credits
    availability, the local L2CAP device resumes the transmission of L2CAP data
    to the peer and also unblocks the upper layers to send more L2CAP data.

    \param chcb Pointer to CHCB instance.
    \param sig Pointer to signal payload.
    \return The size of the signal or 0 if the signal is malformed.
*/
static uint16_t SIGH_Handle_SIGNAL_LE_FLOW_CONTROL_CREDIT(L2CAP_CHCB_T *chcb, 
                                                       L2CAP_SIGNAL_INFO_T *sig)
{
    CID_LE_CREDIT_BASED_FC_CREDIT_T sig_args;
    CIDCB_T *cidcb;

    /* Check if this is a duplicate */
    if(SIGH_DuplicateDetect(chcb, sig))
    {
        return sig->length_with_header;
    }

    sig_args.signal_id  = sig->id;
    read_uints(&sig->data,
               URW_FORMAT(uint16_t, 2, UNDEFINED, 0, UNDEFINED, 0),
               &sig_args.dest_cid,
               &sig_args.credits);

    /* Attempt to get CIDCB instance based on remote CID in signal block, and 
     * also check for validity of cb_flow as there could be a race between the 
     * disconnect request and this event being recieved.
     */
    if(((cidcb = CIDME_GetCidcbRemoteWithHandle(chcb, sig_args.dest_cid)) != NULL) &&
        (CID_IsCBFlowControl(cidcb)))
    {
        /*
         * We can now:
         * 1) Update credits information based on peer having relinquished
         *    some additional Rx credits to us.
         * 2) Attempt to transmit all the queued unsegmented LE L2CAP frames
         *    (if any) over the dynamic channel to the peer.
         * 3) Flow-on to upper layers unblocking it from sending more
         *    unsegmented LE L2CAP frames to us, provided upper layers was
         *    previously flowed-off by us and now we have non zero credits so
         *    we allow upper layers to resume sending more LE L2CAP frames.
         */

        if(CID_CheckCreditOverflow(cidcb->cb_flow->tx_credits,sig_args.credits))
        {
            /*
             * As per spec
             * (i.e. Vol-3/Part A/Section-10.1 LE CREDIT BASED FLOW CONTROL
             *       MODE),
             * The device receiving the credit packet shall disconnect the L2CAP
             * channel if the credit count exceeds 65535.
             *
             * The payload part of the signal which is contained in an MBLK
             * is generically cleaned up by the LE common signal handler & hence
             * not cleaned here.
             *
             */
            L2CA_DisconnectInd(cidcb, 0, L2CA_DISCONNECT_FLOW_VIOLATION);
            /* Return indicating signal handled */
            return sig->length_with_header;
        }
        /* Step - 1) */
        cidcb->cb_flow->tx_credits += sig_args.credits;

        /* Step - 2) */
        CB_FLOW_Reschedule(cidcb);

        /* Step - 3) */
        if(cidcb->cb_flow->flow_off && cidcb->cb_flow->tx_credits)
        {
            cidcb->cb_flow->flow_off = FALSE;
        }
    }
    else
    {
        /* Received LE FLOW CONTROL CREDIT Signal for invalid CID */
       CSR_LOG_TEXT_INFO((L2CAP, 0, "Invalid CID"));  
    }

    /* Return indicating signal handled */
    return sig->length_with_header;
}
#endif /* INSTALL_L2CAP_LECOC_CB */
#endif



/*! \brief Setup a txqe for signal transmission

    This function is a pre-send callback. It's called from channel
    transmit loop when a "signal callback frametype" is found. This
    function prepares the txqe pointer for transmission by setting up
    the header, mblk and all the other stuff that goes into the txqe.

    This function will attempt to also send/prepare any other pending
    signals upto the maximum signal MTU.

    If signal queue is not empty when we are about to return from this
    function, we re-schedule a new callback in the transmitter.

    The txqe must have enough header-space to contain a standard CID
    header.

    \param chcb Pointer to CHCB instance.
    \param txqe Pointer to the tx element to fill out
*/
void SIGH_SignalPresend(L2CAP_CHCB_T *chcb,
                        TXQE_T *txqe)
{
    SIG_SIGNAL_T *signal_queue = chcb->signal_queue;
    mblk_size_t mblk_chain_length = 0;
    MBLK_T *mblk_chain_ptr = NULL;

    /* This has effectively removed the scheduled callback */
    chcb->signal_scheduled = FALSE;

    /* Handle sending multiple signals in one L2CAP packet */
    while(signal_queue)
    {
        SIG_SIGNAL_T *sig_ptr;
        uint8_t *signal_data;
        MBLK_T *mblk_ptr;

        /* Remove signal from queue and prepare for next */
        sig_ptr = signal_queue;
        signal_queue = sig_ptr->next_ptr;
        mblk_ptr = sig_ptr->signal;
        signal_data = mblk_map(mblk_ptr, 0, mblk_get_length(mblk_ptr));

        txqe->cid = sig_ptr->local_cid;

        switch(sig_ptr->signal_type)
        {

#ifdef INSTALL_L2CAP_LECOC_CB
            case SIGNAL_LE_FLOW_CONTROL_CREDIT:
            {
                /* As there is no response for LE flow credit signal, we don't 
                 * need to re-transmit the signal, we should make sure that 
                 * l2cap identifier is unique. sig_ptr will get freed after 
                 * the signal is scheduled to be sent, just like other response
                 * signals, don't break from here */
                signal_data[1] = SIGH_SignalIdObtain(chcb);
            }
            pfree(sig_ptr);
            break;
#endif
            default:
            {
                /* Free the signal block, data block now owned by MBLK */
                pfree(sig_ptr);
            }
            break;
        }

        /* Add MBLK to end of list */
        mblk_chain_ptr = mblk_add_tail(mblk_ptr, mblk_chain_ptr);
        mblk_chain_length = mblk_get_length(mblk_chain_ptr);

        mblk_unmap(mblk_ptr, signal_data);
        break;
    }

    chcb->signal_queue = signal_queue;

    /* Augment the txqe element. Transmitter (who called us) will the
     * continue to send it */
    if(mblk_chain_ptr != NULL)
    {
        txqe->mblk = mblk_chain_ptr;
        txqe->flush = FLUSH_INFINITE_TIMEOUT;
        CH_DataAddHeader(txqe,
                         (uint16_t)CH_GET_SIGNAL_CID(chcb),
                         mblk_chain_length, NULL, 0);
    }

    /* Poke the transmitter loop to shovel data through if possible */
    if(signal_queue != NULL)
    {
        CH_DataTxSignalCallback(chcb);
    }
}

/*! \brief Send L2CAP signal

    Request from a CID to send signal. Stuff it onto the queue and
    send it if possible.

    \param chcb Pointer to the CHCB instance
    \param sig_ptr Pointer to the signal to send
*/
void SIGH_SignalSend(L2CAP_CHCB_T *chcb, SIG_SIGNAL_T *sig_ptr)
{
    /* Add signal to queue, we'll send it later */
    SIGH_SignalQueueAdd(&chcb->signal_queue, sig_ptr);

    /* Poke the transmitter loop to shovel data through if possible */
    CH_DataTxSignalCallback(chcb);
}


/*! \brief Bluetooth Low Energy signal reception

    The BLE only uses a subset of the signals and use CID 5 instead of
    the normal L2CAP signalling channel
*/
#ifdef INSTALL_ULP
void SIGH_LeSignalReceive(L2CAP_CHCB_T *chcb, MBLK_T **mblk, uint16_t signal_size)
{
    uint16_t consumed;
    uint8_t *map;
    L2CAP_SIGNAL_INFO_T sig;

    map = mblk_map(*mblk, 0, signal_size);
    if(map == NULL)
    {
        return;
    }

    /* Decode signal - we know it has enough space for the header */
    sig.data = map;
    read_uints(&sig.data,
               URW_FORMAT(uint8_t, 2, uint16_t, 1, UNDEFINED, 0),
               &sig.type,
               &sig.id,
               &sig.length);
    sig.length_with_header = sig.length + L2CAP_SIZEOF_SIGNAL_HEADER;

    /* Make sure signal length matches value in header */
    if(sig.length_with_header != signal_size)
    {
        mblk_unmap(*mblk, map);
        return;
    }

    consumed = 0;

#ifdef INSTALL_L2CAP_LECOC_CB
    if(sig.type == SIGNAL_LE_FLOW_CONTROL_CREDIT &&
            sig.length == SIGNAL_LE_FLOW_CONTROL_CREDIT_MIN_LENGTH)
    {
        /* BLE specific signals */
        consumed = SIGH_Handle_SIGNAL_LE_FLOW_CONTROL_CREDIT(chcb, &sig);
    }
#endif /* INSTALL_L2CAP_LECOC_CB */

    QBL_UNUSED(consumed);
    mblk_unmap(*mblk, map);
}
#endif






