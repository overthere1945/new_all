/******************************************************************************
 Copyright (c) 2016-2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CSR_H4_H__
#define CSR_H4_H__

#include "csr_panic.h"
#include "csr_types.h"
#include "csr_macro.h"
#include "csr_log_text_2.h"
#include "csr_h4_rx.h"
#include "csr_h4_tx.h"
#include "csr_h4_ext.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CSR_H4_PKT_HDR_SIZE                     1

/* HCI Package Headers */
#define CSR_HCI_HDR_CMD                         0x01
#define CSR_HCI_HDR_ACL                         0x02
#define CSR_HCI_HDR_SCO                         0x03
#define CSR_HCI_HDR_EVT                         0x04
#define CSR_HCI_HDR_ISO                         0x05

#define CSR_PERI_HDR_CMD                        0x31
#define CSR_PERI_HDR_EVT                        0x34

/* HCI header sizes */
#define CSR_HCI_CMD_HDR_SIZE                    3   /* opcode=2, length=1 */
#define CSR_HCI_ACL_HDR_SIZE                    4   /* handle+flags=2, length=2 */
#define CSR_HCI_SCO_HDR_SIZE                    3   /* handle=2, length=1 */
#define CSR_HCI_EVT_HDR_SIZE                    2   /* event=1, length=1 */
#define CSR_HCI_ISO_HDR_SIZE                    4   /* handle+pbf+tsf+rfu=2, length=2 */
#define CSR_PERI_EVT_HDR_SIZE                   3   /* hostId =1, event=1, length=1 */

#define CSR_H4_ACL_LEN_FIELD_IDX                2
#define CSR_H4_SCO_LEN_FIELD_IDX                2
#define CSR_H4_EVT_LEN_FIELD_IDX                1
#define CSR_H4_ISO_LEN_FIELD_IDX                2
#define CSR_H4_PERI_EVT_LEN_FIELD_IDX           2

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrH4Lto);

/* Sub Origins */
#define CSR_H4_LTSO_GEN                         1
#define CSR_H4_LTSO_TX                          2
#define CSR_H4_LTSO_RX                          3


#define CSR_H4_STATE_UNINITIALIZED              0
#define CSR_H4_STATE_INITIALIZED                1
#define CSR_H4_STATE_STARTED                    2
#define CSR_H4_STATE_RESTARTED                  3

/* Extension protocol information */
typedef struct
{
    void *prtInst;
    CsrUint16 transportType;
    CsrH4RxExtDataHandler rxHandler;
    CsrH4StatusHandler statusHandler;
} CsrH4ExtInstance;

/* Instance data for H4 */
typedef struct
{
    CsrUint8 state;
    void *uartHandle;
    CsrH4TxInstance tx;
    CsrH4RxInstance rx;

#if CSR_H4_ALLOW_STRAY_BYTES
    CsrUint32 strayByteCount;               /* To keep count of stray bytes */
#endif

    void *transport;

#ifdef CSR_H4_EXTENSION
    CsrH4ExtInstance *extInst;
#endif
} CsrH4Instance;


/* H4 panic */
#define CsrH4Panic(_reason)         CsrPanic(CSR_TECH_FW, _reason, "h4 panic")

/* Extracts H4 instance from TX instance */
#define CSR_H4_INST_FROM_TX(_tx)                                                \
    ((CsrH4Instance *)(((CsrUint8 *) (_tx)) - CsrOffsetOf(CsrH4Instance, tx)))

/* Extracts H4 instance from RX instance */
#define CSR_H4_INST_FROM_RX(_rx)                                                \
    ((CsrH4Instance *)(((CsrUint8 *) (_rx)) - CsrOffsetOf(CsrH4Instance, rx)))

#ifdef CSR_H4_EXTENSION
/* Extracts transport type */
#define CSR_H4_GET_TRANSPORT_TYPE(_h4Inst) ((_h4Inst)->extInst ?                \
                (_h4Inst)->extInst->transportType :                             \
                TRANSPORT_TYPE_H4_UART)

/* H4 TX queue ready status handler */
#define CSR_H4_TX_QUEUE_READY(_tx)                                              \
    do                                                                          \
    {                                                                           \
        CsrH4ExtInstance *extInst = CSR_H4_INST_FROM_TX(_tx)->extInst;          \
        if (extInst && extInst->statusHandler)                                  \
        {                                                                       \
            extInst->statusHandler(extInst->prtInst,                            \
                                   CSR_H4_STATUS_TX_QUEUE_READY,                \
                                   NULL);                                       \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            CsrH4TxStartTransmit(_tx);                                          \
        }                                                                       \
    } while (0)

/* H4 TX queue empty status handler */
#define CSR_H4_TX_QUEUE_EMPTY(_tx)                                              \
    do                                                                          \
    {                                                                           \
        CsrH4ExtInstance *extInst = CSR_H4_INST_FROM_TX(_tx)->extInst;          \
        if (extInst && extInst->statusHandler)                                  \
        {                                                                       \
            extInst->statusHandler(extInst->prtInst,                            \
                                   CSR_H4_STATUS_TX_QUEUE_EMPTY,                \
                                   NULL);                                       \
        }                                                                       \
    } while (0)

/* H4 RX status handler */
#define CSR_H4_RX_HCI(_rx)                                                      \
    do                                                                          \
    {                                                                           \
        CsrH4ExtInstance *extInst = CSR_H4_INST_FROM_RX(_rx)->extInst;          \
        if (extInst && extInst->statusHandler)                                  \
        {                                                                       \
            extInst->statusHandler(extInst->prtInst,                            \
                                   CSR_H4_STATUS_RX_HCI,                        \
                                   NULL);                                       \
        }                                                                       \
    } while (0)

#else /* CSR_H4_EXTENSION */
#define CSR_H4_GET_TRANSPORT_TYPE(_h4Inst)  TRANSPORT_TYPE_H4_UART
#define CSR_H4_TX_QUEUE_READY               CsrH4TxStartTransmit
#define CSR_H4_TX_QUEUE_EMPTY(_tx)
#define CSR_H4_RX_HCI(_rx)
#endif /* CSR_H4_EXTENSION */

/* Stops and resets the timer */
#define CSR_H4_STOP_TIMER(_tid)                                                 \
    do                                                                          \
    {                                                                           \
        if (_tid != CSR_SCHED_TID_INVALID)                                      \
        {                                                                       \
            CsrSchedTimerCancel(_tid, NULL, NULL);                              \
            _tid = CSR_SCHED_TID_INVALID;                                       \
        }                                                                       \
    } while (0)


/* Restarts the timer */
#define CSR_H4_RESTART_TIMER(_tid, _delay, _handler, _fniarg, _fnvarg)          \
    do                                                                          \
    {                                                                           \
        CSR_H4_STOP_TIMER(_tid);                                                \
        _tid = CsrSchedTimerSet(_delay, _handler, _fniarg, _fnvarg);            \
    } while (0)


/* Check for the HCI messages */
#define CSR_H4_IS_HCI_PACKET(_msg)                                              \
    ((_msg)->chan == TRANSPORT_CHANNEL_HCI ||                                   \
     (_msg)->chan == TRANSPORT_CHANNEL_ACL ||                                   \
     (_msg)->chan == TRANSPORT_CHANNEL_SCO ||                                   \
     (_msg)->chan == TRANSPORT_CHANNEL_ISO ||                                   \
     (_msg)->chan == TRANSPORT_CHANNEL_PERI)

/* Extended H4 data handler */
CsrUint16 CsrH4ExtDataHandler(CsrUint8 * const *writePtr);

#ifdef __cplusplus
}
#endif

#endif
