#ifndef CSR_BT_CORESTACK_LOG_H_
#define CSR_BT_CORESTACK_LOG_H_
/******************************************************************************
 Copyright (c) 2019-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
 ******************************************************************************/

#include "csr_synergy.h"
#include "csr_util.h"
#include "csr_log_text_2.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrBtCoreStackLogTextRegister(void);

CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtCoreStackLto);

/* Log suborigins */
#define CSR_BT_CORE_STACK_LTSO_FSM16          (1)
#define CSR_BT_CORE_STACK_LTSO_FSM32          (2)

#ifdef CSR_TARGET_PRODUCT_VM

#define BLUESTACK_WARNING_COMMON(who_msg, error_code)                           \
    do {                                                                        \
        SYNERGY_HYDRA_LOG_STRING(the_file, SYNERGY_FMT(__FILE__, bonus_arg));   \
        CSR_LOG_TEXT_WARNING((CsrBtCoreStackLto,                                \
                             0,                                                 \
                             who_msg ": ErrorCode=0x%X [%s:%u]",                \
                             error_code,                                        \
                             the_file,                                          \
                             __LINE__));                                        \
    } while(0)
#else /* CSR_TARGET_PRODUCT_VM */

#define BLUESTACK_WARNING_COMMON(who_msg, error_code)                           \
    CSR_LOG_TEXT_WARNING((CsrBtCoreStackLto,                                    \
                         0,                                                     \
                         who_msg "BLUESTACK_WARNING: ErrorCode=0x%X [%s:%u]",   \
                         error_code,                                            \
                         CsrGetBaseName(__FILE__),                              \
                         __LINE__))

#endif /* CSR_TARGET_PRODUCT_VM */

#ifndef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
#define BLUESTACK_WARNING(error_code)       BLUESTACK_WARNING_COMMON("BLUESTACK_WARNING", error_code)
#define DEBOUT(error_code)                  BLUESTACK_WARNING_COMMON("DEBOUT", error_code)
#define DEBDRP(error_code)                  BLUESTACK_WARNING_COMMON("DEBDRP", error_code)
#else
#define BLUESTACK_WARNING(error_code)       SynLogBluestackWarning(error_code, __LINE__)
#define DEBOUT(error_code)                  SynLogBluestackDebout(error_code, __LINE__)
#define DEBDRP(error_code)
#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_CORESTACK_LOG_H_ */

