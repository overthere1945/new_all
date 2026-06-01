/******************************************************************************
 Copyright (c) 2008-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/

#include "csr_bt_hci_convert.h"
#include "csr_bt_hci_convert_hcicmd_private.h"
#include "csr_bt_hci_convert_hcievt_private.h"


static Bool csrBtConvertHciHandleManualCmd(uint8_t **buffer,
                                                         HCI_UPRIM_T *prim)
{
    /*Assume everything will go well initially*/
    Bool status = TRUE;

#ifdef INSTALL_ADVERTISING_EXTENSIONS
    if (prim->hci_cmd.op_code == HCI_ULP_EXT_ADV_SET_DATA)
    {
        hcicmd_array_serialise(buffer, &set_ea_data_array_info, prim);
    }
    else if (prim->hci_cmd.op_code == HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA)
    {
        hcicmd_array_serialise(buffer, &set_ea_scan_resp_data_array_info, prim);
    }
    else if (prim->hci_cmd.op_code == HCI_ULP_EXT_ADV_ENABLE)
    {
        hcicmd_array_serialise(buffer, &ea_enable_array_info, prim);
    }
    else if (prim->hci_cmd.op_code == HCI_ULP_EXT_SCAN_SET_PARAMS)
    {
        hcicmd_array_serialise(buffer, &set_es_params_array_info, prim);
    }
    else
    {
        /*There is no format fetched for this opcode correctly*/
        status = FALSE;
    }
#endif
    return (status);

}

static Bool csrBtConvertHciHandleManualEvt(const uint8_t * * hciData,
                                                       hcievt_info* info, 
                                                       const pdufmt *fmt)
{
    Bool sendHciEvent = TRUE;

    switch(info->start.event_code)
   {
        case HCI_EV_COMMAND_COMPLETE:
        {
            info->out_struct = (HCI_UPRIM_T*) qbl_zpmalloc(sizeof(HCI_EV_COMMAND_COMPLETE_T));
            info->out_struct->hci_event.event_code = info->start.event_code;
            info->out_struct->hci_event.length = info->start.length;

            sendHciEvent = hcievt_cmd_complete_deserialise(hciData,
                                                           &info->out_struct->hci_command_complete_event);

            if (sendHciEvent == FALSE)
            {
                qbl_pfree(info->out_struct->hci_command_complete_event.argument_ptr);
                /*Initialising to NULL to avoid multiple mem free*/
                info->out_struct->hci_command_complete_event.argument_ptr = NULL;
            }
        }
        break;

        case HCI_EV_NUMBER_COMPLETED_PKTS:
        {
            sendHciEvent = hcievt_array_deserialise(hciData,
                                                    info,
                                                    &num_compl_pkts_array_info);
        }
        break;

#ifdef INSTALL_ULP
        case HCI_EV_ULP:
        {

#ifdef INSTALL_ADVERTISING_EXTENSIONS
            if (info->start.ulp_sub_opcode == HCI_EV_ULP_EXT_ADV_REPORT)
            {
                info->out_struct = (HCI_UPRIM_T*) qbl_zpmalloc(sizeof(HCI_EV_ULP_EXT_ADV_REPORT_T));

                rdpdu_struct(hciData,
                             pdufmt_u8,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.num_reports);

                rdpdu_struct(hciData,
                             pdufmt_u16,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.event_type);

                rdpdu_struct(hciData,
                             pdufmt_u8,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.addr_type);

                rdpdu_struct(hciData,
                             pdufmt_bdaddr,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.addr);

                rdpdu_struct(hciData,
                             pdufmt_u8,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.primary_phy);

                rdpdu_struct(hciData,
                             pdufmt_u8,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.secondary_phy);

                rdpdu_struct(hciData,
                             pdufmt_u8,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.adv_sid);

                rdpdu_struct(hciData,
                             pdufmt_u8,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.tx_power);

                rdpdu_type(hciData,
                           pdufmt_el_uint8_t,
                           (void *) ((&info->out_struct->hci_ulp_ext_adv_report.rssi)));

                rdpdu_struct(hciData,
                             pdufmt_u16,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.periodic_adv_interval);

                rdpdu_struct(hciData,
                             pdufmt_u8,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.direct_addr_type);

                rdpdu_struct(hciData,
                             pdufmt_bdaddr,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.direct_addr);

                rdpdu_struct(hciData,
                             pdufmt_u8,
                             (void *) &info->out_struct->hci_ulp_ext_adv_report.data_length);

                /* Write advertising data */
                sendHciEvent = hcievt_array_deserialise_got_count(hciData,
                                                                  info->out_struct,
                                                                  &ext_adv_report_array_info,
                                                                  info->out_struct->hci_ulp_ext_adv_report.data_length);
            }
#endif /* INSTALL_ADVERTISING_EXTENSIONS */
        }
        break;
#endif/* INSTALL_ULP */

        default:
        {
            sendHciEvent = FALSE;
        }
        break;
    }

    return sendHciEvent;
}

Bool CsrBtConvertHciCommand(HCI_UPRIM_T *prim, uint8_t *buffer)
{
    const pdufmt *fmt;
    Bool status = TRUE;

    fmt = hcicmd_id_pdufmt(prim->hci_cmd.op_code);

    wrpdu_struct(&buffer, (pdufmt *) pdufmt_cmd_start, (void *) &prim->hci_cmd);

    if (fmt == NULL)
    {
        status = csrBtConvertHciHandleManualCmd(&buffer, prim);
    }
    else if (fmt != pdufmt_empty)
    {
        /*Fill in the remaining elements into the buffer*/
        wrpdu_struct(&buffer, fmt, (void *) ((&prim->hci_cmd) + 1));
    }

    return status;
}

HCI_UPRIM_T *CsrBtConvertHciEvent(const uint8_t *hciData)
{
    const pdufmt *fmt;
    uint8_t length = CSR_BT_CONVERT_8_FROM_STREAM(&hciData[1]);

    if (length > 0)
    {
        HCI_UPRIM_T *prim = NULL;
        hcievt_info info;
        Bool sendHciEvent = TRUE;

        info.out_struct = NULL;
        info.start.event_code = CSR_BT_CONVERT_8_FROM_STREAM(hciData);
        info.start.length = length;
        hciData = &hciData[2];

#ifdef INSTALL_ULP
        if (info.start.event_code == HCI_EV_ULP)
        {
            info.start.ulp_sub_opcode = CSR_BT_CONVERT_8_FROM_STREAM(hciData);
            hciData = &hciData[1];

            fmt = hcievt_ulp_id_pdufmt(info.start.ulp_sub_opcode);
        }
        else
#endif
        {
            fmt = hcievt_id_pdufmt(info.start.event_code);
        }

        if (fmt != NULL)
        {
#ifdef INSTALL_ULP
            if (info.start.event_code == HCI_EV_ULP)
            {
                SKIP_PDUFMT_BY_COUNT(fmt, sizeof(HCI_ULP_EVENT_COMMON_T));
            }
            else
#endif
            {
                SKIP_PDUFMT_BY_COUNT(fmt, sizeof(HCI_EVENT_COMMON_T));
            }
            sendHciEvent = hcievt_rdpdu_struct(&hciData, &info, fmt);
        }

        else
        {
            /* To reach here the pdu must be one of the handful of variable
            sized pdu types.  We handle these ugly special cases by hand.
            The special case code must check the length of the PDU (as well
            as whether it makes sense).  */

            sendHciEvent = csrBtConvertHciHandleManualEvt(&hciData, &info, fmt);
        }

        if (sendHciEvent)
        {
            info.out_struct->hci_event.event_code = info.start.event_code;
            info.out_struct->hci_event.length = info.start.length;

#ifdef INSTALL_ULP
            if (info.start.event_code == HCI_EV_ULP)
            {
                info.out_struct->hci_ulp_event.ulp_sub_opcode = info.start.ulp_sub_opcode;
            }
#endif
            prim = info.out_struct;

            return prim;
        }
        else
        {
            qbl_pfree(info.out_struct);
            return NULL;
        }
    }

    return NULL;
}

