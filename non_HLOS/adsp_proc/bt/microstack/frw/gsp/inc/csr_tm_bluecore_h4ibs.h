#ifndef CSR_TM_BLUECORE_H4IBS_H__
#define CSR_TM_BLUECORE_H4IBS_H__

/*******************************************************************************

 Copyright (c) 2017 Qualcomm Technologies International, Ltd.

 All Rights Reserved.

 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "csr_synergy.h"
#include "csr_tm_bluecore_h4.h"

#ifdef __cplusplus
extern "C" {
#endif


void CsrTmBlueCoreH4IbsInit(void **gash);
#define CsrTmBlueCoreRegisterUartHandleH4Ibs    CsrTmBlueCoreRegisterUartHandleH4

#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_BLUECORE_H4IBS_H__ */

