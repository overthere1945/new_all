/*
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <aidl/vendor/qti/hardware/bluetooth_sar/BnBluetoothSar.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include "controller.h"
#include "data_handler.h"
#include <utils/Log.h>

using android::hardware::bluetooth::V1_0::implementation::DataHandler;

namespace aidl {
namespace vendor {
namespace qti {
namespace hardware {
namespace bluetooth_sar {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

class BluetoothSar : public BnBluetoothSar {
public:
  BluetoothSar();
  ~BluetoothSar();

  ndk::ScopedAStatus setBluetoothTxPowerCap(int8_t cap) override;
  ndk::ScopedAStatus setBluetoothTechBasedTxPowerCap(int8_t br_cap, int8_t edr_cap, int8_t ble_cap) override;
  ndk::ScopedAStatus setBluetoothModeBasedTxPowerCap(const std::array<uint8_t, 3>& chain0Cap, const std::array<uint8_t, 3>& chain1Cap, const std::array<uint8_t, 6>& beamformingCap) override;
  ndk::ScopedAStatus setBluetoothModeBasedTxPowerCapPlusHR(const std::array<uint8_t, 4>& chain0Cap, const std::array<uint8_t, 4>& chain1Cap, const std::array<uint8_t, 8>& beamformingCap) override;
  ndk::ScopedAStatus setBluetoothAreaCode(const std::array<uint8_t, 3>& areaCode) override;

private:
  bool is_timer_created;
  timer_t btsar_timer;
  void sendDataToController(std::vector<uint8_t> data);
  void InitializeDataHandler();
  void sendDebugInfo();
  static void BTSarTimerExpired();
  void BTSarTimerStart();
  void BTSarTimerStop();
  void BTSarTimerCleanup();

};

}  // namespace bluetooth_sar
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
}  // namespace aidl

