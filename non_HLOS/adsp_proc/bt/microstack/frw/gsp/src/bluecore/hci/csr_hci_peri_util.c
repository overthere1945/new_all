/******************************************************************************
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_hci_lib.h"
#include "csr_hci_peri_util.h"
#include "csr_pmem.h"
#include "hci_arbiter_private.h"

CsrUint8 PeriCmdInternal;


void SendPeriBtOffCommand(void)
{
    CsrUint8 *data;
    data = CsrPmemAlloc(HCI_PERI_ACTIVATE_SS_CMD_LENGTH);

    data[0] = 0x00;
    data[1] = 0xF1;
    data[2] = 0xFF;
    data[3] = 0x03;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x00;

    PeriCmdInternal = 1;

    CsrPeriCommandReqSend(HCI_PERI_ACTIVATE_SS_CMD_LENGTH, data);
}
