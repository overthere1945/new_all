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
	get_le_uint32  -  get a uint32 from a uint8 array
*/

uint32 get_le_uint32(const uint8 *ptr)
{
    return (((uint32)ptr[3] << 24) & 0xFF000000)
	 | (((uint32)ptr[2] << 16) & 0x00FF0000)
	 | (((uint32)ptr[1] << 8 ) & 0x0000FF00)
	 | (((uint32)ptr[0]      ) & 0x000000FF);
}

void put_le_uint32(uint32 val, uint8 *ptr)
{
    ptr[0] = (uint8)(val & 0xff);
    ptr[1] = (uint8)((val >> 8) & 0xff);
    ptr[2] = (uint8)((val >> 16) & 0xff);
    ptr[3] = (uint8)((val >> 24) & 0xff);
}
#endif
