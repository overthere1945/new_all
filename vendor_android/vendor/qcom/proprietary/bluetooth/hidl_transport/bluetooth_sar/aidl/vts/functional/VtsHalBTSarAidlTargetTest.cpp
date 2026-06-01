/*
 * Copyright (c) 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 */
/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "btsar_aidl_hal_test"
#include <android-base/logging.h>
#include <utils/Log.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/vendor/qti/hardware/bluetooth_sar/IBluetoothSar.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHci.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h>
#include <aidl/android/hardware/bluetooth/BnBluetoothHciCallbacks.h>

#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <queue>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

#include <csignal>
#include <ctime>
#include <cutils/properties.h>

using aidl::vendor::qti::hardware::bluetooth_sar::IBluetoothSar;
using aidl::android::hardware::bluetooth::IBluetoothHci;
using aidl::android::hardware::bluetooth::IBluetoothHciCallbacks;
using aidl::android::hardware::bluetooth::Status;

using ndk::ScopedAStatus;
using ndk::SpAIBinder;
 /*
 * Set "persist.vendor.service.bdroid.ssrlvl" property to 3 before executing
 * VTS test cases.
 */


 /*
 * Compilation flag to identify whether to execute test cases with BT turned ON
 * or BT turned OFF from Settings.
 *
 * If the below flag is enabled, run the test cases with BT turned off from Settings.
 *
 * Else if the below line containing the flag is commented out, run the test cases
 * with BT turned ON from Settings
 */
#define ENABLE_VTS_BT_REGISTRATION  TRUE

#define COMMAND_HCI_DEBUG_INFO  { 0x5B, 0xFD, 0x00 }

static constexpr std::chrono::milliseconds kWaitForInitTimeout(9000);
static constexpr std::chrono::milliseconds kInterfaceCloseDelayMs(4000);
#define WAIT_FOR_INIT_TIMEOUT std::chrono::milliseconds(9000)

#define EVENT_CONNECTION_COMPLETE 0x03
#define EVENT_COMMAND_COMPLETE 0x0e
#define EVENT_COMMAND_STATUS 0x0f
#define EVENT_NUMBER_OF_COMPLETED_PACKETS 0x13
#define EVENT_LOOPBACK_COMMAND 0x19

#define EVENT_CODE_BYTE 0
#define EVENT_LENGTH_BYTE 1
#define EVENT_FIRST_PAYLOAD_BYTE 2
#define EVENT_COMMAND_STATUS_STATUS_BYTE 2
#define EVENT_COMMAND_STATUS_ALLOWED_PACKETS_BYTE 3
#define EVENT_COMMAND_STATUS_OPCODE_LSBYTE 4  // Bytes 4 and 5
#define EVENT_COMMAND_COMPLETE_ALLOWED_PACKETS_BYTE 2
#define EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE 3  // Bytes 3 and 4
#define EVENT_COMMAND_COMPLETE_STATUS_BYTE 5
#define EVENT_COMMAND_COMPLETE_FIRST_PARAM_BYTE 6
#define EVENT_LOCAL_HCI_VERSION_BYTE EVENT_COMMAND_COMPLETE_FIRST_PARAM_BYTE
#define EVENT_LOCAL_LMP_VERSION_BYTE EVENT_LOCAL_HCI_VERSION_BYTE + 3

std::string power_level;

// The main test class for Bluetooth HAL.
class BluetoothSarTest : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    // currently test passthrough mode only
    bluetooth_sar = IBluetoothSar::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("vendor.qti.hardware.bluetooth_sar.IBluetoothSar/default")));
    ASSERT_NE(bluetooth_sar, nullptr);
    ALOGI("%s: getService() for bluetooth SAR AIDL", __func__);
  }

  virtual void TearDown() override {
      ALOGI("TearDown Bluetooth SAR AIDL Test");
      bluetooth_sar = nullptr;
  }

  void bluetooth_init();
  void bluetooth_deinit();
  void handle_no_ops();

  // A simple test implementation of BluetoothHciCallbacks.
  class BluetoothHciCallbacks
      : public aidl::android::hardware::bluetooth::BnBluetoothHciCallbacks {
    BluetoothSarTest& parent_;

   public:
    BluetoothHciCallbacks(BluetoothSarTest& parent) : parent_(parent){};

    virtual ~BluetoothHciCallbacks() = default;

    ndk::ScopedAStatus initializationComplete(Status status) {
      static int count = 0;
      if(count == 0)
      parent_.initialized_promise.set_value(status == Status::SUCCESS);
      parent_.initialized = (status == Status::SUCCESS);
      ALOGV("%s (status = %d)", __func__, static_cast<int>(status));
      count++;
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus hciEventReceived(
        const std::vector<uint8_t>& event) {
      parent_.event_cb_count++;
      parent_.event_queue.push(event);
      ALOGV("Event received (length = %d)", static_cast<int>(event.size()));
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus aclDataReceived(
        const std::vector<uint8_t>& data) {
      parent_.acl_cb_count++;
      parent_.acl_queue.push(data);
      ALOGV("ACL Data received (length = %d)", static_cast<int>(data.size()));
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus scoDataReceived(
        const std::vector<uint8_t>& data) {
      parent_.sco_cb_count++;
      parent_.sco_queue.push(data);
      ALOGV("SCO Data received (length = %d)", static_cast<int>(data.size()));
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus isoDataReceived(
      const std::vector<uint8_t>& data) {
      ALOGI("ISO Data received (length = %d)", static_cast<int>(data.size()));
      return ScopedAStatus::ok();
    };
  };

  std::shared_ptr<IBluetoothHci> bluetooth;
  std::shared_ptr<BluetoothHciCallbacks> bluetooth_cb;

  std::queue<std::vector<uint8_t>> event_queue;
  std::queue<std::vector<uint8_t>> acl_queue;
  std::queue<std::vector<uint8_t>> sco_queue;

  bool initialized;
  std::promise<bool> initialized_promise;

  int event_cb_count;
  int sco_cb_count;
  int acl_cb_count;

  int max_acl_data_packet_length;
  int max_sco_data_packet_length;
  int max_acl_data_packets;
  int max_sco_data_packets;

  std::shared_ptr<IBluetoothSar> bluetooth_sar;
};

void BluetoothSarTest::bluetooth_init() {
  ALOGD("Start of bluetooth_init()");
  static int count = 0;
  bluetooth = IBluetoothHci::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("android.hardware.bluetooth.IBluetoothHci/default")));
  ASSERT_NE(bluetooth, nullptr);
  ALOGI("%s: getService() for bluetooth is %s", __func__,
         bluetooth->isRemote() ? "remote" : "local");

  bluetooth_cb = ndk::SharedRefBase::make<BluetoothHciCallbacks>(*this);
  ASSERT_NE(bluetooth_cb, nullptr);

  max_acl_data_packet_length = 0;
  max_sco_data_packet_length = 0;
  max_acl_data_packets = 0;
  max_sco_data_packets = 0;

  initialized = false;
  event_cb_count = 0;
  acl_cb_count = 0;
  sco_cb_count = 0;

  ASSERT_EQ(initialized, false);
  bluetooth->initialize(bluetooth_cb);

  if (count == 0)
  {
      auto future = initialized_promise.get_future();
      auto timeout_status = future.wait_for(kWaitForInitTimeout);
      ASSERT_EQ(timeout_status, std::future_status::ready);
      ASSERT_TRUE(future.get());
  }
  else
      std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_FOR_INIT_TIMEOUT));

  ASSERT_EQ(initialized, true);
  count++;
  ALOGD("End of bluetooth_init()");
}

void BluetoothSarTest::bluetooth_deinit() {
  ALOGD("Start of bluetooth_deinit()");
  if (initialized) {
    ndk::ScopedAStatus ret;
    ret = bluetooth->close();
    std::this_thread::sleep_for(kInterfaceCloseDelayMs);
    if (!ret.isOk()) {
      ALOGE("%s bluetooth HAL server is dead ", __func__);
      initialized = false;
      bluetooth = nullptr;
    }
    handle_no_ops();
    // EXPECT_EQ(static_cast<size_t>(0), event_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), sco_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), acl_queue.size());
  }
  ALOGD("End of bluetooth_deinit()");
}

// Discard NO-OPs from the event queue.
void BluetoothSarTest::handle_no_ops() {
  while (event_queue.size() > 0) {
    std::vector<uint8_t> event = event_queue.front();
    EXPECT_GE(event.size(),
              static_cast<size_t>(EVENT_COMMAND_COMPLETE_STATUS_BYTE));
    bool event_is_no_op =
        (event[EVENT_CODE_BYTE] == EVENT_COMMAND_COMPLETE) &&
        (event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE] == 0x00) &&
        (event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE + 1] == 0x00);
    event_is_no_op |= (event[EVENT_CODE_BYTE] == EVENT_COMMAND_STATUS) &&
                      (event[EVENT_COMMAND_STATUS_OPCODE_LSBYTE] == 0x00) &&
                      (event[EVENT_COMMAND_STATUS_OPCODE_LSBYTE + 1] == 0x00);
    if (event_is_no_op) {
      event_queue.pop();
    } else {
      return;
    }
  }
}

#ifdef ENABLE_VTS_BT_REGISTRATION

TEST_F(BluetoothSarTest, BTSarTxPowerCapTest) {
  bluetooth_init();
  if(bluetooth_sar)
    bluetooth_sar->setBluetoothTxPowerCap(1);

  bluetooth_sar = IBluetoothSar::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("vendor.qti.hardware.bluetooth_sar.IBluetoothSar/default")));

  if(bluetooth_sar) {
    ALOGI("Testing BTSarTxPowerCapTest");
    bluetooth_sar->setBluetoothTxPowerCap(2);
    bluetooth_sar->setBluetoothTxPowerCap(3);
    bluetooth_sar->setBluetoothTxPowerCap(4);
    bluetooth_sar->setBluetoothTxPowerCap(5);
    bluetooth_sar->setBluetoothTxPowerCap(6);
    sleep(2);
    bluetooth_sar->setBluetoothTxPowerCap(3);
    sleep(10);
    ALOGI("VTS waking up after 10 secs");
    bluetooth_sar->setBluetoothTxPowerCap(10);
    bluetooth_sar->setBluetoothTxPowerCap(15);
  }
  sleep(2);
  bluetooth_deinit();
}

TEST_F(BluetoothSarTest, BTSarTxPowerCapOutOfRange) {
  bluetooth_init();
  if(bluetooth_sar) {
    ALOGI("Testing BTSarTxPowerCapOutOfRange");
    bluetooth_sar->setBluetoothTxPowerCap(90);
    bluetooth_sar->setBluetoothTxPowerCap(120);
  }
  sleep(2);
  bluetooth_deinit();
}

TEST_F(BluetoothSarTest, BTSarTxPowerCapNegativePowerLevel) {
  bluetooth_init();
  if(bluetooth_sar) {
    ALOGI("Testing BTSarTxPowerCapNegativePowerLevel");
    bluetooth_sar->setBluetoothTxPowerCap(-5);
  }
  sleep(2);
  bluetooth_deinit();
}

TEST_F(BluetoothSarTest, BTSarTxPowerCapAfterBToff) {
  if(bluetooth_sar) {
    ALOGI("Testing BTSarTxPowerCapAfterBToff");
    bluetooth_sar->setBluetoothTxPowerCap(1);
    bluetooth_sar->setBluetoothTxPowerCap(2);
  }
}

TEST_F(BluetoothSarTest, BTSarTechBasedTxPowerCap) {
  bluetooth_init();
  bluetooth_sar = IBluetoothSar::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("vendor.qti.hardware.bluetooth_sar.IBluetoothSar/default")));
  if(bluetooth_sar) {
    ALOGI("Testing BTSarTechBasedTxPowerCap");
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(3, 4, 5);
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(53, 54, 55);
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(43, 44, 45);
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(33, 34, 45);
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(23, 24, 25);
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(13, 14,15);
    sleep(2);
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(6, 8,10);
    sleep(10);
    ALOGI("VTS waking up after 10 secs");
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(10, 20,40);
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(25, 26,30);
  }
  sleep(2);
  bluetooth_deinit();
}

TEST_F(BluetoothSarTest, BTSarTechBasedTxPowerCapOutOfRange) {
  bluetooth_init();
  bluetooth_sar = IBluetoothSar::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("vendor.qti.hardware.bluetooth_sar.IBluetoothSar/default")));
  if(bluetooth_sar) {
    ALOGI("Testing BTSarTechBasedTxPowerCapOutOfRange");
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(90, 100, 120);
  }
  sleep(2);
  bluetooth_deinit();
}

TEST_F(BluetoothSarTest, BTSarTechBasedTxPowerCapNegativePowerLevel) {
  bluetooth_init();
  bluetooth_sar = IBluetoothSar::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("vendor.qti.hardware.bluetooth_sar.IBluetoothSar/default")));
  if(bluetooth_sar) {
    ALOGI("Testing BTSarTechBasedTxPowerCapNegativePowerLevel");
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(-2, -3, -5);
  }
  sleep(2);
  bluetooth_deinit();
}

TEST_F(BluetoothSarTest, BTSarTechBasedTxPowerCapAfterBToff) {
  bluetooth_sar = IBluetoothSar::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("vendor.qti.hardware.bluetooth_sar.IBluetoothSar/default")));
  if(bluetooth_sar) {
    ALOGI("Testing BTSarTechBasedTxPowerCapAfterBToff");
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(3, 4, 5);
    bluetooth_sar->setBluetoothTechBasedTxPowerCap(22, 34, 45);
  }
}

TEST_F(BluetoothSarTest, BTSarModeBasedTxPowerCapTest) {
  std::array<uint8_t, 3> chain0Cap = {0x01, 0x02, 0x03};
  std::array<uint8_t, 3> chain1Cap = {0x04, 0x05, 0x06};
  std::array<uint8_t, 6> beamformingCap = {0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C};
  bluetooth_init();
  bluetooth_sar = IBluetoothSar::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("vendor.qti.hardware.bluetooth_sar.IBluetoothSar/default")));
  if(bluetooth_sar) {
    ALOGI("Testing BTSarModeBasedTxPowerCapTest");
    bluetooth_sar->setBluetoothModeBasedTxPowerCap(chain0Cap, chain1Cap, beamformingCap);
    bluetooth_sar->setBluetoothModeBasedTxPowerCap(chain0Cap, chain1Cap, beamformingCap);
  }
  sleep(2);
  bluetooth_deinit();
}

TEST_F(BluetoothSarTest, BTSarModeBasedTxPowerCapPlusHRTest) {
  std::array<uint8_t, 4> chain0Cap = {0x01, 0x02, 0x03, 0x04};
  std::array<uint8_t, 4> chain1Cap = {0x05, 0x06, 0x07, 0x08};
  std::array<uint8_t, 8> beamformingCap = {0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
  bluetooth_init();
  bluetooth_sar = IBluetoothSar::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("vendor.qti.hardware.bluetooth_sar.IBluetoothSar/default")));
  if(bluetooth_sar) {
    ALOGI("Testing BTSarModeBasedTxPowerCapPlusHRTest");
    bluetooth_sar->setBluetoothModeBasedTxPowerCapPlusHR(chain0Cap, chain1Cap, beamformingCap);
    bluetooth_sar->setBluetoothModeBasedTxPowerCapPlusHR(chain0Cap, chain1Cap, beamformingCap);
  }
  sleep(2);
  bluetooth_deinit();
}

TEST_F(BluetoothSarTest, BTSarAreaCodeTest) {
  std::array<uint8_t, 3> areaCode = {4, 1, 2};
  bluetooth_init();
  bluetooth_sar = IBluetoothSar::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService("vendor.qti.hardware.bluetooth_sar.IBluetoothSar/default")));
  if(bluetooth_sar) {
    ALOGI("Testing BTSarAreaCodeTest");
    bluetooth_sar->setBluetoothAreaCode(areaCode);
    bluetooth_sar->setBluetoothAreaCode(areaCode);
  }
  sleep(2);
  bluetooth_deinit();
}


#else //ENABLE_VTS_BT_REGISTRATION

TEST_F(BluetoothSarTest, BTSarTestWithPowerLevel) {
  int power_level_value = 0;
  if(!power_level.empty()) {
    power_level_value = std::stoi(power_level);
  }
  ALOGI("power_level_value::= %d", power_level_value);
  if(bluetooth_sar && (power_level_value >= 0)) {
    bluetooth_sar->setBluetoothTxPowerCap(power_level_value);
  }
}

#endif //ENABLE_VTS_BT_REGISTRATION

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ABinderProcess_startThreadPool();
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
