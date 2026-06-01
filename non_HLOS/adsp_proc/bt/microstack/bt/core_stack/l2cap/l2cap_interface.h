/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 2001

*******************************************************************************/

#ifndef _L2CAP_INTERFACE_H
#define _L2CAP_INTERFACE_H

#include "l2cap_config.h"
#include "l2cap_cid.h"
#include "l2cap_signal.h"
#include "qbl_adapter_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! Macro to simplifying send L2CAP primitives. */
#define L2CA_PrimSend(prim)                             \
    L2CA_PutMessage(PHANDLE_TO_QUEUEID((prim)->phandle), L2CAP_PRIM, prim)

/* Send internal L2CAP primitives. */
#define L2CA_PutIntMsg(prim) put_message(L2CAP_IFACEQUEUE,\
                                    GEN_INT_PRIM_ID(L2CAP_PRIM), (prim))

/*
 * Internal L2CAP primitives NOT to have ANY VARIABLE SIZED DATA MEMBERS. Hence
 * simply free the memory allocated for the primitive.
 */
#define L2CA_FreeIntPrimitive(prim) pfree((prim))


/* Scheduler functions */
extern void L2CAP_Init(void **gash);
extern void L2CAP_DeInit(void **gash);
extern void L2CAP_InterfaceHandler(void **gash);

/* ACL manager callback */
extern void dm_acl_client_deinit_l2cap(L2CAP_CHCB_T *chcb);

/* Generic sender (also used by auto-connect) */
/*lint -sem(L2CA_PutMessage, custodial(3)) */
extern void L2CA_PutMessage(qid queue_id, uint16_t msg_id, void *prim);

/* Upstream functions */
#if defined(INSTALL_L2CAP_LECOC_CB)
extern void L2CA_DisconnectInd(CIDCB_T *cidcb, uint8_t signal_id, l2ca_disc_result_t reason);
#endif

extern void L2CA_DataReadInd(const CIDCB_T *const cidcb, MBLK_T *mblk_ptr, l2ca_data_result_t result, uint16_t packets, L2CA_DATAREAD_IND_T **return_prim);
extern void L2CA_DataWriteCfm(CIDCB_T *cidcb, context_t context, uint16_t length, l2ca_data_result_t result);

extern void L2CA_BuildDataWriteCfm(CIDCB_T *cidcb, context_t context, uint16_t length, l2ca_data_result_t result, L2CA_DATAWRITE_CFM_T *prim);


/* BLE */
#ifdef INSTALL_L2CAP_LECOC_CB
void L2CA_AddCreditCfm(phandle_t phandle, l2ca_cid_t cid, context_t context,
                        uint16_t credits, L2CA_RESULT_T result);
#endif

#ifdef __cplusplus
}
#endif 
#endif
