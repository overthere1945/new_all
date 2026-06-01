/*************************************************************************
 * @file lpai_bt_gatt_common.c
 * @brief Implementation of LPAI BT GATT client and server application management.
 * 
 * This file contains management routines for registering/unregistering GATT apps, session handling,
 * event dispatching, and GATT operation QAPI functions for GATT Client and Server roles.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/** Program Specific Header file Inclusions */
#include <zephyr/sys/printk.h>
#include "lpai_bt_gatt_common.h"
#include "qapi_bt_gatt.h"
#include "lpai_bt_app_mgr_adsp_handler.h"
#include "bt_utilities.h"

/** 
 * @def INVALID_EP_ID
 * @brief Macro representing an invalid endpoint ID.
 *
 * This macro is used to indicate that the endpoint ID is not valid.
 */
#define INVALID_EP_ID       0

/** 
 * @def INVALID_SESSION_ID
 * @brief Macro representing an invalid session ID.
 *
 * This macro is used to indicate that the session ID is not valid.
 */
#define INVALID_SESSION_ID  0

/**
 * @brief Array to manage multiple GATT micro apps.
 */
gatt_uapps_t gatt_uapps[GATT_MAX_UAPPS] = {0};

/**
 * @brief Initializes the Common GATT Structures.
 *
 * This function resets the necessary configurations, data structures, and
 * resources required to start the Common Gatt Structures.
 *
 * @note This function does not take any parameters and does not return a value.
 */
void gatt_common_deinit()
{
    for(int i=0;i<GATT_MAX_UAPPS;i++)
    {
        for(int j=0;j<GATT_OFFLOAD_UAPP_MAX_SESSIONS;j++)
        {
            gatt_uapps[i].sessionId[j] = INVALID_SESSION_ID;
        }
    }
}

/**
 * @brief Finds a GATT micro-app by endpoint ID.
 * 
 * @param ep_id Endpoint ID to search for.
 * @return Pointer to the matching gatt_uapps_t if found, NULL otherwise.
 */
static gatt_uapps_t *find_uapp_exists(uint64_t ep_id) {
    if(ep_id == INVALID_EP_ID)
        return NULL;

    for(int i = 0; i < GATT_MAX_UAPPS; ++i) {
        if(gatt_uapps[i].ep_id == ep_id) {
            return &gatt_uapps[i];
        }
    }
    return NULL; // Not found
}

/**
 * @brief Finds a GATT micro-app containing a specific session ID.
 * 
 * @param session_id The session ID to search for.
 * @return Pointer to the matching gatt_uapps_t if found, NULL otherwise.
 */
static gatt_uapps_t* find_uapp_with_session_id(int32_t session_id)
{
    for (int i = 0; i < GATT_MAX_UAPPS; ++i) {
        for (int j = 0; j < GATT_OFFLOAD_UAPP_MAX_SESSIONS; ++j) {
            if (gatt_uapps[i].sessionId[j] == session_id) {
                return &gatt_uapps[i];
            }
        }
    }
    return NULL; // Not found
}

/**
 * @brief Clears the specified session ID from the app context.
 * 
 * @param uapp Pointer to app context.
 * @param session_id Session ID to clear.
 */
static void uapp_clear_session(gatt_uapps_t * uapp, int32_t session_id)
{
    if (!uapp) return;

    for (uint8_t i = 0; i < GATT_OFFLOAD_UAPP_MAX_SESSIONS; ++i)
    {
        if (uapp->sessionId[i] == session_id)
        {
#ifdef DEBUG_BUILD_LOG
            printk("session cleared %d", session_id);
#endif
            uapp->sessionId[i] = INVALID_SESSION_ID;
            break;
        }
    }
}

#ifdef DEBUG_BUILD_LOG
/**
 * @brief Logs details about GATT service registration requests.
 * 
 * @param service           Pointer to registration data.
 * @param service_uuid_holder Holder for service UUID.
 * @param char_list_holder  Pointer to characteristics list data.
 */
static void log_register_service(
    gatt_register_service *service,
    uint8_t *service_uuid_holder,
    char_list_holder_t *char_list_holder)
{
    printk("session_id %d, aclHandle %d, attMtu %d role %d", 
        service->sessionId, service->aclConnectionHandle, service->attMtu, service->role);
    
    for (uint8_t i=0; i<GATT_OFFLOAD_UUID_BYTE_LEN; i++)
    {
        //printk("svc_uuid[%d]: %d", i, service_uuid_holder[i]);
    }

    printk("num_chars %d", char_list_holder->num_chars);
    for (uint8_t i=0; i<char_list_holder->num_chars; i++)
    {
        //printk("attr_handle %d", char_list_holder->chars[i].valueHandle);
        //printk("properties %d", char_list_holder->chars[i].properties);
        for (uint8_t j=0; j<GATT_OFFLOAD_UUID_BYTE_LEN; j++)
        {
            //printk("char_uuid[%d]: %d", j, char_list_holder->chars[i].uuid[j]);
        }
    }
}
#endif

/**
 * @brief Callback for decoding GATT characteristic lists using nanopb.
 * 
 * @param stream Protobuf stream context.
 * @param field  Field context.
 * @param arg    Pointer to char_list_holder_t context.
 * @return true if decoding succeeded, false otherwise.
 */
bool char_list_decode_callback(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    char_list_holder_t *holder = (char_list_holder_t*)(*arg);
    if (holder->num_chars >= GATT_OFFLOAD_UAPP_SESSIONS_MAX_CHARS) return false;

    /* Decode one element */
    GattCharacteristic gatt_char = GattCharacteristic_init_zero;
    uint8_t char_uuid[GATT_OFFLOAD_UUID_BYTE_LEN] = {0};
    gatt_char.uuid.uuid.arg = &char_uuid;
    gatt_char.uuid.uuid.funcs.decode = decode_bytes;
    if (!pb_decode(stream, GattCharacteristic_fields, &gatt_char))
        return false;

    holder->chars[holder->num_chars].properties = gatt_char.properties;
    holder->chars[holder->num_chars].valueHandle = gatt_char.valueHandle;
    memscpy(&holder->chars[holder->num_chars].uuid[0], GATT_OFFLOAD_UUID_BYTE_LEN, &char_uuid[0], GATT_OFFLOAD_UUID_BYTE_LEN);

    holder->num_chars++;
    return true;
}

/**
 * @brief Decode and process a GATT "register service" request from application layer.
 * 
 * @param buffer            Raw request buffer.
 * @param buf_len           Buffer length.
 * @param service           Decoded structure output.
 * @param service_uuid_holder UUID buffer for output.
 * @param char_list_holder  Pointer to decoded characteristics data.
 * @return true if decoding succeeded, false otherwise.
 */
bool decode_gatt_register_service(
    const uint8_t *buffer, size_t buf_len,
    gatt_register_service *service,
    uint8_t *service_uuid_holder,
    char_list_holder_t *char_list_holder)
{
    /* Assign uuid callback/context */
    service->uuid.uuid.funcs.decode = decode_bytes;
    service->uuid.uuid.arg = service_uuid_holder;

    /* Assign gattCharacteristic list callback/context */
    service->gattCharacteristic.funcs.decode = &char_list_decode_callback;
    service->gattCharacteristic.arg = char_list_holder;
    char_list_holder->num_chars = 0; // Initialize

    /* Decode */
    pb_istream_t stream = pb_istream_from_buffer(buffer, buf_len);
    if (!pb_decode(&stream, gatt_register_service_fields, service)) {
        printf("Decode failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
#ifdef DEBUG_BUILD_LOG
    log_register_service(service, service_uuid_holder, char_list_holder);
#endif
    return true;
}

/**
 * @brief Handles a GATT service register request including decoding and callback.
 * 
 * @param appData      Pointer to input data.
 * @param appDataLen   Length of input data.
 * @param uapp         Pointer to the relevant GATT user app.
 */
static void gattRegisterServiceReq(void *appData , uint16_t appDataLen, gatt_uapps_t * uapp)
{
    gatt_register_service reg_service = gatt_register_service_init_zero;
    uint8_t service_uuid[GATT_OFFLOAD_UUID_BYTE_LEN] = {0};
    char_list_holder_t char_list_holder = { .num_chars = 0 };

    if (decode_gatt_register_service((uint8_t *)appData, appDataLen, &reg_service, service_uuid, &char_list_holder))
    {
        uapp_gatt_register_service_rsp_t rsp = {.session_id = reg_service.sessionId};

#ifdef DEBUG_BUILD_LOG
        printk("End Point ID : % " PRIu64 "\n", reg_service.endpointId.endpointID);
        printk("End Point hubID: % " PRIu64 "\n", reg_service.endpointId.hubID);
#endif
        if (reg_service.role != GATT_OFFLOAD_CLIENT_ROLE && reg_service.role != GATT_OFFLOAD_SERVER_ROLE)
        {
            rsp.status = GattStatus_OFFLOAD_STATUS_UNSUPPORTED_ROLE;
        }
        else 
        {
            uint8_t k;
            for (k=0; k<GATT_OFFLOAD_UAPP_MAX_SESSIONS; k++)
            {
                if (uapp->sessionId[k] == INVALID_SESSION_ID)
                {
                    uapp->sessionId[k] = reg_service.sessionId;
                    rsp.status = GattStatus_OFFLOAD_STATUS_SUCCESS;

                    qapi_bt_gatt_service_registered_t evt = {
                        .role = reg_service.role,
                        .sessionId = reg_service.sessionId,
                        .attMtu = reg_service.attMtu,
                        .numChars = char_list_holder.num_chars,
                    };
                    memscpy(&evt.serviceUuid[0], GATT_OFFLOAD_UUID_BYTE_LEN, &service_uuid[0], GATT_OFFLOAD_UUID_BYTE_LEN);
                    
                    for (uint8_t i=0; i<char_list_holder.num_chars; i++)
                    {
                        evt.characteristics[i].handle = char_list_holder.chars[i].valueHandle;
                        evt.characteristics[i].properties = char_list_holder.chars[i].properties;
                        memscpy(&evt.characteristics[i].uuid[0], GATT_OFFLOAD_UUID_BYTE_LEN, &char_list_holder.chars[i].uuid[0], GATT_OFFLOAD_UUID_BYTE_LEN);
                    }
                    uapp->uapp_cb(uapp->ep_id, QAPI_BT_GATT_UAPP_SERVICE_REGISTERED, (void *)&evt, sizeof(evt));
					break;
                }
            }
            if (k == GATT_OFFLOAD_UAPP_MAX_SESSIONS)
            {
                rsp.status = GattStatus_OFFLOAD_STATUS_INSUFF_RESOURCES;
            }
        }
        /* send response back to adsp for offload request */
#ifdef DEBUG_BUILD_LOG
        printk("\nUAPP_GATT_REGISTER_SERVICE_RSP sessionId %d status %d", rsp.session_id, rsp.status);
#endif
        lpai_bt_appmgr_send_endpt_msg_adsp(reg_service.endpointId.endpointID, UAPP_GATT_REGISTER_SERVICE_RSP, 
            sizeof(uapp_gatt_register_service_rsp_t), (void *)&rsp, false);
    }
#ifdef DEBUG_BUILD_LOG
    else
    {
        //cannot send response to adsp since session_id is not decoded.
        printk("decode failed");
    }
#endif
}

/**
 * @brief Handles GATT characteristic operations (read, write, notification, etc.).
 * 
 * @param eventId      Event identifier.
 * @param appData      Pointer to relevant data.
 * @param appDataLen   The length of relevant data.
 * @param uapp         Pointer to the relevant GATT user app.
 */
static void gattCharOperation(uint16_t eventId, void *appData , uint16_t appDataLen, gatt_uapps_t * uapp)
{
    if (!uapp || !uapp->uapp_cb)
        return;
    
    switch (eventId)
    {
        case UAPP_GATT_CLIENT_READ_CHAR_RESP:
        {
            uapp_gatt_char_read_rsp_t *rsp = (uapp_gatt_char_read_rsp_t *)appData;
#ifdef DEBUG_BUILD_LOG
            printk("GATT_CLIENT_READ_CHAR_RESP sessionId %d handle %d error_code %d val_len %d context %x\n", \
                rsp->session_id, rsp->handle, rsp->result, rsp->val_len, rsp->context);
#endif
            qapi_bt_gatt_read_char_rsp_t evt = {
                .sessionId = rsp->session_id,
                .attrHandle = rsp->handle,
                .result = rsp->result,
                .val_len = rsp->val_len,
            };
            if (evt.val_len > 0)
            {
                evt.val = rsp->val;
            }
            uapp->uapp_cb(uapp->ep_id, QAPI_BT_GATT_UAPP_CLIENT_CHAR_READ_RSP, (void *)&evt, sizeof(evt));
        }
        break;
        case UAPP_GATT_CLIENT_WRITE_CHAR_RESP:
        {
            uapp_gatt_char_write_rsp_t *rsp = (uapp_gatt_char_write_rsp_t *)appData;
#ifdef DEBUG_BUILD_LOG
            printk("UAPP_GATT_CLIENT_WRITE_CHAR_RESP sessionId %d handle %d error_code %d context %x\n", \
                rsp->session_id, rsp->handle, rsp->result, rsp->context);
#endif
            qapi_bt_gatt_write_char_rsp_t evt = {
                .sessionId = rsp->session_id,
                .attrHandle = rsp->handle,
                .result = rsp->result,
             };
            uapp->uapp_cb(uapp->ep_id, QAPI_BT_GATT_UAPP_CLIENT_CHAR_WRITE_RSP, (void *)&evt, sizeof(evt));
        }
        break;
        case UAPP_GATT_CLIENT_NOTIFICATION_IND:
        {
            uapp_gatt_char_notif_ind_t *ind = (uapp_gatt_char_notif_ind_t *)appData;
#ifdef DEBUG_BUILD_LOG
            printk("UAPP_GATT_CLIENT_NOTIFICATION_IND sessionId %d handle %d val_len %d %x context\n", \
                ind->session_id, ind->handle, ind->val_len);
#endif
            qapi_bt_gatt_char_notif_t evt = {
                .sessionId = ind->session_id,
                .attrHandle = ind->handle,
                .val_len = ind->val_len,
             };
            if (evt.val_len > 0)
            {
                evt.val = ind->val;
            }
            uapp->uapp_cb(uapp->ep_id, QAPI_BT_GATT_UAPP_CLIENT_CHAR_NOTIFICATION_IND, (void *)&evt, sizeof(evt));
        }
        break;
        case UAPP_GATT_SERVER_READ_CHAR_IND:
        {
            uapp_gatt_char_read_req_t *ind = (uapp_gatt_char_read_req_t *)appData;
#ifdef DEBUG_BUILD_LOG
            printk("UAPP_GATT_SERVER_READ_CHAR_IND sessionId %d handle %d mtu %d context %x\n", \
                ind->session_id, ind->handle, ind->mtu, ind->context);
#endif
            qapi_bt_gatt_read_char_req_t evt = {.sessionId = ind->session_id, .attrHandle=ind->handle};
            uapp->uapp_cb(uapp->ep_id, QAPI_BT_GATT_UAPP_SERVER_CHAR_READ_REQ, &evt, sizeof(evt));
        }
        break;
        case UAPP_GATT_SERVER_WRITE_CHAR_IND:
        {
            uapp_gatt_char_write_req_t *ind = (uapp_gatt_char_write_req_t *)appData;
#ifdef DEBUG_BUILD_LOG
            printk("UAPP_GATT_SERVER_WRITE_CHAR_IND sessionId %d handle %d val_len %d context %x\n", \
                ind->session_id, ind->handle, ind->val_len, ind->context);
#endif
            qapi_bt_gatt_write_char_req_t evt = {
                .sessionId = ind->session_id,
                .attrHandle = ind->handle,
                .val_len = ind->val_len,
                .val = ind->val,
            };
            uapp->uapp_cb(uapp->ep_id, QAPI_BT_GATT_UAPP_SERVER_CHAR_WRITE_REQ, (void *)&evt, sizeof(evt));
        }
        break;
        case UAPP_GATT_SERVER_NOTIFICATION_RESP:
        {
            uapp_gatt_char_notif_ind_rsp_t *ind = (uapp_gatt_char_notif_ind_rsp_t *)appData;
            qapi_bt_gatt_char_notif_rsp_t evt = {.result = ind->result,};
            uapp->uapp_cb(uapp->ep_id, QAPI_BT_GATT_UAPP_SERVER_CHAR_NOTIFICATION_RSP, (void *)&evt, sizeof(evt));
        }
        break;

        default:
            break;
    }
}


/**
 * @brief Handles incoming events/messages from Bluetooth app manager.
 * 
 * @param endPointId     Endpoint identifier.
 * @param eventId        Event identifier.
 * @param appDataLen     Length of associated data.
 * @param appData        Pointer to data.
 * @param proto_encoded  Whether data is proto encoded.
 */
void gatt_app_mgr_cb(uint64_t endPointId, uint16_t eventId , uint16_t appDataLen, void *appData, bool proto_encoded)
{
    printk("gatt_uapp_cb Evt %d, data_len %d\n", eventId, appDataLen);

    gatt_uapps_t * uapp = find_uapp_exists(endPointId);
    if (!uapp)
        return;
    
	switch(eventId)
	{
        case UAPP_GATT_APP_REG_RESP:
        {
            uapp->uapp_cb(endPointId, QAPI_BT_GATT_UAPP_ACTIVATE_CFM, appData, sizeof(qapi_bt_gatt_app_activate_cfm_t));
        }
        break;
        case UAPP_GATT_APP_UNREG_RESP:
        {
            uapp->uapp_cb(endPointId, QAPI_BT_GATT_UAPP_DEACTIVATE_CFM, appData, sizeof(qapi_bt_gatt_app_deactivate_cfm_t));
        }
        break;
        case UAPP_GATT_REGISTER_SERVICE_REQ:
        {
            gattRegisterServiceReq(appData, appDataLen, uapp);
        }
        break;
        case UAPP_GATT_UNREGISTER_SERVICE_REQ:
        {
            qapi_bt_gatt_service_unregistered_t *evt = (qapi_bt_gatt_service_unregistered_t *)appData;
#ifdef DEBUG_BUILD_LOG
            printk("UAPP_GATT_UNREGISTER_SERVICE_EVT session_id %d\n", evt->sessionId);
#endif
            uapp_clear_session(uapp, evt->sessionId);
            uapp->uapp_cb(endPointId, QAPI_BT_GATT_UAPP_SERVICE_UNREGISTERED, (void *)evt, sizeof(qapi_bt_gatt_service_unregistered_t));

            /* Send response to Q6 */
            uapp_gatt_unregister_service_ind_t ind = {.session_id = evt->sessionId};
            glink_err_type ret = lpai_bt_appmgr_send_endpt_msg_adsp(uapp->ep_id, UAPP_GATT_UNREGISTER_SERVICE_RSP, 
                sizeof(uapp_gatt_unregister_service_ind_t), (void *)&ind, false);
        }
        break;
        case UAPP_GATT_CLIENT_READ_CHAR_RESP:
        case UAPP_GATT_CLIENT_WRITE_CHAR_RESP:
        case UAPP_GATT_CLIENT_NOTIFICATION_IND:
        case UAPP_GATT_SERVER_READ_CHAR_IND:
        case UAPP_GATT_SERVER_WRITE_CHAR_IND:
        case UAPP_GATT_SERVER_NOTIFICATION_RESP:
        {
            gattCharOperation(eventId, appData, appDataLen, uapp);
        }
        break;
		default:
		{
            break;
		}
	}
}

/**
 * @brief Registers a GATT application with the app manager.
 * GATT Micro apps to invoke this API during system boot/init sequence.
 * @param endpoint   Endpoint details.
 * @param cb         Callback function for events.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */

qapi_gatt_result_t qapi_gatt_app_register(   endPointDetails_t endpoint,      uapp_cb_t cb)
{
    for (uint8_t i=0; i<GATT_MAX_UAPPS; i++)
    {
        if (gatt_uapps[i].ep_id == INVALID_EP_ID)
        {
            gatt_uapps[i].ep_id = endpoint.endPointId.epId;
            gatt_uapps[i].uapp_cb = cb;
            lpai_bt_app_mgr_register_endpt_client(gatt_app_mgr_cb, endpoint);
            return QAPI_BT_GATT_RESULT_SUCCESS;
        }
    }
    return QAPI_BT_GATT_RESULT_INSUFF_RESOURCES;
}
/**
 * @brief Activates a previously registered GATT app endpoint.
 * 
 * @param ep_id Endpoint ID.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */
qapi_gatt_result_t qapi_gatt_app_activate(uint64_t ep_id)
{
    if (find_uapp_exists(ep_id) != NULL)
    {
        glink_err_type ret = lpai_bt_appmgr_send_endpt_msg_adsp (ep_id, UAPP_GATT_APP_REG_REQ, 0, NULL, false);
#ifdef DEBUG_BUILD_LOG
        printk("Sent gatt_client_app_activate %d\n", ret);
#endif
        return (ret == 0) ? QAPI_BT_GATT_RESULT_SUCCESS : QAPI_BT_GATT_RESULT_INTERNAL_ERROR;
    }
    return QAPI_BT_GATT_RESULT_INVALID_PARAM;
}

/**
 * @brief Deactivates a registered GATT app endpoint.
 * 
 * @param ep_id Endpoint ID.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */
qapi_gatt_result_t qapi_gatt_app_deactivate(uint64_t ep_id)
{
    if (find_uapp_exists(ep_id) != NULL)
    {
        glink_err_type ret = lpai_bt_appmgr_send_endpt_msg_adsp(ep_id, UAPP_GATT_APP_UNREG_REQ, 0, NULL, false);
#ifdef DEBUG_BUILD_LOG
        printk("Sent qapi_gatt_client_app_deactivate %d\n", ret);
#endif
        return (ret == 0) ? QAPI_BT_GATT_RESULT_SUCCESS : QAPI_BT_GATT_RESULT_INTERNAL_ERROR;
    }
    return QAPI_BT_GATT_RESULT_INVALID_PARAM;
}

/**
 * @brief Unregisters a GATT service by session ID.
 * 
 * @param session_id Session ID to unregister.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */
qapi_gatt_result_t qapi_gatt_unregister_service
    (int32_t session_id)
{
    gatt_uapps_t * uapp = find_uapp_with_session_id(session_id);

    if (uapp == NULL)
        return QAPI_BT_GATT_RESULT_INVALID_PARAM;

    uapp_clear_session(uapp, session_id);

    uapp_gatt_unregister_service_ind_t ind = {.session_id = session_id};
    glink_err_type ret = lpai_bt_appmgr_send_endpt_msg_adsp(uapp->ep_id, UAPP_GATT_UNREGISTER_SERVICE_IND, 
        sizeof(uapp_gatt_unregister_service_ind_t), (void *)&ind, false);
#ifdef DEBUG_BUILD_LOG
    printk("Sent unregister_service %d\n", ret);
#endif
    return (ret == 0) ? QAPI_BT_GATT_RESULT_SUCCESS : QAPI_BT_GATT_RESULT_INTERNAL_ERROR;
}

/**
 * @brief Issues a GATT client read characteristic request.
 * 
 * @param req Structure with the session ID and attribute handle.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */
qapi_gatt_result_t qapi_gatt_client_read_char_req(
    qapi_bt_gatt_read_char_req_t req)
{
    /* validate sessionId */
    gatt_uapps_t * uapp = find_uapp_with_session_id(req.sessionId);
    if (uapp == NULL)
        return QAPI_BT_GATT_RESULT_INVALID_PARAM;

    uapp_gatt_char_read_req_t read_req = {.session_id=req.sessionId, .handle=req.attrHandle, .context=0x11111111};
    glink_err_type ret = lpai_bt_appmgr_send_endpt_msg_adsp (uapp->ep_id, UAPP_GATT_CLIENT_READ_CHAR_REQ, 
        sizeof(uapp_gatt_char_read_req_t), &read_req, false);
    return (ret == 0) ? QAPI_BT_GATT_RESULT_SUCCESS : QAPI_BT_GATT_RESULT_INTERNAL_ERROR;
}

/**
 * @brief Issues a GATT client write characteristic request.
 * 
 * @param req Structure with the session ID and attribute/data to write.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */

qapi_gatt_result_t qapi_gatt_client_write_char_req
    (qapi_bt_gatt_write_char_req_t req)
{
    /* validate input parameters like sessionId, value buffer*/
    gatt_uapps_t * uapp = find_uapp_with_session_id(req.sessionId);
    if (uapp == NULL || (req.val_len && !req.val))
        return QAPI_BT_GATT_RESULT_INVALID_PARAM;

    uint16_t len = sizeof(uapp_gatt_char_write_req_t) + req.val_len;
    uapp_gatt_char_write_req_t *write_req = malloc(len);
    if (write_req == NULL)
    {
        return QAPI_BT_GATT_RESULT_INSUFF_RESOURCES;
    }
    write_req->session_id = req.sessionId;
    write_req->handle = req.attrHandle;
    write_req->val_len = req.val_len;
    write_req->writeWithoutRsp = req.writeWithoutRsp;
    write_req->context = 0x22222222; //test purpose
    memscpy(&write_req->val[0], QAPI_GATT_MTU_MAX, req.val, req.val_len);

    glink_err_type ret = lpai_bt_appmgr_send_endpt_msg_adsp (uapp->ep_id, UAPP_GATT_CLIENT_WRITE_CHAR_REQ, len, write_req, false);
    free(write_req);
    return (ret == 0) ? QAPI_BT_GATT_RESULT_SUCCESS : QAPI_BT_GATT_RESULT_INTERNAL_ERROR;
}

/**
 * @brief Sends a notification/indication response for a GATT client request.
 * 
 * @param rsp Notification/Indication response data.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */
qapi_gatt_result_t qapi_gatt_client_notif_rsp 
    (qapi_bt_gatt_char_notif_rsp_t rsp)
{
    /* place holder to send response for notification/indication to remote. 
        Micro Stack do not expect any response for notification as of now */
    
    return QAPI_BT_GATT_RESULT_SUCCESS;
}

/**
 * @brief Responds to a server-side GATT read characteristic request.
 * 
 * @param rsp Read response data.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */

qapi_gatt_result_t qapi_gatt_server_read_char_rsp(
    qapi_bt_gatt_read_char_rsp_t rsp)
{
    /* validate input parameters like sessionId */
    gatt_uapps_t * uapp = find_uapp_with_session_id(rsp.sessionId);
    if (uapp == NULL || (rsp.val_len && !rsp.val))
        return QAPI_BT_GATT_RESULT_INVALID_PARAM;

#ifdef DEBUG_BUILD_LOG
    if (rsp.val == NULL) 
        printk("NULL value\n");
#endif
    uint16_t len = sizeof(uapp_gatt_char_read_rsp_t) + rsp.val_len;
    uapp_gatt_char_read_rsp_t *read_rsp = malloc(len);
    if (read_rsp == NULL)
    {
        return QAPI_BT_GATT_RESULT_INSUFF_RESOURCES;
    }
    read_rsp->session_id = rsp.sessionId;
    read_rsp->handle = rsp.attrHandle;
    read_rsp->val_len = rsp.val_len;
    read_rsp->result = rsp.result;
    memscpy(&read_rsp->val[0], QAPI_GATT_MTU_MAX, rsp.val, rsp.val_len);
    glink_err_type ret = lpai_bt_appmgr_send_endpt_msg_adsp
        (uapp->ep_id, UAPP_GATT_SERVER_READ_CHAR_IND_RESP, len, read_rsp, false);
    free(read_rsp);
    return (ret == 0) ? QAPI_BT_GATT_RESULT_SUCCESS : QAPI_BT_GATT_RESULT_INTERNAL_ERROR;
}

/**
 * @brief Responds to a server-side GATT write characteristic request.
 * 
 * @param rsp Write response data.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */
qapi_gatt_result_t qapi_gatt_server_write_char_rsp(
    qapi_bt_gatt_write_char_rsp_t rsp)
{
    /* validate input parameters like sessionId */
    gatt_uapps_t * uapp = find_uapp_with_session_id(rsp.sessionId);
    if (uapp == NULL)
        return QAPI_BT_GATT_RESULT_INVALID_PARAM;

    uapp_gatt_char_write_rsp_t write_rsp = {.session_id=rsp.sessionId, .handle=rsp.attrHandle, .result=rsp.result};
    glink_err_type ret = lpai_bt_appmgr_send_endpt_msg_adsp
        (uapp->ep_id, UAPP_GATT_SERVER_WRITE_CHAR_IND_RESP, sizeof(uapp_gatt_char_write_rsp_t), &write_rsp, false);
    return (ret == 0) ? QAPI_BT_GATT_RESULT_SUCCESS : QAPI_BT_GATT_RESULT_INTERNAL_ERROR;
}

/**
 * @brief Sends a notification/indication as a GATT server to a client.
 * 
 * @param req Notification structure.
 * @return QAPI_BT_GATT_RESULT_SUCCESS on success, error otherwise.
 */
qapi_gatt_result_t qapi_gatt_server_send_char_notif( 
    qapi_bt_gatt_char_notif_t req)
{
    /* validate input parameters like ep_id, sessionId */
    gatt_uapps_t * uapp = find_uapp_with_session_id(req.sessionId);
    if (uapp == NULL || (req.val_len && !req.val))
        return QAPI_BT_GATT_RESULT_INVALID_PARAM;
#ifdef DEBUG_BUILD_LOG
    if (req.val == NULL) 
        printk("NULL value\n");
#endif

    uint16_t len = sizeof(uapp_gatt_char_notif_ind_t) + req.val_len;
    uapp_gatt_char_notif_ind_t *notif_ind = malloc(len);
    if (notif_ind == NULL)
    {
        return QAPI_BT_GATT_RESULT_INSUFF_RESOURCES;
    }

    notif_ind->session_id = req.sessionId;
    notif_ind->handle = req.attrHandle;
    notif_ind->val_len = req.val_len;
    notif_ind->context = 0x33333333;//test purpose
    memscpy(&notif_ind->val[0], QAPI_GATT_MTU_MAX, req.val, req.val_len);
    glink_err_type ret = lpai_bt_appmgr_send_endpt_msg_adsp
        (uapp->ep_id, UAPP_GATT_SERVER_NOTIFICATION_REQ, len, notif_ind, false);
    free(notif_ind);
    return (ret == 0) ? QAPI_BT_GATT_RESULT_SUCCESS : QAPI_BT_GATT_RESULT_INTERNAL_ERROR;
}

