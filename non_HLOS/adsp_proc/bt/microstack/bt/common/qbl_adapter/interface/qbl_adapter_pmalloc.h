#ifndef QBL_ADAPTER_PMALLOC_H__
#define QBL_ADAPTER_PMALLOC_H__
/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_bt_core_stack_pmalloc.h"

#ifndef CSR_TARGET_PRODUCT_VM
#define primfree(_id, _msg)     (pfree(_msg))
#else
#define primfree(_id, _msg)     (bluestack_msg_free(_id, _msg))
#endif /* !CSR_TARGET_PRODUCT_VM */
#endif


