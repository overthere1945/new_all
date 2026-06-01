/*******************************************************************************

Copyright (C) 2007 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/

#include "l2cap_private.h"


CIDCB_T * CIDME_AllocCidCb(protocol_t protocol)
{
    l2ca_cid_t local_cid;
    CIDCB_T *cidcb=NULL;
    uint16_t index;

    for (index = 0; index < L2CAP_MAX_NUM_CIDS; index++)
    {
        if (mcb.cid_table[index] == NULL)
        {
            cidcb = zpnew(CIDCB_T);
            cidcb->priority    = L2CA_MICROSTACK_PRIORITY;
            local_cid = 0x8000 | (index << 2) | protocol;
            cidcb->local_cid = local_cid;
            mcb.cid_table[index] = cidcb;

            L2CAP_INFO("Generated CID 0x%x", local_cid);
            
            return cidcb;
        }
    }
    BLUESTACK_PANIC(PANIC_L2CAP_INVALID_CID);
    return cidcb;
}

/*! \brief Initialise basic members in a CIDCB structure

    This function initialises all common members in a CIDCB structure.
    Note that the alloc function zeros the structure, so there's no
    need to initialise variables to NULL/false/0.
*/
void CIDME_InitCidcb(CIDCB_T *cidcb,
                     L2CAP_CHCB_T *chcb,
                     l2ca_cid_t remote_cid,
                     struct psm_tag_t *psm_reg,
                     psm_t remote_psm,
                     context_t context)
{
    cidcb->remote_cid     = remote_cid;

    if ((cidcb->local_psm = psm_reg) != NULL)
        cidcb->p_handle = psm_reg->p_handle;

    cidcb->remote_psm     = remote_psm;
    cidcb->chcb           = chcb;
    cidcb->context        = context;
    cidcb->local_mtu      = L2CA_MTU_DEFAULT;
    cidcb->remote_mtu     = L2CA_MTU_DEFAULT;
    cidcb->local_flush_to = FLUSH_INFINITE_TIMEOUT;
    cidcb->state          = CID_ST_NULL;
}



/*! \brief Free CID instance

    This function frees up a CID instance. Caller must make sure that
    any queued data blocks or signals have been freed before hand.
*/
void CIDME_FreeCidcb(CIDCB_T *cidcb)
{
    L2CAP_CHCB_T *chcb_ptr;
    CIDCB_T *cidcb_ptr;
    CIDCB_T **cidcb_ptr_ptr;
    uint16_t index;
    
    if ((chcb_ptr = cidcb->chcb) != NULL)
    {
        /* Remove CIDCB from list */
        for (cidcb_ptr_ptr = &chcb_ptr->cidcb_list; 
             (cidcb_ptr = *cidcb_ptr_ptr) != NULL; 
             cidcb_ptr_ptr = &cidcb_ptr->next_ptr)
        {
            /* Check if we have found instance to remove */
            if (cidcb_ptr == cidcb)
            {
                /* Remove instance from list and exit loop */
                *cidcb_ptr_ptr = cidcb_ptr->next_ptr;
                break;
            }
        }
    }

#ifdef INSTALL_L2CAP_LECOC_CB
    if(CID_IsCBFlowControl(cidcb))
        CB_FLOW_Free(cidcb, L2CA_DATA_SILENT);
#endif

    for (index = 0; index < L2CAP_MAX_NUM_CIDS; index++)
    {
        if (mcb.cid_table[index] == cidcb)
        {
            mcb.cid_table[index] = NULL;
        }
    }
    L2CAP_INFO("Free CID 0x%x", cidcb->local_cid);
    pfree(cidcb);
}

/*! \brief Get CID instance via the local CID

    Search the list of CIDs and return the one with the matching local
    CID number.
*/
CIDCB_T *CIDME_GetCidcb(l2ca_cid_t cid)
{
    uint16_t index;

    for (index = 0; index < L2CAP_MAX_NUM_CIDS; index++)
    {
        if ((mcb.cid_table[index] != NULL) && 
            (cid == mcb.cid_table[index]->local_cid))
        {
            return mcb.cid_table[index];
        }
    }

    /* Match not found or CID out of range */
    return NULL;
}


/*! \brief Get CID instance via the CHCB handle
  
    Obtain CID instance via local CID and connection handle.
*/
struct cidtag *CIDME_GetCidcbWithHandle(const L2CAP_CHCB_T *chcb, l2ca_cid_t cid)
{
    CIDCB_T *cidcb;

    for (cidcb = chcb->cidcb_list; cidcb != NULL; cidcb = cidcb->next_ptr)
    {
        if (cidcb->offload_cid == cid)
        {
            return cidcb;
        }
    }

    /* Match not found or CID out of range */
    return NULL;
}

/*! \brief Get CID instance from CHCB list with remote CID

    Obtain CID instance via remote CID and connection handle.

    \param chcb pointer to chcb structure.
    \param remote_cid remote cid to find cid structure
    \return pointer to cid structure if found
*/
struct cidtag *CIDME_GetCidcbRemoteWithHandle(const L2CAP_CHCB_T *chcb, 
                                              l2ca_cid_t remote_cid)
{
    CIDCB_T *cidcb;

    for (cidcb = chcb->cidcb_list; cidcb != NULL; cidcb = cidcb->next_ptr)
    {
        if (cidcb->remote_cid == remote_cid)
            break;
    }

    return cidcb;
}


context_t CIDME_RegistrationContext(const CIDCB_T *cidcb)
{
    if (cidcb == NULL || cidcb->local_psm == NULL)
        return 0;

    return cidcb->local_psm->reg_ctx;
}


