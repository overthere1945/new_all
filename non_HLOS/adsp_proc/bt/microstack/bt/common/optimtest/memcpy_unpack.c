/******************************************************************************
 Copyright (c) 2000-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"
#ifndef CSR_TARGET_PRODUCT_WEARABLE
#include	"optim.h"

/****************************************************************************
NAME
	memcpy_unpack  -  copy memory turning uint16 into 2 * uint8
*/

void memcpy_unpack(uint8 *dest, const uint16 *source, uint16 length)
{
    uint16 word;

    while(length--)
    {
	word = *source++;
	*dest++ = word & 0xff;
	*dest++ = (word >> 8) & 0xff;
    }
}
#endif
