
/*!
        Copyright (C) 2015 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Put an old-style BD_ADDR_T into a transport specific Bluetooth address.
*/

#include "tpbdaddr_private.h"

void tpbdaddr_copy_from_bd_addr(TP_BD_ADDR_T *dst,
                               PHYSICAL_TRANSPORT_T src_tp_type,
                               uint8_t src_type,
                               const BD_ADDR_T *src)
{
    dst->tp_type = src_tp_type;
    dst->addrt.type = src_type;
    dst->addrt.addr = *src;
}

