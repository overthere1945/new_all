/******************************************************************************
 Copyright 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #26 $
******************************************************************************/
#ifndef MS_EXT_ADV_LIB_H__
#define MS_EXT_ADV_LIB_H__

#include "csr_synergy.h"
#include "csr_bt_profiles.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_msg_transport.h"

#include "csr_bt_tasks.h"
#include "csr_bt_addr.h"
#include "ms_adv_scan_prim.h"
#ifdef __cplusplus
extern "C" {
#endif


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
void BmmExtAdvRegisterAppAdvSetReqSend(CsrSchedQid appHandle);


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
void BmmExtAdvUnregisterAppAdvSetReqSend(CsrSchedQid appHandle,
                                        CsrUint8 advHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvSetParamsReqSend
 *
 *  DESCRIPTION
 *      Write the advertising set V2 paramaters. 
 *      Stack sends MS_EXT_ADV_SET_PARAMS_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 * 
 *  PARAMETERS
 *      appHandle:             Application handle
 *      advHandle              A registered advertising set value
 *      advEventProperties     Advertising type. Valid bit options are: HCI_ULP_EXT_ADV_NON_CONN_AND_NON_SCAN_ADVERTISING and 
 *                              HCI_ULP_EXT_ADV_INCLUDE_TX_POWER
 *      primaryAdvIntervalMin  Minimum advertising interval N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms)
 *      primaryAdvIntervalMax  Maximum advertising interval N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms)
 *      primaryAdvChannelMap   Bit mask field (bit 0 = Channel 37, bit 1 = Channel 38 and bit 2 = Channel 39)
 *      ownAddrType            Local address type
 *      peerAddr               Remote device address
 *      advFilterPolicy        
 *      primaryAdvPhy          PHY for advertising paackets on Primary advertising channels
 *                             1 - LE 1M, 3 - LE Coded
 *      secondaryAdvMaxSkip    Maximum advertising events on the primary advertising
 *                             channel that can be skipped before sending an AUX_ADV_IND.
 *      secondaryAdvPhy        PHY for advertising paackets on Secondary advertising channels 
 *                             1 - LE 1M, 2 - LE 2M, 3 - LE Coded
 *      advSid                 Advertsing set ID.
 *      advTxPower             Max power level at which the adv packets are transmitted 
 *                             Range : -127 to +20 in dBm, 0x7F for Host has no preference
 *                             Controller can choose power level lower than or equal to the one specified by Host
 *      scanReqNotifyEnable    RFA. Always set to 0.
 *      primaryAdvPhyOptions   RFA. Always set to 0.
 *      secondaryAdvPhyOptions RFA. Always set to 0.
 *      featureMask            RFA. Always set to 0..
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
                                             MsExtAdvSetParamsMask featureMask);


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvSetDataReqSend
 *
 *  DESCRIPTION
 *      Write the extended advertising data.
 *      Stack sends MS_EXT_ADV_SET_DATA_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle         Identity of the calling process
 *      advHandle         A registered advertising set value
 *      operation         Part of the data to be set. 0 : Intermittent Fragment,
 *                        1 : First Fragment, 2 : Last Fragment, 3 : Complete Data
 *      fragPreference    Fragmentation preference at controller. 0 : May fragment , 1 : No fragment
 *      dataLen           Advertising data length (0 to 251 octets).
 *      data              Array of pointers to a 32 octet dynamically allocated buffer,
 *                        Max array size is MS_EXT_ADV_DATA_BYTES_PTRS_MAX
 *----------------------------------------------------------------------------*/
void BmmExtAdvSetDataReqSend(CsrSchedQid appHandle,
                                    CsrUint8 advHandle,
                                    CsrUint8 operation,
                                    CsrUint8 fragPreference,
                                    CsrUint8 dataLen,
                                    CsrUint8 *data[]);


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvReadMaxAdvDataLenReqSend
 *
 *  DESCRIPTION
 *      Read the extended advertising max data length supported by controller.
 *      Stack sends MS_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle      Identity of the calling process
 *      advHandle      A registered advertising set value
 *----------------------------------------------------------------------------*/
void BmmExtAdvReadMaxAdvDataLenReqSend(CsrSchedQid appHandle, CsrUint8 advHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvSetRandomAddrReqSend
 *
 *  DESCRIPTION
 *      Set the advertising set's random device address to be used.
 *      Stack sends MS_EXT_ADV_SET_RANDOM_ADDR_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle        Identity of the calling process
 *      advHandle        A registered advertising set value
 *      action           Option for generating or setting a random address. The following actions are permitted:
 *                           MS_EXT_ADV_ADDRESS_WRITE_STATIC
 *                           MS_EXT_ADV_ADDRESS_WRITE_NON_RESOLVABLE
 *                           MS_EXT_ADV_ADDRESS_WRITE_RESOLVABLE
 *      randomAddr       Random address to be set
 *----------------------------------------------------------------------------*/
void BmmExtAdvSetRandomAddrReqSend(CsrSchedQid appHandle,
                                              CsrUint8 advHandle,
                                              CsrUint16 action,
                                              CsrBtDeviceAddr randomAddr);

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
 *  PARAMETERS
 *      appHandle      Identity of the calling process
 *----------------------------------------------------------------------------*/
void BmmExtAdvSetsInfoReqSend(CsrSchedQid appHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtAdvMultiEnableReqSend
 *
 *  DESCRIPTION
 *      Enable/disable advertising for 1 to MS_EXT_ADV_MAX_ADV_HANDLES advertising sets. Further info
 *      be found in BT5.1: command = LE Set Extended Advertising Enable Command.
 *      Stack sends MS_EXT_ADV_MULTI_ENABLE_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle:      Identity of the calling process
 *      enable:         Enable/disable advertising
 *      numSets:        Number of advertising sets to be enabled or
 *                      disabled by this prim.
 *      config:         Advertising set config data
 *----------------------------------------------------------------------------*/
void BmmExtAdvMultiEnableReqSend(CsrSchedQid appHandle,
                                           CsrUint8 enable,
                                           CsrUint8 numSets,
                                           AdvEnableConfig config[MS_EXT_ADV_MAX_ADV_HANDLES]);


#ifdef __cplusplus
}
#endif

#endif
