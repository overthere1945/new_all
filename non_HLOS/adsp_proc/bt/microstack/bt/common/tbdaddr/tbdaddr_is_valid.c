/*!
        Copyright (C) 2010 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Validate a fat Bluetooth address.
*/

#include "tbdaddr_private.h"

static bool_t random_part_is_valid(const TYPED_BD_ADDR_T *addrt)
{
    uint8_t a;

    /* Check NAP and UAP */
    a = TBDADDR_UAP(*addrt);
    if (a != 0 && a != 0xFF)
        return TRUE;

    if ((TBDADDR_NAP(*addrt) & 0xFF) != a)
        return TRUE;

    if ((((TBDADDR_NAP(*addrt) >> 8) & 0x3F) | (a & 0xC0)) != a)
        return TRUE;

    if (!TBDADDR_IS_PRIVATE_RESOLVABLE(*addrt))
    {
        /* Check LAP */
        if ((TBDADDR_LAP(*addrt) & 0xFF) != a)
            return TRUE;

        if (((TBDADDR_LAP(*addrt) >> 8) & 0xFF) != a)
            return TRUE;

        if (((TBDADDR_LAP(*addrt) >> 16) & 0xFF) != a)
            return TRUE;
    }

    return FALSE;
}

bool_t tbdaddr_is_valid(const TYPED_BD_ADDR_T *addrt)
{
    if (TBDADDR_IS_PUBLIC(*addrt))
        return TRUE;

    if (TBDADDR_IS_RANDOM(*addrt) && random_part_is_valid(addrt))
    {
        if (TBDADDR_IS_STATIC(*addrt) || TBDADDR_IS_PRIVATE(*addrt))
        {
            return TRUE;
        }
    }

    return FALSE;
}
