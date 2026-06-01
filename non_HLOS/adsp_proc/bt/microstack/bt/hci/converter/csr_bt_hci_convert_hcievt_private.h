/******************************************************************************
 Copyright (c) 2016-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/
#ifndef __HCIEVT_PRIVATE_H__
#define __HCIEVT_PRIVATE_H__

#include "qbl_hci.h"
#include "csr_bt_hci_convert_pdufmt_private.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SKIP_PDUFMT_BY_COUNT(fmt, count) fmt = fmt + count

typedef struct hcicmd_info_struct
{
    HCI_ULP_EVENT_COMMON_T start;
    HCI_UPRIM_T *out_struct;
} hcievt_info;

/****************************************************************************
 NAME
 hcievt_id_pdufmt  -  match hci event id to pdufmt

 DESCRIPTION
 This will return the pdu format that descripes the given event
 code.  NULL is returned if the event code is not known, or if
 the format is complex.
 */

const pdufmt* hcievt_id_pdufmt(hci_event_code_t event);

#ifdef INSTALL_ULP
/****************************************************************************
 NAME
 hcievt_ulp_id_pdufmt  -  match ulp hci event id to pdufmt

 DESCRIPTION
 This will return the pdu format that descripes the given event
 code.  NULL is returned if the event code is not known, or if
 the format is complex.
 */

const pdufmt* hcievt_ulp_id_pdufmt(hci_event_code_t event);
#endif

/****************************************************************************
 NAME
 hcievt_cmd_complete_deserialise  -  write a command complete event

 DESCRIPTION
 This is a helper function to write the command complete
 events.  I treat these events as two sections, a common
 command complete section, and an extension block that contains
 the data specific to the opcode (this section might be empty).
 */

Bool hcievt_cmd_complete_deserialise(const uint8_t **pdu,
                                               HCI_EV_COMMAND_COMPLETE_T *src);

/****************************************************************************
 NAME
 hcievt_array_deserialise  -  Read in an array pdu.

 DESCRIPTION
 This function will read in an array PDU.  This will contains a
 number of repeated blocks (at the end of the pdu).  The
 'pdu_array_info' structure contains all of the information for
 reading these.  The only thing that cannot be changed is the
 size of the repeat count (it must be a uint8).  The format of
 the pdu_array_info is described in 'pdufmt_private.h'.
 */

Bool hcievt_array_deserialise(const uint8_t **pdu,
                                    hcievt_info* info,
                                    const pdu_array_info* array_info);

/****************************************************************************
 NAME
 hcievt_array_deserialise_got_count  -  read in an array pdu (of N blocks).

 DESCRIPTION
 This function reads in the repeated section of an array pdu.
 Only the last few fields of the 'pdu_array_info' structure are
 valid (see pdufmt_private.h for more info).  It reads in
 'count' blocks.  No checking of 'count' or the buffer size is
 performed.

 info->out_struct must be a valid pointer when this is called.
 */

Bool hcievt_array_deserialise_got_count(const uint8_t **pdu,
                                                 HCI_UPRIM_T* info,
                                                 const pdu_array_info* array_info,
                                                 uint8_t count);

/****************************************************************************
 NAME
 hcievt_rdpdu_struct  -  Read in format of fairly normal pdu
 DESCRIPTION
 This is a helper function that will read in the standard format pdu's.
 */

/*  */
Bool hcievt_rdpdu_struct(const uint8_t * * pdu_data,
                                hcievt_info* info,
                                const pdufmt *fmt);

/****************************************************************************
 NAME
 hcievt_cmd_comp_deserialise  -  Read in command complete array

 DESCRIPTION
 This function will read in an array PDU.  This will contains a
 number of repeated blocks (at the end of the pdu).  The
 'pdu_array_info' structure contains all of the information for
 reading these.  The only thing that cannot be changed is the
 size of the repeat count (it must be a uint8).  The format of
 the pdu_array_info is described in 'pdufmt_private.h'.
 */

Bool hcievt_cmd_comp_deserialise(const uint8_t **pdu,
                                           HCI_UPRIM_T* info,
                                           const pdu_array_info* array_info,
                                           uint8_t num);

#ifdef __cplusplus
}
#endif

#endif /* __HCIEVT_PRIVATE_H__ */

