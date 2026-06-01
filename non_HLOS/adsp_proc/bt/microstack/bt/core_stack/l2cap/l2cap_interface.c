/*******************************************************************************

Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 2001

\brief  This file implements the interface to the upper layer.

        It handles receiving downstream primitives from the upper
        layer and routing the primitives to the correct channel
        instance.  It also includes helper functions to send upstream
        primitives.
*******************************************************************************/

#include "l2cap_private.h"

/*! \brief Send L2CA primitives to specified queue.
    \param queue_id ID of queue to put primitive on, if bit 15 set primitive is
                    put on "to host" queue.
    \param msg_id ID of message.
    \param prim Pointer to primitive.
*/
void L2CA_PutMessage(qid queue_id, uint16_t msg_id, void *prim)
{
    put_message(queue_id, msg_id, prim);
}

/*! \brief Handler function for L2CAP downstream primitives.

    This function is called whenever there are messages on the L2CAP
    Interface Queue.
*/
void L2CAP_InterfaceHandler(void **gash)
{
    CIDCB_T *cidcb;
    uint16_t mi;
    L2CA_UPRIM_T *l2cap_prim;
    PARAM_UNUSED(gash);

    /* Get message from interface queue */
    if (!get_message(L2CAP_IFACEQUEUE, &mi, (void **)&l2cap_prim))
        return;

    /* Check this is actually an L2CAP primitive */
    if (mi == L2CAP_PRIM)
    {
        /* Handle L2CAP downstream primitive */
        switch (l2cap_prim->type)
        {
#ifdef INSTALL_L2CAP_LECOC_CB
           case L2CA_ADD_CREDIT_REQ:
            {
                l2ca_cid_t cid = l2cap_prim->l2ca_add_credit_req.cid;

                /* Obtain CID instance */
                cidcb = CIDME_GetCidcb(cid);

                if (cidcb != NULL && CID_IsCBFlowControl(cidcb))
                {
                    uint16_t credits = l2cap_prim->l2ca_add_credit_req.credits;
                    uint16_t total_credit = cidcb->cb_flow->tot_rx_credits;

                    /* Check if the addition of new credits causes over flow */
                    if (CID_CheckCreditOverflow(total_credit, credits))
                    {
                        L2CA_AddCreditCfm(cidcb->p_handle, cid, cidcb->context,
                                            credits, L2CA_RESULT_FAILED);
                        break;
                    }

                    if(credits)
                    {
                        SIG_SIGNAL_T * sig_ptr;

                        /* ToDo:Decrease the credit if it passes the threshold*/
                        sig_ptr = SIG_SignalLEFlowControlCredit
                                                            (cidcb->offload_cid,
                                                             credits);
                        /* Send the signal */
                        SIGH_SignalSend(cidcb->chcb, sig_ptr);

                        cidcb->cb_flow->rx_credits += credits;
                        cidcb->cb_flow->tot_rx_credits += credits;
                        cidcb->cb_flow->app_rx_credits += credits;
                    }
                    else
                    {
                        /* Effective credits received from application */
                        credits = cidcb->cb_flow->tot_rx_credits;
                    }
                    L2CA_AddCreditCfm(cidcb->p_handle, cid, cidcb->context,
                                        credits, L2CA_RESULT_SUCCESS);
                }
            }
            break;
#endif
            case L2CA_DISCONNECT_REQ:
            {
                
            }
            break;

            case L2CA_DISCONNECT_RSP:
            {

            }
            break;

            case L2CA_DATAWRITE_REQ:
            {
                /* Obtain CID instance */
                cidcb = CIDME_GetCidcb(l2cap_prim->l2ca_datawrite_req.cid);
                if (cidcb != NULL)
                {
                    /* Pass to CID instance */
                    CID_DataWriteReq(cidcb,
                                     &l2cap_prim->l2ca_datawrite_req.data,
                                     l2cap_prim->l2ca_datawrite_req.req_ctx);
                }
            }
            break;

            case L2CA_DATAREAD_RSP:
            {
                /*
                 * Read ack from upper layer.
                 *
                 * Only makes sense in BR/EDR L2CAP 1.2 retransmission/flow
                 * control and LE Credit Based Flow control modes.
                 */
                cidcb = CIDME_GetCidcb(l2cap_prim->l2ca_dataread_rsp.cid);
                if(cidcb != NULL)
                {
#ifdef INSTALL_L2CAP_LECOC_CB
                    if(CID_IsCBFlowControl(cidcb))
                    {
                        CB_FLOW_DataReadAck(cidcb,
                                        l2cap_prim->l2ca_dataread_rsp.packets);
                    }
#endif
                }
            }
            break;

            default:
            {
                /* Received unknown primitive */
                BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
            }
            BLUESTACK_BREAK_IF_PANIC_RETURNS
        }

        /* Free the primitive and any other memory associated with primitive */
        L2CA_FreePrimitive(l2cap_prim);
    }
    else if (mi == GEN_INT_PRIM_ID(L2CAP_PRIM))
    {
        /* Handle L2CAP internal downstream primitive */
        switch (l2cap_prim->type)
        {
#ifdef INSTALL_SOCKET_OFFLOAD        
            case L2CA_INTERNAL_PASSTHROUGH_DATA_REQ:
            {
                L2CAINT_ProcessPassthroughDataReq((L2CA_INTERNAL_PASSTHROUGH_DATA_REQ_T *)l2cap_prim);
            }
            break;
#endif
            default:
            {
                /* Received unknown primitive */
                BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
            }
            BLUESTACK_BREAK_IF_PANIC_RETURNS
        }
        
        /* Free the memory associated with the internal primitive. */
        L2CA_FreeIntPrimitive(l2cap_prim);
    }
    else
    {
        /* Received unknown primitive */
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }
}


/*! \brief Wrapper function for sending L2CA_DISCONNECT_IND primitives to
           higher layer.

    Note: Handles all BR/EDR, AMP & LE transport.
*/
void L2CA_DisconnectInd(CIDCB_T *cidcb, uint8_t signal_id, l2ca_disc_result_t reason)
{
    L2CA_MAKE_PRIM(L2CA_DISCONNECT_IND, cidcb->p_handle);
    prim->cid = cidcb->local_cid;
    prim->reg_ctx = CIDME_RegistrationContext(cidcb);
    prim->con_ctx = cidcb->context;
    prim->identifier = signal_id;
    prim->reason = reason;

    L2CA_PrimSend(prim);
}


/*! \brief Set elements of given L2CA_DATAWRITE_CFM.
 
    \param cidcb Pointer to channel control block.
    \param context Opaque context from L2CA_DATAWRITE_REQ.
    \param length Length of data that has been sent.
    \param result Indication of success or failure.
    \param prim Pointer to allocation for primitive.
*/
void L2CA_BuildDataWriteCfm(CIDCB_T *cidcb,
                            context_t context,
                            uint16_t length,
                            l2ca_data_result_t result,
                            L2CA_DATAWRITE_CFM_T *prim)
{
    prim->type = L2CA_DATAWRITE_CFM;
    prim->phandle = CID_DataPhandle(cidcb);
    prim->cid = cidcb->local_cid;
    prim->reg_ctx = CIDME_RegistrationContext(cidcb);
    prim->req_ctx = context;
    prim->con_ctx = cidcb->context;
    prim->length = length;
    prim->result = result;
}

/*! \brief Wrapper function for sending L2CA_DATAWRITE_CFM primitives to
           higher layer.
*/
void L2CA_DataWriteCfm(CIDCB_T *cidcb, context_t context, uint16_t length, l2ca_data_result_t result)
{
    if(cidcb != NULL)
    {
        L2CA_DATAWRITE_CFM_T *prim = pnew(L2CA_DATAWRITE_CFM_T);
        L2CA_BuildDataWriteCfm(cidcb, context, length, result, prim);
        L2CA_PrimSend(prim);
    }
}

/*! \brief Wrapper function for sending L2CA_DATAREAD_IND primitives to
     higher layer.
*/
void L2CA_DataReadInd(const CIDCB_T *const cidcb,
                      MBLK_T *mblk_ptr,
                      l2ca_data_result_t result,
                      uint16_t packets,
                      L2CA_DATAREAD_IND_T **return_prim)
{
    L2CA_DATAREAD_IND_T *prim = pnew(L2CA_DATAREAD_IND_T);
    prim->type = L2CA_DATAREAD_IND;
    prim->phandle = CID_DataPhandle(cidcb),
    prim->cid = cidcb->local_cid;
    prim->reg_ctx = CIDME_RegistrationContext(cidcb);
    prim->con_ctx = cidcb->context;
    prim->data = mblk_ptr;
    prim->result = result;
    prim->length = 0;
    prim->packets = packets;

    if (return_prim == NULL)
        L2CA_PrimSend(prim);
    else
        *return_prim = prim;
}


/*! \brief Wrapper function for sending L2CA_ADD_CREDIT_CFM primitive to
           higher layer.
 */
#ifdef INSTALL_L2CAP_LECOC_CB
void L2CA_AddCreditCfm(phandle_t phandle, l2ca_cid_t cid, context_t context,
                        uint16_t credits, L2CA_RESULT_T result)
{
    L2CA_MAKE_PRIM(L2CA_ADD_CREDIT_CFM, phandle);
    prim->cid  = cid;
    prim->context = context;
    prim->credits = credits;
    prim->result = result;
    L2CA_PrimSend(prim);
}
#endif


