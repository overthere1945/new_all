/*******************************************************************************

Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef _L2CAP_CONFIG_H
#define _L2CAP_CONFIG_H

#include INC_DIR(bluestack,l2cap_prim.h)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILD_FOR_HOST
#define INSTALL_L2CAP_DATA_ABORT_SUPPORT
#endif

#define L2CAP_MAX_NUM_CIDS        (32)

/* Maximum MTU: We need to allow for a few extra bytes for the
 * extended L2CAP headers */
#define L2CAP_MTU_MAXIMUM         (0xFFF0)

/* Left-shift random number: 1<<6 = 0x40. This controls how many bits
 * the random part of local CIDs can take up. Do NOT change this
 * unless you know what you're doing! */
#define L2CAP_CID_RANDOM_LSH      (6)
#define L2CAP_CID_INVALID_INDEX   0xFFFF


#define L2CAP_SIGNAL_STD_MTU_MAX       (48)     /*!< Outgoing signalling MTU defined in spec up to 2.1 */
#define L2CAP_SIGNAL_EXT_MTU_MAX       (672)    /*!< Outgoing signalling MTU for 3.0+ devices */
#define L2CAP_SIGNAL_BLE_MTU_MAX       (23)     /*!< Low energy MTU is fixed at 23 octets */

/*! \brief Lengths used for checking against ACL packets */
#define L2CAP_ACL_MTU_MAX              (L2CAP_SIZEOF_MAX_HEADER - L2CAP_SIZEOF_CID_HEADER + L2CAP_SIZEOF_FCS_FIELD)
#define L2CAP_ACL_RAW_LENGTH_MAX       (L2CAP_SIZEOF_MAX_HEADER + L2CAP_SIZEOF_FCS_FIELD)

/* Number of RTX and ERTX retries */
#define L2CAP_RTX_RETRIES         (1)
#define L2CAP_ERTX_RETRIES        (1)


/* The special HCI handle used for broadcasts - we just need to make
 * sure we're out of the normal HCI assignment range :-D */
#define L2CAP_BCAST_HCI_HANDLE    (0x0BBC)


#define L2CAP_MAX_TX_QUEUES                      (3)


/* This defines number of record of signalling request
   L2CAP can have for each Channel Control Block (CHCB) for
   detection of duplicate request.
   L2CAP_NUM_DUPLICATE_SIGNAL_RECORD number of last signal
   request's ID, type and length are stored.
 */
#define L2CAP_NUM_DUPLICATE_SIGNAL_RECORD       3



#ifdef __cplusplus
}
#endif

#endif
