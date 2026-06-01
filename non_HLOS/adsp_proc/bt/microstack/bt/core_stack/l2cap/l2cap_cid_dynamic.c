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


/*! \brief Channel is going down, so do cleanup

    When a disconnect is received either locally or remotely, we must
    discard any pending signals, data and configuration to avoid rogue
    signals being sent upwards after the channel has died.
*/
void CID_DisconnectCleanup(CIDCB_T *cidcb)
{
    /* Only dynamic channels have signals and config buffers */
    if(!CID_IsFixed(cidcb))
    {
        /*
         * Currently LE dynamic channels don't involve any configuration
         * procedures and hence config cleanup is ignored.
         *
         * ToDo: In future, if LE dynamic channels has configuration
         * procedures similar to BR/EDR dynamic channels then we can
         * unconditionally remove the pending configuration similar to pending
         * signals.
         */
        /* Remove any pending signals (if any) */
        SIGH_SignalQueueEmptyWithCid(&(cidcb->chcb->signal_queue),
                                     cidcb->offload_cid);
        SIGH_SignalQueueEmptyWithCid(&(cidcb->chcb->signal_pending),
                                     cidcb->offload_cid);
    }

#ifdef INSTALL_L2CAP_LECOC_CB
    if(CID_IsCBFlowControl(cidcb))
    {
        CB_FLOW_Free(cidcb, L2CA_DATA_LINK_TERMINATED);
    }
#endif

}

