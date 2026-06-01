/*!
        Copyright (C) 2010 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Put an old-style public BD_ADDR_T into a fat Bluetooth address.
*/

#include "tbdaddr_private.h"

void tbdaddr_copy_from_public_bd_addr(TYPED_BD_ADDR_T *dst,
                                      const BD_ADDR_T *src)
{
    TBDADDR_TYPE(*dst) = TBDADDR_PUBLIC;
    TBDADDR_ADDR(*dst) = *src;
}
