/******************************************************************************
 Copyright (c) 2004-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"
#ifndef CSR_TARGET_PRODUCT_WEARABLE
#include "optim.h"

/****************************************************************************
NAME
	saturate_add  -  Add two 32-bit numbers, saturating on overflow
*/

void saturate_add(uint32 val, uint32 *sum)
{
    if ((0 - val) < *sum)
	*sum = 0xffffffff;
    else
	*sum += val;
}
#endif
