/*
 * 파일명: QcLecocSocketOffloadBridge.cpp
 * 목적 및 기능:
 * - HLOS vendor/native Bluetooth stack에서 확보한 LECoC socket context를
 *   android.hardware.bluetooth.socket.IBluetoothSocket/default AIDL service로 전달한다.
 * - socket HAL은 SocketContext를 protobuf socket_open으로 변환하여 /dev/glink_pkt_bt_socket_hal_chnl로 ADSP에 전달한다.
 */

#include "QcLecocSocketOffloadBridge.h"                                      /* change(add)-hyungchul-20260511-0001: public bridge API 포함 */

#include <android/binder_auto_utils.h>                                       /* change(add)-hyungchul-20260511-0001: ndk::SpAIBinder */
#include <android/binder_manager.h>                                          /* change(add)-hyungchul-20260511-0001: AServiceManager_waitForService */
#include <android/binder_process.h>                                          /* change(add)-hyungchul-20260511-0001: ABinderProcess_startThreadPool */

#include <aidl/android/hardware/bluetooth/socket/BnBluetoothSocketCallback.h> /* change(add)-hyungchul-20260511-0001: callback base */
#include <aidl/android/hardware/bluetooth/socket/IBluetoothSocket.h>          /* change(add)-hyungchul-20260511-0001: socket offload AIDL */
#include <aidl/android/hardware/bluetooth/socket/SocketCapabilities.h>        /* change(add)-hyungchul-20260511-0001: capability type */
#include <aidl/android/hardware/bluetooth/socket/Status.h>                    /* change(add)-hyungchul-20260511-0001: status enum */

#include <inttypes.h>                                                        /* change(add)-hyungchul-20260511-0001: PRId64/PRIx64 */
#include <log/log.h>                                                         /* change(add)-hyungchul-20260511-0001: ALOGI/ALOGE */
#include <memory>                                                            /* change(add)-hyungchul-20260511-0001: std::shared_ptr */
#include <mutex>                                                             /* change(add)-hyungchul-20260511-0001: std::mutex */
#include <string>                                                            /* change(add)-hyungchul-20260511-0001: std::string */

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "qc_lecoc_offload_bridge"                                   /* change(add)-hyungchul-20260511-0001: logcat tag */

namespace socket_aidl = aidl::android::hardware::bluetooth::socket;

namespace {

class QcBluetoothSocketCallback final : public socket_aidl::BnBluetoothSocketCallback {
public:
    ndk::ScopedAStatus openedComplete(int64_t socketId,
                                      socket_aidl::Status status,
                                      const std::string &reason) override
    {
        ALOGI("change(add)-hyungchul-20260511-0001: openedComplete socketId=%" PRId64 ", status=%d, reason=%s",
              socketId,
              static_cast<int>(status),
              reason.c_str());
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus close(int64_t socketId,
                             const std::string &reason) override
    {
        ALOGI("change(add)-hyungchul-20260511-0001: close callback socketId=%" PRId64 ", reason=%s",
              socketId,
              reason.c_str());
        return ndk::ScopedAStatus::ok();
    }
};

std::mutex g_mutex;
std::shared_ptr<socket_aidl::IBluetoothSocket> g_socket_service;
std::shared_ptr<QcBluetoothSocketCallback> g_callback;
bool g_binder_threadpool_started = false;

static void startBinderThreadPoolLocked(void)
{
    if (!g_binder_threadpool_started) {
        ALOGI("change(add)-hyungchul-20260511-0001: start binder thread pool");
        ABinderProcess_startThreadPool();
        g_binder_threadpool_started = true;
    }
}

static std::shared_ptr<socket_aidl::IBluetoothSocket> waitForSocketServiceLocked(void)
{
    if (g_socket_service != nullptr) {
        ALOGD("change(add)-hyungchul-20260511-0001: reuse cached IBluetoothSocket service");
        return g_socket_service;
    }

    startBinderThreadPoolLocked();

    const std::string instance = std::string(socket_aidl::IBluetoothSocket::descriptor) + "/default";
    ALOGI("change(add)-hyungchul-20260511-0001: wait for service %s", instance.c_str());

    ndk::SpAIBinder binder(AServiceManager_waitForService(instance.c_str()));
    if (binder.get() == nullptr) {
        ALOGE("change(add)-hyungchul-20260511-0001: AServiceManager_waitForService failed: %s", instance.c_str());
        return nullptr;
    }

    g_socket_service = socket_aidl::IBluetoothSocket::fromBinder(binder);
    if (g_socket_service == nullptr) {
        ALOGE("change(add)-hyungchul-20260511-0001: IBluetoothSocket::fromBinder failed");
        return nullptr;
    }

    g_callback = ndk::SharedRefBase::make<QcBluetoothSocketCallback>();
    ndk::ScopedAStatus status = g_socket_service->registerCallback(g_callback);
    if (!status.isOk()) {
        ALOGE("change(add)-hyungchul-20260511-0001: registerCallback failed: %s", status.getDescription().c_str());
        g_socket_service.reset();
        g_callback.reset();
        return nullptr;
    }

    ALOGI("change(add)-hyungchul-20260511-0001: IBluetoothSocket service connected and callback registered");
    return g_socket_service;
}

static bool validateParams(const qc_lecoc_offload_params_t *params)
{
    if (params == nullptr) {
        ALOGE("change(add)-hyungchul-20260511-0001: params is null");
        return false;
    }
    if (params->socket_id <= 0) {
        ALOGE("change(add)-hyungchul-20260511-0001: invalid socket_id=%" PRId64, params->socket_id);
        return false;
    }
    if (params->local_cid == 0 || params->remote_cid == 0) {
        ALOGE("change(add)-hyungchul-20260511-0001: invalid cid local=%u remote=%u", params->local_cid, params->remote_cid);
        return false;
    }
    if (params->local_mtu == 0 || params->remote_mtu == 0) {
        ALOGE("change(add)-hyungchul-20260511-0001: invalid mtu local=%u remote=%u", params->local_mtu, params->remote_mtu);
        return false;
    }
    if (params->local_mps == 0 || params->remote_mps == 0) {
        ALOGE("change(add)-hyungchul-20260511-0001: invalid mps local=%u remote=%u", params->local_mps, params->remote_mps);
        return false;
    }
    return true;
}

}  // namespace

bool qc_lecoc_offload_init(void)
{
    ALOGI("change(add)-hyungchul-20260511-0001: qc_lecoc_offload_init enter");

    std::lock_guard<std::mutex> lock(g_mutex);
    std::shared_ptr<socket_aidl::IBluetoothSocket> service = waitForSocketServiceLocked();
    if (service == nullptr) {
        ALOGE("change(add)-hyungchul-20260511-0001: qc_lecoc_offload_init failed, service is null");
        return false;
    }

    socket_aidl::SocketCapabilities caps;
    ALOGI("change(add)-hyungchul-20260511-0001: call getSocketCapabilities");

    ndk::ScopedAStatus status = service->getSocketCapabilities(&caps);
    if (!status.isOk()) {
        ALOGE("change(add)-hyungchul-20260511-0001: getSocketCapabilities failed: %s", status.getDescription().c_str());
        return false;
    }

    ALOGI("change(add)-hyungchul-20260511-0001: Socket capabilities: LECoC sockets=%d mtu=%d RFCOMM sockets=%d maxFrameSize=%d",
          caps.leCocCapabilities.numberOfSupportedSockets,
          caps.leCocCapabilities.mtu,
          caps.rfcommCapabilities.numberOfSupportedSockets,
          caps.rfcommCapabilities.maxFrameSize);

    return true;
}

bool qc_lecoc_offload_open(const qc_lecoc_offload_params_t *params)
{
    ALOGI("change(add)-hyungchul-20260511-0001: qc_lecoc_offload_open enter");

    if (!validateParams(params)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    std::shared_ptr<socket_aidl::IBluetoothSocket> service = waitForSocketServiceLocked();
    if (service == nullptr) {
        ALOGE("change(add)-hyungchul-20260511-0001: qc_lecoc_offload_open failed, service is null");
        return false;
    }

    socket_aidl::SocketContext ctx;
    ctx.socketId = params->socket_id;
    ctx.name = params->socket_name != nullptr ? std::string(params->socket_name) : std::string("QC_BLE_T_LECOC");
    ctx.aclConnectionHandle = static_cast<int32_t>(params->acl_connection_handle);
    ctx.endpointId.hubId = static_cast<int64_t>(params->endpoint_hub_id != 0 ? params->endpoint_hub_id : QC_AWM_LECOC_ENDPOINT_HUB_ID);
    ctx.endpointId.id = static_cast<int64_t>(params->endpoint_id != 0 ? params->endpoint_id : QC_AWM_LECOC_ENDPOINT_ID);

    socket_aidl::LeCocChannelInfo lecoc;
    lecoc.localCid = static_cast<int32_t>(params->local_cid);
    lecoc.remoteCid = static_cast<int32_t>(params->remote_cid);
    lecoc.psm = static_cast<int32_t>(params->psm);
    lecoc.localMtu = static_cast<int32_t>(params->local_mtu);
    lecoc.remoteMtu = static_cast<int32_t>(params->remote_mtu);
    lecoc.localMps = static_cast<int32_t>(params->local_mps);
    lecoc.remoteMps = static_cast<int32_t>(params->remote_mps);
    lecoc.initialRxCredits = static_cast<int32_t>(params->initial_rx_credits);
    lecoc.initialTxCredits = static_cast<int32_t>(params->initial_tx_credits);

    ctx.channelInfo = socket_aidl::ChannelInfo::make<socket_aidl::ChannelInfo::leCocChannelInfo>(lecoc);

    ALOGI("change(add)-hyungchul-20260511-0001: LECoC offload params socketId=%" PRId64
          ", name=%s, acl=%u, psm=%u, localCid=%u, remoteCid=%u, localMtu=%u, remoteMtu=%u, localMps=%u, remoteMps=%u, rxCredits=%u, txCredits=%u, endpointHub=0x%" PRIx64 ", endpoint=0x%" PRIx64,
          params->socket_id,
          ctx.name.c_str(),
          params->acl_connection_handle,
          params->psm,
          params->local_cid,
          params->remote_cid,
          params->local_mtu,
          params->remote_mtu,
          params->local_mps,
          params->remote_mps,
          params->initial_rx_credits,
          params->initial_tx_credits,
          static_cast<uint64_t>(ctx.endpointId.hubId),
          static_cast<uint64_t>(ctx.endpointId.id));

    ndk::ScopedAStatus status = service->opened(ctx);
    if (!status.isOk()) {
        ALOGE("change(add)-hyungchul-20260511-0001: IBluetoothSocket.opened failed: %s", status.getDescription().c_str());
        return false;
    }

    ALOGI("change(add)-hyungchul-20260511-0001: IBluetoothSocket.opened AIDL call success, wait openedComplete callback");
    return true;
}

bool qc_lecoc_offload_close(int64_t socket_id)
{
    ALOGI("change(add)-hyungchul-20260511-0001: qc_lecoc_offload_close enter socketId=%" PRId64, socket_id);

    if (socket_id <= 0) {
        ALOGE("change(add)-hyungchul-20260511-0001: invalid socket_id=%" PRId64, socket_id);
        return false;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    std::shared_ptr<socket_aidl::IBluetoothSocket> service = waitForSocketServiceLocked();
    if (service == nullptr) {
        ALOGE("change(add)-hyungchul-20260511-0001: qc_lecoc_offload_close failed, service is null");
        return false;
    }

    ndk::ScopedAStatus status = service->closed(socket_id);
    if (!status.isOk()) {
        ALOGE("change(add)-hyungchul-20260511-0001: IBluetoothSocket.closed failed: %s", status.getDescription().c_str());
        return false;
    }

    return true;
}
