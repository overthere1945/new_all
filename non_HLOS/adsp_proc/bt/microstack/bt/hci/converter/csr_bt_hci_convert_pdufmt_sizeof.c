/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_bt_hci_convert_pdufmt_private.h"

/****************************************************************************
 NAME
 sizeof_struct  -  how much memory does a structure need
 */

size_t sizeof_struct(const pdufmt *fmt)
{
    if (fmt == pdufmt_empty)
        return 0;

    while (fmt->type != pdufmt_el_term)
        fmt++;

    return fmt->offset;
}

/****************************************************************************
 NAME
 num_octets_for_struct  -  how many octets does a structure need
 */

size_t num_octets_for_struct(const pdufmt *fmt)
{
    static const uint32_t octets[] =
    {
#define PDUFMT_X(a, b) b
      PDUFMT_TABLE_NO_TERM
#undef PDUFMT_X
    };

    uint16_t num;
    pdufmt_type type;

    if (fmt == pdufmt_empty || fmt == NULL)
        return 0;

    num = 0;
    for (; (type = fmt->type) < pdufmt_el_term; fmt++)
        num += octets[type] * (fmt->repeat + 1);

    return num;
}

