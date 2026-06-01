/*******************************************************************************

Copyright (C) 2008 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dm_private.h"
#include "dm_hci_interface.h"
#include "ext_scan_manager.h"
#ifdef ENABLE_SHUTDOWN

#if !defined(REALLY_ON_HOST) && !defined(BLUESTACK_HOST_IS_APPS)
/*static void dm_hci_free_upstream_primitive(HCI_UPRIM_T *prim)
{
    switch (prim->event_code)
    {
        case HCI_EV_INQUIRY_RESULT:
            {
                pdestroy_array((void **) prim->hci_inquiry_result_event.result,
                               HCI_MAX_INQ_RESULT_PTRS);
                break;
            }

        case HCI_EV_INQUIRY_RESULT_WITH_RSSI:
            {
                pdestroy_array((void **) prim->hci_inquiry_result_with_rssi_event.result,
                               HCI_MAX_INQ_RESULT_PTRS);
                break;
            }

        case HCI_EV_EXTENDED_INQUIRY_RESULT:
            {
                pdestroy_array((void **) prim->hci_extended_inquiry_result_event.eir_data_part,
                               HCI_EIR_DATA_PACKET_PTRS);
                break;
            }

        case HCI_EV_REMOTE_NAME_REQ_COMPLETE:
            {
                pdestroy_array((void **) prim->hci_remote_name_request_complete.name_part,
                               HCI_LOCAL_NAME_BYTE_PACKET_PTRS);
                break;
            }

        case HCI_EV_LOOPBACK_COMMAND:
            {
                pdestroy_array((void **) prim->hci_loopback_command_event.loopback_part_ptr,
                               HCI_LOOPBACK_BYTE_PACKET_PTRS);
                break;
            }

        case HCI_EV_RETURN_LINK_KEYS:
            {
                pdestroy_array((void **) prim->hci_ret_linkkeys_event.link_key_bd_addr,
                               HCI_STORED_LINK_KEY_MAX);
                break;
            }

        case HCI_EV_NUMBER_COMPLETED_PKTS:
            {
                pdestroy_array((void **) prim->hci_num_compl_pkts_event.num_completed_pkts_ptr,
                               HCI_EV_NUM_HANDLE_COMPLETE_PACKET_PTRS);
                break;
            }

        case HCI_EV_CSB_RECEIVE:
            {
                pdestroy_array((void **) prim->hci_csb_receive.data_part,
                               HCI_CSB_RECV_PACKET_PTRS);
                break;
            }

        case HCI_EV_COMMAND_COMPLETE:
            {
                switch (prim->hci_command_complete_event.op_code)
                {
                    case HCI_READ_EXTENDED_INQUIRY_RESPONSE_DATA:
                        {
                            pdestroy_array((void **) prim->hci_command_complete_event.argument_ptr->read_extended_inquiry_response_data_args.eir_data_part,
                                           HCI_EIR_DATA_PACKET_PTRS);
                            break;
                        }

                    case HCI_READ_LOCAL_NAME:
                        {
                            pdestroy_array((void **) prim->hci_command_complete_event.argument_ptr->read_local_name_args.name_part,
                                           HCI_LOCAL_NAME_BYTE_PACKET_PTRS);
                            break;
                        }

                    case HCI_READ_LOCAL_SUPP_COMMANDS:
                        {
                            pdestroy_array((void **) prim->hci_command_complete_event.argument_ptr->read_local_supp_commands_args.supp_commands,
                                           HCI_READ_SUPP_COMMANDS_PACKET_PTRS);
                            break;
                        }

                    case HCI_READ_CURRENT_IAC_LAP:
                        {
                            pdestroy_array((void **) prim->hci_command_complete_event.argument_ptr->read_current_iac_lap_args.iac_lap,
                                           HCI_IAC_LAP_PTRS);
                            break;
                        }

                    default:
                        {
                            break;
                        }
                }

                pfree(prim->hci_command_complete_event.argument_ptr);
                break;
            }

        default:
            {
                break;
            }
    }

    pfree(prim);
}
*/
#endif

static void adv_scan_mgt_queue_deinit(void)
{
#if 0
    uint16_t msg_type;
    void *msg_data;

    /* Empty message queue. */
    while (get_message(ADV_SCAN_IFACEQUEUE, &msg_type, &msg_data))
    {
        if (msg_type == ENV_PRIM)
            pfree(msg_data);
        else
            dm_free_downstream_primitive((DM_UPRIM_T*)msg_data);
    }
#endif
}
#if 0
#if !defined(REALLY_ON_HOST) && !defined(BLUESTACK_HOST_IS_APPS)
void dm_hci_deinit(void **gash)
{
    uint16_t msg_type;
    void *msg_data;

    while (get_message(DM_HCIQUEUE, &msg_type, &msg_data))
    {
        if (msg_type == DM_PRIM)
        {
            mblk_destroy(((DM_DATA_FROM_HCI_REQ_T*)msg_data)->data);
            pfree(msg_data);
        }
        else if (msg_type == HCI_PRIM)
        {
            dm_hci_free_upstream_primitive((HCI_UPRIM_T *) msg_data);
        }
        else
        {
            pfree(msg_data);
        }
    }
}
#endif
#endif
void adv_scan_deinit(void **gash)
{
    QBL_UNUSED(gash);

    adv_scan_mgt_queue_deinit();
    dm_hci_interface_deinit();

#ifdef BUILD_FOR_HOST
    CsrBtHcishimDeinit();
#endif
    MsExtScanManagerDeInit();

}

#endif

