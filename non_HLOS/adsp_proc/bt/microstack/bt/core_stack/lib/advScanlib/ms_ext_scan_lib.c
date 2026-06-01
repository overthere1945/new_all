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
void BmmExtScanGetGlobalParamsReqSend(CsrSchedQid appHandle)
{
    MsExtScanGetGlobalParamsReq *prim;

    prim = CsrPmemAlloc(sizeof(MsExtScanGetGlobalParamsReq));
    prim->type = MS_EXT_SCAN_GET_GLOBAL_PARAMS_REQ;
    prim->appHandle = appHandle;

    msAdvScanMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtScanSetGlobalParamsReqSend
 *
 *  DESCRIPTION
 *      Write the global parameters to be used during extended scanning.
 *
 *      This function should be called once at startup to set the global 
 *      prarmeters to be used by scanners during scanning.
 *      Stack sends MS_EXT_SCAN_SET_GLOBAL_PARAMS_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *----------------------------------------------------------------------------*/
void BmmExtScanSetGlobalParamsReqSend(CsrSchedQid appHandle,
                                                CsrUint8 flags,
                                                CsrUint8 own_address_type,
                                                CsrUint8 scanning_filter_policy,
                                                CsrUint8 filter_duplicates,
                                                CsrUint16 scanning_phys,
                                                MsScanningPhy *phys)
{
    MsExtScanSetGlobalParamsReq *prim;
    CsrUint8 index = 0;

    prim = CsrPmemZalloc(sizeof(MsExtScanSetGlobalParamsReq));
    prim->type = MS_EXT_SCAN_SET_GLOBAL_PARAMS_REQ;
    prim->appHandle = appHandle;
    prim->flags = flags;
    prim->ownAddressType = own_address_type;
    prim->scanningFilterPolicy = scanning_filter_policy;
    prim->filterDuplicates = filter_duplicates;
    prim->scanningPhy = scanning_phys;
    if (MS_SCAN_LE_1M_PHY_BIT_MASK & scanning_phys)
    {
        prim->phys[index] = phys[index];
        index++;
    }
    if (MS_SCAN_LE_CODED_PHY_BIT_MASK & scanning_phys)
        prim->phys[index] = phys[index];

    msAdvScanMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtScanRegisterScannerReqSend
 *
 *  DESCRIPTION
 *      Register a scanner and filter rules to be used during extended 
 *      scanning.Stack sends MS_EXT_SCAN_REGISTER_SCANNER_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
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
                                                CsrUint8 *reg_ad_types)
{
    MsExtScanRegisterScannerReq *prim;
    CsrUint8 i;

    prim = CsrPmemZalloc(sizeof(MsExtScanRegisterScannerReq));
    prim->type = MS_EXT_SCAN_REGISTER_SCANNER_REQ;
    prim->appHandle = appHandle;
    prim->flags = flags;
    prim->advFilter = adv_filter;
    prim->advFilterSubField1 = adv_filter_sub_field1;
    prim->adStructureFilterSubField2 = adv_filter_sub_field2;
    prim->adStructureFilter = ad_structure_filter;
    prim->adStructureFilterSubField1 = ad_structure_filter_sub_field1;
    prim->adStructureFilterSubField2 = ad_structure_filter_sub_field2;
    prim->numRegAdTypes = (num_reg_ad_types <= MS_SCAN_MAX_REG_AD_TYPES) ?
        num_reg_ad_types : MS_SCAN_MAX_REG_AD_TYPES;
    for (i = 0; i < prim->numRegAdTypes; i++)
    {
        prim->regAdTypes[i] = reg_ad_types[i];
    }

    msAdvScanMsgTransport(prim);
}

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
                                                CsrUint8 scan_handle)
{
    MsExtScanUnregisterScannerReq *prim;

    prim = CsrPmemAlloc(sizeof(MsExtScanUnregisterScannerReq));
    prim->type = MS_EXT_SCAN_UNREGISTER_SCANNER_REQ;
    prim->appHandle = appHandle;
    prim->scanHandle = scan_handle;

    msAdvScanMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtScanEnableScannersReqSend
 *
 *  DESCRIPTION
 *      Enable or Disable registered scanners.
 *      Stack sends MS_EXT_SCAN_ENABLE_SCANNERS_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *----------------------------------------------------------------------------*/
void BmmExtScanEnableScannersReqSend(CsrSchedQid appHandle,
                                                CsrUint8 enable,
                                                CsrUint8 num_of_scanners,
                                                MsScanners *scanners)
{
    MsExtScanEnableScannersReq *prim;
    CsrUint8 i;

    prim = CsrPmemZalloc(sizeof(MsExtScanEnableScannersReq));
    prim->type = MS_EXT_SCAN_ENABLE_SCANNERS_REQ;
    prim->appHandle = appHandle;
    prim->enable = enable;
    prim->numOfScanners = (num_of_scanners <= MS_SCAN_MAX_SCANNERS) ?
        num_of_scanners : MS_SCAN_MAX_SCANNERS;
    for (i = 0; i < prim->numOfScanners; i++)
    {
        prim->scanners[i] = scanners[i];
    }

    msAdvScanMsgTransport(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      BmmExtScanGetCtrlScanInfoReqSend
 *
 *  DESCRIPTION
 *      Get Controller's scanners configuration information.
 *      Stack sends MS_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM back to the application.
 *      HCI Error code would be returned in the confirm message to indicate success or failure.
 *
 *----------------------------------------------------------------------------*/
void BmmExtScanGetCtrlScanInfoReqSend(CsrSchedQid appHandle)
{
    MsExtScanGetCtrlScanInfoReq *prim;

    prim = CsrPmemAlloc(sizeof(MsExtScanGetCtrlScanInfoReq));
    prim->type = MS_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ;
    prim->appHandle = appHandle;

    msAdvScanMsgTransport(prim);
}





