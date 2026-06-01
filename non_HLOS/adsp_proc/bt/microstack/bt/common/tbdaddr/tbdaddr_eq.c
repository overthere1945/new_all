/*!
        Copyright (C) 2010 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Copy a fat Bluetooth device address.
*/

#include "tbdaddr_private.h"
#include INC_DIR(common,common.h)

bool_t tbdaddr_eq(const TYPED_BD_ADDR_T *a, const TYPED_BD_ADDR_T *b)
{
    return TBDADDR_TYPE(*a) == TBDADDR_TYPE(*b)
                && bd_addr_eq(&TBDADDR_ADDR(*a), &TBDADDR_ADDR(*b));
}
