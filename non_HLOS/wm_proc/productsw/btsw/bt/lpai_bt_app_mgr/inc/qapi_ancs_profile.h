/**
 * @file qapi_ancs_profile.h
 * @brief ANCS Profile Interface using GATT Offload APIs
 */

#ifndef QAPI_ANCS_PROFILE_H
#define QAPI_ANCS_PROFILE_H

#include <stdint.h>
#include <vector>

/* ANCS Event Types */
typedef enum {
    QAPI_GATT_ANCS_AQCUIRED,          // ANCS Service Found and Handles Acquired
    QAPI_GATT_ANCS_RELEASED,          // ANCS Service Released/Disconnected
    QAPI_GATT_ANCS_NS_IND,            // Notification Source Indication (New/Mod/Rem Notification)
    QAPI_GATT_ANCS_DS_NOTIF_ATTR_IND, // Data Source: Notification Attributes
    QAPI_GATT_ANCS_DS_APP_ATTR_IND,   // Data Source: App Attributes
} QapiAncsEventType;

/* ANCS Event Flags (matches ANCS spec) */
#define QAPI_ANCS_EVT_FLAG_SILENT          (1 << 0)
#define QAPI_ANCS_EVT_FLAG_IMPORTANT       (1 << 1)
#define QAPI_ANCS_EVT_FLAG_PRE_EXISTING    (1 << 2)
#define QAPI_ANCS_EVT_FLAG_POSITIVE_ACTION (1 << 3)
#define QAPI_ANCS_EVT_FLAG_NEGATIVE_ACTION (1 << 4)

/* ANCS Category IDs */
#define QAPI_ANCS_CAT_OTHER                0
#define QAPI_ANCS_CAT_INCOMING_CALL        1
#define QAPI_ANCS_CAT_MISSED_CALL          2
#define QAPI_ANCS_CAT_VOICEMAIL            3
#define QAPI_ANCS_CAT_SOCIAL               4
#define QAPI_ANCS_CAT_SCHEDULE             5
#define QAPI_ANCS_CAT_EMAIL                6
#define QAPI_ANCS_CAT_NEWS                 7
#define QAPI_ANCS_CAT_HEALTH_AND_FITNESS   8
#define QAPI_ANCS_CAT_BUSINESS_AND_FINANCE 9
#define QAPI_ANCS_CAT_LOCATION             10
#define QAPI_ANCS_CAT_ENTERTAINMENT        11

/* Structure for Notification Source Indication */
typedef struct {
    uint8_t evt_id;
    uint8_t evt_flag;
    uint8_t cat_id;
    uint8_t cat_cnt;
    uint32_t notification_uid;
} QapiAncsNsIndData;

/* Structure for Data Source Indication (Attributes) */
typedef struct {
    uint32_t notification_uid; // For AppAttr this might be 0 or ignored depending on context, but useful for tracking
    const uint8_t *attr_list; // Raw TLV data or formatted string buffer
    uint16_t attr_list_size;
} QapiAncsDsAttrIndData;

/* Structure for Handles Acquired */
typedef struct {
    uint16_t notif_source_handle;
    uint16_t data_source_handle;
    uint16_t control_point_handle;
} QapiAncsAcquiredData;

/* Union of all ANCS Event Data */
typedef union {
    QapiAncsAcquiredData acquired;
    QapiAncsNsIndData ns_ind;
    QapiAncsDsAttrIndData ds_attr_ind;
} QapiAncsEventData;

/* Callback function prototype */
typedef void (*QapiAncsCallback_t)(QapiAncsEventType eventType, QapiAncsEventData *eventData);

/* API Prototypes */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the ANCS profile with the GATT system.
 * @param callback User callback to receive ANCS events.
 */
void QapiGattAncsRegister(QapiAncsCallback_t callback);

/**
 * @brief Requests attributes for a specific notification.
 * @param notification_uid The UID of the notification.
 * @param attr_list List of attribute IDs (and optional lengths) to retrieve.
 * @param attr_list_size Size of the attribute list.
 */
void QapiGattAncsGetNotificationAttributes(uint32_t notification_uid, const uint8_t *attr_list, uint16_t attr_list_size);

/**
 * @brief Requests attributes for a specific app.
 * @param app_id Null-terminated string of the App Identifier.
 * @param attr_list List of attribute IDs to retrieve.
 * @param attr_list_size Size of the attribute list.
 */
void QapiGattAncsGetAppAttributes(const char *app_id, const uint8_t *attr_list, uint16_t attr_list_size);

#ifdef __cplusplus
}
#endif

#endif // QAPI_ANCS_PROFILE_H