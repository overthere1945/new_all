/*******************************************************************************
Copyright (C) 2008 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\brief  L2CAP private header file
*******************************************************************************/
#ifndef _L2CAP_PRIVATE_
#define _L2CAP_PRIVATE_

#include "qbl_adapter_hci.h"                    /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_panic.h"                  /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_pmalloc.h"                /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_types.h"                  /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_logging.h"                /* QBLUESTACK_REFACTOR */

#include <string.h>
#include INC_DIR(common,common.h)
#include INC_DIR(common,error.h)
#include "l2cap_chme.h"
#include "l2cap_cid.h"
#include "l2cap_cidme.h"
#include "l2cap_common.h"
#include "l2cap_con_handle.h"
#include "l2cap_config.h"
#include "l2cap_control.h"
#include "l2cap_flow.h"
#include "l2cap_interface.h"
#include INC_DIR(bluestack,l2cap_prim.h)
#include "l2cap_signal.h"
#include "l2cap_sig_handle.h"
#include "l2cap_types.h"
#include INC_DIR(l2caplib,l2caplib.h)
#include INC_DIR(mblk,mblk.h)
#include "qbl_patch.h"
#include INC_DIR(tbdaddr,tpbdaddr.h)
#include "dm_private.h"

#include "l2cap_int_prim.h"
#include "hci_arbiter_private.h"
#include "l2caplib_private.h"

#include INC_DIR(bluestack,bluetooth.h)         /* QBLUESTACK_REFACTOR */


#include INC_DIR(common,bkeyval.h) /* Is there a reason this is not a 'self contained' header? */

void L2CAINT_ProcessPassthroughDataReq(L2CA_INTERNAL_PASSTHROUGH_DATA_REQ_T *req);
void L2CAINT_PassthroughDataReq(uint16_t length, void *p_data);

typedef uint8_t protocol_t;
#define PROTOCOL_ATT      ((protocol_t)0x01)
#define PROTOCOL_RFCOMM   ((protocol_t)0x02)
#define PROTOCOL_LECOC    ((protocol_t)0x03)

CIDCB_T * CIDME_AllocCidCb(protocol_t protocol);

#define L2CAP_LOG_ENABLE

#if defined(L2CAP_LOG_ENABLE)
#define L2CAP_INFO(...) CSR_LOG_TEXT_INFO((L2CAP , 0, __VA_ARGS__))
#define L2CAP_DEBUG(...) CSR_LOG_TEXT_INFO((L2CAP , 0, __VA_ARGS__))
#define L2CAP_WARNING(...)  CSR_LOG_TEXT_WARNING((L2CAP , 0, __VA_ARGS__))
#define L2CA_ERROR(...)  CSR_LOG_TEXT_ERROR((L2CAP , 0, __VA_ARGS__))
#define L2CAP_PANIC(code, str)  {CsrPanic(CSR_TECH_BT, code, str);} 
#else
#define L2CAP_INFO(...)
#define L2CAP_DEBUG(...)
#define L2CAP_WARNING(...)
#define L2CA_ERROR(...)
#define L2CAP_PANIC(code, str)  {CsrPanic(CSR_TECH_BT, code, str);} 
#endif 

#endif /* _L2CAP_PRIVATE_ */

