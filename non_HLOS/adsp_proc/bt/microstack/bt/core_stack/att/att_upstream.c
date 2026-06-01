/*******************************************************************************

Copyright (C) 2010 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "att_private.h"

#ifdef INSTALL_ATT_MODULE

void att_error_cfm(phandle_t phandle, att_conn_t *conn, uint8_t op,
                   uint16_t handle, att_result_t rc)
{
    switch (op)
    {
        case ATT_OP_FIND_INFO_REQ:
        case ATT_OP_FIND_INFO_RSP:
            att_find_info_cfm(phandle, conn->cid, handle,
                              ATT_UUID_NONE, NULL, rc);
            break;    

        case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
        case ATT_OP_FIND_BY_TYPE_VALUE_RSP:
            att_find_by_type_value_cfm(phandle, conn->cid, handle, 0, rc);
            break;    

        case ATT_OP_READ_BY_TYPE_REQ:
        case ATT_OP_READ_BY_TYPE_RSP:
            att_read_by_type_cfm(phandle, conn->cid, handle, rc, 0, NULL);
            break;

        case ATT_OP_READ_REQ:
        case ATT_OP_READ_RSP:
            att_read_cfm(phandle, conn->cid, rc, 0, NULL);
            break;
            
        case ATT_OP_READ_BLOB_REQ:
        case ATT_OP_READ_BLOB_RSP:
            att_read_blob_cfm(phandle, conn->cid, rc, 0, NULL);
            break;

        case ATT_OP_READ_MULTI_REQ:
        case ATT_OP_READ_MULTI_RSP:
            att_read_multi_cfm(phandle, conn->cid, rc, 0, NULL);
            break;
            
        case ATT_OP_READ_BY_GROUP_TYPE_REQ:
        case ATT_OP_READ_BY_GROUP_TYPE_RSP:
            att_read_by_group_type_cfm(phandle, conn->cid, rc, handle, 0,
                                       0, NULL);
            break;
            
        case ATT_OP_WRITE_REQ:
        case ATT_OP_WRITE_RSP:
            att_write_cfm(phandle, conn->cid, rc);
            break;

        case ATT_OP_PREPARE_WRITE_REQ:
        case ATT_OP_PREPARE_WRITE_RSP:
            att_prepare_write_cfm(phandle, conn->cid, handle, 0 /* offs */,
                                  rc, 0, NULL);
            break;
            
        case ATT_OP_EXECUTE_WRITE_REQ:
        case ATT_OP_EXECUTE_WRITE_RSP:
            att_execute_write_cfm(phandle, conn->cid, handle, rc);
            break;

        case ATT_OP_HANDLE_VALUE_IND:
            att_handle_value_cfm(phandle, conn->cid, rc);
            break;
            
#ifdef INSTALL_EATT
        case ATT_OP_READ_MULTI_VAR_REQ:
        case ATT_OP_READ_MULTI_VAR_RSP:
            att_read_multi_var_cfm(phandle, conn->cid, rc, handle, 0, NULL);
            break;
#endif

        default:
            /* silently ignore */
            break;
    }
}

void att_register_cfm(phandle_t phandle, att_result_t rc)
{
    MAKE_PRIM(ATT_REGISTER_CFM);

    prim->phandle = phandle;
    prim->result = rc;

    SEND_PRIM();
}

void att_unregister_cfm(phandle_t phandle, att_result_t rc)
{
    MAKE_PRIM(ATT_UNREGISTER_CFM);

    prim->phandle = phandle;
    prim->result = rc;

    SEND_PRIM();
}

#ifndef ATT_FLAT_DB_SUPPORT
void att_add_cfm(phandle_t phandle, att_result_t rc)
{
    MAKE_PRIM(ATT_ADD_CFM);

    prim->phandle = phandle;
    prim->result = rc;

    SEND_PRIM();
}

void att_remove_cfm(phandle_t phandle, uint16_t num_attr, att_result_t rc)
{
    MAKE_PRIM(ATT_REMOVE_CFM);

    prim->phandle = phandle;
    prim->num_attr = num_attr;
    prim->result = rc;

    SEND_PRIM();
}
#endif /* !ATT_FLAT_DB_SUPPORT */




void att_disconnect_ind(phandle_t phandle, uint16_t cid, l2ca_disc_result_t reason)
{
    MAKE_PRIM(ATT_DISCONNECT_IND);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->reason = reason;
    
    SEND_PRIM();
}


void att_find_info_cfm(phandle_t phandle, l2ca_cid_t cid, uint16_t handle, att_uuid_type_t uuid_type, const ATT_UUID_T *uuid, att_result_t rc)
{
    MAKE_PRIM(ATT_FIND_INFO_CFM);

    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;

    switch (uuid_type)
    {
        case ATT_UUID16:
            /* convert UUID16 into UUID128 format */
            /* NOTE: it is still enough to just use uuid[0] to get UUID16 */
            prim->uuid[0] = uuid->n[0] & 0xffff; /* UUID16 */
            prim->uuid[1] = att_base_uuid.n[1];
            prim->uuid[2] = att_base_uuid.n[2];
            prim->uuid[3] = att_base_uuid.n[3];
            break;

        case ATT_UUID128:
            /* copy UUID128 */
            prim->uuid[0] = uuid->n[0];
            prim->uuid[1] = uuid->n[1];
            prim->uuid[2] = uuid->n[2];
            prim->uuid[3] = uuid->n[3];
            
            if (att_is_uuid16(uuid))
                uuid_type = ATT_UUID16;
            break;

        default: /* ATT_UUID_NONE, for errors */
            /* prim->uuid is already zeroed, do nothing */
            break;
    }
    
    prim->uuid_type = uuid_type;
    prim->result = rc;

    SEND_PRIM();
}

void att_find_by_type_value_cfm(phandle_t phandle, l2ca_cid_t cid, uint16_t handle, uint16_t end, att_result_t rc)
{
    MAKE_PRIM(ATT_FIND_BY_TYPE_VALUE_CFM);

    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;
    prim->end = end;
    prim->result = rc;

    SEND_PRIM();
}

void att_read_by_type_cfm(phandle_t phandle, l2ca_cid_t cid, uint16_t handle, att_result_t rc, uint16_t size_value, uint8_t *value)
{
    MAKE_PRIM(ATT_READ_BY_TYPE_CFM);

    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;
    prim->result = rc;
    prim->size_value = size_value;
    prim->value = value;
    
    SEND_PRIM();
}

void att_read_cfm(phandle_t phandle, l2ca_cid_t cid, att_result_t rc, uint16_t size_value, uint8_t *value)
{
    MAKE_PRIM(ATT_READ_CFM);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->result = rc;
    prim->size_value = size_value;
    prim->value = value;
    
    SEND_PRIM();
}

void att_read_blob_cfm(phandle_t phandle, l2ca_cid_t cid, att_result_t rc, uint16_t size_value, uint8_t *value)
{
    MAKE_PRIM(ATT_READ_BLOB_CFM);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->result = rc;
    prim->size_value = size_value;
    prim->value = value;
    
    SEND_PRIM();
}

void att_read_multi_cfm(phandle_t phandle, l2ca_cid_t cid, att_result_t rc, uint16_t size_value, uint8_t *value)
{
    MAKE_PRIM(ATT_READ_MULTI_CFM);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->result = rc;
    prim->size_value = size_value;
    prim->value = value;
    
    SEND_PRIM();
}

void att_read_by_group_type_cfm(phandle_t phandle, l2ca_cid_t cid, att_result_t rc, uint16_t handle, uint16_t end, uint16_t size_value, uint8_t *value)
{
    MAKE_PRIM(ATT_READ_BY_GROUP_TYPE_CFM);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->result = rc;
    prim->handle = handle;
    prim->end = end;    
    prim->size_value = size_value;
    prim->value = value;
    
    SEND_PRIM();
}

void att_write_cfm(phandle_t phandle, l2ca_cid_t cid, att_result_t rc)
{
    MAKE_PRIM(ATT_WRITE_CFM);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->result = rc;
    
    SEND_PRIM();
}

/* These confirmation messages are always called from downstream API handlers
 * hence it is carrying the downstream primitive as a parameter. 
 * This is done to logically crub calls to be made to this function from that 
 * context only.
 */
void att_write_track_cfm(ATT_WRITE_CMD_T *req, att_result_t rc)
{
    MAKE_PRIM(ATT_WRITE_CMD_CFM);
        
    prim->phandle = req->phandle;
    prim->cid = req->cid;
    prim->result = rc;
    prim->handle = req->handle;
    prim->context = req->context;
    
    SEND_PRIM();
}

void att_prepare_write_cfm(phandle_t phandle, l2ca_cid_t cid, uint16_t handle, uint16_t offset, att_result_t rc, uint16_t size_value, uint8_t *value)
{
    MAKE_PRIM(ATT_PREPARE_WRITE_CFM);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;
    prim->offset = offset;
    prim->result = rc;
    prim->size_value = size_value;
    prim->value = value;
    
    SEND_PRIM();
}

void att_execute_write_cfm(phandle_t phandle, l2ca_cid_t cid, uint16_t handle,
                           att_result_t rc)
{
    MAKE_PRIM(ATT_EXECUTE_WRITE_CFM);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;
    prim->result = rc;
    
    SEND_PRIM();
}

void att_handle_value_ind(phandle_t phandle, l2ca_cid_t cid, uint16_t handle, uint16_t flags, uint16_t size_value, uint8_t *value)
{
    MAKE_PRIM(ATT_HANDLE_VALUE_IND);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;
    prim->flags = flags;
    prim->size_value = size_value;
    prim->value = value;
    
    SEND_PRIM();
}

void att_handle_value_cfm(phandle_t phandle, l2ca_cid_t cid, att_result_t rc)
{
    MAKE_PRIM(ATT_HANDLE_VALUE_CFM);

    prim->phandle = phandle;
    prim->cid = cid;
    prim->result = rc;
    
    SEND_PRIM();
}

/* These confirmation messages are always called from downstream API handlers
 * hence it is carrying the downstream primitive as a parameter. 
 * This is done to logically crub calls to be made to this function from that 
 * context only.
 */
void att_handle_track_value_cfm(ATT_HANDLE_VALUE_NTF_T *req, att_result_t rc)
{
    MAKE_PRIM(ATT_HANDLE_VALUE_NTF_CFM);

    prim->phandle = req->phandle;
    prim->cid = req->cid;
    prim->result = rc;
    prim->context = req->context;
    prim->handle = req->handle;

    SEND_PRIM();
}

void att_access_ind(phandle_t phandle, l2ca_cid_t cid, uint16_t handle,
                    uint16_t flags, uint16_t offs, uint16_t size_value,
                    uint8_t *value)
{
    MAKE_PRIM(ATT_ACCESS_IND);

    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;
    prim->flags = flags;
    prim->offset = offs;
    prim->size_value = size_value;
    prim->value = value;

    SEND_PRIM();
}


#ifdef INSTALL_EATT
void att_multi_handle_value_ntf_ind(phandle_t phandle,
                                    l2ca_cid_t cid,
                                    uint16_t size_value,
                                    uint8_t *value)
{
    MAKE_PRIM(ATT_MULTI_HANDLE_VALUE_NTF_IND);

    prim->phandle = phandle;
    prim->cid = cid;
    prim->size_value = size_value;
    prim->value = value;

    SEND_PRIM();
}

void att_multi_handle_value_ntf_cfm(ATT_MULTI_HANDLE_VALUE_NTF_REQ_T *req,
                                    att_result_t rc)
{
    MAKE_PRIM(ATT_MULTI_HANDLE_VALUE_NTF_CFM);

    prim->phandle = req->phandle;
    prim->context = req->context;
    prim->cid = req->cid;
    prim->result = rc;

    SEND_PRIM();
}

void att_read_multi_var_cfm(phandle_t phandle,
                            l2ca_cid_t cid,
                            att_result_t rc,
                            uint16_t error_handle,
                            uint16_t size_value,
                            uint8_t *value)
{
    MAKE_PRIM(ATT_READ_MULTI_VAR_CFM);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->result = rc;
    prim->error_handle = error_handle;
    prim->size_value = size_value;
    prim->value = value;

    SEND_PRIM();
}

void att_read_multi_var_ind(phandle_t phandle,
                            l2ca_cid_t cid,
                            uint16_t flags,
                            uint16_t size_handles,
                            uint16_t *handles)
{
    MAKE_PRIM(ATT_READ_MULTI_VAR_IND);

    prim->phandle = phandle;
    prim->cid = cid;
    prim->flags = flags;
    prim->size_handles = size_handles;
    prim->handles = handles;

    SEND_PRIM();
}

void att_enhanced_register_cfm(phandle_t phandle,
                               context_t context,
                               att_result_t rc)
{
    MAKE_PRIM(ATT_ENHANCED_REGISTER_CFM);

    prim->phandle = phandle;
    prim->context = context;
    prim->result = rc;

    SEND_PRIM();
}

void att_enhanced_unregister_cfm(phandle_t phandle,
                                 context_t context,
                                 att_result_t rc)
{
    MAKE_PRIM(ATT_ENHANCED_UNREGISTER_CFM);

    prim->phandle = phandle;
    prim->context = context;
    prim->result = rc;

    SEND_PRIM();
}

void  att_enhanced_connect_cfm(ATT_FUNC_STATE
                               phandle_t phandle,
                               TP_BD_ADDR_T *tp_addrt,
                               l2ca_conflags_t flags,
                               att_mode_t mode,
                               l2ca_conn_result_t result,
                               EATT_CID_INFO_T *cid_info)
{
    uint8_t i;
    MAKE_PRIM(ATT_ENHANCED_CONNECT_CFM);

    prim->phandle = phandle;
    prim->tp_addrt = *tp_addrt;
    prim->flags = flags;
    prim->mode = mode;
    prim->result = result;

    if (cid_info != NULL)
    {
        if (cid_info->slen != 0)
        {
            prim->mtu = cid_info->mtu;

            prim->num_cid_success = cid_info->slen;
            qbl_memscpy(&prim->cid_success[0],
                   sizeof(uint16_t)*EATT_MAX_CIDS,
                   &cid_info->cids[0], 
                    sizeof(uint16_t)*cid_info->slen);
            /* Reset the identifier as we have received the confirmation
             * and its no longer required */
            for (i = 0; i < cid_info->slen; i++)
            {
                att_conn_t *conn;
                if ((conn = att_conn_find(ATT_ARG cid_info->cids[i])) != NULL)
                {
                    conn->identifier = 0;
                }
            }
        }

        if (cid_info->flen != 0)
        {
            prim->num_cid_fail = cid_info->flen;
            qbl_memscpy(&prim->cid_fail[0],
                   sizeof(uint16_t)*EATT_MAX_CIDS,
                   &cid_info->cids[cid_info->slen], 
                    sizeof(uint16_t)*cid_info->flen);
        }
    }

    SEND_PRIM();
}

void  att_enhanced_connect_ind(phandle_t phandle,
                               TP_BD_ADDR_T *tp_addrt,
                               l2ca_conflags_t flags,
                               att_mode_t mode,
                               uint16_t identifier,
                               uint16_t mtu,
                               EATT_CID_INFO_T *cid_info)
{
    MAKE_PRIM(ATT_ENHANCED_CONNECT_IND);

    prim->phandle = phandle;
    prim->tp_addrt = *tp_addrt;
    prim->flags = flags;
    prim->mode = mode;
    prim->identifier = identifier;
    prim->mtu = mtu;
    prim->num_cid = cid_info->len;

    if (cid_info->len != 0)
    {
        qbl_memscpy(&prim->cid[0],
               sizeof(uint16_t)*EATT_MAX_CIDS,
               &cid_info->cids[0], 
               sizeof(uint16_t)*cid_info->len);
    }

    SEND_PRIM();
}

#ifdef ENABLE_EATT_RECONFIG_REQ
void  att_enhanced_reconfigure_cfm(phandle_t phandle,
                                   uint16_t cid,
                                   uint16_t mtu,
                                   att_rcfg_result_t result)
{
    MAKE_PRIM(ATT_ENHANCED_RECONFIGURE_CFM);

    prim->phandle = phandle;
    prim->cid = cid;
    prim->mtu = mtu;
    prim->result = result;

    SEND_PRIM();
}
#endif

void  att_enhanced_reconfigure_ind(ATT_FUNC_STATE
                                   uint16_t identifier,
                                   EATT_CID_INFO_T *cid_info)
{
    uint16_t i;
    att_conn_t *conn;
    MAKE_PRIM(ATT_ENHANCED_RECONFIGURE_IND);

    prim->phandle = ATT_STATE(phandle);
    prim->identifier = identifier;
    prim->num_cid = cid_info->len;

    for (i=0; i < cid_info->len; i++)
    {
        if ((conn = att_conn_find(ATT_ARG cid_info->cids[i])) != NULL)
        {
            prim->cid[i] = cid_info->cids[i];
            prim->mtu[i] = MIN(conn->local_mtu, conn->scratch);
        }
    }

    SEND_PRIM();
}

#endif /* INSTALL_EATT */

#ifdef GATT_OFFLOAD
void att_error_ind(phandle_t phandle, uint16_t cid, att_error_t error)
{
    MAKE_PRIM(ATT_ERROR_IND);
        
    prim->phandle = phandle;
    prim->cid = cid;
    prim->error  = error;

    ATT_LOG_DEBUG("ATT Error Ind CID 0x%x error 0x%x", cid, error);
    SEND_PRIM();
}
#endif

#endif /* INSTALL_ATT_MODULE */
