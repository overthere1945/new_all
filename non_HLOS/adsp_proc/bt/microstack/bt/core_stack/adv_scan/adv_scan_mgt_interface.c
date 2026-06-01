/*******************************************************************************

Copyright (C) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/

#include "dm_private.h"
#include "ext_adv_manager.h"
#include "dm_hci_interface.h"
#include "ext_scan_manager.h"


/*! Public Definitons */



#define MS_ADV_SCAN_LTSO_GEN                    0
#define MS_ADV_SCAN_LTSO_STATE                  1
#define MS_ADV_SCAN_LTSO_QUEUE                  2

/*! Private Function prototypes */
static void msAdvScanInitInstData(msAdvScanInstanceData_t *AdvScanData);
static void msAdvScanUnlockQueue(msAdvScanInstanceData_t *AdvScanData);
static void msAdvScanLockQueue(msAdvScanInstanceData_t *AdvScanData);
static void msExtAdvScanRestoreQueueHandler(msAdvScanInstanceData_t *advScanData);
static void adv_scan_iface_msg_handler(msAdvScanInstanceData_t *advScanData);



/*! Public Data */
#ifdef CSR_BT_GLOBAL_INSTANCE
/* Adv Scan Instance Data */
msAdvScanInstanceData_t  msAdvScanData;

/* Get pointer to Adv Scan Module Instance data */
#define msAdvScanGetInstanceDataPtr() (&msAdvScanData)
#endif

#define MS_ADV_SCAN_QUEUE_UNLOCK           ((adv_scan_prim_t) 0xFFFF)
#define MS_ADV_SCAN_QUEUE_LOCKED(advScanData)      ((advScanData)->lockMsg != MS_ADV_SCAN_QUEUE_UNLOCK)


#define MsAdvScanGeneralException(eventClass, primType, state, message)   \
    CsrGeneralException(MsAdvScanLto, 0 , (eventClass), (CsrUint16) (primType), (CsrUint16) (state), (message))




typedef void (* SignalHandlerType)(msAdvScanInstanceData_t * taskData);


/*! Private Data Types */
static const SignalHandlerType ms_ae_function_ptr[] =
{
    NULL,                                       /* No MSG */
    NULL,                                       /* MS_EXT_ADV_SCAN_HOUSE_CLEANING_REQ */
    MsExtAdvRegisterAppAdvSetReqHandler,        /* MS_EXT_ADV_REGISTER_APP_ADV_SET_REQ */
    MsExtAdvUnregisterAppAdvSetReqHandler,      /* MS_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ */
    MsExtAdvSetParamsReqHandler,                /* MS_EXT_ADV_SET_PARAMS_REQ */
    MsExtAdvSetDataReqHandler,                  /* MS_EXT_ADV_SET_DATA_REQ */
    MsExtAdvSetScanRespDataReqHandler,          /* MS_EXT_ADV_SET_SCAN_RESP_DATA_REQ */
    MsExtAdvReadMaxAdvDataLenReqHandler,        /* MS_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ */
    MsExtAdvSetRandomAddrReqHandler,             /* MS_EXT_ADV_SET_RANDOM_ADDR_REQ */
    MsExtAdvSetsInfoReqHandler,                  /* MS_EXT_ADV_SETS_INFO_REQ */
    MsExtAdvMultiEnableReqHandler,               /* MS_EXT_ADV_MULTI_ENABLE_REQ */

    MsExtScanGlobalParamGetReqHandler,            /* MS_EXT_SCAN_GET_GLOBAL_PARAMS_REQ */
    MsExtScanGlobalParamSetReqHandler,            /* MS_EXT_SCAN_SET_GLOBAL_PARAMS_REQ */
    MsExtScanRegisterScannertReqHandler,         /* MS_EXT_SCAN_REGISTER_SCANNER_REQ */
    MsExtScanUnregisterScannerReqHandler,        /* MS_EXT_SCAN_UNREGISTER_SCANNER_REQ */
    MsExtScanEnableScannerReqHandler,            /* MS_EXT_SCAN_ENABLE_SCANNERS_REQ */
    MsExtScanGetCtrlScanInfoReqHandler,          /* MS_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ */
};


/*! Private Functions */
static void msAdvScanInitInstData(msAdvScanInstanceData_t *AdvScanData)
{
    AdvScanData->lockMsg = MS_ADV_SCAN_QUEUE_UNLOCK;
    MS_ADV_SCAN_STATE_CHANGE(AdvScanData->globalState, MS_ADV_SCAN_STATE_NOT_READY);
}

static void msAdvScanUnlockQueue(msAdvScanInstanceData_t *AdvScanData)
{
    CSR_LOG_TEXT_INFO((MsAdvScanLto, MS_ADV_SCAN_LTSO_QUEUE, "Unlocked Msg Id 0x%04x",AdvScanData->lockMsg));

    AdvScanData->lockMsg = MS_ADV_SCAN_QUEUE_UNLOCK;
}

static void msAdvScanLockQueue(msAdvScanInstanceData_t *AdvScanData)
{
    CsrBtMsPrim         *primPtr;
    primPtr = (CsrPrim *) AdvScanData->recvMsgP;
    AdvScanData->lockMsg = *primPtr;
    CSR_LOG_TEXT_INFO((MsAdvScanLto, MS_ADV_SCAN_LTSO_QUEUE, "Locked Msg Id 0x%04x",AdvScanData->lockMsg));
}

static void msExtAdvScanRestoreQueueHandler(msAdvScanInstanceData_t *advScanData)
{
    CsrUint16          eventClass;
    void *              msg;

    msAdvScanUnlockQueue(advScanData);

    if(CsrMessageQueuePop(&advScanData->saveQueue, &eventClass, &msg))
    {
        SynergyMessageFree(MS_ADV_SCAN_PRIM, advScanData->recvMsgP);
        advScanData->recvMsgP = msg;
        adv_scan_iface_msg_handler(advScanData);
        if(advScanData->recvMsgP)
        {
            MsAdvScanFreeDownstreamMessageContents(advScanData->recvMsgP);
            CsrPmemFree(advScanData->recvMsgP);
            advScanData->recvMsgP = NULL;
        }
    }
}

static void adv_scan_iface_msg_handler(msAdvScanInstanceData_t *advScanData)
{
    CsrUint16 id = *(CsrPrim*)advScanData->recvMsgP;

    if(id == MS_EXT_ADV_SCAN_HOUSE_CLEANING_REQ)
    {
        msExtAdvScanRestoreQueueHandler(advScanData);
    }
    else
    {
        if (MS_ADV_SCAN_QUEUE_LOCKED(advScanData))
        {    /* Need to save signal, because we are waiting for a complete signal.*/
            CsrMessageQueuePush(&advScanData->saveQueue, MS_ADV_SCAN_PRIM, advScanData->recvMsgP);
            advScanData->recvMsgP = NULL;
        }
        else
        {    /* The Adv Scan Module is ready, just proceed */
            if ((id <= MS_PRIM_ADV_SCAN_DOWNSTREAM_HIGHEST) &&
                ((ms_ae_function_ptr[(CsrUint16)(id - MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST)]) != NULL))
            {
                msAdvScanLockQueue(advScanData);
                /* Use jump table */
                (ms_ae_function_ptr[(CsrUint16) (id - MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST)])(advScanData);
            }
            else
            {
                MsAdvScanGeneralException(MS_ADV_SCAN_PRIM,
                                        id,
                                        advScanData->globalState,
                                        "");
            }

        }
    }
}


/*! Public Functions */

/*! \brief Initialisation function
*/
void MsAdvScanInit(void **gash)
{
    msAdvScanInstanceData_t  *advScanData;
#ifdef CSR_BT_GLOBAL_INSTANCE
    *gash = &msAdvScanData;
#else
    *gash = (void *) CsrPmemZalloc(sizeof(msAdvScanInstanceData_t));
#endif
    advScanData = (msAdvScanInstanceData_t *) *gash;

    msAdvScanInitInstData(advScanData);

    /*! Initialise HCI interface */
    dm_hci_interface_deinit();

    MsExtAdvManagerInit();
    MsExtScanManagerInit();

}


/*! \brief Interface handler for all events.
*/
void MsAdvScanIfaceHandler(void **gash)
{
    uint16_t m=0;;
    void *message=NULL;
    msAdvScanInstanceData_t    *advScanData;

    advScanData = (msAdvScanInstanceData_t *) (*gash);

    get_message(ADV_SCAN_IFACEQUEUE, &m , &message);
    advScanData->recvMsgP = message;

    switch (m)
    {
        case MS_ADV_SCAN_PRIM:
            adv_scan_iface_msg_handler(advScanData);
            break;

        default:
            BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }
    if(advScanData->recvMsgP)
    {
        MsAdvScanFreeDownstreamMessageContents(advScanData->recvMsgP);
    }
    CsrPmemFree(advScanData->recvMsgP);
    advScanData->recvMsgP = NULL;

}



