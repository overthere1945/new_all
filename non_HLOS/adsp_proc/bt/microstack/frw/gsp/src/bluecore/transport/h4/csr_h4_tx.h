#ifndef CSR_H4_TX_H__
#define CSR_H4_TX_H__
/*******************************************************************************

 Copyright (c) 2016 Qualcomm Technologies International, Ltd.

 All Rights Reserved.

 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "csr_sched.h"
#include "csr_time.h"
#include "csr_types.h"
#include "csr_transport.h"


#ifdef __cplusplus
extern "C" {
#endif

/* H4 TX states */
#define CSR_H4_TX_STATE_HDR                     (0)
#define CSR_H4_TX_STATE_BODY                    (1)

typedef struct
{
    CsrUint8 state;

    TXMSG *msgQ;

#ifdef CSR_H4_EXTENSION
    TXMSG *extMsgQ;
    CsrUint8 extDataCount;
#endif

    CsrSchedTid overflowWaitTimer;
    CsrTime timePerByte;

    CsrSchedBgint bgint;
    CsrSchedBgint bgintIsr;
} CsrH4TxInstance;

CsrBool CsrH4TxSendMsg(CsrH4TxInstance *tx, TXMSG *msg);
void CsrH4TxStartTransmit(CsrH4TxInstance* tx);
void CsrH4TxInit(CsrH4TxInstance *tx);
void CsrH4TxDeinit(CsrH4TxInstance *tx);


#define CSR_H4_TX_STATE_SET(_tx, _state)                                \
    do                                                                  \
    {                                                                   \
        if ((_tx)->state != _state)                                     \
        {                                                               \
            CSR_LOG_TEXT_DEBUG((CsrH4Lto,                               \
                                CSR_H4_LTSO_TX,                         \
                                "Tx state changed: %d -> %d",           \
                                (_tx)->state,                           \
                                _state));                               \
            (_tx)->state = _state;                                      \
        }                                                               \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif
