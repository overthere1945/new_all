/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"
#ifdef GATT_SEQUENCING

#include "hci_att_arb.h"
#include "csr_hci_lib.h"
#include "csr_transport.h"
#include "hci_ble_scan_arb.h"
#include "att_private.h"
#include "l2cap_chme.h"


/*     L2CAP_CHCB_T *chcb = CHME_GetConHandle(*handle_plus_flags & HCI_CONNECTION_HANDLE_MASK); */
/*! Public Data */

/* Global states */
#ifdef CSR_BT_GLOBAL_INSTANCE
hciAttArbInstanceData_t  hciAttArbData;
#define hciAttArbGetInstanceDataPtr() (&hciAttArbData)
#endif

#define SHALL_LOCK_MICRO_STACK_MSGS(chcb)       (((chcb->hciAttArbData.attReqMsgStatePrimaryStack == ATT_REQ_SENT) && (chcb->hciAttArbData.attReqMsgStateMicroStack == ATT_REQ_RECEIVED)) ||  \
                                                  ((chcb->hciAttArbData.attIndMsgStatePrimaryStack == ATT_INDICATION_SENT) && (chcb->hciAttArbData.attIndMsgStateMicroStack == ATT_INDICATION_RECEIVED)))

#define SHALL_LOCK_PRIMARY_STACK_MSGS(chcb)     (((chcb->hciAttArbData.attReqMsgStateMicroStack == ATT_REQ_SENT) && (chcb->hciAttArbData.attReqMsgStatePrimaryStack == ATT_REQ_RECEIVED)) ||     \
                                                  ((chcb->hciAttArbData.attIndMsgStateMicroStack == ATT_INDICATION_SENT) && (chcb->hciAttArbData.attIndMsgStatePrimaryStack == ATT_INDICATION_RECEIVED)))

#define IsHciAttArbQueueUnLocked(sender, hciAttArbData, chcb)    (!IsHciAttArbQueueLocked(sender, hciAttArbData, chcb))


#define hciAttArbGeneralException(eventClass, primType, state, message)   \
    CsrGeneralException(hciAttArbLto, 0 , (eventClass), (CsrUint16) (primType), (CsrUint16) (state), (message))

#define HCI_ATT_ARB_LTSO_GEN                    0
#define HCI_ATT_ARB_LTSO_STATE                  1
#define HCI_ATT_ARB_LTSO_QUEUE                  2


typedef void (* SignalHandlerType)(AttMsgSender sender, hciAttArbInstanceData_t * taskData);


/*! Private function  prototypes */
static void hci_att_arb_iface_msg_handler(hciAttArbInstanceData_t *hciAttArbData);
static void hci_att_arb_unhandled_prim(const HCI_ATT_ARB_PRIM_T *const prim);
void hciAttArbRestoreQueueHandler(AttMsgSender sender, hciAttArbInstanceData_t *hciAttArbData);
void hciAttArbAttMsgHandler(AttMsgSender sender, hciAttArbInstanceData_t *hciAttArbData);


/*! Private data types */
static const SignalHandlerType hci_att_arb_function_ptr[] =
{
    hciAttArbRestoreQueueHandler,               /* HCI_ATT_ARB_HOUSE_CLEANING_REQ */
    hciAttArbAttMsgHandler,        /* HCI_ATT_ARB_ATT_MSG */
};


/*! Private Functions */

/*! \brief This is a generic function to invoke when an unhandled message is 
           recieved at the interface.
    \param p_uprim Pointer to unhandled msg primitive.
*/
static void hci_att_arb_unhandled_prim(const HCI_ATT_ARB_PRIM_T *const p_uprim)
{
    QBL_UNUSED(p_uprim);
    BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
}


static CsrMessageQueueType **AttArbFindTheQueueForGivenGattConn(AttMsgSender attSender, L2CAP_CHCB_T *chcb)
{
    if (attSender == HCI_ATT_SENDER_MICRO_STACK)
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "queued in Micro stack queue = 0x%u", (&chcb->hciAttArbData.saveMicroStackQueue)));
        return (&chcb->hciAttArbData.saveMicroStackQueue);
    }
    else// if(attSender == HCI_ATT_SENDER_PRIMARY_STACK)
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "queued in Primary stack queue = 0x%u", (&chcb->hciAttArbData.savePrimaryStackQueue)));
        return (&chcb->hciAttArbData.savePrimaryStackQueue);
    }
}

static void handleAttReqCommon(hciAttArbInstanceData_t *hciAttArbData, AttMsgSender attSender, CsrUint16 id)
{
    HciAttArbAttMsgReq *attReq;
    attReq = (HciAttArbAttMsgReq *)hciAttArbData->recvMsgP;

    if (IsHciAttArbQueueLocked(attSender, hciAttArbData, attReq->chcb))
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "ATT ARB queue is locked"));
        CsrMessageQueuePush(AttArbFindTheQueueForGivenGattConn(attSender, attReq->chcb), HCI_ATT_ARB_PRIM, hciAttArbData->recvMsgP);
        hciAttArbData->recvMsgP = NULL;
    }
    else
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "ATT ARB queue is not locked"));
        if ((id <= HCI_ATT_ARB_PRIM_DOWNSTREAM_HIGHEST) &&
            (hci_att_arb_function_ptr[(CsrUint16)(id - HCI_ATT_ARB_PRIM_DOWNSTREAM_LOWEST)] != NULL))
        {
            if(HciAttArbLockQueue(attSender, id, hciAttArbData, ATT_MSG_RECEIVED))
            {
                CsrMessageQueuePush(AttArbFindTheQueueForGivenGattConn(attSender, attReq->chcb), HCI_ATT_ARB_PRIM, hciAttArbData->recvMsgP);
                hciAttArbData->recvMsgP = NULL;
            }
            else
            {
                /* Use jump table */
                (hci_att_arb_function_ptr[(CsrUint16) (id - HCI_ATT_ARB_PRIM_DOWNSTREAM_LOWEST)])(attSender, hciAttArbData);
            }
        }
    }
}

static void hci_att_arb_iface_msg_handler(hciAttArbInstanceData_t *hciAttArbData)
{
    CsrUint16 id = *(CsrPrim*)hciAttArbData->recvMsgP;
    HciAttArbAttMsgReq *attReq;
    HciAttArbHouseCleaningReq *houseCleaningReq;
    CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Msg Id 0x%04x", id));

    switch (id)
    {
        case HCI_ATT_ARB_HOUSE_CLEANING_REQ:
        {
            houseCleaningReq = (HciAttArbHouseCleaningReq *)hciAttArbData->recvMsgP;
            hciAttArbRestoreQueueHandler(houseCleaningReq->reqSender, hciAttArbData);
            break;
        }

        case HCI_ATT_ARB_ATT_MSG:
        {
            attReq = (HciAttArbAttMsgReq *)hciAttArbData->recvMsgP;
            handleAttReqCommon(hciAttArbData, attReq->attSender, id);
            break;
        }

        default:
        {
            hci_att_arb_unhandled_prim((HCI_ATT_ARB_PRIM_T *)hciAttArbData->recvMsgP);
            break;
        }
    }
}

void hciAttArbRestoreQueueHandler(AttMsgSender sender, hciAttArbInstanceData_t *hciAttArbData)
{
    CsrUint16          eventClass;
    void *              msg;
    L2CAP_CHCB_T        *chcb;
    HciAttArbHouseCleaningReq *prim = (HciAttArbHouseCleaningReq *)hciAttArbData->recvMsgP;
    chcb = prim->chcb;
    CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "hciAttArbRestoreQueueHandler"));

    if(IsHciAttArbQueueUnLocked(HCI_ATT_SENDER_PRIMARY_STACK, hciAttArbData, chcb))
    {
        
        while(CsrMessageQueuePop(&(chcb->hciAttArbData.savePrimaryStackQueue) , &eventClass, &msg))
        {
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Primary stack msg poped"));
            SynergyMessageFree(HCI_ATT_ARB_PRIM, hciAttArbData->recvMsgP);
            hciAttArbData->recvMsgP = msg;
            hci_att_arb_iface_msg_handler(hciAttArbData);
            pfree(hciAttArbData->recvMsgP);
            hciAttArbData->recvMsgP = NULL;
        }
    }

    if(IsHciAttArbQueueUnLocked(HCI_ATT_SENDER_MICRO_STACK, hciAttArbData, prim->chcb))
    {
        while(CsrMessageQueuePop(&(chcb->hciAttArbData.saveMicroStackQueue), &eventClass, &msg))
        {
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Micro stack msg poped"));
            SynergyMessageFree(HCI_ATT_ARB_PRIM, hciAttArbData->recvMsgP);
            hciAttArbData->recvMsgP = msg;
            hci_att_arb_iface_msg_handler(hciAttArbData);
            pfree(hciAttArbData->recvMsgP);
            hciAttArbData->recvMsgP = NULL;
        }
    }
}


void hciAttArbAttMsgHandler(AttMsgSender sender, hciAttArbInstanceData_t *hciAttArbData)
{
    HciAttArbAttMsgReq *prim = (HciAttArbAttMsgReq *)hciAttArbData->recvMsgP;
    TXQUEUE_T *queue;
    CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "hciAttArbAttMsgHandler. sender 0x%04x", sender));
    queue = &((prim->chcb)->queue);
    HciAttArbLockQueue(sender, HCI_ATT_ARB_ATT_MSG, hciAttArbData, ATT_MSG_SENT);
    CH_DataTxAppend(queue, prim->txqe, prim->priority);
    CH_DataSendQueued(prim->chcb, queue, prim->priority, FALSE);
}

bool isMsgAttReqMsg(AttMsgSender sender, HciAttArbAttMsgReq *prim)
{
    uint8_t *map;
    uint8_t op;

    if(prim->txqe && (prim->txqe)->mblk)
    {
        map = mblk_map((prim->txqe)->mblk, (sender == HCI_ATT_SENDER_PRIMARY_STACK? 9 : 0), 1);
        op = (UINT8_R(map, 0)) & ATT_OP_MASK;
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "isMsgAttReqMsg. op 0x%04x", op));
        mblk_unmap((prim->txqe)->mblk, map);

    }
    else
    {
        return FALSE;
    }

    return (ATT_IS_SERVER_OPERATION(op) &&
            op != ATT_OP_HANDLE_VALUE_CFM &&
            op != ATT_OP_HANDLE_VALUE_IND &&
            !ATT_IS_OP_CMD(op));
}

bool isMsgAttIndicationMsg(AttMsgSender sender, HciAttArbAttMsgReq *prim)
{
    uint8_t *map;
    uint8_t op;

    if(prim->txqe && (prim->txqe)->mblk)
    {
        map = mblk_map((prim->txqe)->mblk, (sender == HCI_ATT_SENDER_PRIMARY_STACK ? 9 : 0), 1);
        op = (UINT8_R(map, 0)) & ATT_OP_MASK;
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "isMsgAttIndicationMsg. op 0x%04x", op));
        mblk_unmap((prim->txqe)->mblk, map);

    }
    else
    {
        return FALSE;
    }

    return (op == ATT_OP_HANDLE_VALUE_IND);
}


extern HciAttArbAttMsgReq *HciAttArbAttMsg_struct(AttMsgSender sender, 
                                            TXQE_T *txqe,
                                            L2CAP_CHCB_T *chcb,
                                            uint8_t priority,
                                            bool_t fromNhcp)
{
    HciAttArbAttMsgReq *prim = pnew(HciAttArbAttMsgReq);

    prim->type = HCI_ATT_ARB_ATT_MSG;
    prim->attSender = sender;
    prim->txqe = txqe;
    prim->chcb = chcb;
    prim->priority = priority;
    prim->fromNhcp = fromNhcp;

    return prim;
}

extern HciAttArbHouseCleaningReq *HciAttArbHouseCleaningReq_struct(AttMsgSender sender, L2CAP_CHCB_T *chcb)
{
    HciAttArbHouseCleaningReq *prim = pnew(HciAttArbHouseCleaningReq);

    prim->type = HCI_ATT_ARB_HOUSE_CLEANING_REQ;
    prim->reqSender = sender;
    prim->chcb = chcb;
    return prim;
}


/*! \brief Initialisation function
 
    \param gash Unused.
*/
void HciAttArbInit(void **gash)
{
#ifdef CSR_BT_GLOBAL_INSTANCE
    *gash = &hciAttArbData;
#else
    *gash = (void *) CsrPmemZalloc(sizeof(hciAttArbInstanceData_t));
#endif
}


/*! \brief Interface handler for all events.

    \param gash Unused.
*/
void HciAttArbIfaceHandler(void **gash)
{
    uint16_t m=0;;
    void *message=NULL;
    hciAttArbInstanceData_t    *hciAttArbData;

    hciAttArbData = (hciAttArbInstanceData_t *) (*gash);

    get_message(HCI_ATT_ARB_IFACEQUEUE, &m , &message);
    hciAttArbData->recvMsgP = message;

    switch (m)
    {
        case HCI_ATT_ARB_PRIM:
            hci_att_arb_iface_msg_handler(hciAttArbData);
            break;

        default:
            BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    pfree(hciAttArbData->recvMsgP);
    hciAttArbData->recvMsgP = NULL;

}

extern void hci_att_arb_deinit(void **gash)
{
    CSR_UNUSED(gash);
    CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "hci_att_arb_deinit"));
}


bool HciAttArbLockQueue(AttMsgSender sender, CsrUint16 id, hciAttArbInstanceData_t *hciAttArbData, ATT_MSG_ACTION msgAction)
{
    HciAttArbAttMsgReq *prim;
    L2CAP_CHCB_T        *chcb;

    if(id == HCI_ATT_ARB_ATT_MSG)
    {
        prim = (HciAttArbAttMsgReq *)hciAttArbData->recvMsgP;
        chcb = prim->chcb;

        if(isMsgAttReqMsg(sender, prim))
        {
            if(sender == HCI_ATT_SENDER_MICRO_STACK)
            {
                chcb->hciAttArbData.attReqMsgStateMicroStack = (msgAction == ATT_MSG_RECEIVED ? ATT_REQ_RECEIVED : ATT_REQ_SENT);
                CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "attReqMsgStateMicroStack = %d", chcb->hciAttArbData.attReqMsgStateMicroStack));
            }
            else if(sender == HCI_ATT_SENDER_PRIMARY_STACK)
            {
                chcb->hciAttArbData.attReqMsgStatePrimaryStack = (msgAction == ATT_MSG_RECEIVED ? ATT_REQ_RECEIVED : ATT_REQ_SENT);
                CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "attReqMsgStatePrimaryStack = %d", chcb->hciAttArbData.attReqMsgStatePrimaryStack));
            }
        }
        else if(isMsgAttIndicationMsg(sender, prim))
        {
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Its a ATT ind msg"));
            if(sender == HCI_ATT_SENDER_MICRO_STACK)
            {
                chcb->hciAttArbData.attIndMsgStateMicroStack = (msgAction == ATT_MSG_RECEIVED ? ATT_INDICATION_RECEIVED : ATT_INDICATION_SENT);
                CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "attIndMsgStateMicroStack = %d", chcb->hciAttArbData.attIndMsgStateMicroStack));
            }
            else if(sender == HCI_ATT_SENDER_PRIMARY_STACK)
            {
                chcb->hciAttArbData.attIndMsgStatePrimaryStack = (msgAction == ATT_MSG_RECEIVED ? ATT_INDICATION_RECEIVED : ATT_INDICATION_SENT);
                CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "attIndMsgStatePrimaryStack = %d", chcb->hciAttArbData.attIndMsgStatePrimaryStack));
            }

        }
        else
        {
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Not a ATT req msg"));
        }

        if(SHALL_LOCK_MICRO_STACK_MSGS(chcb))
        {
            chcb->hciAttArbData.lockTakenMicroStack = TRUE;
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "lockTakenMicroStack = %d", chcb->hciAttArbData.lockTakenMicroStack));
        }

        if(SHALL_LOCK_PRIMARY_STACK_MSGS(chcb))
        {
            chcb->hciAttArbData.lockTakenPrimaryStack = TRUE;
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "lockTakenPrimaryStack = %d", chcb->hciAttArbData.lockTakenPrimaryStack));
        }

        if((sender == HCI_ATT_SENDER_MICRO_STACK) &&
           chcb->hciAttArbData.lockTakenMicroStack)
        {
            return TRUE;
        }
        else if((sender == HCI_ATT_SENDER_PRIMARY_STACK) &&
                chcb->hciAttArbData.lockTakenPrimaryStack)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Msg not HCI_ATT_ARB_ATT_MSG"));
        return FALSE;
    }
}

bool IsHciAttArbQueueLocked(AttMsgSender sender, hciAttArbInstanceData_t *hciAttArbData, L2CAP_CHCB_T *chcb)
{
    CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "sender = %d", sender));

    if(sender == HCI_ATT_SENDER_MICRO_STACK)
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "lockTakenMicroStack = %d", chcb->hciAttArbData.lockTakenMicroStack));
        return chcb->hciAttArbData.lockTakenMicroStack;
    }
    else if(sender == HCI_ATT_SENDER_PRIMARY_STACK)
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "lockTakenPrimaryStack = %d", chcb->hciAttArbData.lockTakenPrimaryStack));
        return chcb->hciAttArbData.lockTakenPrimaryStack;
    }
    else
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "ERROR: Wrong sender"));
        return TRUE;
    }
}

void HciAttArbUnlockQueue(AttMsgSender sender,  L2CAP_CHCB_T *chcb, hciAttArbInstanceData_t *hciAttArbData, ATT_ROLE role)
{
    if(sender == HCI_ATT_SENDER_MICRO_STACK)
    {
        if(role == ATT_CLIENT)
        {
            chcb->hciAttArbData.attReqMsgStateMicroStack = ATT_REQ_STATE_IDLE;
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "attReqMsgStateMicroStack = %d", chcb->hciAttArbData.attReqMsgStateMicroStack));
        }
        else
        {
            chcb->hciAttArbData.attIndMsgStateMicroStack = ATT_INDICATION_STATE_IDLE;
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "attIndMsgStateMicroStack = %d", chcb->hciAttArbData.attIndMsgStateMicroStack));
        }

    }
    else if(sender == HCI_ATT_SENDER_PRIMARY_STACK)
    {
        if(role == ATT_CLIENT)
        {
            chcb->hciAttArbData.attReqMsgStatePrimaryStack = ATT_REQ_STATE_IDLE;
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "attReqMsgStatePrimaryStack = %d", chcb->hciAttArbData.attReqMsgStatePrimaryStack));

        }
        else
        {
            chcb->hciAttArbData.attIndMsgStatePrimaryStack = ATT_INDICATION_STATE_IDLE;
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "attIndMsgStatePrimaryStack = %d", chcb->hciAttArbData.attIndMsgStatePrimaryStack));
        }
    }

    if(chcb->hciAttArbData.lockTakenMicroStack && !SHALL_LOCK_MICRO_STACK_MSGS(chcb))
    {
        chcb->hciAttArbData.lockTakenMicroStack = FALSE;
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "lockTakenMicroStack = %d", chcb->hciAttArbData.lockTakenMicroStack));

    }
    if(chcb->hciAttArbData.lockTakenPrimaryStack && !SHALL_LOCK_PRIMARY_STACK_MSGS(chcb))
    {
        chcb->hciAttArbData.lockTakenPrimaryStack = FALSE;
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "lockTakenPrimaryStack = %d", chcb->hciAttArbData.lockTakenPrimaryStack ));

    }
    HciAttArbHouseCleaningSend(sender, chcb);
}

HciArbStack HciAttArbGetDestinationStack(HciArbHciHandle hciHandle, 
                                          HciArbGattRole role, 
                                          HciArbL2capCid cid)
{
    AttMsgSender sender;
    sender = HciAttArbGetDestinationAndUnlockQueue(hciHandle, (role == HCI_ARB_GATT_SERVER ? ATT_SERVER : ATT_CLIENT), cid);
    return (sender == HCI_ATT_SENDER_MICRO_STACK ? HCI_ARB_MICRO_STACK : HCI_ARB_PRIMARY_STACK);
}

AttMsgSender HciAttArbGetDestinationAndUnlockQueue(uint16 aclHandle, ATT_ROLE role, HciArbL2capCid cid)
{
    uint8 dest = HCI_ATT_SENDER_PRIMARY_STACK;
    L2CAP_CHCB_T *chcb = CHME_GetConHandle(aclHandle);
    CSR_UNUSED(cid);
    CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "role = %d", role));

    if(!chcb)
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "chcb is NULL. dest = %d ", dest));
        return HCI_ATT_SENDER_PRIMARY_STACK;
    }
    
    if(role == ATT_SERVER)
    {
        if(chcb->hciAttArbData.attIndMsgStatePrimaryStack == ATT_INDICATION_SENT)
        {
            dest = HCI_ATT_SENDER_PRIMARY_STACK;
        }
        else if(chcb->hciAttArbData.attIndMsgStateMicroStack == ATT_INDICATION_SENT)
        {
            dest = HCI_ATT_SENDER_MICRO_STACK;
        }
        else
        {
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Wrong state"));
        }
    }
    else
    {
        if(chcb->hciAttArbData.attReqMsgStatePrimaryStack == ATT_REQ_SENT)
        {
            dest = HCI_ATT_SENDER_PRIMARY_STACK;
        }
        else if(chcb->hciAttArbData.attReqMsgStateMicroStack == ATT_REQ_SENT)
        {
            dest = HCI_ATT_SENDER_MICRO_STACK;
        }
        else
        {
            CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Wrong state"));
        }
    }
    CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "dest = %d", dest));

    HciAttArbUnlockQueue(dest, chcb, &hciAttArbData, role);
    return dest;
}

void HciAttArbClearOffloadedGATTAclHandle(L2CAP_CHCB_T *chcb)
{
    CsrUint16          eventClass;
    void *              msg;

    while(CsrMessageQueuePop(&chcb->hciAttArbData.savePrimaryStackQueue, &eventClass, &msg))
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Primary stack msg poped"));
        pfree(msg);
        msg = NULL;
    }
    while(CsrMessageQueuePop(&chcb->hciAttArbData.saveMicroStackQueue, &eventClass, &msg))
    {
        CSR_LOG_TEXT_INFO((hciAttArbLto, HCI_ATT_ARB_LTSO_QUEUE, "Primary stack msg poped"));
        pfree(msg);
        msg = NULL;
    }
}
#endif


