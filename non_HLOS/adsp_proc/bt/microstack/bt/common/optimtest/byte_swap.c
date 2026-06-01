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
	byte_swap  -  copy memory turning uint16 into 2 * uint8
*/

void byte_swap(uint16 *data, uint16 words)
{
    while(words--)
    {
	uint16 word = *data;
	*data = (word >> 8) | (word << 8);
	++data;
    }
}
#endif
