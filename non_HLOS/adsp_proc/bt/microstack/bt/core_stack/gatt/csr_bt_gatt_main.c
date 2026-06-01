/******************************************************************************
 Copyright (c) 2010-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #8 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_gatt_private.h"

static const CsrBtGattHandlerType csrBtDispatchGatt[CSR_BT_GATT_PRIM_DOWNSTREAM_COUNT] =
{
    CsrBtGattRegisterReqHandler,                 /* CSR_BT_GATT_REGISTER_REQ */  
    CsrBtGattUnregisterReqHandler,               /* CSR_BT_GATT_UNREGISTER_REQ */ 
    CsrBtGattDbAllocReqHandler,                  /* CSR_BT_GATT_DB_ALLOC_REQ */ 
    CsrBtGattDbDeallocReqHandler,                /* CSR_BT_GATT_DB_DEALLOC_REQ */
    CsrBtGattDbAddReqHandler,                    /* CSR_BT_GATT_DB_ADD_REQ */
    CsrBtGattDbRemoveReqHandler,                 /* CSR_BT_GATT_DB_REMOVE_REQ */
    CsrBtGattDbAccessResHandler,                 /* CSR_BT_GATT_DB_ACCESS_RES */
    CsrBtGattEventSendReqHandler,                /* CSR_BT_GATT_EVENT_SEND_REQ */
    CsrBtGattDiscoverServicesReqHandler,         /* CSR_BT_GATT_DISCOVER_SERVICES_REQ */
    CsrBtGattDiscoverCharacReqHandler,           /* CSR_BT_GATT_DISCOVER_CHARAC_REQ */  
    CsrBtGattDiscoverCharacDescriptorsReqHandler,/* CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_REQ */
    CsrBtGattFindInclServicesReqHandler,         /* CSR_BT_GATT_FIND_INCL_SERVICES_REQ */
    CsrBtGattReadReqHandler,                     /* CSR_BT_GATT_READ_REQ */
    CsrBtGattReadByUuidReqHandler,               /* CSR_BT_GATT_READ_BY_UUID_REQ */
    CsrBtGattReadMultiReqHandler,                /* CSR_BT_GATT_READ_MULTI_REQ */
    CsrBtGattWriteReqHandler,                    /* CSR_BT_GATT_WRITE_REQ */
    CsrBtGattCancelReqHandler,                   /* CSR_BT_GATT_CANCEL_REQ */
    CsrBtGattSetEventMaskReqHandler,             /* CSR_BT_GATT_SET_EVENT_MASK_REQ */
    CsrBtGattClientRegisterServiceReqHandler,    /* CSR_BT_GATT_CLIENT_REGISTER_SERVICE_REQ */
    CsrBtGattClientIndicationRspHandler,         /* CSR_BT_GATT_CLIENT_INDICATION_RSP */
    CsrBtGattFlatDbRegisterReqHandler,           /* CSR_BT_GATT_FLAT_DB_REGISTER_REQ */
    CsrBtGattFlatDbRegisterHandleRangeReqHandler,/* CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_REQ */
    NULL,                                        /* CSR_BT_GATT_CLIENT_EXCHANGE_MTU_REQ */
    NULL,                                        /* CSR_BT_GATT_REMOTE_CLIENT_EXCHANGE_MTU_RES */
    CsrBtGattDbCommitReqHandler,                 /* CSR_BT_GATT_DB_COMMIT_REQ */
    CsrBtGattPriorityReqHandler,                 /* CSR_BT_GATT_APP_PRIO_CHANGE_REQ */
    CsrBtGattReadMultiVarReqHandler,             /* CSR_BT_GATT_READ_MULTI_VAR_REQ */
    CsrBtGattReadMultiVarRspHandler,             /* CSR_BT_GATT_READ_MULTI_VAR_RSP */
    CsrBtGattEattDisconnectReqHandler,           /* CSR_BT_GATT_EATT_DISCONNECT_REQ */ 
    CsrBtGattConnectBredrReqHandler,             /* CSR_BT_GATT_CONNECT_BREDR_REQ */
    CsrBtGattAcceptBredrReqHandler,              /* CSR_BT_GATT_ACCEPT_BREDR_REQ */
    CsrBtGattCancelAcceptBredrReqHandler,        /* CSR_BT_GATT_CANCEL_ACCEPT_BREDR_REQ */
    CsrBtGattConnectBredrResHandler,             /* CSR_BT_GATT_CONNECT_BREDR_RES */
    CsrBtGattDisconnectBredrReqHandler,          /* CSR_BT_GATT_DISCONNECT_BREDR_REQ */
	CsrBtGattConfigModeReqHandler,               /* CSR_BT_GATT_CONFIG_MODE_REQ */
    GattOffloadConnInfoReqHandler,               /* GATT_OFFLOAD_CONN_INFO_REQ */
    GattOffloadEnableServiceReqHandler,          /* GATT_OFFLOAD_ENABLE_SERVICE_REQ */
    GattOffloadDisableServiceReqHandler          /* GATT_OFFLOAD_DISABLE_SERVICE_REQ */
};

static const CsrBtGattAttHandlerType csrBtDispatchAtt[CSR_BT_GATT_ATT_PRIM_UP_COUNT] =
{
    CsrBtGattAttRegisterCfmHandler,                 /* ATT_REGISTER_CFM */  
    NULL,                                           /* ATT_UNREGISTER_CFM (Never unregister Att)*/ 
    CsrBtGattAttAddDbCfmHandler,                    /* ATT_ADD_DB_CFM (Flat db only)*/ 
    CsrBtGattAttAddCfmHandler,                      /* ATT_ADD_CFM */
    CsrBtGattAttRemoveCfmHandler,                   /* ATT_REMOVE_CFM */
    CsrBtGattAttConnectCfmHandler,                  /* ATT_CONNECT_CFM */
    CsrBtGattAttConnectIndHandler,                  /* ATT_CONNECT_IND */
    NULL,                                           /* ATT_DISCONNECT_CFM (Robinson only) */
    CsrBtGattAttDisconnectIndHandler,               /* ATT_DISCONNECT_IND */
    NULL,                                           /* ATT_EXCHANGE_MTU_CFM */ 
    NULL,                                           /* ATT_EXCHANGE_MTU_IND */
    CsrBtGattAttFindInfoCfmHandler,                 /* ATT_FIND_INFO_CFM */     
    CsrBtGattAttFindByTypeValueCfmHandler,          /* ATT_FIND_BY_TYPE_VALUE_CFM */
    CsrBtGattAttReadByTypeCfmHandler,               /* ATT_READ_BY_TYPE_CFM */
    CsrBtGattAttReadCfmHandler,                     /* ATT_READ_CFM */   
    CsrBtGattAttReadBlobCfmHandler,                 /* ATT_READ_BLOB_CFM */
    CsrBtGattAttReadMultiCfmHandler,                /* ATT_READ_MULTI_CFM */
    CsrBtGattAttReadByGroupTypeCfmHandler,          /* ATT_READ_BY_GROUP_TYPE_CFM */
    CsrBtGattAttWriteCfmHandler,                    /* ATT_WRITE_CFM */
    CsrBtGattAttPrepareWriteCfmHandler,             /* ATT_PREPARE_WRITE_CFM */
    CsrBtGattAttExecuteWriteCfmHandler,             /* ATT_EXECUTE_WRITE_CFM */
    CsrBtGattAttHandleValueCfmHandler,              /* ATT_HANDLE_VALUE_CFM */
    CsrBtGattAttHandleValueIndHandler,              /* ATT_HANDLE_VALUE_IND */
    CsrBtGattAttAccessIndHandler,                   /* ATT_ACCESS_IND */
    NULL,                                           /* ATT_WRITE_CMD_CFM */
    NULL,                                           /* ATT_HANDLE_VALUE_NTF_CFM */
    CsrBtGattAttAddRobustCachingCfmHandler,         /* ATT_ADD_ROBUST_CACHING_CFM */
    CsrBtGattAttChangeAwareIndHandler,              /* ATT_CHANGE_AWARE_IND */
    NULL,                                           /* ATT_CLOSE_CFM */
    NULL,                                           /* ATT_SET_BREDR_LOCAL_MTU_CFM */
    CsrBtGattAttReadMultiVarCfmHandler,             /* ATT_READ_MULTI_VAR_CFM */
    CsrBtGattAttReadMultiVarIndHandler,             /* ATT_READ_MULTI_VAR_IND */
    CsrBtGattAttHandleMultiValueCfmHandler,         /* ATT_HANDLE_VALUE_MULTI_CFM */
    CsrBtGattAttHandleValueMultiIndHandler,         /* ATT_HANDLE_VALUE_MULTI_IND */
    CsrBtGattEattRegisterCfmHandler,                /* EATT_REGISTER_CFM */
    NULL,                                           /* EATT_UNREGISTER_CFM (Never unregister EATT) */
    CsrBtGattEattConnectCfmHandler,                 /* ATT_ENHANCED_CONNECT_CFM */
    CsrBtGattEattConnectIndHandler,                 /* ATT_ENHANCED_CONNECT_IND */
    NULL,                                           /* ATT_ENHANCED_RECONFIGURE_IND */
    CsrBtGattAttDebugIndHandler,                    /* ATT_DEBUG_IND */
    GattOffloadAttErrorIndHandler                   /* ATT_ERROR_IND */
};

#ifdef CSR_BT_GLOBAL_INSTANCE
GattMainInst gattMainInstance;
#endif /* CSR_BT_GLOBAL_INSTANCE */

GattMainInst *gattMainInstPtr;


#ifdef CSR_LOG_ENABLE
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtGattLto);
#ifdef CSR_TARGET_PRODUCT_VM
#include "csr_bt_gatt_prim_enum_dbg.h"
CSR_PRESERVE_GENERATED_ENUM(CsrBtGattPrim)
#endif /*CSR_TARGET_PRODUCT_VM*/
#endif /* CSR_LOG_ENABLE */


/*prototype fpr csr_bt_gatt_main.c*/
void CsrBtGattInit(void **gash);
void CsrBtGattInterfaceHandler(void **gash);

void CsrBtGattInit(void **gash)
{
#ifdef CSR_BT_GLOBAL_INSTANCE
    *gash              = &gattMainInstance;
#else /* !CSR_BT_GLOBAL_INSTANCE */
    *gash              = CsrPmemZalloc(sizeof(GattMainInst));
#endif /* CSR_BT_GLOBAL_INSTANCE */

    gattMainInstPtr           = *gash;
    CsrBtGattInitHandler(gattMainInstPtr);
}

void CsrBtGattInterfaceHandler(void **gash)
{
    CsrUint16    eventClass=0;
    GattMainInst *inst = *gash;

    if (CsrSchedMessageGet(&eventClass, &inst->msg))
    {
        CsrPrim type = *(CsrPrim *)inst->msg;

        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
#ifdef CSR_TARGET_PRODUCT_VM
                CSR_LOG_TEXT_INFO((CsrBtGattLto, 0, "CsrBtGattInterfaceHandler MESSAGE:CsrBtGattPrim:0x%x", type));
#endif
                if ((type < CSR_BT_GATT_PRIM_DOWNSTREAM_COUNT) && csrBtDispatchGatt[type])
                {
                    if(!csrBtDispatchGatt[type](inst))
                    {
                        CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "No gatt owner %d", type));
                    }
                }
                else
                {
                    CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "GATT hdlr unknown prim %d", type));
                }

                if (inst->msg)
                {
                    CsrBtGattFreeDownstreamMessageContents(eventClass, inst->msg);
                }
                break;
            }
            case CSR_BT_ATT_PRIM:
            {
                if ((type - ATT_PRIM_UP) < CSR_BT_GATT_ATT_PRIM_UP_COUNT &&
                    csrBtDispatchAtt[(CsrUint16)(type - ATT_PRIM_UP)])
                {
                    csrBtDispatchAtt[(CsrUint16)(type - ATT_PRIM_UP)](inst);
                }
                else
                {
                    CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "GATT hdlr unknown att prim %d", type));
                }
                attlib_free((ATT_UPRIM_T *)inst->msg);
                inst->msg = NULL;
                break;
            }

            default:
                CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "GATT hdlr unknown event class %d", eventClass));
                break;
        }
        SynergyMessageFree(eventClass, inst->msg);
        inst->msg = NULL;
    }
}

#ifdef ENABLE_SHUTDOWN
void CsrBtGattDeinit(void **gash)
{
    CsrUint16    eventClass=0;
    GattMainInst *inst;
    CsrUint8 i;

    inst = *gash;

    while (CsrSchedMessageGet(&eventClass, &inst->msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
                CsrBtGattFreeDownstreamMessageContents(eventClass, inst->msg);
                break;
            case CSR_BT_ATT_PRIM:
                attlib_free((ATT_UPRIM_T *)inst->msg);
                inst->msg = NULL;
                break;

            default:
                CSR_LOG_TEXT_WARNING((CsrBtGattLto, 0, "GATT deinit unknown event class %d", eventClass));
                break;
        }
        SynergyMessageFree(eventClass, inst->msg);
    }

    for(i = 0; i < NO_OF_QUEUE ; i++)
    {
        CsrCmnListDeinit(&(inst->queue[i]));
    }

    CsrCmnListDeinit(&(inst->appInst));
    CsrCmnListDeinit(&(inst->prepare));
    CsrCmnListDeinit(&inst->connInst);

#ifndef CSR_BT_GLOBAL_INSTANCE
    CsrPmemFree(inst);
#else
    CsrMemSet(inst, 0, sizeof(GattMainInst));
#endif
    *gash = NULL;
}
#endif

