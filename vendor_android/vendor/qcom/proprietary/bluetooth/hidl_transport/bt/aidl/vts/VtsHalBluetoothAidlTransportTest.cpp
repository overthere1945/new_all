/*
 * Copyright (c) 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a contribution.
 *
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/BnBluetoothHciCallbacks.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHci.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h>
#include <aidl/android/hardware/bluetooth/Status.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <csignal>
#include <ctime>
#include <cutils/properties.h>
#include <wake_lock.h>
//#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using aidl::android::hardware::bluetooth::IBluetoothHci;
using aidl::android::hardware::bluetooth::IBluetoothHciCallbacks;
using aidl::android::hardware::bluetooth::Status;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;

//using android::hardware::hidl_vec;

#define WAIT_FOR_INIT_TIMEOUT std::chrono::milliseconds(2900)
#define WAIT_FOR_SSR (10000)
#define WAIT_FOR_RX_THREAD_STUCK_TIMEOUT std::chrono::milliseconds(3000)
#define INTERFACE_CLOSE_DELAY_MS (200)
#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )
#define MAX_CMD_LOG (10)
#define MAX_DATA_LOG (10)
#define MAX_EVENT_LOG (10)
#define COMMAND_GET_SOC_CTS_STATUS \
  { 0x0c, 0xfc, 0x01, 0x39 }
#define COMMAND_GET_SOC_RTS_STATUS \
  { 0x0c, 0xfc, 0x03, 0x3A }
#define COMMAND_RESET_UART_FLOW_ON \
  { 0x00, 0xFD, 0x00 }
#define COMMAND_GET_CTS_STATUS \
  { 0x10, 0xFD, 0x00 }
#define COMMAND_HCI_GET_DEBUG_INFO \
  { 0x5B, 0xFD, 0x00 }
#define SSR_SW_ERROR_FATAL \
  { 0x0C, 0xFC, 0x01, 0x26 }
#define SSR_SW_EXCEPTION_DIV_BY_0 \
  { 0x0C, 0xFC, 0x01, 0x27 }
#define SSR_SW_EXCEPTION_NULL_PTR \
  { 0x0C, 0xFC, 0x01, 0x28 }
#define SSR_WATCHDOG_BITS \
   { 0x0C, 0xFC, 0x01, 0x29 }
#define VENDOR_SPECIFIC_EVENT 0xFF
#define VENDOR_SPECIFIC_EVENT_CODE_BYTE1 0xFC
#define VENDOR_SPECIFIC_EVENT_CODE_BYTE2 0x00
#define COMMAND_HCI_READ_BUFFER_SIZE \
  { 0x05, 0x10, 0x00 }

// Bluetooth Core Specification 3.0 + HS
static constexpr uint8_t kHciMinimumHciVersion = 5;
// Bluetooth Core Specification 3.0 + HS
static constexpr uint8_t kHciMinimumLmpVersion = 5;

static constexpr size_t kNumHciCommandsBandwidth = 100;
static constexpr size_t kNumScoPacketsBandwidth = 100;
static constexpr size_t kNumAclPacketsBandwidth = 100;
static constexpr std::chrono::milliseconds kWaitForInitTimeout(2900);
static constexpr std::chrono::milliseconds kWaitForHciEventTimeout(2000);
static constexpr std::chrono::milliseconds kWaitForScoDataTimeout(1000);
static constexpr std::chrono::milliseconds kWaitForAclDataTimeout(1000);
static constexpr std::chrono::milliseconds kInterfaceCloseDelayMs(200);

static constexpr uint8_t kCommandHciShouldBeUnknown[] = {
    0xff, 0x3B, 0x08, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
static constexpr uint8_t kCommandHciReadLocalVersionInformation[] = {0x01, 0x10,
                                                                     0x00};
static constexpr uint8_t kCommandHciReadBufferSize[] = {0x05, 0x10, 0x00};
static constexpr uint8_t kCommandHciWriteLoopbackModeLocal[] = {0x02, 0x18,
                                                                0x01, 0x01};
static constexpr uint8_t kCommandHciReset[] = {0x03, 0x0c, 0x00};
static constexpr uint8_t kCommandHciSynchronousFlowControlEnable[] = {
    0x2f, 0x0c, 0x01, 0x01};
static constexpr uint8_t kCommandHciWriteLocalName[] = {0x13, 0x0c, 0xf8};
static constexpr uint8_t kHciStatusSuccess = 0x00;
static constexpr uint8_t kHciStatusUnknownHciCommand = 0x01;

static constexpr uint8_t kEventConnectionComplete = 0x03;
static constexpr uint8_t kEventCommandComplete = 0x0e;
static constexpr uint8_t kEventCommandStatus = 0x0f;
static constexpr uint8_t kEventNumberOfCompletedPackets = 0x13;
static constexpr uint8_t kEventLoopbackCommand = 0x19;

static constexpr size_t kEventCodeByte = 0;
static constexpr size_t kEventLengthByte = 1;
static constexpr size_t kEventFirstPayloadByte = 2;
static constexpr size_t kEventCommandStatusStatusByte = 2;
static constexpr size_t kEventCommandStatusOpcodeLsByte = 4;    // Bytes 4 and 5
static constexpr size_t kEventCommandCompleteOpcodeLsByte = 3;  // Bytes 3 and 4
static constexpr size_t kEventCommandCompleteStatusByte = 5;
static constexpr size_t kEventCommandCompleteFirstParamByte = 6;
static constexpr size_t kEventLocalHciVersionByte =
    kEventCommandCompleteFirstParamByte;
static constexpr size_t kEventLocalLmpVersionByte =
    kEventLocalHciVersionByte + 3;

static constexpr size_t kEventConnectionCompleteParamLength = 11;
static constexpr size_t kEventConnectionCompleteType = 11;
static constexpr size_t kEventConnectionCompleteTypeSco = 0;
static constexpr size_t kEventConnectionCompleteTypeAcl = 1;
static constexpr size_t kEventConnectionCompleteHandleLsByte = 3;

static constexpr size_t kEventNumberOfCompletedPacketsNumHandles = 2;

static constexpr size_t kAclBroadcastFlagOffset = 6;
static constexpr uint8_t kAclBroadcastFlagPointToPoint = 0x0;
static constexpr uint8_t kAclBroadcastPointToPoint =
    (kAclBroadcastFlagPointToPoint << kAclBroadcastFlagOffset);

static constexpr uint8_t kAclPacketBoundaryFlagOffset = 4;
static constexpr uint8_t kAclPacketBoundaryFlagFirstAutoFlushable = 0x2;
static constexpr uint8_t kAclPacketBoundaryFirstAutoFlushable =
    kAclPacketBoundaryFlagFirstAutoFlushable << kAclPacketBoundaryFlagOffset;

// To discard Qualcomm ACL debugging
static constexpr uint16_t kAclHandleQcaDebugMessage = 0xedc;

static const std::string kMaxAppSleepTimer = "--maxAppSleepTimer=";
static const std::string kMinAppSleepTimer = "--minAppSleepTimer=";
static const std::string kPINCTRLTimerValue = "--pinctrlTimerValue=";
static const std::string kMaxIterations = "--maxIterations=";
static const std::string kMaxMsgsPerIteration = "--maxMsgsPerIteration=";
static const std::string kLogEvents = "--logEvents";
static const std::string kConsoleLog = "--consoleLog";
static const std::string kLogCommands = "--logCommands";
static const std::string kLogData = "--logData";
static uint32_t maxAppSleepTimer = 1000; // in ms
static uint32_t pinctrlTimerValue = 100; // in ms
static uint32_t minAppSleepTimer = 0; // in ms
static uint32_t maxIterations = 1;
static uint32_t maxMsgsPerIteration = 1;
static bool     logEvents = false;
static bool     consoleLog = false;
static bool     logCommands = false;
static bool     logData = false;
bool timerExpired = false;

enum SleepTimerState {
  TIMER_NOT_CREATED = 0x00,
  TIMER_CREATED,
  TIMER_ACTIVE
};

class ThroughputLogger {
 public:
  ThroughputLogger(std::string task)
      : total_bytes_(0),
        task_(task),
        start_time_(std::chrono::steady_clock::now()) {}

  ~ThroughputLogger() {
    if (total_bytes_ == 0) {
      return;
    }
    std::chrono::duration<double> duration =
        std::chrono::steady_clock::now() - start_time_;
    double s = duration.count();
    if (s == 0) {
      return;
    }
    double rate_kb = (static_cast<double>(total_bytes_) / s) / 1024;
    ALOGI("%s %.1f KB/s (%zu bytes in %.3fs)", task_.c_str(), rate_kb,
          total_bytes_, s);
  }

  void setTotalBytes(size_t total_bytes) { total_bytes_ = total_bytes; }

 private:
  size_t total_bytes_;
  std::string task_;
  std::chrono::steady_clock::time_point start_time_;
};

// The main test class for Bluetooth HAL.
class BluetoothTransportTest : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    ALOGE("Start of SetUp()");
    static int count = 0;
    Wakelock::Init();
    Wakelock::Acquire();
    initialized = false;
    // currently test passthrough mode only
    hci = IBluetoothHci::fromBinder(
        SpAIBinder(AServiceManager_waitForService("android.hardware.bluetooth.IBluetoothHci/default")));
    ASSERT_NE(hci, nullptr);
    ALOGI("%s: getService() for bluetooth hci is %s", __func__,
          hci->isRemote() ? "remote" : "local");

#if 0
    // Lambda function
    auto on_binder_death = [](void* /*cookie*/) { FAIL(); };

    bluetooth_hci_death_recipient =
        AIBinder_DeathRecipient_new(on_binder_death);
    ASSERT_NE(bluetooth_hci_death_recipient, nullptr);
    ASSERT_EQ(STATUS_OK,
              AIBinder_linkToDeath(hci->asBinder().get(),
                                   bluetooth_hci_death_recipient, 0));
#endif
    hci_cb = ndk::SharedRefBase::make<BluetoothHciCallbacks>(*this);
    ASSERT_NE(hci_cb, nullptr);

    max_acl_data_packet_length = 0;
    max_sco_data_packet_length = 0;
    max_acl_data_packets = 0;
    max_sco_data_packets = 0;
    simulateRxThreadStuck = false;
    serviceDiedOk = false;

    event_cb_count = 0;
    acl_cb_count = 0;
    sco_cb_count = 0;

    isUartRtsTC = false;
    isUartCtsTC = false;
    ASSERT_FALSE(initialized);

    ASSERT_TRUE(hci->initialize(hci_cb).isOk());

    if (count == 0)
    {
        auto future = initialized_promise.get_future();
        auto timeout_status = future.wait_for(kWaitForInitTimeout);
        ASSERT_EQ(timeout_status, std::future_status::ready);
        ASSERT_TRUE(future.get());
    }
    else
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_FOR_INIT_TIMEOUT));

    Wakelock::Release();
    ASSERT_TRUE(initialized);
    count++;
    ALOGE("End of SetUp()");
  }

  virtual void TearDown() override {
    ALOGE("Start of TearDown()");
    // Should not be checked in production code
    if (initialized) {
    Wakelock::Acquire();
    ASSERT_TRUE(hci->close().isOk());
    std::this_thread::sleep_for(kInterfaceCloseDelayMs);
    ALOGI("TearDown: SleepTimer Expired");
    handle_no_ops();
    discard_qca_debugging();
    EXPECT_EQ(static_cast<size_t>(0), event_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), sco_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), acl_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), iso_queue.size());
    Wakelock::Release();
    Wakelock::CleanUp();
    }
    ALOGI("TearDown");
    ALOGE("End of TearDown()");
  }

  void setBufferSizes();
  void setSynchronousFlowControlEnable();
  void sendHCICommand(std::vector<uint8_t> cmd);
  void SendCmdToUARTFlowON();
  void GetCTSStatus();

  // Functions called from within tests in loopback mode
  void sendAndCheckHci(int num_packets);
  void sendAndCheckSco(int num_packets, size_t size, uint16_t handle);
  void sendAndCheckAcl(int num_packets, size_t size, uint16_t handle);
  void sendAndCheckISO(int num_packets, size_t size, uint16_t handle);

  // Helper functions to try to get a handle on verbosity
  void reset();
  void enterLoopbackMode();
  void handle_no_ops();
  void discard_qca_debugging();
  void wait_for_event(bool timeout_is_error);
  void wait_for_command_complete_event(std::vector<uint8_t> cmd);
  int wait_for_completed_packets_event(uint16_t handle);
  void StartSleepTimer(int sleepTime);
  bool StopSleepTimer();
  void StartGetRTSCTSStatusTimer(int sleepTime);
  bool StopRTSCTSStatusTimer();
  bool IsSIBSEnabled();

  // A simple test implementation of BluetoothHciCallbacks.
  class BluetoothHciCallbacks
      : public aidl::android::hardware::bluetooth::BnBluetoothHciCallbacks {
    BluetoothTransportTest& parent_;

   public:
    BluetoothHciCallbacks(BluetoothTransportTest& parent) : parent_(parent){};

    virtual ~BluetoothHciCallbacks() = default;

    ndk::ScopedAStatus initializationComplete(Status status) {
      static int count = 0;
      ALOGE("Start of initializationComplete()");
      std::cout << __func__ << "- Status: " << (int32_t)status << std::endl;
      if (count == 0)
      parent_.initialized_promise.set_value(status == Status::SUCCESS);
      parent_.initialized = (status == Status::SUCCESS);
      ALOGI("%s (status = %d)", __func__, static_cast<int>(status));
      ALOGE("End of initializationComplete()");
      count ++;
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus hciEventReceived(const std::vector<uint8_t>& event) {
      ALOGE("Start of hciEventReceived()");
      if ((event[0] == VENDOR_SPECIFIC_EVENT) &&
          (event[2] == VENDOR_SPECIFIC_EVENT_CODE_BYTE1) &&
          (event[3] == VENDOR_SPECIFIC_EVENT_CODE_BYTE2)) {
        std::cout << "Ignoring Enhanced Controller Logs" << std::endl;
        ALOGE("End of hciEventReceived() vendor specific events check");
        return ScopedAStatus::ok();
      }
      if (parent_.simulateRxThreadStuck) {
        // Don't process events for some time
         std::cout << "Simulating Rx thread wait" << std::endl;
         std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_FOR_RX_THREAD_STUCK_TIMEOUT));
         ALOGI("hciEventReceived: RxThreadStuck SleepTimer Expired");
         ALOGE("End of hciEventReceived() rx thread simulation check");
         return ScopedAStatus::ok();
      }

      parent_.event_cb_count++;
      parent_.event_queue.push(event);
      ALOGI("Event received #:%d (length = %d)", parent_.event_cb_count, static_cast<int>(event.size()));
      if (logEvents) {
        std::ostringstream os;
        std::unique_lock<std::mutex> guard(parent_.log_mutex);
        os << "Event received:" << parent_.event_cb_count << " Length: 0x" << event.size() << "  ";
        for(int i = 0; (i < MAX_EVENT_LOG) && (i < event.size()); i++)
          os << " 0x" << HEX(event[i]);
        os << std::endl;
        if (consoleLog)
          std::cout << os.str();
        ALOGI("%s: %s", __func__, os.str().c_str());
      }
      ALOGE("End of hciEventReceived()");
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus aclDataReceived(const std::vector<uint8_t>& data) {
      ALOGE("Start of aclDataReceived()");
      parent_.acl_cb_count++;
      parent_.acl_queue.push(data);
      ALOGI("ACL Data received #:%d (length = %d)", parent_.acl_cb_count, static_cast<int>(data.size()));
      std::cout << "AclData received #:" << parent_.acl_cb_count << " Length: 0x" << data.size() << std::endl;
      if (logData) {
        std::unique_lock<std::mutex> guard(parent_.log_mutex);
       for(int i = 0; (i < MAX_DATA_LOG) && (i < data.size()); i++)
         std::cout << " 0x" << HEX(data[i]);
       std::cout << std::endl;
      }
      ALOGE("End of aclDataReceived()");
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus scoDataReceived(const std::vector<uint8_t>& data) {
      ALOGE("Start of scoDataReceived()");
      parent_.sco_cb_count++;
      parent_.sco_queue.push(data);
      ALOGI("SCO Data received (length = %d)", static_cast<int>(data.size()));
      if (logData) {
        std::unique_lock<std::mutex> guard(parent_.log_mutex);
        std::cout << "ScoData received #:" << parent_.sco_cb_count << " Length: 0x " << data.size() << std::endl;
        for(int i = 0; (i < MAX_DATA_LOG) && (i < data.size()); i++)
          std::cout << " 0x" << HEX(data[i]);
        std::cout << std::endl;
      }
      ALOGE("End of scoDataReceived()");
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus isoDataReceived(const std::vector<uint8_t>& data) {
      ALOGE("Start of isoDataReceived()");
      ALOGI("ISO Data received (length = %d)", static_cast<int>(data.size()));
      return ScopedAStatus::ok();
    };
  };

  template <class T>
  class WaitQueue {
   public:
    WaitQueue(){};

    virtual ~WaitQueue() = default;

    bool empty() const {
      std::lock_guard<std::mutex> lock(m_);
      return q_.empty();
    };

    size_t size() const {
      std::lock_guard<std::mutex> lock(m_);
      return q_.size();
    };

    void push(const T& v) {
      std::lock_guard<std::mutex> lock(m_);
      q_.push(v);
      ready_.notify_one();
    };

    bool pop(T& v) {
      std::lock_guard<std::mutex> lock(m_);
      if (q_.empty()) {
        return false;
      }
      v = std::move(q_.front());
      q_.pop();
      return true;
    };

    bool front(T& v) {
      std::lock_guard<std::mutex> lock(m_);
      if (q_.empty()) {
        return false;
      }
      v = q_.front();
      return true;
    };

    void wait() {
      std::unique_lock<std::mutex> lock(m_);
      while (q_.empty()) {
        ready_.wait(lock);
      }
    };

    bool waitWithTimeout(std::chrono::milliseconds timeout) {
      std::unique_lock<std::mutex> lock(m_);
      while (q_.empty()) {
        if (ready_.wait_for(lock, timeout) == std::cv_status::timeout) {
          return false;
        }
      }
      return true;
    };

    bool tryPopWithTimeout(T& v, std::chrono::milliseconds timeout) {
      std::unique_lock<std::mutex> lock(m_);
      while (q_.empty()) {
        if (ready_.wait_for(lock, timeout) == std::cv_status::timeout) {
          return false;
        }
      }
      v = std::move(q_.front());
      q_.pop();
      return true;
    };

   private:
    mutable std::mutex m_;
    std::queue<T> q_;
    std::condition_variable_any ready_;
  };

  std::shared_ptr<IBluetoothHci> hci;
  std::shared_ptr<BluetoothHciCallbacks> hci_cb;
  AIBinder_DeathRecipient* bluetooth_hci_death_recipient;
  WaitQueue<std::vector<uint8_t>> event_queue;
  WaitQueue<std::vector<uint8_t>> acl_queue;
  WaitQueue<std::vector<uint8_t>> sco_queue;
  WaitQueue<std::vector<uint8_t>> iso_queue;

  bool initialized;
  std::promise<bool> initialized_promise;
  int event_cb_count;
  int sco_cb_count;
  int acl_cb_count;
  int iso_cb_count;

  int max_acl_data_packet_length;
  int max_sco_data_packet_length;
  int max_acl_data_packets;
  int max_sco_data_packets;
  std::vector<uint16_t> sco_connection_handles;
  std::vector<uint16_t> acl_connection_handles;
  static bool simulateRxThreadStuck;
  static bool serviceDiedOk;
  static bool isUartRtsTC;
  static bool isUartCtsTC;
  static std::mutex log_mutex;
  static int sleep_timer_state_;
  static int pinctrl_timer_state_;
  timer_t sleep_timer_;
  timer_t pinctrl_timer_;
  static std::condition_variable cv;
  static std::mutex cv_m;
  static void SleepTimerExpiry(union sigval sig);
  static void RTSCTSTimerExpiry(union sigval sig);
};

bool BluetoothTransportTest::simulateRxThreadStuck = false;
bool BluetoothTransportTest::serviceDiedOk = false;
bool BluetoothTransportTest::isUartRtsTC  = false;
bool BluetoothTransportTest::isUartCtsTC  = false;
std::mutex BluetoothTransportTest::log_mutex;
int BluetoothTransportTest::sleep_timer_state_ = TIMER_NOT_CREATED;
int BluetoothTransportTest::pinctrl_timer_state_ = TIMER_NOT_CREATED;
std::condition_variable BluetoothTransportTest::cv;
std::mutex BluetoothTransportTest::cv_m;

// Discard NO-OPs from the event queue.
void BluetoothTransportTest::handle_no_ops() {
  ALOGE("Start of handle_no_ops()");
  ALOGI("%s", __func__);
  while (!event_queue.empty()) {
    std::vector<uint8_t> event;
    event_queue.front(event);
    ASSERT_GE(event.size(),
              static_cast<size_t>(kEventCommandCompleteStatusByte));
    bool event_is_no_op =
        (event[kEventCodeByte] == kEventCommandComplete) &&
        (event[kEventCommandCompleteOpcodeLsByte] == 0x00) &&
        (event[kEventCommandCompleteOpcodeLsByte + 1] == 0x00);
    event_is_no_op |= (event[kEventCodeByte] == kEventCommandStatus) &&
                      (event[kEventCommandStatusOpcodeLsByte] == 0x00) &&
                      (event[kEventCommandStatusOpcodeLsByte + 1] == 0x00);
    if (event_is_no_op) {
      event_queue.pop(event);
    } else {
      break;
    }
  }
  ALOGE("End of handle_no_ops()");
}

// Discard Qualcomm ACL debugging
void BluetoothTransportTest::discard_qca_debugging() {
  ALOGE("Start of discard_qca_debugging()");
  while (!acl_queue.empty()) {
    std::vector<uint8_t> acl_packet;
    acl_queue.front(acl_packet);
    uint16_t connection_handle = acl_packet[1] & 0xF;
    connection_handle <<= 8;
    connection_handle |= acl_packet[0];
    bool packet_is_no_op = connection_handle == kAclHandleQcaDebugMessage;
    if (packet_is_no_op) {
      acl_queue.pop(acl_packet);
    } else {
      break;
    }
  }
  ALOGE("End of discard_qca_debugging()");
}

// Receive an event, discarding NO-OPs.
void BluetoothTransportTest::wait_for_event(bool timeout_is_error = true) {
  ALOGE("Start of wait_for_event()");
  ALOGI("%s", __func__);
  // Wait until we get something that's not a no-op.
  while (true) {
    Wakelock::Acquire();
    bool event_ready = event_queue.waitWithTimeout(kWaitForHciEventTimeout);
    Wakelock::Release();
    ASSERT_TRUE(event_ready || !timeout_is_error);
    if (event_queue.empty()) {
      // waitWithTimeout timed out
      return;
    }
    handle_no_ops();
    if (!event_queue.empty()) {
      // There's an event in the queue that's not a no-op.
      return;
    }
  }
  ALOGE("End of wait_for_event()");
}

// Wait until a command complete is received.
void BluetoothTransportTest::wait_for_command_complete_event(
    std::vector<uint8_t> cmd) {
  ALOGE("Start of wait_for_command_complete_event");
  ALOGI("%s", __func__);
  ASSERT_NO_FATAL_FAILURE(wait_for_event());
  std::vector<uint8_t> event;
  ASSERT_FALSE(event_queue.empty());
  ASSERT_TRUE(event_queue.pop(event));

  ASSERT_GT(event.size(), static_cast<size_t>(kEventCommandCompleteStatusByte));
  ASSERT_EQ(kEventCommandComplete, event[kEventCodeByte]);
  ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
  ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
  ASSERT_EQ(kHciStatusSuccess, event[kEventCommandCompleteStatusByte]);
  ALOGE("End of wait_for_command_complete_event");
}

// Send the command to read the controller's buffer sizes.
void BluetoothTransportTest::setBufferSizes() {
  ALOGE("Start of setBufferSizes");
  std::vector<uint8_t> cmd{
      kCommandHciReadBufferSize,
      kCommandHciReadBufferSize + sizeof(kCommandHciReadBufferSize)};
  std::cout << __func__ << " ";
  ALOGI("%s", __func__);
  sendHCICommand(cmd);
  std::cout << std::endl;

  ASSERT_NO_FATAL_FAILURE(wait_for_event());
  if (event_queue.empty()) {
    return;
  }
  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));

  ASSERT_EQ(kEventCommandComplete, event[kEventCodeByte]);
  ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
  ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
  ASSERT_EQ(kHciStatusSuccess, event[kEventCommandCompleteStatusByte]);

  max_acl_data_packet_length =
      event[kEventCommandCompleteStatusByte + 1] +
      (event[kEventCommandCompleteStatusByte + 2] << 8);
  max_sco_data_packet_length = event[kEventCommandCompleteStatusByte + 3];
  max_acl_data_packets = event[kEventCommandCompleteStatusByte + 4] +
                         (event[kEventCommandCompleteStatusByte + 5] << 8);
  max_sco_data_packets = event[kEventCommandCompleteStatusByte + 6] +
                         (event[kEventCommandCompleteStatusByte + 7] << 8);

  ALOGI("%s: ACL max %d num %d SCO max %d num %d", __func__,
        static_cast<int>(max_acl_data_packet_length),
        static_cast<int>(max_acl_data_packets),
        static_cast<int>(max_sco_data_packet_length),
        static_cast<int>(max_sco_data_packets));
  ALOGE("End of setBufferSizes");
}

// Enable flow control packets for SCO
void BluetoothTransportTest::setSynchronousFlowControlEnable() {
  ALOGE("Start of setSynchronousFlowControlEnable");
  std::vector<uint8_t> cmd{kCommandHciSynchronousFlowControlEnable,
                           kCommandHciSynchronousFlowControlEnable +
                               sizeof(kCommandHciSynchronousFlowControlEnable)};
  hci->sendHciCommand(cmd);

  wait_for_command_complete_event(cmd);
  ALOGE("End of setSynchronousFlowControlEnable");
}

void BluetoothTransportTest::sendHCICommand(std::vector<uint8_t> cmd) {
  ALOGE("Start of sendHCICommand");
  if (logCommands) {
    std::unique_lock<std::mutex> guard(log_mutex);
    std::ostringstream os;
    os << " Command Sent. Length: 0x" << cmd.size() << "  ";
    for(int i = 0; (i < MAX_CMD_LOG) && (i < cmd.size()); i++)
      os << " 0x" << HEX(cmd[i]);
    os << std::endl;
    if (consoleLog)
      std::cout << os.str();
    ALOGI("%s: %s", __func__, os.str().c_str());
  }
  hci->sendHciCommand(cmd).isOk();
  ALOGE("End of sendHCICommand");
}

// Send an HCI command (in Loopback mode) and check the response.
void BluetoothTransportTest::sendAndCheckHci(int num_packets) {
  ALOGE("Start of sendAndCheckHci");
  ThroughputLogger logger = {__func__};
  int command_size = 0;
  for (int n = 0; n < num_packets; n++) {
    // Send an HCI packet
    std::vector<uint8_t> write_name{
        kCommandHciWriteLocalName,
        kCommandHciWriteLocalName + sizeof(kCommandHciWriteLocalName)};
    // With a name
    char new_name[] = "John Jacob Jingleheimer Schmidt ___________________0";
    size_t new_name_length = strlen(new_name);
    for (size_t i = 0; i < new_name_length; i++) {
      write_name.push_back(static_cast<uint8_t>(new_name[i]));
    }
    // And the packet number
    size_t i = new_name_length - 1;
    for (int digits = n; digits > 0; digits = digits / 10, i--) {
      write_name[i] = static_cast<uint8_t>('0' + digits % 10);
    }
    // And padding
    for (size_t i = 0; i < 248 - new_name_length; i++) {
      write_name.push_back(static_cast<uint8_t>(0));
    }

    std::cout << __func__ << ":";
    sendHCICommand(write_name);

    // Check the loopback of the HCI packet
    ASSERT_NO_FATAL_FAILURE(wait_for_event());

    std::vector<uint8_t> event;
    ASSERT_TRUE(event_queue.pop(event));

    size_t compare_length = (write_name.size() > static_cast<size_t>(0xff)
                                 ? static_cast<size_t>(0xff)
                                 : write_name.size());
    ASSERT_GT(event.size(), compare_length + kEventFirstPayloadByte - 1);

    ASSERT_EQ(kEventLoopbackCommand, event[kEventCodeByte]);
    ASSERT_EQ(compare_length, event[kEventLengthByte]);

    // Don't compare past the end of the event.
    if (compare_length + kEventFirstPayloadByte > event.size()) {
      compare_length = event.size() - kEventFirstPayloadByte;
      ALOGE("Only comparing %d bytes", static_cast<int>(compare_length));
    }

    if (n == num_packets - 1) {
      command_size = write_name.size();
    }

    for (size_t i = 0; i < compare_length; i++) {
      ASSERT_EQ(write_name[i], event[kEventFirstPayloadByte + i]);
    }
  }
  logger.setTotalBytes(command_size * num_packets * 2);
  ALOGE("End of sendAndCheckHci");
}

// Send a SCO data packet (in Loopback mode) and check the response.
void BluetoothTransportTest::sendAndCheckSco(int num_packets, size_t size,
                                        uint16_t handle) {
  ALOGE("Start of sendAndCheckSco");
  ThroughputLogger logger = {__func__};
  for (int n = 0; n < num_packets; n++) {
    // Send a SCO packet
    std::vector<uint8_t> sco_packet;
    sco_packet.push_back(static_cast<uint8_t>(handle & 0xff));
    sco_packet.push_back(static_cast<uint8_t>((handle & 0x0f00) >> 8));
    sco_packet.push_back(static_cast<uint8_t>(size & 0xff));
    //sco_packet.push_back(static_cast<uint8_t>((size & 0xff00) >> 8));
    for (size_t i = 0; i < size - 3; i++) {
      sco_packet.push_back(static_cast<uint8_t>(i + n));
    }
    if (logData) {
      std::unique_lock<std::mutex> guard(log_mutex);
      std::cout << __func__ << ": Tx: " << sco_packet.size() << std::endl;
      for(int i = 0; i < sco_packet.size(); i++)
        std::cout << " 0x" << HEX(sco_packet[i]);
      std::cout << std::endl;
    }
    hci->sendScoData(sco_packet);

    // Check the loopback of the SCO packet
    std::vector<uint8_t> sco_loopback;
    ASSERT_TRUE(
        sco_queue.tryPopWithTimeout(sco_loopback, kWaitForScoDataTimeout));

    ASSERT_EQ(sco_packet.size(), sco_loopback.size());
    size_t successful_bytes = 0;

    for (size_t i = 0; i < sco_packet.size(); i++) {
      if (sco_packet[i] == sco_loopback[i]) {
        successful_bytes = i;
      } else {
        ALOGE("Miscompare at %d (expected %x, got %x)", static_cast<int>(i),
              sco_packet[i], sco_loopback[i]);
        ALOGE("At %d (expected %x, got %x)", static_cast<int>(i + 1),
              sco_packet[i + 1], sco_loopback[i + 1]);
        break;
      }
    }
    ASSERT_EQ(sco_packet.size(), successful_bytes + 1);
  }
  logger.setTotalBytes(num_packets * size * 2);
  ALOGE("End of sendAndCheckSco");
}

// Send an ACL data packet (in Loopback mode) and check the response.
void BluetoothTransportTest::sendAndCheckAcl(int num_packets, size_t size,
                                        uint16_t handle) {
  ALOGE("Start of sendAndCheckAcl");
  static int count = 1;
  ThroughputLogger logger = {__func__};
  for (int n = 0; n < num_packets; n++) {
    std::cout << __func__ << ": Data# " << count << std::endl;
    count++;
    // Send an ACL packet
    std::vector<uint8_t> acl_packet;
    acl_packet.push_back(static_cast<uint8_t>(handle & 0xff));
    acl_packet.push_back(static_cast<uint8_t>((handle & 0x0f00) >> 8) |
                         kAclBroadcastPointToPoint |
                         kAclPacketBoundaryFirstAutoFlushable);
    acl_packet.push_back(static_cast<uint8_t>(size & 0xff));
    acl_packet.push_back(static_cast<uint8_t>((size & 0xff00) >> 8));
    for (size_t i = 0; i < size; i++) {
      acl_packet.push_back(static_cast<uint8_t>(i + n));
    }
    if (logData) {
      std::unique_lock<std::mutex> guard(log_mutex);
      std::cout << __func__ << ": Tx: " << acl_packet.size() << std::endl;
      for(int i = 0; i < acl_packet.size(); i++)
        std::cout << " 0x" << HEX(acl_packet[i]);
      std::cout << std::endl;
    }
    hci->sendAclData(acl_packet);

    std::vector<uint8_t> acl_loopback;
    // Check the loopback of the ACL packet
    ASSERT_TRUE(
        acl_queue.tryPopWithTimeout(acl_loopback, kWaitForAclDataTimeout));

    ASSERT_EQ(acl_packet.size(), acl_loopback.size());
    size_t successful_bytes = 0;

    for (size_t i = 0; i < acl_packet.size(); i++) {
      if (acl_packet[i] == acl_loopback[i]) {
        successful_bytes = i;
      } else {
        ALOGE("Miscompare at %d (expected %x, got %x)", static_cast<int>(i),
              acl_packet[i], acl_loopback[i]);
        ALOGE("At %d (expected %x, got %x)", static_cast<int>(i + 1),
              acl_packet[i + 1], acl_loopback[i + 1]);
        break;
      }
    }
    ASSERT_EQ(acl_packet.size(), successful_bytes + 1);
  }
  logger.setTotalBytes(num_packets * size * 2);
  ALOGE("End of sendAndCheckAcl");
}

// Send a iso data packet (in Loopback mode) and check the response.
void BluetoothTransportTest::sendAndCheckISO(int num_packets, size_t size,
                                        uint16_t handle) {
  ALOGE("Start of sendAndCheckISO");
  if (hci != nullptr)
  {
    ALOGI("entered VTS");
    ThroughputLogger logger = {__func__};
    for (int n = 0; n < num_packets; n++) {
    // Send a iso packet
      std::vector<uint8_t> iso_packet;
      std::vector<uint8_t> iso_vector;
      iso_vector.push_back(static_cast<uint8_t>(handle & 0xff));
      iso_vector.push_back(static_cast<uint8_t>((handle & 0x0f00) >> 8));
      iso_vector.push_back(static_cast<uint8_t>(size & 0xff));
      iso_vector.push_back(static_cast<uint8_t>((size & 0xff00) >> 8));
      for (size_t i = 0; i < size; i++) {
        iso_vector.push_back(static_cast<uint8_t>(i + n));
      }
      iso_packet = iso_vector;
      if (logData) {
        std::unique_lock<std::mutex> guard(log_mutex);
        std::cout << __func__ << ": Tx: " << iso_vector.size() << std::endl;
        for(int i = 0; i < iso_vector.size(); i++)
          std::cout << " 0x" << HEX(iso_vector[i]);
          std::cout << std::endl;
    }
    hci->sendIsoData(iso_vector);
    }
    ALOGE("End of sendAndCheckISO");
  } else {
    ALOGE("ISO is not suported in V1_0");
  }
}

// Return the number of completed packets reported by the controller.
int BluetoothTransportTest::wait_for_completed_packets_event(uint16_t handle) {
  ALOGE("Start of wait_for_completed_packets_event");
  int packets_processed = 0;
  wait_for_event(false);
  if (event_queue.empty()) {
    ALOGI("%s: waitForBluetoothCallback timed out.", __func__);
    return packets_processed;
  }
  while (!event_queue.empty()) {
    std::vector<uint8_t> event;
    EXPECT_TRUE(event_queue.pop(event));

    EXPECT_EQ(kEventNumberOfCompletedPackets, event[kEventCodeByte]);
    EXPECT_EQ(1, event[kEventNumberOfCompletedPacketsNumHandles]);

    uint16_t event_handle = event[3] + (event[4] << 8);
    EXPECT_EQ(handle, event_handle);

    packets_processed += event[5] + (event[6] << 8);
  }
  ALOGE("End of wait_for_completed_packets_event");
  return packets_processed;
}

// Send the reset command and wait for a response.
void BluetoothTransportTest::reset() {
  ALOGE("Start of reset");
  std::vector<uint8_t> reset{kCommandHciReset,
                             kCommandHciReset + sizeof(kCommandHciReset)};
  sendHCICommand(reset);

  wait_for_command_complete_event(reset);
  ALOGE("End of reset");
}

// Send local loopback command and initialize SCO and ACL handles.
void BluetoothTransportTest::enterLoopbackMode() {
  ALOGE("Start of enterLoopbackMode");
  std::vector<uint8_t> cmd{kCommandHciWriteLoopbackModeLocal,
                           kCommandHciWriteLoopbackModeLocal +
                               sizeof(kCommandHciWriteLoopbackModeLocal)};
  std::cout << __func__ << " ";
  ALOGI("%s", __func__);
  sendHCICommand(cmd);
  std::cout << std::endl;

  // Receive connection complete events with data channels
  int connection_event_count = 0;
  bool command_complete_received = false;
  while (true) {
    wait_for_event(false);
    if (event_queue.empty()) {
      // Fail if there was no event received or no connections completed.
      ASSERT_TRUE(command_complete_received);
      ASSERT_LT(0, connection_event_count);
      return;
    }
    std::vector<uint8_t> event;
    ASSERT_TRUE(event_queue.pop(event));
    ASSERT_GT(event.size(),
              static_cast<size_t>(kEventCommandCompleteStatusByte));
    if (event[kEventCodeByte] == kEventConnectionComplete) {
      ASSERT_GT(event.size(),
                static_cast<size_t>(kEventConnectionCompleteType));
      ASSERT_EQ(event[kEventLengthByte], kEventConnectionCompleteParamLength);
      uint8_t connection_type = event[kEventConnectionCompleteType];

      ASSERT_TRUE(connection_type == kEventConnectionCompleteTypeSco ||
                  connection_type == kEventConnectionCompleteTypeAcl);

      // Save handles
      uint16_t handle = event[kEventConnectionCompleteHandleLsByte] |
                        event[kEventConnectionCompleteHandleLsByte + 1] << 8;
      if (connection_type == kEventConnectionCompleteTypeSco) {
        sco_connection_handles.push_back(handle);
      } else {
        acl_connection_handles.push_back(handle);
      }

      ALOGI("Connect complete type = %d handle = %d",
            event[kEventConnectionCompleteType], handle);
      connection_event_count++;
    } else {
      ASSERT_EQ(kEventCommandComplete, event[kEventCodeByte]);
      ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
      ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
      ASSERT_EQ(kHciStatusSuccess, event[kEventCommandCompleteStatusByte]);
      command_complete_received = true;
    }
  }
  ALOGE("End of enterLoopbackMode");
}

 void BluetoothTransportTest::SleepTimerExpiry(union sigval sig) {
   ALOGE("Start of SleepTimerExpiry");
   BluetoothTransportTest *btTransportTest = static_cast<BluetoothTransportTest*>(sig.sival_ptr);
   ALOGI("%s", __func__);
   if (btTransportTest) {
     {
      std::unique_lock<std::mutex> lk(BluetoothTransportTest::cv_m);
      timerExpired = true;
    }
    // signal the condition variable
    cv.notify_one();
  }
  ALOGE("End of SleepTimerExpiry");
}

bool BluetoothTransportTest::IsSIBSEnabled() {
  ALOGE("Start of IsSIBSEnabled");
  char value[PROPERTY_VALUE_MAX] = { '\0' };
  ALOGI("%s", __func__);
  property_get("persist.vendor.service.bdroid.sibs", value, NULL);
  ALOGE("End of IsSIBSEnabled");
  return (strcmp(value, "true") == 0) ? true : false;
}

void BluetoothTransportTest::SendCmdToUARTFlowON() {
  ALOGE("Start of SendCmdToUARTFlowON");
  std::vector<uint8_t> cmd = COMMAND_RESET_UART_FLOW_ON;
  ALOGI("%s", __func__);
  sendHCICommand(cmd);
  std::cout << std::endl;
  ALOGE("End of SendCmdToUARTFlowON");
}

void BluetoothTransportTest::GetCTSStatus() {
  ALOGE("Start of GetCTSStatus");
  ALOGI("%s", __func__);
  sendHCICommand(COMMAND_GET_CTS_STATUS);
  ALOGE("End of GetCTSStatus");
}

void BluetoothTransportTest::RTSCTSTimerExpiry(union sigval sig) {
  ALOGE("Start of RTSCTSTimerExpiry");
  BluetoothTransportTest *btTransportTest = static_cast<BluetoothTransportTest*>(sig.sival_ptr);
  ALOGI("%s", __func__);
  if (btTransportTest) {
    if (isUartRtsTC) {
       std::vector<uint8_t> cmd = COMMAND_RESET_UART_FLOW_ON;
       btTransportTest->SendCmdToUARTFlowON();
       btTransportTest->StopRTSCTSStatusTimer();
       isUartRtsTC = false;
     } else if (isUartCtsTC) {
       btTransportTest->GetCTSStatus();
       btTransportTest->StopRTSCTSStatusTimer();
       isUartCtsTC = false;
     }
  }
  ALOGE("End of RTSCTSTimerExpiry");
}

void BluetoothTransportTest::StartSleepTimer(int sleepTime) {
  ALOGE("Start of StartSleepTimer");
  int status;
  struct itimerspec ts;
  struct sigevent se;
  uint32_t timeout_ms;
  timerExpired = false;

  ALOGV("%s", __func__);

  if (sleep_timer_state_ == TIMER_NOT_CREATED) {
    se.sigev_notify_function = (void (*)(union sigval))SleepTimerExpiry;
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = this;
    se.sigev_notify_attributes = NULL;

    status = timer_create(CLOCK_BOOTTIME_ALARM, &se, &sleep_timer_);
     if (status == 0) {
       ALOGI("%s: Sleep timer created", __func__);
       sleep_timer_state_ = TIMER_CREATED;
     } else {
       ALOGE("%s: Error creating Sleep timer %d\n", __func__, status);
     }
   }

   if (sleep_timer_state_ == TIMER_CREATED || sleep_timer_state_ == TIMER_ACTIVE) {
     timeout_ms = sleepTime;
     ts.it_value.tv_sec = timeout_ms / 1000;
     ts.it_value.tv_nsec = 1000000 * (timeout_ms % 1000);
     ts.it_interval.tv_sec = 0;
     ts.it_interval.tv_nsec = 0;
     status = timer_settime(sleep_timer_, 0, &ts, 0);
     if (status < 0)
       ALOGE("%s:Failed to set Sleep timer: %d", __func__, status);
     else {
       ALOGI("%s: timer started", __func__);
       sleep_timer_state_ = TIMER_ACTIVE;
    }
  }
  ALOGE("End of StartSleepTimer");
}

bool BluetoothTransportTest::StopSleepTimer() {
   ALOGE("Start of StopSleepTimer");
   int status;
   struct itimerspec ts;
   bool was_active = false;

   ALOGI("%s", __func__);

   ts.it_value.tv_sec = 0;
   ts.it_value.tv_nsec = 0;
   ts.it_interval.tv_sec = 0;
   ts.it_interval.tv_nsec = 0;
   was_active = true;

   status = timer_settime(sleep_timer_, 0, &ts, 0);
   if (status == -1)
     ALOGE("%s:Failed to stop sleep timer", __func__);
   else if (status == 0) {
     ALOGV("%s: Sleep timer Stopped", __func__);
     sleep_timer_state_ = TIMER_CREATED;
     if (timer_delete(sleep_timer_) == 0) {
       ALOGV("%s: Sleep timer Deleted", __func__);
       sleep_timer_state_ = TIMER_NOT_CREATED;
     }
   }
   ALOGE("End of StopSleepTimer");
  return was_active;
}

void BluetoothTransportTest::StartGetRTSCTSStatusTimer(int sleepTime) {
  ALOGE("Start of StartGetRTSCTSStatusTimer");
  int status;
  struct itimerspec ts;
  struct sigevent se;
  uint32_t timeout_ms;
  ALOGI("%s", __func__);

  if (pinctrl_timer_state_ == TIMER_NOT_CREATED) {
    se.sigev_notify_function = (void (*)(union sigval))RTSCTSTimerExpiry;
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = this;
    se.sigev_notify_attributes = NULL;

    status = timer_create(CLOCK_BOOTTIME_ALARM, &se, &pinctrl_timer_);
    if (status == 0) {
      ALOGI("%s: StartGetRTSCTSStatusTimer timer created", __func__);
      pinctrl_timer_state_ = TIMER_CREATED;
    } else {
      ALOGE("%s: Error creating pinctrl timer %d\n", __func__, status);
    }
  }

  if (pinctrl_timer_state_ == TIMER_CREATED || pinctrl_timer_state_ == TIMER_ACTIVE) {
    timeout_ms = sleepTime;
    ts.it_value.tv_sec = timeout_ms / 1000;
    ts.it_value.tv_nsec = 1000000 * (timeout_ms % 1000);
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    status = timer_settime(pinctrl_timer_, 0, &ts, 0);
    if (status < 0) {
      ALOGE("%s:Failed to set StartGetRTSCTSStatusTimer timer: %d", __func__, status);
    } else {
      ALOGI("%s: timer started", __func__);
      pinctrl_timer_state_ = TIMER_ACTIVE;
    }
  }
  ALOGE("End of StartGetRTSCTSStatusTimer");
}

bool BluetoothTransportTest::StopRTSCTSStatusTimer() {
   ALOGE("Start of StopRTSCTSStatusTimer");
   int status;
   struct itimerspec ts;
   bool was_active = false;

   ALOGI("%s", __func__);

   if ((pinctrl_timer_state_ == TIMER_ACTIVE) || (pinctrl_timer_state_ == TIMER_CREATED)) {
     ts.it_value.tv_sec = 0;
     ts.it_value.tv_nsec = 0;
     ts.it_interval.tv_sec = 0;
     ts.it_interval.tv_nsec = 0;
     was_active = true;
     status = timer_settime(pinctrl_timer_, 0, &ts, 0);

     if (status == -1)
       ALOGE("%s:Failed to stop pinctrl tc timer", __func__);
     else if (status == 0) {
       ALOGV("%s: Pinctrl TC timer Stopped", __func__);
       pinctrl_timer_state_ = TIMER_CREATED;
       if (timer_delete(pinctrl_timer_) == 0) {
          pinctrl_timer_state_ = TIMER_NOT_CREATED;
          ALOGV("%s: Pinctrl TC timer Deleted", __func__);
       }
     }
   }
   ALOGE("End of StopRTSCTSStatusTimer");
   return was_active;
}

// Empty test: Initialize()/Close() are called in SetUp()/TearDown().
TEST_F(BluetoothTransportTest, InitializeAndClose) {
  ALOGE("===========Testcase: InitializationAndClose===============");
  std::cout << "====-=====Testcase: InitializationAndClose============" << std::endl;
}

// Send an HCI Reset with sendHciCommand and wait for a command complete event.
TEST_F(BluetoothTransportTest, HciReset) {
  ALOGE("===================Testcase: HciReset====================");
  std::cout << "====================Testcase: HciReset====================" << std::endl;
  reset();
  ALOGE("===================End of Testcase: HciReset====================");
}

TEST_F(BluetoothTransportTest, RTSPinCtrlTest) {
  ALOGE("===================Testcase: RTSPinCtrlTest====================");
  std::cout << "===================Testcase: RTSPinCtrlTest===================" << std::endl;
  isUartRtsTC = true;
  std::vector<uint8_t> cmd = COMMAND_GET_SOC_CTS_STATUS;
  sendHCICommand(cmd);
  StartGetRTSCTSStatusTimer(pinctrlTimerValue);
  wait_for_event(true);

  if (event_queue.size() == 0)  return;

  std::vector<uint8_t> event;
  event_queue.pop(event);
  std::cout << "Event received:";
  for (int i = 0; i < event.size(); i++) {
     ALOGI("event[%d] is %x", i, event[i]);
     std::cout << " 0x" << HEX(event[i]);
  }
  std::cout << std::endl;
  if (event[event.size()-1] == 0x01) std::cout << "====[PASSED]==== TC passed" << std::endl;
  else std::cout << "====[FAILED]==== TC failed" << std::endl;
  ALOGE("===================End of Testcase: RTSPinCtrlTest====================");
}

TEST_F(BluetoothTransportTest, CTSPinCtrlTest) {
  ALOGE("===================Testcase: CTSPinCtrlTest====================");
  std::cout << "===================Testcase: CTSPinCtrlTest===================" << std::endl;
  ALOGI("value of pinctrlTimerValue is %d", pinctrlTimerValue);
  if (pinctrlTimerValue <= 500 && pinctrlTimerValue >= 10) {
    std::cout << "Valid timeout range 10ms <= timeout value <= 500ms" << std::endl;
  } else {
    std::cout << "Timeout not in valid range, assigning default 100 ms" << std::endl;
    pinctrlTimerValue = 100;
  }
  StopRTSCTSStatusTimer();
  std::vector<uint8_t> cmd = COMMAND_GET_SOC_RTS_STATUS;
  isUartCtsTC = true;
  cmd.push_back(static_cast<uint8_t>(pinctrlTimerValue));
  cmd.push_back(static_cast<uint8_t>(0));
  sendHCICommand(cmd);

  wait_for_event(true);
  if (event_queue.size() == 0) {
     return;
   } else {
     std::vector<uint8_t> event1;
     event_queue.pop(event1);
     ALOGI("event received , now send COMMAND_GET_CTS_STATUS to SOC set RTS");
     sendHCICommand(COMMAND_GET_CTS_STATUS);
     ALOGI("wait_for_event of COMMAND_GET_CTS_STATUS");
     wait_for_event(true);

     if (event_queue.size() == 0) return;
     std::vector<uint8_t> event;
     event_queue.pop(event);
     std::cout << "Event received:";
     for (int i = 0; i < event.size(); i++) {
        ALOGI("event[%d] is %x", i, event[i]);
        std::cout << " 0x" << HEX(event[i]);
     }
     std::cout << std::endl;
     if (event[event.size()-1] == 0x01) std::cout << "====[PASSED]====:SOC side RTS was set"
                                                  << std::endl;
     else std::cout << "====[FAILED]====: Failed to set SOC side RTS" << std::endl;

     ALOGI("Sending COMMAND_GET_CTS_STATUS to check back SOC released RTS or not");
     StartGetRTSCTSStatusTimer(pinctrlTimerValue);
     wait_for_event(true);

     if (event_queue.size() == 0) return;
     event_queue.pop(event);
     std::cout << "Event received:";
     for (int i = 0; i < event.size(); i++) {
        ALOGI("event[%d] is %x", i, event[i]);
        std::cout << " 0x" << HEX(event[i]);
    }
    std::cout << std::endl;
    if (event[event.size()-1] == 0x00) std::cout << "====[PASSED]====:SOC side RTS released now"
                                                 << std::endl;
    else std::cout << "====[FAILED]====: Failed to release SOC side RTS" << std::endl;
  }
  ALOGE("===================End of Testcase: CTSPinCtrlTest====================");
}

// Read and check the HCI version of the controller.
TEST_F(BluetoothTransportTest, HciVersionTest) {
  ALOGE("====================Testcase: HciVersionTest====================");
  std::cout << "====================Testcase: HciVersionTest====================" << std::endl;
  reset();
  std::vector<uint8_t> cmd{kCommandHciReadLocalVersionInformation,
                           kCommandHciReadLocalVersionInformation +
                               sizeof(kCommandHciReadLocalVersionInformation)};
  sendHCICommand(cmd);

  ASSERT_NO_FATAL_FAILURE(wait_for_event());

  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));
  ASSERT_GT(event.size(), static_cast<size_t>(kEventLocalLmpVersionByte));

  ASSERT_EQ(kEventCommandComplete, event[kEventCodeByte]);
  ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
  ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
  ASSERT_EQ(kHciStatusSuccess, event[kEventCommandCompleteStatusByte]);

  ASSERT_LE(kHciMinimumHciVersion, event[kEventLocalHciVersionByte]);
  ASSERT_LE(kHciMinimumLmpVersion, event[kEventLocalLmpVersionByte]);
  ALOGE("====================End of Testcase: HciVersionTest====================");
}

// Enter loopback mode, but don't send any packets.
TEST_F(BluetoothTransportTest, WriteLoopbackMode) {
  reset();
  ALOGE("====================Testcase: WriteLoopbackMode====================");
  std::cout << "====================Testcase: WriteLoopbackMode====================" << std::endl;
  enterLoopbackMode();
  ALOGE("====================End of Testcase: WriteLoopbackMode====================");
}

TEST_F(BluetoothTransportTest, RandomHostCommands) {
  ALOGE("====================Testcase: RandomHostCommands====================");
  std::cout << "====================Testcase: RandomHostCommands====================" << std::endl;
  int sleepMs = 0;
  int maxMsgs = 0;
  std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point end_time_;
  std::chrono::duration<double> duration(0.0);
  std::vector<uint8_t> cmd = COMMAND_HCI_READ_BUFFER_SIZE;
  int count = 1;

  for(int i = 0; i < maxIterations; i++) {
    ALOGI("RandomHostCommands - Iteration: %d", i + 1);
    sleepMs = rand() % maxAppSleepTimer;
    if (sleepMs <  minAppSleepTimer)
      sleepMs = minAppSleepTimer;
    {
       StartSleepTimer(sleepMs);
       std::unique_lock<std::mutex> lk(BluetoothTransportTest::cv_m);
       BluetoothTransportTest::cv.wait(lk);
       ALOGI("RandomHostCommands: SleepTimer Expired");
     }
     end_time_ = std::chrono::steady_clock::now();
     duration = end_time_ - start_time_;
     start_time_ = end_time_;
     maxMsgs = rand() % maxMsgsPerIteration;
     if (maxMsgs < 1) maxMsgs = 1;
     std::cout << "Iteration:" << (i + 1) << " Duration:"
               << duration.count() << " SleepMs: " << sleepMs << " Msgs: "
               << maxMsgs << std::endl;
     ALOGI("RandomHostCommands: Iteration:%d, sleepMs: %d, Msgs:%d, Duration:%f",
           (i + 1), sleepMs, maxMsgs, duration.count());
     for(int j = 0; j < maxMsgs; j++) {
       std::cout << "HostCommand# " << count << std::endl;
       count++;
       sendHCICommand(cmd);
       wait_for_command_complete_event(cmd);
     }
   }
   StopSleepTimer();
   ALOGE("====================End of Testcase: RandomHostCommands====================");
}

TEST_F(BluetoothTransportTest, RandomHostData) {
  ALOGE("====================Testcase: RandomHostData====================");
  std::cout << "====================Testcase: RandomHostData====================" << std::endl;
  setBufferSizes();
  enterLoopbackMode();

  int sleepMs = 0;
  int maxMsgs = 0;
  sendAndCheckHci(1);
  std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point end_time_;
  std::chrono::duration<double> duration;

  for(int i = 0; i < maxIterations; i++) {
    ALOGI("RandomHostData - Iteration: %d", i + 1);
    sleepMs = rand() % maxAppSleepTimer;
    if (sleepMs <  minAppSleepTimer)
      sleepMs = minAppSleepTimer;
    {
      StartSleepTimer(sleepMs);
      std::unique_lock<std::mutex> lk(BluetoothTransportTest::cv_m);
      BluetoothTransportTest::cv.wait(lk);
      ALOGI("RandomHostData: SleepTimer Expired");
    }
    end_time_ = std::chrono::steady_clock::now();
    duration = end_time_ - start_time_;
    start_time_ = end_time_;
    if (acl_connection_handles.size() > 0) {
      maxMsgs = rand() % maxMsgsPerIteration;
      // ensure atleast one message is sent
      if (maxMsgs == 0) maxMsgs = 1;
      std::cout << "Iteration:" << (i + 1) << " Duration:"
                << duration.count() << " SleepMs: " << sleepMs << " Msgs: "
                << maxMsgs << std::endl;
      ALOGI("RandomHostCommands: Iteration:%d, sleepMs: %d, Msgs:%d, Duration:%f",
          (i + 1), sleepMs, maxMsgs, duration.count());
      EXPECT_LT(0, max_acl_data_packet_length);
      sendAndCheckAcl(maxMsgs, max_acl_data_packet_length, acl_connection_handles[0]);
      int acl_packets_sent = maxMsgs;
      int completed_packets =
        wait_for_completed_packets_event(acl_connection_handles[0]);
      if (acl_packets_sent != completed_packets) {
        ALOGI("%s: packets_sent (%d) != completed_packets (%d)", __func__,
            acl_packets_sent, completed_packets);
      }
    }
  }
  StopSleepTimer();
  ALOGE("====================End of Testcase: RandomHostData====================");
}

TEST_F(BluetoothTransportTest, BtOnOff) {
  ALOGE("====================Testcase: BtOnOff====================");
  std::cout << "====================Testcase: BtOnOff====================" << std::endl;
  TearDown();
  for(int i = 0; i < maxIterations; i++) {
    ALOGI("%s - Iteration: %d", __func__, i + 1);
    std::cout << "Iteration:" << i << std::endl;
    SetUp();
    TearDown();
  }
  SetUp();
  ALOGE("====================End of Testcase: BtOnOff====================");
}

TEST_F(BluetoothTransportTest, SsrStackInitiated) {
  ALOGE("====================Testcase: SsrStackInitiated====================");
  std::cout << "====================Testcase: SsrStackInitiated====================" << std::endl;
  for(int i = 0; i < maxIterations; i++) {
    std::cout << "Iteration:" << (i + 1) << std::endl;
    ALOGI("SsrStackInitiated - Iteration: %d", i + 1);
    std::vector<uint8_t> cmd = COMMAND_HCI_GET_DEBUG_INFO;
    serviceDiedOk = true;
    sendHCICommand(cmd);
    wait_for_event(false);
    {
      // Acquire Wakelock as system may go to sleep and serviceDied() may not get called
      Wakelock::Acquire();
      StartSleepTimer(WAIT_FOR_SSR);
      std::unique_lock<std::mutex> lk(BluetoothTransportTest::cv_m);
      BluetoothTransportTest::cv.wait(lk);
      ALOGI("SsrStackInitiated: SleepTimer Expired");
      Wakelock::Release();
    }
    serviceDiedOk = false;
    std::vector<uint8_t> event;
    while (event_queue.size())
      event_queue.pop(event);

    StopSleepTimer();
    SetUp(); // Turn BT On
  }
  ALOGE("====================End of Testcase: SsrStackInitiated====================");
}

TEST_F(BluetoothTransportTest, RxThreadStuck) {
  ALOGE("====================Testcase: RxThreadStuck====================");
  std::cout << "====================Testcase: RxThreadStuck====================" << std::endl;
  for(int i = 0; i < maxIterations; i++) {
    ALOGI("RxThreaddStuck - Iteration: %d", i + 1);
    std::cout << "Iteration:" << (i + 1) << std::endl;
   std::vector<uint8_t> cmd = COMMAND_HCI_READ_BUFFER_SIZE;
   serviceDiedOk = true;
   sendHCICommand(cmd);
   wait_for_event(false);
   {
     StartSleepTimer(WAIT_FOR_SSR);
     std::unique_lock<std::mutex> lk(BluetoothTransportTest::cv_m);
     BluetoothTransportTest::cv.wait(lk);
     ALOGI("RxThreadStuck: SleepTimer Expired");
   }
   serviceDiedOk = false;
   simulateRxThreadStuck = false;
   std::vector<uint8_t> event;
   while (event_queue.size())
     event_queue.pop(event);
   ASSERT_TRUE(hci->close().isOk());
   StopSleepTimer();
   SetUp(); // Turn BT On
  }
  ALOGE("====================ENd of Testcase: RxThreadStuck====================");
}

// Send an unknown HCI command and wait for the error message.
TEST_F(BluetoothTransportTest, HciUnknownCommand) {
  ALOGE("====================Testcase: HciUnknownCommand====================");
  std::cout << "====================Testcase: HciUnknownCommand====================" << std::endl;
  reset();
  std::vector<uint8_t> cmd{
      kCommandHciShouldBeUnknown,
      kCommandHciShouldBeUnknown + sizeof(kCommandHciShouldBeUnknown)};
  sendHCICommand(cmd);

  ASSERT_NO_FATAL_FAILURE(wait_for_event());

  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));

  ASSERT_GT(event.size(), static_cast<size_t>(kEventCommandCompleteStatusByte));
  if (event[kEventCodeByte] == kEventCommandComplete) {
    ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
    ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
    ASSERT_EQ(kHciStatusUnknownHciCommand,
              event[kEventCommandCompleteStatusByte]);
  } else {
    ASSERT_EQ(kEventCommandStatus, event[kEventCodeByte]);
    ASSERT_EQ(cmd[0], event[kEventCommandStatusOpcodeLsByte]);
    ASSERT_EQ(cmd[1], event[kEventCommandStatusOpcodeLsByte + 1]);
    ASSERT_EQ(kHciStatusUnknownHciCommand,
              event[kEventCommandStatusStatusByte]);
  }
  ALOGE("====================End of Testcase: HciUnknownCommand====================");
}

TEST_F(BluetoothTransportTest, SsrSwErrFatal) {
  ALOGE("====================Testcase: SsrSwErrFatal====================");
  std::cout << "====================Testcase: SsrSwErrFatal====================" << std::endl;
  for(int i = 0; i < maxIterations; i++) {
    std::cout << "Iteration:" << (i + 1) << std::endl;
    ALOGI("SsrSwErrFatal: Iteration:%d", (i + 1));
    std::vector<uint8_t> cmd = SSR_SW_ERROR_FATAL;
    serviceDiedOk = true;
    sendHCICommand(cmd);
    wait_for_event(false);
    {
      // Acquire Wakelock as system may go to sleep and serviceDied() may not get called
      Wakelock::Acquire();
      StartSleepTimer(WAIT_FOR_SSR);
      std::unique_lock<std::mutex> lk(BluetoothTransportTest::cv_m);
      BluetoothTransportTest::cv.wait(lk);
      ALOGI("SsrSwErrFatal: SleepTimer Expired");
      Wakelock::Release();
    }
    serviceDiedOk = false;
    std::vector<uint8_t> event;
    while (event_queue.size())
      event_queue.pop(event);

    StopSleepTimer();
    SetUp(); // Turn BT On
  }
  ALOGE("====================End of Testcase: SsrSwErrFatal====================");
}

TEST_F(BluetoothTransportTest, SsrSwExceptionDivBy0) {
  ALOGE("====================Testcase: SsrSwExceptionDivBy0====================");
  std::cout << "====================Testcase: SsrSwExcpetionDivBy0====================" << std::endl;
  for(int i = 0; i < maxIterations; i++) {
    std::cout << "Iteration:" << (i + 1) << std::endl;
    ALOGI("SsrSwExceptionDivBy0: Iteration:%d", (i + 1));
    std::vector<uint8_t> cmd = SSR_SW_EXCEPTION_DIV_BY_0;
   serviceDiedOk = true;
   sendHCICommand(cmd);
   wait_for_event(false);
   {
     // Acquire Wakelock as system may go to sleep and serviceDied() may not get called
     Wakelock::Acquire();
     StartSleepTimer(WAIT_FOR_SSR);
     std::unique_lock<std::mutex> lk(BluetoothTransportTest::cv_m);
     BluetoothTransportTest::cv.wait(lk);
     ALOGI("SsrSwExceptionDivBy0: SleepTimer Expired");
     Wakelock::Release();
   }
   serviceDiedOk = false;
   std::vector<uint8_t> event;
   while (event_queue.size())
     event_queue.pop(event);

   StopSleepTimer();
   SetUp(); // Turn BT On
  }
  ALOGE("====================End of Testcase: SsrSwExceptionDivBy0====================");
}

TEST_F(BluetoothTransportTest, SsrSwExceptionNullPtr) {
  ALOGE("====================Testcase: SsrSwExceptionNullPtr====================");
  std::cout << "====================Testcase: SsrSwExceptionNullPtr====================" << std::endl;
  for(int i = 0; i < maxIterations; i++) {
    std::cout << "Iteration:" << (i + 1) << std::endl;
    ALOGI("SsrSwExceptionNullPtr: Iteration:%d", (i + 1));
    std::vector<uint8_t> cmd = SSR_SW_EXCEPTION_NULL_PTR;
    serviceDiedOk = true;
    sendHCICommand(cmd);
    wait_for_event(false);
    {
      // Acquire Wakelock as system may go to sleep and serviceDied() may not get called
      Wakelock::Acquire();
      StartSleepTimer(WAIT_FOR_SSR);
      std::unique_lock<std::mutex> lk(BluetoothTransportTest::cv_m);
      BluetoothTransportTest::cv.wait(lk);
      ALOGI("SsrSwExceptionNullPtr: SleepTimer Expired");
      Wakelock::Release();
    }
    serviceDiedOk = false;
    std::vector<uint8_t> event;
    while (event_queue.size())
      event_queue.pop(event);

    StopSleepTimer();
    SetUp(); // Turn BT On
  }
  ALOGE("====================ENd of Testcase: SsrSwExceptionNullPtr====================");
}

TEST_F(BluetoothTransportTest, SsrWatchDogBits) {
 ALOGE("====================Testcase: SsrWatchDogBits====================");
 std::cout << "====================Testcase: SsrWatchDogBits====================" << std::endl;
 for(int i = 0; i < maxIterations; i++) {
   std::cout << "Iteration:" << (i + 1) << std::endl;
   ALOGI("SsrWatchDogBits: Iteration:%d", (i + 1));
   std::vector<uint8_t> cmd = SSR_WATCHDOG_BITS;
   serviceDiedOk = true;
   sendHCICommand(cmd);
    wait_for_event(false);
    {
      // Acquire Wakelock as system may go to sleep and serviceDied() may not get called
      Wakelock::Acquire();
      StartSleepTimer(WAIT_FOR_SSR);
      std::unique_lock<std::mutex> lk(BluetoothTransportTest::cv_m);
      BluetoothTransportTest::cv.wait(lk);
      ALOGI("SsrWatchDogBits: SleepTimer Expired");
      Wakelock::Release();
    }
    serviceDiedOk = false;
    std::vector<uint8_t> event;
    while (event_queue.size())
      event_queue.pop(event);

    StopSleepTimer();
    SetUp(); // Turn BT On
  }
  ALOGE("====================End of Testcase: SsrWatchDogBits====================");
}




// Enter loopback mode and send a single command.
TEST_F(BluetoothTransportTest, LoopbackModeSingleCommand) {
  ALOGE("Start of LoopbackModeSingleCommand");
  reset();
  setBufferSizes();

  enterLoopbackMode();

  sendAndCheckHci(1);
  ALOGE("End of LoopbackModeSingleCommand");
}

// Enter loopback mode and send a single SCO packet.
TEST_F(BluetoothTransportTest, LoopbackModeSingleSco) {
  ALOGE("Start of LoopbackModeSingleSco");
  reset();
  setBufferSizes();
  setSynchronousFlowControlEnable();

  enterLoopbackMode();

  if (!sco_connection_handles.empty()) {
    ASSERT_LT(0, max_sco_data_packet_length);
    sendAndCheckSco(1, max_sco_data_packet_length, sco_connection_handles[0]);
    int sco_packets_sent = 1;
    int completed_packets =
        wait_for_completed_packets_event(sco_connection_handles[0]);
    if (sco_packets_sent != completed_packets) {
      ALOGW("%s: packets_sent (%d) != completed_packets (%d)", __func__,
            sco_packets_sent, completed_packets);
    }
  }
  ALOGE("End of LoopbackModeSingleSco");
}

// Enter loopback mode and send a single ACL packet.
TEST_F(BluetoothTransportTest, LoopbackModeSingleAcl) {
  ALOGE("Start of LoopbackModeSingleAcl");
  reset();
  setBufferSizes();

  enterLoopbackMode();

  if (!acl_connection_handles.empty()) {
    ASSERT_LT(0, max_acl_data_packet_length);
    sendAndCheckAcl(1, max_acl_data_packet_length - 1,
                    acl_connection_handles[0]);
    int acl_packets_sent = 1;
    int completed_packets =
        wait_for_completed_packets_event(acl_connection_handles[0]);
    if (acl_packets_sent != completed_packets) {
      ALOGW("%s: packets_sent (%d) != completed_packets (%d)", __func__,
            acl_packets_sent, completed_packets);
    }
  }
  ASSERT_GE(acl_cb_count, 1);
  ALOGE("End of LoopbackModeSingleAcl");
}

// Enter loopback mode and send command packets for bandwidth measurements.
TEST_F(BluetoothTransportTest, LoopbackModeCommandBandwidth) {
  ALOGE("Start of LoopbackModeCommandBandwidth");
  reset();
  setBufferSizes();

  enterLoopbackMode();

  sendAndCheckHci(kNumHciCommandsBandwidth);
  ALOGE("End of LoopbackModeCommandBandwidth");
}

// Enter loopback mode and send SCO packets for bandwidth measurements.
TEST_F(BluetoothTransportTest, LoopbackModeScoBandwidth) {
  ALOGE("Start of LoopbackModeScoBandwidth");
  reset();
  setBufferSizes();
  setSynchronousFlowControlEnable();

  enterLoopbackMode();

  if (!sco_connection_handles.empty()) {
    ASSERT_LT(0, max_sco_data_packet_length);
    sendAndCheckSco(kNumScoPacketsBandwidth, max_sco_data_packet_length,
                    sco_connection_handles[0]);
    int sco_packets_sent = kNumScoPacketsBandwidth;
    int completed_packets =
        wait_for_completed_packets_event(sco_connection_handles[0]);
    if (sco_packets_sent != completed_packets) {
      ALOGW("%s: packets_sent (%d) != completed_packets (%d)", __func__,
            sco_packets_sent, completed_packets);
    }
  }
  ALOGE("End of LoopbackModeScoBandwidth");
}

// Enter loopback mode and send packets for ACL bandwidth measurements.
TEST_F(BluetoothTransportTest, LoopbackModeAclBandwidth) {
  ALOGE("Start of LoopbackModeAclBandwidth");
  reset();
  setBufferSizes();

  enterLoopbackMode();

  if (!acl_connection_handles.empty()) {
    ASSERT_LT(0, max_acl_data_packet_length);
    sendAndCheckAcl(kNumAclPacketsBandwidth, max_acl_data_packet_length - 1,
                    acl_connection_handles[0]);
    int acl_packets_sent = kNumAclPacketsBandwidth;
    int completed_packets =
        wait_for_completed_packets_event(acl_connection_handles[0]);
    if (acl_packets_sent != completed_packets) {
      ALOGW("%s: packets_sent (%d) != completed_packets (%d)", __func__,
            acl_packets_sent, completed_packets);
    }
  }
  ALOGE("End of LoopbackModeAclBandwidth");
}

// Set all bits in the event mask
TEST_F(BluetoothTransportTest, SetEventMask) {
  ALOGE("Start of SetEventMask");
  reset();
  std::vector<uint8_t> set_event_mask{
      0x01, 0x0c, 0x08 /*parameter bytes*/, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff};
  hci->sendHciCommand({set_event_mask});
  wait_for_command_complete_event(set_event_mask);
  ALOGE("End of SetEventMask");
}

// Set all bits in the LE event mask
TEST_F(BluetoothTransportTest, SetLeEventMask) {
  ALOGE("Start of SetLeEventMask");
  reset();
  std::vector<uint8_t> set_event_mask{
      0x20, 0x0c, 0x08 /*parameter bytes*/, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff};
  hci->sendHciCommand({set_event_mask});
  wait_for_command_complete_event(set_event_mask);
  ALOGE("End of SetLeEventMask");
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BluetoothTransportTest);
INSTANTIATE_TEST_SUITE_P(BluetoothHci, BluetoothTransportTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothHci::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ALOGE("Start of main");
  ABinderProcess_startThreadPool();
  ::testing::InitGoogleTest(&argc, argv);

  for (int i = 1; i < argc; i++) {
    if (argv[i] == NULL) continue;

    if (string(argv[i]).find(kMaxAppSleepTimer) == 0) {
      string value = string(argv[i]).substr(kMaxAppSleepTimer.length());
      maxAppSleepTimer = std::atoi(value.c_str());
    } else if (string(argv[i]).find(kPINCTRLTimerValue) == 0) {
      string value = string(argv[i]).substr(kPINCTRLTimerValue.length());
      pinctrlTimerValue = std::atoi(value.c_str());
    } else if (string(argv[i]).find(kMinAppSleepTimer) == 0) {
      string value = string(argv[i]).substr(kMinAppSleepTimer.length());
      minAppSleepTimer = std::atoi(value.c_str());
    } else if (string(argv[i]).find(kMaxIterations) == 0) {
      string value = string(argv[i]).substr(kMaxIterations.length());
      maxIterations = std::atoi(value.c_str());
    } else if (string(argv[i]).find(kMaxMsgsPerIteration) == 0) {
      string value = string(argv[i]).substr(kMaxMsgsPerIteration.length());
      maxMsgsPerIteration = std::atoi(value.c_str());
    } else if (string(argv[i]).find(kLogEvents) == 0) {
      logEvents = true;
   } else if (string(argv[i]).find(kConsoleLog) == 0) {
     consoleLog = true;
    } else if (string(argv[i]).find(kLogCommands) == 0) {
      logCommands = true;
    } else if (string(argv[i]).find(kLogData) == 0) {
      logData = true;
    }

    // Shift the remainder of the argv list left by one.
    for (int j = i; j != argc; j++) {
      argv[j] = argv[j + 1];
    }

    // Decrements the argument count.
    argc--;

    // We also need to decrement the iterator as we just removed an element.
    i--;
  }
  std::cout << "Max time:" << maxAppSleepTimer << " Min Time:" << minAppSleepTimer
            << " Iterations:" << maxIterations << " MaxMsgsPerIteration:"
            << maxMsgsPerIteration << " logEvents:" << logEvents
            << " logCommands:" << logCommands << " logData:" << logData
            << " pinctrlTimerValue:" << pinctrlTimerValue
            << " consoleLog:" << consoleLog << std::endl;
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  ALOGE("End of main");
  return status;
}
