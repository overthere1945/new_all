/*******************************************************************************

Copyright (C) 2008 - 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/

#include <stdarg.h>
#include <string.h>
#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include INC_DIR(common,common.h)

/*! \brief Set the BD Address to all zeroes.
    
    \param p_bd_addr Pointer to Bluetooth address to be zeroed.
*/
void bd_addr_zero(BD_ADDR_T *p_bd_addr)
{
    memset(p_bd_addr, 0 , sizeof(BD_ADDR_T));
}

