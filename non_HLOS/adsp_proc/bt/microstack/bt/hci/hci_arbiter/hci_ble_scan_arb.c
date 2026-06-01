/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "hci_cmd_arb.h"
#include "csr_hci_lib.h"
#include "csr_transport.h"
#include "hci_ble_scan_arb.h"
#include "hci.h"
#include "csr_hci_sef.h"
#include "csr_hci_prim.h"

/*! Private Definitions */
typedef enum
{
    SCAN_ARB_IN_PROGRESS,
    SCAN_ARB_RESULT_COMPLETE
}SCAN_ARB_STATE_CHANGE_RESULT_T;

typedef enum
{
    BLE_SCAN_SM_STARTED_EVT = 0,
    BLE_SCAN_DISABLE_RECEIVED_EVT,
    BLE_SCAN_PARAMS_CHANGED_EVT,
    BLE_SCAN_ENABLE_RECEIVED_EVT
}BLE_SCAN_EVT_T;

#define BLE_SCAN_MAX_EVT    (BLE_SCAN_ENABLE_RECEIVED_EVT + 1)

typedef void (*smFunction)(void);

/*! Private Data */
HCI_BLE_SCAN_ARB_T hciBleScanArb;


/*! Private Function Prototype */
static bool IsItBleScanParamCommand( uint8 *payload, uint16 len);
static bool IsItBleScanEnableCommand( uint8 *payload, uint16 len);
static void HciBleScanArbSaveTheScanParamSetCommand(uint8 sender, uint8 *payload, uint16 len);
static void triggerScanStateMachine(BLE_SCAN_ARB_STATE_T startState, BLE_SCAN_ARB_STATE_T targetState);
static void hciBleScanSendSimulatedScanEnableResponse(void);
static void hciBleScanSendSimulatedScanParamResponse(void);
static void hciTriggerScanDisable(void);
static void hciTriggerScanParamSet(void);
static void hciTriggerScanEnable(void);
static void handleEventsScanArbStateMachine(BLE_SCAN_EVT_T evt);
static void exitScanStateMachine(void);


/*! Private Functions */
static void exitScanStateMachine(void)
{
    HciBleScanArbDeinit();
}

static void hciTriggerScanDisable(void)
{
    uint8 scanEnableCmd[]={0x01, 0x42, 0x20, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8 *cmd = CsrPmemAlloc(sizeof(scanEnableCmd));

    SynMemCpyS(cmd, sizeof(scanEnableCmd), scanEnableCmd, sizeof(scanEnableCmd));
    CsrHciCommandReqSend(sizeof(scanEnableCmd), cmd);
}

static void hciTriggerScanParamSet(void)
{
    /* We will always send the cached scan param command. */
    if(hciBleScanArb.cmdPayload)
    {
        hciBleScanArb.scanParamsConfiguredByPrimaryStack = TRUE;
        CsrHciCommandReqSend(hciBleScanArb.cmdPayloadLen, hciBleScanArb.cmdPayload);
    }
    else
    {
        BLUESTACK_PANIC(BT_PANIC_INVALID_SCAN_PARM_NOT_CACHED_STATE);
    }
}

static void hciTriggerScanEnable(void)
{
    uint8 scanEnableCmd[]={0x01, 0x42, 0x20, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8 *cmd = CsrPmemAlloc(sizeof(scanEnableCmd));

    SynMemCpyS(cmd, sizeof(scanEnableCmd), scanEnableCmd, sizeof(scanEnableCmd));
    CsrHciCommandReqSend(sizeof(scanEnableCmd), cmd);
}

static bool IsItBleScanParamCommand( uint8 *payload, uint16 len)
{
    uint16 cmdOpcode;
    cmdOpcode = HCI_ARB_GET_UINT16(payload);
    return (cmdOpcode == HCI_ULP_EXT_SCAN_SET_PARAMS);
}

static bool IsItBleScanEnableCommand( uint8 *payload, uint16 len)
{
    uint16 cmdOpcode;
    cmdOpcode = HCI_ARB_GET_UINT16(payload);
    return ((cmdOpcode == HCI_ULP_EXT_SCAN_ENABLE) && (payload[3]));
}

static bool IsItBleScanDisableCommand( uint8 *payload, uint16 len)
{
    uint16 cmdOpcode;
    cmdOpcode = HCI_ARB_GET_UINT16(payload);
    return ((cmdOpcode == HCI_ULP_EXT_SCAN_ENABLE) && (!payload[3]));
}


static void HciBleScanArbSaveTheScanParamSetCommand(uint8 sender, uint8 *payload, uint16 len)
{
    hciBleScanArb.cmdSender = sender;
    hciBleScanArb.cmdPayload = payload;
    hciBleScanArb.cmdPayloadLen = len;
}

static void hciBleScanSendSimulatedScanParamResponse(void)
{
    uint8 scanParamResp[]={0x04, 0x0e, 0x04, 0x01, 0x41, 0x20, 0x00};
    uint8 *evt = CsrPmemAlloc(sizeof(scanParamResp));

    SynMemCpyS(evt, sizeof(scanParamResp), scanParamResp, sizeof(scanParamResp));
    CsrHciSendEventIndToRegisteredHciEventHandler(evt, sizeof(scanParamResp));
}

static void hciBleScanSendSimulatedScanEnableResponse(void)
{
    uint8 scanEnableCommandResp[]={0x04, 0x0e, 0x04, 0x01, 0x42, 0x20, 0x00};
    uint8 *evt = CsrPmemAlloc(sizeof(scanEnableCommandResp));

    SynMemCpyS(evt, sizeof(scanEnableCommandResp), scanEnableCommandResp, sizeof(scanEnableCommandResp));
    CsrHciSendEventIndToRegisteredHciEventHandler(evt, sizeof(scanEnableCommandResp));
}

static void handleEventsScanArbStateMachine(BLE_SCAN_EVT_T evt)
{
    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "scanArbCurrentState=0x%x evt= 0x%x", hciBleScanArb.scanArbCurrentState, evt));
    switch(hciBleScanArb.scanArbCurrentState)
    {
        case STATE_IDLE:
        {
            if(evt == BLE_SCAN_SM_STARTED_EVT)
            {
                hciBleScanArb.scanArbCurrentState = STATE_SCAN_DISABLE_IN_PROGRESS;
                hciTriggerScanDisable();
            }
            else
                BLUESTACK_PANIC(BT_PANIC_INVALID_SCAN_ARB_STATE);
        }
        break;

        case STATE_SCAN_DISABLE_IN_PROGRESS:
        {
            if(evt == BLE_SCAN_DISABLE_RECEIVED_EVT)
            {
                hciBleScanArb.scanArbCurrentState = STATE_SCAN_DISABLE_DONE;
                if(hciBleScanArb.scanArbTargetState != hciBleScanArb.scanArbCurrentState)
                {
                    hciBleScanArb.scanArbCurrentState = STATE_SCAN_PARAM_SET_IN_PROGRESS;
                    hciTriggerScanParamSet();
                }
                else
                {
                    exitScanStateMachine();
                }
            }
            else
                BLUESTACK_PANIC(BT_PANIC_INVALID_SCAN_ARB_STATE);
        }
        break;

        case STATE_SCAN_PARAM_SET_IN_PROGRESS:
        {
            if(evt == BLE_SCAN_PARAMS_CHANGED_EVT)
            {
                hciBleScanArb.scanArbCurrentState = STATE_SCAN_PARAM_SET_DONE;
                if(hciBleScanArb.scanArbTargetState != hciBleScanArb.scanArbCurrentState)
                {
                    hciBleScanArb.scanArbCurrentState = STATE_SCAN_ENABLE_IN_PROGRESS;
                    hciTriggerScanEnable();
                }
                else
                {
                    exitScanStateMachine();
                }
            }
            else
                BLUESTACK_PANIC(BT_PANIC_INVALID_SCAN_ARB_STATE);
        }
        break;

        case STATE_SCAN_ENABLE_IN_PROGRESS:
        {
            if(evt == BLE_SCAN_ENABLE_RECEIVED_EVT)
            {
                hciBleScanArb.scanArbCurrentState = STATE_SCAN_ENABLE_DONE;
                if(hciBleScanArb.scanArbTargetState != hciBleScanArb.scanArbCurrentState)
                {
                    BLUESTACK_PANIC(BT_PANIC_INVALID_SCAN_ARB_STATE);
                }
                if(hciBleScanArb.cmdPayload)
                {
                    hciBleScanSendSimulatedScanParamResponse();
                }
                exitScanStateMachine();
            }
            else
                BLUESTACK_PANIC(BT_PANIC_INVALID_SCAN_ARB_STATE);
        }
        break;

        default:
                BLUESTACK_PANIC(BT_PANIC_INVALID_SCAN_ARB_STATE);
        break;
    }
    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "scanArbCurrentState=0x%x scanArbTargetState= 0x%x", hciBleScanArb.scanArbCurrentState, hciBleScanArb.scanArbTargetState));
}


static void triggerScanStateMachine(BLE_SCAN_ARB_STATE_T startState, BLE_SCAN_ARB_STATE_T targetState)
{
    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "startState=0x%x target state= 0x%x", startState, targetState));
    if(hciBleScanArb.scanArbCurrentState != STATE_IDLE)
    {
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Wrong Current state."));
        BLUESTACK_PANIC(BT_PANIC_INVALID_SCAN_ARB_STATE);
    }
    hciBleScanArb.scanArbStartState = startState;
    hciBleScanArb.scanArbCurrentState = hciBleScanArb.scanArbStartState;
    hciBleScanArb.scanArbTargetState = targetState;
    HciCmdArbLockQueue(HCI_CMD_SPL_ARBITRATION_LOGIC, &hciCmdArbData);
    handleEventsScanArbStateMachine(BLE_SCAN_SM_STARTED_EVT);
}

/*! Public Functions */
void HciBleScanArbInit(void)
{
    hciBleScanArb.scanArbCurrentState = STATE_IDLE;
    hciBleScanArb.scanArbStartState = STATE_IDLE;
    hciBleScanArb.scanEnabledByMicroStack = FALSE;
    hciBleScanArb.scanEnabledByPrimaryStack = FALSE;
    hciBleScanArb.scanParamsConfiguredByMicroStack = FALSE;
    hciBleScanArb.scanParamsConfiguredByPrimaryStack = FALSE;
    hciBleScanArb.scanArbTargetState = STATE_IDLE;
    hciBleScanArb.cmdPayload = NULL;
    hciBleScanArb.cmdPayloadLen = 0;
    HciCmdArbUnlockQueue(HCI_CMD_SPL_ARBITRATION_LOGIC, &hciCmdArbData);

}

/*! Public Functions */
void HciBleScanArbDeinit(void)
{
    hciBleScanArb.scanArbCurrentState = STATE_IDLE;
    hciBleScanArb.scanArbStartState = STATE_IDLE;
    hciBleScanArb.scanEnabledByMicroStack = FALSE;
    hciBleScanArb.scanEnabledByPrimaryStack = FALSE;
    hciBleScanArb.scanParamsConfiguredByMicroStack = FALSE;
    hciBleScanArb.scanParamsConfiguredByPrimaryStack = FALSE;
    hciBleScanArb.scanArbTargetState = STATE_IDLE;
    if(hciBleScanArb.cmdPayload)
        CsrPmemFree(hciBleScanArb.cmdPayload);
    hciBleScanArb.cmdPayload = NULL;
    hciBleScanArb.cmdPayloadLen = 0;
    HciCmdArbUnlockQueue(HCI_CMD_SPL_ARBITRATION_LOGIC, &hciCmdArbData);
}

bool HciBleScanArbitration(uint8 sender, uint8 *payload, uint16 len)
{
    bool result = FALSE;
    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "HciBleScanArbitration- sender= 0x%x", sender));
    if(IsItBleScanParamCommand(payload, len))
    {
        if(sender == HCI_CMD_SENDER_PRIMARY_STACK)
        {
            if(IsScanAlreadyEnabledByMicroStack())
            {
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Scan param set by Micro stack"));
                /* Disable the scan and then go ahead with the commands from Primary stack. */
                HciBleScanArbSaveTheScanParamSetCommand(sender, payload, len);
                triggerScanStateMachine(hciBleScanArb.scanArbCurrentState, STATE_SCAN_ENABLE_DONE);
                result = TRUE;
            }
            else
            {
                /* Go ahead with the scan commands. */
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Proceed with scan param cmd"));
                hciBleScanArb.scanParamsConfiguredByPrimaryStack = TRUE;
            }
        }
        else if(sender == HCI_CMD_SENDER_MICRO_STACK)
        {
            if(HasPrimaryStackAlreadyConfiguredScanParams())
            {
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Scan param set by Micro stack"));
                /* Simulate the Scan param response and don't send the command down. Free memory of the command*/
                hciBleScanSendSimulatedScanParamResponse();
                CsrPmemFree(payload);
                result = TRUE;
            }
            else
            {
                /* Go ahead with the command */
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Proceed with scan param cmd"));
                hciBleScanArb.scanParamsConfiguredByMicroStack = TRUE;
            }
        }
    }
    else if(IsItBleScanEnableCommand(payload, len))
    {
        if(sender == HCI_CMD_SENDER_PRIMARY_STACK)
        {
            if(IsScanAlreadyEnabledByMicroStack())
            {
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Scan enabled by Micro stack, proceeding"));
                /* Go ahead with the scan commands. */
                hciBleScanArb.scanEnabledByPrimaryStack = TRUE;
            }
            else
            {
                /* Go ahead with the scan commands. */
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Scan not enabled, proceeding anyway"));
                hciBleScanArb.scanEnabledByPrimaryStack = TRUE;
            }
        }
        else if(sender == HCI_CMD_SENDER_MICRO_STACK)
        {
            if(IsScanAlreadyEnabledByPrimaryStack())
            {
                /* Go ahead with the scan commands. */
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Scan already enabled by primary stack."));
                hciBleScanArb.scanEnabledByMicroStack = TRUE;
                hciBleScanSendSimulatedScanEnableResponse();
                CsrPmemFree(payload);
                result = TRUE;
            }
            else
            {
                /* Go ahead with the scan commands. */
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Scan not enabled, proceeding anyway"));
                hciBleScanArb.scanEnabledByMicroStack = TRUE;
            }
        }
    }
    else if(IsItBleScanDisableCommand(payload, len))
    {
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Its BLE scan disable cmd "));
        if(sender == HCI_CMD_SENDER_PRIMARY_STACK)
        {
            if(IsScanAlreadyEnabledByMicroStack())
            {
                /* We don't want to disable scan in controller in this scenario. */
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "scan already enabled by Microstack "));
                hciBleScanSendSimulatedScanEnableResponse();
                hciBleScanArb.scanEnabledByPrimaryStack = FALSE;
                CsrPmemFree(payload);
                result = TRUE;
            }
            else
            {
                /* Go ahead with the scan commands. */
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Go ahead with the cmd "));
                hciBleScanArb.scanEnabledByPrimaryStack = FALSE;
            }
        }
        else if(sender == HCI_CMD_SENDER_MICRO_STACK)
        {
            if(IsScanAlreadyEnabledByPrimaryStack())
            {
                /* We don't want to disable scan in controller in this scenario. */
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "scan already enabled by Primary stack "));
                hciBleScanSendSimulatedScanEnableResponse();
                hciBleScanArb.scanEnabledByMicroStack = FALSE;
                CsrPmemFree(payload);
                result = TRUE;
            }
            else
            {
                /* Go ahead with the scan commands. */
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Go ahead with the cmd "));
                hciBleScanArb.scanEnabledByMicroStack = FALSE;
            }
        }
    }
    return result;
}

bool IsHciEventMeantForSpecialArbitrationLogic(CsrHciEventInd *prim)
{
    bool eventConsumed = FALSE;
    if(IsHciCmdArbQueueLocked(HCI_CMD_SPL_ARBITRATION_LOGIC, &hciCmdArbData) && prim &&
       (prim->payload[0] == HCI_EV_COMMAND_COMPLETE))
    {
        if(((((uint16) prim->payload[4]) << 8) | ((uint16) prim->payload[3])) == HCI_ULP_EXT_SCAN_SET_PARAMS)
        {
            eventConsumed = TRUE;
            if(hciBleScanArb.scanArbCurrentState == STATE_SCAN_PARAM_SET_IN_PROGRESS)
            {
                if(prim->payload[5] == HCI_SUCCESS)
                {
                    handleEventsScanArbStateMachine(BLE_SCAN_PARAMS_CHANGED_EVT);
                }
                else
                {
                    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Scan param set failed"));
                }
            }
            else
            {
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "scan param evt in wrong state"));
            }
        }
        else if(((((uint16) prim->payload[4]) << 8) | ((uint16) prim->payload[3])) == HCI_ULP_EXT_SCAN_ENABLE)
        {
            eventConsumed = TRUE;
            if(prim->payload[5] == HCI_SUCCESS)
            {
                if(hciBleScanArb.scanArbCurrentState == STATE_SCAN_DISABLE_IN_PROGRESS)
                {
                    handleEventsScanArbStateMachine(BLE_SCAN_DISABLE_RECEIVED_EVT);
                }
                else if(hciBleScanArb.scanArbCurrentState == STATE_SCAN_ENABLE_IN_PROGRESS)
                {
                    handleEventsScanArbStateMachine(BLE_SCAN_ENABLE_RECEIVED_EVT);
                }
                else
                {
                    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Scan enable/disable evt came in wrong state"));
                }
            }
            else
            {
                CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Scan enable failed"));
            }
        }
    }
    return eventConsumed;
}



