#ifndef _CSR_BT_HCISIZELUT_H_
#define _CSR_BT_HCISIZELUT_H_
/******************************************************************************
 Copyright (c) 2010-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "qbl_hci_types.h"
#include "mblk.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrBtHcishimInsertLengthByOpcode(HCI_UPRIM_T *pv_hci_uprim);

#ifdef __cplusplus
}
#endif

#endif
