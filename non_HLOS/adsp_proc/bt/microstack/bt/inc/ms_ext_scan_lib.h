/******************************************************************************
 Copyright 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #26 $
******************************************************************************/
#ifndef MS_EXT_SCAN_LIB_H__
#define MS_EXT_SCAN_LIB_H__

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
 *      BmmExtScanGetGlobalParamsReqSend
 *
 *  DESCRIPTION
 *      Read the global parameters to be used during extended scanning.
 *      Stack sends MS_EXT_SCAN_GET_GLOBAL_PARAMS_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle:           Application handle
 *----------------------------------------------------------------------------*/
void BmmExtScanGetGlobalParamsReqSend(CsrSchedQid appHandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtScanSetGlobalParamsReqSend
 *
 *  DESCRIPTION
 *      Write the global parameters to be used during extended scanning.
 *
 *      This function should be called once at startup to set the global 
 *      prarmeters to be used by scanners during scanning.
 *
 *      This function can not be called when scanners are scanning. Application
 *      will need to stop scanning and then set parameters and restart scanning.
 *
 *      Parameter values should be set as described below : 
 *      flags : RFU. Should be set to 0.
 *      own_address_type:
 *           0 - Public
 *           1 - Random
 *           2 - Resolvable Private Address (If resolved list has no match use public)
 *           3 - Resolvable Private address (If resolved list has no match use random)
 *      scanning_filter_policy :
 *           0 - Accept all advertising packets, except directed not addressed to device
 *           1 - White list only
 *           2 - Initiators Identity address is not this device.
 *           3 - White list only and Initiators Identity address identifies this device.
 *      filter_duplicates : RFU, should be set to 0
 *      scanning_phys : BIT_0 - LE 1M, BIT_1 - LE 2M, BIT_2 - LE Coded
 *      phys : Scan Parameter Values for each LE PHYs
 *             Scan Type [0 - Passive Scanning , 1 - Active Scanning]
 *             Scan Interval [ 0x04 to 0xFFFF (2.5 ms to 40.95 s), Default = 100 ms]
 *             Scan Window [ 0x04 to 0xFFFF (2.5 ms to 40.95 s), Default = 95 ms]
 *
 *      Stack sends MS_EXT_SCAN_SET_GLOBAL_PARAMS_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle:              Identity of the calling process
 *      flags:                  Bit field (RFU)
 *      own_address_type:       Local address type
 *      scanning_filter_policy: Scanning filter policy
 *      filter_duplicates:      Filter duplicates
 *      scanning_phys:          Bit Field for LE PHYs to be used for Scanning
 *      phys:                   Scan Parameter Values to be used for each LE PHYs.
 *----------------------------------------------------------------------------*/
void BmmExtScanSetGlobalParamsReqSend(CsrSchedQid appHandle,
                                                CsrUint8 flags,
                                                CsrUint8 own_address_type,
                                                CsrUint8 scanning_filter_policy,
                                                CsrUint8 filter_duplicates,
                                                CsrUint16 scanning_phys,
                                                MsScanningPhy *phys);


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtScanRegisterScannerReqSend
 *
 *  DESCRIPTION
 *      Register a scanner and filter rules to be used during extended 
 *      scanning.Stack sends MS_EXT_SCAN_REGISTER_SCANNER_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle: Identity of the calling process
 *      flags : RFU. Should be set to 0.
 *      adv_filter : 
 *         0 - Receive all advertising data. Uses sub field below.
 *         1 - Do not receive any advertising data.
 *         3 - Only receive extended advertising data (Secondary adverting channel).
 *      adv_filter_sub_field1: RFU. Should be set to 0.
 *      adv_filter_sub_field2 : RFU. Should be set to 0.
 *      ad_structure_filter :RFU. Should be set to 0.
 *      ad_structure_filter_sub_field1 : RFU. Should be set to 0.
 *      ad_structure_filter_sub_field2 : RFU. Should be set to 0.
 *      num_reg_ad_types : RFU. Should be set to 0.
 *      reg_ad_types : RFU. Should be set to NULL.
 *----------------------------------------------------------------------------*/
void BmmExtScanRegisterScannerReqSend(CsrSchedQid appHandle,
                                                CsrUint32 flags,
                                                CsrUint16 adv_filter,
                                                CsrUint16 adv_filter_sub_field1,
                                                CsrUint32 adv_filter_sub_field2,
                                                CsrUint16 ad_structure_filter,
                                                CsrUint16 ad_structure_filter_sub_field1,
                                                CsrUint32 ad_structure_filter_sub_field2,
                                                CsrUint8 num_reg_ad_types,
                                                CsrUint8 *reg_ad_types);


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtScanUnregisterScannerReqSend
 *
 *  DESCRIPTION
 *      Unregister the scanner registered with the passed scan handle.
 *      Stack sends MS_EXT_SCAN_UNREGISTER_SCANNER_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *      scan_handle:         Scan handle of the registered scanner
 *----------------------------------------------------------------------------*/
void BmmExtScanUnregisterScannerReqSend(CsrSchedQid appHandle,
                                                CsrUint8 scan_handle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtScanEnableScannersReqSend
 *
 *  DESCRIPTION
 *      Enable or Disable registered scanners.
 *      Stack sends MS_EXT_SCAN_ENABLE_SCANNERS_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *
 *  PARAMETERS
 *      appHandle:            Identity of the calling process
 *      enable:               Scan enable or disable
 *      num_of_scanners:      Upto 5 scanner will be enabled or disabled.
 *      scanners:             Scan handles of the scanner to enable or disable.
 *                            Duration - 0 Scanning until disabled.
 *                                      0x01 - 0xFFFF (In seconds)
 *----------------------------------------------------------------------------*/
void BmmExtScanEnableScannersReqSend(CsrSchedQid appHandle,
                                                CsrUint8 enable,
                                                CsrUint8 num_of_scanners,
                                                MsScanners *scanners);

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtScanGetCtrlScanInfoReqSend
 *
 *  DESCRIPTION
 *      Get Controller's scanners configuration information.
 *      Stack sends MS_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *
 *  PARAMETERS
 *      appHandle:           Identity of the calling process
 *----------------------------------------------------------------------------*/
void BmmExtScanGetCtrlScanInfoReqSend(CsrSchedQid appHandle);



#ifdef __cplusplus
}
#endif

#endif
