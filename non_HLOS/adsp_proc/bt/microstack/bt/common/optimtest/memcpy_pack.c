/******************************************************************************
 Copyright (c) 2003-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"
#ifndef CSR_TARGET_PRODUCT_WEARABLE
#include	"optim.h"

/****************************************************************************
NAME
	memcpy_pack  -  copy memory turning 2 * uint8 into uint16
*/

void memcpy_pack(uint16 *dest, const uint8 *source, uint16 length)
{
    while(length--)
    {
      uint16 lo = *source++ & 0xFF;
      uint16 hi = *source++;
      *dest++ = (hi<<8) | lo;
    }
}
#endif
