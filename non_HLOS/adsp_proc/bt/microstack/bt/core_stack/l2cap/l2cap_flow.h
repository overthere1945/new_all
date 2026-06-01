/*******************************************************************************

Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef _L2CAP_FLOW_H_
#define _L2CAP_FLOW_H_

#include "l2cap_common.h"
#include "l2cap_config.h"
#include "l2cap_con_handle.h"
#include "l2cap_cid.h"
#include "qbl_macros.h"
#include INC_DIR(mblk,mblk.h)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_L2CAP_LECOC_CB

/*
 * Credit count limit for current/effective Rx credits (i.e. CURR_RX_CREDITS)
 * in order to control the trigger for a LE Flow Control Credit L2CAP PDU to
 * be sent to the peer.
 */
/* 1/3rd of total allocated Rx credits or 2, whichever is greater so that 
 * Credit replenishment is faster when the total credits is very less.
 * Note: When current/effective Rx credit becomes 1, the communication gets
 *       serialized and hence not making full use of sending multiple pkts
 *       without waiting for credits, leading to throughput reduction.
 */
#define L2CAP_LE_COC_CURR_RX_CREDITS_THRESHOLD(cnt)     MAX(2, ROUND_DIV(cnt, 3))
#endif /* #ifdef INSTALL_L2CAP_LECOC_CB */

#if defined(INSTALL_L2CAP_ENHANCED_SUPPORT) || defined(INSTALL_L2CAP_LECOC_CB)
/*! Placeholder for in/out going flow control packet info */
typedef struct FLOWPKT_T_tag
{
    struct FLOWPKT_T_tag    *next;
    MBLK_T                  *mblk;
    context_t                context;
    uint16_t                 sdu_length;
    BITFIELD(uint8_t,        sdu_mode, 2);
    BITFIELD(FLOWSEQ_T,      seq, L2CAP_FLOW_SEQ_BITS);
    uint8_t                  retrans;
#ifdef INSTALL_L2CAP_DATA_ABORT_SUPPORT
    L2CAP_ABORTED_PACKET_T  *aborted;   /*!< List of subsequent packets aborted */
#endif
} FLOWPKT_T;
#endif


#ifdef INSTALL_L2CAP_LECOC_CB
extern bool_t CB_FLOW_DataRead(CIDCB_T *cidcb, MBLK_T *mblk,
                                uint16_t payload_size);
extern void CB_FLOW_Free(CIDCB_T *cidcb, l2ca_data_result_t result);
extern void CB_FLOW_FatalError(CIDCB_T *cidcb,
                                MBLK_T *mblk,
                                l2ca_disc_result_t error_code);
extern void CB_Flow_AttemptToSendCredits(CIDCB_T *cidcb);
extern void CB_FLOW_DataReadAck(CIDCB_T *cidcb, uint16_t pkt_ack_cnt);
extern void CB_FLOW_DataWrite(CIDCB_T *cidcb, MBLK_T *mblk, context_t context);
extern void CB_FLOW_Reschedule(CIDCB_T *cidcb);
extern void CB_FLOW_FlushQueue(CIDCB_T *cidcb, l2ca_data_result_t result);

#endif /* #ifdef INSTALL_L2CAP_LECOC_CB */
#ifdef __cplusplus
}
#endif

#endif

