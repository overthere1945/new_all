/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #5 $
******************************************************************************/

#include "csr_synergy.h"


#include "csr_log_text_2.h"
#include "csr_random_common.h"
#include "hci.h"
#include "ext_adv_manager.h"
#include "csr_bt_profiles.h"

void MsExtAdvSetDataCfmHandler(uint8_t status, uint8_t advHandle)
{
    MsExtAdvSetDataCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = MS_EXT_ADV_SET_DATA_CFM;
    prim->resultCode = (CsrBtResultCode) status;
    prim->advHandle = advHandle;

    CsrSchedMessagePut(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM, (prim));
    MsExtAdvScanLocalQueueHandler();
}

void MsExtAdvSetScanRespDataCfmHandler(uint8_t status, uint8_t advHandle)
{
    MsExtAdvSetScanRespDataCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = MS_EXT_ADV_SET_SCAN_RESP_DATA_CFM;
    prim->resultCode = (CsrBtResultCode) status;
    prim->advHandle = advHandle;

    CsrSchedMessagePut(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM, (prim));
    MsExtAdvScanLocalQueueHandler();
}

void MsExtAdvReadMaxAdvDataLenCfmHandler(uint8_t status, uint16           maxAdvDataLen)
{
    MsExtAdvReadMaxAdvDataLenCfm *prim;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = MS_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM;
    prim->resultCode = (CsrBtResultCode) status;
    prim->maxAdvDataLen = maxAdvDataLen;

    CsrSchedMessagePut(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM, (prim));
    MsExtAdvScanLocalQueueHandler();
}

void MsExtAdvTerminatedIndHandler(void *msg)
{
    HCI_EV_ULP_ADV_SET_TERMINATED_T *dmPrim = msg;
    MsExtAdvTerminatedInd *prim;
    uint8_t index;

    prim = CsrPmemAlloc(sizeof(*prim));
    prim->type = MS_EXT_ADV_TERMINATED_IND;
    prim->advHandle = dmPrim->adv_handle;
    prim->reason = dmPrim->status;
    prim->eaEvents = dmPrim->num_completed_ea_events;
    prim->maxAdvSets = maxSupportedAdvSets;
    appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(dmPrim->adv_handle)].advertising &= ~ADV_ENABLED_CTRL;
    for (index = 0; index < maxSupportedAdvSets; index++)
    {
        if (appRegHandles[index].advertising & ADV_ENABLED_CTRL)
            prim->advBits |= EXT_ADV_ENABLED << index;
    }
    CsrSchedMessagePut(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM, (prim));
    MsExtAdvScanLocalQueueHandler();
}

/*! \brief Tell ms_ext_adv_manager if the controller accepted the enable advertising command
 *
    \param status HCI_SUCCESS or error
 */
void MsExtAdvMultiEnableCommandDone(uint8_t status)
{
    MsExtAdvMultiEnableCfm *cfm;
    uint8_t index;

    cfm = zpnew(MsExtAdvMultiEnableCfm);
    cfm->type = MS_EXT_ADV_MULTI_ENABLE_CFM;
    cfm->resultCode = status;
    cfm->maxAdvSets = maxSupportedAdvSets;

    /* Report current advertising states for all advertising sets including
       ADV_HANDLE_FOR_LEGACY_API */
    for (index = 0; index < maxSupportedAdvSets; index++)
    {
        if (appRegHandles[index].advertising & ADV_ENABLED_CTRL)
            cfm->advBits |= EXT_ADV_ENABLED << (index + lowestAdvHandleMicroStack);
    }

    CsrSchedMessagePut(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM, (cfm));
    MsExtAdvScanLocalQueueHandler();

}



