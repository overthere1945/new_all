/*
 *  Copyright (c) 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved..
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <errno.h>
#include <utils/Log.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <cutils/properties.h>
#include <poll.h>
#include "xm_main.h"
#include "data_handler.h"
#include "xm_packetizer.h"

#ifdef XPAN_ENABLED
#include "xm_xprofile_if.h"
#include "xm_qhci_if.h"
#include "xm_wifi_if.h"
#include "xm_xac_if.h"
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "vendor.qti.xpan@1.0-xpanmanager"
#define MAX_SCENARIOS 8
#define COP_VER_INFO_LEN 17

using android::hardware::bluetooth::V1_0::implementation::DataHandler;

namespace xpan {
namespace implementation {

XMPacketizer packetizer;

#ifdef XPAN_ENABLED
XMXprofileIf profile_if;
XMQhciIf qhci_if;
XMWifiIf wifi_if;
XMXacIf xac_if;
char xpan_value[PROPERTY_VALUE_MAX] = {'\0'};
std::mutex xp_retry_mtx;
#endif

std::shared_ptr<XpanManager> XpanManager::main_instance = nullptr;
uint8_t *cp_cop_ver_info = NULL;
uint8_t *cp_cop_ver_in_use = NULL;
uint8_t *cp_cop_ver_supported = NULL;

XpanManager::XpanManager()
{
}

XpanManager::~XpanManager()
{
  ALOGD("%s", __func__);
}

std::shared_ptr<XpanManager> XpanManager::Get()
{
  if (!main_instance)
    main_instance.reset(new XpanManager());
  return main_instance;
}

#ifdef XPAN_ENABLED
xm_state_machine *XpanManager::xm_get_sm_ptr(void)
{
  return &xm_cache.xm_state;
}

XmAudioBearerReq *XpanManager::xm_get_audiobearer_data(void)
{
  return &xm_cache.AudioBearerData;
}

XmBearerPreferenceReq *XpanManager::xm_get_bearer_preference_data(void)
{
  return &xm_cache.BearerPreferenceData;
}

XmUseCaseReq *XpanManager::xm_get_usecase_data(void)
{
  return &xm_cache.UseCaseData;
}

TwtParameters *XpanManager::xm_get_twtparams_data(void)
{
  return &xm_cache.TwtParams;
}

void XpanManager::xm_set_usecase(UseCaseType usecase)
{
  xm_cache.UseCaseData.usecase = usecase;
}

UseCaseType XpanManager::xm_get_usecase(void)
{
  return xm_cache.UseCaseData.usecase;
}

/* This callback is fired either during audio bearer req
 * or unprepare audio bearer rsp.
 */ 
void XpanManager::AudioBearerTimeOut(union sigval sig)
{
  ALOGI("%s: triggered", __func__);
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  msg->AudioBearerTimeout.eventId = XM_PREPARE_AUDIO_BEARER_TIMEOUT;
  if (main_instance)
    main_instance->PostMessage(msg);
}

/* This callback is fired when bearer preference rsp from cp 
 * is delayed.
 */ 
void XpanManager::BearerPreferenceTimeOut(union sigval sig)
{
  ALOGI("%s: triggered", __func__);
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  msg->AudioBearerTimeout.eventId = XM_BEARER_PREFERENCE_TIMEOUT;
  if (main_instance)
    main_instance->PostMessage(msg);
}

/* This callback is fired when xp retry
 */
void XpanManager::XpRetryTimeOut(union sigval sig)
{
  ALOGI("%s: triggered", __func__);
  std::unique_lock<std::mutex> lck(xp_retry_mtx);
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  msg->AudioBearerTimeout.eventId = XP_XM_GET_WIFI_STATUS;
  if (main_instance)
    main_instance->PostMessage(msg);
}

void XpanManager::ReScheduleDrvStatus(bool type)
{
  static uint8_t count = 0;

  std::unique_lock<std::mutex> lck(xp_retry_mtx);
  /* type = true when xp is active
   * type = false when xp is not active
   */
  if (type == true) {
    if (xp_retry) {
      ALOGI("%s: deleting xp retry timer", __func__);
      delete_timer(xp_retry);
      xp_retry = NULL;
    }
    count = 0;
    return;
  }

  if (count <= 50 && xp_retry) {
    count ++;
    ALOGI("%s: starting xp reschedule timer", __func__);
    start_timer(xp_retry, XM_XP_RETRY_TIMEOUT);
  } else {
    count = 0;
    if (xp_retry) {
      ALOGI("%s: xp retry timer", __func__);
      delete_timer(xp_retry); 
      xp_retry = NULL;
    }
  }
}
#endif

void XpanManager::xm_set_adsp_state(uint8_t state)
{
  xm_cache.adsp_state = state;
}

uint8_t XpanManager::xm_get_adsp_state(void)
{
  return xm_cache.adsp_state;
}

#ifdef XPAN_ENABLED
bool XpanManager::EnableXpanModules(bool is_xpan_supported)
{
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return false;
  }
  msg->XpanInit.eventId = XM_INITIALIZE_XPAN_MODULES;
  msg->XpanInit.is_xpan_supported = is_xpan_supported;
  std::unique_lock<std::mutex> buff_wq_lck(buff_xm_wq_mtx);
  if (!PostMessage(msg))
  {
    ALOGI("%s adding XM_INITIALIZE_XPAN_MODULES to buffer queue", __func__);
    msg = (xm_ipc_msg_t *)malloc(XM_IPC_MSG_SIZE);
    msg->XpanInit.eventId = XM_INITIALIZE_XPAN_MODULES;
    msg->XpanInit.is_xpan_supported = is_xpan_supported;
    buff_xm_workqueue.push(msg);
  }
  buff_wq_lck.unlock();
  return true;
}

bool XpanManager::InitializeXpanModules(bool is_xpan_supported)
{
  bool status = false;

  if (is_xpan_supported) {
    /* Initialize XPAN provider AIDL */
    profile_if.Initialize();

    audio_bearer = create_timer("audio bearer", AudioBearerTimeOut);
    if (audio_bearer == NULL)
      goto update_status;

    bearer_preference_req = create_timer("Bearer Preference Req", BearerPreferenceTimeOut);
    if (bearer_preference_req == NULL)
      goto update_status;


    xp_retry = create_timer("XpRetryTimer", XpRetryTimeOut);
    if (xp_retry == NULL)
      goto update_status;

    wifi_if.InitWifiIf();
    SET_BIT(xm_state, XM_WIFI_LIB_ENABLED);

    memset(&xm_cache, 0, sizeof(XmAppData));
    /* Initialize XPAN Applications Controller */
    status = xac_if.Initialize();
    if (!status) {
      ALOGE("%s: failed to initialze xac", __func__);
      goto update_status;
    } else {
      SET_BIT(xm_state, XM_XAC_ENABLED);
    }
  } else {
    ALOGD("%s: XPAN prop not enabled, proceeding without initialzing wifi vendor lib",
        __func__);
  }
update_status:
  return status;
}
#endif

void XpanManager::XpanManagerMainThreadRoutine(void)
{
  DataHandler *data_handler = DataHandler::Get();
  bool status = false;

  {
    std::unique_lock<std::mutex> lck(xm_wq_mtx);
    int pipe_fds[2];
    if (pipe2(pipe_fds, O_NONBLOCK)) {
      ALOGE("%s: failed to create pipe", __func__);
      goto update_status;
    }

    read_fd = pipe_fds[0];
    write_fd = pipe_fds[1];
  }

  /*Initialize Cop Version Info*/
  Cp_Cop_Ver_Info_Init();

  /* Initialize Glink Channel */
  glink_transport = GlinkTransport::Get();
  if (!glink_transport) {
    goto update_status;
  } else {
    glink_fd = glink_transport->OpenGlinkChannel(TYPE_GLINK_CC);
    if (glink_fd < 0) {
      goto update_status;
    }
  }

  /* Initialize KPTransport */
  kp_transport = KernelProxyTransport::Get();
  if (!kp_transport) {
    goto update_status;
  } else {
    kp_fd = kp_transport->OpenKpTransport();
    if (kp_fd < 0) {
      goto update_status;
    }
  }

  status = true;

update_status:
  if (data_handler)
    data_handler->XMInitialized(status);
  else {
  ALOGE("%s: DataHandler is nullptr, returning from here", __func__);	  
  }

  /* Terminate main thread if status is set to false */
  if (!status)
    return;

  fd_watcher_.WatchFdForNonBlockingReads(glink_fd, kp_fd,
  [](int fd) {packetizer. OnDataReady(fd); });


  SET_BIT(xm_state, XM_FD_WATCHER_ENABLED);
  ALOGI("%s: xm_state %d", __func__, xm_state);

  std::unique_lock<std::mutex> buff_lck(buff_xm_wq_mtx);
  if (std::atomic_exchange(&main_thread_running, true)) return;
  if (buff_xm_workqueue.size()) {
    std::unique_lock<std::mutex> lck(xm_wq_mtx);
    // buffered requests should be given priority
    ALOGI("%s Adding buffered messages to xm_workqueue", __func__);
    while (xm_workqueue.size()) {
      buff_xm_workqueue.push(xm_workqueue.front());
      xm_workqueue.pop();
    }
    xm_workqueue.swap(buff_xm_workqueue);
    lck.unlock();
    uint8_t data = XM_MSG_QUEUE;
    TEMP_FAILURE_RETRY(write(write_fd, &data, 1));
  }
  buff_lck.unlock();
  NotifyLoglvl();

  while (main_thread_running) {
    struct pollfd pfd  = {0};
    pfd.fd = read_fd;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, -1);
    if (ret > 0) {
      if ((pfd.revents & POLLIN)) {
        char buffer = XM_MSG_QUEUE;
        TEMP_FAILURE_RETRY(read(read_fd, &buffer, 1));
        if (buffer == XM_MSG_QUEUE) {
          while(main_thread_running)
          {
            std::unique_lock<std::mutex> lck(xm_wq_mtx);
            if (xm_workqueue.empty()) {
              lck.unlock();
              break;
            } else {
              xm_ipc_msg_t *msg = xm_workqueue.front();
              xm_workqueue.pop();
              lck.unlock();
              XmIpcMsgHandler(msg);
            }
          }
        } else if (buffer == XM_STOP_THREAD) {
        ALOGI("%s: stopping xm thread", __func__);
       }
      }
    }
  }

  ALOGI("%s: is stopped", __func__);
  status = true;
  pthread_exit(&status);
}

int XpanManager::Deinitialize(bool is_xpan_supported)
{

  if (IS_BIT_SET(xm_state, XM_FD_WATCHER_ENABLED)) {
    fd_watcher_.StopWatchingFileDescriptors();
    CLEAR_BIT(xm_state, XM_FD_WATCHER_ENABLED);
  }

#ifdef XPAN_ENABLED
  {
    std::unique_lock<std::mutex> lck(xp_retry_mtx);
    if (xp_retry) {
      ALOGI("%s: xp retry timer", __func__);
      delete_timer(xp_retry);
      xp_retry = NULL;
    }
  }
#endif
  if (!std::atomic_exchange(&main_thread_running, false)) {
    ALOGW("%s: main thread already stopped", __func__);
  }
  ALOGI("%s clearing out pending message", __func__);
  {
    /* Unqueue all the pending messages */
    std::unique_lock<std::mutex> lck(xm_wq_mtx);
    uint8_t data = XM_STOP_THREAD;
    write(write_fd, &data, 1);
    while(!xm_workqueue.empty()) {
      xm_ipc_msg_t *msg = xm_workqueue.front();
      xm_workqueue.pop();
    }
  }
  pthread_t xm_thread_id = main_thread.native_handle();
  pthread_t this_thread_id = pthread_self();
  ALOGD("%s: xm_thread_id %u this_thread_id %u", __func__, xm_thread_id, this_thread_id);

  if (this_thread_id != xm_thread_id && main_thread.joinable()) {
   ALOGD("%s: joining main thread", __func__);
   main_thread.join();
   ALOGI("%s: joined main thread", __func__);
  } else if (this_thread_id == xm_thread_id) {
    ALOGW("%s: not cleaning up xm main thread as caller and calling TID are same", __func__);
  }

  /* acquire lock here to ensure no other threads are processing data
   * further
   */
  std::unique_lock<std::mutex> buff_lck(buff_xm_wq_mtx);
  std::unique_lock<std::mutex> lck(xm_wq_mtx);
  while(!xm_workqueue.empty()) {
    xm_ipc_msg_t *msg = xm_workqueue.front();
    xm_workqueue.pop();
  }
  while(!buff_xm_workqueue.empty()) {
    xm_ipc_msg_t *msg = buff_xm_workqueue.front();
    free(msg);
    buff_xm_workqueue.pop();
  }
#ifdef XPAN_ENABLED
  if (is_xpan_supported) {
    if (IS_BIT_SET(xm_state, XM_WIFI_LIB_ENABLED))
      wifi_if.DeInitWifiIf();
    profile_if.Deinitialize();
    if (IS_BIT_SET(xm_state, XM_XAC_ENABLED))
      xac_if.Deinitialize();
    if (audio_bearer) {
      ALOGI("%s: deleting audio bearer timer", __func__);
      delete_timer(audio_bearer);
    }
    if (bearer_preference_req) {
      ALOGI("%s: deleting bearer preference req timer", __func__);
      delete_timer(bearer_preference_req); 
    }
  }
#endif
 
  /* May be Rx thread might have started here */ 
  if (IS_BIT_SET(xm_state, XM_FD_WATCHER_ENABLED))
    fd_watcher_.StopWatchingFileDescriptors();
 
  if (glink_transport && glink_fd != -1) {
    glink_transport->CloseGlinkChannel(glink_fd);
    delete glink_transport;
  }

  if (kp_transport && kp_fd != -1) {
    kp_transport->CloseKpTransport(kp_fd);
    delete kp_transport;
  }

  /* Reset XM states */
  xm_state = 0;
  glink_fd = -1;
  kp_fd = -1;
  if (read_fd > 0)
    close(read_fd);
  if (write_fd > 0)
    close(write_fd);

  read_fd = -1;
  write_fd = -1;

  if (main_instance) {
    main_instance.reset();
    main_instance = NULL;
  }

  if (cp_cop_ver_info) {
    free(cp_cop_ver_info);
    cp_cop_ver_info = NULL;
  }


 return 0;
}

int XpanManager::Initialize(void)
{
  /* Reset XM states */
  xm_state = 0;
  glink_fd = -1;
  kp_fd = -1;
  read_fd = -1;
  write_fd = -1;
  main_thread = std::thread([this]() {
  ALOGD("%s: Started XPAN manager main thread", __func__);
  XpanManagerMainThreadRoutine();
  });
  if (!main_thread.joinable()) return -1;
  return 0;
}

void XpanManager::NotifyLoglvl(void)
{
  char value[PROPERTY_VALUE_MAX] = {'\0'};
  uint8_t val;
  ALOGI("%s", __func__);

  /* Update CP Log Lvl */
  {
    xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
    if( msg == NULL)
    {
      ALOGE("%s: nullptr", __func__);
      return;
    }
    property_get("persist.vendor.service.cpdata.loglvl", value, XM_CP_DEFAULT_LOG_LVL);
    ALOGI("%s: CP Data log level set to %s", __func__, value);
    val = (uint8_t)strtoul (value, NULL, 16);
    msg->Loglvl.eventId = XM_CP_LOG_LVL;
    msg->Loglvl.len = 1;
    msg->Loglvl.data = val;
    PostMessage(msg);
  }

  {
    xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
    if( msg == NULL)
    {
      ALOGE("%s: nullptr", __func__);
      return;
    }
    msg->Loglvl.eventId = XM_CP_LOG_LVL;
    msg->Loglvl.len = 1;
    msg->Loglvl.data = val;
    CacheCpMessage(msg);
  }

  /* Update btfmcodec Log Lvl */
  {
    xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
    if( msg == NULL)
    {
      ALOGE("%s: nullptr", __func__);
      return;
    }
    /* Below bit has to be set to enable respective logging.
     * Bit 0: Error message.
     * Bit 1: Warning message.
     * Bit 2: Debug message.
     * Bit 3: Info message.
     *
     * 0x08 is similar to 0x0F.
     * 0x04 is similar to 0x07.
     * 0x02 is similar to 0x03.
     *
     * 0x03 is default log level for BTFM Codec.
     */
    property_get("persist.vendor.service.btfmcodec.loglvl", value, XM_BTFMCODEC_DEFAULT_LOG_LVL);
    ALOGI("%s: btfmcodec log level set to %s", __func__, value);
    val = (uint8_t)strtoul (value, NULL, 16);
    msg->Loglvl.eventId = XM_KP_LOG_LVL;
    msg->Loglvl.len = 1;
    msg->Loglvl.data = val;
    PostMessage(msg);
  }
}

void XpanManager::NotifyCoPVer(uint8_t len, uint8_t *data)
{
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  uint8_t *cop_ver_info = (uint8_t *) malloc(len);

  if(!cop_ver_info) {
    ALOGE("%s: unable to create heap memory for cop_ver_info : %s (%d)", __func__,
        strerror(errno), errno);
    free(msg);
    return;
  }

  ALOGI("%s Rx CoP ver", __func__);

  memcpy(cop_ver_info, data, len);

  msg->CoPInd.eventId = DH_XM_COP_VER_IND;
  msg->CoPInd.len = len;
  msg->CoPInd.data = cop_ver_info;
  PostMessage(msg);

  /* Queue event to cache queue */
  {
    xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
    if( msg == NULL)
    {
      ALOGE("%s: nullptr", __func__);
      return;
    }

    if(!cp_cop_ver_info) {
      ALOGE("%s: unable to create heap memory for cp_cop_ver_info : %s (%d)",
        __func__, strerror(errno), errno);
      free(msg);
      return;
    }

    memcpy(cp_cop_ver_info, data, len);;

    msg->CoPInd.eventId = XM_CP_COP_VER_IND;
    msg->CoPInd.len = len;
    msg->CoPInd.data = cp_cop_ver_info;
    CacheCpMessage(msg);
  }
}

bool XpanManager::CacheCpMessage(xm_ipc_msg_t *msg)
{
  if (!main_thread_running) {
    ALOGW("%s: Main worker thread is not ready to process this message: %s",
          __func__, ConvertMsgtoString(msg->eventId));
    free(msg);
    return false;
  }

  cp_wq_mtx.lock();
  cp_workqueue.push(msg);
  cp_wq_mtx.unlock();

  return true;
}

bool XpanManager::PostMessage(xm_ipc_msg_t * msg)
{
  if (!main_thread_running) {
    ALOGW("%s: Main worker thread is not ready to process this message: %s",
          __func__, ConvertMsgtoString(msg->eventId));
    free(msg);
    return false;
  }

  xm_wq_mtx.lock();
  xm_workqueue.push(msg);
  xm_wq_mtx.unlock();
  
  uint8_t data = XM_MSG_QUEUE;
  if (TEMP_FAILURE_RETRY(write(write_fd, &data, 1)) < 0) {
    ALOGE("%s: failed to notify xm thread", __func__);
    return false;
  }
  return true;
}

#ifdef XPAN_ENABLED
void XpanManager::BearerSwitchReq(xm_ipc_msg_t *msg)
{
  XmIpcEventId eventId = msg->eventId;
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
  XmBearerPreferenceReq *BearerPreferenceData = xm_get_bearer_preference_data();
  xm_state_machine *xm_state = xm_get_sm_ptr();
  xm_sm_state current_state = xm_sm_get_current_state(xm_state);
  bdaddr_t bdaddr = xm_sm_get_current_active_device(xm_state);
  bool urgency = BearerPreferenceData->urgency;

  if (current_state == IDLE || current_state == XPAN_P2P_Disconnecting ||
      current_state == XPAN_AP_Disconnecting || current_state == BT_Disconnecting) {
    ALOGW("%s: audio might have stopped discarding this switch", __func__);
    return;
  }
  /* Check if this is valid timeout */
  if (eventId == XM_BEARER_PREFERENCE_TIMEOUT && BearerPreferenceData->orginator) {
    if (BearerPreferenceData->is_xpan_ap_roaming_started) {
      ALOGW("%s: notifying cp that switch failed due to timeout", __func__);
      packetizer.BearerSwitchRsp(XPAN_AP_PREP, XM_XP_FAILED);
      memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
      memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
      return;
    } else {
      ALOGW("%s: BearerPreference Timeout triggered", __func__);
      profile_if.BearerPreferenceRsp(bdaddr,
                BearerPreferenceData->reqtransport,
                XM_XP_BEARER_PREFERENCE_TIMEOUT,
                BearerPreferenceData->requestor);
      // Notify QHCI of failure as well.
      if (BearerPreferenceData->requestor == QHCI) {
          qhci_if.BearerPreferenceFailed(bdaddr, BearerPreferenceData->reqtransport,
            XM_XP_BEARER_PREFERENCE_TIMEOUT);
      }
      /* Notify WLAN either failed to switch or there is a
       * timeout to switch. This is by asking WLAN to switch
       * to previous transport.
       */ 
       uint8_t wlan_trans;
       if (BearerPreferenceData->reqtransport == BT_LE) 
       wlan_trans  = XPAN_P2P;
       else 
       wlan_trans  = BT_LE;
       WifiTransportSwitchReq(wlan_trans); 
       memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
       memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
       return;
    }
  } else if (eventId == XM_BEARER_PREFERENCE_TIMEOUT) {
      ALOGW("%s: BearerPreference Timeout triggered. No BearerPreferenceData", __func__);
      return;
  }

  TransportType type = msg->BearerSwitchReq.transport;
  /* first check if any BearerPreferenceReq is pending */
  if (BearerPreferenceData->orginator == XM_XP) {
    if (type != BearerPreferenceData->reqtransport) {
      ALOGW("%s: Req transport type by %s is %s and accepted transport type %s",
            __func__, OrginatorToString((uint8_t)BearerPreferenceData->orginator),
        TransportTypeToString(BearerPreferenceData->reqtransport),
        TransportTypeToString(type));
      profile_if.BearerPreferenceRsp(bdaddr, BearerPreferenceData->reqtransport,
            XM_XP_WRONG_TRANSPORT_TYPE_REQ,
            BearerPreferenceData->requestor);
     /* Notify WLAN either failed to switch or there is a
      * timeout to switch. This is by asking WLAN to switch
      * to previous transport.
      */ 
      uint8_t wlan_trans;
      if (BearerPreferenceData->reqtransport == BT_LE) 
       wlan_trans  = XPAN_P2P;
      else 
       wlan_trans  = BT_LE;

      WifiTransportSwitchReq(wlan_trans); 
    } else {
      if (BearerPreferenceData->orginator == XM_XP)
        profile_if.BearerPreferenceRsp(bdaddr, BearerPreferenceData->reqtransport,
            XM_SUCCESS, BearerPreferenceData->requestor);
      if (type == XPAN_AP_PREP) {
        ALOGD("%s: waiting for eb roaming complete notifications", __func__);
        BearerPreferenceData->is_xpan_ap_roaming_started = true;
        return;
      }
    }
    stop_timer(bearer_preference_req);;
  }


  /* Don't allow switch if requested transport is already inplace */
  if ((type == BT_LE && (current_state == BT_Connected ||
      current_state == BT_Connecting)) || (type == XPAN_P2P &&
      (current_state == XPAN_P2P_Connected || current_state == XPAN_P2P_Connecting)) ||
       (type == XPAN_AP && (current_state == XPAN_AP_Connecting ||
       current_state == XPAN_AP_Connected)) || current_state == IDLE) {
    ALOGW("%s: Requested transport is %s and current state is %s",
        __func__, TransportTypeToString(type), StateToString(current_state));
    if (BearerPreferenceData->orginator == XM_XP) {
      profile_if.BearerPreferenceRsp(bdaddr, BearerPreferenceData->reqtransport,
            (current_state ==
            IDLE ? XM_FAILED_AS_SEAMLESS_SWITCH_IS_NOT_ALLOWED:
            XM_FAILED_STATE_ALREADY_IN_REQUESTED_TRANSPORT),
            BearerPreferenceData->requestor);
    } else {
      packetizer.BearerSwitchRsp(type, XM_FAILED_STATE_ALREADY_IN_REQUESTED_TRANSPORT);
    }
   
     /* Notify WLAN either failed to switch or there is a
      * timeout to switch. This is by asking WLAN to switch
      * to previous transport.
      */ 
     uint8_t wlan_trans;
     if (BearerPreferenceData->reqtransport == BT_LE) 
       wlan_trans  = XPAN_P2P;
     else 
       wlan_trans  = BT_LE;
     
     WifiTransportSwitchReq(wlan_trans); 
     return;
  }

  ALOGI("%s: Req transport by CP is %s current_state is %s", __func__,
        TransportTypeToString(type), StateToString(current_state));

  if (type == BT_LE)
    xm_sm_set_current_state(xm_state, BT_Connecting);
  else if(type == XPAN_P2P)
    xm_sm_set_current_state(xm_state, XPAN_P2P_Connecting);
  else if(type == XPAN_AP)
    xm_sm_set_current_state(xm_state, XPAN_AP_Connecting);

  memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
  AudioBearerData->eventId = eventId;
  AudioBearerData->orginator = CP_To_XM;
  AudioBearerData->waiting_for_rsp |= 0x01 << XM_CP;
  AudioBearerData->waiting_for_rsp |= 0x01 << XM_QHCI;

  AudioBearerData->rx_bearer_ind = false;
  AudioBearerData->delayed_req = 0;
  AudioBearerData->type = type;
  AudioBearerData->requestor = BearerPreferenceData->requestor;
  memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));

  if (type == XPAN_AP) {
    start_timer(audio_bearer, XM_SEAMLESS_AUDIO_BEARER_AP_TIMEOUT);
  } else {
    start_timer(audio_bearer, XM_SEAMLESS_AUDIO_BEARER_TIMEOUT);
  }

  /* If request is to switch to BT, delay sending prepare audio
   * switch request to profile and kp.
   */
  if (type == BT_LE) {
    AudioBearerData->delayed_req |= 0x01 << XM_KP;
    AudioBearerData->delayed_req |= 0x01 << XM_XP;
    /* Delay PrepareAudioBearer Req to XAC  while moving from XPAN_AP to BT */
    if (current_state == XPAN_AP_Connecting || current_state == XPAN_AP_Connected)
      AudioBearerData->delayed_req |= 0x01 << XM_XAC;
    packetizer.PrepareAudioBearerReqtoCp(type);
  } else {
    AudioBearerData->waiting_for_rsp |= 0x01 << XM_XP;
    AudioBearerData->waiting_for_rsp |= 0x01 << XM_KP;
    UseCaseType usecase_type = xm_get_usecase();
    GetCurrentUsecase(type);
    if (usecase_type == USECASE_XPAN_NONE) {
      profile_if.UseCaseUpdate(bdaddr, xm_get_usecase());
    }
    profile_if.XmXpPrepareAudioBearerReq(bdaddr, type);
    xm_sm_state current_state = xm_sm_get_current_state(xm_state);
    if (current_state == XPAN_AP_Connecting || current_state == XPAN_AP_Connected) {
      AudioBearerData->waiting_for_rsp |= 0x01 << XM_XAC;
      xac_if.PrepareAudioBearerReq(bdaddr, type, urgency);
    }

    packetizer.PrepareAudioBearerReqtoCp(type);
    packetizer.PrepareAudioBearerReqtoKp(type);
    // At this point usecase is not active, donot send usecase to wifi module
    //UseCaseType current_usecase = xm_get_usecase();
    //XmWifiUseCaseUpdate(current_usecase);
  }

  qhci_if.PrepareAudioBearerReq(bdaddr, type, XM_To_QHCI);
}

void XpanManager::QhciPrepareAudioBearerReq(xm_ipc_msg_t *msg)
{
  XmIpcEventId eventId = msg->eventId;
  QhciXmPrepareAudioBearerReq AudioBearerReq = msg->AudioBearerReq;
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
  TransportType type = AudioBearerReq.type;
  xm_state_machine *xm_state = xm_get_sm_ptr();
  xm_sm_state current_state = xm_sm_get_current_state(xm_state);

  if (type == BT_LE && (current_state == BT_Connected ||
      current_state == BT_Connecting)) {
    ALOGW("%s: Requested transport by QHCI is %s and current state is %s",
        __func__, TransportTypeToString(type),
        StateToString(current_state));
    qhci_if.PrepareAudioBearerRsp(AudioBearerReq.bdaddr,
            XM_FAILED_STATE_ALREADY_IN_REQUESTED_TRANSPORT, XM_To_QHCI);
    return;
  } else if (type == XPAN_P2P && (current_state == XPAN_P2P_Connected ||
             current_state== XPAN_P2P_Connecting)) {
    ALOGW("%s: Requested transport by QHCI is %s and current state is %s",
          __func__, TransportTypeToString(type),
          StateToString(current_state));
    qhci_if.PrepareAudioBearerRsp(AudioBearerReq.bdaddr,
        XM_FAILED_STATE_ALREADY_IN_REQUESTED_TRANSPORT, XM_To_QHCI);
    return;
  } else if (type == XPAN_AP && (current_state == XPAN_AP_Connected ||
             current_state== XPAN_AP_Connecting)) {
    ALOGW("%s: Requested transport by QHCI is %s and current state is %s",
          __func__, TransportTypeToString(type),
          StateToString(current_state));
    qhci_if.PrepareAudioBearerRsp(AudioBearerReq.bdaddr,
        XM_FAILED_STATE_ALREADY_IN_REQUESTED_TRANSPORT, XM_To_QHCI);
    return;
  }

  ALOGI("%s: Req transport by QHCI is %s current_state is %s",
        __func__, TransportTypeToString(type),
        StateToString(current_state));

  if (type == BT_LE)
    xm_sm_set_current_state(xm_state, BT_Connecting);
  else if (type == XPAN_P2P)
    xm_sm_set_current_state(xm_state, XPAN_P2P_Connecting);
  else
    xm_sm_set_current_state(xm_state, XPAN_AP_Connecting);

  xm_sm_set_current_active_device(xm_state, AudioBearerReq.bdaddr);
  /* As of now there is no request can be sent from QHCI to prepare
   * Audio bearer req for BT. If this is added handle to send a delayed
   * message to XP.
   */
  AudioBearerData->eventId = QHCI_XM_PREPARE_AUDIO_BEARER_REQ;
  start_timer(audio_bearer, XM_AUDIO_BEARER_TIMEOUT);
 
  AudioBearerData->orginator = QHCI_To_XM;
  AudioBearerData->type = type;
  AudioBearerData->rx_bearer_ind = false;
  if (type == XPAN_AP) {
    AudioBearerData->waiting_for_rsp |= 0x01 << XM_XAC;
    AudioBearerData->waiting_for_rsp |= 0x01 << XM_XP;
    AudioBearerData->waiting_for_rsp |= 0x01 << XM_CP;
    AudioBearerData->waiting_for_rsp |= 0x01 << XM_KP;

    profile_if.ProcessMessage(eventId, msg);
    packetizer.ProcessMessage(eventId, msg);
    xac_if.PrepareAudioBearerReq(AudioBearerReq.bdaddr, type);
  } else if (type == XPAN_P2P) {
    AudioBearerData->waiting_for_rsp |= 0x01 << XM_WIFI;
    AudioBearerData->delayed_req |= 0x01 << XM_XP;
    AudioBearerData->delayed_req |= 0x01 << XM_CP;
    AudioBearerData->delayed_req |= 0x01 << XM_KP;
    WifiTransportSwitchReq((uint8_t)type);
  }
}

void XpanManager::ConvertToXmStatus(uint8_t pkt_from, RspStatus *status)
{
  if (pkt_from == XM_KP) {
   if (*status > XM_KP_MSG_INVALID)
     *status = (RspStatus) XM_KP_MSG_INVALID;
   else
     *status = (RspStatus)(XM_KP_ERROR_REASON_OFFSET + (*status));  
  }
}

int XpanManager::GetStatsInterval(void)
{
  XmUseCaseReq *UseCaseData = xm_get_usecase_data();
  UseCaseType usecase = xm_get_usecase();
  int interval;

  if (usecase == USECASE_XPAN_LOSSLESS)
    interval = LOSSLESS_USECASE_MULTIPLIER * (UseCaseData->twt_si / 1000);
  else if (usecase == USECASE_XPAN_GAMING)
    interval = GAMING_VBC_USECASE_MULTIPLIER * (UseCaseData->twt_si / 1000);
  else if (usecase == USECASE_XPAN_VBC)
    interval = GAMING_VBC_USECASE_MULTIPLIER * (UseCaseData->twt_si / 1000);
  else if (usecase == USECASE_XPAN_AP_LOSSLESS || USECASE_XPAN_AP_VOICE)
    interval = 100;
  else {
    ALOGE("%s:%s", __func__,UseCaseToString(usecase));
    interval = 0;
  }
  return interval;
}

void XpanManager::UpdateStats(bool enable, uint8_t interface_type)
{
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  msg->EnableStats.eventId = XM_WIFI_ENABLE_STATS;
  msg->EnableStats.enable = enable;
  msg->EnableStats.interval = enable ? GetStatsInterval(): 0;
  msg->EnableStats.interface_type = interface_type;
  ALOGI("%s: interval : %d on interface %s",__func__,  msg->EnableStats.interval,
    interface_type == 0 ? "XPAN SAP" : "XPAN STA");
  if (enable && msg->EnableStats.interval == 0) {
   ALOGI("%s: skip sending stats enable as interval is zero ",__func__);
   free(msg);
   return;
  }
  PostMessage(msg);
}

void XpanManager::BearerSwitchRsp(XmIpcEventId eventId, xm_ipc_msg_t *msg)
{
  RspStatus status = XM_SUCCESS;
  uint8_t pkt_from = XM_ORG_INVALID;
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
  xm_state_machine *xm_state = xm_get_sm_ptr();
  bdaddr_t bdaddr = xm_sm_get_current_active_device(xm_state);
  xm_sm_state current_state  = xm_sm_get_current_state(xm_state);
  TransportType type = AudioBearerData->type;

  if (current_state == BT_Connected || current_state == XPAN_P2P_Connected ||
      current_state == IDLE) {
    ALOGW("%s: discarding this event %s as state is :%s", __func__,
    ConvertMsgtoString(eventId), StateToString(current_state));
    memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
    return;
  }

  if (eventId == XP_XM_PREPARE_AUDIO_BEARER_RSP ||
      eventId == XAC_XM_PREPARE_AUDIO_BEARER_RSP) {
    status = msg->AudioBearerRsp.status;
    pkt_from = eventId == XP_XM_PREPARE_AUDIO_BEARER_RSP ? XM_XP : XM_XAC;
    ALOGI("%s: received prepare audio bearer rsp from %s with status:%s",
        __func__, pkt_from == XM_XP ? "profile": "XAC", StatusToString(status));
    if (status == XM_SUCCESS) {
      if (!memcmp((const char *)&bdaddr,
        (const char *)&msg->AudioBearerReq.bdaddr, sizeof(bdaddr_t))) {
        ALOGI("%s bd address matched:%s", __func__,
            ConvertRawBdaddress(bdaddr));  
        if (pkt_from == XM_XAC && type == XPAN_AP) {
          AudioBearerData->stats_enabled  = true;
          UpdateStats(true, 1);
        }
      } else {
        ALOGE("%s bdaddr mis-matched req generated on %s and req performed %s",
          __func__, ConvertRawBdaddress(bdaddr),
          ConvertRawBdaddress(msg->AudioBearerReq.bdaddr));
        status = XM_XP_PERFORMED_ON_WRONG_BD_ADDRESS;
      }
    }
  } else if (eventId == KP_XM_PREPARE_AUDIO_BEARER_RSP) {
    status = (RspStatus)msg->KpAudioBearerRsp.status;
    ALOGI("%s: received prepare audio bearer rsp from kp with status:%s",
        __func__, StatusToString(status));
    pkt_from = XM_KP;
  } else if (eventId == CP_XM_PREPARE_AUDIO_BEARER_RSP) {
    status = (RspStatus)msg->CpAudioBearerRsp.status;
    ALOGI("%s: received prepare audio bearer rsp from cp with status:%s",
    __func__, StatusToString(status));
    pkt_from = XM_CP;
    if (status == XM_SUCCESS) {
      if (type == XPAN_AP) {
        xac_if.InitiateLmpBearerSwitch(bdaddr, type);
      } else if (type == XPAN_P2P) {
        AudioBearerData->stats_enabled  = true;
        UpdateStats(true, 0);
      }
    }
  } else if (eventId == XM_PREPARE_AUDIO_BEARER_TIMEOUT) {
    ALOGI("%s: Recevied audio bearer timeout", __func__);
    status = XM_FAILED_DUE_TO_AUDIO_BEARER_TIMEOUT;
  } else if (eventId == XM_QHCI_PREPARE_AUDIO_BEARER_RSP) {
    status = msg->AudioBearerRsp.status;
    ALOGI("%s: Recevied prepare audio bearer rsp from Qhci with status:%s",
      __func__, StatusToString(status));
    pkt_from = XM_QHCI;
  }

  if (status == XM_SUCCESS) {
    AudioBearerData->waiting_for_rsp = (AudioBearerData->waiting_for_rsp &
                                  (~(0x01 << pkt_from)));
    if (type == BT_LE && !AudioBearerData->waiting_for_rsp && AudioBearerData->delayed_req) {
      if (IS_BIT_SET(AudioBearerData->delayed_req, XM_KP)) {
        ALOGI("%s: Rsp received from CP & QHCI modules. Now requesting for KP",
              __func__);
        CLEAR_BIT(AudioBearerData->delayed_req, XM_KP);
        AudioBearerData->waiting_for_rsp |= 0x01 << XM_KP;
        packetizer.PrepareAudioBearerReqtoKp(type);
     } else if (IS_BIT_SET(AudioBearerData->delayed_req, XM_XP)) {
        ALOGI("%s: Rsp received from CP,QHCI &KP modules. Now requesting for XP",
              __func__);
        CLEAR_BIT(AudioBearerData->delayed_req, XM_XP);
        AudioBearerData->waiting_for_rsp |= 0x01 << XM_XP;
        profile_if.XmXpPrepareAudioBearerReq(bdaddr, type);
     } else if (IS_BIT_SET(AudioBearerData->delayed_req, XM_XAC)) {
       ALOGI("%s: Rsp received from CP,QHCI,KP,XP modules. Now requesting for XAC",
              __func__);
        CLEAR_BIT(AudioBearerData->delayed_req, XM_XAC);
        AudioBearerData->waiting_for_rsp |= 0x01 << XM_XAC;
        xac_if.PrepareAudioBearerReq(bdaddr, type);
     }
    } else if (!AudioBearerData->waiting_for_rsp && !AudioBearerData->delayed_req) {
        packetizer.BearerSwitchRsp(type, XM_SUCCESS);
        AudioBearerData->bearer_switch_rsp = true;
    }
  } else {
    if (AudioBearerData->stats_enabled) {
      if (current_state == XPAN_P2P_Connecting) {
          XmWifiUseCaseUpdate(USECASE_XPAN_NONE);
          UpdateStats(false, 0);
      }
      else if (current_state == XPAN_AP_Connecting) {
          XmUseCaseReq *useCaseData = xm_get_usecase_data();
          CpRemoteApParams *Ind = &msg->CpRemoteParamsInd;
          msg->CpRemoteParamsInd.num_devices = 0;
          useCaseData->usecase = USECASE_XPAN_NONE;
          ALOGD("%s: Usecase %s ", __func__, UseCaseToString(useCaseData->usecase));
          XmStaWifiUseCaseUpdate(msg);
          free(Ind->EbParams);
          UpdateStats(false, 1);
      }
    }

    if (current_state == XPAN_P2P_Connecting || current_state == XPAN_AP_Connecting) {
      XmUseCaseReq *UseCaseData = xm_get_usecase_data();
      UseCaseType type = UseCaseData->usecase;
      ALOGI("%s: reverting correct usecase from %s to %s", __func__,
         UseCaseToString(type), UseCaseToString(UseCaseData->prev_usecase));
      UseCaseData->usecase = UseCaseData->prev_usecase;
    }
    memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
    /* Note- Bearer switch ind to go from XM to QHCI for failures and timeout */
    qhci_if.BearerSwitchInd(bdaddr, type, status);
    if (!AudioBearerData->bearer_switch_rsp)
      packetizer.BearerSwitchRsp(type, status);
    else
      packetizer.CpBearerSwitchInd(type, status);
    if (!IS_BIT_SET(AudioBearerData->delayed_req, XM_KP)) {
      CLEAR_BIT(AudioBearerData->delayed_req, XM_KP);
      if (status != XM_AC_AUDIO_SERVER_EXCEPTION) {
         packetizer.KpBearerSwitchInd(status);
      }
    }
    if (!IS_BIT_SET(AudioBearerData->delayed_req, XM_XP)) {
      CLEAR_BIT(AudioBearerData->delayed_req, XM_XP);
      profile_if.XmXpBearerSwitchInd(bdaddr, type, status);
    }
    if (!IS_BIT_SET(AudioBearerData->delayed_req, XM_XAC)) {
      CLEAR_BIT(AudioBearerData->delayed_req, XM_XAC);
      xac_if.BearerSwitchInd(bdaddr, type, status);
    }
    WifiBearerIndication(type, status);
    xm_sm_revert_current_state(xm_state);
    stop_timer(audio_bearer);
  }
}

void XpanManager::PrepareAudioBearerRsp(XmIpcEventId eventId, xm_ipc_msg_t *msg)
{
  RspStatus status = XM_SUCCESS;
  uint8_t pkt_from = XM_ORG_INVALID; 
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
  TransportType type = AudioBearerData->type;
  xm_state_machine *xm_state = xm_get_sm_ptr();
  xm_sm_state current_state  = xm_sm_get_current_state(xm_state);
  bdaddr_t bdaddr = xm_sm_get_current_active_device(xm_state);
  TransportType new_transport = AudioBearerData->type;

  if (current_state == BT_Connected || current_state == XPAN_P2P_Connected ||
      current_state == XPAN_AP_Connected || current_state == IDLE) {
    ALOGW("%s: discarding this event %s as state is :%s", __func__,
    ConvertMsgtoString(eventId), StateToString(current_state));
    return;
  }
  
  if (eventId == XP_XM_PREPARE_AUDIO_BEARER_RSP ||
      eventId == XAC_XM_PREPARE_AUDIO_BEARER_RSP) {
    status = msg->AudioBearerRsp.status;
    pkt_from = eventId == XP_XM_PREPARE_AUDIO_BEARER_RSP ? XM_XP : XM_XAC;
    ALOGI("%s: received prepare audio bearer rsp from %s with status:%s",
        __func__, pkt_from == XM_XP ? "profile": "XAC", StatusToString(status));
    if (status == XM_SUCCESS) {
      if (!memcmp((const char *)&bdaddr,
        (const char *)&msg->AudioBearerReq.bdaddr, sizeof(bdaddr_t))) {
        ALOGI("%s bd address matched:%s", __func__,
          ConvertRawBdaddress(bdaddr));  
        if (pkt_from == XM_XAC && new_transport == XPAN_AP) {
            AudioBearerData->stats_enabled  = true;
            UpdateStats(true, 1);
        }
      } else {
        ALOGE("%s bdaddr mis-matched req generated on %s and req performed %s",
          __func__, ConvertRawBdaddress(bdaddr),
          ConvertRawBdaddress(msg->AudioBearerReq.bdaddr));
        status = XM_XP_PERFORMED_ON_WRONG_BD_ADDRESS;
      }
    }
  } else if (eventId == KP_XM_PREPARE_AUDIO_BEARER_RSP) {
    status = (RspStatus)msg->KpAudioBearerRsp.status;
    ALOGI("%s: received prepare audio bearer rsp from kp with status:%s",
        __func__, StatusToString(status));
    pkt_from = XM_KP;
  } else if (eventId == CP_XM_PREPARE_AUDIO_BEARER_RSP) {
    status = (RspStatus)msg->CpAudioBearerRsp.status;
    ALOGI("%s: received prepare audio bearer rsp from cp with status:%s",
        __func__, StatusToString(status));
    pkt_from = XM_CP;
    if (status == XM_SUCCESS) {
        if (new_transport == XPAN_AP) {
           xac_if.InitiateLmpBearerSwitch(bdaddr, new_transport);
        }
      }
  } else if (eventId == XM_PREPARE_AUDIO_BEARER_TIMEOUT) {
    ALOGI("%s: Recevied audio bearer timeout", __func__);
    status = XM_FAILED_DUE_TO_AUDIO_BEARER_TIMEOUT;
  } else if (eventId == WIFI_XM_TRANSPORT_SWITCH_RSP) {
    status = (RspStatus)msg->WiFiBearerSwitchRsp.status; 
    ALOGI("%s: Recevied transport rsp from wifi with status %s",
        __func__, StatusToString(status));
    pkt_from = XM_WIFI;
  }

  if (status == XM_SUCCESS) {
    AudioBearerData->waiting_for_rsp = (AudioBearerData->waiting_for_rsp &
                                  (~(0x01 << pkt_from)));
    if (type == XPAN_P2P && !AudioBearerData->waiting_for_rsp && AudioBearerData->delayed_req) {
      ALOGI("%s: sending prepareaudioreq to other modules..", __func__);
      AudioBearerData->waiting_for_rsp |= 0x01 << XM_XP;
      AudioBearerData->waiting_for_rsp |= 0x01 << XM_CP;
      AudioBearerData->waiting_for_rsp |= 0x01 << XM_KP;
      profile_if.XmXpPrepareAudioBearerReq(bdaddr, type);
      packetizer.PrepareAudioBearerReqtoCp(type);
      packetizer.PrepareAudioBearerReqtoKp(type);
      /* clear this flag as no requests are pending */
      AudioBearerData->delayed_req = 0;
    }

    if (!AudioBearerData->waiting_for_rsp) {
      ALOGI("%s: Rsp received from all modules", __func__);
      if (AudioBearerData->orginator == QHCI_To_XM) {
        qhci_if.PrepareAudioBearerRsp(bdaddr, status, XM_To_QHCI);
        if (new_transport == XPAN_AP)
          AudioBearerData->rx_bearer_ind = true;
      } else {
        ALOGE("%s: No orginator...", __func__);
      }
    }
  } else {
    ConvertToXmStatus(pkt_from, &status);
    ALOGE("%s: Recevied prepare audio bearer rsp with failure status :%s",
    __func__, StatusToString(status));
    xm_sm_revert_current_state(xm_state);

    if (xm_sm_get_current_state(xm_state) == IDLE)
      xm_sm_set_current_active_device(xm_state, ACTIVE_BDADDR);

    stop_timer(audio_bearer);
    if (AudioBearerData->stats_enabled) {
        if (current_state == XPAN_P2P_Connecting) {
            XmWifiUseCaseUpdate(USECASE_XPAN_NONE);
            UpdateStats(false, 0);
        }
        else if (current_state == XPAN_AP_Connecting) {
            XmUseCaseReq *useCaseData = xm_get_usecase_data();
            CpRemoteApParams *Ind = &msg->CpRemoteParamsInd;
            msg->CpRemoteParamsInd.num_devices = 0;
            useCaseData->usecase = USECASE_XPAN_NONE;
            ALOGD("%s: Usecase %s ", __func__, UseCaseToString(useCaseData->usecase));
            XmStaWifiUseCaseUpdate(msg);
            free(Ind->EbParams);
            UpdateStats(false, 1);
        }
    }
    uint8_t waiting_for_rsp = AudioBearerData->waiting_for_rsp;
    /* waiting_for_rsp stands zero if prepare audio bearer rsp is received
     * from all the clients and XM notified to orginator, but Bearer ind
     * is not received from the XP. In that case update bearer ind to
     * orginator with failed status
     */
    if (waiting_for_rsp == 0) {
      ALOGE("%s: may be this is a timeout due to bearer indication", __func__);
      if (AudioBearerData->orginator == QHCI_To_XM) {
        qhci_if.BearerSwitchInd(bdaddr, type, status);
      }
      packetizer.KpBearerSwitchInd(status);
      packetizer.CpBearerSwitchInd(type, status);
      profile_if.XmXpBearerSwitchInd(bdaddr, type, status);
      if (new_transport == XPAN_AP)
        xac_if.BearerSwitchInd(bdaddr, new_transport, status);
    } else if (AudioBearerData->orginator == QHCI_To_XM) {
      if (type == XPAN_P2P && !AudioBearerData->delayed_req) {
        packetizer.CpBearerSwitchInd(type, status);
        profile_if.XmXpBearerSwitchInd(bdaddr, type, status);
        packetizer.KpBearerSwitchInd(status);
      }
      qhci_if.PrepareAudioBearerRsp(bdaddr, status, XM_To_QHCI);
      if (new_transport == XPAN_AP)
        xac_if.BearerSwitchInd(bdaddr, new_transport, status);
    } else {
      ALOGE("%s: No orginator...", __func__);
    }

    ALOGI("Updated failure status %s to all clients", __func__);
    memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
    return;
  }

  /* we reach below block if profile has responded with audio bearer indications
   * and prepare audio bearer rsp is delayed from other clients. If respone
   * received before timeout. Update AudioBearer ind clients with success
   * status as we reach here if only profile notified audio bearer ind with
   * status success.
   */
  if (!AudioBearerData->waiting_for_rsp && AudioBearerData->rx_bearer_ind) {
    ALOGI("%s: Rx audio bearer rsp from all the waiting clients", __func__);
    ALOGI("%s: Rx AudioBearer indcation too, so updating now to all clients",
    __func__);
    packetizer.KpBearerSwitchInd(XM_SUCCESS);
    packetizer.CpBearerSwitchInd(type, XM_SUCCESS);
    qhci_if.BearerSwitchInd(bdaddr, type, XM_SUCCESS);
    WifiBearerIndication(type, XM_SUCCESS);
    memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
    stop_timer(audio_bearer);
    xm_sm_move_to_next_state(xm_state);
  }
}

void XpanManager::BearerSwitchInd(xm_ipc_msg_t *msg)
{
  RspStatus status = msg->XpBearerSwitchInd.status;
  xm_state_machine *xm_state = xm_get_sm_ptr();
  bdaddr_t bdaddr = xm_sm_get_current_active_device(xm_state);
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
  TransportType type = AudioBearerData->type;
  xm_sm_state current_state  = xm_sm_get_current_state(xm_state);
  uint8_t waiting_for_rsp = AudioBearerData->waiting_for_rsp;

  if (current_state == BT_Connected || current_state == XPAN_P2P_Connected ||
      current_state == IDLE) {
    ALOGW("%s: discarding this event %s as state is :%s", __func__,
        ConvertMsgtoString(msg->eventId), StateToString(current_state));
    return;
  }

  if(AudioBearerData->eventId != QHCI_XM_PREPARE_AUDIO_BEARER_REQ) {
    ALOGW("%s: this indication is not expected discarding", __func__);
    return;
  }

  ALOGI("%s: profile notified with status :%s", __func__, StatusToString(status));

  AudioBearerData->rx_bearer_ind = true;

  if (!memcmp((const char *)&bdaddr, (const char *)&msg->XpBearerSwitchInd.bdaddr, sizeof(bdaddr_t))) {
    ALOGI("%s bd address matched:%s\n", __func__, ConvertRawBdaddress(bdaddr));
  } else {
    ALOGE("%s bdaddress mis-matched req generated on %s and req performed %s",
          __func__, ConvertRawBdaddress(bdaddr),
          ConvertRawBdaddress(msg->AudioBearerReq.bdaddr));
    status = XM_XP_PERFORMED_ON_WRONG_BD_ADDRESS;
  }

  if (status == XM_SUCCESS) {
    if (!waiting_for_rsp) {
      ALOGI("%s: stoping audio bearer timer as all modules notified with rsp",
        __func__);
      stop_timer(audio_bearer);
      xm_sm_move_to_next_state(xm_state);
    } else {
      if (IS_BIT_SET(waiting_for_rsp, XM_CP))
        ALOGW("%s: Waiting for prepare audio bearer rsp from CP", __func__);
      if (IS_BIT_SET(waiting_for_rsp, XM_KP))
        ALOGW("%s: Waiting for prepare audio bearer rsp from KP", __func__);
      if (IS_BIT_SET(waiting_for_rsp, XM_QHCI))
        ALOGW("%s: Waiting for prepare audio bearer rsp from QHCI", __func__);
      return;
    }
  } else {
    ALOGI("%s: stoping audio bearer timer", __func__);
    stop_timer(audio_bearer);
    xm_sm_revert_current_state(xm_state);
    if (AudioBearerData->stats_enabled) {
      if (current_state == XPAN_P2P_Connecting) {
          XmWifiUseCaseUpdate(USECASE_XPAN_NONE);
          UpdateStats(false, 0);
      }
      else {
          XmUseCaseReq *useCaseData = xm_get_usecase_data();
          CpRemoteApParams *Ind = &msg->CpRemoteParamsInd;
          msg->CpRemoteParamsInd.num_devices = 0;
          useCaseData->usecase = USECASE_XPAN_NONE;
          ALOGD("%s: Usecase %s ", __func__, UseCaseToString(useCaseData->usecase));
          XmStaWifiUseCaseUpdate(msg);
          free(Ind->EbParams);
          UpdateStats(false, 1);
      }
    }
  }

  packetizer.KpBearerSwitchInd(status);
  packetizer.CpBearerSwitchInd(type, status);
  qhci_if.BearerSwitchInd(bdaddr, type, status);
  WifiBearerIndication(type, status);
  memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
  return;
}

void XpanManager::TransportUpdate(xm_ipc_msg_t *msg)
{
  XmIpcEventId eventId = msg->eventId;
  TransportType type = msg->TransportUpdate.transport;
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
  xm_state_machine *xm_state = xm_get_sm_ptr();
  xm_sm_state current_state  = xm_sm_get_current_state(xm_state);
  xm_sm_state prev_state  = xm_sm_get_prev_state(xm_state);
  bdaddr_t bdaddr = xm_sm_get_current_active_device(xm_state);

  if(AudioBearerData->eventId == QHCI_XM_PREPARE_AUDIO_BEARER_REQ ||
     AudioBearerData->eventId == QHCI_XM_UNPREPARE_AUDIO_BEARER_REQ) {
    ALOGW("%s: this indication is not expected discarding", __func__);
    return;
  }

  if (eventId == XAC_XM_TRANSPORT_UPDATE && !AudioBearerData->waiting_for_xac_transport_update) {
    ALOGW("%s: discarding unexpected event from xac", __func__);
    return;
  }

  if (current_state == IDLE) {
    ALOGW("%s: Might have moved to idle start discarding it", __func__);
    return;
  } else if (!AudioBearerData->orginator) {
    ALOGW("%s: No orginator this can due to timeout or pkt not expected now", __func__);
    return;
  }

  if (type != AudioBearerData->type) {
    ALOGI("%s: current transport used %s and requested transport %s",
          __func__, TransportTypeToString(type),
        TransportTypeToString(AudioBearerData->type));
    xm_sm_revert_current_state(xm_state);
    qhci_if.BearerSwitchInd(bdaddr, AudioBearerData->type, XM_CP_FAILED_TO_SWITCH_BEARER);
    packetizer.KpBearerSwitchInd(XM_CP_FAILED_TO_SWITCH_BEARER);
    profile_if.XmXpBearerSwitchInd(bdaddr, AudioBearerData->type, XM_CP_FAILED_TO_SWITCH_BEARER);
    if (type == XPAN_AP || prev_state == XPAN_AP_Connected)
      xac_if.BearerSwitchInd(bdaddr, AudioBearerData->type,
                             XM_CP_FAILED_TO_SWITCH_BEARER);
    WifiBearerIndication(AudioBearerData->type, XM_CP_FAILED_TO_SWITCH_BEARER);
    stop_timer(audio_bearer);
    memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
    return;
  }

  ALOGI("%s: Requested transport is %s and current state is %s",
        __func__, TransportTypeToString(type), StateToString(current_state));

  if ((type == BT_LE && (current_state == BT_Connecting)) ||
     (type == XPAN_P2P && (current_state == XPAN_P2P_Connecting)) ||
     (type == XPAN_AP && (current_state == XPAN_AP_Connecting))) {
    if (!AudioBearerData->waiting_for_rsp && !AudioBearerData->delayed_req) {
      ALOGI("%s: Informing all waiting clients that transport is active", __func__);
      /* For LE->AP or AP->LE, Bearer Switch indication is sent from AC to qhci after unpause*/
      if (type == XPAN_AP || prev_state == XPAN_AP_Connected) {
	/* Send XAC about transport as unpause has to be triggered before
	 * we send any message on gatt.
	 */
	  if (!AudioBearerData->waiting_for_xac_transport_update) {
            ALOGI("%s: Informing xac that transport is active", __func__);
	    AudioBearerData->waiting_for_xac_transport_update = true;
            xac_if.BearerSwitchInd(bdaddr, AudioBearerData->type, XM_SUCCESS);
	    return;
          }
      } else {
        ALOGI("%s: Informing qhci that transport is active", __func__);
        qhci_if.BearerSwitchInd(bdaddr, type, XM_SUCCESS);
      }
      packetizer.KpBearerSwitchInd(XM_SUCCESS);
      profile_if.XmXpBearerSwitchInd(bdaddr, type, XM_SUCCESS);
      WifiBearerIndication(type, XM_SUCCESS);
      memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
      /* Tear down TWT once we move to BT_connected */
      if (prev_state == XPAN_P2P_Connected && current_state == BT_Connecting) {
        if (profile_if.GetTwtPropStatus() == TWT_CONFIG_ENABLE)
          TerminateTwt();
      } else if (prev_state == XPAN_AP_Connected && current_state == BT_Connecting) {
          XmUseCaseReq *useCaseData = xm_get_usecase_data();
          CpRemoteApParams *Ind = &msg->CpRemoteParamsInd;
          msg->CpRemoteParamsInd.num_devices = 0;
          useCaseData->usecase = USECASE_XPAN_NONE;
          ALOGD("%s: Usecase %s ", __func__, UseCaseToString(useCaseData->usecase));
          XmStaWifiUseCaseUpdate(msg);
          free(Ind->EbParams);
          UpdateStats(false, 1);
      }
      xm_sm_move_to_next_state(xm_state);
      stop_timer(audio_bearer);
      return;
    } else {
      ALOGE("%s: is not expected at this time. waiting_for_rsp %d delayed_req %d",
        __func__, AudioBearerData->waiting_for_rsp, AudioBearerData->delayed_req);
      return;
    }
  } else {
    ALOGE("%s: We might have moved to a connected state", __func__);
  }
}

void XpanManager::BearerPreferenceReq(xm_ipc_msg_t *msg)
{
  XmIpcEventId eventId = msg->eventId;
  TransportType type = NONE;
  xm_state_machine *xm_state = xm_get_sm_ptr();
  xm_sm_state current_state = xm_sm_get_current_state(xm_state);
  XmBearerPreferenceReq *BearerPreferenceData = xm_get_bearer_preference_data();
  bdaddr_t  req_bdaddr;
  bdaddr_t active_bdaddr = xm_sm_get_current_active_device(xm_state);
  uint8_t requestor = msg->BearerPreference.requestor;
  uint8_t reason = msg->BearerPreference.reason;
  bool urgency = msg->BearerPreference.urgency;
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();

  if (BearerPreferenceData->orginator != 0 &&
      BearerPreferenceData->is_xpan_ap_roaming_started) {
    RspStatus status;
    type = (TransportType)msg->BearerPreference.transport;
    if (type == XPAN_AP) {
      memcpy(&req_bdaddr, &msg->BearerPreference.bdaddr, sizeof(bdaddr_t));
      ALOGD("%s: eb notified that roaming is completed asking xac for ip notification", __func__);
      BearerPreferenceData->is_xpan_ap_roaming_completed = true;
      xac_if.FetchAndUpdateIpDetailstoCp(req_bdaddr,
          XAC_CP_REMOTE_PARAMS_IND_DURING_ROAMING, NULL);
      status = XM_SUCCESS;
      profile_if.BearerPreferenceRsp(msg->BearerPreference.bdaddr,
		                   BearerPreferenceData->reqtransport,
                                   status, BearerPreferenceData->requestor);
    } else {
      ALOGD("%s: eb notified with different transport and type %s",
	    __func__, TransportTypeToString(type));
      status = XM_FAILED_AS_SEAMLESS_SWITCH_IS_NOT_ALLOWED;
      packetizer.BearerSwitchRsp(XPAN_AP_PREP, status);
      profile_if.BearerPreferenceRsp(msg->BearerPreference.bdaddr,
		                   BearerPreferenceData->reqtransport,
                                   status, BearerPreferenceData->requestor);
      stop_timer(bearer_preference_req);
      memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
    }
    return;
  }

  if (BearerPreferenceData->orginator != 0) {
    ALOGW("%s Not allowing this request as already XM is processing other", __func__);
    if (eventId == XP_XM_BEARER_PREFERENCE_REQ) {
      profile_if.BearerPreferenceRsp(msg->BearerPreference.bdaddr,
                 (TransportType)msg->BearerPreference.transport,
                 XM_XP_WRONG_TRANSPORT_TYPE_REQ, requestor);
      return;
    }
  }

  if (eventId == XP_XM_BEARER_PREFERENCE_REQ) {
    type = (TransportType)msg->BearerPreference.transport;
    memcpy(&req_bdaddr, &msg->BearerPreference.bdaddr, sizeof(bdaddr_t));

    if (type >= NONE) {
      ALOGE("%s: Invalid transport type req by profile", __func__);
      profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_XP_WRONG_TRANSPORT_TYPE_REQ,
           requestor);
      return;
    }

    if (type == XPAN_AP) {
      // Check with XAC if IP details are available before accepting the request */
      if (!xac_if.CheckIpDetailsAvailibility(req_bdaddr, type, urgency)) {
        profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_AC_BEARER_PREFERENCE_REJECTED,
           requestor);
        return;
      }
    }

    if ((type == BT_LE || type == XPAN_AP) && (current_state == IDLE)) {
      ALOGD("%s: sending to AC - IDLE case", __func__);
      if (!xac_if.isTransportReadyForSwitch(req_bdaddr)) {
        ALOGE("%s: XAC is in transition state. Reject bearer preference", __func__);
        profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_XP_FAILED, requestor);
        return;
      }
      bool status;
      status = xac_if.BearerPreferenceReq(req_bdaddr, type, urgency);
      if ((type == BT_LE) && !status)
        profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_XP_FAILED, requestor);
      else
        profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_SUCCESS, requestor);
      return;
    } else if ((type == XPAN_AP) && (current_state == XPAN_AP_Connecting)) {
      ALOGD("%s: sending to AC - Streaming case(urgency update)", __func__);
      xac_if.BearerPreferenceReq(req_bdaddr, type, urgency);
      profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_SUCCESS, requestor);
      return;
    } else if (current_state == IDLE && type == XPAN_AP_PREP) {
      ALOGD("%s: Not expected this from xp", __func__);
      profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_SUCCESS, requestor);
      return;
    } else if ((type == BT_LE) && (current_state == XPAN_AP_Connected)) {
        if (!qhci_if.isLeLinkconnected(req_bdaddr) &&
            qhci_if.isStreamingActive(req_bdaddr)) {
          ALOGD("%s: LE ACL not connected, rejecting Bearer Preference", __func__);
          profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_XP_FAILED, requestor);
          return;
        }
    }
    if ((type == BT_LE) && ((current_state == XPAN_P2P_Connecting) ||
            (current_state == XPAN_P2P_Connected) ||
            (current_state == XPAN_AP_Connecting) ||
            (current_state == XPAN_P2P_Connected))) {
        if (!qhci_if.isLeTransportReadyForSwitch()) {
          ALOGD("%s:  LE Transport not ready for switch", __func__);
          profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_XP_FAILED, requestor);
          return;
        }
    }
    if ((type == BT_LE || type == XPAN_AP) &&
        (current_state != XPAN_P2P_Connected && current_state != XPAN_P2P_Connecting)) {
      if (!xac_if.isTransportReadyForSwitch(req_bdaddr)) {
        ALOGE("%s: XAC is in transition state. Reject bearer preference", __func__);
        profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_XP_FAILED, requestor);
        return;
      }
    }

    if (memcmp((const char *)&active_bdaddr, (const char *)&req_bdaddr,
      sizeof(bdaddr_t))) {
      ALOGE("%s BD address mismatched req device %s active device %s", __func__,
          ConvertRawBdaddress(req_bdaddr),
          ConvertRawBdaddress(active_bdaddr));

      if (type == XPAN_AP) {
        ALOGI("%s: Notifying xac", __func__);
        BearerPreferenceData->orginator = XM_XP;
        xac_if.BearerPreferenceReq(req_bdaddr, type);
        profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_SUCCESS, requestor);
      } else {
        profile_if.BearerPreferenceRsp(req_bdaddr, type, XM_XP_WRONG_BDADDRESS, requestor);
      }
      return;
    }

    BearerPreferenceData->orginator = XM_XP;
  }

  ALOGI("Rx %s for transport_type %s from %s", __func__,
        TransportTypeToString(type),
    (requestor == REQUESTOR_EB) ? "Earbud": "WiFi");

  /* Don't accept switch if it in process of switching to some transport or
   * Don't switch if transport is already in place.
   */
  if ((type == BT_LE && (current_state == BT_Connected || current_state == BT_Connecting)) ||
      (type == XPAN_P2P && (current_state == XPAN_P2P_Connected || current_state== XPAN_P2P_Connecting)) ||
      (type == XPAN_AP && (current_state == XPAN_AP_Connected || current_state == XPAN_AP_Connecting)) ||
       (current_state == IDLE || (current_state == BT_Connecting || current_state == XPAN_P2P_Connecting ||
    current_state == XPAN_AP_Connecting))) {
    ALOGW("%s: Requested transport by %s is %s and current state is %s",
    __func__, OrginatorToString(BearerPreferenceData->orginator),
    TransportTypeToString(type), StateToString(current_state));
    RspStatus status;
    if (current_state == IDLE || current_state == BT_Connecting ||
    current_state == XPAN_P2P_Connecting || current_state == XPAN_AP_Connecting)
      status = XM_FAILED_AS_SEAMLESS_SWITCH_IS_NOT_ALLOWED;
    else 
      status = XM_FAILED_STATE_ALREADY_IN_REQUESTED_TRANSPORT;
    profile_if.BearerPreferenceRsp(req_bdaddr, type, status, requestor);
    memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
    return;
  } else if ((requestor == REQUESTOR_WLAN && current_state == XPAN_AP_Connected && type == XPAN_P2P) ||
        (requestor == REQUESTOR_WLAN && current_state == XPAN_AP_Connected && type == BT_LE &&
    reason != 1)) {
    ALOGW("%s: rejecting this Request transport by %s is %s and current state is %s",
    __func__, OrginatorToString(BearerPreferenceData->orginator),
    TransportTypeToString(type), StateToString(current_state));
    RspStatus status;
    if (current_state == IDLE || current_state == BT_Connecting ||
    current_state == XPAN_P2P_Connecting || current_state == XPAN_AP_Connecting)
      status = XM_FAILED_AS_SEAMLESS_SWITCH_IS_NOT_ALLOWED;
    else 
      status = XM_FAILED_STATE_ALREADY_IN_REQUESTED_TRANSPORT;
    profile_if.BearerPreferenceRsp(req_bdaddr, type, status, requestor);
    memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
    return;
  }

  if (reason == 1) {
    ALOGW("%s: special case allowing AP -> LE transition as SAP is terminating", __func__);
  }

  start_timer(bearer_preference_req, XM_BEARER_PREF_TIMEOUT);
  BearerPreferenceData->reqtransport = type;
  BearerPreferenceData->requestor = requestor;
  BearerPreferenceData->urgency = urgency;

  memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));

  if (type == XPAN_AP_PREP) {
    packetizer.ProcessMessage(eventId ,msg);
    return;
  }

  AudioBearerData->eventId = XP_XM_BEARER_PREFERENCE_REQ;
  if (requestor ==  REQUESTOR_WLAN || type == BT_LE || type == XPAN_AP)
    packetizer.ProcessMessage(eventId ,msg);
  else
    WifiTransportSwitchReq(type); 
}

void XpanManager::WifiTransportSwitchRsp(XmIpcEventId eventId , xm_ipc_msg_t *msg)
{
  RspStatus status = XM_SUCCESS;
  xm_state_machine *xm_state = xm_get_sm_ptr();
  bdaddr_t active_bdaddr = xm_sm_get_current_active_device(xm_state);
  XmBearerPreferenceReq *BearerPreferenceData = xm_get_bearer_preference_data();
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();

  memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
  
  status = (RspStatus)msg->WiFiBearerSwitchRsp.status; 
  ALOGI("%s: Recevied transport rsp from wifi with status %s",
      __func__, StatusToString(status));
  if (status == XM_SUCCESS) {
    TransportType type = BearerPreferenceData->reqtransport;
    packetizer.BearerPreferenceInd(type);
  } else {
    TransportType type = BearerPreferenceData->reqtransport;
    uint8_t requestor = BearerPreferenceData->requestor;
    profile_if.BearerPreferenceRsp(active_bdaddr, type, XM_WIFI_REJECTED_TRANSPORT_SWITCH, requestor);
    memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
    stop_timer(bearer_preference_req);
  }
}

void XpanManager::WifiTransportSwitchReq(uint8_t transport_type)
{
  
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  msg->eventId = XM_WIFI_TRANSPORT_SWITCH_REQ;
  msg->WiFiTransportSwitch.transport_type = transport_type;
  msg->WiFiTransportSwitch.status = WIFI_TRANS_SWITCH_STATUS_REQUEST;
  wifi_if.PostMessage(msg);
}

/* Wifi require only transport switch completion message
 * In failure cases, it is not required to send request to
 * switch to old transport.
 */
void XpanManager::WifiBearerIndication(uint8_t transport_type, uint8_t status)
{
  
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  msg->eventId = XM_WIFI_TRANSPORT_SWITCH_COMPLETED;
  msg->WiFiTransportSwitch.transport_type = transport_type;
  if (status == XM_SUCCESS)
    msg->WiFiTransportSwitch.status = WIFI_TRANS_SWITCH_STATUS_COMPLETED;
  else
    msg->WiFiTransportSwitch.status = WIFI_TRANS_SWITCH_STATUS_REJECTED;

  wifi_if.PostMessage(msg);
}

void XpanManager::QhciUnPrepareAudioBearerReq(xm_ipc_msg_t *msg)
{
  XmIpcEventId eventId = msg->eventId;
  QhciXmUnPrepareAudioBearerReq UnPrepareAudioBearerReq = msg->UnPrepareAudioBearerReq;
  TransportType type = UnPrepareAudioBearerReq.type;
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
  xm_state_machine *xm_state = xm_get_sm_ptr();
  xm_sm_state current_state = xm_sm_get_current_state(xm_state);
  xm_sm_state prev_state = xm_sm_get_prev_state(xm_state);
  bdaddr_t bdaddr = xm_sm_get_current_active_device(xm_state);
  XmBearerPreferenceReq *BearerPreferenceData = xm_get_bearer_preference_data();

  ALOGW("%s: Requested transport by QHCI is %s and current state is %s",
        __func__, TransportTypeToString(type), StateToString(current_state));

  if (type != NONE) {
    qhci_if.UnPrepareAudioBearerRsp(UnPrepareAudioBearerReq.bdaddr,
            XM_FAILED_REQUSTED_WRONG_TRANSPORT_TYPE);
    return;
  } else if (current_state == IDLE) {
    qhci_if.UnPrepareAudioBearerRsp(UnPrepareAudioBearerReq.bdaddr,
            XM_NOTALLOWING_UNPREPARE_AS_STATE_IS_IDLE);
    return;
  }

  if (memcmp((const char *)&bdaddr, (const char *)&UnPrepareAudioBearerReq.bdaddr,
      sizeof(bdaddr_t))) {
    ALOGE("%s BD address mismatched req device %s active device %s", __func__,
          ConvertRawBdaddress(UnPrepareAudioBearerReq.bdaddr),
          ConvertRawBdaddress(bdaddr));
    qhci_if.UnPrepareAudioBearerRsp(UnPrepareAudioBearerReq.bdaddr,
           XM_FAILED_REQUSTED_WRONG_BDADDRESS);
    return;
  }
  if (BearerPreferenceData->orginator) {
      ALOGI("%s ongoing bearer Preference req from %s with transport %s", __func__,
        (BearerPreferenceData->requestor == REQUESTOR_EB) ? "Earbud": "WiFi",
        TransportTypeToString(BearerPreferenceData->reqtransport));
      profile_if.BearerPreferenceRsp(bdaddr,  BearerPreferenceData->reqtransport,
        XM_FAILED, BearerPreferenceData->requestor);
  }

  /* Move to Disconnecting state */

  if (current_state == XPAN_P2P_Connected ||
      current_state == XPAN_P2P_Connecting) {
    xm_sm_set_current_state(xm_state, XPAN_P2P_Disconnecting);
  } else if (current_state == BT_Connected ||
     current_state == BT_Connecting) {
     xm_sm_set_current_state(xm_state, BT_Disconnecting);
  } else if (current_state == XPAN_AP_Connecting ||
             current_state == XPAN_AP_Connected) {
    xm_sm_set_current_state(xm_state, XPAN_AP_Disconnecting);
    AudioBearerData->waiting_for_rsp |= 0x01 << XM_XAC;
    xac_if.PrepareAudioBearerReq(bdaddr, type);
  }

  if (current_state == BT_Connecting && prev_state == XPAN_AP_Connected)
    xac_if.BearerSwitchInd(bdaddr, BT_LE, XM_SUCCESS, XM_KP_PORT_CLOSED);
   
  AudioBearerData->eventId = QHCI_XM_UNPREPARE_AUDIO_BEARER_REQ;
  AudioBearerData->orginator = QHCI_To_XM;
  AudioBearerData->waiting_for_rsp |= 0x01 << XM_XP;
  AudioBearerData->waiting_for_rsp |= 0x01 << XM_CP;
  AudioBearerData->waiting_for_rsp |= 0x01 << XM_KP;
  AudioBearerData->type = type;

  /* Don't wait for bearer indications */
  AudioBearerData->rx_bearer_ind = false;
  start_timer(audio_bearer, XM_AUDIO_BEARER_TIMEOUT);
  profile_if.XmXpPrepareAudioBearerReq(bdaddr, type);
  packetizer.ProcessMessage(eventId, msg);
  WifiBearerIndication(BT_LE, XM_SUCCESS);
  xm_set_usecase(USECASE_XPAN_NONE);
  memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
  stop_timer(bearer_preference_req);
  if (current_state == XPAN_AP_Disconnecting) {
    XmUseCaseReq *useCaseData = xm_get_usecase_data();
    CpRemoteApParams *Ind = &msg->CpRemoteParamsInd;
    msg->CpRemoteParamsInd.num_devices = 0;
    useCaseData->usecase = USECASE_XPAN_NONE;
    ALOGD("%s: Usecase %s ", __func__, UseCaseToString(useCaseData->usecase));
    XmStaWifiUseCaseUpdate(msg);
    free(Ind->EbParams);
    UpdateStats(false, 1);
  }
  if (profile_if.GetTwtPropStatus() == TWT_CONFIG_ENABLE)
    TerminateTwt();
}

void XpanManager::UnPrepareAudioBearerRsp(XmIpcEventId eventId, xm_ipc_msg_t *msg)
{
  RspStatus status = XM_SUCCESS;
  uint8_t pkt_from;
  XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
  xm_state_machine *xm_state = xm_get_sm_ptr();
  bdaddr_t bdaddr = xm_sm_get_current_active_device(xm_state);
  xm_sm_state current_state  = xm_sm_get_current_state(xm_state);

//memcpy(&bdaddr, xm_sm_get_current_active_device(&xm_state), sizeof(bdaddr_t));
  if (current_state != BT_Disconnecting &&
      current_state != XPAN_P2P_Disconnecting &&
      current_state != XPAN_AP_Disconnecting) {
    ALOGW("%s: discarding this event %s as state is :%s", __func__,
    ConvertMsgtoString(eventId), StateToString(current_state));
    return;
  }
 
  if (eventId == XP_XM_PREPARE_AUDIO_BEARER_RSP ||
      eventId == XAC_XM_PREPARE_AUDIO_BEARER_RSP) {
    status = msg->AudioBearerRsp.status;
    pkt_from = eventId == XP_XM_PREPARE_AUDIO_BEARER_RSP ? XM_XP : XM_XAC;
    ALOGI("%s: received prepare audio bearer rsp from %s with status:%s",
        __func__, pkt_from == XM_XP ? "profile": "XAC", StatusToString(status));
    if (status == XM_SUCCESS) {
      if (!memcmp((const char *)&bdaddr, (const char *)&msg->AudioBearerReq.bdaddr, sizeof(bdaddr_t))) {
        ALOGI("%s bd address matched:%s\n", __func__, ConvertRawBdaddress(bdaddr));
      } else {
        ALOGE("%s bdaddr mis-matched req generated on %s and req performed %s",
          __func__, ConvertRawBdaddress(bdaddr),
          ConvertRawBdaddress(msg->AudioBearerReq.bdaddr));
        status = XM_XP_PERFORMED_ON_WRONG_BD_ADDRESS;
      }
    }
  } else if (eventId == KP_XM_PREPARE_AUDIO_BEARER_RSP) {
    status = (RspStatus)msg->KpAudioBearerRsp.status;
    ALOGI("%s: received prepare audio bearer rsp from kp with status:%s",
        __func__, StatusToString(status));
    pkt_from = XM_KP;
  } else if (eventId == CP_XM_PREPARE_AUDIO_BEARER_RSP) {
    status = (RspStatus)msg->CpAudioBearerRsp.status;
    ALOGI("%s: received prepare audio bearer rsp from cp with status:%s",
        __func__, StatusToString(status));
    pkt_from = XM_CP;
  } else if (eventId == XM_PREPARE_AUDIO_BEARER_TIMEOUT) {
    ALOGI("%s: Recevied audio bearer timeout", __func__);
    status = XM_FAILED_DUE_TO_UNPREPARE_AUDIO_BEARER_TIMEOUT;
  }

  if (status == XM_SUCCESS) {
    AudioBearerData->waiting_for_rsp = (AudioBearerData->waiting_for_rsp &
                          (~(0x01 << pkt_from)));
    if (AudioBearerData->waiting_for_rsp)
      return;
    else
      ALOGI("%s: Rsp received for all other modules", __func__);
  } else {
    ALOGE("%s: Recevied prepare audio bearer rsp with failure status",
        __func__);
  }

  /* reset the state machine */
  xm_sm_reset_state(xm_state);
  
  stop_timer(audio_bearer);
  qhci_if.UnPrepareAudioBearerRsp(bdaddr, status);
  memset(AudioBearerData, 0, sizeof(XmAudioBearerReq));
}

void XpanManager::GetCurrentUsecase(TransportType trans)
{
  XmUseCaseReq *UseCaseData = xm_get_usecase_data();
  UseCaseType type = UseCaseData->usecase;
  ALOGI("%s: Current UseCaseType %s ",
            __func__, UseCaseToString(type));

  UseCaseType prev_usecase = type;
  type = qhci_if.GetCurrentUsecase();
  if (type  == USECASE_XPAN_NONE) {
    ALOGE("%s: invalid usecase type from qhci", __func__);
  } else {
    ALOGI("%s: Recevied UseCaseType %s from qhci",
          __func__, UseCaseToString(type));
    if (type == USECASE_LE_HQ || type == USECASE_XPAN_LOSSLESS || type == USECASE_XPAN_AP_LOSSLESS) {
      if (trans == XPAN_P2P)
        type = USECASE_XPAN_LOSSLESS;
      else if (trans == XPAN_AP)
        type = USECASE_XPAN_AP_LOSSLESS;
      else if (trans == BT_LE)
        type = USECASE_LE_HQ;
    } else if (type == USECASE_XPAN_LE_VOICE || type == USECASE_XPAN_AP_VOICE) {
      if (trans == XPAN_AP)
        type = USECASE_XPAN_AP_VOICE;
      else if (trans == BT_LE)
        type = USECASE_XPAN_LE_VOICE;
    }
    UseCaseData->usecase = type;
    UseCaseData->prev_usecase = prev_usecase;
    ALOGI("%s: current UseCaseType %s and new usecase type %s",
          __func__, UseCaseToString(prev_usecase), UseCaseToString(type));
  }
}

void XpanManager::XmStaWifiUseCaseUpdate(xm_ipc_msg_t *msg)
{
  XmUseCaseReq *UseCaseData = xm_get_usecase_data();
  xm_ipc_msg_t *usecase = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( usecase == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  uint8_t rev_macaddr[6];
  int num_devices = msg->CpRemoteParamsInd.num_devices;
  CpRemoteApParams *Ind = &msg->CpRemoteParamsInd;
  xm_state_machine *xm_state = xm_get_sm_ptr();
  xm_sm_state current_state = xm_sm_get_current_state(xm_state);

  ALOGI("%s usecase: %s, num_devices = %d", __func__, UseCaseToString(UseCaseData->usecase), num_devices);
  usecase->UseCase.eventId = XM_WIFI_STA_USECASE_UPDATE;

  if (num_devices) {
    if (current_state == XPAN_AP_Connecting || current_state == XPAN_AP_Connected)
      usecase->UseCase.usecase = USECASE_XPAN_AP_LOSSLESS;
    else
      usecase->UseCase.usecase = UseCaseData->usecase;
  } else {
    usecase->UseCase.usecase = USECASE_XPAN_NONE;
    ALOGD("%s: changing usecase to none %s ", __func__, UseCaseToString(usecase->UseCase.usecase));
  }
  usecase->UseCase.gaming_vbc_si = 0;
  usecase->UseCase.right_earbud_offset = 0;

  memset(&usecase->UseCase.LeftEb, 0, sizeof(macaddr_t));
  memset(&usecase->UseCase.RightEb, 0, sizeof(macaddr_t));

  for (int i = 0 ; i < num_devices; i++) {
    for (int j = 0; j < 6; j++)
      rev_macaddr[j] = Ind->EbParams[i].eb_mac_addr.b[6-j-1];

    if (Ind->EbParams[i].eb_audio_loc == FRONT_LEFT) {
      memcpy(&usecase->UseCase.LeftEb, &rev_macaddr, sizeof(macaddr_t));
    }
    if (Ind->EbParams[i].eb_audio_loc == FRONT_RIGHT) {
      memcpy(&usecase->UseCase.RightEb, &rev_macaddr, sizeof(macaddr_t));
    }
  }
  UseCaseData->usecase_orginator = XM_SELF;

  ALOGI("%s: left eb %02X:%02X:%02X:%02X:%02X:%02X", __func__, usecase->UseCase.LeftEb.b[0],
        usecase->UseCase.LeftEb.b[1], usecase->UseCase.LeftEb.b[2],
        usecase->UseCase.LeftEb.b[3], usecase->UseCase.LeftEb.b[4], usecase->UseCase.LeftEb.b[5]);
  ALOGI("%s: right eb %02X:%02X:%02X:%02X:%02X:%02X", __func__, usecase->UseCase.RightEb.b[0],
        usecase->UseCase.RightEb.b[1], usecase->UseCase.RightEb.b[2],
        usecase->UseCase.RightEb.b[3], usecase->UseCase.RightEb.b[4], usecase->UseCase.RightEb.b[5]);
  ALOGI("%s gaming_vbc_si %d and right_earbud_offset %d", __func__,
    UseCaseData->gaming_vbc_si, UseCaseData->right_earbud_offset);
  PostMessage(usecase);
}

void XpanManager::XmWifiUseCaseUpdate(UseCaseType usecase)
{
  XmUseCaseReq *UseCaseData = xm_get_usecase_data();
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  memset(msg, 0, XM_IPC_MSG_SIZE);
  ALOGI("%s usecase: %s", __func__, UseCaseToString(usecase));
  msg->UseCase.eventId = XM_WIFI_USECASE_UPDATE;
  msg->UseCase.usecase = usecase;

  if (usecase != USECASE_XPAN_NONE) {
    msg->UseCase.gaming_vbc_si = UseCaseData->gaming_vbc_si;
    msg->UseCase.right_earbud_offset = UseCaseData->right_earbud_offset;
    memcpy(&msg->UseCase.LeftEb, &UseCaseData->LeftEb, sizeof(macaddr_t));
    memcpy(&msg->UseCase.RightEb, &UseCaseData->RightEb, sizeof(macaddr_t));
  }

  UseCaseData->usecase_orginator = XM_SELF;

  ALOGI("%s right eb mac addr %s", __func__, ConvertRawMacaddress(msg->UseCase.RightEb));
  ALOGI("%s left eb mac addr %s", __func__, ConvertRawMacaddress(msg->UseCase.LeftEb));
  ALOGI("%s gaming_vbc_si %d and right_earbud_offset %d", __func__,
  msg->UseCase.gaming_vbc_si, msg->UseCase.right_earbud_offset);
  PostMessage(msg);
}

void XpanManager::QhciUseCaseUpdate(xm_ipc_msg_t *msg)
{
  XmUseCaseReq *UseCaseData = xm_get_usecase_data();
  UseCaseType usecase = msg->UseCase.usecase;
  UseCaseType current_usecase = UseCaseData->usecase;

  ALOGI("%s: current usecase:%s updated usecase %s", __func__,
        UseCaseToString(current_usecase), UseCaseToString(usecase));
  ALOGI("%s right eb mac addr %s", __func__, ConvertRawMacaddress(UseCaseData->RightEb));
  ALOGI("%s left eb mac addr %s", __func__, ConvertRawMacaddress(UseCaseData->LeftEb));

  UseCaseData->usecase_orginator = XM_QHCI;
  UseCaseData->usecase = usecase;

  msg->UseCase.gaming_vbc_si = UseCaseData->gaming_vbc_si;
  msg->UseCase.right_earbud_offset = UseCaseData->right_earbud_offset;

  memcpy(&msg->UseCase.LeftEb, &UseCaseData->LeftEb, sizeof(macaddr_t));
  memcpy(&msg->UseCase.RightEb, &UseCaseData->RightEb, sizeof(macaddr_t));

  packetizer.ProcessMessage(msg->eventId ,msg);
  profile_if.ProcessMessage(msg->eventId, msg);
  wifi_if.PostMessage(msg);
}

void XpanManager::TerminateTwt(void)
{
  xm_ipc_msg_t *msg = (xm_ipc_msg_t *) malloc(XM_IPC_MSG_SIZE);
  if( msg == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    return;
  }
  XPANTwtSessionParams *Twtparams = (XPANTwtSessionParams *)
         malloc(0 * sizeof(XPANTwtSessionParams));;
  if (Twtparams == NULL)
  {
    ALOGE("%s: nullptr", __func__);
    free (msg);
    return;
  }
  msg->TwtParams.eventId = XP_XM_TWT_SESSION_EST;
  msg->TwtParams.num_devices = 0;
  msg->TwtParams.right_earbud_offset = 0;
  msg->TwtParams.vbc_si = 0;
  msg->TwtParams.params = Twtparams;
  ProcessTwtEvent(msg);
}

void XpanManager::CacheTwtParams(xm_ipc_msg_t *msg)
{
  TwtParameters *TwtParams = xm_get_twtparams_data();
  std::vector <XPANTwtSessionParams> params  = TwtParams->params;
  int i, num_devices = msg->TwtParams.num_devices;
  int cache_num_device  = TwtParams->num_devices;

  ALOGI("%s", __func__);
  for (i = 0 ;  i < cache_num_device; i++) {
    TwtParams->params.pop_back();
  }

  TwtParams->num_devices = 0;
  TwtParams->right_earbud_offset = 0;
  TwtParams->vbc_si = 0;

  if (!num_devices) {
      return;
  } else {
    TwtParams->num_devices = num_devices;
    TwtParams->right_earbud_offset = msg->TwtParams.right_earbud_offset;
    TwtParams->vbc_si = msg->TwtParams.vbc_si;
    XPANTwtSessionParams params;
    XPANTwtSessionParams *tparams =  msg->TwtParams.params;
    for (int i = 0; i < num_devices; i++) {
      params.interval = tparams[i].interval;
      params.peroid = tparams[i].peroid;
      params.location = tparams[i].location;
      for (int j =0; j < sizeof(macaddr_t); j++)
        params.mac_addr.b[j] = tparams[i].mac_addr.b[j];
      TwtParams->params.push_back(params);
   }
  }
}

void XpanManager::ProcessTwtEvent(xm_ipc_msg_t *msg)
{
  ALOGI("%s ", __func__);
  CacheTwtParams(msg);
  GetEbDetails(msg);
  packetizer.ProcessMessage(msg->eventId ,msg);
  if (msg->TwtParams.num_devices && xm_get_usecase() != USECASE_XPAN_NONE) {
    UpdateStats(true, 0);
    XmWifiUseCaseUpdate(xm_get_usecase());
  } else  if (msg->TwtParams.num_devices == 0) {
    XmWifiUseCaseUpdate(USECASE_XPAN_NONE);
    UpdateStats(false, 0);
  }
}

#if 0
void XpanManager::WifiTransportSwitch(xm_ipc_msg_t *msg)
{
  /* Type holds the message type whether it is request, response
   * or notifications.
   */	
  uint8_t type = msg->WiFiTransportSwitch.status;
  uint8_t transport_type = msg->WiFiTransportSwitch.transport_type;

  XmBearerPreferenceReq *BearerPreferenceData = xm_get_bearer_preference_data();

  if (type == WIFI_TRANS_SWITCH_STATUS_REQUEST) {
    profile_if.WifiTransportPreferenceReq(transport_type);
    return;
  } else if ((type == WIFI_TRANS_SWITCH_STATUS_REJECTED ||
	     type == WIFI_TRANS_SWITCH_STATUS_COMPLETED) &&
	     BearerPreferenceData->orginator == XM_XP) {
    RspStatus status = XM_SUCCESS;
    if (transport_type != BearerPreferenceData->reqtransport) {
      ALOGW("%s: Req transport type by %s is %s and accepted transport type %s",
            __func__, OrginatorToString((uint8_t)BearerPreferenceData->orginator),
      TransportTypeToString(BearerPreferenceData->reqtransport),
      TransportTypeToString((TransportType)transport_type));
      status = XM_XP_WRONG_TRANSPORT_TYPE_REQ;
    } else if (type == WIFI_TRANS_SWITCH_STATUS_REJECTED) {
      ALOGW("%s: wifi rejected transport switch to %s", __func__,
            TransportTypeToString((TransportType)transport_type));
      status = XM_WIFI_REJECTED_TRANSPORT_SWITCH;
    } else {
      ALOGW("%s: wifi accepted transport switch to %s Now Requesting to CP", __func__,
            TransportTypeToString((TransportType)transport_type));
      packetizer.BearerPreferenceInd(BearerPreferenceData->reqtransport);
      return;
    }

    profile_if.BearerPreferenceRsp(BearerPreferenceData->bdaddr,
		                   BearerPreferenceData->reqtransport,
		                   status, BearerPreferenceData->requestor);
    stop_timer(bearer_preference_req);
  } else if (type == WIFI_TRANS_SWITCH_STATUS_REJECTED ||
	     type == WIFI_TRANS_SWITCH_STATUS_COMPLETED) {
    /* This block of code will be executed when */
  }
}
#endif

void XpanManager::AudioTransportUpdate(xm_ipc_msg_t *msg)
{
  TransportType type = msg->AudioUpdate.type;
  xm_state_machine *xm_state = xm_get_sm_ptr();
  xm_sm_state current_state  = xm_sm_get_current_state(xm_state);
  bdaddr_t active_device = xm_sm_get_current_active_device(xm_state);
  XmBearerPreferenceReq *BearerPreferenceData = xm_get_bearer_preference_data();

  ALOGI("%s: Current state of xm %s and transport update by QHCI %s",
        __func__, StateToString(current_state), TransportTypeToString(type));

  if (current_state == IDLE && type == BT_LE) {
    xm_sm_set_current_state(xm_state, BT_Connected);
    xm_sm_set_current_active_device(xm_state, msg->AudioUpdate.bdaddr);
    profile_if.XmXpBearerSwitchInd(msg->AudioUpdate.bdaddr, type, XM_SUCCESS);
    /* Response will not be handled in this scenario */
    WifiBearerIndication(type, XM_SUCCESS);
  } else if ((current_state == BT_Connected || current_state == BT_Connecting)
       && type == NONE) {
    profile_if.XmXpPrepareAudioBearerReq(active_device, NONE);
    /* reset the state machine */
    xm_sm_reset_state(xm_state);
    stop_timer(bearer_preference_req);
    if (current_state == BT_Connecting) {
      ALOGI("%s notifying kp to unpause", __func__);
      packetizer.KpBearerSwitchInd(XM_FAILED);
      packetizer.PrepareAudioBearerReqtoKp(NONE);
    }
    // Check if request pending was for AP or LE transport type
    if (BearerPreferenceData->reqtransport == XPAN_AP) {
      ALOGI("%s notifying XAC to do idle transition for AP", __func__);
      if (!xac_if.isTransportReadyForSwitch(msg->AudioUpdate.bdaddr)) {
        ALOGE("%s: XAC is in transition state. Reject bearer preference", __func__);
        profile_if.BearerPreferenceRsp(msg->AudioUpdate.bdaddr,
            BearerPreferenceData->reqtransport, XM_XP_FAILED,
            BearerPreferenceData->requestor);
        memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
        return;
      }
      bool status;
      status = xac_if.BearerPreferenceReq(msg->AudioUpdate.bdaddr,
      BearerPreferenceData->reqtransport,
      BearerPreferenceData->urgency);
      profile_if.BearerPreferenceRsp(msg->AudioUpdate.bdaddr,
      BearerPreferenceData->reqtransport, XM_SUCCESS,
      BearerPreferenceData->requestor);
    }
    memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
  }
}


void XpanManager::GetEbDetails(xm_ipc_msg_t *msg)
{
  XmUseCaseReq *UseCaseData = xm_get_usecase_data();
  XPANTwtSessionParams *params;
  int i, num_devices = msg->TwtParams.num_devices;
  uint8_t j, mac_addr_len = sizeof(macaddr_t);
  uint8_t rev_macaddr[6];

  memset(&UseCaseData->LeftEb, 0, sizeof(macaddr_t));
  memset(&UseCaseData->RightEb, 0, sizeof(macaddr_t));
  if (!num_devices) {
    ALOGI("%s: Twt termintated. defaulting eb details", __func__);
    UseCaseData->gaming_vbc_si = 0;
    UseCaseData->right_earbud_offset = 0;
    return;
  }

  params = msg->TwtParams.params; 
  for (i = 0; i < num_devices; i++) {
    for (j = 0; j < mac_addr_len; j++)
      rev_macaddr[j] = params[i].mac_addr.b[mac_addr_len-j-1];
    if (params[i].location == FRONT_LEFT)
      memcpy(&UseCaseData->LeftEb, &rev_macaddr, mac_addr_len);
    if (params[i].location == FRONT_RIGHT)
      memcpy(&UseCaseData->RightEb, &rev_macaddr, mac_addr_len);
  }

  UseCaseData->twt_si = params->interval;
  UseCaseData->gaming_vbc_si = msg->TwtParams.vbc_si;
  UseCaseData->right_earbud_offset = msg->TwtParams.right_earbud_offset;
}

void XpanManager::UpdateWifiDrvStatus(void)
{
  wifi_if.UpdateWifiDrvStatus();
}

void XpanManager::UseCaseStartInd(xm_ipc_msg_t *msg)
{
  xm_state_machine *xm_state = xm_get_sm_ptr();
  TwtParameters *TwtParams = xm_get_twtparams_data();
  xm_sm_state current_state  = xm_sm_get_current_state(xm_state);
  uint8_t stream_id = msg->KpInd.stream_id;
  ALOGI("%s: current_state=%s, stream_id=%d", __func__,
      StateToString(current_state), stream_id);
  if (current_state == XPAN_P2P_Connecting || current_state == XPAN_P2P_Connected) {
    profile_if.ReUpdateTwtSessionParams(TwtParams, stream_id);
  } else if (current_state == XPAN_AP_Connected || current_state == XPAN_AP_Connecting) {
    bdaddr_t active_bdaddr = xm_sm_get_current_active_device(xm_state);
    ALOGI("%s: active_bdaddr=%s", __func__, active_bdaddr.toString().c_str());
    if (active_bdaddr.isEmpty()) {
      packetizer.WARUseCaseRsp(XM_SUCCESS);
    } else {
      xac_if.FetchAndUpdateIpDetailstoCp(active_bdaddr, XAC_CP_REMOTE_PARAMS_REQ,
          (void *)((uintptr_t)stream_id));
    }
  }
}
#endif

void XpanManager::AdspStateUpdate(xm_ipc_msg_t *msg)
{
  XmIpcEventId eventId = msg->eventId;
  int i;

  if (eventId == KP_XM_ADSP_STATE_IND &&
      msg->AdspState.action == BTM_ADSP_SSR_AFTER_POWERUP) {
    ALOGI("%s: ADSP booted up completely", __func__);
    xm_set_adsp_state(1);
 } else if (eventId == CP_XM_ADSP_STATE_IND &&
      msg->AdspState.action == BTM_ADSP_SSR_AFTER_POWERUP) {
      if (xm_get_adsp_state()) {
        ALOGI("%s: adsp glink channel booted up", __func__);
        xm_set_adsp_state(0);
        {
          std::queue <xm_ipc_msg_t *> cp_wq = cp_workqueue;
          while(1)
          {
            cp_wq_mtx.lock();
            if(cp_wq.empty())
            {
              cp_wq_mtx.unlock();
              break;
            } else {
              xm_ipc_msg_t *msg = cp_wq.front();
              cp_wq.pop();
              cp_wq_mtx.unlock();
              CpIpcMsgHandler(msg);
            }
          }
       }
      } else {
        ALOGW("%s: Discarding event", __func__);
      }
  }
}

#ifdef XPAN_ENABLED
void XpanManager::PortStateUpdate(xm_ipc_msg_t *msg)
{
  XmIpcEventId eventId = msg->eventId;
  xm_state_machine *xm_state = xm_get_sm_ptr();
  xm_sm_state current_state  = xm_sm_get_current_state(xm_state);
  xm_sm_state prev_state  = xm_sm_get_prev_state(xm_state);
  bdaddr_t active_device = xm_sm_get_current_active_device(xm_state);

  if (eventId == KP_XM_PORT_STATE_IND &&
    msg->PortState.port_state == 0) {
    if (current_state == XPAN_AP_Connecting && prev_state == BT_Connected)
      xac_if.BearerSwitchInd(active_device, XPAN_AP, XM_SUCCESS, XM_KP_PORT_CLOSED);
    else if (current_state == BT_Connecting && prev_state == XPAN_AP_Connected)
      xac_if.BearerSwitchInd(active_device, BT_LE, XM_SUCCESS, XM_KP_PORT_CLOSED);
    stop_timer(audio_bearer);
  } else {
   ALOGW("%s: Discarding event", __func__);
  }
}
#endif

void XpanManager::CpIpcMsgHandler(xm_ipc_msg_t *msg)
{
  XmIpcEventId eventId = msg->eventId;
  ALOGI("%s: processing event %d (%s)", __func__, eventId, ConvertMsgtoString(eventId));

  switch (eventId) {
    case XM_CP_COP_VER_IND:
#ifdef XPAN_ENABLED
    case XP_XM_HOST_PARAMETERS:
    case XAC_CP_BURST_INT_REQ :
#endif
    case XM_CP_LOG_LVL: {
      packetizer.ProcessMessage(eventId ,msg);
      break;
    } default : {
      ALOGE("%s: invalid %d (%s)", __func__, eventId, ConvertMsgtoString(eventId));
    }
  }
}

void XpanManager::XmIpcMsgHandler(xm_ipc_msg_t *msg)
{
  XmIpcEventId eventId = msg->eventId;
  ALOGI("%s: processing event %d (%s)", __func__, eventId, ConvertMsgtoString(eventId));

  switch (eventId) {
    case DH_XM_COP_VER_IND:
    case XM_CP_LOG_LVL:
    case XM_KP_LOG_LVL:{
      packetizer.ProcessMessage(eventId ,msg);
      break;
    } case CP_XM_ADSP_STATE_IND:
      case KP_XM_ADSP_STATE_IND: {
      AdspStateUpdate(msg);
      break;
    }
#ifdef XPAN_ENABLED
    case KP_XM_PORT_STATE_IND:
      //PortStateUpdate(msg);
      break;
    case QHCI_XM_REMOTE_SUPPORT_XPAN:
    case WIFI_XM_ACS_RESULTS:
    case WIFI_XM_SSR_EVENT:
    case WIFI_XM_SAP_POWER_SAVE_STATE_RSP:
    case WIFI_XP_SET_AP_AVB_RSP:
    case WIFI_XM_TWT_EVENT:
    case XAC_XP_START_FILTERED_SCAN:
    case XAC_INITIATE_BLUETOOTH_CONNECTION_IND:
    case QHCI_XP_CONNECT_LINK_RSP:
    case XAC_XP_MDNS_REQ:
    case XAC_XP_CURRENT_TRANS_UPDATE: 
    case XAC_XP_REGISTER_MDNS_SERVICE:
    case XP_XAC_HANDSET_PORT_RSP:
    case WIFI_XM_DRV_STATE:
    case WIFI_XM_COUNTRY_CODE_UPDATE:
    case WIFI_XM_POSSIBLE_TO_CREATE_AP_IFACE:
    case WIFI_XM_CSA: {
      profile_if.ProcessMessage(eventId, msg);
      break;
    } case XM_XP_CREATE_SAP_INF_STATUS: {
      profile_if.ProcessMessage(eventId, msg);
      qhci_if.ProcessMessage(eventId, msg);
      break;
    } case KP_XM_USECASE_STATE_IND: {
      UseCaseStartInd(msg);
      break;
    } case XP_XM_GET_WIFI_STATUS: {
      UpdateWifiDrvStatus();  
      break;
    } case XP_XM_BONDED_DEVICES_LIST: {
      qhci_if.ProcessMessage(eventId, msg);
      xac_if.ProcessMessage(eventId, msg);
      free(msg->BondedDevies.bdaddr);
      break;
    } case XP_XM_TRANSPORT_ENABLED: 
       case XP_WIFI_SCAN_STARTED:
      case XP_QHCI_CONNECT_LINK_REQ: {
      qhci_if.ProcessMessage(eventId, msg);
      break;
    } case QHCI_XM_USECASE_UPDATE: {
      if (msg->UseCase.usecase != USECASE_LE_HQ) {
        QhciUseCaseUpdate(msg);
      } else {
        profile_if.ProcessMessage(msg->eventId, msg);
      }
      break;
    } case QHCI_XM_PREPARE_AUDIO_BEARER_REQ: {
      QhciPrepareAudioBearerReq(msg);
      break;
    } case XM_INITIALIZE_XPAN_MODULES: {
      InitializeXpanModules(msg->XpanInit.is_xpan_supported);
      break;
    } case XM_QHCI_PREPARE_AUDIO_BEARER_RSP:
      case KP_XM_PREPARE_AUDIO_BEARER_RSP:
      case CP_XM_PREPARE_AUDIO_BEARER_RSP: 
      case XP_XM_PREPARE_AUDIO_BEARER_RSP:
      case XAC_XM_PREPARE_AUDIO_BEARER_RSP:
      case WIFI_XM_TRANSPORT_SWITCH_RSP:
      case XM_PREPARE_AUDIO_BEARER_TIMEOUT: {
      XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
      if (AudioBearerData && eventId == XM_PREPARE_AUDIO_BEARER_TIMEOUT) {
        xm_state_machine *xm_state = xm_get_sm_ptr();
        bdaddr_t bdaddr = xm_sm_get_current_active_device(xm_state);
        if (AudioBearerData->requestor == QHCI) {
          // Notify QHCI of failure as well.
          qhci_if.BearerPreferenceFailed(bdaddr, AudioBearerData->type,
             XM_FAILED_DUE_TO_AUDIO_BEARER_TIMEOUT);
        }
      }
      if (AudioBearerData->eventId == QHCI_XM_UNPREPARE_AUDIO_BEARER_REQ)
        UnPrepareAudioBearerRsp(eventId, msg);
      else if (AudioBearerData->eventId == QHCI_XM_PREPARE_AUDIO_BEARER_REQ)
        PrepareAudioBearerRsp(eventId, msg);
      else if (AudioBearerData->eventId == CP_XM_BEARER_SWITCH_REQ)
        BearerSwitchRsp(eventId, msg);
      else if (AudioBearerData->eventId == XP_XM_BEARER_PREFERENCE_REQ)
        WifiTransportSwitchRsp(eventId, msg);
      else
        ALOGW("%s: AudioBearerData might have cleared", __func__);
      break;
    } case XP_XM_AUDIO_BEARER_SWITCHED: {
      BearerSwitchInd(msg);
      break;
    } case XP_XM_ENABLE_ACS:
      case XM_WIFI_ENABLE_STATS :
      case XP_XM_SAP_POWER_STATE :
      case XP_XM_SAP_CURRENT_STATE :
      case XP_XM_CREATE_SAP_INF :
      case XP_WIFI_CANCEL_AP_AVB:
      case XP_WIFI_CONNECTED_EB_DETAILS:
      case XP_WIFI_DISCONNECTED_EB_DETAILS:
      case XP_TEARDOWN_TWT:
      case XP_WIFI_SET_AP_AVB_REQ:
      case XM_WIFI_USECASE_UPDATE:
      case XM_WIFI_STA_USECASE_UPDATE: {
      wifi_if.PostMessage(msg);
      break;
    } case XP_WIFI_TRANSPORT_SWITCH_RSP: {
        xm_state_machine *xm_state = xm_get_sm_ptr();
        xm_sm_state current_state = xm_sm_get_current_state(xm_state);
        uint8_t status = msg->WiFiTransportSwitch.status;
        if (status != XM_SUCCESS)
              wifi_if.PostMessage(msg);
        else if (current_state != IDLE)
            ALOGW("%s: Dropping XP_WIFI_TRANSPORT_SWITCH_RSP event as audio usecase running", __func__);
        else
          wifi_if.PostMessage(msg);
       break;
    } case XP_XM_BEARER_PREFERENCE_REQ: {
      BearerPreferenceReq(msg);
      break;
    } case XP_XM_HOST_PARAMETERS :
      case QHCI_CP_ENCODER_LIMIT_UPDATE :
      case XAC_CP_BSSID_CHANGE_IND :
      case XAC_CP_ROLE_SWITCH_IND: {
      packetizer.ProcessMessage(eventId ,msg);
      break;
    } case XAC_CP_REMOTE_PARAMS_IND :
      case XAC_CP_REMOTE_PARAMS_REQ : {
      XmUseCaseReq *useCaseData = xm_get_usecase_data();
      CpRemoteApParams *Ind = &msg->CpRemoteParamsInd;
      if (useCaseData) {
        if (useCaseData->usecase == USECASE_XPAN_AP_VOICE ||
            useCaseData->usecase == USECASE_XPAN_LE_VOICE) {
          msg->CpRemoteParamsInd.periodicity = WLAN_WAKE_INTERVAL_VOICE;
        } else {
          msg->CpRemoteParamsInd.periodicity = WLAN_WAKE_INTERVAL_MUSIC;
          // rx_udp_port is valid only for voice usecase and in future for game
          // Todo: Add checks for gaming once Gaming support over R4 is enabled
          msg->CpRemoteParamsInd.rx_udp_port = 0;
          // Disable DTLS for music for simplicity wrt power and transmit
          // packet within 70 ms wake time.
          msg->CpRemoteParamsInd.encryption = 0;
          for (int i =0; i< 16; i++)
              msg->CpRemoteParamsInd.psk[i] = 0;
        }
        ALOGD("%s: Usecase %s periodicity %d", __func__,
              UseCaseToString(useCaseData->usecase), msg->CpRemoteParamsInd.periodicity);
      }
      packetizer.ProcessMessage(eventId ,msg);
      XmStaWifiUseCaseUpdate(msg);
      if (eventId == XAC_CP_REMOTE_PARAMS_REQ &&
          Ind->num_devices > 0) {
        XmAudioBearerReq *AudioBearerData = xm_get_audiobearer_data();
        AudioBearerData->stats_enabled = true;
        UpdateStats(true, 1);
      }
      free(Ind->EbParams);
      break;
    } case XP_XM_TWT_SESSION_REQ:
      case XP_XM_TWT_SESSION_EST: {
      xm_state_machine *xm_state = xm_get_sm_ptr();
      xm_sm_state current_state = xm_sm_get_current_state(xm_state);
      if (current_state == XPAN_P2P_Disconnecting || current_state == IDLE ||
           current_state == BT_Connected) {
        ALOGW("%s: Dropping TWT event as no audio usecase running", __func__);
      } else {
        ProcessTwtEvent(msg);
    }
    break;
    } case QHCI_XM_UNPREPARE_AUDIO_BEARER_REQ: {
      QhciUnPrepareAudioBearerReq(msg);
      break;
    } case CP_XM_DELAY_REPORTING : {
      qhci_if.DelayReporting(msg);
      break;
    } case XAC_XM_TRANSPORT_UPDATE:
      case CP_XM_TRANSPORT_UPDATE: {
      TransportUpdate(msg);
      break;
    } case CP_XM_BEARER_SWITCH_REQ: 
      case XM_BEARER_PREFERENCE_TIMEOUT: {
      BearerSwitchReq(msg);
      break;
    } case WIFI_XM_USECASE_UPDATE_RSP: {
      break;
    } case XP_XAC_MDNS_RSP:
      case XP_XAC_REMOTE_AP_DETAILS:
      case XP_XAC_BONDED_STATE_UPDATE :
      case XP_XAC_HANDSET_PORT_REQ: {
        xac_if.ProcessMessage(eventId, msg);
      break;
    } case XP_XAC_LOCAL_AP_DETAILS : {
      wifi_if.PostMessage(msg);
      xac_if.ProcessMessage(eventId, msg);
      break;
    } case XP_REMOTE_FEATURE_MASK : {
        xac_if.ProcessMessage(eventId, msg);
        break;
      } case QHCI_XM_AUDIO_STREAM_UPDATE: {
      AudioTransportUpdate(msg);
      break;
    } case WIFI_XP_TRANSPORT_SWITCH_REQ: {
      profile_if.BearerPreferenceReq(WIFI_VENDOR, 
              (uint8_t) msg->WiFiTransportSwitch.transport_type,
              msg->WiFiTransportSwitch.reason);
      break;
    } case QHCI_XP_BEARER_PREFERENCE_REQ: {
        profile_if.BearerPreferenceReq(QHCI, msg->QhciBearerPreference.transport,
                msg->QhciBearerPreference.reason);
        break;
    } case XAC_CP_REMOTE_PARAMS_IND_DURING_ROAMING: {
      XmUseCaseReq *useCaseData = xm_get_usecase_data();
      CpRemoteApParams *Ind = &msg->CpRemoteParamsInd;
      XmBearerPreferenceReq *BearerPreferenceData = xm_get_bearer_preference_data();
      if (useCaseData) {
        if (useCaseData->usecase == USECASE_XPAN_AP_VOICE ||
            useCaseData->usecase == USECASE_XPAN_LE_VOICE) {
          msg->CpRemoteParamsInd.periodicity = WLAN_WAKE_INTERVAL_VOICE;
        } else {
          msg->CpRemoteParamsInd.periodicity = WLAN_WAKE_INTERVAL_MUSIC;
          // rx_udp_port is valid only for voice usecase and in future for game
          // Todo: Add checks for gaming once Gaming support over R4 is enabled
          msg->CpRemoteParamsInd.rx_udp_port = 0;
          // Disable DTLS for music for simplicity wrt power and transmit
          // packet within 70 ms wake time.
          msg->CpRemoteParamsInd.encryption = 0;
          for (int i =0; i< 16; i++)
              msg->CpRemoteParamsInd.psk[i] = 0;
        }
        ALOGD("%s: Usecase %s periodicity %d", __func__,
              UseCaseToString(useCaseData->usecase), msg->CpRemoteParamsInd.periodicity);
      }
      packetizer.ProcessMessage(eventId ,msg);
      free(Ind->EbParams);
      if (BearerPreferenceData->is_xpan_ap_roaming_started &&
        BearerPreferenceData->is_xpan_ap_roaming_completed) {
        ALOGD("xac notified roaming complete");
        packetizer.BearerSwitchRsp(XPAN_AP_PREP, XM_SUCCESS);
        stop_timer(bearer_preference_req);
        memset(BearerPreferenceData, 0, sizeof(XmBearerPreferenceReq));
      }
      break;
    } case XAC_XP_BEARER_SWITCH_FAILED_RSP: {
      profile_if.ProcessMessage(eventId, msg);
      break;
    } default : {
      ALOGE("%s: invalid event %d (%s)", __func__, eventId, ConvertMsgtoString(eventId));
    }
#endif
  }
  free(msg);
}

void XpanManager::Cp_Cop_Ver_Info_Init() {
  cp_cop_ver_info = (uint8_t *) malloc(COP_VER_INFO_LEN);
  if (cp_cop_ver_info == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return;
  }
  //Initialize Cop Version In Use
  cp_cop_ver_info[A2DP_SRC_DEFAULT] = MAX_SCENARIOS;
  cp_cop_ver_info[A2DP_SRC_APTXAA_LHDC] = CONN_PROXY_COP_NOT_USED;
  cp_cop_ver_info[A2DP_SRC_ABR] = CONN_PROXY_COP_V1;
  cp_cop_ver_info[A2DP_SINK] = CONN_PROXY_COP_V1;
  cp_cop_ver_info[ISO] = CONN_PROXY_COP_V1;
  cp_cop_ver_info[ISO + 1] = CONN_PROXY_COP_NOT_USED; // For COP Version 2, Cop version info has to be 4.

  //Initialize Cop Version Supported
  cp_cop_ver_info[MAX_SCENARIOS + A2DP_SRC_APTXAA_LHDC] = CONN_PROXY_COP_V0;
  cp_cop_ver_info[MAX_SCENARIOS + A2DP_SRC_ABR] = CONN_PROXY_COP_V0;
  cp_cop_ver_info[MAX_SCENARIOS + A2DP_SINK] = CONN_PROXY_COP_V0;
  cp_cop_ver_info[MAX_SCENARIOS + ISO] = CONN_PROXY_COP_V0;
  cp_cop_ver_info[MAX_SCENARIOS + ISO + 1] = CONN_PROXY_COP_NOT_USED; // For COP Version 2, Cop version info has to be 4.

}

uint8_t* XpanManager::GetCopVersionInUse() {
  if (cp_cop_ver_info == NULL) {
    ALOGE("%s: cp_cop_ver_info is NULL", __func__);
    return NULL;
  }
  cp_cop_ver_in_use = (uint8_t*)malloc(cp_cop_ver_info[0]);
  if (cp_cop_ver_in_use == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return cp_cop_ver_in_use;
  }
  int param_idx = 1;
  for (int idx = 0; idx < cp_cop_ver_info[0]; idx++) {
    cp_cop_ver_in_use[idx] = cp_cop_ver_info[param_idx];
    if (cp_cop_ver_in_use[idx] & CopV0Support) {
      cp_cop_ver_in_use[idx] = CONN_PROXY_COP_V1;
    }
    else if (cp_cop_ver_in_use[idx] & CopV1Support) {
      cp_cop_ver_in_use[idx] = CONN_PROXY_COP_V1;
    }
    else if (cp_cop_ver_in_use[idx] & CopV2Support) {
      cp_cop_ver_in_use[idx] = CONN_PROXY_COP_V2;
    }
    param_idx += 2;
  }
  return cp_cop_ver_in_use;
}

uint8_t* XpanManager::GetCopVersionSupported() {
  if (cp_cop_ver_info == NULL) {
    ALOGE("%s: cp_cop_ver_info is NULL", __func__);
    return NULL;
  }
  cp_cop_ver_supported = (uint8_t*)malloc(cp_cop_ver_info[0]);
  if (cp_cop_ver_supported == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return cp_cop_ver_supported;
  }
  int param_idx = 2;
  for (int idx = 0; idx < cp_cop_ver_info[0]; idx++) {
    cp_cop_ver_supported[idx] = cp_cop_ver_info[param_idx];
    if (cp_cop_ver_supported[idx] & CopV0Support) {
      cp_cop_ver_supported[idx] = CONN_PROXY_COP_V0;
    }
    else if (cp_cop_ver_supported[idx] & CopV1Support) {
      cp_cop_ver_supported[idx] = CONN_PROXY_COP_V1;
    }
    else if (cp_cop_ver_supported[idx] & CopV2Support) {
      cp_cop_ver_supported[idx] = CONN_PROXY_COP_V2;
    }
    param_idx += 2;
  }
  return cp_cop_ver_supported;
}

void XpanManager::FreeCopVersionSupported() {
  if (cp_cop_ver_supported) {
    free(cp_cop_ver_supported);
  }
}

void XpanManager::FreeCopVersionInUse() {
  if(cp_cop_ver_in_use) {
    free(cp_cop_ver_in_use);
  }
}


} // namespace xpan
} // namaespace implementation;
