/*******************************************************************************

Copyright (C) 2009 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "rfcomm_private.h"

#ifdef INSTALL_RFCOMM_MODULE

/*! Create a new RFCOMM primitive.
*/
#define RFC_MAKE_PRIM(TYPE, p_handle)\
    TYPE##_T *prim = zpnew(TYPE##_T);prim->type = (RFC_PRIM_T)(TYPE);prim->phandle = p_handle

/*! Shortcut for sending upstream primitives.
*/
#define RFC_SEND_PRIM(prim) \
    put_message(BMM_IFACEQUEUE, RFCOMM_PRIM, prim)


/*! \brief Handler function for RFCOMM downstream primitives.

    \param rfc_params - Pointer to rfcomm instance data.
    \param rfc_prim - received primitive
*/
void rfc_host_handler(RFC_CTRL_PARAMS_T *rfc_params,
                      RFCOMM_UPRIM_T *rfc_prim)
{
    switch (rfc_prim->type)
    {
        case RFC_DATAWRITE_REQ:
            rfc_handle_datawrite_req(rfc_params,
                                     &rfc_prim->rfc_datawrite_req);
            break;

        case RFC_DATAREAD_RSP:
            rfc_handle_dataread_rsp(rfc_params,
                                    rfc_prim->rfc_dataread_rsp.conn_id);
            break;

        default :
            /* Unknown primitive from the host.
            */
            rfc_send_error_ind(rfc_params->rfc_ctrl->phandle,
                               rfc_prim->type,
                               RFC_UNKNOWN_PRIMITIVE);
            break;
    }

    pfree(rfc_prim);
}

/*! \brief Send an RFC_DISCONNECT_IND.

    This function notifies the host of a remote disconnection of a specific
    channel.

    \param p_dlc - pointer to the data for the disconnected channel
    \param reason - disconnection code
*/
void rfc_send_disconnect_ind(RFC_CHAN_T     *p_dlc,
                             RFC_RESPONSE_T reason)
{
    RFC_MAKE_PRIM(RFC_DISCONNECT_IND, p_dlc->phandle);
    prim->conn_id = p_dlc->info.dlc.conn_id;
    prim->reason = reason;
    RFC_SEND_PRIM(prim);
}


/*! \brief Send an RFC_DATAWRITE_CFM.

    This function is called from the data packet processing code which could be
    for a stream or datawrite request from the host. To avoid having to check
    for streams all over that code, the check is made here and the cfm only sent
    if it is not a stream.

    \param phandle - protocol handle used to determine the message destination
    \param conn_id - connection id of the channel for which the modem status req
                     was issued.
    \param status - RFC_SUCCESS or error code
    \param is_stream - TRUE if the channel is a stream, in which case no cfm is
                       sent.
*/
void rfc_send_datawrite_cfm(phandle_t phandle,
                            uint16_t  conn_id,
                            RFC_RESPONSE_T   status,
                            bool_t is_stream)
{
    if(!is_stream)
    {
        RFC_MAKE_PRIM(RFC_DATAWRITE_CFM, phandle);

        prim->conn_id = conn_id;
        prim->status = status;
        RFC_SEND_PRIM(prim);
    }
}

/*! \brief Send an RFC_DATAREAD_IND.

    This function forwards a raw data packet to the host. Note this function has
    a boolean return parameter in order for it to be used in a function pointer
    variable in the DLC data structure as the notify function depending on
    whether streams are in use or not.

    \param rfc_params - Pointer to rfcomm instance data.
    \returns result - Always TRUE for this function.
*/
bool_t rfc_send_dataread_ind(RFC_CTRL_PARAMS_T *rfc_params)
{
    RFC_MAKE_PRIM(RFC_DATAREAD_IND, rfc_params->p_dlc->phandle);

    /* If we are on chip then we can leave the data in the mblk and let
       non_hci_convert extract and send it across the wire. In this case we
       set the primitive length to 0 to signify mblk. The mblk will be destroyed
       by non_hci_convert.
    */
    prim->payload_length = 0;
    prim->payload = rfc_params->mblk;
    prim->conn_id = rfc_params->p_dlc->info.dlc.conn_id;

    RFC_SEND_PRIM(prim);

    return TRUE;
}

/*! \brief Send an RFC_ERROR_IND.

    This function reports a generic error to the host.

    \param phandle - protocol handle used to determine the message destination
    \param err_prim_type - type of the prim causing the error
    \param status - Actual error

*/
void rfc_send_error_ind(phandle_t phandle,
                        RFC_PRIM_T err_prim_type,
                        RFC_RESPONSE_T  status)
{
    RFC_MAKE_PRIM(RFC_ERROR_IND, phandle);

    prim->err_prim_type = err_prim_type;
    prim->status = status;

    RFC_SEND_PRIM(prim);
}


#endif /* INSTALL_RFCOMM_MODULE */
