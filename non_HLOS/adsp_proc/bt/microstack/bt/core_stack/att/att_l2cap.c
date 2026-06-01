/*******************************************************************************

Copyright (C) 2009 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\brief  Attribute Protocol interface to L2CAP.
*******************************************************************************/
#include "att_private.h"

#ifdef INSTALL_ATT_MODULE

#ifdef ENABLE_EVENT_COUNTER_REPORTING
uint32_t    AttEventCounter = INVALID_EVENT_COUNTER_VALUE;
#endif

#define L2CAP_CONNECT_ERROR(x)           ((x) == L2CA_MISC_FAILED        || \
                                          (x) == L2CA_MISC_OUT_OF_MEM    || \
                                          (x) == L2CA_MISC_NO_CONNECTION)

#define MIN_PDU_SIZE_WRITE_CMD              (3)
#define MIN_PDU_SIZE_HANDLE_VALUE_NOTIFY    (3)

/* Prototype of ATT PDU handler */
#define HANDLER(min, max, func)    { min, max, att_handle_##func }
/* Dummy table entries for unsupported/reserved PDUs */
#define INVALID()                  { 0, 0, NULL }

/* ATT handler table contains all the supported ATT pdus types. 
 * For each supported PDUs minimum and maximum allowed sizes are listed and it 
 * will checked against every packet that is recieved before proceeding to 
 * packet processing. Function handler for each of the op codes are listed, some
 * set of ATT PDUs have common function handlers.
 */
const att_handler_t att_handlers[] =
{
         /* min, max, handler */
    INVALID(),                                /* 00 */
    HANDLER(5,  5,  error_rsp),               /* 01 Error rsp */
    INVALID(),                                /* 02 Exchange MTU req */
    INVALID(),                                /* 03 Exchange MTU rsp */
    INVALID(),                                /* 04 Read Information rq */
    HANDLER(2,  0,  find_info_rsp),           /* 05 Read Information rsp */
    INVALID(),                                /* 06 Find By Type Value req */
    HANDLER(1,  0,  find_by_type_value_rsp),  /* 07 Find By Type Value rsp */
    HANDLER(7, 21,  read_by_common_req),      /* 08 Read By Type req */
    HANDLER(4,  0,  read_by_type_rsp),        /* 09 Read By Type rsp */    
    HANDLER(3,  3,  read_common_req),         /* 0a Read req */
    HANDLER(1,  0,  read_common_rsp),         /* 0b Read rsp */    
    HANDLER(5,  5,  read_common_req),         /* 0c Read Blob req */
    HANDLER(1,  0,  read_common_rsp),         /* 0d Read Blob rsp */
    INVALID(),                                /* 0e Read Multiple req */
    HANDLER(1,  0,  read_common_rsp),         /* 0f Read Multiple rsp */
    INVALID(),                                /* 10 Read By Group Type req */
    HANDLER(6,  0,  read_by_group_type_rsp),  /* 11 Read By Group Type rsp */
    HANDLER(3,  0,  write_common_req),        /* 12 Write req */
    HANDLER(1,  1,  write_common_rsp),        /* 13 Write rsp */
    INVALID(),                                /* 14 */
    INVALID(),                                /* 15 */
    HANDLER(5,  0,  prepare_write_req),       /* 16 Prepare Write req */
    HANDLER(5,  0,  write_common_rsp),        /* 17 Prepare Write rsp */
    HANDLER(2,  2,  execute_write_req),       /* 18 Execute Write req */
    HANDLER(1,  1,  execute_write_rsp),       /* 19 Execute Write rsp */
    INVALID(),                                /* 1a */
    HANDLER(3,  0,  read_common_rsp),         /* 1b Handle Value notif */
    INVALID(),                                /* 1c */
    HANDLER(3,  0,  read_common_rsp),         /* 1d Handle Value ind */
    HANDLER(1,  1,  handle_value_cfm),        /* 1e Handle Value cfm */
#ifdef INSTALL_EATT
    INVALID(),                                /* 1f */
    HANDLER(5,  0,  read_multi_req),          /* 20 Read Multiple var req */
    HANDLER(5,  0,  read_common_rsp),         /* 21 Read Multiple var rsp */
    INVALID(),                                /* 22 */
    HANDLER(9,  0,  read_common_rsp),         /* 23 Multi Handle Value notif */
#endif
};

#define ATT_UUID_INVALID        (~ATT_UUID_NONE)

att_conn_t *att_conn_add(ATT_FUNC_STATE
                         l2ca_cid_t cid)
{
    att_conn_t *conn = zpnew(att_conn_t);

    if(conn)
    {
        conn->next = ATT_STATE(conn);
        ATT_STATE(conn) = conn;
        conn->cid = cid;
    }
    return conn;
}


att_conn_t *att_conn_find(ATT_FUNC_STATE
                           l2ca_cid_t cid)
{
    att_conn_t *conn;

    for (conn = ATT_STATE(conn); conn; conn = conn->next)
    {
        if (conn->cid == cid)
            break;
    }
    
    return conn;
}


/*! \brief Remove ATT connection structure.
           Cleans up ATT connection record corresponding to supplied cid.
    \param cid  l2cap cid of ATT connection to be removed.
    \returns None.
*/
void att_conn_rm(ATT_FUNC_STATE
                 l2ca_cid_t cid)
{
    att_conn_t *conn, **cp;

    for (cp = &ATT_STATE(conn); (conn = *cp) != NULL; cp = &conn->next)
    {
        if (conn->cid == cid)
        {
            /* update server list */
            *cp = conn->next;
            
            /* clear write queue and common info when last bearer is removed */
            if ((conn->cid != ATT_CID_LOCAL) &&
                 att_is_single_att_bearer(ATT_ARG conn))
            {
                att_wq_free(conn);
                pfree(conn->common);
            }


            /* Clean up unprocessed incoming data */
            att_clean_pending_queue(conn);

            pfree(conn->out.buf);

            pfree(conn);
            break;
        }
    }
}

/*! \brief Finds the connection address for ATT connection
    \param conn  pointer to ATT connection structure.
    \param conn_addr connection address.
    \returns TRUE if operation is successful, FALSE if operation fails
             or connection is specific to local CID.
*/
bool_t att_conn_get_addr(att_conn_t *conn, TYPED_BD_ADDR_T *conn_addr)
{
    bool_t result = FALSE;

    if (conn->cid != ATT_CID_LOCAL)
    {
#if !defined(BUILD_FOR_HOST) || defined(BLUESTACK_HOST_IS_APPS)
        result = L2CA_GetBdAddrFromCid(conn->cid, conn_addr);
#else
        tbdaddr_copy(conn_addr, &conn->addrt);
        result = TRUE;
#endif
    }
    return result;
}


/*! \brief Allocates outgoing data packet of a connection.
          Opcodes with packet length less than or equal to 
          the length of ATT_OP_ERROR_RSP packet (5 octets) would fail hard as 
          such a failure would be fatal to operation. All other allocation shall
          fail softly and an appropriate error code relayed back.

    \param conn  pointer to ATT connection structure.
    \param op  opcode to create the needed packet.
    \returns Length of bytes available in buffer allocated.
*/
uint16_t att_pkt_create(att_conn_t *conn, uint8_t op)
{
    uint16_t len;

    if ((op & ATT_OP_MASK) >= (sizeof(att_handlers) / sizeof(att_handler_t)))
        return ATT_PKT_CREATE_FAILED;

    /* For PDUs where the length is known/fixed, allocate the max PDU size.
     * If the length is not fixed, allocate MTU size */
    if ((len = att_handlers[op & ATT_OP_MASK].max) == 0)
        len = conn->mtu;

    return att_pkt_create_len(conn, op, len);
}

uint16_t att_pkt_create_len(att_conn_t *conn, uint8_t op, uint16_t len)
{
    uint8_t *data;
    
    /* packet pending, just return bytes left in the buffer */
    if (conn->out.buf)
    {
        return (uint16_t)(conn->mtu - (conn->out.data - conn->out.buf));
    }

    if (len > conn->mtu)
    {
        return ATT_PKT_CREATE_FAILED;
    }

    if((op == ATT_OP_ERROR_RSP ) ||
       (op == ATT_OP_EXECUTE_WRITE_RSP) ||
       (op == ATT_OP_HANDLE_VALUE_CFM))
    {
        if(ATT_OP_ERROR_RSP == op)
            len = ATT_OP_ERROR_RSP_LEN;
        else /* Note the response len for execute write rsp and value cfm is 1 */
            len = ATT_OP_HANDLE_VALUE_CFM_LEN;
        /* Fail hard in case of a small allocation not available */
        data = pmalloc(len);
    }
    else
    {
        /* fail soft on cases those are not critical */
        data = xpmalloc(len);
    }

    conn->out.buf = conn->out.data = data;

    if(data)
    {
        /* store opcode */
        write_uint8(&conn->out.data, op);

        /* bytes available in the buffer */
        return (uint16_t)(len - (conn->out.data - conn->out.buf));
    }
    else
    {
        return ATT_PKT_CREATE_FAILED;
    }
}

/*! \brief Frees outgoing data packet of a connection.
          This function is called when data in outgoing buffer is no longer 
          relevant.
    \param conn  pointer to ATT connection structure.
    \returns None.
*/
void att_out_pkt_free(att_conn_t *conn)
{
    /* destroy outgoing packet */
    if (conn->out.data)
    {
        pfree(conn->out.buf);
        conn->out.buf = conn->out.data = NULL;
    }
}

/****************************************************************************
NAME
    att_pkt_write - write data to the end of outgoing packet

*/
uint16_t att_pkt_write(att_conn_t *conn,
                       const uint8_t *value,
                       uint16_t size_value)
{
    uint16_t len;

    /* make sure not to copy over the buffer */
    len = (uint16_t)MIN(conn->mtu - (conn->out.data - conn->out.buf),
              size_value);
    
    qbl_memscpy(conn->out.data, len, value, len);
    conn->out.data += len;

    /* return bytes left on the buffer */
    return (uint16_t)(conn->mtu - (conn->out.data - conn->out.buf));
}

/****************************************************************************
NAME
    att_get_total_available_app_credits - Gets the available application credits
    in the system for a specific credit type

    Application credits could be for stream path or prim path. This is the 
    number of ATT cmd/notifications application(prim/stream) can send to ATT.
*/
uint16_t att_get_total_available_app_credits(ATT_FUNC_STATE 
                                             att_credit_type_t credit_type)
{
    int16_t max_att_credits;
    int16_t app_credits;

    /* Streams always uses small MTU band
     * Prim can use either small or large MTU band, depending on payload size */
    max_att_credits = (credit_type == ATT_CREDIT_TYPE_PRIM_LARGE_BAND) ?
                                      ATT_MAX_CREDIT_FOR_LARGE_BAND_MTU :
                                      ATT_MAX_CREDIT_FOR_SMALL_BAND_MTU ;

    /* Get total available ATT credit, temporarily store it in app_credits */
    app_credits = max_att_credits - 
                    att_total_consumed_att_credits(ATT_ARG credit_type);

    if (app_credits > 0)
    {
        /* Convert ATT credit to App credit */
        app_credits /= (credit_type == ATT_CREDIT_TYPE_STREAM) ? 
                            ATT_NUM_CREDITS_PER_STREAM : 
                            ATT_NUM_CREDITS_PER_PRIM;
    }
    else
        app_credits = 0;

    return (uint16_t)app_credits;
}


/*! \brief ATT credits consumed for a given credit type on all connections.
    A pointer to the ATT connection structure may be passed to this function.
    \param credit_type Credit type for which consumed credits is calculated

    \returns .total consumed/reserved ATT credits across all connections.
*/
uint16_t att_total_consumed_att_credits(ATT_FUNC_STATE 
                                        att_credit_type_t credit_type)
{
    att_conn_t *conn;
    uint8_t att_credits = 0;

    /* Note:
     * ATT credits for connections on ATT_CID_LOCAL CID shall not be counted
     */
    for (conn = ATT_STATE(conn); conn; conn = conn->next)
    {
        /* Skip the connection for Local CID */
        if (conn->cid == ATT_CID_LOCAL)
            continue;

        if (credit_type == ATT_CREDIT_TYPE_PRIM_LARGE_BAND)
        {
            if (conn->large_band_credits > 0)
                att_credits += conn->large_band_credits;
        }
        else /* Credit type Prim small and Stream */
        {
            if (conn->small_band_credits > 0)
                att_credits += conn->small_band_credits;
        }

        /* If no credits are consumed by this connection, reserve mandatory 
         * credits for this connection.  
         */
        if (ATT_MANDATORY_CREDIT_AVAILABLE(conn))
        {
            /* account for mandatory credits depending on credit type */
            if (credit_type == ATT_CREDIT_TYPE_STREAM)
                att_credits += ATT_NUM_CREDITS_PER_STREAM;
            else /* Credit type Prim small and large */
                att_credits += ATT_NUM_CREDITS_PER_PRIM;
        }
    }
    return att_credits;
}

/*! \brief Consumes the ATT credits based on credit type for a connection
    \param conn A pointer to the ATT connection structure
    \param credit_type Credits are consumed as per credit type

    \returns None.
*/
static void att_conn_consume_att_credits(att_conn_t *conn, 
                                         att_credit_type_t credit_type)
{
    if (credit_type == ATT_CREDIT_TYPE_STREAM)
        conn->small_band_credits += ATT_NUM_CREDITS_PER_STREAM;
    else if (credit_type == ATT_CREDIT_TYPE_PRIM_LARGE_BAND)
        conn->large_band_credits += ATT_NUM_CREDITS_PER_PRIM;
    else
        conn->small_band_credits += ATT_NUM_CREDITS_PER_PRIM;
}

/*! \brief Releases the remaining ATT credits when L2CAP_DATAWRITE_CFM is recvd
           (stage 2)
    \param conn A pointer to the ATT connection structure
    \param is_large_band Set to TRUE if DATAWRITE_CFM is recvd for large band

    \returns None.
*/
static void att_conn_release_remaining_att_credits(att_conn_t *conn,
                                                   bool_t is_large_band)
{
#ifdef ATT_GLOBAL_STATE
    if (is_large_band)
    {
        if (conn->large_band_credits >= 1)
            conn->large_band_credits -= 1;
    }
    else
    {
        if (conn->small_band_credits >= 1)
            conn->small_band_credits -= 1;
    }
#else
    /* Streams not considered since it is not supported on Synergy 
     * Distributed credit release not supported, release the complete credits */
    if (is_large_band)
    {
        if (conn->large_band_credits >= ATT_NUM_CREDITS_PER_PRIM)
            conn->large_band_credits -= ATT_NUM_CREDITS_PER_PRIM;
    }
    else
    {
        if (conn->small_band_credits >= ATT_NUM_CREDITS_PER_PRIM)
            conn->small_band_credits -= ATT_NUM_CREDITS_PER_PRIM;
    }
#endif
}

#ifdef ATT_GLOBAL_STATE
/*! \brief Releases partial ATT credits when MBLK is destroyed (stage 1).
    \param mblk A pointer to MBLK which is destroyed
    \param credit_type Credit type associated with MBLK

    \returns None.
*/
static void att_conn_release_partial_att_credits(MBLK_T *mblk, 
                                                 att_credit_type_t credit_type)
{
    /* Distributed credit release is supported only for BlueCore
     * and HydraCore */
    const uint16_t *key = mblk_get_metadata(mblk);
    att_conn_t *conn = att_conn_find(*key);

    if (conn == NULL)
        return;

    if (credit_type == ATT_CREDIT_TYPE_STREAM)
    {
        if (conn->small_band_credits >= ATT_NUM_RELEASE_PARTIAL_CREDITS_STREAM)
           conn->small_band_credits -= ATT_NUM_RELEASE_PARTIAL_CREDITS_STREAM;
    }
    else if (credit_type == ATT_CREDIT_TYPE_PRIM_LARGE_BAND)
    {
        if (conn->large_band_credits >= ATT_NUM_RELEASE_PARTIAL_CREDITS_PRIM)
            conn->large_band_credits -= ATT_NUM_RELEASE_PARTIAL_CREDITS_PRIM;
    }
    else
    {
        if (conn->small_band_credits >= ATT_NUM_RELEASE_PARTIAL_CREDITS_PRIM)
            conn->small_band_credits -= ATT_NUM_RELEASE_PARTIAL_CREDITS_PRIM;
    }
}

static void mblk_destroyed_stream(MBLK_T *mblk)
{
    att_conn_release_partial_att_credits(mblk, ATT_CREDIT_TYPE_STREAM);
}

static void mblk_destroyed_small_band(MBLK_T *mblk)
{
    att_conn_release_partial_att_credits(mblk, ATT_CREDIT_TYPE_PRIM_SMALL_BAND);
}

static void mblk_destroyed_large_band(MBLK_T *mblk)
{
    att_conn_release_partial_att_credits(mblk, ATT_CREDIT_TYPE_PRIM_LARGE_BAND);
}
#endif

static att_credit_type_t att_get_credit_type(uint16_t len, bool_t is_stream)
{
    if (is_stream)
        return ATT_CREDIT_TYPE_STREAM;
    else if (len > ATT_CREDIT_BAND_SMALL)
        return ATT_CREDIT_TYPE_PRIM_LARGE_BAND;
    else
        return ATT_CREDIT_TYPE_PRIM_SMALL_BAND;
}

/*! \brief Flood defense for ATT commands and notifications
    \param conn A pointer to the ATT connection structure
    \param len payload len for prim path, ATT header len for stream path
    \param from_stream Set to TRUE for notifications/cmd from stream path

    \returns MBLK.
*/
static MBLK_T * att_pkt_create_with_flood_defense(ATT_FUNC_STATE 
                                                  att_conn_t *conn,
                                                  uint16_t len,
                                                  bool_t from_stream)
{
    MBLK_T *mblk;
    att_credit_type_t att_credit_type = att_get_credit_type(len, from_stream);

    /* Flood defense is applicable to only commands and notifications, and as 
     * part of this distributed credit release is impemented */

    /* Terminologies 
     * 
     * App credits    : Indicates number of ATT packets that can be pushed for
     *                  prim and streams path
     * ATT credits    : Indicates total number of memory allocations that ATT 
     *                  can make for all ATT notifications and commands to 
     *                  provide flow control.
     *                  For prim path, 5 credits required for sending a PDU
     *                  For streams path,6 credits required for sending a PDU
     */

    /* Distributed credit release mechanism 
     * 
     * In distributed ATT credit system, ATT credits are released at two 
     * different stage to optimize memory usage yet not compromize on higher  
     * data rate usecases. In this system consumed ATT credits represents  
     * pmalloc usage more accurately.
     * 
     * ATT credits are consumed when a notification/command is sent to L2CAP,
     * its released in two stages 
     *  Stage 1 : when MBLK is destroyed (when copied to HCI/to-air buffer)
     *  Stage 2 : when L2CA_DATAWRITE_CFM is received(data is sent over the air)
     * 
     * ATT Credits are consumed or released as per the below table
     * 
     *                  || PDU sent  || MBLK destroyed || L2CA_DATAWRITE_CFM
     * ----------------------------------------------------------------------
     *  Stream          ||    +6     ||      -5        ||       -1
     *  Prim-small band ||    +3     ||      -2        ||       -1
     *  Prim-large band ||    +3     ||      -2        ||       -1
     * ----------------------------------------------------------------------
     */
     
     /* With Mandatory credits concept, each ATT connection gets atleast one 
      * App credit when the particular connection has consumed none.
      * This ensures one ATT connection taking up all credits doesn't starve 
      * other ATT connection when required */

    if (!ATT_MANDATORY_CREDIT_AVAILABLE(conn) &&
        (att_get_total_available_app_credits(ATT_ARG att_credit_type) == 0))
        return NULL;

#ifdef ATT_GLOBAL_STATE
    {
        uint16_t metadata = conn->cid;

        if ((mblk = mblk_data_create_meta(conn->out.buf, len, TRUE, 
                                    &metadata, sizeof(metadata))) == NULL)
            return NULL;

        if (att_credit_type == ATT_CREDIT_TYPE_STREAM)
            mblk_set_destructor(mblk, mblk_destroyed_stream);
        else if (att_credit_type == ATT_CREDIT_TYPE_PRIM_LARGE_BAND)
            mblk_set_destructor(mblk, mblk_destroyed_large_band);
        else
            mblk_set_destructor(mblk, mblk_destroyed_small_band);
    }
#else
    /* Distributed credit release not supported for Synergy */
    if ((mblk = mblk_data_create(conn->out.buf, len, TRUE)) == NULL)
        return NULL;
#endif

    att_conn_consume_att_credits(conn, att_credit_type);

    return mblk;
}

/*! \brief Sends an outgoing data down to l2cap when not of local CID.
          It might return busy for unsolicited data pkts in case 
          falling short of credits. In case the conn has not consumed any 
          credits then it will any way send the packet and increament 
          consumed_credit.
    \param conn  pointer to ATT connection structure.
    \returns .ATT_RESULT_BUSY if failed to send else Success.
*/
att_result_t att_pkt_send(ATT_FUNC_STATE att_conn_t *conn)
{
    return att_pkt_send_mblk(ATT_ARG conn, NULL);
}

/*! \brief Send outgoing data to L2CAP as an MBLK.
    \param conn pointer to ATT connection structure.
    \param mblk_data pointer to optional MBLK for data.
    \returns ATT_RESULT_SUCCESS if data sent, otherwise ATT_RESULT_BUSY.
*/
att_result_t att_pkt_send_mblk(ATT_FUNC_STATE att_conn_t *conn, MBLK_T *mblk_data)
{
    uint16_t len = (uint16_t)(conn->out.data - conn->out.buf); /* packet length */
    MBLK_T *mblk;
    uint8_t op = conn->out.buf[0];
    bool_t is_cmd_or_notification = 
            (ATT_IS_OP_CMD(op) || ATT_IS_OP_HANDLE_VALUE_NOTIFY(op) ||
            (op == ATT_OP_MULTI_HANDLE_VALUE_NOT));
    bool_t from_stream = (mblk_data != NULL) ? TRUE : FALSE;

    if (is_cmd_or_notification)
    {
        /* ATT protocol doesn't have flow control for ATT command and 
         * notification, so our implementation should have defence against 
         * flooding of these PDUs from the upperlayers */
        mblk = att_pkt_create_with_flood_defense(ATT_ARG conn, len, from_stream);
    }
    else
    {
        mblk = mblk_data_create(conn->out.buf, len, TRUE);
    }

    if (mblk == NULL)
    {
        att_out_pkt_free(conn);
        return ATT_RESULT_BUSY;
    }

    mblk = mblk_add_tail(mblk_data, mblk);

    /* set pending flag */
    att_pend_set(conn, conn->out.buf[0]);

    /* send mblk to the peer */
    if (conn->cid != ATT_CID_LOCAL)
    {
        context_t context = op;

        if (att_get_credit_type(len, from_stream) == 
                                ATT_CREDIT_TYPE_PRIM_LARGE_BAND)
            context |= ATT_PRIM_CTX_LARGE_BAND;
       /* 
         * pass the op code and band info as write context. When cfm is received,
         * handle credits for all unsolicited operations based on this opcode
        */
        L2CA_DataWriteReqEx(conn->cid, 0, mblk, context);

    }
    /* send fake dataread back to ourself */
    else
    {
        MAKE_PRIM(L2CA_DATAREAD_IND);
        prim->cid = ATT_CID_LOCAL;
        prim->data = mblk;
        put_message(ATT_IFACEQUEUE, L2CAP_PRIM, prim);
    }
    
    /* clear outbuffer */
    conn->out.buf = conn->out.data = NULL;
    return ATT_RESULT_SUCCESS;
}

/* Get UUID value from mblk */
/****************************************************************************
NAME
    att_read_uuid  -  Get UUID value from the packet

FUNCTION
    This function understands below ATT concepts and reads a UUID from incoming
    ATT pactek accordingly.
    - A ATT PDU can have 3 different types of UUID(s), 16bit,
      32bit or 128bit.
    - A ATT PDU can contain UUID(s) of length 16bit or 128bit.
    - A 32bit UUID is always masked with Bluetooth base UUID.
    - A 16bit UUID can be masked with Bluetooth base UUID to make
      them 128bit in size.

PARAMETERS
    p_in (IN)
          Pointer to ATT role specific in buffer.
    uuid (OUT)
          Pointer to UUID array where UUID has to be copied. Regardless of
          the UUID type, all four elements will contain UUID.
    uuid_type (IN)
          UUID type, if the type is unknown, it can be set to ATT_UUID_NONE
          this function will try get the type from unread data length in the
          packet.
RETURNS
    TRUE if the UUID could be read, FALSE if something went wrong.

    Something going wrong means that amount of data left in the buffer is not
    exactly a valid UUID sizes to be received over the air (16 or 2 octects).
*/
bool_t att_read_uuid(att_in_t *p_in, ATT_UUID_T *uuid, uint16_t uuid_type)
{
    uint16_t pos;
    uint16_t bits; /* uuid size in bits */

    switch (uuid_type)
    {
        case ATT_UUID_NONE:
            /* determine uuid type by length */
            switch (att_conn_in_buf_len(p_in))
            {
                case 0: /* no filter */
                    return FALSE;
                    
                case 2: /* 16bit length UUID */
                    bits = 16;
                    break;
           
                case 16: /* 128bit length UUID */
                    bits = 128;
                    break;
                    
                default:
                    return FALSE;
            }
            break;

        case ATT_UUID16:
            bits = 16;
            break;

        case ATT_UUID128:
            bits = 128;
            break;

        default:
            return FALSE;
    }


    if (bits == 16)
    {
        uuid->n[0] = read_uint8(&p_in->data);
        uuid->n[0] |= (uint32_t)(read_uint8(&p_in->data) << 8);

        att_pad_uuid_32_to_128(uuid);

        return TRUE;
    }
        
    for (pos = 4; pos--; )
    {
        uint32_t tmp;

        tmp = read_uint8(&p_in->data);
        tmp |= (uint32_t)(read_uint8(&p_in->data) << 8);
        tmp |= (uint32_t) read_uint8(&p_in->data) << 16;
        tmp |= (uint32_t) read_uint8(&p_in->data) << 24;

        uuid->n[pos] = tmp;
    }

    return TRUE;
}


/*! \brief Extracts appropriate error handle that should be sent in error
           response pdu for the incoming client's request.
    \param data  pointer to incoming data
    \param length  length of incoming data
    \returns the handle to be sent with the error code
*/
uint16_t att_pdu_error_handle(const uint8_t *data, uint16_t length)
{
    uint16_t handle = 0x0000;
    uint8_t opcode;

    /* Check if we have received enough bytes to search for handle */
    if (length < (ATT_OPCODEN_LEN + ATT_HANDLE_LEN))
        return 0;

    opcode = UINT8_R(data, 0);

    /* Check on the opcode and extract handle appropriately */
    switch(opcode)
    {
        case ATT_OP_FIND_INFO_REQ:
        case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
        case ATT_OP_READ_BY_TYPE_REQ:
        case ATT_OP_READ_BY_GROUP_TYPE_REQ:
        case ATT_OP_READ_REQ:
        case ATT_OP_READ_BLOB_REQ:
        case ATT_OP_WRITE_REQ:
        case ATT_OP_PREPARE_WRITE_REQ:
            /* Get the attr handle */
            handle = (UINT8_R(data, 2) <<8 ) | UINT8_R(data, 1);
            /* fall-through */
        case ATT_OP_READ_MULTI_REQ:
        case ATT_OP_EXECUTE_WRITE_REQ:
        case ATT_OP_READ_MULTI_VAR_REQ:
            /* fall-through */
        default:
            break;
    }

    return handle;
}


/*! \brief Get packet handler for recieved ATT PDU. 
    \param op  pointer to uint8_t variable to hold opcode of received ATT PDU.
    \param p_in  pointer to role specific incoming data
    \returns pointer to att handler for recieved ATT PDU.
*/
const att_handler_t *att_get_pkt_handler(uint8_t *op, att_in_t *p_in)
{
    const att_handler_t *pkt;

    /* get opcode */
    p_in->data = &p_in->buf[0]; /* set correct starting point */
    *op = read_uint8(&p_in->data);

    /* known opcode */
    if ((*op & ATT_OP_MASK) < (sizeof(att_handlers) / sizeof(att_handler_t)))
        pkt = &att_handlers[*op & ATT_OP_MASK];
    else
        return NULL;

    /* check that we have a handler */
    if (!pkt->handler)
        return NULL;

    return pkt;
}

/*! \brief Function does some basic checks on recieved PDU
    \param op  opcode of received ATT PDU.
    \param conn  pointer to connection structure.
    \param p_in  pointer to role specific incoming data
    \param pkt  pointer to att handler for recieved ATT PDU
    \returns ATT result
*/
att_result_t att_validate_pdu(uint8_t op, att_conn_t *conn, att_in_t *p_in,
                              const att_handler_t *pkt)
{
    uint16_t extra_len = 0;

    /* Ignore ATT commands in change unaware state */
    if (ATT_IS_CLIENT_CHANGE_UNAWARE(conn) && ATT_IS_OP_CMD(op))
        return ATT_RESULT_REQUEST_NOT_SUPPORTED;

    /* command method is allowed only for ATT write procedures */
    if ((op & ATT_OP_CMD_FLAG) == ATT_OP_CMD_FLAG
        && (op & ATT_OP_WRITE_REQ) != ATT_OP_WRITE_REQ)
            return ATT_RESULT_REQUEST_NOT_SUPPORTED;

    /* check authentication signature availability */
    if (op & ATT_OP_AUTH_FLAG)
    {
        /* Write command is only PDU supporting auth */
        if (op != (ATT_OP_WRITE_REQ | ATT_OP_AUTH_FLAG | ATT_OP_CMD_FLAG))
            return ATT_RESULT_REQUEST_NOT_SUPPORTED; /* auth not allowed */
        else
            extra_len += ATT_OP_AUTH_LEN; /* auth sig must be present */
    }

    /* check packet minimum length */
    if (p_in->len < pkt->min + extra_len)
        return ATT_RESULT_INVALID_PDU;

    /* check packet maximum length */
    if (pkt->max && p_in->len + extra_len > pkt->max)
        return ATT_RESULT_INVALID_PDU;


    return ATT_RESULT_SUCCESS;
}

/*! \brief Extracts and returns opcode from a given mblk.
    \param data  mblk of ATT PDU.
    \returns ATT opcode from supplied mblk.
*/
uint8_t att_opcode_from_mblk(MBLK_T *data)
{
    uint8_t *mapped_data;
    uint8_t opcode;

    mapped_data = mblk_map(data, 0, ATT_OPCODEN_LEN);
    opcode = UINT8_R(mapped_data, 0);
    mblk_unmap(data, mapped_data);

    return opcode;
}


/*! \brief This function does some basic checks on recieved PDU and determines 
           the ATT handler to be called.
    \param ind pointer to l2cap data read indication structure.
    \param p_in - pointer to att_in_t sturcture.
    \param op - ATT opcode.
    \param conn - pointer to connection structure.
    \param data_length - incoming ATT data length.
*/
static void att_handle_packet(ATT_FUNC_STATE const L2CA_DATAREAD_IND_T *ind, att_in_t *p_in,
                              uint8_t op, att_conn_t *conn,
                              mblk_size_t data_length)
{
    const att_handler_t *pkt;
    bool_t pkt_done = TRUE;
    att_result_t result = ATT_RESULT_REQUEST_NOT_SUPPORTED;

    if(!p_in)
    {
        /* Create incoming data holder */
        p_in = zpnew(att_in_t);

        /* Get a memory block from incoming MBLK */
        p_in->len = data_length;
        p_in->buf = pmalloc(data_length);

        if(ind)
            mblk_copy_to_memory(ind->data, 0, data_length, p_in->buf);

#ifdef ENABLE_EVENT_COUNTER_REPORTING
        if(ind && (ind->cid != 0))
        {
            AttEventCounter = *((uint32_t *)mblk_get_metadata(ind->data));
        }
#endif

        L0_DBG_MSG2("att_handle_packet: Create incoming data holder, p_in->len %d, pending 0x%x", p_in->len, conn->pend);
        /* Initialize data read pointer */
        p_in->data = p_in->buf;

        /* Update the role specific incoming data buffer base on opcode */
        if (ATT_IS_SERVER_OPERATION(op))
            conn->server_in = p_in;
        else
            conn->client_in = p_in;
    }

    if (!p_in->len)
        return;


    /* handle packet */
    if (((pkt = att_get_pkt_handler(&op, p_in)) != NULL) &&
        ((result = att_validate_pdu(op, conn, p_in, pkt)) == ATT_RESULT_SUCCESS))
    {
        if (att_pend_check(conn, op) &&
            att_handle_pkt_if_change_unaware(ATT_ARG conn, p_in))
        {
            /* handle pdu */
            if ((pkt_done = pkt->handler(ATT_ARG conn, NULL)) == FALSE)
            {
                conn->handler = pkt->handler;
            }
        }
    }
    /* Send error, but only if this was a request */
    else if (!ATT_IS_OP_CMD(op)) 
    {
        if (pkt == NULL || ATT_IS_SERVER_OPERATION(op))
            att_send_error_rsp(ATT_ARG conn, op, 0, result);
    }

    if (pkt_done)
    {
        if(ATT_IS_SERVER_OPERATION(op))
        {
            /* Move incoming data queue head of ATT server. 
             * Note : There is no such queue for ATT client role. */
            conn->server_in = p_in->next;
            conn->handler = NULL;
        }
        else
        {
            conn->client_in = NULL;
        }

        pfree(p_in->buf);
        pfree(p_in);
    }
    else
    {
        /* For multiple attributes, we should not free the out buffer
         * as we are building the output buffer by sending multiple access_ind
         * or from local database
         */
        if (op != ATT_OP_READ_MULTI_VAR_REQ)
            att_out_pkt_free(conn);
    }
}


/*! \brief Processes ATT PDUs recieved from l2cap. 
           ATT handler functions are called to process PDU depending on the 
           opcode. If the recieved PDU contains authenticated signature then 
           signture will be verified in DM module and this function will be 
           invoked again. 
           While processing the incoming PDU, ATT can be blocked on the inputs 
           from application(because of access ind) or DM (because of signature 
           verification), in such cases further incoming PDUs are queued up in 
           offchip and only response is queued up in onchip.
           MBLK in the indication should be freed by the caller.
    \param ind  pointer to l2cap data read indication structure.
    \param c  pointer to connection structure, set to NULL for new incoming data.
    \returns None.
*/
void att_l2ca_dataread_ind(ATT_FUNC_STATE
                           L2CA_DATAREAD_IND_T *ind, att_conn_t *c)
{
    att_conn_t *conn = NULL;
    uint8_t op = 0;
    att_in_t *p_in = NULL;
    mblk_size_t data_length = 0;

    patch_fn(fsm_shared_patchpoint);

    if((ind == NULL) && (c == NULL))
    {
        /* Both parameters cannot be NULL. Called in two cases,
           1. For incoming data, *ind is not NULL
           2. For signature verification, *c is not NULL.
        */
        L0_DBG_MSG("att_l2ca_dataread_ind both parameters ind and c are NULL, unexpected!!");
        return;
    }

    /* DM has verified the signature, continue with packet processing */
    if (c)
    {
        conn = c;
        p_in = conn->server_in;
    }

    /* process new incoming packet */
    else
    {
        L0_DBG_MSG1("att_l2ca_dataread_ind ind->cid 0x%x", ind->cid);
        ATT_LOG_INFO("L2CA DATA READ IND cid 0x%x ", ind->cid);

        /* Indicate L2CAP that the data has been consumed */
        if (ind->cid != ATT_CID_LOCAL)
            L2CA_DataReadRsp(ind->cid, ind->packets);

        if ((conn = att_conn_find(ATT_ARG ind->cid)) == NULL)
        {
            ATT_LOG_WARNING("ATT conn not present");
            return;
        }

        /* Get rid of the link incase of MTU violations */
        if ((data_length = mblk_get_length(ind->data)) > conn->mtu)
        {
            att_handle_protocol_violation(conn);
            return;
        }

        op = att_opcode_from_mblk(ind->data);

        /* Ignore ATT commands in change unaware state */
        if (ATT_IS_CLIENT_CHANGE_UNAWARE(conn) && ATT_IS_OP_CMD(op))
            return;


        /* Are we waiting for any responses from upper-layer or DM ? Queue is 
         * only neccessary for ATT server role 
         */
        if (ATT_IS_SERVER_OPERATION(op) && conn->server_in != NULL)
        {
            /* Check and decide if we want to add it to pending queue */
            att_add_pending_queue(ATT_ARG conn, ind);
            return;
        }
    }

    /* All packets except ATT handle value notification and only
     * ATT handle value notification which doesn't have a matching
     * stream gets pushed into legacy path of execution.
     */
    att_handle_packet(ATT_ARG ind, p_in, op, conn, data_length);
}

/*! \brief Cleans up outstanding Request and Indication when link gets 
          disconnected.
    \param conn  ATT connection structure
    \returns None.
*/
void att_conn_clean(ATT_FUNC_STATE att_conn_t *conn)
{
    /* cancel any outstanding ATT transaction timers */
    att_timer_cancel(&conn->server_timer);
    att_timer_cancel(&conn->client_timer);

    /* send pending responses, we don't know handle, so lets just use 0 */
    if (conn->pend & PEND_HANDLE_CFM)
    {
        att_error_cfm(ATT_STATE(phandle), conn, ATT_OP_HANDLE_VALUE_IND,
                      0, ATT_RESULT_TIMEOUT);
    }

    if (PEND_GET_RSP())
    {
        att_error_cfm(ATT_STATE(phandle), conn, (uint8_t)PEND_GET_RSP(),
                      0, ATT_RESULT_TIMEOUT);
    }
}

/*! \brief Handles l2cap disconnect indication of an ATT connection
    \param cid of the disconnected link
    \param reason for disconnection
    \returns None.
*/
static void att_l2ca_disconnect_ind(ATT_FUNC_STATE
                                l2ca_cid_t cid, l2ca_disc_result_t reason)
{
    att_conn_t *conn;

    if ((conn = att_conn_find(ATT_ARG cid)) == NULL)
    {
        return;
    }

    att_conn_clean(ATT_ARG conn);

    att_disconnect_ind(ATT_STATE(phandle), cid, reason);

    att_conn_rm(ATT_ARG cid);
}


uint16 att_get_write_pdu_len(uint8_t op, uint16_t payload_len, bool from_stream)
{
    /* This function gets the PDU len required for allocating the Write PDUs.
     * ATT_OP_PREPARE_WRITE_REQ, Write Req/command and Signed Write*/
    uint16 len = ATT_OPCODEN_LEN + ATT_HANDLE_LEN + payload_len;

    if (op == ATT_OP_PREPARE_WRITE_REQ)
        len += ATT_PREPARE_WRITE_REQ_OFFSET_LEN;
    else if (ATT_IS_AUTH_FLAG_SET(op))
        len += ATT_OP_AUTH_LEN;

    if (from_stream)
    {
        /* payload is retained as the mblk from streams */
        len = ATT_OPCODEN_LEN + ATT_HANDLE_LEN;
    }

    return len;
}

/*! \brief Includes length field while framing Read multi var response
    \param conn_in  ATT connection structure
    \param size_value  length of attribute
    \returns bytes left in conn out buffer.
*/
uint16_t att_prepare_for_multi_var_rsp(att_conn_t *conn, uint16_t size_value)
{
    /* For ATT_OP_READ_MULTI_VAR_REQ, include len for each attr */
    if (GET_SERVER_IN_OP(conn) == ATT_OP_READ_MULTI_VAR_REQ)
    {
        /* Ensure sufficient space is available for writing 2 bytes length */
        if ((conn->mtu - (conn->out.data - conn->out.buf)) < 2)
            return 0;

        write_uint16(&conn->out.data, size_value);
    }

    /* Return no of bytes left */
    return (uint16_t)(conn->mtu - (conn->out.data - conn->out.buf));
}


/*! \brief Multiple ATT bearers can be created with a remote device,
       This function checks if there are multiple bearers for a device.
    \param conn_in  ATT connection structure
    \returns TRUE when there is single bearer for a device.
*/
bool att_is_single_att_bearer(ATT_FUNC_STATE att_conn_t *conn_in)
{
    att_conn_t *conn;

    for (conn = ATT_STATE(conn); conn; conn = conn->next)
    {
        if ((conn != conn_in) && (conn->common == conn_in->common))
            return FALSE;
    }

    return TRUE;
}

void att_l2cap_handler(ATT_FUNC_STATE
                       uint16_t type, L2CA_UPRIM_T *prim)
{
    switch (type)
    {
        case L2CA_DISCONNECT_IND:
            L2CA_DisconnectRsp(
                prim->l2ca_disconnect_ind.identifier,
                prim->l2ca_disconnect_ind.cid);
            att_l2ca_disconnect_ind(
                ATT_ARG
                prim->l2ca_disconnect_ind.cid,
                prim->l2ca_disconnect_ind.reason);
            break;

        case L2CA_DATAREAD_IND:
            att_l2ca_dataread_ind(ATT_ARG &prim->l2ca_dataread_ind, NULL);
            break;

        case L2CA_DATAWRITE_CFM:
        {
            att_conn_t *conn;
            uint16_t op;
            context_t req_ctx = prim->l2ca_datawrite_cfm.req_ctx;
            bool_t is_large_band = 
               ((req_ctx & ATT_PRIM_CTX_LARGE_BAND) == ATT_PRIM_CTX_LARGE_BAND);

            op = req_ctx & 0xFF;
            if ((op & ATT_OP_CMD_FLAG) || 
                (op == ATT_OP_HANDLE_VALUE_NOT) ||
                (op == ATT_OP_MULTI_HANDLE_VALUE_NOT))
            {
                if ((conn = att_conn_find(ATT_ARG prim->l2ca_datawrite_cfm.cid)) != NULL)
                {
                    att_conn_release_remaining_att_credits(conn, is_large_band);
                }
            }
        }
            break;
            

#ifdef INSTALL_EATT
        case L2CA_AUTO_TP_CONNECT_IND:
            att_l2ca_auto_tp_connect_ind(ATT_ARG 
                                          &prim->l2ca_auto_tp_connect_ind);
            break;

        case L2CA_AUTO_TP_CONNECT_CFM:
            att_l2ca_auto_tp_connect_cfm(ATT_ARG 
                                         &prim->l2ca_auto_tp_connect_cfm);
            break;

        /* L2CAP sends this primitive to indicate remote L2CAP channel is 
         * busy/ready w.r.t L2CAP credits. Since ATT has flood defense,
         * ATT can send the PDUs to L2CAP which gets buffered at L2CAP. 
         * Buffered PDUs are sent when credits are available at L2CAP.
         * Hence ATT just consumes the message, no extra handling is added */
        case L2CA_BUSY_IND:
            break;
#endif /* INSTALL_EATT */

        default:
            BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
            BLUESTACK_BREAK_IF_PANIC_RETURNS
    }
        
    L2CA_FreePrimitive(prim);
}

#endif /* INSTALL_ATT_MODULE */
