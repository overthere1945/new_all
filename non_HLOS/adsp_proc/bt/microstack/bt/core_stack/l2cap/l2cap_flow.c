/*******************************************************************************

Copyright (C) 2008 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\brief  This file implements the L2CAP flow & error control extensions.
        It also implements LE L2CAP flow control mode i.e. Credit-Based.
*******************************************************************************/

#include "l2cap_private.h"
#include INC_DIR(common,external_logging.h)


#ifdef INSTALL_L2CAP_LECOC_CB
/*! \brief Reassemble segmented PDUs (i.e. LE-frames) into a single SDU.

    The strategy is first to handle the 1st PDU, as this'll tell us to prepare
    for the soon-to-follow PDUs corresponding to same SDU. For the PDUs
    succeeding the 1st PDU, just add the MBLK into the assembly chain. For each
    PDU received, decrement the pending SDU length by the mblk length (except
    for the 1st PDU where the SDU length field (i.e. 2 octets) needs to be added
    pending SDU length before decrementing by the mblk length.

    Second step is to figure out the last PDU corresponding to a specific SDU.
    Pending SDU length of zero indicates the last PDU. At this instance, pass
    the MBLK chain to the upper layers.

    As upper layers are not known to start/continuaton/end PDUs of a segmented
    SDU, L2CAP internally reassembles these PDUs into their equivalent SDU
    before sending it across to the upper layers.

    PRECONDITIONS FOR THIS FUNCTION: PDUs (i.e. LE-frames) arrive in CORRECT
    transmitted order and the MBLK must contain only the actual payload (no
    headers), except for the SDU length field in case of the 1st LE-frame.

    \param cidcb Pointer to the CIDCB_T structure.
    \param flow Pointer to the CB_FLOWINFO_T structure.
    \param mblk The MBLK data - also contains and/or SDU length.
    \returns TRUE if function was successful, FALSE if a fatal error
             was discovered and raised.
*/
static bool_t CB_FLOW_SarAssemble(CIDCB_T *cidcb,
                               CB_FLOWINFO_T *flow,
                               MBLK_T *mblk)
{
    L2CA_DATAREAD_IND_T *ind;
    bool_t ok  = FALSE;
    mblk_size_t mblk_length = mblk_get_length(mblk);

    ind = flow->l2ca_dataread_ind;

    /* Start packets */
    if (ind == NULL) /* Similar to L2CAP_SAR_START */
    {
        /* Check if LE frame/packet of segmented SDU is malformed */
        if(flow->sdu_len_remaining)
        {
            L2CA_DataReadInd(cidcb, mblk, L2CA_DATA_PARTIAL, 1, &ind);
            flow->sdu_len_remaining -= mblk_length;

            /* Data has been consumed */
            ok = TRUE;
        }
    }
    /* Continuations and ends */
    else
    {
        if(flow->sdu_len_remaining >= mblk_length)
        {
            flow->sdu_len_remaining -= mblk_length;
            /* We always append data for both continue and end */
            ind->data   = (ind->data == NULL ?
                            mblk : mblk_add_tail(mblk, ind->data));
            mblk = NULL;
            ind->packets++;

            if(flow->sdu_len_remaining != 0) /* Similar to L2CAP_SAR_CONTINUE */
            {
                /* Data has been consumed */
                ok = TRUE;
            }
            else if(flow->sdu_len_remaining == 0) /* Similar to L2CAP_SAR_END */
            {
                ind->length = 0; /* To indicate data is contained within MBLK */
                /* Handover MBLK to upper layers */
                ind->result = L2CA_DATA_PARTIAL_END;
                L2CA_PrimSend(ind);
                cidcb->cb_flow->app_rx_credits =
                            (ind->packets > cidcb->cb_flow->app_rx_credits) ?
                            0:
                            (cidcb->cb_flow->app_rx_credits - ind->packets);
                /*
                 * Remove local reference to data as upper layer must consume it
                 * and acknowledge it.
                 */
                ind  = NULL;
                ok = TRUE;
            }
        }
    }

    if(!ok)
    {
        /* Error in reassembly.*/
        /*
         * As per spec
         * (i.e. Vol-3/Part A/Section-3.4.3 Information Payload Field),
         * if the sum of the payload lengths for the LE-frames exceeds the
         * specified SDU length, the receiver shall disconnect the channel.
         *
         * Note:
         * This also caters the progressive problem of reassembling bad SDUs.
         * i.e. If there are octets lost in any of the received LE frames of the
         * current SDU, by disconnection we eventually avoid a progressive
         * problem of reassembling the current SDU with octets from succeeding
         * SDU. This is an acceptable behavior.
         */
        DEBDRP(0x7004);
        CB_FLOW_FatalError(cidcb, mblk, L2CA_DISCONNECT_SAR_VIOLATION);
        return FALSE;
    }

    /* Prevent dangling reference */
    flow->l2ca_dataread_ind = ind;

    /* Success */
    return TRUE;
}

/*! \brief Receive a LE L2CAP frame.

    This is where new LE packets are passed into the credit based flow control
    module. We perform the required analysis of the packet like
    - Check whether its segemented/un-segmented. If segmented trigger the
      reassembly.
    - Sanitize the LE frames length against MTU, MPS & SDU.

    \param cidcb Pointer to the CIDCB_T structure.
    \param mblk The MBLK data - also contains and/or SDU length.
    \param payload_size Length of the MBLK.
                        - i.e. sdu_length_size (only present in 1st LE frame of
                               an SDU) + information_payload_size.
    \return TRUE  : LE L2CAP frame successfully decoded/re-assembled.
            FALSE : Otherwise.
*/
bool_t CB_FLOW_DataRead (CIDCB_T *cidcb, MBLK_T *mblk, uint16_t payload_size)
{
    if(cidcb != NULL && cidcb->cb_flow != NULL)
    {
        CB_FLOWINFO_T *flow;
        uint16_t mps;

        flow = cidcb->cb_flow;
        mps = flow->local_mps;

        if(/* Validate MPS for 1st LE frame, irrespective of segementation */
           (flow->l2ca_dataread_ind == NULL &&
            ((payload_size < L2CAP_SIZEOF_SDU_LEN_FIELD) ||
             ((payload_size - L2CAP_SIZEOF_SDU_LEN_FIELD) > mps))) ||
           /* Validate MPS for succeeding LE frames, in case of segmentation */
           (flow->l2ca_dataread_ind != NULL && payload_size > mps))
        {
            /*
             * As per spec
             * (i.e. Vol-3/Part A/Section-3.4.3 Information Payload Field),
             * if the payload length of any LE-frame exceeds the receiver's MPS,
             * the receiver shall disconnect the channel.
             */
            DEBDRP(0x7000);
            CB_FLOW_FatalError(cidcb, mblk, L2CA_DISCONNECT_MPS_VIOLATION);
            return FALSE;
        }

        if(cidcb->cb_flow->rx_credits == 0)
        {
            /*
             * As per spec
             * (i.e. Vol-3/Part A/Section-10.1 LE CREDIT BASED FLOW CONTROL
             *       MODE),
             * The device shall also disconnect the L2CAP channel if it receives
             * an LE-frame on an L2CAP channel from the peer device that has a
             * credit count of zero.
             *
             */
            DEBDRP(0x7001);
            CB_FLOW_FatalError(cidcb, mblk, L2CA_DISCONNECT_FLOW_VIOLATION);
            return FALSE;
        }

        do
        {
            uint16_t info_payload_size;

            info_payload_size = flow->l2ca_dataread_ind == NULL ?
                                (payload_size - L2CAP_SIZEOF_SDU_LEN_FIELD) :
                                payload_size;

            /* Prepare unsegmented/segmented SDUs to be sent to application */
            if(flow->l2ca_dataread_ind == NULL)
            {
                /* SDU length read out instead of peeking */
                if(mblk_read_head16(&mblk, &flow->sdu_len_remaining) &&
                    /* Is Unsegmented/Segmented LE frame properly framed? */
                    flow->sdu_len_remaining >= info_payload_size)
                {
                    /* Validate SDU length against agreed MTU */
                    if(flow->sdu_len_remaining > cidcb->local_mtu)
                    {
                        /*
                         * As per spec
                         * (i.e. Vol-3/Part A/Section-3.4.3 Information Payload
                         *       Field), if the SDU length field value exceeds
                         * the receiver's MTU, the receiver shall disconnect the
                         * channel.
                         */
                        DEBDRP(0x7002);
                        CB_FLOW_FatalError(cidcb, mblk,
                                            L2CA_DISCONNECT_MTU_VIOLATION);
                        return FALSE;
                    }

                    if(flow->sdu_len_remaining == info_payload_size)
                    {
                        /* Unsegmented */
                        if(info_payload_size)
                        {
                            L2CA_DataReadInd(cidcb, mblk, L2CA_DATA_SUCCESS, 1,
                                                NULL);
                            if (cidcb->cb_flow->app_rx_credits > 0)
                            {
                                cidcb->cb_flow->app_rx_credits--;
                            }
                        }
                        else
                        {
                            /*
                             * Zero sized LE frame, ignore w/o credit
                             * consumption.
                             */
                            return FALSE;
                        }
                        /* Break to attempt auto-replenishing credits to peer */
                        break;
                    }
                    /* Segmented and is handled below */
                }
                else
                {
                    /*
                     * Unsegmented/Segmented LE frame is malformed.
                     * Case-1: SDU length < info_payload_size, Segmented LE
                     *         frame malformed.
                     * Case-2: Failed to read SDU length, Unsegmented/Segmented
                     *         LE frame malformed.
                     * ToDo: Define informative error code if required.
                     */
                    DEBDRP(0x7003);
                    CB_FLOW_FatalError(cidcb, mblk,
                                        L2CA_DISCONNECT_SAR_VIOLATION);
                    return FALSE;
                }
            }

            /* We reach here if the SDU is Segmented, so we re-assemble */
            if(info_payload_size == 0 ||
                !CB_FLOW_SarAssemble(cidcb, flow, mblk))
            {
                /*
                 * Two cases we arrive here:
                 * 1) Zero sized LE frame, ignore w/o credit consumption.
                 * 2) Fatal error in re-assembly occurred, bail out now.
                 */
                return FALSE;
            }
        }while(0);

        /* Consume/Exhaust a Rx credit for the LE frame received */
        cidcb->cb_flow->rx_credits--;

        /*
         * By now we have successfully received an unsegmented SDU or partial
         * segmented SDU so kickstart auto replenishing of credits to peer, as
         * applicable.
         */
        CB_Flow_AttemptToSendCredits(cidcb);
        return TRUE;
    }

    return FALSE;
}

/*! \brief Free the flow control structure for a CIDCB

    Function must be called to free the resources allocated to Credit Based
    Flow Control mode.

    \param cidcb Pointer to the CIDCB_T structure.
    \param result L2CAP error/reason code.
*/
void CB_FLOW_Free(CIDCB_T *cidcb, l2ca_data_result_t result)
{
    if(cidcb != NULL && cidcb->cb_flow != NULL)
    {
        CB_FLOWINFO_T *flow;
        flow = cidcb->cb_flow;

        /* Clear the frames pending transmission */
        CB_FLOW_FlushQueue(cidcb, result);

        /* Free instance */
        L2CA_FreePrimitive((L2CA_UPRIM_T*)flow->l2ca_dataread_ind);
        pfree(flow);
        cidcb->cb_flow = NULL;
    }
}

/*! \brief Fatal error handling

    In CB (for LE) mode, fatal errors may happen, and
    we may need to kill the link. This function will help cleanup the mess
    and then inject the INTERNAL_DISCONNECT into the CID state
    machine, which will then trigger a disconnect or ACL drop incase
    of fixed channels.

    \param cidcb Pointer to the CIDCB_T structure.
    \param mblk The MBLK data - also contains and/or SDU length.
    \param error_code L2CAP error/reason code.
*/
void CB_FLOW_FatalError(CIDCB_T *cidcb,
                            MBLK_T *mblk,
                            l2ca_disc_result_t error_code)
{
    if(mblk != NULL)
    {
        mblk_destroy(mblk);
    }
    L2CA_DisconnectInd(cidcb, 0, error_code);
}

/*! \brief Check if a LE FLOW CONTROL CREDIT L2CAP PDU being sent.

    Send LE FLOW CONTROL CREDIT L2CAP PDU if need be based on the current
    credits status meeting the threshold conditions.

    \param cidcb Pointer to the CIDCB_T structure.
*/
void CB_Flow_AttemptToSendCredits(CIDCB_T *cidcb)
{
    CB_FLOWINFO_T *flow = cidcb->cb_flow;

    /*
     * As per spec, credits sent can't be zero as allowable range is 1-65535.
     *
     * Credit replishment is differently handled for UNSEGMENTED & SEGMENTED
     * SDUs.
     *
     * When UNSEGMENTED SDU is received, credits maintained at application level
     * and internal Rx credits follow each other. So unless the application has
     * acknowledged reception the credit replenishment doesn't kick in.
     *
     * When SEGMENTED SDU is received, credits maintained at application level
     * is updated when the full SDU has arrived but the internal Rx credits
     * is updated/exhausted on reception of each data PDU. Till the time
     * credits at application level is greater than internal Rx credits,
     * we continue to extend these delta credits to prevent false stalls. As
     * credits at application level is indicative of the current application
     * buffer capacity, it is ensured that we never overflow the buffer.
     * NOTE: When credits at application level reaches '0' since application
     * hasn't acknowledged these SDUs, we await the application acknowledgement
     * for the credit replishment to kick in.
     */
    if(flow->app_rx_credits && flow->rx_credits < flow->app_rx_credits &&
        flow->rx_credits <=
            L2CAP_LE_COC_CURR_RX_CREDITS_THRESHOLD(flow->tot_rx_credits))
    {
        uint16_t credits = flow->app_rx_credits - flow->rx_credits;
        SIG_SIGNAL_T * sig_ptr;
        sig_ptr = SIG_SignalLEFlowControlCredit(cidcb->offload_cid, credits);
        /* Send the signal */
        SIGH_SignalSend(cidcb->chcb, sig_ptr);

        flow->rx_credits += credits;
    }
}

/*! \brief Upper layer acknowledge of data read.

    This function is called when we receive a L2CA_DATAREAD_RSP signal
    from the application in response to previously received start and
    continuation SAR frames.

    We update the accumulated Rx credits based on the number of LE-frames
    acknowledged and then make an attempt to send LE FLOW CONTROL CREDIT L2CAP
    PDU if need be based on the current credits status.

    Note: The LE SDUs to which application has responded can cause Bluestack to
          auto-replenish credits.
          - When our current Rx credits have hit the threshold limit, we extend
            current Rx credits (capped to total Rx credits) by borrowing the
            difference credits from the current application provided Rx credits.
          - So when the application has responded to these LE SDUs (even if
            such responses are consolidated for several LE SDUs), we further
            ensure to cap the accumulated Rx credits to total Rx credits.

    \param cidcb Pointer to the CIDCB_T structure.
    \param pkt_ack_cnt Number of LE data frames/PDUs acknowledged.
*/
void CB_FLOW_DataReadAck(CIDCB_T *cidcb, uint16_t pkt_ack_cnt)
{
    cidcb->cb_flow->app_rx_credits += pkt_ack_cnt;
    if(cidcb->cb_flow->app_rx_credits > cidcb->cb_flow->tot_rx_credits)
    {
        cidcb->cb_flow->app_rx_credits = cidcb->cb_flow->tot_rx_credits;
    }
    CB_Flow_AttemptToSendCredits(cidcb);
}

/*! \brief Upper layer wants to send data.

    Always construct the Credit Based flow control queue element.
    Fill in the details and append it to the tail of the queue. If sufficient
    L2CAP level credits are available then proceed sending L2CAP frame
    as UNSEGMENTED.

    \param cidcb Pointer to the CIDCB_T structure.
    \param mblk The MBLK data - excludes SDU length field.
    \param context Opaque context from L2CA_DATAWRITE_REQ.
*/
void CB_FLOW_DataWrite(CIDCB_T *cidcb, MBLK_T *mblk, context_t context)
{
    FLOWPKT_T *pkt;
    CB_FLOWINFO_T *flow = cidcb->cb_flow;

    if(!flow->flow_off &&
      (flow->tx_credits < CH_getQueuedCnt(cidcb)))
    {
        /* Flow-off to upper layers until peer relinquishes more credits */
        flow->flow_off = TRUE;
        /*
         * Since we have flowed off, queue the data for transmission.
         *
         * Attempt to send the queue data. If enough credits aren't
         * available then CB_FLOW_Reschedule() would anyhow avoid moving
         * the queued data to ACL transmission queue.
         */
    }

    /* Construct Credit based flow control queue element */
    pkt = zpnew(FLOWPKT_T);

    /* Fill in (relevant) data */
    pkt->mblk = mblk;
    pkt->context = context;
    pkt->sdu_length = mblk_get_length(mblk);
    pkt->sdu_mode = L2CAP_SAR_UNSEGMENTED;
    pkt->next = NULL;

#ifdef INSTALL_L2CAP_DATA_ABORT_SUPPORT
    pkt->aborted = NULL; /* Not supported yet */
#endif

    /* Insert Data PDU into Credit based flow control queue */
    if(flow->tx_tail == NULL)
    {
        flow->tx_tail = pkt;
        flow->tx_head = pkt;
    }
    else
    {
        flow->tx_tail->next = pkt;
        flow->tx_tail = pkt;
    }
    /* Request ourselves to be scheduled for transmission */
    CB_FLOW_Reschedule(cidcb);
}

#if defined(INSTALL_L2CAP_ECBFC) || defined(INSTALL_L2CAP_LECOC_TX_SEG)
static bool_t CB_FLOW_SarSegment(CIDCB_T *cidcb, 
                              CB_FLOWINFO_T *flow,
                              TXQE_T **p_txqe,
                              FLOWPKT_T *pkt,
                              uint16_t *offset);
#endif /* INSTALL_L2CAP_ECBFC || INSTALL_L2CAP_LECOC_TX_SEG */

/*! \brief Reschedule Tx transmission.

    Tx transmission is scheduled when application has sent data with sufficient
    L2CAP level credits are currently available or when the peer has replenished
    L2CAP level credits.

    \param cidcb Pointer to the CIDCB_T structure.
*/
void CB_FLOW_Reschedule(CIDCB_T *cidcb)
{
    CB_FLOWINFO_T *flow = cidcb->cb_flow;
    TXQUEUE_T *queue = CID_GetTxQueue(cidcb);

    if(flow != NULL)
    {
        FLOWPKT_T *pkt;

        while(flow->tx_credits > 0 && (pkt = flow->tx_head) != NULL)
        {
            TXQE_T *txqe = NULL;
#if !defined(INSTALL_L2CAP_ECBFC) && !defined(INSTALL_L2CAP_LECOC_TX_SEG)
            uint8_t sdu_len[L2CAP_SIZEOF_SDU_LEN_FIELD];
            uint8_t *sdu_len_copy = sdu_len;

            /* Move the queued frame to ACL transmission queue */
            txqe = (TXQE_T*)zpmalloc(sizeof(TXQE_T) + L2CAP_SIZEOF_CID_HEADER +
                                        L2CAP_SIZEOF_SDU_LEN_FIELD);

            txqe->type = L2CAP_FRAMETYPE_NORMAL;
            txqe->cid = cidcb->local_cid;
            txqe->context = pkt->context;
            txqe->mblk = pkt->mblk;
            txqe->flush = cidcb->local_flush_to;

            /* Construct the header */
            write_uint16(&sdu_len_copy, pkt->sdu_length);
            CH_DataAddHeader(txqe, cidcb->remote_cid, pkt->sdu_length, sdu_len,
                                L2CAP_SIZEOF_SDU_LEN_FIELD);
#else
            if(CB_FLOW_SarSegment(cidcb, flow, &txqe, pkt, &flow->sar_offset))
            {
                /* De-chain & destroy element appended to ACL transmission queue */
                flow->tx_head = flow->tx_head->next;
                pfree(pkt);

                /* Reset SAR offset when progressing the tx queue */
                flow->sar_offset = 0;
            }
#endif /* !INSTALL_L2CAP_ECBFC && !INSTALL_L2CAP_LECOC_TX_SEG */

            /*lint -esym(429, txqe) Freed later on handling of NCP event.*/
            CH_CBDataTxAppend(queue, txqe, cidcb->priority);

            /*
             * Since the unsegmented LE L2CAP frame is now queued to ACL data
             * queue,its ok to update credits.
             * Though it would have been better to update it when the frame is
             * actually sent over the air, we choose to update here since we
             * commonly converge here in either case when data is sent by
             * application or resume data transmission on peer replishing
             * credits.
             */
            flow->tx_credits--;

#if !defined(INSTALL_L2CAP_ECBFC) && !defined(INSTALL_L2CAP_LECOC_TX_SEG)
            /* De-chain & destroy element appended to ACL transmission queue */
            flow->tx_head = flow->tx_head->next;
            pfree(pkt);
#endif /* !INSTALL_L2CAP_ECBFC && !INSTALL_L2CAP_LECOC_TX_SEG */
        }

        /* Reset the queue iterators to indicate the queue is empty now */
        if(flow->tx_head == NULL)
        {
            flow->tx_tail = flow->tx_head;
        }
    }

    /* Kick the ACL transmission */
    CH_DataSendQueued(cidcb->chcb, queue, cidcb->priority, FALSE);

}

#if defined(INSTALL_L2CAP_ECBFC) || defined(INSTALL_L2CAP_LECOC_TX_SEG)
static bool_t CB_FLOW_SarSegment(CIDCB_T *cidcb, 
                              CB_FLOWINFO_T *flow,
                              TXQE_T **p_txqe,
                              FLOWPKT_T *pkt,
                              uint16_t *offset)
{
    TXQE_T *txqe = NULL;
    uint8_t sdu_len[L2CAP_SIZEOF_SDU_LEN_FIELD];
    uint8_t *sdu_len_copy = sdu_len;

    if(pkt->sdu_length > (flow->remote_mps - L2CAP_SIZEOF_SDU_LEN_FIELD))
    {
        uint16_t   this_length;
        MBLK_T    *this_mblk;
        uint8_t    this_sar;

        /* Determine continuation bits */
        if(*offset == 0)
        {
            txqe = (TXQE_T*)zpmalloc(sizeof(TXQE_T) + L2CAP_SIZEOF_CID_HEADER +
                                        L2CAP_SIZEOF_SDU_LEN_FIELD);
            /* Calculate length of this fragment */
            this_length = MIN(pkt->sdu_length - *offset,
                              (flow->remote_mps - L2CAP_SIZEOF_SDU_LEN_FIELD));
            /* Construct the header */
            write_uint16(&sdu_len_copy, pkt->sdu_length);
            CH_DataAddHeader(txqe, cidcb->remote_cid, this_length, sdu_len,
                                L2CAP_SIZEOF_SDU_LEN_FIELD);
            /* This is the first one */
            this_sar = L2CAP_SAR_START;
        }
        else
        {
            txqe = (TXQE_T*)zpmalloc(sizeof(TXQE_T) + L2CAP_SIZEOF_CID_HEADER);
            /* Calculate length of this fragment */
            this_length = MIN(pkt->sdu_length - *offset,
                              flow->remote_mps);
            CH_DataAddHeader(txqe, cidcb->remote_cid, this_length, NULL, 0);
            if(this_length == (pkt->sdu_length - *offset))
            {
                /* This will be the last fragment */
                this_sar = L2CAP_SAR_END;
            }
            else
            {
                /* This is a continuation */
                this_sar = L2CAP_SAR_CONTINUE;
            }
        }
        /* Create the fragment */
        this_mblk = mblk_duplicate_region(pkt->mblk, *offset, this_length);
        if(this_mblk == NULL)
        {
            /* Out of memory when creating the duplicate MBLK. We
             * should be able to recover if we just leave the queue
             * alone */
#ifdef INSTALL_L2CAP_DEBUG
            BLUESTACK_PANIC(PANIC_MBLK_CREATE_FAILURE);
#endif
            pfree(txqe);
            return FALSE;
        }

        /* Re-use the old queue element if this is the last
         * fragment */
        if(this_sar == L2CAP_SAR_END)
        {
            /* This is the end fragment so resuse the packet
             * structure. Also destroy original mblk as it has now
             * been completely replaced by duplicates */
            mblk_destroy(pkt->mblk);
        }

        txqe->type = L2CAP_FRAMETYPE_NORMAL;
        txqe->cid = cidcb->local_cid;
        txqe->context = pkt->context;
        txqe->mblk = this_mblk;
        txqe->flush = cidcb->local_flush_to;

        /* Update offset counter and "replace" queue element */
        *offset += this_length;
        *p_txqe = txqe;

        /* If this was the last fragment, caller can destroy original
         * queue element */
        return this_sar == L2CAP_SAR_END;
    }
    else
    {
        txqe = (TXQE_T*)zpmalloc(sizeof(TXQE_T) + L2CAP_SIZEOF_CID_HEADER +
                                    L2CAP_SIZEOF_SDU_LEN_FIELD);
        
        txqe->type = L2CAP_FRAMETYPE_NORMAL;
        txqe->cid = cidcb->local_cid;
        txqe->context = pkt->context;
        txqe->mblk = pkt->mblk;
        txqe->flush = cidcb->local_flush_to;
        
        /* Construct the header */
        write_uint16(&sdu_len_copy, pkt->sdu_length);
        CH_DataAddHeader(txqe, cidcb->remote_cid, pkt->sdu_length, sdu_len,
                            L2CAP_SIZEOF_SDU_LEN_FIELD);
        *p_txqe = txqe;
        return TRUE;
    }
}
#endif /* INSTALL_L2CAP_ECBFC || INSTALL_L2CAP_LECOC_TX_SEG */

/*! \brief Flush the Credit Based flow control queue for frames that are
           pending transmission.

    This is a generic flush queue function. It also sends the corresponding
    L2CA_DATAWRITE_CFMs for frames pending transmission.

    \param cidcb Pointer to the CIDCB_T structure.
    \param result L2CAP error/reason code.
*/
void CB_FLOW_FlushQueue(CIDCB_T *cidcb, l2ca_data_result_t result)
{
    CB_FLOWINFO_T *flow = cidcb->cb_flow;
 
    if(flow != NULL)
    {
        FLOWPKT_T *pkt;
        while((pkt = flow->tx_head) != NULL)
        {
            /* Inform upper layer if allowed to do so */
            if(result != L2CA_DATA_SILENT)
            {
                L2CA_DataWriteCfm(cidcb,
                                  pkt->context,
                                  pkt->sdu_length,
                                  result);
            }

            /* De-chain & destroy the element flushed */
            flow->tx_head = flow->tx_head->next;
            mblk_destroy(pkt->mblk);
            pfree(pkt);
        }

        /* Reset the queue iterators to indicate the queue is emptied/flushed */
        flow->tx_tail = flow->tx_head;
    }
}


#endif /* #ifdef INSTALL_L2CAP_LECOC_CB */
