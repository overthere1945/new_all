/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CSR_TM_BLUECORE_IPC_H__
#define CSR_TM_BLUECORE_IPC_H__

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrTmBlueCoreIpcInit(void **gash);
void CsrTmBlueCoreRegisterIpcDrvHandle(void *handle);

#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_BLUECORE_IPC_H__ */

