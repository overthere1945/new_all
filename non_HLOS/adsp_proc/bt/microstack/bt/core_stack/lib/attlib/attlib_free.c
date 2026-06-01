/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_free
 *
 *  DESCRIPTION
 *      Free an ATT primitive and all data asocciated to it.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_free(
    ATT_UPRIM_T *p_prim
    )
{
    if (p_prim == NULL)
    {
        return;
    }

    /* Free data associated to the primitive */
    switch (p_prim->type)
    {
        case ATT_ADD_DB_REQ:
#if !defined(BUILD_FOR_HOST) || defined(BLUESTACK_HOST_IS_APPS)
            if (!vm_const_is_encoded(p_prim->att_add_db_req.db))
#endif /* !BUILD_FOR_HOST */
                pfree(p_prim->att_add_db_req.db);
            break;

#ifndef ATT_FLAT_DB_SUPPORT
        case ATT_ADD_REQ:
        {
            att_attr_t *p;
            att_attr_t **pp;

            for (pp = &p_prim->att_add_req.attrs; (p = *pp) != NULL; /* nop */)
            {
                *pp = p->next;
                if (p->value)
                {
                    pfree(p->value);
                }
                pfree(p);
            }
            
            break;
        }
#endif
        
        case ATT_FIND_BY_TYPE_VALUE_REQ:
            pfree(p_prim->att_find_by_type_value_req.value);
            break;
            
        case ATT_READ_BY_TYPE_CFM:
            pfree(p_prim->att_read_by_type_cfm.value);
            break;
            
        case ATT_READ_CFM:
            pfree(p_prim->att_read_cfm.value);
            break;
            
        case ATT_READ_BLOB_CFM:
            pfree(p_prim->att_read_blob_cfm.value);
            break;
            
        case ATT_READ_MULTI_REQ:
            pfree(p_prim->att_read_multi_req.handles);
            break;
            
        case ATT_READ_MULTI_CFM:
            pfree(p_prim->att_read_multi_cfm.value);
            break;
            
        case ATT_READ_BY_GROUP_TYPE_CFM:
            pfree(p_prim->att_read_by_group_type_cfm.value);
            break;
            
        case ATT_WRITE_REQ:
            pfree(p_prim->att_write_req.value);
            break;
            
        case ATT_PREPARE_WRITE_REQ:
            pfree(p_prim->att_prepare_write_req.value);
            break;
            
        case ATT_PREPARE_WRITE_CFM:
            pfree(p_prim->att_prepare_write_cfm.value);
            break;
            
        case ATT_HANDLE_VALUE_REQ:
            pfree(p_prim->att_handle_value_req.value);
            break;
            
        case ATT_HANDLE_VALUE_IND:
            pfree(p_prim->att_handle_value_ind.value);
            break;
            
        case ATT_ACCESS_IND:
            pfree(p_prim->att_access_ind.value);
            break;
            
        case ATT_ACCESS_RSP:
            pfree(p_prim->att_access_rsp.value);
            break;
            
        case ATT_WRITE_CMD:
            pfree(p_prim->att_write_cmd.value);
            break;
            
        case ATT_HANDLE_VALUE_NTF:
            pfree(p_prim->att_handle_value_ntf.value);
            break;
            
        case ATT_DEBUG_IND:
            pfree(p_prim->att_debug_ind.debug);
            break;
            
#ifdef INSTALL_EATT
        case ATT_READ_MULTI_VAR_REQ:
            pfree(p_prim->att_read_multi_var_req.handles);
            break;

        case ATT_READ_MULTI_VAR_CFM:
            pfree(p_prim->att_read_multi_var_cfm.value);
            break;

        case ATT_READ_MULTI_VAR_IND:
            pfree(p_prim->att_read_multi_var_ind.handles);
            break;

        case ATT_READ_MULTI_VAR_RSP:
            pfree(p_prim->att_read_multi_var_rsp.value);
            break;

        case ATT_MULTI_HANDLE_VALUE_NTF_REQ:
            pfree(p_prim->att_multi_handle_value_ntf_req.value);
            break;

        case ATT_MULTI_HANDLE_VALUE_NTF_IND:
            pfree(p_prim->att_multi_handle_value_ntf_ind.value);
            break;
#endif /* INSTALL_EATT */

        default:
            break;
    }

    primfree(ATT_PRIM, p_prim);
}

#endif /* INSTALL_ATT_MODULE */
