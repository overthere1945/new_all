/**************************************************************************
 * @file     lpai_bt_app_mgr.h
 * @brief    LPAI BT APP Manager header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_GATT_MANAGERS_H
#define LPAI_BT_GATT_MANAGERS_H
#include "gatt_managers.h"
#include <zephyr/sys/printk.h>
#include "lpai_bt_gatt_msgs.h"
#include "lpai_bt_app_mgr_rpc.h"
#include <span>
#include <stdlib.h>
#include <string.h>
#include "err.h"
#include "lpai_bt_gatt_proto.h"
#include "lpai_bt_gatt.pb.h"
#include "types.h"
#include "gatt_interface.h"

extern std::span<std::byte> g_session_span;

namespace bok::gatt {
struct ClientAppEntry
    {
        int64_t endpoint_id;
        GattAppClientDelegate *delegate;
    };

    struct ServerAppEntry
    {
        int64_t endpoint_id;
        GattAppServerDelegate *delegate;
    };
 // GattSessionManagerImpl: maintains registered client/server apps
    class GattSessionManagerImpl : public GattSessionManager
    {
    public:
        static GattSessionManagerImpl &Instance();

        void QapiRegisterApp(GattClientAppRegisterParams &&params) override;

        void QapiRegisterApp(GattServerAppRegisterParams &&params) override;

        void QapiRegisterApp(GattClientAppRegisterParams &&client_params,
                         GattServerAppRegisterParams &&server_params) override;

        void QapiAttributesAcquiredConfirmation(QapiAttributesAcquiredConfirmationParams && params) override;

        void QapiAttributesReleasedConfirmation(QapiAttributesReleasedConfirmationParams && params) override;

        void QapiReleaseAttributes(QapiReleaseAttributesParams&& params) override;

        ~GattSessionManagerImpl() override = default;

        // Optional: accessors for testing or usage
        GattAppClientDelegate *GetClientDelegate(int64_t endpoint_id);

        GattAppServerDelegate *GetServerDelegate(int64_t endpoint_id);
        
        void RegisterEndPointsWithStack();

    private:
        GattSessionManagerImpl() = default;
        GattSessionManagerImpl(const GattSessionManagerImpl &) = delete;
        GattSessionManagerImpl &operator=(const GattSessionManagerImpl &) = delete;

        static constexpr int kMaxApps = 2;
        ClientAppEntry client_apps_[kMaxApps] = {};
        ServerAppEntry server_apps_[kMaxApps] = {};
    };

    // GattClientManagerImpl: maintains registered client apps
    class GattClientManagerImpl : public GattClientManager
    {
    public:
        static GattClientManagerImpl &Instance();

        void QapiNotifyComplete(QapiNotifyCompleteParams &&params) override;
        
        void QapiIndicateComplete(QapiIndicateCompleteParams && /*params*/) override;

        void QapiReadRequest(QapiReadRequestParams &&params) override;
        
        void QapiWriteWithoutResponseRequest(QapiWriteWithoutResponseRequestParams &&params) override;
        
        void QapiWriteRequest(QapiWriteRequestParams && /*params*/) override;

        ~GattClientManagerImpl() override = default;

    private:
        GattClientManagerImpl() = default;
        GattClientManagerImpl(const GattClientManagerImpl &) = delete;
        GattClientManagerImpl &operator=(const GattClientManagerImpl &) = delete;
    };

    // GattServerManagerImpl: maintains registered server apps
    class GattServerManagerImpl : public GattServerManager
    {
    public:
        static GattServerManagerImpl &Instance();
        

        void QapiNotifyRequest(QapiNotifyRequestParams &&params) override;
        
        void QapiIndicateRequest(QapiIndicateRequestParams && /*params*/) override;

        void QapiReadComplete(QapiReadCompleteParams &&params) override;
        
        void QapiWriteWithoutResponseComplete(QapiWriteWithoutResponseCompleteParams &&params) override;
        
        void QapiWriteComplete(QapiWriteCompleteParams && /*params*/) override;

        ~GattServerManagerImpl() override = default;

    private:
        GattServerManagerImpl() = default;
        GattServerManagerImpl(const GattServerManagerImpl &) = delete;
        GattServerManagerImpl &operator=(const GattServerManagerImpl &) = delete;
    };

}
#endif