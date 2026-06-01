/*
 * Copyright (c) 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 */

/*
 * Copyright 2016 The Android Open Source Project
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

#pragma once

#include <aidl/android/hardware/bluetooth/BnBluetoothHci.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h>
#include <log/log.h>

#include <string>

#include "hci_internals.h"

namespace aidl::android::hardware::bluetooth::impl {

class BluetoothDeathRecipient;

// Bluetooth HAL implementation
class BluetoothHci : public BnBluetoothHci {
public:
  BluetoothHci();

  ndk::ScopedAStatus
  initialize(const std::shared_ptr<IBluetoothHciCallbacks> &cb) override;

  ndk::ScopedAStatus
  initialize_aidl(const std::shared_ptr<IBluetoothHciCallbacks> &cb);

  ndk::ScopedAStatus
  sendHciCommand(const std::vector<uint8_t> &packet) override;

  ndk::ScopedAStatus sendAclData(const std::vector<uint8_t> &packet) override;

  ndk::ScopedAStatus sendScoData(const std::vector<uint8_t> &packet) override;

  ndk::ScopedAStatus sendIsoData(const std::vector<uint8_t> &packet) override;

  ndk::ScopedAStatus close() override;

private:
  std::shared_ptr<BluetoothDeathRecipient> mDeathRecipient;

  std::shared_ptr<IBluetoothHciCallbacks> event_cb_;

  void sendDataToController(HciPacketType type, const std::vector<uint8_t> &v);
};

} // namespace aidl::android::hardware::bluetooth::impl
