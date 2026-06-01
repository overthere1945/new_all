/******************************************************************************
 Copyright (c) 2016-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_bt_hci_convert_hcievt_private.h"
#include "csr_bt_hci_convert_hcicmd_private.h"


Bool hcievt_cmd_complete_deserialise(
    const uint8_t **pdu,
    HCI_EV_COMMAND_COMPLETE_T *src)
{
    const pdufmt* fmt = pdufmt_cmd_complete;

    src->argument_ptr = qbl_zpmalloc(sizeof(*src->argument_ptr));

    SKIP_PDUFMT_BY_COUNT(fmt, sizeof(HCI_EVENT_COMMON_T));

    /* We write the common block of the command complete event code */
    rdpdu_struct(pdu,
                 fmt,
                 (void *) src);

    /* Get the format for the command_complete extension block */
    fmt = hcicmd_id_ret_pdufmt(src->op_code);

    /* Write the extension block (if any) */
    if (fmt == NULL)
    {
        /*Free it up as it was not filled and no one to utilise it*/
        qbl_pfree(src->argument_ptr);
        src->argument_ptr = NULL;
        return FALSE;
    }
    else if (fmt != pdufmt_empty)
    {
        /* Write the parameters to the command complete */
        rdpdu_struct(pdu, fmt, (void *) src->argument_ptr);
    }

    /* Everything seems to have worked */
    return TRUE;
}

