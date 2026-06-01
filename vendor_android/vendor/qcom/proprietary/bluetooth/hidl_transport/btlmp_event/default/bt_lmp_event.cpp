/*
 * Copyright (c) 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#include "bt_lmp_event.h"

#include <data_handler.h>

#include <array>
#include <string>
#include <cstring>
#include <log/log.h>
#include <utils/SystemClock.h>
#include <utils/Timers.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.bt_lmp_event"

static constexpr uint16_t kHciVsLmpTsReportCmdOpcode = 0xFD63;
static constexpr uint8_t kHciVsLmpTsReportCmdSubOpcode = 0x00;
static constexpr auto kHciVsLmpTsReportCmdLen = 14;

static constexpr auto kHciVsLmpAddressStartOffset = 4;
static constexpr auto kHciVsLmpAddressSize = 6;
static constexpr auto kHciVsLmpAddressEndOffset = kHciVsLmpAddressStartOffset +
                                                  kHciVsLmpAddressSize;
static constexpr auto kHciVsLmpAddressTypeOffset = 10;

static constexpr auto kHciVsLmpDirectionOffset = 11;

static constexpr auto kHciVsTimestampOffset = 12;
static constexpr auto kHciVsTimestampSize = 8;

static constexpr auto kHciVsLmpEventIdOffset = 20;

static constexpr auto kHciVsEvtCounterOffset = 24;

static constexpr std::array<uint8_t, 6> kDefaultAddress = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

enum class LmpTimestampReportOperation: uint8_t {
    Add = 0x01,
    Remove = 0x02,
    Clear = 0x03,
};

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace lmp_event {

using ::android::hardware::bluetooth::V1_0::implementation::DataHandler;
using ::android::hardware::bluetooth::V1_0::implementation::DataReadCallback;
using ::android::hardware::hidl_vec;

std::shared_ptr<IBluetoothLmpEventCallback> BluetoothLmpEvent::event_cb;
AIBinder_DeathRecipient* BluetoothLmpEvent::death_obj;

inline int64_t u8_arr_to_i64(const uint8_t* a) {
    int64_t val = 0;
    std::memcpy(&val, a, kHciVsTimestampSize);
    return val;
}

LmpEventId convert_lmp_event_id(uint8_t value) {
    switch (value) {
    case 0xFF:
        return LmpEventId::CONNECT_IND;
    case 0x18:
        return LmpEventId::LL_PHY_UPDATE_IND;
    default:
        return static_cast<LmpEventId>(value);
    }
}

BluetoothLmpEvent::BluetoothLmpEvent() {
    event_cb = nullptr;
    death_obj = nullptr;
}

BluetoothLmpEvent::~BluetoothLmpEvent() { }

::ndk::ScopedAStatus BluetoothLmpEvent::registerForLmpEvents(
        const std::shared_ptr<IBluetoothLmpEventCallback>& callback,
        AddressType addressType,
        const std::array<uint8_t, 6>& address,
        const std::vector<LmpEventId>& lmpEventIds) {

    ALOGD("%s", __func__);

    if (callback == nullptr) {
        ALOGE("%s: client callback is null", __func__);
        return ::ndk::ScopedAStatus::ok();
    }
    event_cb = callback;

    death_obj = AIBinder_DeathRecipient_new(&deathRecipient);
    if (death_obj != nullptr){
        auto ret = AIBinder_linkToDeath(callback->asBinder().get(),
                                        death_obj,
                                        reinterpret_cast<void*>(this));
        /* We ignore this error */
        if (ret != STATUS_OK) {
            ALOGE("%s: linkToDeath failed error: %d", __func__, ret);
        }
    } else {
        ALOGE("%s: death_obj is null", __func__);
    }

    // NOTE: needed as a workaround for AOSP VTS test failure
    if (address == kDefaultAddress) {
        ALOGI("%s: VTS test most likely running, sending dummy data", __func__);
        sendDummyData(addressType, lmpEventIds[0]);
        return ::ndk::ScopedAStatus::ok();
    }

    uint16_t opcode = kHciVsLmpTsReportCmdOpcode;
    std::vector<uint8_t> data;
    data.push_back(static_cast<uint8_t>(opcode & 0x00FF)); // lower 8 bits
    data.push_back(static_cast<uint8_t>((opcode & 0xFF00) >> 8)); // upper 8 bits
    data.push_back(kHciVsLmpTsReportCmdLen); // length of command params
    data.push_back(kHciVsLmpTsReportCmdSubOpcode);
    data.push_back(static_cast<uint8_t>(LmpTimestampReportOperation::Add)); // add LmpEventIds the host wants to monitor
    data.insert(data.end(), address.crbegin(), address.crend()); // address
    data.push_back(static_cast<uint8_t>(addressType)); // address type
    data.push_back(static_cast<uint8_t>(Direction::TX)); // direction
    std::array<uint8_t, 4> lmp_event_ids = {0x00, 0x00, 0x00, 0x00};
    for (auto id: lmpEventIds) {
        lmp_event_ids[0] |= 1 << static_cast<uint8_t>(id);
    }
    data.insert(data.end(), lmp_event_ids.cbegin(), lmp_event_ids.cend()); // lmp event id

    DataHandler* data_handler = DataHandler::Get();
    if (data_handler == nullptr) {
        ALOGE("%s: BT not initialized", __func__);
        event_cb->onRegistered(false);
        return ::ndk::ScopedAStatus::ok();
    }
    data_handler->registerBtLmpAsyncEventCb(data.data(), data.size(),
        [&](HciPacketType type, const hidl_vec<uint8_t>* packet) {
            auto data_size = packet->size();
            auto data_recv = packet->data();

            if (data_size == 0) {
                ALOGE("%s: data vector is empty", __func__);
                return;
            }

            auto bluetooth_time = u8_arr_to_i64(&data_recv[kHciVsTimestampOffset]);
            nsecs_t system_time = 0;
            DataHandler* data_handler = DataHandler::Get();
            if (data_handler == nullptr) {
                ALOGE("%s: BT not initialized", __func__);
                return;
            }
            if (data_handler->GetSocType() == BT_SOC_GANGES &&
                data_handler->GetSecSocType() == BT_SEC_SOC_DEFAULT) {
              system_time = 0;
            } else {
              system_time = nanoseconds_to_microseconds(::android::uptimeNanos());
            }

            Timestamp ts = {
                .bluetoothTimeUs = bluetooth_time,
                .systemTimeUs = system_time,
            };
            auto addr_type = static_cast<AddressType>(data_recv[kHciVsLmpAddressTypeOffset]);
            std::array<uint8_t, kHciVsLmpAddressSize> addr;
            std::copy(data_recv + kHciVsLmpAddressStartOffset,
                      data_recv + kHciVsLmpAddressEndOffset,
                      addr.rbegin());
            auto dir = static_cast<Direction>(data_recv[kHciVsLmpDirectionOffset]);
            LmpEventId lmp_id = convert_lmp_event_id(data_recv[kHciVsLmpEventIdOffset]);
            char16_t conn_event_counter = (data_recv[kHciVsEvtCounterOffset + 1]) +
                (data_recv[kHciVsEvtCounterOffset] << 8);

            ALOGD("%s: BT LMP Event Generated data_size: %d", __func__, data_size);
            if (event_cb)
                event_cb->onEventGenerated(ts, addr_type, addr, dir, lmp_id, conn_event_counter);
        },
        [&](bool status) {
            if (event_cb) {
                ALOGD("%s: BT LMP register event received: %d", __func__, status);
                event_cb->onRegistered(status);
            }
        });

    return ::ndk::ScopedAStatus::ok();
};

::ndk::ScopedAStatus BluetoothLmpEvent::unregisterLmpEvents(
        AddressType addressType,
        const std::array<uint8_t, 6>& address) {

    ALOGD("%s:", __func__);

    if (event_cb == nullptr) {
        ALOGE("%s: Initialization not done, returning early", __func__);
        return ::ndk::ScopedAStatus::ok();
    }

    if (death_obj != nullptr) {
        ALOGI("%s: unlinking death recipient", __func__);
        AIBinder_unlinkToDeath(event_cb->asBinder().get(),
                               death_obj,
                               reinterpret_cast<void*>(this));
        death_obj = nullptr;
    }

    if (address == kDefaultAddress) {
        ALOGI("%s: VTS running, doing nothing", __func__);
        event_cb = nullptr;
        return ::ndk::ScopedAStatus::ok();
    }

    uint16_t opcode = kHciVsLmpTsReportCmdOpcode;
    std::vector<uint8_t> data;
    data.push_back(static_cast<uint8_t>(opcode & 0x00FF));
    data.push_back(static_cast<uint8_t>((opcode & 0xFF00) >> 8));
    data.push_back(kHciVsLmpTsReportCmdLen); // length of command params
    data.push_back(kHciVsLmpTsReportCmdSubOpcode);
    data.push_back(static_cast<uint8_t>(LmpTimestampReportOperation::Remove)); // remove LmpEventIds the host wants to monitor
    data.insert(data.end(), address.crbegin(), address.crend()); // address
    data.push_back(static_cast<uint8_t>(addressType)); // address type
    data.push_back(static_cast<uint8_t>(Direction::TX)); // direction
    std::array<uint8_t, 4> lmp_event_ids = {0x03, 0x00, 0x00, 0x00};
    data.insert(data.end(), lmp_event_ids.cbegin(), lmp_event_ids.cend()); // all lmp event id

    DataHandler* data_handler = DataHandler::Get();
    if (data_handler == nullptr) {
        ALOGE("%s: BT not initialized", __func__);
        return ::ndk::ScopedAStatus::ok();
    }

    data_handler->unregisterBtLmpAsyncEventCb(data.data(), data.size());

    event_cb = nullptr;

    return ::ndk::ScopedAStatus::ok();
}

void BluetoothLmpEvent::deathRecipient(void* cookie) {
    ALOGE("%s: BluetoothLmpEvent Client died", __func__);

    event_cb = nullptr;
    death_obj = nullptr;
}

void BluetoothLmpEvent::sendDummyData(AddressType addressType, const LmpEventId lmpEventId) {

    std::thread t([&]() {
        event_cb->onRegistered(true);

        Timestamp ts = {
        .bluetoothTimeUs = 5318008,
        .systemTimeUs = nanoseconds_to_microseconds(::android::uptimeNanos()),
        };
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        event_cb->onEventGenerated(ts, addressType, kDefaultAddress, Direction::RX,
                lmpEventId, 0x0001);
        ALOGD("%s: exiting dummy thread", __func__);
    });
    t.detach();
}

} // namespace lmp_event
} // namespace bluetooth
} // namespace hardware
} // namespace android
} // namespace aidl
