#ifndef CSR_H4_RX_H__
#define CSR_H4_RX_H__
/*****************************************************************************

      Copyright (c) 2016 Qualcomm Technologies International, Ltd.

      All Rights Reserved.

      Qualcomm Technologies International, Ltd. Confidential and Proprietary. 

*****************************************************************************/


#include "csr_sched.h"
#include "csr_types.h"


#ifdef __cplusplus
extern "C" {
#endif

#define CSR_H4_MAX_PKT_SIZE                     (4)

/* H4 RX states */
#define CSR_H4_RX_STATE_PKT_HDR                 (0)
#define CSR_H4_RX_STATE_HCI_HDR                 (1)
#define CSR_H4_RX_STATE_HCI_BODY                (2)
#define CSR_H4_RX_STATE_EXT_DATA                (3)

typedef struct
{
    CsrUint8 state;

    CsrUint8 h4Header;
    CsrUint8 hciHeader[CSR_H4_MAX_PKT_SIZE];

    CsrUint16 hciPacketLength;
    CsrUint8 *hciPacket;

    CsrUint8 *writePtr;
    CsrUint16 bytesToRead;

#if CSR_H4_PKT_READ_TIMEOUT
    CsrSchedTid pktReadTimer;
#endif

    CsrSchedBgint bgintReassemble;
} CsrH4RxInstance;

void CsrH4RxInit(CsrH4RxInstance *rx);
void CsrH4RxDeinit(CsrH4RxInstance *rx);

#define CSR_H4_RX_STATE_SET(_rx, _state)                                \
    do                                                                  \
    {                                                                   \
        if ((_rx)->state != _state)                                     \
        {                                                               \
            CSR_LOG_TEXT_DEBUG((CsrH4Lto,                               \
                                CSR_H4_LTSO_RX,                         \
                                "Rx state changed: %d -> %d",           \
                                (_rx)->state,                           \
                                _state));                               \
            (_rx)->state = _state;                                      \
        }                                                               \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif
