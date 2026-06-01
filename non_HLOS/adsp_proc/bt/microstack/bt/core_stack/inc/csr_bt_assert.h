#ifndef CSR_BT_CSR_BT_ASSERT_H__
#define CSR_BT_CSR_BT_ASSERT_H__
/******************************************************************************
 Copyright (c) 2008-2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_panic.h"
#include "csr_pmem.h"
#include "csr_bt_tasks.h"
#include "csr_bt_profiles.h"
#include "csr_bt_util.h"
#include "csr_log_text_2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CSR_BT_LOG_TEXT_ERROR_RET_VAL(handle, subOrigin, cond, retval)        \
    do {                                                                      \
        if (!(cond))                                                          \
        {                                                                     \
            CSR_LOG_TEXT_ERROR((handle,                                       \
                               subOrigin,                                     \
                               "Assertion \"%s\" failed at %s:%u",            \
                               #cond,                                         \
                               __FILE__,                                      \
                               __LINE__));                                    \
            return (retval);                                                  \
        }                                                                     \
    } while(0)

#define CSR_BT_LOG_TEXT_ERROR_RET(handle, subOrigin, cond)                    \
    do {                                                                      \
        if (!(cond))                                                          \
        {                                                                     \
            CSR_LOG_TEXT_ERROR((handle,                                       \
                               subOrigin,                                     \
                               "Assertion \"%s\" failed at %s:%u",            \
                               #cond,                                         \
                               __FILE__,                                      \
                               __LINE__));                                    \
            return;                                                           \
        }                                                                     \
    } while(0)

#define CSR_BT_RET_VAL_IF_FAIL(cond, retval)                        \
    do {                                                            \
        if (!(cond))                                                \
        {                                                           \
            return (retval);                                        \
        }                                                           \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* ndef _CM_MAIN_H */
