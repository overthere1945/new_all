/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_profiles.h"
#include "csr_pmem.h"
#include "ms_ext_adv_lib.h"
#include "csr_bt_util.h"

/*! Private Function prototypes */
static void msAdvScanMsgTransport(void* __msg);


/*! Private Functions */
static void msAdvScanMsgTransport(void* __msg)
{
    CsrMsgTransport(ADV_SCAN_IFACEQUEUE, MS_ADV_SCAN_PRIM,__msg);
}


/*! Public Functions */
/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvRegisterAppAdvSetReqSend
 *
 *  DESCRIPTION
 *      Application register to use as an advertising set.
 *      Stack sends MS_EXT_ADV_REGISTER_APP_ADV_SET_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *----------------------------------------------------------------------------*/

void BmmExtAdvRegisterAppAdvSetReqSend(CsrSchedQid appHandle)
{
    MsExtAdvRegisterAppAdvSetReq *prim;

    prim = CsrPmemAlloc(sizeof(MsExtAdvRegisterAppAdvSetReq));
    prim->type = MS_EXT_ADV_REGISTER_APP_ADV_SET_REQ;
    prim->appHandle = appHandle;

    msAdvScanMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvUnregisterAppAdvSetReqSend
 *
 *  DESCRIPTION
 *      Application un-register the advertising set.
 *      Stack sends MS_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle:       Application handle
 *      advHandle        A registered advertising set value
 *----------------------------------------------------------------------------*/
void BmmExtAdvUnregisterAppAdvSetReqSend(CsrSchedQid appHandle, CsrUint8 advHandle)
{
    MsExtAdvUnregisterAppAdvSetReq *prim;

    prim = CsrPmemAlloc(sizeof(MsExtAdvUnregisterAppAdvSetReq));
    prim->type = MS_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;

    msAdvScanMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvSetParamsReqSend
 *
 *  DESCRIPTION
 *      Write the advertising set V2 paramaters. 
 *      Stack sends MS_EXT_ADV_SET_PARAMS_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 * 
 *----------------------------------------------------------------------------*/
void BmmExtAdvSetParamsReqSend(CsrSchedQid appHandle,
                                            CsrUint8 advHandle,
                                            CsrUint16 advEventProperties,
                                            CsrUint32 primaryAdvIntervalMin,
                                            CsrUint32 primaryAdvIntervalMax,
                                            CsrUint8 primaryAdvChannelMap,
                                            CsrUint8 ownAddrType,
                                            TYPED_BD_ADDR_T peerAddr,
                                            CsrUint8 advFilterPolicy,
                                            CsrUint16 primaryAdvPhy,
                                            CsrUint8 secondaryAdvMaxSkip,
                                            CsrUint16 secondaryAdvPhy,
                                            CsrUint16 advSid,
                                            CsrInt8 advTxPower,
                                            CsrUint8 scanReqNotifyEnable,
                                            CsrUint8 primaryAdvPhyOptions,
                                            CsrUint8 secondaryAdvPhyOptions,
                                            MsExtAdvSetParamsMask featureMask)
{
    MsExtAdvSetParamsReq *prim;

    prim = CsrPmemAlloc(sizeof(MsExtAdvSetParamsReq));
    CsrMemSet(prim, 0, sizeof(MsExtAdvSetParamsReq));
    prim->type = MS_EXT_ADV_SET_PARAMS_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->advEventProperties = advEventProperties;
    prim->primaryAdvIntervalMin = primaryAdvIntervalMin;
    prim->primaryAdvIntervalMax = primaryAdvIntervalMax;
    prim->primaryAdvChannelMap = primaryAdvChannelMap;
    prim->ownAddrType = ownAddrType;
    CsrBtAddrCopy(&(prim->peerAddr), &(peerAddr));
    prim->advFilterPolicy = advFilterPolicy;
    prim->primaryAdvPhy = primaryAdvPhy;
    prim->secondaryAdvMaxSkip = secondaryAdvMaxSkip;
    prim->secondaryAdvPhy = secondaryAdvPhy;
    prim->advSid = advSid;
    prim->advTxPower = advTxPower;
    prim->scanReqNotifyEnable = scanReqNotifyEnable;
    prim->primaryAdvPhyOptions = primaryAdvPhyOptions;
    prim->secondaryAdvPhyOptions = secondaryAdvPhyOptions;
    prim->featureMask = featureMask;

    msAdvScanMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvSetDataReqSend
 *
 *  DESCRIPTION
 *      Write the extended advertising data.
 *      Stack sends MS_EXT_ADV_SET_DATA_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *----------------------------------------------------------------------------*/
void BmmExtAdvSetDataReqSend(CsrSchedQid appHandle,
                                    CsrUint8 advHandle,
                                    CsrUint8 operation,
                                    CsrUint8 fragPreference,
                                    CsrUint8 dataLen,
                                    CsrUint8 *data[])
{
    CsrUint8 i;
    MsExtAdvSetDataReq *prim;

    prim = CsrPmemZalloc(sizeof(MsExtAdvSetDataReq));
    CsrMemSet(prim, 0, sizeof(MsExtAdvSetDataReq));
    prim->type = MS_EXT_ADV_SET_DATA_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->operation = operation;
    prim->fragPreference = fragPreference;
    prim->dataLength = (dataLen < MS_EXT_ADV_DATA_LENGTH_MAX) ? 
        dataLen : MS_EXT_ADV_DATA_LENGTH_MAX;
    for (i = 0; i < MS_EXT_ADV_DATA_BYTES_PTRS_MAX; i++)
    {
        prim->data[i] = data[i];
    }

    msAdvScanMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvSetScanRespDataReqSend
 *
 *  DESCRIPTION
 *      Write the extended advertising's scan response data.
 *      Stack sends MS_EXT_ADV_SET_SCAN_RESP_DATA_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *----------------------------------------------------------------------------*/
void BmmExtAdvSetScanRespDataReqSend(CsrSchedQid appHandle,
                                    CsrUint8 advHandle,
                                    CsrUint8 operation,
                                    CsrUint8 fragPreference,
                                    CsrUint8 dataLen,
                                    CsrUint8 *data[])
{
    CsrUint8 i;
    MsExtAdvSetScanRespDataReq *prim;

    prim = CsrPmemZalloc(sizeof(MsExtAdvSetScanRespDataReq));
    CsrMemSet(prim, 0, sizeof(MsExtAdvSetScanRespDataReq));
    prim->type = MS_EXT_ADV_SET_SCAN_RESP_DATA_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->operation = operation;
    prim->fragPreference = fragPreference;
    prim->dataLength = (dataLen < MS_EXT_ADV_SCAN_RESP_DATA_LENGTH_MAX) ? 
        dataLen : MS_EXT_ADV_SCAN_RESP_DATA_LENGTH_MAX;
    for (i = 0; i < MS_EXT_ADV_SCAN_RESP_DATA_BYTES_PTRS_MAX; i++)
    {
        prim->data[i] = data[i];
    }

    msAdvScanMsgTransport(prim);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvReadMaxAdvDataLenReqSend
 *
 *  DESCRIPTION
 *      Read the extended advertising max data length supported by controller.
 *      Stack sends MS_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *----------------------------------------------------------------------------*/
void BmmExtAdvReadMaxAdvDataLenReqSend(CsrSchedQid appHandle, CsrUint8 advHandle)
{
    MsExtAdvReadMaxAdvDataLenReq *prim;

    prim = CsrPmemAlloc(sizeof(MsExtAdvReadMaxAdvDataLenReq));
    prim->type = MS_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;

    msAdvScanMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvSetRandomAddrReqSend
 *
 *  DESCRIPTION
 *      Set the advertising set's random device address to be used.
 *      Stack sends MS_EXT_ADV_SET_RANDOM_ADDR_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *----------------------------------------------------------------------------*/
void BmmExtAdvSetRandomAddrReqSend(CsrSchedQid appHandle,
                                              CsrUint8 advHandle,
                                              CsrUint16 action,
                                              CsrBtDeviceAddr randomAddr)
{
    MsExtAdvSetRandomAddrReq *prim;

    prim = CsrPmemAlloc(sizeof(MsExtAdvSetRandomAddrReq));
    prim->type = MS_EXT_ADV_SET_RANDOM_ADDR_REQ;
    prim->appHandle = appHandle;
    prim->advHandle = advHandle;
    prim->action = action;
    prim->randomAddr = randomAddr;

    msAdvScanMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvSetsInfoReqSend
 *
 *  DESCRIPTION
 *      Reports advertising and registered states for all advertising sets 
 *      supported by a device.
 *      Stack sends MS_EXT_ADV_SETS_INFO_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *----------------------------------------------------------------------------*/
void BmmExtAdvSetsInfoReqSend(CsrSchedQid appHandle)
{
    MsExtAdvSetsInfoReq *prim;

    prim = CsrPmemAlloc(sizeof(MsExtAdvSetsInfoReq));
    prim->type = MS_EXT_ADV_SETS_INFO_REQ;
    prim->appHandle = appHandle;

    msAdvScanMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvMultiEnableReqSend
 *
 *  DESCRIPTION
 *      Enable/disable advertising.
 *      Stack sends MS_EXT_ADV_MULTI_ENABLE_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *----------------------------------------------------------------------------*/
void BmmExtAdvMultiEnableReqSend(CsrSchedQid appHandle,
                                           CsrUint8 enable,
                                           CsrUint8 numSets,
                                           AdvEnableConfig config[MS_EXT_ADV_MAX_ADV_HANDLES])
{
    MsExtAdvMultiEnableReq *prim;
    CsrUint8 i;

    prim = CsrPmemZalloc(sizeof(MsExtAdvMultiEnableReq));
    prim->type = MS_EXT_ADV_MULTI_ENABLE_REQ;
    prim->appHandle = appHandle;
    prim->enable = enable;
    prim->numSets = numSets;
    prim->numSets = (numSets <= MS_EXT_ADV_MAX_ADV_HANDLES) ?
        numSets : MS_EXT_ADV_MAX_ADV_HANDLES;
    for (i = 0; i < prim->numSets; i++)
    {
        prim->config[i] = config[i];
    }

    msAdvScanMsgTransport(prim);
}




