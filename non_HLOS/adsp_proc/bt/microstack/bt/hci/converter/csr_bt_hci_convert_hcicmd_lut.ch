/****************************************************************************
 Copyright (c) 2016-2026 Qualcomm Technologies International, Ltd.

 FILE:           
 csr_bt_hci_convert_hcicmd_lut.ch

 DESCRIPTION:   
 Top level converter for _hci commands and events

 REVISION:      : #5 


****************************************************************************/
  /* This file is AUTO-GENERATED. DO NOT modify! */
/* fields are:
 *   command_format
 *   response_format
 *   response_type
 *   response_length
 *
 * command_format is only NULL if there is no command for this
 * opcode, or it is a special command that is handled by hand
 * roled code.
 *
 * response_length is only non-zero if the response_type is a
 * command complete of some sort.
 */

static const opcode_info hcicmd_lut_ogf_1[] = {
};
static const opcode_info hcicmd_lut_ogf_2[] = {
};
static const opcode_info hcicmd_lut_ogf_3[] = {
};
static const opcode_info hcicmd_lut_ogf_4[] = {
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    CMD_LUT_ENTRY(/* HCI_READ_BUFFER_SIZE */
        LUT_PDUFMT(empty),
        LUT_PDUFMT(Su16sSu8sSu16sSu16s),
        error_ret_cmnd_cmplt, HCI_READ_BUFFER_SIZE_ARG_LEN
    ),
};
static const opcode_info hcicmd_lut_ogf_5[] = {
};
static const opcode_info hcicmd_lut_ogf_6[] = {
};
static const opcode_info hcicmd_lut_ogf_8[] = {
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    CMD_LUT_ENTRY(/* HCI_ULP_READ_BUFFER_SIZE */
        LUT_PDUFMT(empty),
        LUT_PDUFMT(Su16sSu8s),
        error_ret_cmnd_cmplt, HCI_ULP_READ_BUFFER_SIZE_ARG_LEN
    ),
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_ADV_SET_RANDOM_ADDR */
        LUT_PDUFMT(Su8sSu24sSu8sSu16s),
        LUT_PDUFMT(empty),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_ADV_SET_RANDOM_ADDR_ARG_LEN
    ),
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_ADV_SET_PARAMS */
        
        LUT_PDUFMT(Su8sSu16sSu24sSu24sSu8sSu8sSu8sSu24sSu8sSu16sSu8sSi8sSu8sSu8sSu8sSu8sSu8s),
        LUT_PDUFMT(Si8s),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_ADV_SET_PARAMS_ARG_LEN
    ),
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_ADV_SET_DATA */
        PDUFMT_NULL,
        LUT_PDUFMT(empty),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_ADV_SET_DATA_ARG_LEN
    ),
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA */
        PDUFMT_NULL,
        LUT_PDUFMT(empty),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_ARG_LEN
    ),
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_ADV_ENABLE */
        PDUFMT_NULL,
        LUT_PDUFMT(empty),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_ADV_ENABLE_ARG_LEN
    ),
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN */
        LUT_PDUFMT(empty),
        LUT_PDUFMT(Su16s),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_ARG_LEN
    ),
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_ADV_READ_NUM_OF_ADV_SETS */
        LUT_PDUFMT(empty),
        LUT_PDUFMT(Su8s),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_ADV_READ_NUM_OF_ADV_SETS_ARG_LEN
    ),
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_ADV_REMOVE_ADV_SET */
        LUT_PDUFMT(Su8s),
        LUT_PDUFMT(empty),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_ADV_REMOVE_ADV_SET_ARG_LEN
    ),
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    NO_CMD_LUT_ENTRY, /* -nothing-here- */
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_SCAN_SET_PARAMS */
        PDUFMT_NULL,
        LUT_PDUFMT(empty),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_SCAN_SET_PARAMS_ARG_LEN
    ),
    CMD_LUT_ENTRY(/* HCI_ULP_EXT_SCAN_ENABLE */
        LUT_PDUFMT(Su8sSu8sSu16sSu16s),
        LUT_PDUFMT(empty),
        error_ret_cmnd_cmplt, HCI_ULP_EXT_SCAN_ENABLE_ARG_LEN
    ),
};
static const opcode_info * const hcicmd_lut[] = {
    NULL,
    hcicmd_lut_ogf_1,
    hcicmd_lut_ogf_2,
    hcicmd_lut_ogf_3,
    hcicmd_lut_ogf_4,
    hcicmd_lut_ogf_5,
    hcicmd_lut_ogf_6,
    NULL,
    hcicmd_lut_ogf_8,
};

static const uint16_t hcicmd_lut_len[] = {
    0,
    sizeof(hcicmd_lut_ogf_1) / sizeof(hcicmd_lut_ogf_1[0]),
    sizeof(hcicmd_lut_ogf_2) / sizeof(hcicmd_lut_ogf_2[0]),
    sizeof(hcicmd_lut_ogf_3) / sizeof(hcicmd_lut_ogf_3[0]),
    sizeof(hcicmd_lut_ogf_4) / sizeof(hcicmd_lut_ogf_4[0]),
    sizeof(hcicmd_lut_ogf_5) / sizeof(hcicmd_lut_ogf_5[0]),
    sizeof(hcicmd_lut_ogf_6) / sizeof(hcicmd_lut_ogf_6[0]),
    0,
    sizeof(hcicmd_lut_ogf_8) / sizeof(hcicmd_lut_ogf_8[0]),
};

