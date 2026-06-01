/*
 *  Copyright (c) 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved..
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <mutex>
#include <atomic>
#include <queue>
#include "xm_glink_transport.h"
#include "xm_kp_transport.h"
#include "xm_async_fd_watcher.h"
#include "xm_ipc_if.h"
#include "xm_state_machine.h"
#include "timer.h"

#define XM_CP_DEFAULT_LOG_LVL         "0x11"
#define XM_BTFMCODEC_DEFAULT_LOG_LVL  "0x03"
#define LOSSLESS_USECASE_MULTIPLIER    1
#define GAMING_VBC_USECASE_MULTIPLIER  4
#define XM_FD_WATCHER_ENABLED          0
#define XM_WIFI_LIB_ENABLED            1
#define XM_XAC_ENABLED                 2
#define XM_MSG_QUEUE                   0
#define XM_STOP_THREAD                 1
namespace xpan {
namespace implementation {

typedef struct {
  std::mutex xm_lock;
  struct xm_state_machine xm_state;
  XmAudioBearerReq AudioBearerData;
  XmBearerPreferenceReq BearerPreferenceData;
  XmUseCaseReq UseCaseData;
  TwtParameters TwtParams;
  uint8_t adsp_state;
} __attribute__((packed)) XmAppData;

class XpanManager
{
  public:
   XpanManager();
   ~XpanManager();
   int Initialize(void);
   static std::shared_ptr <XpanManager> Get(void);
   int Deinitialize(bool);
   void NotifyCoPVer(uint8_t, uint8_t *);
   bool PostMessage(xm_ipc_msg_t *);
   bool CacheCpMessage(xm_ipc_msg_t *);
   bool EnableXpanModules(bool);
   void ReScheduleDrvStatus(bool);
   uint8_t* GetCopVersionInUse(void);
   uint8_t* GetCopVersionSupported(void);
   void FreeCopVersionSupported(void);
   void FreeCopVersionInUse(void);
  private:
   void XpanManagerMainThreadRoutine(void);
   void XmIpcMsgHandler(xm_ipc_msg_t *);
   static void AudioBearerTimeOut(union sigval);
   static void BearerPreferenceTimeOut(union sigval);
   void QhciPrepareAudioBearerReq(xm_ipc_msg_t *);
   void PrepareAudioBearerRsp(XmIpcEventId, xm_ipc_msg_t *);
   void BearerSwitchInd(xm_ipc_msg_t *);
   void QhciUnPrepareAudioBearerReq(xm_ipc_msg_t *);
   void UnPrepareAudioBearerRsp(XmIpcEventId, xm_ipc_msg_t *);
   void ConvertToXmStatus(uint8_t, RspStatus*);
   void NotifyLoglvl(void);
   void QhciUseCaseUpdate(xm_ipc_msg_t *);
   void XmWifiUseCaseUpdate(UseCaseType);
   void UpdateStats(bool, uint8_t);
   int GetStatsInterval(void);
   void BearerSwitchReq(xm_ipc_msg_t *);
   void BearerSwitchRsp(XmIpcEventId eventId, xm_ipc_msg_t *);
   void BearerPreferenceReq(xm_ipc_msg_t *);
   void TransportUpdate(xm_ipc_msg_t *msg);
   void GetCurrentUsecase(TransportType);
   void ProcessTwtEvent(xm_ipc_msg_t *);
   void TerminateTwt(void);

   xm_state_machine *xm_get_sm_ptr(void);
   XmAudioBearerReq *xm_get_audiobearer_data(void);
   XmBearerPreferenceReq *xm_get_bearer_preference_data(void);
   XmUseCaseReq *xm_get_usecase_data(void);
   void GetEbDetails(xm_ipc_msg_t *);
   void xm_set_usecase(UseCaseType);
   UseCaseType xm_get_usecase(void);
   void AdspStateUpdate(xm_ipc_msg_t *);
   void PortStateUpdate(xm_ipc_msg_t *msg);
   void CpIpcMsgHandler(xm_ipc_msg_t *);
   bool InitializeXpanModules(bool);
   void xm_set_adsp_state(uint8_t state);
   uint8_t xm_get_adsp_state(void);
   void AudioTransportUpdate(xm_ipc_msg_t *);
   void CacheTwtParams(xm_ipc_msg_t *);
   TwtParameters *xm_get_twtparams_data(void);
   void UseCaseStartInd(xm_ipc_msg_t *);
   void XmStaWifiUseCaseUpdate(xm_ipc_msg_t *);
   void WifiTransportSwitch(xm_ipc_msg_t *);
   void WifiTransportSwitchReq(uint8_t);
   void WifiBearerIndication(uint8_t, uint8_t);
   void WifiTransportSwitchRsp(XmIpcEventId, xm_ipc_msg_t *);
   void UpdateWifiDrvStatus(void);
   void Cp_Cop_Ver_Info_Init(void);
   static void XpRetryTimeOut(union sigval sig);

   /* Declared below as static to avoid mutiple references */
   std::mutex xm_wq_mtx;
   std::mutex cp_wq_mtx;
   std::mutex buff_xm_wq_mtx;
   std::queue <xm_ipc_msg_t *> xm_workqueue;
   std::queue <xm_ipc_msg_t *> buff_xm_workqueue;
   std::queue <xm_ipc_msg_t *> cp_workqueue;
   std::atomic_bool main_thread_running;
   static std::shared_ptr<XpanManager> main_instance;
   struct alarm_t *audio_bearer;
   struct alarm_t *bearer_preference_req;
   struct alarm_t *xp_retry;

   GlinkTransport *glink_transport;
   KernelProxyTransport *kp_transport;
   std::thread main_thread;
   int glink_fd;
   int kp_fd;
   int read_fd;
   int write_fd;
   XMAsyncFdWatcher fd_watcher_;
   XmAppData xm_cache;
   uint32_t xm_state;
};

} // namespace implementation
} // namespace xpan
