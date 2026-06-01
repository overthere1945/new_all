/******************************************************************************
 Copyright (c) 2016-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/

#include "qbl_hci.h"
#include "csr_bt_hci_convert_pdufmt_private.h"


const pdu_array_info num_compl_pkts_array_info = {
    sizeof(HCI_EV_NUMBER_COMPLETED_PKTS_T),

    offsetof(HCI_EV_NUMBER_COMPLETED_PKTS_T, num_handles),
    pdufmt_u8, /* fmt_of_header */

    offsetof(HCI_EV_NUMBER_COMPLETED_PKTS_T, num_handles),
    offsetof(HCI_EV_NUMBER_COMPLETED_PKTS_T, num_completed_pkts_ptr),
    pdufmt_u16u16, /* fmt_of_entry */

    HCI_HOST_NUM_COMPLETED_PACKETS_PER_PTR, /* entries_per_block */
    sizeof(HANDLE_COMPLETE_T), /* size_of_entry */
    HCI_HOST_NUM_COMPLETED_PACKETS_PER_PTR
        * sizeof(HANDLE_COMPLETE_T) /* size_of_block */
};

#ifdef INSTALL_ADVERTISING_EXTENSIONS
const pdu_array_info set_ea_data_array_info = {
    sizeof(HCI_ULP_EXT_ADV_SET_DATA_T),

    offsetof(HCI_ULP_EXT_ADV_SET_DATA_T, adv_handle),
    pdufmt_set_ea_data, /* fmt_of_header */

    offsetof(HCI_ULP_EXT_ADV_SET_DATA_T, adv_data_len),
    offsetof(HCI_ULP_EXT_ADV_SET_DATA_T, adv_data),
    pdufmt_u8, /* fmt_of_entry */

    HCI_ULP_ADV_DATA_BYTES_PER_PTR, /* entries_per_block */
    sizeof(uint8_t), /* size_of_entry */
    HCI_ULP_ADV_DATA_BYTES_PER_PTR * sizeof(uint8_t) /* size_of_block */
};

const pdu_array_info set_ea_scan_resp_data_array_info = {
    sizeof(HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_T),

    offsetof(HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_T, adv_handle),
    pdufmt_set_ea_scan_resp_data, /* fmt_of_header */

    offsetof(HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_T, scan_resp_data_len),
    offsetof(HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_T, scan_resp_data),
    pdufmt_u8, /* fmt_of_entry */

    HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR, /* entries_per_block */
    sizeof(uint8_t), /* size_of_entry */
    HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR * sizeof(uint8_t) /* size_of_block */
};

const pdu_array_info ea_enable_array_info = {
    sizeof(HCI_ULP_EXT_ADV_ENABLE_T),

    offsetof(HCI_ULP_EXT_ADV_ENABLE_T, enable),
    pdufmt_ea_enable, /* fmt_of_header */

    offsetof(HCI_ULP_EXT_ADV_ENABLE_T, num_of_sets),
    offsetof(HCI_ULP_EXT_ADV_ENABLE_T, adv_sets),
    pdufmt_ea_enable_adv_sets, /* fmt_of_entry */

    HCI_ULP_ENABLE_ADV_SETS_PER_PTR, /* entries_per_block */
    sizeof(EA_ENABLE_ADV_SET_T), /* size_of_entry */
    HCI_ULP_ENABLE_ADV_SETS_PER_PTR * sizeof(EA_ENABLE_ADV_SET_T) /* size_of_block */
};

const pdu_array_info set_es_params_array_info = {
    sizeof(HCI_ULP_EXT_SCAN_SET_PARAMS_T),

    offsetof(HCI_ULP_EXT_SCAN_SET_PARAMS_T, own_addr_type),
    pdufmt_set_es_params, /* fmt_of_header */

    offsetof(HCI_ULP_EXT_SCAN_SET_PARAMS_T, scanning_phys), /* TODO MRB This is a bit field not number so needs converting? */
    offsetof(HCI_ULP_EXT_SCAN_SET_PARAMS_T, scanning_phy),
    pdufmt_set_es_params_scanning_phy, /* fmt_of_entry */

    HCI_ULP_SCANNING_PHYS_PER_PTR, /* entries_per_block */
    sizeof(ES_SCANNING_PHY_T), /* size_of_entry */
    HCI_ULP_SCANNING_PHYS_PER_PTR * sizeof(ES_SCANNING_PHY_T) /* size_of_block */
};

const pdu_array_info ext_adv_report_array_info = {
    sizeof(HCI_EV_ULP_EXT_ADV_REPORT_T), /* size_of_base */

    offsetof(HCI_EV_ULP_EXT_ADV_REPORT_T, num_reports), /* offset_of_header */
    pdufmt_ext_adv_resp, /* fmt_of_header */

    offsetof(HCI_EV_ULP_EXT_ADV_REPORT_T, data_length), /* offset_of_count */
    offsetof(HCI_EV_ULP_EXT_ADV_REPORT_T, data), /* offset_of_array */
    pdufmt_u8, /* fmt_of_entry */

    HCI_ULP_EV_ADV_DATA_BYTES_PER_PTR, /* entries_per_block */
    sizeof(uint8_t), /* size_of_entry */
    HCI_ULP_EV_ADV_DATA_BYTES_PER_PTR * sizeof(uint8_t) /* size_of_block */
};

#endif /* INSTALL_ADVERTISING_EXTENSIONS */

