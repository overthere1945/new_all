/******************************************************************************
 Copyright (c) 1999-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

/* BCHS_EXPORT_POINT_START */
  
#include	"optim.h"

/****************************************************************************
NAME
	udiv3216  -  Do a 32 by 16 bit division return quotient and remainder
*/

uint32 udiv3216(uint16 *r, uint16 d, uint32 n)
{
    uint32 q;

    q = n/d;
    if (r != NULL)
	*r = (uint16) (n - q*d);

    return q;
}

/* BCHS_EXPORT_POINT_END */
