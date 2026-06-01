/*
  * Copyright (c) 2017, 2024 Qualcomm Technologies, Inc.
  * All Rights Reserved.
  * Confidential and Proprietary - Qualcomm Technologies, Inc.
  *
*/

#pragma once
#include <aidl/vendor/qti/hardware/fm/BnFmHci.h>
#include <aidl/vendor/qti/hardware/fm/IFmHciCallbacks.h>

#include <future>
#include <string>

#include "controller.h"
#include "data_handler.h"
#include <utils/Log.h>
#include <cutils/properties.h>
#include "FmIoctlHci.h"


using ::aidl::vendor::qti::hardware::fm::BnFmHci;
using ::aidl::vendor::qti::hardware::fm::IFmHciCallbacks;
using ::android::hardware::hidl_vec;
using DataReadCallback = std::function<void(HciPacketType, const hidl_vec<uint8_t>*)>;
using android::hardware::bluetooth::V1_0::implementation::DataHandler;


namespace aidl::vendor::qti::hardware::fm::impl {
class FmDeathRecipient;

// This FM HAL implementation connects with a serial port at dev_path_.
class FmHci : public BnFmHci {
 public:
  FmHci();

  ndk::ScopedAStatus initialize(
      const std::shared_ptr<IFmHciCallbacks>& cb) override;

  ndk::ScopedAStatus sendHciCommand(
      const std::vector<uint8_t>& packet) override;


  ndk::ScopedAStatus close() override;

 private:
  int mFd{-1};
  std::shared_ptr<IFmHciCallbacks> mCb = nullptr;
  static struct fm_hal_t *fmhal;
  static void fmDoIoctlThread();

  std::shared_ptr<FmDeathRecipient> mDeathRecipient;

  std::string mDevPath;

  //::vendor::qti::hardware::fm::async::AsyncFdWatcher mFdWatcher;

  int getFdFromDevPath();

  void sendDataToController(HciPacketType type, const std::vector<uint8_t>& data);

  // Send a reset command and discard all packets until a reset is received.
  void reset();
};
}