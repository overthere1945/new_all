/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_bt_hci_convert_hcicmd_private.h"
#include <string.h>

/****************************************************************************
 NAME
 hcicmd_array_read  -  Read in an array pdu.
 */

void hcicmd_array_serialise(uint8_t **pdu,
                            const pdu_array_info* array_info,
                            void* src_struct)
{
    /* Write the header */
    wrpdu_struct(pdu,
                 array_info->fmt_of_header,
                 (uint8_t *) src_struct + array_info->offset_of_header);

    /* Write the blocks */
    hcicmd_array_serialise_no_header(pdu,
                                     array_info,
                                     src_struct,
                                     *(uint8_t *) ((uint8_t *) src_struct
                                                    + array_info->offset_of_count));
}

/****************************************************************************
 NAME
 hcievt_array_write_no_header  -  Write an array pdu (of N entries).
 */

void hcicmd_array_serialise_no_header(uint8_t **pdu,
                                      const pdu_array_info* array_info,
                                      void* base,
                                      uint8_t num)
{
    uint16_t i;
    uint8_t *entry_ptr;
    uint8_t **array_ptr;

    /* Check that we have something to do */
    if (num == 0)
        return;

    /*LINTED*/
    array_ptr = (uint8_t **) ((uint8_t *) base + array_info->offset_of_array);

    /* Get the first block */
    entry_ptr = *array_ptr;
    i = (uint16_t)array_info->entries_per_block;

    /* Write each block */
    while (1)
    {
        /* If we have got a NULL pointer in a block, then we will skip it */
        if (entry_ptr != NULL)
        {
            wrpdu_struct(pdu, array_info->fmt_of_entry, entry_ptr);

            entry_ptr += array_info->size_of_entry;
        }
        else
        {
            /* H16.70: The block was NULL, fill with zeros instead.
             This is to support the cetecom tester, which expects
             that the end of the rnr/rln block is zero initialized.
             We also increment the PDU pointer. (TP/INF/BV-13-C). */
            uint32_t len = num_octets_for_struct(array_info->fmt_of_entry);
            memset(*pdu, 0, len);
            *pdu += len;
        }

        /* Count how many we have output */
        if (--num == 0)
        {
            /* We still need to free this block */
            break;
        }

        /* Check to see if we are on the next pointer */
        if (--i == 0)
        {

            /* Move to the next block */
            array_ptr++;
            /* Get the next block */
            entry_ptr = *array_ptr;
            i = (uint16_t)array_info->entries_per_block;
        }
    }

    return;
}

