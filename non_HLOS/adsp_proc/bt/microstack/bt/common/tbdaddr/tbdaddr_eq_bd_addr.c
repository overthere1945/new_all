/*!
        Copyright (C) 2010 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Compare a fat Bluetooth address with a thin one.
*/

#include "tbdaddr_private.h"

bool_t tbdaddr_eq_bd_addr(const TYPED_BD_ADDR_T *addrt,
                               uint8_t bd_addr_type,
                               const BD_ADDR_T *bd_addr)
{
    TYPED_BD_ADDR_T temp;

    tbdaddr_copy_from_bd_addr(&temp, bd_addr_type, bd_addr);
    return tbdaddr_eq(addrt, &temp);
}
