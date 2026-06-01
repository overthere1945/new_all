/*******************************************************************************

Copyright (C) 2009 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "rfcomm_private.h" 

#ifdef INSTALL_RFCOMM_MODULE

RFC_CTRL_T  rfc_ctrl_allocation;

/* Local function prototypes.
*/ 
static bool_t rfc_send_credit_only_frame(RFC_CTRL_PARAMS_T *rfc_params,
                                         uint16_t send_threshold);

static void rfc_queue_data_packet(RFC_CHAN_T *p_dlc, 
                                  MBLK_T *payload);

static void rfc_remove_queued_data_packet(RFC_CHAN_T *p_dlc);

static MBLK_T *get_next_payload(RFC_CTRL_PARAMS_T *rfc_params);

/* Function specifications.
*/ 


/*! \brief Handle credit return timer firing event
 
    \param arg1 - generic integer value passed to the timer function during the
                  timer_start call.
    \param arg2 - generic pointer value passed to the timer function during the
                  timer_start call.
*/
void rfc_credit_return_timer_event(uint16_t arg1, void *arg2)
{
    RFC_CTRL_PARAMS_T *context = (RFC_CTRL_PARAMS_T *) arg2;
    RFC_CTRL_PARAMS_T rfc_params;
    PARAM_UNUSED(arg1);

    rfc_timer_expired(context,
                      &rfc_params);

    (void)rfc_send_credit_only_frame(&rfc_params, 0);
}


/*! \brief Wrapper function for timer start function 
 
    A wrapper to timer_start is needed in order to take a pmalloc'd copy of the
    rfcomm instance data to pass to the timer function.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param context - Pointer to context pointer 
    \param timer - timeout in seconds
*/
void rfc_timer_start(RFC_CTRL_PARAMS_T *rfc_params,
                     RFC_CTRL_PARAMS_T **context,
                     RFC_TIMER_VALUES_T timer)
{
    RFC_CTRL_PARAMS_T  *new_context;
    void (*fn)(uint16_t mi, void *mv) = NULL;
    TIME delay = 0;

    if(NULL != *context)
    {
        /* There is already a timer running which is using this context! This
           should never happen and if it does indicates a fatal condition!
        */ 
        BLUESTACK_PANIC(
                        PANIC_RFCOMM_TIMER_ALREADY_STARTED);
    }

    new_context = zpnew(RFC_CTRL_PARAMS_T);

    /* Store the pointer to pmalloc'd area
    */ 
    *context = new_context;

    /* Copy the instance data to the context.
    */ 
    qbl_memscpy((void *)*context,
            sizeof(RFC_CTRL_PARAMS_T),
            (void *)rfc_params,
            sizeof(RFC_CTRL_PARAMS_T));

    switch(timer)
    {
        case RFC_CREDIT_RETURN_TIMER:

            fn = rfc_credit_return_timer_event;
            delay = RFC_CREDIT_RETURN_TIMER_VALUE;
            RFC_LOG_DEBUG("RFCOMM Credit timer Start ");
            break;

        default:      
            /* Error!
            */ 
            BLUESTACK_PANIC(
                            PANIC_RFCOMM_INVALID_TIMER_TYPE);
            BLUESTACK_BREAK_IF_PANIC_RETURNS
    }

    timer_start(&((*context)->chan_timer), 
                delay,
                fn,
                0,
                (void *)(*context));
}

/*! \brief Wrapper function for timer cancel function 
 
    A wrapper to timer_cancel is needed in order to free the pmalloc'd copy of
    the rfcomm instance data.
 
    \param context - Pointer to context pointer 
*/
void rfc_timer_cancel(RFC_CTRL_PARAMS_T **context)
{
    if(NULL != *context)
    {
        RFC_LOG_DEBUG("RFCOMM timer Cancel ");
        timer_cancel(&((*context)->chan_timer));
        pfree(*context);
        *context = NULL;        
    }
}


/*! \brief Tidy up timer context once the timer has fired. 
 
    Restores the rfcomm instance data from the supplied context. A search is
    then made through the p_dlc or the p_mux looking for the matching context.
    If p_dlc is non-NULL then it means it was a data channel for which the timer
    was started. If p_dlc is NULL but p_mux is non-NULL then it was the mux for
    which a timer was started. If both are NULL then something has gone
    seriously wrong!!
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param context - Context pointer 
*/
void rfc_timer_expired(RFC_CTRL_PARAMS_T *context,
                       RFC_CTRL_PARAMS_T *rfc_params)
{
    bool_t match_dlc = FALSE;
    bool_t match_mux = FALSE;
    uint8_t i;

    qbl_memscpy((void *)rfc_params,
            sizeof(RFC_CTRL_PARAMS_T),
            (void *)context, 
            sizeof(RFC_CTRL_PARAMS_T));


    /* Determine which channel the timer was hooked to...
    */ 
    if(NULL != rfc_params->p_dlc)
    {   
        for(i=0; i<RFC_NUM_TIMERS; i++)
        {
            if(rfc_params->p_dlc->timers[i] == context)
            {
                rfc_params->p_dlc->timers[i] = NULL;
                match_dlc = TRUE;
                break;
            }            
        }
    }
                    
    if(NULL != rfc_params->p_mux)
    {    
        for(i=0; i<=RFC_TEST_CONTEXT; i++)
        {
            if(rfc_params->p_mux->timers[i] == context)
            {
                rfc_params->p_mux->timers[i] = NULL;
                match_mux = TRUE;
                break;
            }
        }
    }

    RFC_LOG_DEBUG("RFCOMM timer Expired %d ", match_dlc);

    if(!match_dlc && !match_mux)
    {
        BLUESTACK_PANIC(
                        PANIC_RFCOMM_INVALID_TIMER_CONTEXT);                    
    }

    pfree(context);
}


/*! \brief Remove and delete a queued data packet 
 
    \param p_dlc - Pointer to the channel for which the data is destined.
*/
static void rfc_remove_queued_data_packet(RFC_CHAN_T *p_dlc)
{
    RFC_QUEUED_DATA_T *p = p_dlc->info.dlc.p_data_q->p_next;

    p_dlc->info.dlc.p_data_q->payload = NULL;
    pfree(p_dlc->info.dlc.p_data_q);
    p_dlc->info.dlc.p_data_q = p;

}

/*! \brief Queue a received data packet 
 
    If a data packet (host or stream sourced) cannot be immediately/fully
    processed on receipt it is added to an internal queue.
 
    \param p_dlc - Pointer to the channel for which the data is destined.
    \param payload - pointer to the MBLK containing the data
*/
static void rfc_queue_data_packet(RFC_CHAN_T *p_dlc, 
                                  MBLK_T *payload)
{
    RFC_QUEUED_DATA_T *p;
    RFC_QUEUED_DATA_T **pp;
    RFC_QUEUED_DATA_T *new_p;

    pp = &(p_dlc->info.dlc.p_data_q);

    if(NULL != *pp)
    {
        /* Find end of queued data.
        */ 
        for (p = *pp; p->p_next != NULL ; p = p->p_next)
        {
            ;
        }
        
        /* p now points to the last element in the q.
        */ 
        pp = &(p->p_next);
    }

    /* Create a new queue element and add this payload to it.
    */ 
    new_p = pnew(RFC_QUEUED_DATA_T);
    new_p->p_next = NULL;
    new_p->payload = payload;

    *pp = new_p;
}


/*! \brief Handle receipt of a new data packet 
 
    This function is called when a data kick has been received from either the
    host (by receipt of a datawrite req) or from the stream subsystem. The kick
    may have been just for credits, just for data or both. If data is present
    then it is added to the data queue. The main process data function is then
    called.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param payload - pointer to the MBLK containing the raw data
    \param credit_frame_send - Send a credit frame
*/
void rfc_new_data_packet(RFC_CTRL_PARAMS_T *rfc_params,
                         MBLK_T *payload, bool_t credit_frame_send)
{
    if( NULL != payload )
    {
        rfc_queue_data_packet(rfc_params->p_dlc,
                              payload);
    }

     /* Kick the rfc_process_txdata, it will take care of all the cases
      */
    rfc_process_txdata(rfc_params, credit_frame_send);
}

/*! \brief Send a raw data UIH frame to the peer
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param payload_len - Size of actual raw data. Will be 0 if a pure credit
                         frame is required.
    \param rx_credits - Number of credits to sent to the peer
    \param p_data - pointer to the MBLK containing the raw data
*/
void rfc_send_raw_uih_frame(RFC_CTRL_PARAMS_T *rfc_params,
                            uint16_t payload_len,
                            uint8_t rx_credits,
                            MBLK_T   *p_data)
{
    MBLK_T   *mblk_data;
    uint8_t  *data_frame = NULL;
    MBLK_T   *mblk_fcs;
    uint8_t  *fcs_frame = NULL;
    uint8_t  frame_len = RFC_MIN_FRAME_LEN;
    uint8_t  fcs; 
    uint16_t conn_id = 0;
    RFC_LINK_DESIGNATOR from = 
        RFC_IS_INITIATOR(rfc_params->p_mux->flags)
        ? RFC_INITIATOR 
        : RFC_RESPONDER;

    if(payload_len > 0)
    {
        conn_id = rfc_params->p_dlc->info.dlc.conn_id;
        if(payload_len <= RFC_CMD_MAX_LEN_OCTET_VAL)
        {
            /* If sending data then the FCS will be put in a separate MBLK and
               appended to the data, thus reduce the default frame length.
               However if the payload length is >7 bits worth then the frame
               length would be increased by one byte and thus the addition and
               subtraction cancel each other out.
            */ 
            frame_len --;
        }
    }

    if(rx_credits > 0)
    {
        frame_len ++; /* For the credit field */                    

        /* Precalculated fcs for a data frame with credits
        */ 
        fcs = RFC_FCS_UIH_PF(rfc_params->p_dlc->fcs_out);          

        /* If we are sending credits then we can cancel any outstanding credit
           timer.
        */ 
        rfc_timer_cancel(&(rfc_params->p_dlc->timers[RFC_CREDIT_CONTEXT]));
    }
    else
    {
        /* Precalculated fcs for a pure data frame
        */ 
        fcs = RFC_FCS_UIH(rfc_params->p_dlc->fcs_out);          
    }

    mblk_data = mblk_malloc_create((void **)&data_frame, 
                                   frame_len);

    if (NULL == mblk_data)
    {
        /* PANIC , memory exhaustion!
        */ 
        BLUESTACK_PANIC(PANIC_MBLK_CREATE_FAILURE);        
    }

    /* Create the front end of the UIH data frame.
    */ 
    rfc_create_uih_header(rfc_params->p_dlc->dlci,
                          &data_frame,
                          payload_len,
                          from,
                          rx_credits);

    if(payload_len > 0)
    {
        /* Append this MBLK to the front of the data one.
        */ 
        mblk_data = mblk_add_head(mblk_data, p_data);

        mblk_fcs = mblk_malloc_create((void **)&fcs_frame, 1);
        if (NULL == mblk_fcs)
        {
            /* PANIC , memory exhaustion!
            */ 
            BLUESTACK_PANIC(PANIC_MBLK_CREATE_FAILURE);        
        }        
        write_uint8(&fcs_frame, fcs);
        mblk_data = mblk_add_tail(mblk_fcs, mblk_data);
    }
    else
    {
        write_uint8(&data_frame, fcs);
    }

    /* Send to L2CAP. Conn id sent as user context in order to track
       datawrite confirms for raw data packets.
    */ 
    L2CA_DataWriteReqEx(rfc_params->p_mux->info.mux.cid, 
                        0,/* Indicates MBLK interface*/
                        mblk_data,
                        conn_id);

#if defined(BUILD_FOR_HOST) && !defined(BLUESTACK_HOST_IS_APPS)
    /* If this is an off chip host stack and this packet contains data (ie not a
       credit only one) then inform the host that a data packet has been sent to
       L2CAP.
    */ 
    if(payload_len > 0)
    {
        rfc_send_datawrite_cfm(rfc_params->p_dlc->phandle,
                               rfc_params->p_dlc->info.dlc.conn_id,
                               RFC_DATAWRITE_PENDING,
                               FALSE);
    }
#endif 
}

/*! \brief Send a credit only data frame to the peer
 
    Create and send a credit only data packet to the peer device providing there
    are some credits available.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param send_threshold - number of credits which will trigger the send
*/
static bool_t rfc_send_credit_only_frame(RFC_CTRL_PARAMS_T *rfc_params,
                                         uint16_t send_threshold)
{
    if(rfc_params->p_dlc->info.dlc.rx_credits > send_threshold)
    {
        uint8_t rx_credits_to_send = 
            MIN(RFC_MAX_RX_CREDITS_PER_FRAME, rfc_params->p_dlc->info.dlc.rx_credits);            
    
        rfc_send_raw_uih_frame(rfc_params,
                               0,
                               rx_credits_to_send,
                               NULL);
        rfc_params->p_dlc->info.dlc.rx_credits -= rx_credits_to_send;
        return TRUE;
    }

    return FALSE;
}


/*! \brief Handle a RFC_DATAWRITE_REQ primitive
 
    This function validates a datawrite request from the host and if there is
    valid data and/or credits to send to the peer device then it is passed to
    the tx data processing function.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param prim - Pointer to the data to be sent
*/
void rfc_handle_datawrite_req(RFC_CTRL_PARAMS_T *rfc_params,
                              RFC_DATAWRITE_REQ_T *prim) 
{
    RFC_RESPONSE_T status = RFC_INVALID_PAYLOAD;
    uint16_t payload_len = mblk_get_length(prim->payload) ;
    bool_t credit_frame_send = FALSE;

    if(prim->rx_credits > 0 || payload_len > 0)
    {
        rfc_find_chan_by_id(rfc_params, 
                            prim->conn_id);

        /*
         * If locally triggered rfcomm disconnect is underway (i.e. local device is awaiting UA control
         * frame from remote peer) then further data exchanges from either device end can be
         * ignored/dropped for optimal performance.
         */
        if(NULL != rfc_params->p_dlc &&
           !RFC_IS_DISCONNECTING(rfc_params->p_dlc->flags))
        {
            /* Check the payload length against the negotiated max frame 
                size 
            */
            if( payload_len <= rfc_params->p_dlc->info.dlc.max_frame_size)
            {
                rfc_params->p_dlc->info.dlc.rx_credits += prim->rx_credits;

                if(!payload_len)
                {
#ifdef INSTALL_SOCKET_OFFLOAD                
                   rfc_params->p_dlc->info.dlc.allocated_rx_credits += prim->rx_credits;
#endif
                   credit_frame_send = TRUE;
                }

                rfc_new_data_packet(rfc_params,
                                prim->payload, credit_frame_send);
                prim->payload = NULL;
                return;
            }
        }
        else
        {
            status = RFC_INVALID_CHANNEL;
        }
    }

    mblk_destroy(prim->payload);

    rfc_send_datawrite_cfm(rfc_params->rfc_ctrl->phandle,
                           prim->conn_id,
                           status,
                           rfc_params->rfc_ctrl->use_streams);                

}

/*! \brief get next MBLK containing payload

    This function prepares a mblk of maximum size of rfcomm maximum frame size 
    from a given stream buffer. 
    In non-stream case, it splits out a mblk of maximum size of rfcomm maximum 
    frame size.

    \param rfc_params Pointer to rfcomm instance data.
*/
static MBLK_T *get_next_payload(RFC_CTRL_PARAMS_T *rfc_params)
{
    MBLK_T* p_data;

    if (NULL == rfc_params->p_dlc->info.dlc.p_data_q)
        return NULL;

    p_data = rfc_params->p_dlc->info.dlc.p_data_q->payload;

    /*
     * If complete p_data fits into one rfcomm packet then payload will be NULL
     * So, move p_data_q to next element and free current p_data_q.
     */
    if ((rfc_params->p_dlc->info.dlc.p_data_q->payload = 
          mblk_split(p_data, rfc_params->p_dlc->info.dlc.max_frame_size)) == NULL)
    {
        rfc_remove_queued_data_packet(rfc_params->p_dlc);
    }

    return p_data;
}

/*! \brief Process data from the Host or Stream interfaces
 
    This function will try and process any currently queued data for as long as
    there are tx credits available.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param credit_frame_send - Send a credit frame
*/
void rfc_process_txdata(RFC_CTRL_PARAMS_T *rfc_params, bool_t credit_frame_send)
{
    bool_t data_frame_sent = FALSE;
    bool_t use_credit_fc = RFC_CREDIT_FC_USED(rfc_params->p_mux->flags);

    for(;;)
    {
        MBLK_T *p_data;
        uint8_t rx_credits_to_send = 0;

        /* See if flow control allow us to send data. */

        if (use_credit_fc)
        {
            if(rfc_params->p_dlc->info.dlc.tx_credits == 0)
            {
                /* Just pass the control to the place where we can send a
                   credit only frame
                */
                break;
            }
        }
        else
        {
            if(!RFC_IS_FC_TX_ENABLED(rfc_params->p_mux->flags) ||
               !RFC_IS_MSC_FC_TX_ENABLED(rfc_params->p_dlc->flags))
            {
                /* Flow control is asserted so process nothing. */
                return;
            }
        }

        /* See if we have any data to send. */
        if ((p_data = get_next_payload(rfc_params)) == NULL)
            break;

        /* We have data, consume a credit and work out how many to return. */
        if (use_credit_fc)
        {
            rx_credits_to_send =
                (uint8_t)(MIN(RFC_MAX_RX_CREDITS_PER_FRAME,
                              rfc_params->p_dlc->info.dlc.rx_credits));

            rfc_params->p_dlc->info.dlc.tx_credits--;
            rfc_params->p_dlc->info.dlc.rx_credits -= rx_credits_to_send;
        }

        rfc_send_raw_uih_frame(rfc_params,
                               mblk_get_length(p_data),
                               rx_credits_to_send,
                               p_data);

        data_frame_sent = TRUE;
    }

    if (!data_frame_sent)
    {
        uint16_t thresh = credit_frame_send ? 0 :
            rfc_params->p_dlc->info.dlc.allocated_rx_credits / RFC_CREDIT_SEND_THRESHOLD;

        /* Try to send a credit only frame, for DATAWRITES is payload size 0
             a credit only frame is immediatly sent.
        */
        rfc_try_send_credit_only_frame(rfc_params, thresh);
     }

}

/*! \brief try to send a credit only frame
 
    This function will try to send a credit only frame based on watermark
    if it is unable to send then it starts a timer of 500ms to send the 
    credit only frame afterwards unconditionally.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param thresh        - water mark for sending the credit only frame
*/
void rfc_try_send_credit_only_frame(RFC_CTRL_PARAMS_T *rfc_params, uint16_t thresh)
{
    /* Check if we have hit the credit threshold which triggers a
         credit only data frame to be sent to the peer.
    */ 
    if(rfc_params->p_dlc != NULL)
    {
       if(!rfc_send_credit_only_frame(rfc_params, thresh))
       {
           /* If we have credits that werent sufficient to trigger the
                threshold and we dont have a timer currently running, then
                start one.
           */
           if(NULL == rfc_params->p_dlc->timers[RFC_CREDIT_CONTEXT])
           {
               /* Start credit timer of 500ms, we will send a credit frame anyway on its expiry.
                    if a frame with credit is sent from somewhere else this timer is stopped.
               */ 
               rfc_timer_start(rfc_params,
                           &(rfc_params->p_dlc->timers[RFC_CREDIT_CONTEXT]),
                           RFC_CREDIT_RETURN_TIMER);
            }
        }
    }
}

/*! \brief Handle a RFC_DATAREAD_RSP primitive
 
    Handles a dataread response from the host. This indicates that the previous
    data sent has been used and thus a credit can be released (if credit based
    flow control is being used).

    \param rfc_params - Pointer to rfcomm instance data.
    \param conn_id - connection id of the channel for which the dataread rsp
                     was issued.
*/
void rfc_handle_dataread_rsp(RFC_CTRL_PARAMS_T *rfc_params,
                             uint16_t conn_id)
{
    rfc_find_chan_by_id(rfc_params, conn_id);

    /*
     * If locally triggered rfcomm disconnect is underway (i.e. local device is awaiting UA control
     * frame from remote peer) then further data exchanges from either device end can be
     * ignored/dropped for optimal performance.
     */
    if(NULL != rfc_params->p_dlc &&
       !RFC_IS_DISCONNECTING(rfc_params->p_dlc->flags))
    {
        /* Response has been received for a valid channel thus update the
           credits if required.
        */ 
        if(RFC_CREDIT_FC_USED(rfc_params->p_mux->flags))
        {
            uint16_t thresh = 
            rfc_params->p_dlc->info.dlc.allocated_rx_credits / RFC_CREDIT_SEND_THRESHOLD;

            rfc_params->p_dlc->info.dlc.rx_credits ++;

            rfc_try_send_credit_only_frame(rfc_params, thresh);
        }
    }
}

#endif /* INSTALL_RFCOMM_MODULE */
