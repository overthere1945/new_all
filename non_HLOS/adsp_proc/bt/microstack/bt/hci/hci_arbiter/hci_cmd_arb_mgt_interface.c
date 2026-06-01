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
/*! Public Data */

/* Global states */
#ifdef CSR_BT_GLOBAL_INSTANCE
hciCmdArbInstanceData_t  hciCmdArbData;
#define hciCmdArbGetInstanceDataPtr() (&hciCmdArbData)
#endif

#define HCI_CMD_ARB_STATE_NOT_READY                   ((HciCmdArbGlobal) 0)
#define HCI_CMD_ARB_STATE_IDLE                        ((HciCmdArbGlobal) 1)

#define HCI_CMD_ARB_QUEUE_UNLOCK           ((uint16_t) 0xFFFF)

#define HCI_CMD_ARB_BLE_SCAN_ARB           ((uint16_t) 0xFFFE)



#define hciCmdArbGeneralException(eventClass, primType, state, message)   \
    CsrGeneralException(hciCmdArbLto, 0 , (eventClass), (CsrUint16) (primType), (CsrUint16) (state), (message))

#define HCI_CMD_ARB_LTSO_GEN                    0
#define HCI_CMD_ARB_LTSO_STATE                  1
#define HCI_CMD_ARB_LTSO_QUEUE                  2

#define HCI_CMD_ARB_STATE_CHANGE(_var, _state)                            \
    do                                                                  \
    {                                                                   \
        CSR_LOG_TEXT_DEBUG((hciCmdArbLto,                                  \
                           HCI_CMD_ARB_LTSO_STATE,                        \
                           #_var" state: %d -> %d",                     \
                           (_var),                                      \
                           (_state)));                                  \
        (_var) = (_state);                                              \
    } while (0)


typedef void (* SignalHandlerType)(uint8 sender, hciCmdArbInstanceData_t * taskData);


/*! Private function  prototypes */
static void hci_cmd_arb_iface_msg_handler(hciCmdArbInstanceData_t *hciCmdArbData);
static void hci_cmd_arb_unhandled_prim(const HCI_CMD_ARB_PRIM_T *const prim);
static void hciCmdArbInitInstData(hciCmdArbInstanceData_t *hciCmdArbData);
void hciCmdArbRestoreQueueHandler(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData);
void hciCmdArbCmdHandler(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData);
void hciCmdArbVSCmdHandler(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData);
void hciPeriCmdArbCmdHandler(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData);


/*! Private data types */
static const SignalHandlerType hci_cmd_arb_function_ptr[] =
{
    hciCmdArbRestoreQueueHandler,               /* HCI_CMD_ARB_HOUSE_CLEANING_REQ */
    hciCmdArbCmdHandler,        /* HCI_CMD_ARB_CMD_REQ */
    hciCmdArbVSCmdHandler,        /* HCI_CMD_ARB_VS_CMD_REQ */
    hciPeriCmdArbCmdHandler,     /* HCI_PERI_CMD_ARB_CMD_REQ */

};

#ifdef CONFIG_BT_TESTER
CsrUint8 hciReset;
#endif


/*! Private Functions */

/*! \brief This is a generic function to invoke when an unhandled message is 
           recieved at the interface.
    \param p_uprim Pointer to unhandled command primitive.
*/
static void hci_cmd_arb_unhandled_prim(const HCI_CMD_ARB_PRIM_T *const p_uprim)
{
    QBL_UNUSED(p_uprim);
    BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
}

static void handleCmdReqCommon(hciCmdArbInstanceData_t *hciCmdArbData, uint8 cmdSender, CsrUint16 id, bool isMicroStack, uint8 *payload)
{
    if (IsHciCmdArbQueueLocked(cmdSender, hciCmdArbData))
    {
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "CMD ARB queue is locked"));
        if (isMicroStack)
        {
            CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "queued in Micro stack queue"));
            CsrMessageQueuePush(&hciCmdArbData->saveMicroStackQueue, HCI_CMD_ARB_PRIM, hciCmdArbData->recvMsgP);
        }
        else
        {
            CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "queued in Primary stack queue"));
            CsrMessageQueuePush(&hciCmdArbData->savePrimaryStackQueue, HCI_CMD_ARB_PRIM, hciCmdArbData->recvMsgP);
        }
        hciCmdArbData->recvMsgP = NULL;
    }
    else
    {
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "CMD ARB queue is not locked"));
        if ((id <= HCI_CMD_ARB_PRIM_DOWNSTREAM_HIGHEST) &&
            (hci_cmd_arb_function_ptr[(CsrUint16)(id - HCI_CMD_ARB_PRIM_DOWNSTREAM_LOWEST)] != NULL))
        {
            HciCmdArbLockQueue(cmdSender, hciCmdArbData);
            if (isMicroStack)
            {
                hciCmdArbData->lockMsgMicroStackCmdOpcode = (hci_op_code_t)(((uint16_t)payload[1] << 8) | ((uint16_t)payload[0]));
            }
            /* Use jump table */
            (hci_cmd_arb_function_ptr[(CsrUint16) (id - HCI_CMD_ARB_PRIM_DOWNSTREAM_LOWEST)])(cmdSender, hciCmdArbData);
        }
    }
}

static void hci_cmd_arb_iface_msg_handler(hciCmdArbInstanceData_t *hciCmdArbData)
{
    CsrUint16 id = *(CsrPrim*)hciCmdArbData->recvMsgP;
    HciArbCmdReq *cmdReq;
    HciArbVsCmdReq *vsCmdReq;
    HciArbHouseCleaningReq *houseCleaningReq;
    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "hci_cmd_arb_iface_msg_handler Msg Id 0x%04x", id));

    switch (id)
    {
        case HCI_CMD_ARB_HOUSE_CLEANING_REQ:
        {
            houseCleaningReq = (HciArbHouseCleaningReq *)hciCmdArbData->recvMsgP;
            hciCmdArbRestoreQueueHandler(houseCleaningReq->reqSender, hciCmdArbData);
            break;
        }

        case HCI_CMD_ARB_CMD_REQ:
        case HCI_PERI_CMD_ARB_CMD_REQ:
        {
            cmdReq = (HciArbCmdReq *)hciCmdArbData->recvMsgP;
            handleCmdReqCommon(hciCmdArbData, cmdReq->cmdSender, id, cmdReq->cmdSender == HCI_CMD_SENDER_MICRO_STACK, cmdReq->payload);
            break;
        }

        case HCI_CMD_ARB_VS_CMD_REQ:
        {
            vsCmdReq = (HciArbVsCmdReq *)hciCmdArbData->recvMsgP;
            handleCmdReqCommon(hciCmdArbData, vsCmdReq->cmdSender, id, vsCmdReq->cmdSender == HCI_CMD_SENDER_MICRO_STACK, (uint8 *)vsCmdReq->data);
            break;
        }

        default:
        {
            hci_cmd_arb_unhandled_prim((HCI_CMD_ARB_PRIM_T *)hciCmdArbData->recvMsgP);
            break;
        }
    }
}

static void hciCmdArbInitInstData(hciCmdArbInstanceData_t *hciCmdArbData)
{
    hciCmdArbData->lockMsgMicroStack = HCI_CMD_ARB_QUEUE_UNLOCK;
    hciCmdArbData->lockMsgPrimaryStack = HCI_CMD_ARB_QUEUE_UNLOCK;
    hciCmdArbData->lockMsgSplArbLogic = HCI_CMD_ARB_QUEUE_UNLOCK;
    hciCmdArbData->lockMsgPrimaryStackCount = 0;
    HCI_CMD_ARB_STATE_CHANGE(hciCmdArbData->globalState, HCI_CMD_ARB_STATE_IDLE);
    HciBleScanArbInit();
}





void hciCmdArbRestoreQueueHandler(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData)
{
    CsrUint16          eventClass;
    void *              msg;

    HciCmdArbUnlockQueue(sender, hciCmdArbData);

    while(CsrMessageQueuePop(&hciCmdArbData->savePrimaryStackQueue, &eventClass, &msg))
    {
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Primary stack msg poped"));
        SynergyMessageFree(HCI_CMD_ARB_PRIM, hciCmdArbData->recvMsgP);
        hciCmdArbData->recvMsgP = msg;
        hci_cmd_arb_iface_msg_handler(hciCmdArbData);
        pfree(hciCmdArbData->recvMsgP);
        hciCmdArbData->recvMsgP = NULL;
    }

    if(!IsHciCmdArbQueueLocked(HCI_CMD_SENDER_MICRO_STACK, hciCmdArbData))
    {
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "check micro stack queue for msg"));
        if(CsrMessageQueuePop(&hciCmdArbData->saveMicroStackQueue, &eventClass, &msg))
        {
            CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Micro stack msg poped"));
            SynergyMessageFree(HCI_CMD_ARB_PRIM, hciCmdArbData->recvMsgP);
            hciCmdArbData->recvMsgP = msg;
            hci_cmd_arb_iface_msg_handler(hciCmdArbData);
            pfree(hciCmdArbData->recvMsgP);
            hciCmdArbData->recvMsgP = NULL;
        }
    }
}



void hciCmdArbCmdHandler(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData)
{
    HciArbCmdReq *prim = hciCmdArbData->recvMsgP;
    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Send HCI cmd. sender 0x%04x", sender));
    /* BLE Scan arbitration */
    if(HciBleScanArbitration(sender, prim->payload, prim->len))
    {
        return;
    }

    CsrHciCommandReqSend(prim->len, prim->payload);
}

void hciCmdArbVSCmdHandler(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData)
{
    HciArbVsCmdReq *prim = hciCmdArbData->recvMsgP;
    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Send HCI VS cmd. sender 0x%04x", sender));
    CsrHciVendorSpecificCommandReqSend(TRANSPORT_CHANNEL_QC_HCIVS, prim->data);
}

void hciPeriCmdArbCmdHandler(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData)
{
    HciPeriArbCmdReq *prim = hciCmdArbData->recvMsgP;
    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Send PERI cmd. sender 0x%04x", sender));
    CsrPeriCommandReqSend(prim->len, prim->payload);
}

extern HciArbCmdReq *HciArbCommandReq_struct(uint8 sender, CsrUint16 payloadLength, CsrUint8 *payload)
{
    HciArbCmdReq *prim = pnew(HciArbCmdReq);

    prim->type = HCI_CMD_ARB_CMD_REQ;
    prim->cmdSender = sender;
    prim->len = payloadLength;
    prim->payload = payload;

    return prim;
}

extern HciArbHouseCleaningReq *HciArbHouseCleaningReq_struct(uint8 sender)
{
    HciArbHouseCleaningReq *prim = pnew(HciArbHouseCleaningReq);

    prim->type = HCI_CMD_ARB_HOUSE_CLEANING_REQ;
    prim->reqSender = sender;
    return prim;
}

extern HciArbVsCmdReq *HciArbVsCommandReq_struct(uint8 sender, void *data)
{
    HciArbVsCmdReq *prim = pnew(HciArbVsCmdReq);

    prim->type = HCI_CMD_ARB_VS_CMD_REQ;
    prim->cmdSender = sender;
    prim->data = data;

    return prim;
}

extern HciPeriArbCmdReq *HciPeriArbCommandReq_struct(uint8 sender, CsrUint16 payloadLength, CsrUint8 *payload)
{
    HciPeriArbCmdReq *prim = pnew(HciPeriArbCmdReq);

    prim->type = HCI_PERI_CMD_ARB_CMD_REQ;
    prim->cmdSender = sender;
    prim->len = payloadLength;
    prim->payload = payload;

    return prim;
}


/*! \brief Initialisation function
 
    \param gash Unused.
*/
void HciCmdArbInit(void **gash)
{
    hciCmdArbInstanceData_t  *hciCmdArbDataPtr;
#ifdef CSR_BT_GLOBAL_INSTANCE
    *gash = &hciCmdArbData;
#else
    *gash = (void *) CsrPmemZalloc(sizeof(hciCmdArbInstanceData_t));
#endif
    hciCmdArbDataPtr = (hciCmdArbInstanceData_t *) *gash;

    hciCmdArbInitInstData(hciCmdArbDataPtr);
}


/*! \brief Interface handler for all events.

    \param gash Unused.
*/
void HciCmdArbIfaceHandler(void **gash)
{
    uint16_t m=0;;
    void *message=NULL;
    hciCmdArbInstanceData_t    *hciCmdArbData;

    hciCmdArbData = (hciCmdArbInstanceData_t *) (*gash);

    get_message(HCI_CMD_ARB_IFACEQUEUE, &m , &message);
    hciCmdArbData->recvMsgP = message;

    switch (m)
    {
        case HCI_CMD_ARB_PRIM:
            hci_cmd_arb_iface_msg_handler(hciCmdArbData);
            break;

        default:
            BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    pfree(hciCmdArbData->recvMsgP);
    hciCmdArbData->recvMsgP = NULL;

}

extern void hci_cmd_arb_deinit(void **gash)
{
    CSR_UNUSED(gash);
    hciCmdArbData.lockMsgMicroStack = HCI_CMD_ARB_QUEUE_UNLOCK;
    hciCmdArbData.lockMsgPrimaryStack = HCI_CMD_ARB_QUEUE_UNLOCK;
    hciCmdArbData.lockMsgSplArbLogic = HCI_CMD_ARB_QUEUE_UNLOCK;
    HCI_CMD_ARB_STATE_CHANGE(hciCmdArbData.globalState, HCI_CMD_ARB_STATE_NOT_READY);
    HciBleScanArbDeinit();
}

bool IsMicroStackWaitingForAnyResponseEvent(void)
{
    if((hciCmdArbData.lockMsgMicroStack == HCI_CMD_ARB_CMD_REQ) ||
        (hciCmdArbData.lockMsgMicroStack == HCI_CMD_ARB_VS_CMD_REQ))
    {
        CSR_LOG_TEXT_INFO((HCI, 0, "Micro stack: waiting for response evt"));
        return TRUE;
    }
    CSR_LOG_TEXT_INFO((HCI, 0, "Micro stack: no evt response wait"));
    return FALSE;
}

bool HasPrimaryStackTakenTheHCICmdArbLock(void)
{
    if(hciCmdArbData.lockMsgPrimaryStack == HCI_CMD_ARB_CMD_REQ)
    {
        return TRUE;
    }
    return FALSE;
}

void HciCmdArbLockQueue(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData)
{
    HciCmdArbPrim         *primPtr;

    primPtr = (CsrPrim *) hciCmdArbData->recvMsgP;

    if(sender == HCI_CMD_SENDER_MICRO_STACK)
    {
        hciCmdArbData->lockMsgMicroStack = *primPtr;
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Micro stack locked Msg Id 0x%04x", hciCmdArbData->lockMsgMicroStack));
    }
    else if(sender == HCI_CMD_SENDER_PRIMARY_STACK)
    {
        hciCmdArbData->lockMsgPrimaryStack = *primPtr;
        hciCmdArbData->lockMsgPrimaryStackCount++;
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Primary stack locked Msg Id 0x%04x lockMsgPrimaryStackCount %d", hciCmdArbData->lockMsgPrimaryStack, hciCmdArbData->lockMsgPrimaryStackCount));
    }
    else
    {
        hciCmdArbData->lockMsgSplArbLogic = HCI_CMD_ARB_BLE_SCAN_ARB;
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Special Arbitration Logic locked Msg Id 0x%04x", hciCmdArbData->lockMsgSplArbLogic));
    }
}

bool IsHciCmdArbQueueLocked(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData)
{
    CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "sender = %d, lockMsgMicroStack= 0x%04x lockMsgPrimaryStack= 0x%04x lockMsgSplArbLogic= 0x%04x", sender, hciCmdArbData->lockMsgMicroStack, hciCmdArbData->lockMsgPrimaryStack, hciCmdArbData->lockMsgSplArbLogic));

    if(sender == HCI_CMD_SENDER_MICRO_STACK)
    {
        if((hciCmdArbData->lockMsgMicroStack != HCI_CMD_ARB_QUEUE_UNLOCK) ||
           (hciCmdArbData->lockMsgPrimaryStack != HCI_CMD_ARB_QUEUE_UNLOCK) ||
           (hciCmdArbData->lockMsgSplArbLogic != HCI_CMD_ARB_QUEUE_UNLOCK))
        {
            CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Micro stack queue locked"));
            return TRUE;
        }
    }
    else if(sender == HCI_CMD_SENDER_PRIMARY_STACK)
    {
        if((hciCmdArbData->lockMsgMicroStack != HCI_CMD_ARB_QUEUE_UNLOCK) ||
           (hciCmdArbData->lockMsgSplArbLogic != HCI_CMD_ARB_QUEUE_UNLOCK))
        {
            CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Primary stack queue locked lockMsgMicroStack 0x%04x lockMsgSplArbLogic= 0x%04x", hciCmdArbData->lockMsgMicroStack, hciCmdArbData->lockMsgSplArbLogic));
            return TRUE;
        }
    }
    else if(sender == HCI_CMD_SPL_ARBITRATION_LOGIC)
    {
        if(hciCmdArbData->lockMsgSplArbLogic != HCI_CMD_ARB_QUEUE_UNLOCK)
        {
            CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Primary stack queue locked lockMsgMicroStack 0x%04x lockMsgSplArbLogic= 0x%04x", hciCmdArbData->lockMsgMicroStack, hciCmdArbData->lockMsgSplArbLogic));
            return TRUE;
        }
    }
    else
    {
        return TRUE;
    }
    return FALSE;
}

void HciCmdArbUnlockQueue(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData)
{
    if(sender == HCI_CMD_SENDER_MICRO_STACK)
    {
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Micro stack Unlocked Msg Id 0x%04x", hciCmdArbData->lockMsgMicroStack));
        hciCmdArbData->lockMsgMicroStack = HCI_CMD_ARB_QUEUE_UNLOCK;
    }
    else if(sender == HCI_CMD_SENDER_PRIMARY_STACK)
    {
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Primary Stack Unlocked Msg Id 0x%04x", hciCmdArbData->lockMsgPrimaryStack));
        hciCmdArbData->lockMsgPrimaryStackCount--;
        if(hciCmdArbData->lockMsgPrimaryStackCount == 0)
        {
            hciCmdArbData->lockMsgPrimaryStack = HCI_CMD_ARB_QUEUE_UNLOCK;
        }
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "lockMsgPrimaryStackCount %d", hciCmdArbData->lockMsgPrimaryStackCount));
    }
    else
    {
        CSR_LOG_TEXT_INFO((hciCmdArbLto, HCI_CMD_ARB_LTSO_QUEUE, "Special Arbitration logic Unlocked Msg Id 0x%04x", hciCmdArbData->lockMsgSplArbLogic));
        hciCmdArbData->lockMsgSplArbLogic = HCI_CMD_ARB_QUEUE_UNLOCK;
    }
}

#ifdef CONFIG_BT_TESTER
bool HciArbiterSkipTestCmd(uint16 length, uint8 *payload)
{
    uint8 opcode1;
    uint8 opcode2;
    bool sendEvent = FALSE;

    if (length < 4 || payload == NULL)
    {
        return FALSE;
    }

    opcode1 = payload[1];
    opcode2 = payload[2];

    /* Check for HCI Reset command */
    if (opcode1 == 0x03 && opcode2 == 0x0C && payload[3] == 0x00)
    {
        hciReset++;
        if (hciReset > 1)
        {
            CSR_LOG_TEXT_INFO((HCI, 0, "Simulated HCI reset Event being routed to primary stack"));
            sendEvent = TRUE;
        }
    }

    /* Check for VS command 0xFC08 */
    else if (opcode1 == 0x08 && opcode2 == 0xFC && payload[3] == 0x00)
    {
        CSR_LOG_TEXT_INFO((HCI, 0, "VS Event FC 08 being routed to primary stack"));
        sendEvent = TRUE;
    }

    if (sendEvent)
    {
        uint8 *payloadEvent = CsrPmemAlloc(7);

        if (payloadEvent != NULL)
        {
            payloadEvent[0] = HCI_ARB_HCI_EVT_TYPE;
            payloadEvent[1] = 0x0E;
            payloadEvent[2] = 0x04;
            payloadEvent[3] = 0x01;
            payloadEvent[4] = opcode1;
            payloadEvent[5] = opcode2;
            payloadEvent[6] = payload[3];

            HciArbSendHciMsgToPrimaryStack(7, payloadEvent);
        }

        CsrPmemFree(payload);
        return TRUE;
    }

    return FALSE;
}

#endif

