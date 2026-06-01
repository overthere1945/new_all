/*
 * Copyright (c) 2017 - 2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */
//
// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <pthread.h>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <hidl/HidlSupport.h>
#include "data_handler.h"
#include "logger.h"
#include "soc_properties.h"
#include <cutils/properties.h>
#include "bluetooth_address.h"
#include <utils/Log.h>
#include <signal.h>
#include <string>
#include <android-base/file.h>
#include "hci_internals.h"
#include "SMCLoader.h"

#ifdef XPAN_SUPPORTED
#include "controller.h"
#include "qhci_main.h"
#include "qhci_hm.h"
#endif
#include "state_info.h"

#ifdef BT_CP_CONNECTED
#include "xm_main.h"
#endif

#ifdef GOOGLE_OFFLOAD_ENABLED
#include "bluetooth_socket_impl.h"
using bluetooth::socket::BluetoothSocketLocalIntf;
#endif

#ifdef GATT_OFFLOAD_ENABLED
#include "bluetooth_gatt_offload_impl.h"
using bluetooth::gatt::BluetoothGattLocalIntf;
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#ifdef XPAN_SUPPORTED
using ::xpan::implementation::QHci;
#define XPAN_FEATURE_FLAG  (0x80)
#define XPAN_FEATURE_INDEX (4)
#endif

#ifdef BT_CP_CONNECTED
using ::xpan::implementation::XpanManager;
#endif

#ifdef BT_VER_1_1
#define LOG_TAG "vendor.qti.bluetooth@1.1-data_handler"
#else
#define LOG_TAG "vendor.qti.bluetooth@1.0-data_handler"
#endif

#define PROC_PANIC_PATH     "/proc/sysrq-trigger"
#define MSM_ID_SYSFS_NODE  "/sys/devices/soc0/soc_id"
extern "C" {
#include "libsoc_helper.h"
}

#define GNG_SOC_VER_1_0             0x01
#define GNG_SOC_VER_2_0             0x02

//FMD
#define STATUS_SUCCESS              0x00
#define TMO_SOC_VER_1_0             0x01
#define TMO_SOC_VER_2_0             0x02
#define OTHER_FMD_SUPPORTED_BT_SOC  0x03

#define PWR_DRV_FMD_OPERATION_SUCESSFULL 0x00
#define PWR_DRV_FMD_OPERATION_FAILED     0x01
#define INVALID_FMD_OPERATION            0x02

#ifdef QTI_BT_FMD_SUPPORTED
static const int fmd_operation_cmd_ = 0xbfb2;
static const int power_ctrl_cmd_ = 0xbfad;
uint16_t fmd_key_inx = 0;
static uint8_t fmd_key_data[5080] = {0};
bool fmd_supported_property_read = false;
bool isFmdSupported = false;
bool vtsWarFmdTestCase = false;
fmd_config_header_t fmd_config = {0};
#endif
#define CRITICAL_BATTERY_PERCENTAGE 10

namespace {

using android::hardware::bluetooth::V1_0::implementation::DataHandler;

#ifdef QTI_BT_FMD_SUPPORTED
fmdOperationStruct fmdStruct;
bool log_fmd_soc_cmd = false;
#endif

DataHandler *data_handler = nullptr;
std::mutex init_mutex_;
//For FTM
static bool isFtmEnabled = false;
std::thread calibration_thread;
static bool cal_through_acl;
std::mutex evt_wait_mutex_;
std::mutex cleanup_thread_state_mutex_;
std::condition_variable event_wait_cv;
bool event_wait;
uint16_t awaited_evt;
uint8_t subOpcode;
#ifdef BT_CP_CONNECTED
XpanManager xpan_manager;
bool is_xpan_supported = false;
#endif

#ifdef BTOFF_DELAY_SUPPORT
static std::mutex btoff_delay_timer_mutex;
static timer_t btoff_delay_timer_;
static TimerState btoff_delay_timer_state;
static int is_btoff_delay_support = -1;
#endif

// TPI Async Events
const uint8_t HCI_VS_COEX_STX_BT_PWR_REPORT_IND        = 0x1B;
const uint8_t HCI_VS_COEX_STX_BT_MAX_PWR_IND           = 0x1C;
const uint8_t HCI_VS_COEX_STX_BT_STATE_IND             = 0x1D;
const uint8_t HCI_VS_COEX_STX_V3_BT_NE_REPORT_IND      = 0x1E;
const uint8_t HCI_VS_BT_TRAFFIC_PROFILE_REPORT         = 0x01;

const uint8_t HCI_VND_SPECIFIC_EVENT = 0xFF;
const uint8_t HCI_VS_DEBUG_EVENTS = 0xB4;
const uint8_t HCI_VS_DEBUG_EVENTS_TRAFFIC_PROFILE = 0xB5;

const uint8_t EVENT_COMMAND_STATUS = 0x0F;
const uint16_t OPCODE_VS_LMP_TIMESTAMP_REPORT_CMD = 0xFD63;

static constexpr size_t kBtLmpAsyncEventLen    = 26;
static constexpr size_t kBtLmpRegisterEventLen = 6;

}

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_vec;
using DataReadCallback = std::function<void(HciPacketType, const hidl_vec<uint8_t>*)>;
using InitializeCallback = std::function<void(bool success)>;

#ifdef XPAN_SUPPORTED
bool xpan_support_prop_read = false;
bool is_target_support_xpan = false;
bool xpan_supported = false;
#endif

#ifdef QTI_BT_FMD_SUPPORTED
std::condition_variable wait_fmdkeys;
std::mutex fmd_keys_wait_mutex_;
#define FMD_KEYS_TIMEOUT 700
enum FinderKeysStatus {
  NO_FINDER_KEYS_RECEIVED = 0,
  AWAITING_FINDER_KEYS,
  FINDER_KEYS_RECEIVED,
  DEFAULT_FINDER_KEYS_ASSIGNED,
};
uint8_t fmd_keys_status = NO_FINDER_KEYS_RECEIVED;
fmd_mode_info_t fmd_info;
#endif

std::mutex cal_mutex;
std::condition_variable cal_cv;
bool status;
bool isCalPropertyRead = false;

#ifdef PASSTHROUGH_ENABLED
pthread_cond_t  DataHandler::passthrough_enable_disable_cond;
pthread_mutex_t DataHandler::passthrough_enable_disable_lock;
#endif

bool soc_need_reload_patch = true;
bool soc_baudrate_reset_to_default = false;
bool DataHandler :: caught_signal = false;
unsigned int ClientStatus;
unsigned int RxthreadStatus;
std::mutex DataHandler::init_timer_mutex_;
#ifdef SECURE_PERIPHERAL_ENABLED
uint8_t DataHandler::currSecureState = IPeripheralState_STATE_NONSECURE;
void* DataHandler::periContext = nullptr;
timer_t DataHandler::shutdown_timer_id = 0;
#endif
int DataHandler::init_status_ = INIT_STATUS_IDLE;
bool DataHandler::secureEvent = false;
bool DataHandler::offload_host_config_sent = false;
bool isGlinkLogging = false;
int isFMDentered = -1;

#define BTTPI_SAR_MIN_EVENT_LEN         4
#define BTTPI_EVENT_LEN         5
#define BTTPI_ASYNC_EVENT_LEN   7
#define MIN_OPCODE_LEN         (5)

#ifdef IS_PERI_ENABLED
#define MIN_PERI_EVT_OPCODE_LEN 9
#define MIN_PERI_NTF_OPCODE_LEN 7
#define MIN_PERI_BD_EVT_OPCODE_LEN 10
#ifdef PASSTHROUGH_ENABLED
#define MIN_PERI_SPI_TXP_SWITCH_OPCODE_LEN 8
#endif
#define LOG_PERI_CRASH_DUMP 0xf0
#define HCI_PERI_SET_BAUDRATE  (0x02)
#endif

#define CAL_SLEEP_DURIATION 2  //2 sec
#define VND_EVT_CLASS_VS_RADIO_CAL_NVM_DATA  0x1D
#define MSG_TYPE_VS_RADIO_CAL_NVM_DATA  0x03

/* Startup time is set to 4.95 sec w.r.t HIDL.
 * This is done because many times HIDL detect close triggered before
 * startup timer expiry but in actual startup timer is expired.
 * Delay of Close() call excution from BT Stack to HIDL should be considered.
 */
#define HIDL_INIT_TIMEOUT  (14950)
/* HIDL INIT timeout in case XMEM patch file
 * is used with default download configuration.
 */
#define HIDL_INIT_TIMEOUT_DEFAULT_XMEM (11850)
/* HIDL INIT timeout in case XMEM patch file
 * is used with download configuration set to
 * have rsp for every tlv download cmd.
 */
#define HIDL_INIT_TIMEOUT_XMEM (19850)
#define HCI_CMD_TIMEOUT  (2000)

/* timer to initiate shutdown when device moves to secure state */
#define SECURE_SHUTDOWN_TIMER (50)

#define GET_SOC_ID_IOCTL_RETRY_INTERVAL  (100) // ms

#define HOST_ADD_ON_ADV_AUDIO_UNICAST_FEAT_MASK   0x01
#define HOST_ADD_ON_ADV_AUDIO_BCA_FEAT_MASK       0x02
#define HOST_ADD_ON_ADV_AUDIO_BCS_FEAT_MASK       0x04
#define HOST_ADD_ON_ADV_AUDIO_STEREO_RECORDING    0x08
#define HOST_ADD_ON_ADV_AUDIO_LC3Q_FEAT_MASK      0x10
#define HOST_ADD_ON_QHS_FEAT_MASK                 0x20

#ifdef USER_DEBUG
void DataHandler::SetTransport(HciUartTransport *uart_transport)
{
  uart_transport_ = uart_transport;
}

void DataHandler::SendCTSStatusToClient(unsigned char status) {
  ALOGV("%s: ##",__func__);
  hidl_vec<uint8_t> hidl_data;
  std::vector<uint8_t> hidl_vector  = FRAME_GET_CTS_SOC_STATUS;
  hidl_vector.push_back(static_cast<uint8_t>(status));
  hidl_data =  hidl_vector;
  std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;
  ProtocolCallbacksType *cb_data = nullptr;
  it = protocol_info_.find(TYPE_BT);
  if (it != protocol_info_.end()) {
    cb_data = (ProtocolCallbacksType*)it->second;
  }

  // execute callbacks here
  if (cb_data != nullptr && controller_ != nullptr && cb_data->data_read_cb != nullptr) {
    cb_data->data_read_cb(HCI_PACKET_TYPE_EVENT, &hidl_data);
  }
}

int DataHandler::HandleFlowControl(userial_vendor_ioctl_op_t op)
{
  int flags = 0, err = -1;
  switch (op) {
  case USERIAL_OP_FLOW_ON:
    ALOGV("%s: ## userial_vendor_ioctl: UART Flow On ", __func__);
    if ((err = uart_transport_->Ioctl(USERIAL_OP_FLOW_ON, &flags)) < 0) {
      ALOGE("%s: HW Flow-ON error: 0x%x \n", __func__, err);
      return err;
    }
    break;
  case USERIAL_OP_FLOW_OFF:
    ALOGV("%s: ## userial_vendor_ioctl: UART Flow OFF ", __func__);
    if ((err = uart_transport_->Ioctl(USERIAL_OP_FLOW_OFF, &flags)) < 0) {
      ALOGE("%s: HW Flow-OFF error: 0x%x \n", __func__, err);
      return err;
    }
    break;
  default:
    break;
  }
  return err;
}

int DataHandler::GetCTSLineStatus()
{
  int serial;
  int ret = -1;
  ALOGV("%s:", __func__);
  ret = ioctl(uart_transport_->GetCtrlFd(), TIOCMGET, &serial);
  if (ret < 0) {
    ALOGE("%s: TIOCMGET error =%d", __func__, ret);
    return ret;
  } else {
    ALOGI("%s: TIOCM_CTS SUCCESS = %02x", __func__, serial & TIOCM_CTS);
    return (serial & TIOCM_CTS ? 0 : 1);
  }
}

bool DataHandler::command_is_get_cts_status(const unsigned char* buf, int len)
{
  bool result = false;
  const unsigned char get_cts_status[] = COMMAND_GET_CTS_STATUS;

  ALOGV("%s: length = %d ", __func__, len);
  if (memcmp(get_cts_status, buf, ARRAY_SIZE(get_cts_status)) == 0) {
    result= true;
  }
  return result;
}

bool DataHandler::command_is_get_rts_status(const unsigned char* buf, int len)
{
  bool result = false;
  const unsigned char get_rts_status[] = COMMAND_GET_SOC_CTS_STATUS;

  ALOGV("%s: length = %d ", __func__, len);
  if (len >= ARRAY_SIZE(get_rts_status) &&
      memcmp(get_rts_status, buf, ARRAY_SIZE(get_rts_status)) == 0) {
    result= true;
  }
  return result;
}

bool DataHandler::command_is_reset_uart_flow(const unsigned char* buf, int len)
{
  bool result = false;
  const unsigned char reset_uart_flow[] = COMMAND_RESET_UART_FLOW_ON;

  ALOGV("%s: length = %d ", __func__, len);
  if (memcmp(reset_uart_flow, buf, ARRAY_SIZE(reset_uart_flow)) == 0) {
    result= true;
  }
  return result;
}
#endif

#ifdef SECURE_PERIPHERAL_ENABLED
int32_t DataHandler::NotifyEvent(const uint32_t peripheral, const uint8_t state) {
  ALOGI("%s: TZ Notification state[%d]", __func__, state);
  std::unique_lock<std::mutex> guard(init_mutex_);
  if (peripheral != CPeripheralAccessControl_BLUETOOTH_UID) {
    ALOGE("Invalid parameters received. peripheral:%d", peripheral);
    return -1;
  }
  uint8_t newSecureState = state;

  /**
   * Handling the state where connection got broken to get
   * state change notification
   */
  if (newSecureState == STATE_RESET_CONNECTION) {
    ALOGI("%s: Possible ssgtzd link got broken..\n", __func__);
    // This is a blocking call until ssgtzd gets restored
    do {
      periContext = registerPeripheralCB(CPeripheralAccessControl_BLUETOOTH_UID, NotifyEvent);
      if (periContext != NULL) {
        ALOGI("%s: Call back registered for Peripheral[0x%x] \n", __func__, peripheral);
        break;
      }
      // Sleep for 50 ms before retry
      usleep(100 * 1000);
    } while (true);

    /* Getting current peripheral state after re-connection */
    newSecureState = getPeripheralState(periContext);
    if (state == PRPHRL_ERROR) {
      ALOGE("%s: Failed to get Peripheral state from TZ\n", __func__);
      deregisterPeripheralCB(periContext);
      periContext = NULL;
      // treat this case same as entry to secure mode
      newSecureState = IPeripheralState_STATE_SECURE;
    }
  }

  if (newSecureState != currSecureState) {
    ALOGI("Secure State Changed. CurrState(%d)->NewState(%d)",
           currSecureState, newSecureState);
    currSecureState = newSecureState;

    // Turn BT/FM/ANT Off if state changed to secure and BT is ON
    if ((currSecureState == IPeripheralState_STATE_SECURE) &&
        (init_status_ != INIT_STATUS_IDLE)) {
      ALOGI("Disabling BT/FM/ANT");
      secureEvent = true;
      if (data_handler) {
        if (data_handler->Close(TYPE_BT))
          ALOGI("%s: BT Disabled", __func__);
        if (data_handler->Close(TYPE_FM))
          ALOGI("%s: FM Disabled", __func__);
        if (data_handler->Close(TYPE_ANT))
          ALOGI("%s: ANT Disabled", __func__);
      }
      ALOGI("Disabled BT/FM/ANT");
      StartShutdownTimer();
    }
  }
  return 0;
}
#endif

void DataHandler::data_service_sighandler(int signum)
{
  ALOGD("%s: Setting signal 15 caught status as true", __func__);

  if (data_handler)
    data_handler->SetSignalCaught();
  // lock is required incase of multiple binder threads
  std::unique_lock<std::mutex> guard(init_mutex_);
  ALOGW("%s: Caught Signal: %d", __func__, signum);

  if (data_handler) {
    if (data_handler->Close(TYPE_BT))
        goto cleanup;
    if (data_handler->Close(TYPE_FM))
        goto cleanup;
    if (data_handler->Close(TYPE_ANT))
        goto cleanup;
    ALOGD("%s: cleanup is skipped as close will take care of it", __func__);
    return;
cleanup:
    ALOGI("%s: deleting data_handler", __func__);
    delete data_handler;
    data_handler = NULL;
  } else {
    ALOGD("%s: data_handler is null", __func__);
  }
  kill(getpid(), SIGKILL);
}

int DataHandler::data_service_setup_sighandler(void)
{
  struct sigaction sig_act;

  ALOGI("%s: Entry", __func__);
  memset(&sig_act, 0, sizeof(sig_act));
  sig_act.sa_handler = data_handler->data_service_sighandler;
  sigemptyset(&sig_act.sa_mask);

  sigaction(SIGTERM, &sig_act, NULL);

  return 0;
}

int DataHandler::block_signal(int signum,bool block)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,signum);
    int op = block ? SIG_BLOCK : SIG_UNBLOCK;
    return pthread_sigmask(op,&set,NULL);
}

bool DataHandler::isBTSarEvent(HciPacketType type, const uint8_t* data) {
  uint16_t opcode = ((data[4] << 8) | data[3]);
  uint16_t subopcode = 0;
  if (data[1] > BTTPI_SAR_MIN_EVENT_LEN) {
    subopcode = data[6];
    ALOGD("%s: sar_cmd_opcode: %02x sar_cmd_subopcode: %02x opcode: %02x subopcode: %02x",
    __func__, sar_cmd_opcode, sar_cmd_subopcode, opcode, subopcode);
  } else {
      if ((type == HCI_PACKET_TYPE_EVENT) &&
          (opcode == sar_cmd_opcode)) {
        sar_cmd_opcode = 0x00;
        sar_cmd_subopcode = 0x00;
        ALOGD("%s: Error returned from Controller = %02x", __func__, data[5]);
        return true;
    }
  }
  if (type == HCI_PACKET_TYPE_EVENT &&
    (opcode == sar_cmd_opcode) &&
      (sar_cmd_subopcode == subopcode)) {
    sar_cmd_opcode = 0x00;
    sar_cmd_subopcode = 0x00;
    ALOGD("%s: true",__func__);
    return true;
  }
  return false;
}

bool DataHandler::isBtTpiEvent(HciPacketType type, const uint8_t* data) {
  uint16_t opcode = ((data[4] << 8) | data[3]);
  uint16_t subopcode = 0;
  if (data[1] > BTTPI_SAR_MIN_EVENT_LEN) {
    subopcode = data[6];
    ALOGD("%s: cmd_opcode: %02x cmd_subopcode: %02x opcode: %02x subopcode: %02x",
    __func__, cmd_opcode, cmd_subopcode, opcode, subopcode);
  } else {
      if ((type == HCI_PACKET_TYPE_EVENT) &&
          (opcode == cmd_opcode)) {
        cmd_opcode = 0x00;
        cmd_subopcode = 0x00;
        ALOGD("%s: Error returned from Controller = %02x", __func__, data[5]);
        return true;
    }
  }
  if ((type == HCI_PACKET_TYPE_EVENT) &&
      (opcode == cmd_opcode) &&
      (cmd_subopcode == subopcode)) {
    cmd_opcode = 0x00;
    cmd_subopcode = 0x00;
    ALOGD("%s: true", __func__);
    return true;
  }
  return false;
}

bool DataHandler::isBtTpiAsyncEvent(HciPacketType type, const uint8_t* data) {
  uint8_t event = data[4];
  uint8_t eventType = data[5];
  uint8_t tpiEvent = data[6];
  if ((event == HCI_VND_SPECIFIC_EVENT) &&
      (eventType == HCI_VS_DEBUG_EVENTS || eventType == HCI_VS_DEBUG_EVENTS_TRAFFIC_PROFILE) &&
      ((tpiEvent == HCI_VS_COEX_STX_BT_PWR_REPORT_IND) ||
       (tpiEvent == HCI_VS_COEX_STX_BT_MAX_PWR_IND) ||
       (tpiEvent == HCI_VS_COEX_STX_V3_BT_NE_REPORT_IND) ||
       (tpiEvent == HCI_VS_COEX_STX_BT_STATE_IND) ||
       (tpiEvent == HCI_VS_BT_TRAFFIC_PROFILE_REPORT))) {
    ALOGD("%s: true",__func__);
    return true;
  }
  return false;
}

bool DataHandler::isBtTpiAsyncVSEvent(HciPacketType type, const uint8_t* data) {
  uint8_t event = data[0];
  uint8_t eventType = data[2];
  uint8_t tpiEvent = data[3];
  if ((event == HCI_VND_SPECIFIC_EVENT) &&
      (eventType == HCI_VS_DEBUG_EVENTS || eventType == HCI_VS_DEBUG_EVENTS_TRAFFIC_PROFILE) &&
      ((tpiEvent == HCI_VS_COEX_STX_BT_PWR_REPORT_IND) ||
       (tpiEvent == HCI_VS_COEX_STX_BT_MAX_PWR_IND) ||
       (tpiEvent == HCI_VS_COEX_STX_V3_BT_NE_REPORT_IND) ||
       (tpiEvent == HCI_VS_COEX_STX_BT_STATE_IND) ||
       (tpiEvent == HCI_VS_BT_TRAFFIC_PROFILE_REPORT))) {
    ALOGD("%s: true",__func__);
    return true;
  }
  return false;
}

#ifdef QTI_BT_LMP_EVENT_SUPPORTED
bool DataHandler::isBtLmpAsyncEvent(const uint8_t* data) {
  uint8_t event_code = data[0];
  uint8_t vendor_event_class = data[2];
  return event_code == HCI_VND_SPECIFIC_EVENT && vendor_event_class == 0xD0;
}

bool DataHandler::isBtLmpRegisterEvent(const uint8_t* data) {
  uint8_t event_code = data[0];
  uint16_t opcode = *(uint16_t *)(data + 4);

  return event_code == EVENT_COMMAND_STATUS &&
      opcode == OPCODE_VS_LMP_TIMESTAMP_REPORT_CMD;
}
#endif

bool DataHandler::SendBTSarData(const uint8_t *data, size_t length, DataReadCallback event_cb)
{
  ALOGD("DataHandler::SendBTSarData()");
  if (!isProtocolInitialized(TYPE_BT)) {
    ALOGE("BT HAL not registered, hence not sending BTSAR command");
    return false;
  }
  if (diag_interface_.GetCleanupStatus(TYPE_BT) || diag_interface_.isSsrTriggered()) {
    ALOGE("BT Cleanup in progress, hence not sending BT SAR command");
    return false;
  }
  if (event_cb)
    btsar_event_cb = event_cb;
  if (data != nullptr) {
    sar_cmd_opcode = ((data[1] << 8) | data[0]);
    sar_cmd_subopcode = data[3];
    if (SendData(TYPE_BT, HCI_PACKET_TYPE_COMMAND, data, length) > 0) {
      return true;
    } else {
      // sending command failed, reset sar_cmd_opcode again
      sar_cmd_opcode = 0x00;
      sar_cmd_subopcode = 0x00;
    }
  }
  return false;
}

bool DataHandler::SendBtTpiData(const uint8_t *data, size_t length, DataReadCallback event_cb)
{
  ALOGD("%s", __func__);
  if (!isProtocolInitialized(TYPE_BT)) {
    ALOGE("BT HAL not registered, hence not sending BT TPI command");
    return false;
  }
  if (diag_interface_.GetCleanupStatus(TYPE_BT) || diag_interface_.isSsrTriggered()) {
    ALOGE("BT Cleanup or SSR in progress, hence not sending BT TPI command");
    return false;
  }
  if (event_cb)
    bttpi_event_cb = event_cb;
  if (data != nullptr) {
    cmd_opcode = ((data[1] << 8) | data[0]);
    cmd_subopcode = data[3];
    if (SendData(TYPE_BT, HCI_PACKET_TYPE_COMMAND, data, length) > 0) {
      return true;
    } else {
      // sending command failed, reset cmd_opcode again
      cmd_opcode = 0x00;
      cmd_subopcode = 0x00;
    }
  }
  return false;
}

void DataHandler::registerTpiAsyncEventCb(DataReadCallback event_cb)
{
  ALOGD("%s", __func__);
  if (!isProtocolInitialized(TYPE_BT)) {
    ALOGE("BT HAL not registered, hence not sending BT TPI event");
    return;
  }
  if (event_cb)
    bttpi_asyncevent_cb = event_cb;
  return;
}

void DataHandler::unRegisterTpiAsyncEventCb()
{
  ALOGD("%s", __func__);
  bttpi_asyncevent_cb = (DataReadCallback)(nullptr);
  return;
}

#ifdef QTI_BT_LMP_EVENT_SUPPORTED
void DataHandler::registerBtLmpAsyncEventCb(const uint8_t* data, size_t size,
                                            DataReadCallback event_cb,
                                            OnRegisterCallback register_cb) {
  ALOGD("%s: enter", __func__);
  if (!isProtocolInitialized(TYPE_BT)) {
    ALOGE("%s: BT HAL not registered, hence not sending BT LMP Event", __func__);
    return;
  }
  if (event_cb) btlmp_asyncevent_cb = event_cb;
  if (register_cb) btlmp_register_cb = register_cb;

  if (data != nullptr) {
    if (SendData(TYPE_BT, HCI_PACKET_TYPE_COMMAND, data, size) > 0) {
      ALOGD("%s: sent BT LMP register command", __func__);
    } else {
      ALOGD("%s: failed to send BT LMP register command", __func__);
    }
  }
}

void DataHandler::unregisterBtLmpAsyncEventCb(const uint8_t* data, size_t size) {
  ALOGD("%s", __func__);
  if (data != nullptr) {
    if (SendData(TYPE_BT, HCI_PACKET_TYPE_COMMAND, data, size) > 0) {
      ALOGD("%s: sent BT LMP unregister command", __func__);
    } else {
      ALOGD("%s: failed to send BT LMP unregister command", __func__);
    }
  }

  btlmp_asyncevent_cb = nullptr;
  btlmp_register_cb = nullptr;
}
#endif

void DataHandler :: setFtmStatus(bool status)
{
  isFtmEnabled = status;
  ALOGD("[%s] isFtmEnabled is %d ", __func__, isFtmEnabled);
}

bool DataHandler :: getFtmStatus(void)
{
  ALOGD("[%s] isFtmEnabled is %d ", __func__, isFtmEnabled);
  return isFtmEnabled;
}

HalStatus DataHandler::Init(ProtocolType type, InitializeCallback init_cb,
                       DataReadCallback data_read_cb)
{
  // lock required incase of multiple binder threads
  ALOGW("DataHandler:: Init()");
  std::unique_lock<std::mutex> guard(init_mutex_);
#ifdef BTOFF_DELAY_SUPPORT
  if (checkBTCloseDelay(type)) {
    ALOGD("%s: BT is being turned on, so cancel BT shutdown", __func__);
    if (type == TYPE_BT && data_handler && data_handler->logger_) {
        ALOGD("%s: clean up stack_timeout_triggered", __func__);
        data_handler->logger_->stack_timeout_triggered = false;
    }
    setProtocolCallback(init_cb, data_read_cb);
    init_cb(true);
    ALOGD("%s: init_cb return from stack", __func__);
    return HAL_SUCCESS;
  }
#endif
  if (!data_handler) {
    data_handler = new DataHandler();
    data_handler->data_service_setup_sighandler();
    data_handler->cleanup_timer_state_machine_.timer_state = TIMER_NOT_CREATED;
  }
  return data_handler->Open(type, init_cb, data_read_cb);
}

void DataHandler::CleanUp(ProtocolType type)
{
  // lock is required incase of multiple binder threads
  std::unique_lock<std::mutex> guard(init_mutex_);
  ALOGW("DataHandler::CleanUp()");
#ifdef BTOFF_DELAY_SUPPORT
  if (BTCloseDelay(type)) {
    ALOGD("%s: BT off delay", __func__);
    setProtocolCallback(nullptr, nullptr);
    return;
  }
#endif
  if (data_handler) {
    bool cleanup_thread_started = false;
    if (data_handler->protocol_info_.size() == MIN_CLIENTS_ACTIVE) {
      cleanup_thread_started = true;
      data_handler->StartCleanupTimer();
    }

    bool status = data_handler->Close(type);
    if (cleanup_thread_started)
      data_handler->StopCleanupThreadTimer();

    if (status) {
      delete data_handler;
      data_handler = NULL;
    }
  }
}

bool DataHandler::isProtocolInitialized(ProtocolType pType)
{
  std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;
  bool status = false;

  it = protocol_info_.find(pType);
  if (it != protocol_info_.end()) {
    ProtocolCallbacksType *cb_data = (ProtocolCallbacksType*)it->second;
    if(!cb_data->is_pending_init_cb && init_status_ == INIT_STATUS_SUCCESS) {
      status = true;
    }
    else {
      status = false;
    }
  }
  else {
    status = false;
  }
  ALOGI("%s: status:%d",__func__, status);
  return status;
}

bool DataHandler::isProtocolAdded(ProtocolType pType)
{
  std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;
  bool status = false;

  it = protocol_info_.find(pType);
  if (it != protocol_info_.end()) {
    status = true;
  }
  else {
    status = false;
  }

  ALOGI("%s: status:%d",__func__, status);
  return status;
}

/*
* Function will gives the SPI Client Driver file descriptor
*/
int DataHandler :: GetSpiDriverFd(void) {
  return pwr_drv_Fd;
}

/*
* Function will close the SPI Client Driver file descriptor
*/
void DataHandler :: CloseSpiDriverFd(void) {
  ALOGD("%s\n", __func__);
  close(pwr_drv_Fd);
  pwr_drv_Fd = -1;
}

bool DataHandler :: UpdateSpiDriverFd(void) {
  ALOGD("%s\n", __func__);

  pwr_drv_Fd = open("/dev/spibt", O_RDWR | O_NONBLOCK);

  if (pwr_drv_Fd < 0) {
    return false;
  }

  return true;
}

DataHandler * DataHandler::Get()
{
  return data_handler;
}

DataHandler::DataHandler() {
#ifdef IS_PERI_ENABLED
  notifysignal_ = NotifySignal::Get();
#endif
  soc_cal_status = CAL_STATUS::CALIBRATION_STATUS_IDLE;
  isSocSupportCal = false;
  logger_ = Logger::Get();
  cmd_opcode = 0x00;
  cmd_subopcode = 0x00;
  sar_cmd_opcode = 0x00;
  sar_cmd_subopcode = 0x00;
  is_xmem_read_ = false;
  is_init_thread_killed = true;
  xmem_prop_val_ = 0;
  memset(&host_add_on_features, 0, sizeof( HostAddOnFeatures_t));
  isBtTpiEventFlag = false;

#ifdef PASSTHROUGH_ENABLED
  setIsPassthroughEnabled(false);
#endif
}

bool DataHandler::IsSocAlwaysOnEnabled()
{
  /* If HAL is configured as LAZY, the library will be unloaded
   * from the primary memory when the client's usage count goes to
   * zero. For the next BT ON fw will be loaded back. Having SoC
   * always on will have unnecessary power lekage.
   */
#ifndef LAZY_SERVICE
  char value[PROPERTY_VALUE_MAX] = {'\0'};
  logger_->PropertyGet("persist.vendor.service.bdroid.soc.alwayson", value, "false");
  return ((strcmp(value, "true") == 0) &&
            ((soc_type_ == BT_SOC_CHEROKEE) ||
            (soc_type_ == BT_SOC_HASTINGS)  ||
            (soc_type_ == BT_SOC_HAMILTON)  ||
            (soc_type_ == BT_SOC_MOSELLE)   ||
            (soc_type_ == BT_SOC_GANGES)    ||
            (soc_type_ == BT_SOC_ORNE)      ||
            (soc_type_ == BT_SOC_EVROS)     ||
            (soc_type_ == BT_SOC_COLOGNE)   ||
			(soc_type_ == BT_SOC_THEMISTO)));
#else
  ALOGD("%s SoC always ON not supported on this platform", __func__);
  return false;
#endif
}

BluetoothSocType DataHandler::GetSocType()
{
  return soc_type_;
}

BluetoothSecSocType DataHandler::GetSecSocType()
{
  return sec_soc_type_;
}

BluetoothSocType DataHandler::GetSocTypeInt()
{
  int ret = 0;
  char soc[PROPERTY_VALUE_MAX];
  struct timeval tv;
  ret = logger_->PropertyGet("persist.vendor.qcom.bluetooth.soc", soc, NULL);
  if (ret == 0) {
    ALOGW("%s: SOC property is not set", __func__);
    gettimeofday(&tv, NULL);
    logger_->SetCurrentactivityStartTime(tv,
      BT_HOST_REASON_SOC_NAME_UNKOWN, "GET CHIP ID IOCTL RETRY");
    logger_->SetPrimaryCrashReason(BT_HOST_REASON_INIT_FAILED);
    logger_->SetSecondaryCrashReason(BT_HOST_REASON_SOC_NAME_UNKOWN);
    std::string ioctl_return_val;
    sec_soc_type_ = BT_SEC_SOC_DEFAULT;
    while ((ioctl_return_val = setVendorPropertiesDefault()) == "SoC_NAME_UNKOWN") {
      ALOGE("%s: Didnt get SoC id from SPI Client Driver, retrying in %d ms",
              __func__, GET_SOC_ID_IOCTL_RETRY_INTERVAL);
      usleep(GET_SOC_ID_IOCTL_RETRY_INTERVAL * 1000);
    }
    gettimeofday(&tv, NULL);
    logger_->CheckAndAddToDelayList(&tv);
    strlcpy(soc, ioctl_return_val.c_str(), sizeof(soc));
  }
  if (!strncasecmp(soc, "themisto", sizeof("themisto"))) {
    ALOGI("%s SOC property  set to themisto", __func__);
    soc_type_ = BT_SOC_THEMISTO;
  }else {
    ALOGI("%s SOC property  set to %s", __func__, soc);
    if (!strncasecmp(soc, "rome", sizeof("rome"))) {
      soc_type_ = BT_SOC_ROME;
    }else if (!strncasecmp(soc, "cherokee", sizeof("cherokee"))) {
      soc_type_ = BT_SOC_CHEROKEE;
    }else if (!strncasecmp(soc, "ath3k", sizeof("ath3k"))) {
      soc_type_ = BT_SOC_AR3K;
    }else if (!strncasecmp(soc, "hastings", sizeof("hastings"))) {
      soc_type_ = BT_SOC_HASTINGS;
    }else if (!strncasecmp(soc, "genoa", sizeof("genoa"))) {
      soc_type_ = BT_SOC_GENOA;
    }else if (!strncasecmp(soc, "moselle", sizeof("moselle"))) {
      soc_type_ = BT_SOC_MOSELLE;
    }else if (!strncasecmp(soc, "hamilton", sizeof("hamilton"))) {
      soc_type_ = BT_SOC_HAMILTON;
    }else if (!strncasecmp(soc, "ganges", sizeof("ganges"))) {
      char soc[PROPERTY_VALUE_MAX];
      ret = logger_->PropertyGet("persist.vendor.qcom.bluetooth.sec_soc", soc, NULL);
      if (!strncasecmp(soc, "brahma", sizeof("brahma"))) {
        sec_soc_type_ = BT_SEC_SOC_BRAHMA;
      }
      soc_type_ = BT_SOC_GANGES;
    }else if (!strncasecmp(soc, "orne", sizeof("orne"))) {
      soc_type_ = BT_SOC_ORNE;
    } else if (!strncasecmp(soc, "evros", sizeof("evros"))) {
    soc_type_ = BT_SOC_EVROS;
    } else if (!strncasecmp(soc, "cologne", sizeof("cologne"))) {
    soc_type_ = BT_SOC_COLOGNE;
    }else if (!strncasecmp(soc, "themisto", sizeof("themisto"))) {
      soc_type_ = BT_SOC_THEMISTO;
    }else {
      ALOGI("persist.vendor.qcom.bluetooth.soc unknown, so using pronto.\n");
      soc_type_ = BT_SOC_DEFAULT;
    }
  }
  ALOGI("%s soc_type_ %d sec_soc_type_ %d", __func__, soc_type_, sec_soc_type_);
  return soc_type_;
}


// this is used to send the actual packet.
size_t DataHandler::SendData(ProtocolType ptype, HciPacketType packet_type,
                             const uint8_t *data, size_t length)
{
#ifdef SECURE_PERIPHERAL_ENABLED
  if (currSecureState == IPeripheralState_STATE_SECURE) {
    ALOGD("%s: Returning as device is in secure state", __func__);
    return 0;
  }
#endif
  if (CheckSignalCaughtStatus()) {
    ALOGD("%s: Return as SIGTERM Signal is caught", __func__);
    return 0;
  }

  std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;
  {
    it = protocol_info_.find(ptype);
    if (it == protocol_info_.end()) {
      ALOGE("%s: NO entry found for the protocol %d \n", __func__, ptype);
      return 0;
    }
    if (init_status_ != INIT_STATUS_SUCCESS) {
      ALOGE("%s: BT Daemon not initialized, ignore packet", __func__);
      return 0;
    }
  }

#ifdef USER_DEBUG
  if (command_is_reset_uart_flow(data, length)) {
    ALOGD("<%s: received reset UART flow command", __func__);
    //set UART flow on
    HandleFlowControl(USERIAL_OP_FLOW_ON);
    return 0;
  }

  if (command_is_get_cts_status(data, length)) {
    ALOGD("<%s: received get UART CTS command", __func__);
    SendCTSStatusToClient(GetCTSLineStatus());
    return 0;
  }
#endif

  UpdateRingBuffer(packet_type, data, length);

#ifdef XPAN_SUPPORTED
  if (is_xpan_supported) {
    const uint8_t *data_new = QHci::Get()->UpdateTxPktHandle(packet_type, (uint8_t *)data, length);
    if (packet_type == HCI_PACKET_TYPE_COMMAND) {
    uint8_t status = QHci::Get()->IsQhciTxPkt((data_new != NULL)? (uint8_t *)data_new:(uint8_t *)data, length);
      if (status == 1) {
        QHci::Get()->ProcessTxPktCmd((data_new != NULL)? data_new:data, length);
      } else if (status ==  2) {
          ALOGD("%s Ignore the packet, it need by AC Module", __func__);
          //free((uint8_t *)data);
          return 0;
      } else {
        if (controller_ != nullptr)
          return controller_->SendPacket(packet_type, (data_new != NULL)? data_new:data, length);
      }
    } else if ((packet_type == HCI_PACKET_TYPE_ACL_DATA) &&
           ((QHci::Get()->IsQHciApTransportEnable((data[1]<<8) | data[0]))
           || (QHci::Get()->isL2capPauseActive((data[1]<<8) | data[0])))) {
        //Dont send to SOC
        QHci::Get()->ProcessTxAclData(data, length);
      return 0;
    } else {
      if (controller_ != nullptr)
        return controller_->SendPacket(packet_type, (data_new != NULL)? data_new:data, length);
    }
    if (data_new != NULL) {
      free((uint8_t *)data_new);
    }
  } else {
    if (controller_ != nullptr)
      return controller_->SendPacket(packet_type, data, length);
  }
#else
#ifdef PASSTHROUGH_ENABLED
	if(getIsPassthroughEnabled() == true){
	  sendPacketOnHCIArbiterGlink(packet_type, data, length);
	}else{
	  if (controller_ != nullptr)
	    return controller_->SendPacket(packet_type, data, length);
	}
#else
  if (controller_ != nullptr)
    return controller_->SendPacket(packet_type, data, length);
#endif
#endif

  return 0;
}

#ifdef PASSTHROUGH_ENABLED
void DataHandler::sendPacketOnHCIArbiterGlink(HciPacketType packet_type,const uint8_t *data, size_t length){
  if(isGlinkLogging){
    ALOGD("%s: packet_type :: %02x length :: %02x ",__func__,packet_type,length);
    char buffer_from_stack[MSG_SIZE_MAX];
    int offset = 0;
    for(int i = 0; i < length; i++) {
        offset += snprintf(buffer_from_stack + offset, sizeof(buffer_from_stack) - offset, "%02x ", data[i]);
    }
    ALOGD("%s: Stack Buffer : %s", __func__,buffer_from_stack);
  }
  if(hci_arbiter_transport != nullptr){
    uint8_t hci_cmd_data[MSG_SIZE_MAX];
    size_t bytes_written = 0;
        uint16_t protoEncoded = PACKET_ENCODED;
    uint16_t total_packet_length = EXTRA_LENGTH_FOR_TOTAL_PACKET_GLINK + length;
    uint16_t payload_length = EXTRA_LENGTH_FOR_PAYLOAD_GLINK + length;
    hci_cmd_data[0] = (uint8_t)packet_type;
        hci_cmd_data[1] = (packet_type >> 8) & 0xFF;
        hci_cmd_data[2] = protoEncoded & 0xff;
        hci_cmd_data[3] = protoEncoded >> 8;
        hci_cmd_data[4] = (uint8_t)payload_length;
        hci_cmd_data[5] = (payload_length >> 8) & 0xFF;
        hci_cmd_data[6] = (uint8_t)packet_type;
    memcpy(hci_cmd_data + PAYLOAD_ON_GLINK_OFFSET, data, length);

    if(isGlinkLogging){
      char final_buffer[MSG_SIZE_MAX];
      int final_buffer_offset = 0;
      for(int i = 0; i < total_packet_length; i++) {
          final_buffer_offset += snprintf(final_buffer + final_buffer_offset, sizeof(final_buffer) - final_buffer_offset, "%02x ", hci_cmd_data[i]);
      }
      ALOGD("%s: Final Glink Buffer : %s", __func__,final_buffer);
    }
    hci_arbiter_transport->write(hci_cmd_data,total_packet_length,&bytes_written);

    if(isGlinkLogging)
      ALOGD("%s: write payload bytes_written=%d", __func__, (int)bytes_written);
  }
}

void DataHandler::sendPassthroughDisable(){
  if(running_arbiter_ch_){
    ALOGD("%s: Send G_Offload Disable Command over HCI_Arbiter Glink",__func__);
    std::string passthrough_disable_msg = build_passthrough_disable_msg();
    uint8_t *tmpBuf = (uint8_t*)passthrough_disable_msg.c_str();
    uint8_t packet_type = tmpBuf[0];
    ALOGD("%s: packet type : %d and length %d", __func__,packet_type, passthrough_disable_msg.length());
    size_t bytes_written = 0;
    hci_arbiter_transport->write(tmpBuf,passthrough_disable_msg.length(),&bytes_written);
    ALOGD("%s: write payload bytes_written=%d", __func__, (int)bytes_written);
    pthread_cond_init(&DataHandler :: passthrough_enable_disable_cond, NULL);
    pthread_mutex_init(&DataHandler :: passthrough_enable_disable_lock, NULL);
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    long ns = timeout.tv_nsec + 1000000 * (PASSTHROUGH_ENABLE_DISABLE_TIMEOUT%1000);
    timeout.tv_nsec = ns%1000000000;
    timeout.tv_sec += ns/1000000000 + PASSTHROUGH_ENABLE_DISABLE_TIMEOUT/1000;
    int ret = pthread_cond_timedwait(&DataHandler::passthrough_enable_disable_cond,
                                      &DataHandler::passthrough_enable_disable_lock,&timeout);
    if (ret == ETIMEDOUT) {
      ALOGD("%s: passthrough_disable timedout",__func__);
    }else if(ret == 0){
      ALOGD("%s: passthrough_disable Response Successful. Continue cleanup",__func__);
    }else{
      ALOGD("pthread_cond_timedwait error - passthrough_enable_disable_cond");
    }
    pthread_mutex_unlock(&passthrough_enable_disable_lock);
    pthread_mutex_destroy(&passthrough_enable_disable_lock);
    pthread_cond_destroy(&passthrough_enable_disable_cond);
    hci_arbiter_transport->close();
    if (hci_arbiter_transport != nullptr) {
        delete hci_arbiter_transport;
        hci_arbiter_transport = nullptr;
    }
    std::atomic_exchange(&running_arbiter_ch_, false);
    write(wakeup_pipe_fd_[1], "0", 1);
    if (glink_rx_thread && glink_rx_thread->joinable()) {
      glink_rx_thread->join();
      ALOGD("%s:glink_rx_thread closed",__func__);
    }
    close(wakeup_pipe_fd_[0]);
    close(wakeup_pipe_fd_[1]);
  }else{
    ALOGE("%s: HCI_Arbiter glink is not open",__func__);
  }
}
#endif

#ifdef DUMP_RINGBUF_LOG
void DataHandler::AddHciCommandTag(char* dest_tag_str, struct timeval& time_val, char * opcode)
{
  uint32_t w_index = 0;

  memset(dest_tag_str, 0, RX_TAG_STR_LEN);
  add_time_str(dest_tag_str, &time_val);

  w_index = strlen(dest_tag_str);
  snprintf(dest_tag_str+w_index, strlen(opcode) + 1, "-%s", opcode);
}
#endif

void DataHandler::InternalOnPacketReady(ProtocolType ptype, HciPacketType type,
                          const hidl_vec<uint8_t>*hidl_data, bool from_soc) {

  uint16_t len = hidl_data->size();
  const uint8_t* data = hidl_data->data();
  ProtocolCallbacksType *cb_data = nullptr;
  static bool reset_rxthread_stuck_prop = true;
  std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;

  // update the pending Init cb and other callbacks
  it = protocol_info_.find(ptype);
  if (it != protocol_info_.end()) {
    cb_data = (ProtocolCallbacksType*)it->second;
  } else {
    ALOGE("%s: Didnt get the callbacks", __func__);
  }

  // execute callbacks here
  if (cb_data != nullptr && controller_ != nullptr) {
    bool isbqr = false;
    if(transport_type_ == BT_TRANSPORT_TYPE_SPI) {
      if (controller_ && (((SpiController *)controller_)->IsBqrRieEnabled()))
        isbqr = true;
    }
    else if(transport_type_ == BT_TRANSPORT_TYPE_UART) {
      if (controller_ && (((UartController *)controller_)->IsBqrRieEnabled()))
        isbqr = true;
    }
    if (!cb_data->is_pending_init_cb ) {
      if (!diag_interface_.isSsrTriggered() || isbqr ) {
        controller_->StartRxThreadTimer();
      }
      if (cb_data->data_read_cb != nullptr) {
        cb_data->data_read_cb(type, hidl_data);
      } else {
        ALOGE("%s: data_read_cb is null", __func__);
      }
      controller_->StopRxThreadTimer();
    } else if (diag_interface_.isSsrTriggered() && isbqr ) {
        if (cb_data->data_read_cb != nullptr) {
          cb_data->data_read_cb(type, hidl_data);
        } else {
          ALOGE("%s: data_read_cb is null", __func__);
        }
        controller_->StopRxThreadTimer();
    }
    logger_->inc_rx_stats_counter();

    /* Reset the prop if previous iterations have incremented
     * the Rx thread stuck property.
     */
    if (reset_rxthread_stuck_prop) {
      std::unique_lock<std::mutex> lock(property_reset_mutex_);
      if (is_last_client_cleanup_in_progress == false && !prop_reset_thread.joinable()) {
        prop_reset_thread = std::thread([]() {
        struct sigaction old_sa, sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = usr2_handler;
        sigaction(SIGUSR1, &sa, &old_sa);
        ALOGD("%s: Resetting Rx thread stuck prop", __func__);
        property_set("persist.vendor.service.bdroid.rxthread.stuck.count", "0");
        ALOGD("%s: Resetting Rx thread stuck prop completed", __func__);
        });
      } else {
        ALOGD("%s: Unable to start prop_reset_thread as cleanup(%d) is in process or thread is running",
              __func__, is_last_client_cleanup_in_progress);
      }
      reset_rxthread_stuck_prop = false;
    }
  } else {
    ALOGD("%s: packet discarded and not handled", __func__);
    for (int pktindex = 0; pktindex < len && pktindex < 5; ++pktindex)
      ALOGD("%s: discarded packet[%d] = \t0x%02x ", __func__, pktindex, data[pktindex]);
  }
  delete hidl_data;
}

#ifdef XPAN_SUPPORTED
void DataHandler::OnPacketReadyFromQHci(HciPacketType type,
                    const hidl_vec<uint8_t>*hidl_data, bool from_soc) {

  const uint8_t* data = hidl_data->data();
  uint16_t len = hidl_data->size();
  logger_->ProcessRx(type, data, len);
  OnPacketReady(TYPE_BT, type, hidl_data, from_soc);
}
#endif

/* HCI vendor evt type returns false
 * ACL evt type returns true
 */

void DataHandler::updateSocCalibrationSupportStatus(char status_byte) {
  ALOGI("%s:status_byte = 0x%02x", __func__, status_byte);
  if (status_byte & HCI_ADDON_CMD_CALIBRATION_BIT) {
    isSocSupportCal = true;
    ALOGI("%s:SOC Calibration Supported", __func__);
    return;
  }
  isSocSupportCal = false;
  ALOGI("%s:SOC Calibration Not-Supported", __func__);
}

bool DataHandler::getFarmeType() {
  return cal_through_acl;
}

void DataHandler::updateCalFarmeType(void) {
  static bool isPropertyRead = 0;
  char value[PROPERTY_VALUE_MAX] = {'\0'};

  if (isPropertyRead)
    return;

  isPropertyRead = true;

  logger_->PropertyGet("persist.vendor.service.bdroid.cal_through_acl", value, "false");

  if (strcmp(value, "true") == 0)
    cal_through_acl = true;
  else
    cal_through_acl = false;
}

CAL_STATUS DataHandler::getCalibrationStatus() {
  return soc_cal_status;
}

void DataHandler::startCalibrationThread() {
  calibration_thread = std::thread([this]() {
    ALOGI("startCalibrationThread: CAL Thread started\n");
    std::unique_lock<std::mutex> lock(property_cal_tread_mutex_);
    struct sigaction old_sa, sa;
    if (!data_handler->controller_) {
      ALOGE("startCalibrationThread: returning from here has data_handler->controller_ destroyed\n");
      return;
    }
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = usr2_handler;
    sigaction(SIGUSR1, &sa, &old_sa);
    ALOGD("startCalibrationThread: Calibration data restore will be initiated in next 2 sec\n");
    sleep(CAL_SLEEP_DURIATION);
#ifdef IS_PERI_ENABLED
    if (diag_interface_.GetCleanupStatus(TYPE_BT) ||
        diag_interface_.isSsrTriggered() ||
        NotifySignal::Get()->getUWBSsrStatus()) {
#else
    if (diag_interface_.GetCleanupStatus(TYPE_BT) ||
        diag_interface_.isSsrTriggered()) {
#endif
        ALOGE(" startCalibrationThread: Not initiating soc calibration data loging,"
          " returning as close called/SSR/SSR on UWB\n");
        return;
    }
    ALOGD("startCalibrationThread: Request for SOC Calibration status");

    if (data_handler->controller_) {
      data_handler->controller_->updateCalEvntRecvStatus(false);
    } else {
      ALOGE("%s: returning from here has data_handler->controller_ destroyed\n", __func__);
      return;
    }

    if (!sendCommandWait(HCI_READ_CAL_NVM_DATA, TYPE_BT)) {
      if (diag_interface_.GetCleanupStatus(TYPE_BT)) {
        ALOGI("startCalibrationThread: SOC_CAL : Cleanup is in progress, exiting");
        return;
      }
      ALOGE(" startCalibrationThread: Failed to recv rsp for HCI_READ_CAL_NVM_DATA cmd");
      logger_->SetSecondaryCrashReason(BT_HOST_REASON_FAILED_RECV_RSP_FOR_READ_CAL_NVM_DATA_CMD);
    } else {
      switch (getCalibrationStatus()) {
        case CAL_STATUS::SOC_CALIBRATION_SUCCESSFULL :
        {
          ALOGD(" startCalibrationThread: SOC Calibration is Successfull");
          status = false;
          if (data_handler->controller_) {
            data_handler->controller_->waitForCalDataToCollect();
          } else {
            ALOGE("%s: returning from here has data_handler->controller_ destroyed\n", __func__);
            return;
          }
          soc_cal_status = CAL_STATUS::CALIBRATION_STATUS_IDLE;
          logger_->SetSecondaryCrashReason(BT_SOC_REASON_DEFAULT);
          if (diag_interface_.GetCleanupStatus(TYPE_BT)) {
            calSignalToClose();
            ALOGE(" startCalibrationThread: Notified to resume the close");
          }
          ALOGD(" startCalibrationThread: SOC Calibration thread exited now");
          return;
        }
        case CAL_STATUS::ERR_UNSUPPORTED_FEATURE_PARAM :
        {
          ALOGD(" startCalibrationThread: Calibration fetaure is unsupported in this SOC");
          ALOGD(" startCalibrationThread: Calibration thread exited now");
          return;
        }
        case CAL_STATUS::ERR_NO_NEW_CAL_DATA :
        {
          ALOGD(" startCalibrationThread: SOC Calibration data is same as stored in host");
          ALOGD(" startCalibrationThread: Calibration thread exited now");
          return;
        }
        case CAL_STATUS::ERR_NO_VALID_CAL_DATA :
        {
          ALOGD(" startCalibrationThread: SOC has no vaild calibration data to send to host");
          ALOGD(" startCalibrationThread: Calibration thread exited now");
          return;
        }
        case CAL_STATUS::ERR_INVALID_HCI_COMMAND_PARAM :
        {
          ALOGE(" startCalibrationThread: ERR_INVALID_HCI_COMMAND_PARAM");
          logger_->SetSecondaryCrashReason(BT_HOST_REASON_ERR_INVALID_HCI_CAL_CMD_PARAM);
          break;
        }
        case CAL_STATUS::CALIBRATION_STATUS_IDLE :
        {
          ALOGE(" startCalibrationThread: CALIBRATION_STATUS_IDLE");
          logger_->SetSecondaryCrashReason(BT_HOST_REASON_CALIBRATION_STATUS_IDLE);
          break;
        }
        default:
        {
          ALOGE(" startCalibrationThread: Calibration status received is invalid!!!!");
          ALOGE(" startCalibrationThread: Calibration thread exited now");
          return;
        }
      }
    }
    if (data_handler->controller_) {
      data_handler->controller_->SsrCleanup(BT_HOST_REASON_STORE_SOC_CALIBRATION_FAILED);
    } else {
      ALOGE("%s: returning from here has data_handler->controller_ destroyed\n", __func__);
      return;
    }
  });
}

void DataHandler::OnPacketReady(ProtocolType ptype, HciPacketType type,
                                const hidl_vec<uint8_t>*hidl_data, bool from_soc)
{
  std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;

  uint16_t len = hidl_data->size();
  const uint8_t* data = hidl_data->data();
#ifdef PASSTHROUGH_ENABLED
  if ((ptype == TYPE_PERI) && (type == HCI_PACKET_TYPE_PERI_EVT) && from_soc) {
      uint16_t opcode = ((data[6] << 8) | data[5]);
      ALOGV("%s: opcode is :: %02x",__func__, opcode);
      if(opcode == HCI_VS_GENERAL_OPCODE_PERI){
        uint8_t status = data[7];
        uint8_t subOpCode = data[8];
        if(status == STATUS_SUCCESS && subOpCode == HCI_PERI_BT_SPI_TXP_SWITCH_SUB_OPCODE){
          ALOGD("%s: SPI TXP Switch Successful",__func__);
          if(data_handler->isAwaiterPeriSPITxpSwitchEvt(data,len)){
            ALOGI("%s: Received Peri SPI TXP switch evt", __func__);
            event_wait = true;
            event_wait_cv.notify_all();
          }else {
            ALOGI("%s: Not Peri SPI TxP Switch Evt \n", __func__);
          }
        }else{
          ALOGD("%s: SPI TXP Switch Failed",__func__);
        }
        return;
      }
  }
#endif

#ifdef XPAN_SUPPORTED
  if (is_xpan_supported) {
    if ((ptype == TYPE_BT) && (data_handler->GetQHciState()) && from_soc) {
      QHci::Get()->UpdateRxPktHandle(type, (uint8_t*) data);
      if (type == HCI_PACKET_TYPE_EVENT) {
        bool status = QHci::Get()->IsQhciRxPkt(hidl_data);
        if (status) {
          logger_->ProcessRx(type, data, len);
          ALOGD("%s: RX Packet Need by QHCI", __func__);
          QHci::Get()->ProcessRxPktEvent(type, hidl_data);
          delete hidl_data;
          return;
        }
      }
    }
  }
#endif

  if (from_soc) {
    logger_->ProcessRx(type, data, len);
    // Delete packet if SCO packet received of 0 length
    if (ptype == TYPE_BT && type == HCI_PACKET_TYPE_SCO_DATA && len == HCI_SCO_PREAMBLE_SIZE) {
      delete hidl_data;
      return;
    }

    bool isbqr = false;
    if(transport_type_ == BT_TRANSPORT_TYPE_SPI) {
      if (controller_ && (((SpiController *)controller_)->IsBqrRieEnabled()))
        isbqr = true;
    }
    else if(transport_type_ == BT_TRANSPORT_TYPE_UART) {
      if (controller_ && (((UartController *)controller_)->IsBqrRieEnabled()))
        isbqr = true;
    }

    /*  if BQR RIE is enabled then dont send HW err evt coming from SoC
     *  to BT Stack as we will be sending BQR RIE */
    if (len == LENGTH_HW_ERROR_EVT && data[0] == BT_HW_ERR_EVT && data[1] == BT_HW_ERR_FRAME_SIZE
        && data[2] != HOST_SPECIFIC_HW_ERR_EVT && isbqr ) {
      ALOGD("%s: HW err event from SoC handled internally and not sent to BT stack", __func__);
      ALOGD("%s: HW err packet:  %02x %02x  %02x", __func__, data[0], data[1], data[2]);
      delete hidl_data;
      return;
    }

    {
      if (getFarmeType() && (type == HCI_PACKET_TYPE_ACL_DATA)) {
        uint16_t handle = data[0]|(data[1]<<8);
        if (handle == CAL_ACL_HANDLE) {
          delete hidl_data;
          return;
        }
      }

      if ((data[0] == LOG_BT_EVT_VENDOR_SPECIFIC) &&
          (data[2] == VND_EVT_CLASS_VS_RADIO_CAL_NVM_DATA) &&
          (data[3] == MSG_TYPE_VS_RADIO_CAL_NVM_DATA)) {
        ALOGV("%s: Consuming HCI_VS_RADIO_CAL_NVM_DATA in trasnport\n", __func__);
        delete hidl_data;
        return;
      }
    }

    if (logger_->IsControllerLogPkt(type, data, len)) {
      ALOGV("%s:Received a controller log packet\n", __func__);
      if(!logger_->IsHciFwSnoopEnabled()) {
        delete hidl_data;
        return;
      }
    }
#ifdef IS_PERI_ENABLED
    if (type == HCI_PACKET_TYPE_PERI_EVT) {
      if (data_handler->isAwaitedPeriEvent(data, len)) {
        ALOGI("%s: Received BT Deactivate resp evt", __func__);
      } else if (data_handler->isAwaitedPeriNtf(data, len)) {
        ALOGI("%s: Received BT Deactivate Notification evt", __func__);
        event_wait = true;
        event_wait_cv.notify_all();
      } else if (data_handler->isAwaitedPeriBDEvt(data, len)) {
        ALOGI("%s: Received BT baudrate change evt", __func__);
        event_wait = true;
        event_wait_cv.notify_all();
      } else if (data[3] == LOG_PERI_CRASH_DUMP &&
        (data[4]==LOG_HCI_PERI_CRASH_DUMP_MEMDUMP ||
         data[4]==LOG_HCI_PERI_CRASH_DUMP_INFORMATION)) {
           delete hidl_data;
           return;
      } else {
        //ALOGW("%s: Not a peri awaited event \n", __func__);
      }
      delete hidl_data;
      return;
    }
#endif

    /* BT Event */
    if (type == HCI_PACKET_TYPE_EVENT && data_handler->isAwaitedEvent(data, len)) {
      ALOGW("%s: Received event for command sent internally: %02x %02x \n",
              __func__, data[3], data[4]);

#ifdef QTI_BT_FMD_SUPPORTED
      if (IsFmdModeEnabled() && fmd_info.fmd_mode_progress) {
        if (!IsSocAcceptedFmdInfo(hidl_data)) {
          fmd_info.fmd_mode_progress = false;
          fmd_info.fmd_enable_sent = false;
        }
      }
#endif

      if ((len >= 5) && (data[3] == (uint8_t)HCI_VS_RADIO_READ_CAL_NVM_DATA) &&
          (data[4] == (uint8_t)(HCI_VS_RADIO_READ_CAL_NVM_DATA >> 8)))
        soc_cal_status = (CAL_STATUS)data[5];
      delete hidl_data;
      event_wait = true;
      event_wait_cv.notify_all();
      return;
    }

    if ((cmd_opcode != 0x00 || sar_cmd_opcode != 0x00) && ptype == TYPE_BT && len >= BTTPI_EVENT_LEN) {
      if (cmd_opcode != 0x00 && isBtTpiEvent(type, data)) {
        if (bttpi_event_cb) {
          DataHandler::Get()->isBtTpiEventFlag = true;
          controller_->StartRxThreadTimer();
          bttpi_event_cb(type, hidl_data);
          controller_->StopRxThreadTimer();
          DataHandler::Get()->isBtTpiEventFlag = false;
        }
        delete hidl_data;
        return;
      } else if (sar_cmd_opcode != 0x00 && isBTSarEvent(type, data)) {
        if (btsar_event_cb) {
          btsar_event_cb(type, hidl_data);
        }
        delete hidl_data;
        return;
      }
    }

    if (ptype == TYPE_BT && len >= BTTPI_EVENT_LEN) {
      bool is_tpi_evt = false;
      if (isBtTpiAsyncVSEvent(type, data))
        is_tpi_evt = true;
      else if (len >= BTTPI_ASYNC_EVENT_LEN && isBtTpiAsyncEvent(type, data))
        is_tpi_evt = true;
      if (is_tpi_evt) {
        if (bttpi_asyncevent_cb) {
          DataHandler::Get()->isBtTpiEventFlag = true;
          controller_->StartRxThreadTimer();
          bttpi_asyncevent_cb(type, hidl_data);
          controller_->StopRxThreadTimer();
          DataHandler::Get()->isBtTpiEventFlag = false;
        }
        delete hidl_data;
        return;
      }
    }

#ifdef QTI_BT_LMP_EVENT_SUPPORTED
    if (ptype == TYPE_BT &&
        type == HCI_PACKET_TYPE_EVENT &&
        len >= kBtLmpAsyncEventLen &&
        isBtLmpAsyncEvent(data)) {
      if (btlmp_asyncevent_cb) btlmp_asyncevent_cb(type, hidl_data);
      delete hidl_data;
      return;
    }
    if (ptype == TYPE_BT &&
        type == HCI_PACKET_TYPE_EVENT &&
        len >= kBtLmpRegisterEventLen &&
        isBtLmpRegisterEvent(data)) {
      if (btlmp_register_cb) btlmp_register_cb(data[2] == 0x00);
      delete hidl_data;
      return;
    }
#endif

    ProtocolCallbacksType *cb_data = nullptr;
    /* Dont send any data if cleanup is in progress for FM/ANT */
    if (ptype != TYPE_BT && diag_interface_.GetCleanupStatus(ptype)) {
      if (GetRxthreadStatus(ptype)) {
        SetRxthreadStatus(false, ptype);
        ALOGW("Skip sending packet to client: %d as cleanup in process\n", ptype);
      }
      delete hidl_data;
      return;
    }

    /* stack_timeout_triggered stands to true if BREDR_CLEANUP
     * and STACK_DISABLE timeouts are triggered in stack during
     * BT OFF.
     */
    if (ptype == TYPE_BT && logger_->stack_timeout_triggered) {
      ALOGW("%s: Timeout triggered in stack discarding packet", __func__);
      delete hidl_data;
      return;
    }
  }

  std::unique_lock<std::mutex> guard(internal_mutex_);

  InternalOnPacketReady(ptype, type, hidl_data, from_soc);
  return;

}

#ifdef PASSTHROUGH_ENABLED
void DataHandler::processHciArbiterRx(){
  if (isGlinkLogging)
      ALOGD("processHciArbiterRx :: running_arbiter_ch_ is :: %d",running_arbiter_ch_.load());
  uint8_t *readBuffer = (uint8_t *)malloc(MSG_SIZE_MAX*sizeof(uint8_t));
  if (readBuffer == NULL) {
      ALOGE("%s: readBuffer malloc failed",__func__);
      return;
  }
  while (running_arbiter_ch_) {
    int rcPoll = hci_arbiter_transport->poll(wakeup_pipe_fd_[0],-1);
      if (-1 == rcPoll) {
          ALOGE("Poll Failure");
          break;
      }
      int num = hci_arbiter_transport->read(readBuffer, MSG_SIZE_MAX*sizeof(uint8_t));
      if (isGlinkLogging)
        ALOGD("processHciArbiterRx num of bytes read from stream is :: %d",num);
      if(num < MSG_SIZE_MIN) {
        ALOGE("response is too short ::  %d",num);
      }else {
        uint16_t opcode = (readBuffer[1] << 8) | readBuffer[0];
        uint16_t rsp_status = 0;
        uint16_t encode_decode = (readBuffer[3] << 8) | readBuffer[2];
        uint16_t length = (readBuffer[5] << 8) | readBuffer[4];
        //HciPacketType packet_type = (HciPacketType)opcode;
        switch (opcode) {
          case HCI_PACKET_TYPE_UNKNOWN:
          {
            ALOGV("%s: Packet Type HCI_PACKET_TYPE_UNKNOWN",__func__);
            break;
          }
          case PASSTHROUGH_ENABLE_DONE_CB:
          {
              ALOGD("%s: Packet Type PASSTHROUGH_ENABLE_DONE_CB",__func__);
              // waiting on internal_mutex_ to get unlocked before sending the signal on cond var
              // sometimes the Rx on glink is returning faster than passthrough_enable cmd Tx. 
              // So, cond var is signalled even before the wait.
              std::unique_lock<std::mutex> guard(internal_mutex_);
              uint16_t rsp_status = 0;
              if(length == 2){
                rsp_status = (readBuffer[7] << 8) | readBuffer[6];
              }else{
                ALOGE("%s: Unexpected length for PASSTHROUGH_ENABLE_DONE_CB");
              }
              if(rsp_status == 0x0000){
                ALOGD("%s: PASSTHROUGH_ENABLE_DONE_CB success",__func__);
                // register BT PDR handler.
                hci_arbiter_transport->register_bt_pdr_cb(&DataHandler::bt_pdr_handler);
                pthread_cond_signal(&DataHandler :: passthrough_enable_disable_cond);
              }else{
                ALOGE("%s: PASSTHROUGH_ENABLE_DONE_CB failure",__func__);
              }
            break;
          }
          case PASSTHROUGH_DISABLE_DONE_CB:
          {
              ALOGD("%s: Packet Type PASSTHROUGH_DISABLE_DONE_CB",__func__);
              uint16_t rsp_status = 0;
              if(length == 2){
                rsp_status = (readBuffer[7] << 8) | readBuffer[6];
              }else{
                ALOGE("%s: Unexpected length for PASSTHROUGH_DISABLE_DONE_CB");
              }
              if(rsp_status == 0x0000){
                ALOGD("%s: PASSTHROUGH_DISABLE_DONE_CB success",__func__);
                pthread_cond_signal(&DataHandler :: passthrough_enable_disable_cond);
              }else{
                ALOGE("%s: PASSTHROUGH_DISABLE_DONE_CB failure",__func__);
              }
            break;
          }
          case HCI_PACKET_TYPE_COMMAND:
          {
              ALOGV("%s: Packet Type HCI_PACKET_TYPE_COMMAND",__func__);
            break;
          }
          case HCI_PACKET_TYPE_ACL_DATA:
          {
            ALOGV("%s: Packet Type HCI_PACKET_TYPE_ACL_DATA :: length :: %02x",__func__,length);
              if (length > 0) {
                hidl_vec<uint8_t> *hidl_data = new hidl_vec<uint8_t>;
                hidl_data->resize(length - EXTRA_LENGTH_FOR_PAYLOAD_GLINK);
                memcpy(hidl_data->data(), readBuffer + PAYLOAD_ON_GLINK_OFFSET, length - EXTRA_LENGTH_FOR_PAYLOAD_GLINK);
                OnPacketReady(TYPE_BT, HCI_PACKET_TYPE_ACL_DATA, hidl_data, true);
              }
            break;
          }
          case HCI_PACKET_TYPE_EVENT:
          {
            ALOGV("%s: Packet Type HCI_PACKET_TYPE_EVENT :: length :: %02x",__func__,length);
              if (length > 0) {
                hidl_vec<uint8_t> *hidl_data = new hidl_vec<uint8_t>;
                hidl_data->resize(length - EXTRA_LENGTH_FOR_PAYLOAD_GLINK);
                memcpy(hidl_data->data(), readBuffer + PAYLOAD_ON_GLINK_OFFSET, length - EXTRA_LENGTH_FOR_PAYLOAD_GLINK);
                OnPacketReady(TYPE_BT, HCI_PACKET_TYPE_EVENT, hidl_data, true);
              }
            break;
          }
          case HCI_PACKET_TYPE_ISO_DATA:
          {
              ALOGV("%s: Packet Type HCI_PACKET_TYPE_ISO_DATA",__func__);
            break;
          }
          case HCI_PACKET_TYPE_PERI_EVT:
          {
            ALOGV("%s: Packet Type HCI_PACKET_TYPE_PERI_EVT :: length :: %02x",__func__,length);
            if (length > 0) {
              hidl_vec<uint8_t> *hidl_data = new hidl_vec<uint8_t>;
              hidl_data->resize(length - EXTRA_LENGTH_FOR_PAYLOAD_GLINK);
              memcpy(hidl_data->data(), readBuffer + PAYLOAD_ON_GLINK_OFFSET, length - EXTRA_LENGTH_FOR_PAYLOAD_GLINK);
              OnPacketReady(TYPE_BT, HCI_PACKET_TYPE_PERI_EVT, hidl_data, true);
            }
            break;
          }
          default:
          {
            ALOGE("%s: Default Case",__func__);
            break;
          }
        }
      }
  }
  free(readBuffer);
}
#endif

// signal handler
void DataHandler::data_handler_exit_cb(int /* s */)
{
  bool status = TRUE;
  ALOGD("%s: Unlocking bugreport mutex as init thread killed", __func__);
  Logger::bugreport_mutex.unlock();
  ALOGI("%s: exit\n", __func__);
  Wakelock :: UnlockWakelockMutex();
  pthread_exit(&status);
}

// signal handler
void DataHandler::usr2_handler(int /* s */)
{
  bool status = TRUE;

  ALOGI("%s: exit\n", __func__);
  pthread_exit(&status);
}

unsigned int  DataHandler :: GetRxthreadStatus(ProtocolType Type)
{
  return (RxthreadStatus & (0x01 << Type));
}

void DataHandler :: SetRxthreadStatus(bool status, ProtocolType Type)
{
  if (status)
    RxthreadStatus = (RxthreadStatus | (0x01 << Type));
  else
    RxthreadStatus = (RxthreadStatus & (~(0x01 << Type)));

}

unsigned int  DataHandler :: GetClientStatus(ProtocolType Type)
{
  return (ClientStatus & (0x01 << Type));
}

void DataHandler :: SetClientStatus(bool status, ProtocolType Type)
{
  if (status)
    ClientStatus = (ClientStatus | (0x01 << Type));
  else
    ClientStatus = (ClientStatus & (~(0x01 << Type)));

  if (logger_)
    logger_->SetClientStatus(status, Type);
}

bool DataHandler::closeWaitForCal(int time_out) {
  std::unique_lock<std::mutex> cal_lock(cal_mutex);
  ALOGE("%s = %d msec", __func__, time_out);
  cal_cv.wait_for(cal_lock, std::chrono::milliseconds(time_out), [](){return status;});
  return status;
}

void DataHandler :: calSignalToClose() {
  std::lock_guard<std::mutex> cal_lock(cal_mutex);
  status = true;
  cal_cv.notify_all();
  ALOGE("%s: Notify to the wating clinet", __func__);
}

HalStatus DataHandler::Open(ProtocolType type, InitializeCallback init_cb,
                       DataReadCallback data_read_cb)
{
  char dst_buff[MAX_BUFF_SIZE];
  char init_buff[MAX_BUFF_SIZE];
  struct timeval tv;
  std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;
  std::unique_lock<std::mutex> guard(internal_mutex_);
#ifdef SECURE_PERIPHERAL_ENABLED
  /* register with secure service and check current secure mode */
  char disable_perisec[PROPERTY_VALUE_MAX] = {'\0'};
  logger_->PropertyGet("persist.vendor.service.bdroid.disable_perisec",
                disable_perisec, "0");
  if (strcmp(disable_perisec, "1") != 0) { // peripheral security is enabled
    int ret = 0;
    uint8_t state = IPeripheralState_STATE_NONSECURE;

    if (periContext == nullptr) {
      /* register callback function with TZ service */
      periContext = registerPeripheralCB(
                               CPeripheralAccessControl_BLUETOOTH_UID,
                               NotifyEvent);
      if (periContext == nullptr) {
        ALOGE("%s: Failed to register BT peripheral with TZ", __func__);
        return HAL_HARDWARE_INITIALIZATION_ERROR;
      } else
        ALOGI("%s: Successfuly registered BT peripheral with TZ", __func__);
    } else
      ALOGI("%s: BT peripheral already registered with TZ", __func__);

    ret = getPeripheralState(periContext);
    if (ret == PRPHRL_ERROR) {
      ALOGE("Failed to get BT Peripheral state from TZ");
      return HAL_HARDWARE_INITIALIZATION_ERROR;
    } else {
      currSecureState = ret;
    }
    if (currSecureState == IPeripheralState_STATE_SECURE) {
      ALOGE("%s: Device is in secure area. BT/FM/ANT can't be turned ON",
        __func__);
      return HAL_HARDWARE_INITIALIZATION_ERROR;
    }
  } else
    ALOGI("%s: Peripheral security is disabled", __func__);
#endif
  /* Don't register new client when SSR in progress. This avoids
   * Crash as we will kill daemon after collecting the dump.
   */
  if (diag_interface_.isSsrTriggered()) {
    ALOGE("<%s: Returning as SSR or cleanup in progress>", __func__);
    return HAL_HARDWARE_INITIALIZATION_ERROR;
  }

  if (isProtocolAdded(type)) {
    ALOGE("<%s: Returning as protocol already added>", __func__);
    return HAL_ALREADY_INITIALIZED;
  }

  ALOGI("Open init_status %d \n", init_status_);
  SetClientStatus(true, type);
  SetRxthreadStatus(true, type);

  gettimeofday(&tv, NULL);
  snprintf(init_buff, sizeof(init_buff), "HCI initialize rcvd from client type = %d", type);
  BtState::Get()->AddLogTag(dst_buff, tv, init_buff);
  BtState::Get()->SetTsHCIInitClose(HCI_INIT, dst_buff);

  // update the pending Init cb and other callbacks
  it = protocol_info_.find(type);
  if (it == protocol_info_.end()) {
    ProtocolCallbacksType *cb_data  = new (ProtocolCallbacksType);
    cb_data->type = type;
    cb_data->is_pending_init_cb = true;
    cb_data->init_cb = init_cb;
    cb_data->data_read_cb = data_read_cb;
    protocol_info_[type] = cb_data;
  }
  switch (init_status_) {
    case INIT_STATUS_INITIALIZING:
      return HAL_SUCCESS;
      break;
    case INIT_STATUS_SUCCESS:
      /* During previous BT ON, Stack timeout might be triggered
       * but HIDL is still active to serve other clients. Resetting
       * the flag to clear previous BT stack timeout state.
       */
      if (type == TYPE_BT && logger_)
        logger_->stack_timeout_triggered = false;

      it = protocol_info_.find(type);
      if (it != protocol_info_.end()) {
        ProtocolCallbacksType *cb_data = (ProtocolCallbacksType*)it->second;
        cb_data->is_pending_init_cb = false;
        cb_data->init_cb(true);
      }
      return HAL_SUCCESS;
      break;
    case INIT_STATUS_FAILED:
      init_thread_.join();
      [[fallthrough]];
    case INIT_STATUS_IDLE:
      init_status_ = INIT_STATUS_INITIALIZING;
#ifdef WAKE_LOCK_ENABLED
      Wakelock :: Init();
#endif
      break;
    default:
    {
      ALOGE("Unhandled init_status");
      break;
    }
  }

  logger_->ResetCrashReasons();

  // Opening Powe driver and getting File Descriptors.
  if(!UpdateSpiDriverFd()) {
    ALOGE("%s:Failed to Open SPI Client Driver FD Returning", __func__);
    return HAL_HARDWARE_INITIALIZATION_ERROR;
  }

#ifdef IS_PERI_ENABLED
  // Registering call back Fn to handle OOBS/SPI Client Driver signals
  // in User sapce which sent from SPI Client Driver
  notifysignal_->RegSigIOCallBack();

  // Register BT PID with SPI Client Driver
  if(!notifysignal_->RegisterService()) {
   ALOGE("%s:Unable to Register service with SPI Client Driver\n", __func__);
   ALOGE("%s:Fails to recive Co-Ordinate Reset Signals\n", __func__);
  }
#endif

  init_thread_ = std::thread([this, type]() {
    // Init thread is alive now.
    is_init_thread_killed = false;
    init_thread_id = std::this_thread::get_id();
    bool status = false;
    struct sigaction old_sa, sa;
    char cb_status_buf[MAX_BUFF_SIZE] = {'\0'};
    char dst_buff[MAX_BUFF_SIZE];
    struct timeval tv;

    /* Start Init timer to detect Init stuck. */
    StartInitTimer();
    is_last_client_cleanup_in_progress = false;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = data_handler_exit_cb;
    sigaction(SIGUSR2, &sa, &old_sa);
    //Get SoC type, if not present retry with CHIP ID IOCTL.
    soc_type_ = GetSocTypeInt();
    {
      std::unique_lock<std::mutex> lock(DataHandler::init_timer_mutex_);
      // Returning as CHIP ID IOCTL retry caused init timeout.
      if (GetInitTimerState() == TIMER_OVERFLOW) {
        ALOGW("Initialization timeout detected cleanup is in process");
        // Init thread exited.
        is_init_thread_killed = true;
        return;
      }
    }
    if (!IsSocAlwaysOnEnabled()) {
        soc_need_reload_patch = true;
    }
    ALOGI("init_thread_: soc_need_reload_patch = %d", soc_need_reload_patch);
    gettimeofday(&tv, NULL);
    logger_->SetCurrentactivityStartTime(tv,
      BT_HOST_REASON_FILE_SYSTEM_CALL_STUCK, "CONTROLLER CONSTRUCTOR STUCK");
    logger_->SetPrimaryCrashReason(BT_HOST_REASON_INIT_FAILED);
    logger_->SetSecondaryCrashReason(BT_HOST_REASON_FILE_SYSTEM_CALL_STUCK);
    ALOGI("soc_type_ is :: %d",soc_type_);
    if (soc_type_ == BT_SOC_SMD) {
      controller_ = static_cast<Controller *> (new MctController(soc_type_));
      transport_type_ = BT_TRANSPORT_TYPE_SMD;
#ifdef IS_PERI_ENABLED
    } else if(soc_type_ == BT_SOC_THEMISTO){
      controller_ = static_cast<Controller *> (new SpiController(soc_type_, sec_soc_type_));
      transport_type_ = BT_TRANSPORT_TYPE_SPI;
#ifdef PASSTHROUGH_ENABLED
      ALOGI("%s : hci_arbiter_transport new object",__func__);
      hci_arbiter_transport = new HciGlinkTransport();
#endif
#endif
    } else {
      controller_ = static_cast<Controller *> (new UartController(soc_type_, sec_soc_type_));
      transport_type_ = BT_TRANSPORT_TYPE_UART;
    }
    gettimeofday(&tv, NULL);
    logger_->CheckAndAddToDelayList(&tv);

    if (controller_) {
      status = controller_->Init([this](ProtocolType ptype, HciPacketType type,
                                        const hidl_vec<uint8_t> *hidl_data)   {
                                   OnPacketReady(ptype, type, hidl_data, true);
                                 });
      gettimeofday(&tv, NULL);
      snprintf(dst_buff, sizeof(dst_buff), "Controller Init status = %d", status);
      BtState::Get()->AddLogTag(cb_status_buf, tv, dst_buff);
      BtState::Get()->SetTsCtrlrInitStatus(cb_status_buf);
      if (status)
        SetHostAddOnFeatures(controller_->GetChipVersion());
        SetScramblingFeature(controller_->GetChipVersion());
    }

#ifdef IS_PERI_ENABLED
    if (soc_type_ == BT_SOC_GANGES && status == false &&
	controller_ && (controller_)->GetCleanupStatus()) {
      ALOGI("%s: cleanup is in progress and returning from here", __func__);
      return;
    }
#endif

    if (status) {
      if (!soc_need_reload_patch) {
        if (!sendCommandWait(HCI_RESET_CMD, type)) {
          StopInitTimer();
          ALOGE("%s: Failed to receive rsp for first HCI RESET CMD", __func__);
          ALOGE("%s: SSR is in progress returning from here", __func__);
          // Init thread exited.
          is_init_thread_killed = true;
          return;
        }
        if (!sendCommandWait(HCI_WRITE_BD_ADDRESS, type)) {
          StopInitTimer();
          ALOGE("%s: Failed to receive rsp for HCI WRITE BD address CMD", __func__);
          ALOGE("%s: SSR is in progress returning from here", __func__);
          // Init thread exited.
          is_init_thread_killed = true;
          return;
        }
        // Reset reason to default.
        logger_->SetSecondaryCrashReason(BT_SOC_REASON_DEFAULT);
      }
      SetOffloadHostConfig(type);
#ifdef BT_CP_CONNECTED
      /* Send CoP version to XM */
      UpdateCopVersion(controller_->GetCoPVersion());
#endif
#ifdef XPAN_SUPPORTED
      XpanInitModules();
#endif
#ifdef QTI_BT_FMD_SUPPORTED
      if ((GetSocType() == BT_SOC_GANGES) || (GetSocType() == BT_SOC_ORNE) || (GetSocType() == BT_SOC_COLOGNE) || (GetSocType() == BT_SOC_THEMISTO)) {
        property_set("persist.bluetooth.finder.supported", "true");
      } else {
        property_set("persist.bluetooth.finder.supported", "false");
      }
#endif
    }
    StopInitTimer();
    std::unique_lock<std::mutex> guard(internal_mutex_);
    bool kill_needed = false;
    if (status) {
      /* Stop moving further if timeout detected */
      {
        guard.unlock();
        std::unique_lock<std::mutex> lock(DataHandler::init_timer_mutex_);
        if (GetInitTimerState() == TIMER_OVERFLOW) {
          ALOGW("Initialization timeout detected cleanup is in process");
          // Init thread exited.
          is_init_thread_killed = true;
          return;
        }
        guard.lock();
        init_status_ = INIT_STATUS_SUCCESS;
        ALOGD("Firmware download succeded.");
#ifdef PASSTHROUGH_ENABLED
  ALOGD("Pass Through Arch Enabled ::: Send TXP Switch VSC");
  if (!sendCommandWait(HCI_VSC_PERI_BT_SPI_TXP_SWITCH, TYPE_PERI)){
    ALOGD("SPI TxP Switch Failed.");
    init_status_ = INIT_STATUS_FAILED;
  }else{
    // Opening hci_arbiter glink node and starting a reader thread
    if(hci_arbiter_transport != nullptr){
        ALOGI("%s : opening hci_arbiter_transport",__func__);
        int ret_fd = hci_arbiter_transport->open(HCI_ARBITER_GLINK_NODE);
        if (ret_fd <= 0) {
          ALOGD("%s: hci_arbiter glink open failed",__func__);
          std::atomic_exchange(&running_arbiter_ch_, false);
        }else{
          ALOGD("%s: hci_arbiter glink open successful...start reader thread",__func__);
          char glink_logging[PROPERTY_VALUE_MAX];
          int ret = property_get("persist.vendor.qcom.bluetooth.glink.logging", glink_logging,"false");
          if (strcmp(glink_logging, "true") == 0)
            isGlinkLogging = true;
          else
            isGlinkLogging = false;
          std::atomic_exchange(&running_arbiter_ch_, true);
          if (!glink_rx_thread || !glink_rx_thread->joinable()) {
            if (pipe2(wakeup_pipe_fd_, O_NONBLOCK) == -1) {
              ALOGE("%s: Failed to create wakeup pipe",__func__);
            }
            glink_rx_thread = std::unique_ptr<std::thread>(new std::thread(&DataHandler::processHciArbiterRx, this));
          }
        }
    }
    pthread_cond_init(&DataHandler :: passthrough_enable_disable_cond, NULL);
    pthread_mutex_init(&DataHandler :: passthrough_enable_disable_lock, NULL);
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    long ns = timeout.tv_nsec + 1000000 * (PASSTHROUGH_ENABLE_DISABLE_TIMEOUT%1000);
    timeout.tv_nsec = ns%1000000000;
    timeout.tv_sec += ns/1000000000 + PASSTHROUGH_ENABLE_DISABLE_TIMEOUT/1000;
    guard.unlock();
    ALOGD("%s: Send G_Offload Enable Command over HCI_Arbiter Glink",__func__);
    std::string passthrough_enable_msg = build_passthrough_enable_msg();
    uint8_t *tmpBuf = (uint8_t*)passthrough_enable_msg.c_str();
    uint8_t packet_type = tmpBuf[0];
    ALOGD("%s: packet type : %d and length %d", __func__,packet_type, passthrough_enable_msg.length());
    size_t bytes_written = 0;
    guard.lock();
    hci_arbiter_transport->write(tmpBuf,passthrough_enable_msg.length(),&bytes_written);
    ALOGD("%s: write payload bytes_written=%d", __func__, (int)bytes_written);
    guard.unlock();
    int ret = pthread_cond_timedwait(&DataHandler::passthrough_enable_disable_cond,
                                      &DataHandler::passthrough_enable_disable_lock,&timeout);

    if (ret == ETIMEDOUT) {
      // passthrough_enable Timed out
      ALOGD("%s: passthrough_enable timedout",__func__);
      init_status_ = INIT_STATUS_FAILED;
      status = false;

      /* Recovery mechanism, switch transport back to SPI again for next BT turn on
         Do not clean up here since controller and transport clean up is part of init
         failure to be sent to native layer
      */
      if(GetControllerRef() != nullptr) {
          ((SpiController *)GetControllerRef())->handleQ6SSR();
      }
    }else if(ret == 0){
      ALOGD("%s: passthrough_enable Response Successful. Send Initialization Complete to Stack",__func__);
      setIsPassthroughEnabled(true);
    }else{
      ALOGD("pthread_cond_timedwait error - passthrough_enable_disable_cond");
    }
    // release lock
    guard.lock();
    pthread_mutex_unlock(&passthrough_enable_disable_lock);
    pthread_mutex_destroy(&passthrough_enable_disable_lock);
    pthread_cond_destroy(&passthrough_enable_disable_cond);
  }
#endif
      }
    } else {
      /* Stop moving further if timeout is detected */
      {
        guard.unlock();
        std::unique_lock<std::mutex> lock(DataHandler::init_timer_mutex_);
        if (GetInitTimerState() == TIMER_OVERFLOW || logger_->isSsrTriggered()) {
          ALOGW("Init timeout or SSR detected discarding cleanup from init thread");
          // Init thread exited.
          is_init_thread_killed = true;
          return;
        }
        guard.lock();
        ALOGE("Controller Init failed ");
        init_status_ = INIT_STATUS_FAILED;
        logger_->SetRecoveryStartTime();
      }

      /* Setting primary and secondary reason as PATCH_DLDNG_FAILED
       * in case of Controller init failure
       */
      logger_->SetPrimaryCrashReason(BT_HOST_REASON_INIT_FAILED);
      if (logger_->GetInitFailureReason() != BT_SOC_REASON_DEFAULT) {
        logger_->SetSecondaryCrashReason(logger_->GetInitFailureReason());
#ifdef IS_PERI_ENABLED
        if ((logger_->GetInitFailureReason() == BT_HOST_REASON_PERI_ACCESS_DISALLOWED) ||
            (logger_->GetInitFailureReason() == BT_HOST_REASON_BT_PWR_VOTE_FAILED)) {
#else
        if (logger_->GetInitFailureReason() == BT_HOST_REASON_BT_PWR_VOTE_FAILED) {
#endif
          kill_needed = true;
        }
      } else if (soc_type_ != BT_SOC_SMD && logger_->GetUartErrCode() != UART_REASON_DEFAULT) {
        logger_->SetSecondaryCrashReason(logger_->GetUartErrCode());
      } else {
        logger_->SetSecondaryCrashReason(BT_SOC_REASON_DEFAULT);
      }

      if (controller_) {
        /* Unlocking internal_mutex so that root inflammation event can be sent */
        guard.unlock();
        SendBqrRiePacket();
        guard.lock();
      }

      // Collect dumps only if there is no forced reboot
      if (data_handler->CheckSignalCaughtStatus() == false &&
          secureEvent == false) {
        logger_->PrepareDumpProcess();
        logger_->StoreCrashReason();
        // Add Delay list info. in state file object.
        logger_->AddDelayListInfo();
        logger_->CollectDumps(true, true);
      }
    }

    std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;
    for (auto& it: protocol_info_) {
      ProtocolCallbacksType *cb_data = (ProtocolCallbacksType*)it.second;
      cb_data->is_pending_init_cb = false;
      gettimeofday(&tv, NULL);
      snprintf(dst_buff, sizeof(dst_buff), "Init callback status = %d", status);
      BtState::Get()->AddLogTag(cb_status_buf, tv, dst_buff);
      BtState::Get()->SetTsStatusOfCbSent(cb_status_buf);
      if(status){
        ALOGD("Controller Init Successful : Sending init_cb");
      }else{
        ALOGD("Controller Init Failed : Sending init_cb");
      }
      cb_data->init_cb(status);
    }

    // clear the list if the controller open call fails
    if (!status) {
      /* clearing up all callback data as controller init itself failed */
      for (auto& it: protocol_info_) {
        ProtocolCallbacksType *cb_data = (ProtocolCallbacksType*)it.second;
        delete (cb_data);
      }
      protocol_info_.clear();
      BtState::Get()->Cleanup();
#ifdef USER_DEBUG
      if (!data_handler->CheckSignalCaughtStatus() && kill_needed == false) {
        char value[PROPERTY_VALUE_MAX] = {'\0'};
        property_get("persist.vendor.service.bdroid.trigger_crash", value, "0");
        // call kernel panic so that all dumps are collected
        if (strcmp(value, "1") == 0) {
          ALOGE("%s: Do kernel panic immediately as property \"trigger_crash\" set to %s",
                 __func__, value);
          if (((UartController *)controller_)->bt_kernel_panic() == 0) {
            return;
          } else {
            ALOGE("%s: kernel panic failed, doing abort", __func__);
          }
        }
        ALOGE("%s: Aborting daemon to recover as controller init failed", __func__);
        abort();
      } else if (kill_needed) {
        if (logger_->GetInitFailureReason() == BT_HOST_REASON_BT_PWR_VOTE_FAILED) {
          ALOGE("init_thread_: Killing daemon as soc power voting failed");
#ifdef IS_PERI_ENABLED
        } else if (logger_->GetInitFailureReason() == BT_HOST_REASON_PERI_ACCESS_DISALLOWED) {
          ALOGE("init_thread_: Killing daemon as peri access not granted");
#endif
        }
        ALOGE("%s: Killing daemon as peri access not granted", __func__);
        kill(getpid(), SIGKILL);
      } else {
        // Delete current dumped logs, as issue triggered during reboot.
        logger_->DeleteCurrentDumpedFiles();
        /* user triggerred reboot, no need to call abort */
        ALOGE("%s: Killing daemon as user triggered forced reboot", __func__);
        kill(getpid(), SIGKILL);
      }
#else
      ALOGE("%s: Killing daemon to recover as controller init failed", __func__);
      kill(getpid(), SIGKILL);
#endif

    }

    guard.unlock();
    // BT ON successful
    property_set("persist.vendor.service.bdroid.system_delay_crash_count", "0");

    if (diag_interface_.GetCleanupStatus(TYPE_BT))
      ALOGE("init_thread_: Not initiating soc calibration data loging, returning as close called");
    else if (diag_interface_.isSsrTriggered())
      ALOGE("init_thread_: Not initiating soc calibration data loging, returning as SSR is running");
#ifdef IS_PERI_ENABLED
    else if (NotifySignal::Get()->getUWBSsrStatus())
      ALOGE("init_thread_: Not initiating soc calibration data loging, returning as SSR running on UWB");
#endif
    else if (!isSocSupportCal)
      ALOGE("init_thread_: Not initiating soc calibration data loging, returning as fetaure is not supported");
    else
      startCalibrationThread();

#ifdef QTI_BT_FMD_SUPPORTED
    if ((GetSocType() == BT_SOC_GANGES) || (GetSocType() == BT_SOC_ORNE) || (GetSocType() == BT_SOC_COLOGNE) || (GetSocType() == BT_SOC_THEMISTO)) {
      if (!fmd_supported_property_read) {
        char value_prop_pf[PROPERTY_VALUE_MAX] = {'\0'};
        property_get("persist.vendor.bluetooth.fmd_support", value_prop_pf, "false");
        ALOGI("init_thread_: FMD READING PROP soc_type_ %d, fmd_support = %s", GetSocType(), value_prop_pf);
        fmd_supported_property_read = true;
        if (strcmp(value_prop_pf, "true") == 0) {
          isFmdSupported = true;
          GenerateKeyData();
          if (!fmd_config.prop_read) {
            GetFmdHeaderDataInfo();
            GetFmdConfigInfo();
            GetFmdPropValues();
            fmd_config.prop_read = true;
          }
        }
      }
    }
#endif
    // Init thread exited.
    is_init_thread_killed = true;
    ALOGD("init_thread_: init thread exited now");
  });

#ifdef XPAN_SUPPORTED
  if (!xpan_support_prop_read) {
    char value_prop_pf[PROPERTY_VALUE_MAX] = {'\0'};
    property_get("persist.vendor.qti.btadvaudio.target.support.xpan", value_prop_pf, "");
    xpan_support_prop_read = true;
    if (strcmp(value_prop_pf, "true") == 0) {
      is_target_support_xpan = true;
    }
  }

#endif

#ifdef BT_CP_CONNECTED
  block_signal(SIGTERM,1);
  XpanManager::Get()->Initialize();
  block_signal(SIGTERM,0);
#endif
  return HAL_SUCCESS;
}

#ifdef XPAN_SUPPORTED
  void DataHandler :: XpanInitModules(void) {
    struct timeval tv;
    if ((soc_type_ == BT_SOC_GANGES || soc_type_ == BT_SOC_ORNE) && is_target_support_xpan) {
      SocAddOnFeatures_t* soc_features =  controller_->GetAddOnFeatureList();
      bool soc_supports_xpan = false;

      if (soc_features == NULL) {
        ALOGE("soc_features is null checking if property is set", __func__);
      } else if (soc_features->features[XPAN_FEATURE_INDEX] & XPAN_FEATURE_FLAG) {
        ALOGI("%s: SoC supports xpan: Byte[4:8] xpan flag enabled.", __func__);
        soc_supports_xpan = true;
      } else {
        ALOGI("%s: SoC doesn't supports xpan checking if property is set", __func__);
      }

      char value_prop[PROPERTY_VALUE_MAX] = {'\0'};
      ALOGI("%s: Reading a property: adv_transport", __func__);
      property_get("persist.vendor.service.bt.adv_transport", value_prop, "");
      if (strcmp(value_prop, "") == 0) {
        property_set("persist.vendor.service.bt.adv_transport", "true");
        xpan_supported = true;
        ALOGI("%s: set XPAN is supported", __func__);
      } else if (strcmp(value_prop, "true") == 0) {
        ALOGI("%s: XPAN is supported", __func__);
        xpan_supported = true;
      } else {
        ALOGI("%s: XPAN is not supported", __func__);
        xpan_supported = false;
      }

      property_set("persist.vendor.qcom.bluetooth.adv_transport",
                                     xpan_supported ? "true" : "false");

      char value_prop_xpan[PROPERTY_VALUE_MAX] = {'\0'};
      property_get("persist.vendor.qcom.bluetooth.adv_transport",
                                                  value_prop_xpan, "false");
      if (strcmp(value_prop_xpan, "true") == 0) {
        ALOGI("%s: vendor prop for xpan is set", __func__);
      } else {
        ALOGI("%s: vendor prop for xpan is not set", __func__);
      }

      gettimeofday(&tv, NULL);
      logger_->SetCurrentactivityStartTime(tv,
        BT_HOST_REASON_XPAN_THREAD_START_STUCK, "XPAN THREAD START STUCK");
      logger_->SetPrimaryCrashReason(BT_HOST_REASON_INIT_FAILED);
      logger_->SetSecondaryCrashReason(BT_HOST_REASON_XPAN_THREAD_START_STUCK);
      if (xpan_supported) {
        //block SIGTERM signal until init of XPAN modules is done
        block_signal(SIGTERM,1);
        QHci::Get()->Init();
        is_xpan_supported = xpan_supported;
        XpanManager::Get()->EnableXpanModules(is_xpan_supported);
        //unblock SIGTERM signal, after init done
        block_signal(SIGTERM,0);
      }
      gettimeofday(&tv, NULL);
      logger_->CheckAndAddToDelayList(&tv);
      logger_->SetSecondaryCrashReason(BT_SOC_REASON_DEFAULT);
    }
  }
#endif

#ifdef QTI_BT_FMD_SUPPORTED
template <typename T>
struct SysfsStringEnumMap {
    const char* s;
    T val;
};

template <typename T>
static std::optional<T> mapSysfsString(const char* str, SysfsStringEnumMap<T> map[]) {
    for (int i = 0; map[i].s; i++)
        if (!strcmp(str, map[i].s))
            return map[i].val;

    return std::nullopt;
}

enum BatteryStatus {
    BATTERY_STATUS_UNKNOWN = 1,
    BATTERY_STATUS_CHARGING = 2,
    BATTERY_STATUS_DISCHARGING = 3,
    BATTERY_STATUS_NOT_CHARGING = 4,
    BATTERY_STATUS_FULL = 5,
};

enum BatteryChargingState {
    BATTERY_STATE_INVALID = 0,
    /**
     * Default state.
     */
    BATTERY_STATE_NORMAL = 1,
    /**
     * Reported when the battery is too cold to charge at a normal
     * rate or stopped charging due to low temperature.
     */
    BATTERY_STATE_TOO_COLD = 2,
    /**
     * Reported when the battery is too hot to charge at a normal
     * rate or stopped charging due to hot temperature.
     */
    BATTERY_STATE_TOO_HOT = 3,
    /**
     * The device is using a special charging profile that designed
     * to prevent accelerated aging.
     */
    BATTERY_STATE_LONG_LIFE = 4,
    /**
     * The device is using a special charging profile designed to
     * improve battery cycle life, performances or both.
     */
    BATTERY_STATE_ADAPTIVE = 5,
};

enum BatteryCapacityLevel {
    /**
     * Battery capacity level is unsupported.
     * Battery capacity level must be set to this value if and only if the
     * implementation is unsupported.
     */
    BATTERY_CAPACITY_UNSUPPORTED = -1,
    /**
     * Battery capacity level is unknown.
     * Battery capacity level must be set to this value if and only if battery
     * is not present or the battery capacity level is unknown/uninitialized.
     */
    BATTERY_CAPACITY_UNKNOWN,
    /**
     * Battery is at critical level. The Android framework must schedule a
     * shutdown when it sees this value from the HAL.
     */
    BATTERY_CAPACITY_CRITICAL,
    /**
     * Battery is low. The Android framework may limit the performance of
     * the device when it sees this value from the HAL.
     */
    BATTERY_CAPACITY_LOW,
    /**
     * Battery level is normal.
     */
    BATTERY_CAPACITY_NORMAL,
    /**
     * Battery level is high.
     */
    BATTERY_CAPACITY_HIGH,
    /**
     * Battery is full. It must be set to FULL if and only if battery level is
     * 100.
     */
    BATTERY_CAPACITY_FULL,
};

int getBatteryStatus(const char* status) {
    static SysfsStringEnumMap<int> batteryStatusMap[] = {
            {"Unknown", BATTERY_STATUS_UNKNOWN},
            {"Charging", BATTERY_STATUS_CHARGING},
            {"Discharging", BATTERY_STATUS_DISCHARGING},
            {"Not charging", BATTERY_STATUS_NOT_CHARGING},
            {"Full", BATTERY_STATUS_FULL},
            {NULL, BATTERY_STATUS_UNKNOWN},
    };

    auto ret = mapSysfsString(status, batteryStatusMap);
    if (!ret) {
        ALOGW("Unknown battery status '%s'\n", status);
        *ret = BATTERY_STATUS_UNKNOWN;
    }

    return *ret;
}

bool DataHandler::IsShutdownFMDTriggered() {
  char shutdown_prop[PROPERTY_VALUE_MAX] = {'\0'};
  property_get("sys.shutdown.requested", shutdown_prop, "");
  ALOGI("%s: shutdown_prop = %s", __func__, shutdown_prop);

  const char* user_req_str = "0user";
  const char* battery_str = "0battery";
  const char* empty_str = "";

  std::string shutdown_req_str = shutdown_prop;
  if (shutdown_req_str.find(battery_str) != std::string::npos) {
    ALOGI("%s: FMD ShutDown Mode Active due to Battery Drain", __func__);
    return true;
  } else if ((shutdown_req_str.find(user_req_str) != std::string::npos) || shutdown_req_str == "0" ) {
    ALOGI("%s: FMD ShutDown Mode Active due to user shutdown", __func__);
    return true;
  } else if (shutdown_req_str.find(empty_str) != std::string::npos &&
      data_handler->CheckSignalCaughtStatus()) {
      std::string present_battery_capacity_buf;
      std::string present_battery_level_buf;
      std::string present_battery_status_buf;
      int present_battery_capacity = 0;
      int present_present_battery_status = 0;
      int present_present_battery_level = 0;
      android::base::ReadFileToString("/sys/class/power_supply/battery/present", &present_battery_capacity_buf);
      android::base::ReadFileToString("/sys/class/power_supply/battery/capacity", &present_battery_level_buf);
      android::base::ReadFileToString("/sys/class/power_supply/battery/status", &present_battery_status_buf);
      ALOGI("%s:present:battery level = %s", __func__, present_battery_capacity_buf.c_str());
      ALOGI("%s:present:battery percentage = %s", __func__, present_battery_level_buf.c_str());
      ALOGI("%s:present:battery status = %s", __func__, present_battery_status_buf.c_str());
      present_battery_capacity = atoi(present_battery_capacity_buf.c_str()); 
      present_present_battery_level = atoi(present_battery_level_buf.c_str()); 
      present_present_battery_status = getBatteryStatus(present_battery_status_buf.c_str()); 
      ALOGI("%s:present:battery level = %d", __func__, present_battery_capacity);
      ALOGI("%s:present:battery percentage = %d %%", __func__, present_present_battery_level);
      ALOGI("%s:present:battery status = %d", __func__, present_present_battery_status);
      if ((present_battery_capacity == BATTERY_CAPACITY_CRITICAL) &&
        (present_present_battery_status != BATTERY_STATUS_CHARGING ||
            present_present_battery_status != BATTERY_STATUS_FULL) &&
        present_present_battery_level < CRITICAL_BATTERY_PERCENTAGE) {
          ALOGI("%s: FMD ShutDown Mode Active due to Battery level is critical", __func__);
          return true;
      } else {
        ALOGI("%s: FMD ShutDown Mode Not Active as Battery level is normal", __func__);
        return false;
      }
  } else {
    return false;
  }
}

void DataHandler::UserSetFmdMode(bool enable) {
  if ((GetSocType() != BT_SOC_GANGES) && (GetSocType() != BT_SOC_ORNE) && (GetSocType() != BT_SOC_COLOGNE) && (GetSocType() != BT_SOC_THEMISTO)) {
    ALOGW("%s: FMD Mode Not Supported for %d for enable %d", __func__,
             GetSocType(), enable);
    isFmdSupported = false;
    vtsWarFmdTestCase = enable;
    return;
  }

  if (isFmdSupported && enable) {
    ALOGI("%s: FMD Mode already Enabled", __func__);
  } else if (!isFmdSupported && !enable) {
    ALOGI("%s: FMD Mode already disabled", __func__);
  } else if (!isFmdSupported && enable) {
    char value_prop_pf[PROPERTY_VALUE_MAX] = {'\0'};
    ALOGI("%s: FMD Mode Needs to enable ", __func__);
    isFmdSupported = true;
    GenerateKeyData();
    property_set("persist.vendor.bluetooth.fmd_support", "true");
    GetFmdHeaderDataInfo();
    GetFmdConfigInfo();
  } else if (isFmdSupported && !enable) {
    ALOGI("%s: FMD Mode Needs to disable ", __func__);
    isFmdSupported = false;
    property_set("persist.vendor.bluetooth.fmd_support", "false");
  }
}

const char* DataHandler::GetFmdOperationString(uint8_t operation) {
  switch (operation) {
    case ENABLE_FMD:
      return "ENABLE_FMD";
    case DISABLE_FMD:
      return "DISABLE_FMD";
    case UPDATE_SOC_VER:
      return "UPDATE_SOC_VER";
    default:
      return "UNKNOWN FMD OPERATION";
  }
}

int DataHandler::fmdOperationNotifyToSPIDriver(enum FmdOperation operation) {

  fmdStruct.fmdOperation = operation;

  switch (fmdStruct.fmdOperation) {
    case UPDATE_SOC_VER: {
      uint64_t socFwVer = GetChipVersion();
      if (socFwVer == GANGES_VER_1_0 || socFwVer == BRAHMA_VER_1_0) {
        fmdStruct.socFwVer = (uint8_t) GNG_SOC_VER_1_0;
      } else if (socFwVer == GANGES_VER_2_0 || socFwVer == BRAHMA_VER_2_0) {
        fmdStruct.socFwVer = (uint8_t) GNG_SOC_VER_2_0;
      } else if ((GetSocType() == BT_SOC_ORNE) || (GetSocType() == BT_SOC_COLOGNE)) {
        fmdStruct.socFwVer = (uint8_t) OTHER_FMD_SUPPORTED_BT_SOC;
      } else if ((GetSocType() == BT_SOC_THEMISTO)) {
        fmdStruct.socFwVer = (uint8_t) TMO_SOC_VER_1_0 ;
      }
      break;
    }
    case ENABLE_FMD:
    case DISABLE_FMD:
      break;
    default:
      ALOGE("Invalid FMD operation received,"
        " Not notifying to SPI Client Driver, returning from here", __func__);
      return INVALID_FMD_OPERATION;
  }

  ALOGI("%s: Notifying to SPI Client Driver for FMD operation = %s",
    __func__, GetFmdOperationString(fmdStruct.fmdOperation));

  int err = ioctl(DataHandler::Get()->GetSpiDriverFd(),
    fmd_operation_cmd_, (void*)&fmdStruct);

  if( err < 0) {
    ALOGE(" IOCTL failed to notify fmd operation to Kernel :%d error =(%s)", err, strerror(errno));
    return PWR_DRV_FMD_OPERATION_FAILED;
  }

  return PWR_DRV_FMD_OPERATION_SUCESSFULL;
}

bool DataHandler::GetFmdMode() {
  if ((GetSocType() != BT_SOC_GANGES) && (GetSocType() != BT_SOC_ORNE) && (GetSocType() != BT_SOC_COLOGNE) && (GetSocType() != BT_SOC_THEMISTO)) {
    ALOGW("%s: FMD Mode Not Supported for %d", __func__, GetSocType());
    return vtsWarFmdTestCase;
  }

  ALOGI("%s: mode %d ", __func__, isFmdSupported);
  return isFmdSupported;
}

void DataHandler::CheckAndSetFmdMode() {
  if (isFmdSupported) {
    if ((GetSocType() == BT_SOC_GANGES) || (GetSocType() == BT_SOC_ORNE) || (GetSocType() == BT_SOC_COLOGNE) || (GetSocType() == BT_SOC_THEMISTO)) {
      if (IsShutdownFMDTriggered()) {
        uint8_t curr_fmd_keys_status = fmd_keys_status;
        if (fmd_keys_status != FINDER_KEYS_RECEIVED) {
          ALOGI("%s: Wait for Finder App Keys - FMD_KEYS_STATUS = %d",
                  __func__, fmd_keys_status);
          fmd_keys_status = AWAITING_FINDER_KEYS;
          FmdWaitForKeys(FMD_KEYS_TIMEOUT);
        }
        ALOGW("%s: FMD_KEYS_STATUS = %d ", __func__, fmd_keys_status);
        if ((curr_fmd_keys_status != DEFAULT_FINDER_KEYS_ASSIGNED) &&
            (fmd_keys_status != FINDER_KEYS_RECEIVED)) {
          ALOGW("%s: Disable FMD Mode as NO FMD_KEYS RECEIVED from Finder APP", __func__);
          SetFmdMode(false);
        } else {
          SetFmdMode(true);
        }
      } else {
        SetFmdMode(false);
      }
    } else {
      SetFmdMode(false);
    }
  }
  return;
}

void DataHandler::sendEids(uint8_t *key_data, uint16_t num_keys) {
  if (isFmdSupported) {
    uint16_t length = num_keys * 20;
    ALOGI("%s: length %d", __func__, num_keys);
    if (length <= FMD_TOTAL_ADV_DATA_KEY_SIZE) {
      memcpy(fmd_key_data, key_data, length);
      fmd_info.no_of_keys = num_keys;
      fmd_info.length = length;
      ALOGI("%s: no_of_keys %d %d", __func__, fmd_info.no_of_keys,
              fmd_info.length);
    }
    if (fmd_keys_status == AWAITING_FINDER_KEYS) {
      FmdFinderKeysReceived();
    } else {
      fmd_keys_status = FINDER_KEYS_RECEIVED;
    }
  }
}

void DataHandler::SendFmdCmdsToSoc() {
  ProtocolType type = TYPE_BT;
  fmd_info.fmd_mode_progress = true;
  if (GetChipVersion() != GANGES_VER_1_0 &&
      GetChipVersion() != BRAHMA_VER_1_0) {
    ALOGI("%s: Send HCI_SET_FMD_CONFIG_PARAMS_V2_CMD to SOC", __func__);
    if (!data_handler->sendCommandWait(HCI_SET_FMD_CONFIG_PARAMS_V2_CMD,
                                        type)) {
      ALOGE("%s: Failed to receive response for"
            "HCI_SET_FMD_CONFIG_PARAMS_V2_CMD", __func__);
      goto NEGATIVE_OR_NO_RSP_FROM_SOC;
    }
    ALOGD("%s: Successfully Sent HCI_SET_FMD_CONFIG_PARAMS_V2_CMD to Soc", __func__);
  } else {
    ALOGI("%s: Send HCI_SET_FMD_CONFIG_PARAMS_CMD to SOC", __func__);
    if (!data_handler->sendCommandWait(HCI_SET_FMD_CONFIG_PARAMS_CMD, type)) {
      ALOGE("%s: Failed to receive response for "
            "HCI_SET_FMD_CONFIG_PARAMS_CMD", __func__);
      goto NEGATIVE_OR_NO_RSP_FROM_SOC;
    }
    ALOGD("%s: Successfully Sent HCI_SET_FMD_CONFIG_PARAMS_CMD to Soc", __func__);
  }
  {
    ALOGI("%s: Send fmd keys through HCI_WRITE_FMD_ADV_DATA_CMD to SOC", __func__);
    uint16_t num_of_pkts = (fmd_info.length / HCI_VSC_FMD_ADV_DATA_PKT_LEN);

    if (fmd_info.length % HCI_VSC_FMD_ADV_DATA_PKT_LEN)
      num_of_pkts++;

    for (int i = 0, fmd_key_inx = 0; i < num_of_pkts; i++) {
      if (fmd_key_inx < fmd_info.length) {

        if (!fmd_info.fmd_mode_progress)
          goto NEGATIVE_OR_NO_RSP_FROM_SOC;
        ALOGI("%s: Fmd Keys frame num #%d/%d", __func__, i+1,num_of_pkts);
        if (!data_handler->sendCommandWait(HCI_WRITE_FMD_ADV_DATA_CMD, type)) {
          ALOGE("%s: Failed to receive response for"
                 "HCI_WRITE_FMD_ADV_DATA_CMD", __func__);
          goto NEGATIVE_OR_NO_RSP_FROM_SOC;
        }
      }
    }
    ALOGD("%s: Successfully Sent Fmd keys to soc", __func__);
  }

  if (!fmd_info.fmd_mode_progress)
    goto NEGATIVE_OR_NO_RSP_FROM_SOC;

  ALOGI("Send HCI_SET_FMD_MODE_CMD to Soc\n");
  if (!data_handler->sendCommandWait(HCI_SET_FMD_MODE_CMD, type)) {
    ALOGE("%s: Failed to receive response for HCI_SET_FMD_MODE_CMD", __func__);
  } else if (fmd_info.fmd_mode_progress) {
    ALOGD("%s: Successfully Sent HCI_SET_FMD_MODE_CMD to Soc ", __func__);
    fmdOperationNotifyToSPIDriver(ENABLE_FMD);
    return;
  }

NEGATIVE_OR_NO_RSP_FROM_SOC:
  if(GetSocType() != BT_SOC_THEMISTO)
    fmdOperationNotifyToSPIDriver(DISABLE_FMD);
  return;
}

std::optional<std::string> GetSystemProperty(const std::string& property)
{
  std::array<char, PROPERTY_VALUE_MAX> value_array{0};
  auto value_len = property_get(property.c_str(), value_array.data(), nullptr);
  if (value_len <= 0) {
    return std::nullopt;
  }
  return std::string(value_array.data(), value_len);
}

void DataHandler::GetFmdHeaderDataInfo()
{
  char* tok = NULL;
  char *config_info = NULL;
  int i = 0;
  uint8_t valid_delimiter = 0;

  char prop_info[PROPERTY_VALUE_MAX] = {'\0'};
  property_get("persist.vendor.qcom.bluetooth.fmd_header", prop_info,
               "8:02:01:06:18:16:AA:FE:40");
  ALOGI("%s: FMD Header Value ---%s", __func__, prop_info);
  //Extract Config_header Length
  tok = strtok_r(prop_info, ":", &config_info);
  if (tok != NULL) {
    fmd_config.header_length = strtol(tok, NULL, 16);
  }
  std::optional<std::string> result =
    GetSystemProperty("persist.vendor.qcom.bluetooth.fmd_header");
  valid_delimiter = count(result->begin(), result->end(), ':');

  ALOGD("%s: Config_header_length %d valid_delimiter %d", __func__,
        fmd_config.header_length, valid_delimiter);

  if ((fmd_config.header_length <= MAX_FMD_CONFIG_HEADER_LENGTH) &&
       (valid_delimiter == fmd_config.header_length)){
    while (i < fmd_config.header_length) {
      tok = strtok_r(NULL, ":", &config_info);
      if (tok != NULL) {
        fmd_config.config_header[i] = strtol(tok, NULL, 16);
      }
      ALOGD("%s: Config_header_data index %d 0x%x", __func__, i,
             fmd_config.config_header[i]);
      i++;
    }
  } else {
    ALOGE("%s Invalid Config Header Length. Set to Default values", __func__);
    fmd_config.header_length = 8;
    uint8_t default_header[] = {0x02, 0x01, 0x06, 0x18, 0x16, 0xAA, 0xFE, 0x40};
    memcpy(&fmd_config.config_header[0], &default_header[0],
            fmd_config.header_length);
    for (int i = 0; i < fmd_config.header_length; i++) {
      ALOGD("Fmd_header_info [%d]: 0x%x", i, fmd_config.config_header[i]);
    }
  }
}

void DataHandler::SetDefaultFmdConfigParams() {
  ALOGD("%s ", __func__);

  //2 sec Adv interval
  fmd_config.adv_interval[0] = 0x80;
  fmd_config.adv_interval[1] = 0x0C;

  //17mins key rotation timer value
  fmd_config.advData_Timeout[0] = 0x60;
  fmd_config.advData_Timeout[1] = 0x90;
  fmd_config.advData_Timeout[2] = 0x0F;
  fmd_config.advData_Timeout[3] = 0x00;

  fmd_config.tx_pwr = 0x7F;
  fmd_config.per_pkt_key_length = 0x14;
  fmd_config.restart_keys = 1;
  ALOGD("FMD Adv interval 0x%x",
        (fmd_config.adv_interval[1] << 8)|fmd_config.adv_interval[0]);
  ALOGD("FMD Key Rotation Timeout 0x%x", fmd_config.advData_Timeout[0] |
                                (fmd_config.advData_Timeout[1] << 8) |
                                (fmd_config.advData_Timeout[2] << 16) |
                                (fmd_config.advData_Timeout[3] << 24));

  ALOGD("FMD Tx_pwr 0x%x", fmd_config.tx_pwr);
  ALOGD("FMD Each Key Data 0x%x", fmd_config.per_pkt_key_length);
  ALOGD("FMD Restart_keys 0x%x", fmd_config.restart_keys);

}

void DataHandler::GetFmdPropValues()
{
  memset(&fmdStruct, 0 , sizeof(fmdStruct));
  char value[PROPERTY_VALUE_MAX] = {'\0'};

  logger_->PropertyGet("persist.vendor.bluetooth.fmd.do_not_reboot_with_usb_conn", value, "-1");
  if (strcmp(value, "true") == 0) {
    fmdStruct.rebootStatus = true;
    ALOGI("%s: property do_not_reboot_with_usb_conn set to NOT REBOOT (%d)\n",
      __func__, fmdStruct.rebootStatus);
  } else if(strcmp(value, "false") == 0) {
    fmdStruct.rebootStatus = false;
    ALOGI("%s: property do_not_reboot_with_usb_conn set to REBOOT (%d)\n",
      __func__, fmdStruct.rebootStatus);
  } else if (strcmp(value, "-1") == 0) {
    fmdStruct.rebootStatus = -1;
    ALOGI("%s: property do_not_reboot_with_usb_conn didn't configured\n", __func__);
  } else {
    fmdStruct.rebootStatus = -1;
    ALOGI("%s: property do_not_reboot_with_usb_conn configured invalid option = %d\n",
      __func__, atoi(value));
  }

  logger_->PropertyGet("persist.vendor.bluetooth.fmd.stop_counter", value, "-1");
  if (strcmp(value, "-1") == 0) {
    fmdStruct.fmd_stop_counter = -1;
    ALOGI("%s: property fmd_stop_counter didn't configured\n", __func__);
  } else {
    int16_t temp = (int16_t) atoi(value);
    if (temp >= 0 && temp <= 0xFF) {
      fmdStruct.fmd_stop_counter = temp;
      ALOGI("%s: property fmd_stop_counter set to (0x%02x)\n",
        __func__, fmdStruct.fmd_stop_counter);
    } else {
      fmdStruct.fmd_stop_counter = -1;
      ALOGI("%s: property fmd_stop_counter set to invalid count = (0x%02x),"
        " only allowed b/w the 0 to 0xFF range\n", __func__, temp);
    }
  }

  logger_->PropertyGet("persist.vendor.bluetooth.fmd.log_soc_cmd", value, "false");
  if (strcmp(value, "true") == 0) {
    log_fmd_soc_cmd = true;
    ALOGI("%s: property log_fmd_soc_cmd = (%d), Log all the FMD cmd sending to SOC\n",
      __func__, log_fmd_soc_cmd);
  } else {
    log_fmd_soc_cmd = false;
    ALOGI("%s: property log_soc_cmd = (%d), Loging FMD cmd sent to SOC Disabled\n",
      __func__, log_fmd_soc_cmd);
  }
}

void DataHandler::GetFmdConfigInfo()
{
  char* tok = NULL;
  char *config_info = NULL;
  int i = 0;

  std::optional<std::string> result =
    GetSystemProperty("persist.vendor.qcom.bluetooth.fmd_config_info");

  if (result) {
    ALOGD("Property Len : %d Property Value %s", result->size(), result->data());
    int valid_delimiter = count(result->begin(), result->end(), ':');
    if ((result->size() == 26) && valid_delimiter == 8) {
      char prop_info[PROPERTY_VALUE_MAX] = {'\0'};
      property_get("persist.vendor.qcom.bluetooth.fmd_config_info", prop_info,
                    "80:0C:60:90:0F:00:00:14:01");
      //Extract 2 Bytes of ADV Interval
      tok = strtok_r(prop_info, ":", &config_info);
      if (tok != NULL) {
        fmd_config.adv_interval[0] = strtol(tok, NULL, 16);
      }
      tok = strtok_r(NULL, ":", &config_info);
      if (tok != NULL) {
        fmd_config.adv_interval[1] = strtol(tok, NULL, 16);
      }
      ALOGD("%s: Adv Interval 0x%x 0x%x", __func__,
              fmd_config.adv_interval[0],
              fmd_config.adv_interval[1]);
      uint16_t adv_interval = (fmd_config.adv_interval[1] << 8 |
                                fmd_config.adv_interval[0]);
      if (adv_interval > LE_MAX_ADV_INTERVAL) {
        ALOGE("%s: Invalid Adv Value 0x%x. Setting to default value 2sec",
                __func__, adv_interval);
        fmd_config.adv_interval[0] = 0x80;
        fmd_config.adv_interval[1] = 0x0C;
      }

      //Extract 4 Bytes of Key data Timeout
      for (i = 2; i < 6; i++) {
        tok = strtok_r(NULL, ":", &config_info);
        if (tok != NULL) {
          fmd_config.advData_Timeout[i-2] = strtol(tok, NULL, 16);
        }
        ALOGD("%s:  advData_Timeout 0x%x", __func__,
                fmd_config.advData_Timeout[i-2]);
      }

      uint32_t key_timeout_val = fmd_config.advData_Timeout[0] |
                                (fmd_config.advData_Timeout[1] << 8) |
                                (fmd_config.advData_Timeout[2] << 16) |
                                (fmd_config.advData_Timeout[3] << 24);
      if (key_timeout_val > MAX_KEY_ROTATION_TIMER_VALUE) {
        ALOGE("%s: Invalid key rotation Timeout value 0x%x"
                "Setting to default value 17min",
                __func__, key_timeout_val);
        fmd_config.advData_Timeout[0] = 0x60;
        fmd_config.advData_Timeout[1] = 0x90;
        fmd_config.advData_Timeout[2] = 0x0F;
        fmd_config.advData_Timeout[3] = 0x00;
      }

      //Extract Tx_pwr
      tok = strtok_r(NULL, ":", &config_info);
      if (tok != NULL)
        fmd_config.tx_pwr = strtol(tok, NULL, 16);

      if ((int8_t)fmd_config.tx_pwr < (int8_t)0xE4 || (int8_t)fmd_config.tx_pwr > (int8_t)0x14) {
        ALOGE("%s:  Tx_pwr 0x%x is out of range [-28 to 20]. Setting to default 0x7F.",
              __func__, fmd_config.tx_pwr);
        fmd_config.tx_pwr = 0x7F;
      }

      //Extract Each Keydata Size
      tok = strtok_r(NULL, ":", &config_info);
      if (tok != NULL)
        fmd_config.per_pkt_key_length = strtol(tok, NULL, 16);

      if (fmd_config.per_pkt_key_length != 0x14) {
        fmd_config.per_pkt_key_length = 0x14;
        ALOGE("%s:  Current supported Length is 20bytes", __func__);
      }

      //Extract restart Keys flag
      tok = strtok_r(NULL, ":", &config_info);
      if (tok != NULL)
        fmd_config.restart_keys = strtol(tok, NULL, 16);

      if (fmd_config.restart_keys > 1) {
        ALOGE("%s: Invalid restart_keys 0x%x .Setting default value 0", __func__,
                fmd_config.restart_keys);
        fmd_config.restart_keys = 1;
      }
    } else {
      //Set default parameters
      SetDefaultFmdConfigParams();
    }
  } else {
    SetDefaultFmdConfigParams();
  }

}
#endif

#ifdef PASSTHROUGH_ENABLED

std::vector<uint8_t> DataHandler::build_switch_spi_to_ipc_vsc() {
  ALOGD("%s:",__func__);
  uint16_t opcode = HCI_VS_GENERAL_OPCODE_PERI;
  std::vector<uint8_t> data;
  data.push_back((uint8_t)HCI_PACKET_TYPE_PERI_CMD);
  data.push_back((uint8_t)BT_HOST_ID);
  data.push_back((uint8_t)opcode);
  data.push_back((uint8_t)(opcode >> 8));
  data.push_back(3); // Length of Data
  data.push_back((uint8_t)HCI_PERI_BT_SPI_TXP_SWITCH_SUB_OPCODE); // Sub Opcode
  data.push_back((uint8_t)TRANSPORT_ID_SPI);
  data.push_back((uint8_t)TRANSPORT_ID_UART);
  return data;
}

std::vector<uint8_t> DataHandler::build_switch_uart_to_spi_vsc() {
  ALOGD("%s:",__func__);
  uint16_t opcode = HCI_VS_GENERAL_OPCODE_PERI;
  std::vector<uint8_t> data;
  data.push_back((uint8_t)HCI_PACKET_TYPE_PERI_CMD);
  data.push_back((uint8_t)BT_HOST_ID);
  data.push_back((uint8_t)opcode);
  data.push_back((uint8_t)(opcode >> 8));
  data.push_back(3); // Length of Data
  data.push_back((uint8_t)HCI_PERI_BT_SPI_TXP_SWITCH_SUB_OPCODE); // Sub Opcode
  data.push_back((uint8_t)TRANSPORT_ID_UART); //current txp uart
  data.push_back((uint8_t)TRANSPORT_ID_SPI); // new txp spi
  return data;
}

std::string DataHandler::build_passthrough_enable_msg(){
  uint8_t passthrough_enable[8];
  uint16_t forOpcode = PASSTHROUGH_ENABLE;
  uint16_t protoEncoded = PACKET_NOT_ENCODED;
  uint16_t length = 2;
  uint16_t mode;
#ifdef GOOGLE_OFFLOAD_ENABLED
  if(getFtmStatus() == true){
    mode = PASSTHROUGH_MODE;
  }else{
    mode = OFFLOAD_MODE;
  }
#else
  mode = PASSTHROUGH_MODE;
#endif
  passthrough_enable[0] = forOpcode & 0xff;
  passthrough_enable[1] = (forOpcode >> 8);
  passthrough_enable[2] = protoEncoded & 0xff;
  passthrough_enable[3] = (protoEncoded >> 8);
  passthrough_enable[4] = length & 0xff;
  passthrough_enable[5] = (length >> 8);
  passthrough_enable[6] = mode & 0xff;
  passthrough_enable[7] = (mode >> 8);

  ALOGD("%s: %s :: %02x %02x %02x %02x %02x %02x %02x %02x",
        __func__, (mode == OFFLOAD_MODE) ? "offload mode" : "passthrough mode",
        passthrough_enable[0], passthrough_enable[1], passthrough_enable[2],
        passthrough_enable[3], passthrough_enable[4], passthrough_enable[5],
        passthrough_enable[6], passthrough_enable[7]);

  std::string msgStr(reinterpret_cast<char*>(passthrough_enable), sizeof(passthrough_enable));
  return msgStr;
}

std::string DataHandler::build_passthrough_disable_msg(){
  uint8_t passthrough_disable[8];
  uint16_t forOpcode = PASSTHROUGH_DISABLE;
  uint16_t protoEncoded = PACKET_NOT_ENCODED;
  uint16_t length = 2;
  uint16_t mode;
#ifdef GOOGLE_OFFLOAD_ENABLED
  if( diag_interface_.isSsrTriggered() || diag_interface_.isDiagSsrTriggered()) {
    mode = SSR_MODE;
  }else if(getFtmStatus() == true){
    mode = PASSTHROUGH_MODE;
  }else if(isFMDentered == STATUS_SUCCESS){
    mode = FMD_MODE;
  }
  else{
    mode = OFFLOAD_MODE;
  }
#else
  mode = PASSTHROUGH_MODE;
#endif

  passthrough_disable[0] = forOpcode & 0xff;
  passthrough_disable[1] = (forOpcode >> 8);
  passthrough_disable[2] = protoEncoded & 0xff;
  passthrough_disable[3] = (protoEncoded >> 8);
  passthrough_disable[4] = length & 0xff;
  passthrough_disable[5] = (length >> 8);
  passthrough_disable[6] = mode & 0xff;
  passthrough_disable[7] = (mode >> 8);

  ALOGD("%s: passthrough_disable is :: %02x %02x %02x %02x %02x %02x %02x %02x",
        __func__, passthrough_disable[0], passthrough_disable[1], passthrough_disable[2],
        passthrough_disable[3], passthrough_disable[4], passthrough_disable[5],
        passthrough_disable[6], passthrough_disable[7]);

  std::string msgStr(reinterpret_cast<char*>(passthrough_disable), sizeof(passthrough_disable));
  return msgStr;
}

void DataHandler::setIsPassthroughEnabled(bool isPassthroughEnableSuccess){
  is_passthrough_enable_success = isPassthroughEnableSuccess;
}

bool DataHandler::getIsPassthroughEnabled(){
  return is_passthrough_enable_success;
}

#endif

bool DataHandler::Close(ProtocolType type)
{
  char dst_buff[MAX_BUFF_SIZE];
  char type_buff[MAX_BUFF_SIZE];
  int retFMDEntry = 1;
  struct timeval tv;
  std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;

  ALOGI("%s: ProtocolType = %d", __func__, type);
#ifdef QTI_BT_FMD_SUPPORTED
  if ((GetSocType() == BT_SOC_GANGES) || (GetSocType() == BT_SOC_ORNE) || (GetSocType() == BT_SOC_COLOGNE) || (GetSocType() == BT_SOC_THEMISTO)) {
    CheckAndSetFmdMode();
    if (IsFmdModeEnabled() && (type == TYPE_BT)) {
      if(GetSocType() == BT_SOC_THEMISTO){
#ifdef IS_SMC_HAL_V2
        SMCLoader* smc_loader = SMCLoader::getInstance();
        retFMDEntry = smc_loader->prepareFMDEntry();
        ALOGE("%s: prepareFMDEntry return = %d", __func__, retFMDEntry);
#endif
        if(retFMDEntry == STATUS_SUCCESS){
          switch (fmdOperationNotifyToSPIDriver(UPDATE_SOC_VER)) {
            case PWR_DRV_FMD_OPERATION_SUCESSFULL:
              ALOGI("%s: Successfully notified to SPI Client Driver, configure SOC for FMD Operation",__func__);
            break;
            case PWR_DRV_FMD_OPERATION_FAILED:
              ALOGE("%s: Not configuring SOC for FMD, as failure encountered to notify SPI Client Driver",
                __func__);
              fmdOperationNotifyToSPIDriver(DISABLE_FMD);
              SetFmdMode(false);
            break;
            case INVALID_FMD_OPERATION:
              ALOGE("%s: Not configuring SOC for FMD, as invalid operation feeded", __func__);
              SetFmdMode(false);
            break;
          }
        }
      }
    }
  }
#endif

  if (protocol_info_.size() == 0 && secureEvent) {
    ALOGD("%s: Cleanup is already done. Return", __func__);
    return true;
  }

#ifdef BT_CP_CONNECTED
  if (type == TYPE_BT) {
    ALOGD("%s Initiating close to XM with xpan_supported %d", __func__, is_xpan_supported);
    XpanManager::Get()->Deinitialize(is_xpan_supported);
  }
#endif

  /* stop init timer if close is called by last client. */
  if (protocol_info_.size() == MIN_CLIENTS_ACTIVE) {
    StopInitTimer();
    {
      std::unique_lock<std::mutex> lock(property_reset_mutex_);
      is_last_client_cleanup_in_progress = true;
      if (prop_reset_thread.joinable()) {
        ALOGD("%s: sending SIGUSR1 signal", __func__);
        pthread_kill(prop_reset_thread.native_handle(), SIGUSR1);
        ALOGD("%s: joining prop reset thread", __func__);
        prop_reset_thread.join();
        ALOGI("%s: joined prop reset thread", __func__);
      }
    }
  }

  if (getCalibrationStatus() == CAL_STATUS::SOC_CALIBRATION_SUCCESSFULL) {
    ALOGI("%s: Calibration data loging is going on, wait for (%d msec) to complete\n",
      __func__, CALIBRATION_DATA_LOG_TIME);
    closeWaitForCal(CALIBRATION_DATA_LOG_TIME);
    ALOGI("%s: Come out form wating, Initiate the cleanup process\n", __func__);
  }

  std::unique_lock<std::mutex> lock(property_cal_tread_mutex_);
  if (calibration_thread.joinable()) {
    ALOGD("%s: calibration_thread: sending SIGUSR1 signal", __func__);
    pthread_kill(calibration_thread.native_handle(), SIGUSR1);
    ALOGD("%s: joining calibration_thread", __func__);
    calibration_thread.join();
    ALOGI("%s: joined calibration_thread", __func__);
  }

  if (!controller_) {
#ifdef WAKE_LOCK_ENABLED
    Wakelock :: CleanUp();
#endif
    /* Do cleanup if controller init failed */
    if (init_status_ == INIT_STATUS_FAILED) {
      if (init_thread_.joinable()) {
        ALOGD("%s: joining init thread", __func__);
        init_thread_.join();
        ALOGI("%s: joined init thread", __func__);
      }
      return true;
    } else {
      if (init_thread_.joinable()) {
        ALOGD("%s: sending SIGUSR1 signal", __func__);
        pthread_kill(init_thread_.native_handle(), SIGUSR2);
        ALOGD("%s: joining init thread", __func__);
        init_thread_.join();
        ALOGI("%s: joined Init thread", __func__);
      }
      controller_ = NULL;
      // Checking delay list and set appropriate crash reason.
      logger_->CheckDelayListAndSetCrashReason();
      if (type == TYPE_BT) {
        hidl_vec<uint8_t> *bt_packet_ = new hidl_vec<uint8_t>;
        ProtocolCallbacksType *cb_data;
        logger_->FrameBqrRieEvent(bt_packet_);
        std::unique_lock<std::mutex> guard(internal_mutex_);
        /* Posting crash reason to client */
        auto it = protocol_info_.find(TYPE_BT);
        if (it != protocol_info_.end() && logger_->GetClientStatus(TYPE_BT)) {
          cb_data = (ProtocolCallbacksType*)it->second;
          ALOGD("%s : Sending BQR RIE to BT stack", __func__);
          if (cb_data->data_read_cb != nullptr) {
            cb_data->data_read_cb(HCI_PACKET_TYPE_EVENT, bt_packet_);
          } else {
            ALOGE("%s: data_read_cb is null", __func__);
          }
        }
      }

      /*callback cleanup incase when controller constructor
        call is ongoing/stuck and we call close*/
      ALOGD("%s: deleting callback data", __func__);
      it = protocol_info_.find(type);
      if (it != protocol_info_.end()) {
        ProtocolCallbacksType *cb_data = reinterpret_cast<ProtocolCallbacksType *> (it->second);
        delete (cb_data);
        protocol_info_.erase(it);
      }
      logger_->PrepareDumpProcess();
      logger_->CollectDumps(false, true);
#ifdef USER_DEBUG
      if (data_handler->CheckSignalCaughtStatus()) {
        // Delete current dumped logs, as issue triggered during reboot.
        logger_->DeleteCurrentDumpedFiles();
        /* user triggerred reboot, no need to call abort */
        ALOGE("%s: Killing daemon as user triggered forced reboot", __func__);
        kill(getpid(), SIGKILL);
      } else if (data_handler->GetInitTimerState() != TIMER_OVERFLOW) {
        /* close called before startup time (2.9 sec) is finished  */
        ALOGE("%s: Killing daemon to recover as close called before startup timer expiry",
              __func__);
        kill(getpid(), SIGKILL);
      } else {
        if (logger_->GetSecondaryCrashReasonCode() == BT_HOST_REASON_SOC_NAME_UNKOWN) {
          ALOGE("%s: Aborting daemon as IOCTL RETRY for SoC id caused init timeout", __func__);
        } else {
          ALOGE("%s: Aborting daemon to recover as controller constructor is stuck", __func__);
        }
        abort();
      }
#else
      if (logger_->GetSecondaryCrashReasonCode() == BT_HOST_REASON_SOC_NAME_UNKOWN) {
        ALOGE("%s: Killing daemon as IOCTL RETRY for SoC id caused init timeout", __func__);
      } else {
        ALOGE("%s: Killing daemon to recover as controller constructor is stuck", __func__);
      }
      kill(getpid(), SIGKILL);
#endif
    }
    return false;
  }

  if (soc_type_ != BT_SOC_SMD && controller_)
    (controller_)->SetCleanupStatusDuringSSR();

  gettimeofday(&tv, NULL);
  snprintf(type_buff, sizeof(type_buff), "HCI Close rcvd from client type = %d", type);
  BtState::Get()->AddLogTag(dst_buff, tv, type_buff);
  BtState::Get()->SetTsHCIInitClose(HCI_CLOSE, dst_buff);

#ifdef IS_PERI_ENABLED
  if (NotifySignal::Get()->GetSubSystemSsrStatus() != SUB_STATE_IDLE) {
    controller_->WaitforCrashdumpFinish();
  }
#endif

  bool status = false;
  bool Cleanup_Status = true;

  ALOGI("DataHandler:: init_status %d", init_status_);

  ALOGD("%s: Signal close to Diag interface", __func__);
  if (!diag_interface_.SignalHALCleanup(type)) {
    ALOGE("%s: Returning as SSR or cleanup in progress", __func__);
    Cleanup_Status = false;
  }

  /* In case FM/ANT Client we set client status false at the start of close
   * as no data will be sent from now on.
   * In case BT Client we set client status false if init status is INIT_STATUS_SUCCESS
   * as no data will be sent from now on.
   * If init status is INIT_STATUS_INITIALIZING then dont set client status as false as
   * we need to send BQR RIE to BT STACK and after that eventually kill/abort HIDL daemon.
   */
  if (type != TYPE_BT || (type == TYPE_BT && init_status_ == INIT_STATUS_SUCCESS))
    SetClientStatus(false, type);

  /* Stop moving forward if the HAL Cleanup is in process */
  if (!Cleanup_Status)
    return false;

  if (data_handler && type == TYPE_BT && init_status_ == INIT_STATUS_SUCCESS) {
    if (!data_handler->sendCommandWait(HCI_RESET_CMD, type)) {
      ALOGE("%s:Failed to receive response for second HCI RESET CMD", __func__);
      ALOGE("%s SSR is in progress at BT or UWB, returning from here", __func__);
      return false;
    }
  }

#ifdef QTI_BT_FMD_SUPPORTED
  if (IsFmdModeEnabled() && (type == TYPE_BT) && (retFMDEntry == STATUS_SUCCESS)) {
    SendFmdCmdsToSoc();
  }
#endif

  std::unique_lock<std::mutex> guard(internal_mutex_);
  if (protocol_info_.size() == 1) {

    /* Unlock internal mutex to unblock Rx thread if they are any pending packets
     * to be posted, So that CC for Pre shutdown command will be recevied.
     */
    guard.unlock();
    if(data_handler && data_handler->soc_type_ >= BT_SOC_CHEROKEE &&
       init_status_ == INIT_STATUS_SUCCESS) {
#ifdef QTI_BT_FMD_SUPPORTED
      if (!IsFmdModeEnabled()) {
#endif
        if (!data_handler->sendCommandWait(SOC_PRE_SHUTDOWN_CMD, type)) {
          ALOGE("%s: Failed to receive response for PRE SHUTDOWN CMD", __func__);
          ALOGE("%s: SSR is in progress returning from here", __func__);
          return false;
        }
#ifdef QTI_BT_FMD_SUPPORTED
     }
#endif
    }

    if (init_status_ == INIT_STATUS_SUCCESS) {
      /* checking SocAlwaysOn property to decide whether to close
       * transport completely or partialy
       */
      if (!IsSocAlwaysOnEnabled()
#ifdef IS_PERI_ENABLED
      || isBatteryLevelCritical()
#endif
      ) {
        soc_need_reload_patch = true;
      } else {
        soc_need_reload_patch = false;
      }
#ifdef IS_PERI_ENABLED
      if (data_handler && data_handler->soc_type_ == BT_SOC_GANGES &&
          soc_need_reload_patch) {
#ifdef QTI_BT_FMD_SUPPORTED
        if (!IsFmdModeEnabled()) {
#endif
          if (!data_handler->sendCommandWait(HCI_ACTIVATE_SS_CMD, type)) {
            ALOGE("%s:Failed to receive response for Activate BT power off cmd", __func__);
            ALOGE("%s SSR is in progress returning from here", __func__);
            return false;
          }
          if (!data_handler->sendCommandWait(HCI_SET_BAUDRATE_CMD, type)) {
            ALOGE("%s:Failed to recevie rsp for baudrate set", __func__);
            ALOGE("%s SSR is in progress returning from here", __func__);
            return false;
          } else {
            soc_baudrate_reset_to_default = true;
          }
#ifdef QTI_BT_FMD_SUPPORTED
        }
#endif
      }
#endif

      logger_->PrepareDumpProcess();
      /*collect ringbuffer dumps if its respective property is set to true*/
      logger_->CollectDumps(false, false);
      /* Cleanup returns false when dump collected and other thread
       * is in process of doing post dump procedure.
       */
      if (!controller_->Cleanup()) {
        ALOGW("Skip controller cleanup as other thread is in process of cleanup: %s", __func__);
        return false;
      }
      ALOGW("controller Cleanup done");
      if (!data_handler->CheckSignalCaughtStatus()) {
        delete controller_;
        controller_ = nullptr;
      }

      if (soc_need_reload_patch) {
        offload_host_config_sent = false;
      }
    }
    guard.lock();

    if (init_status_ > INIT_STATUS_IDLE ) {
      if ( INIT_STATUS_INITIALIZING == init_status_) {
#ifdef IS_PERI_ENABLED
        if (soc_type_ != BT_SOC_GANGES || data_handler->CheckSignalCaughtStatus() ||
	        secureEvent || data_handler->GetInitTimerState() == TIMER_OVERFLOW) {
          // Kill init thread.
          KillInitThread();
        } else {
          ALOGI("%s: waiting here for fw download to complete as close called before"
            "startup timer expiry", __func__);
          int ret = controller_->WaitforFwDownloadCmpl();
          if (ret == 0) {
            guard.unlock();
            bool status;
            status = data_handler->DeactivatePeri();
            if (status == false) {
              guard.lock();
              ret = -1;
              ALOGD("%s: Unable deactive peri", __func__);
            } else {
              status = data_handler->ChangeSoCbd();
              if (status == false) {
                guard.lock();
                ret = -1;
                ALOGD("%s: Unable deactive peri", __func__);
              }
            }

            KillInitThread();
            if (ret == -1 && notifysignal_) {
              notifysignal_->NotifyDriver(SSR_ON_BT);
            }
          }
        }
#else
        // Kill init thread.
        KillInitThread();
#endif
        // Collect dumps only if there is no forced reboot
        if (data_handler && !data_handler->CheckSignalCaughtStatus() &&
            !secureEvent &&
            data_handler->GetInitTimerState() == TIMER_OVERFLOW) {
          logger_->PrepareDumpProcess();
          logger_->SetPrimaryCrashReason(BT_HOST_REASON_INIT_FAILED);

          /* Unlocking internal_mutex so that root inflammation event can be sent */
          guard.unlock();
          SendBqrRiePacket();
          guard.lock();
          // Add Delay list info. in state file object.
          logger_->CheckDelayListAndSetCrashReason();
          // Save dumps.
          logger_->CollectDumps(true, true);
        }

        /* complete transport cleanup as we are aborting/killing HIDL daemon */
        soc_need_reload_patch = true;

        // close the transport
        controller_->Disconnect();

#ifdef WAKE_LOCK_ENABLED
        Wakelock :: CleanUp();
#endif

#ifdef USER_DEBUG
        if (data_handler && data_handler->GetInitTimerState() != TIMER_OVERFLOW) {
          /* close called before startup time (2.9 sec) is finished  */
          ALOGE("%s: Killing daemon to recover as close called before startup timer expiry",
                __func__);
          kill(getpid(), SIGKILL);
        } else if (data_handler && data_handler->CheckSignalCaughtStatus()) {
          // Delete current dumped logs, as issue triggered during reboot.
          logger_->DeleteCurrentDumpedFiles();
          /* user triggerred reboot, no need to call abort */
          ALOGE("%s: Killing daemon as user triggered forced reboot", __func__);
          kill(getpid(), SIGKILL);
        } else {
          ALOGE("%s: Aborting daemon to recover as init is stuck", __func__);
          abort();
        }
#else
        ALOGE("%s: Killing daemon to recover as init is stuck", __func__);
        kill(getpid(), SIGKILL);
#endif
      }

      if (init_thread_.joinable()) {
        ALOGD("%s: joining init thread", __func__);
        init_thread_.join();
        ALOGI("DataHandler:: joined Init thread \n");
      }

      init_status_ = INIT_STATUS_IDLE;
    }
    BtState::Get()->Cleanup();
#ifdef WAKE_LOCK_ENABLED
    Wakelock :: CleanUp();
#endif

    status = true;
  }

#ifdef XPAN_SUPPORTED
  if (is_xpan_supported)
    QHci::Get()->DeInit();
#endif

#ifdef GOOGLE_OFFLOAD_ENABLED
  auto btSocketLocalInterface = BluetoothSocketLocalIntf::getInterface();
  if (btSocketLocalInterface) {
    btSocketLocalInterface->deInitialize();
  }else{
    ALOGE("%s btSocketLocalInterface is null", __func__);
  }
#endif

#ifdef GATT_OFFLOAD_ENABLED
  auto btGattLocalInterface = BluetoothGattLocalIntf::getInterface();
  if (btGattLocalInterface) {
    btGattLocalInterface->deInitialize();
  }else{
    ALOGE("%s btGattLocalInterface is null", __func__);
  }
#endif

  it = protocol_info_.find(type);
  if (it != protocol_info_.end()) {
    ProtocolCallbacksType *cb_data = reinterpret_cast<ProtocolCallbacksType *> (it->second);
    delete (cb_data);
    protocol_info_.erase(it);
  }

  diag_interface_.SignalEndofCleanup(type);
  return status;
}

#ifdef IS_PERI_ENABLED
bool DataHandler::isBatteryLevelCritical(){
  std::string battery_level_buf;
  int battery_level=0;
  android::base::ReadFileToString("/sys/class/power_supply/battery/capacity",&battery_level_buf);
  battery_level=atoi(battery_level_buf.c_str());
  return battery_level <= CRITICAL_BATTERY_PERCENTAGE;
}
#endif

#ifdef XPAN_SUPPORTED
void DataHandler::UpdateRingBufferFromQHci(HciPacketType packet_type,
                                            const uint8_t * data, int length) {
  ALOGD("%s: length %d ", __func__, length);
  logger_->ProcessTx(packet_type, data, length);
}
#endif


void DataHandler::UpdateRingBuffer(HciPacketType packet_type, const uint8_t *data, int length)
{
#ifdef DUMP_RINGBUF_LOG
  gettimeofday(&time_hci_cmd_arrived_, NULL);
  snprintf(last_hci_cmd_timestamp_.opcode, OPCODE_LEN, "0x%02x%02x", data[0], data[1]);
  AddHciCommandTag(last_hci_cmd_timestamp_.hcicmd_timestamp,
                   time_hci_cmd_arrived_, last_hci_cmd_timestamp_.opcode);
  logger_->SaveLastStackHciCommand(last_hci_cmd_timestamp_.hcicmd_timestamp);
#endif
  logger_->ProcessTx(packet_type, data, length);
}

#ifdef IS_PERI_ENABLED
bool DataHandler::DeactivatePeri(void)
{
  bool event_wait_temp;
  uint16_t awaited_evt_temp;

  std::unique_lock<std::mutex> guard(evt_wait_mutex_);
  awaited_evt_temp = 0;
  event_wait_temp = event_wait = false;
 
  HciPacketType packet_type = HCI_PACKET_TYPE_PERI_CMD;
  const uint8_t data[] = {0x00, 0xf1, 0xff, 0x03, 0x00, 0x01, 0x00};
  int length = 7;

  ALOGI("%s: Sending Activate cmd for power_off for ssId : %d, action:%d ",
         __func__, BT_SS, HCI_ACTION_POWER_OFF);
  awaited_evt = HCI_VS_GENERAL_OPCODE_PERI;
  UpdateRingBuffer(packet_type, data, length);
  if (!controller_->SendPacket(packet_type, data, length)) {
    ALOGE("Unable to send Activate cmd \n");
    awaited_evt = 0;
  }

  if (awaited_evt) {
    event_wait_cv.wait_for(guard, std::chrono::milliseconds(HCI_CMD_TIMEOUT), []
                           {return event_wait;});
    if (event_wait)
      return true;
  }
  return false;
}

bool DataHandler::ChangeSoCbd(void)
{
  bool event_wait_temp;
  uint16_t awaited_evt_temp;

  std::unique_lock<std::mutex> guard(evt_wait_mutex_);
  awaited_evt_temp = 0;
  event_wait_temp = event_wait = false;
  ALOGI("%s: HCI_SET_BAUDRATE_CMD during force close",  __func__);
  awaited_evt = HCI_VS_GENERAL_OPCODE_PERI;
  subOpcode = HCI_PERI_SET_BAUDRATE;
  if (controller_->ResetBaudrate() < 0) {
    ALOGW("%s: failed to reset baudrate to 115200bps", __func__);
    awaited_evt = false;
    subOpcode = 0;
  }

  if (awaited_evt) {
    event_wait_cv.wait_for(guard, std::chrono::milliseconds(HCI_CMD_TIMEOUT), []
                           {return event_wait;});
    if (event_wait)
      return true;
  }
  return false;
}
#endif

#ifdef QTI_BT_FMD_SUPPORTED
void DataHandler::logFmdCmd(const uint8_t *data, int length) {

  std::stringstream data_buff;
  std::string print_cmd = "FMD-CMD : ";

  for (int i=0; i<length; i++) {
    data_buff <<  std::uppercase << std::hex << (int)data[i] << " ";
  }
  print_cmd += data_buff.str();

  ALOGI("%s", print_cmd.c_str());
}
#endif

bool DataHandler::sendCommandWait(HciCommand cmd, ProtocolType type)
{
  char dst_buff[MAX_BUFF_SIZE];
  struct timeval tv;
  bool event_wait_temp;
  uint16_t awaited_evt_temp;
  int SS_Ssr_flag;
  PrimaryReasonCode reason;

  /* Data handler is exposed to different layers. In future
   * might be this call can be used in those layers too.
   * To monitor command credit flow,  lock is acquired until
   * the response is received or timeout is occurred.
   */
  std::unique_lock<std::mutex> guard(evt_wait_mutex_);
  gettimeofday(&tv, NULL);
  BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Failed to send internal CMD");
  BtState::Get()->SetHciInternalCmdSent(dst_buff);

  awaited_evt_temp = 0;
  event_wait_temp = event_wait = false;
  reason = BT_HOST_REASON_DEFAULT_NONE;

  switch (cmd) {
    case HCI_RESET_CMD:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_COMMAND;
      const uint8_t data[] = {0x03, 0x0C, 0x00};
      int length = 3;

      ALOGI("Sending HCI Reset \n");
      awaited_evt = *(uint16_t *)(&data);
      UpdateRingBuffer(packet_type, data, length);
#ifdef PASSTHROUGH_ENABLED
      if(getIsPassthroughEnabled() == true){
        sendPacketOnHCIArbiterGlink(packet_type, data, length);
      }else{
        if (!controller_->SendPacket(packet_type, data, length)) {
          ALOGE("Unable to send HCI Reset \n");
          awaited_evt = 0;
        }
      }
#else
      if (!controller_->SendPacket(packet_type, data, length)) {
        ALOGE("Unable to send HCI Reset \n");
        awaited_evt = 0;
      }
#endif
      break;
    }
#ifdef IS_PERI_ENABLED
    case HCI_ACTIVATE_SS_CMD:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_PERI_CMD;
      const uint8_t data[] = {0x00, 0xf1, 0xff, 0x03, 0x00, 0x01, 0x00};
      int length = 7;

      ALOGI("%s: Sending Activate cmd for power_off for ssId : %d, action:%d ",
            __func__, BT_SS, HCI_ACTION_POWER_OFF);
      awaited_evt = HCI_VS_GENERAL_OPCODE_PERI;
      UpdateRingBuffer(packet_type, data, length);
      if (!controller_->SendPacket(packet_type, data, length)) {
        ALOGE("Unable to send Activate cmd \n");
        awaited_evt = 0;
      }
      break;
    }
    case HCI_SET_BAUDRATE_CMD:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_PERI_CMD;
      const uint8_t data[] = {0x00, 0xf1, 0xff, 0x02, 0x02, 0x00};
      int length = 6;

      ALOGI("%s: HCI_SET_BAUDRATE_CMD",  __func__);
      awaited_evt = HCI_VS_GENERAL_OPCODE_PERI;
      UpdateRingBuffer(packet_type, data, length);
      subOpcode = HCI_PERI_SET_BAUDRATE;
      if (controller_->ResetBaudrate() < 0) {
        ALOGW("%s: failed to reset baudrate to 115200bps", __func__);
            awaited_evt = false;
        subOpcode = 0;
      }
      break;
    }
#ifdef PASSTHROUGH_ENABLED
    case HCI_VSC_PERI_BT_SPI_TXP_SWITCH:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_PERI_CMD;
      std::vector<uint8_t> cmd_data = build_switch_spi_to_ipc_vsc();
      ALOGI("%s: HCI_VSC_PERI_BT_SPI_TXP_SWITCH with sleep bytes",  __func__);
      awaited_evt = HCI_VS_GENERAL_OPCODE_PERI;
      UpdateRingBuffer(packet_type, cmd_data.data() + 1, cmd_data.size() - 1);
      subOpcode = HCI_PERI_BT_SPI_TXP_SWITCH_SUB_OPCODE;
      if (!(static_cast<SpiController*>(controller_))->SendPacketWithOptions(packet_type,
        cmd_data.data(), cmd_data.size(), SPI_ENABLE_SLEEP_BIT)) {
        ALOGE("Unable to send BT_SPI_TXP_SWITCH cmd \n");
        awaited_evt = 0;
      }
      break;
    }

    case HCI_VSC_PERI_BT_UART_TXP_SWITCH:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_PERI_CMD;
      std::vector<uint8_t> cmd_data = build_switch_uart_to_spi_vsc();
      ALOGI("%s: HCI_VSC_PERI_BT_TXP_SWITCH",  __func__);
      awaited_evt = HCI_VS_GENERAL_OPCODE_PERI;
      UpdateRingBuffer(packet_type, cmd_data.data(), cmd_data.size());
      subOpcode = HCI_PERI_BT_SPI_TXP_SWITCH_SUB_OPCODE;
      if (!(static_cast<SpiController*>(controller_))->SendPacketWithOptions(packet_type,
        cmd_data.data(), cmd_data.size(), SPI_DISABLE_SOFT_RESET_BIT)) {
        ALOGE("Unable to send BT_UART_TXP_SWITCH cmd \n");
        awaited_evt = 0;
      }
      break;
    }
#endif
#endif
    case SOC_PRE_SHUTDOWN_CMD:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_COMMAND;
      const uint8_t data[] = {0x08, 0xFC, 0x00};
      int length = 3;

      ALOGI("Sending Pre-shutdown command \n");
      awaited_evt = *(uint16_t *)(&data);
      UpdateRingBuffer(packet_type, data, length);
#ifdef PASSTHROUGH_ENABLED
      if(getIsPassthroughEnabled() == true){
        sendPacketOnHCIArbiterGlink(packet_type, data, length);
      }else{
        if (!controller_->SendPacket(packet_type, data, length)) {
          ALOGE("Unable to send Pre-shutdown command \n");
          awaited_evt = 0;
        }
      }
#else
      if (!controller_->SendPacket(packet_type, data, length)) {
        ALOGE("Unable to send Pre-shutdown command \n");
        awaited_evt = 0;
      }
#endif
      break;
    }
    case HCI_WRITE_BD_ADDRESS:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_COMMAND;
      uint8_t data[HCI_WRITE_BD_ADDRESS_LENGTH] = {0x14, 0xFC, 0x06};

      BluetoothAddress::GetLocalAddress(&data[HCI_WRITE_BD_ADDRESS_OFFSET]);
      BluetoothAddress::le2bd(&data[HCI_WRITE_BD_ADDRESS_OFFSET]);
      ALOGI("Sending HCI_WRITE_BD_ADDRESS command \n");
      awaited_evt = *(uint16_t *)(&data);
      UpdateRingBuffer(packet_type, data, HCI_WRITE_BD_ADDRESS_LENGTH);

      if (!controller_->SendPacket(packet_type, data, HCI_WRITE_BD_ADDRESS_LENGTH)) {
        ALOGE("Unable to send HCI_WRITE_BD_ADDRESS \n");
        awaited_evt = 0;
      }
      break;
    }
    case HCI_SET_OFFLOAD_HOST_CONFIG_CMD:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_COMMAND;
      uint8_t data[HCI_MAX_CMD_SIZE] = {0};
      size_t len = 0;
      bool ret = MakeHciCmdForSetOffloadHostConfig(data, &len);
      if (!ret)
        return false;

      ALOGI("Sending HCI_SET_OFFLOAD_HOST_CONFIG_CMD command");
      awaited_evt = *(uint16_t *)(&data);
      UpdateRingBuffer(packet_type, data, len);
      if (!controller_->SendPacket(packet_type, data, len)) {
        ALOGE("Unable to send HCI_SET_OFFLOAD_HOST_CONFIG_CMD");
        awaited_evt = 0;
      }
      break;
    }
#ifdef QTI_BT_FMD_SUPPORTED
    case HCI_SET_FMD_CONFIG_PARAMS_CMD:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_COMMAND;
      //13D8
      uint8_t data[] = {0x37, 0xFC, 0x06, 0x10, 0xD8, 0x13, 0xFD, 0x40,
                              0x06};
      int length = 9;

      data[4] = (fmd_info.length & 0x00FF);
      data[5] = (fmd_info.length & 0xFF00) >> 8;
      //no of keys
      data[6] = fmd_info.no_of_keys;
      ALOGI("HCI_SET_FMD_CONFIG_PARAMS_CMD to Soc NumKeys %d, total Keylength %d\n",
              data[6], fmd_info.length);
      awaited_evt = *(uint16_t *)(&data);
      UpdateRingBuffer(packet_type, data, length);
#ifdef PASSTHROUGH_ENABLED
      sendPacketOnHCIArbiterGlink(packet_type, data, length);
#else
      if (!controller_->SendPacket(packet_type, data, length)) {
        ALOGE("Unable to send HCI_SET_FMD_CONFIG_PARAMS_CMD \n");
        awaited_evt = 0;
      }
#endif
      break;
    }
    case HCI_SET_FMD_CONFIG_PARAMS_V2_CMD:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_COMMAND;
      uint8_t data[100] = {0};
      data[0] = 0x37;
      data[1] = 0xFC;
      data[2] = 0x0E + fmd_config.header_length; //Length
      data[3] = 0x10; //Sub-opcode
      data[4] = (fmd_info.length & 0x00FF);
      data[5] = (fmd_info.length & 0xFF00) >> 8;

      data[6] = fmd_info.no_of_keys;//No of Keys
      data[7] = fmd_config.adv_interval[0];
      data[8] = fmd_config.adv_interval[1]; //Adv interval
      data[9] = fmd_config.advData_Timeout[0];
      data[10] = fmd_config.advData_Timeout[1];
      data[11] = fmd_config.advData_Timeout[2];
      data[12] = fmd_config.advData_Timeout[3]; //Key rotation time
      data[13] = fmd_config.tx_pwr; //Tx power
      data[14] = fmd_config.per_pkt_key_length; //Each Key data Size
      data[15] = fmd_config.restart_keys; // Restart Keys
      data[16] = fmd_config.header_length; //Header Info length

      memcpy(&data[17], &fmd_config.config_header[0],
              fmd_config.header_length);
      uint8_t length = data[2] + 3;
      ALOGI("Num of fmd keys %d, total Keylength %d\n",
              data[6], fmd_info.length);

      awaited_evt = *(uint16_t *)(&data);
      UpdateRingBuffer(packet_type, data, length);
      if (log_fmd_soc_cmd)
        logFmdCmd(data, length);
#ifdef PASSTHROUGH_ENABLED
      sendPacketOnHCIArbiterGlink(packet_type, data, length);
#else
      if (!controller_->SendPacket(packet_type, data, length)) {
        ALOGE("Unable to send HCI_WRITE_FMD_ADV_DATA_CMD \n");
        awaited_evt = 0;
      }
#endif
      break;
    }
    case HCI_WRITE_FMD_ADV_DATA_CMD:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_COMMAND;
      uint8_t data[255] = {0};
      uint8_t header[HCI_VSC_FMD_ADV_DATA_HEADER_SIZE] =
                           {0x37, 0xFC, 0xFB, 0x11, 0xF9};
      int length = HCI_VSC_FMD_ADV_DATA_PKT_LEN +
                    HCI_VSC_FMD_ADV_DATA_HEADER_SIZE;

      memcpy(&data[0], header, HCI_VSC_FMD_ADV_DATA_HEADER_SIZE);

      if (fmd_key_inx + HCI_VSC_FMD_ADV_DATA_PKT_LEN <
            fmd_info.length) {
        memcpy(&data[5], &fmd_key_data[fmd_key_inx],
          HCI_VSC_FMD_ADV_DATA_PKT_LEN);
        fmd_key_inx = fmd_key_inx + HCI_VSC_FMD_ADV_DATA_PKT_LEN;
      } else {
        data[2] = fmd_info.length - fmd_key_inx + 2;
        data[4] = fmd_info.length - fmd_key_inx;
        length = fmd_info.length - fmd_key_inx + 5;
        memcpy(&data[5], &fmd_key_data[fmd_key_inx],
          fmd_info.length - fmd_key_inx);
        fmd_key_inx = fmd_info.length;
      }

      awaited_evt = *(uint16_t *)(&data);
      UpdateRingBuffer(packet_type, data, length);
      if (log_fmd_soc_cmd)
        logFmdCmd(data, length);
#ifdef PASSTHROUGH_ENABLED
      sendPacketOnHCIArbiterGlink(packet_type, data, length);
#else
      if (!controller_->SendPacket(packet_type, data, length)) {
        ALOGE("Unable to send HCI_WRITE_FMD_ADV_DATA_CMD \n");
        awaited_evt = 0;
      }
#endif
      break;
    }
    case HCI_SET_FMD_MODE_CMD:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_COMMAND;
      const uint8_t data[] = {0x37, 0xFC, 0x02, 0x12, 0x1};
      int length = 5;

      awaited_evt = *(uint16_t *)(&data);
      UpdateRingBuffer(packet_type, data, length);
      if (log_fmd_soc_cmd)
        logFmdCmd(data, length);
#ifdef PASSTHROUGH_ENABLED
      sendPacketOnHCIArbiterGlink(packet_type, data, length);
#else
      if (!controller_->SendPacket(packet_type, data, length)) {
        ALOGE("Unable to send HCI_SET_FMD_MODE_CMD \n");
        awaited_evt = 0;
      }
#endif
      fmd_info.fmd_enable_sent = true;
      break;
    }
#endif
    case HCI_READ_CAL_NVM_DATA:
    {
      HciPacketType packet_type = HCI_PACKET_TYPE_COMMAND;
      uint8_t data[] = {0x3B, 0xFC, 0x02, 0x09, 0x00};
      int length = 5;

      updateCalFarmeType();

      //data[4] = 0x00  Data will be sent as HCI packet format
      //data[4] = 0x01  Data will be sent as ACL packet format
      if (getFarmeType()) {
        data[4] = 0x01;
        ALOGI("Sending HCI_READ_CAL_NVM_DATA cmd with packet format as ACL\n");
      } else {
        ALOGI("Sending HCI_READ_CAL_NVM_DATA cmd with packet format as HCI\n");
      }

      awaited_evt = *(uint16_t *)(&data);
      UpdateRingBuffer(packet_type, data, length);
#ifdef PASSTHROUGH_ENABLED
      sendPacketOnHCIArbiterGlink(packet_type, data, length);
#else
      if (!controller_->SendPacket(packet_type, data, length)) {
        ALOGE("Unable to Send HCI_READ_CAL_NVM_DATA command \n");
        awaited_evt = 0;
      }
#endif
      break;
    }
    default:
      ALOGW("%s: fallback to default case as command not handled", __func__);
      return false;
  }

  if (awaited_evt) {
    SetAppropriateLog(cmd, event_wait_temp, type);
    event_wait_cv.wait_for(guard, std::chrono::milliseconds(HCI_CMD_TIMEOUT), []
                           {return event_wait;});
    /* event_wait stands false if no response received
     * for the command sent.
    */
    event_wait_temp = event_wait;
    awaited_evt_temp = awaited_evt;
    awaited_evt = 0;
    subOpcode = 0;
    /* Set appropriate logs based on even wait flag */
    SetAppropriateLog(cmd, event_wait_temp, type);
  } else {
    /* This block of code executes when failed to send command.
     * This failure can be due to SSR is in progress. Check SSR
     * status before updating secondary crash reasons.
     */
#ifdef IS_PERI_ENABLED
    SS_Ssr_flag = NotifySignal::Get()->GetSubSystemSsrStatus();
    if ((SS_Ssr_flag == UWB_SSR_COMPLETED) || (SS_Ssr_flag == SSR_ON_UWB)) {
      ALOGE("%s: failed to send internal command as UWB Sub-system got the SSR", __func__);
      return false;
    } else if (diag_interface_.isSsrTriggered()) {
#else
    if (diag_interface_.isSsrTriggered()) {
#endif
      ALOGE("%s: failed to send internal command as SSR in progress", __func__);
      return false;
    } else  {
      logger_->SetSecondaryCrashReason(BT_HOST_REASON_FAILED_TO_SEND_CMD);
      reason = BT_HOST_REASON_FAILED_TO_SEND_INTERNAL_CMD;
    }
  }

  if (!event_wait_temp) {
     diag_interface_.SetInternalCmdTimeout();
     diag_interface_.ResetCleanupflag();
     if (reason == BT_HOST_REASON_DEFAULT_NONE) {
       reason = BT_HOST_REASON_SSR_INTERNAL_CMD_TIMEDOUT;
       ALOGE("%s: failed to receive rsp for:%02x %02x", __func__,
             (awaited_evt_temp & 0xFF), (((awaited_evt_temp >> 0x08) & 0xFF)));
     }

   /* Below code update the reason as Rx thread Stuck
    * when SSR is already triggered but not moved further due to cleanup in progress
    */
  if ((controller_->GetPreviousReason() == BT_HOST_REASON_RX_THREAD_STUCK) ||
      (controller_->GetPreviousReason() == BT_HOST_REASON_SSR_UNABLE_TO_WAKEUP_SOC)) {
        ALOGE("%s: overwriting the reason = %02x with prv_reason = %02x",
               __func__, reason, controller_->GetPreviousReason());
        reason = controller_->GetPreviousReason();
     }

     if(HCI_VSC_PERI_BT_UART_TXP_SWITCH != cmd) {
         controller_->SsrCleanup(reason);
         controller_->WaitforCrashdumpFinish();
     } else
         ALOGE("%s: Failed to receive rsp for TXP switch cmd=0x%04x, reason=0x%02x", __func__, cmd, reason);
  }

  return event_wait_temp;
}

void DataHandler::SetAppropriateLog(HciCommand cmd, bool event_wait_temp,
                                    ProtocolType type)
{
  char dst_buff[MAX_BUFF_SIZE];
  struct timeval tv;

  gettimeofday(&tv, NULL);
  switch(cmd) {
    case HCI_RESET_CMD:
    {
      bool cleanup_status = logger_->GetCleanupStatus(type);

      if (awaited_evt && !cleanup_status) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent first HCI RESET CMD");
        logger_->SetSecondaryCrashReason(BT_HOST_REASON_HCI_RESET_CC_NOT_RCVD);
      } else if (awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent second HCI RESET CMD");
        logger_->SetSecondaryCrashReason(BT_HOST_REASON_HCI_RESET_CC_NOT_RCVD);
      } else if (event_wait_temp && !cleanup_status) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp rcvd for first HCI RESET CMD");
      } else if (event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp rcvd for second HCI RESET CMD");
      } else if (!event_wait_temp && !cleanup_status) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp not rcvd for first HCI RESET CMD");
      } else if (!event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp not rcvd for second HCI RESET CMD");
      }

      break;
    }
    case SOC_PRE_SHUTDOWN_CMD:
    {
      if (awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent HCI Pre shutdown CMD");
        logger_->SetSecondaryCrashReason(BT_HOST_REASON_HCI_PRE_SHUTDOWN_CC_NOT_RCVD);
      } else if (event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp rcvd for Pre shutdown CMD");
      } else {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp not rcvd for Pre shutdown CMD");
      }
      break;
    }
    case HCI_WRITE_BD_ADDRESS:
    {
      if (awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent HCI BD address CMD");
        logger_->SetSecondaryCrashReason(BT_HOST_REASON_HCI_SET_BD_ADDRESS_CC_NOT_RCVD);
      } else if(event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp rcvd for HCI BD address CMD");
      } else {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp not rcvd for HCI BD address CMD");
      }
      break;
    }
    case HCI_SET_OFFLOAD_HOST_CONFIG_CMD:
    {
      if (awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent HCI_SET_OFFLOAD_HOST_CONFIG_CMD");
        logger_->SetSecondaryCrashReason(BT_HOST_REASON_HCI_SET_OFFLOAD_HOST_CONFIG_CC_NOT_RCVD);
      } else if (event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv,
            (char *)"Rsp rcvd for HCI_SET_OFFLOAD_HOST_CONFIG_CMD");
        logger_->SetSecondaryCrashReason(BT_SOC_REASON_DEFAULT);
      } else {
        BtState::Get()->AddLogTag(dst_buff, tv,
            (char *)"Rsp not rcvd for HCI_SET_OFFLOAD_HOST_CONFIG_CMD");
      }
      break;
    }
#ifdef IS_PERI_ENABLED
    case HCI_ACTIVATE_SS_CMD:
    {
      if(awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent BT Activate SS CMD");
        logger_->SetSecondaryCrashReason(BT_HOST_REASON_HCI_ACTIVATE_CC_NOT_RCVD);
      } else if(event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Resp rcvd for BT Activate CMD");
      }
      break;
    }
    case HCI_SET_BAUDRATE_CMD:
    {
      if(awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent Set Baudrate cmd");
        logger_->SetSecondaryCrashReason(BT_HOST_REASON_PERI_SETBAUD_CC_NOT_RCVD);
      } else if(event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Resp rcvd for set baudrate cmd");
      }
      break;
    }
#ifdef PASSTHROUGH_ENABLED
    case HCI_VSC_PERI_BT_SPI_TXP_SWITCH:
    {
      if(awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent Spi TXP Switch to Ipc");
        logger_->SetSecondaryCrashReason(BT_HOST_HCI_PERI_BT_SPI_TXP_SWITCH_RSP_NOT_RCVD);
      } else if(event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Resp rcvd for TXP Switch to Ipc");
      }
      break;
    }
#endif
#ifdef QTI_BT_FMD_SUPPORTED
    case HCI_SET_FMD_CONFIG_PARAMS_CMD:
    {
      if (awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent FMD config params CMD");
      } else if(event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp rcvd for FMD config params CMD");
      } else {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp not rcvd for FMD config params CMD");
      }
      break;
    }
    case HCI_SET_FMD_CONFIG_PARAMS_V2_CMD:
    {
      if (awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent Set FMD config params V2 CMD");
      } else if(event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp rcvd for Set FMD config params V2 CMD");
      } else {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp not rcvd for Set FMD config params V2 CMD");
      }
      break;
    }
    case HCI_WRITE_FMD_ADV_DATA_CMD:
    {
      if (awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent Write FMD adv data CMD");
      } else if(event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp rcvd for Write FMD adv data CMD");
      } else {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp not rcvd for Write FMD adv data CMD");
      }
      break;
    }
    case HCI_SET_FMD_MODE_CMD:
    {
      if (awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent Set FMD mode CMD");
      } else if(event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp rcvd for Set FMD mode CMD");
      } else {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Rsp not rcvd for Set FMD mode CMD");
      }
      break;
    }
#endif
#endif
    case HCI_READ_CAL_NVM_DATA:
    {
      if(awaited_evt) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Sent Calibration read cmd");
        logger_->SetSecondaryCrashReason(BT_HOST_REASON_BT_CAL_CMD_CC_NOT_RCVD);
      } else if(event_wait_temp) {
        BtState::Get()->AddLogTag(dst_buff, tv, (char *)"Resp rcvd for set Calibration read cmd");
      }
      break;
    }
    default:
      ALOGW("%s: fallback to default case as command not handled", __func__);
      return;
  }

  if (awaited_evt)
    BtState::Get()->SetHciInternalCmdSent(dst_buff);
  else
    BtState::Get()->SetHciInternalCmdRsp(dst_buff);
}

inline bool DataHandler::isAwaitedEvent(const uint8_t *buff, uint16_t len)
{
  if (len < MIN_OPCODE_LEN)
    return false;

  if (awaited_evt && ((*(uint16_t *)(&buff[3])) == awaited_evt))
    return true;
  else
    return false;
}

#ifdef IS_PERI_ENABLED
inline bool DataHandler::isAwaitedPeriNtf(const uint8_t *buff, uint16_t len)
{
  if ((len == MIN_PERI_NTF_OPCODE_LEN) &&
     ((static_cast<uint16_t>(buff[4])) == HCI_PERI_EVT_SS_ACTIVATE_COMPLETE))
    return true;

  return false;
}

inline bool DataHandler::isAwaitedPeriEvent(const uint8_t *buff, uint16_t len)
{
   if (awaited_evt && len == MIN_PERI_EVT_OPCODE_LEN &&
       (awaited_evt == (static_cast<uint16_t>(buff[8])<<8|(buff[7]))))
     return true;

   return false;
}

inline bool DataHandler::isAwaitedPeriBDEvt(const uint8_t *buff, uint16_t len)
{
   if (awaited_evt && len == MIN_PERI_BD_EVT_OPCODE_LEN &&
       (awaited_evt == (static_cast<uint16_t>(buff[7])<<8|(buff[6]))) && subOpcode == buff[9])
     return true;

   return false;
}

#ifdef PASSTHROUGH_ENABLED
inline bool DataHandler::isAwaiterPeriSPITxpSwitchEvt(const uint8_t *buff, uint16_t len)
{
  ALOGD("%s: awaited_evt :: %02x", __func__,awaited_evt);
  ALOGD("%s: subOpcode :: %02x", __func__,subOpcode);
  ALOGD("%s: len :: %d %02x %02x %02x %02x %02x %02x %02x %02x %02x",__func__, len, buff[0],buff[1],buff[2],buff[3],buff[4],buff[5],
    buff[6],buff[7],buff[8]);
  if (awaited_evt && len == HCI_PERI_BT_SPI_TXP_SWITCH_LENGTH &&
    (awaited_evt == (static_cast<uint16_t>(buff[6])<<8|(buff[5]))) && subOpcode == buff[8])
  return true;

  return false;
}
#endif

#endif

void DataHandler::SetSignalCaught()
{
  caught_signal = true;
}

bool DataHandler::CheckSignalCaughtStatus()
{
  return caught_signal;
}

SocAddOnFeatures_t* DataHandler:: GetSoCAddOnFeatures()
{
  if (isProtocolInitialized(TYPE_BT)) {
    return controller_->GetAddOnFeatureList();
  } else {
    return NULL;
  }
}

HostAddOnFeatures_t* DataHandler:: GetHostAddOnFeatures()
{
  if (isProtocolInitialized(TYPE_BT) && host_add_on_features.feat_mask_len > 0) {
    return &host_add_on_features;
  } else {
    return NULL;
  }
}

uint64_t DataHandler :: GetChipVersion()
{
  if (isProtocolInitialized(TYPE_BT)) {
    return controller_->GetChipVersion();
  } else {
    return INVALID_CHIP_VERSION;
  }
}

void DataHandler::StartSocCrashWaitTimer()
{
  if (controller_)
    controller_->StartSocCrashWaitTimer();
}

void DataHandler::SendBqrRiePacket()
{
  if (soc_type_ != BT_SOC_SMD && controller_) {
    if (data_handler->getTransportType() == BT_TRANSPORT_TYPE_SPI)
      ((SpiController *)controller_)->SendBqrRiePacket();
    else
      ((UartController *)controller_)->SendBqrRiePacket();
  }
}

void inline DataHandler::SetInitTimerState(TimerState state)
{
  init_timer_.timer_state = state;
}

inline TimerState DataHandler::GetInitTimerState()
{
  return init_timer_.timer_state;
}

void DataHandler::InitTimeOut(union sigval sig)
{
  ALOGE("%s: SoC Initialization stuck detected", __func__);
  std::unique_lock<std::mutex> guard(init_mutex_);
  {
    std::unique_lock<std::mutex> lock(DataHandler::init_timer_mutex_);

    if (!data_handler) {
      ALOGE("%s data handler instance has been destroyed", __func__);
      return;
    }

    /* Discard the timeout callback execution if init_status and
    * timerstate status updated to success.
    */
    if (data_handler->init_status_ != INIT_STATUS_INITIALIZING ||
        data_handler->GetInitTimerState() != TIMER_ACTIVE) {
      ALOGW("Discarding %s:() as Initialization is successful", __func__);
      return;
    }

    data_handler->SetInitTimerState(TIMER_OVERFLOW);
  }
  // Start SSR.
  if (data_handler && data_handler->controller_ != nullptr) {
    data_handler->controller_->SsrCleanup(BT_HOST_REASON_INIT_FAILED);
  }
  if (data_handler && data_handler->isProtocolAdded(TYPE_BT))
    data_handler->Close(TYPE_BT);

  if (data_handler && data_handler->isProtocolAdded(TYPE_FM))
    data_handler->Close(TYPE_FM);

  if (data_handler && data_handler->isProtocolAdded(TYPE_ANT))
    data_handler->Close(TYPE_ANT);
}

void DataHandler::StopInitTimer()
{
  struct itimerspec ts;
  TimerState init_timer_state;

  std::unique_lock<std::mutex> lock(DataHandler::init_timer_mutex_);
  init_timer_state = GetInitTimerState();

  if (init_timer_state == TIMER_NOT_CREATED) {
      ALOGD("%s: InitTimer already stopped", __func__);
      return;
  } else if (init_timer_state != TIMER_NOT_CREATED &&
             init_timer_state!= TIMER_OVERFLOW) {
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    if (timer_settime(init_timer_.timer_id, 0, &ts, 0) == -1) {
      ALOGE("%s:Failed to stop Init thread timer", __func__);
    }
    timer_delete(init_timer_.timer_id);
    SetInitTimerState(TIMER_NOT_CREATED);
    ALOGD("%s: InitTimer Stopped", __func__);
    return;
  } else if(init_timer_state == TIMER_OVERFLOW) {
    ALOGW("%s: Failed to stop Init timer this could be due to TIMEOUT", __func__);
  }
}

void DataHandler::StartInitTimer()
{
  struct itimerspec ts;
  struct sigevent se;
  uint32_t timeout;
  int prop_val = 0;

  ALOGV("%s", __func__);
  if (init_timer_.timer_state == TIMER_NOT_CREATED) {
    se.sigev_notify_function = (void (*)(union sigval))InitTimeOut;
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = &init_timer_.timer_id;
    se.sigev_notify_attributes = NULL;

    if (!timer_create(CLOCK_MONOTONIC, &se, &init_timer_.timer_id))
      SetInitTimerState(TIMER_CREATED);
    else
      ALOGE("%s: Failed to create InitTimer", __func__);
  }

  if ((GetInitTimerState() == TIMER_CREATED) ||
      (GetInitTimerState() == TIMER_OVERFLOW)) {
    prop_val = IsXMEMEnabled();
    if (prop_val == 1) {
      timeout = HIDL_INIT_TIMEOUT_DEFAULT_XMEM;
    } else if(prop_val == 2) {
      timeout = HIDL_INIT_TIMEOUT_XMEM;
    } else {
      timeout = HIDL_INIT_TIMEOUT;
    }
    ts.it_value.tv_sec = (timeout / 1000);
    ts.it_value.tv_nsec = 1000000 * (timeout % 1000);
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    if ((timer_settime(init_timer_.timer_id, 0, &ts, 0)) == -1) {
      ALOGE("%s: Failed to start Init timer", __func__);
      return;
    } else {
      SetInitTimerState(TIMER_ACTIVE);
      ALOGD("%s: Init timer started", __func__);
    }
  }
}

#ifdef SECURE_PERIPHERAL_ENABLED
void DataHandler::InitiateShutdown(union sigval sig)
{
  ALOGE("%s: Secure mode shutdown timer expired", __func__);
  if (periContext) {
    if (deregisterPeripheralCB(periContext) == PRPHRL_SUCCESS)
      ALOGI("%s: Deregistered BT peripheral", __func__);
    else
      ALOGE("%s: Failure in Deregistering BT peripheral", __func__);
    periContext = nullptr;
  }
  kill(getpid(), SIGKILL);
}

void DataHandler::StartShutdownTimer()
{
  struct itimerspec ts;
  struct sigevent se;

  ALOGI("%s", __func__);
  if (shutdown_timer_id == 0) {
    se.sigev_notify_function = (void (*)(union sigval))InitiateShutdown;
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = &shutdown_timer_id;
    se.sigev_notify_attributes = NULL;

    if (timer_create(CLOCK_MONOTONIC, &se, &shutdown_timer_id)) {
      ALOGE("%s: Failed to create Shutdown Timer", __func__);
      return;
    }

    ts.it_value.tv_sec = (SECURE_SHUTDOWN_TIMER / 1000);
    ts.it_value.tv_nsec = 1000000 * (SECURE_SHUTDOWN_TIMER % 1000);
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    if ((timer_settime(shutdown_timer_id, 0, &ts, 0)) == -1) {
      ALOGE("%s: Failed to start shutdown timer", __func__);
      return;
    } else
      ALOGD("%s: Shutdown timer started", __func__);
  } else
      ALOGE("%s: ShutdownTimer already created", __func__);
}
#endif

int DataHandler::IsXMEMEnabled() {
  char value[PROPERTY_VALUE_MAX] = {'\0'};
  int prop_val = 0;

  if(!is_xmem_read_) {
    logger_->PropertyGet("persist.vendor.bluetooth.enable_XMEM", value, "0");
    prop_val = atoi(value);
    ALOGD("%s : XMEM property value = %d", __func__, prop_val);
    if (prop_val >=0 && prop_val <= 2) {
      xmem_prop_val_ = prop_val;
    } else {
      ALOGE("%s: Invalid value %d set for enable xmem property: \n"
            "\"persist.vendor.bluetooth.enable_XMEM\"\n"
            "set 1 for enabling default xmem patch download configuration OR\n"
            "set 2 for xmem patch with rsp for every tlv download cmd OR\n"
            "set 0 to disable xmem patch download", __func__, prop_val);
      return 0;
    }
    is_xmem_read_ = true;
  }
  return xmem_prop_val_;
}

int DataHandler::GetInitStatus() {
  return init_status_;
}

void DataHandler::StackTimeoutTriggered()
{
  ALOGW("%s: Stack was not properly closed", __func__);
  Logger::Get()->stack_timeout_triggered = true;
}

void DataHandler:: LogPwrSrcsUartFlowCtrl() {
  if (data_handler && data_handler->controller_ != nullptr) {
    UartController * instance = (UartController *)data_handler->controller_;
    instance->LogPwrSrcsUartFlowCtrl();
  }
}

bool DataHandler :: CheckForUartFailureStatus() {
  if (data_handler && data_handler->controller_ != nullptr) {
    UartController * instance = (UartController *)data_handler->controller_;
    return instance->CheckForUartFailureStatus();
  }
  return false;
}

#ifdef BT_CP_CONNECTED
bool DataHandler :: UpdateCopVersion(CoPVerSupported cop_ver_supported) {
 if (cop_ver_supported.len == 0) {
   ALOGE("%s: failed to get cop version from controller", __func__);
   return false;
 }
 std::shared_ptr <XpanManager> xm =  XpanManager::Get();
 if (xm) {
   xm->NotifyCoPVer(cop_ver_supported.len, cop_ver_supported.payload);
 return true;
 } else {
   return false;
 }
}

#endif

void DataHandler::SetOffloadHostConfig(ProtocolType type)
{
  char value[PROPERTY_VALUE_MAX] = {'\0'};
  logger_->PropertyGet(QSH_BLE_SNOOP_ENABLE_PROPERTY, value, "false");
  bool snoop_enabled = (strcmp(value, "true") == 0);
  bool need_send_config_cmd = snoop_enabled;

  if (need_send_config_cmd || offload_host_config_sent) {
    ALOGI("%s: need_send_config_cmd=%d, snoop_enabled=%d, config_already_sent=%d",
         __func__, need_send_config_cmd, snoop_enabled, offload_host_config_sent);
    offload_host_config_sent = need_send_config_cmd;

    if (!sendCommandWait(HCI_SET_OFFLOAD_HOST_CONFIG_CMD, type)) {
      ALOGE("%s: Failed to receive rsp for HCI_SET_OFFLOAD_HOST_CONFIG_CMD", __func__);
    }
  }
}

bool DataHandler::MakeHciCmdForSetOffloadHostConfig(
    uint8_t *cmd, size_t *len)
{
  if (cmd == NULL || len == NULL)
    return false;

  char value[PROPERTY_VALUE_MAX] = { '\0' };
  logger_->PropertyGet(QSH_BLE_SNOOP_ENABLE_PROPERTY, value, "false");
  bool snoop_enabled = (strcmp(value, "true") == 0);

  uint16_t opcode = cmd_opcode_pack(HCI_VENDOR_CMD_OGF, HCI_VS_OFFLOAD_HOST_COMM_CMD_OCF);
  cmd[0] = (uint8_t)opcode;
  cmd[1] = (uint8_t)(opcode >> 8);
  // skip cmd[2] // length
  cmd[3] = HCI_VS_OFFLOAD_HOST_SET_CONFIG_SUB_OPCODE;
  // skip cmd[4] //length of config
  cmd[5] = 0;
  if (snoop_enabled) {
    cmd[5] |= HCI_VS_OFFLOAD_HOST_BTSNOOP_BIT_MASK;
  } else {
    cmd[5] &= ~HCI_VS_OFFLOAD_HOST_BTSNOOP_BIT_MASK;
  }

  uint8_t *p_cmd_last = &(cmd[5]) + 1;
  cmd[2] = p_cmd_last - cmd - HCI_COMMAND_HDR_SIZE;
  cmd[4] = cmd[2] - 2;

  *len = p_cmd_last - cmd;

  if (cmd[4] > HCI_VS_OFFLOAD_HOST_MAX_CONFIG_LEN) {
    ALOGE("%s: config len=%d, exceeds max value", __func__, cmd[4]);
    return false;
  }

  return true;
}

BluetoothTransportType DataHandler::getTransportType(){
  return transport_type_;
}

void DataHandler :: SetHostAddOnFeatures(uint64_t chip_ver) {
  /* | UNICAST | BroadCast Assist |  BroadCast Service|
   * | Stereo recording |LC3Q |
   */
  uint8_t adv_audio_support_mask = 0;
  uint16_t host_add_on_feature_value = 0;
  char value[PROPERTY_VALUE_MAX] = {'\0'};

  ALOGI("%s: chip_ver: (0x%16llx), chip_ver_str: %s", __func__,
      (unsigned long long)chip_ver, convertChipVerToStr(chip_ver));

  if (chip_ver == HSP_VER_2_0 || chip_ver == HSP_VER_2_0_G ||
      chip_ver == HSP_VER_2_1 || chip_ver == HSP_VER_2_1_G ||
      chip_ver == HAMILTON_VER_1_0 || chip_ver == HAMILTON_VER_1_0_1 ||
      chip_ver == HAMILTON_VER_2_0 || chip_ver == MOSELLE_VER_1_2 ||
      chip_ver == GANGES_VER_1_0 || chip_ver == GANGES_VER_2_0 ||
      chip_ver == BRAHMA_VER_1_0 || chip_ver == BRAHMA_VER_2_0 ||
      chip_ver == ORNE_BT_VER_1_0|| chip_ver == EVROS_BT_VER_2 ||
      chip_ver == COLOGNE_BT_VER_1_0 || chip_ver == THEMISTO_VER_1_0 ||
	  chip_ver == THEMISTO_VER_2_0) {
    adv_audio_support_mask |= HOST_ADD_ON_ADV_AUDIO_UNICAST_FEAT_MASK;
    adv_audio_support_mask |= HOST_ADD_ON_ADV_AUDIO_BCA_FEAT_MASK;
    adv_audio_support_mask |= HOST_ADD_ON_ADV_AUDIO_BCS_FEAT_MASK;
    adv_audio_support_mask |= HOST_ADD_ON_ADV_AUDIO_STEREO_RECORDING;
    adv_audio_support_mask |= HOST_ADD_ON_ADV_AUDIO_LC3Q_FEAT_MASK;
    host_add_on_features.features[0] |= HOST_ADD_ON_QHS_FEAT_MASK;
  } else if (chip_ver == MOSELLE_VER_1_0 || chip_ver == MOSELLE_VER_1_1) {
    adv_audio_support_mask |= HOST_ADD_ON_ADV_AUDIO_BCS_FEAT_MASK;
    adv_audio_support_mask |= HOST_ADD_ON_ADV_AUDIO_BCA_FEAT_MASK;
    adv_audio_support_mask |= HOST_ADD_ON_ADV_AUDIO_UNICAST_FEAT_MASK;
    adv_audio_support_mask |= HOST_ADD_ON_ADV_AUDIO_LC3Q_FEAT_MASK;
  }

  host_add_on_features.features[0] |= adv_audio_support_mask;

  /*  Look for the index of highest byte where feature bit set
   *  index + 1 gives the length of features mask
   */
  for (int i = HOST_ADD_ON_FEATURE_MASK_MAX_LENGTH - 1; i >= 0; i--) {
    if(host_add_on_features.features[i] & HOST_ADD_ON_FEATURE_MASK_MAX_LENGTH) {
       host_add_on_features.feat_mask_len = i + 1;
      break;
    }
  }

  if (!property_get("persist.vendor.bluetooth.host_addon", value, "")) {
    host_add_on_feature_value |= host_add_on_features.features[0];
    host_add_on_feature_value |= host_add_on_features.features[1] << 8;
    property_set("persist.vendor.bluetooth.host_addon",
                 std::to_string(host_add_on_feature_value).c_str());
    ALOGI(
        "%s: host add on features property set for %s with feature set val:%d",
        __func__, convertChipVerToStr(chip_ver), host_add_on_feature_value);
  }

  ALOGI("%s: Decoded host add on features for %s with feature set val:%d",
        __func__, convertChipVerToStr(chip_ver),
    host_add_on_features.features[0]);
}

void DataHandler :: SetScramblingFeature(uint64_t chip_ver) {
    static bool is_set = false;

    if(is_set) return;
    ALOGI("%s: chip_ver: (0x%16llx), chip_ver_str: %s", __func__,
    (unsigned long long)chip_ver, convertChipVerToStr(chip_ver));

  if (chip_ver == CHEROKEE_VER_2_0 || chip_ver == CHEROKEE_VER_2_0_1 ||
      chip_ver == CHEROKEE_VER_2_1 || chip_ver == CHEROKEE_VER_2_1_1 ||
      chip_ver == APACHE_VER_1_0_0 || chip_ver == APACHE_VER_1_1_0 ||
      chip_ver == APACHE_VER_1_2_0 || chip_ver == APACHE_VER_1_2_1 ) {
    ALOGI("%s: for Apache & Cherookee chipset , SetScrambling flag to true", __func__);
    property_set("persist.vendor.qcom.bluetooth.scram.enabled", "true");
  }
    is_set = true;
 }

const char * DataHandler :: convertChipVerToStr(uint64_t chip_ver) {
  if ( QCA_VER_PROD_ID(chip_ver) == PROD_ID_BT_SOC_EVROS ) {
    chip_ver =  QCA_CHIPSET_VER_NO_SOC_ID(chip_ver);
  }
  switch (chip_ver){
    case ROME_VER_2_1:
      return "ROME_VER_2_1";
    case ROME_VER_3_0:
      return "ROME_VER_3_0";
    case ROME_VER_3_2:
      return "ROME_VER_3_2";
    case CHEROKEE_VER_2_0:
      return "CHEROKEE_VER_2_0";
    case CHEROKEE_VER_2_0_1:
      return "CHEROKEE_VER_2_0_1";
    case CHEROKEE_VER_2_1:
      return "CHEROKEE_VER_2_1";
    case CHEROKEE_VER_2_1_1:
      return "CHEROKEE_VER_2_1_1";
    case CHEROKEE_VER_3_1:
      return "CHEROKEE_VER_3_1";
    case CHEROKEE_VER_3_2:
    case CHEROKEE_VER_3_2_UMC:
      return "CHEROKEE_VER_3_2";
    case APACHE_VER_1_0_0:
      return "APACHE_VER_1_0_0";
    case APACHE_VER_1_1_0:
      return "APACHE_VER_1_1_0";
    case APACHE_VER_1_2_0:
      return "APACHE_VER_1_2_0";
    case APACHE_VER_1_2_1:
      return "APACHE_VER_1_2_1";
    case COMANCHE_VER_1_0_1:
      return "COMANCHE_VER_1_0_1";
    case COMANCHE_VER_1_1:
      return "COMANCHE_VER_1_1";
    case COMANCHE_VER_1_2:
    case COMANCHE_VER_1_2_UMC:
      return "COMANCHE_VER_1_2";
    case COMANCHE_VER_1_3:
    case COMANCHE_VER_1_3_TSMC:
    case COMANCHE_VER_1_3_UMC:
      return "COMANCHE_VER_1_3";
    case GENOA_VER_1_0:
      return "GENOA_VER_1_0";
    case GENOA_VER_2_0:
      return "GENOA_VER_2_0";
    case GENOA_VER_2_0_0_2:
      return "GENOA_VER_2_0_0_2";
    case HASTINGS_VER_2_0:
      return "HASTINGS_VER_2_0";
    case HSP_VER_1_0:
      return "HSP_VER_1_0";
    case HSP_VER_1_1:
      return "HSP_VER_1_1";
    case HSP_VER_2_0:
    case HSP_VER_2_0_G:
      return "HSP_VER_2_0";
    case HSP_VER_2_1:
    case HSP_VER_2_1_G:
      return "HSP_VER_2_1";
    case MOSELLE_VER_1_0:
      return "MOSELLE_VER_1_0";
    case MOSELLE_VER_1_1:
      return "MOSELLE_VER_1_1";
    case MOSELLE_VER_1_2:
      return "MOSELLE_VER_1_2";
    case HAMILTON_VER_1_0:
      return "HAMILTON_VER_1_0";
    case HAMILTON_VER_1_0_1:
      return "HAMILTON_VER_1_0_1";
    case HAMILTON_VER_2_0:
      return "HAMILTON_VER_2_0";
    case GANGES_VER_1_0:
      return "GANGES_VER_1_0";
    case GANGES_VER_2_0:
      return "GANGES_VER_2_0";
    case BRAHMA_VER_1_0:
      return "BRAHMA_VER_1_0";
    case BRAHMA_VER_2_0:
      return "BRAHMA_VER_2_0";
    case ORNE_BT_VER_1_0:
      return "ORNE_BT_VER_1_0";
    case COLOGNE_BT_VER_1_0:
      return "COLOGNE_BT_VER_1_0";
    case EVROS_BT_VER_1:
      return "EVROS_BT_VER_1";
    case EVROS_BT_VER_2:
      return "EVROS_BT_VER_2";
    case THEMISTO_VER_1_0:
      return "THEMISTO_VER_1_0";
    case THEMISTO_VER_2_0:
      return "THEMISTO_VER_2_0";
    case INVALID_CHIP_VERSION:
      if (soc_type_ == BT_SOC_SMD)
        return "Pronto";
    default:
      return "INVALID_CHIP_VERSION";
  }
}

void DataHandler :: KillInitThread() {
  if (!is_init_thread_killed) {
    ALOGE("%s: Killing init thread", __func__);
    if (pthread_kill(init_thread_.native_handle(), SIGUSR2))
      ALOGE("%s: Failed to kill init thread, Errno: %s", __func__, strerror(errno));
    is_init_thread_killed = true;
    // Sleep for 50 ms to ensure that thread exit happens
    usleep(50 * 1000);

    // Ensure acquired mutex(s) are released
    logger_->UnlockRingbufferMutex();
    controller_->UnlockControllerMutex();
    logger_->UnlockDiagMutex();
  }
}

Controller* DataHandler::GetControllerRef() {
  return controller_;
}

#ifdef XPAN_SUPPORTED
void DataHandler :: QHciInitialized(bool status) {
  qhci_initialized = status;
  ALOGI("QHCI notified with status :%d", status);
}

bool DataHandler::GetQHciState()
{
  return qhci_initialized;
}
#endif

#ifdef QTI_BT_FMD_SUPPORTED
bool DataHandler::IsFmdModeEnabled() {
  return fmd_mode && isFmdSupported;
}

void DataHandler::SetFmdMode(bool enable) {
  fmd_mode = enable;
  ALOGI("%s Enable : %d", __func__, enable);
}

bool DataHandler::IsFmdModeEnabledSendToSoc() {
  return fmd_info.fmd_enable_sent;
}

void DataHandler::FmdWaitForKeys(int time_out) {
  std::unique_lock<std::mutex> fmd_lock(fmd_keys_wait_mutex_);
  ALOGE("%s = %d msec ", __func__, time_out);
  wait_fmdkeys.wait_for(fmd_lock, std::chrono::milliseconds(time_out));
  return;
}

void DataHandler :: FmdFinderKeysReceived() {
  std::lock_guard<std::mutex> fmd_lock(fmd_keys_wait_mutex_);
  fmd_keys_status = FINDER_KEYS_RECEIVED;
  wait_fmdkeys.notify_all();
  ALOGI("%s: Notify to the wating Client", __func__);
}

void DataHandler::GenerateKeyData() {
  char value_prop_pf[PROPERTY_VALUE_MAX] = {'\0'};
  property_get("persist.vendor.bluetooth.fmd_test_keys", value_prop_pf,
               "true");
  if (strcmp(value_prop_pf, "false") == 0) {
    ALOGI("%s Skiping FMD Test keys data", __func__);
    return;
  }

  int i = 0;
  for(i = 0; i < FMD_TOTAL_ADV_DATA_KEY_SIZE; i++) {
    fmd_key_data[i] = i%255;
  }
  fmd_info.no_of_keys = FMD_TOTAL_ADV_DATA_KEY_SIZE/20;
  fmd_info.length = FMD_TOTAL_ADV_DATA_KEY_SIZE;
  fmd_keys_status = DEFAULT_FINDER_KEYS_ASSIGNED;
  ALOGI("%s No of Keys : %d length %d", __func__, fmd_info.no_of_keys,
          fmd_info.length);
}

bool DataHandler::IsSocAcceptedFmdInfo(const hidl_vec <uint8_t> *hidl_data) {
  const uint8_t* data = hidl_data->data();
  uint16_t opcode = ((data[4] << 8) | (data[3]));
  ALOGI("%s: (subopcode:0x%x) Status %d ",
          __func__,opcode,status);
  if (opcode == 0xFC37) {
    uint8_t sub_opcode = data[6];
    if ((sub_opcode == 0x10) || (sub_opcode == 0x11) ||
         (sub_opcode = 0x12)) {
      uint8_t status = data[5];
      if(sub_opcode == 0x12 && status == 0x00){
#ifdef IS_SMC_HAL_V2
        SMCLoader* smc_loader = SMCLoader::getInstance();
        isFMDentered = smc_loader->enterFMD();
        ALOGE("%s: (subopcode:0x%x) Status %d ret for enterFMD is %d",
              __func__, status, sub_opcode,isFMDentered);
        if(isFMDentered == STATUS_SUCCESS)
          GetFmdPropValues();
#endif
        return true;
      }
      else if (status) {
        ALOGI("%s: (subopcode:0x%x) Fail Status %d",
          __func__, status, sub_opcode);
        return false;
      } else {
        return true;
      }
    }
  }
  return false;
}
#endif

std::thread::id DataHandler :: GetInitThreadId() {
  return init_thread_id;
}

#ifdef BT_CP_CONNECTED
void DataHandler :: XMInitialized(bool status) {
  xpan_manager_initialized = status;
  ALOGI("XM manager notified with status :%d", status);
}

bool DataHandler::GetXMState()
{
  return xpan_manager_initialized;
}
#endif

#ifdef BTOFF_DELAY_SUPPORT
bool DataHandler::isBtOffDelaySupport() {
  if (is_btoff_delay_support == -1) {
    bool enabled = property_get_bool("persist.vendor.service.bdroid.btoffdelay", false);
    is_btoff_delay_support = enabled ? 1 : 0;
  }
  return (is_btoff_delay_support == 1);
}

#define BTOFF_DELAY_TIMEOUT 10000
int DataHandler::getBTOffDelayTimeoutValue() {
  return property_get_int32("persist.vendor.service.bdroid.btoffdelay_timeout",
                             BTOFF_DELAY_TIMEOUT);
}

void DataHandler::setProtocolCallback(InitializeCallback init_cb,
                                        DataReadCallback data_read_cb) {
  std::map<ProtocolType, ProtocolCallbacksType *>::iterator it;
  DataHandler *data_handler = DataHandler::Get();

  it = data_handler->protocol_info_.find(TYPE_BT);
  if (it != data_handler->protocol_info_.end()) {
    ALOGD("%s: update callbacks", __func__);
    ProtocolCallbacksType *cb_data = (ProtocolCallbacksType*) it->second;
    cb_data->init_cb = init_cb;
    cb_data->data_read_cb = data_read_cb;
  }
}

void DataHandler::BtOffDelayTimeout(union sigval) {
  std::unique_lock<std::mutex> guard(init_mutex_);
  std::unique_lock<std::mutex> lock(btoff_delay_timer_mutex);

  ALOGD("%s: BTOffDelayTimer expire, close BT now", __func__);
  btoff_delay_timer_state = TIMER_OVERFLOW;

  if (data_handler) {
    bool cleanup_thread_started = false;
    if (data_handler->protocol_info_.size() == MIN_CLIENTS_ACTIVE) {
      cleanup_thread_started = true;
      data_handler->StartCleanupTimer();
    }

    bool status = data_handler->Close(TYPE_BT);
    if (cleanup_thread_started)
      data_handler->StopCleanupThreadTimer();

    if (status) {
      delete data_handler;
      data_handler = NULL;
    }
  }
}

int DataHandler::getBtOffTimerState() {
  std::unique_lock<std::mutex> lock(btoff_delay_timer_mutex);

  return btoff_delay_timer_state;
}

void DataHandler::StartBTOffTimer() {
  struct itimerspec ts;
  struct sigevent se;
  uint32_t timeout = (uint32_t) getBTOffDelayTimeoutValue();
  std::unique_lock<std::mutex> lock(btoff_delay_timer_mutex);

  if (btoff_delay_timer_state == TIMER_NOT_CREATED) {
    se.sigev_notify_function = (void (*)(union sigval)) BtOffDelayTimeout;
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = &btoff_delay_timer_;
    se.sigev_notify_attributes = NULL;
    if (!timer_create(CLOCK_MONOTONIC, &se, &btoff_delay_timer_)) {
      btoff_delay_timer_state = TIMER_CREATED;
    } else {
      ALOGE("%s: Failed to create BTOffDelayTimer", __func__);
    }
  }
  if (btoff_delay_timer_state == TIMER_CREATED ||
        btoff_delay_timer_state == TIMER_OVERFLOW) {
    ts.it_value.tv_sec = (timeout / 1000);
    ts.it_value.tv_nsec = 1000000 * (timeout % 1000);
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    if (timer_settime(btoff_delay_timer_, 0, &ts, 0) == -1) {
      ALOGE("%s: Failed to start BTOffDelayTimer timer", __func__);
      return;
    } else {
      btoff_delay_timer_state = TIMER_ACTIVE;
      ALOGD("%s: BTOffDelayTimer timer started with timeout %d", __func__, timeout);
    }
  }
}

void DataHandler::StopBTOffTimer() {
  struct itimerspec ts;
  std::unique_lock<std::mutex> lock(btoff_delay_timer_mutex);

  ALOGD("%s: stop BTOffDelayTimer", __func__);
  if (btoff_delay_timer_state == TIMER_NOT_CREATED) {
    ALOGD("%s: BTOffDelayTimer already stopped", __func__);
    return;
  } else if (btoff_delay_timer_state == TIMER_ACTIVE ||
          btoff_delay_timer_state == TIMER_CREATED){
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    if (timer_settime(btoff_delay_timer_, 0, &ts, 0) == -1) {
      ALOGE("%s:Failed to stop BTOffDelayTimer thread timer", __func__);
    }
    timer_delete(btoff_delay_timer_);
    btoff_delay_timer_state = TIMER_NOT_CREATED;
    ALOGD("%s: BTOffDelayTimer was deleted", __func__);
  }
}

bool DataHandler::checkBTCloseDelay(ProtocolType type) {
  bool ret = false;
  ALOGI("%s: type = %d", __func__, type);
  if (isBtOffDelaySupport() && type == TYPE_BT) {
    int state = getBtOffTimerState();
    if (state  == TIMER_ACTIVE || state == TIMER_CREATED) {
      ALOGD("%s: BTOffDelayTimer is starting, stop it now", __func__);
      StopBTOffTimer();
      ret = true;
    }
  }
  return ret;
}

bool DataHandler::BTCloseDelay(ProtocolType type) {
  bool ret = false;
  ALOGI("%s: type = %d", __func__, type);
  if (isBtOffDelaySupport() && type == TYPE_BT) {
    StartBTOffTimer();
    ALOGD("%s: start BTOffDelayTimer", __func__);
    ret = true;
  }
  return ret;
}

#endif

bool DataHandler::IsHciPacketValid(HciPacketType type)
{
  bool result = false;

  if (HCI_PACKET_TYPE_EVENT == type || HCI_PACKET_TYPE_ACL_DATA == type ||
      HCI_PACKET_TYPE_SCO_DATA == type || HCI_PACKET_TYPE_COMMAND == type ||
      HCI_PACKET_TYPE_ANT_CTRL == type || HCI_PACKET_TYPE_ANT_DATA == type ||
      HCI_PACKET_TYPE_FM_CMD == type || HCI_PACKET_TYPE_FM_EVENT == type ||
      HCI_PACKET_TYPE_ISO_DATA == type || HCI_PACKET_TYPE_PERI_EVT == type ||
      HCI_PACKET_TYPE_PERI_CMD == type || HCI_PACKET_TYPE_PERI_DATA == type) {
    result = true;
  }

  return result;
}

ProtocolType DataHandler::GetProtocol(HciPacketType pkt_type)
{
  ProtocolType type = TYPE_BT;

  switch (pkt_type) {
    case HCI_PACKET_TYPE_COMMAND:
    case HCI_PACKET_TYPE_ACL_DATA:
    case HCI_PACKET_TYPE_SCO_DATA:
    case HCI_PACKET_TYPE_EVENT:
    case HCI_PACKET_TYPE_ISO_DATA:
      type = TYPE_BT;
      break;
    case HCI_PACKET_TYPE_ANT_CTRL:
    case HCI_PACKET_TYPE_ANT_DATA:
      type = TYPE_ANT;
      break;
    case HCI_PACKET_TYPE_FM_CMD:
    case HCI_PACKET_TYPE_FM_EVENT:
      type = TYPE_FM;
      break;
    case HCI_PACKET_TYPE_PERI_CMD:
    case HCI_PACKET_TYPE_PERI_DATA:
    case HCI_PACKET_TYPE_PERI_EVT:
      type = TYPE_PERI;
      break;
    default:
      break;
  }
  return type;
};

void DataHandler::StartCleanupTimer()
{
  int status;
  struct itimerspec ts;
  struct sigevent se;

  ALOGI("%s", __func__);
  if (GetCleanupThreadTimerState() == TIMER_NOT_CREATED) {
    se.sigev_notify_function = (void (*)(union sigval))CleanupThreadTimeOut;
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = this;
    se.sigev_notify_attributes = NULL;

    ALOGI("%s created", __func__);
    status = timer_create(CLOCK_MONOTONIC, &se, &cleanup_timer_state_machine_.timer_id);
    if (status == 0)
      SetCleanupThreadTimerState(TIMER_CREATED);
  }

  if ((GetCleanupThreadTimerState() == TIMER_CREATED) ||
      (GetCleanupThreadTimerState() == TIMER_OVERFLOW)) {
    cleanup_timer_state_machine_.timeout_ms = BT_CLOSE_TIMEOUT;
    ts.it_value.tv_sec = cleanup_timer_state_machine_.timeout_ms / 1000;
    ts.it_value.tv_nsec = 1000000 * (cleanup_timer_state_machine_.timeout_ms % 1000);
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    ALOGI("%s timer started", __func__);
    status = timer_settime(cleanup_timer_state_machine_.timer_id, 0, &ts, 0);
    if (status == -1)
      ALOGE("%s:Failed to set RxThread Usage timer", __func__);
    else
      SetCleanupThreadTimerState(TIMER_ACTIVE);
  }
}

void DataHandler::CleanupThreadTimeOut(union sigval sig)
{
  std::unique_lock<std::mutex> guard(cleanup_thread_state_mutex_);
  ALOGW("%s: lapsed 30 seconds for cleanup", __func__);
  kill(getpid(), SIGKILL);
}

void DataHandler::StopCleanupThreadTimer()
{
  int status;
  struct itimerspec ts;

  if (GetCleanupThreadTimerState() != TIMER_NOT_CREATED) {
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    status = timer_settime(cleanup_timer_state_machine_.timer_id, 0, &ts, 0);
    if(status == -1) { 
      ALOGE("%s:Failed to stop Rx thread timer",__func__);
      return;
    }
    ALOGI("%s: cleanup timer Stopped",__func__);
    timer_delete(cleanup_timer_state_machine_.timer_id);
    SetCleanupThreadTimerState(TIMER_NOT_CREATED);
  }
}

TimerState DataHandler::GetCleanupThreadTimerState()
{
  std::unique_lock<std::mutex> guard(cleanup_thread_state_mutex_);
  return cleanup_timer_state_machine_.timer_state;
}

void DataHandler::SetCleanupThreadTimerState(TimerState state)
{
  std::unique_lock<std::mutex> guard(cleanup_thread_state_mutex_);
  cleanup_timer_state_machine_.timer_state = state;
}

/*
 * bt_pdr_handler is intended to handle BT PDR notification
 * from remote proc over Glink. Handle it same as Q6 SSR and
 * restart BT HAL.
*/
#ifdef PASSTHROUGH_ENABLED
void DataHandler::bt_pdr_handler(void)
{
  ALOGE("%s: called", __func__);

  DataHandler *data_handler = DataHandler::Get();
  if (data_handler == nullptr)
  {
    ALOGE("%s: failed to get data handler", __func__);
    return;
  }

  SpiController *instance = (SpiController *)data_handler->controller_;
  if (instance == nullptr)
  {
    ALOGE("%s: failed to get SPI controller", __func__);
    return;
  }
  /* ADSP SSR and BT PDR have same way of handing BT recovery and there
  * is chance that remoteproc triggers both event simultaneously.
  */
  if(instance->isQ6SsrOngoing() == false)
  {
    instance->handleQ6SSR();
    data_handler->CloseSpiDriverFd();
    // Kill BT HAL to start the recovery process at application level
    kill(getpid(), SIGKILL);
  } else {
    ALOGE("%s: Q6 SSR ongoing. Bailing out",__func__);
  }
}
#endif

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
