/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

\brief  This file defines the interface to the L2CAP channel state machine.
*******************************************************************************/

#ifndef _L2CAP_CID_H
#define _L2CAP_CID_H

#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include INC_DIR(common,common.h)
#include INC_DIR(mblk,mblk.h)
#include "l2cap_config.h"
#include "l2cap_types.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef uint16_t l2ca_state_t;

/*! \brief LE credit based flow control credit signal block

    This signal block holds the parameters from the LE credit based flow control
    (FC) credit signal, this block is used to provide Tx credits to local L2CAP
    device equivalent to the Rx credits relinquished by peer L2CAP device.
*/
typedef struct
{
    l2ca_identifier_t     signal_id;
    uint16_t              length;
    uint16_t              dest_cid;
    uint16_t              credits;
} CID_LE_CREDIT_BASED_FC_CREDIT_T;


/*! \brief Is CID using a Credit Based Flow Control mode? */
#ifdef INSTALL_L2CAP_LECOC_CB
#define CID_IsCBFlowControl(cidcb) ((cidcb)->cb_flow != NULL)

#define CID_CheckCreditOverflow(total_credit,extra_credit) \
    (((uint32_t)(total_credit) + (extra_credit)) > L2CAP_LECOC_MAX_RX_CREDITS)

#else
#define CID_IsCBFlowControl(cidcb) (FALSE)
#endif

/*! \brief Check for fixed channel */
#ifdef INSTALL_L2CAP_FIXED_CHANNEL_BASE_SUPPORT
#define CID_IsFixed(cidcb) ((cidcb)->remote_cid < L2CA_CID_DYNAMIC_FIRST \
        && (cidcb)->remote_cid != L2CA_CID_INVALID)
#else
#define CID_IsFixed(cidcb) (FALSE)
#endif

#define CID_DataPhandle(cidcb) ((cidcb)->p_handle)

#define CID_IsFixedWithTX(cidcb) CID_IsFixed(cidcb)

#define CID_GetTxQueue(cidcb) (&(cidcb->chcb->queue))

#define L2CA_PRIMARY_STACK_PRIORITY      0

#ifdef INSTALL_SOCKET_OFFLOAD
#define L2CA_SIGNALLING_PRIORITY         1
#else
#define L2CA_SIGNALLING_PRIORITY         0
#endif
#define L2CA_MICROSTACK_PRIORITY         1


#ifdef INSTALL_L2CAP_LECOC_CB
typedef struct CB_FLOWINFO_T_tag
{
    uint16_t                tx_credits;          /*!< Number of PDUs that can be sent to peer */
    uint16_t                rx_credits;          /*!< Number of PDUs that peer can send us (currently) */
    uint16_t                tot_rx_credits;      /*!< Total Rx credits provided to peer */
    uint16_t                app_rx_credits;      /*!< Application current Rx credits */
    struct FLOWPKT_T_tag*   tx_head;             /*!< Transmit PDU queue head element */
    struct FLOWPKT_T_tag*   tx_tail;             /*!< Transmit PDU queue tail element */
    l2ca_mtu_t              local_mps;           /*!< Local maximum payload size */
    l2ca_mtu_t              remote_mps;          /*!< Remote maximum payload size */
    L2CA_DATAREAD_IND_T*    l2ca_dataread_ind;   /*!< Data primitive being reassembled */
#if defined(INSTALL_L2CAP_ECBFC) || defined(INSTALL_L2CAP_LECOC_TX_SEG)
    uint16_t                sar_offset;          /*!< SDU segmentation offset for transmissions */
#endif /* INSTALL_L2CAP_ECBFC || INSTALL_L2CAP_LECOC_TX_SEG */
    uint16_t                sdu_len_remaining;   /*!< length of remaining SDU to be assembled */
    BITFIELD(bool_t,        flow_off, 1);        /*!< Indicate application to stop transmitting */
} CB_FLOWINFO_T;
#endif

/*! \brief Channel Control Block.

    The Channel Control Block holds the parameters needs to manage a L2CAP
    channel.  Each L2CAP channel will have an instance of this control block.
*/
typedef struct cidtag
{
    struct cidtag                        *next_ptr;             /*!< Pointer to next CIDCB */
    struct chtag                         *chcb;                 /*!< Pointer to parent ACL */
    l2ca_cid_t                            local_cid;            /*!< Local channel ID */
    l2ca_cid_t                            remote_cid;           /*!< Remote channel ID */
    l2ca_mtu_t                            local_mtu;            /*!< Local MTU */
    l2ca_mtu_t                            remote_mtu;           /*!< Remote MTU */
    l2ca_timeout_t                        local_flush_to;       /*!< Local flush timeout */
    struct psm_tag_t                      *local_psm;           /*!< Pointer to local PSM structure */
    psm_t                                 remote_psm;           /*!< Remote PSM */
    phandle_t                             p_handle;             /*!< P-Handle for control primitives */
    context_t                             context;              /*!< Opaque context number for upper layer */
    BITFIELD(uint8_t,                     priority, 2);         /*!< L2CAP queue priority. */

    l2ca_state_t                          state;                /*!< The state of the CID */

#ifdef INSTALL_L2CAP_LECOC_CB
    CB_FLOWINFO_T                        *cb_flow;              /*!< Placeholder for credit based flow control */
#endif
#ifdef INSTALL_SOCKET_OFFLOAD
    l2ca_cid_t                            offload_cid;          /*!< CID shared by Primary stack */    
#endif
} CIDCB_T;



/* Data path - bypasses state machine */
extern void CID_DataWriteReq(CIDCB_T *cidcb, MBLK_T **mblk_ptr, context_t context);
extern bool_t CID_DataReadInd(CIDCB_T *cidcb, MBLK_T *mblk_ptr, mblk_size_t mblk_size, uint8_t *header);

/* Utility functions */
extern void CID_DisconnectCleanup(CIDCB_T *cidcb);


void CID_DataReadFatalError(CIDCB_T *cidcb,
                            MBLK_T *mblk,
                            l2ca_disc_result_t error_code);

#ifdef __cplusplus
}
#endif
#endif
