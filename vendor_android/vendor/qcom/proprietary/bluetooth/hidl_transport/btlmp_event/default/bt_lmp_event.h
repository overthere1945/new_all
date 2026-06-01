/*
 * Copyright (c) 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <array>

#include <aidl/android/hardware/bluetooth/lmp_event/BnBluetoothLmpEvent.h>
#include <aidl/android/hardware/bluetooth/lmp_event/BnBluetoothLmpEventCallback.h>
#include <aidl/android/hardware/bluetooth/lmp_event/AddressType.h>
#include <aidl/android/hardware/bluetooth/lmp_event/LmpEventId.h>
#include <aidl/android/hardware/bluetooth/lmp_event/Direction.h>
#include <aidl/android/hardware/bluetooth/lmp_event/Timestamp.h>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace lmp_event {

class BluetoothLmpEvent: public BnBluetoothLmpEvent {
private:
    static std::shared_ptr<IBluetoothLmpEventCallback> event_cb;
    static AIBinder_DeathRecipient* death_obj;
public:
    BluetoothLmpEvent();
    virtual ~BluetoothLmpEvent();

    ::ndk::ScopedAStatus registerForLmpEvents(const std::shared_ptr<IBluetoothLmpEventCallback>& callback,
                                              AddressType addressType,
                                              const std::array<uint8_t, 6>& address,
                                              const std::vector<LmpEventId>& lmpEventIds);
    ::ndk::ScopedAStatus unregisterLmpEvents(AddressType addressType, const std::array<uint8_t, 6>& address);
    void sendDummyData(AddressType addressType, const LmpEventId lmpEventId);

    static void deathRecipient(void* cookie);

};
} // namespace lmp_event
} // namespace bluetooth
} // namespace hardware
} // namespace android
} // namespace aidl

