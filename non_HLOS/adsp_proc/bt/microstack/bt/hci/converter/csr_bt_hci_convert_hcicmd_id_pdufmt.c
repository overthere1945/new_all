/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_bt_hci_convert_hcicmd_private.h"

/* This is the structure that holds info about an opcode.  It contains
 the format for the command, and also the format for the returned
 parameters if there is a command complete message.  Info is also
 stored as to whether we should return a command complete or a
 command status for this opcode.
 */
#define PDUFMT_BASE ((const pdufmt *) &all_pdufmts)
#define PDUFMT_NULL PDUFMT_NULL_INDEX
#define PDUFMT_NULL_INDEX (sizeof(all_pdufmts)/sizeof(pdufmt))

typedef struct
{
    uint16_t error_type ;/* Return PDU type */
    uint16_t pdu_cmd_idx; /* PDU format */
    /* HERE: The following flexelint suppression should not be needed, but
     * for some reason this use of 'error_len' is not been seen.  This
     * should be investigated, fixed and the lint suppression comment removed.
     */
    /*lint -esym(754,error_len) 'error_len' referenced in 
     *     auto generated code #included though 'hcicmd_lut.ch'
     */
    uint16_t error_len;/* Return PDU length */
    uint16_t pdu_ret_idx ; /* Command complete PDU format */
} opcode_info;

/* This is the machine generated table of opcode_info's */
#define LUT_PDUFMT(fmt) \
    (offsetof(struct all_pdufmts_struct, fmt)/sizeof(pdufmt))
#define CMD_LUT_ENTRY(pdu_cmd, pdu_ret, error_type, error_len) \
    { error_type, pdu_cmd, error_len, pdu_ret }
#define NO_CMD_LUT_ENTRY \
    CMD_LUT_ENTRY(PDUFMT_NULL, PDUFMT_NULL, error_ret_status_no_opcode, 0)

static const opcode_info* hcicmd_opcode_info(hci_op_code_t opcode);

#include "csr_bt_hci_convert_hcicmd_lut.ch"

/****************************************************************************
 NAME
 hcicmd_opcode_info  -  match hci command id to pdufmt

 DESCRIPTION
 This function will return a pointer to the opcode_info
 structure for the given opcode.  This function does the work
 for the four functions below.
 */
static const opcode_info* hcicmd_opcode_info(hci_op_code_t opcode)
{
    static const opcode_info none = NO_CMD_LUT_ENTRY;
    const uint16_t ogf = opcode >> HCI_OGF_BIT_OFFSET;
    const uint16_t ocf = opcode & HCI_OPCODE_MASK;

    if (ogf < sizeof(hcicmd_lut_len) / sizeof(hcicmd_lut_len[0]))
    {

        if (ocf < hcicmd_lut_len[ogf])
        {
            return &hcicmd_lut[ogf][ocf];
        }
    }

    return &none;
}

/****************************************************************************
 NAME
 hcicmd_id_pdufmt  -  match hci command id to pdufmt
 */

const pdufmt* hcicmd_id_pdufmt(hci_op_code_t opcode)
{
    uint16_t idx = hcicmd_opcode_info(opcode)->pdu_cmd_idx;

    return idx >= PDUFMT_NULL_INDEX ? NULL : PDUFMT_BASE + idx;
}

/****************************************************************************
 NAME
 hcicmd_id_ret_pdufmt  -  match hci command id to ret pdufmt
 */

const pdufmt* hcicmd_id_ret_pdufmt(hci_op_code_t opcode)
{
    uint16_t idx = hcicmd_opcode_info(opcode)->pdu_ret_idx;

    return idx >= PDUFMT_NULL_INDEX ? NULL : PDUFMT_BASE + idx;
}

