/*******************************************************************************
Copyright (C) 2009 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\brief  Handles the creation and destruction of the mux and data channels.

*******************************************************************************/
#include "rfcomm_private.h"
#include "qbl_adapter_logging.h"

#ifdef INSTALL_RFCOMM_MODULE

static uint16_t rfc_allocate_conn_id(RFC_CTRL_PARAMS_T *rfc_params);
static void rfc_dlc_destroy(RFC_CTRL_PARAMS_T *rfc_params);
static void rfc_mux_destroy(RFC_CTRL_PARAMS_T *rfc_params);
static void rfc_cancel_all_timers(RFC_CHAN_T *chan);


/*! \brief nostream RFC operator table

    This table contain pointers to functions for the various operations that
    can be performed during the data transfer (kick for new data or, a notification)
    without using streams.
*/

static const RFC_ACTIONS_VTABLE_T nostream_vtable =
{
    rfc_send_dataread_ind,
    rfc_process_txdata
};


/*! \brief Create a new MUX structure.

    \param rfc_params - Pointer to rfcomm instance data.
    \param phandle - Protocol handle used to identify the higher
                     entity to which primitives will be sent.
    \param p_bd_addr - Pointer to the bluetooth address of the peer
    \param local_l2cap_control - Amp specific identifier
    \param initiator - True if the Mux is being created by the initiator of the
                       connection , False otherwise
*/
bool_t rfc_mux_new(RFC_CTRL_PARAMS_T *rfc_params,
                 phandle_t         phandle,
                 BD_ADDR_T         *p_bd_addr,
                 l2ca_controller_t local_l2cap_control, 
                 bool_t            initiator)
{
    RFC_CHAN_T *p_mux;

    PARAM_UNUSED(p_bd_addr);
    PARAM_UNUSED(local_l2cap_control);

    /* Allocate new mux structure */
    p_mux = xzpnew(RFC_CHAN_T);
    if (p_mux == NULL)
    {
        /* Allocation fails, return false*/
        return FALSE;
    }

    /* Initialise mux */
    p_mux->phandle = phandle;
    p_mux->state = RFC_ST_DISCONNECTED;

    /* The CID will be completed once the L2CAP channel is up.
    */ 
    p_mux->info.mux.cid = L2CA_CID_INVALID;

    /* Initialise the flow control flags. We assume that CFC will be used and
       CFC ON is 0 so there is no bit to enable.
    */
    RFC_SET_FC_TX_ENABLED(p_mux->flags); 
    RFC_SET_FC_RX_ENABLED(p_mux->flags); 

    /* Set the initiator bit for the RFCOMM session. If we are the initiator
       then we must always attempt to use credit based flow control.
    */ 
    if (initiator)
    {
        RFC_SET_INITIATOR(p_mux->flags);
    }

    /* Precalculate the fcs for data frames received on the Mux channel.
    */ 
    rfc_precalc_fcs_values(p_mux);

    /* Add new mux to head of list */
    p_mux->p_next = rfc_params->rfc_ctrl->mux_list;
    rfc_params->rfc_ctrl->mux_list = p_mux;

    rfc_params->p_mux = p_mux;
    return TRUE;
}

/*! \brief Lookup a Mux channel using a CID.
 
    Searches through the list of Mux channels available looking for a matching
    CID. If one is found then a pointer to its data structure is logged in the
    rfcomm instance.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param cid - L2CAP channel id to search for.
*/
void rfc_get_mux_by_cid(RFC_CTRL_PARAMS_T *rfc_params,
                        l2ca_cid_t  cid)
{
    RFC_CHAN_T *p_mux;

    for (p_mux = rfc_params->rfc_ctrl->mux_list; 
         p_mux != NULL && p_mux->info.mux.cid != cid; 
         p_mux = p_mux->p_next)
        ;

    rfc_params->p_mux = p_mux;
}

/*! \brief Find a channel using a DLCI.
 
    Scan all the channels associated with the supplied mux (as logged in the
    rfcomm instance data) for a matching dlci. NB This could match the mux
    channel itself. If a match is found then the dlc data structure is also
    logged in the rfcomm instance.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param dlci - dlc channel id to look for
*/
void rfc_find_chan_by_dlci(RFC_CTRL_PARAMS_T *rfc_params,
                           uint8_t dlci)
{
    RFC_CHAN_T *p_dlc;

    if (dlci == 0)
    {
        rfc_params->p_dlc = rfc_params->p_mux;
        return;
    }

    for (p_dlc = rfc_params->p_mux->info.mux.p_dlcs;
         p_dlc != NULL;
         p_dlc = p_dlc->p_next)
    {
        if (p_dlc->dlci == dlci)
        {
            break;
        }
    }

    rfc_params->p_dlc = p_dlc;
}


/*! \brief Find a channel using a connection id.
 
    Scan all the channels associated with the supplied mux (as logged in the
    rfcomm instance data) for a matching connection id. If a match is found then
    both the mux and the dlc data structures are logged in the rfcomm instance.
    If no match is found then the mux and dlc structures in the rfcomm instance
    are set to NULL.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param conn_id - connection id to look for
*/
void rfc_find_chan_by_id(RFC_CTRL_PARAMS_T *rfc_params,
                         uint16_t conn_id)
{
    RFC_CHAN_T *p_mux;
    RFC_CHAN_T *p_dlc;

    for (p_mux = rfc_params->rfc_ctrl->mux_list; 
         p_mux != NULL; 
         p_mux = p_mux->p_next)
    {
        /* Check the client channels
        */ 
        for (p_dlc = p_mux->info.mux.p_dlcs;
             p_dlc != NULL;
             p_dlc = p_dlc->p_next)
        {
            if (p_dlc->info.dlc.conn_id == conn_id)
            {
                rfc_params->p_mux = p_mux;
                rfc_params->p_dlc = p_dlc;
                return;
            }
        }
    }

    rfc_params->p_mux = NULL;
    rfc_params->p_dlc = NULL;
}


/*! \brief Pre-calculates and stores checksums for data frames.
 
    The checksums for UIH frames are calculated purely on the address and
    control fields which are fixed for a specific DLC. Thus at the point of DLC
    creation the checksum can be calculated once and stored and then just used
    whenever required.
 
    \param p_chan - Pointer to DLC to calculate checksum for.
*/
void rfc_precalc_fcs_values(RFC_CHAN_T *p_chan)
{
    uint8_t data[RFC_UIH_FCS_CALC_SIZE];
    uint8_t address;
    uint8_t dirbit;
    RFC_LINK_DESIGNATOR from;

    /* Outgoing UIH/UIH_PF frames
    */ 
    dirbit = RFC_DIRBIT(p_chan->flags);
    from = (dirbit == 1) ?  RFC_INITIATOR : RFC_RESPONDER;
    
    address = rfc_create_address_field(p_chan->dlci,
                                       from,
                                       RFC_DATA,
                                       RFC_FRAMELEVEL);

    /* Calculate and store the FCS without credit based flow control.
    */ 
    data[0] = address;
    data[1] = RFC_UIH;
    p_chan->fcs_out = rfc_frame_crc(data, RFC_UIH_FCS_CALC_SIZE);

    /* Calculate and store the FCS with credit based flow control. The address
       field is the same so this can be stepped over.
    */ 
    data[1] = RFC_UIH_PF;
    
    p_chan->fcs_out |= 
        (rfc_frame_crc(data, RFC_UIH_FCS_CALC_SIZE) << 8); 

    /* Incoming UIH/UIH_PF frames.
       Inverse of the direction bit.
    */ 
    dirbit = (~dirbit) & 0x1;
    from = (dirbit == 1) ?  RFC_INITIATOR : RFC_RESPONDER;
    address = rfc_create_address_field(p_chan->dlci,
                                       from,
                                       RFC_DATA,
                                       RFC_FRAMELEVEL);

    /* Calculate and store the FCS without credit based flow control.
    */ 
    data[0] = address;
    data[1] = RFC_UIH;
    
    p_chan->fcs_in = rfc_frame_crc(data, RFC_UIH_FCS_CALC_SIZE);

    /* Calculate and store the FCS with credit based flow control. The address
       field is the same so this can be stepped over.
    */ 
    data[1] = RFC_UIH_PF;

    p_chan->fcs_in |= 
        (rfc_frame_crc(data, RFC_UIH_FCS_CALC_SIZE) << 8);
}


/*! \brief Calculates a new , unique connection id.
 
    \param rfc_params - Pointer to rfcomm instance data.
*/
static uint16_t rfc_allocate_conn_id(RFC_CTRL_PARAMS_T *rfc_params)
{
    uint16_t conn_id;
    RFC_CHAN_T *p_mux;

    /* The search process can corrupt the p_mux value in rfc_params if a
       matching channel is found and thus we need to preserve this first.
    */
    p_mux = rfc_params->p_mux;
     
    /* Now pick a new conn id and scan to ensure this new value is not already
       in use. The 'forever' loop should be safe as even assuming we had lots
       and lots of memory available the maximum number of allocated RFCOMM
       channels we could have would be 7 (connections) * 62 channels, which is a
       lot less than the 14 bits of connection id.
    */
    for(;;)
    {
        rfc_params->rfc_ctrl->last_conn_id++;

        /* Preserve conn id marker bits.
        */ 
        rfc_params->rfc_ctrl->last_conn_id |= RFC_MIN_CONN_ID;
        conn_id = rfc_params->rfc_ctrl->last_conn_id; 

        rfc_find_chan_by_id(rfc_params,
                            conn_id);

        if (NULL == rfc_params->p_dlc)
        {
            /* There is no matching DLC with this conn_id thus it is safe to
               use.
            */ 
            break;
        }
    }

    /* Restore p_mux.
    */ 
    rfc_params->p_mux = p_mux;

    return conn_id;
}

/*! \brief Create a new DLC structure.
 
    For outgoing connections the connection parameters will be valid and passed
    down from the connect request. For incoming connections the connection
    parameters will initially be NULL.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param phandle - Protocol handle used to identify the higher
                     entity to which primitives will be sent.
    \param dlci - The DLC id for the new connection
    \param p_config - Pointer to connection parameters to be used
    \param initiator - True if the DLC is being created by the initiator of the
                       connection , False otherwise
    \param context - host supplied context value
*/
bool_t rfc_dlc_new(RFC_CTRL_PARAMS_T *rfc_params,
                 uint8_t dlci,
                 phandle_t phandle,
                 RFC_CONNECTION_PARAMETERS_T  *p_config,
                 bool_t   initiator,
                 context_t context)
{
    RFC_CHAN_T *p_new_dlc;
    RFC_CHAN_T **pp;
    RFC_CHAN_T *p;

    PARAM_UNUSED(p_config);

    /* Allocate new DLC and config structures */
    p_new_dlc = xzpnew(RFC_CHAN_T);
    if (p_new_dlc == NULL)
    {
        /* Allocation fails, return false*/
        return FALSE;
    }

    /* Initialise */
    p_new_dlc->phandle = phandle;
    p_new_dlc->state = RFC_ST_DISCONNECTED;

    p_new_dlc->info.dlc.conn_id = rfc_allocate_conn_id(rfc_params);
    p_new_dlc->dlci = dlci;
    p_new_dlc->context = context;

    if (initiator)
    {
        RFC_SET_INITIATOR(p_new_dlc->flags);        
    }

    /* Precalculate the FCS values for UIH/UIH_PF frames.
    */
    rfc_precalc_fcs_values(p_new_dlc);

    {
        p_new_dlc->info.dlc.vtable = &nostream_vtable;
    }

    /* Add new DLC to tail of list */
    for (pp = &(rfc_params->p_mux->info.mux.p_dlcs);
         (p = *pp) != NULL ;
         pp = &(p->p_next) )
    {
        ;
    }

    *pp = p_new_dlc;
    rfc_params->p_dlc = p_new_dlc;
    return TRUE;
}

/*! \brief Cancel timers.
 
    \param chan - Pointer to the channel on which to cancel the timers
*/
static void rfc_cancel_all_timers(RFC_CHAN_T *chan)
{
    /* Cancel all timers on the specified channel
    */ 
    rfc_timer_cancel(&(chan->timers[RFC_CONNECT_CONTEXT]));
    rfc_timer_cancel(&(chan->timers[RFC_DISCONNECT_CONTEXT]));
    rfc_timer_cancel(&(chan->timers[RFC_CREDIT_CONTEXT]));
    rfc_timer_cancel(&(chan->timers[RFC_RPN_CONTEXT]));
    rfc_timer_cancel(&(chan->timers[RFC_MSC_CONTEXT]));
    rfc_timer_cancel(&(chan->timers[RFC_RLS_CONTEXT]));
    rfc_timer_cancel(&(chan->timers[RFC_TEST_CONTEXT]));
    rfc_timer_cancel(&(chan->timers[RFC_MSC_INIT_CONTEXT]));
    rfc_timer_cancel(&(chan->timers[RFC_FCON_CONTEXT]));
    rfc_timer_cancel(&(chan->timers[RFC_FCOFF_CONTEXT]));
}

/*! \brief Destroy a channel structure.
 
    Removes a channel structure (a MUX or DLC) from the specified channel list
    and then frees the allocated memory.
 
    \param chan_list - Pointer to the channel list
    \param chan - Pointer to Pointer to the channel to be destroyed
*/
void rfc_channel_destroy(RFC_CHAN_T **chan_list,
                         RFC_CHAN_T **chan)
{
    RFC_CHAN_T *p;
    RFC_CHAN_T **pp;

    /* Search for the channel on the supplied list.
    */ 
    for (pp = chan_list;
         (p = *pp) != NULL ;
         pp = &(p->p_next) )
    {
        if (p == *chan)
        {
            /* Unhook this channel from the channel list.
            */ 
            *pp = p->p_next;

            rfc_cancel_all_timers(p);

            /* Free any allocated data. This would only be on a data channel.
            */ 
            if (p->dlci != 0)
            {
                if(p->info.dlc.p_data_q != NULL)
                {
                    RFC_QUEUED_DATA_T **pp_rfc_payload = NULL;
                    RFC_QUEUED_DATA_T *p_rfc_payload = NULL;

                    for(pp_rfc_payload = &(p->info.dlc.p_data_q);
                          (p_rfc_payload = *pp_rfc_payload) != NULL;
                         )
                    {
                        /* Unhook the chain */
                        *pp_rfc_payload = p_rfc_payload->p_next;

                        if(p_rfc_payload->payload != NULL)
                        {
                          mblk_destroy (p_rfc_payload->payload);
                        }

                        pfree(p_rfc_payload);
                        p_rfc_payload = NULL;
                    }
                    p->info.dlc.p_data_q = NULL;
                }
            }

            pfree(p);
            *chan = NULL;
            return;
        }
    }
}

/*! \brief Destroy a Mux structure.
 
    Removes a MUX structure from the Mux list. Basically wraps the generic
    destroy channel function.
 
    \param rfc_params - Pointer to rfcomm instance data.
*/
static void rfc_mux_destroy(RFC_CTRL_PARAMS_T *rfc_params)
{
    rfc_channel_destroy(&(rfc_params->rfc_ctrl->mux_list),
                        &(rfc_params->p_mux));
}

/*! \brief Destroy a DLC structure.
 
    Removes a DLC structure from the DLC list. Basically wraps the generic
    destroy channel function. If this is the last DLC on the list then a MUX
    shutdown timer will be started, if no new connection arrives during that
    time then the Mux will also be destroyed.
 
    \param rfc_params - Pointer to rfcomm instance data.
*/
static void rfc_dlc_destroy(RFC_CTRL_PARAMS_T *rfc_params)
{    
    /* Ensure that the p_dlc is not Mux */
    if(rfc_params->p_dlc != 0 && rfc_params->p_dlc->dlci != 0)
    {
        rfc_channel_destroy(&(rfc_params->p_mux->info.mux.p_dlcs),
                            &(rfc_params->p_dlc));
    }
    /* If this was the last DLC on the Mux channel then we may need to destroy
       the Mux as well.
    */ 
    if (NULL == rfc_params->p_mux->info.mux.p_dlcs)
    {
        rfc_mux_destroy(rfc_params);
    }
}

void rfc_release_dlc(RFC_CTRL_PARAMS_T *rfc_params)
{
    rfc_dlc_destroy(rfc_params);
}

/*! \brief Terminate a RFCOMM session.
 
    Removes all DLC structures and the associated Mux as specified in the
    instance data. Closure indications will be sent for all outstanding
    channels.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param status - reason for the shutdown
*/
void rfc_shutdown_session(RFC_CTRL_PARAMS_T *rfc_params,
                          RFC_RESPONSE_T status)
{
    RFC_CHAN_T *p_dlc;

    while ((p_dlc = rfc_params->p_mux->info.mux.p_dlcs) != NULL)
    {
        rfc_params->p_dlc = p_dlc;
        rfc_dlc_destroy(rfc_params);
    }

    /* Now destroy the MUX.
    */
    rfc_mux_destroy(rfc_params);
}


#endif /* INSTALL_RFCOMM_MODULE */
