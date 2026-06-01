/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#ifndef __HCI_ARBITER_EVENT_H__
#define __HCI_ARBITER_EVENT_H__

#include "bluetooth.h"
#include "csr_hci_prim.h"
#include "hci_arbiter_int.h"
#include "hci_arbiter_lib.h"


#ifdef __cplusplus
extern "C" {
#endif

#define BT_HCI_CRASH_EVENT_MIN_SIZE        9
#define PERI_HCI_CRASH_EVENT_MIN_SIZE      10

#define CRASH_EVENT_CODE                   0xFF
#define BT_CRASH_EVENT_CLASS               0x01
#define PERI_CRASH_EVENT_CLASS             0xF0
#define BT_CRASH_EVENT_MSG_TYPE            0x09
#define PERI_CRASH_EVENT_MSG_TYPE          0x05

#define BTSS_SMC_CRASH                     0xB3

void HciArbProcessHciEvents(CsrHciEventInd *prim);

void HciArbProcessCommandCompleteEventForVendorSpecificCommand(CsrHciVendorSpecificEventInd *prim);

void HciArbProcessPeriEvents(CsrPeriEventInd *prim);

#ifdef __cplusplus
}
#endif

#endif /* __HCI_ARBITER_EVENT_H__ */

