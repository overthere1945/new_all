/*******************************************************************************

Copyright (C) 2008 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/

#include <stdarg.h>
#include <string.h>
#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include INC_DIR(common,common.h)

/* Mask the extra byte in LAP */
#define BD_ADDR_MASK_LAP ((uint32_t)0x00FFFFFF)

/*! \brief Compare two BD Addresses for equality.

    \param p_bd_addr_1 Pointer to first Bluetooth address.
    \param p_bd_addr_2 Pointer to second Bluetooth address.
    \returns TRUE if equal, FALSE if different.
*/
bool_t bd_addr_eq(
    const BD_ADDR_T *p_bd_addr_1,
    const BD_ADDR_T *p_bd_addr_2
    )
{
    if (((p_bd_addr_1->lap & BD_ADDR_MASK_LAP) == (p_bd_addr_2->lap & BD_ADDR_MASK_LAP)) &&
        (p_bd_addr_1->uap == p_bd_addr_2->uap) &&
        (p_bd_addr_1->nap == p_bd_addr_2->nap))
    {
        return TRUE;
    }

    return FALSE;
}


