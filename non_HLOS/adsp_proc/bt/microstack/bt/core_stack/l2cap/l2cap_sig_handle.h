/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef _L2CAP_SIG_HANDLE_H
#define _L2CAP_SIG_HANDLE_H

#include INC_DIR(mblk,mblk.h)
#include INC_DIR(common,common.h)
#include "l2cap_signal.h"

#ifdef __cplusplus
extern "C" {
#endif


extern void SIGH_SignalSend(L2CAP_CHCB_T *chcb, SIG_SIGNAL_T *signal_block);
extern void SIGH_SignalPresend(L2CAP_CHCB_T *chcb, TXQE_T *txqe);
extern void SIGH_SignalQueueEmptyWithCid(SIG_SIGNAL_T **sig_list, l2ca_cid_t cid);
extern void SIGH_SignalQueueEmpty(SIG_SIGNAL_T **sig_list, const BD_ADDR_T *const p_bd_addr);

#ifdef INSTALL_ULP
extern void SIGH_LeSignalReceive(L2CAP_CHCB_T *chcb, MBLK_T **mblk, uint16_t signal_size);
#endif

#ifdef __cplusplus
}
#endif
#endif
