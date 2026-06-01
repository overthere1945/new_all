/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
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
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/android/hardware/bluetooth/ranging/BnBluetoothChannelSoundingSession.h>
#include <aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h>

#include <vector>
#include <android-base/logging.h>
#include <utils/Log.h>

namespace aidl::android::hardware::bluetooth::ranging::impl {

using ::aidl::android::hardware::bluetooth::ranging::ChannelSoudingRawData;
#ifdef BCS_HAL_V2
using ::aidl::android::hardware::bluetooth::ranging::ChannelSoundingProcedureData;
using ::aidl::android::hardware::bluetooth::ranging::Config;
using ::aidl::android::hardware::bluetooth::ranging::ProcedureEnableConfig;
#endif
using ::aidl::android::hardware::bluetooth::ranging::Reason;
using ::aidl::android::hardware::bluetooth::ranging::ResultType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;
using aidl::android::hardware::bluetooth::ranging::ModeType;
using aidl::android::hardware::bluetooth::ranging::ChannelSoundingSingleSideData;
using aidl::android::hardware::bluetooth::ranging::StepTonePct;
using aidl::android::hardware::bluetooth::ranging::ComplexNumber;


class BluetoothChannelSoundingSession
    : public BnBluetoothChannelSoundingSession {
 public:
  BluetoothChannelSoundingSession(
      std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback,
      Reason reason);

  ndk::ScopedAStatus getVendorSpecificReplies(
      std::optional<std::vector<std::optional<VendorSpecificData>>>*
          _aidl_return) override;
  ndk::ScopedAStatus getSupportedResultTypes(
      std::vector<ResultType>* _aidl_return) override;
  ndk::ScopedAStatus isAbortedProcedureRequired(bool* _aidl_return) override;
  ndk::ScopedAStatus writeRawData(
      const ChannelSoudingRawData& in_rawData) override;
  ndk::ScopedAStatus close(Reason in_reason) override;
#ifdef BCS_HAL_V2
  ndk::ScopedAStatus writeProcedureData(
       const ChannelSoundingProcedureData& in_procedureData) override;
  ndk::ScopedAStatus updateChannelSoundingConfig(
       const Config& in_config) override;
  ndk::ScopedAStatus updateProcedureEnableConfig(
       const ProcedureEnableConfig& in_procedureEnableConfig) override;
  ndk::ScopedAStatus updateBleConnInterval(int in_bleConnInterval) override;
#endif
 private:
  std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback_;
};

}  // namespace aidl::android::hardware::bluetooth::ranging::impl
