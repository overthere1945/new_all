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
	memset_nobc  -  set memory without using BC
*/

void memset_nobc(uint8 *dest, uint8 word, uint16 repeats)
{
    while(repeats--)
	*dest++ = word;
}
#endif
