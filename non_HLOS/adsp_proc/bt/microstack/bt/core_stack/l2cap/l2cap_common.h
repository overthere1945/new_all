/*******************************************************************************

Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.


\brief  This file defines common internal values and types used with L2CAP.
*******************************************************************************/

#ifndef _L2CAP_COMMON_H
#define _L2CAP_COMMON_H

#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include "qbl_adapter_pmalloc.h"
#include INC_DIR(bluestack,l2cap_prim.h)
#include INC_DIR(mblk,mblk.h)
#include "l2cap_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! Macro to allocate a L2CAP primitive. */
#define L2CA_MAKE_PRIM(TYPE, p_handle)                                  \
    TYPE##_T *prim = zpnew(TYPE##_T); prim->type = TYPE; prim->phandle = p_handle

/*! /brief Signal reject codes */
enum
{
    SIGNAL_REJECT_UNKNOWN, /*!< Signal rejected due to unknown/misc. cause */
    SIGNAL_REJECT_MTU,     /*!< Signal rejected due to MTU violation */
    SIGNAL_REJECT_CID      /*!< Signal rejected due to invalid CID */
};


/*! \brief Internal frame/data packet types */
typedef enum
{
    L2CAP_FRAMETYPE_NONE,                     /*!< No Tx data type */
    L2CAP_FRAMETYPE_RAW,                      /*!< Raw frame */
    L2CAP_FRAMETYPE_SIGNAL,                   /*!< S-frame: request and response signal packets */
    L2CAP_FRAMETYPE_NORMAL,                   /*!< B-frame: Basic L2CAP packet */
    L2CAP_FRAMETYPE_CONNECTIONLESS,           /*!< G-frame: Connectionless L2CAP packet */
    L2CAP_FRAMETYPE_FLOW_CONTROL              /*!< I/S-frame: Flow and error control packets */
} FRAMETYPE_T;

/*! \brief Bitmasks to extract unique/random number from CID */
#define L2CAP_CID_NUMBER_MASK          (L2CA_CID_DYNAMIC_FIRST - 1) /* 0x003f */
#define L2CAP_CID_RANDOM_MASK          (~(L2CAP_CID_NUMBER_MASK))   /* 0xff40 */

/*! \brief L2CAP signalling bits */
#define L2CAP_SIGNAL_ID_MAX            (255)    /*!< Maximum signal id */
#define L2CAP_SIGNAL_CONTINUATION      (0x0001) /*!< Continuation bit in config flags */

/*! \brief Constant PDU header field offsets */
#define L2CAP_PKT_POS_LENGTH           (0)    /*!< Position of length field in L2CAP packet */
#define L2CAP_PKT_POS_CID              (2)    /*!< Position of CID field in L2CAP packet */
#define L2CAP_PKT_POS_PAYLOAD          (4)    /*!< Position of payload in Basic L2CAP packet */
#define L2CAP_CL_PKT_POS_LENGTH        (0)    /*!< Position of length field in Group L2CAP packet */
#define L2CAP_CL_PKT_POS_CID           (2)    /*!< Position of CID field in Group L2CAP packet */
#define L2CAP_CL_PKT_POS_PSM           (4)    /*!< Position of PSM in Group L2CAP packet */
#define L2CAP_CL_PKT_POS_PAYLOAD       (6)    /*!< Position of payload in Group L2CAP packet */
#define L2CAP_FLOW_POS                 (4)    /*!< Position of flow and error control header(s) */

/*! \brief Constant PDU header file sizes */
#define L2CAP_SIZEOF_SIGNAL_HEADER     (4)    /*!< Size of signal header in octets */
#define L2CAP_SIZEOF_CID_HEADER        (4)    /*!< Size of C/B-frame header fields (cid and length) */
#define L2CAP_SIZEOF_CL_HEADER         (6)    /*!< Size of CID+PSM header in CL L2CAP packets */
#define L2CAP_SIZEOF_IS_HEADER         (6)    /*!< Size of CID+CONTROL header in I/S packets */
#define L2CAP_SIZEOF_CID_FIELD         (2)    /*!< Size of CID field */
#define L2CAP_SIZEOF_PSM_FIELD         (2)    /*!< Size of PSM field in octets */
#define L2CAP_SIZEOF_STD_CONTROL_FIELD (2)    /*!< Size of standard/enhanced flow control field in octets */
#define L2CAP_SIZEOF_EXT_CONTROL_FIELD (4)    /*!< Size of extended flow control field in octets */
#define L2CAP_SIZEOF_SDU_LEN_FIELD     (2)    /*!< Size of flow control "SDU length" field in octets */
#define L2CAP_SIZEOF_FCS_FIELD         (2)    /*!< Size of Frame Check Sequence PDU trailer */
#define L2CAP_SIZEOF_STD_HEADER        (2+4)  /*!< Size of standard/enhanced flow control and CID header (no SDU, no FCS) */
#define L2CAP_SIZEOF_EXT_HEADER        (4+4)  /*!< Size of extended flow control and CID header (no SDU, no FCS) */

/*! \brief L2CAP configuration option sizes */
#define L2CAP_SIZEOF_OPTION_HEADER     0x02   /*!< L2CAP configuration options length */
#define L2CAP_SIZEOF_OPTION_MTU        0x02   /*!< L2CAP option size for MTU */
#define L2CAP_SIZEOF_OPTION_FLUSH      0x02   /*!< L2CAP option size for flush timeout */
#define L2CAP_SIZEOF_OPTION_QOS        0x16   /*!< L2CAP option size for Quality-of-service (22 decimal) */
#define L2CAP_SIZEOF_OPTION_FLOW       0x09   /*!< L2CAP option size flow & error control */
#define L2CAP_SIZEOF_OPTION_FCS        0x01   /*!< L2CAP option size for optional FCS */
#define L2CAP_SIZEOF_OPTION_FLOWSPEC   0x10   /*!< L2CAP option size for AMP extended flow spec (16 decimal) */
#define L2CAP_SIZEOF_OPTION_EXT_WINDOW 0x02   /*!< L2CAP option size for extended window size */
#define L2CAP_MAX_SIZEOF_OPTION_UNKNOWN 0xFF  /*!< L2CAP option data maximum size for unknown option type */

/*! \brief Reject signal sizes */
#define L2CAP_SIGNAL_SIZEOF_REJECT     0x06   /*!< Size of a command reject "not understood" */
#define L2CAP_SIGNAL_SIZEOF_INV_MTU    0x08   /*!< Size of a command reject "MTU exceeded" */
#define L2CAP_SIGNAL_SIZEOF_INV_CID    0x0A   /*!< Size of a command reject "invalid CID" */

/*! \brief F&EC: Segmentation and reassembly values */
#define L2CAP_SAR_UNSEGMENTED          0x00   /*!< PDU contains a complete SDU */
#define L2CAP_SAR_START                0x01   /*!< PDU is the start frame of segmented SDU */
#define L2CAP_SAR_END                  0x02   /*!< PDU is final frame of segmented SDU */
#define L2CAP_SAR_CONTINUE             0x03   /*!< PDU is somewhere in between a start and end frame */


/*! \brief F&EC: Max sequence numbers */
#define L2CAP_STD_MAX_SEQUENCE         0x40   /*!< Maximum sequence number  (0x0 - 0x3f) */
#define L2CAP_EXT_MAX_SEQUENCE         0x4000 /*!< Extended window sequence (0x0 - 0x3fff) */

/*! \brief Basic types and lengths for extended mode */
/*! \brief Header and sequence number sizes */
#define L2CAP_SIZEOF_MAX_HEADER        (8)    /*!< Size of largest possible frame header (I+SDU no FCS) */
#define L2CAP_FLOW_SEQ_BITS            (6)    /*!< L2CAP1.2 requires (at least) 6 bits per sequence */
#define L2CAP_SIZEOF_SEQNO_T           (1)    /*!< Sizeof sequence number is 1 byte */
typedef uint8_t  FLOWSEQ_T;                   /*!< Standard L2CAP requires 8 bits, 1 byte */
typedef uint16_t FLOWCTRL_T;                  /*!< Control header native CPU format */


/* Define which HCI error codes received during disconnect are 'normal' */
#define L2CAP_IS_ERROR_NORMAL(x)    ((x) == (uint16_t)HCI_ERROR_OETC_USER || \
                                     (x) == (uint16_t)HCI_ERROR_OETC_POWERING_OFF || \
                                     (x) == (uint16_t)HCI_ERROR_CONN_TERM_LOCAL_HOST)

#ifdef INSTALL_L2CAP_LECOC_CB
#define L2CAP_LECOC_MAX_RX_CREDITS          65535
#endif

#ifdef __cplusplus
}
#endif 

#endif
