/*******************************************************************************

Copyright (C) 2009 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "att_private.h"


#ifdef INSTALL_ATT_MODULE


static void att_prim_handler(ATT_FUNC_STATE
                             ATT_UPRIM_T *prim)
{
    patch_fn(fsm_shared_patchpoint);

    switch (prim->type)
    {

        /* att_client.c
         ********************************************************************/

        case ATT_FIND_INFO_REQ:
            att_find_info_req(ATT_ARG
                              &prim->att_find_info_req);
            break;

        case ATT_FIND_BY_TYPE_VALUE_REQ:
            att_find_by_type_value_req(ATT_ARG
                                       &prim->att_find_by_type_value_req);
            break;
            
        case ATT_READ_BY_TYPE_REQ:
            att_read_by_type_req(ATT_ARG
                                 &prim->att_read_by_type_req);
            break;
            
        case ATT_READ_REQ:
            att_read_req(ATT_ARG
                         &prim->att_read_req);
            break;

        case ATT_READ_BLOB_REQ:
            att_read_blob_req(ATT_ARG
                              &prim->att_read_blob_req);
            break;

        case ATT_READ_MULTI_REQ:
            att_read_multi_req(ATT_ARG
                               &prim->att_read_multi_req);
            break;

        case ATT_READ_BY_GROUP_TYPE_REQ:
            att_read_by_group_type_req(ATT_ARG
                                       &prim->att_read_by_group_type_req);
            break;
            
        case ATT_WRITE_REQ:
            att_write_req(ATT_ARG
                          &prim->att_write_req);
            break;

        case ATT_WRITE_CMD:
            att_write_track_cmd(ATT_ARG
                          &prim->att_write_cmd,
                          FALSE);
            break;

        case ATT_PREPARE_WRITE_REQ:
            att_prepare_write_req(ATT_ARG
                                  &prim->att_prepare_write_req);
            break;

        case ATT_EXECUTE_WRITE_REQ:
            att_execute_write_req(ATT_ARG
                                  &prim->att_execute_write_req);
            break;

        case ATT_HANDLE_VALUE_RSP:
            att_handle_value_rsp(ATT_ARG
                                 &prim->att_handle_value_rsp);
            break;

        /* att_server.c
         ********************************************************************/
        case ATT_REGISTER_REQ:
            att_register_req(ATT_ARG
                             &prim->att_register_req);
            break;
        
        case ATT_UNREGISTER_REQ:
            att_unregister_req(ATT_ARG
                               &prim->att_unregister_req);
            break;

#ifdef ATT_FLAT_DB_SUPPORT
        case ATT_ADD_DB_REQ:
            att_add_db_req(ATT_ARG
                           &prim->att_add_db_req);
            break;
            
#else /* !ATT_FLAT_DB_SUPPORT */
        case ATT_ADD_REQ:
            att_add_req(ATT_ARG
                        &prim->att_add_req);
            break;

        case ATT_REMOVE_REQ:
            att_remove_req(ATT_ARG
                           &prim->att_remove_req);
            break;
#endif /* ATT_FLAT_DB_SUPPORT */


        case ATT_HANDLE_VALUE_REQ:
            att_handle_value_req(ATT_ARG
                                 &prim->att_handle_value_req, FALSE);
            break;

        case ATT_HANDLE_VALUE_NTF:
            att_handle_track_value_ntf(ATT_ARG
                                 &prim->att_handle_value_ntf);
            break;

        case ATT_ACCESS_RSP:
            att_access_rsp(ATT_ARG
                           &prim->att_access_rsp);
            break;
            
        /* att_eatt.c
         ********************************************************************/
#ifdef INSTALL_EATT
        case ATT_MULTI_HANDLE_VALUE_NTF_REQ:
            att_multi_handle_value_ntf_req(ATT_ARG
                                   &prim->att_multi_handle_value_ntf_req);
            break;

        case ATT_READ_MULTI_VAR_REQ:
            att_read_multi_var_req(ATT_ARG
                                   &prim->att_read_multi_var_req);
            break;

        case ATT_READ_MULTI_VAR_RSP:
            att_read_multi_var_rsp(ATT_ARG
                                   &prim->att_read_multi_var_rsp);
            break;

        case ATT_ENHANCED_REGISTER_REQ:
            att_enhanced_register_req(ATT_ARG
                                      &prim->att_enhanced_register_req);
            break;

        case ATT_ENHANCED_UNREGISTER_REQ:
            att_enhanced_unregister_req(ATT_ARG
                                        &prim->att_enhanced_unregister_req);
            break;

        case ATT_ENHANCED_CONNECT_REQ:
            att_enhanced_connect_req(ATT_ARG 
                                     &prim->att_enhanced_connect_req);
            break;

        case ATT_ENHANCED_CONNECT_RSP:
            att_enhanced_connect_rsp(ATT_ARG
                                     &prim->att_enhanced_connect_rsp);
            break;

#ifdef ENABLE_EATT_RECONFIG_REQ
        case ATT_ENHANCED_RECONFIGURE_REQ:
            att_enhanced_reconfigure_req(ATT_ARG 
                                         &prim->att_enhanced_reconfigure_req);
            break;
#endif

        case ATT_ENHANCED_RECONFIGURE_RSP:
            att_enhanced_reconfigure_rsp(ATT_ARG
                                         &prim->att_enhanced_reconfigure_rsp);
            break;

#endif /* INSTALL_EATT */

        default:
            BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
            BLUESTACK_BREAK_IF_PANIC_RETURNS
    }
    
    /* at this point primitive can be freed - all special cases are
     * handled in primitive handlers */
    attlib_free(prim);
}

void att_handler(void **gash)
{
    uint16_t type;
    void *msg;
#ifndef ATT_GLOBAL_STATE
    ATT_T *att = (ATT_T*)*gash;
#else
    QBL_UNUSED(gash);
#endif

    if (get_message(ATT_IFACEQUEUE, &type, &msg))
    {
        switch (type)
        {                
            case L2CAP_PRIM:
                att_l2cap_handler(ATT_ARG
                                  ((L2CA_UPRIM_T*)msg)->type,
                                  (L2CA_UPRIM_T*)msg);
                break;

            case ATT_PRIM:
                att_prim_handler(ATT_ARG
                                 (ATT_UPRIM_T*)msg);
                break;

            default:
                BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
                BLUESTACK_BREAK_IF_PANIC_RETURNS
        }
    }

    return;
}

#endif /* INSTALL_ATT_MODULE */
