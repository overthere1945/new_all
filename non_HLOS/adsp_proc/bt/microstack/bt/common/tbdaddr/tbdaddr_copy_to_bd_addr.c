
/*!
        Copyright (C) 2010 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Extract an old-style BD_ADDR_T from a fat Bluetooth address.
        Note that BD_ADDR_T does not contain address type information.
*/

#include "tbdaddr_private.h"

uint8_t tbdaddr_copy_to_bd_addr(BD_ADDR_T *dst, const TYPED_BD_ADDR_T *src)
{
    *dst = TBDADDR_ADDR(*src);
    return TBDADDR_TYPE(*src);
}
