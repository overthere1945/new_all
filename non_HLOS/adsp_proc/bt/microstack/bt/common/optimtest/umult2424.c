/******************************************************************************
 Copyright (c) 2018-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "optim.h"

/****************************************************************************
NAME
        umult2424  -  do a 24 by 24 bit multiplication

RETURNS
        The result is written into *r.

        r must NOT be NULL.
*/
void umult2424(uint32 a1, uint32 a2, util_uint48 *r)
{
    uint32 tmp;

    tmp = (a1 & 0xffff) * (a2 & 0xffff);
    r->val[2] = tmp & 0xffff;

    tmp >>= 16;
    tmp += (a1 & 0xffff) * (a2 >> 16);
    tmp += (a1 >> 16) * (a2 & 0xffff);
    tmp += ((a1 >> 16) * (a2 >> 16)) << 16;

    r->val[1] = tmp & 0xffff;
    r->val[0] = tmp >> 16;
}
