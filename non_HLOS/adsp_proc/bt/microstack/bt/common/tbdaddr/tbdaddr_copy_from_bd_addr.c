
/*!
        Copyright (C) 2010 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Put an old-style BD_ADDR_T into a fat Bluetooth address.
*/

#include "tbdaddr_private.h"

void tbdaddr_copy_from_bd_addr(TYPED_BD_ADDR_T *dst,
                               uint8_t src_type,
                               const BD_ADDR_T *src)
{
    dst->type = src_type;
    dst->addr = *src;
}
