/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "bluetooth_gatt_offload_impl.h"
#include "g_offload_constants.h"
#include <utils/Log.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <memory>

#ifdef GATT_OFFLOAD_ENABLED
#include "protobuf/proto/bt_gatt_offload.pb.h"
#include "protobuf/include/GattOffloadInterface.h"
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.hardware.bluetooth.gatt"

#if USE_VENDOR_GATT_HAL
using namespace aidl::vendor::qti::hardware::bluetooth::gatt::impl;
#else
using namespace aidl::android::hardware::bluetooth::gatt::impl;
#endif

using bluetooth::gatt::BluetoothGattLocalIntf;

namespace gatt_offload_proto = gatt::offload::proto;

#if USE_VENDOR_GATT_HAL
namespace aidl::vendor::qti::hardware::bluetooth::gatt::impl {
#else
namespace aidl::android::hardware::bluetooth::gatt::impl {
#endif
    BluetoothGattImpl* instance = NULL;

    BluetoothGattImpl::BluetoothGattImpl() {
        ALOGI("******** BluetoothGattImpl ********\n");
        instance = this;
        BluetoothGattLocalIntf::init(this);
        gatt_offload_transport = new HciGlinkTransport();
        localGattCapabilities = new LocalGattCapabilities;
    }

    void BluetoothGattImpl::deInitialize() {
        ALOGI("******** deInitialize ********\n");
        gatt_offload_transport->close();
        std::atomic_exchange(&running_gatt_offload_ch_, false);
        write(wakeup_pipe_fd_[1], "0", 1);
        if (glink_rx_thread && glink_rx_thread->joinable()) {
            glink_rx_thread->join();
            ALOGD("%s:glink_rx_thread closed",__func__);
        }
        close(wakeup_pipe_fd_[0]);
        close(wakeup_pipe_fd_[1]);
    }
    BluetoothGattImpl::~BluetoothGattImpl() {
        ALOGI("******** ~BluetoothGattImpl ********\n");
        BluetoothGattLocalIntf::cleanUp();
        if (gatt_offload_transport != nullptr) {
            delete gatt_offload_transport;
            gatt_offload_transport = nullptr;
        }
        if (localGattCapabilities != nullptr) {
            delete localGattCapabilities;
            localGattCapabilities = nullptr;
        }
    }

    ::ndk::ScopedAStatus BluetoothGattImpl::init(const std::shared_ptr<IBluetoothGattCallback>& in_callback)  {
        ALOGI("******** Init ********\n");
        if (in_callback == nullptr) {
            return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
        }
        callback_ = in_callback;
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus BluetoothGattImpl::sendGlinkMessage(uint16_t msg_id, uint16_t encode_decode_val,
                         const std::string& payload, const char* func_name) {

        uint8_t header[PROTO_HEADER_LENGTH];
        header[0] = msg_id & 0xFF;
        header[1] = (msg_id >> 8);
        header[2] = encode_decode_val & 0xFF;
        header[3] = (encode_decode_val >> 8);
        uint16_t payload_length = payload.length();
        header[4] = payload_length & 0xFF;
        header[5] = (payload_length >> 8);
        std::string msgStr;
        msgStr.reserve(PROTO_HEADER_LENGTH + payload_length);
        msgStr.append(reinterpret_cast<const char*>(header), PROTO_HEADER_LENGTH);
        msgStr.append(payload);
        uint16_t total_len = msgStr.length();
        ALOGI("%s: Sending message (ID: 0x%x, Length: %u)", func_name, msg_id, total_len);
        for (uint16_t i = 0; i < total_len; i++) ALOGD("0x%x ", (uint8_t)msgStr[i]);
        if(gatt_offload_transport != nullptr){
            size_t bytes_written = 0;
            int ret = gatt_offload_transport->write((uint8_t*)(msgStr.c_str()), msgStr.length(), &bytes_written);
            if (ret < 0) {
                ALOGE("%s: gatt_offload glink write failed with error: %d", func_name, ret);
                return ::ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_TRANSACTION_FAILED));
            }
            ALOGD("%s: write payload bytes_written=%zu", func_name, bytes_written);
        }
        return ::ndk::ScopedAStatus::ok();
    }

#if USE_VENDOR_GATT_HAL
    ::ndk::ScopedAStatus BluetoothGattImpl::getGattCapabilities(::aidl::vendor::qti::hardware::bluetooth::gatt::GattCapabilities* _aidl_return) {
#else
    ::ndk::ScopedAStatus BluetoothGattImpl::getGattCapabilities(::aidl::android::hardware::bluetooth::gatt::GattCapabilities* _aidl_return) {
#endif
        ALOGI("******** getGattCapabilities ********\n");
        if (gatt_offload_transport != nullptr) {
            int ret_fd = gatt_offload_transport->open(GATT_OFFLOAD_GLINK_NODE);
            if (ret_fd <= 0) {
                ALOGE("%s: gatt_offload glink open failed with error: %d",__func__, ret_fd);
                std::atomic_exchange(&running_gatt_offload_ch_, false);
            } else {
                ALOGD("%s: gatt_offload glink open successful...start reader thread",__func__);
                 std::atomic_exchange(&running_gatt_offload_ch_, true);
                if (!glink_rx_thread || !glink_rx_thread->joinable()) {
                if (pipe2(wakeup_pipe_fd_, O_NONBLOCK) == -1) {
                        ALOGE("%s: Failed to create wakeup pipe",__func__);
                    }
                    glink_rx_thread = std::make_unique<std::thread>(&BluetoothGattImpl::processGattOffloadRx, this);
                }
            }
        }

        {
            std::lock_guard<std::mutex> Lock(gatt_cap_cond_mtx);
            ALOGI("%s : Entered lock capabilities set to false", __func__);
            gattCapabilitiesReady_ = false;
        }

        std::string payload;
        ::ndk::ScopedAStatus status = sendGlinkMessage(
            BT_LE_GET_GATT_CAPABILITIES,
            PROTO_ENC_DEC,
            payload,
            __func__
        );
        if (!status.isOk()) {
            return status;
        }

        std::unique_lock<std::mutex> lk(gatt_cap_cond_mtx);
        ALOGI("%s : TimeOut set to 2sec for capabilities response.", __func__);
        constexpr auto kTimeout = std::chrono::seconds(2);
        bool wait_status = gatt_cap_cond.wait_for(lk, kTimeout, [this] {
            return gattCapabilitiesReady_;
        });
        if (!wait_status) {
           ALOGE("%s: Timeout waiting for GATT capabilities response", __func__);
           return ::ndk::ScopedAStatus::fromExceptionCode(STATUS_TIMED_OUT);
        }

        std::lock_guard<std::mutex> cap_lock(gattCapabilities_mtx);
        _aidl_return->supportedGattClientProperties = localGattCapabilities->supported_gatt_client_properties;
        _aidl_return->supportedGattServerProperties = localGattCapabilities->supported_gatt_server_properties;

        return ::ndk::ScopedAStatus::ok();
    }

#if USE_VENDOR_GATT_HAL
    ::ndk::ScopedAStatus BluetoothGattImpl::registerService(int32_t in_sessionId, int32_t in_aclConnectionHandle,
                        int32_t in_attMtu, ::aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGatt::Role in_role,
                        const ::aidl::vendor::qti::hardware::bluetooth::gatt::Uuid& in_serviceUuid,
                        const std::vector<::aidl::vendor::qti::hardware::bluetooth::gatt::GattCharacteristic>& in_gattChars,
                        const ::aidl::android::hardware::contexthub::EndpointId& in_endpointId) {
         ALOGI("******** registerService ********\n");

        std::lock_guard<std::mutex> lock(stateTable_mtx);
        //Check if session with the provided session id is already open
        if(getState(in_sessionId) != STATE_CLOSED ){
            ALOGI("%s: Session is already in opening / closing / opened state for %lld. So, not opening again",
                  __func__,in_sessionId);
            return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
        }

        stateTable.push_back({in_sessionId, STATE_OPENING});
        dumpStateTable();
        //Send Register Service proto message
        std::string payload;
        gatt_offload_proto::gatt_register_service _gatt_register_service;
        _gatt_register_service.set_sessionid(in_sessionId);
        ALOGD("%s: in_sessionId : %d", __func__, in_sessionId);

        _gatt_register_service.set_aclconnectionhandle(in_aclConnectionHandle);
        ALOGD("%s: in_aclConnectionHandle : %ld", __func__, in_aclConnectionHandle);
        _gatt_register_service.set_attmtu(in_attMtu);
        ALOGD("%s: in_attMtu : %d", __func__, in_attMtu);
        _gatt_register_service.set_role((gatt_offload_proto::Role) in_role);
        ALOGD("%s: in_role : %d", __func__, in_role);
        gatt_offload_proto::Uuid *_service_uuid = _gatt_register_service.mutable_uuid();
        std::string s_uuid;
        for(const uint8_t c:in_serviceUuid.uuid) {
            s_uuid.push_back((char)c);
            ALOGD("%s: %x", __func__, c);
        }
        _service_uuid->set_uuid(s_uuid);
        ALOGD("%s: in_serviceUuid: %s", __func__, s_uuid.c_str());
        for(uint16_t a = 0; a < in_gattChars.size(); a++) {
            ALOGD("%s: gattChars size: %zu",__func__, in_gattChars.size());
            gatt_offload_proto::GattCharacteristic *_gatt_char = _gatt_register_service.add_gattcharacteristic();
            gatt_offload_proto::Uuid *_char_uuid = _gatt_char->mutable_uuid();
            std::string c_uuid;
            for(const uint8_t c : in_gattChars[a].uuid.uuid)
            {
                c_uuid.push_back((char)c);
                ALOGD("%s: %x", __func__, c);
            }
            _char_uuid->set_uuid(c_uuid);
            ALOGD("%s: in_gattChars[a].uuid.toString(): %s", __func__, c_uuid.c_str());
            _gatt_char->set_properties(in_gattChars[a].properties);
            ALOGD("%s: in_gattChars[a].properties : %d", __func__, in_gattChars[a].properties);
            _gatt_char->set_valuehandle(in_gattChars[a].valueHandle);
            ALOGD("%s: in_gattChars[a].valueHandle : %d", __func__, in_gattChars[a].valueHandle);
        }
        gatt_offload_proto::EndpointId *_end_point_id = _gatt_register_service.mutable_endpointid();
        _end_point_id->set_hubid((uint64_t)in_endpointId.hubId);
        ALOGD("%s: in_endpointId.hubId : %lld", __func__, in_endpointId.hubId);
        _end_point_id->set_endpointid((uint64_t)in_endpointId.id);
        ALOGD("%s: in_endpointId.id : %lld", __func__, in_endpointId.id);
        _gatt_register_service.SerializeToString(&payload);
        ALOGD("%s: payload length is %zu", __func__, payload.length());

        return sendGlinkMessage(
            BT_LE_GATT_REGISTER_SERVICE,
            PROTO_ENC_DEC,
            payload,
            __func__
        );
    }
#else
    ::ndk::ScopedAStatus BluetoothGattImpl::registerService(const ::aidl::android::hardware::bluetooth::gatt::GattSession& in_session) {
        ALOGI("******** registerService ********\n");

        std::lock_guard<std::mutex> lock(stateTable_mtx);
        //Check if session with the provided session id is already open
        if(getState(in_session.sessionId) != STATE_CLOSED ){
            ALOGI("%s: Session is already in opening / closing / opened state for %lld. So, not opening again",
                  __func__,in_session.sessionId);
            return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
        }

        stateTable.push_back({in_session.sessionId, STATE_OPENING});
        dumpStateTable();
        //Send Register Service proto message
        std::string payload;
        gatt_offload_proto::gatt_register_service _gatt_register_service;
        _gatt_register_service.set_sessionid(in_session.sessionId);
        ALOGD("%s: sessionId : %d", __func__, in_session.sessionId);

        _gatt_register_service.set_aclconnectionhandle(in_session.aclConnectionHandle);
        ALOGD("%s: aclConnectionHandle : %ld", __func__, in_session.aclConnectionHandle);
        _gatt_register_service.set_attmtu(in_session.attMtu);
        ALOGD("%s: attMtu : %d", __func__, in_session.attMtu);
        _gatt_register_service.set_role((gatt_offload_proto::Role) in_session.role);
        ALOGD("%s: in_session.role : %d", __func__, in_session.role);
        gatt_offload_proto::Uuid *_service_uuid = _gatt_register_service.mutable_uuid();
        std::string s_uuid;
        for(const uint8_t c:in_session.serviceUuid.uuid) {
            s_uuid.push_back((char)c);
            ALOGD("%s: %x", __func__, c);
        }
        _service_uuid->set_uuid(s_uuid);
        ALOGD("%s: serviceUuid: %s", __func__, s_uuid.c_str());
        for(uint16_t a = 0; a < in_session.characteristics.size(); a++) {
            ALOGD("%s: characteristics size: %zu",__func__, in_session.characteristics.size());
            gatt_offload_proto::GattCharacteristic *_gatt_char = _gatt_register_service.add_gattcharacteristic();
            gatt_offload_proto::Uuid *_char_uuid = _gatt_char->mutable_uuid();
            std::string c_uuid;
            for(const uint8_t c : in_session.characteristics[a].uuid.uuid)
            {
                c_uuid.push_back((char)c);
                ALOGD("%s: %x", __func__, c);
            }
            _char_uuid->set_uuid(c_uuid);
            ALOGD("%s: characteristics[a].uuid.toString(): %s", __func__, c_uuid.c_str());
            _gatt_char->set_properties(in_session.characteristics[a].properties);
            ALOGD("%s: characteristics[a].properties : %d", __func__, in_session.characteristics[a].properties);
            _gatt_char->set_valuehandle(in_session.characteristics[a].valueHandle);
            ALOGD("%s: characteristics[a].valueHandle : %d", __func__, in_session.characteristics[a].valueHandle);
        }
        gatt_offload_proto::EndpointId *_end_point_id = _gatt_register_service.mutable_endpointid();
        _end_point_id->set_hubid((uint64_t)in_session.endpointId.hubId);
        ALOGD("%s: endpointId.hubId : %lld", __func__, in_session.endpointId.hubId);
        _end_point_id->set_endpointid((uint64_t)in_session.endpointId.id);
        ALOGD("%s: endpointId.id : %lld", __func__, in_session.endpointId.id);
        _gatt_register_service.SerializeToString(&payload);
        ALOGD("%s: payload length is %zu", __func__, payload.length());

        return sendGlinkMessage(
            BT_LE_GATT_REGISTER_SERVICE,
            PROTO_ENC_DEC,
            payload,
            __func__
        );
    }
#endif

    ::ndk::ScopedAStatus BluetoothGattImpl::unregisterService(int32_t in_sessionId) {
        ALOGI("******** Unregister Service ********\n");
        std::lock_guard<std::mutex> lock(stateTable_mtx);
        if(getState(in_sessionId) != STATE_OPENED ){
            ALOGI("%s: Session is not opened for %ld. So, cannot close", __func__,in_sessionId);
            return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
        }
        if(getState(in_sessionId) == STATE_CLOSING ){
            ALOGI("%s: Session close is already in progress for session with Id - %ld.", __func__,in_sessionId);
            return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
        }

        updateState(stateTable, in_sessionId, STATE_CLOSING);
        dumpStateTable();

        //Send Unregister Service proto message
        std::string payload;
        gatt_offload_proto::gatt_unregister_service _gatt_unregister_service;
        _gatt_unregister_service.set_sessionid(in_sessionId);
        ALOGD("%s: in_sessionId : %d", __func__, in_sessionId);
        _gatt_unregister_service.SerializeToString(&payload);
        ALOGI("%s: payload length is %zu", __func__, payload.length());

        return sendGlinkMessage(
            BT_LE_GATT_UNREGISTER_SERVICE,
            PROTO_ENC_DEC,
            payload,
            __func__
        );
    }

    ::ndk::ScopedAStatus BluetoothGattImpl::clearServices(int32_t in_aclConnectionHandle) {
        ALOGI("******** Clear Services ********\n");

        // Send clear services proto message
        std::string payload;
        gatt_offload_proto::gatt_clear_services _gatt_clear_services;
        _gatt_clear_services.set_aclconnectionhandle(in_aclConnectionHandle);
        ALOGD("%s: in_aclConnectionHandle : %ld", __func__, in_aclConnectionHandle);
        _gatt_clear_services.SerializeToString(&payload);
        ALOGI("%s: payload length is %zu", __func__, payload.length());

        return sendGlinkMessage(
            BT_LE_GATT_CLEAR_SERVICES,
            PROTO_ENC_DEC,
            payload,
            __func__
        );
    }

    std::string BluetoothGattImpl::stateToString(State state) {
        switch (state) {
            case STATE_OPENING: return "STATE_OPENING";
            case STATE_OPENED: return "STATE_OPENED";
            case STATE_CLOSING: return "STATE_CLOSING";
            case STATE_CLOSED: return "STATE_CLOSED";
            default: return "UNKNOWN_STATE";
        }
    }

    BluetoothGattImpl::State BluetoothGattImpl::getState(int32_t session_id) {
        for (const auto& entry : stateTable) {
            if (entry.session_id == session_id) {
                ALOGD("%s: session_id: %ld is in %s state",__func__,session_id, stateToString(entry.state).c_str());
                return entry.state;
            }
        }
        ALOGD("%s: Entry with session_id %ld not found", __func__, session_id);
        return STATE_CLOSED;
    }

    void BluetoothGattImpl::dumpStateTable(){
        ALOGI("******** dumpStateTable ********\n");
        for (const auto& entry : stateTable) {
            ALOGD("session_id : %ld state : %s",entry.session_id, stateToString(entry.state).c_str());
        }
    }

    void BluetoothGattImpl::updateState(std::vector<StateTableEntry>& stateTable, int32_t session_id, State newState) {
        for (auto& entry : stateTable) {
            if (entry.session_id == session_id) {
                entry.state = newState;
                return;
            }
        }
        ALOGD("%s: Entry with session_id %ld not found",__func__, session_id);
    }

    void BluetoothGattImpl::removeEntry(int32_t session_id) {
        if(session_id == 0) {
            ALOGD("%s: Clearing the state table",__func__);
            stateTable.clear();
            return;
        }
        for (auto it = stateTable.begin(); it != stateTable.end(); ++it) {
            if (it->session_id == session_id) {
                stateTable.erase(it);
                ALOGD("%s: Entry with session_id %ld removed from state table",__func__, session_id);
                return;
            }
        }
        ALOGD("%s: Entry with session_id %ld not found",__func__, session_id);
    }

#if USE_VENDOR_GATT_HAL
    std::string BluetoothGattImpl::statusToString(aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status status) {
        switch (status) {
            case aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status::SUCCESS:
                 return "SUCCESS";
            case aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status::INVALID_ENDPOINT_ID:
                 return "INVALID_ENDPOINT_ID";
            case aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status::UNSUPPORTED_ROLE:
                 return "UNSUPPORTED_ROLE";
            case aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status::INSUFFICIENT_RESOURCES:
                 return "INSUFFICIENT_RESOURCES";
            case aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status::FAILURE:
                 return "FAILURE";
            default:
                 return "UNKNOWN_STATUS";
        }
    }
#else
    std::string BluetoothGattImpl::statusToString(aidl::android::hardware::bluetooth::gatt::Status status) {
        switch (status) {
            case aidl::android::hardware::bluetooth::gatt::Status::SUCCESS:
                 return "SUCCESS";
            case aidl::android::hardware::bluetooth::gatt::Status::INVALID_ENDPOINT_ID:
                 return "INVALID_ENDPOINT_ID";
            case aidl::android::hardware::bluetooth::gatt::Status::UNSUPPORTED_ROLE:
                 return "UNSUPPORTED_ROLE";
            case aidl::android::hardware::bluetooth::gatt::Status::INSUFFICIENT_RESOURCES:
                 return "INSUFFICIENT_RESOURCES";
            case aidl::android::hardware::bluetooth::gatt::Status::FAILURE:
                 return "FAILURE";
            default:
                 return "UNKNOWN_STATUS";
        }
    }
#endif


#if USE_VENDOR_GATT_HAL
std::string BluetoothGattImpl::errorToString(aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Error error) {
        switch (error) {
            case aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Error::DATABASE_OUT_OF_SYNC:
                 return "DATABASE_OUT_OF_SYNC";
            case aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Error::RESPONSE_TIMEOUT:
                 return "RESPONSE_TIMEOUT";
            case aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Error::PROTOCOL_VIOLATION:
                 return "PROTOCOL_VIOLATION";
            default:
                 return "UNKNOWN_ERROR";
        }
    }
#else
    std::string BluetoothGattImpl::errorToString(aidl::android::hardware::bluetooth::gatt::ErrorReport::Error error) {
        switch (error) {
            case aidl::android::hardware::bluetooth::gatt::ErrorReport::Error::DATABASE_OUT_OF_SYNC:
                 return "DATABASE_OUT_OF_SYNC";
            case aidl::android::hardware::bluetooth::gatt::ErrorReport::Error::RESPONSE_TIMEOUT:
                 return "RESPONSE_TIMEOUT";
            case aidl::android::hardware::bluetooth::gatt::ErrorReport::Error::PROTOCOL_VIOLATION:
                 return "PROTOCOL_VIOLATION";
            default:
                 return "UNKNOWN_ERROR";
        }
    }

#endif

    void BluetoothGattImpl::processGattOffloadRx(){
        ALOGD("%s: running_gatt_offload_ch_ is :: %d",__func__,running_gatt_offload_ch_.load());
        std::vector<uint8_t> readBuffer(MSG_SIZE_MAX);

        while (running_gatt_offload_ch_.load()) {
            int rcPoll = gatt_offload_transport->poll(wakeup_pipe_fd_[0], -1);
            if (!running_gatt_offload_ch_.load()) {
                 ALOGI("%s: Shutdown requested during poll, exiting thread.", __func__);
                 break;
            }
            if (-1 == rcPoll) {
                ALOGE("%s: Poll Failure",__func__);
                break;
            }
            int num = gatt_offload_transport->read(readBuffer.data(), MSG_SIZE_MAX);
            ALOGD("%s: num of bytes read from stream is :: %d",__func__, num);
            if (num < MSG_SIZE_MIN) {
                ALOGE("%s: Slate response is too short ::  %d",__func__, num);
            } else {
                uint16_t msg_id = (readBuffer[1] << 8) | readBuffer[0];
                uint16_t encode_decode =(readBuffer[3] << 8) | readBuffer[2];
                uint16_t length = (readBuffer[5] << 8) | readBuffer[4];
                std::string resBufferString;
                ALOGD("%s: msg_id : %d, length : %d",__func__, msg_id, length);
                switch (msg_id) {
                    case BT_LE_GATT_REGISTER_SERVICE_COMPLETE: {
                        ALOGD("%s: BT_LE_GATT_REGISTER_SERVICE_COMPLETE",__func__);
                        if(length > 0) {
                            resBufferString.assign(reinterpret_cast<char*>(readBuffer.data() + 6), length);

                            gatt_offload_proto::gatt_register_service_complete _gatt_register_service_complete;
                            bool ret = _gatt_register_service_complete.ParseFromString(resBufferString);
                            if(!ret) {
                                ALOGE("%s: Unable to parse string",__func__);
                                break;
                            }
                            int32_t session_id = 0;
                            if(_gatt_register_service_complete.has_sessionid()) {
                                session_id = _gatt_register_service_complete.sessionid();
                                ALOGI("%s: Session Id : %ld",__func__,session_id);
                            }
#if USE_VENDOR_GATT_HAL
                            aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status status =
                                aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status::SUCCESS;
#else
                            aidl::android::hardware::bluetooth::gatt::Status status =
                                aidl::android::hardware::bluetooth::gatt::Status::SUCCESS;
#endif
                            if(_gatt_register_service_complete.has_status()) {
#if USE_VENDOR_GATT_HAL
                                status = (aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status)
                                         _gatt_register_service_complete.status();
#else
                                status = (aidl::android::hardware::bluetooth::gatt::Status)
                                         _gatt_register_service_complete.status();
#endif
                                std::lock_guard<std::mutex> lock(stateTable_mtx);
                                if(status ==
#if USE_VENDOR_GATT_HAL
                                   aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Status::SUCCESS){
#else
                                   aidl::android::hardware::bluetooth::gatt::Status::SUCCESS){
#endif
                                    ALOGI("%s: Gatt Offload Register Successful for Session ID : %d",__func__,session_id);
                                    updateState(stateTable, session_id, STATE_OPENED);
                                    dumpStateTable();
                                }else{
                                    ALOGI("%s: Gatt Offload Register Unsuccessful for Session ID : %d",__func__,session_id);
                                    updateState(stateTable, session_id, STATE_CLOSED);
                                    dumpStateTable();
                                    removeEntry(session_id);
                                }
                                if (callback_ != nullptr) {
                                callback_->registerServiceComplete(session_id,status,statusToString(status));
                                }
                            }
                        }
                        break;
                    } case BT_LE_GATT_UNREGISTER_SERVICE_COMPLETE: {
                        ALOGD("%s: BT_LE_GATT_UNREGISTER_SERVICE_COMPLETE",__func__);
                        if(length > 0) {
                            resBufferString.assign(reinterpret_cast<char*>(readBuffer.data() + 6), length);

                            gatt_offload_proto::gatt_unregister_service_complete _gatt_unregister_service_complete;
                            bool ret = _gatt_unregister_service_complete.ParseFromString(resBufferString);
                            if(!ret) {
                                ALOGE("%s: Unable to parse string",__func__);
                                break;
                            }
                            int32_t session_id = 0;
                            if(_gatt_unregister_service_complete.has_sessionid()) {
                                session_id = _gatt_unregister_service_complete.sessionid();
                                ALOGI("%s: Session Id : %ld",__func__,session_id);
                            }
                            std::lock_guard<std::mutex> lock(stateTable_mtx);
                            if (callback_ != nullptr) {
                            callback_->unregisterServiceComplete(session_id,"SUCCESS");
                            }
                            updateState(stateTable, session_id, STATE_CLOSED);
                            dumpStateTable();
                            removeEntry(session_id);
                        }
                        break;
                    } case BT_LE_GATT_CLEAR_SERVICES_COMPLETE: {
                        ALOGD("%s: BT_LE_GATT_CLEAR_SERVICES_COMPLETE",__func__);
                        if(length > 0) {
                            resBufferString.assign(reinterpret_cast<char*>(readBuffer.data() + 6), length);

                            gatt_offload_proto::gatt_clear_services_complete _gatt_clear_services_complete;
                            bool ret = _gatt_clear_services_complete.ParseFromString(resBufferString);
                            if(!ret) {
                                ALOGE("%s: Unable to parse string",__func__);
                                break;
                            }
                            int32_t conn_handle = 0;
                            if(_gatt_clear_services_complete.has_aclconnectionhandle()) {
                                conn_handle = _gatt_clear_services_complete.aclconnectionhandle();
                                ALOGI("%s: Connection Handle : %ld",__func__,conn_handle);
                            }
                            std::lock_guard<std::mutex> lock(stateTable_mtx);
                            if (callback_ != nullptr) {
                            callback_->clearServicesComplete(conn_handle,"SUCCESS");
                            }
                            removeEntry(0);
                        }
                        break;
                    } case BT_LE_GATT_ERROR_REPORT: {
                        ALOGD("%s: BT_LE_GATT_ERROR_REPORT",__func__); 
                        if(length > 0) {
                            resBufferString.assign(reinterpret_cast<char*>(readBuffer.data() + 6), length);

                            gatt_offload_proto::gatt_error_report _gatt_error_report;
                            bool ret = _gatt_error_report.ParseFromString(resBufferString);
                            if(!ret) {
                                ALOGE("%s: Unable to parse string",__func__);
                                break;
                            }
                            int32_t conn_handle = 0;
                            int32_t local_cid = 0;
#if USE_VENDOR_GATT_HAL
                            aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Error error =
                                aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Error::UNKNOWN;
#else
                            aidl::android::hardware::bluetooth::gatt::ErrorReport::Error error =
                                aidl::android::hardware::bluetooth::gatt::ErrorReport::Error::UNKNOWN;
#endif
                            if(_gatt_error_report.has_aclconnectionhandle()) {
                                conn_handle = _gatt_error_report.aclconnectionhandle();
                                ALOGI("%s: Connection Handle : %ld",__func__,conn_handle);
                            }
                            if(_gatt_error_report.has_localcid()) {
                                local_cid = _gatt_error_report.localcid();
                                ALOGI("%s: Local CID : %ld",__func__,local_cid);
                            }
                            if(_gatt_error_report.has_error()) {
#if USE_VENDOR_GATT_HAL
                                error = (aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGattCallback::Error)
                                        _gatt_error_report.error();
#else
                                error = (aidl::android::hardware::bluetooth::gatt::ErrorReport::Error)
                                        _gatt_error_report.error();
#endif
                                ALOGI("%s: Error : %d",__func__,error);
                            }
                            if (callback_ != nullptr) {
#if USE_VENDOR_GATT_HAL
                                callback_->errorReport(conn_handle, local_cid, error, errorToString(error));
#else
                                aidl::android::hardware::bluetooth::gatt::ErrorReport errToReport;
                                errToReport.aclConnectionHandle = conn_handle;
                                errToReport.localCid = local_cid;
                                errToReport.error = error;
                                errToReport.reason = errorToString(error);
                                callback_->errorReport(errToReport);
#endif
                            }
                        }
                        break;
                    } case BT_LE_GET_GATT_CAPABILITIES_RSP: {
                        ALOGD("%s: BT_LE_GET_GATT_CAPABILITIES_RSP",__func__);
                        if(length > 0) {
                            resBufferString.assign(reinterpret_cast<char*>(readBuffer.data() + 6), length);

                            gatt_offload_proto::gatt_get_capabilities_rsp _gatt_get_capabilites;
                            bool ret = _gatt_get_capabilites.ParseFromString(resBufferString);
                            if(!ret) {
                                ALOGE("%s: Unable to parse string",__func__);
                                break;
                            }
                            std::lock_guard<std::mutex> cap_lock(gattCapabilities_mtx);
                            if(_gatt_get_capabilites.has_capabilities()) {
                                gatt_offload_proto::GattCapabilities *capabilities =
                                                  _gatt_get_capabilites.mutable_capabilities();
                                localGattCapabilities->supported_gatt_client_properties =
                                                     capabilities->suppgattclientproperties();
                                localGattCapabilities->supported_gatt_server_properties =
                                                     capabilities->suppgattserverproperties();
                                ALOGI("%s: Gatt Capabilites, suppGattClientProperties : %d",
                                __func__,localGattCapabilities->supported_gatt_client_properties);
                                ALOGI("%s: Gatt Capabilites, suppGattServerProperties : %d",
                                __func__,localGattCapabilities->supported_gatt_server_properties);
                            }
                            std::lock_guard<std::mutex> lock(gatt_cap_cond_mtx);
                            gattCapabilitiesReady_ = true;
                            gatt_cap_cond.notify_one();
                        }
                        break;
                    } default :
                        ALOGE("Gatt Offload: Unknown Event!!! msg_id: 0x%x", msg_id);
                        break;
                }
            }
        }
    }
}
namespace bluetooth {
namespace gatt {
static BluetoothGattLocalIntf* local_instance = NULL;
class BluetoothGattLocalIntfImpl: public BluetoothGattLocalIntf {
    public:
    BluetoothGattImpl* service_;
    BluetoothGattLocalIntfImpl(BluetoothGattImpl* service) {
        ALOGI("%s", __func__);
        service_ = service;
    }
    ~BluetoothGattLocalIntfImpl() {
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
BluetoothGattLocalIntf* BluetoothGattLocalIntf::getInterface() {
  return local_instance;
}
void BluetoothGattLocalIntf::init(BluetoothGattImpl* service) {
  ALOGI("%s", __func__);
  if (!local_instance) {
    local_instance = new BluetoothGattLocalIntfImpl(service);
  }
}
void BluetoothGattLocalIntf::cleanUp() {
  ALOGI("%s", __func__);
  if (local_instance) {
    local_instance->deInitialize();
    delete local_instance;
    local_instance = NULL;
  }
}
}
}
