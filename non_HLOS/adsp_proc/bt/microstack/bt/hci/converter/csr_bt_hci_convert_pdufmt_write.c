/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "qbl_panic.h"
#include "qbl_panic_codes.h"
#include "csr_bt_hci_convert_pdufmt_private.h"

/****************************************************************************
 NAME
 wrpdu_type  -  write some typed data into a buffer
 */

size_t wrpdu_type(uint8_t **pdu_data,
                   const pdufmt_type fmt,
                   const void * const struct_data)
{
    size_t size;
    uint8_t *ptr = *pdu_data;

    switch (fmt)
    {
        case pdufmt_el_int8_t:
            case pdufmt_el_uint8_t:
            ptr[0] = *((const uint8_t*) struct_data);

            *pdu_data = ptr + 1;
            size = sizeof(uint8_t);
            break;

        case pdufmt_el_uint16_t:
        {
            uint16_t temp = *((const uint16_t*) struct_data);

            ptr[0] = (uint8_t) temp;
            temp >>= 8;
            ptr[1] = (uint8_t) temp;

            *pdu_data = ptr + 2;
            size = sizeof(uint16_t);
            break;
        }

        case pdufmt_el_int24_t:
        {
            int24_t temp = *((const int24_t*) struct_data);

            ptr[0] = (uint8_t) (temp);
            temp >>= 8;
            ptr[1] = (uint8_t) (temp);
            temp >>= 8;
            ptr[2] = (uint8_t) (temp);

            *pdu_data = ptr + 3;
            size = sizeof(int24_t);
            break;
        }

        case pdufmt_el_uint24_t:
        {
            uint24_t temp = *((const uint24_t*) struct_data);

            ptr[0] = (uint8_t) (temp);
            temp >>= 8;
            ptr[1] = (uint8_t) (temp);
            temp >>= 8;
            ptr[2] = (uint8_t) (temp);

            *pdu_data = ptr + 3;
            size = sizeof(uint24_t);
            break;
        }

        case pdufmt_el_uint32_t:
        {
            uint32_t temp = *((const uint32_t*) struct_data);

            ptr[0] = (uint8_t) (temp);
            temp >>= 8;
            ptr[1] = (uint8_t) (temp);
            temp >>= 8;
            ptr[2] = (uint8_t) (temp);
            temp >>= 8;
            ptr[3] = (uint8_t) (temp);

            *pdu_data = ptr + 4;
            size = sizeof(uint32_t);
            break;
        }

        default:
            size = 0;
            qbl_panic(QBL_PANIC_HCI_CONVERT_INVALID_DATA_TYPE);
    }

    return (size);
}

/****************************************************************************
 NAME
 wrpdu_struct  -  copy data from a struct to the packet
 */

void wrpdu_struct(uint8_t **pdu_data,
                  const pdufmt *fmt,
                  const void * const void_struct_data)
{
    const uint8_t * const struct_data = (const unsigned char *) void_struct_data;
    pdufmt_type type;

    /* Exit if we've nothing to do. */
    if (fmt == NULL)
        return;

    /* Walk the list of fmt descriptors, writing from the structure
     and into the pdu.  */
    for (; (type = fmt->type) != pdufmt_el_term; fmt++)
    {
        uint16_t
        repeat = fmt->repeat + 1;
        const uint8_t *ptr = struct_data + fmt->offset;

        while (repeat--)
            ptr += wrpdu_type(pdu_data, type, ptr);
    }
}

