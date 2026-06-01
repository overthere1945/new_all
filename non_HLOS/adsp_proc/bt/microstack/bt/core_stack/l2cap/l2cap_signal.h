/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 2001

*******************************************************************************/

#ifndef _L2CAP_SIGNAL_H    /* Once is enough */
#define _L2CAP_SIGNAL_H

#include <stdarg.h>
#include INC_DIR(common,common.h)
#include INC_DIR(bluestack,l2cap_prim.h)
#include "l2cap_common.h"
#include "l2cap_types.h"

#include "qbl_adapter_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Signal types

    This enum lists all the possible signal types that are used in L2CAP.
*/
typedef enum signal_type
{
    SIGNAL_COMMAND_REJECT = 0x02,
#if defined (DISABLE_DM_BREDR) && defined (INSTALL_L2CAP_LECOC_CB)
    SIGNAL_DISCONNECT_REQ = 0x06,
    SIGNAL_DISCONNECT_RES = 0x07,
#endif

#ifdef INSTALL_ULP
#ifdef INSTALL_L2CAP_LECOC_CB
    SIGNAL_LE_CREDIT_BASED_CONN_REQ = 0x14,
    SIGNAL_LE_CREDIT_BASED_CONN_RSP = 0x15,
    SIGNAL_LE_FLOW_CONTROL_CREDIT = 0x16,
#endif /* INSTALL_L2CAP_LECOC_CB */
#endif
    SIGNAL_UNKNOWN
} SIGNAL_T;


/*! \brief Signal minimum lengths
 
    This enum lists the minimum lengths of relevant L2CAP signals. */
typedef enum
{
#if defined (DISABLE_DM_BREDR) && defined (INSTALL_L2CAP_LECOC_CB)
    SIGNAL_DISCONNECT_REQ_MIN_LENGTH = 0x0004,
    SIGNAL_DISCONNECT_RES_MIN_LENGTH = 0x0004,
#endif

#ifdef INSTALL_ULP
    SIGNAL_LE_CREDIT_BASED_CONN_REQ_MIN_LENGTH = 0x000a,
    SIGNAL_LE_CREDIT_BASED_CONN_RSP_MIN_LENGTH = 0x000a,
    SIGNAL_LE_FLOW_CONTROL_CREDIT_MIN_LENGTH = 0x0004,
#endif
} L2CAP_SIGNAL_MIN_LENGTH_T;


/*! \brief Signal Control Block.

    The signal control block holds the parameters and data needed for
    L2CAP signal transmission and re-transmission.  Signals can be
    stored on a queue pending tranmission or pending ackowledgment.
    Each signal has an independent retranmission (ERTX or RTX) timer.
*/
typedef struct sig_signal_tag
{
    struct sig_signal_tag                *next_ptr;             /*!< Pointer tonext signal */

    BITFIELD(uint8_t,                     signal_type, 5);      /*!< Signal type */
    BITFIELD(uint8_t,                     signal_id, 8);        /*!< Unique signal ID */
#if (L2CAP_RTX_RETRIES < 2) && (L2CAP_ERTX_RETRIES < 2)
    BITFIELD(uint8_t,                     rtx_timer_count, 1);  /*!< Number of RTX retries left */
    BITFIELD(uint8_t,                     ertx_timer_count, 1); /*!< Number of ERTX retries left */
#else
    uint8_t                               rtx_timer_count;      /*!< Number of RTX retries left */
    uint8_t                               ertx_timer_count;     /*!< Number of ERTX retries left */
#endif
    context_t                             req_ctx;              /*!< User context */
    MBLK_T                               *signal;               /*!< Pointer tosignal payload in MBLK */
    tid_t                                 timer_id;             /*!< ERTX/RTX Timer ID */
    l2ca_cid_t                            local_cid;            /*!< Local CID for this signal */
    phandle_t                             p_handle;             /*!< P-Handle for this signal, used for ECHO/GET_INFO signals */
} SIG_SIGNAL_T;

extern SIG_SIGNAL_T *SIG_SignalCreate(l2ca_cid_t local_cid,
                                      uint16_t signal_size, 
                                      uint8_t signal_type,
                                      l2ca_identifier_t signal_id,
                                      unsigned int count,
                                      ...);

#if defined(INSTALL_ULP) && defined(INSTALL_L2CAP_LECOC_CB)
extern SIG_SIGNAL_T *SIG_SignalLEFlowControlCredit(l2ca_cid_t local_cid,
                                                   uint16_t credits);
#endif /* defined(INSTALL_ULP) && defined(INSTALL_L2CAP_LECOC_CB) */

#ifdef __cplusplus
}
#endif 
#endif

