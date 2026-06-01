#ifndef CSR_BT_LOG_TECH_INFO_REGISTER_H__
#define CSR_BT_LOG_TECH_INFO_REGISTER_H__
/******************************************************************************
 Copyright (c) 2010-2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#if defined(CSR_LOG_ENABLE) && !defined(CSR_TARGET_PRODUCT_VM)

#ifdef __cplusplus
extern "C" {
#endif

extern void CsrBtLogTechInfoRegister(void);

#ifdef __cplusplus
}
#endif

#endif /* CSR_LOG_ENABLE */
#endif
