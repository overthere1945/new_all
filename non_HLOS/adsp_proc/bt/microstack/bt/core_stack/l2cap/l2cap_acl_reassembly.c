/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\brief  Low-level L2CAP ACL packet reassembly. This function is invoked
        directly from hci-top. Once an ACL packet has been fully
        assembled, it's injected onto the DM HCI interface queue.

        Header file is l2cap_con_handle.h
*******************************************************************************/

#include "l2cap_private.h"

#ifdef BUILD_FOR_HOST

/*! \brief ACL reassembly

    For host builds we assemble ACL packets in L2CAP itself. This lets
    us do early sanity checks against the configured MTU. The function
    is called by the DM HCI handler. Once a packet is complete, we
    simply return it. If non-complete, return NULL. The DM is
    responsible for passing the assembled packet into L2CAP.

    Note:
    Any code modifications to this function should be reflected in 
    sdm_l2cap_acl_reassemble() as well.
*/
MBLK_T *L2CA_AclReassemble(hci_connection_handle_t hci_flags,
                           MBLK_T *mblk)
{
    hci_connection_handle_t handle;
    mblk_size_t size;
    L2CAP_CHCB_T *chcb;
    bool_t is_start;

    /* Extract handle and size */
    size = mblk_get_length(mblk);
    handle = hci_flags & HCI_CONNECTION_HANDLE_MASK;

    /* Obtain CHCB handle */
    chcb = CHME_GetConHandle(handle);
    if(chcb == NULL)
    {
        /* Bogus handle */
        BLUESTACK_WARNING(CON_HANDLE_ERR);
        mblk_destroy(mblk);
        return NULL;
    }

    /* If not a continuation, it's a start/start-flushable */
    is_start = (hci_flags & HCI_PKT_BOUNDARY_MASK) != HCI_PKT_BOUNDARY_FLAG_CONT;

    /* We have received a start packet while we are still processing the 
     * previous PDU. */
    if(is_start && (chcb->reassem.mblk != NULL))
    {
        /* Last packet was not reasembled correctly. Discard it. */
        BLUESTACK_WARNING(CON_HANDLE_ERR);
        mblk_destroy(chcb->reassem.mblk);
        chcb->reassem.mblk = NULL;
        chcb->reassem.offset = 0;
    }

    /* If we have received lesser data than the size of L2CAP_SIZEOF_CID_HEADER 
     * then collect the data into the header_data array */
    if((size + chcb->reassem.offset) < L2CAP_SIZEOF_CID_HEADER)
    {
        if(chcb->reassem.mblk == NULL)
        {
            if(is_start)
            {
                chcb->reassem.offset = 0;
                chcb->reassem.mblk = mblk;
            }
            else
            {
                /* Continuation packets received out of order */
                BLUESTACK_WARNING(CON_HANDLE_ERR);
                mblk_destroy(mblk);
                return NULL;
            }
        }
        else
        {
            chcb->reassem.mblk = mblk_add_tail(mblk, chcb->reassem.mblk);
        }
        chcb->reassem.offset += size;
        return NULL;
    }
    /* Check if the existing data and new-data-set forms the L2CAP header */
    else if ((chcb->reassem.offset < L2CAP_SIZEOF_CID_HEADER) &&
          (chcb->reassem.mblk != NULL))
    {
        mblk = mblk_add_tail(mblk, chcb->reassem.mblk);
        chcb->reassem.mblk = NULL;
        is_start = TRUE;

        /* Get the size of newly formed MBLK chain */
        size = size + chcb->reassem.offset;
        chcb->reassem.offset = 0;
        /* Continue processing the MBLK data as earlier */
    }

    /* If the received PDU is a start fragment and 
       (size > L2CAP_SIZEOF_CID_HEADER) or smaller fragments were reassembled 
       for the initial processing.
    */
    if(is_start)
    {
        CIDCB_T *cidcb;
        uint16_t cid;
        uint16_t len;
        uint16_t mtu;
        uint8_t *map;

        /* Extract CID and length from packet - we need to map it in
         * first */
        map = mblk_map(mblk, 0, L2CAP_SIZEOF_CID_HEADER);
        if(map == NULL)
        {
            /* Error in mapping */
            BLUESTACK_WARNING(CON_HANDLE_ERR);
            mblk_destroy(mblk);
            return NULL;
        }
        len = UINT16_R(map, L2CAP_PKT_POS_LENGTH);
        cid = UINT16_R(map, L2CAP_PKT_POS_CID);
        mblk_unmap(mblk, map);

        /* If we have a CIDCB, get the MTU for this channel */
        cidcb = CIDME_GetCidcbWithHandle(chcb, cid);
        mtu = (uint16_t)((cidcb != NULL) ? cidcb->local_mtu : L2CA_MTU_DEFAULT);
        
        /* Make sure MTU isn't exceeded. Note that more fine grained
         * checks will be performed at a later stage, i.e. once we
         * know how to decode the header(s) based on the channel
         * configuration */
        if((len > (mtu + L2CAP_ACL_MTU_MAX)) ||       /* checks header 'length' field */
           (size > (mtu + L2CAP_ACL_RAW_LENGTH_MAX))) /* checks actual data size */
        {
            BLUESTACK_WARNING(CON_HANDLE_ERR);
            CID_DataReadFatalError(cidcb, mblk, L2CA_DISCONNECT_MTU_VIOLATION);
            return NULL;
        }
        
        /* Packet OK. Append it to the reassembly */
        chcb->reassem.length = len;
        chcb->reassem.offset = size;
        chcb->reassem.mblk = mblk;
        chcb->reassem.local_cid = cid;
    }
    /* Continuation - make sure we already have a reassembly buffer */
    else if(chcb->reassem.mblk != NULL)
    {
        /* Drop everything if this ACL packet will make us exceed the
         * actual L2CAP packet size */
        if((size + chcb->reassem.offset) > (chcb->reassem.length + L2CAP_SIZEOF_CID_HEADER))
        {
            CIDCB_T *cidcb;

            BLUESTACK_WARNING(CON_HANDLE_ERR);
            cidcb = CIDME_GetCidcbWithHandle(chcb, chcb->reassem.local_cid);
            CID_DataReadFatalError(cidcb, mblk, L2CA_DISCONNECT_SAR_VIOLATION);
            chcb->reassem.local_cid = L2CA_CID_INVALID;
            mblk_destroy(chcb->reassem.mblk);
            chcb->reassem.mblk = NULL;
            chcb->reassem.offset = 0;
            return NULL;
        }

        /* Continuation seems to be OK. Append it */
        chcb->reassem.mblk = mblk_add_tail(mblk, chcb->reassem.mblk);
        chcb->reassem.offset += size;
    }
    else
    {
        /* Continuation packets received out of order */
        BLUESTACK_WARNING(CON_HANDLE_ERR);
        mblk_destroy(mblk);
        return NULL;
    }

    /* Check for completion */
    if(chcb->reassem.offset == (chcb->reassem.length + L2CAP_SIZEOF_CID_HEADER))
    {
        chcb->reassem.local_cid = L2CA_CID_INVALID;
        /* Return the completed MBLK and reset buffer */
        mblk = chcb->reassem.mblk;
        chcb->reassem.mblk = NULL;
        chcb->reassem.offset = 0;
        return mblk;
    }
    else
    {
        /* Nothing ready this time */
        return NULL;
    }
}

#endif /* BUILD_FOR_HOST */
