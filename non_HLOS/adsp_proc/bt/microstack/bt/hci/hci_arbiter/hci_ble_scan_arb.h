/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#ifndef __HCI_BLE_SCAN_ARB_H__
#define __HCI_BLE_SCAN_ARB_H__

#include "bluetooth.h"
#include "csr_prim_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    STATE_IDLE,
    STATE_SCAN_DISABLE_IN_PROGRESS,
    STATE_SCAN_DISABLE_DONE,
    STATE_SCAN_PARAM_SET_IN_PROGRESS,
    STATE_SCAN_PARAM_SET_DONE,
    STATE_SCAN_ENABLE_IN_PROGRESS,
    STATE_SCAN_ENABLE_DONE
}BLE_SCAN_ARB_STATE_T;

typedef struct
{
    bool                 scanParamsConfiguredByPrimaryStack;
    bool                 scanEnabledByPrimaryStack;
    bool                 scanParamsConfiguredByMicroStack;
    bool                 scanEnabledByMicroStack;

    BLE_SCAN_ARB_STATE_T    scanArbCurrentState;
    BLE_SCAN_ARB_STATE_T    scanArbStartState;
    BLE_SCAN_ARB_STATE_T    scanArbTargetState;

    uint8                   cmdSender;
    uint8                  *cmdPayload;
    uint16                 cmdPayloadLen;
}HCI_BLE_SCAN_ARB_T;

extern HCI_BLE_SCAN_ARB_T hciBleScanArb;

#define IsScanAlreadyEnabledByMicroStack()                  (hciBleScanArb.scanEnabledByMicroStack)
#define HasMicroStackAlreadyConfiguredScanParams()          (hciBleScanArb.scanParamsConfiguredByMicroStack)
#define IsScanAlreadyEnabledByPrimaryStack()                (hciBleScanArb.scanEnabledByPrimaryStack)
#define HasPrimaryStackAlreadyConfiguredScanParams()        (hciBleScanArb.scanParamsConfiguredByPrimaryStack)



/*! Public Functions */
extern void HciBleScanArbInit(void);
extern void HciBleScanArbDeinit(void);
extern bool HciBleScanArbitration(uint8 sender, CsrUint8 *payload, CsrUint16 len);
extern bool IsHciEventMeantForSpecialArbitrationLogic(CsrHciEventInd *prim);




#ifdef __cplusplus
}
#endif

#endif /* __HCI_BLE_SCAN_ARB_H__ */

