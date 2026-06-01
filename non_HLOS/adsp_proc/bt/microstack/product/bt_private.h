/******************************************************************************
 Copyright (c) 2021 - 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#ifndef BT_PRIVATE_H__
#define BT_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "csr_bt_util.h"
#include "csr_log_text_2.h"
#include "bt_main.h"
#include "csr_time_common.h"
#include "csr_bt_config_microstack.h"

#define CSR_BT_RUN_TASK_DM_HCI                        (1)
#define CSR_BT_RUN_TASK_L2CAP                         (1)
#define CSR_BT_RUN_TASK_RFCOMM                        (1)
#define CSR_BT_RUN_TASK_ATT                           (1)
#define CSR_BT_RUN_TASK_GATT                          (1)
#define CSR_BT_RUN_TASK_BMM                           (1)
#define CSR_BT_RUN_TASK_TEST_TASK                     (1)
#define CSR_BT_RUN_TASK_ADV_SCAN                      (1)
#define CSR_BT_RUN_TASK_HCI_CMD_ARB                   (1)
#ifdef GATT_SEQUENCING
#define CSR_BT_RUN_TASK_HCI_ATT_ARB                   (1)
#endif /* GATT_SEQUENCING */


#define ID_STACK    0
#define ID_APP      1

#define API_LTSO_GEN        0

typedef CsrBool (*BtHostEventCallback)(CsrUint16 q, CsrUint16 mi, void *mv);

typedef struct BtHostAppEntry_tag
{
    struct BtHostAppEntry_tag *next;   /* Pointer to next BtHostAppEntry */
    BtAppHandle        handle;         /* Application Handle */ 
    BtHostAppCallback  appCallback;    /* Application Callback for handle*/
} BtHostAppEntry;

typedef struct
{
    BtHostAppEntry   *appInfo;         /* application specific info */
    BtHostEventCallback eventCallback;
    CsrBool appCtxRunning;             /* Variable to help in debugging, to identify if
                                        * app thread is running in BT Host context */
    CsrPrim primType;                  /* Variable to help in debugging,
                                        * primitive id being processed by application */
    CsrUint16 mi;                     /* Variable to help in debugging,
                                        * mi being processed by application */
} BtHostApiInstData;
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
typedef struct
{
    size_t patchLength;
    size_t nvmLength;
    BT_BD_ADDR btDeviceAddr;
    BT_HOST_PATCH_NVM_CB  patchCb;
    BT_HOST_PATCH_NVM_CB  nvmCb;
} BtHostPatchNvmData;

typedef struct
{
    CsrTime startTimeLow;
    CsrTime startTimeHigh;
    CsrUint64 totalTime;
} BtHostPatchNvmDownloadTime;

extern BtHostPatchNvmDownloadTime patchNvmDownloadTime;
#endif

extern void *schedInstance;

void BtTransportInitialise(void);
void BtTransportDeinitialise(void);


CsrBool BtHostPlatformCallBack(CsrSchedQid q, CsrUint16 mi, void *mv);

void BtHostExtCbInit(BtHostEventCallback eventCb);

CsrBool BtHostExternalAppCallBack(CsrSchedQid q, CsrUint16 mi, void *mv);

void BtHostRemoveAppHandleInfo(BtAppHandle handle);
void BtHostAddAppHandleInfo(BtAppHandle handle, BtHostAppCallback appCallback);
boolean BtHostRegisterAppHandleInfo(BtAppHandle handle, BtHostAppCallback appCb);

#ifdef PATCH_NVM_BUFFER_DOWNLOAD
void BtHostInitPatchNvmDownload(void);
void BtHostResetPatchNvmInst(void);
boolean BtHostIsFtmModeEnabled(void);
#endif

extern BtHostApiInstData BtHostApiInst;


#ifdef __cplusplus
}
#endif

#endif
