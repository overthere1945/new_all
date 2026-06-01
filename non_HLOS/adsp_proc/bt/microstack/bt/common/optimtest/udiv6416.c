/******************************************************************************
 Copyright (c) 2005-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

/* BCHS_EXPORT_POINT_START */

#include	"optim.h"

/****************************************************************************
NAME
	udiv6416  -  Do a 64 by 16 bit division return quotient and remainder
*/
uint16 udiv6416(optim_uint64 *q, uint16 d, const optim_uint64 *n)
{
    optim_uint64 tmp;
    uint16 r;
    uint32 qt;

    if (q == NULL)
	q = &tmp;

    q->msw = udiv3216(&r, d, n->msw);
    qt = udiv3216(&r, d, ((uint32) r << 16) | (n->lsw >> 16));
    q->lsw = (qt << 16) |
	udiv3216(&r, d, ((uint32) r << 16) | (n->lsw & 0xffff));

    return r;
}

/* BCHS_EXPORT_POINT_END */
