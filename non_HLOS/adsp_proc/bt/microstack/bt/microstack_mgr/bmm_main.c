/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#include "csr_synergy.h"
#include "bmm_prim.h"
#include "csr_framework_ext.h"
#include "csr_bt_common.h"
#include "csr_bt_tasks.h"
#include "bluetooth.h"
#include "bmm_private.h"
#include "hci_arbiter_private.h"
#include "csr_tm_bluecore_lib.h"
#include "l2caplib.h"
#include "rfcomm_lib.h"

/******************************************************************************
  BmmReset:
******************************************************************************/
void BmmReset(void)
{
    BMM_INFO("BmmReset");
    CsrMemSet(&gBmm, 0x00, sizeof(gBmm));
    gBmm.state = BMM_STATE_NOT_INITIALIZED;
}

/******************************************************************************
  BmmInit: BMM init function
******************************************************************************/
void BmmInit(void **gash)
{
    CSR_UNUSED(gash);

    BmmReset();
    gBmm.state = BMM_STATE_INIT_IN_PROGRESS;
    CsrTmBlueCoreActivateTransportReqSend(BMM_IFACEQUEUE);
}

/******************************************************************************
  BmmArrivalHandler: Message handler function for BMM messages
******************************************************************************/
void BmmArrivalHandler(void* msg)
{
    BmmPrim *type = (BmmPrim *)msg;

    switch (*type)
    {
        case BMM_SOCKET_RFCOMM_OFFLOAD_REQ:
        {
            BmmSocketRfcommOffloadReqHandler((BmmSocketRfcommOffloadReq *)msg);
            break;
        }

        case BMM_SOCKET_LECOC_OFFLOAD_REQ:
        {
            BmmSocketLecocOffloadReqHandler((BmmSocketLecocOffloadReq *)msg);
            break;
        }

        case BMM_SOCKET_CLOSE_REQ:
        {
            BmmSocketCloseReqHandler((BmmSocketCloseReq *)msg);
            break;
        }

        case BMM_SOCKET_SET_PARAMS_REQ:
        {
            BmmSocketSetParamsReqHandler((BmmSocketSetParamsReq *)msg);
            break;
        }

        case BMM_SOCKET_DATA_REQ:
        {
            BmmSocketDataReqHandler((BmmSocketDataReq *)msg);
            break;
        }

        case BMM_SOCKET_DATA_RSP:
        {
            BmmSocketDataRspHandler((BmmSocketDataRsp *)msg);
            break;
        }

        case BMM_SET_EVENT_MASK_REQ:
        {
            BmmSetEventMaskReqHandler((BmmSetEventMaskReq*)msg);
            break;
        }

        case BMM_PREPARE_STOP_REQ:
        {
            BmmPrepareStopReqHandler((BmmPrepareStopReq*)msg);
            break;
        }

        default:
        {
            break;
        }
    }
    BmmFreePrimitive(msg);
}


/******************************************************************************
  BmmL2capArrivalHandler: Message handler function for L2CAP messages
******************************************************************************/
void BmmL2capArrivalHandler(void* msg)
{
    l2cap_prim_t *type = (l2cap_prim_t *)msg;

    switch (*type)
    {
        case L2CA_DATAWRITE_CFM:
        {
            BmmLecocDataWriteCfmHandler((L2CA_DATAWRITE_CFM_T *)msg);
            break;
        }

        case L2CA_DATAREAD_IND:
        {
            BmmLecocDataReadIndHandler((L2CA_DATAREAD_IND_T **)&msg);
            break;
        }

        case L2CA_ADD_CREDIT_CFM:
        {
            BmmLecocAddCreditCfmHandler((L2CA_ADD_CREDIT_CFM_T *)msg);
            break;
        }

        case L2CA_DISCONNECT_IND:
        {
            BmmLecocDisconnectIndHandler((L2CA_DISCONNECT_IND_T *)msg);
            break;
        }

        default:
        {
            break;
        }
    }
    L2CA_FreePrimitive(msg);
}


/******************************************************************************
  BmmRfcommArrivalHandler: Message handler function for RFCOMM messages
******************************************************************************/
void BmmRfcommArrivalHandler(void* msg)
{
    RFC_PRIM_T *type = (RFC_PRIM_T *)msg;

    switch (*type)
    {
        case RFC_DATAWRITE_CFM:
        {
            BmmRfcommDataWriteCfmHandler((RFC_DATAWRITE_CFM_T *)msg);
            break;
        }

        case RFC_DATAREAD_IND:
        {
            BmmRfcommDataReadIndHandler((RFC_DATAREAD_IND_T **)&msg);
            break;
        }
        
        case RFC_DISCONNECT_IND:
        {
            BmmRfcommDisconnectIndHandler((RFC_DISCONNECT_IND_T *)msg);
            break;
        }

        default:
        {
            break;
        }
    }
    rfc_free_primitive(msg);
}


/******************************************************************************
  BmmTransportManagerHandler: Message handler for Transport Manager messages
******************************************************************************/
void BmmTransportManagerHandler(void* msg)
{
    CsrPrim *type = (CsrPrim *)msg;

    switch (*type)
    {
        case CSR_TM_BLUECORE_ACTIVATE_TRANSPORT_CFM:
        {
            BmmActivateTransportCfmHandler((CsrTmBluecoreActivateTransportCfm *)msg);
            break;
        }

        default:
        {
            break;
        }
    }
}

/******************************************************************************
  BmmHciArbiterArrivalHandler: Message handler function for HCI Arbiter messages
******************************************************************************/
void BmmHciArbiterArrivalHandler(void* msg)
{
    HciArbiterPrim *type = (HciArbiterPrim *)msg;

    switch (*type)
    {
        case HCI_ARBITER_BT_OFF_IND:
        {
            BmmHciArbiterBtOffIndHandler((HciArbiterBtOffInd*)msg);
            break;
        }

        case HCI_ARBITER_BTSS_CRASH_IND:
        {
            BmmHciArbiterBtssCrashIndHandler((HciArbiterBtssCrashInd *)msg);
            break;
        }

        default:
        {
            break;
        }
    }
}

/******************************************************************************
  BmmHandler: Top level message handler function 
******************************************************************************/
void BmmHandler(void **gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CSR_UNUSED(gash);

    CsrSchedMessageGet(&eventClass , &msg);

    switch (eventClass)
    {
        case BMM_PRIM:
        {
            BmmArrivalHandler(msg);
            break;
        }

        case L2CAP_PRIM:
        {
            BmmL2capArrivalHandler(msg);
            msg = NULL;
            break;
        }

        case RFCOMM_PRIM:
        {
            BmmRfcommArrivalHandler(msg);
            msg = NULL;
            break;
        }

        case CSR_TM_BLUECORE_PRIM:
        {
            BmmTransportManagerHandler(msg);
            break;
        }

        case HCI_ARBITER_PRIM:
        {
            BmmHciArbiterArrivalHandler(msg);
            break;
        }

        default:
        {
            break;
        }
    }

    CsrPmemFree(msg);
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************
    This function is called by the scheduler to perform a graceful shutdown
    of a scheduler task.
    This function must:
    1)    empty the input message queue and free any allocated memory in the
        messages.
    2)    free any instance data that may be allocated.
****************************************************************************/
void BmmDeinit(void **gash)
{
#if 0
    uint16 eventClass = 0;
    void* msg = NULL;
    
    while (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case BMM_PRIM:
                BmmFreePrimitive(msg);
                break;

            case L2CAP_PRIM:
                L2CA_FreePrimitive(msg);
                msg = NULL;
                break;

            case RFCOMM_PRIM:
                rfc_free_primitive(msg);
                msg = NULL;
                break;

            case CSR_TM_BLUECORE_PRIM:
                break;

            default:
                break;
        }
        CsrPmemFree(msg);
    }

    *gash = NULL;
#endif
    CSR_UNUSED(gash);
    BmmPropagateEvent(BMM_EVENT_MASK_SUBSCRIBE_DEINITIALIZED);
}
#endif /* ENABLE_SHUTDOWN */




