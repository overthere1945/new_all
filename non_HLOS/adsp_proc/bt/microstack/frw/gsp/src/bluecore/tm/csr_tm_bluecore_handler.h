/******************************************************************************
 Copyright (c) 2008-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CSR_TM_BLUECORE_HANDLER_H__
#define CSR_TM_BLUECORE_HANDLER_H__

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_result.h"
#include "csr_message_queue.h"
#include "csr_tm_bluecore_task.h"
#include "csr_tm_bluecore_prim.h"

#ifndef CSR_USE_QCA_CHIP
#include "csr_bccmd_private_prim.h"
#endif

#include "csr_list.h"
#include "csr_transport.h"
#include "csr_time.h"
#ifdef CSR_USE_QCA_CHIP
#include "csr_tm_qc_hcivs_lib.h"
#include "csr_tm_qc_hcivs_util.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define CSR_TM_BLUECORE_STATE_IDLE                  (0x00)
#define CSR_TM_BLUECORE_STATE_ACTIVATING            (0x01)
#define CSR_TM_BLUECORE_STATE_RESTARTING            (0x02)
#define CSR_TM_BLUECORE_STATE_RESETTING             (0x03)
#define CSR_TM_BLUECORE_STATE_ACTIVATED             (0x04)
#define CSR_TM_BLUECORE_STATE_DEACTIVATING          (0x05)

#ifdef CSR_USE_QCA_CHIP
typedef enum
{
    DOWNLOAD_IDLE,                              /* initial state where download is not started */
    M0_DOWNLOAD,                                /* downloading M0 patch file */
    AUP_DOWNLOAD,                               /* downloadig AUP patch file */
    NVM_DOWNLOAD,                               /* downloading NVM file */
    MAX_NUMBER_OF_DOWNLOAD_STATES               /* dummy enum not to be used for the state machine */
}patchDownloadState;
#endif

typedef struct CsrTmBlueCoreHandleListTag
{
    struct CsrTmBlueCoreHandleListTag *next; /* must be 1st */
    struct CsrTmBlueCoreHandleListTag *prev; /* must be 2nd */
    CsrSchedQid                        phandle;
} CsrTmBlueCoreHandleList;

typedef struct
{
#ifdef CSR_BLUECORE_ONOFF
    CsrCmnList_t         transportDelegates;
    CsrSchedQid          transportActivator;
    CsrUint16            pendingResponses;
    CsrMessageQueueType *saveQueue;
#ifdef CSR_BLUECORE_PING_INTERVAL
    CsrSchedTid pingTimerId;
#endif
#else
    CsrCmnList_t transportActivators;
#endif
    CsrUint8    numberOfForcedReset;
    CsrUint8    state;
    CsrSchedTid timerId;
    void       *blueCoreTransportHandle;
    void       *recvMsgP;
    CsrSchedQid nopHandler;
    CsrUint8   *savedNop;
    CsrUint16   savedNopLength;
#ifdef CSR_CHIP_MANAGER_ENABLE
    CsrBool      activateTransportCfmIsSend;
    CsrCmnList_t cmStatusSubscribers;
    CsrCmnList_t cmReplayersRegistered;
    CsrCmnList_t cmReplayersStarted;
    CsrSchedTid  pingTimerId;
    CsrTime      pingInterval;
    CsrBool      pingStarted;
#endif
#ifdef CSR_USE_BCSP_HTRANS
    void *service_handle;
#endif
#ifdef CSR_USE_QCA_CHIP
    CsrQcBin *qcBinVar;
    patchDownloadState qcPatchDownloadState;
#endif
} CsrTmBlueCoreInstanceData;

#define CSR_TM_BLUECORE_LIST_ADD(listPtr) (CsrTmBlueCoreHandleList *) CsrCmnListElementAddLast((CsrCmnList_t *) listPtr, sizeof(CsrTmBlueCoreHandleList))
#define CSR_TM_BLUECORE_LIST_ITERATE(listPtr, func, data) CsrCmnListIterate((CsrCmnList_t *) listPtr, func, (void *) data)
#define CSR_TM_BLUECORE_LIST_ITERATE_AND_REMOVE(listPtr, func, data) CsrCmnListIterateAllowRemove((CsrCmnList_t *) listPtr, func, (void *) data)
#define CSR_TM_BLUECORE_LIST_IS_EMPTY(listPtr) (CsrCmnListGetCount(listPtr) == 0)

#define CSR_QC_HCI_VS_RESULT_SUCCESS                                    (0x0000)
#define CSR_QC_HCI_VS_RESULT_ERROR                                      (0x0001)

#ifdef PATCH_NVM_BUFFER_DOWNLOAD
typedef enum
{
    PATCH_DOWNLOAD_COMPLETE,
    NVM_DOWNLOAD_COMPLETE,
    PATCH_DATA_GET,
    NVM_DATA_GET,
    DOWNLOAD_ERROR,
    BOOTSTRAP_COMPLETE
} download_callback_op;

typedef CsrUint8 CsrTmBlueCorePatchNvmState;

#define IDLE_DOWNLOAD_STATE           ((CsrTmBlueCorePatchNvmState) 0x00)
#define PATCH_DOWNLOAD_STATE          ((CsrTmBlueCorePatchNvmState) 0x01)
#define NVM_DOWNLOAD_STATE            ((CsrTmBlueCorePatchNvmState) 0x02)

#define TLV_SEGMENT_LENGTH_MAX                     243

typedef CsrBool (*CSR_TM_BLUECORE_PATCH_NVM_CB)(download_callback_op operation, size_t offset, size_t *bufferLength, CsrUint8 **buffer, CsrUint8 *bufferToBeFreed, CsrUint8 status);


typedef struct
{
    CsrUint24 lap;   /*!< Lower Address Part 00..23 */
    CsrUint8  uap;   /*!< upper Address Part 24..31 */
    CsrUint16 nap;   /*!< Non-significant    32..47 */
} BD_ADDR;

typedef struct
{
    CsrSize currentIndex; /* Current index of patch/nvm segment download */
    CsrSize index; /* Overall download index of patch/nvm length */
    CsrSize length; /* Length of current segment of patch/nvm */
    CsrUint8  *payload; /* Pointer to current buffer of patch/nvm data */
    CsrUint8  bufferToBeFreed; /* Buffer to be freed by bluecore or not */
} CsrTmBlueCoreTlvData;

typedef struct
{
    CsrSize patchLength;
    CsrSize nvmLength;
    BD_ADDR btDeviceAddr;
    CSR_TM_BLUECORE_PATCH_NVM_CB  patchCb;
    CSR_TM_BLUECORE_PATCH_NVM_CB  nvmCb;
    CsrTmBlueCoreTlvData tlvData;
    CsrTmBlueCorePatchNvmState state;
} CsrTmBlueCorePatchNvmData;

void CsrTmBlueCoreStartPatchNvmDownload(void);
void CsrTmBlueCoreSendBootstrapComplete(void);
void CsrTmBlueCoreHandlePatchNvmDownloadCfm(CsrResult result);
void CsrTmBlueCoreResetFtmMode(void);

#endif
#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_BLUECORE_HANDLER_H__ */
