/******************************************************************************
 Copyright (c) 2008-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_pmem.h"

void pdestroy_array(void **ptr, CsrUint16 num)
{
    while (num--)
    {
        CsrPmemFree(*ptr);
        *ptr = NULL;
        ptr++;
    }
}

