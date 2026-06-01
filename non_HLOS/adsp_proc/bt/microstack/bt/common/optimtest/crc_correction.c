/******************************************************************************
 Copyright (c) 2003-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"
#ifndef CSR_TARGET_PRODUCT_WEARABLE
#include "optim.h"

#define POLY 0x8005

/****************************************************************************
NAME
	crc_correction  -  Helper for L2CAP CRC correction
*/

uint16 crc_correction(uint16 a, uint16 b)
{
    uint16 result = 0;

    while (a != 0)
    {
	if ((a & 1) != 0)
	    result ^= b;
	a >>= 1;
	b = (b << 1) ^ ((b & 0x8000) ? POLY : 0);
    }

    return result;
}
#endif

