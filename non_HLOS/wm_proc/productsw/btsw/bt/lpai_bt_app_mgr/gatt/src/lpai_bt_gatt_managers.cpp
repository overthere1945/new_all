/**************************************************************************
 * @file     lpai_bt_app_mgr.h
 * @brief    LPAI BT APP Manager header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#include "lpai_bt_gatt_managers.h"

#define GATT_MGR_DEBUG(...) do {} while(0)
// #define GATT_MGR_DEBUG printk /** TODO : Comment in the prod build */
#define GATT_MGR_PRINT_ERR printk

namespace bok::gatt
{
    GattSessionManagerImpl &GattSessionManagerImpl::Instance()
    {
        static GattSessionManagerImpl instance;
        return instance;
    }

    void GattSessionManagerImpl::QapiRegisterApp(GattClientAppRegisterParams &&params)
    {
        for (int i = 0; i < kMaxApps; ++i)
        {
            if (client_apps_[i].delegate == nullptr)
            {
                client_apps_[i].endpoint_id = params.endpoint_id;
                client_apps_[i].delegate = &params.delegate;
                return;
            }
        }
        GATT_MGR_DEBUG("Client App registration failed: max limit reached\n");
    }

    void GattSessionManagerImpl::QapiRegisterApp(GattServerAppRegisterParams &&params)
    {
        for (int i = 0; i < kMaxApps; ++i)
        {
            if (server_apps_[i].delegate == nullptr)
            {
                server_apps_[i].endpoint_id = params.endpoint_id;
                server_apps_[i].delegate = &params.delegate;
                GATT_MGR_DEBUG("Registered Server App: endpoint_id=%lld\n", params.endpoint_id);
                return;
            }
        }
        GATT_MGR_DEBUG("Server App registration failed: max limit reached\n");
    }

    void GattSessionManagerImpl::QapiRegisterApp(GattClientAppRegisterParams &&client_params,
                                             GattServerAppRegisterParams &&server_params)
    {
        QapiRegisterApp(std::move(client_params));
        QapiRegisterApp(std::move(server_params));
        GATT_MGR_DEBUG("Registered Client+Server Apps: client_endpoint_id=%lld, server_endpoint_id=%lld\n",
               client_params.endpoint_id, server_params.endpoint_id);
    }

    void GattSessionManagerImpl::QapiAttributesAcquiredConfirmation(QapiAttributesAcquiredConfirmationParams &&params)
    {
        uapp_gatt_session_registered_rsp_t *rsp = (uapp_gatt_session_registered_rsp_t *)app_mgr_rpc_get_empty_msg();
        rsp->session_id = params.session_id;
        rsp->status = (int32_t)params.status;
        int32_t len = sizeof(uapp_gatt_session_registered_rsp_t);

        /* free data buffer */
        if (params.attributes.data())
            free((void *)params.attributes.data());

        GATT_MGR_DEBUG("QapiAttributesAcquiredConfirmation called %d %d\n", params.status, rsp->status);
        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_SESSION_REGISTERED_RESP, len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiAttributesAcquiredConfirmation\n");
        }
    }

    void GattSessionManagerImpl::QapiReleaseAttributes(QapiReleaseAttributesParams &&params) {
        uapp_gatt_session_unregister_ind_t *ind = (uapp_gatt_session_unregister_ind_t *)app_mgr_rpc_get_empty_msg();
        int len = sizeof(uapp_gatt_session_unregister_ind_t);
        ind->session_id = params.session_id;
        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_SESSION_UNREGISTERED_IND, len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiAttributesReleasedConfirmation\n");
        }
    }

    void GattSessionManagerImpl::QapiAttributesReleasedConfirmation(QapiAttributesReleasedConfirmationParams &&params)
    {
        int32_t len = sizeof(uapp_gatt_session_unregistered_t);

        GATT_MGR_DEBUG("QapiAttributesReleasedConfirmation called for sessionId %d status %d\n", params.session_id, params.status);
        /** Send the response to offload manager */
        uapp_gatt_session_unregistered_t *rsp = (uapp_gatt_session_unregistered_t *)app_mgr_rpc_get_empty_msg();
        rsp->session_id = params.session_id;
        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_SESSION_UNREGISTERED_RESP, len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiAttributesReleasedConfirmation\n");
        }
    }

    // Optional: accessors for testing or usage
    GattAppClientDelegate *GattSessionManagerImpl::GetClientDelegate(int64_t endpoint_id)
    {
        for (int i = 0; i < kMaxApps; ++i)
        {
            if (client_apps_[i].delegate && client_apps_[i].endpoint_id == endpoint_id)
                return client_apps_[i].delegate;
        }
        return nullptr;
    }

    GattAppServerDelegate *GattSessionManagerImpl::GetServerDelegate(int64_t endpoint_id)
    {
        for (int i = 0; i < kMaxApps; ++i)
        {
            if (server_apps_[i].delegate && server_apps_[i].endpoint_id == endpoint_id)
                return server_apps_[i].delegate;
        }
        return nullptr;
    }

    void GattSessionManagerImpl::RegisterEndPointsWithStack()
    {
        for (int i = 0; i < kMaxApps; ++i)
        {
            if (client_apps_[i].delegate)
            {
                int e = app_mgr_rpc_send_endpt_msg_adsp(client_apps_[i].endpoint_id, UAPP_GATT_APP_REG_REQ, 0, 0);
                if (e != 0)
                {
                    GATT_MGR_PRINT_ERR("failed to send QapiRegisterApp\n");
                }
            }
            /* server registration */
            if (server_apps_[i].delegate)
            {
                int e = app_mgr_rpc_send_endpt_msg_adsp(server_apps_[i].endpoint_id, UAPP_GATT_APP_REG_REQ, 0, 0);
                if (e != 0)
                {
                    GATT_MGR_PRINT_ERR("failed to send QapiRegisterApp\n");
                }
            }
        }
    }

    template <typename T>
    std::span<T> *createDynamicSpan(const std::span<T> &original)
    {
        // Create a new span on the heap
        return new std::span<T>(original);
    }

    GattClientManagerImpl &GattClientManagerImpl::Instance()
    {
        static GattClientManagerImpl instance;
        return instance;
    }

    void GattClientManagerImpl::QapiNotifyComplete(QapiNotifyCompleteParams &&params)
    {
        /* TBD : do we have message for this */

        /* free the span */
        free((void *)params.data.data());
        GATT_MGR_DEBUG("QapiNotifyComplete called\n");
    }

    void GattClientManagerImpl::QapiIndicateComplete(QapiIndicateCompleteParams && /*params*/)
    {
        GATT_MGR_DEBUG("QapiIndicateComplete called\n");
    }

    void GattClientManagerImpl::QapiReadRequest(QapiReadRequestParams &&params)
    {
        uapp_gatt_char_read_req_t *ind = (uapp_gatt_char_read_req_t *)app_mgr_rpc_get_empty_msg();
        ind->handle = params.attribute_id;
        ind->session_id = params.session_id;
        ind->context = reinterpret_cast<uint32_t>(createDynamicSpan(params.buffer));
        uint16_t total_data_len = sizeof(uapp_gatt_char_read_req_t);

        GATT_MGR_DEBUG("QapiReadRequest Sent to Q6 SessionId %d, AttrHandle %d\n", ind->session_id, ind->handle);

        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_CLIENT_READ_CHAR_REQ, total_data_len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiReadRequest\n");
        }
    }

    void GattClientManagerImpl::QapiWriteWithoutResponseRequest(QapiWriteWithoutResponseRequestParams &&params)
    {
        GATT_MGR_DEBUG("QapiWriteWithoutResponseRequest called\n");
        uapp_gatt_char_write_req_t *ind = (uapp_gatt_char_write_req_t *)app_mgr_rpc_get_empty_msg();
        ind->handle = params.attribute_id;
        ind->context = reinterpret_cast<uint32_t>(createDynamicSpan(params.data));
        ind->session_id = params.session_id;
        ind->val_len = params.data.size();
        ind->writeWithoutRsp = true;
        memcpy(ind->val, params.data.data(), ind->val_len);
        uint16_t total_data_len = sizeof(uapp_gatt_char_write_req_t) + ind->val_len;

        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_CLIENT_WRITE_CHAR_CMD, total_data_len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiWriteWithoutResponseRequest\n");
        }
    }

    void GattClientManagerImpl::QapiWriteRequest(QapiWriteRequestParams &&params)
    {
        GATT_MGR_DEBUG("QapiWriteRequest called\n");
        uapp_gatt_char_write_req_t *ind = (uapp_gatt_char_write_req_t *)app_mgr_rpc_get_empty_msg();
        ind->handle = params.attribute_id;
        ind->context = reinterpret_cast<uint32_t>(createDynamicSpan(params.data));
        ind->session_id = params.session_id;
        ind->val_len = params.data.size();
        ind->writeWithoutRsp = false;
        memcpy(ind->val, params.data.data(), ind->val_len);
        uint16_t total_data_len = sizeof(uapp_gatt_char_write_req_t) + ind->val_len;

        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_CLIENT_WRITE_CHAR_REQ, total_data_len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiWriteRequest\n");
        }
    }

    GattServerManagerImpl &GattServerManagerImpl::Instance()
    {
        static GattServerManagerImpl instance;
        return instance;
    }

    void GattServerManagerImpl::QapiNotifyRequest(QapiNotifyRequestParams &&params)
    {
        GATT_MGR_DEBUG("QapiNotifyRequest called\n");
        uapp_gatt_char_notif_ind_t *ind = (uapp_gatt_char_notif_ind_t *)app_mgr_rpc_get_empty_msg();
        ind->handle = params.attribute_id;
        ind->session_id = params.session_id;
        ind->val_len = params.data.size();
        ind->context = reinterpret_cast<uint32_t>(createDynamicSpan(params.data));
        GATT_MGR_DEBUG("ind->context : %u", ind->context);
        uint16_t total_data_len = ind->val_len + sizeof(uapp_gatt_char_notif_ind_t);

        memcpy(ind->val,
               params.data.data(),
               params.data.size());

        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_SERVER_NOTIFICATION_REQ, total_data_len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiNotifyRequest\n");
        }
    }

    void GattServerManagerImpl::QapiIndicateRequest(QapiIndicateRequestParams && /*params*/)
    {
        GATT_MGR_DEBUG("QapiIndicateRequest called\n");
    }

    void GattServerManagerImpl::QapiReadComplete(QapiReadCompleteParams &&params)
    {
        GATT_MGR_DEBUG("QapiReadComplete called\n");
        uapp_gatt_char_read_rsp_t *ind = (uapp_gatt_char_read_rsp_t *)app_mgr_rpc_get_empty_msg();
        ind->handle = params.attribute_id;
        ind->session_id = params.session_id;
        ind->val_len = params.data.size();
        uint16_t total_data_len = ind->val_len + sizeof(uapp_gatt_char_read_rsp_t);

        memcpy(ind->val,
               params.data.data(),
               params.data.size());

        /** free the span data */
        free(params.data.data());

        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_SERVER_READ_CHAR_IND_RESP, total_data_len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiReadComplete\n");
        }
    }

    void GattServerManagerImpl::QapiWriteWithoutResponseComplete(QapiWriteWithoutResponseCompleteParams &&params)
    {
        GATT_MGR_DEBUG("QapiWriteWithoutResponseComplete called\n");
        uapp_gatt_char_write_rsp_t *ind = (uapp_gatt_char_write_rsp_t *)app_mgr_rpc_get_empty_msg();
        ind->handle = params.attribute_id;
        ind->session_id = params.session_id;
        ind->result = (gatt_uapp_status_t)params.status;
        uint16_t total_data_len = sizeof(uapp_gatt_char_write_rsp_t);

        free((void *)params.data.data());
        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_SERVER_WRITE_CHAR_IND_RESP, total_data_len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiWriteWithoutResponseComplete\n");
        }
    }

    void GattServerManagerImpl::QapiWriteComplete(QapiWriteCompleteParams &&params)
    {
        GATT_MGR_DEBUG("QapiWriteComplete called\n");
        uapp_gatt_char_write_rsp_t *ind = (uapp_gatt_char_write_rsp_t *)app_mgr_rpc_get_empty_msg();
        ind->handle = params.attribute_id;
        ind->session_id = params.session_id;
        ind->result = (gatt_uapp_status_t)params.status;
        uint16_t total_data_len = sizeof(uapp_gatt_char_write_rsp_t);

        free((void *)params.data.data());
        int e = app_mgr_rpc_send_endpt_msg_adsp(0, UAPP_GATT_SERVER_WRITE_CHAR_IND_RESP, total_data_len, 0);
        if (e != 0)
        {
            GATT_MGR_PRINT_ERR("failed to send QapiWriteComplete\n");
        }
    }

}

extern "C"
{
    static uint8_t *get_copy(uint8_t *data, uint16_t data_len)
    {
        uint8_t *newData = (uint8_t *)malloc(data_len);
        if (newData == NULL)
        {
            ERR_FATAL("malloc failed", 0, 0, 0);
        }
        memcpy((uint8_t *)newData, data, data_len);
        return newData;
    }

    static void handle_client_evts(uint16_t opcode, bok::gatt::GattAppClientDelegate *cDelegate, uint8_t *data, uint16_t data_len)
    {
        switch (opcode)
        {
        case UAPP_GATT_CLIENT_READ_CHAR_RESP:
        {
            uapp_gatt_char_read_rsp_t *ind = (uapp_gatt_char_read_rsp_t *)data;
            bok::gatt::QapiReadCompleteParams params;
            params.session_id = ind->session_id;
            params.attribute_id = ind->handle;
            params.status = (bok::gatt::Status)ind->result;

            /** Return the address of the buffer */
            std::span<std::byte> *span_ptr = reinterpret_cast<std::span<std::byte> *>(ind->context);
            if (span_ptr == NULL)
            {
                /** error fatal */
                ERR_FATAL("invalid span ptr", 0, 0, 0);
            }
            memcpy(span_ptr->data(), ind->val, ind->val_len);
            params.data = std::span<std::byte>(span_ptr->data(), ind->val_len);
            delete span_ptr;
            GATT_MGR_DEBUG("QapiReadComplete span byte_len %d\n", params.data.size());
            cDelegate->QapiReadComplete(std::move(params));
        }
        break;
        case UAPP_GATT_CLIENT_WRITE_CHAR_RESP:
        {
            uapp_gatt_char_write_rsp_t *ind = (uapp_gatt_char_write_rsp_t *)data;
            bok::gatt::QapiWriteWithoutResponseCompleteParams params;
            params.session_id = ind->session_id;
            params.attribute_id = ind->handle;
            params.status = (bok::gatt::Status)ind->result;
            GATT_MGR_DEBUG("ind->type : %d\n", ind->writeWithoutRsp);
            std::span<std::byte> *span_ptr = reinterpret_cast<std::span<std::byte> *>(ind->context);
            if (span_ptr == NULL)
            {
                /** error fatal */
                ERR_FATAL("invalid span ptr", 0, 0, 0);
            }
            params.data = *span_ptr;
            delete span_ptr;
            if(ind->writeWithoutRsp)
                cDelegate->QapiWriteWithoutResponseComplete(std::move(params));
            else {
                bok::gatt::QapiWriteCompleteParams wwr_params;
                memcpy(&wwr_params, &params, sizeof(params));
                cDelegate->QapiWriteComplete(std::move(wwr_params));
            }
        }
        break;
        case UAPP_GATT_CLIENT_NOTIFICATION_IND:
        {
            uapp_gatt_char_notif_ind_t *ind = (uapp_gatt_char_notif_ind_t *)data;
            bok::gatt::QapiNotifyReceivedParams params;
            params.session_id = ind->session_id;
            params.attribute_id = ind->handle;

            uint8_t *data = get_copy(ind->val, ind->val_len);
            params.data = std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(data),
                ind->val_len);
            cDelegate->QapiNotifyReceived(std::move(params));
        }
        break;
        }
    }

    static void handle_server_evts(uint16_t opcode, bok::gatt::GattAppServerDelegate *sDelegate, uint8_t *data, uint16_t data_len)
    {
        switch (opcode)
        {
        case UAPP_GATT_SERVER_READ_CHAR_IND:
        {
            uapp_gatt_char_read_req_t *ind = (uapp_gatt_char_read_req_t *)data;
            bok::gatt::QapiReadReceivedParams params;
            params.session_id = ind->session_id;
            params.attribute_id = ind->handle;

            int len = ind->mtu - 1;
            uint8_t *data = (uint8_t *)malloc(len); /** 1 byte opcode  */
            if (data == NULL)
            {
                ERR_FATAL("malloc failed", 0, 0, 0);
            }

            params.buffer = std::span<std::byte>(
                reinterpret_cast<std::byte *>(data),
                len);
            sDelegate->QapiReadReceived(std::move(params));
        }
        break;
        case UAPP_GATT_SERVER_WRITE_CHAR_IND:
        {
            uapp_gatt_char_write_req_t *ind = (uapp_gatt_char_write_req_t *)data;
            bok::gatt::QapiWriteWithoutResponseReceivedParams params;
            params.session_id = ind->session_id;
            params.attribute_id = ind->handle;

            uint8_t *data = (uint8_t *)malloc(ind->val_len);
            if (ind->val_len && data == NULL)
            {
                ERR_FATAL("malloc failed", 0, 0, 0);
            }
            memcpy(data, ind->val, ind->val_len);
            params.data = std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(data),
                ind->val_len);
            if (ind->writeWithoutRsp)
                sDelegate->QapiWriteWithoutResponseReceived(std::move(params));
            else
            {
                bok::gatt::QapiWriteReceivedParams rparams;
                memcpy(&rparams, &params, sizeof(rparams));
                sDelegate->QapiWriteReceived(std::move(rparams));
            }
        }
        break;
        case UAPP_GATT_SERVER_NOTIFICATION_RESP:
        {
            uapp_gatt_char_notif_ind_rsp *ind = (uapp_gatt_char_notif_ind_rsp *)data;
            bok::gatt::QapiNotifyCompleteParams params;
            params.session_id = ind->session_id;
            params.attribute_id = ind->handle;
            params.status = (bok::gatt::Status)ind->result;
            GATT_MGR_DEBUG("ind->context : %u", ind->context);

            /** Return the address of the buffer */
            std::span<std::byte>* span_ptr = reinterpret_cast<std::span<std::byte>*>(ind->context);
            if(span_ptr == NULL) {
                /** error fatal */
                ERR_FATAL("invalid span ptr", 0, 0, 0);
            }

            params.data = *span_ptr;
            delete span_ptr;
            sDelegate->QapiNotifyComplete(std::move(params));
        }
        break;
        }
    }
    bool gatt_mgr_check_endpoint(uint64_t ep_id)
    {
        bok::gatt::GattAppClientDelegate *cDelegate = bok::gatt::GattSessionManagerImpl::Instance().GetClientDelegate(ep_id);
        bok::gatt::GattAppServerDelegate *sDelegate = bok::gatt::GattSessionManagerImpl::Instance().GetServerDelegate(ep_id);
        if (cDelegate != nullptr || sDelegate != nullptr)
            return true;
        return false;
    }

    void gatt_mgr_evt_handler(uint16_t opcode, uint64_t ep_id, uint8_t *data, uint16_t data_len)
    {
        GATT_MGR_DEBUG("gatt_mgr_evt_handler:opcode[%d]\n", opcode);
        bok::gatt::GattAppClientDelegate *cDelegate = bok::gatt::GattSessionManagerImpl::Instance().GetClientDelegate(ep_id);
        bok::gatt::GattAppServerDelegate *sDelegate = bok::gatt::GattSessionManagerImpl::Instance().GetServerDelegate(ep_id);
        switch (opcode)
        {
        case UAPP_GATT_APP_REG_RESP:
        {
            /* nothing to do here */
        }
        break;
        case UAPP_GATT_SESSION_REGISTERED_REQ:
        {
            if (cDelegate == nullptr && sDelegate == nullptr)
            {
                /* invalid ep id received */
                GATT_MGR_PRINT_ERR("Invalid endpoint id : %lld\n", ep_id);
                return;
            }
            bok::gatt::QapiAttributesAcquiredParams params;
            /* fill the params */
            // fill_attributes_acquired_params(&params, data, data_len);

            gatt_register_service reg_service = gatt_register_service_init_zero;
            uint8_t service_uuid[GATT_OFFLOAD_UUID_BYTE_LEN] = {0};
            char_list_holder_t char_list_holder = {.num_chars = 0};

            bool result = decode_gatt_register_service((uint8_t *)data, data_len,
                                                       &reg_service, service_uuid, &char_list_holder);
            GATT_MGR_DEBUG("decode_gatt_register_service[%d]\n", result);
            if (!result)
            {
                return;
            }
            params.session_id = reg_service.sessionId;
            params.notify_mtu = reg_service.attMtu - 3;   /** /** -3 bytes : 1 byte opcode 2 bytes handle */
            params.read_mtu = reg_service.attMtu - 1; /** -1 byte : opcode  */
            params.write_mtu = reg_service.attMtu - 3;    /** -3 bytes : 1 byte opcode and 2 bytes handle */
            memcpy(params.service_uuid.data(), service_uuid, GATT_OFFLOAD_UUID_BYTE_LEN);

            /* fill the attributes now */
            bok::gatt::AttributeInfo *infos = (bok::gatt::AttributeInfo *)malloc(sizeof(bok::gatt::AttributeInfo) * char_list_holder.num_chars);
            if (infos == NULL)
            {
                ERR_FATAL("malloc failed", 0, 0, 0);
            }
            GATT_MGR_DEBUG("manager info: %x\n", infos);

            std::span<bok::gatt::AttributeInfo> s(infos, char_list_holder.num_chars);
            for (uint8_t i = 0; i < char_list_holder.num_chars; ++i)
            {
                infos[i].attribute_id = char_list_holder.chars[i].valueHandle;
                memcpy(infos[i].characteristic_uuid.data(), char_list_holder.chars[i].uuid, GATT_OFFLOAD_UUID_BYTE_LEN);
                int32_t properties = char_list_holder.chars[i].properties;
                infos[i].supports_read = (properties & 0x02) != 0;
                infos[i].supports_write_with_response = (properties & 0x08) != 0;
                infos[i].supports_write = (properties & 0x04) != 0;
                infos[i].supports_notify = (properties & 0x10) != 0;
                infos[i].supports_indicate = (properties & 0x20) != 0;
            }
            params.attributes = s;

            if (cDelegate != nullptr)
            {
                cDelegate->QapiAttributesAcquired(std::move(params));
            }
            else
            {
                sDelegate->QapiAttributesAcquired(std::move(params));
            }
        }
        break;
        case UAPP_GATT_SESSION_UNREGISTERED_REQ:
        {
            /* attributes released */
            if (cDelegate == nullptr && sDelegate == nullptr)
            {
                /* invalid ep id received */
                GATT_MGR_PRINT_ERR("Invalid endpoint id : %lld\n", ep_id);
                return;
            }
            bok::gatt::QapiAttributesReleasedParams params;
            uapp_gatt_session_unregister_ind_t *ind = (uapp_gatt_session_unregister_ind_t *)data;
            params.session_id = ind->session_id;

            if (cDelegate != nullptr)
            {
                cDelegate->QapiAttributesReleased(std::move(params));
            }
            else
            {
                sDelegate->QapiAttributesReleased(std::move(params));
            }
        }
        break;
        case UAPP_GATT_CLIENT_READ_CHAR_RESP:
        case UAPP_GATT_CLIENT_WRITE_CHAR_RESP:
        case UAPP_GATT_CLIENT_NOTIFICATION_IND:
        {
            /* fall through*/
            if (cDelegate == nullptr)
            {
                GATT_MGR_DEBUG("no app with ep_id %lld\n", ep_id);
                return;
            }
            handle_client_evts(opcode, cDelegate, data, data_len);
        }
        break;
        case UAPP_GATT_SERVER_READ_CHAR_IND:
        case UAPP_GATT_SERVER_WRITE_CHAR_IND:
        case UAPP_GATT_SERVER_NOTIFICATION_RESP:
        {
            /* fall through*/
            if (sDelegate == nullptr)
            {
                GATT_MGR_DEBUG("no app with ep_id %lld\n", ep_id);
                return;
            }
            handle_server_evts(opcode, sDelegate, data, data_len);
        }
        break;
        }
    }
    void gatt_mgr_handle_bt_on_evt()
    {
        bok::gatt::GattSessionManagerImpl::Instance().RegisterEndPointsWithStack();
    }
}
