/*!
        Copyright (C) 2010 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Check type of fat Bluetooth address.
*/

#include "tbdaddr_private.h"

bool_t tbdaddr_is_private_nonresolvable(const TYPED_BD_ADDR_T *addrt)
{
    return TBDADDR_IS_RANDOM(*addrt)
            && (TBDADDR_NAP(*addrt) & TBDADDR_RANDOM_NAP_TYPE_MASK)
                    == TBDADDR_RANDOM_NAP_TYPE_PRIVATE_NONRESOLVABLE;
}
