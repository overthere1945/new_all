/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifdef INSTALL_ATT_MODULE

#ifndef __ATTLIB_H__
#define __ATTLIB_H__

#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include INC_DIR(bluestack,att_prim.h)


#ifdef __cplusplus
extern "C" {
#endif


void attlib_access_rsp(
    phandle_t phandle,
    uint16_t cid,
    uint16_t handle,
    att_result_t result,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    );

#ifdef ATT_FLAT_DB_SUPPORT
void attlib_add_db_req(
    phandle_t phandle,
    uint16_t size_db,
    uint16_t *db,
    ATT_UPRIM_T **pp_prim
    );
#endif /* ATT_FLAT_DB_SUPPORT */

#ifndef ATT_FLAT_DB_SUPPORT
void attlib_add_req(
    phandle_t phandle,
    att_attr_t *attrs,
    ATT_UPRIM_T **pp_prim
    );
#endif /* !ATT_FLAT_DB_SUPPORT */

void attlib_connect_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    L2CA_CONNECTION_T connection,
    l2ca_conflags_t flags,
    ATT_UPRIM_T **pp_prim
    );

#ifdef INSTALL_ATT_BREDR
void attlib_connect_rsp(
    phandle_t phandle,
    uint16_t cid,
    l2ca_conn_result_t  response,
    ATT_UPRIM_T **pp_prim
    );
#endif

void attlib_disconnect_req(
    phandle_t phandle,
    uint16_t cid,
    ATT_UPRIM_T **pp_prim
    );

void attlib_exchange_mtu_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t mtu,     
    ATT_UPRIM_T **pp_prim
    );

void attlib_exchange_mtu_rsp(
    phandle_t phandle,
    uint16_t cid,
    uint16_t server_mtu,
    ATT_UPRIM_T **pp_prim
    );

void attlib_execute_write_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t flags,
    ATT_UPRIM_T **pp_prim
    );

void attlib_find_by_type_value_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t start,
    uint16_t end,
    uint16_t uuid,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    );

void attlib_find_info_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t start,
    uint16_t end,
    ATT_UPRIM_T **pp_prim
    );

void attlib_handle_value_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t handle,
    uint16_t flags,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    );

void attlib_handle_value_rsp(
    phandle_t phandle,
    uint16_t cid,
    ATT_UPRIM_T **pp_prim
    );

void attlib_prepare_write_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t handle,
    uint16_t offset,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    );

void attlib_read_blob_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t handle,
    uint16_t offset,
    ATT_UPRIM_T **pp_prim
    );

void attlib_read_by_group_type_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t start,
    uint16_t end,
    att_uuid_type_t group_type,
    uint32_t *group,
    ATT_UPRIM_T **pp_prim
    );

void attlib_read_by_type_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t start,
    uint16_t end,
    att_uuid_type_t uuid_type,
    uint32_t *uuid,
    ATT_UPRIM_T **pp_prim
    );

void attlib_read_multi_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t size_handles,
    uint16_t *handles,
    ATT_UPRIM_T **pp_prim
    );

void attlib_read_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t handle,
    ATT_UPRIM_T **pp_prim
    );

void attlib_register_req(
    phandle_t phandle,
    ATT_UPRIM_T **pp_prim
    );

#ifndef ATT_FLAT_DB_SUPPORT
void attlib_remove_req(
    phandle_t phandle,
    uint16_t start,
    uint16_t end,
    ATT_UPRIM_T **pp_prim
    );
#endif /* !ATT_FLAT_DB_SUPPORT */

void attlib_unregister_req(
    phandle_t phandle,
    ATT_UPRIM_T **pp_prim
    );

void attlib_write_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t handle,
    uint16_t flags,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    );

void attlib_handle_value_ntf(
    phandle_t phandle,
    context_t context,
    uint16_t cid,
    uint16_t handle,
    uint16_t flags,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    );

void attlib_write_cmd(
    phandle_t phandle,
    context_t context,
    uint16_t cid,
    uint16_t handle,
    uint16_t flags,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    );


void attlib_free(
    ATT_UPRIM_T *p_prim
    );

void attlib_add_robust_caching_req(
    phandle_t phandle,
    context_t context,
    TP_BD_ADDR_T *tp_addrt,
    uint16_t change_aware,
    ATT_UPRIM_T **pp_prim
    );

void attlib_set_bredr_local_mtu_req(
    phandle_t phandle,
    context_t context,
    uint16_t mtu,
    ATT_UPRIM_T **pp_prim
    );

#ifdef INSTALL_EATT
void attlib_read_multi_var_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t size_handles,
    uint16_t *handles,
    ATT_UPRIM_T **pp_prim
    );

void attlib_multi_handle_value_ntf_req(
    phandle_t phandle,
    context_t context,
    uint16_t cid,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    );

void attlib_read_multi_var_rsp(
    phandle_t phandle,
    uint16_t cid,
    att_result_t result,
    uint16_t error_handle,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    );

void attlib_enhanced_register_req(
    phandle_t phandle,
    context_t context,
    uint32_t flags,
    ATT_UPRIM_T **pp_prim
    );

void attlib_enhanced_unregister_req(
    phandle_t phandle,
    context_t context,
    ATT_UPRIM_T **pp_prim
    );

void attlib_enhanced_connect_req(
    phandle_t phandle,
    TP_BD_ADDR_T  *tp_addrt,
    l2ca_conflags_t flags,
    att_mode_t mode,
    uint16_t num_bearers,
    uint16_t mtu,
    uint16_t initial_credits,
    att_priority_t priority,
    ATT_UPRIM_T **pp_prim
    );

void attlib_enhanced_connect_rsp(
    phandle_t phandle,
    uint16_t  identifier,
    uint16_t  num_cid_success,
    l2ca_conn_result_t  response,
    uint16_t mtu,
    uint16_t initial_credits,
    att_priority_t priority,
    ATT_UPRIM_T **pp_prim
    );

void attlib_enhanced_reconfigure_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t mtu,
    ATT_UPRIM_T **pp_prim
    );

void attlib_enhanced_reconfigure_rsp(
    phandle_t phandle,
    uint16_t identifier,
    ATT_UPRIM_T **pp_prim
    );
#endif /* INSTALL_EATT */

#ifdef __cplusplus
}
#endif

#endif  /* __ATTLIB_H__ */
#endif  /* INSTALL_ATT_MODULE */
