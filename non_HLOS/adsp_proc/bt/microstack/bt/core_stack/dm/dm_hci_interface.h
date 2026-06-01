/*******************************************************************************

Copyright (C) 2024 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifndef _DM_HCI_INTERFACE_H_
#define _DM_HCI_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qbl_adapter_hci.h"                    /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_panic.h"                  /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_pmalloc.h"                /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_types.h"                  /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_logging.h"                /* QBLUESTACK_REFACTOR */

#include INC_DIR(common,common.h)
#include INC_DIR(common,error.h)

#include <string.h>
#include <stdio.h>
#include INC_DIR(mblk,mblk.h)
#include "qbl_patch.h"
#include INC_DIR(tbdaddr,tpbdaddr.h)
#include INC_DIR(bluestack,bluetooth.h)         /* QBLUESTACK_REFACTOR */


typedef struct
{
    uint8_t                 data_length;
    uint16_t                flow_control_flags; 
    uint8_t                 credit_not_required;
    uint32_t                context;
    uint16_t                phandle;
} DM_VS_CMD_INFO_T;

bool microStackConsumedEvent;
#define IS_EVENT_CONSUMED_IN_MICRO_STACK()  (microStackConsumedEvent)
#define SET_EVENT_CONSUMED_IN_MICRO_STACK() (microStackConsumedEvent = TRUE)
#define RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG() (microStackConsumedEvent = FALSE)


extern uint16_t micro_stack_command_credit_available;

bool_t dm_hci_l2cap_data(
        uint16_t logical_link_id, 
        MBLK_T *mblk);
void dm_hci_event_handler(HCI_UPRIM_T* hci_prim);

void dm_hci_handle_completed_packets(uint16_t handle,
                                     uint16_t completed_packets);
void DM_SendMessage(void *message);
void dm_hci_interface_deinit(void);
void send_to_hci(HCI_UPRIM_T *pv_hci_uprim);
extern void dm_hci_ulp_ea_cleanup(void);

#ifdef L2CAP_HCI_DATA_CREDIT_SLOW_CHECKS
/*! \brief Ensure DM and L2CAP agree on how many credits are outstanding. */
void dm_hci_data_credit_audit(void);
#else
#define dm_hci_data_credit_audit()
#endif

extern bool IsMicroStackInterestedInThisEvent(hci_event_code_t event_code, uint8_t ulp_sub_opcode, uint16_t cmd_opcode);
extern void dm_free_hci_event(HCI_UPRIM_T *evt);


#ifdef __cplusplus
}
#endif

#endif /* _DM_HCI_INTERFACE_H_ */
