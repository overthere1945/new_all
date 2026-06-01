/**
 * @file qapi_ancs_profile.cpp
 * @brief Implementation of ANCS Profile using bok::gatt interface
 */

#include "qapi_ancs_profile.h"
#include "gatt_interface.h"
#include "lpai_bt_gatt_managers.h"
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <cstring>
#include <vector>

#define ANCS_DEBUG(...) do {} while(0)
#define ANCS_PRINT printk

/* Standard ANCS UUIDs */
// Notification Source: 9FBF120D-6301-42D9-8C58-25E699A21DBD
static const uint8_t ANCS_UUID_NS[] = {0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C, 0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F};
// Control Point: 69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9
static const uint8_t ANCS_UUID_CP[] = {0xD9, 0xD9, 0xAA, 0xFD, 0xBD, 0x9B, 0x21, 0x98, 0xA8, 0x49, 0xE1, 0x45, 0xF3, 0xD8, 0xD1, 0x69};
// Data Source: 22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB
static const uint8_t ANCS_UUID_DS[] = {0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE, 0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22};

/* ANCS Command IDs */
#define ANCS_CMD_GET_NOTIF_ATTR 0
#define ANCS_CMD_GET_APP_ATTR   1

using namespace bok::gatt;

/* Global state for the profile */
static QapiAncsCallback_t g_ancs_callback = NULL;

namespace bok::gatt {

    class AncsClientDelegate : public GattAppClientDelegate
    {
    public:
        uint32_t session_id = 0;
        uint16_t handle_ns = 0;
        uint16_t handle_ds = 0;
        uint16_t handle_cp = 0;
        
        // Helper to check UUID match with std::array
        bool is_uuid_match(const std::array<uint8_t, 16>& uuid_arr, const uint8_t* target_uuid) 
        {
             return std::memcmp(uuid_arr.data(), target_uuid, 16) == 0;
        }

        void QapiAttributesAcquired(QapiAttributesAcquiredParams &&params) override
        {
            ANCS_DEBUG("ANCS: Attributes Acquired for Session %d\n", params.session_id);
            this->session_id = params.session_id;
            
            AttributeInfo *info = (AttributeInfo *)params.attributes.data();
            int len = params.attributes.size();

            // Iterate through attributes to find ANCS handles
            for (int i = 0; i < len; i++)
            {
                if (is_uuid_match(info[i].characteristic_uuid, ANCS_UUID_NS)) 
                {
                    ANCS_DEBUG("Found NS Handle: %d\n", info[i].attribute_id);
                    handle_ns = info[i].attribute_id;
                }
                else if (is_uuid_match(info[i].characteristic_uuid, ANCS_UUID_DS)) 
                {
                    ANCS_DEBUG("Found DS Handle: %d\n", info[i].attribute_id);
                    handle_ds = info[i].attribute_id;
                }
                else if (is_uuid_match(info[i].characteristic_uuid, ANCS_UUID_CP)) 
                {
                    ANCS_DEBUG("Found CP Handle: %d\n", info[i].attribute_id);
                    handle_cp = info[i].attribute_id;
                }
            }

            if (handle_ns && handle_ds && handle_cp && g_ancs_callback) 
            {
                QapiAncsEventData data;
                data.acquired.notif_source_handle = handle_ns;
                data.acquired.data_source_handle = handle_ds;
                data.acquired.control_point_handle = handle_cp;
                g_ancs_callback(QAPI_GATT_ANCS_AQCUIRED, &data);
            }
            
            QapiAttributesAcquiredConfirmationParams p = {params.session_id, params.attributes, Status::kOk};
            GattSessionManagerImpl::Instance().QapiAttributesAcquiredConfirmation(std::move(p));
        }

        void QapiAttributesReleased(QapiAttributesReleasedParams &&params) override
        {
            ANCS_DEBUG("ANCS: Attributes Released Session %d\n", params.session_id);
            if (session_id == params.session_id && g_ancs_callback)
            {
                session_id = 0;
                handle_ns = 0; handle_ds = 0; handle_cp = 0;
                g_ancs_callback(QAPI_GATT_ANCS_RELEASED, NULL);
            }
            
            QapiAttributesReleasedConfirmationParams p = {params.session_id, Status::kOk};
            GattSessionManagerImpl::Instance().QapiAttributesReleasedConfirmation(std::move(p));
        }

        void QapiNotifyReceived(QapiNotifyReceivedParams &&params) override
        {
            if (params.attribute_id == handle_ns) 
            {
                 ANCS_DEBUG("ANCS: NS Notification\n");
                 if (params.data.size() >= 8) 
                 {
                    uint8_t *p = (uint8_t *)params.data.data();
                    QapiAncsEventData evt;
                    evt.ns_ind.evt_id = p[0];
                    evt.ns_ind.evt_flag = p[1];
                    evt.ns_ind.cat_id = p[2];
                    evt.ns_ind.cat_cnt = p[3];
                    evt.ns_ind.notification_uid = (uint32_t)p[4] | ((uint32_t)p[5] << 8) | ((uint32_t)p[6] << 16) | ((uint32_t)p[7] << 24);
                    if (g_ancs_callback)
                    {
                        g_ancs_callback(QAPI_GATT_ANCS_NS_IND, &evt);
                    }
                }
            }
            else if (params.attribute_id == handle_ds) 
            {
                ANCS_DEBUG("ANCS: DS Notification\n");
                // Parse Data Source Data
                // If Command = 0 (GetNotifAttr): CmdID(1) | UID(4) | TLV...
                // If Command = 1 (GetAppAttr):   CmdID(1) | AppID(String\0) | TLV...
                
                if (params.data.size() >= 1) 
                {
                    uint8_t *p = (uint8_t *)params.data.data();
                    uint8_t cmd_id = p[0];
                    size_t tlv_offset = 0;
                    uint32_t uid = 0;

                    QapiAncsEventData evt;
                    memset(&evt, 0, sizeof(evt));

                    if (cmd_id == ANCS_CMD_GET_NOTIF_ATTR) 
                    {
                        ANCS_DEBUG("ANCS: Notif ATTR\n");
                        if (params.data.size() < 5) return; // Malformed
                        uid = (uint32_t)p[1] | ((uint32_t)p[2] << 8) | ((uint32_t)p[3] << 16) | ((uint32_t)p[4] << 24);
                        tlv_offset = 5;
                        evt.ds_attr_ind.notification_uid = uid;
                        
                        // Copy TLV data
                        evt.ds_attr_ind.attr_list = p + tlv_offset;
                        evt.ds_attr_ind.attr_list_size = (uint16_t)(params.data.size() - tlv_offset);
                        if (g_ancs_callback)
                        {
                            g_ancs_callback(QAPI_GATT_ANCS_DS_NOTIF_ATTR_IND, &evt);
                        }
                    } 
                    else if (cmd_id == ANCS_CMD_GET_APP_ATTR) 
                    {
                        ANCS_DEBUG("ANCS: App ATTR\n");
                        // Scan for null terminator of AppID to find start of TLV
                        tlv_offset = 1; 
                        while(tlv_offset < params.data.size() && p[tlv_offset] != 0) 
                        {
                            tlv_offset++;
                        }
                        tlv_offset++; // Skip the null terminator

                        if (tlv_offset <= params.data.size()) 
                        {
                            evt.ds_attr_ind.notification_uid = 0; // Not applicable for App Attr
                            
                            // Copy TLV data
                            evt.ds_attr_ind.attr_list = p + tlv_offset;
                            evt.ds_attr_ind.attr_list_size = (uint16_t)(params.data.size() - tlv_offset);
                            if (g_ancs_callback)
                            {
                                g_ancs_callback(QAPI_GATT_ANCS_DS_APP_ATTR_IND, &evt);
                            }
                        }
                    }
                }
            }

            if (params.data.data() != NULL)
            {
                free((void *)params.data.data());
            }
            QapiNotifyCompleteParams resp_params;
            resp_params.attribute_id = params.attribute_id;
            resp_params.session_id = params.session_id;
            resp_params.status = Status::kOk;
            GattClientManagerImpl::Instance().QapiNotifyComplete(std::move(resp_params));
        }
        void QapiWriteWithoutResponseComplete(QapiWriteWithoutResponseCompleteParams &&params) override
        {
            if(params.data.data()) free((void*)params.data.data()); 
        }
        void QapiReadComplete(QapiReadCompleteParams &&params) override
        { 
            if(params.data.data()) free((void*)params.data.data()); 
        }
        void QapiWriteComplete(QapiWriteCompleteParams &&params) override
        {
            ANCS_DEBUG("ANCS: WriteWithResponseComplete Status %d\n", params.status);
            if(params.data.data()) free((void*)params.data.data()); 
        }
        void QapiIndicateReceived(QapiIndicateReceivedParams &&params) override {}
    };

    // Singleton instance of the delegate
    static AncsClientDelegate g_ancs_delegate;
}

// Global API Implementations

void QapiGattAncsRegister(QapiAncsCallback_t callback)
{
    g_ancs_callback = callback;
    
    // Register the delegate with the GATT Manager.
    bok::gatt::GattClientAppRegisterParams cParams = {(int64_t)0xFAFAFAFAFAFAFA06, bok::gatt::g_ancs_delegate};
    bok::gatt::GattSessionManagerImpl::Instance().QapiRegisterApp(std::move(cParams));
}

void QapiGattAncsGetNotificationAttributes(uint32_t notification_uid, const uint8_t *attr_list, uint16_t attr_list_size)
{
    if (bok::gatt::g_ancs_delegate.session_id == 0 || bok::gatt::g_ancs_delegate.handle_cp == 0) 
    {
        ANCS_PRINT("ANCS Error: Not connected or handles not found\n");
        return;
    }
    if (!attr_list || !attr_list_size)
    {
        ANCS_PRINT("ANCS Error: Invalid Param\n");
        return;
    }

    // Command Format: CmdID(1) | UID(4) | Attrs...
    std::vector<uint8_t> payload;
    payload.push_back(ANCS_CMD_GET_NOTIF_ATTR);
    payload.push_back((uint8_t)(notification_uid));
    payload.push_back((uint8_t)(notification_uid >> 8));
    payload.push_back((uint8_t)(notification_uid >> 16));
    payload.push_back((uint8_t)(notification_uid >> 24));
    
    for (int i = 0; i < attr_list_size; i++) 
    {
        payload.push_back(attr_list[i]);
    }

    bok::gatt::QapiWriteRequestParams params;
    params.session_id = bok::gatt::g_ancs_delegate.session_id;
    params.attribute_id = bok::gatt::g_ancs_delegate.handle_cp;
    
    uint8_t *data = (uint8_t *)malloc(payload.size());
    if (data) 
    {
        memcpy(data, payload.data(), payload.size());
        params.data = std::span<std::byte>(reinterpret_cast<std::byte *>(data), payload.size());
        bok::gatt::GattClientManagerImpl::Instance().QapiWriteRequest(std::move(params));
        ANCS_DEBUG("ANCS : CP Write Req\n");
    }
}

void QapiGattAncsGetAppAttributes(const char *app_id, const uint8_t *attr_list, uint16_t attr_list_size)
{
    if (bok::gatt::g_ancs_delegate.session_id == 0 || bok::gatt::g_ancs_delegate.handle_cp == 0)
    {
        ANCS_PRINT("ANCS Error: Not connected or handles not found\n");
        return;
    }
    if (!attr_list || !attr_list_size)
    {
        ANCS_PRINT("ANCS Error: Invalid Param\n");
        return;
    }

    // Command Format: CmdID(1) | AppIdentifier(Start\0) | Attrs...
    std::vector<uint8_t> payload;
    payload.push_back(ANCS_CMD_GET_APP_ATTR);
    
    // App Identifier must be null terminated
    size_t app_len = strlen(app_id);
    for(size_t i=0; i<=app_len; i++) 
    {
        payload.push_back(app_id[i]);
    }
    
    for (int i = 0; i < attr_list_size; i++) 
    {
        payload.push_back(attr_list[i]);
    }

    bok::gatt::QapiWriteRequestParams params;
    params.session_id = bok::gatt::g_ancs_delegate.session_id;
    params.attribute_id = bok::gatt::g_ancs_delegate.handle_cp;
    
    uint8_t *data = (uint8_t *)malloc(payload.size());
    if (data) 
    {
        memcpy(data, payload.data(), payload.size());
        params.data = std::span<std::byte>(reinterpret_cast<std::byte *>(data), payload.size());
        bok::gatt::GattClientManagerImpl::Instance().QapiWriteRequest(std::move(params));
        ANCS_DEBUG("ANCS : CP Write Req\n");
    }
}

extern "C" void ancs_deinit()
{
    ANCS_PRINT("ANCS: Deinit.\n");

    // Send the ANCS released event to the application if a callback is registered.
    if (g_ancs_callback && bok::gatt::g_ancs_delegate.session_id != 0)
    {
        ANCS_DEBUG("ANCS: Sending ANCS released event to application.\n");
        g_ancs_callback(QAPI_GATT_ANCS_RELEASED, NULL);
    }
    // Clear all the data on the ANCS profile.
    bok::gatt::g_ancs_delegate.session_id = 0;
    bok::gatt::g_ancs_delegate.handle_ns = 0;
    bok::gatt::g_ancs_delegate.handle_ds = 0;
    bok::gatt::g_ancs_delegate.handle_cp = 0;
}

