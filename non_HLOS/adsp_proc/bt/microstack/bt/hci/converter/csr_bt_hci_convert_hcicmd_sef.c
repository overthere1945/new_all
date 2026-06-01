/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_bt_hci_convert_hcicmd_private.h"

/****************************************************************************
 NAME
 hcicmd_set_event_filter_struct  -  Read a SEF command
 */

Bool hcicmd_set_event_filter(uint8_t **pdu, HCI_UPRIM_T * info)
{
    static const pdufmt* const filter_fmt[6] =
    {
      NULL,
      pdufmt_Su24u24s,
      pdufmt_bdaddr,
      pdufmt_u8,
      pdufmt_Su24u24su8,
      pdufmt_Su24u8u16su8
    };
    static const uint16_t filter_len[6] =
    {
      2,
      2 + 3 + 3,
      2 + 6,
      2 + 1,
      2 + 3 + 3 + 1,
      2 + 6 + 1
    };

    uint16_t len;
    uint16_t filter_idx;
    Bool result;

    /* This is a do{}while(0) loop so that I dont have to use goto */
    do
    {
        wrpdu_type(pdu,
                   pdufmt_el_uint8_t,
                   &info->hci_set_event_filter.filter_type);

        /* Get the length of the packet, we decrement this and check that
         it is zero at the end */
        len = info->hci_cmd.length;

        if (info->hci_set_event_filter.filter_type == CLEAR_ALL_FILTERS)
        {
            if (len != 1)
            {
                result = FALSE;
                break;
            }

            return TRUE;
        }
        else if (info->hci_set_event_filter.filter_type > CONNECTION_FILTER)
        {
            /* Invalid value! */
            result = FALSE;
            break;
        }

        wrpdu_type(pdu,
                   pdufmt_el_uint8_t,
                   &info->hci_set_event_filter.filter_condition_type);

        if (info->hci_set_event_filter.filter_condition_type
            > ADDRESSED_DEVICE_RESPONDED)
        {
            result = FALSE;
            break;
        }

        /* Get the index of the filter (into the local array above) */
        filter_idx = info->hci_set_event_filter.filter_type * 3
                     + info->hci_set_event_filter.filter_condition_type
                     - 3;

        /* Check that the length is correct */
        len -= filter_len[filter_idx];
        if (len != 0)
        {
            result = FALSE;
            break;
        }

        /* Read in the last part of the filter.  If the format pointer is NULL
         then this function will return with success! */
        wrpdu_struct(pdu,
                     filter_fmt[filter_idx],
                     &info->hci_set_event_filter.condition);

        return TRUE;

        /*NOTREACHED*/
    }
    /*CONSTANTCONDITION*/
    while (0);

    return result;
}

