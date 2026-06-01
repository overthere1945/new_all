#ifndef CSR_QVSC_HANDLER_H__
#define CSR_QVSC_HANDLER_H__
/******************************************************************************
Copyright (c) 2016-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #2 $
******************************************************************************/


#include "csr_sched.h"
#include "csr_types.h"
#include "csr_result.h"
#include "csr_message_queue.h"
#include "csr_log_text_2.h"
#include "csr_list.h"
#include "csr_qvsc_prim.h"
#include "csr_qvsc_private_prim.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrQvscLto);
#define CSR_QVSC_LTSO_GENERAL           0
#define CSR_QVSC_LTSO_STATE             1

#define CSR_QVSC_HEADER_SIZE            0x03

#define CSR_QVSC_STATE_NULL             0
#define CSR_QVSC_STATE_IDLE             1
#define CSR_QVSC_STATE_REQ              2
#define CSR_QVSC_STATE_TLV              3
#ifdef CSR_DSPM_KCS_DOWNLOAD
#define CSR_QVSC_STATE_KCS              4
#endif

#define CSR_QVSC_RESPONSE_TIME_MAX      (1 * CSR_SCHED_SECOND)

typedef struct
{
    CsrUint8 *payload;
    CsrUint16 length;
    CsrUint16 index;
    CsrQvscTlvDnldConfig config;
} CsrQvscTlvData;

typedef struct
{
    CsrSchedQid phandle;
    CsrUint8 state;
    CsrQvscTlvData tlvData;
    CsrMessageQueueType *messageQueue;
    void *recvMsgP;
    CsrCmnList_t subsList;
    CsrQvscSubscrptionId subsIdCounter;
#ifdef CSR_FRW_INSTALL_TRANS_RECOVERY
    CsrSchedTid timerID;
#endif
} CsrQvscInstanceData;

typedef struct CsrQvscSubsListElemTag
{
    struct CsrQvscSubsListElemTag *next; /* must be 1st                 */
    struct CsrQvscSubsListElemTag *prev; /* must be 2nd                 */
    CsrSchedQid phandle;                 /* Subscribed application qid  */ 
    CsrQvscSubscrptionId subsId;         /* Subscription ID             */
    CsrUint8 patternLen;                 /* Subscription pattern length */
    CsrUint8 *pattern;                   /* Subscription pattern        */
} CsrQvscSubsListElement;

typedef struct CsrQvscUnsolicitedHciEventTag
{
    CsrBool processed;
    CsrUint8 dataLen;
    CsrUint8 *data;
} CsrQvscUnsolicitedHciEvent;

#define CSR_QVSC_CHANGE_STATE(_inst, _state)                                 \
    do                                                                       \
    {                                                                        \
        CSR_LOG_TEXT_INFO((CsrQvscLto,                                       \
                           CSR_QVSC_LTSO_STATE,                              \
                           "%d -> %d",                                       \
                           (_inst)->state,                                   \
                           (_state)));                                       \
        (_inst)->state = (_state);                                           \
    } while (0)

/* Add Subscription data to list */
#define CSR_QVSC_ADD_SUBS_ELEM(_list)                                        \
    ((CsrQvscSubsListElement *)CsrCmnListElementAddLast(&(_list),            \
                                                        sizeof(CsrQvscSubsListElement)))

/* Find subscibed event by subscription data  */
#define CSR_QVSC_FIND_SUBS_ELEM_BY_DATA(_list,_data)                         \
    ((CsrQvscSubsListElement *)CsrCmnListSearch(&(_list),                    \
                                                CsrQvscFindSubsElemByData,   \
                                                (void *)(_data)))

/* Find subscibed event and send CSR_QVSC_EVENT_IND */
#define CSR_QVSC_FIND_SUBS_ELEM_AND_SEND_IND(_list, _data)                   \
    (CsrCmnListIterate((CsrCmnList_t *) &(_list),                            \
                       CsrQvscFindSubsAndSendInd,                            \
                       (void *)(_data)))

/* Find subscibed event using subscription ID */
#define CSR_QVSC_FIND_SUBS_ELEM_BY_ID(_list,_id)                             \
    ((CsrQvscSubsListElement *)CsrCmnListSearch(&(_list),                    \
                                                CsrQvscFindSubsElemById,     \
                                                (void *)(_id)))

/* Remove subscription using phandle and subscription ID */
#define CSR_QVSC_REMOVE_SUBS_ELEM(_list,_data)                               \
    (CsrCmnListIterateAllowRemove(&(_list),                                  \
                                  CsrQvscFindSubsElemByHandleAndId,          \
                                  (void *)(_data)))


CsrUint8 CsrQvscTlvSegmentSend(CsrQvscTlvData *downloadInst);
void CsrQvscTlvDownloadInitiate(CsrQvscTlvData *downloadInst);
void CsrQvscTlvDownloadComplete(CsrQvscInstanceData *qvscInst, CsrResult result);

#ifdef CSR_DSPM_KCS_DOWNLOAD
void CsrQvscKcsDownloadComplete(CsrQvscInstanceData *qvscInst,
                                CsrResult result,
                                CsrUint16 kcsBundleId);
void CsrQvscKcsRemoveComplete(CsrQvscInstanceData *qvscInst, CsrResult result);
#endif /* CSR_DSPM_KCS_DOWNLOAD */

CsrBool CsrQvscTlvDownloadReqHandler(CsrQvscInstanceData *qvscInst, void *msg);

#ifdef CSR_DSPM_KCS_DOWNLOAD
CsrBool CsrQvscKcsDownloadReqHandler(CsrQvscInstanceData *qvscInst, void *msg);
CsrBool CsrQvscKcsRemoveReqHandler(CsrQvscInstanceData *qvscInst, void *msg);
#endif /* CSR_DSPM_KCS_DOWNLOAD */

CsrBool CsrQvscHciVendorSpecificEventIndHandler(CsrQvscInstanceData *qvscInst);

void CsrQvscPrimHandler(CsrQvscInstanceData *qvscInst);

void CsrQvscSendCommand(CsrUint16 ocf, void *payload, CsrUint8 payloadLength);
void CsrQvscRestore(CsrQvscInstanceData *qvscInst);

void CsrQvscCfmSend(CsrSchedQid phandle,
                    CsrUint8 *payload,
                    CsrUint16 payloadLength);
void CsrQvscTlvDownloadCfmSend(CsrSchedQid phandle, CsrResult result);

#ifdef CSR_DSPM_KCS_DOWNLOAD
void CsrQvscKcsDownloadCfmSend(CsrSchedQid phandle,
                               CsrResult result,
                               CsrUint16 kcsBundleId);
void CsrQvscKcsRemoveCfmSend(CsrSchedQid phandle, CsrResult result);
#endif /* CSR_DSPM_KCS_DOWNLOAD */

void CsrQvscSubscribeCfmSend(CsrSchedQid phandle,
                             CsrQvscSubscrptionId subsId,
                             CsrResult result);
void CsrQvscUnsubscribeCfmSend(CsrSchedQid phandle, CsrResult result);
void CsrQvscEventIndSend(CsrSchedQid phandle,
                         CsrQvscSubscrptionId subscriptionId,
                         CsrUint8 eventLength, 
                         CsrUint8* event);

CsrBool CsrQvscFindSubsElemById(CsrCmnListElm_t *elem, void *id);
CsrBool CsrQvscFindSubsElemByData(CsrCmnListElm_t *elem, void *data);
CsrBool CsrQvscFindSubsElemByHandleAndId(CsrCmnListElm_t *elem, void *data);
void CsrQvscFindSubsAndSendInd(CsrCmnListElm_t *elem, void *data);
void CsrQvscSubsListDeinit(CsrCmnListElm_t *elem);
CsrQvscSubscrptionId CsrQvscGenerateSubscriptionId(CsrQvscInstanceData *qvscInst);


void CsrQvscFreeDownstreamMessageContents(CsrUint16 eventClass, void *message);
void CsrQvscPrivateFreeDownstreamMessageContents(CsrUint16 eventClass, void *message);

#ifdef __cplusplus
}
#endif

#endif /* CSR_QVSC_HANDLER_H__ */

