/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_bt_hci_convert_hcievt_private.h"

#define LUT_PDUFMT(fmt) (&all_pdufmts.fmt[0])
#define PDUFMT_NULL NULL
#include "csr_bt_hci_convert_hcievt_lut.ch"

/****************************************************************************
 NAME
 hcievt_id_pdufmt  -  match hci command id to pdufmt
 */

const pdufmt* hcievt_id_pdufmt(hci_event_code_t event)
{
    if (event < sizeof(hcievt_lut) / sizeof(hcievt_lut[0]))
    {
        return hcievt_lut[event];
    }

    return NULL;
}

#ifdef INSTALL_ULP
/****************************************************************************
 NAME
 hcievt_id_pdufmt  -  match ulp hci command id to pdufmt
 */

const pdufmt* hcievt_ulp_id_pdufmt(hci_event_code_t event)
{
    if (event < sizeof(hcievt_ulp_lut) / sizeof(hcievt_ulp_lut[0]))
    {
        return hcievt_ulp_lut[event];
    }

    return NULL;
}
#endif

