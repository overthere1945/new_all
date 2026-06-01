/******************************************************************************
 Copyright (c) 2019-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
 ******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_core_stack_log.h"

#ifdef CSR_LOG_ENABLE
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtCoreStackLto);
#endif

#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)
static const CsrCharString *subOrigins[] =
{
    "FSM16",
    "FSM32"
};
#endif /* CSR_LOG_ENABLE */

void CsrBtCoreStackLogTextRegister(void)
{
    CSR_LOG_TEXT_REGISTER(&CsrBtCoreStackLto, "CORE_STACK", CSR_ARRAY_SIZE(subOrigins), subOrigins);
}
