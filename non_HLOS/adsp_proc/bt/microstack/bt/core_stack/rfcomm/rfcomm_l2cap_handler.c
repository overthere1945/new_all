/*******************************************************************************

Copyright (C) 2009 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "rfcomm_private.h"

#ifdef INSTALL_RFCOMM_MODULE

/*! Local function prototypes.
*/ 
static void rfc_handle_l2ca_dataread_ind(RFC_CTRL_PARAMS_T *rfc_params,
                                         L2CA_DATAREAD_IND_T *p_prim);
static void rfc_handle_l2ca_disconnect_ind(RFC_CTRL_PARAMS_T *rfc_params,
                                           L2CA_DISCONNECT_IND_T *p_prim);
static void rfc_handle_l2ca_datawrite_cfm(RFC_CTRL_PARAMS_T *rfc_params,
                                          L2CA_DATAWRITE_CFM_T *p_prim);


/*! \brief Handler an L2CAP data read indication
 
    This function processes a L2CA_DATAREAD_IND prim. The primitive is first
    checked to see if the received data was successful. If it was then a search
    by CID is made to try and locate a matching Mux channel. The data portion is
    passed to a matching Mux. Failure to match the a Mux is serious and will
    cause an L2CAP disconnect request for the specified CID to be sent.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param p_prim - Pointer to received primitive
*/ 
static void rfc_handle_l2ca_dataread_ind(RFC_CTRL_PARAMS_T *rfc_params,
                                         L2CA_DATAREAD_IND_T *p_prim)
{
    rfc_params->mblk = (MBLK_T *)p_prim->data; 

    /* Unlink MBLK from primitive so it doesn't get destroyed automatically */
    p_prim->data = NULL;
    
    /* Check that L2CAP data was actually received successfully */
    if (p_prim->result == L2CA_DATA_SUCCESS)
    {
        /* Get the Mux channel associated with the CID on which this data
           arrived.
        */ 
        rfc_get_mux_by_cid(rfc_params, p_prim->cid);

        if (NULL != rfc_params->p_mux)
        {
            /* mblk will be tidied up downline in this case.
            */
            rfc_parse_frame(rfc_params,
                            mblk_get_length(rfc_params->mblk));  
            
            if (p_prim->packets > 0)
            {
                L2CA_DataReadRsp(p_prim->cid, p_prim->packets);                
            }

            return;
        }
    }

    /* To get here either a dataread ind with an unsuccessful result or an
       unknown CID has been received, either way destroy the MBLK. We do
       nothing about the rogue L2CAP channel though except ignore it.
    */ 
    mblk_destroy(rfc_params->mblk);
}

/*! \brief Handler an L2CAP disconnect indication

    \param rfc_params - Pointer to rfcomm instance data.
    \param p_prim - Pointer to received primitive
*/ 
static void rfc_handle_l2ca_disconnect_ind(RFC_CTRL_PARAMS_T *rfc_params,
                                           L2CA_DISCONNECT_IND_T *p_prim)
{
    RFC_RESPONSE_T  reason = RFC_ABNORMAL_DISCONNECT;

    rfc_get_mux_by_cid(rfc_params, p_prim->cid);

    if (NULL != rfc_params->p_mux)
    {
        /* Found a matching Mux, thus shutdown the associated RFCOMM session.
           A disconnected l2cap channel for which there is an RFCOMM session
           would be considered an ABNORMAL DISCONNECT at the RFCOMM layer.
           However if the l2cap channel has also abnormally dropped then we will
           report that status instead.
        */
        if (p_prim->reason != L2CA_DISCONNECT_NORMAL)
        {
            reason = (RFC_RESPONSE_T)p_prim->reason;
        }
        rfc_shutdown_session(rfc_params, reason); 
    }

    L2CA_DisconnectRsp(p_prim->identifier,
                       p_prim->cid);

}

 
/*! \brief Handler an L2CAP data write confirmation
 
    This function processes a L2CA_DATAWRITE_CFM prim. 
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param p_prim - Pointer to received primitive
*/ 
static void rfc_handle_l2ca_datawrite_cfm(RFC_CTRL_PARAMS_T *rfc_params,
                                          L2CA_DATAWRITE_CFM_T *p_prim)
{
    RFC_CHAN_T *p_mux;

    /* Check that the datawrite was successful, if not then this indicates
       terminal failure of the L2CAP link and means we have no choice but to
       issue a disconnect request for that cid. First lookup the cid for which
       the datawrite was received. If it is unknown then we will just ignore
       this prim.
    */ 
    rfc_get_mux_by_cid(rfc_params, p_prim->cid);

    if (NULL != rfc_params->p_mux)
    {
        if (p_prim->result == L2CA_DATA_SUCCESS)
        {
            /* Valid cid, check user context to see if this is the confirmation of a raw data
               packet being sent as these are the only ones we care about.
            */ 
            if (0 != p_prim->req_ctx)
            {
                /* Preserve mux matched by cid as the channel search will
                   overwrite it.
                */ 
                p_mux = rfc_params->p_mux;
                rfc_find_chan_by_id(rfc_params, p_prim->req_ctx);

                /* If the channel was found then both the p_mux and p_dlc fields will be
                   non NULL. Ensure the channel matched mux matches that matched to
                   the cid.
                */ 
                if(NULL != rfc_params->p_dlc && 
                   p_mux == rfc_params->p_mux)
                {
                    /* This is a datawrite confirm for a raw data packet on a
                       known channel thus send datawrite confirm to host.
                    */ 
                    rfc_send_datawrite_cfm(rfc_params->p_dlc->phandle,
                                           rfc_params->p_dlc->info.dlc.conn_id,
                                           RFC_SUCCESS,
                                           rfc_params->rfc_ctrl->use_streams);
                }
            }      
        }
        else
        {
            BLUESTACK_WARNING(RFCOMM_ERROR);
        }
    }
}
 
/*! \brief Process an L2CAP primitive
 
    This function processes an l2cap primitive and then frees it.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param l2cap_prim - Pointer to received primitive
*/ 
void rfc_l2cap_handler(RFC_CTRL_PARAMS_T *rfc_params,
                       L2CA_UPRIM_T *l2cap_prim)
{
    switch (l2cap_prim->type)
    {                        
        case L2CA_DATAWRITE_CFM:
            rfc_handle_l2ca_datawrite_cfm(rfc_params,
                                          &l2cap_prim->l2ca_datawrite_cfm);
            break;
            
        case L2CA_DATAREAD_IND:
            rfc_handle_l2ca_dataread_ind(rfc_params,
                                         &l2cap_prim->l2ca_dataread_ind);
            break;
            
        case L2CA_DISCONNECT_IND:
            rfc_handle_l2ca_disconnect_ind(rfc_params,
                                           &l2cap_prim->l2ca_disconnect_ind);
            break;

        default:
            break;
    }

    L2CA_FreePrimitive(l2cap_prim);
}


#endif /* INSTALL_RFCOMM_MODULE */
