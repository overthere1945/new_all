/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "bluetooth_socket_impl.h"
#include "g_offload_constants.h"
#include <utils/Log.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef GOOGLE_OFFLOAD_ENABLED
#include "protobuf/proto/bt_socket_mgr.pb.h"
#include "protobuf/include/OffloadManagerInterface.h"
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.hardware.bluetooth.socket"

using namespace aidl::android::hardware::bluetooth::socket::impl;
using bluetooth::socket::BluetoothSocketLocalIntf;

namespace google_offload_proto = google::offload::proto;

namespace aidl::android::hardware::bluetooth::socket::impl {
    BluetoothSocketImpl* instance = NULL;
    pthread_cond_t BluetoothSocketImpl::capabilities_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t BluetoothSocketImpl::capabilities_lock = PTHREAD_MUTEX_INITIALIZER;

    BluetoothSocketImpl::BluetoothSocketImpl() {
        ALOGI("******** BluetoothSocketImpl ********\n");
        instance = this;
        BluetoothSocketLocalIntf::init(this);
        socket_offload_transport = new HciGlinkTransport();
        localSocketCapabilities = new LocalSocketCapabilities;
    }

    void BluetoothSocketImpl::deInitialize() {
        ALOGI("******** deInitialize ********\n");
        socket_offload_transport->close();
        std::atomic_exchange(&running_socket_offload_ch_, false);
        write(wakeup_pipe_fd_[1], "0", 1);
        if (glink_rx_thread && glink_rx_thread->joinable()) {
            glink_rx_thread->join();
            ALOGD("%s:glink_rx_thread closed",__func__);
        }
        close(wakeup_pipe_fd_[0]);
        close(wakeup_pipe_fd_[1]);
    }

    BluetoothSocketImpl::~BluetoothSocketImpl() {
        ALOGI("******** ~BluetoothSocketImpl ********\n");
        BluetoothSocketLocalIntf::cleanUp();
        // Free allocated memory
        if (socket_offload_transport != nullptr) {
            delete socket_offload_transport;
            socket_offload_transport = nullptr;
        }
        if (localSocketCapabilities != nullptr) {
            delete localSocketCapabilities;
            localSocketCapabilities = nullptr;
        }
        pthread_mutex_destroy(&capabilities_lock);
        pthread_cond_destroy(&capabilities_cond);
        instance = NULL;
    }

    ::ndk::ScopedAStatus BluetoothSocketImpl::registerCallback(const std::shared_ptr<IBluetoothSocketCallback>& in_callback)  {
        ALOGI("******** registerCallback ********\n");
        if(in_callback == nullptr){
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(STATUS_BAD_VALUE, "Callback cannot be null");
        }
        callback_ = in_callback;
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus BluetoothSocketImpl::getSocketCapabilities(::aidl::android::hardware::bluetooth::socket::SocketCapabilities* _aidl_return) {
        ALOGI("******** getSocketCapabilities ********\n");
        if(socket_offload_transport != nullptr){
            int ret_fd = socket_offload_transport->open(SOCKET_OFFLOAD_GLINK_NODE);
            if (ret_fd <= 0) {
                ALOGD("%s: socket_offload glink open failed",__func__);
                std::atomic_exchange(&running_socket_offload_ch_, false);
            }else{
                ALOGD("%s: socket_offload glink open successful...start reader thread",__func__);
                std::atomic_exchange(&running_socket_offload_ch_, true);
                if (!glink_rx_thread || !glink_rx_thread->joinable()) {
                    if (pipe2(wakeup_pipe_fd_, O_NONBLOCK) == -1) {
                        ALOGE("%s: Failed to create wakeup pipe",__func__);
                    }
                    glink_rx_thread = std::unique_ptr<std::thread>(new std::thread(&BluetoothSocketImpl::processSocketOffloadRx, this));
                }
            }
        }else{
            ALOGE("%s: socket_offload_transport is null", __func__);
            return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(STATUS_BAD_VALUE, "Transport not initialized");
        }

        if (!localSocketCapabilities) {
            ALOGE("%s: localSocketCapabilities is null", __func__);
            return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(STATUS_BAD_VALUE, "Socket capabilities not initialized");
        }
        uint8_t socket_capabilities_msg[PROTO_HEADER_LENGTH];
        uint16_t forOpcode = BT_OFFLOAD_SOCKET_CAPS;
        socket_capabilities_msg[0] = forOpcode & 0xff;
        socket_capabilities_msg[1] = (forOpcode >> 8);
        uint16_t payload_length = 0;
        uint16_t protoEncoded = 0;
        socket_capabilities_msg[2] = protoEncoded & 0xff;
        socket_capabilities_msg[3] = (protoEncoded >> 8);
        socket_capabilities_msg[4] = payload_length & 0xFF;
        socket_capabilities_msg[5] = (payload_length >> 8);
        char resBuffer[PROTO_HEADER_LENGTH];
        memcpy(resBuffer, (char *) socket_capabilities_msg, PROTO_HEADER_LENGTH);
        std::string msgStr(resBuffer, PROTO_HEADER_LENGTH);
        uint8_t *tmpBuf = (uint8_t*)msgStr.c_str();
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        long ns = timeout.tv_nsec + 1000000 * (SOCKET_CAPS_TIMEOUT%1000);
        timeout.tv_nsec = ns%1000000000;
        timeout.tv_sec += ns/1000000000 + SOCKET_CAPS_TIMEOUT/1000;
        pthread_mutex_lock(&BluetoothSocketImpl::capabilities_lock);
        size_t bytes_written = 0;
        socket_offload_transport->write(tmpBuf, msgStr.length(), &bytes_written);
        ALOGD("%s: write payload bytes_written=%d", __func__, (int)bytes_written);
        if (bytes_written != msgStr.length()) {
            ALOGE("%s: Failed to write complete message, expected %zu bytes but wrote %zu", __func__, msgStr.length(), bytes_written);
            pthread_mutex_unlock(&BluetoothSocketImpl::capabilities_lock);
            return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(STATUS_BAD_VALUE, "Failed to send socket capabilities request");
        }
        int ret = pthread_cond_timedwait(&BluetoothSocketImpl::capabilities_cond,
                                            &BluetoothSocketImpl::capabilities_lock,&timeout);
        pthread_mutex_unlock(&BluetoothSocketImpl::capabilities_lock);
        if (ret == ETIMEDOUT) {
            // socket_caps Timed out
            ALOGD("%s: socket_caps_enable timedout",__func__);
            return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(STATUS_BAD_VALUE, "Socket capabilities request timed out");
        }else if(ret == 0){
            ALOGD("%s: socket_caps Response Successful",__func__);
        }else{
            ALOGD("pthread_cond_timedwait error - capabilities_cond");
            return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(STATUS_BAD_VALUE, "Failed to get socket capabilities");
        }
        // LeCoC Capabilities
        _aidl_return->leCocCapabilities.numberOfSupportedSockets = localSocketCapabilities->lecoc_num_of_supported_sockets;
        _aidl_return->leCocCapabilities.mtu = localSocketCapabilities->lecoc_mtu;
        // RFCOMM Capablities
        _aidl_return->rfcommCapabilities.numberOfSupportedSockets = localSocketCapabilities->rfcomm_num_of_supported_sockets;
        _aidl_return->rfcommCapabilities.maxFrameSize = localSocketCapabilities->rfcomm_max_frame_size;
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus BluetoothSocketImpl::opened(const ::aidl::android::hardware::bluetooth::socket::SocketContext& in_context) {
        // To-do : Read from in_context and populate
        ALOGI("******** opened ********\n");
        ALOGI("%s: socket_id is :: %lld ",__func__,in_context.socketId);
        if(getState((int64_t)in_context.socketId) != STATE_CLOSED ){
            ALOGI("%s: Socket is already in opening / closing / opened state for %lld. So, not opening again", __func__,in_context.socketId);
            return ::ndk::ScopedAStatus::ok();
        }

        if(isMaxOffloadSocketLimitReached((uint8_t)in_context.channelInfo.getTag())){
            ALOGD("%s: returning failure as Max sockets limit reached",__func__);
            std::string failure_reason = "Max Offload Connections Reached";
            callback_->openedComplete((int64_t)in_context.socketId,Status::FAILURE,failure_reason);
            return ::ndk::ScopedAStatus::ok();
        }

        stateTable.push_back({(int64_t)in_context.socketId, STATE_OPENING, (uint8_t)in_context.channelInfo.getTag()});
        dumpStateTable();
        uint8_t socket_opened_msg[PROTO_HEADER_LENGTH];
        uint16_t forOpcode = BT_OFFLOAD_SOCKET_OPEN;
        socket_opened_msg[0] = forOpcode & 0xff;
        socket_opened_msg[1] = (forOpcode >> 8);

        std::string payload;
        google_offload_proto::socket_open _socket_open;

        google_offload_proto::SocketContext *_socket_context = _socket_open.mutable_ctx();
        _socket_context->set_socket_id((int64_t)in_context.socketId);
        _socket_context->set_socket_name(in_context.name);
        _socket_context->set_aclconnectionhandle((uint32_t)in_context.aclConnectionHandle);
        _socket_context->set_type((google_offload_proto::Protocol)in_context.channelInfo.getTag());

        google_offload_proto::ChannelInfo *_channel_info = _socket_context->mutable_channel_info();

        ALOGD("%s: TAG is :: %d",__func__,in_context.channelInfo.getTag());
        switch((int)in_context.channelInfo.getTag()){
            case google_offload_proto::LE_COC:
            {
                 google_offload_proto::LECOCChannelInfo *_le_coc_channel_info = _channel_info->mutable_lecoc_channel_info();
                _le_coc_channel_info->set_localcid((uint32_t)in_context.channelInfo.get<ChannelInfo::leCocChannelInfo>().localCid);
                _le_coc_channel_info->set_remotecid((uint32_t)in_context.channelInfo.get<ChannelInfo::leCocChannelInfo>().remoteCid);
                _le_coc_channel_info->set_psm((uint32_t)in_context.channelInfo.get<ChannelInfo::leCocChannelInfo>().psm);
                _le_coc_channel_info->set_localmtu((uint32_t)in_context.channelInfo.get<ChannelInfo::leCocChannelInfo>().localMtu);
                _le_coc_channel_info->set_remotemtu((uint32_t)in_context.channelInfo.get<ChannelInfo::leCocChannelInfo>().remoteMtu);
                _le_coc_channel_info->set_localmps((uint32_t)in_context.channelInfo.get<ChannelInfo::leCocChannelInfo>().localMps);
                _le_coc_channel_info->set_remotemps((uint32_t)in_context.channelInfo.get<ChannelInfo::leCocChannelInfo>().remoteMps);
                _le_coc_channel_info->set_initialrxcredits((uint32_t)in_context.channelInfo.get<ChannelInfo::leCocChannelInfo>().initialRxCredits);
                _le_coc_channel_info->set_initialtxcredits((uint32_t)in_context.channelInfo.get<ChannelInfo::leCocChannelInfo>().initialTxCredits);

                break;
            }
            case google_offload_proto::RFCOMM:
            {
                 google_offload_proto::RFCOMMChannelInfo *_rfcomm_channel_info = _channel_info->mutable_rfcomm_channel_info();
                _rfcomm_channel_info->set_localcid((uint32_t)in_context.channelInfo.get<ChannelInfo::rfcommChannelInfo>().localCid);
                _rfcomm_channel_info->set_remotecid((uint32_t)in_context.channelInfo.get<ChannelInfo::rfcommChannelInfo>().remoteCid);
                _rfcomm_channel_info->set_localmtu((uint32_t)in_context.channelInfo.get<ChannelInfo::rfcommChannelInfo>().localMtu);
                _rfcomm_channel_info->set_remotemtu((uint32_t)in_context.channelInfo.get<ChannelInfo::rfcommChannelInfo>().remoteMtu);
                _rfcomm_channel_info->set_initialrxcredits((uint32_t)in_context.channelInfo.get<ChannelInfo::rfcommChannelInfo>().initialRxCredits);
                _rfcomm_channel_info->set_initialtxcredits((uint32_t)in_context.channelInfo.get<ChannelInfo::rfcommChannelInfo>().initialTxCredits);
                _rfcomm_channel_info->set_dlci((uint32_t)in_context.channelInfo.get<ChannelInfo::rfcommChannelInfo>().dlci);
                _rfcomm_channel_info->set_maxframesize((uint32_t)in_context.channelInfo.get<ChannelInfo::rfcommChannelInfo>().maxFrameSize);
                _rfcomm_channel_info->set_muxinitiatorflag((uint32_t)in_context.channelInfo.get<ChannelInfo::rfcommChannelInfo>().muxInitiator);
                break;
            }
            default:
                break;
        }

        google_offload_proto::EndpointId *_end_point_id = _socket_context->mutable_offloadendpointid();
        _end_point_id->set_hubid((uint64_t)in_context.endpointId.hubId);
        _end_point_id->set_endpointid((uint64_t)in_context.endpointId.id);

        _socket_open.SerializeToString(&payload);
        ALOGI("%s: payload length is %d", __func__, payload.length());

        uint16_t payload_length = payload.length();
        uint16_t protoEncoded = 0;
        socket_opened_msg[2] = protoEncoded & 0xff;
        socket_opened_msg[3] = (protoEncoded >> 8);
        socket_opened_msg[4] = payload_length & 0xFF;
        socket_opened_msg[5] = (payload_length >> 8);

        char resBuffer[PROTO_HEADER_LENGTH];
        memcpy(resBuffer, (char *) socket_opened_msg, PROTO_HEADER_LENGTH);

        std::string msgStr(resBuffer, PROTO_HEADER_LENGTH);
        msgStr.append(payload);

        uint8_t *tmpBuf = (uint8_t*)msgStr.c_str();
        if(socket_offload_transport != nullptr){
            size_t bytes_written = 0;
            socket_offload_transport->write(tmpBuf,msgStr.length(),&bytes_written);
            ALOGD("%s: write payload bytes_written=%d", __func__, (int)bytes_written);
        }

        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus BluetoothSocketImpl::closed(int64_t in_socketId) {
        ALOGI("******** closed ********\n");
        if(getState(in_socketId) != STATE_OPENED ){
            ALOGI("%s: Socket is already in opening / closing / closed state for %lld. So, not closing again", __func__,in_socketId);
            return ::ndk::ScopedAStatus::ok();
        }
        updateState(stateTable, in_socketId, STATE_CLOSING);
        dumpStateTable();
        ALOGI("%s: socket id is :: %lld\n",__func__,in_socketId);
        uint8_t socket_closed_msg[PROTO_HEADER_LENGTH];
        uint16_t forOpcode = BT_OFFLOAD_SOCKET_CLOSE;
        socket_closed_msg[0] = forOpcode & 0xff;
        socket_closed_msg[1] = (forOpcode >> 8);

        std::string payload;
        google_offload_proto::socket_close _socket_close;

        _socket_close.set_sock_id(in_socketId);
        _socket_close.SerializeToString(&payload);
        ALOGI("%s: payload length is %d", __func__, payload.length());

        uint16_t payload_length = payload.length();

        uint16_t protoEncoded = 0;
        socket_closed_msg[2] = protoEncoded & 0xff;
        socket_closed_msg[3] = (protoEncoded >> 8);
        socket_closed_msg[4] = payload_length & 0xFF;
        socket_closed_msg[5] = (payload_length >> 8);

        char resBuffer[PROTO_HEADER_LENGTH];
        memcpy(resBuffer, (char *) socket_closed_msg, PROTO_HEADER_LENGTH);

        std::string msgStr(resBuffer, PROTO_HEADER_LENGTH);
        msgStr.append(payload);

        uint8_t *tmpBuf = (uint8_t*)msgStr.c_str();
        if(socket_offload_transport != nullptr){
            size_t bytes_written = 0;
            socket_offload_transport->write(tmpBuf,msgStr.length(),&bytes_written);
            ALOGD("%s: write payload bytes_written=%d", __func__, (int)bytes_written);
        }

        return ::ndk::ScopedAStatus::ok();
    }

    std::string BluetoothSocketImpl::byteArrayToString(const std::array<uint8_t, 6>& bdAddr) {
        std::ostringstream oss;

        for (size_t i = 0; i < bdAddr.size(); ++i) {
            ALOGD("%s: bdAddr[%d] is %02x",__func__,i, bdAddr[i]);
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bdAddr[i]);
        }

        return oss.str();
    }

    std::string BluetoothSocketImpl::stateToString(State state) {
        switch (state) {
            case STATE_OPENING: return "STATE_OPENING";
            case STATE_OPENED: return "STATE_OPENED";
            case STATE_CLOSING: return "STATE_CLOSING";
            case STATE_CLOSED: return "STATE_CLOSED";
            default: return "UNKNOWN_STATE";
        }
    }

    BluetoothSocketImpl::State BluetoothSocketImpl::getState(int64_t sock_id) {
        for (const auto& entry : stateTable) {
            if (entry.sock_id == sock_id) {
                ALOGD("%s: sock_id: %lld is in %s state",__func__,sock_id, stateToString(entry.state).c_str());
                return entry.state;
            }
        }
        ALOGD("%s: Entry with sock_id %lld not found", __func__, sock_id);
        return STATE_CLOSED; // Default return value if not found
    }

    void BluetoothSocketImpl::dumpStateTable(){
        ALOGI("******** dumpStateTable ********\n");
        for (const auto& entry : stateTable) {
            ALOGD("sock_id : %lld state : %s type : %d",entry.sock_id, stateToString(entry.state).c_str(), entry.type);
        }
    }

    // Function to update the state of an existing entry
    void BluetoothSocketImpl::updateState(std::vector<StateTableEntry>& stateTable, int64_t sock_id, State newState) {
        for (auto& entry : stateTable) {
            if (entry.sock_id == sock_id) {
                entry.state = newState;
                return;
            }
        }
        ALOGD("%s: Entry with sock_id %lld not found",__func__, sock_id);
    }

    void BluetoothSocketImpl::removeEntry(int64_t sock_id) {
        for (auto it = stateTable.begin(); it != stateTable.end(); ++it) {
            if (it->sock_id == sock_id) {
                stateTable.erase(it);
                return;
            }
        }
        ALOGD("%s: Entry with sock_id %lld not found",__func__, sock_id);
    }

    bool BluetoothSocketImpl::isMaxOffloadSocketLimitReached(uint8_t socket_type){
        bool is_max_reached = false;
        if (localSocketCapabilities == nullptr) {
            ALOGE("%s: localSocketCapabilities is null, cannot check socket limits", __func__);
            return true;
        }
        ALOGD("%s: socket_type :: %d",__func__,socket_type);
        int count = countEntriesWithType(stateTable, socket_type);
        ALOGD("%s: %d sockets with type %d are present currently",__func__,count, socket_type);
        ALOGD("%s: localSocketCapabilities.lecoc_num_of_supported_sockets is :: %d",__func__,localSocketCapabilities->lecoc_num_of_supported_sockets);
        ALOGD("%s: localSocketCapabilities.rfcomm_num_of_supported_sockets is :: %d",__func__,localSocketCapabilities->rfcomm_num_of_supported_sockets);
        switch (socket_type)
        {
        case google_offload_proto::LE_COC:
            if(count >= localSocketCapabilities->lecoc_num_of_supported_sockets){
                is_max_reached = true;
            }
            break;

        case google_offload_proto::RFCOMM:
            if(count >= localSocketCapabilities->rfcomm_num_of_supported_sockets){
                is_max_reached = true;
            }
            break;
        default:
            break;
        }
        ALOGD("%s: is_max_reached :: %d",__func__,is_max_reached);
        return is_max_reached;
    }

    int BluetoothSocketImpl::countEntriesWithType(std::vector<StateTableEntry>& stateTable, int type) {
        int count = 0;
        for (const auto& entry : stateTable) {
            if (entry.type == type) {
                count++;
            }
        }
        return count;
    }

    void BluetoothSocketImpl::processSocketOffloadRx(){
        ALOGD("%s: running_socket_offload_ch_ is :: %d",__func__,running_socket_offload_ch_.load());
        uint8_t *readBuffer = (uint8_t *)malloc(MSG_SIZE_MAX*sizeof(uint8_t));
        if (readBuffer == NULL) {
            ALOGE("%s: readBuffer malloc failed",__func__);
            return;
        }
        while (running_socket_offload_ch_) {
            int rcPoll = socket_offload_transport->poll(wakeup_pipe_fd_[0], -1);
            if (-1 == rcPoll) {
                ALOGE("%s: Poll Failure",__func__);
                break;
            }
            int num = socket_offload_transport->read(readBuffer, MSG_SIZE_MAX*sizeof(uint8_t));
            ALOGD("%s: num of bytes read from stream is :: %d",__func__, num);
            if(num < MSG_SIZE_MIN) {
                ALOGE("%s: Slate response is too short ::  %d",__func__, num);
            }else {
                uint16_t msg_id = (readBuffer[1] << 8) | readBuffer[0];
                uint16_t encode_decode =(readBuffer[3] << 8) | readBuffer[2];
                uint16_t length = (readBuffer[5] << 8) | readBuffer[4];
                std::string resBufferString;

                switch (msg_id) {
                    case BT_OFFLOAD_SOCKET_OPEN_CB:
                    {
                        ALOGD("%s: Packet Type BT_OFFLOAD_SOCKET_OPEN_CB",__func__);
                        if (length > 0) {
                            char resBuffer[length];
                            memcpy(resBuffer, (char *) (readBuffer + 6) , length);
                            resBufferString.assign(resBuffer, length);
                            google_offload_proto::socket_open_rsp socket_open_rsp;
                            bool ret = socket_open_rsp.ParseFromString(resBufferString);
                            if(!ret) {
                                ALOGE("%s: Unable to parse string",__func__);
                                break;
                            }
                            if(socket_open_rsp.has_sock_id()){
                                int64_t sock_id = socket_open_rsp.sock_id();
                                ALOGI("%s: sock_id is :: %lld",__func__,sock_id);
                                if(socket_open_rsp.has_status()){
                                    uint32_t status = socket_open_rsp.status();
                                    ALOGI("%s: status is :: %d",__func__,status);
                                    if(socket_open_rsp.has_reason()){
                                        std::string reason = socket_open_rsp.reason();
                                        ALOGI("%s: reason is :: %s",__func__,reason.c_str());
                                        if(status == 0){
                                            ALOGI("%s: Offload Socket Open Successful for Sock ID : %lld",__func__,sock_id);
                                            updateState(stateTable, sock_id, STATE_OPENED);
                                            callback_->openedComplete(sock_id,Status::SUCCESS,reason);
                                            dumpStateTable();
                                        }else{
                                            ALOGI("%s: Offload Socket Open Failed for Sock ID : %lld",__func__,sock_id);
                                            callback_->openedComplete(sock_id,Status::FAILURE,reason);
                                            removeEntry(sock_id);
                                            dumpStateTable();
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case BT_OFFLOAD_SOCKET_CLOSE_CB:
                    {
                        ALOGD("%s: Packet Type BT_OFFLOAD_SOCKET_CLOSE_CB",__func__);
                        if (length > 0) {
                            char resBuffer[length];
                            memcpy(resBuffer, (char *) (readBuffer + 6) , length);
                            resBufferString.assign(resBuffer, length);
                            google_offload_proto::socket_close_rsp socket_close_rsp;
                            bool ret = socket_close_rsp.ParseFromString(resBufferString);
                            if(!ret) {
                                ALOGE("%s: Unable to parse string",__func__);
                                break;
                            }
                            if(socket_close_rsp.has_sock_id()){
                                int64_t sock_id = socket_close_rsp.sock_id();
                                ALOGI("%s: sock_id is :: %lld",__func__,sock_id);
                                if(socket_close_rsp.has_reason()){
                                    std::string reason = socket_close_rsp.reason();
                                    ALOGI("%s: Socket %lld closed with reason :: %s",__func__, sock_id, reason.c_str());
                                    callback_->close(sock_id, reason);
                                    removeEntry(sock_id);
                                    dumpStateTable();
                                }else{
                                    ALOGI("%s: reason is not avaialbe",__func__);
                                }
                            }
                        }
                        break;
                    }
                    case BT_OFFLOAD_SOCKET_CLOSE_IND:
                    {
                        ALOGD("%s: Packet Type BT_OFFLOAD_SOCKET_CLOSE_IND",__func__);
                        if (length > 0) {
                            char resBuffer[length];
                            memcpy(resBuffer, (char *) (readBuffer + 6) , length);
                            resBufferString.assign(resBuffer, length);
                            google_offload_proto::socket_close_ind socket_close_ind;
                            bool ret = socket_close_ind.ParseFromString(resBufferString);
                            if(!ret) {
                                ALOGE("%s: Unable to parse string",__func__);
                                break;
                            }
                            if(socket_close_ind.has_sock_id()){
                                int64_t sock_id = socket_close_ind.sock_id();
                                ALOGI("%s: sock_id is :: %lld",__func__,sock_id);
                                if(socket_close_ind.has_reason()){
                                    uint32_t reason = socket_close_ind.reason();
                                    ALOGI("%s: Offload Socket Close Ind Sock ID : %lld with reason : %d",__func__,sock_id,reason);
                                    closed(sock_id);
                                }
                            }
                        }
                        break;
                    }
                    case BT_OFFLOAD_SOCKET_CAPS:
                    {
                        ALOGD("%s: Packet Type BT_OFFLOAD_SOCKET_CAPS",__func__);
                        if (length > 0) {
                            char resBuffer[length];
                            memcpy(resBuffer, (char *) (readBuffer + 6) , length);
                            resBufferString.assign(resBuffer, length);
                            google_offload_proto::SocketCapabilities socketcapabilities;
                            bool ret = socketcapabilities.ParseFromString(resBufferString);
                            if(!ret) {
                                ALOGE("%s: Unable to parse string",__func__);
                                break;
                            }
                            if (socketcapabilities.has_rfcomm_capabilities()) {
                                const auto& rfcomm_capabilities = socketcapabilities.rfcomm_capabilities();
                                localSocketCapabilities->rfcomm_num_of_supported_sockets = rfcomm_capabilities.numofsocketsupported();
                                localSocketCapabilities->rfcomm_max_frame_size = rfcomm_capabilities.maxframesize();
                                ALOGI("%s: numofsocketsupported of rfcomm:: %d",__func__,localSocketCapabilities->rfcomm_num_of_supported_sockets);
                                ALOGI("%s: maxframesize of rfcomm:: %d",__func__,localSocketCapabilities->rfcomm_max_frame_size);
                            }
                            if (socketcapabilities.has_lecoc_capabilities()) {
                                const auto& lecoc_capabilities = socketcapabilities.lecoc_capabilities();
                                localSocketCapabilities->lecoc_num_of_supported_sockets = lecoc_capabilities.numofsocketsupported();
                                localSocketCapabilities->lecoc_mtu = lecoc_capabilities.mtu();
                                ALOGI("%s: numofsocketsupported of lecoc:: %d",__func__,localSocketCapabilities->lecoc_num_of_supported_sockets);
                                ALOGI("%s: mtu of lecoc:: %d",__func__,localSocketCapabilities->lecoc_mtu);
                            }
                            pthread_cond_signal(&BluetoothSocketImpl::capabilities_cond);
                        }
                        break;
                    }
                    default:
                    {
                        ALOGD("%s: Default Case",__func__);
                        break;
                    }
                }
            }
        }
        free(readBuffer);
    }
}

namespace bluetooth {
namespace socket {

static BluetoothSocketLocalIntf* local_instance = NULL;

class BluetoothSocketLocalIntfImpl: public BluetoothSocketLocalIntf {
    public:
    BluetoothSocketImpl* service_;

    BluetoothSocketLocalIntfImpl(BluetoothSocketImpl* service) {
        ALOGI("%s", __func__);
        service_ = service;
    }

    ~BluetoothSocketLocalIntfImpl() {
        ALOGI("%s", __func__);
        service_ = NULL;
    }

    void deInitialize() {
        ALOGI("%s", __func__);
        if (service_) {
            service_->deInitialize();
        }else{

        }
    }
};

BluetoothSocketLocalIntf* BluetoothSocketLocalIntf::getInterface() {
  return local_instance;
}

void BluetoothSocketLocalIntf::init(BluetoothSocketImpl* service) {
  ALOGI("%s", __func__);
  if (!local_instance) {
    local_instance = new BluetoothSocketLocalIntfImpl(service);
  }
}

void BluetoothSocketLocalIntf::cleanUp() {
  ALOGI("%s", __func__);
  if (local_instance) {
    local_instance->deInitialize();
    delete local_instance;
    local_instance = NULL;
  }
}

}
}
