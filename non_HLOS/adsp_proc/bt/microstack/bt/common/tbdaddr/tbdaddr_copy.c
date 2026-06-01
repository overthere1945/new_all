/*!

        Copyright (C) 2010 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Copy a fat Bluetooth device address.
*/

#include "tbdaddr_private.h"

void tbdaddr_copy(TYPED_BD_ADDR_T *dst, const TYPED_BD_ADDR_T *src)
{
    *dst = *src;
}
