/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

\brief  This file defines the interface for L2CAP connections, there is
        a one-to-one mapping between a L2CAP connection and an ACL.
*******************************************************************************/

#ifndef _L2CAP_CON_HANDLE_H
#define _L2CAP_CON_HANDLE_H

#include INC_DIR(mblk,mblk.h)
#include INC_DIR(common,common.h)
#include "l2cap_config.h"
#include "l2cap_types.h"
#include "l2cap_cid.h"

#include INC_DIR(tbdaddr,tbdaddr.h)

/* Get L2CA_CHCB_T from DM_ACL_T */
#define CH_GET_CHCB(p_acl) \
    DM_ACL_CLIENT_GET_DATA((p_acl), dm_acl_client_l2cap)

/* Get DM_ACL_T from L2CA_CHCB_T */
#define CH_GET_ACL(chcb) \
    DM_ACL_CLIENT_GET_ACL((chcb), dm_acl_client_l2cap)

/* Get BD_ADDR_T from L2CA_CHCB_T */
#define CH_GET_TBDADDR(chcb) \
    DM_ACL_CLIENT_GET_TBDADDR((chcb), dm_acl_client_l2cap)
#define CH_GET_BD_ADDR(chcb) (&TBDADDR_ADDR(*CH_GET_TBDADDR(chcb)))

/* Get HCI Connection Handle from L2CA_CHCB_T */
#define CH_GET_HANDLE(chcb) \
    DM_ACL_CLIENT_GET_HANDLE((chcb), dm_acl_client_l2cap)

#define CH_GET_CHCB_FROM_QUEUE(q) \
    ((L2CAP_CHCB_T*)(((uint8_t*)(q)) - offsetof(L2CAP_CHCB_T, queue)))

/* Determine whether L2CA_CHCB_T is for BLE or BR/EDR */
#ifdef INSTALL_ULP
#define CH_IS_ULP(chcb)   dm_acl_is_ble(CH_GET_ACL(chcb))
#define CH_ULP_FLAG(chcb) DM_ACL_GET_CONNECTION_FLAGS(CH_GET_ACL(chcb))
#else
#define CH_IS_ULP(chcb) FALSE
#define CH_ULP_FLAG(chcb) ((l2ca_conflags_t)0)
#endif

#define CID_GET_CONN_FLAG(cidcb) DM_ACL_GET_CONNECTION_FLAGS(CH_GET_ACL((cidcb)->chcb))

#ifdef SUPPORT_SEPARATE_LE_BUFFERS
#define CH_GET_ACL_TYPE(chcb)  ((!CH_IS_ULP(chcb))? \
            DM_AMP_ACL_TYPE_BR_EDR : DM_AMP_ACL_TYPE_LE)
#define L2CA_FLAGS_ULP(flags) \
            (((flags) > DM_ACL_BR_EDR_FLAG_THRESHOLD)? TRUE:FALSE)
#else
#define CH_GET_ACL_TYPE(chcb)  DM_AMP_ACL_TYPE_BR_EDR
#define L2CA_FLAGS_ULP(flags)  FALSE
#endif

#define CH_IS_CONNECTIONLESS(chcb) FALSE

/* Determine whether L2CA_CHCB_T is connected. Connectionless
   channel is alwasy connected. */
#define CH_IS_CONNECTED(chcb) dm_acl_is_connected(CH_GET_ACL(chcb))


/* The outgoing signalling MTU depends on AMP support */
#ifdef INSTALL_AMP_SUPPORT
#define CH_GET_SIGNAL_MTU(chcb) \
    ((chcb)->signal_mtu)
#else
#define CH_GET_SIGNAL_MTU(chcb) \
    (L2CAP_SIGNAL_STD_MTU_MAX)
#endif

/* Get signal channel ID for CHCB (BT/LE) */
#ifdef INSTALL_ULP
#define CH_GET_SIGNAL_CID(chcb) (CH_IS_ULP(chcb) ? L2CA_CID_LE_SIGNAL \
                                                 : L2CA_CID_SIGNAL)
#else
#define CH_GET_SIGNAL_CID(chcb) (L2CA_CID_SIGNAL)
#endif

/* Get AMP-enabled builds to compile */
#ifndef HCI_BR_EDR_PHANDLE
#define HCI_BR_EDR_PHANDLE INVALID_PHANDLE
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief linkable L2CA_DATAWRITE_CFMs and L2CA_DATAWRITE_ABORT_CFMs

    These can be sent as L2CA_DATAWRITE_CFMs or L2CA_DATAWRITE_ABORT_CFMs
    and can also be linked together in a list. The list is
    used to store confirms for aborted packets until
    they can be sent to the application. */
typedef struct L2CAP_ABORTED_PACKET_T_tag
{
    union
    {
        L2CA_UPSTREAM_COMMON_T common;
        L2CA_DATAWRITE_ABORT_CFM_T abort_cfm;
        L2CA_DATAWRITE_CFM_T datawrite_cfm;
    } u;
    struct L2CAP_ABORTED_PACKET_T_tag *next;
} L2CAP_ABORTED_PACKET_T;

/*! \brief Common transmit queue element

    This structure serves as the low-level placeholder for data which
    is about to be sent over an ACL. Once the element has been sent, a
    completion function is called (based on the 'type') together with
    pointer to the current CHCB, calldata and context.

    The transmit function will issue a callback _before_ transmitting
    anything in case the MBLK is a NULL-pointer. In this way, the FEC
    and signal handling code can decide for it's own what data to
    send.

    Note that the 'trailersize' and 'trailersent' on-host is used for
    the forged McDSP packets so we need the full 16 bit length
    range. On-chip is can only ever be used for the FCS, hence only 3
    bits.
*/
typedef struct txqetag
{
        struct txqetag                       *next;                 /*!< Next queue element */
        BITFIELD(FRAMETYPE_T,                 type, 4);             /*!< Frame/data type */
        BITFIELD(uint8_t,                     headersize, 6);       /*!< Real header size in octets */
        BITFIELD(uint16_t,                    trailersize, 3);      /*!< Trailer size in octets */
        BITFIELD(uint16_t,                    trailersent, 3);      /*!< Octets of trailer already sent */
        MBLK_T                               *mblk;                 /*!< MBLK chain */
        l2ca_cid_t                            cid;                  /*!< Transmitting cid (local) */
        context_t                             context;              /*!< Datawrite req/cfm context */
        l2ca_timeout_t                        flush;                /*!< Flush timeout */
        mblk_size_t                           sent;                 /*!< Octets of the MBLK sent so far */
        uint16_t                              credits;              /*!< Outstanding credits for this element */
#ifdef INSTALL_L2CAP_DATA_ABORT_SUPPORT
        L2CAP_ABORTED_PACKET_T               *aborted;              /*!< List of subsequent packets aborted */
#endif
#ifndef INSTALL_L2CAP_CRC
        uint8_t                               trailer[2];           /*!< Trailing bytes, used for off-chip CRC */
#endif
        uint8_t                               header[1];            /*!< Extendable frame header */
} TXQE_T;

/* Transmit queue handling */
#ifdef INSTALL_L2CAP_CONNLESS_SUPPORT
extern void CH_CLDataSendQueued(void);
#else
#define CH_CLDataSendQueued() ((void)0)
#endif
extern void CH_DataSendQueued(L2CAP_CHCB_T *chcb, TXQUEUE_T *queue, uint8_t priority, bool_t from_nhcp);
extern void CH_DataAddHeader(TXQE_T *txqe, uint16_t cid, uint16_t mblksize,
                             uint8_t *head, uint8_t headsize);
extern bool_t CH_DataTxBasic(struct cidtag *cidcb, context_t context, MBLK_T *mblk);
extern void CH_DataTxSignalCallback(L2CAP_CHCB_T *chcb);

/* Utility functions */
extern void CH_CleanupCidcbTx(L2CAP_CHCB_T *chcb, struct cidtag *cidcb);
extern void CH_DataRx(L2CAP_CHCB_T *chcb, MBLK_T *mblk, bool_t bcast);
extern void CH_CompletedPackets(L2CAP_CHCB_T *chcb, uint16_t completed_packets);

#ifdef INSTALL_L2CAP_DATA_ABORT_SUPPORT
extern void CH_EmptyAbortQueue(CIDCB_T *cidcb, L2CAP_ABORTED_PACKET_T **abort_queue, l2ca_data_result_t result);
#else
#define CH_EmptyAbortQueue(cidcb, abort_queue, result)
#endif /* INSTALL_L2CAP_DATA_ABORT_SUPPORT */

/* HCI data credit debugging. */
#ifdef L2CAP_HCI_DATA_CREDIT_SLOW_CHECKS
#ifdef INSTALL_L2CAP_CONNLESS_SUPPORT
uint16_t CH_CLUsedDataCredits(void);
#else
#define CH_CLUsedDataCredits() 0
#endif
uint16_t CH_UsedDataCredits(L2CAP_CHCB_T *chcb);
#endif

/* Cleanup functions */
extern void CH_FlushTxqe(TXQE_T *txqe, l2ca_data_result_t result);
extern uint16_t CH_FlushPendingQueueWithCid(TXQUEUE_T *queue, l2ca_cid_t cid, l2ca_data_result_t result);
extern void CH_FlushCidcbPendingTxDataQueue(L2CAP_CHCB_T *chcb, CIDCB_T *cidcb, l2ca_data_result_t result);
extern void CH_FlushCidcbTxSignalQueue(L2CAP_CHCB_T *chcb, CIDCB_T *cidcb);
extern uint16_t CH_FlushChcb(L2CAP_CHCB_T *chcb, l2ca_data_result_t result);
extern uint16_t CH_FlushDoneQueue(TXQUEUE_T *queue, l2ca_data_result_t result);

#ifdef INSTALL_L2CAP_LECOC_CB
uint16_t CH_getQueuedCnt(CIDCB_T *cidcb);
bool_t CH_isCBFC_dataFrame(CIDCB_T *cidcb, uint32_t type);
void CH_CBDataTxAppend(TXQUEUE_T *queue, TXQE_T *txqe, uint8_t priority);
#endif

/* ACL reassemble - implemented in l2cap_acl_reassembly.c */
#ifdef BUILD_FOR_HOST
extern MBLK_T *L2CA_AclReassemble(hci_connection_handle_t hci_flags, MBLK_T *mblk);
#endif

void CH_DataTxAppend(TXQUEUE_T *queue, TXQE_T *txqe, uint8_t priority);

#ifdef __cplusplus
}
#endif
#endif
