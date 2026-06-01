/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <aidl/android/hardware/bluetooth/socket/BnBluetoothSocket.h>
#include <aidl/android/hardware/bluetooth/socket/SocketCapabilities.h>
#include <aidl/android/hardware/bluetooth/socket/IBluetoothSocketCallback.h>
#include <aidl/android/hardware/bluetooth/socket/Status.h>
#include "hci_glink_transport.h"

#include <thread>

namespace aidl::android::hardware::bluetooth::socket::impl {

using ::aidl::android::hardware::bluetooth::socket::BnBluetoothSocket;
using ::aidl::android::hardware::bluetooth::socket::SocketCapabilities;
using ::aidl::android::hardware::bluetooth::socket::IBluetoothSocketCallback;
using ::aidl::android::hardware::bluetooth::socket::Status;

    class BluetoothSocketImpl : public BnBluetoothSocket {
    public:
        BluetoothSocketImpl();
        ~BluetoothSocketImpl();

        void deInitialize();

        ::ndk::ScopedAStatus registerCallback(const std::shared_ptr<::aidl::android::hardware::bluetooth::socket::IBluetoothSocketCallback>& in_callback) override;
        ::ndk::ScopedAStatus getSocketCapabilities(::aidl::android::hardware::bluetooth::socket::SocketCapabilities* _aidl_return) override;
        ::ndk::ScopedAStatus opened(const ::aidl::android::hardware::bluetooth::socket::SocketContext& in_context) override;
        ::ndk::ScopedAStatus closed(int64_t in_socketId) override;

    private:

        struct LocalSocketCapabilities {
            int lecoc_num_of_supported_sockets;
            int lecoc_mtu;
            int rfcomm_num_of_supported_sockets;
            int rfcomm_max_frame_size;
        };

        static pthread_cond_t capabilities_cond;
        static pthread_mutex_t capabilities_lock;

        std::shared_ptr<IBluetoothSocketCallback> callback_;
        HciGlinkTransport* socket_offload_transport = nullptr;
        LocalSocketCapabilities* localSocketCapabilities = nullptr;
        std::atomic<bool> running_socket_offload_ch_{false};
        std::unique_ptr<std::thread> glink_rx_thread;
        int wakeup_pipe_fd_[2];
        void processSocketOffloadRx();
        bool isMaxOffloadSocketLimitReached(uint8_t socket_type);
        std::string byteArrayToString(const std::array<uint8_t, 6>& bdAddr);


        // Define the states
        enum State {
            STATE_OPENING,
            STATE_OPENED,
            STATE_CLOSING,
            STATE_CLOSED
        };

        // Define a structure for the state table entries
        struct StateTableEntry {
            int64_t sock_id;
            State state;
            int type;
        };

        std::string stateToString(State state);
        void updateState(std::vector<StateTableEntry>& stateTable, int64_t sock_id, State newState);
        void dumpStateTable();
        State getState(int64_t sock_id);
        void removeEntry(int64_t sock_id);
        int countEntriesWithType(std::vector<StateTableEntry>& stateTable, int type);
        //Initialize the State Table
        std::vector<StateTableEntry> stateTable;

    };
}

namespace bluetooth {
namespace socket {

class BluetoothSocketLocalIntf {
  public:
    virtual ~BluetoothSocketLocalIntf() = default;
    static void init(aidl::android::hardware::bluetooth::socket::impl::BluetoothSocketImpl* bluetoothSocketImpl);
    static void cleanUp();
    static BluetoothSocketLocalIntf* getInterface();
    virtual void deInitialize() = 0;
};

}
}
