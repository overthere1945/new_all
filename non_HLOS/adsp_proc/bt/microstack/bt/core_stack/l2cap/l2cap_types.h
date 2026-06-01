/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef _L2CAP_TYPES_H
#define _L2CAP_TYPES_H

 #include "l2cap_common.h"
 #include "csr_message_queue.h"
#ifdef __cplusplus
extern "C" {
#endif

/*! \brief L2CAP sizes for data to be sent over HCI */
typedef struct
{
    uint16_t header;
    uint16_t body;
    uint16_t trailer;
} L2CAP_CH_DATA_SIZES_T;

/*! \brief Received Request signal record holder

    This is a placeholder to deal with received request signals. We
    store identifier, type and length of a signal received.
*/
typedef struct sigattrtag
{
    BITFIELD(uint8_t,                   type, 8);       /*!< Signal Type */
    BITFIELD(uint8_t,                   identifier, 8); /*!< Signal Identifier */
    uint16_t                            length;         /*!< Signal Length (including header) */
} SIG_ATTR_T;

/*! \brief Duplicate request signal records

    This is a placeholder to store last L2CAP_NUM_DUPLICATE_SIGNAL_RECORD
    request signals received.
    We store identifier, lenght and type for these signals.
    In addition it stores the last response sent if it has a
    corresponding last request record.
*/
typedef struct dupsigrecordtag
{
    SIG_ATTR_T                          sig_attr[L2CAP_NUM_DUPLICATE_SIGNAL_RECORD];
                                         /*!< Received signal attributes ID, length and type holder */
    uint8_t                             last_req;
                                         /*!< duplicate signal array index containing place where last
                                           signal request is added */
    MBLK_T                              *sig_data;      /*!< Last Response signal data (if it has
                                                             corresponding last request in stored records) */
} DUPSIG_RECORD_T;



/*! \brief Transmit queue

    The generalised transmit queue used both by the BR/EDR path
    directly from the CHCB structure, and an AMP transmit queue.
*/
typedef struct champqtag
{
    BITFIELD(uint16_t, credits_taken, 7);
    BITFIELD(bool_t,                      tx_active, 1);        /*!< Transmit loop active, prevent callback deadlock */
    BITFIELD(uint8_t, reserved_credit, 1);                      /*!< Our own private credit */
    struct txqetag                       *tx_queue[L2CAP_MAX_TX_QUEUES]; /*!< Transmit queue */
    struct txqetag                       *tx_done;              /*!< Completed queue */
    struct txqetag                       *tx_current;           /*!< Current element under transmission */
} TXQUEUE_T;

/*! \brief ACL reassembly buffer

    For host builds, the ACL reassembly function is a part of L2CAP.
    For this, we need a place to store non-complete packets.
*/
typedef struct
{
    MBLK_T                               *mblk;                 /*!< Reassembly buffer MBLK */
    l2ca_fs_flush_t                       length;               /*!< Expected length - total size can be 2^16+4 */
    l2ca_fs_flush_t                       offset;               /*!< How much received so far - total size can be 2^16+4 */
    l2ca_cid_t                            local_cid;            /*!< Local cid */
} CHCB_REASSEMBLE_T;

#ifdef GATT_SEQUENCING

typedef enum
{
    ATT_REQ_STATE_IDLE,
    ATT_REQ_RECEIVED,
    ATT_REQ_SENT,
}ATT_REQ_MSG_STATE;

typedef enum
{
    ATT_INDICATION_STATE_IDLE,
    ATT_INDICATION_RECEIVED,
    ATT_INDICATION_SENT
}ATT_IND_MSG_STATE;


typedef enum
{
    ATT_MSG_RECEIVED,
    ATT_MSG_SENT,
}ATT_MSG_ACTION;

/*! \brief HCI Att arbitration data.

    This holds the various message states required for ATT arbitration.
*/

typedef struct
{
    ATT_REQ_MSG_STATE       attReqMsgStateMicroStack;
    ATT_REQ_MSG_STATE       attReqMsgStatePrimaryStack;
    ATT_IND_MSG_STATE       attIndMsgStateMicroStack;
    ATT_IND_MSG_STATE       attIndMsgStatePrimaryStack;

    bool                    lockTakenMicroStack;
    bool                    lockTakenPrimaryStack;
    CsrMessageQueueType     *saveMicroStackQueue;
    CsrMessageQueueType     *savePrimaryStackQueue;
}hciAttArbData_t;
#endif /* GATT_SEQUENCING */

/*! \brief Connection Handle Control Block.

    The Connection Handle Control Block holds the parameters needs to
    manage a L2CAP connection.  Each L2CAP connection will have an
    instance of this control block.
*/
typedef struct chtag
{
    struct cidtag                        *cidcb_list;           /*!< Pointer to first CIDCB instance for this connection */

    CHCB_REASSEMBLE_T                     reassem;              /*!< On-host reassembly buffer */

    BITFIELD(bool_t,                      signal_scheduled, 1); /*!< Next available signal ID and duplicate avoidance buffer */
    BITFIELD(l2ca_identifier_t,           signal_id, 8);        /*!< Last outgoing signal ID */
    DUPSIG_RECORD_T                       signal_dup_record;    /*!< Duplicate signal request records and last response holder */
    struct sig_signal_tag                *signal_queue;         /*!< Queue of signals */
    struct sig_signal_tag                *signal_pending;       /*!< Queue of signals pending response */
    TXQUEUE_T                             queue;                /*!< BR/EDR transmit queue and friends */
#ifdef INSTALL_SOCKET_OFFLOAD
    MBLK_T                                *data;                /*!< Fragmented ACL packets from primary stack stored till complete PDU is received */
    int16_t                               bytes_pending;        /*!< No of bytes pending for framing complete PDU *data*/
#endif
#ifdef GATT_SEQUENCING
    hciAttArbData_t                       hciAttArbData;        /*!< HCI ATT arbitration data */
#endif /* GATT_SEQUENCING */
} L2CAP_CHCB_T;

#ifdef __cplusplus
}
#endif

#endif
