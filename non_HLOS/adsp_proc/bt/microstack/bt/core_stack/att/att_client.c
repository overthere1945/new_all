/*******************************************************************************

Copyright (C) 2009 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\brief  Attribute Protocol Client Interface and over-the-air handlers

*******************************************************************************/
#include "att_private.h"

#ifdef INSTALL_ATT_MODULE


/* common read information request + read by type + read by group type
   request */
static att_result_t read_common_uuid_req(ATT_FUNC_STATE
                                         uint8_t op, l2ca_cid_t cid,
                                         uint16_t start, uint16_t end,
                                         att_uuid_type_t uuid_type,
                                         const uint32_t uuid[4])
{
    att_conn_t *conn;
    uint16_t i;

    /* check the request for valid cid and corresponding conn */
    if ( (conn = att_conn_find(ATT_ARG cid)) == NULL)
        return ATT_RESULT_INVALID_CID;

    else if (PEND_GET_RSP())
        return ATT_RESULT_BUSY;    

    else if (!start || start > end)
        return ATT_RESULT_INVALID_HANDLE;

    switch (uuid_type)
    {
        case ATT_UUID_NONE:
        case ATT_UUID16:
            break;

        case ATT_UUID128:
            if (att_array_is_uuid16(uuid))
            {
                uuid_type = ATT_UUID16;
            }
            break;

        default:
            return ATT_RESULT_INVALID_UUID;
    }

    if(ATT_PKT_CREATE_FAILED == att_pkt_create(conn, op))
    {
        return ATT_RESULT_INSUFFICIENT_RESOURCES;
    }
    write_uint16(&conn->out.data, start);
    write_uint16(&conn->out.data, end);

    /* add type/filter */
    switch (uuid_type)
    {
        case ATT_UUID16:
            write_uint16(&conn->out.data, (uint16_t)(uuid[0] & 0xffff));
            break;

        case ATT_UUID128:
            for (i = 4; i--; )
            {
                write_uint32(&conn->out.data, &uuid[i]);
            }
            break;

        default:
            break;
    }
        
    att_pkt_send(ATT_ARG conn);

    return ATT_RESULT_SUCCESS;
}

/* wrapper for read information request */
void att_find_info_req(ATT_FUNC_STATE
                       ATT_FIND_INFO_REQ_T *req)
{
    att_result_t rc;

    rc = read_common_uuid_req(ATT_ARG ATT_OP_FIND_INFO_REQ,
                              req->cid,
                              req->start,
                              req->end,
                              ATT_UUID_NONE, NULL);

    if (rc != ATT_RESULT_SUCCESS)
        att_find_info_cfm(req->phandle, req->cid, req->start,
                          ATT_UUID_NONE, NULL, rc);
}

/* find by type value request */
void att_find_by_type_value_req(ATT_FUNC_STATE
                                ATT_FIND_BY_TYPE_VALUE_REQ_T *req)
{
    att_conn_t *conn;
    att_result_t rc;

    /* check the request */
    if (  (conn = att_conn_find(ATT_ARG req->cid)) == NULL)
        rc = ATT_RESULT_INVALID_CID;

    else if (PEND_GET_RSP())
        rc = ATT_RESULT_BUSY;
    
    else if (!req->start || req->start > req->end)
        rc = ATT_RESULT_INVALID_HANDLE;

    else
    {
        if(ATT_PKT_CREATE_FAILED == 
             att_pkt_create_len(conn, ATT_OP_FIND_BY_TYPE_VALUE_REQ,
                (att_handlers[ATT_OP_FIND_BY_TYPE_VALUE_REQ].min + req->size_value)))
        {
            rc = ATT_RESULT_INSUFFICIENT_RESOURCES;
        }
        else
        {
            write_uint16(&conn->out.data, req->start);
            write_uint16(&conn->out.data, req->end);
            write_uint16(&conn->out.data, req->uuid);
            att_pkt_write(conn, req->value, req->size_value);

            att_pkt_send(ATT_ARG conn);
            return;
        }
    }
    att_find_by_type_value_cfm(req->phandle, req->cid, req->start, 0, rc);
}

/* wrapper for read by type request */
void att_read_by_type_req(ATT_FUNC_STATE
                          ATT_READ_BY_TYPE_REQ_T *req)
{
    att_result_t rc;

    rc = read_common_uuid_req(ATT_ARG ATT_OP_READ_BY_TYPE_REQ,
                          req->cid,
                          req->start,
                          req->end,
                          req->uuid_type,
                          req->uuid);

    if (rc != ATT_RESULT_SUCCESS)
        att_read_by_type_cfm(req->phandle, req->cid, req->start, rc, 0, NULL);
}

/* common read + read blob + read multiple request 
 *  + read multiple variable request*/
att_result_t read_common_req(ATT_FUNC_STATE
                             uint8_t op, l2ca_cid_t cid,
                             uint16_t num_handle, uint16_t *handle,
                             uint16_t offs)
{
    att_conn_t *conn;
    uint16_t len_to_write;
    uint16_t hdl_value;

    /* check the request */
    if ((conn = att_conn_find(ATT_ARG cid)) == NULL)
        return ATT_RESULT_INVALID_CID;
    
    else if (PEND_GET_RSP())
        return ATT_RESULT_BUSY;
    
    else if (!*handle)
        return ATT_RESULT_INVALID_HANDLE;

    else if (offs > ATT_LEN_MAX)
        return ATT_RESULT_INVALID_OFFSET;

    /* all ok, send the request */
    else
    {
        uint16_t pdu_len = conn->mtu;

        if ((op == ATT_OP_READ_REQ) || (op == ATT_OP_READ_BLOB_REQ))
            pdu_len = att_handlers[op].max;
        else if ((op == ATT_OP_READ_MULTI_REQ) || 
                 (op == ATT_OP_READ_MULTI_VAR_REQ))
        {
            /* num_handle is always >= 2 for read multi (var) req*/
            pdu_len = att_handlers[op].min + ((num_handle-2) * ATT_HANDLE_LEN);
        }

        len_to_write = att_pkt_create_len(conn, op, pdu_len);
        if(ATT_PKT_CREATE_FAILED == len_to_write)
        {
            return ATT_RESULT_INSUFFICIENT_RESOURCES;
        }

        num_handle = MIN(num_handle, len_to_write / 2);

        for (;num_handle;num_handle--)
        {
            hdl_value = *(handle++);
            write_uint16(&conn->out.data, hdl_value);
        }

        if (op == ATT_OP_READ_BLOB_REQ)
            write_uint16(&conn->out.data, offs);

        att_pkt_send(ATT_ARG conn);
    }

    return ATT_RESULT_SUCCESS;
}

/* wrapper for read request */
void att_read_req(ATT_FUNC_STATE
                  ATT_READ_REQ_T *req)
{
    att_result_t rc;

    rc = read_common_req(ATT_ARG ATT_OP_READ_REQ,
                         req->cid,
                         1, &req->handle,
                         0 /* offset */ );

    if (rc != ATT_RESULT_SUCCESS)
        att_read_cfm(req->phandle, req->cid, rc, 0, NULL);
}

/* wrapper for read blob */
void att_read_blob_req(ATT_FUNC_STATE
                       ATT_READ_BLOB_REQ_T *req)
{
    att_result_t rc;

    rc = read_common_req(ATT_ARG ATT_OP_READ_BLOB_REQ,
                         req->cid,
                         1, &req->handle,
                         req->offset);

    if (rc != ATT_RESULT_SUCCESS)
        att_read_blob_cfm(req->phandle, req->cid, rc, 0, NULL);
}

/* wrapper for read multiple */
void att_read_multi_req(ATT_FUNC_STATE
                        ATT_READ_MULTI_REQ_T *req)
{
    att_result_t rc;

    if (req->size_handles >= 2) /* 2 is minm no handles in read multi req */
    {
        rc = read_common_req(ATT_ARG ATT_OP_READ_MULTI_REQ,
                             req->cid,
                             req->size_handles, req->handles,
                             0 /* offset */);
    }
    else
        rc = ATT_RESULT_INVALID_PARAMS;

    if (rc != ATT_RESULT_SUCCESS)
        att_read_multi_cfm(req->phandle, req->cid, rc, 0, NULL);
}

/* wrapper for read by type request */
void att_read_by_group_type_req(ATT_FUNC_STATE
                                ATT_READ_BY_GROUP_TYPE_REQ_T *req)
{
    att_result_t rc;

    switch (req->group_type)
    {
        case ATT_UUID16:
        case ATT_UUID128:
            rc = read_common_uuid_req(ATT_ARG ATT_OP_READ_BY_GROUP_TYPE_REQ,
                                      req->cid,
                                      req->start,
                                      req->end,
                                      req->group_type,
                                      req->group);
            break;

        default:
            rc = ATT_RESULT_INVALID_UUID;
    }

    if (rc != ATT_RESULT_SUCCESS)
        att_read_by_group_type_cfm(req->phandle, req->cid, rc, req->start,
                                   0, 0, NULL);
}

/* common write + write blob + prepare write */
static att_result_t write_common_req(ATT_FUNC_STATE
                                     uint8_t op, l2ca_cid_t cid,
                                     uint16_t handle, uint16_t offs,
                                     uint16_t size_value, uint8_t *value,
                                     bool from_stream)
{
    att_conn_t *conn;
    uint16 len;
    QBL_UNUSED(from_stream);

    /* check the request */
    if ((conn = att_conn_find(ATT_ARG cid)) == NULL)
        return ATT_RESULT_INVALID_CID;

    else if (PEND_GET_RSP() && !ATT_IS_OP_CMD(op))
        return ATT_RESULT_BUSY;

    else if (!handle)
        return ATT_RESULT_INVALID_HANDLE;

    else if (offs > ATT_LEN_MAX) /* offs == 0 if op != blob */
        return ATT_RESULT_INVALID_OFFSET;

    /* all ok, send the request */
    else
    {
        len = att_get_write_pdu_len(op, size_value, from_stream);

        if(ATT_PKT_CREATE_FAILED == att_pkt_create_len(conn, op, len))
        {
            return ATT_RESULT_INSUFFICIENT_RESOURCES;
        }
        write_uint16(&conn->out.data, handle);

        if (op == ATT_OP_PREPARE_WRITE_REQ)
            write_uint16(&conn->out.data, offs);

#ifdef INSTALL_STREAM_MODULE
        if(ATT_IS_OP_WRITE_CMD(op) && from_stream)
        {
            att_result_t result = ATT_RESULT_SUCCESS;

            /* Send the MBLK packet */
            if (ATT_RESULT_SUCCESS != att_pkt_send_mblk(ATT_ARG conn, (MBLK_T *)value))
            {
                result = ATT_RESULT_BUSY;
            }

            return result;
        }
        else
#endif /* INSTALL_STREAM_MODULE */
        {
            (void)att_pkt_write(conn, value, size_value);
        }

        /* This could be a write command and that might not be successfully sent */
        return(att_pkt_send(ATT_ARG conn));
    }
}

/* wrapper for write command/write request */
void att_write_req(ATT_FUNC_STATE
                   ATT_WRITE_REQ_T *req)
{
    att_result_t rc;
    uint8_t flags = 0;
    att_conn_t *conn = NULL;

    rc = ATT_RESULT_SUCCESS;
    
    /* Signed writes should be done on un-encrypted
    * link. This should be the first point we check. If
    * it is performed on an encrypted link we will reject
    * the request.
    */
    if(req->flags & ATT_WRITE_SIGNED 
            && req->cid != ATT_CID_LOCAL)
    {
        /* check the request */
        if ((conn = att_conn_find(ATT_ARG req->cid)) == NULL)
            rc = ATT_RESULT_INVALID_CID;
    }

    /* NOTE: Magically ATT_WRITE_COMMND flags is equal to ATT_OP_CMD_FLAG,
       and ATT_WRITE_SIGNED is equal to ATT_OP_AUTH_FLAG. */

    /* Write command, may include auth flag as well */
    if (req->flags & ATT_WRITE_COMMAND)
    {
        /* Allow auth flag only in write command */
        flags = req->flags & (ATT_OP_CMD_FLAG | ATT_OP_AUTH_FLAG);
    }

    /* signed write, can only be a command so let's force it */
    else if (req->flags & ATT_WRITE_SIGNED)
    {
        flags = ATT_OP_CMD_FLAG | ATT_OP_AUTH_FLAG;
    }

    if(rc == ATT_RESULT_SUCCESS)
    {

        rc = write_common_req(
            ATT_ARG
            (uint8_t)(ATT_OP_WRITE_REQ | flags),
            req->cid,
            req->handle,
            0 /* offset */,
            req->size_value, 
            req->value,
            FALSE);
    }

    /* data signing to be done, don't send the CFM yet */
    if (rc == ATT_RESULT_INSUFFICIENT_AUTHENTICATION)
        return;
            
    /* in case of write command, success indicates that the command was sent,
       otherwise send only errors */
    else if ((flags & ATT_OP_CMD_FLAG) || rc != ATT_RESULT_SUCCESS)
        att_write_cfm(req->phandle, req->cid, rc);
}

/*! \brief Handles the command invoking write command with application tracking 
           context and would confirm with the database handle and context back.
           Returns ATT_WRITE_CMD_CFM indicating success/failure with handle and 
           context.
    \param req pointer to the write data with attribute handle and application 
           context.
    \param from_stream boolean flag to distinguish write command is sent from 
           stream or primitive path.

    \returns void
*/
void att_write_track_cmd(ATT_FUNC_STATE
                   ATT_WRITE_CMD_T *req,
                   bool from_stream)
{
    att_result_t rc = ATT_RESULT_INVALID_CID;
    att_conn_t *conn;

    /* Signed writes and write req not supported on this API */
    if(req->flags != ATT_WRITE_COMMAND)
    {
        rc = ATT_RESULT_INVALID_PARAMS;
    }
    else if (((conn = att_conn_find(ATT_ARG req->cid)) != NULL) &&
              from_stream &&
              att_stream_pdu_exceeds_mtu(conn, (MBLK_T*)req->value))
    {
        /* Command from stream exceeds the MTU */
        rc = ATT_RESULT_INVALID_PARAMS;
    }
    else if (conn !=NULL)
    {
        rc = write_common_req(
            ATT_ARG
            (uint8_t)(ATT_OP_WRITE_REQ | req->flags),
            req->cid,
            req->handle,
            0 /* offset */,
            req->size_value, 
            req->value,
            from_stream);
    }

    if(!from_stream)
        att_write_track_cfm(req, rc);

}

/* wrapper for write blob request */
void att_prepare_write_req(ATT_FUNC_STATE
                           ATT_PREPARE_WRITE_REQ_T *req)
{
    att_result_t rc;

    /* Local operations do not support write queue */
    if (req->cid == ATT_CID_LOCAL)
    {
        rc = ATT_RESULT_REQUEST_NOT_SUPPORTED;
    }
    else
    {    
        rc = write_common_req(
            ATT_ARG
            ATT_OP_PREPARE_WRITE_REQ,
            req->cid,
            req->handle,
            req->offset,
            req->size_value, 
            req->value,
            FALSE);
    }

    /* send error, but ignore value as it was not sent at all */
    if (rc != ATT_RESULT_SUCCESS)
        att_prepare_write_cfm(req->phandle, req->cid, req->handle,
                              req->offset, rc, 0, NULL);
}

void att_execute_write_req(ATT_FUNC_STATE
                           ATT_EXECUTE_WRITE_REQ_T *req)
{
    att_result_t rc;
    att_conn_t *conn;

    /* Local operations do not support write queue */
    if (req->cid == ATT_CID_LOCAL)
        rc = ATT_RESULT_REQUEST_NOT_SUPPORTED;

    else if ((conn = att_conn_find(ATT_ARG req->cid)) == NULL)
        rc = ATT_RESULT_INVALID_CID;

    else if (PEND_GET_RSP())
        rc = ATT_RESULT_BUSY;
    
    else
    {
        if(ATT_PKT_CREATE_FAILED == 
                            att_pkt_create(conn, ATT_OP_EXECUTE_WRITE_REQ))
        {
            rc = ATT_RESULT_INSUFFICIENT_RESOURCES;
        }
        else
        {
            write_uint8(&conn->out.data, (uint8_t)req->flags);
            att_pkt_send(ATT_ARG conn);
            return;
        }
    }
    att_execute_write_cfm(req->phandle, req->cid, 0 /* handle */, rc);
}

void att_handle_value_rsp(ATT_FUNC_STATE
                          ATT_HANDLE_VALUE_RSP_T *req)
{
    att_conn_t *conn;

    if ((conn = att_conn_find(ATT_ARG req->cid)) == NULL)
    {
        return;
    }

    else if (!(conn->pend & PEND_HANDLE_RSP))
    {
        return;
    }

    /* Fails hard on not able to create the packet of length one */
    att_pkt_create(conn, ATT_OP_HANDLE_VALUE_CFM);
    att_pkt_send(ATT_ARG conn);
}

/*
 * Responses from the server
 ****************************************************************************/
ATT_HANDLER(error_rsp)
{
    uint8_t req_op;
    uint16_t handle;
    uint8_t rc;
    
    ATT_HANDLER_IRQ_UNUSED

    /* read pdu */
    req_op = read_uint8(&conn->client_in->data);
    handle = read_uint16(&conn->client_in->data);
    rc = read_uint8(&conn->client_in->data);

    /* send error to the application */
    if (att_pend_check(conn, (uint8_t)(req_op | ATT_OP_RSP_MASK)))
    {
        att_error_cfm(ATT_STATE(phandle), conn, req_op, handle, rc);
    }
#ifdef GATT_OFFLOAD
    if (rc == ATT_RESULT_DATABASE_OUT_OF_SYNC)
    {
        att_error_ind(ATT_STATE(phandle), conn->cid, ATT_ERROR_DB_OUT_OF_SYNC);
    }
#endif
    return TRUE;
}


/* Read information response from server */
ATT_HANDLER(find_info_rsp)
{
    uint16_t handle;
    ATT_UUID_T uuid;
    uint16_t len;
    uint16_t dlen;
    att_uuid_type_t uuid_type;
    
    ATT_HANDLER_IRQ_UNUSED

    /* read pdu */
    switch (read_uint8(&conn->client_in->data))
    {
        case ATT_FORMAT_UUID16:
            uuid_type = ATT_UUID16;
            dlen = 4; /* 4 bytes of data per entry */
            break;

        case ATT_FORMAT_UUID128: /* UUID128 */
            uuid_type = ATT_UUID128;
            dlen = 18; /* 18 bytes of data per entry */
            break;

        default:
            /* invalid packet, set length to something that will fail
             * length check later and cause disconnect and cleanup */
            dlen = 0xffff;
            uuid_type = ATT_UUID_NONE; /* keep compiler happy */
    }

    len = CLIENT_IN_BUF_LEN(conn);
    
    /* check packet length */
    if (len % dlen)
    {
        return TRUE;
    }

    while (len)
    {
        att_result_t rc;
        
        handle = read_uint16(&conn->client_in->data);
        (void) att_read_uuid(conn->client_in, &uuid, uuid_type);
        len -= dlen;
        
        if (len)
        {
            rc = ATT_RESULT_SUCCESS_MORE;
        }
        else
        {    
            rc = ATT_RESULT_SUCCESS;
        }
        
        att_find_info_cfm(ATT_STATE(phandle), conn->cid, handle, uuid_type,
                          &uuid, rc);
    }

    return TRUE;
}

/* find by type value response from server */
ATT_HANDLER(find_by_type_value_rsp)
{
    uint16_t len;
    
    ATT_HANDLER_IRQ_UNUSED

    /* read pdu */
    len = CLIENT_IN_BUF_LEN(conn);

    /* check packet length */
    if (len % 4)
    {
        return TRUE;
    }

    while (len)
    {
        att_result_t rc;
        uint16_t handle;
        uint16_t end;

        handle = read_uint16(&conn->client_in->data);
        end = read_uint16(&conn->client_in->data);

        len -= 4;        
        
        if (len)
        {
            rc = ATT_RESULT_SUCCESS_MORE;
        }
        else
        {    
            rc = ATT_RESULT_SUCCESS;
        }

        att_find_by_type_value_cfm(ATT_STATE(phandle), conn->cid, handle, end,
                                   rc);
    }

    return TRUE;
}

/* Read by type response from server */
ATT_HANDLER(read_by_type_rsp)
{
    uint16_t handle;
    uint16_t len;
    uint16_t dlen;
    uint8_t *data = NULL;

    ATT_HANDLER_IRQ_UNUSED

    /* read pdu */
    dlen = read_uint8(&conn->client_in->data);
    len = CLIENT_IN_BUF_LEN(conn);

    /* check packet length. Packet should atleast contain attribute handle */
    if (dlen < ATT_HANDLE_LEN || len % dlen)
    {
        att_read_by_type_cfm(ATT_STATE(phandle), conn->cid, 0,
                ATT_RESULT_INVALID_LENGTH, 0, NULL);
        return TRUE;
    }

    dlen -= 2; /* remove handle size from datalen */
    
    while (len)
    {
        att_result_t rc;
    
        len -= 2 + dlen;
        
        handle = read_uint16(&conn->client_in->data);

        if (dlen && (data = xpmalloc(dlen)) != NULL)
        {
            qbl_memscpy(data, dlen, conn->client_in->data, dlen);
            conn->client_in->data += dlen;
        }
        else/* break the loop on failure of xpmalloc() */
        {
            att_read_by_type_cfm(ATT_STATE(phandle), conn->cid, 0,
                ATT_RESULT_SUCCESS, 0, NULL);
            break;
        }

        if (len)
        {
            rc = ATT_RESULT_SUCCESS_MORE;
        }
        else
        {    
            rc = ATT_RESULT_SUCCESS;
        }
        
        att_read_by_type_cfm(ATT_STATE(phandle), conn->cid, handle, rc, dlen,
                             data);
    }

    return TRUE;
}

/* Handler function for these PDUs from server
   Read Response, 
   Read multi response,
   Read Multi variable Response,
   Read blob Response,
   Multiple Handle Value Notification,
   Handle value Notification
   Handle value Indication
*/
ATT_HANDLER(read_common_rsp)
{
    uint16_t handle = 0;
    uint16_t len;
    uint8_t *data = NULL;

    ATT_HANDLER_IRQ_UNUSED

    /* read pdu */

    if (GET_CLIENT_IN_OP(conn) == ATT_OP_HANDLE_VALUE_NOT ||
        GET_CLIENT_IN_OP(conn) == ATT_OP_HANDLE_VALUE_IND)
    {    
        handle = read_uint16(&conn->client_in->data);
    }

    len = CLIENT_IN_BUF_LEN(conn);
    
    if (len && (data = xpmalloc(len)) != NULL)
    {
        qbl_memscpy(data, len, conn->client_in->data, len);
        conn->client_in->data += len;
    }

    switch (GET_CLIENT_IN_OP(conn))
    {
        case ATT_OP_READ_RSP:
            att_read_cfm(ATT_STATE(phandle), conn->cid, ATT_RESULT_SUCCESS,
                         len, data);
            break;
            
        case ATT_OP_READ_BLOB_RSP:
            att_read_blob_cfm(ATT_STATE(phandle), conn->cid,
                              ATT_RESULT_SUCCESS, len, data);
            break;

        case ATT_OP_READ_MULTI_RSP:
            att_read_multi_cfm(ATT_STATE(phandle), conn->cid,
                               ATT_RESULT_SUCCESS, len, data);
            break;
            
        case ATT_OP_HANDLE_VALUE_IND:
            att_handle_value_ind(ATT_STATE(phandle), conn->cid, handle,
                                 ATT_HANDLE_VALUE_INDICATION, len, data);
            break;

#ifdef INSTALL_EATT
        case ATT_OP_MULTI_HANDLE_VALUE_NOT:
            /* For application not supporting MHVN, don't send it.
             * This is possible for legacy ATT bearer */
            if (ATT_STATE(eatt_registered))
                att_multi_handle_value_ntf_ind(ATT_STATE(phandle),
                                               conn->cid,
                                               len, data);
            else if (data)
                pfree(data);
            break;

        case ATT_OP_READ_MULTI_VAR_RSP:
            att_read_multi_var_cfm(ATT_STATE(phandle), conn->cid,
                                   ATT_RESULT_SUCCESS, 0, len, data);
            break;
#endif /* INSTALL_EATT */

        case ATT_OP_HANDLE_VALUE_NOT:
            {
                att_handle_value_ind(ATT_STATE(phandle), conn->cid, handle,
                                     ATT_HANDLE_VALUE_NOTIFICATION, len, data);
                return TRUE;
            }
            /* fall-through */
        default :
            if(data) 
            {
                pfree(data);
            }
            break;
    }

    return TRUE;
}

/* Read By Group Type response from server */
ATT_HANDLER(read_by_group_type_rsp)
{
    uint16_t handle;
    uint16_t end;
    uint16_t len;
    uint16_t dlen;
    uint8_t *data = NULL;

    ATT_HANDLER_IRQ_UNUSED

    /* read pdu */
    dlen = read_uint8(&conn->client_in->data);
    len = CLIENT_IN_BUF_LEN(conn);

    /* check packet length. Packet should atleast contain 4 octets of
       attribute data list comprising of attr handle (2 octets) and
       end group handle (2 octets).
    */
    if (dlen < (ATT_HANDLE_LEN + 2) || len % dlen)
    {
        att_read_by_group_type_cfm(ATT_STATE(phandle), conn->cid, 
                ATT_RESULT_INVALID_LENGTH, 0, 0, 0, NULL);
        return TRUE;
    }

    dlen -= 4; /* remove handle and end sizes from datalen */

    while (len)
    {
        att_result_t rc;
        
        len -= 4 + dlen;
        
        handle = read_uint16(&conn->client_in->data);
        end = read_uint16(&conn->client_in->data);

        if (dlen && (data = xpmalloc(dlen)) != NULL)
        {
            qbl_memscpy(data, dlen, conn->client_in->data, dlen);
            conn->client_in->data += dlen;
        }
        else /* break the loop on failure of xpmalloc() */
        {
            att_read_by_group_type_cfm(ATT_STATE(phandle), conn->cid, 
                ATT_RESULT_SUCCESS, 0, 0, 0, NULL);
            break;
        }

        if (len)
        {
            rc = ATT_RESULT_SUCCESS_MORE;
        }
        else
        {    
            rc = ATT_RESULT_SUCCESS;
        }
        
        att_read_by_group_type_cfm(ATT_STATE(phandle), conn->cid, rc,
                                   handle, end, dlen, data);
    }

    return TRUE;
}

/* Write, Write blob or Prepare write response from server */
ATT_HANDLER(write_common_rsp)
{
    ATT_HANDLER_IRQ_UNUSED

    /* read pdu */
    
    switch (GET_CLIENT_IN_OP(conn))
    {
        case ATT_OP_WRITE_RSP:
            att_write_cfm(ATT_STATE(phandle), conn->cid, ATT_RESULT_SUCCESS);
            break;

        case ATT_OP_PREPARE_WRITE_RSP:
        {
            uint16_t handle;
            uint16_t offs;
            uint16_t len;
            uint8_t *data = NULL;
    
            handle = read_uint16(&conn->client_in->data);
            offs = read_uint16(&conn->client_in->data);
            len = CLIENT_IN_BUF_LEN(conn);
                
            if (len && (data = xpmalloc(len)) != NULL)
            {
                qbl_memscpy(data, len, conn->client_in->data, len);
                conn->client_in->data += len;
            }
            
            att_prepare_write_cfm(ATT_STATE(phandle), conn->cid, handle, offs,
                                  ATT_RESULT_SUCCESS, len, data);
            break;
        }
    }

    return TRUE;
}

/* Execute write response from server */
ATT_HANDLER(execute_write_rsp)
{
    ATT_HANDLER_IRQ_UNUSED

    att_execute_write_cfm(ATT_STATE(phandle), conn->cid, 0 /* handle */,
                          ATT_RESULT_SUCCESS);

    return TRUE;
}

#endif /* INSTALL_ATT_MODULE */
