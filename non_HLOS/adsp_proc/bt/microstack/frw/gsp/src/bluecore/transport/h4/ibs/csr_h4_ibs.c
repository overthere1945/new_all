/******************************************************************************
 Copyright (c) 2017-2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_h4_ibs.h"

#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_macro.h"
#include "csr_tm_bluecore_h4.h"
#include "csr_h4_ext.h"

#ifdef CSR_TARGET_PRODUCT_WEARABLE
CsrUint8 txSleepTriggered;
#endif

#ifdef CSR_LOG_ENABLE
/*
static const CsrCharString *h4IbsSubOrigins[] =
{
  "General",
  "TX",
  "RX",
  "Power",
};
*/
static const CsrCharString *TEXT_IBS_POWER_STATE[] =
{
 "OFF",
 "ON"
};

static const CsrCharString *TEXT_IBS_RX_STATE[] =
{
 "AWAKE",
 "SLEEP"
};

static const CsrCharString *TEXT_IBS_TX_STATE[] =
{
 "SENDING",
 "AWAKE",
 "SLEEP",
 "WAKING"
};

static const CsrCharString *TEXT_IBS_TX_EVENT[] =
{
 "DATA TO TRANSMIT",
 "TRANSMIT COMPLETE",
 "IDLE TIMEOUT",
 "WAKE TIMEOUT",
 "WAKE ACK"
};
#endif


static CsrBool csrH4IbsTxStateMachine(CsrH4IbsInstance *ibsInst, CsrUint8 event);
static void csrH4IbsActivate(CsrH4IbsInstance *ibsInst);
static CsrBool csrH4IbsIsActivated(CsrH4IbsInstance *ibsInst, CsrUint8 message);


/* Calculates current power state */
static void csrH4IbsUartVote(CsrH4IbsInstance *ibsInst)
{
    CsrUint8 powerState;

    if (ibsInst->txState == CSR_H4_IBS_TX_STATE_SLEEP &&
        ibsInst->rxState == CSR_H4_IBS_RX_STATE_SLEEP)
    {
        powerState = CSR_H4_IBS_POWER_STATE_OFF;
    }
    else
    {
        powerState = CSR_H4_IBS_POWER_STATE_ON;
    }

    if (powerState != ibsInst->powerState)
    {
        if (CsrH4SetPowerState(powerState))
        {
            CSR_H4_IBS_POWER_STATE_SET(ibsInst, powerState);
        }
    }
}

#ifdef CSR_TARGET_PRODUCT_WEARABLE
static void csrH4IbstxSleepTriggered(CsrH4IbsInstance *ibsInst)
{
    txSleepTriggered = 0;
    csrH4IbsUartVote(ibsInst);
}
#endif

/* Sends IBS message to controller */
static void csrH4IbsTxSendSignal(CsrH4IbsInstance *ibsInst, CsrUint8 signal)
{
    ibsInst->ibsMsg.chan = signal; /* Rest of fields of ibsMsg are always 0 */

    CSR_LOG_TEXT_DEBUG((ibsInst->lto,
                        CSR_H4_IBS_LTSO_GEN,
                        "Send: 0x%02X",
                        signal));

    CsrH4SendExtData(CsrMemDup(&ibsInst->ibsMsg, sizeof(ibsInst->ibsMsg)));
}

static void csrH4IbsTxIdleTimeoutHandler(CsrUint16 unused, void *context)
{
    CsrH4IbsInstance *ibsInst = (CsrH4IbsInstance *) context;

    ibsInst->idleTimer = CSR_SCHED_TID_INVALID;
    csrH4IbsTxStateMachine(ibsInst, CSR_H4_IBS_TX_EV_IDLE_TIMEOUT);
}

static void csrH4IbsWakeTimeoutHandler(CsrUint16 unused, void *context)
{
    CsrH4IbsInstance *ibsInst = (CsrH4IbsInstance *) context;

    ibsInst->wakeRetryTimer = CSR_SCHED_TID_INVALID;
    csrH4IbsTxStateMachine(ibsInst, CSR_H4_IBS_TX_EV_WAKE_TIMEOUT);
}

/****************** TX state machine action functions *************************/
typedef void (*CsrH4IbsTxActionFunc)(CsrH4IbsInstance *ibsInst);

static void txActionSendSleepReq(CsrH4IbsInstance *ibsInst)
{
    csrH4IbsTxSendSignal(ibsInst, CSR_H4_IBS_MSG_SLEEP);

    CSR_H4_IBS_TX_STATE_SET(ibsInst, CSR_H4_IBS_TX_STATE_SLEEP);
#ifdef CSR_TARGET_PRODUCT_WEARABLE
    /* For wearable : Power off the transport after sleep signal is sent down to avoid race condition of
     * UART transport getting powered off before sleep byte is picked for transmission from the Tx queue */
    txSleepTriggered = 1;
#else
    csrH4IbsUartVote(ibsInst);
#endif
}

static void txActionTriggerTransmit(CsrH4IbsInstance *ibsInst)
{
    CSR_H4_IBS_STOP_TIMER(ibsInst->idleTimer);
    CSR_H4_IBS_STOP_TIMER(ibsInst->wakeRetryTimer);
    CSR_H4_IBS_WAKE_RETRY_COUNT_RESET(ibsInst);

    CsrH4StartTransmit();

    CSR_H4_IBS_TX_STATE_SET(ibsInst, CSR_H4_IBS_TX_STATE_SENDING);
}

static void txActionSendWakeReq(CsrH4IbsInstance *ibsInst)
{
    CSR_H4_IBS_TX_STATE_SET(ibsInst, CSR_H4_IBS_TX_STATE_WAKING);
    csrH4IbsUartVote(ibsInst);

    csrH4IbsTxSendSignal(ibsInst, CSR_H4_IBS_MSG_WAKE);
    CSR_H4_IBS_WAKE_RETRY_COUNT_INC(ibsInst);

    CSR_H4_IBS_RESTART_TIMER(ibsInst->wakeRetryTimer,
                             CSR_H4_IBS_TX_AWAKE_TIME * (ibsInst->wakeRetryCount + 1),
                             csrH4IbsWakeTimeoutHandler,
                             0,
                             ibsInst);
}

static void txActionRestartIdleTimer(CsrH4IbsInstance *ibsInst)
{
    CSR_H4_IBS_RESTART_TIMER(ibsInst->idleTimer,
                             CSR_H4_IBS_TX_IDLE_TIME,
                             csrH4IbsTxIdleTimeoutHandler,
                             0,
                             ibsInst);
    CSR_H4_IBS_TX_STATE_SET(ibsInst, CSR_H4_IBS_TX_STATE_AWAKE);
}

static void txActionNone(CsrH4IbsInstance *ibsInst)
{
    CSR_UNUSED(ibsInst);
}


static const CsrH4IbsTxActionFunc csrH4IbsTxActions[CSR_H4_IBS_TX_STATES][CSR_H4_IBS_TX_EVENTS] =
{
  /* CSR_H4_IBS_TX_STATE_SENDING */
  {
    txActionTriggerTransmit,    /* CSR_H4_IBS_TX_EV_DATA                */
    txActionRestartIdleTimer,   /* CSR_H4_IBS_TX_EV_TRANSMIT_COMPLETE   */
    NULL,                       /* CSR_H4_IBS_TX_EV_IDLE_TIMEOUT        */
    NULL,                       /* CSR_H4_IBS_TX_EV_WAKE_TIMEOUT        */
    txActionNone                /* CSR_H4_IBS_TX_EV_WAKE_ACK            */
  },

  /* CSR_H4_IBS_TX_STATE_AWAKE */
  {
    txActionTriggerTransmit,    /* CSR_H4_IBS_TX_EV_DATA                */
    txActionNone,               /* CSR_H4_IBS_TX_EV_TRANSMIT_COMPLETE   */
    txActionSendSleepReq,       /* CSR_H4_IBS_TX_EV_IDLE_TIMEOUT        */
    NULL,                       /* CSR_H4_IBS_TX_EV_WAKE_TIMEOUT        */
    txActionNone                /* CSR_H4_IBS_TX_EV_WAKE_ACK            */
  },

  /* CSR_H4_IBS_TX_STATE_SLEEP */
  {
    txActionSendWakeReq,        /* CSR_H4_IBS_TX_EV_DATA                */
#ifdef CSR_TARGET_PRODUCT_WEARABLE
    csrH4IbstxSleepTriggered,   /* CSR_H4_IBS_TX_EV_TRANSMIT_COMPLETE   */
#else
    txActionNone,               /* CSR_H4_IBS_TX_EV_TRANSMIT_COMPLETE   */
#endif
    NULL,                       /* CSR_H4_IBS_TX_EV_IDLE_TIMEOUT        */
    NULL,                       /* CSR_H4_IBS_TX_EV_WAKE_TIMEOUT        */
    txActionSendSleepReq        /* CSR_H4_IBS_TX_EV_WAKE_ACK            */
  },

  /* CSR_H4_IBS_TX_STATE_WAKING */
  {
    txActionNone,               /* CSR_H4_IBS_TX_EV_DATA                */
    txActionNone,               /* CSR_H4_IBS_TX_EV_TRANSMIT_COMPLETE   */
    NULL,                       /* CSR_H4_IBS_TX_EV_IDLE_TIMEOUT        */
    txActionSendWakeReq,        /* CSR_H4_IBS_TX_EV_WAKE_TIMEOUT        */
    txActionTriggerTransmit     /* CSR_H4_IBS_TX_EV_WAKE_ACK            */
  },
};

static CsrBool csrH4IbsTxStateMachine(CsrH4IbsInstance *ibsInst, CsrUint8 event)
{
    if (csrH4IbsTxActions[ibsInst->txState][event])
    {
        CSR_LOG_TEXT_DEBUG((ibsInst->lto,
                            CSR_H4_IBS_LTSO_TX,
                            "Event: %s",
                            TEXT_IBS_TX_EVENT[event]));

        (csrH4IbsTxActions[ibsInst->txState][event])(ibsInst);

        return (TRUE);
    }
    else
    {
        CSR_LOG_TEXT_WARNING((ibsInst->lto,
                              CSR_H4_IBS_LTSO_TX,
                              "Unexpected event: %d",
                              event));
        return (FALSE);
    }
}


/**************************** IBS message handlers ****************************/
static void csrH4IbsRxWakeIndHandler(CsrH4IbsInstance *ibsInst)
{
    CSR_H4_IBS_RX_STATE_SET(ibsInst, CSR_H4_IBS_RX_STATE_AWAKE);
    csrH4IbsUartVote(ibsInst);
    csrH4IbsTxSendSignal(ibsInst, CSR_H4_IBS_MSG_WAKE_ACK);

    /* Optimization: Considering any incoming message other than SLEEP request
     * as WAKE_ACK while IBS is waking TX path. */
    if (ibsInst->txState == CSR_H4_IBS_TX_STATE_WAKING)
    {
        csrH4IbsTxStateMachine(ibsInst, CSR_H4_IBS_TX_EV_WAKE_ACK);
    }
}

static void csrH4IbsRxSleepIndHandler(CsrH4IbsInstance *ibsInst)
{
    CSR_H4_IBS_RX_STATE_SET(ibsInst, CSR_H4_IBS_RX_STATE_SLEEP);
#ifdef CSR_TARGET_PRODUCT_WEARABLE
    if (!txSleepTriggered)
#endif
    {
        csrH4IbsUartVote(ibsInst);
    }
}

static void csrH4IbsRxWakeAckHandler(CsrH4IbsInstance *ibsInst)
{
    csrH4IbsTxStateMachine(ibsInst, CSR_H4_IBS_TX_EV_WAKE_ACK);
}

/* Handles data from controller */
static CsrUint16 csrH4IbsRxHandler(void *inst, CsrUint8 * const *writePtr)
{
    CsrH4IbsInstance *ibsInst = (CsrH4IbsInstance *) inst;
    CsrUint8 message = **writePtr;

    if (csrH4IbsIsActivated(ibsInst, message))
    {
#if 0 /* QCA UART driver autonomously feeds WAKE msg on waking from sleep */
        if (ibsInst->powerState == CSR_H4_IBS_POWER_STATE_OFF)
        { /* Any packet received during low power state should be considered as wake request. */
            csrH4IbsRxWakeIndHandler(ibsInst);
        }
        else
#endif
        {
            switch (message)
            {
                case CSR_H4_IBS_MSG_WAKE:
                    csrH4IbsRxWakeIndHandler(ibsInst);
                    break;
                case CSR_H4_IBS_MSG_SLEEP:
                    csrH4IbsRxSleepIndHandler(ibsInst);
                    break;
                case CSR_H4_IBS_MSG_WAKE_ACK:
                    csrH4IbsRxWakeAckHandler(ibsInst);
                    break;
                default:
                    CsrH4SyncLossHandler(CSR_H4_SYNC_LOSS_TYPE_INVALID_HDR);
                    break;
            }
        }
    }

    return (0); /* IBS packets are 1 byte message, so we don't need to read anymore */
}

/* Handles statuses from H4 */
static void csrH4IbsStatusHandler(void *inst, CsrUint8 status, void *opaque)
{
    CsrH4IbsInstance *ibsInst = (CsrH4IbsInstance *) inst;

    if (ibsInst->activated)
    {
        switch (status)
        {
            case CSR_H4_STATUS_TX_QUEUE_EMPTY:
                csrH4IbsTxStateMachine(ibsInst,
                                       CSR_H4_IBS_TX_EV_TRANSMIT_COMPLETE);
                break;

            case CSR_H4_STATUS_TX_QUEUE_READY:
                csrH4IbsTxStateMachine(ibsInst, CSR_H4_IBS_TX_EV_DATA);
                break;

            case CSR_H4_STATUS_RX_HCI:
                /* Optimization: Considering any incoming message other than SLEEP request
                 * as WAKE_ACK while IBS is waking TX path. */
                if (ibsInst->txState == CSR_H4_IBS_TX_STATE_WAKING)
                {
                    csrH4IbsTxStateMachine(ibsInst, CSR_H4_IBS_TX_EV_WAKE_ACK);
                }
                break;

            default:
                CSR_LOG_TEXT_WARNING((ibsInst->lto,
                                      CSR_H4_IBS_LTSO_GEN,
                                      "Unknown H4 status=%d received",
                                      status));
                break;
        }
    }
    else
    { /* IBS is still inactive */
        switch (status)
        {
            case CSR_H4_STATUS_TX_QUEUE_EMPTY:
                /* Do nothing */
                break;

            case CSR_H4_STATUS_TX_QUEUE_READY:
                /* In inactive state, H4IBS must act as stand-alone H4.
                 * IBS must straight away allow H4 to transmit HCI packets. */
                CsrH4StartTransmit();
                break;

            case CSR_H4_STATUS_RX_HCI:
                /* Do nothing */
                break;

            default:
                CSR_LOG_TEXT_WARNING((ibsInst->lto,
                                      CSR_H4_IBS_LTSO_GEN,
                                      "H4 status=%d in inactive state",
                                      status));
        }
    }

    CSR_UNUSED(opaque);
}

/* Activates IBS */
static void csrH4IbsActivate(CsrH4IbsInstance *ibsInst)
{
    CSR_LOG_TEXT_INFO((ibsInst->lto, CSR_H4_IBS_LTSO_GEN, "Activated"));
    ibsInst->activated = TRUE;

    CSR_H4_IBS_POWER_STATE_SET(ibsInst, CSR_H4_IBS_POWER_STATE_ON);

    /* This makes H4 inform IBS if transmit queue is empty */
    txActionTriggerTransmit(ibsInst);
}

static CsrBool csrH4IbsIsActivated(CsrH4IbsInstance *ibsInst, CsrUint8 message)
{
    if (!ibsInst->activated)
    {
        switch (message)
        {
            case CSR_H4_IBS_MSG_WAKE:
            case CSR_H4_IBS_MSG_SLEEP:
            case CSR_H4_IBS_MSG_WAKE_ACK:
                /* Any IBS message from BT controller is treated as IBS activation */
                csrH4IbsActivate(ibsInst);
                break;

            default:
                CSR_LOG_TEXT_ERROR((ibsInst->lto,
                                   CSR_H4_IBS_LTSO_RX,
                                   "0x%02X received in inactive state",
                                   message));
                CsrH4SyncLossHandler(CSR_H4_SYNC_LOSS_TYPE_INVALID_HDR);
                break;
        }
    }

    return (ibsInst->activated);
}

void CsrTmBlueCoreH4IbsInit(void **gash)
{
    CsrH4IbsInstance *ibsInst = (CsrH4IbsInstance *) CsrPmemZalloc(sizeof(*ibsInst));

#ifdef CSR_TARGET_PRODUCT_WEARABLE
    txSleepTriggered = 0;
#endif

    CSR_LOG_TEXT_REGISTER(&(ibsInst->lto),
                          "IBS",
                          CSR_ARRAY_SIZE(h4IbsSubOrigins),
                          h4IbsSubOrigins);

    CSR_LOG_TEXT_INFO((ibsInst->lto, CSR_H4_IBS_LTSO_GEN, "Initialized"));
    ibsInst->activated = FALSE; /* IBS is activated on receiving first IBS message from controller */

    CsrTmBlueCoreH4Init(gash);
    CsrH4RegisterExtension(TRANSPORT_TYPE_H4IBS,
                           ibsInst,
                           csrH4IbsRxHandler,
                           csrH4IbsStatusHandler);
}

