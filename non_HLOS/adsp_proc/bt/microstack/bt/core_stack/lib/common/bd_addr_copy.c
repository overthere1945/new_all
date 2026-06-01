/*******************************************************************************

Copyright (C) 2008 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/

#include <stdarg.h>
#include <string.h>
#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include INC_DIR(common,common.h)

/*! \brief Copy the source BD Address into the destination BD address.
    
    \param p_bd_addr_dest Pointer to destination Bluetooth address.
    \param p_bd_addr_src Pointer to source Bluetooth address.
*/
void bd_addr_copy(BD_ADDR_T *p_bd_addr_dest, const BD_ADDR_T *p_bd_addr_src)
{
    qbl_memscpy(p_bd_addr_dest, sizeof(BD_ADDR_T), p_bd_addr_src, sizeof(BD_ADDR_T));
}

