/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/

#ifndef _L2CAP_CIDME_H
#define _L2CAP_CIDME_H

#include INC_DIR(common,common.h)
#include "l2cap_cid.h"
#include "l2cap_con_handle.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CID_ST_NULL              ((l2ca_state_t) 0)
#define CID_ST_OPEN              ((l2ca_state_t) 1)
#define CID_ST_DISCONNECT_LOCAL  ((l2ca_state_t) 2)
#define CID_ST_OPEN_FIXED        ((l2ca_state_t) 3)
#define CID_ST_OPEN_CB           ((l2ca_state_t) 4)

extern void CIDME_FreeCidcb(CIDCB_T *cidcb);
extern void CIDME_Unroute(CIDCB_T *cidcb);
extern struct cidtag *CIDME_GetCidcb(l2ca_cid_t cid);
extern struct cidtag *CIDME_GetCidcbWithHandle(const L2CAP_CHCB_T *chcb, l2ca_cid_t cid);

extern struct cidtag *CIDME_GetCidcbRemoteWithHandle(const L2CAP_CHCB_T *chcb, l2ca_cid_t remote_cid);

extern void CIDME_InitCidcb(CIDCB_T *cidcb,
                            L2CAP_CHCB_T *chcb,
                            l2ca_cid_t remote_cid,
                            struct psm_tag_t *psm_reg,
                            psm_t remote_psm,
                            context_t context);
extern context_t CIDME_RegistrationContext(const CIDCB_T *cidcb);

#ifdef __cplusplus
}
#endif 
#endif
