/*******************************************************************************

Copyright (C) 2008 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 2001

*******************************************************************************/

#ifndef _L2CAPLIB_H_
#define _L2CAPLIB_H_

#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include "qbl_adapter_scheduler.h"
#include INC_DIR(bluestack,l2cap_prim.h)

#ifdef __cplusplus
extern "C" {
#endif

/* Managment helpers for L2CA_CONFIG_T structures */
extern void L2CA_FreeConfigPtrs(L2CA_CONFIG_T *config);
extern void L2CA_FreeTpConfigPtrs(L2CA_TP_CONFIG_T *tpconfig);
extern void L2CA_CullConfigPtrs(L2CA_CONFIG_T *config);
extern void L2CA_MergeConfigPtrs(L2CA_CONFIG_T *target, L2CA_CONFIG_T *source);
/*lint -sem(L2CA_FreePrimitive, custodial(1)) */
extern void L2CA_FreePrimitive(L2CA_UPRIM_T *prim);

#if defined(INSTALL_STREAM_MODULE) || defined(INSTALL_ATT_MODULE)
extern bool_t L2CA_GetBdAddrFromCid(uint16_t cid,
                                    TYPED_BD_ADDR_T *addrt);
#endif

#if defined(INSTALL_STREAM_MODULE) || defined(TRAPSET_BLUESTACK) || defined(INSTALL_SM_MODULE)
extern bool_t L2CA_GetTpBdAddrFromCid(uint16_t cid,
                                    TP_BD_ADDR_T *tpaddrt);
#endif

#ifdef TRAPSET_BLUESTACK
bool_t L2CA_OverrideConnCtx(l2ca_cid_t cid, phandle_t p_handle, context_t context);
#endif

#if !defined(DISABLE_L2CAP_CONNECTION_FSM_SUPPORT) || \
     defined(INSTALL_L2CAP_LECOC_CB)
extern void L2CA_RegisterReq(psm_t psm,
                             phandle_t phandle,
                             uint16_t mode_mask,
                             uint16_t flags,
                             context_t reg_ctx);
extern void L2CA_UnRegisterReq(psm_t psm_local,
                               phandle_t phandle);
#endif
#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
extern void L2CA_ConnectReq(const BD_ADDR_T *p_bd_addr,
                            psm_t psm_local,
                            psm_t psm_remote,
                            context_t con_ctx,
                            DM_SM_SERVICE_T *substitute_security_service);
extern void L2CA_ConnectRsp(l2ca_cid_t cid,
                            l2ca_identifier_t identifier,
                            l2ca_conn_result_t response,
                            context_t con_ctx);
extern void L2CA_ConfigReq(l2ca_cid_t cid,
                           l2ca_options_t options, l2ca_options_t hints,
                           uint8_t unknownLength, uint8_t *unknown,
                           l2ca_mtu_t mtu, l2ca_timeout_t flush_to,
                           L2CA_QOS_T *qos,
                           L2CA_FLOW_T *flow,
                           l2ca_fcs_t fcs,
                           L2CA_FLOWSPEC_T *flowspec,
                           l2ca_window_size_t ext_window);
extern void L2CA_ConfigReqCs(l2ca_cid_t cid,
                             L2CA_CONFIG_T *config);
extern void L2CA_ConfigRsp(l2ca_cid_t cid,
                           l2ca_identifier_t identifier,
                           l2ca_conf_result_t response,
                           l2ca_options_t options,
                           l2ca_options_t hints,
                           uint8_t unknownLength,
                           uint8_t *unknown,
                           l2ca_mtu_t mtu,
                           l2ca_timeout_t flush_to,
                           L2CA_QOS_T *qos, 
                           L2CA_FLOW_T *flow, 
                           l2ca_fcs_t fcs,
                           L2CA_FLOWSPEC_T *flowspec, 
                           l2ca_window_size_t ext_window);
extern void L2CA_ConfigRspCs(l2ca_cid_t cid,
                             l2ca_identifier_t identifier,
                             l2ca_conf_result_t response,
                             L2CA_CONFIG_T *config);
extern void L2CA_MulticastReq(l2ca_cid_t *cids, uint16_t length, void *p_data);
#endif
#if !defined(DISABLE_L2CAP_CONNECTION_FSM_SUPPORT) || \
     defined(INSTALL_L2CAP_LECOC_CB)
extern void L2CA_DisconnectReq(l2ca_cid_t cid);
extern void L2CA_DisconnectCtxReq(context_t con_ctx);
extern void L2CA_DisconnectRsp(l2ca_identifier_t identifier, l2ca_cid_t cid);
#endif
#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
extern void L2CA_PingReq(const BD_ADDR_T *p_bd_addr, phandle_t phandle, void *p_data,
                         uint16_t length, context_t req_ctx, l2ca_conflags_t flags);
extern void L2CA_PingRsp(const BD_ADDR_T *p_bd_addr, void *p_data, uint16_t length,
                  l2ca_identifier_t identifier);
extern void L2CA_GetInfoReq(const BD_ADDR_T *p_bd_addr, phandle_t phandle,
                            uint16_t info_type,context_t req_ctx, l2ca_conflags_t flags);
/*lint -sem(L2CA_AutoConnectReq, custodial(9)) */
extern void L2CA_AutoConnectReq(l2ca_cid_t           cid,
                                psm_t                psm_local,
                                const BD_ADDR_T     *p_bd_addr,
                                psm_t                psm,
                                context_t            con_ctx,
                                l2ca_controller_t    remote_control,
                                l2ca_controller_t    local_control,
                                uint16_t             conftab_length,
                                uint16_t            *conftab);
/*lint -sem(L2CA_AutoConnectRsp, custodial(6)) */
extern void L2CA_AutoConnectRsp(l2ca_identifier_t    identifier,
                                l2ca_cid_t           cid,
                                l2ca_conn_result_t   response,
                                context_t            con_ctx,
                                uint16_t             conftab_length,
                                uint16_t            *conftab);

extern void L2CA_GetChannelInfoReq(l2ca_cid_t cid);

extern void L2CA_AutoTpConnectReq(l2ca_cid_t           cid,
                                  psm_t                psm_local,
                                  const TP_BD_ADDR_T   *p_tp_bd_addr,
                                  psm_t                psm,
                                  context_t            con_ctx,
                                  l2ca_controller_t    remote_control,
                                  l2ca_controller_t    local_control,
                                  uint16_t             conftab_length,
                                  uint16_t            *conftab);

extern void L2CA_AutoTpConnectRsp(l2ca_identifier_t    identifier,
                                  l2ca_cid_t           cid,
                                  l2ca_conn_result_t   response,
                                  context_t            con_ctx,
                                  uint16_t             conftab_length,
                                  uint16_t            *conftab);
#endif

extern void L2CA_DataWriteReqEx(l2ca_cid_t cid, uint16_t length, void *p_data, context_t context);
#define L2CA_DataWriteReq(cid, length, p_data) (L2CA_DataWriteReqEx((cid), (length), (p_data), 0))
extern void L2CA_DataReadRsp(l2ca_cid_t cid, uint16_t packets);

#ifdef INSTALL_L2CAP_RAW_SUPPORT
extern void L2CA_RawDataReq(l2ca_cid_t cid, uint16_t length, void *p_data, uint16_t raw_length, uint16_t flush_to);
extern void L2CA_ExRawModeReq(l2ca_cid_t cid, l2ca_raw_t mode, phandle_t phandle);
extern void L2CA_RawModeReq(l2ca_cid_t cid,bool_t cid_rx, bool_t acl_rx, phandle_t phandle);
#endif/* INSTALL_L2CAP_RAW_SUPPORT */

extern void L2CA_DataWriteAbortReq(l2ca_cid_t cid);

#ifdef INSTALL_L2CAP_ENHANCED_SUPPORT
extern void L2CA_BusyReq(l2ca_cid_t cid, bool_t busy);
#endif /* ENHANCED_SUPPORT */

#ifdef INSTALL_L2CAP_FIXED_CHANNEL_BASE_SUPPORT
#ifdef INSTALL_L2CAP_FIXED_CHANNEL_SUPPORT
extern void L2CA_RegisterFixedCidReq(phandle_t phandle,
                                     l2ca_cid_t fixed_cid,
                                     L2CA_CONFIG_T *config,
                                     context_t reg_ctx);
extern void L2CA_MapFixedCidReq(const TYPED_BD_ADDR_T *addrt,
                                l2ca_cid_t fixed_cid,
                                context_t  con_ctx,
                                l2ca_conflags_t flags);
extern void L2CA_MapFixedCidRsp(l2ca_cid_t cid,
                                context_t  con_ctx,
                                l2ca_conflags_t flags);
#endif

extern void L2CA_UnmapFixedCidReq(l2ca_cid_t cid);

#if defined(INSTALL_L2CAP_CONNLESS_SUPPORT) || defined(INSTALL_L2CAP_UCD_SUPPORT)
extern void L2CA_MapConnectionlessCidReq(const BD_ADDR_T *p_bd_addr,
                                         psm_t cl_local_psm,
                                         psm_t cl_remote_psm,
                                         context_t con_ctx,
                                         l2ca_conflags_t flags);

extern void L2CA_MapConnectionlessCidRsp(l2ca_cid_t cid,
                                         context_t con_ctx,
                                         psm_t ucd_remote_psm,
                                         l2ca_conflags_t flags);
#endif /* INSTALL_L2CAP_CONNLESS_SUPPORT || INSTALL_L2CAP_UCD_SUPPORT */
#endif /* FIXED_CHANNEL_SUPPORT */

/* BR/EDR may also support flowspecs */
#ifdef INSTALL_L2CAP_FLOWSPEC_SUPPORT
extern L2CA_FLOWSPEC_T *L2CA_AllocFlowspec(void);
#endif

#ifdef INSTALL_L2CAP_QOS_SUPPORT
extern L2CA_QOS_T *L2CA_AllocQoS(void);
#endif

#ifdef INSTALL_AMP_SUPPORT
extern void L2CA_CreateChannelReq(psm_t               psm_local,
                                  const BD_ADDR_T    *bd_addr,
                                  psm_t               psm,
                                  context_t           con_ctx,
                                  l2ca_controller_t   remote_control,
                                  l2ca_controller_t   local_control,
                                  DM_SM_SERVICE_T *substitute_security_service);
extern void L2CA_CreateChannelRsp(l2ca_identifier_t   identifier,
                                  l2ca_cid_t          cid,
                                  l2ca_conn_result_t  response,
                                  context_t           con_ctx);
extern void L2CA_MoveChannelReq(l2ca_cid_t          cid,
                                l2ca_controller_t   remote_control,
                                l2ca_controller_t   local_control);
extern void L2CA_MoveChannelRsp(l2ca_identifier_t   identifier,
                                l2ca_cid_t          cid,
                                l2ca_move_result_t  response);
#endif /* AMP_SUPPORT */

#ifdef INSTALL_L2CAP_LECOC_CB
extern void L2CA_AddCreditReq(l2ca_cid_t cid, 
                              context_t context, 
                              uint16_t credits);
#endif /* INSTALL_L2CAP_LECOC_CB */

#ifdef __cplusplus
}
#endif 
#endif /* _L2CAPLIB_H_ */
