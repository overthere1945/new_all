/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 2001

*******************************************************************************/

#include "l2cap_private.h"
 
/*! \brief Create signal

    This function creates a new signal block including the data block.
    A signal comprises of two memory block, the first is a control block used
    for management of the signal in the L2CAP layer, the other block is the
    data block, this contain the actual payload that is sent over the ACL.

    \param local_cid Local CID that this signal is associated with.
    \param signal_size Size of signal data.
    \param signal_type Signal type.
    \param signal_id ID of this signal.
    \param count Number of subsequent uint16 arguments to be added to signal.
    \return Pointer to signal.
*/
SIG_SIGNAL_T *SIG_SignalCreate(l2ca_cid_t local_cid,
                               uint16_t signal_size,
                               uint8_t signal_type,
                               l2ca_identifier_t signal_id,
                               unsigned int count,
                               ...)
{
    SIG_SIGNAL_T *sig = zpnew(SIG_SIGNAL_T);
    uint8_t *data;
    va_list ap;

    /* We might consider allowing this to fail softly. But if we don't have
       enough memory to create a signal then the game is probably up. */
    if ((sig->signal = mblk_malloc_create((void**)&data,
            (mblk_size_t)(L2CAP_SIZEOF_SIGNAL_HEADER + signal_size))) == NULL)
        BLUESTACK_PANIC(PANIC_HEAP_EXHAUSTION);

    memset(data, 0, L2CAP_SIZEOF_SIGNAL_HEADER + signal_size);

    sig->signal_type = signal_type;
    sig->signal_id = signal_id;

    /*
     * SIGNAL_COMMAND_REJECT & SIGNAL_LE_FLOW_CONTROL_CREDIT has zeroed timer
     * counts. SIGNAL_LE_FLOW_CONTROL_CREDIT doesn't have a response and hence
     * don't attempt to retransmit.
     * All other signals take the defaults.
     */
    if (signal_type != SIGNAL_COMMAND_REJECT
#if defined(INSTALL_ULP) && defined(INSTALL_L2CAP_LECOC_CB)
    && signal_type != SIGNAL_LE_FLOW_CONTROL_CREDIT
#endif
    )
    {
        sig->rtx_timer_count = L2CAP_RTX_RETRIES;
        sig->ertx_timer_count = L2CAP_ERTX_RETRIES;
    }
    sig->local_cid = local_cid;
    sig->p_handle = INVALID_PHANDLE;

    write_uints(&data,
                URW_FORMAT(uint8_t, 2, uint16_t, 1, UNDEFINED, 0),
                signal_type,
                signal_id,
                signal_size);

    va_start(ap, count);

    while (count-- != 0)
        write_uint16(&data, (uint16_t)va_arg(ap, unsigned int));

    va_end(ap);

    return sig;
}


#if defined(INSTALL_ULP) && defined(INSTALL_L2CAP_LECOC_CB)
/*! \brief Creates signal to increase the number of credit extended to the 
           remote device for the local channel. 
    \param local_cid local cid of the channel for which credits are being extended
    \param credits number of credits 
    \return Pointer to signal control block or NULL if creation failed.
*/
SIG_SIGNAL_T *SIG_SignalLEFlowControlCredit(l2ca_cid_t local_cid,
                                            uint16_t credits)
{
    return  SIG_SignalCreate(local_cid, 
                             SIGNAL_LE_FLOW_CONTROL_CREDIT_MIN_LENGTH,
                             SIGNAL_LE_FLOW_CONTROL_CREDIT,
                             0, 2, local_cid, credits);
}

#endif /* defined(INSTALL_ULP) && defined(INSTALL_L2CAP_LECOC_CB) */

