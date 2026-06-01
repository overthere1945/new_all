/*******************************************************************************
Copyright (C) 2025 - 2026 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "att_private.h"
#ifdef INSTALL_SOCKET_OFFLOAD

#include "socket_offload.h"

#define ATT_CONNECT_AS_MASTER 0x08

/******************************************************************************
  att_offload_conn : Update ATT fixed channel information
******************************************************************************/
void att_offload_conn(TYPED_BD_ADDR_T *addrt,
                      l2ca_cid_t cid,
                      uint16_t mtu)
{
    att_conn_t *conn = NULL;

    for (conn = ATT_STATE(conn); conn; conn = conn->next)
    {
        if (conn->cid == cid)
        {
            ATT_LOG_DEBUG("ATT conn exists");
            conn->mtu = mtu;
            return;
        }
    }

    /* add an instance of conn */
    if ((conn = att_conn_add(ATT_ARG cid)) != NULL)
    {
        conn->flags = ATT_CONNECT_AS_MASTER;
        conn->mtu = mtu;

        tbdaddr_copy(&conn->addrt, addrt);
        ATT_LOG_INFO("ATT instance created");
    }
    else
    {
        ATT_PANIC(PANIC_ATT_INVALID_STATE, "ATT invalid state");
    }
}

ATT_HANDLER(write_common_req)
{
    uint16_t handle;
    uint16_t len;
    uint8_t op;
    
    /* Set the correct starting point */
    conn->server_in->data = &conn->server_in->buf[1];

    /* read pdu */
    handle = read_uint16(&conn->server_in->data);
    len = SERVER_IN_BUF_LEN(conn); 

    op = GET_SERVER_IN_PDU(conn);

    if (irq == NULL)
    {
        uint16_t flags = ATT_ACCESS_WRITE | ATT_ACCESS_WRITE_COMPLETE |
                         ATT_ACCESS_PERMISSION | ATT_ACCESS_ENCRYPTION_KEY_LEN;    
        uint8_t *data = NULL;

        if (len && (data = xpmalloc(len)) != NULL)
        {
            qbl_memscpy(data, len, conn->server_in->data, len);
        }

        if (op & ATT_OP_CMD_FLAG)
        {
            flags |= ATT_ACCESS_WRITE_CMD;
        }

        att_access_ind(ATT_STATE(phandle), conn->cid, handle, 
                       flags, 0 /* offs */, len, data);
        return FALSE;
    }

    ATT_LOG_INFO("ATT Access Response Opcode %x Result 0x%x", op, irq->result);

    /* send response */
    if (!(op & ATT_OP_CMD_FLAG))
    {
        if (irq->result == ATT_RESULT_SUCCESS)
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
            att_send_error_rsp(ATT_ARG conn, op, handle, irq->result);
        }
    }

    return TRUE;
}

ATT_HANDLER(read_common_req)
{
    uint16_t handle;
    uint16_t offs = 0;
    uint16_t len;
    uint8_t op = GET_SERVER_IN_OP(conn);
    uint16_t flags = ATT_ACCESS_READ | ATT_ACCESS_PERMISSION | 
                     ATT_ACCESS_ENCRYPTION_KEY_LEN;

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
            att_pkt_write(conn, irq->value, irq->size_value);
            att_pkt_send(ATT_ARG conn);
        }
        else
        {
            att_send_error_rsp(ATT_ARG conn, op, irq->handle, irq->result);
        }
        return TRUE;
    }

    /* request data from the application */
    att_access_ind(ATT_STATE(phandle), conn->cid, handle,
                   flags, offs, 0, NULL);
    return FALSE;
}


#endif

