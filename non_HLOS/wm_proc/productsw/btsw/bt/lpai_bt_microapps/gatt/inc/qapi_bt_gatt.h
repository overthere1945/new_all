/******************************************************************************
 * @file     qapi_bt_gatt.h
 * @brief    Header file contains QAPI for BT GATT Offload and Client and Server operations.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

#ifndef QAPI_BT_GATT_H
#define QAPI_BT_GATT_H

/** Program Specific Header file Inclusions */
#include <stdint.h>
#include <stdbool.h>
#include <stringl.h>
#include "lpai_bt_app_mgr_client_interface.h"
#include "lpai_bt_app_mgr_adsp_handler.h"
#include "lpai_bt_app_mgr_ctxhub_handler.h"
#include "lpai_bt_gatt_common.h"

/***************************************************************************
 * @name GATT Offload Event Opcodes
 * @brief QAPI Event opcodes that GATT micro app shall receive
 ***************************************************************************/
#define QAPI_BT_GATT_UAPP_ACTIVATE_CFM                  0x0001  /**< Activation confirmation */
#define QAPI_BT_GATT_UAPP_DEACTIVATE_CFM                0x0002  /**< Deactivation confirmation */
#define QAPI_BT_GATT_UAPP_SERVICE_REGISTERED            0x0003  /**< Service registered */
#define QAPI_BT_GATT_UAPP_SERVICE_UNREGISTERED          0x0004  /**< Service unregistered */
#define QAPI_BT_GATT_UAPP_CLIENT_CHAR_READ_RSP          0x0005  /**< Client characteristic read response */
#define QAPI_BT_GATT_UAPP_CLIENT_CHAR_WRITE_RSP         0x0006  /**< Client characteristic write response */
#define QAPI_BT_GATT_UAPP_CLIENT_CHAR_NOTIFICATION_IND  0x0007  /**< Client characteristic notification indication */
#define QAPI_BT_GATT_UAPP_SERVER_CHAR_READ_REQ          0x0008  /**< Server characteristic read request */
#define QAPI_BT_GATT_UAPP_SERVER_CHAR_WRITE_REQ         0x0009  /**< Server characteristic write request */
#define QAPI_BT_GATT_UAPP_SERVER_CHAR_NOTIFICATION_RSP  0x000A  /**< Server characteristic notification response */
#define QAPI_BT_GATT_UAPP_CLIENT_TPUT_CFM               0x000B  /**< Client throughput confirmation */
#define QAPI_BT_GATT_UAPP_SERVER_TPUT_CFM               0x000C  /**< Server throughput confirmation */

/***************************************************************************
 * @name GATT Result Codes
 * @brief Result codes returned on invoking QAPIs
 ***************************************************************************/
/**
 * @typedef qapi_gatt_result_t
 * @brief Result code for GATT API functions.
 */
typedef uint8_t qapi_gatt_result_t;

/**< Operation success */
#define QAPI_BT_GATT_RESULT_SUCCESS                  0x00
/**< Invalid parameter */
#define QAPI_BT_GATT_RESULT_INVALID_PARAM            0x01
/**< Invalid operation */
#define QAPI_BT_GATT_RESULT_INVALID_OPERATION        0x02
/**< Insufficient resources */
#define QAPI_BT_GATT_RESULT_INSUFF_RESOURCES         0x03
/**< Internal error */
#define QAPI_BT_GATT_RESULT_INTERNAL_ERROR           0x04

/***************************************************************************
 * @name Maximum MTU Size
 ***************************************************************************/
/**< MAX supported ATT MTU for GATT client/server role */
#define QAPI_GATT_MTU_MAX                        512

/******************************************************************************
 * @struct qapi_bt_gatt_app_activate_cfm_t
 * @brief Structure for activation confirmation response.
 ******************************************************************************/
typedef struct qapi_bt_gatt_app_activate_cfm {
    qapi_gatt_result_t result; /**< Result code */
} qapi_bt_gatt_app_activate_cfm_t;

/******************************************************************************
 * @struct qapi_bt_gatt_app_deactivate_cfm_t
 * @brief Structure for deactivation confirmation response.
 ******************************************************************************/
typedef struct qapi_bt_gatt_app_deactivate_cfm {
    qapi_gatt_result_t result; /**< Result code */
} qapi_bt_gatt_app_deactivate_cfm_t;

/******************************************************************************
 * @struct qapi_gatt_characteristic_t
 * @brief Structure for GATT characteristic info.
 ******************************************************************************/
typedef struct qapi_gatt_characteristic {
    uint16_t handle;                                      /**< Attribute handle */
    uint16_t properties;                                  /**< Characteristic properties */
    uint8_t  uuid[GATT_OFFLOAD_UUID_BYTE_LEN];            /**< 128-bit UUID */
} qapi_gatt_characteristic_t;

/******************************************************************************
 * @struct qapi_bt_gatt_service_registered_t
 * @brief Structure for service registered event.
 ******************************************************************************/
typedef struct qapi_bt_gatt_service_registered {
    uint8_t                    role;                      /**< Server/client role */
    int32_t                    sessionId;                 /**< Session identifier */
    int32_t                    attMtu;                    /**< MTU */
    uint8_t                    serviceUuid[GATT_OFFLOAD_UUID_BYTE_LEN]; /**< 128-bit service UUID */
    uint16_t                   numChars;                  /**< Number of characteristics */
    qapi_gatt_characteristic_t characteristics[GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS]; /**< Characteristics list */
} qapi_bt_gatt_service_registered_t;

/******************************************************************************
 * @struct qapi_bt_gatt_service_unregistered_t
 * @brief Structure for service unregistered event.
 ******************************************************************************/
typedef struct qapi_bt_gatt_service_unregistered {
    int32_t sessionId;                                    /**< Session identifier */
} qapi_bt_gatt_service_unregistered_t;

/******************************************************************************
 * @struct qapi_bt_gatt_read_char_req_t
 * @brief Structure for characteristic read request.
 ******************************************************************************/
typedef struct qapi_bt_gatt_read_char_req {
    int32_t  sessionId;                                   /**< Session identifier */
    uint16_t attrHandle;                                  /**< Attribute handle */
} qapi_bt_gatt_read_char_req_t;

/******************************************************************************
 * @struct qapi_bt_gatt_read_char_rsp_t
 * @brief Structure for characteristic read response.
 ******************************************************************************/
typedef struct qapi_bt_gatt_read_char_rsp {
    int32_t  sessionId;                                   /**< Session identifier */
    uint16_t attrHandle;                                  /**< Attribute handle */
    uint16_t result;                                      /**< Result of operation */
    uint16_t val_len;                                     /**< Value length */
    uint8_t  *val;                                        /**< Pointer to value. App must free this. */
} qapi_bt_gatt_read_char_rsp_t;

/******************************************************************************
 * @struct qapi_bt_gatt_write_char_req_t
 * @brief Structure for characteristic write request.
 ******************************************************************************/
typedef struct qapi_bt_gatt_write_char_req {
    int32_t  sessionId;                                   /**< Session identifier */
    uint16_t attrHandle;                                  /**< Attribute handle */
    uint16_t val_len;                                     /**< Value length */
    uint8_t  *val;                                        /**< Pointer to value */
    bool     writeWithoutRsp;                             /**< If true, write without response */
} qapi_bt_gatt_write_char_req_t;

/******************************************************************************
 * @struct qapi_bt_gatt_write_char_rsp_t
 * @brief Structure for characteristic write response.
 ******************************************************************************/
typedef struct qapi_bt_gatt_write_char_rsp {
    int32_t  sessionId;                                   /**< Session identifier */
    uint16_t attrHandle;                                  /**< Attribute handle */
    uint16_t result;                                      /**< Result of operation */
} qapi_bt_gatt_write_char_rsp_t;

/******************************************************************************
 * @struct qapi_bt_gatt_char_notif_t
 * @brief Structure for send/receive characteristic notification.
 ******************************************************************************/
typedef struct qapi_bt_gatt_char_notif {
    int32_t  sessionId;                                   /**< Session identifier */
    uint16_t attrHandle;                                  /**< Attribute handle */
    uint16_t val_len;                                     /**< Value length */
    uint8_t  *val;                                        /**< Pointer to value */
} qapi_bt_gatt_char_notif_t;

/******************************************************************************
 * @struct qapi_bt_gatt_char_notif_rsp_t
 * @brief Structure for notification or indication response.
 ******************************************************************************/
typedef struct qapi_bt_gatt_char_notif_rsp {
    uint16_t result;                                      /**< Result of operation */
} qapi_bt_gatt_char_notif_rsp_t;

/******************************************************************************
 * @brief Register a micro-app endpoint with GATT. This API to be called during system start-up.
 * This API internally register endpoint with App Manager.
 * @param endpoint Endpoint configuration.
 * @param cb Callback to register.
 * @return Result code.
 ******************************************************************************/
qapi_gatt_result_t qapi_gatt_app_register
(
    endPointDetails_t endpoint,
    uapp_cb_t cb
);

/**
 * @brief Activates the GATT micro-app. 
 * This API to be called before any GATT offload is initiated on this endpoint.
 * @param ep_id Micro-app endpoint ID.
 * @return Result code.
 */
qapi_gatt_result_t qapi_gatt_app_activate
(
    uint64_t ep_id
);

/**
 * @brief Deactivates the GATT micro-app.
 * This API is expected to be called after all GATT sessions are unoffloaded.
 * @param ep_id Micro-app endpoint ID.
 * @return Result code.
 */
qapi_gatt_result_t qapi_gatt_app_deactivate
(
    uint64_t ep_id
);

/**
 * @brief Unregister a GATT service. Before calling this API,
 * micro app to complete any pending responses.
 * @param session_id GATT session to unregister.
 * @return Result code.
 */
qapi_gatt_result_t qapi_gatt_unregister_service
(
    int32_t session_id
);

/**
 * @brief Issue a client characteristic read request. 
 * Micro-app shall receive response using QAPI_BT_GATT_UAPP_CLIENT_CHAR_READ_RSP.
 * Micro-app to wait for this response before sending next request.
 * @param req Characteristic read request info.
 * @return Result code.
 */
qapi_gatt_result_t qapi_gatt_client_read_char_req
(
    qapi_bt_gatt_read_char_req_t req
);

/**
 * @brief Issue a client characteristic write request.
 * Micro-app to receive response using QAPI_BT_GATT_UAPP_CLIENT_CHAR_WRITE_RSP.
 * Micro-app to wait for the response before sending next request.
 * @param req Characteristic write request info.
 * @return Result code.
 */
qapi_gatt_result_t qapi_gatt_client_write_char_req
(
    qapi_bt_gatt_write_char_req_t req
);

/**
 * @brief Send notification confirmation from client.
 * Micro-app to invoke this API as response for QAPI_BT_GATT_UAPP_CLIENT_CHAR_NOTIFICATION_IND.
 * @param rsp Notification response info.
 * @return Result code.
 */
qapi_gatt_result_t qapi_gatt_client_notif_rsp
(
    qapi_bt_gatt_char_notif_rsp_t rsp
);

/**
 * @brief Issue a server read characteristic response.
 * Micro-app to invoke this API as response for QAPI_BT_GATT_UAPP_SERVER_CHAR_READ_REQ.
 * @param rsp Read response info.
 * @return Result code.
 */
qapi_gatt_result_t qapi_gatt_server_read_char_rsp
(
    qapi_bt_gatt_read_char_rsp_t rsp
);

/**
 * @brief Issue a server write characteristic response.
 * Micro-app to invoke this API as response for QAPI_BT_GATT_UAPP_SERVER_CHAR_WRITE_REQ.
 * @param rsp Write response info.
 * @return Result code.
 */
qapi_gatt_result_t qapi_gatt_server_write_char_rsp
(
    qapi_bt_gatt_write_char_rsp_t rsp
);

/**
 * @brief Server send characteristic notification.
 * Micro-app to receive response using QAPI_BT_GATT_UAPP_SERVER_CHAR_NOTIFICATION_RSP.
 * Micro-app to wait for the response before sending next notification.
 * @param req Notification info.
 * @return Result code.
 */
qapi_gatt_result_t qapi_gatt_server_send_char_notif
(
    qapi_bt_gatt_char_notif_t req
);

#endif /* QAPI_BT_GATT_H */

