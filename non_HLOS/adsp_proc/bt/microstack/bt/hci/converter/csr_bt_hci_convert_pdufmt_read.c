/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "qbl_panic.h"
#include "qbl_panic_codes.h"
#include "csr_bt_hci_convert_pdufmt_private.h"
#include "csr_bt_hci_convert_hcicmd_private.h"
#include "csr_bt_hci_convert_hcievt_private.h"

/****************************************************************************
 NAME
 rdpdu_type  -  copy data from a packet to a typed variable
 */

size_t rdpdu_type(const uint8_t **pdu_data,
                   const pdufmt_type fmt,
                   void * const struct_data)
{
    size_t size;
    const uint8_t* ptr = *pdu_data;

    switch (fmt)
    {
        case pdufmt_el_int8_t:
        {
            int8_t temp = ((const int8_t *) ptr)[0];
            *((int8_t*) struct_data) = temp;

            *pdu_data = ptr + 1;
            size = sizeof(int8_t);
            break;
        }

        case pdufmt_el_uint8_t:
            *((uint8_t*) struct_data) = ptr[0];

            *pdu_data = ptr + 1;
            size = sizeof(uint8_t);
            break;

        case pdufmt_el_uint16_t:
            *((uint16_t*) struct_data) = ((uint16_t) ptr[0])
                                          | ((uint16_t) ptr[1] << 8);

            *pdu_data = ptr + 2;
            size = sizeof(uint16_t);
            break;

        case pdufmt_el_int24_t:
            *((int24_t*) struct_data) = (uint16_t) (((uint16_t) ptr[0])
                                                       | ((uint16_t) ptr[1] << 8))
                                                       | ((int24_t) ptr[2] << 16);

            *pdu_data = ptr + 3;
            size = sizeof(int24_t);
            break;

        case pdufmt_el_uint24_t:
            *((uint24_t*) struct_data) = (uint16_t) (((uint16_t) ptr[0])
                                                       | ((uint16_t) ptr[1] << 8))
                                                       | ((uint24_t) ptr[2] << 16);

            *pdu_data = ptr + 3;
            size = sizeof(uint24_t);
            break;

        case pdufmt_el_uint32_t:
            *((uint32_t*) struct_data) = (uint16_t) (((uint16_t) ptr[0]
                                                        | (((uint16_t) ptr[1]) << 8)))
                                                        | ((uint32_t) (uint16_t) ((uint16_t) ptr[2]
                                                                      | (((uint16_t) ptr[3])
                                                                         << 8)) << 16);

            *pdu_data = ptr + 4;
            size = sizeof(uint32_t);
            break;

        default:
            size = 0;
            qbl_panic(QBL_PANIC_HCI_CONVERT_INVALID_DATA_TYPE);
    }

    return (size);
}

/****************************************************************************
 NAME
 rdpdu_struct  -  copy data from a packet to a struct
 */

void rdpdu_struct(const uint8_t * * pdu_data,
                  const pdufmt *fmt,
                  void * const void_struct_data)
{
    uint8_t * const struct_data = (uint8_t *) void_struct_data;
    pdufmt_type type;

    /* Succeed if we've nothing to do. */
    if (fmt == NULL)
        return;

    /* Walk the list of format descriptors, reading from the input pdu
     at each stage and writing to the struct.  */

    for (; (type = fmt->type) != pdufmt_el_term; fmt++)
    {
        uint16_t repeat = fmt->repeat + 1;
        uint8_t *ptr = struct_data + fmt->offset;

        while (repeat--)
            ptr += rdpdu_type(pdu_data, type, ptr);
    }
}

