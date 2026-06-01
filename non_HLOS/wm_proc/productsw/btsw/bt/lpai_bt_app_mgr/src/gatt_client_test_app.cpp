/**************************************************************************
 * @file     gatt_client_test_app.cpp
 * @brief    Gatt Client Test App Source File.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#include "gatt_interface.h"
#include <zephyr/sys/printk.h>
#include "lpai_bt_gatt_managers.h"
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>

#define TEST_APP_DEBUG(...) do {} while(0)
// #define TEST_APP_DEBUG printk
/** if throughput proc is not active, then print */
#define TEST_APP_PRINT(fmt, ...) \
    do { \
        if (!pending_proc.active) { \
            printk(fmt, ##__VA_ARGS__); \
        } \
    } while (0)
// #define TEST_APP_PRINT printk
#define TEST_APP_PRINT_ERROR printk

#define MAX_ATTRIBUTES 4
#define MAX_MTU 517

using namespace bok::gatt;
struct gatt_server_app_attribute
{
    uint16_t attribute_id;
    uint32_t session_id;
    uint8_t *val;
    uint16_t len;
};

typedef enum {
    server_tx, 
    client_tx,
    server_rx,
    client_rx,
}proc_type_t;

struct throughput_proc {
    bool active;
    proc_type_t type;
    Status error;
    uint32_t session_id;
    uint16_t num_packets;
    uint16_t attribute_id;
    uint16_t pending_packets;
    uint16_t packet_size;
    uint64_t start_time;
    uint64_t end_time;
};

static throughput_proc pending_proc;
double last_throughput;
gatt_server_app_attribute attributes[MAX_ATTRIBUTES];


static void send_notify_request(uint32_t session_id, uint16_t attribute_id, uint16_t val_len);
static void send_write_request(uint32_t session_id, uint16_t attribute_id, uint16_t val_len, uint8_t write_cmd);


static void init_attributes()
{
    for (uint8_t i = 0; i < MAX_ATTRIBUTES; i++)
    {
        if (attributes[i].val)
        {
            free(attributes[i].val);
        }
    }
    memset(attributes, 0, sizeof(attributes));
}

extern "C" void gatt_app_deinit()
{
    TEST_APP_PRINT("gatt_app_deinit\n");
    init_attributes();
    last_throughput = 0;
    memset (&pending_proc, 0, sizeof(pending_proc));
}

static int8_t get_empty_idx()
{
    for (uint8_t i = 0; i < MAX_ATTRIBUTES; i++)
    {
        if (attributes[i].session_id == 0 && attributes[i].attribute_id == 0)
        {
            return i;
        }
    }
    return -1;
}

static gatt_server_app_attribute *get_attribute_from_att_id(uint16_t att_id , uint32_t session_id)
{
    for (uint8_t i = 0; i < MAX_ATTRIBUTES; i++)
    {
        if (attributes[i].attribute_id == att_id && attributes[i].session_id == session_id)
        {
            return attributes + i;
        }
    }
    return NULL;
}


static void add_attribute(uint32_t session_id, AttributeInfo *info)
{
    gatt_server_app_attribute *attribute;
    int8_t idx = get_empty_idx();

    if (idx == -1)
    {
        /** no more offloads are possible */
        TEST_APP_PRINT_ERROR("no more offloads possible\n");
        return;
    }
    attribute = attributes + idx;
    attribute->attribute_id = info->attribute_id;
    attribute->session_id = session_id;
    attribute->val = (uint8_t *)malloc(MAX_MTU);
    if (attribute->val == NULL)
    {
        ERR_FATAL("malloc failed", 0, 0, 0);
    }
    memset(attribute->val, 0, MAX_MTU);
    attribute->len = 0;
}
static void remove_attributes(uint32_t session_id)
{
    for (uint8_t i = 0; i < MAX_ATTRIBUTES; i++)
    {
        if (attributes[i].session_id == session_id)
        {
            if (attributes[i].val)
                free(attributes[i].val);
            memset(attributes + i, 0, sizeof(gatt_server_app_attribute));
        }
    }
}

/** code to print results of throughput pending_procedure */
static void print_proc_result() {
    TEST_APP_DEBUG("[%llu][%llu][%d][%d]\n", pending_proc.start_time, pending_proc.end_time, pending_proc.num_packets, pending_proc.packet_size);
    /** print calculated throughput */
    if(pending_proc.error != Status::kOk) {
        TEST_APP_PRINT_ERROR("Error occured during throughput : %d", (int)pending_proc.error);
        return;
    }
    double throughput = (double)(pending_proc.num_packets * pending_proc.packet_size * 8) / (double)(pending_proc.end_time - pending_proc.start_time);
    last_throughput = throughput;
    TEST_APP_PRINT("Throughput : %.2f kbps\n", throughput);
}

namespace bok::gatt
{
    class TestGattAppClientDelegate : public GattAppClientDelegate
    {
        public:
        
        void QapiNotifyReceived(QapiNotifyReceivedParams &&params) override
        {
            TEST_APP_PRINT("QapiNotifyReceived called with: sessionId %d handle %d len : %d\n", params.session_id, params.attribute_id, params.data.size());
            for (uint16_t i = 0; i < params.data.size(); i++)
            {
                TEST_APP_DEBUG("%02x", ((uint8_t *)(params.data.data()))[i]);
            }
            // TEST_APP_PRINT("\n");
            GattClientManager &clManager = GattClientManagerImpl::Instance();
            QapiNotifyCompleteParams resp_params;
            resp_params.attribute_id = params.attribute_id;
            resp_params.session_id = params.session_id;
            resp_params.data = params.data;
            resp_params.status = Status::kOk;
            clManager.QapiNotifyComplete(std::move(resp_params));
            
            /** Check Throughput procedure */
            if(pending_proc.active &&
               pending_proc.type == client_rx && 
               pending_proc.session_id == params.session_id &&
               pending_proc.attribute_id == params.attribute_id) {
                if(pending_proc.pending_packets == pending_proc.num_packets) {
                    /** this is the first packet */
                    pending_proc.start_time = k_uptime_get();
                }
                if(pending_proc.packet_size != params.data.size()) {
                    pending_proc.error = Status::kInvalidOperation;
                }
                else pending_proc.pending_packets--;
                if(pending_proc.pending_packets == 0) {
                    pending_proc.end_time = k_uptime_get();
                    pending_proc.active = false;
                    print_proc_result();
                }
            }
        }

        void QapiIndicateReceived(QapiIndicateReceivedParams &&params) override
        {
            TEST_APP_PRINT("QapiIndicateReceived called with: \n");
        }

        void QapiReadComplete(QapiReadCompleteParams &&params) override
        {
            TEST_APP_PRINT("QapiReadComplete called with: sessionId %d AttrHandle %d data_len %d status : %d\n",
                   params.session_id, params.attribute_id, params.data.size(), (int)params.status);
            for (size_t i = 0; i < params.data.size(); ++i)
            {
                TEST_APP_PRINT("%02x", ((uint8_t *)(params.data.data()))[i]);
            }
            TEST_APP_PRINT("\n");
            if (params.data.data() != NULL)
                free((void *)params.data.data());
        }

        void QapiWriteWithoutResponseComplete(QapiWriteWithoutResponseCompleteParams &&params) override
        {
            TEST_APP_PRINT("QapiWriteWithoutResponseComplete called with: sessionId %d AttrHandle %d Status %d\n",
                   params.session_id, params.attribute_id, params.status);

            if (params.data.data() != NULL)
                free((void *)params.data.data());
            
            /** Check Throughput procedure */
            if(pending_proc.active &&
               pending_proc.type == client_tx && 
               pending_proc.session_id == params.session_id &&
               pending_proc.attribute_id == params.attribute_id) {
                if(params.status == Status::kOk) {
                    pending_proc.pending_packets--;

                    if(pending_proc.pending_packets == 0) {
                        pending_proc.end_time = k_uptime_get();
                        pending_proc.active = false;
                        print_proc_result();
                    }
                    else
                        send_write_request(pending_proc.session_id, pending_proc.attribute_id, pending_proc.packet_size, true);
                }
                else {
                    pending_proc.error = params.status;
                }
            }
        }

        void QapiWriteComplete(QapiWriteCompleteParams &&params) override
        {
            TEST_APP_PRINT("QapiWriteComplete called with: sessionId %d AttrHandle %d Status %d\n",
                   params.session_id, params.attribute_id, params.status);
            if (params.data.data() != NULL)
                free((void *)params.data.data());
        }
        // Implement all pure virtuals from GattAppDelegate
        void QapiAttributesAcquired(QapiAttributesAcquiredParams &&params) override
        {
            TEST_APP_PRINT("QapiAttributesAcquired %d\n", params.session_id);
            AttributeInfo *info = (AttributeInfo *)params.attributes.data();
            int len = params.attributes.size();
            
            TEST_APP_DEBUG("len : %d info : %x\n", len, info);
            for (int i = 0; i < len; i++)
            {
                TEST_APP_PRINT("attribute id: %d \n", info->attribute_id);
                info++;
            }

            QapiAttributesAcquiredConfirmationParams p = {params.session_id, params.attributes, Status::kOk};
            GattSessionManager &manager = GattSessionManagerImpl::Instance();
            manager.QapiAttributesAcquiredConfirmation(std::move(p));
        }
        void QapiAttributesReleased(QapiAttributesReleasedParams &&params) override
        {
            TEST_APP_PRINT("AttributesReleased SessionId %d\n", params.session_id);
            QapiAttributesReleasedConfirmationParams p = {params.session_id, Status::kOk};
            
            GattSessionManager &manager = GattSessionManagerImpl::Instance();
            manager.QapiAttributesReleasedConfirmation(std::move(p));
            /** set the pending proc in error state */
            if(pending_proc.active &&
               pending_proc.session_id == params.session_id) {
                pending_proc.error = Status::kSessionClosed;
            }
        }
        TestGattAppClientDelegate() : GattAppClientDelegate() {}
    };

    class TestGattAppServerDelegate : public GattAppServerDelegate
    {
    public:
        void QapiAttributesAcquired(QapiAttributesAcquiredParams &&params) override
        {
            TEST_APP_PRINT("QapiAttributesAcquired %d\n", params.session_id);
            AttributeInfo *info = (AttributeInfo *)params.attributes.data();
            int len = params.attributes.size();
            
            for (int i = 0; i < len; i++)
            {
                TEST_APP_PRINT("attribute id: %d\n", info->attribute_id);
                /** add attribute to server db */
                add_attribute(params.session_id, info);
                info++;
            }

            QapiAttributesAcquiredConfirmationParams p = {params.session_id, params.attributes, (Status)0};
            GattSessionManager &manager = GattSessionManagerImpl::Instance();
            manager.QapiAttributesAcquiredConfirmation(std::move(p));
        }

        void QapiAttributesReleased(QapiAttributesReleasedParams &&params) override
        {
            TEST_APP_PRINT("AttributesReleased SessionId %d\n", params.session_id);
            remove_attributes(params.session_id);

            QapiAttributesReleasedConfirmationParams p = {params.session_id, Status::kOk};
            GattSessionManager &manager = GattSessionManagerImpl::Instance();
            manager.QapiAttributesReleasedConfirmation(std::move(p));

            /** set the pending proc in error state */
            if(pending_proc.active &&
               pending_proc.session_id == params.session_id) {
                pending_proc.error = Status::kSessionClosed;
            }
        }

        void QapiNotifyComplete(QapiNotifyCompleteParams &&params) override
        {
            TEST_APP_PRINT("Notify complete received handle : %d status : %d\n", params.attribute_id, (int)params.status);
            if (params.data.data() != NULL)
                free((void *)params.data.data());
            
            /** Check Throughput procedure */
            if(pending_proc.active &&
               pending_proc.type == server_tx && 
               pending_proc.session_id == params.session_id &&
               pending_proc.attribute_id == params.attribute_id) {
                if(params.status == Status::kOk) {
                    pending_proc.pending_packets--;

                if(pending_proc.pending_packets == 0) {
                    pending_proc.end_time = k_uptime_get();
                    pending_proc.active = false;
                    print_proc_result();
                }
                else
                    send_notify_request(pending_proc.session_id, pending_proc.attribute_id, pending_proc.packet_size);
                }
                else {
                    pending_proc.error = params.status;
                }
            }
        }

        void QapiIndicateComplete(QapiIndicateCompleteParams &&params) override
        {
            TEST_APP_PRINT("Indicate complete received\n");
        }

        void QapiReadReceived(QapiReadReceivedParams &&params) override
        {
            TEST_APP_PRINT("Read request received on attribute handle : %d\n", params.attribute_id);

            /** Check if the attribute exists */
            gatt_server_app_attribute *local_att = get_attribute_from_att_id(params.attribute_id,params.session_id);
            Status status = Status::kOk;
            GattServerManager &manager = GattServerManagerImpl::Instance();
            
            if (local_att == NULL)
            {
                TEST_APP_PRINT_ERROR("No attribute with attribute id : %d\n", params.attribute_id);
                status = Status::kGATTInvalidHandle;
                QapiReadCompleteParams rParams = {
                params.session_id,
                params.attribute_id,
                std::span<std::byte>(params.buffer.data(), 0),
                status};
                manager.QapiReadComplete(std::move(rParams));
                return ;
            }
            memcpy(params.buffer.data(), local_att->val, local_att->len);
            QapiReadCompleteParams rParams = {
                params.session_id,
                params.attribute_id,
                std::span<std::byte>(params.buffer.data(), local_att->len),
                status};
            manager.QapiReadComplete(std::move(rParams));
        }

        void QapiWriteWithoutResponseReceived(QapiWriteWithoutResponseReceivedParams &&params) override
        {
            TEST_APP_PRINT("Write request received on session id : %d attribute handle : %d len : %d\n", params.session_id, params.attribute_id, params.data.size());
            /** Check if the attribute exists */
            gatt_server_app_attribute *local_att = get_attribute_from_att_id(params.attribute_id,params.session_id);
            Status status = Status::kOk;
            if (local_att == NULL)
            {
                TEST_APP_PRINT_ERROR("No attribute with attribute id : %d\n", params.attribute_id);
                status = Status::kGATTInvalidHandle;
            }
            else
            {
                memcpy(local_att->val, params.data.data(), params.data.size());
                local_att->len = params.data.size();
            }
            GattServerManager &manager = GattServerManagerImpl::Instance();
            QapiWriteWithoutResponseCompleteParams rParams = {params.session_id,
                                           params.attribute_id,
                                           *reinterpret_cast<std::span<std::byte> *>(&params.data),
                                           status};
            manager.QapiWriteWithoutResponseComplete(std::move(rParams));
            
            /** Check Throughput procedure */
            if(pending_proc.active &&
               pending_proc.type == server_rx && 
               pending_proc.session_id == params.session_id &&
               pending_proc.attribute_id == params.attribute_id) {
                if(pending_proc.pending_packets == pending_proc.num_packets) {
                    /** this is the first packet */
                    pending_proc.start_time = k_uptime_get();
                }
                if(pending_proc.packet_size != params.data.size()) {
                    pending_proc.error = Status::kInvalidOperation;
                }
                else pending_proc.pending_packets--;
                if(pending_proc.pending_packets == 0) {
                    pending_proc.end_time = k_uptime_get();
                    pending_proc.active = false;
                    print_proc_result();
                }
            }
        }

        void QapiWriteReceived(QapiWriteReceivedParams &&params)
        {
            TEST_APP_PRINT("WriteWithResponse request received on attribute handle : %d\n", params.attribute_id);
            /** Check if the attribute exists */
            gatt_server_app_attribute *local_att = get_attribute_from_att_id(params.attribute_id,params.session_id);
            Status status = Status::kOk;
            if (local_att == NULL)
            {
                TEST_APP_PRINT_ERROR("No attribute with attribute id : %d\n", params.attribute_id);
                status = Status::kGATTInvalidHandle;
            }
            else
            {
                memcpy(local_att->val, params.data.data(), params.data.size());
                local_att->len = params.data.size();
            }
            GattServerManager &manager = GattServerManagerImpl::Instance();
            QapiWriteCompleteParams rParams = {params.session_id,
                                           params.attribute_id,
                                           *reinterpret_cast<std::span<std::byte> *>(&params.data),
                                           status};
            manager.QapiWriteComplete(std::move(rParams));
        }

        TestGattAppServerDelegate() : GattAppServerDelegate() {}
    };
}


static bool validate_length(int length) {
    if(length > MAX_MTU)
    {
        TEST_APP_PRINT_ERROR("value length more than supported max mtu : %d\n", MAX_MTU);
        return false;
    }
    return true;
}


static void send_read_request(uint32_t session_id, uint16_t attribute_id)
{
    GattClientManager &clManager = GattClientManagerImpl::Instance();
    QapiReadRequestParams params;

    params.session_id = session_id;
    params.attribute_id = attribute_id;
    uint8_t *data = (uint8_t *)malloc(MAX_MTU);
    if (data == NULL)
    {
        ERR_FATAL("malloc failed", 0, 0, 0);
    }
    params.buffer = std::span<std::byte>(
        reinterpret_cast<std::byte *>(data),
        MAX_MTU);
    TEST_APP_PRINT("Read Request sessionId %d attributeId %d span_len %d\n",
           params.session_id, params.attribute_id, params.buffer.size());
    clManager.QapiReadRequest(std::move(params));
}

static void send_write_with_rsp_req(uint32_t session_id, uint16_t attribute_id, uint16_t val_len)
{
    if(!validate_length(val_len))return;
    GattClientManager &clManager = GattClientManagerImpl::Instance();
    QapiWriteRequestParams params;

    params.session_id = session_id;
    params.attribute_id = attribute_id;
    uint8_t *data = (uint8_t *)malloc(val_len);
    if (val_len && data == NULL)
    {
        ERR_FATAL("malloc failed", 0, 0, 0);
    }
    for (int i = 0; i < val_len; i++)
        data[i] = (i % 10); /** just for testing purpose */
    params.data = std::span<std::byte>(
        reinterpret_cast<std::byte *>(data),
        val_len);

    clManager.QapiWriteRequest(std::move(params));
}

static void send_write_request(uint32_t session_id, uint16_t attribute_id, uint16_t val_len, uint8_t write_cmd)
{
    if (!write_cmd)
    {
        send_write_with_rsp_req(session_id, attribute_id, val_len);
        return;
    }
    
    if(!validate_length(val_len))return;
    GattClientManager &clManager = GattClientManagerImpl::Instance();
    QapiWriteWithoutResponseRequestParams params;

    params.session_id = session_id;
    params.attribute_id = attribute_id;
    uint8_t *data = (uint8_t *)malloc(val_len);
    if (val_len && data == NULL)
    {
        ERR_FATAL("malloc failed", 0, 0, 0);
    }
    for (int i = 0; i < val_len; i++)
        data[i] = (i % 10); /** just for testing purpose */
    params.data = std::span<std::byte>(
        reinterpret_cast<std::byte *>(data),
        val_len);
    TEST_APP_DEBUG("Write Request sessionId %d attributeId %d span_len %d\n",
           params.session_id, params.attribute_id, params.data.size());
    clManager.QapiWriteWithoutResponseRequest(std::move(params));
}

static void send_notify_request(uint32_t session_id, uint16_t attribute_id, uint16_t val_len)
{
    /** Check if the attribute exists */
    gatt_server_app_attribute *local_att = get_attribute_from_att_id(attribute_id , session_id);
    if (local_att == NULL)
    {
        TEST_APP_PRINT_ERROR("No attribute with attribute id : %d or Session Id not found ", attribute_id);
        return;
    }

    if(!validate_length(val_len))return;

    GattServerManager &srManager = GattServerManagerImpl::Instance();
    QapiNotifyRequestParams params;

    params.session_id = session_id;
    params.attribute_id = attribute_id;

    uint8_t *data = (uint8_t *)malloc(val_len);
    if (val_len != 0 && data == NULL)
    {
        ERR_FATAL("malloc failed", 0, 0, 0);
    }
    for (int i = 0; i < val_len; i++)
        data[i] = (i % 10); /** just for testing purpose */
    
    /** set the value of the local attribute */
    memcpy(local_att->val, data, val_len);
    local_att->len = val_len;
    params.data = std::span<std::byte>(
        reinterpret_cast<std::byte *>(data),
        val_len);
    TEST_APP_DEBUG("Notification Request sessionId %d attributeId %d span_len %d\n",
           params.session_id, params.attribute_id, params.data.size());
    srManager.QapiNotifyRequest(std::move(params));
}

static TestGattAppClientDelegate cDelegate;
static TestGattAppServerDelegate sDelegate;
static void register_app()
{
    GattClientAppRegisterParams cParams = {(int64_t)0xFAFAFAFAFAFAFA08, cDelegate};
    GattServerAppRegisterParams sParams = {(int64_t)0xFAFAFAFAFAFAFA07, sDelegate};
    GattSessionManager &manager = GattSessionManagerImpl::Instance();
    manager.QapiRegisterApp(std::move(cParams), std::move(sParams));
}

extern "C"
{
    void register_dummy_app()
    {
        register_app();
    }
    void send_gatt_read_request(uint32_t session_id, uint16_t attribute_id)
    {
        send_read_request(session_id, attribute_id);
    }
    void send_gatt_write_request(uint32_t session_id, uint16_t attribute_id, uint16_t val_len, uint8_t write_cmd)
    {
        send_write_request(session_id, attribute_id, val_len, write_cmd);
    }
    void gatt_server_app_notif_send(uint32_t sessionId, uint16_t attrHandle, uint16_t val_len)
    {
        send_notify_request(sessionId, attrHandle, val_len);
    }

    static void perform_throughput_proc(void)
    {
         if (pending_proc.type == server_tx)
             send_notify_request(pending_proc.session_id, pending_proc.attribute_id, pending_proc.packet_size);
         else if(pending_proc.type == client_tx)
             send_write_request(pending_proc.session_id, pending_proc.attribute_id, pending_proc.packet_size, true);
    }
    static void fill_throughput_proc(uint32_t sessionId, uint16_t attrHandle, uint16_t pktCnt, uint16_t valLen, proc_type_t type) {
        pending_proc.active = true;
        pending_proc.attribute_id = attrHandle;
        pending_proc.error = Status::kOk;
        pending_proc.num_packets = pktCnt;
        pending_proc.packet_size = valLen;
        pending_proc.pending_packets = pktCnt;
        pending_proc.session_id = sessionId;
        pending_proc.type = type;
    }
    void gatt_client_app_tx_bulk_transfer(uint32_t sessionId, uint16_t attrHandle, uint16_t pktCnt, uint16_t valLen) {
        
        fill_throughput_proc(sessionId, attrHandle, pktCnt, valLen, client_tx);
        pending_proc.start_time = k_uptime_get();
        perform_throughput_proc();
    }

    void write_duration_on_remote(uint32_t sessionId, uint16_t attrHandle)
    {
        GattClientManager &clManager = GattClientManagerImpl::Instance();
        QapiWriteWithoutResponseRequestParams params;

        params.session_id = sessionId;
        params.attribute_id = attrHandle;
        uint8_t *data = (uint8_t *)malloc(4);
        if (data == NULL)
        {
            ERR_FATAL("malloc failed", 0, 0, 0);
        }

        data[0] = 0x3C;
        data[1] = 0x00;
        data[2] = 0x00;
        data[3] = 0x00;

        params.data = std::span<std::byte>(
            reinterpret_cast<std::byte *>(data),
            4);
        TEST_APP_PRINT("value updated on remote\n");
        clManager.QapiWriteWithoutResponseRequest(std::move(params));
    }

    void gatt_client_app_rx_bulk_transfer(uint32_t sessionId, uint16_t attrHandle, uint16_t pktCnt, uint16_t valLen) {
        write_duration_on_remote(sessionId, attrHandle);
        fill_throughput_proc(sessionId, attrHandle, pktCnt, valLen, client_rx);
    }

    void gatt_server_app_tx_bulk_transfer(uint32_t sessionId, uint16_t attrHandle, uint16_t pktCnt, uint16_t valLen) {
        fill_throughput_proc(sessionId, attrHandle, pktCnt, valLen, server_tx);
        pending_proc.start_time = k_uptime_get();
        perform_throughput_proc();
    }

    void gatt_server_app_rx_bulk_transfer(uint32_t sessionId, uint16_t attrHandle, uint16_t pktCnt, uint16_t valLen) {
        fill_throughput_proc(sessionId, attrHandle, pktCnt, valLen, server_rx);
    }

    void release_attributes(uint32_t session_id) {
        /** get the instance of SessionManager  */
        GattSessionManager &manager = GattSessionManagerImpl::Instance();
        QapiReleaseAttributesParams params;
        params.session_id = session_id;

        manager.QapiReleaseAttributes(std::move(params));
        /** remove the attributes */
        remove_attributes(session_id);
    }

    void print_last_throughput() {
        TEST_APP_PRINT("Throughput : %.2f kbps\n", last_throughput);
    }

    void exit_throughput_proc() {
        pending_proc.active = false;
        TEST_APP_PRINT("Throughput procedure ended\n");
    }
}
