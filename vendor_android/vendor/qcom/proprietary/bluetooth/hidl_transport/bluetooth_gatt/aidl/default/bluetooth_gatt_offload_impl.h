/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#if USE_VENDOR_GATT_HAL
#include <aidl/vendor/qti/hardware/bluetooth/gatt/BnBluetoothGatt.h>
#include <aidl/vendor/qti/hardware/bluetooth/gatt/IBluetoothGattCallback.h>
#include <aidl/vendor/qti/hardware/bluetooth/gatt/GattCapabilities.h>
#include <aidl/vendor/qti/hardware/bluetooth/gatt/GattCharacteristic.h>
#include <aidl/vendor/qti/hardware/bluetooth/gatt/Uuid.h>
#else
#include <aidl/android/hardware/bluetooth/gatt/BnBluetoothGatt.h>
#include <aidl/android/hardware/bluetooth/gatt/IBluetoothGattCallback.h>
#include <aidl/android/hardware/bluetooth/gatt/GattCapabilities.h>
#include <aidl/android/hardware/bluetooth/gatt/GattCharacteristic.h>
#include <aidl/android/hardware/bluetooth/gatt/Status.h>
#include <aidl/android/hardware/bluetooth/gatt/GattSession.h>
#include <aidl/android/hardware/bluetooth/gatt/ErrorReport.h>
#include <aidl/android/hardware/bluetooth/gatt/Uuid.h>
#endif

#include "hci_glink_transport.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

#if USE_VENDOR_GATT_HAL
namespace aidl::vendor::qti::hardware::bluetooth::gatt::impl {
#else
namespace aidl::android::hardware::bluetooth::gatt::impl {
#endif

#if USE_VENDOR_GATT_HAL
using ::aidl::vendor::qti::hardware::bluetooth::gatt::BnBluetoothGatt;
using ::aidl::vendor::qti::hardware::bluetooth::gatt::GattCapabilities;
using ::aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback;
using ::aidl::vendor::qti::hardware::bluetooth::gatt::GattCharacteristic;
using ::aidl::vendor::qti::hardware::bluetooth::gatt::Uuid;
#else
using ::aidl::android::hardware::bluetooth::gatt::BnBluetoothGatt;
using ::aidl::android::hardware::bluetooth::gatt::GattCapabilities;
using ::aidl::android::hardware::bluetooth::gatt::IBluetoothGattCallback;
using ::aidl::android::hardware::bluetooth::gatt::GattCharacteristic;
using ::aidl::android::hardware::bluetooth::gatt::Uuid;
using ::aidl::android::hardware::bluetooth::gatt::Status;
using ::aidl::android::hardware::bluetooth::gatt::GattSession;
using ::aidl::android::hardware::bluetooth::gatt::ErrorReport;
#endif

using ::aidl::android::hardware::contexthub::EndpointId;

  class BluetoothGattImpl : public BnBluetoothGatt {
    public:
        BluetoothGattImpl();
        ~BluetoothGattImpl();
        void deInitialize();

#if USE_VENDOR_GATT_HAL
       ::ndk::ScopedAStatus init(const std::shared_ptr<::aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback>& in_callback) override;
        ::ndk::ScopedAStatus getGattCapabilities(::aidl::vendor::qti::hardware::bluetooth::gatt::GattCapabilities* _aidl_return) override;
        ::ndk::ScopedAStatus registerService(int32_t in_sessionId, int32_t in_aclConnectionHandle, int32_t in_attMtu,::aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGatt::Role in_role,
                                             const ::aidl::vendor::qti::hardware::bluetooth::gatt::Uuid& in_serviceUuid, const std::vector<::aidl::vendor::qti::hardware::bluetooth::gatt::GattCharacteristic>& in_gattChars,
                                             const ::aidl::android::hardware::contexthub::EndpointId& in_endpointId) override;
#else
        ::ndk::ScopedAStatus init(const std::shared_ptr<::aidl::android::hardware::bluetooth::gatt::IBluetoothGattCallback>& in_callback) override;
        ::ndk::ScopedAStatus getGattCapabilities(::aidl::android::hardware::bluetooth::gatt::GattCapabilities* _aidl_return) override;
        ::ndk::ScopedAStatus registerService(const GattSession& in_session) override;
#endif
        ::ndk::ScopedAStatus unregisterService(int32_t in_sessionId) override;
        ::ndk::ScopedAStatus clearServices(int32_t in_aclConnectionHandle) override;

    private:

        struct LocalGattCapabilities {
            int supported_gatt_client_properties;
            int supported_gatt_server_properties;
        };
        enum decode_type {
            PROTO_ENC_DEC,
            PROTO_RAW_BYTES,
            PROTO_NONE,
         };
        enum State {
            STATE_OPENING,
            STATE_OPENED,
            STATE_CLOSING,
            STATE_CLOSED
        };
        std::condition_variable gatt_cap_cond;
        std::mutex gatt_cap_cond_mtx;
        struct StateTableEntry {
            int32_t session_id;
            State state;
        };
        std::string stateToString(State state);
        void updateState(std::vector<StateTableEntry>& stateTable, int32_t session_id, State newState);
        void dumpStateTable();
        State getState(int32_t session_id);
        void removeEntry(int32_t session_id);
        std::vector<StateTableEntry> stateTable;
        ::ndk::ScopedAStatus sendGlinkMessage(uint16_t msg_id, uint16_t encode_decode_val,
              const std::string& payload, const char* func_name);

        std::shared_ptr<IBluetoothGattCallback> callback_;
        HciGlinkTransport* gatt_offload_transport = nullptr;
        LocalGattCapabilities* localGattCapabilities = nullptr;
        std::atomic<bool> running_gatt_offload_ch_{false};
        int wakeup_pipe_fd_[2];
        std::unique_ptr<std::thread> glink_rx_thread;
        std::mutex stateTable_mtx;
        std::mutex gattCapabilities_mtx;
        bool gattCapabilitiesReady_ = false;
        void processGattOffloadRx();
#if USE_VENDOR_GATT_HAL
        std::string statusToString(aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status status);
        std::string errorToString(aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Error error);
#else
        std::string statusToString(Status status);
        std::string errorToString(ErrorReport::Error error);
#endif
    };
}
namespace bluetooth {
namespace gatt {
class BluetoothGattLocalIntf {
  public:
    virtual ~BluetoothGattLocalIntf() = default;
#if USE_VENDOR_GATT_HAL
    static void init(aidl::vendor::qti::hardware::bluetooth::gatt::impl::BluetoothGattImpl* bluetoothGattImpl);
#else
    static void init(aidl::android::hardware::bluetooth::gatt::impl::BluetoothGattImpl* bluetoothGattImpl);
#endif
    static void cleanUp();
    static BluetoothGattLocalIntf* getInterface();
    virtual void deInitialize() = 0;
};
}
}
