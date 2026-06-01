/*!
        Copyright (C) 2014 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file   tpbdaddr_eq.c

\brief  Helper function to compare Transport Bluetooth device address.
*/

#include "tbdaddr_private.h"
#include INC_DIR(common,common.h)

bool_t tpbdaddr_eq(const TP_BD_ADDR_T *a, const TP_BD_ADDR_T *b)
{
    return TPBDADDR_TRANSPORT(*a) == TPBDADDR_TRANSPORT(*b)
                && tbdaddr_eq(&TPBDADDR_ADDRT(*a), &TPBDADDR_ADDRT(*b));
}
