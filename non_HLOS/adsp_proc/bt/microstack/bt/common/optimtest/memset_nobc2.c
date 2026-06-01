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
	memset_nobc2  -  set memory in pairs of words without using BC
*/

void memset_nobc2(uint8 *dest, uint8 first_word, uint8 second_word,
		  uint16 repeats)
{
    while(repeats--)
    {
	*dest++ = first_word;
	*dest++ = second_word;
    }
}
#endif
