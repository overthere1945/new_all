/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "csr_synergy.h"

#ifdef INCLUDE_HCI_ARBITER
#include "hci_arbiter_private.h"
#include "csr_log_text_2.h"
#include "csr_hci_lib.h"
#include "dm_hci_interface.h"
#include "csr_bt_hci_convert.h"
#include "csr_hci_common.h"
#include "csr_hci_util.h"
#include "hci_cmd_arb.h"
#include "ext_adv_manager.h"
#include "csr_hci_util.h"
#include "csr_qvsc_task.h"
#include "hci_arbiter_event.h"
#include "csr_serial_com.h"
#include "hci_ble_scan_arb.h"
#include "bt_main.h"
#include "csr_hci_prim.h"
#include "csr_result.h"
#include "csr_hci_sef.h"

typedef enum {
    EVENT_TYPE_HCI,
    EVENT_TYPE_PERI
} EventType;

static bool IsCrashEventFromController(void *prim, EventType type)
{
    bool result = FALSE;
    hci_event_code_t event_code;
    uint8 event_class;
    uint8 msg_type;
    HciArbiterBtssErrorCode reason = 0;
    uint8 *payload;
    uint8 payloadLength = 0;

    if (prim == NULL)
    {
        return FALSE;
    }

    if (type == EVENT_TYPE_HCI)
    {
        CsrHciEventInd *hciPrim = (CsrHciEventInd *)prim;

        payload = hciPrim->payload;
        payloadLength = hciPrim->payloadLength;

        if (payloadLength > BT_HCI_CRASH_EVENT_MIN_SIZE)
        {
            event_code = CSR_BT_CONVERT_8_FROM_STREAM(&payload[0]);
            event_class = CSR_BT_CONVERT_8_FROM_STREAM(&payload[2]);
            msg_type = CSR_BT_CONVERT_8_FROM_STREAM(&payload[3]);

            if (event_code == CRASH_EVENT_CODE && event_class == BT_CRASH_EVENT_CLASS && msg_type == BT_CRASH_EVENT_MSG_TYPE)
            {
                result = TRUE;
                reason = HCI_ARBITER_BTSS_BT_ERROR;
            }
        }
    }
    else if (type == EVENT_TYPE_PERI)
    {
        CsrPeriEventInd *periPrim = (CsrPeriEventInd *)prim;

        payload = periPrim->payload;
        payloadLength = periPrim->payloadLength;

        if (payloadLength > PERI_HCI_CRASH_EVENT_MIN_SIZE)
        {
            event_code = CSR_BT_CONVERT_8_FROM_STREAM(&payload[1]);
            event_class = CSR_BT_CONVERT_8_FROM_STREAM(&payload[3]);
            msg_type = CSR_BT_CONVERT_8_FROM_STREAM(&payload[4]);

            if (event_code == CRASH_EVENT_CODE && event_class == PERI_CRASH_EVENT_CLASS && msg_type == PERI_CRASH_EVENT_MSG_TYPE)
            {
                uint8 dump_reason = CSR_BT_CONVERT_8_FROM_STREAM(&payload[9]);
                result = TRUE;

                if (dump_reason == BTSS_SMC_CRASH)
                {
                    reason = HCI_ARBITER_BTSS_SMC_ERROR;
                }
                else
                {
                    reason = HCI_ARBITER_BTSS_PERI_ERROR;
                }
            }
        }
    }

    if (result)
    {
        HciArbiterBtssCrashIndSend(reason);
        CsrHciBlockTransmission();
    }

    return result;
}

bool HciArbHandleHciEventInMicroStack(CsrHciEventInd *prim)
{
    HCI_UPRIM_T *evt = NULL;
    bool eventConsumed = FALSE;
    RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();
    hci_event_code_t eventCode = prim->payload[0];
    uint8_t ulpSubOpcode = 0x00;
    uint16_t cmdOpcode = 0x00;

    CSR_LOG_TEXT_INFO((HCI, 0, "Hci Event length %d eventCode %x", prim->payloadLength, eventCode));

    if (IsCrashEventFromController((void*)prim, EVENT_TYPE_HCI))
    {
        CSR_LOG_TEXT_INFO((HCI, 0, "crash evt"));
        eventConsumed = TRUE;
    }
    else
    {
        if(HciArbIsPassthroughMode())
        {
            if(eventCode == HCI_EV_COMMAND_STATUS)
            {
                if(((((uint16) prim->payload[5]) << 8) | ((uint16) prim->payload[4])) == 0x0000)
                {
                    CSR_LOG_TEXT_INFO((HCI, 0, "NOP evt consumed in Microstack"));
                    SET_EVENT_CONSUMED_IN_MICRO_STACK();
                }
                eventConsumed = IS_EVENT_CONSUMED_IN_MICRO_STACK();
                CSR_LOG_TEXT_INFO((HCI, 0, "eventConsumed %d", eventConsumed));
            }
        }
        else if(IsHciEventMeantForSpecialArbitrationLogic(prim))
        {
            eventConsumed = TRUE;
        }
        else
        {
            if (eventCode == HCI_EV_ULP)
            {
                ulpSubOpcode = prim->payload[2];
                CSR_LOG_TEXT_INFO((HCI, 0, "ulpSubOpcode %x", ulpSubOpcode));
            }
            
            if (eventCode == HCI_EV_COMMAND_COMPLETE)
            {
                cmdOpcode = ((((uint16) prim->payload[4]) << 8) | ((uint16) prim->payload[3]));
                CSR_LOG_TEXT_INFO((HCI, 0, "cmdOpcode %x", cmdOpcode));
            }

            if(IsMicroStackInterestedInThisEvent(eventCode, ulpSubOpcode, cmdOpcode))
            {
                evt = CsrBtConvertHciEvent(((CsrHciEventInd*)prim)->payload);
            }

            if( evt != NULL )
            {
                dm_hci_event_handler(evt);
                eventConsumed = IS_EVENT_CONSUMED_IN_MICRO_STACK();
                CSR_LOG_TEXT_INFO((HCI, 0, "eventConsumed %d", eventConsumed));
                dm_free_hci_event(evt);
            }
            RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();
        }
    }

    return eventConsumed;
}


void HciArbProcessHciEvents(CsrHciEventInd *prim)
{
    if(!HciArbHandleHciEventInMicroStack(prim))
    {
        uint16 len = prim->payloadLength+1;
        uint8  *payload = CsrPmemAlloc(len);
        payload[0] = HCI_ARB_HCI_EVT_TYPE;
        CSR_LOG_TEXT_INFO((HCI, 0, "Evt sent to primary stack"));
        SynMemCpyS(&payload[1], prim->payloadLength, prim->payload, prim->payloadLength);
        if(HasPrimaryStackTakenTheHCICmdArbLock())
        {
            HciCommandArbHouseCleaningSend(HCI_CMD_SENDER_PRIMARY_STACK);
        }
        HciArbSendHciMsgToPrimaryStack(len, payload);
    }

}

bool HciArbHandleCommandCompleteEventForVendorSpecificCommand(CsrHciVendorSpecificEventInd *prim)
{
    RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();

    if(IsMicroStackWaitingForAnyResponseEvent())
    {
        CSR_LOG_TEXT_INFO((HCI, 0, "VS CC consumed in Microstack"));
        SET_EVENT_CONSUMED_IN_MICRO_STACK();
        CsrHciSendVendorSpecificEventIndExt(CSR_QVSC_IFACEQUEUE, prim->channel, prim->event, prim->data);
        HciCommandArbHouseCleaningSend(HCI_CMD_SENDER_MICRO_STACK);
        prim->data = NULL;
    }
    return (IS_EVENT_CONSUMED_IN_MICRO_STACK());
}

void HciArbProcessCommandCompleteEventForVendorSpecificCommand(CsrHciVendorSpecificEventInd *prim)
{
    if(!HciArbHandleCommandCompleteEventForVendorSpecificCommand(prim))
    {
        CsrUint16 length;
        CsrUint8 *payload;
        length = CsrMblkGetLength(prim->data);

        payload = CsrPmemAlloc(length + 2);
        payload[0] = HCI_ARB_HCI_EVT_TYPE;
        payload[1] = CSR_HCI_EV_COMMAND_COMPLETE;
        CsrMblkCopyToMemory(prim->data, 0, length, &payload[2]);
        CSR_LOG_TEXT_INFO((HCI, 0, "VS CC sent to Primary stack"));
        if(HasPrimaryStackTakenTheHCICmdArbLock())
        {
            HciCommandArbHouseCleaningSend(HCI_CMD_SENDER_PRIMARY_STACK);
        }
        HciArbSendHciMsgToPrimaryStack(length + 2, payload);
    }
}

void HciArbProcessPeriEvents(CsrPeriEventInd *prim)
{
    if (IsCrashEventFromController((void*)prim, EVENT_TYPE_PERI))
    {
        /* Post HCI Arbiter msg for BTSS crash event */
    }
    else
    {
        CsrUint16 len = prim->payloadLength+1;

        for(uint16 i=0; i < prim->payloadLength && i < 20;i++)
        {
            CSR_LOG_TEXT_INFO((HCI, 0, "%d", prim->payload[i]));
        }

        if (PeriCmdInternal)
        {
            if (len == PERI_ACTIVATE_COMPLETE_EVENT_LENGTH && prim->payload[PERI_ACTIVATE_COMPLETE_MSG_TYPE_OFFSET-1] == HCI_PERI_SS_ACTIVATE_COMPLETE &&
                    prim->payload[PERI_ACTIVATE_COMPLETE_SUBSYSTEM_OFFSET-1] == BTSS_SUBSYSTEM && prim->payload[PERI_ACTIVATE_COMPLETE_ACTION_OFFSET-1] == BT_OFF_ACTION)
            {
                CSR_LOG_TEXT_INFO((HCI, 0, "PERI BT OFF rcvd"));

                PeriCmdInternal = 0;

                HciArbiterBtOffIndSend(HCI_ARBITER_RESULT_SUCCESS);
                CsrHciBlockTransmission();
            }
            else if (len == PERI_COMMAND_STATUS_EVENT_LENGTH && prim->payload[PERI_COMMAND_STATUS_MSG_TYPE_OFFSET-1] == HCI_PERI_COMMAND_STATUS)
            {
                if (prim->payload[PERI_COMMAND_STATUS_RESULT_OFFSET-1] != 0x00)
                {
                    CSR_LOG_TEXT_INFO((HCI, 0, "PERI Cmd status evt failed, Panic"));
                }
                else
                {
                    CSR_LOG_TEXT_INFO((HCI, 0, "PERI Cmd status evt success"));
                }
            }
        }
        else
        {
            CsrUint8  *payload = CsrPmemAlloc(len);
            payload[0] = HCI_ARB_PERI_EVT_TYPE;
            CSR_LOG_TEXT_INFO((HCI, 0, "Evt sent to primary stack"));
            SynMemCpyS(&payload[1], prim->payloadLength, prim->payload, prim->payloadLength);
            if(HasPrimaryStackTakenTheHCICmdArbLock())
            {
                HciCommandArbHouseCleaningSend(HCI_CMD_SENDER_PRIMARY_STACK);
            }
            HciArbSendHciMsgToPrimaryStack(len, payload);
        }
    }
}

#endif
