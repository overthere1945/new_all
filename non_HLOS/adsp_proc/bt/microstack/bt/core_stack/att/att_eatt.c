/*******************************************************************************

Copyright (C) 2019 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "att_private.h"

#if defined(INSTALL_ATT_MODULE) && defined(INSTALL_EATT)

/* Atleast one bit should be set either ATT_REG_FLAG_EATT_BREDR_SUPPORT
 * or ATT_REG_FLAG_EATT_LE_SUPPORT */
#define ATT_ENHANCED_MIN_REG_FLAG (ATT_REG_FLAG_EATT_BREDR_SUPPORT | \
                                   ATT_REG_FLAG_EATT_LE_SUPPORT)
#define ATT_IS_ENHANCED_REGISTER_FLAGS_INVALID(flags)    \
        ((flags > ATT_ENHANCED_MIN_REG_FLAG) ||          \
         (flags & ATT_ENHANCED_MIN_REG_FLAG) == 0)

#define EATT_MAX_CONFTAB_LEN 20
#define EATT_MTU_MIN         64

const uint16_t cids_key[EATT_MAX_CIDS] = {L2CA_AUTOPT_ECBFC_CID1,
                                          L2CA_AUTOPT_ECBFC_CID2,
                                          L2CA_AUTOPT_ECBFC_CID3,
                                          L2CA_AUTOPT_ECBFC_CID4,
                                          L2CA_AUTOPT_ECBFC_CID5};

typedef enum att_req_type_tag
{
    ATT_TYPE_EATT_CONNECT,
    ATT_TYPE_EATT_RECONFIG,
    /* Other types can be added here */
    ATT_TYPE_INVALID
} att_req_type_t;

/*! \brief Handles the ATT_MULTI_HANDLE_VALUE_NTF_REQ_T.
           Sends Multiple Handle Value notification (MHVN) to remote client.
           This can be sent on both EATT and ATT bearer(optional).
           Remote client can enable MHVN on ATT bearer through 
           Client Supported Features characteristic. Only then GATT can use
           this primitive for ATT bearer.
           Returns ATT_HANDLE_VALUE_MULTI_NTF_CFM indicating success/failure.
    \param req pointer to the notification.
    \returns void
*/
void att_multi_handle_value_ntf_req(ATT_FUNC_STATE
                                    ATT_MULTI_HANDLE_VALUE_NTF_REQ_T *req)
{
    att_conn_t *conn;
    att_result_t rc = ATT_RESULT_SUCCESS;

    if ((conn = att_conn_find(ATT_ARG req->cid)) == NULL)
    {
        rc = ATT_RESULT_INVALID_CID;
    }
    else if (ATT_IS_CLIENT_CHANGE_UNAWARE(conn))
    {
        rc = ATT_RESULT_CHANGE_UNAWARE_DISALLOWED;
    }
    else
    {
        uint16 len = ATT_OPCODEN_LEN + req->size_value;

        if (len < att_handlers[ATT_OP_MULTI_HANDLE_VALUE_NOT].min)
        {
            rc = ATT_RESULT_INVALID_PDU;
        }
        else if (len > conn->mtu)
        {
            rc = ATT_RESULT_MTU_VIOLATION;
        }
        else if (ATT_PKT_CREATE_FAILED == 
                att_pkt_create_len(conn, ATT_OP_MULTI_HANDLE_VALUE_NOT, len))
        {
            rc = ATT_RESULT_INSUFFICIENT_RESOURCES;
        }
        else
        {
            (void)att_pkt_write(conn, req->value, req->size_value);
            if (ATT_RESULT_SUCCESS != att_pkt_send(ATT_ARG conn))
            {
                rc = ATT_RESULT_BUSY;
            }
        }
    }

    att_multi_handle_value_ntf_cfm(req, rc);
}

/*! \brief Handles the ATT_READ_MULTI_VAR_RSP_T.
    \param req pointer to the ATT_READ_MULTI_VAR_RSP_T.
    \returns void
*/
void att_read_multi_var_rsp(ATT_FUNC_STATE
                            ATT_READ_MULTI_VAR_RSP_T *rsp)
{
    att_conn_t *conn;
    uint8_t op;

    if ((conn = att_conn_find(ATT_ARG rsp->cid)) == NULL)
        return;

    if ((conn->server_in == NULL) || (conn->server_in->buf == NULL))
        return;

    if ((op = GET_SERVER_IN_OP(conn)) != ATT_OP_READ_MULTI_VAR_REQ)
        return;

    if (rsp->result != ATT_RESULT_SUCCESS)
        att_send_error_rsp(ATT_ARG conn, op, rsp->error_handle, rsp->result);
    else
    {
        if (ATT_PKT_CREATE_FAILED == 
                 att_pkt_create_len(conn,
                                    ATT_OP_READ_MULTI_VAR_RSP,
                                    ATT_OPCODEN_LEN + rsp->size_value))
        {
            att_send_error_rsp(ATT_ARG conn, op, 0,
                               ATT_RESULT_INSUFFICIENT_RESOURCES);
        }
        else
        {
            (void)att_pkt_write(conn, rsp->value, rsp->size_value);
            (void)att_pkt_send(ATT_ARG conn);
        }
    }

    att_remove_pending_queue_head(conn);
    conn->handler = NULL;
    att_check_pending_queue(ATT_ARG conn);
}

/*! \brief Handles Read Multi Var Req received from remote client
           Sends Error response if validation/permission check fails
           Sends ATT_READ_MULTI_VAR_IND if IRQ is set for all attr.
           otherwise returns ATT_RMV_NOT_PROCESSED
    \param conn pointer to connection structure.
    \returns ATT_RMV_IND_SENT if ATT_READ_MULTI_VAR_IND is sent
             ATT_RMV_ERROR_RSP_SENT if Error response is sent
             ATT_RMV_NOT_PROCESSED if IRQ is not set for atleast one handle
*/
att_rmv_result_t att_process_read_multi_var(ATT_FUNC_STATE att_conn_t *conn)
{
    att_result_t rc = ATT_RESULT_SUCCESS;
    att_attr_t *attr = NULL;
    uint16_t handle=0;
    uint16_t *data;
    uint16_t len;
    uint16_t i=0;

    /* For application not supporting ATT_READ_MULTI_VAR_IND, use the
     * access ind/rsp sequence for framing the Read Multi Var Response.
     * This is possible especially for legacy ATT bearer */
    if (!ATT_IS_EATT_REGISTERED())
        return ATT_RMV_NOT_PROCESSED;

    len = SERVER_IN_BUF_LEN(conn);

    if ((data = xpmalloc(len)) == NULL)
        rc = ATT_RESULT_INSUFFICIENT_RESOURCES;

    /* If IRQ is set for all the attributes, send ATT_READ_MULTI_VAR_IND
     * otherwise read the value from ATT DB or through ACCESS_IND/RSP sequence
     */
    while ((rc == ATT_RESULT_SUCCESS) && len)
    {
        handle = read_uint16(&conn->server_in->data);
        len -= 2;
        data[i++] = handle;

        if ((attr = att_attr_find(ATT_ARG handle, NULL)) == NULL)
        {
            rc = ATT_RESULT_INVALID_HANDLE;
        }
        else if (!(att_attr_perm(ATT_ARG attr) & ATT_PERM_READ))
        {
            rc = ATT_RESULT_READ_NOT_PERMITTED;
        }
        else
        {
            rc = check_auth(ATT_ARG conn, attr, ATT_PERM_READ);

            /* IRQ is not set for atleast one attr, read the values from
             * local database. Handling is same as the legacy PDU
             * Read Multi Req */
            if ((rc == ATT_RESULT_SUCCESS) && !ATT_IS_READ_IRQ_SET(attr))
            {
                /* Set the correct starting point */
                conn->server_in->data = &conn->server_in->buf[1];
                pfree(data);
                return ATT_RMV_NOT_PROCESSED;
            }
        }
    }

    if (rc == ATT_RESULT_SUCCESS)
    {
        att_read_multi_var_ind(ATT_STATE(phandle),
                               conn->cid,
                               (uint16_t)(ATT_ACCESS_READ | ATT_ACCESS_PERMISSION),
                               i, data);
        return ATT_RMV_IND_SENT;
    }
    else
    {
        att_send_error_rsp(ATT_ARG conn,
                           ATT_OP_READ_MULTI_VAR_REQ,
                           handle, rc);
        if (data != NULL)
            pfree(data);

        return ATT_RMV_ERROR_RSP_SENT;
    }
}


/* wrapper for Read multiple variable request */
void att_read_multi_var_req(ATT_FUNC_STATE ATT_READ_MULTI_VAR_REQ_T *req)
{
    att_result_t rc = ATT_RESULT_INVALID_PARAMS;

    /* 2 is minm no handles in read multi var req */
    if ((req->size_handles >= 2) && req->handles)
    {
        rc = read_common_req(ATT_ARG ATT_OP_READ_MULTI_VAR_REQ,
                             req->cid,
                             req->size_handles, req->handles,
                             0 /* offset */);
    }

    if (rc != ATT_RESULT_SUCCESS)
        att_read_multi_var_cfm(req->phandle, req->cid, rc, 0, 0, NULL);
}

/*! \brief Handler function for ATT_ENHANCED_REGISTER_REQ.
    \param req pointer to ATT_ENHANCED_REGISTER_REQ_T structure.
    \returns void
*/
void att_enhanced_register_req(ATT_FUNC_STATE ATT_ENHANCED_REGISTER_REQ_T *req)
{
    att_result_t rc = ATT_RESULT_SUCCESS;

    /* These are the valid registration configuration
     * 1 : Legacy application/application not supporting EATT 
     *     only uses ATT_REGISTER_REQ.
     * 2 : Application with EATT support uses ATT_ENHANCED_REGISTER_REQ
     *     and not ATT_REGISTER_REQ. This will internally enable both
     *     ATT and EATT.
     */
    if (ATT_STATE(eatt_registered))
    {
        rc = ATT_RESULT_ALREADY_REGISTERED;
    }
    else if (ATT_IS_ENHANCED_REGISTER_FLAGS_INVALID(req->flags))
    {
        rc = ATT_RESULT_INVALID_PARAMS;
    }
    else
    {
        rc = att_process_register_req(ATT_ARG req->phandle);
    }

    if (rc == ATT_RESULT_SUCCESS)
    {
        uint16_t flags = (L2CA_REGFLAG_USE_TP_PRIMS | L2CA_REGFLAG_USE_ECBFC);

        ATT_STATE(ctx) = req->context;

        if (req->flags & ATT_REG_FLAG_EATT_BREDR_SUPPORT)
            ATT_STATE(eatt_bredr_support) = TRUE;

        if (req->flags & ATT_REG_FLAG_EATT_LE_SUPPORT)
        {
            ATT_STATE(eatt_le_support) = TRUE;
            flags |= L2CA_REGFLAG_FOR_LE_TP;
        }

        /* If LE is enabled, Register req is sent for LE first */
        L2CA_RegisterReq(EATT_PSM,
                         QUEUEID_TO_PHANDLE(ATT_IFACEQUEUE),
                         0,
                         flags,
                         0);
    }
    else
    {
        att_enhanced_register_cfm(req->phandle, req->context, rc);
    }
}

/*! \brief Handler function for ATT_ENHANCED_UNREGISTER_REQ.
    \param req pointer to ATT_ENHANCED_UNREGISTER_REQ_T structure.
    \returns void
*/
void att_enhanced_unregister_req(ATT_FUNC_STATE
                                 ATT_ENHANCED_UNREGISTER_REQ_T *req)
{
    att_result_t rc = ATT_RESULT_SUCCESS;

    if (!ATT_STATE(eatt_registered))
    {
        rc = ATT_RESULT_OPERATION_NOT_ALLOWED;
    }
    else if (ATT_STATE(phandle) != req->phandle)
    {
        rc = ATT_RESULT_INVALID_PHANDLE;
    }
    else
    {
        att_conn_t *conn;

        /* If there are any active connection, reject the unregister req.
         * Unregister will reset the ATT_STATE(phandle), required for 
         * sending remote initiated disconnect */
        for (conn = ATT_STATE(conn); conn; conn = conn->next)
        {
            if (conn->cid != ATT_CID_LOCAL)
            {
                rc = ATT_RESULT_OPERATION_NOT_ALLOWED;
                break;
            }
        }
    }

    if (rc == ATT_RESULT_SUCCESS)
    {
        ATT_STATE(ctx) = req->context;

        /* L2CAP to unregister PSM for all transports */
        L2CA_UnRegisterReq(EATT_PSM, QUEUEID_TO_PHANDLE(ATT_IFACEQUEUE));
    }
    else
    {
        att_enhanced_unregister_cfm(req->phandle, req->context, rc);
    }
}

/*! \brief Resets the EATT specific registration parameters 
           This is called when the EATT registration fails or unregistered.
    \returns void
*/
static void eatt_reset_registration(ATT_FUNC_STATE_1ARG)
{
    ATT_STATE(eatt_bredr_support) = FALSE;
    ATT_STATE(eatt_le_support) = FALSE;
    ATT_STATE(eatt_registered) = FALSE;
    ATT_STATE(phandle) = 0;
    att_conn_rm(ATT_ARG ATT_CID_LOCAL);
}

/*! \brief Handler function for L2CA_REGISTER_CFM_T - EATT PSM
    \param cfm pointer to L2CA_REGISTER_CFM_T structure.
    \returns void
*/
void eatt_l2ca_register_cfm(ATT_FUNC_STATE L2CA_REGISTER_CFM_T *cfm)
{
    if ((cfm->result == L2CA_MISC_SUCCESS) &&
        (cfm->flags & L2CA_REGFLAG_FOR_LE_TP) &&
         ATT_STATE(eatt_bredr_support))
    {
        /* CFM received for LE, BR/EDR registration is pending */
        L2CA_RegisterReq(EATT_PSM, 
                         QUEUEID_TO_PHANDLE(ATT_IFACEQUEUE),
                         0,
                         (L2CA_REGFLAG_USE_TP_PRIMS | L2CA_REGFLAG_USE_ECBFC),
                         0);
    }
    else
    {
        att_result_t rc = ATT_RESULT_SUCCESS;

        /* Failure case or registration is complete */
        phandle_t phandle = ATT_STATE(phandle);

        if (cfm->result == L2CA_MISC_SUCCESS)
        {
            ATT_STATE(eatt_registered) = TRUE;
        }
        else
        {
            rc = ATT_RESULT_OPERATION_FAILED;

            eatt_reset_registration(ATT_ARG_1);
        }

        att_enhanced_register_cfm(phandle, ATT_STATE(ctx), rc);
    }
}

/*! \brief Handler function for L2CA_UNREGISTER_CFM_T - EATT PSM
    \param cfm pointer to L2CA_UNREGISTER_CFM_T structure.
    \returns void
*/
void eatt_l2ca_unregister_cfm(ATT_FUNC_STATE L2CA_UNREGISTER_CFM_T *cfm)
{
    att_result_t rc = ATT_RESULT_OPERATION_FAILED;
    phandle_t phandle = ATT_STATE(phandle);

    if (cfm->result == L2CA_MISC_SUCCESS)
    {
        rc = ATT_RESULT_SUCCESS;

        eatt_reset_registration(ATT_ARG_1);

        /* delete attributes associated with this server */
        (void)att_attr_remove(ATT_ARG ATT_HANDLE_MIN, ATT_HANDLE_MAX);
    }

    att_enhanced_unregister_cfm(phandle, ATT_STATE(ctx), rc);
}

/*! \brief Prepares conftab from ATT_ENHANCED_CONNECT_REQ_T.
    \param prim pointer to ATT_ENHANCED_CONNECT_REQ_T structure.
    \param len returns size of conftab.
    \returns conftab
*/
static uint16_t *eatt_conftab_req(ATT_ENHANCED_CONNECT_REQ_T *req,
                                  uint16_t *len)
{
    uint16_t *conftab = pmalloc(EATT_MAX_CONFTAB_LEN * sizeof(uint16_t));
    uint16_t i = 0;

    conftab[i++] = BKV_SEPARATOR;
    conftab[i++] = L2CA_AUTOPT_ECBFC;
    conftab[i++] = TRUE;

    conftab[i++] = L2CA_AUTOPT_CONN_FLAGS;
    conftab[i++] = req->flags;
    conftab[i++] = L2CA_AUTOPT_ECBFC_NUM_CONNS;
    conftab[i++] = req->num_bearers;

    conftab[i++] = L2CA_AUTOPT_MTU_IN;
    conftab[i++] = req->mtu;
    conftab[i++] = L2CA_AUTOPT_CREDITS;
    conftab[i++] = req->initial_credits;
    conftab[i++] = L2CA_AUTOPT_CHANNEL_PRIORITY;
    conftab[i++] = req->priority;

    conftab[i++] = BKV_BARRIER;

    *len = i;
    return conftab;
}

/*! \brief Prepares conftab from ATT_ENHANCED_CONNECT_RSP_T.
    \param prim pointer to ATT_ENHANCED_CONNECT_RSP_T structure.
    \param len returns size of conftab.
    \returns conftab
*/
static uint16_t *eatt_conftab_rsp(ATT_ENHANCED_CONNECT_RSP_T *rsp,
                                  uint16_t *len)
{
    uint16_t i = 0;
    uint16_t mtu = 0;
    uint16_t priority = 0;
    uint16_t initial_credits = 0;
    uint16_t num_cid_success = 0;
    uint16_t *conftab = pmalloc(EATT_MAX_CONFTAB_LEN * sizeof(uint16_t));

    /* rsp != NULL : Response initiated from application 
     * rsp == NULL : Response triggered internally in Bluestack */
    if (rsp != NULL)
    {
        mtu = rsp->mtu;
        priority = rsp->priority;
        initial_credits = rsp->initial_credits;
        num_cid_success = rsp->num_cid_success;
    }

    conftab[i++] = BKV_SEPARATOR;
    conftab[i++] = L2CA_AUTOPT_ECBFC;
    conftab[i++] = TRUE;

    conftab[i++] = L2CA_AUTOPT_ECBFC_NUM_CONN_SUCCESS;
    conftab[i++] = num_cid_success;

    conftab[i++] = L2CA_AUTOPT_MTU_IN;
    conftab[i++] = mtu;
    conftab[i++] = L2CA_AUTOPT_CREDITS;
    conftab[i++] = initial_credits;
    conftab[i++] = L2CA_AUTOPT_CHANNEL_PRIORITY;
    conftab[i++] = priority;

    conftab[i++] = BKV_BARRIER;

    *len = i;
    return conftab;
}

/*! \brief Checks if MTU can be supported for a platform
           This is a common validation function for MTU
           This can also restrict a max MTU for a specific platform if needed
    \param mtu MTU for a connection
    \returns TRUE if validation succeeds
*/
static bool_t att_validate_mtu(uint16_t mtu)
{
    uint8_t *buf;

    /* Attempt memory allocation using the mtu requested.
        Report failure for a large MTU which is always expected
        to fail due to platform memory pool constraints.
        Memory allocation could still fail at later point of time for a 
        large MTU, so application is expected to configure MTU to a 
        sensible value as mentioned in att_prim.h */

    if ((buf = xpmalloc(mtu)) == NULL)
        return FALSE;

    pfree(buf);
    return TRUE;
}

static void att_update_mtu(att_conn_t *conn,
                           uint16_t local_mtu,
                           uint16_t remote_mtu)
{
    /* Store the local and remote MTU information,
     * this is required for deriving the MTU
     * once the reconfiguration is over */
    conn->local_mtu = local_mtu;
    conn->remote_mtu = remote_mtu;

    /* Symmetric MTU is used for (E)ATT and this is 
     * the minimum MTU supported between two devices.*/
    conn->mtu = MIN(local_mtu, remote_mtu);
}


/*! \brief Prepares conftab for MTU reconfiguration.
    \param cid_info pointer to EATT_CID_INFO_T structure.
    \param is_req TRUE if RECONFIG_REQ, for RSP set to FALSE.
    \param len returns size of conftab.
    \returns conftab
*/
static uint16_t *eatt_conftab_reconfig(EATT_CID_INFO_T * cid_info,
                                       bool_t is_req,
                                       uint16_t *len)
{
    if ((cid_info == NULL) || (cid_info->len > EATT_MAX_CIDS))
    {
        *len = 0;
        return NULL;
    }
    else
    {
        uint16_t *conftab = pmalloc(EATT_MAX_CONFTAB_LEN * sizeof(uint16_t));
        uint16_t i = 0;
        uint16_t j;

        conftab[i++] = BKV_SEPARATOR;
        conftab[i++] = L2CA_AUTOPT_ECBFC;
        conftab[i++] = TRUE;

        conftab[i++] = L2CA_AUTOPT_ECBFC_NUM_CONNS;
        conftab[i++] = cid_info->len;

        for (j=0; j < cid_info->len; j++)
        {
            conftab[i++] = cids_key[j];
            conftab[i++] = cid_info->cids[j];
        }

        if (is_req)
        {
            /* Valid for reconfigure req */
            conftab[i++] = L2CA_AUTOPT_MTU_IN;
            conftab[i++] = cid_info->mtu;
        }

        conftab[i++] = BKV_BARRIER;

        *len = i;
        return conftab;
    }
}

static att_req_type_t att_parse_tp_connect_cfm(ATT_FUNC_STATE 
                                        L2CA_AUTO_TP_CONNECT_CFM_T *cfm,
                                        EATT_CID_INFO_T *cid_info)
{
    att_conn_t *conn;
    att_req_type_t type = ATT_TYPE_INVALID;

    memset(cid_info, 0x00, sizeof(EATT_CID_INFO_T));

    if (cfm->psm_local == EATT_PSM)
    {
        /* With EATT, multiple CIDs can be supported in a single primitive.
         * L2CAP uses conftab for indicating CID list for EATT */
        if ((cfm->config_ext_length == 0) && (cfm->config_ext == NULL))
        {
            /* Failure case, no CID created e.g L2CA_RESULT_OUT_OF_MEM 
             * CID 0 will be used in this case */
            type = ATT_TYPE_EATT_CONNECT;
        }
        else if (!eatt_parse_conftab(cfm->config_ext,
                                cfm->config_ext_length, cid_info))
        {
            BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
        }
        else
        {
            /* Parsing is successful */
            conn = att_conn_find(ATT_ARG cid_info->cids[0]);
            if ((conn != NULL) && ATT_IS_RECONFIG_IN_PROGRESS(conn))
                type = ATT_TYPE_EATT_RECONFIG;
            else
                type = ATT_TYPE_EATT_CONNECT;
        }
    }
    else
    {
        /* This can be extended to BR/EDR unenhanced ATT bearer */
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    return type;
}

static att_req_type_t att_parse_tp_connect_ind(ATT_FUNC_STATE 
                                        L2CA_AUTO_TP_CONNECT_IND_T *ind,
                                        EATT_CID_INFO_T *cid_info)
{
    att_req_type_t type = ATT_TYPE_EATT_CONNECT;

    memset(cid_info, 0x00, sizeof(EATT_CID_INFO_T));

    if (ind->psm_local == EATT_PSM)
    {
        uint16_t i;

        /* With EATT, multiple CIDs can be supported in a single primitive.
         * L2CAP uses conftab for indicating CID list for EATT */
        if (!eatt_parse_conftab(ind->conftab, ind->conftab_length, cid_info))
        {
            BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
        }

        if (!eatt_parse_conftab(ind->conftab, ind->conftab_length, cid_info))
        {
            BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
        }

        for (i=0; i < cid_info->len; i++)
        {
            /* Atleast one conn exists, reconfig case */
            if (att_conn_find(ATT_ARG cid_info->cids[i]) != NULL)
            {
                type = ATT_TYPE_EATT_RECONFIG;
                break;
            }
        }
    }

    return type;
}


static att_rcfg_result_t eatt_process_reconfig(ATT_FUNC_STATE 
                                               EATT_CID_INFO_T *cid_info,
                                               bool_t * mtu_change)
{
    uint16_t i;
    att_conn_t *conn;

    for (i=0; i < cid_info->len; i++)
    {
        conn = att_conn_find(ATT_ARG cid_info->cids[i]);

        /* Reconfig failed - one or more CIDs invalid */
        if (conn == NULL)
            return ATT_RECONFIG_INVALID_CID;

        /* reduction in size of MTU not allowed */
        if (cid_info->mtu < conn->mtu)
            return ATT_RECONFIG_INVALID_MTU;

        if (MIN(conn->local_mtu, cid_info->mtu) > conn->mtu)
            *mtu_change = TRUE;

        /* scratch is updated with proposed MTU value
         * MTU would be updated when the application responds */
        conn->scratch = cid_info->mtu;
    }

    return ATT_RECONFIG_SUCCESSFUL;
}

static void att_send_reconfig_response(ATT_FUNC_STATE
                                       EATT_CID_INFO_T * cid_info,
                                       l2ca_identifier_t id,
                                       att_rcfg_result_t result)
{
    uint16_t i;
    uint16_t *conftab;
    uint16_t size_conftab;

    for (i=0; i < cid_info->len ; i++)
    {
        att_conn_t *conn;

        if ((conn = att_conn_find(ATT_ARG cid_info->cids[i])) != NULL)
        {
            /* Switch to new MTU */
            if (result == ATT_RESULT_SUCCESS)
                att_update_mtu(conn, conn->local_mtu, conn->scratch);

            conn->scratch = 0;
        }
    }

    conftab = eatt_conftab_reconfig(cid_info, FALSE, &size_conftab);

    L2CA_AutoTpConnectRsp(id, cid_info->cids[0],
                          (l2ca_conn_result_t)result,
                          0, size_conftab, conftab);

}


/*! \brief This validates the parameters in ATT_ENHANCED_CONNECT_REQ
           Returns failure is validation fails.
    \param req pointer to ATT_ENHANCED_CONNECT_REQ_T structure
    \returns L2CA_RESULT_SUCCESS if validation succeeds
*/
static l2ca_conn_result_t eatt_validate_connect_req(ATT_FUNC_STATE 
                                             ATT_ENHANCED_CONNECT_REQ_T *req)
{
    l2ca_conn_result_t rc = L2CA_CONNECT_SUCCESS;
    bool_t is_le = L2CA_CONFLAG_IS_LE(req->flags);

    if (!ATT_STATE(eatt_registered))
    {
        rc = L2CA_CONNECT_FAILED;
    }
    else if ((is_le && (req->tp_addrt.tp_type != LE_ACL)) ||
             (!is_le && (req->tp_addrt.tp_type != BREDR_ACL)))
    {
        rc = L2CA_CONNECT_FAILED;
    }
    else if (req->mode == ATT_MODE_EATT)
    {
        if ((req->initial_credits == 0) ||
            (req->mtu < EATT_MTU_MIN) ||
            (req->num_bearers == 0) || 
            (req->num_bearers > EATT_MAX_CIDS) ||
            (!is_le && !ATT_STATE(eatt_bredr_support)) ||
            (is_le && !ATT_STATE(eatt_le_support)) ||
            (!att_validate_mtu(req->mtu)))
            rc = L2CA_CONNECT_FAILED;
    }
    else
    {
        /* Currently ATT_ENHANCED_CONNECT_REQ is supported only on 
         * EATT bearer. If required this support can be added later
         */
        rc = L2CA_CONNECT_FAILED;
    }

    return rc;
}

/*! \brief This validates the parameters in ATT_ENHANCED_CONNECT_RSP_T
           Returns failure is validation fails.
    \param req pointer to ATT_ENHANCED_CONNECT_RSP_T structure
    \returns L2CA_RESULT_SUCCESS if validation succeeds
*/
static l2ca_conn_result_t eatt_validate_connect_rsp(ATT_FUNC_STATE 
                                             ATT_ENHANCED_CONNECT_RSP_T *rsp)
{
    l2ca_conn_result_t rc = L2CA_CONNECT_SUCCESS;

    if (!ATT_STATE(eatt_registered) ||
        (rsp->num_cid_success > EATT_MAX_CIDS) ||
        ((rsp->num_cid_success != 0) && 
         ((rsp->initial_credits == 0) || 
          (rsp->mtu < EATT_MTU_MIN) ||
          (!att_validate_mtu(rsp->mtu)))))
        rc = L2CA_CONNECT_FAILED;

    return rc;
}

/*! \brief Handler function for ATT_ENHANCED_CONNECT_REQ
           Initiates EATT bearer establishment by sending Auto TP primitive.
    \param req pointer to ATT_ENHANCED_CONNECT_REQ_T structure
    \returns void
*/
void att_enhanced_connect_req(ATT_FUNC_STATE
                              ATT_ENHANCED_CONNECT_REQ_T *req)
{
    l2ca_conn_result_t result = eatt_validate_connect_req(ATT_ARG req);

    if (result != L2CA_CONNECT_SUCCESS)
    {
        att_enhanced_connect_cfm(ATT_ARG
                                 req->phandle,
                                 &req->tp_addrt,
                                 req->flags,
                                 req->mode,
                                 result,
                                 NULL);
    }
    else if (req->mode == ATT_MODE_EATT)
    {
        uint16_t size_conftab;
        uint16_t *conftab = eatt_conftab_req(req, &size_conftab);

        L2CA_AutoTpConnectReq(0, EATT_PSM, &req->tp_addrt, EATT_PSM,
                              req->mtu, 0, 0, size_conftab, conftab);
    }
    /* Currently only ATT_MODE_EATT is supported */
}

/*! \brief Handler function for ATT_ENHANCED_CONNECT_RSP
    \param req pointer to ATT_ENHANCED_CONNECT_RSP_T structure
    \returns void
*/
void att_enhanced_connect_rsp(ATT_FUNC_STATE
                              ATT_ENHANCED_CONNECT_RSP_T *rsp)
{
    l2ca_conn_result_t result = eatt_validate_connect_rsp(ATT_ARG rsp);

    if (result == L2CA_CONNECT_SUCCESS)
    {
        att_conn_t *conn;
        uint16_t *conftab;
        l2ca_identifier_t id;
        uint16_t size_conftab;
        att_conn_t *active_conn;
        TYPED_BD_ADDR_T conn_addrt;
        l2ca_cid_t cid = (l2ca_cid_t) rsp->identifier;
        bool_t conn_present;

        /* active_conn refers to first connection sent in connect ind */
        if (((active_conn = att_conn_find(ATT_ARG cid)) == NULL) ||
             (!ATT_IS_CONNECT_IND_RECEIVED(active_conn)))
        {
            return; /* silently ignore */
        }

        /* There could be race condition b/w application initiating ACL close
         * and receiving ATT_ENHANCED_CONNECT_IND. Application is expected to 
         * send response in this case due to L2CAP design, and application 
         * finally receives ATT_ENHANCED_CONNECT_CFM.
         * In this case att_conn_get_addr() returns FALSE incase of onchip 
         * solution as L2CAP CIDs doesn't exist */
        conn_present = att_conn_get_addr(active_conn, &conn_addrt);

        id = active_conn->identifier;

        for (conn = ATT_STATE(conn); conn; )
        {
            TYPED_BD_ADDR_T addrt;
            att_conn_t *next = conn->next;

            if ((!conn_present ||
                (att_conn_get_addr(conn, &addrt) &&
                tbdaddr_eq(&conn_addrt, &addrt))) &&
                ATT_IS_CONNECT_IND_RECEIVED(conn) &&
                (conn->identifier == id))
            {
                ATT_CLEAR_CONNECT_IND_RECEIVED(conn);

                if (rsp->num_cid_success == 0)
                {
                    /* Reject all connections case, 
                     * in this case there is no CFM from L2CAP.
                     * So clear all conn structure locally */
                    att_conn_rm(ATT_ARG conn->cid);
                }
            }
            conn = next;
        }

        conftab = eatt_conftab_rsp(rsp, &size_conftab);

        L2CA_AutoTpConnectRsp(id, cid,
                              rsp->response, rsp->mtu,
                              size_conftab, conftab);
    }
    else
    {
        /* Panic if primitive validation fails */
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }
}

/*! \brief Parse the conftab received in L2CA_AUTO_TP_CONNECT_IND/CFM.
           Populates the information in cid_info if parsing is successful.
    \param conftab Received conftab ptr
    \param conftab_len Length of conftab 
    \param cid_info pointer to EATT_CID_INFO_T 
    \returns Returns TRUE if parsing is successful
*/
/*   Format of conftab w.r.t multiple CID
 *   For L2CA_AUTO_TP_CONNECT_CFM :
 *   |<---------------- EATT_MAX_CIDS --------------------->|
 *   |<------ slen ------->|<------ flen ------->|          |
 *   |  cid[0]  |  cid[1]  |  cid[2]  |  cid[3]  |  cid[4]  |

 *   For L2CA_AUTO_TP_CONNECT_IND :
 *   |<---------------- EATT_MAX_CIDS --------------------->|
 *   |<------ len -------> |                                |
 *   |  cid[0]  |  cid[1]  |  cid[2]  |  cid[3]  |  cid[4]  |
 */
bool_t eatt_parse_conftab(uint16_t *conftab,
                          uint16_t conftab_len,
                          EATT_CID_INFO_T * cid_info)
{
    BKV_ITERATOR_T iter;
    uint16_t len;  /* CID len received in L2CA_AUTO_TP_CONNECT_IND */
    uint16_t slen; /* success CID len in L2CA_AUTO_TP_CONNECT_CFM */
    uint16_t flen; /* failure CID len in L2CA_AUTO_TP_CONNECT_CFM */

    if ((cid_info == NULL) || 
        (conftab_len == 0) ||
        (conftab == NULL))
        return FALSE;

    /* Setup conftab access and iterator */
    iter.iterator = 0;
    iter.block = conftab;
    iter.size = conftab_len;

    (void)BKV_Scan16Single(&iter, L2CA_AUTOPT_ECBFC_NUM_CONNS, &len);
    (void)BKV_Scan16Single(&iter, L2CA_AUTOPT_ECBFC_NUM_CONN_SUCCESS, &slen);
    (void)BKV_Scan16Single(&iter, L2CA_AUTOPT_ECBFC_NUM_CONN_FAILURE, &flen);
    (void)BKV_Scan16Single(&iter, L2CA_AUTOPT_ECBFC_CID1, &cid_info->cids[0]);
    (void)BKV_Scan16Single(&iter, L2CA_AUTOPT_ECBFC_CID2, &cid_info->cids[1]);
    (void)BKV_Scan16Single(&iter, L2CA_AUTOPT_ECBFC_CID3, &cid_info->cids[2]);
    (void)BKV_Scan16Single(&iter, L2CA_AUTOPT_ECBFC_CID4, &cid_info->cids[3]);
    (void)BKV_Scan16Single(&iter, L2CA_AUTOPT_ECBFC_CID5, &cid_info->cids[4]);
    (void)BKV_Scan16Single(&iter, L2CA_AUTOPT_MTU_OUT, &cid_info->mtu);

    if ((len != 0) && ((slen != 0) || (flen != 0)))
        return FALSE;

    if ((len > EATT_MAX_CIDS)||(slen > EATT_MAX_CIDS)||(flen > EATT_MAX_CIDS))
        return FALSE;

    if ((slen + flen) > EATT_MAX_CIDS)
       return FALSE;

    cid_info->len = len;
    cid_info->slen = slen;
    cid_info->flen = flen;
    return TRUE;
}

/*! \brief Handles L2CA_AUTO_TP_CONNECT_CFM_T
    \param cid CID
    \param cfm pointer to L2CA_AUTO_TP_CONNECT_CFM_T structure 
    \param eatt_success Applicable only for EATT, TRUE if EATT CID is success
    \returns void
*/
static void att_process_connect_cfm(ATT_FUNC_STATE
                                    uint16_t cid,
                                    L2CA_AUTO_TP_CONNECT_CFM_T *cfm,
                                    bool_t eatt_success)
{
    att_conn_t *conn;
    uint16_t flags = cfm->flags;

    /* L2CAP doesn't fill L2CA_CONFLAG_INCOMING,updating this
     * explicitly as application expects this flag */
    if ((conn = att_conn_find(ATT_ARG cid)) != NULL)
        flags = cfm->flags | (conn->flags & L2CA_CONFLAG_INCOMING);

    if ((cfm->psm_local == EATT_PSM) && eatt_success)
    {
         /* For outgoing connection, allocate conn if NULL*/
        if ((conn == NULL) && ((conn = att_conn_add(ATT_ARG cid)) == NULL))
        {
            L2CA_DisconnectReq(cid);
            return;
        }

        /* L2CA_CONNECT_SUCCESS : Valid for both outgoing and incoming 
         * connection. conn must be non-NULL. */
#if defined(BUILD_FOR_HOST) && !defined(BLUESTACK_HOST_IS_APPS)
            tbdaddr_copy(&conn->addrt, &cfm->tp_addrt.addrt);
#endif
        att_update_mtu(conn, (uint16_t)cfm->con_ctx, cfm->config.remote_mtu);
        conn->flags = flags;

        if (cfm->psm_local == EATT_PSM)
            ATT_SET_EATT_BEARER(conn);

        att_update_common_info(ATT_ARG conn);
    }
    else
    {
        /* Anything other than above error cases, is considered to be a
         * failure. We will destroy conn associated with cid and update
         * ATT_STATE(conn) */
        att_conn_rm(ATT_ARG cid);
    }
}

/*! \brief Handler function for L2CA_AUTO_TP_CONNECT_CFM_T.
    \param cfm pointer to L2CA_AUTO_TP_CONNECT_CFM_T.
    \returns void
*/
void att_l2ca_auto_tp_connect_cfm(ATT_FUNC_STATE
                                  L2CA_AUTO_TP_CONNECT_CFM_T *cfm)
{
    att_req_type_t type;
    att_conn_t *conn = NULL;
    EATT_CID_INFO_T cid_info;

    /* L2CA_AUTO_TP_CONNECT_CFM is not received during reconfiguration
     * on the responder device */
    type = att_parse_tp_connect_cfm(ATT_ARG cfm, &cid_info);

#ifdef ENABLE_EATT_RECONFIG_REQ
    if (type == ATT_TYPE_EATT_RECONFIG)
    {
        if ((conn = att_conn_find(ATT_ARG cid_info.cids[0])) != NULL)
        {
            /* conn should always exist for reconfig */
            if (cfm->result == L2CA_ECBFC_RECONF_SUCCESSFUL)
                att_update_mtu(conn, cfm->config.mtu, cfm->config.remote_mtu);

            ATT_CLEAR_RECONFIG_IN_PROGRESS(conn);

            att_enhanced_reconfigure_cfm(ATT_STATE(phandle),
                                         cid_info.cids[0],
                                         conn->mtu,
                                         (att_rcfg_result_t)cfm->result);
        }
    }
    else 
#endif
    if (type == ATT_TYPE_EATT_CONNECT)
    {
        uint16_t i;
        l2ca_conflags_t cfm_flags = cfm->flags;

        for (i=0; i < (cid_info.slen + cid_info.flen); i++)
        {
            att_process_connect_cfm(ATT_ARG cid_info.cids[i],
                                    cfm, (i < cid_info.slen));
        }

        if ((cid_info.slen > 0) &&
            (conn = att_conn_find(ATT_ARG cid_info.cids[0])) != NULL)
        {
            /* These are same for all CIDs in CFM (success case)*/
            cid_info.mtu = conn->mtu;
            cfm_flags = conn->flags;
        }

        /* When there are multiple CIDs, single connect cfm is sent to app */
        att_enhanced_connect_cfm(ATT_ARG ATT_STATE(phandle), &cfm->tp_addrt,
                                 cfm_flags, ATT_MODE_EATT,
                                 cfm->result, &cid_info);
    }
}

/*! \brief Handler function for L2CA_AUTO_TP_CONNECT_IND_T.
    \param ind pointer to L2CA_AUTO_TP_CONNECT_IND_T.
    \returns void
*/
void att_l2ca_auto_tp_connect_ind(ATT_FUNC_STATE
                                  L2CA_AUTO_TP_CONNECT_IND_T *ind)
{
    att_req_type_t type;
    EATT_CID_INFO_T cid_info;

    type = att_parse_tp_connect_ind(ATT_ARG ind, &cid_info);

    if (type == ATT_TYPE_EATT_RECONFIG)
    {
        att_rcfg_result_t rc;
        bool_t mtu_change = FALSE;

        rc = eatt_process_reconfig(ATT_ARG &cid_info, &mtu_change);

        if ((rc == ATT_RECONFIG_SUCCESSFUL) && mtu_change)
            att_enhanced_reconfigure_ind(ATT_ARG ind->identifier, &cid_info);
        else
            /* Failure case or no change in MTU, 
             * response is internally generated in this case */
            att_send_reconfig_response(ATT_ARG &cid_info, ind->identifier, rc);
    }
    else if (type == ATT_TYPE_EATT_CONNECT)
    {
        uint16_t i;
        att_conn_t *conn;
        l2ca_conflags_t flags = (ind->flags | L2CA_CONFLAG_INCOMING);

        for (i=0; i < cid_info.len; i++)
        {
            if ((conn = att_conn_add(ATT_ARG cid_info.cids[i])) != NULL)
            {
                conn->flags = flags;
                ATT_SET_EATT_BEARER(conn);
                ATT_SET_CONNECT_IND_RECEIVED(conn);
                /* Store the L2CAP identifer, this is required while
                 * sending L2CA_AutoTpConnectRsp to L2CAP */
                conn->identifier = ind->identifier;
            }
            else
            {
                /* Few CIDs created but not all,
                 * send only the partial accepted CID list to application */
                cid_info.len = i;
                break;
            }
        }

        if (i == 0)
        {
            uint16_t *conftab;
            uint16_t size_conftab;

            /* Not able to create CIDs, send error response internally */
            conftab = eatt_conftab_rsp(NULL, &size_conftab);

            L2CA_AutoTpConnectRsp(ind->identifier, ind->cid,
                                  L2CA_SOME_CONN_REFUSED_NO_RESOURCES,
                                  0, size_conftab, conftab);

        }
        else
        {
            /* There is a possibility to receive same L2CAP identifier from two
             * different devices. Since RSP doesn't have address, send first
             * CID as identifier in IND and use this when RSP is received */
            att_enhanced_connect_ind(ATT_STATE(phandle),
                                     &ind->tp_addrt,
                                     flags,
                                     ATT_MODE_EATT,
                                     cid_info.cids[0],
                                     0,
                                     &cid_info);
        }
    }
}

/*! \brief Updates the change aware state for multiple bearers.
           This is called when change unaware state changes for a bearer
           and needs to be updated for other bearers
    \param conn_in  ATT connection structure
    \returns void
*/
void att_update_robust_caching_for_eatt(ATT_FUNC_STATE att_conn_t *conn_in)
{
    TYPED_BD_ADDR_T conn_addrt;
    att_conn_t *conn;

    if (!att_conn_get_addr(conn_in, &conn_addrt))
        return;

    for (conn = ATT_STATE(conn); conn; conn = conn->next)
    {
        TYPED_BD_ADDR_T addrt;

        if ((conn != conn_in) &&
             att_conn_get_addr(conn, &addrt) &&
             tbdaddr_eq(&conn_addrt, &addrt))
        {
            if (ATT_IS_SET_CHANGE_AWARE_ON_NEXT_ATT_REQ_HASH(conn_in))
                ATT_SET_CHANGE_AWARE_ON_NEXT_ATT_REQ_HASH(conn);
            else if (ATT_IS_CLIENT_CHANGE_AWARE(conn_in))
                ATT_SET_CHANGE_AWARE(conn);
        }
    }
}


#ifdef ENABLE_EATT_RECONFIG_REQ
/*! \brief Handler function for ATT_ENHANCED_RECONFIGURE_REQ
           Initiates MTU reconfiguration on EATT bearer.
    \param req pointer to ATT_ENHANCED_RECONFIGURE_REQ_T structure
    \returns void
*/
void att_enhanced_reconfigure_req(ATT_FUNC_STATE
                                  ATT_ENHANCED_RECONFIGURE_REQ_T *req)
{
    att_conn_t *conn;
    att_result_t rc = ATT_RESULT_SUCCESS;

    /* Primitive validation */
    if ((conn = att_conn_find(ATT_ARG req->cid)) == NULL)
        rc = ATT_RESULT_INVALID_CID;
    else if (!ATT_IS_EATT_BEARER(conn))
        rc = ATT_RESULT_OPERATION_NOT_ALLOWED;
    else if (req->mtu < EATT_MTU_MIN)
        rc = ATT_RESULT_INVALID_MTU;
    else if (req->mtu < conn->mtu)
        rc = ATT_RECONFIG_INVALID_MTU;
    else if ((req->cid == ATT_CID_LOCAL) ||
             (conn->mtu == 0) ||
              ATT_IS_RECONFIG_IN_PROGRESS(conn) ||
              !att_validate_mtu(conn->mtu))
        rc = ATT_RESULT_OPERATION_NOT_ALLOWED;

    /* Error case or requested MTU is same is the current local MTU.
     * reconfig procedure is not initiated in this case */
    if ((rc != ATT_RESULT_SUCCESS) ||
        (req->mtu == conn->local_mtu))
    {
        uint16_t mtu = (rc == ATT_RESULT_SUCCESS) ? conn->mtu : 0;

        att_enhanced_reconfigure_cfm(req->phandle, req->cid, mtu, rc);
    }
    else
    {
        uint16_t *conftab;
        uint16_t size_conftab;
        TP_BD_ADDR_T tp_addrt;
        EATT_CID_INFO_T cid_info;

        /* MTU reconfiguration for only one CID is supported as initiator */
        /* If the requested MTU is less than the remote MTU, still reconfig
           is initiated currently, although final MTU may not change.
           But if remote device also triggers MTU reconfig, there will be
           change in MTU */
        cid_info.len = 1;
        cid_info.mtu = req->mtu;
        cid_info.cids[0] = req->cid;

        ATT_SET_RECONFIG_IN_PROGRESS(conn);

        conftab = eatt_conftab_reconfig(&cid_info, TRUE, &size_conftab);

        (void)att_conn_get_addr(conn, &tp_addrt.addrt);
        tp_addrt.tp_type = L2CA_CONFLAG_IS_LE(conn->flags)? LE_ACL :BREDR_ACL;

        L2CA_AutoTpConnectReq(req->cid, EATT_PSM, &tp_addrt, EATT_PSM,
                              req->mtu, 0, 0, size_conftab, conftab);
    }
}
#endif

/*! \brief Handler function for ATT_ENHANCED_RECONFIGURE_RSP
    \param rsp pointer to ATT_ENHANCED_RECONFIGURE_RSP_T structure
    \returns void
*/
void att_enhanced_reconfigure_rsp(ATT_FUNC_STATE
                                  ATT_ENHANCED_RECONFIGURE_RSP_T *rsp)
{
    EATT_CID_INFO_T cid_info;

    if (!ATT_STATE(eatt_registered) ||
        (rsp->num_cid == 0) ||
        (rsp->num_cid > EATT_MAX_CIDS))
    {
        /* Panic if primitive validation fails */
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    cid_info.len = rsp->num_cid;
    qbl_memscpy(&cid_info.cids[0],
           sizeof(uint16_t)*EATT_MAX_CIDS,
           &rsp->cid[0],
           sizeof(uint16_t)*rsp->num_cid);

    att_send_reconfig_response(ATT_ARG &cid_info,
                               (l2ca_identifier_t)rsp->identifier, 
                               rsp->result);
}


bool att_cid_is_eatt_bearer(ATT_FUNC_STATE uint16_t cid)
{
    att_conn_t *conn;
    bool eatt_bearer = FALSE;

    if (((conn = att_conn_find(ATT_ARG (l2ca_cid_t)cid)) != NULL) &&
          ATT_IS_EATT_BEARER(conn))
        eatt_bearer = TRUE;

    return eatt_bearer;
}

#endif /* defined(INSTALL_ATT_MODULE) && defined(INSTALL_EATT) */
