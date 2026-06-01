/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/
#ifndef __HCICMD_PRIVATE_H__
#define __HCICMD_PRIVATE_H__

#include "qbl_hci.h"
#include "csr_bt_hci_convert_pdufmt_private.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 NAME
 hcicmd_id_pdufmt  -  match hci command id to pdufmt

 DESCRIPTION
 This function will return a pointer to the pdufmt array that
 is associated with the given opcode.  If the opcode is unknown,
 or has a complex (variable length) PDU, we will return NULL.  If
 the PDU contains no further information (apart from the opcode
 and length), we will return pdufmt_empty.
 */

const pdufmt* hcicmd_id_pdufmt(hci_op_code_t opcode);

/****************************************************************************
 NAME
 hcicmd_id_ret_pdufmt  -  match hci command id to ret pdufmt

 DESCRIPTION
 This function will return the pdufmt for the return data
 structure.  This is the data that is returned in the command
 complete event for this opcode.  If the opcode has no return
 data then pdufmt_empty is returned.  If the opcode should not
 generate a command complete event, or we do not know about the
 opcode, we return NULL.
 */

const pdufmt* hcicmd_id_ret_pdufmt(hci_op_code_t opcode);

/****************************************************************************
 NAME
 hcicmd_array_serialise  -  Write in an array pdu.

 DESCRIPTION
 This function will read in an array PDU.  This will contains a
 number of repeated blocks (at the end of the pdu).  The
 'pdu_array_info' structure contains all of the information for
 reading these.  The only thing that cannot be changed is the
 size of the repeat count (it must be a uint8).  The format of
 the pdu_array_info is described in 'pdufmt_private.h'.
 */

void hcicmd_array_serialise(uint8_t **pdu,
                                const pdu_array_info* array_info,
                                void* src_struct);

/****************************************************************************
 NAME
 hcicmd_array_serialise_no_header  -  Write in an array pdu (of N blocks).

 DESCRIPTION
 This function reads in the repeated section of an array pdu.
 Only the last few fields of the 'pdu_array_info' structure are
 valid (see pdufmt_private.h for more info).  It reads in
 'count' blocks.  No checking of 'count' or the buffer size is
 performed.

 info->out_struct must be a valid pointer when this is called.
 */

void hcicmd_array_serialise_no_header(uint8_t **pdu,
                                             const pdu_array_info* array_info,
                                             void* base,
                                             uint8_t num);

/****************************************************************************
 NAME
 hcicmd_set_event_filter  -  Write a SEF command

 DESCRIPTION
 This function deals with reading the Set Event Filter command
 from the host.  It checks that the command makes sense (is the
 correct length, and the values are in range).  This is the
 only truly anoying pdu.
 */

Bool hcicmd_set_event_filter(uint8_t **pdu, HCI_UPRIM_T * info);

#ifdef __cplusplus
}
#endif

#endif /* __HCICMD_PRIVATE_H__ */

