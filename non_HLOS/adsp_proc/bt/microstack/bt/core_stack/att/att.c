/*******************************************************************************

Copyright (C) 2009 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "att_private.h"

#ifdef INSTALL_ATT_MODULE

#ifdef ATT_GLOBAL_STATE
ATT_T att;
#endif /* ATT_GLOBAL_STATE */

/* Bluetooth Base UUID 00000000-0000-1000-8000-00805f9b34fb */
/* this could be shared with SDP */
const ATT_UUID_T att_base_uuid = {
    {
        0xffff0000U, /* ~UUID16, this is used as a mask */
        0x00001000U,
        0x80000080U,
        0x5f9b34fbU
    }
};



/* TODO: do not enable L2CAP channels until ATT_REGISTER_REQ is called. */
void att_init(void **gash)
{
#ifdef ATT_GLOBAL_STATE
    QBL_UNUSED(gash);
    memset(&att, 0, sizeof(ATT_T));
#else /* ATT_GLOBAL_STATE */
    ATT_T *att = zpmalloc(sizeof(ATT_T));

    *gash = (void*)att;
    memset(att, 0, sizeof(ATT_T));
#endif /* !ATT_GLOBAL_STATE */

}

/* Shutdown function for Synergy */
#ifdef ENABLE_SHUTDOWN
void att_deinit(void **gash)
{
    uint16_t type;
    void *msg;
#ifndef ATT_GLOBAL_STATE
    ATT_T *att = (ATT_T*)*gash;
#else
    QBL_UNUSED(gash);
#endif /* !ATT_GLOBAL_STATE */

    /* Free queue messages from VM */
    while(get_message(ATT_IFACEQUEUE, &type, &msg))
    {
        switch (type)
        {
            case L2CAP_PRIM:
                L2CA_FreePrimitive((L2CA_UPRIM_T*)msg);
                break;

            case ATT_PRIM:
                attlib_free((ATT_UPRIM_T*)msg);
                break;

            default:
                BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
                BLUESTACK_BREAK_IF_PANIC_RETURNS
        }
    }

    /* free all existing connections */
    while (ATT_STATE(conn))
    {
        ATT_STATE(conn)->pend = PEND_NONE;
        att_conn_rm(ATT_ARG ATT_STATE(conn)->cid);
    }

    /* free the database */
    att_attr_remove(ATT_ARG 1, 0xffff);
    /* free the main instance */
#ifndef ATT_GLOBAL_STATE
    pfree(att);
#endif
}
#endif /* ENABLE_SHUTDOWN */

#if ATT_TIMEOUT
/*! \brief Starts ATT timer for a specific role for a given connection
    \param conn  ATT connection structure
    \param isclient  ATT role for which timer should be started,
                     either client or server
    \returns None
*/
void att_timer_start(att_conn_t *conn, bool_t isclient)
{
    tid * timer = (isclient ? (&conn->client_timer) : (&conn->server_timer));

    timer_start(timer, ATT_TIMEOUT * SECOND,
                    att_timer_expired, (uint16_t)isclient, conn);
}

/*! \brief call back function invoked on timer expiry to ATT transaction 
    \param mi  ATT role information
    \param mv  ATT connection structure
    \returns None
*/
void att_timer_expired(uint16_t mi, void *mv)
{
    bool_t isclient = (bool_t)mi;
    att_conn_t *conn = (att_conn_t*)mv;
    
    if(isclient)
    {
        TIMER_EXPIRED(conn->client_timer);
        att_timer_cancel(&conn->server_timer);
    }
    else
    {
        TIMER_EXPIRED(conn->server_timer);
        att_timer_cancel(&conn->client_timer);
    }

#ifdef GATT_OFFLOAD
    att_conn_clean(ATT_ARG conn);
    PEND_SET_RSP(PEND_NONE);    
    att_error_ind(ATT_STATE(phandle), conn->cid, ATT_ERROR_RESPONSE_TIMEOUT);
#else
    /* Just disconnect, and let disconnect ind report error */
    att_handle_protocol_violation(conn);
#endif
}
#endif /* ATT_TIMEOUT */

/*! \brief Sets pending variable to indicate remote device's pending response. 
           Pending variable will be updated for outgoing Request and Indication 
           along with timer trigger
    \param conn  ATT connection structure
    \param op  opcode of the outgoing data
    \returns None.
*/
void att_pend_set(att_conn_t *conn, uint8_t op)
{
    /* client -> server PDUs 
     * Trigger timer and set pending for only outgoing Requests */
    if (!(op & ATT_OP_RSP_MASK))
    {
        /* commands do not use flow control */
        if (op & ATT_OP_CMD_FLAG)
        {
            return;
        }
        /* handle value confirmation is actually not a request */
        else if (op == ATT_OP_HANDLE_VALUE_CFM)
        {
            /* reset handle value indication lock */
            conn->pend &= ~PEND_HANDLE_RSP;
            return;
        }

        /* set the response we are waiting for */
        PEND_SET_RSP(op | ATT_OP_RSP_MASK);

        if (conn->cid != ATT_CID_LOCAL)
            att_timer_start(conn, TRUE);
    }

    /* server -> client PDUs
     * Trigger timer and set pending for only outgoing Indications */
    else if (op == ATT_OP_HANDLE_VALUE_IND)
    {
        /* set waiting for confirmation flag */
        conn->pend |= PEND_HANDLE_CFM;
    
        /* start timer */
        if (conn->cid != ATT_CID_LOCAL)
            att_timer_start(conn, FALSE);
    }
}

/*! \brief Checks whether the received PDU is allowed or not, if it is allowed 
           and PDU is a response/confirmation then resets pending variable bit 
           that was set when transaction was triggered. Similarly upon 
           transaction complete timer is cancelled.
    \param conn  ATT connection structure
    \param op  opcode of the received data
    \returns None.
*/
bool_t att_pend_check(att_conn_t *conn, uint8_t op)
{
    /* Client -> Server PDUs
     * Requests, Commands and Handle Value Confirmations
     */
    if (!(op & ATT_OP_RSP_MASK))
    {
        /* commands are always allowed */
        if (op & ATT_OP_CMD_FLAG)
        {
            /* nop */
        }

        /* handle value confirmation is allowed only after we sent ind */
        else if (op == ATT_OP_HANDLE_VALUE_CFM &&
                 conn->pend & PEND_HANDLE_CFM)
        {
            /* reset waiting flag */
            conn->pend &= ~PEND_HANDLE_CFM;

            if (conn->cid != ATT_CID_LOCAL)
                att_timer_cancel(&conn->server_timer);
        }
        
        /* we are already processing a request */
        else if (op == ATT_OP_HANDLE_VALUE_CFM ||
                 PEND_GET_REQ())
        {
            att_handle_protocol_violation(conn);
            return FALSE;
        }
    }    
    /* Server -> Client PDUs 
     * Response, Indication and Notification
     */
    else 
    {
        /* response to pending request */
        if (op == PEND_GET_RSP())
        {
            PEND_SET_RSP(PEND_NONE);

            if (conn->cid != ATT_CID_LOCAL)
                att_timer_cancel(&conn->client_timer);
        }
        
        /* handle value indication, allow only one at a time */
        else if (op == ATT_OP_HANDLE_VALUE_IND &&
                 !(conn->pend & PEND_HANDLE_RSP))
        {
            /* set waiting for app response flag */
            conn->pend |= PEND_HANDLE_RSP;
        }
        
        /* notification is always allowed and has no timer */
        else if ((op == ATT_OP_HANDLE_VALUE_NOT) ||
                 (op == ATT_OP_MULTI_HANDLE_VALUE_NOT))
        {
            /* nop */
        }
        
        /* error response is checked later based on original opcode */
        else if (op != ATT_OP_ERROR_RSP)
        {
            /* something that we were not supposed to get */
            att_handle_protocol_violation(conn);
            return FALSE;
        }
    }
    
    /* Operation is allowed */
    return TRUE;
}

/*! \brief Send ATT primitives to specified queue.
    \param queue_id ID of queue to put primitive on, if bit 15 set primitive is
           put on "to host" queue.
    \param msg_id ID of message.
    \param prim Pointer to primitive.
*/
void att_send_message(qid queue_id, uint16_t msg_id, void *prim)
{
    put_message(queue_id, msg_id, prim);
}

/*! \brief Handle ATT protocol violation.
     Note: One can extend this function accept severity as parameter to decide 
     action.
    \param conn  ATT connection structure
    \returns None.
*/
void att_handle_protocol_violation(att_conn_t* conn)
{
    ATT_LOG_DEBUG("ATT Protocol violation");

#ifdef GATT_OFFLOAD
    att_error_ind(ATT_STATE(phandle), conn->cid, ATT_ERROR_PROTOCOL_VIOLATION);
#else
    /* Disconnect the link,that is the best option that we have right now */
    att_disconnect(conn);
#endif
}


uint16_t att_conn_in_buf_len (att_in_t *in_buf)
{
    return (uint16_t)(in_buf->len - (in_buf->data - in_buf->buf));
}

void att_pad_uuid_32_to_128(ATT_UUID_T *uuid)
{
    qbl_memscpy(&uuid->n[1], 3*sizeof(uint32_t), &att_base_uuid.n[1], 3*sizeof(uint32_t));
}

bool_t att_uuid128_eq(const ATT_UUID_T *uuid1, const ATT_UUID_T *uuid2)
{
    return uuid1->n[0] == uuid2->n[0] && uuid1->n[1] == uuid2->n[1] &&
        uuid1->n[2] == uuid2->n[2] && uuid1->n[3] == uuid2->n[3];
}

bool_t att_array_is_uuid16(const uint32_t u[4])
{
    return (u[0] & att_base_uuid.n[0]) == 0 &&
        u[1] == att_base_uuid.n[1] &&
        u[2] == att_base_uuid.n[2] &&
        u[3] == att_base_uuid.n[3];
}

bool_t att_stream_pdu_exceeds_mtu(att_conn_t *conn, MBLK_T *mblk)
{
    return ((ATT_OPCODEN_LEN + ATT_HANDLE_LEN + mblk_get_length(mblk)) >
                conn->mtu);
}

#endif /* INSTALL_ATT_MODULE */
