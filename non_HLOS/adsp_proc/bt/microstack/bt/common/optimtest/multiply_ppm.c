/******************************************************************************
 Copyright (c) 2018-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"
#ifndef CSR_TARGET_PRODUCT_WEARABLE
#include "optim.h"


/****************************************************************************
NAME
        multiply_skew  -  48 bit multiply then divide by 2^20
*/
int32 multiply_skew(uint32 t, int16 skew)
{
    bool positive;
    uint16 uskew, t_lsw, t_msw; 
    int32 result;

    /* This is more or less a straight copy of the assembler equivalent */
    if (skew < 0)
    {
        uskew = -skew;
        positive = FALSE;
    }
    else
    {
        uskew = skew;
        positive = TRUE;
    }

    t_lsw = (uint16) (t & 0xffff);
    t_msw = (uint16) (t >> 16);
    result = ((((uint32) t_lsw * uskew) >> 16)
               + ((uint32) t_msw * uskew)) >> 4;


    if (positive)
        return result;
    else
        return -result;
}
#endif
