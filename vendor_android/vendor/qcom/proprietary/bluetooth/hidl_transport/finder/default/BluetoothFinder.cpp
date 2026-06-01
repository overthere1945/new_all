/*
 * Copyright (c) 2022, 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 */
 
/*
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

#include "BluetoothFinder.h"
#include "data_handler.h"
#include <utils/Log.h>
#include "btcommon_interface_defs.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "vendor.qti.hardware.bluetooth.finder"

using namespace aidl::android::hardware::bluetooth::finder::impl;
using ::android::hardware::bluetooth::V1_0::implementation::DataHandler;

#define MAX_NUM_KEYS_SOC_SUPPORT 254
#define FMD_TOTAL_ADV_DATA_KEY_SIZE 5080

namespace aidl::android::hardware::bluetooth::finder::impl {
  BluetoothFinder* instance = NULL;

  BluetoothFinder::BluetoothFinder() {
    instance = this;
  }
  BluetoothFinder::~BluetoothFinder() {
    instance = NULL;
  }

::ndk::ScopedAStatus BluetoothFinder::sendEids(const ::std::vector<Eid>& keys) {
  ALOGD("%s: keys_size %d ",__func__, keys.size());
#ifdef QTI_BT_FMD_SUPPORTED
  uint8_t key_adv_data[FMD_TOTAL_ADV_DATA_KEY_SIZE];

  DataHandler *data_handler = DataHandler::Get();
  uint16_t keys_size = (keys.size() < 255)?keys.size():MAX_NUM_KEYS_SOC_SUPPORT;
  if (data_handler != nullptr) {
    for (int i = 0; i < keys_size; i++) {
      for (int j = 0; j < 20; j++) {
        key_adv_data[j+ (i*20)] = keys[i].bytes[j];
      }
      ALOGD("%s: key data validation::  %d %d ",__func__, key_adv_data[i*20], key_adv_data[(i*20) + 1]);
    }
    //call to datahandler
    data_handler->sendEids(key_adv_data, keys_size);
  } else {
    ALOGE("%s: Data handler is null",__func__);
  }
#endif

  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BluetoothFinder::setPoweredOffFinderMode(bool enable) {
  ALOGD("%s: %d",__func__, enable);

#ifdef QTI_BT_FMD_SUPPORTED
  DataHandler *data_handler = DataHandler::Get();
  if (data_handler != nullptr) {
    //call to datahandler
    data_handler->UserSetFmdMode(enable);
  } else {
    ALOGE("%s: Data handler is null",__func__);
  }
#endif
  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BluetoothFinder::getPoweredOffFinderMode(
    bool* _aidl_return) {
    *_aidl_return = false;

#ifdef QTI_BT_FMD_SUPPORTED
  DataHandler *data_handler = DataHandler::Get();
  if (data_handler != nullptr) {
    //call to datahandler
    *_aidl_return = data_handler->GetFmdMode();
  } else {
    ALOGE("%s: Data handler is null",__func__);
  }
#endif
  ALOGD("%s: %d",__func__, *_aidl_return);
  return ::ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::bluetooth::finder::impl
