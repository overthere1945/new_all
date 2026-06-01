/**
 * @file ancs_micro_app.cpp
 * @brief Sample application demonstrating ANCS interface usage with TLV parsing
 *        Updated to sequentially chain Notification Attribute and App Attribute requests.
 */

#include "qapi_ancs_profile.h"
#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <string.h>

#define ANCS_APP_PRINT printk
#define ANCS_APP_DEBUG(...) do {} while(0)


/* ANCS Category IDs */
#define CAT_ID_OTHER    0
#define CAT_ID_SOCIAL   4  // SMS, WhatsApp, etc.
#define CAT_ID_EMAIL    6

/* Global to track the category of the notification currently being processed */
static uint8_t g_current_cat_id = 0;

/**
 * @brief Helper to parse and print ANCS TLV data. 
 *        Also extracts the App Identifier (Attr ID 0) if present.
 * 
 * @param data Pointer to the TLV buffer
 * @param total_len Total length of the buffer
 * @param out_app_id Buffer to store the found App Identifier string
 * @param max_len Size of the out_app_id buffer
 * @return true if App Identifier was found and extracted
 */
bool ParseAndPrintAncsTlv(const uint8_t *data, uint16_t total_len, char *out_app_id, uint16_t max_len)
{
    uint16_t offset = 0;
    bool found_app_id = false;
    
    ANCS_APP_DEBUG("--- ANCS Attributes (Total Len: %d) ---\n", total_len);

    while (offset < total_len)
    {
        // Check if we have enough bytes for ID + Length (3 bytes)
        if (offset + 3 > total_len) 
        {
            ANCS_APP_PRINT("  [Warn] Malformed TLV Header at offset %d\n", offset);
            break;
        }

        uint8_t attr_id = data[offset++];
        uint16_t attr_len = (uint16_t)data[offset] | ((uint16_t)data[offset + 1] << 8);
        offset += 2;

        // Check if value length exceeds remaining buffer
        if (offset + attr_len > total_len) 
        {
            // Print what we know (ID/Len) and indicate truncation
            ANCS_APP_PRINT("  ID: %d | Len: %d | Value: [Truncated]\n", attr_id, attr_len);
            break; 
        }

        // OPTIMIZED PRINT: 
        // %.*s takes two arguments: the length (int) and the string pointer (char*)
        ANCS_APP_PRINT("  ID: %d | Len: %d | Value: %.*s\n", 
                       attr_id, 
                       attr_len, 
                       attr_len, 
                       (char*)&data[offset]);

        // Logic: Capture App Identifier (ID 0)
        if (attr_id == 0 && out_app_id != NULL && attr_len > 0)
        {
            uint16_t copy_len = (attr_len < max_len - 1) ? attr_len : (max_len - 1);
            memcpy(out_app_id, &data[offset], copy_len);
            out_app_id[copy_len] = '\0'; // Null terminate
            found_app_id = true;
        }

        offset += attr_len;
    }
    return found_app_id;
}

/* Callback to handle ANCS events */
void AncsMicroAppCallback(QapiAncsEventType eventType, QapiAncsEventData *eventData)
{
    switch (eventType)
    {
    case QAPI_GATT_ANCS_AQCUIRED:
    {
        ANCS_APP_PRINT("ANCS Acquired! NS: %d, DS: %d, CP: %d\n", 
            eventData->acquired.notif_source_handle,
            eventData->acquired.data_source_handle,
            eventData->acquired.control_point_handle);
        break;
    }
    case QAPI_GATT_ANCS_RELEASED:
    {
        ANCS_APP_PRINT("ANCS Released\n");
        break;
    }
    case QAPI_GATT_ANCS_NS_IND:
    {
        ANCS_APP_PRINT("ANCS Notif Ind: UID %u, Cat %d, Cat_cnt %d, Flags %02x\n",
            eventData->ns_ind.notification_uid,
            eventData->ns_ind.cat_id,
            eventData->ns_ind.cat_cnt,
            eventData->ns_ind.evt_flag);
        
        // Store Category for later decision making
        g_current_cat_id = eventData->ns_ind.cat_id;

        // Only process "Added" (0) or "Modified" (1) events
        if (eventData->ns_ind.evt_id <= 1) 
        {
            // 1. Standard: Request Notification Attributes (AppIdentifier, Title, Message)
            ANCS_APP_DEBUG(" -> Requesting Notif Attributes for UID %u\n", eventData->ns_ind.notification_uid);
            uint8_t notif_attrs[] = {
                0, //AppIdentifier (ID 1)
                1, 0x20, 0x00, // Title (ID 1), max 32 bytes
                3, 0x40, 0x00  // Message (ID 3), max 64 bytes
            }; 
            QapiGattAncsGetNotificationAttributes(eventData->ns_ind.notification_uid, notif_attrs, sizeof(notif_attrs));
            
            // NOTE: Do NOT request App Attributes here to avoid back-to-back write errors.
        }
        break;
    }
    case QAPI_GATT_ANCS_DS_NOTIF_ATTR_IND:
    {
        ANCS_APP_DEBUG("ANCS Notification Attributes Received (UID: %u)\n", eventData->ds_attr_ind.notification_uid);
        
        char app_id_str[128] = {0};
        bool has_app_id = false;

        if (eventData->ds_attr_ind.attr_list_size > 0) 
        {
            // Parse TLV and try to extract App Identifier
            has_app_id = ParseAndPrintAncsTlv(eventData->ds_attr_ind.attr_list, 
                                              eventData->ds_attr_ind.attr_list_size,
                                              app_id_str,
                                              sizeof(app_id_str));
        }

        // STEP 2: Logic to fetch App Attributes
        // Only if we found an App ID AND the category matches our interest (Social/SMS)
        if (has_app_id && g_current_cat_id == CAT_ID_SOCIAL)
        {
            ANCS_APP_DEBUG(" -> Found AppID: '%s' (Cat: Social). Requesting App Attributes...\n", app_id_str);
            
            // Request Display Name (ID 0) for this App ID
            uint8_t app_attrs[] = { 0 }; 
            QapiGattAncsGetAppAttributes(app_id_str, app_attrs, sizeof(app_attrs));
        }
        else if (has_app_id)
        {
             ANCS_APP_PRINT(" -> Found AppID: '%s' but Category (%d) is not Social. Skipping App Attrs.\n", app_id_str, g_current_cat_id);
        }
        break;
    }
    case QAPI_GATT_ANCS_DS_APP_ATTR_IND:
    {
        ANCS_APP_DEBUG("ANCS App Attributes Received\n");
        if (eventData->ds_attr_ind.attr_list_size > 0) 
        {
            // Just print, no extraction needed
            ParseAndPrintAncsTlv(eventData->ds_attr_ind.attr_list, eventData->ds_attr_ind.attr_list_size, NULL, 0);
        }
        break;
    }
    default:
        break;
    }
}

/* Entry point to start the ANCS App */
extern "C" void start_ancs_app()
{
    ANCS_APP_PRINT("Starting ANCS Micro App ...\n");
    QapiGattAncsRegister(AncsMicroAppCallback);
}