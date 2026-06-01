/*******************************************************************************

Copyright (C) 2009 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "att_private.h"


#include INC_DIR(optim,optim.h)

#ifdef BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE
static void att_cmd_after_delay(uint16_t cid, void *ptr);
#endif /* BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE */

#ifdef INSTALL_ATT_MODULE

/****************************************************************************
NAME
    is_op_req - check type of PDU

*/
static bool is_op_req(uint8_t op)
{

    return (ATT_IS_SERVER_OPERATION(op) &&
            op != ATT_OP_HANDLE_VALUE_CFM &&
            !ATT_IS_OP_CMD(op));

}
/****************************************************************************
NAME
    check_auth - check client authentication

*/
att_result_t check_auth(ATT_FUNC_STATE att_conn_t *conn, att_attr_t *attr, uint16_t op)
{

    return ATT_RESULT_SUCCESS;
}

#ifdef BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE
/*
 * This fix is added for B-108559 for synergy only.
 * There is race between encryption change indication and
 * ATT_ command from other side. There is actually a encryption change and ATT_
 * command is expecting that change.But encryption change indication is received
 * after the ATT command causing the ATT command to fail.
 *
 * This is a bug in FW B-108465. But for temporary purpose,
 * it is being fixed in Bluestack.
 */
static void att_cmd_after_delay(uint16_t cid, void *ptr)
{
    att_conn_t *conn;

#ifndef ATT_GLOBAL_STATE
    struct att_state_tag * att = (struct att_state_tag *)ptr;
#endif /* ATT_GLOBAL_STATE */

    if ((conn = att_conn_find(ATT_ARG cid)) == NULL ||
        conn->handler == NULL)
    {
        /* silently ignore */
        return;
    }

    /* Encryption state might have moved to encrypted now, trigger pdu processing */
    att_check_pending_queue(ATT_ARG conn);
}
#endif /* BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE */



/****************************************************************************
NAME
    att_check_attr_length_in_wq - Check whether the length of attr's in the 
    write queue are within defined limits if they are fixed length attr's
    if length is invalid return handle value in err_handle
*/
static att_result_t att_check_attr_length_in_wq(ATT_FUNC_STATE att_conn_t *conn, uint16_t *err_handle)
{
    att_writeq_t *wq, *wq_node;
    uint16_t handle = 0xffff;
    att_attr_t *attr = NULL;
    att_result_t rc = ATT_RESULT_SUCCESS;
    uint32_t max_length = 0;
    uint16_t DONE_MASK = 0x8000; /* Safe as attr len will never be >= hex(8000) */
    bool_t invalid_offset = FALSE;

    for (wq = conn->common->wq; 
         wq; 
         max_length = 0, handle = 0xffff, wq = att_wq_next(conn, wq))
    {
        if ((rc == ATT_RESULT_SUCCESS) && (wq->handle < handle))
            attr = att_attr_find(ATT_ARG wq->handle, &handle);

        /* Ensure we are not on a flagged-node */
        if(attr && !(wq->len & DONE_MASK))
        {
            for (wq_node = wq; 
                 wq_node && (rc == ATT_RESULT_SUCCESS); 
                 wq_node = att_wq_next(conn, wq_node))
            {
                /* Check that we are on a matching node of the write queue */
                if (handle == wq_node->handle)
                {
                    if(wq_node->offs > att_attr_size(attr))
                       invalid_offset = TRUE;

                    /* We found the handle here, tap the max length*/
                    if(((uint32_t)(wq_node->offs + wq_node->len)) > max_length)
                        max_length = wq_node->offs + wq_node->len;

                    /* Flag this node as done */
                    wq_node->len |= DONE_MASK;
                }
            }
        }

        /* Verify that max_length is within limits */
        if(attr
          && !(att_attr_raw_flags(attr) & ATT_ATTR_DYNLEN)
          && max_length)
        {
             if (invalid_offset)
             {
                rc = ATT_RESULT_INVALID_OFFSET;
                *err_handle = handle;
             }
             else if (max_length > att_attr_size(attr))
             {        
                rc = ATT_RESULT_INVALID_LENGTH;
                *err_handle = handle;
             }
        }
        
        /* Ensure that this flag is removed by the time we exit the loop */
        wq->len &= ~DONE_MASK;
    }

    return rc;
}

/****************************************************************************
NAME
    att_is_access_permitted - Check whether the access to the 
    attribute is permitted on a certain radio (LE/BREDR)
*/
#ifndef ATT_FLAT_DB_SUPPORT
static bool_t att_is_access_permitted(att_conn_t *conn, att_attr_t *attr)
{
    bool_t rc = TRUE;

#ifdef INSTALL_ATT_BREDR
    uint16_t flags = att_attr_flags(attr);
#else
    QBL_UNUSED(attr);
#endif /* INSTALL_ATT_BREDR */


    if (conn->cid == ATT_CID_LOCAL)
    {  
        /* On a local channel there is check
         *  not required for radio type.
         */
        return rc;
    }

     return rc;
}
#endif /* !ATT_FLAT_DB_SUPPORT */


/****************************************************************************
NAME
    att_check_app_authorized - Check whether the specific  handle for an attribue
    is already authorized by the app.
*/
static bool_t att_is_app_authorized(att_conn_t *conn,uint16_t handle)
{
    att_writeq_t *wq;

    for ( wq = conn->common->wq;
          wq; 
          wq = att_wq_next(conn, wq))
    {
        if (wq->handle == handle)
            return TRUE;
    }
    return FALSE;
}

/*
 * Requests from the client
 ****************************************************************************/

/****************************************************************************
NAME
    add_read_by_common_attr - add attribute data to common read by uuid
                              response
*/
static bool_t add_read_by_common_attr(att_conn_t *conn,
                                      uint16_t handle,
                                      uint16_t end,
                                      att_attr_t **attrp,
                                      ATT_ACCESS_RSP_T **irqp,
                                      uint16_t *status)
{
    att_attr_t *attr = *attrp;
    ATT_ACCESS_RSP_T *irq = *irqp;
    uint16_t size_value;
    uint16_t hdrlen;
    uint16_t len = 0;
    uint16_t maxlen = 0;
    uint8_t op = GET_SERVER_IN_OP(conn);

    *status = ATT_RESULT_SUCCESS;

    /* get data length */
    if (irq)
    {
        size_value = irq->size_value;
    }
    else
    {
#ifndef ATT_FLAT_DB_SUPPORT
        size_value = att_attr_size(attr);
#else /* ATT_FLAT_DB_SUPPORT */
        size_value = att_act_attr_size(attr);
#endif /* !ATT_FLAT_DB_SUPPORT */
    }

    /* add header length */
    switch (op)
    {
        case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
            hdrlen = 4; /* handle(2) + end(2) */
            size_value = 0;
            len = hdrlen;
            break;
            
        case ATT_OP_READ_BY_TYPE_REQ:
            hdrlen = 2; /* handle(2) */
            end = 0; /* no end handle available */
            break;
            
        case ATT_OP_READ_BY_GROUP_TYPE_REQ:
            hdrlen = 4; /* handle(2) + end(2) */
            break;
            
        default:
            return FALSE;
    }
    
    /* create the response packet */
    if (!conn->out.data)
    {
        /* maximum length is 8 bits, so the attribute may get a saturated value
         * in case of a massive MTU 
         */
        maxlen = att_pkt_create(conn, (uint8_t)(op | ATT_OP_RSP_MASK));

        if(ATT_PKT_CREATE_FAILED == maxlen)
        {
            /* 
             * Return indicating that it is time to respond 
             * and prepare to respond error.
             */
            *status = ATT_PKT_CREATE_FAILED;
            return FALSE;
        }
        else
        {
            maxlen = MIN(maxlen, 0xff);
        }

        if (!len)
        {
            len = MIN(maxlen - 1, hdrlen + size_value);
            write_uint8(&conn->out.data, (uint8_t)len);
        }
    }

    /* get the maximum data length from the packet */
    if (!len)
    {
        len = conn->out.buf[1];

        /* check length */
        if (len != hdrlen + size_value)
        {
            return 0;
        }
    }

    /* add handle */
    write_uint16(&conn->out.data, handle);

    /* write end handle */
    if (end)
    {
        write_uint16(&conn->out.data, end);
    }

    /* write the data */
    if (irq)
    {
        att_pkt_write(conn, irq->value, (uint8_t)(len - hdrlen));
    }
    else
    {
#ifdef ATT_FLAT_DB_SUPPORT
        att_attr_read(&conn->out.data, attr, 0, (uint8_t)(len - hdrlen), NULL);
#else /* !ATT_FLAT_DB_SUPPORT */
        att_attr_read(&conn->out.data, attr, 0, (uint8_t)(len - hdrlen));
#endif /* ATT_FLAT_DB_SUPPORT */
    }

    /* reset pointers */
    *irqp = NULL;
    *attrp = NULL;
    
    /* room for more? */
    return (conn->mtu - (conn->out.data - conn->out.buf)) >= len;
}

/****************************************************************************
NAME
    att_handle_read_by_common_req - handle ATT requests from client

    This function takes care of the following ATT requests:
    - Find By Type Value request
    - Read By Type request
    - Read By Group Type request
*/
ATT_HANDLER(read_by_common_req)
{
    att_result_t rc = ATT_RESULT_READ_NOT_PERMITTED; /* default error */
    att_attr_t *found = NULL;
    uint16_t foundh = 0; /* handle of found attribute */
    att_attr_t *attr;
    uint16_t start;
    uint16_t end;
    ATT_UUID_T uuid;
    uint16_t handle;
    uint16_t group_end = 0;
    uint16_t uuid_type;
    bool_t get_group = FALSE;
    uint16_t add_read_status = ATT_RESULT_SUCCESS;
    uint8_t op = GET_SERVER_IN_OP(conn);

    switch (op)
    {
        case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
            uuid_type = ATT_UUID16;
            get_group = TRUE;
            break;


        case ATT_OP_READ_BY_GROUP_TYPE_REQ:
            get_group = TRUE;
            /* fall through */

        default:
            uuid_type = ATT_UUID_NONE; /* determine by length */
    }
    
    /* read pdu */
    conn->server_in->data = &conn->server_in->buf[1]; /* set correct starting point */
    start = read_uint16(&conn->server_in->data);
    end = read_uint16(&conn->server_in->data);

    /* read filter condition */
    if (!att_read_uuid(conn->server_in, &uuid, uuid_type))
    {
        att_send_error_rsp(ATT_ARG conn, op, start, ATT_RESULT_INVALID_PDU);
        return TRUE;
    }

    /* check parameters */
    if (!start || start > end)
    {
        att_send_error_rsp(ATT_ARG conn, op, start, ATT_RESULT_INVALID_HANDLE);
        return TRUE;
    }

    if (ATT_IS_CLIENT_CHANGE_UNAWARE(conn))
    {
        /* Allow only Database Hash read in change unaware state.
         * In change unaware state, only ATT_OP_READ_BY_TYPE_REQ
         * is forwarded to this ATT_HANDLER.
         */
        att_send_error_rsp(ATT_ARG conn, op, start, ATT_RESULT_DATABASE_OUT_OF_SYNC);
        return TRUE;
    }

    if (irq)
    {
        if (irq->result != ATT_RESULT_SUCCESS)
        {
            att_send_error_rsp(ATT_ARG conn, op, irq->handle, irq->result);
            return TRUE;
        }
        
        start = irq->handle + 1;
        foundh = irq->handle;
        group_end = 0xffff;
        
        if (op == ATT_OP_FIND_BY_TYPE_VALUE_REQ)
        {
            /* compare the value */
            if (irq->size_value == SERVER_IN_BUF_LEN(conn) &&
                memcmp(conn->server_in->data, irq->value, irq->size_value) == 0)
            {
                found = att_attr_find(ATT_ARG foundh, NULL);
            }
        }
        else
        {
            found = att_attr_find(ATT_ARG foundh, NULL);
        }
    }
    
    /* traverse through database */
    for (attr = att_attr_find(ATT_ARG start, &handle);
         attr && (handle <= end || found);
         attr = att_attr_next(ATT_ARG attr, &handle))
    {
        if (att_attr_match_uuid128(attr, &uuid))
        {
            /* add data */
            if (found)
            {
                /* NOTE: In case of Find By Type Value request, according to
                 * the ATT spec, the end handle shall be same as start handle
                 * if UUID is not grouping attribute defined by a higher layer
                 * specification. We have no way of knowing what future specs
                 * may define so only UUIDs recognised by att_attr_isgroup
                 * are considered as a 'grouping attribute'. */
                if (group_end == 0xffff)
                {
                    if (get_group)
                        group_end = handle - 1;
                    else
                        group_end = foundh;
                }

                if (!add_read_by_common_attr(conn, foundh, group_end,
                               &found, &irq, &add_read_status) || handle > end)
                {
                    break;
                }
            }

            /* Have a good default error code for matched iteration */
            rc = ATT_RESULT_READ_NOT_PERMITTED;

#ifndef ATT_FLAT_DB_SUPPORT
            if(!att_is_access_permitted(conn, attr))
            {
                /* Request failed as accessed over disallowed radio.
                   Get error code from application */
                att_access_ind(ATT_STATE(phandle), conn->cid, handle,
                              (ATT_ACCESS_DISALLOWED_OVER_RADIO|ATT_ACCESS_READ), 0, 0, NULL);
                return FALSE;
            }
#endif /* !ATT_FLAT_DB_SUPPORT */

            /* is read allowed? */
            if (conn->cid != ATT_CID_LOCAL && /* always allow local */
                (!(att_attr_perm(ATT_ARG attr) & ATT_PERM_READ) ||
                 ((rc = check_auth(ATT_ARG conn, attr, ATT_PERM_READ)) != ATT_RESULT_SUCCESS)))
            {
                if (conn->out.data) break; /* stop at this attribute */
                                           /* In case check auth fails due to reason ATT_RESULT_ENCRYPTION_PENDING,
                                              and if there are already matching attributes found, send the response
                                              and do not wait for encryption process to complete. */

                /* It is first attribute reading which need encryption and enryption is pending,
                   wait for encryption process to complete. */
#ifdef BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE
                if(rc == ATT_RESULT_ENCRYPTION_PENDING)
                    return FALSE;
#endif /* BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE */

                /* first attribute, fail the whole request */
                att_send_error_rsp(ATT_ARG conn, op, handle, rc);
                return TRUE;
            }
            
            /* Attribute is handled by the application */
#ifndef ATT_FLAT_DB_SUPPORT
            /* ACCESS_IND to be sent if IRQ, AUTHORIZATION or ENC_KEY_REQUIREMENTS is set */
            else if (att_attr_raw_flags(attr) & (ATT_ATTR_IRQ | ATT_ATTR_IRQ_R)
                 || (att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION)
                 || (att_attr_raw_flags(attr) & ATT_ATTR_ENC_KEY_REQUIREMENTS))
            {
                uint16_t flags = ATT_ACCESS_READ;

                if(att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION)
                    flags |= ATT_ACCESS_PERMISSION;

                if(att_attr_raw_flags(attr) & ATT_ATTR_ENC_KEY_REQUIREMENTS)
                    flags |= ATT_ACCESS_ENCRYPTION_KEY_LEN;

                /* request data from the application */
                att_access_ind(ATT_STATE(phandle), conn->cid, handle,
                              flags, 0, 0, NULL);
                return FALSE;
            }
#else /* ATT_FLAT_DB_SUPPORT */
            else if (att_attr_raw_flags(attr) & (ATT_ATTR_IRQ | ATT_ATTR_IRQ_R))
            {
                att_access_ind(ATT_STATE(phandle), conn->cid, handle,
                               (uint16_t)(ATT_ACCESS_READ | ATT_ACCESS_PERMISSION), 
                               0, 0, NULL);
                return FALSE;
            }
#endif /* !ATT_FLAT_DB_SUPPORT */
            /* add attribute to response */
            else
            {
                if (op == ATT_OP_FIND_BY_TYPE_VALUE_REQ)
                {
                    if (!att_attr_isgroup(attr))
                    {
                        get_group = FALSE;
                    }
#ifndef ATT_FLAT_DB_SUPPORT
                    if (att_attr_size(attr) != SERVER_IN_BUF_LEN(conn) ||
                        !att_attr_match(attr, conn->server_in->data))
#else /* ATT_FLAT_DB_SUPPORT */
                    if (!att_attr_match(attr, conn))
#endif /* !ATT_FLAT_DB_SUPPORT */
                    {
                        /* attribute does not match, skip to the next */
                        group_end = handle - 1;
                        continue;
                    }
                }
                
                found = attr;
                foundh = handle;
                group_end = 0xffff;
            }
        }

        /* set the group end unless already set */
        else if (found && group_end == 0xffff &&
                 get_group && att_attr_isgroup(attr))
        {
            group_end = handle - 1;
        }
    }

    /* add remaining data */
    if ((add_read_status == ATT_RESULT_SUCCESS)&&(found))
    {
        add_read_by_common_attr(conn, foundh, group_end, &found, 
                                        &irq, &add_read_status);
    }

    /* send packet */
    if ((add_read_status == ATT_RESULT_SUCCESS) && conn->out.data)
    {
        att_pkt_send(ATT_ARG conn);
    }

    /* send error */
    else
    {
        att_send_error_rsp(ATT_ARG conn, op, start, ATT_RESULT_ATTR_NOT_FOUND);
    }

    return TRUE;
}

/* Read or Read blob request from client */
#ifndef GATT_OFFLOAD
ATT_HANDLER(read_common_req)
{
    uint16_t handle;
    uint16_t offs = 0;
    att_result_t rc = ATT_RESULT_INVALID_HANDLE;
    att_result_t auth_rc;
    uint16_t len;
    att_attr_t *attr;
    uint8_t op = GET_SERVER_IN_OP(conn);

    conn->server_in->data = &conn->server_in->buf[1]; /* set data to point to handle */

    /* read pdu */
    handle = read_uint16(&conn->server_in->data);

    /* create the response packet */
    len = att_pkt_create(conn, (uint8_t)(op | ATT_OP_RSP_MASK));
    if(len == ATT_PKT_CREATE_FAILED)
    {
        att_send_error_rsp(ATT_ARG conn, op,
                           handle, ATT_RESULT_INSUFFICIENT_RESOURCES);
        return TRUE;
    }

    if (op == ATT_OP_READ_BLOB_REQ)
        offs = read_uint16(&conn->server_in->data);

    /* response from the application */
    if (irq)
    {
        if (irq->result == ATT_RESULT_SUCCESS)
        {
#ifndef ATT_FLAT_DB_SUPPORT
            /* When IRQ_R is not set and Authorization is required,
             * use attribute value from ATT DB.
             * Fix for Bug B-108170.
             */
            attr = att_attr_find(ATT_ARG irq->handle, NULL);
            /* attr value must not be NULL. Shall we panic if NULL? */
            if(attr!=NULL)
            {
                if(
                    (!(att_attr_raw_flags(attr) & ATT_ATTR_IRQ_R))&&
                    (att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION)
                  )
                {
                    /* read attribute value to the output buffer */
                    att_attr_read(&conn->out.data, attr, offs, len);
                }
                else
                {
                    att_pkt_write(conn, irq->value, irq->size_value);
                }
#else /* ATT_FLAT_DB_SUPPORT */
                att_pkt_write(conn, irq->value, irq->size_value);
#endif /* !ATT_FLAT_DB_SUPPORT */
                att_pkt_send(ATT_ARG conn);
#ifndef ATT_FLAT_DB_SUPPORT
            }
            /* if attribute is not found, ignore access response */
#endif /* !ATT_FLAT_DB_SUPPORT */
        }
        else
        {
            att_send_error_rsp(ATT_ARG conn, op, irq->handle, irq->result);
        }
        return TRUE;
    }

    /* get the attribute */
    if ((attr = att_attr_find(ATT_ARG handle, NULL)) == NULL)
        rc = ATT_RESULT_INVALID_HANDLE;

#ifndef ATT_FLAT_DB_SUPPORT
    else if(!att_is_access_permitted(conn, attr))
    {
        /* Request failed as accessed over disallowed radio.
           Get error code from application */
        att_access_ind(ATT_STATE(phandle), conn->cid, handle,
                       (ATT_ACCESS_DISALLOWED_OVER_RADIO|ATT_ACCESS_READ), offs, 0, NULL);
        return FALSE;
    }
#endif /* !ATT_FLAT_DB_SUPPORT */
    /* is read allowed? */
    else if (conn->cid != ATT_CID_LOCAL && /* always allow local */
             !(att_attr_perm(ATT_ARG attr) & ATT_PERM_READ))
        rc = ATT_RESULT_READ_NOT_PERMITTED;

    else if ((auth_rc = check_auth(ATT_ARG conn, attr, ATT_PERM_READ)) != ATT_RESULT_SUCCESS)
#ifdef BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE
        /* Attribute access which need encryption and enryption is pending,
           wait for encryption process to complete. */
        if(auth_rc == ATT_RESULT_ENCRYPTION_PENDING)
            return FALSE;
        else
#endif /* BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE */
            rc = auth_rc;

    /* we need error code from the application */
#ifndef ATT_FLAT_DB_SUPPORT
    /* ACCESS_IND to be sent if IRQ, AUTHORIZATION or ENC_KEY_REQUIREMENTS is set */
    else if (att_attr_raw_flags(attr) & (ATT_ATTR_IRQ | ATT_ATTR_IRQ_R)
         || (att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION)
         || (att_attr_raw_flags(attr) & ATT_ATTR_ENC_KEY_REQUIREMENTS))
    {
        uint16_t flags = ATT_ACCESS_READ;

        if(att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION)
            flags |= ATT_ACCESS_PERMISSION;
                               
        if(att_attr_raw_flags(attr) & ATT_ATTR_ENC_KEY_REQUIREMENTS)
            flags |= ATT_ACCESS_ENCRYPTION_KEY_LEN;

        /* request data from the application */
        att_access_ind(ATT_STATE(phandle), conn->cid, handle,
                       flags, offs, 0, NULL);
        return FALSE;
    }


    /* are we reading inside attribute? */
    else if (offs > att_attr_size(attr))
        rc = ATT_RESULT_INVALID_OFFSET;

    /* send response */
    else
    {
        /* read attribute value to the output buffer */
        att_attr_read(&conn->out.data, attr, offs, len);
        att_pkt_send(ATT_ARG conn);
        return TRUE;
    }
#else /* ATT_FLAT_DB_SUPPORT */
    else if (att_attr_raw_flags(attr) & (ATT_ATTR_IRQ | ATT_ATTR_IRQ_R))
    {
        att_access_ind(ATT_STATE(phandle), conn->cid, handle,
                        (uint16_t)(ATT_ACCESS_READ | ATT_ACCESS_PERMISSION),
                        offs, 0, NULL);
        return FALSE;
    }

    /* send response */
    else
    {
        /* read attribute value to the output buffer */
        att_attr_read(&conn->out.data, attr, offs, len, &rc);
        if(rc != ATT_RESULT_INVALID_OFFSET)
        {
            att_pkt_send(conn);
            return TRUE;
        }
    }
#endif /* !ATT_FLAT_DB_SUPPORT */
    /* if we got so far something went wrong */
    att_send_error_rsp(ATT_ARG conn, op, handle, rc);
    return TRUE;
}
#endif


/****************************************************************************
NAME
    att_handle_write_common_req - handle ATT requests from client

    This function takes care of the following ATT requests:
    - Write request
    - Write command
    - Signed Write command
*/
#ifndef GATT_OFFLOAD

ATT_HANDLER(write_common_req)
{
    uint16_t handle;
    att_attr_t *attr;
    att_result_t rc = ATT_RESULT_SUCCESS;
    att_result_t auth_rc;
    uint16_t perm_mask = 0;/* Permission mask is initially set to none */
    uint16_t len;
    uint8_t op = 0;
    

    /* Set the correct starting point */
    conn->server_in->data = &conn->server_in->buf[1];

    /* read pdu */
    handle = read_uint16(&conn->server_in->data);
    len = SERVER_IN_BUF_LEN(conn);
    
    /* set the permission requirements */
    if ((op = GET_SERVER_IN_PDU(conn)) & ATT_OP_FLAGS)
    {
        if(op & ATT_OP_CMD_FLAG)
            perm_mask |= ATT_PERM_WRITE_CMD;

        if (op & ATT_OP_AUTH_FLAG)
        {
            perm_mask |= ATT_PERM_AUTHENTICATED;

            /* Signed write is not allowed on EATT bearer */
            if (ATT_IS_EATT_BEARER(conn))
                return TRUE;
        }
    }
    else 
    {
        /* 
         * Appropriately set operation permission mask irrespective of the way
         * write operation is achieved for a database in vm constant or
         * non-constant memory.
         */
        perm_mask = ATT_PERM_WRITE_REQ;
    }

    if (irq) 
    {
        rc = irq->result;

        /* Store the value in db only on special cases */
        if(rc == ATT_RESULT_SUCCESS)
        {
           attr = att_attr_find(ATT_ARG handle, NULL);

           if(!attr)
               rc = ATT_RESULT_INVALID_HANDLE;
           else if(((att_attr_raw_flags(attr) & ATT_ATTR_IRQ) ||
                    ((att_attr_raw_flags(attr) & ATT_ATTR_IRQ_W) && /* IRQ_W is set */
                    !(att_attr_raw_flags(attr) & ATT_ATTR_IRQ_R))) /* IRQ_R is not set */
#ifndef ATT_FLAT_DB_SUPPORT
                /* When IRQ_R is not set and Authorization is required,
                 * use attribute value from ATT DB.
                 * Fix for Bug B-108170.
                 */
                   ||
                   ((!(att_attr_raw_flags(attr) & ATT_ATTR_IRQ_R))&&
                    (att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION))
#endif /* !ATT_FLAT_DB_SUPPORT */
                  )
               /* The onchip flattend ATT DB only has enough bits to store the
                * ATT_ATTR_IRQ flag and not the _W, _R variants. Consequently, the
                * implementation always attempts to update the database with UNLESS the
                * database is in vm const memory.
                * For const databases the updated value is held by the VM.
                */
#if !defined(BUILD_FOR_HOST) || defined(BLUESTACK_HOST_IS_APPS)
               if (! vm_const_is_encoded(attr))
#endif /* ! BUILD_FOR_HOST */
                   rc = att_attr_set(ATT_ARG &attr, 0 /* offset */, len, conn->server_in->data);
        }
    }

    else if ((attr = att_attr_find(ATT_ARG handle, NULL)) == NULL)
        rc = ATT_RESULT_INVALID_HANDLE;


#ifndef ATT_FLAT_DB_SUPPORT
    else if(!att_is_access_permitted(conn, attr))
    {
        /* Request failed as accessed over disallowed radio.
           Get error code from application */
        /* No need to send length and data as ATT
           is asking for error code rather than permissions 
           - is it fine with Synergy? */
        att_access_ind(ATT_STATE(phandle), conn->cid, handle,
                       (ATT_ACCESS_DISALLOWED_OVER_RADIO|ATT_ACCESS_WRITE|ATT_ACCESS_WRITE_COMPLETE),
                       0, 0, NULL);
        return FALSE;
    }
#endif /* !ATT_FLAT_DB_SUPPORT */

    else if (conn->cid != ATT_CID_LOCAL && /* always allow local */
             !(att_attr_perm(ATT_ARG attr) & perm_mask))
        rc = ATT_RESULT_WRITE_NOT_PERMITTED;

    else if ((auth_rc = check_auth(ATT_ARG conn, attr, (uint16_t)op)) != ATT_RESULT_SUCCESS)
#ifdef BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE
            /* Attribute access which need encryption and enryption is pending,
               wait for encryption process to complete. */
            if(auth_rc == ATT_RESULT_ENCRYPTION_PENDING)
                return FALSE;
            else
#endif /* BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE */
                rc = auth_rc;

    /* Attribute is handled by the application */
#ifndef ATT_FLAT_DB_SUPPORT
            /* ACCESS_IND to be sent if IRQ, AUTHORIZATION or ENC_KEY_REQUIREMENTS is set */
    else if((att_attr_raw_flags(attr) & (ATT_ATTR_IRQ | ATT_ATTR_IRQ_W | ATT_ATTR_IRQ_R)
         || (att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION)
         || (att_attr_raw_flags(attr) & ATT_ATTR_ENC_KEY_REQUIREMENTS))
         && (conn->cid == ATT_CID_LOCAL?(att_attr_raw_flags(attr) & ATT_ATTR_IRQ_R):TRUE))
    {
        uint8_t *data = NULL;
        uint16_t flags = ATT_ACCESS_WRITE | ATT_ACCESS_WRITE_COMPLETE;

        if(att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION)
            flags |= ATT_ACCESS_PERMISSION;

        if(att_attr_raw_flags(attr) & ATT_ATTR_ENC_KEY_REQUIREMENTS)
            flags |= ATT_ACCESS_ENCRYPTION_KEY_LEN;

        if (len && (data = xpmalloc(len)) != NULL)
        {
            qbl_memscpy(data, len, conn->server_in->data, len);
        }

        att_access_ind(ATT_STATE(phandle), conn->cid, handle, 
                       flags, 0 /* offs */, len, data);
        return FALSE;
    }

#endif /* !ATT_FLAT_DB_SUPPORT */
    /* store the value */
    else
    {
        rc = att_attr_set(ATT_ARG &attr, 0 /* offset */, len, conn->server_in->data);
    }

    /* send response */
    if (!(op & ATT_OP_CMD_FLAG))
    {
        if (rc == ATT_RESULT_SUCCESS)
        {
            if(ATT_PKT_CREATE_FAILED == 
               att_pkt_create(conn, (uint8_t)(op | ATT_OP_RSP_MASK)))
            {
                /*4 Attempt to send an error response */
                att_send_error_rsp(ATT_ARG conn, op,
                           handle, ATT_RESULT_INSUFFICIENT_RESOURCES);
                return TRUE;
            }
            att_pkt_send(ATT_ARG conn);
        }
        else
        {
            att_send_error_rsp(ATT_ARG conn, op, handle, rc);
        }
    }

    return TRUE;
}
#endif
/****************************************************************************
NAME
    att_handle_prepare_write_req - handle ATT requests from client

    This function takes care of the following ATT requests:
    - Prepare Write request
*/
ATT_HANDLER(prepare_write_req)
{
    uint16_t handle;
    att_result_t rc = ATT_RESULT_SUCCESS;
    att_attr_t *attr;
    uint16_t len = 0;
    uint16_t offs;
    
    /* set the correct starting point */    
    conn->server_in->data = &conn->server_in->buf[1];

    /* read pdu */
    handle = read_uint16(&conn->server_in->data);
    offs = read_uint16(&conn->server_in->data);

    /* if irq do all handling here */
    if (irq)
    {
        /* Store the result of the permissions */
        rc = irq->result;
        if(rc == ATT_RESULT_SUCCESS)
        {
            attr = att_attr_find(ATT_ARG handle, NULL);

            if(!attr)
            {
                rc = ATT_RESULT_INVALID_HANDLE;
            }
            else
            {
                len = SERVER_IN_BUF_LEN(conn);

                /* This does allocation for the response as well */
                rc = att_wq_add(conn, handle, offs, len, conn->server_in->data);
            }
        }
    }

    else if ((attr = att_attr_find(ATT_ARG handle, NULL)) == NULL)
        rc = ATT_RESULT_INVALID_HANDLE;

#ifndef ATT_FLAT_DB_SUPPORT
    else if(!att_is_access_permitted(conn, attr))
    {
        /* Request failed as accessed over disallowed radio.
           Get error code from application */
        att_access_ind(ATT_STATE(phandle), conn->cid, handle,
                       (ATT_ACCESS_DISALLOWED_OVER_RADIO|ATT_ACCESS_WRITE|ATT_ACCESS_WRITE_COMPLETE),
                       0, 0, NULL);
        return FALSE;
    }
#endif /* !ATT_FLAT_DB_SUPPORT */


    else if (conn->cid != ATT_CID_LOCAL && /* always allow local */
             !(att_attr_perm_ext(ATT_ARG attr) & ATT_PERM_RELIABLE_WRITE)
#ifdef DO_NOT_HONOR_RELIABLE_PERMISSIONS
             && !(att_attr_perm(ATT_ARG attr) & ATT_PERM_WRITE_REQ)
#endif
             )
        rc = ATT_RESULT_WRITE_NOT_PERMITTED;

    else if ((rc = check_auth(ATT_ARG conn, attr, ATT_PERM_WRITE_REQ)) != ATT_RESULT_SUCCESS)
    {
#ifdef BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE
                /* Attribute access which need encryption and enryption is pending,
                   wait for encryption process to complete. */
                if(rc == ATT_RESULT_ENCRYPTION_PENDING)
                    return FALSE;
#endif /* BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE */
                /* nop, rc already set */
    }

#ifndef ATT_FLAT_DB_SUPPORT
    else if((att_attr_raw_flags(attr) & (ATT_ATTR_IRQ | ATT_ATTR_IRQ_W | ATT_ATTR_IRQ_R)
         || (att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION)
         || (att_attr_raw_flags(attr) & ATT_ATTR_ENC_KEY_REQUIREMENTS))
         /* Donot send access_ind if already authorized by the app */
         && ((conn->common->wq == NULL) || (!att_is_app_authorized(conn, handle)))) 
    {
        /* Send the WRITE flag so that upper-layers now which operation it is */
        uint16_t flags = ATT_ACCESS_WRITE;

        /* Enable permissions flag if authorization required or IRQ's set */
        if(att_attr_raw_flags(attr) & ATT_ATTR_AUTHORIZATION)
            flags |= ATT_ACCESS_PERMISSION;

        if(att_attr_raw_flags(attr) & ATT_ATTR_ENC_KEY_REQUIREMENTS)
            flags |= ATT_ACCESS_ENCRYPTION_KEY_LEN;

        /* Send ind for permissions (authorization,encryption key size) */
        /* Value checking done during access_ind at execute write (no data sent here) */

        att_access_ind(ATT_STATE(phandle), conn->cid,
                       handle, flags, 0, 0, NULL);
        return FALSE;
    }
#else /* ATT_FLAT_DB_SUPPORT */
    else if (att_attr_raw_flags(attr) & (ATT_ATTR_IRQ | ATT_ATTR_IRQ_W | ATT_ATTR_IRQ_R)
            /* Donot send access_ind if already authorized by the app */
            && ((conn->common->wq == NULL) || (!att_is_app_authorized(conn, handle))))
    {
        /* Send ind for permissions (authorization and encryption key size) */
        /* Value checking done during access_ind at execute write */
        att_access_ind(ATT_STATE(phandle), conn->cid,
                       handle, ATT_ACCESS_PERMISSION, 0, 0, NULL);
        return FALSE;
    }
#endif /* !ATT_FLAT_DB_SUPPORT */
    /* add to write queue */
    else 
    {
        len = SERVER_IN_BUF_LEN(conn);
        rc = att_wq_add(conn, handle, offs, len, conn->server_in->data);
    }

    if (rc == ATT_RESULT_SUCCESS)
    {
        write_uint16(&conn->out.data, handle);
        write_uint16(&conn->out.data, offs);
        att_pkt_write(conn, conn->server_in->data, len);
        att_pkt_send(ATT_ARG conn);
    }
    else
    {
        /* send error */
        att_send_error_rsp(ATT_ARG conn, ATT_OP_PREPARE_WRITE_REQ, handle, rc);
    }

    return TRUE;
}

/* Execute write request from client */
ATT_HANDLER(execute_write_req)
{
    att_result_t rc = ATT_RESULT_SUCCESS;
    att_writeq_t *wq;
    att_attr_t *attr = NULL;
    uint16_t handle = 0xffff;
    bool_t send_irq = FALSE;
    uint8_t flags;

    /* if irq, set the correct starting point */    
    if (irq )
       conn->server_in->data = &conn->server_in->buf[1];

    /* read pdu */
    flags = read_uint8(&conn->server_in->data);
    
    switch (flags)
    {
        case ATT_EXECUTE_WRITE:
            if(!irq)
            {
                /* Check the sanity of length of attr values on fresh execute req. */
                /* To be done only once */
                rc = att_check_attr_length_in_wq(ATT_ARG conn, &handle);
            }

            if(irq && ((rc = irq->result) != ATT_RESULT_SUCCESS))
            {
                /* nop : rc already set */
            }
            else
            {
            /* Either irq with success or fresh execute-write */
            for (wq = conn->common->wq; wq && rc == ATT_RESULT_SUCCESS ; wq = att_wq_next(conn, wq))
            {                
                if (wq->handle < handle)
                {
                    /* start searching from the beginning */
                    attr = att_attr_find(ATT_ARG wq->handle, &handle);
                }
                
                /* traverse through database */
                for (/* nop */;
                     attr && handle <= wq->handle && rc == ATT_RESULT_SUCCESS;
                     attr = att_attr_next(ATT_ARG attr, &handle))
                {
                    if (handle == wq->handle)
                    {
                        if (!irq &&
                               att_attr_raw_flags(attr) & (ATT_ATTR_IRQ | ATT_ATTR_IRQ_W | ATT_ATTR_IRQ_R))
                        {
                            uint8_t *data = NULL;
                            if (wq->len && (data = xpmalloc(wq->len)) != NULL)
                            {
                                qbl_memscpy(data, wq->len, wq->value, wq->len);
                            }
                            att_access_ind(ATT_STATE(phandle), wq->cid,
                                          handle, ATT_ACCESS_WRITE,
                                           wq->offs, wq->len, data);
                            send_irq = TRUE;
                        }
                        /* Write to db only if we have ownership */
                        else if (irq && (rc = irq->result) == ATT_RESULT_SUCCESS
#ifndef ATT_FLAT_DB_SUPPORT
                                     && !(att_attr_raw_flags(attr) & ATT_ATTR_IRQ_R)
#else
                                     && !vm_const_is_encoded(attr) && !(att_attr_raw_flags(attr) & ATT_ATTR_IRQ)
#endif
                                     )
                        {
                            if((rc = att_attr_exe_set(ATT_ARG &attr, wq)) != ATT_RESULT_SUCCESS)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            /* If we sent an access_ind then send the final one */
            /* Donot write to database or clear the queue till we get a rsp */
            if(send_irq)
            {
                att_access_ind(ATT_STATE(phandle), conn->cid,
                              0, ATT_ACCESS_WRITE_COMPLETE,
                              0, 0, NULL);
                return FALSE;
            }
            else if(!irq && rc == ATT_RESULT_SUCCESS)
            {
                /* No IRQ set and no access_rsp, write all data into database */
                handle = 0xffff;
                attr = NULL;

                for (wq = conn->common->wq; wq && rc == ATT_RESULT_SUCCESS ; wq = att_wq_next(conn, wq))
                {
                    if (wq->handle < handle)
                    {
                        /* start searching from the beginning */
                        attr = att_attr_find(ATT_ARG wq->handle, &handle);
                    }
                    for (/* nop */;
                         attr && handle <= wq->handle && rc == ATT_RESULT_SUCCESS;
                         attr = att_attr_next(ATT_ARG attr, &handle))
                    {
                        if (handle == wq->handle)
                        {
                            if((rc = att_attr_exe_set(ATT_ARG &attr, wq)) != ATT_RESULT_SUCCESS)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            }

            /* free the queue */
            att_wq_free(conn);
            break;

        case ATT_EXECUTE_CANCEL:

            /* Client cancels the execute write, notify the application and
             * clear the queue. */
            if (!irq)
            {
                att_access_ind(ATT_STATE(phandle), conn->cid,
                              0, ATT_ACCESS_WRITE_CANCELLED,
                              0, 0, NULL);
                att_wq_free(conn);
                return FALSE;
            }
            /* Response to the client will be sent based on the irq->result,
             * either error rsp or execute write rsp */
            else
            {
                rc = irq->result;
            }
            break;

        default:
            rc = ATT_RESULT_INVALID_PDU;
    }    

    if (rc == ATT_RESULT_SUCCESS)
    {
        /* send response has to pass */
        att_pkt_create(conn, ATT_OP_EXECUTE_WRITE_RSP);
        att_pkt_send(ATT_ARG conn);
    }
    else
    {
        /* send error - the queue is already freed */
        att_send_error_rsp(ATT_ARG conn, ATT_OP_EXECUTE_WRITE_REQ, handle, rc);
    }

    return TRUE;
}

/* Handle value confirmation from client */
ATT_HANDLER(handle_value_cfm)
{
    ATT_HANDLER_IRQ_UNUSED

    /* read pdu */
    /* empty packet */
    
    att_handle_value_cfm(ATT_STATE(phandle), conn->cid, ATT_RESULT_SUCCESS);

    if (ATT_IS_SET_CHANGE_AWARE_ON_HANDLE_VALUE_CFM(conn) &&
        att_is_single_att_bearer(ATT_ARG conn))
    {
        att_move_to_change_aware(ATT_ARG conn);
    }

    return TRUE;
}

/*
 * Responses to the client
 ****************************************************************************/
void att_send_error_rsp(ATT_FUNC_STATE att_conn_t *conn, uint8_t op,
                        uint16_t handle, att_result_t err)
{
    /* destroy outgoing packet */
    if (conn->out.data)
    {
        pfree(conn->out.buf);
        conn->out.buf = conn->out.data = NULL;
    }
    
    /* Do not send error if original opcode was a command */
    if (op & ATT_OP_CMD_FLAG)
    {
        return;
    }

    if (err == ATT_RESULT_DATABASE_OUT_OF_SYNC)
    {
        ATT_SET_CHANGE_AWARE_ON_NEXT_ATT_REQ(conn);
    }

    att_pkt_create(conn, ATT_OP_ERROR_RSP);
    
    write_uint8(&conn->out.data, op);
    write_uint16(&conn->out.data, handle);
    write_uint8(&conn->out.data, (uint8_t)err);

    att_pkt_send(ATT_ARG conn);
}

/*
 * Requests from the application
 ****************************************************************************/
void att_register_req(ATT_FUNC_STATE
                      ATT_REGISTER_REQ_T *req)
{
    att_result_t rc;

    /* TODO: we may want to register initial server for GATT attributes
     * during initialisation - at least have something unless app registers
     * own values */

    rc = att_process_register_req(ATT_ARG req->phandle);

    att_register_cfm(req->phandle, rc);
}

void att_unregister_req(ATT_FUNC_STATE
                        ATT_UNREGISTER_REQ_T *req)
{
    att_result_t rc = ATT_RESULT_INVALID_PHANDLE;

    if (att_conn_find(ATT_ARG ATT_CID_LOCAL) != NULL &&
        ATT_STATE(phandle) == req->phandle)
    {
        att_conn_rm(ATT_ARG ATT_CID_LOCAL);
        ATT_STATE(phandle) = 0;

        /* delete attributes associated with this server */
        att_attr_remove(ATT_ARG ATT_HANDLE_MIN, ATT_HANDLE_MAX);
        
        rc = ATT_RESULT_SUCCESS;
    }

    att_unregister_cfm(req->phandle, rc);
}

#ifndef ATT_FLAT_DB_SUPPORT
void att_add_req(ATT_FUNC_STATE
                 ATT_ADD_REQ_T *req)
{
    att_result_t rc;

    if (req->phandle != ATT_STATE(phandle))
    {
        rc = ATT_RESULT_INVALID_PHANDLE;
    }
    else if (!req->attrs)
    {
        rc = ATT_RESULT_INVALID_DB;
    }
    else
    {    
        rc = att_attr_add(ATT_ARG &req->attrs, 0 /* unused */);
    }

    att_add_cfm(req->phandle, rc);
}

void att_remove_req(ATT_FUNC_STATE
                    ATT_REMOVE_REQ_T *req)
{
    att_result_t rc;
    uint16_t num = 0;
    
    if (req->phandle != ATT_STATE(phandle))
    {
        rc = ATT_RESULT_INVALID_PHANDLE;
    }
    else if (!req->start || req->start > req->end)
    {
        rc = ATT_RESULT_INVALID_HANDLE;
    }
    else
    {
        rc = ATT_RESULT_SUCCESS;
        num = att_attr_remove(ATT_ARG req->start, req->end);
    }

    att_remove_cfm(req->phandle, num, rc);
}
#endif /* !ATT_FLAT_DB_SUPPORT */

void att_handle_value_req(ATT_FUNC_STATE
                          ATT_HANDLE_VALUE_REQ_T *req,
                          bool from_stream)
{
    att_conn_t *conn;
    uint8_t op = ATT_OP_HANDLE_VALUE_NOT;
    att_result_t rc = ATT_RESULT_INVALID_CID;
    MBLK_T *mblk = NULL;
#ifndef GATT_OFFLOAD
    att_attr_t *attr;
 
    if ((conn = att_conn_find(ATT_ARG req->cid)) != NULL &&
        ATT_IS_CLIENT_CHANGE_UNAWARE(conn) &&
        ((req->flags == ATT_HANDLE_VALUE_NOTIFICATION) ||
         !ATT_IS_SERVICE_CHANGED_HANDLE(req->handle)))
    {
        rc = ATT_RESULT_CHANGE_UNAWARE_DISALLOWED;
    }
    else if ((conn != NULL) && from_stream && 
              att_stream_pdu_exceeds_mtu(conn, (MBLK_T*)req->value))
    {
        /* Notification from stream exceeds the MTU */
        rc = ATT_RESULT_INVALID_PARAMS;
    }
    else if ((attr = att_attr_find(ATT_ARG req->handle, NULL)) == NULL)
    {
        /* Attribute handle not found in the ATT Database.
         * This should not happen, return confirm with error. */
        rc = ATT_RESULT_INVALID_HANDLE;
    }
    else if ((conn != NULL) &&
             (rc = check_auth(ATT_ARG conn, attr, ATT_PERM_READ)) != ATT_RESULT_SUCCESS)
    {
        /* Authentication permissions are not met for this attribute handle.
         * Return confirm with error to application */
    }
    else if (conn != NULL)
#else
    if ((conn = att_conn_find(ATT_ARG req->cid)) != NULL)
#endif
    {
        uint16 len = ATT_OPCODEN_LEN + ATT_HANDLE_LEN + req->size_value;

        if (req->flags & ATT_HANDLE_VALUE_INDICATION)
        {
            if (conn->pend & PEND_HANDLE_CFM)
            {
                /* still waiting for confirmation */
                att_handle_value_cfm(req->phandle, req->cid, ATT_RESULT_BUSY);
                return;
            }

            op = ATT_OP_HANDLE_VALUE_IND;
            rc = ATT_RESULT_SUCCESS_SENT;
        }
        else
        {
            op = ATT_OP_HANDLE_VALUE_NOT;
            rc = ATT_RESULT_SUCCESS;
        }

        /* Create an output buffer and write opcode into it */
        if (ATT_PKT_CREATE_FAILED == att_pkt_create_len(conn, op, len))
        {
            rc = ATT_RESULT_INSUFFICIENT_RESOURCES;
        }
        else
        {
            write_uint16(&conn->out.data, req->handle);

            /* Copy value data if not held in MBLK */
            if (req->value != NULL)
            {
                (void)att_pkt_write(conn, req->value, req->size_value);
            }

            if (ATT_RESULT_SUCCESS != att_pkt_send_mblk(ATT_ARG conn, mblk))
            {
                rc = ATT_RESULT_BUSY;
            }
        }
    }

    /* Don't send a confirm message if the data was from a stream because it
     * takes up memory and adds latency. Note phandle is not setup for stream.
     */
    if (!from_stream)
    {
        if ((rc == ATT_RESULT_SUCCESS_SENT) &&
             conn && ATT_IS_CLIENT_CHANGE_UNAWARE(conn))
        {
            ATT_SET_CHANGE_AWARE_ON_HANDLE_VALUE_CFM(conn);
        }

        att_handle_value_cfm(req->phandle, req->cid, rc);
    }
}

/*! \brief Handles the notification ONLY from primitive path.
           Returns ATT_HANDLE_VALUE_NTF_CFM indicating success/failure
           of the notification with attribute handle and context.
    \param req pointer to the notification with attribute handle and context.
    \returns void
*/
void att_handle_track_value_ntf(ATT_FUNC_STATE
                          ATT_HANDLE_VALUE_NTF_T *req)
{
    att_conn_t *conn;
    uint8_t op;
    att_result_t rc = ATT_RESULT_INVALID_CID;
    att_attr_t *attr;
    
    if (req->flags != ATT_HANDLE_VALUE_NOTIFICATION)
    {
        rc = ATT_RESULT_INVALID_PARAMS;
    }
    else if ((conn = att_conn_find(ATT_ARG req->cid)) != NULL &&
              ATT_IS_CLIENT_CHANGE_UNAWARE(conn))
    {
        rc = ATT_RESULT_CHANGE_UNAWARE_DISALLOWED;
    }
    else if ((attr = att_attr_find(ATT_ARG req->handle, NULL)) == NULL)
    {
        /* Attribute handle not found in the ATT Database.
         * This should not happen, return confirm with error. */
        rc = ATT_RESULT_INVALID_HANDLE;
    }
    else if ((conn != NULL) &&
             (rc = check_auth(ATT_ARG conn, attr, ATT_PERM_READ)) != ATT_RESULT_SUCCESS)
    {
        /* Authentication permissions are not met for this attribute handle.
         * Return confirm with error to application */
    }
    else if (conn != NULL)
    {
        uint16 len = ATT_OPCODEN_LEN + ATT_HANDLE_LEN + req->size_value;

        op = ATT_OP_HANDLE_VALUE_NOT;
        rc = ATT_RESULT_SUCCESS;

        if(ATT_PKT_CREATE_FAILED == att_pkt_create_len(conn, op, len))
        {
            rc = ATT_RESULT_INSUFFICIENT_RESOURCES;
        }
        else
        {
            write_uint16(&conn->out.data, req->handle);
            att_pkt_write(conn, req->value, req->size_value);
            if(ATT_RESULT_SUCCESS != att_pkt_send(ATT_ARG conn))
            {
                rc = ATT_RESULT_BUSY;
            }
        }
    }

    att_handle_track_value_cfm(req, rc);
}


/*! \brief Handler function of ATT_ACCESS_RSP.
          Retriggers the processing of the PDU for a connection which generated 
          ATT_ACCESS_IND  to the higherlayers, if processing is complete then 
          processing of queued ATT PDUs would be triggered.
    \param rsp  pointer to access response structure
    \returns None.
*/

void att_access_rsp(ATT_FUNC_STATE
                    ATT_ACCESS_RSP_T *rsp)
{
    att_conn_t *conn;

    if ((conn = att_conn_find(ATT_ARG rsp->cid)) == NULL ||
        conn->handler == NULL)
    {
        /* silently ignore */
        return;
    }

    att_process_pending_msgs(ATT_ARG conn, rsp);
}


/*! \brief Queue up the incoming ATT data when ATT server is busy. 
           When ATT is waiting for a response from DM or Application, it will
           not be able to process incoming data, in such cases PDUs will be 
           queued up in the connection strucutre depending on the importance of 
           PDU and memory availability.The logic to queue up the PDU is as follow,
           PDUs will be queued up till the size reaches 
           ATT_INCOMING_DATA_QUEUE_SIZE irrespective of the PDU type. 
           Incase a Request or Handle Value Confirm received when Q is full then 
           the latest command will be purged from the Q, i.e, last ATT command 
           from the Q. So it is essential that minimum queue size should be 
           set to 2 inorder to support queueing of ATT Request and Handle Value 
           Confirmation for all scenarios.
           Queued up pdu will be handled once the current transaction is 
           complete. 
    \param conn  ATT connection structure.
    \param ind  L2CAP data read indication, which containing data to be queued.
    \returns None.
*/
void att_add_pending_queue(ATT_FUNC_STATE att_conn_t *conn, L2CA_DATAREAD_IND_T *ind)
{
    uint16_t queue_size = 0;
    att_in_t *p_in,*p_in_prev = NULL;
    uint8_t opcode = att_opcode_from_mblk(ind->data);
    uint16_t length = mblk_get_length(ind->data);
    uint8_t *pdu;

    /* Check for protocol violation, if we have received a request,we shouldn't 
     * find any request in the queue */
    if (is_op_req(opcode))
    {
        for (p_in = conn->server_in; p_in != NULL; p_in = p_in->next)
        {
            if (is_op_req(p_in->buf[0]))
            {
                att_handle_protocol_violation(conn);
                return;
            }
        }
    }

    /* Get to the last node of in-data queue, count the nodes while traversing*/
    for (p_in = conn->server_in; 
         p_in->next != NULL; 
         p_in = p_in->next, queue_size++)
    {
        p_in_prev = p_in ;
    }

    /* Low on memory */
    if ((pdu = xpmalloc(length)) == NULL)
    {
        /* When we are short on memory, reply to requests with error and ignore 
         * commands. It is very unlikely that we will not be able to allocate 1 
         * word to queue up ATT handle value confirm, hence it is excluded 
         * from below logic. 
         */
        if (is_op_req(opcode))
        {
            uint16_t err_handle = 0;

            /* Check if we have received enough bytes to search for handle */
            if (length >= (ATT_OPCODEN_LEN + ATT_HANDLE_LEN))
            {
                uint8_t *mapped_data = mblk_map(ind->data, 0, 3);
                err_handle = att_pdu_error_handle(mapped_data, length);
                mblk_unmap(ind->data, mapped_data);
            }

            att_send_error_rsp(ATT_ARG conn, opcode, err_handle,
                               ATT_RESULT_INSUFFICIENT_RESOURCES);
        }
        return;
    }

    /* Incoming data queue is full */
    if (queue_size >= ATT_INCOMING_DATA_QUEUE_SIZE)
    {
        /* We don't have to queue up unreliable ATT Commands when Q is full*/
        if (ATT_IS_OP_CMD(opcode))
        {
            pfree(pdu);
            return;
        }

        /* Purge queued command, so that received Request or Handle Value 
         * Confirmation can be sneaked into the existing queue node */
        if (ATT_IS_OP_CMD(p_in->buf[0]))
        {
            pfree(p_in->buf);
        }
        else if (p_in_prev != NULL && ATT_IS_OP_CMD(p_in_prev->buf[0]))
        {
            pfree(p_in_prev->buf);
            p_in = p_in_prev; 
        }
        else
        {
            /* Peer must have sent unexpected handle value cfm, we would have
             * disconnected anyway in att_pend_check() even if its added to Q */
            att_handle_protocol_violation(conn);
            pfree(pdu);
            return;
        }
    }
    else
    {
        /* Create in-data node to hold the incoming data */
        p_in->next = zpnew(att_in_t);
        p_in = p_in->next;
    }

    /* Create a pmalloc block from mblk and update the buffer pointer */
    p_in->len = length ;
    p_in->buf = pdu;
    mblk_copy_to_memory(ind->data, 0, p_in->len, p_in->buf);

    /* Initialize read pointer */
    p_in->data = p_in->buf;
    return;
}

 /*! \brief Checks incoming data queue and triggers processing of each 
           PDU one by one until the queue becomes empty, unless ATT is blocked 
           on other modules to process the PDU.
    \param conn  ATT connection structure.
    \returns None.
*/
void att_check_pending_queue(ATT_FUNC_STATE att_conn_t *conn)
{
    while(conn->server_in != NULL)
    {
        /* Continue processing of pending queue if we are done handling 
         * this packet */
        if (!att_process_queue_head(ATT_ARG conn))
            return;
    }
}

/*! \brief Clean up incoming data queue for an ATT connection. 
           This should be used during deinit or error handling scenarios such as
           link loss etc.
    \param conn  ATT connection structure.
    \returns None.
*/
void att_clean_pending_queue(att_conn_t *conn)
{
    while(conn->server_in != NULL)
        att_remove_pending_queue_head(conn);

    /* Reset handler function of the pending PDU */
    conn->handler = NULL;
}

/*! \brief Processes head of pending incoming ATT data queue. 
           ATT incoming PDUs are queued when local ATT server is blocked for 
           some reasons. This function triggers the processing of ATT incoming 
           data queue head. While processing the packet, ATT server can move to 
           blocked state because of sign verification or for application inputs,
           in that case the function returns FALSE to indicate PDU processing is
           not complete.
    \param conn  ATT connection structure.
    \returns TRUE if head processing is complete.
*/

bool_t att_process_queue_head(ATT_FUNC_STATE att_conn_t *conn)
{
    uint8_t op;
    const att_handler_t *pkt;
    bool_t pkt_done = TRUE;
    att_result_t result = ATT_RESULT_REQUEST_NOT_SUPPORTED;

    /* handle packet */
    if (((pkt = att_get_pkt_handler(&op, conn->server_in)) != NULL) &&
        ((result = att_validate_pdu(op, conn, conn->server_in, pkt)) == ATT_RESULT_SUCCESS))
    {
        /* signature verification is pending,return "processing not done" status
         * to block other requests being processed */
        if (conn->pend & PEND_AUTH_CHECK)
        {
            return FALSE;
        }

        if (att_pend_check(conn, op) &&
            att_handle_pkt_if_change_unaware(ATT_ARG conn, conn->server_in))
        {
            /* handle pdu */
            if ((pkt_done = pkt->handler(ATT_ARG conn, NULL)) == FALSE)
            {
                conn->handler = pkt->handler;
            }
        }
    }
    else if (!ATT_IS_OP_CMD(op)) 
    {
        if (pkt == NULL || ATT_IS_SERVER_OPERATION(op))
            att_send_error_rsp(ATT_ARG conn, op, 0, result);
    }

    if (pkt_done)
    {
        att_remove_pending_queue_head(conn);
        conn->handler = NULL;
    }
    else
    {
        att_out_pkt_free(conn);
    }
    return pkt_done;
}

/*! \brief Removes incoming data queue head and updates the head. This should
           be called only if packet processing is complete.
    \param conn  ATT connection structure.
    \returns None.
*/
void att_remove_pending_queue_head(att_conn_t *conn)
{
    if (conn->server_in != NULL)
    {
        att_in_t * pnext = conn->server_in->next;

        pfree(conn->server_in->buf);
        pfree(conn->server_in);

        /* we are done with this packet, move the head to next pending packet */
        conn->server_in = pnext;
    }
}


/*! \brief Sets the state to change aware

    \param conn  ATT connection structure
    \returns None.
*/
void att_set_change_aware_state(att_conn_t *conn)
{
    conn->client_state_change_unaware = FALSE;

    ATT_CLEAR_CHANGE_AWARE_ON_NEXT_ATT_REQ(conn);
    ATT_CLEAR_CHANGE_AWARE_ON_NEXT_ATT_REQ_HASH(conn);
    ATT_CLEAR_CHANGE_AWARE_ON_HANDLE_VALUE_CFM(conn);
}

/*! \brief Handler function for change aware
           a) Sets the state to change aware
           b) For bonded device, sets the state to change aware in SMDB
           c) Sends ATT_CHANGE_AWARE_IND to application
    \param conn ATT connection structure
    \param phandle Application handle
    \returns None.
*/
void att_move_to_change_aware(ATT_FUNC_STATE att_conn_t *conn)
{
    ATT_SET_CHANGE_AWARE(conn);
}

/*! \brief Handles the incoming PDU in change unaware state.
           In change-unaware state, when a client requests an operation
           at any Attribute Handle or list of Attribute Handles by sending
           an ATT request, the server shall not perform the operation but
           instead shall send a response with the Error Code set to 
           "Database Out Of Sync".
    \param conn ATT connection structure
    \param p_in pointer to att_in_t structure
    \returns bool_t. Returns TRUE if received PDU can be processed in 
             change unaware.
*/
bool_t att_handle_pkt_if_change_unaware(ATT_FUNC_STATE att_conn_t *conn,
                                        att_in_t *p_in)
{
    uint8_t op = p_in->buf[0];

    /* Allow ATT_OP_EXCHANGE_MTU_REQ as the operation is not related
     * to attribute handle
     * Handle ATT_OP_HANDLE_VALUE_CFM in ATT_HANDLER(handle_value_cfm)
     */
    if (ATT_IS_CLIENT_CHANGE_AWARE(conn) ||
       (op == ATT_OP_EXCHANGE_MTU_REQ) ||
       (op == ATT_OP_HANDLE_VALUE_CFM))
        return TRUE;

    if (is_op_req(op))
    {
        if (ATT_IS_SET_CHANGE_AWARE_ON_NEXT_ATT_REQ_HASH(conn))
        {
            /* Applicable for both single and multiple bearers */
            att_move_to_change_aware(ATT_ARG conn);

#ifdef INSTALL_EATT
            att_update_robust_caching_for_eatt(ATT_ARG conn);
#endif
        }
        else if (ATT_IS_SET_CHANGE_AWARE_ON_NEXT_ATT_REQ(conn) &&
                 att_is_single_att_bearer(ATT_ARG conn))
        {
            /* Applicable only for single bearer */
            att_move_to_change_aware(ATT_ARG conn);
        }
        else if (op != ATT_OP_READ_BY_TYPE_REQ)
        {
            uint16_t handle;

            handle = att_pdu_error_handle(&p_in->buf[0], p_in->len);
            att_send_error_rsp(ATT_ARG conn, op, handle, 
                                ATT_RESULT_DATABASE_OUT_OF_SYNC);
            return FALSE;
        }
    }
    /* Allow client to read Database Hash using Read by Type
     * in change unaware state.
     * so handling of ATT_OP_READ_BY_TYPE_REQ w.r.t change unaware
     * is done in ATT_HANDLER(read_by_common_req)
     */
    return TRUE;
}


/*! \brief Process pending messages from the server queue
    \param conn ATT connection structure
    \param rsp pointer to ATT_ACCESS_RSP_T structure
    \returns None.
*/
void att_process_pending_msgs(ATT_FUNC_STATE att_conn_t *conn,
                             ATT_ACCESS_RSP_T *rsp)
{
    if (conn->handler(ATT_ARG conn, rsp))
    {
        /* We are done with the packet, move the queue head to next */
        att_remove_pending_queue_head(conn);

        conn->handler = NULL;

        att_check_pending_queue(ATT_ARG conn);
    }
}


/*! \brief Handles register req from application
           This is a common function for both legacy ATT and EATT.
           Creates ATT_CID_LOCAL conn if doesn't exists.
    \param phandle Handle received in register req
    \returns ATT_RESULT_SUCCESS if success.
*/
att_result_t att_process_register_req(ATT_FUNC_STATE uint16_t phandle)
{
    att_result_t rc = ATT_RESULT_SUCCESS;
    att_conn_t *conn;

    /* check if server already exists */
    if (att_conn_find(ATT_ARG ATT_CID_LOCAL) != NULL)
    {
        rc = ATT_RESULT_ALREADY_REGISTERED;
    }

    /* add new server */
    else if ((conn = att_conn_add(ATT_ARG ATT_CID_LOCAL)) != NULL)
    {
        conn->mtu = ATT_LOCAL_CID_0_MTU;
        ATT_STATE(phandle) = phandle;
    }

    /* failed to create server */
    else
    {
        rc = ATT_RESULT_INSUFFICIENT_RESOURCES;
    }

    return rc;
}

#endif /* INSTALL_ATT_MODULE */
