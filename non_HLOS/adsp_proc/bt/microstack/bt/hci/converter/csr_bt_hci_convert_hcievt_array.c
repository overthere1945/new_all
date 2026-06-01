/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_bt_hci_convert_hcicmd_private.h"
#include "csr_bt_hci_convert_hcievt_private.h"

/****************************************************************************
 NAME
 hcievt_array_write  -  Write a PDU with a number of repeated blocks.
 */

Bool hcievt_array_deserialise(const uint8_t **pdu,
                                 hcievt_info* info,
                                 const pdu_array_info* array_info)
{
    HCI_UPRIM_T *base;
    uint8_t count;

    /* Allocate the first section */
    /* We rely on the representation of NULL being zero */
    base = (HCI_UPRIM_T *) qbl_zpmalloc(array_info->size_of_base);

    /* Save the pointer */
    info->out_struct = base;

    /* Get the number */
    rdpdu_struct(pdu, array_info->fmt_of_header,
                 (uint8_t *) base + array_info->offset_of_header);

    count = *(uint8_t *) ((uint8_t *) base + array_info->offset_of_count);

    if (info->start.length
        != num_octets_for_struct(array_info->fmt_of_header)
           + count * num_octets_for_struct(array_info->fmt_of_entry))
    {
        qbl_pfree(base);
        /*Initialising to NULL to avoid multiple mem free*/
        base = NULL;
        return FALSE;
    }

    return hcievt_array_deserialise_got_count(pdu, info->out_struct, array_info, count);
}

Bool hcievt_array_deserialise_got_count(const uint8_t **pdu,
                                           HCI_UPRIM_T* info,
                                           const pdu_array_info* array_info,
                                           uint8_t count)
{
    uint8_t *base;
    uint16_t j;
    uint8_t **array_ptr;

    if (count == 0)
        return TRUE;

    /* Get the pointer */
    base = (uint8_t *) info;

    /*LINTED*/
    array_ptr = (uint8_t **) (base + array_info->offset_of_array);

    for (;;)
    {
        uint8_t *entry_ptr;

        entry_ptr = (uint8_t *) qbl_zpmalloc(array_info->size_of_block);
        *array_ptr++ = entry_ptr;

        j = (uint16_t)array_info->entries_per_block;
        do
        {
            rdpdu_struct(pdu, array_info->fmt_of_entry, entry_ptr);
            if (--count == 0)
                return TRUE;
            entry_ptr += array_info->size_of_entry;
        }
        while (--j != 0);
    }
}

Bool hcievt_rdpdu_struct(const uint8_t * * pdu_data,
                            hcievt_info* info,
                            const pdufmt *fmt)
{
    /*Initialising to NULL to avoid multiple mem free*/
    HCI_UPRIM_T* out = NULL;
    uint8_t countVal, primSize;

    countVal = (uint8_t)num_octets_for_struct(fmt);

#ifdef INSTALL_ULP
    if (info->start.event_code == HCI_EV_ULP)
    {
        /*Include the length of the subopcode*/
        countVal = countVal + sizeof(info->start.ulp_sub_opcode);
    }
#endif

    /* Check that the length of the PDU matches what we have been sent */
    if (countVal != info->start.length)
        return FALSE;

    /* There are params, so alloc a data struct.  For the types of 
     structures that we are dealing with here, there is only this
     one memory block, so the cleanup is simple.
     */
    primSize = (uint8_t)sizeof_struct(fmt);
    out = (HCI_UPRIM_T*) qbl_zpmalloc(primSize);

    /* Save the pointer */
    info->out_struct = out;

    /* Copy the pdu to the struct. */
    rdpdu_struct(pdu_data, fmt, (void*) (out));

    /* Return the read structure back to the caller */
    return TRUE;
}

Bool hcievt_cmd_comp_deserialise(const uint8_t **pdu,
                                    HCI_UPRIM_T* info,
                                    const pdu_array_info* array_info,
                                    uint8_t num)
{
    uint8_t count;

    /* Get the number */
    rdpdu_struct(pdu, array_info->fmt_of_header,
                 (uint8_t *) info + array_info->offset_of_header);

    count = *(uint8_t *) ((uint8_t *) info + array_info->offset_of_count);

    if (num != num_octets_for_struct(array_info->fmt_of_header)
               + count * num_octets_for_struct(array_info->fmt_of_entry))
    {
        return FALSE;
    }

    return hcievt_array_deserialise_got_count(pdu, info, array_info, count);
}


