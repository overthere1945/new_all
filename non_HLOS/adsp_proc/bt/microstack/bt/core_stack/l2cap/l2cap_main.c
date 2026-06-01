/*******************************************************************************

Copyright (C) 2008 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2cap_private.h"

/*! \brief Initialise L2CAP.

    This function is called to initialise L2CAP.
*/
void L2CAP_Init(void **gash)
{
    PARAM_UNUSED(gash);
    /* Initialise L2CAP control module */
    MCB_Init();
}


#ifdef ENABLE_SHUTDOWN

/*! \brief CIDCB kill function

    Helper function to silently kill any CIDCB without notifying
    anyone about it.
*/
static void L2CA_KillCidcb(CIDCB_T *cidcb)
{
    /* be silent */
    CH_FlushCidcbPendingTxDataQueue(cidcb->chcb, cidcb, L2CA_DATA_SILENT); 
    CH_FlushCidcbTxSignalQueue(cidcb->chcb, cidcb);

    CIDME_FreeCidcb(cidcb);
    cidcb = NULL;
}

/*! \brief Called from the ACL Manager while it is deinitialising.
 
    L2CAP must clean up the chcb before the ACL Manager can free it.
    Free the dynamic members of the CHCB. The pointer itself will be
    freed in the DM.

    \param chcb Pointer to connection handle control block.
*/
void dm_acl_client_deinit_l2cap(L2CAP_CHCB_T *chcb)
{
    CIDCB_T *cidcb;

    /* Free list of CIDCBs */
    while(chcb->cidcb_list != NULL)
    {
        cidcb = chcb->cidcb_list->next_ptr;
        L2CA_KillCidcb(chcb->cidcb_list);
        chcb->cidcb_list = cidcb;
    }

    /* Flush the remaining bits using the common function */
    (void)CH_FlushChcb(chcb, L2CA_DATA_SILENT); /* be silent */
}

/*! \brief L2CAP deinitialisation

    The is the L2CAP deinit function that gets called by the scheduler
    when the L2CAP task must shut down. Note that the freeing of CHCBs
    and related bits take place in the dm_acl_client_deinit_l2cap().
*/
void L2CAP_Deinit(void **gash)
{
    uint16_t mi;
    L2CA_UPRIM_T *l2cap_prim;

    PARAM_UNUSED(gash);

    /* Free scheduler queue */
    while(get_message(L2CAP_IFACEQUEUE, &mi, (void**)&l2cap_prim))
    {
        if(mi == L2CAP_PRIM)
        {
            L2CA_FreePrimitive(l2cap_prim);
        }
        else
        {
            BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
        }
    }    
}

#endif /* ENABLE_SHUTDOWN */
