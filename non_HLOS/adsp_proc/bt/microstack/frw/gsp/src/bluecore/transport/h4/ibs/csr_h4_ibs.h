/*****************************************************************************

 Copyright (c) 2017-2018 Qualcomm Technologies International, Ltd.

 All Rights Reserved.

 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*****************************************************************************/

#ifndef CSR_H4_IBS_H__
#define CSR_H4_IBS_H__

#include "csr_sched.h"
#include "csr_types.h"
#include "csr_log_text_2.h"
#include "csr_transport.h"

#ifdef __cplusplus
extern "C" {
#endif


/* IBS overall power state */
#define CSR_H4_IBS_POWER_STATE_OFF          (0)
#define CSR_H4_IBS_POWER_STATE_ON           (1)

/* IBS messages */
#define CSR_H4_IBS_MSG_WAKE_ACK             (0xFC)
#define CSR_H4_IBS_MSG_WAKE                 (0xFD)
#define CSR_H4_IBS_MSG_SLEEP                (0xFE)

#define CSR_H4_IBS_MSG_CRASH                (0xFB)  /* Message to induce crash in controller */

/* IBS TX path states */
#define CSR_H4_IBS_TX_STATE_SENDING         (0)
#define CSR_H4_IBS_TX_STATE_AWAKE           (1)
#define CSR_H4_IBS_TX_STATE_SLEEP           (2)
#define CSR_H4_IBS_TX_STATE_WAKING          (3)

#define CSR_H4_IBS_TX_STATES                (4) /* Number of TX states */

/* IBS TX path events */
#define CSR_H4_IBS_TX_EV_DATA               (0)
#define CSR_H4_IBS_TX_EV_TRANSMIT_COMPLETE  (1)
#define CSR_H4_IBS_TX_EV_IDLE_TIMEOUT       (2)
#define CSR_H4_IBS_TX_EV_WAKE_TIMEOUT       (3)
#define CSR_H4_IBS_TX_EV_WAKE_ACK           (4)

#define CSR_H4_IBS_TX_EVENTS                (5) /* Number of TX events */

/* IBS RX path states */
#define CSR_H4_IBS_RX_STATE_AWAKE           (0)
#define CSR_H4_IBS_RX_STATE_SLEEP           (1)

/* Idle time before IBS should put TX path in sleep mode (SLEEP) */
#define CSR_H4_IBS_TX_IDLE_TIME             (CSR_H4IBS_TX_IDLE_TIMEOUT * CSR_SCHED_MILLISECOND)

/* Idle time before IBS should retry waking (send WAKE) controller when
 * there is no acknowledgement (receive WAKE_ACK) */
#define CSR_H4_IBS_TX_AWAKE_TIME            (CSR_H4IBS_TX_WAKE_RETRY_TIMEOUT * CSR_SCHED_MILLISECOND)


/* Sub Origins */
#define CSR_H4_IBS_LTSO_GEN                 (1)
#define CSR_H4_IBS_LTSO_TX                  (2)
#define CSR_H4_IBS_LTSO_RX                  (3)
#define CSR_H4_IBS_LTSO_POWER               (4)

typedef struct
{
    CsrBool activated;
    CsrUint8 powerState;
    CsrUint8 rxState;
    CsrUint8 txState;

    TXMSG ibsMsg;                   /* Placeholder for transmitting IBS messages */

#ifdef CSR_LOG_ENABLE
    CsrLogTextHandle *lto;          /* Log Text Handle */
#endif

    CsrSchedTid idleTimer;
    CsrSchedTid wakeRetryTimer;
    CsrUint32 wakeRetryCount;
} CsrH4IbsInstance;


#define CSR_H4_IBS_RX_STATE_SET(_ibsInst, _state)                               \
    do                                                                          \
    {                                                                           \
        if ((_ibsInst)->rxState != _state)                                      \
        {                                                                       \
            CSR_LOG_TEXT_DEBUG(((_ibsInst)->lto,                                \
                                CSR_H4_IBS_LTSO_RX,                             \
                                "State changed: %s -> %s",                      \
                                TEXT_IBS_RX_STATE[(_ibsInst)->rxState],         \
                                TEXT_IBS_RX_STATE[_state]));                    \
            (_ibsInst)->rxState = _state;                                       \
        }                                                                       \
    } while (0)

#define CSR_H4_IBS_TX_STATE_SET(_ibsInst, _state)                               \
    do                                                                          \
    {                                                                           \
        if ((_ibsInst)->txState != _state)                                      \
        {                                                                       \
            CSR_LOG_TEXT_DEBUG(((_ibsInst)->lto,                                \
                                CSR_H4_IBS_LTSO_TX,                             \
                                "State changed: %s -> %s",                      \
                                TEXT_IBS_TX_STATE[(_ibsInst)->txState],         \
                                TEXT_IBS_TX_STATE[_state]));                    \
            (_ibsInst)->txState = _state;                                       \
        }                                                                       \
    } while (0)


#define CSR_H4_IBS_POWER_STATE_SET(_ibsInst, _state)                            \
    do                                                                          \
    {                                                                           \
        if ((_ibsInst)->powerState != _state)                                   \
        {                                                                       \
            CSR_LOG_TEXT_DEBUG(((_ibsInst)->lto,                                \
                                CSR_H4_IBS_LTSO_POWER,                          \
                                "State changed: %s -> %s",                      \
                                TEXT_IBS_POWER_STATE[(_ibsInst)->powerState],   \
                                TEXT_IBS_POWER_STATE[_state]));                 \
            (_ibsInst)->powerState = _state;                                    \
        }                                                                       \
    } while (0)


/* Stops and resets the timer */
#define CSR_H4_IBS_STOP_TIMER(_tid)                                             \
    do                                                                          \
    {                                                                           \
        if (_tid != CSR_SCHED_TID_INVALID)                                      \
        {                                                                       \
            CsrSchedTimerCancel(_tid, NULL, NULL);                              \
            _tid = CSR_SCHED_TID_INVALID;                                       \
        }                                                                       \
    } while (0)


/* Restarts the timer */
#define CSR_H4_IBS_RESTART_TIMER(_tid, _delay, _handler, _fniarg, _fnvarg)      \
    do                                                                          \
    {                                                                           \
        CSR_H4_IBS_STOP_TIMER(_tid);                                            \
        _tid = CsrSchedTimerSet(_delay, _handler, _fniarg, _fnvarg);            \
    } while (0)


#define CSR_H4_IBS_WAKE_RETRY_COUNT_RESET(_ibsInst) (_ibsInst)->wakeRetryCount = 0

#if CSR_H4_IBS_WAKE_RETRY_COUNT_MAX
#define CSR_H4_IBS_WAKE_RETRY_COUNT_INC(_ibsInst)                               \
    do                                                                          \
    {                                                                           \
        (_ibsInst)->wakeRetryCount++;                                           \
        if ((_ibsInst)->wakeRetryCount > CSR_H4_IBS_WAKE_RETRY_COUNT_MAX)       \
        {                                                                       \
            csrH4IbsTxSendSignal(_ibsInst, CSR_H4_IBS_MSG_CRASH);               \
            CsrH4SyncLossHandler(CSR_H4_SYNC_LOSS_NO_RESPONSE);                 \
        }                                                                       \
    } while (0)
#else
#define CSR_H4_IBS_WAKE_RETRY_COUNT_INC(_ibsInst)   (_ibsInst)->wakeRetryCount++
#endif

#ifdef __cplusplus
}
#endif

#endif
