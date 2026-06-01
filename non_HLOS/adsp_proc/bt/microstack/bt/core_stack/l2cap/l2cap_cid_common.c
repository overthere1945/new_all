/*******************************************************************************

Copyright (C) 2008 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.


\brief  This file implements the L2CAP channel state machine.

        For every L2CAP channel there will exist an instance of the
        CIDCB structure, this structure holds all the state
        information for a L2CAP channel.

        This file includes the channel state machine, upper and lower
        interfaces. The data path is removed from the state machine
        for simplicity and speed reasons.
*******************************************************************************/

#include "l2cap_private.h"

/*!\brief Notify CID instance of received L2CAP packet.

    Data from channel has been read and is targeted this particular
    CID. Send data either to upper layer or into special handler
    function like the F&EC code or the fixed channel buffer.

    This function is NOT used for connectionless data - that's handled
    directly in CH_DataRxConnectionless.
*/
bool_t CID_DataReadInd(CIDCB_T *cidcb,
                       MBLK_T *mblk_ptr,
                       mblk_size_t mblk_size,
                       uint8_t *header)
{

#ifdef INSTALL_L2CAP_LECOC_CB
    if(CID_IsCBFlowControl(cidcb))
    {
        /*
         * CID is using Credit Based Flow Control mode and hence pass data
         * to Credit Based Flow Control module.
         */
        CB_FLOW_DataRead(cidcb, mblk_ptr, mblk_size);
        return TRUE;
    }
    else
#endif
    {
        /* Make sure length is less than MTU in basic mode */
        if (mblk_size <= cidcb->local_mtu)
        {
            /* Pass L2CAP packet to higher layer */
            L2CA_DataReadInd(cidcb, mblk_ptr, L2CA_DATA_SUCCESS, 0, NULL);
            return TRUE;
        }
    }

    QBL_UNUSED(header);

    /* Data was not consumed */
    return FALSE;
}

/*! \brief Send MBLK over logical channel.

    This function attempts to send a data block in a MBLK over the specified
    logical channel.  If the channel is in the open state then we send the
    data immediately otherwise we pass it into the state machine.

    Note: Pointer to MBLK is set to NULL if this function takes ownership of the
    MBLK, otherwise the caller has responsibility for free-ing the MBLK.
*/
void CID_DataWriteReq(CIDCB_T *cidcb, MBLK_T **mblk_ptr, context_t context)
{
#ifdef INSTALL_L2CAP_LECOC_CB
    if(CID_IsCBFlowControl(cidcb))
    {
        if(mblk_get_length(*mblk_ptr) > cidcb->remote_mtu)
        {
            /*
             * Soft fail by responding to upper layers with confirmation &
             * appropriate error code, if an attempt is made to voilate the
             * peer's MTU limit.
             *
             * ToDo: Appropriate reason code needs to be defined.
             */
            L2CA_DataWriteCfm(cidcb, 0, context, L2CA_DATA_LOCAL_ABORTED);
        }
        else
        {
            /* No support for SEGMENTATION as yet */
            CB_FLOW_DataWrite(cidcb, *mblk_ptr, context);
            *mblk_ptr = NULL;
        }
        return;
    }
#endif

    /* Basic mode */
    if(CH_DataTxBasic(cidcb, context, *mblk_ptr))
    {
        *mblk_ptr = NULL;
        return;
    }

    /* Reject the request */
    L2CA_DataWriteCfm(cidcb, 0, context, L2CA_DATA_NO_CONNECTION);
}


/*! \brief ACL reassembly error handling for received L2CAP packets.

     As per spec, MTU & SAR voilation for L2CAP packet received on LE COC are
     considered as fatal and expected to disconnect the LE COC.

    \param cidcb Pointer to the CIDCB_T structure.
    \param mblk The MBLK data corresponding to the L2CAP packet/frame.
    \param error_code L2CAP error/reason code.
*/
void CID_DataReadFatalError(CIDCB_T *cidcb,
                            MBLK_T *mblk,
                            l2ca_disc_result_t error_code)
{
    /*
     * The MTU/MPS/SAR voilation rules are explicitly mentioned for LE COC
     * Credit Based mode (Ref: Vol-3/Part-A/Section 3.4).
     * But for other L2CAP modes on BR/EDR (like Flow Control & Basic),
     * the specification hasn't explicitly stated as fatal.
     * Hence only for LE COC Credit Based mode, the L2CAP channel is destroyed
     * when these contraints are not met. Else for other L2CAP modes, silently
     * discard the L2CAP packets as done earlier.
     */
#ifdef INSTALL_L2CAP_LECOC_CB
    if(cidcb != NULL && CID_IsCBFlowControl(cidcb))
    {
        /* Disconnect L2CAP channel */
        CB_FLOW_FatalError(cidcb, mblk, error_code);
    }
    else
#endif
    {
        /* Flow Control or Basic modes. */
        if(mblk != NULL)
        {
            /* Silently ignore the L2CAP packet. */
            mblk_destroy(mblk);
        }
    }
}

