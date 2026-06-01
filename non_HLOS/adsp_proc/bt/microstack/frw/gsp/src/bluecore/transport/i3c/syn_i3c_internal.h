#ifndef SYN_I3C_INTERNAL_H__
#define SYN_I3C_INTERNAL_H__

/******************************************************************************
 Copyright (c) 2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_log_text_2.h"
#include "csr_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HCI packet type defines from H4 */
#define CSR_HCI_HDR_CMD    0x01
#define CSR_HCI_HDR_ACL    0x02
#define CSR_HCI_HDR_SCO    0x03
#define CSR_HCI_HDR_EVT    0x04
#define CSR_HCI_HDR_ISO    0x05

/* PERI packet type defines from H4 */
#define CSR_PERI_HDR_CMD   0x31
#define CSR_PERI_HDR_EVT   0x34

/* I3C Transport State */
typedef enum
{
    SYN_I3C_STATE_STOPPED = 0,
    SYN_I3C_STATE_STARTED
} SynI3cState;

/* I3C Instance */
typedef struct
{
    SynI3cState state;
    struct CsrTmBlueCoreTransport *transport;
} SynI3cInstance;

/* Log Text Sub-origins */
#define SYN_I3C_LTSO_GEN    0
#define SYN_I3C_LTSO_TX     1
#define SYN_I3C_LTSO_RX     2

/* TX function - only one actually needed */
void SynI3cTxSendMsg(void *msg);

#ifdef __cplusplus
}
#endif

#endif /* SYN_I3C_INTERNAL_H__ */
