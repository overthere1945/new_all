/*==============================================================================*/
/*
 * Copyright (c) 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
FILE      profile_mgr_gatt.c
===========================================================================*/
/**
 * @file profile_mgr_gatt.c
 * @brief Handles GATT profile communication with Microstack.
 *
 * @details
 *   This file implements the Bluetooth GATT Profile Manager for offload feature.
 *   It primarily handles:
 *     1. GATT profile communication with Microstack.
 *        - Enable GATT Service
 *        - Disable GATT Service
 *        - Clearing all GATT Services on ACL handle
 *        - GATT error reporting on ACL handle
 *        - Data transmission/reception on GATT Characteristics
 *   The implementation includes:
 *     - Registration and unregistration of GATT services and endpoints
 *     - Management of GATT service and characteristic data
 *     - Handling of HAL, Microstack, UAPP, and Ustack messages
 *     - Maintenance of GATT session state and error reporting
 *     - Interfacing with offload framework and application endpoints
 */


/*============================================================================*
                                INCLUDE FILES
*============================================================================*/
#include "offload_mgr_msg_utils.h"
#include "profile_mgr_gatt.h"
#include "offload_mgr_log.h"
#include "offload_mgr_client_interface.h"
#include "offload_mgr_internal.h"
#include "endpt_mgr.h"
#include "bt_pal_heap.h"
#include "bt_pal_assert.h"
#include "endpt_mgr_rpc.h"
#include "offload_instance.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "offload_mgr_transport_shim.h"
#include "offload_mgr_proto_utils.h"


/*============================================================================*
                                MACRO DEFINITIONS
*============================================================================*/

/**
 * @def PM_GATT_APP_HANDLE
 * @brief Application handle used for GATT Profile Manager registration with Microstack.
 */
#define PM_GATT_APP_HANDLE 0x7822

/**
 * @def GATT_UAPP_EVT_GET_SESSION_ID
 * @brief Extracts the session ID from a UAPP event pointer.
 * @param evt Pointer to the event data.
 */
#define GATT_UAPP_EVT_GET_SESSION_ID(evt)  (*((int32_t *)(evt)))

/**
 * @def GATT_UAPP_EVT_GET_HANDLE
 * @brief Extracts the handle from a UAPP event pointer.
 * @param evt Pointer to the event data.
 */
#define GATT_UAPP_EVT_GET_HANDLE(evt)      (*((uint16_t *)((uint8_t *)(evt) + sizeof(int32_t) + sizeof(uint32_t))))

/**
 * @def GATT_UAPP_EVT_GET_UAPP_CTX
 * @brief Extracts the handle from a UAPP event pointer.
 * @param evt Pointer to the event data.
 */
#define GATT_UAPP_EVT_GET_UAPP_CTX(evt)      (*((uint32_t *)((uint8_t *)(evt) + sizeof(int32_t))))

/**
 * @def DEBUG_BUILD_LOGS
 * @brief Enable debug build logs for GATT proto module.
 */
//#define DEBUG_BUILD_LOGS

/*===========================================================================
                        TYPE DEFINITINOS
===========================================================================*/

/**
 * @struct gatt_char_holder_t
 * @brief Structure holding the properties and UUID of a GATT characteristic.
 *
 * @details Used during proto decoding of Glink message.
 */
typedef struct gatt_char_holder 
{
    uint8_t uuid[GATT_OFFLOAD_UUID_BYTE_LEN];  /**< 128-bit UUID for characteristic */
    int32_t properties;                        /**< Properties of characteristic */
    int32_t valueHandle;                       /**< Value handle */
} gatt_char_holder_t;

/**
 * @struct char_list_holder_t
 * @brief Structure holding a list of GATT characteristics.
 *
 * @details Used during proto decoding of Glink message.
 */
typedef struct char_list_holder
{
    gatt_char_holder_t chars[GATT_OFFLOAD_SESSION_MAX_CHARS];       /**< Array of characteristics */
    uint8_t num_chars;                                              /**< Number of characteristics */
} char_list_holder_t;

/**
 * @struct gatt_ctx_holder_t
 * @brief Structure holding context for GATT service registration.
 *
 * @details Used during proto decoding for GATT service registration.
 */
typedef struct gatt_ctx_holder 
{
    gatt_register_service   reg_svc;                                /**< Registration service info */
    uint8_t                 svc_uuid_holder[GATT_OFFLOAD_UUID_BYTE_LEN]; /**< Service UUID */
    char_list_holder_t      char_list_holder;                       /**< List of characteristics */
}gatt_ctx_holder_t;


/**
 * @brief Hub ID for AWM.
 * This variable holds the hub ID used by the endpoint manager for AWM.
 */
extern uint64_t endpt_mgr_hub_id_awm;

/**
 * @brief Offload Framework GATT ID.
 * Stores the GATT ID assigned to offload fw.
 */
CsrBtGattId fw_gatt_id;

/**
 * @brief Array of offload instances.
 * Stores all offload_instance_t objects, each representing an offload context for GATT services.
 */
extern offload_instance_t offload_instances[MAX_OFFLOAD_INSTANCES];

/**
 * @brief Array of GATT endpoints for MicroApps (UAPP).
 *
 * Stores GATT endpoint objects for each registered UAPP.
 */
gatt_endpoint_t gatt_uapp[GATT_OFFLOAD_MAX_ENDPOINT];

/**
 * @brief Microstack callback for GATT events.
 *
 * Handles events from Microstack for the GATT Profile Manager.
 *
 * @param handle Application handle.
 * @param eventClass Event class.
 * @param message Pointer to the event message.
 */
static void microstack_gatt_cb(BtAppHandle handle, BtEventClass eventClass, void *message);

/*============================================================================*
                                FUNCTION DEFINITIONS
*============================================================================*/

/*===========================================================================
FUNCTION      profile_mgr_gatt_init
===========================================================================*/
/**
 * @brief Initializes the GATT Profile Manager.
 * @details
 *   - Reset all GATT endpoints for MicroApps.
 *   - Registers the Microstack callback for GATT events.
 *   - Sends Offload Framework GATT registration request to Microstack.
 */
void profile_mgr_gatt_init(void)
{
    memset(gatt_uapp, 0, (sizeof(gatt_endpoint_t)  * GATT_OFFLOAD_MAX_ENDPOINT));
    microstack_register_app_callback(PM_GATT_APP_HANDLE, microstack_gatt_cb);
    CsrBtGattRegisterReqSend(PM_GATT_APP_HANDLE, GATT_OFFLOAD_FRAMEWORK_REG_CB_CTX);
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_deinit
===========================================================================*/
/**
 * @brief Deinitializes the GATT Profile Manager.
 * @details
 *   - Unregisters GATT service from Microstack.
 *   - Deregisters Microstack callback.
 *   - Clears all GATT endpoints for MicroApps.
 */
void profile_mgr_gatt_deinit(void)
{
    CsrBtGattUnregisterReqSend(fw_gatt_id);
    microstack_deregister_app_callback(PM_GATT_APP_HANDLE);
    memset(gatt_uapp, 0, (sizeof(gatt_endpoint_t)  * GATT_OFFLOAD_MAX_ENDPOINT));
}

/*===========================================================================
FUNCTION      find_gatt_endpoint
===========================================================================*/
/**
 * @brief Finds a GATT endpoint by match type.
 * @details
 *   - Searches for a GATT endpoint in the gatt_uapp array by endpoint ID or GATT ID.
 *   - Returns pointer to matching endpoint or NULL if not found.
 * @param match_type Type of match (endpoint ID or GATT ID).
 * @param ep_id Endpoint ID to match.
 * @param gatt_id GATT ID to match.
 * @return Pointer to matching gatt_endpoint_t or NULL.
 */
gatt_endpoint_t* find_gatt_endpoint(gatt_endpoint_match_t match_type, uint64_t ep_id, uint32_t gatt_id) 
{
    for (uint8_t i = 0; i < GATT_OFFLOAD_MAX_ENDPOINT; i++) 
    {
        if ((match_type == ENDPT_MATCH_EP_ID  && gatt_uapp[i].ep_id == ep_id) ||
            (match_type == ENDPT_MATCH_GATT_ID && gatt_uapp[i].gattId == gatt_id))
        {
            return &gatt_uapp[i];
        }
    }
    return NULL;
}

/*===========================================================================
FUNCTION      fetch_gatt_session_in_endpoint
===========================================================================*/
/**
 * @brief Fetches a GATT session in an endpoint by connection ID and characteristic handle.
 * @details
 *   - Searches all services and sessions in an endpoint for a matching connection ID and characteristic handle.
 *   - Returns pointer to matching session or NULL.
 * @param endpoint Pointer to GATT endpoint.
 * @param bt_connId Bluetooth connection ID.
 * @param char_handle Characteristic handle.
 * @return Pointer to matching gatt_session_t or NULL.
 */
gatt_session_t* fetch_gatt_session_in_endpoint(   gatt_endpoint_t *endpoint,       uint32_t bt_connId, uint16_t char_handle)
{
    gatt_svc_t *svc = NULL;
    gatt_session_t *session = NULL;
    for (uint8_t svc_idx = 0; svc_idx < GATT_OFFLOAD_ENDPOINT_MAX_SERVICES; ++svc_idx) 
    {
        svc = &endpoint->svc_list[svc_idx];
        if (svc->btConnId == bt_connId) 
        {
            for (uint8_t sess_idx = 0; sess_idx < GATT_OFFLOAD_SERVICE_MAX_SESSIONS; ++sess_idx) 
            {
                session = &svc->sessions[sess_idx];
                for (size_t char_idx = 0; char_idx < GATT_OFFLOAD_SESSION_MAX_CHARS; ++char_idx) 
                {
                    if (session->char_list[char_idx].handle == char_handle) 
                    {
                        return session;
                    }
                }
            }
        }
    }
    return NULL; // Not found
}

/*===========================================================================
FUNCTION      fetch_num_sessions_in_svc
===========================================================================*/
/**
 * @brief Counts the number of valid sessions in a GATT service.
 * @details
 *   - Iterates through all sessions in a service and counts those with valid session IDs.
 * @param svc Pointer to GATT service.
 * @return Number of valid sessions.
 */
uint8_t fetch_num_sessions_in_svc(gatt_svc_t *svc)
{
    uint8_t num_sessions = 0;
    for (uint8_t i=0; i<GATT_OFFLOAD_SERVICE_MAX_SESSIONS; ++i) 
    {
        if (svc->sessions[i].session_id != GATT_OFFLOAD_INVALID_SESSION_ID)
            num_sessions++;
    }
    return num_sessions;
}

/*===========================================================================
FUNCTION      find_acl_handle_by_btconn_id
===========================================================================*/
/**
 * @brief Finds ACL handle by btconn_id.
 * @details
 *   - Searches all offload instances for a matching btconn_id.
 *   - Returns TRUE and sets ACL handle if found, otherwise FALSE.
 * @param btConnId Bluetooth connection ID.
 * @param aclHandle Pointer to store ACL handle.
 * @return TRUE if found, FALSE otherwise.
 */
bool find_acl_handle_by_btconn_id(uint32_t btConnId, int32_t            *aclHandle)
{
    const offload_instance_t *offload = NULL;
    for (uint16_t i=0; i<MAX_OFFLOAD_INSTANCES; i++)
    {
        offload = &offload_instances[i];
        if (offload->offload_instance_type == OFFLOAD_INSTANCE_TYPE_GATT &&
            offload->state_ctx->data)
        {
            gatt_svc_t *svc = offload->state_ctx->data;
            *aclHandle = svc->acl_handle;
            return TRUE;
        }
    }
    return FALSE;
}

/*===========================================================================
FUNCTION      gatt_handle_exists_in_session
===========================================================================*/
/**
 * @brief Checks if a characteristic handle belongs to a given GATT session.
 * @details
 *   - Iterates through the char_list of the session and looks for the handle.
 * @param session Pointer to GATT session.
 * @param handle  Characteristic handle to check.
 * @return TRUE if handle exists in this session, FALSE otherwise.
 */
static bool gatt_handle_exists_in_session(gatt_session_t *session, uint16_t handle)
{
    if (!session)
    {
        return FALSE;
    }

    for (uint8_t i = 0; i < GATT_OFFLOAD_SESSION_MAX_CHARS; ++i)
    {
        if (session->char_list[i].handle == handle)
        {
            return TRUE;
        }
    }
    return FALSE;
}


/*===========================================================================
FUNCTION      create_offload_instance_and_session
===========================================================================*/
/**
 * @brief Creates or finds an offload instance and session for a GATT endpoint.
 * @details
 *   - Finds or creates a matching service in the endpoint.
 *   - Appends a session to the service.
 *   - Finds or creates an offload instance for the endpoint and service.
 *   - Returns pointer to offload instance or NULL if no slot available.
 * @param endpoint Pointer to GATT endpoint.
 * @param ctx Pointer to GATT context holder.
 * @param session_idx Pointer to store session index.
 * @return Pointer to offload_instance_t or NULL.
 */
offload_instance_t* create_offload_instance_and_session(gatt_endpoint_t* endpoint, 
    gatt_ctx_holder_t *ctx) 
{
    /* 1. Find matching service in svc_list[]
     * 2. If Service does not exist: create new service entry
     * 3. Append session to svc
     * 4. create offload_instance_t for this (endpoint + service)
     */
    uint8_t session_idx;
    gatt_svc_t* svc = NULL;
    /* 1. Find matching service in svc_list[] */
    for (uint8_t i = 0; i < GATT_OFFLOAD_ENDPOINT_MAX_SERVICES; ++i) 
    {
        if (endpoint->svc_list[i].acl_handle == ctx->reg_svc.aclConnectionHandle && 
            endpoint->svc_list[i].role == ctx->reg_svc.role &&
            memcmp(endpoint->svc_list[i].uuid, ctx->svc_uuid_holder, GATT_OFFLOAD_UUID_BYTE_LEN) == 0) 
        {
            svc = &endpoint->svc_list[i];
            break;
        }
    }

    if (svc == NULL) 
    {
        /* 2. Service does not exist: create new service entry */
        for (uint8_t i = 0; i < GATT_OFFLOAD_ENDPOINT_MAX_SERVICES; ++i) 
        {
            if (endpoint->svc_list[i].offloadId == GATT_OFFLOAD_ID_INVALID) 
            { // Unused slot
                svc = &endpoint->svc_list[i];
                memset(svc, 0, sizeof(gatt_svc_t));
                svc->acl_handle = ctx->reg_svc.aclConnectionHandle;
                svc->role = ctx->reg_svc.role;
                memcpy(svc->uuid, ctx->svc_uuid_holder, GATT_OFFLOAD_UUID_BYTE_LEN);
                svc->btConnId = CSR_BT_CONN_ID_INVALID;
                svc->mtu = ctx->reg_svc.attMtu;
                break;
            }
        }
        if (svc == NULL) 
            return NULL; // No room for service
    }

    /* 3. Append session to svc */
    session_idx = GATT_OFFLOAD_SERVICE_MAX_SESSIONS;
    for (uint8_t i = 0; i < GATT_OFFLOAD_SERVICE_MAX_SESSIONS; ++i) 
    {
        if (svc->sessions[i].session_id == GATT_OFFLOAD_INVALID_SESSION_ID) 
        {
            svc->sessions[i].session_id = ctx->reg_svc.sessionId;
            for (uint8_t s=0; s<ctx->char_list_holder.num_chars; s++)
            {
                svc->sessions[i].char_list[s].handle = ctx->char_list_holder.chars[s].valueHandle;
                svc->sessions[i].char_list[s].prop = ctx->char_list_holder.chars[s].properties;
            }
            session_idx = i;
            break;
        }
    }
    if (session_idx == GATT_OFFLOAD_SERVICE_MAX_SESSIONS) 
        return NULL; // No free session slot

    /* 4. create offload_instance_t for this (endpoint + service) */
    offload_instance_t* inst = NULL;
    for (uint8_t i = 0; i < MAX_OFFLOAD_INSTANCES; ++i) 
    {
        if (offload_instances[i].offload_state == Idle) 
        {
            inst = &offload_instances[i];
            inst->offload_instance_type = OFFLOAD_INSTANCE_TYPE_GATT;
            inst->ep_id.id = ctx->reg_svc.endpointId.endpointID;
            inst->ep_id.hub_id = ctx->reg_svc.endpointId.hubID;
            inst->offload_data.gatt_session = &svc->sessions[session_idx];
            inst->state_ctx->data = svc;
            break;
        }
    }
    return inst;
}

/*===========================================================================
FUNCTION      compare_char_handle
===========================================================================*/
/**
 * @brief Comparison function for characteristic handles.
 * @details
 *   - Used for sorting characteristic handles in ascending order.
 * @param a Pointer to first characteristic.
 * @param b Pointer to second characteristic.
 * @return -1, 0, or 1 for qsort.
 */
static int compare_char_handle(const void *a, const void *b)
{
    const gatt_chars_t *ca = (const gatt_chars_t *)a;
    const gatt_chars_t *cb = (const gatt_chars_t *)b;
    if (ca->handle < cb->handle) return -1;
    if (ca->handle > cb->handle) return 1;
    return 0;
}

/** Bubble Sort */
static void sort(gatt_chars_t *char_list, uint16_t size, int (*compare)(const void *a, const void *b))
{
    if(compare == NULL)return;
    for(uint16_t i = 0; i < size - 1; i++)
    {
        for(uint16_t j = 0; j < size - i - 1; j++)
        {
            if(compare(char_list + j, char_list + j + 1) > 0)
            {
                /** swap */
                gatt_chars_t t_char = char_list[j];
                char_list[j] = char_list[j + 1];
                char_list[j + 1] = t_char;
            }
        }
    }
}

/*===========================================================================
FUNCTION      prepare_ustack_char_list
===========================================================================*/
/**
 * @brief Prepares a list of GATT characteristics for Ustack operations.
 * @details
 *   - Aggregates all characteristics from sessions in a service.
 *   - Optionally excludes a session for disable operations.
 *   - Sorts the list by handle.
 * @param svc Pointer to GATT service.
 * @param char_list Output array for characteristics.
 * @param list_cnt Output pointer for count.
 * @param op Ustack operation type.
 * @param exclude_session Session to exclude (optional).
 */
void prepare_ustack_char_list(
        gatt_svc_t  *svc,
        CsrBtGattChar *char_list,     // OUT
        uint8_t *list_cnt,            // OUT
        ustack_op_t op,
        gatt_session_t *exclude_session)
{
    uint8_t idx = 0;
    gatt_session_t *session = NULL;
    bool exclude;

    // Aggregate all chars as before
    for ( uint8_t s=0; s < GATT_OFFLOAD_SERVICE_MAX_SESSIONS; ++s) 
    {
        session = &svc->sessions[s];
        exclude = (op == USTACK_DISABLE_SERVICE) && (session == exclude_session);
        if (!exclude) 
         {
            for (uint8_t i = 0; i < GATT_OFFLOAD_SESSION_MAX_CHARS ; i++) 
            {
                if (session->char_list[i].handle != GATT_OFFLOAD_INVALID_CHAR_HANDLE) 
                {
                    char_list[idx].handle = session->char_list[i].handle;
                    char_list[idx].properties = session->char_list[i].prop;
                    idx++;
                }
            }
        }
    }
    *list_cnt = idx;

    // Sort char_list by handle in ascending order
    if (idx > 1) 
    {
        sort(char_list, idx, compare_char_handle);
    }
}

/*===========================================================================
							FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
FUNCTION      proto_decode_struct
===========================================================================*/
/**
 * @brief Decode a protobuf message into a destination structure.
 * @details
 *   - Creates a protobuf input stream from the buffer.
 *   - Decodes the message using the provided field descriptors.
 *   - Stores the result in the destination structure.
 * @param fields Pointer to protobuf message descriptor.
 * @param evt Pointer to input buffer.
 * @param len Length of input buffer.
 * @param dest_struct Pointer to destination structure.
 * @return TRUE if decoding succeeds, FALSE otherwise.
 */ 
bool proto_decode_struct(
    const pb_msgdesc_t *fields,
    uint8_t *evt,
    size_t len,
    void *dest_struct
    )
{
    pb_istream_t stream = pb_istream_from_buffer(evt, len);
    bool decode = pb_decode(&stream, fields, dest_struct);
    return decode;
}

/*===========================================================================
FUNCTION      proto_encode_and_frame
===========================================================================*/
/**
 * @brief Encode a structure into a protobuf message and frame it for transport.
 * @details
 *   - Creates a protobuf output stream for the transport buffer.
 *   - Encodes the message using the provided field descriptors.
 *   - Updates the transport header with opcode and length.
 *   - Optionally returns the number of encoded bytes.
 * @param fields Pointer to protobuf message descriptor.
 * @param src_struct Pointer to source structure.
 * @param opcode Opcode for the message.
 * @param encoded_bytes_out Optional pointer to store encoded byte count.
 * @return TRUE if encoding succeeds, FALSE otherwise.
 */ 
bool proto_encode_and_frame(
    const pb_msgdesc_t *fields,
    const void *src_struct,
    uint16_t opcode,
    size_t *encoded_bytes_out /* optional, can pass NULL */)
{
    pb_ostream_t stream = pb_ostream_from_buffer(
        offload_mgr_transport_shim_buf + OFFLOAD_MGR_TRANSPORT_SHIM_HDR_LEN,
        OFFLOAD_MGR_TRANSPORT_SHIM_BUF_LEN);

    bool ok = pb_encode(&stream, fields, src_struct);

    if (ok) 
    {
        offload_mgr_transport_shim_encoded_buf_len = stream.bytes_written;
        Offload_Mgr_Update_Header(offload_mgr_transport_shim_buf, opcode, stream.bytes_written);
        if (encoded_bytes_out) 
        {
            *encoded_bytes_out = stream.bytes_written;
        }
    }
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("encode %d: opcode=0x%04x length=%zu", ok, opcode, stream.bytes_written);
#endif
    return ok;
}

#ifdef DEBUG_BUILD_LOGS
/*===========================================================================
FUNCTION      log_gatt_reg_svc_data
===========================================================================*/
/**
 * @brief Logs GATT register service data for debugging.
 * @details
 *   - Logs session ID, ACL handle, MTU, role, service UUID, and characteristic list.
 *   - Only compiled in debug builds.
 * @param ctx Pointer to GATT context holder.
 */
void log_gatt_reg_svc_data(gatt_ctx_holder_t *ctx)
{
    gatt_register_service *reg_service = &ctx->reg_svc;

    OFFLOAD_MGR_LOGL("session_id %d aclHandle %d Mtu %d role %d", \
	reg_service->sessionId, reg_service->aclConnectionHandle, reg_service->attMtu, reg_service->role);
    for (uint8_t i=0; i<GATT_OFFLOAD_UUID_BYTE_LEN; i++)
    {
        OFFLOAD_MGR_LOGL("svc_uuid[%d]: %d", i, ctx->svc_uuid_holder[i]);
    }
    // log endpoint Id
    
    OFFLOAD_MGR_LOGL("num_chars %d", ctx->char_list_holder.num_chars);
    for (uint8_t i=0; i<ctx->char_list_holder.num_chars; i++)
    {
        OFFLOAD_MGR_LOGL("attr_handle %d", ctx->char_list_holder.chars[i].valueHandle);
        OFFLOAD_MGR_LOGL("properties %d", ctx->char_list_holder.chars[i].properties);
        for (uint8_t j=0; j<GATT_OFFLOAD_UUID_BYTE_LEN; j++)
        {
            OFFLOAD_MGR_LOGL("char_uuid[%d]: %d", j, ctx->char_list_holder.chars[i].uuid[j]);
        }
    }
}
#endif /* DEBUG_BUILD_LOGS */

/*===========================================================================
FUNCTION      char_list_decode_callback
===========================================================================*/
/**
 * @brief Callback function for decoding a list of GATT characteristics from protobuf.
 * @details
 *   - Decodes a single GATT characteristic from the protobuf stream.
 *   - Stores properties, value handle, and UUID in the holder.
 *   - Increments the number of decoded characteristics.
 * @param stream Pointer to protobuf input stream.
 * @param field Pointer to protobuf field descriptor.
 * @param arg Pointer to char_list_holder_t.
 * @return TRUE if decoding succeeds, FALSE otherwise.
 */
bool char_list_decode_callback(pb_istream_t *stream, const pb_field_t *field, void **arg) 
{
    char_list_holder_t *holder = (char_list_holder_t*)(*arg);
    if (holder->num_chars >= GATT_OFFLOAD_SESSION_MAX_CHARS) return false;

    GattCharacteristic gatt_char = GattCharacteristic_init_zero;
    uint8_t char_uuid[GATT_OFFLOAD_UUID_BYTE_LEN] = {0};
    gatt_char.uuid.uuid.arg = &char_uuid;
    gatt_char.uuid.uuid.funcs.decode = decode__bytes;
    if (!pb_decode(stream, GattCharacteristic_fields, &gatt_char))
        return false;

    holder->chars[holder->num_chars].properties = gatt_char.properties;
    holder->chars[holder->num_chars].valueHandle = gatt_char.valueHandle;
    memcpy(&holder->chars[holder->num_chars].uuid[0], &char_uuid[0], GATT_OFFLOAD_UUID_BYTE_LEN);

    holder->num_chars++;
    return true;
}

/*===========================================================================
FUNCTION      decode_gatt_register_service
===========================================================================*/
/**
 * @brief Decodes a GATT register service protobuf message.
 * @details
 *   - Sets up decode callbacks for service UUID and characteristic list.
 *   - Decodes the message into the context holder.
 *   - Logs the decoded data if debug is enabled.
 * @param buffer Pointer to input buffer.
 * @param buf_len Length of input buffer.
 * @param ctx Pointer to GATT context holder.
 * @return TRUE if decoding succeeds, FALSE otherwise.
 */
bool decode_gatt_register_service(
    const uint8_t *buffer, size_t buf_len, gatt_ctx_holder_t *ctx)
{
    gatt_register_service *service = &ctx->reg_svc;

    service->uuid.uuid.funcs.decode = decode__bytes;
    service->uuid.uuid.arg = &ctx->svc_uuid_holder[0];

    ctx->char_list_holder.num_chars = 0;
    service->gattCharacteristic.funcs.decode = &char_list_decode_callback;
    service->gattCharacteristic.arg = &ctx->char_list_holder;

    /* Decode */
    bool status = proto_decode_struct(gatt_register_service_fields, buffer, buf_len, service);
#ifdef DEBUG_BUILD_LOGS
    if (!status)
        OFFLOAD_MGR_LOGM("Decode failed");
    else
        log_gatt_reg_svc_data(ctx);
#endif
    return status;
}

/*===========================================================================
FUNCTION      encode_uapp_gatt_register_service_req
===========================================================================*/
/**
 * @brief Encodes and frames a MicroApp (UAPP) GATT register service request.
 * @details
 *   - Frames the endpoint manager RPC header for the register service request.
 *   - Copies the encoded GATT service buffer into the header.
 *   - Sets message format, opcode, data length, and endpoint ID.
 * @param offload Pointer to offload instance.
 * @param header Pointer to endpoint manager RPC header.
 * @return TRUE if encoding and framing succeeds, FALSE otherwise.
 */ 
bool encode_uapp_gatt_register_service_req(offload_instance_t *offload, 
    endpt_mgr_rpc_header_t *header)
{
    gatt_svc_t * svc = offload->state_ctx->data;
    if (svc && svc->buf.data)
    {
    	/* Frame the header */
        header->message_format = MESSAGE_FORMAT_PROTO;
        header->opcode = UAPP_GATT_SESSION_REGISTERED_REQ;
    	header->data_len = offload_mgr_transport_shim_encoded_buf_len = \
            ENDPT_MGR_RPC_HDR_SIZE + svc->buf.len;
        header->endpoint_id = offload->ep_id.id;
        memcpy(header->data, svc->buf.data, svc->buf.len);
        return true;
    }

    return false;
}


/*===========================================================================
FUNCTION      gatt_register_service_rsp
===========================================================================*/
/**
 * @brief Sends GATT register service response to HAL.
 * @details
 *   - Encodes and sends register service complete message to HAL.
 * @param sessionId Session ID.
 * @param status GATT status.
 */
void gatt_register_service_rsp(int32_t sessionId, GattStatus status)
{
    /* status is failure or success */
    gatt_register_service_complete reg_serv_resp = {
        .sessionId = sessionId,
        .status =  status,
    };
    proto_encode_and_frame(gatt_register_service_complete_fields, &reg_serv_resp, 
        BT_OFFLOAD_GATT_REGISTER_SERVICE, NULL);
    Offload_Mgr_SendDataToBTGattHal(offload_mgr_transport_shim_buf,
                                      offload_mgr_transport_shim_encoded_buf_len);
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("REG_SVC_CMP sent to HAL sessionid %d status %d", 
    reg_serv_resp.sessionId, reg_serv_resp.status);
#endif
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_offload_rsp
===========================================================================*/
/**
 * @brief Handles offload response to HAL.
 * @details
 *   - Sends register service response to HAL.
 *   - Frees allocated buffer for GATT service.
 *   - Clears session data on failure.
 * @param data Pointer to offload instance.
 * @param status Status code.
 */
void profile_mgr_gatt_hal_offload_rsp(void *data, int status) 
{
    offload_instance_t *offload = data;
    gatt_session_t *session = offload->offload_data.gatt_session;
    gatt_register_service_rsp(session->session_id, status);

    gatt_svc_t *svc = offload->state_ctx->data;
    if (svc && svc->buf.data)
    {
        bt_pal_free(svc->buf.data);
        svc->buf.len = 0;
        svc->buf.data = NULL;
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_unoffload_rsp
===========================================================================*/
/**
 * @brief Handles unoffload response from HAL.
 * @details
 *   - Sends unregister service response to HAL.
 *   - Clears session data.
 * @param data Pointer to offload instance.
 * @param status Status code.
 */
void profile_mgr_gatt_hal_unoffload_rsp(void *data, int status)
{
    offload_instance_t *offload = data;
    
    /* status is failure or success */
    gatt_unregister_service_complete unreg_serv_resp = {
        .sessionId = offload->offload_data.gatt_session->session_id,
    };

    proto_encode_and_frame(gatt_unregister_service_complete_fields, &unreg_serv_resp, 
        BT_OFFLOAD_GATT_UNREGISTER_SERVICE, NULL);
    Offload_Mgr_SendDataToBTGattHal(offload_mgr_transport_shim_buf,
                                      offload_mgr_transport_shim_encoded_buf_len);
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("UNREG_SVC_CMP sent to HAL sessionid %d", unreg_serv_resp.sessionId);
#endif
}

/*===========================================================================
FUNCTION      send_bt_gatt_hal_clear_services_rsp
===========================================================================*/
/**
 * @brief Sends clear services response to HAL.
 * @details
 *   - Encodes and sends clear services complete message to HAL.
 * @param acl_handle ACL connection handle.
 */
static void send_bt_gatt_hal_clear_services_rsp(int32_t acl_handle)
{
    /* status is failure or success */
    gatt_clear_services_complete clear_serv_rsp = {
        .aclConnectionHandle = acl_handle,
    };
    proto_encode_and_frame(gatt_clear_services_complete_fields, &clear_serv_rsp, 
        BT_OFFLOAD_GATT_CLEAR_SERVICES, NULL);
    Offload_Mgr_SendDataToBTGattHal(offload_mgr_transport_shim_buf,
                                      offload_mgr_transport_shim_encoded_buf_len);
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("CLR_SVC_CMP sent to HAL aclConnectionHandle %d", clear_serv_rsp.aclConnectionHandle);
#endif
}

/*===========================================================================
FUNCTION      map_ustack_error_to_uapp_error
===========================================================================*/
/**
 * @brief Maps uStack result codes to UAPP status.
 * @details Translates GATT/ATT supplier and result code pairs corresponding 
 * gatt_uapp_status_t values used by the UAPP layer.
 * @param resultSupplier CSR_BT_SUPPLIER_GATT and CSR_BT_SUPPLIER_ATT suppliers.
 * @param resultCode Result code reported by the given supplier.
 * @return Mapped UAPP status corresponding to the given supplier and result code.
 */
static gatt_uapp_status_t map_ustack_error_to_uapp_error(CsrBtSupplier        resultSupplier, CsrBtResultCode     resultCode)
{
    gatt_uapp_status_t status  = statusOk;

    if (resultSupplier == CSR_BT_SUPPLIER_GATT)
    {
        switch(resultCode)
        {
            case CSR_BT_GATT_RESULT_SUCCESS:
            {
                status = statusOk;
                break;
            }
            case CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER:
            case CSR_BT_GATT_RESULT_UNKNOWN_CONN_ID:
            case CSR_BT_GATT_RESULT_INVALID_LENGTH:
            case CSR_BT_GATT_RESULT_INTERNAL_ERROR:
            case CSR_BT_GATT_RESULT_INVALID_ATTRIBUTE_VALUE_RECEIVED:
            case CSR_BT_GATT_RESULT_INVALID_HANDLE_RANGE:
            {
                status = statusInvalidParameter;
                break;
            }
            case CSR_BT_GATT_RESULT_INSUFFICIENT_NUM_OF_HANDLES:
            case CSR_BT_GATT_RESULT_ATTR_HANDLES_ALREADY_ALLOCATED:
            {
                status = statusResourceExhausted;
                break;
            }
            case CSR_BT_GATT_RESULT_CANCELLED:
            case CSR_BT_GATT_RESULT_BUSY:
            {
                status = statusInvalidOperation;
                break;
            }
            default:
                status = statusUnknown;
        }
    }
    else if (resultSupplier == CSR_BT_SUPPLIER_ATT)
    {
        if (resultCode != ATT_RESULT_SUCCESS)
        {
            status = statusResourceExhausted + resultCode;
        }
    }
    return status;
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_offload
===========================================================================*/
/**
 * @brief Handles offload request from HAL.
 * @details
 *   - Decodes register service event.
 *   - Validates endpoint and characteristic limits.
 *   - Creates offload instance and session.
 *   - Stores HAL GATT context for later use.
 *   - Handles offload event in state machine.
 * @param evt Pointer to event data.
 * @param len Length of event data.
 */
void profile_mgr_gatt_hal_offload(uint8_t *evt, size_t len)
{
    gatt_ctx_holder_t ctx;
    offload_instance_t *offload = NULL;

    memset(&ctx, 0, sizeof(ctx));

#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("profile_mgr_gatt_hal_offload:\n");
#endif

    // Decode, return on failure (session ID unknown)
    if (!decode_gatt_register_service(evt, len, &ctx))
        return;

    // Check endpoint
    gatt_endpoint_t *gatt_ep = find_gatt_endpoint(ENDPT_MATCH_EP_ID, ctx.reg_svc.endpointId.endpointID, 0);
    if (!gatt_ep) 
    {
        gatt_register_service_rsp(ctx.reg_svc.sessionId, GattStatus_INVALID_ENDPOINT_ID);
        return;
    }

    // Check char limit
    if (ctx.char_list_holder.num_chars > GATT_OFFLOAD_SESSION_MAX_CHARS) 
    {
        gatt_register_service_rsp(ctx.reg_svc.sessionId, GattStatus_INSUFFICIENT_RESOURCES);
        return;
    }

    /* Offload instance shall be created for each service on endpoint */
    offload = create_offload_instance_and_session(gatt_ep, &ctx);
    if (!offload) 
    {
        gatt_register_service_rsp(ctx.reg_svc.sessionId, GattStatus_INSUFFICIENT_RESOURCES);
        return;
    }

    //TBD: handling if state machine is busy

    /* gatt_svc_t shall store HAL GATT ctx parameters that will be used later. 
    As of now gatt_svc_t shall not store uuid of chars since it is not used by MicroStack.
    But complete HAL GATT ctx has to be sent micro-app. Hence store the proto encoded
    buffer, which will be reused to send GATT ctx to Micro app. */
    gatt_svc_t *svc = offload->state_ctx->data;
    svc->buf.data = bt_pal_malloc(len);
    if (!svc->buf.data) 
    {
        gatt_register_service_rsp(ctx.reg_svc.sessionId, GattStatus_INSUFFICIENT_RESOURCES);
        return;
    }
    memcpy(svc->buf.data, evt, len);
    svc->buf.len = len;

    offload_mgr_sm_handle_offload_evt(offload);
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_unoffload
===========================================================================*/
/**
 * @brief Handles unoffload request from HAL.
 * @details
 *   - Decodes unregister service event.
 *   - Finds offload instance by session ID.
 *   - Handles unoffload event in state machine.
 * @param evt Pointer to event data.
 * @param len Length of event data.
 */
void profile_mgr_gatt_hal_unoffload(uint8_t *evt, size_t len)
{
    const offload_instance_t *offload = NULL;
    gatt_unregister_service unreg_serv = gatt_unregister_service_init_default;
    bool status = proto_decode_struct(gatt_unregister_service_fields, evt, len, &unreg_serv);
    if (!status)
    {
        /* TBD: send a close ind back */
#ifdef DEBUG_BUILD_LOGS
        OFFLOAD_MGR_LOGM("proto_decode failed");
#endif
        return;
    }
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("session_id %d", unreg_serv.sessionId);
#endif

    /* get the offload instance */
    offload = offload_instance_find_by_gatt_session_id(unreg_serv.sessionId);
    if (offload)
    {
        offload_mgr_sm_handle_unoffload_evt((offload_instance_t *)offload);
        return;
    }
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGM("offload_mgr_gatt_unregister_service : No such session id");
#endif
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_get_caps
===========================================================================*/
/**
 * @brief Gets GATT offload capabilities.
 * @details
 *   - Retrieves GATT offload capabilities from MicroStack.
 *   - Encodes and sends capabilities response to HAL.
 */
void profile_mgr_gatt_hal_get_caps()
{
    GattOffloadCapabilities caps = {0};
    CsrBtResultCode result = GattOffloadGetCapabilities(&caps);
    gatt_get_capabilities_rsp gattCaps = {
        .capabilities = {
            .suppGattClientProperties = caps.clientProperties,
            .suppGattServerProperties = caps.serverProperties,
        }
    };
    proto_encode_and_frame(gatt_get_capabilities_rsp_fields, &gattCaps, 
        BT_OFFLOAD_GATT_GET_CAPABILITIES, NULL);
    Offload_Mgr_SendDataToBTGattHal(offload_mgr_transport_shim_buf,
                                     offload_mgr_transport_shim_encoded_buf_len);
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_clear_offload_data
===========================================================================*/
/**
 * @brief Clears offload data for GATT service.
 * @details
 *   - Resets GATT service data and pointer in offload instance.
 * @param d1 Pointer to offload instance.
 */
void profile_mgr_gatt_clear_offload_data(void *d1)
{
    offload_instance_t *offload = d1;
    gatt_session_t *session = offload->offload_data.gatt_session;
    memset(session, 0, sizeof(gatt_session_t));
    offload->offload_data.gatt_session = NULL;

    gatt_svc_t *svc = offload->state_ctx->data;
    if (!fetch_num_sessions_in_svc(svc))
    {
        memset (svc, 0, sizeof(gatt_svc_t));
    }
    offload->state_ctx->data = NULL;
}

/*===========================================================================
FUNCTION      clear_all_sessions_on_acl_handle
===========================================================================*/
/**
 * @brief Clears all GATT sessions on a given ACL handle.
 * @details
 *   - Iterates through offload instances and clears sessions matching ACL handle.
 *   - Sends unoffload requests to UAPP and clears offload state.
 * @param aclHandle ACL connection handle.
 * @param btConnId Output pointer for Bluetooth connection ID.
 */
void clear_all_sessions_on_acl_handle(int32_t aclHandle, uint32_t *btConnId)
{
    *btConnId = CSR_BT_CONN_ID_INVALID;
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("clear_sessions aclhandle %d", aclHandle);
#endif
    for (uint16_t i = 0; i < MAX_OFFLOAD_INSTANCES; i++) 
    {
        offload_instance_t *offload = &offload_instances[i];
        if (offload->offload_instance_type != OFFLOAD_INSTANCE_TYPE_GATT ||
            offload->offload_data.gatt_session == NULL ||
            offload->state_ctx->data == NULL)
                continue;

        gatt_svc_t *svc = offload->state_ctx->data;
        if (svc->acl_handle != aclHandle)
            continue;
        
        *btConnId = svc->btConnId;
        profile_mgr_gatt_unoffload_to_uapp(offload);

        /* offload_state_cb for moving to idle state and cleanup */
        offload->offload_state = Idle;
        offload->state_ctx->sm_state_ctx.prev_state = offload->state_ctx->sm_state_ctx.curr_state;
        offload->state_ctx->sm_state_ctx.curr_state = Idle;
        profile_mgr_gatt_clear_offload_data(offload);
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_hal_clear_service
===========================================================================*/
/**
 * @brief Handles clear service request from HAL.
 * @details
 *   - Decodes clear services event.
 *   - Clears all sessions on ACL handle.
 *   - Sends disable service request to MicroStack or clear services response to HAL.
 * @param evt Pointer to event data.
 * @param len Length of event data.
 */
void profile_mgr_gatt_hal_clear_service(uint8_t *evt, size_t len)
{
    gatt_clear_services clear_srvs = gatt_clear_services_init_default;
    bool status = proto_decode_struct(gatt_clear_services_fields, evt, len, &clear_srvs);
    if (!status)
        return;

#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("acl_handle: %d", clear_srvs.aclConnectionHandle);
#endif

    uint32_t btConnId;
    clear_all_sessions_on_acl_handle(clear_srvs.aclConnectionHandle, &btConnId);

    if (btConnId != CSR_BT_CONN_ID_INVALID)
    {
        ustack_cb_ctx_t *ctx = bt_pal_malloc(sizeof(ustack_cb_ctx_t));
        if (!ctx)
        {
            BT_PAL_ASSERT_FATAL_NO_ARGS("");
            return;
        }

        ctx->data = (int32_t *)clear_srvs.aclConnectionHandle;
        ctx->operation = USTACK_CLEAR_SERVICES;
        GattOffloadDisableServiceReqSend(fw_gatt_id, (CsrUint32)ctx, btConnId, 0, NULL); 
    } 
    else 
    {
        send_bt_gatt_hal_clear_services_rsp(clear_srvs.aclConnectionHandle);
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_ustack_enable_svc
===========================================================================*/
/**
 * @brief Enables GATT service in Ustack.
 * @details
 *   - Prepares characteristic list for service.
 *   - Sends enable service request to Ustack.
 * @param offload Pointer to offload instance.
 * @param ctx Pointer to Ustack callback context.
 * @param exclude_session Session to exclude (optional).
 */
void profile_mgr_gatt_ustack_enable_svc(offload_instance_t *offload, ustack_cb_ctx_t *ctx, 
    gatt_session_t *exclude_session)
{
    gatt_svc_t  *svc = offload->state_ctx->data;
    uint8_t list_cnt = 0;

    gatt_endpoint_t* ep = find_gatt_endpoint(ENDPT_MATCH_EP_ID, offload->ep_id.id, 0);
    if (!ep)
        return;

    CsrBtGattChar char_list[GATT_OFFLOAD_SERVICE_MAX_CHARS];
    prepare_ustack_char_list(svc, char_list, &list_cnt, ctx->operation, exclude_session);

    /** 3 */
    GattOffloadService *service = bt_pal_malloc(sizeof(GattOffloadService) + (list_cnt * sizeof(CsrBtGattChar)));
    if (!service)
    {
        BT_PAL_ASSERT_FATAL_NO_ARGS("");
        return;
    }

    service->gattId = ep->gattId;
    service->role = svc->role;
    service->offloadId = svc->offloadId;
    service->uuid.length = GATT_OFFLOAD_UUID_BYTE_LEN;
    memcpy(service->uuid.uuid, svc->uuid, GATT_OFFLOAD_UUID_BYTE_LEN);
    service->numChar = list_cnt;
    memcpy(service->charList, char_list, (sizeof(CsrBtGattChar) * list_cnt));
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("GattOffloadEnableServiceReqSend gattId %d btConnId %d offloadId %d numChar %d", 
            service->gattId, svc->btConnId, service->offloadId, service->numChar);
#endif
    GattOffloadEnableServiceReqSend(fw_gatt_id, (CsrUint32)ctx, svc->btConnId, service);
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_offload_to_ustack
===========================================================================*/
/**
 * @brief Offloads GATT service to Ustack.
 * @details
 *   - Allocates callback context.
 *   - Sends connection info or enable service request to Ustack.
 * @param d1 Pointer to offload instance.
 * @param d2 Unused.
 */
void profile_mgr_gatt_offload_to_ustack(void *d1, void *d2)
{
    offload_instance_t *offload = d1;
    gatt_svc_t *svc = offload->state_ctx->data;

    ustack_cb_ctx_t *ctx = bt_pal_malloc(sizeof(ustack_cb_ctx_t));
    if (!ctx)
    {
        BT_PAL_ASSERT_FATAL_NO_ARGS("");
        return;
    }
    
    ctx->data = d1;

    if (svc->btConnId == CSR_BT_CONN_ID_INVALID)
    {
        ctx->operation = USTACL_ACL_CONN_INFO;
        GattOffloadConnInfoReqSend (fw_gatt_id, (CsrUint32)ctx, svc->acl_handle, svc->mtu);
    }
    else
    {
        ctx->operation = USTACK_ENABLE_SERVICE;
        profile_mgr_gatt_ustack_enable_svc(offload, ctx, NULL);
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_unoffload_to_ustack
===========================================================================*/
/**
 * @brief Unoffloads GATT service from Ustack.
 * @details
 *   - Allocates callback context.
 *   - Sends disable service request or enables service excluding current session.
 * @param data Pointer to offload instance.
 * @param unused_data Unused.
 */
void profile_mgr_gatt_unoffload_to_ustack(void *data, void *unused_data)
{
    offload_instance_t *offload = data;
    gatt_svc_t *svc = offload->state_ctx->data;
    gatt_session_t *session = offload->offload_data.gatt_session;
    uint8_t num_sessions = 0;

    num_sessions = fetch_num_sessions_in_svc(svc);

    ustack_cb_ctx_t *ctx = bt_pal_malloc(sizeof(ustack_cb_ctx_t));
    if (!ctx)
    {
        BT_PAL_ASSERT_FATAL_NO_ARGS("");
        return;
    }

    ctx->data = data;
    ctx->operation = USTACK_DISABLE_SERVICE;

    if (num_sessions == 1) /* means last session in the svc */
    {
        GattOffloadDisableServiceReqSend(fw_gatt_id, (CsrUint32)ctx, svc->btConnId, 
            1, &svc->offloadId);
    }
    else
    {
        profile_mgr_gatt_ustack_enable_svc(offload, ctx, session);
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_offload_completed
===========================================================================*/
/**
 * @brief Callback for offload completion.
 * @details
 *   - Placeholder for offload completion handling.
 * @param data Pointer to offload instance.
 * @param status Status code.
 */
void profile_mgr_gatt_offload_completed(void *data, int status)
{

}

/*===========================================================================
FUNCTION      profile_mgr_gatt_uapp_reg_unreg_resp
===========================================================================*/
/**
 * @brief Sends UAPP register/unregister response.
 * @details
 *   - Prepares and sends response message to endpoint manager.
 * @param endpointId Endpoint ID.
 * @param status Registration result status.
 * @param opcode Operation code.
 */
void profile_mgr_gatt_uapp_reg_unreg_resp(uint64_t endpointId, gatt_uapp_status_t status, uint16_t opcode)
{
    endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();

    /* header */
    header->opcode = opcode;
    header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(uapp_gatt_register_rsp_t);
    header->endpoint_id = endpointId;

    /* msg */
    uapp_gatt_register_rsp_t *rsp = (uapp_gatt_register_rsp_t *)header->data;
    rsp->status = status;

    endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("Sent UAPP_REG_RESP");
#endif
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_send_client_read_rsp_to_uapp
===========================================================================*/
/**
 * @brief Helper function frame gatt Client read response message to uapp.
 * @param header endpt_mgr_rpc_header_t.
 * @param session gatt_session_t.
 * @param handle handle.
 * @param result gatt_uapp_status_t.
 * @param val_len val_len.
 * @param val pointer to attribute val.
 * @param context uapp_context.
 */
static void profile_mgr_gatt_send_client_read_rsp_to_uapp(endpt_mgr_rpc_header_t *header,
    gatt_session_t *session, uint16_t handle, gatt_uapp_status_t result, uint16_t val_len, const uint8_t *val, uint32_t context)
{
    if (!session || !header)
    {
        return;
    }

    header->opcode      = UAPP_GATT_CLIENT_READ_CHAR_RESP;
    header->data_len    = ENDPT_MGR_RPC_HDR_SIZE +
                          sizeof(uapp_gatt_char_read_rsp_t) + val_len;

    uapp_gatt_char_read_rsp_t *rsp = (uapp_gatt_char_read_rsp_t *)header->data;
    rsp->session_id = session->session_id;
    rsp->handle     = handle;
    rsp->result     = result;
    rsp->val_len    = val_len;
    rsp->context    = context;

    if (result == statusOk && val_len && val)
    {
        memcpy(&rsp->val[0], val, val_len);
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_send_client_write_rsp_to_uapp
===========================================================================*/
/**
 * @brief Helper function frame gatt Client write response message to uapp.
 * @param header endpt_mgr_rpc_header_t.
 * @param session gatt_session_t.
 * @param handle handle.
 * @param result gatt_uapp_status_t.
 * @param context uapp_context.
 */
static void profile_mgr_gatt_send_client_write_rsp_to_uapp(endpt_mgr_rpc_header_t *header,
                gatt_session_t *session, uint16_t handle, gatt_uapp_status_t result, uint32_t context, bool write_cmd)
{
    if (!session || !header)
    {
        return;
    }

    header->opcode      = UAPP_GATT_CLIENT_WRITE_CHAR_RESP;
    header->data_len    = ENDPT_MGR_RPC_HDR_SIZE +
                          sizeof(uapp_gatt_char_write_rsp_t);

    uapp_gatt_char_write_rsp_t *rsp = (uapp_gatt_char_write_rsp_t *)header->data;
    rsp->session_id = session->session_id;
    rsp->handle     = handle;
    rsp->result     = result;
    rsp->context    = context;
    rsp->writeWithoutRsp =  write_cmd;
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_send_server_notif_rsp_to_uapp
===========================================================================*/
/**
 * @brief Helper function frame gatt Server notification response message to uapp.
 * @param header endpt_mgr_rpc_header_t.
 * @param session gatt_session_t.
 * @param handle handle.
 * @param result gatt_uapp_status_t.
 * @param context uapp_context.
 */

static void profile_mgr_gatt_send_server_notif_rsp_to_uapp(endpt_mgr_rpc_header_t *header,
                gatt_session_t *session, uint16_t handle, gatt_uapp_status_t result, uint32_t context)
{
    if (!header || !session)
    {
        return;
    }

    header->opcode      = UAPP_GATT_SERVER_NOTIFICATION_RESP;
    header->data_len    = ENDPT_MGR_RPC_HDR_SIZE +
                          sizeof(uapp_gatt_char_notif_ind_rsp_t);

    uapp_gatt_char_notif_ind_rsp_t *rsp =
        (uapp_gatt_char_notif_ind_rsp_t *)header->data;
    rsp->session_id = session->session_id;
    rsp->handle     = handle;
    rsp->result     = result;
    rsp->context    = context;
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_reset_pending_op
===========================================================================*/
/**
 * @brief Helper function to reset pending operation details.
 * @param session gatt_session_t.
 */
static void profile_mgr_gatt_reset_pending_op(gatt_session_t *session)
{
    if(session)
    {
        session->tx_pending_handle = GATT_OFFLOAD_INVALID_CHAR_HANDLE;
        session->tx_pending_op = GATT_OFFLOAD_PENDING_OP_NONE;
        session->uapp_ctx = GATT_OFFLOAD_INVALID_CONTEXT;
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_send_error_rsp_to_uapp
===========================================================================*/
/**
 * @brief Helper function frame error response message to uapp.
 * @param opcode opcode.
 * @param gatt_ep endpoint for GATT.
 * @param session gatt_session_t.
 * @param handle handle.
 * @param result gatt_uapp_status_t.
 * @param context uapp_context.
 */
static void profile_mgr_gatt_send_error_rsp_to_uapp(uint16_t opcode, gatt_endpoint_t *gatt_ep, 
                gatt_session_t *session, uint16_t handle, gatt_uapp_status_t error, uint32_t context)
{
    endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
    if (!gatt_ep || !session || !header)
    {
        return;
    }

    header->endpoint_id = gatt_ep->ep_id;

    switch (opcode)
    {
        case UAPP_GATT_CLIENT_READ_CHAR_REQ:
        {
            profile_mgr_gatt_send_client_read_rsp_to_uapp(header, session,
                handle, error, 0, NULL, context);
        }
        break;

        case UAPP_GATT_CLIENT_WRITE_CHAR_CMD:
        {
            profile_mgr_gatt_send_client_write_rsp_to_uapp(header, session,
                handle, error, context, TRUE);
        }
        break;
        case UAPP_GATT_CLIENT_WRITE_CHAR_REQ:
        {
                profile_mgr_gatt_send_client_write_rsp_to_uapp(header, session,
                    handle, error, context, FALSE);
        }
        break;
        case UAPP_GATT_SERVER_NOTIFICATION_REQ:
        {
            profile_mgr_gatt_send_server_notif_rsp_to_uapp(header, session,
                handle, error, context);
            break;
        }

        default:
            /* SERVER_*_IND_RESP are already responses, nothing more to send. */
            break;
    }

    /* Send only if some helper actually filled a payload */
    if (header->data_len > 0)
    {
        endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm,
                                       header,
                                       header->data_len);
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_offload_to_uapp
===========================================================================*/
/**
 * @brief Offloads GATT service to MicroApp (UAPP).
 * @details
 *   - Encodes and sends register service request to MicroApp.
 * @param data Pointer to offload instance.
 */
void profile_mgr_gatt_offload_to_uapp(void *data)
{
    offload_instance_t *offload = data;
    /* send gatt service request to microapp */
    /* header */
    endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
    
    /* msg */
    bool encode = encode_uapp_gatt_register_service_req(offload, header);
    if (encode)
    {
        endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
#ifdef DEBUG_BUILD_LOGS
        OFFLOAD_MGR_LOGL("Sent UAPP_SESSION_REG_REQ");
#endif
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_unoffload_to_uapp
===========================================================================*/
/**
 * @brief Unoffloads GATT session with MicroApp (UAPP).
 * @details
 *   - Prepares and sends session unregistered event to MicroApp.
 * @param data Pointer to offload instance.
 */
void profile_mgr_gatt_unoffload_to_uapp(void *data)
{
    offload_instance_t *offload = data;
    gatt_session_t *session = offload->offload_data.gatt_session;
    if (!session)
    {
        OFFLOAD_MGR_LOGE("gatt session in NULL");
        return;
    }

    endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();

    /* header */
    header->opcode = UAPP_GATT_SESSION_UNREGISTERED_REQ;
    header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(uapp_gatt_session_unregistered_t);
    header->endpoint_id = offload->ep_id.id;

    /* msg */
    uapp_gatt_session_unregistered_t *req = (uapp_gatt_session_unregistered_t *)header->data;
    req->session_id = session->session_id;

    endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("Sent UAPP_SESSION_UNREG");
#endif

    /* Mock sending pending response if required */
    if (session->tx_pending_op != GATT_OFFLOAD_PENDING_OP_NONE)
    {
        gatt_endpoint_t *gatt_ep = find_gatt_endpoint(ENDPT_MATCH_EP_ID, offload->ep_id.id, 0);
        if (gatt_ep)
        {
            profile_mgr_gatt_send_error_rsp_to_uapp (session->tx_pending_op, gatt_ep, 
                session, session->tx_pending_handle, statusSessionClosed, session->uapp_ctx);
            profile_mgr_gatt_reset_pending_op(session);
        }
    }
}

/*===========================================================================
FUNCTION      send_ustack_events_to_uapp
===========================================================================*/
/**
 * @brief Sends Ustack events to MicroApp (UAPP).
 * @details
 *   - Handles various Ustack event types and sends corresponding messages to MicroApp.
 * @param message Pointer to event message.
 */
void send_ustack_events_to_uapp (void *message)
{
    ustack_evt_hdr *hdr = (ustack_evt_hdr *)message;
#ifdef DEBUG_BUILD_LOGS
    OFFLOAD_MGR_LOGL("Evt %d gattId %d", hdr->type, hdr->gattId);
#endif

    gatt_endpoint_t *gatt_ep = find_gatt_endpoint(ENDPT_MATCH_GATT_ID, 0, hdr->gattId);
    if (!gatt_ep) 
    {
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGM("gatt_ep is NULL");
#endif
        return;
    }

    endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
    header->endpoint_id = gatt_ep->ep_id;

    switch(hdr->type)
    {
        case CSR_BT_GATT_READ_CFM:
        {
            CsrBtGattReadCfm *cfm = (CsrBtGattReadCfm *)message;
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("CSR_BT_GATT_READ_CFM btConnId %d rc %x rs %x", \
                  cfm->btConnId, cfm->resultCode, cfm->resultSupplier );
            OFFLOAD_MGR_LOGL("handle %d val_len %d", cfm->handle, cfm->valueLength);
#endif
            gatt_session_t* session = fetch_gatt_session_in_endpoint(gatt_ep,cfm->btConnId, cfm->handle);
            if (session)
            {
                profile_mgr_gatt_send_client_read_rsp_to_uapp(header, session, cfm->handle, 
                    map_ustack_error_to_uapp_error(cfm->resultSupplier, cfm->resultCode), 
                    cfm->valueLength, cfm->value, session->uapp_ctx);
                profile_mgr_gatt_reset_pending_op(session);
            }
        }
        break;
        case CSR_BT_GATT_WRITE_CFM:
        {
            CsrBtGattWriteCfm *cfm = (CsrBtGattWriteCfm *)message;
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("CSR_BT_GATT_WRITE_CFM btConnId %d rc %x rs %x", \
                cfm->btConnId, cfm->resultCode, cfm->resultSupplier );
            OFFLOAD_MGR_LOGL("handle %d ", cfm->handle);
#endif
            gatt_session_t* session = fetch_gatt_session_in_endpoint(gatt_ep,cfm->btConnId, cfm->handle);
            if (session)
            {
                bool write_cmd = (session->tx_pending_op == UAPP_GATT_CLIENT_WRITE_CHAR_CMD) ? TRUE : FALSE;
                profile_mgr_gatt_send_client_write_rsp_to_uapp(header, session, cfm->handle, 
                    map_ustack_error_to_uapp_error(cfm->resultSupplier, cfm->resultCode), 
                    session->uapp_ctx, write_cmd);
                profile_mgr_gatt_reset_pending_op(session);
            }
        }
        break;
        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            CsrBtGattClientNotificationInd *ind = (CsrBtGattClientNotificationInd *)message;
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("CSR_BT_GATT_CLIENT_NOTIFICATION_IND btConnId %d handle %d val_len %d", \
                ind->btConnId, ind->valueHandle, ind->valueLength);
#endif
            gatt_session_t* session = fetch_gatt_session_in_endpoint(gatt_ep, ind->btConnId, ind->valueHandle);
            if (session)
            {
                header->opcode = UAPP_GATT_CLIENT_NOTIFICATION_IND;
                header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(uapp_gatt_char_notif_ind_t) + ind->valueLength;
            
                uapp_gatt_char_notif_ind_t *rsp = (uapp_gatt_char_notif_ind_t *)header->data;
                rsp->session_id = session->session_id;
                rsp->handle = ind->valueHandle;
                rsp->val_len = ind->valueLength;
                memcpy(&rsp->val[0], ind->value, ind->valueLength);
            }
        }
        break;
        case CSR_BT_GATT_DB_ACCESS_READ_IND:
        {
            CsrBtGattDbAccessReadInd *ind = (CsrBtGattDbAccessReadInd *)message;
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("CSR_BT_GATT_DB_ACCESS_READ_IND btConnId %d attrHandle %d offset %d", \
                ind->btConnId, ind->attrHandle, ind->offset);
#endif
            gatt_session_t* session = fetch_gatt_session_in_endpoint(gatt_ep, ind->btConnId, ind->attrHandle);
            if (session)
            {
                header->opcode = UAPP_GATT_SERVER_READ_CHAR_IND;
                header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(uapp_gatt_char_read_req_t);
            
                /* msg */
                uapp_gatt_char_read_req_t *req = (uapp_gatt_char_read_req_t *)header->data;
                req->session_id = session->session_id;
                req->handle = ind->attrHandle;
                req->mtu = 0;
                offload_instance_t *offload = offload_instance_find_by_gatt_session_id(session->session_id);
                if (offload)
                {
                    gatt_svc_t *svc = offload->state_ctx->data;
                    if (svc)
                        req->mtu = svc->mtu;
                }
            }
        }
        break;
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            CsrBtGattDbAccessWriteInd *ind = (CsrBtGattDbAccessWriteInd *)message;
#ifdef DEBUG_BUILD_LOGS
                OFFLOAD_MGR_LOGL("CSR_BT_GATT_DB_ACCESS_WRITE_IND btConnId %d attrHandle %d ", \
                    ind->btConnId, ind->attrHandle);
                if (ind->writeUnitCount > 0  && ind->writeUnit)
                {
                    OFFLOAD_MGR_LOGL("offset %d val_len %d", ind->writeUnit[0].offset, ind->writeUnit[0].valueLength);
                }
#endif
            gatt_session_t* session = fetch_gatt_session_in_endpoint(gatt_ep, ind->btConnId, ind->attrHandle);
            if (session)
            {
                header->opcode = UAPP_GATT_SERVER_WRITE_CHAR_IND;
                uapp_gatt_char_write_req_t *rsp = (uapp_gatt_char_write_req_t *)header->data;
                rsp->session_id = session->session_id;
                rsp->handle = ind->attrHandle;
                rsp->writeWithoutRsp = ((ind->check & CSR_BT_GATT_ACCESS_WRITE_CMD)? TRUE : FALSE);
                rsp->val_len = 0;
                if (ind->writeUnitCount > 0 && ind->writeUnit)
                {
                    rsp->val_len = ind->writeUnit[0].valueLength;
                    memcpy(&rsp->val[0], ind->writeUnit[0].value , ind->writeUnit[0].valueLength);
                }
                header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(uapp_gatt_char_write_req_t) + rsp->val_len;
            }
        }
        break;
        case CSR_BT_GATT_EVENT_SEND_CFM:
        {
            CsrBtGattEventSendCfm *cfm = (CsrBtGattEventSendCfm *)message;
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("CSR_BT_GATT_EVENT_SEND_CFM gattId %d btConnId %d rs %x rc %x Handle %d", cfm->gattId, cfm->btConnId, cfm->resultSupplier, cfm->resultCode, cfm->attrHandle);
#endif
            gatt_session_t* session = fetch_gatt_session_in_endpoint(gatt_ep, cfm->btConnId, cfm->attrHandle);
            if (session)
            {
                profile_mgr_gatt_send_server_notif_rsp_to_uapp(header, session, cfm->attrHandle, 
                    map_ustack_error_to_uapp_error(cfm->resultSupplier, cfm->resultCode), session->uapp_ctx);
                profile_mgr_gatt_reset_pending_op(session);
            }
        }
        break;
        default:
            return;
    }
    endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
}

/*===========================================================================
FUNCTION      microstack_gatt_cb
===========================================================================*/
/**
 * @brief Microstack callback for GATT events.
 * @details
 *   - Handles various GATT events from Microstack and dispatches to appropriate handlers.
 * @param handle Application handle.
 * @param eventClass Event class.
 * @param message Pointer to event message.
 */
static void microstack_gatt_cb(BtAppHandle handle, BtEventClass eventClass, void *message)
{
    CsrPrim type = *((CsrPrim *)message);
	OFFLOAD_MGR_LOGM("Ustack Evt %x", type);
    switch (type)
    {
        case CSR_BT_GATT_REGISTER_CFM:
        {
            CsrBtGattRegisterCfm *cfm = (CsrBtGattRegisterCfm *)message;
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("gattId:%d, rc %d rs %d", cfm->gattId, cfm->resultCode, cfm->resultSupplier);
#endif
            if (cfm->context == GATT_OFFLOAD_FRAMEWORK_REG_CB_CTX)
            {
                fw_gatt_id = cfm->gattId;
                return;
            }

            if (cfm->context < GATT_OFFLOAD_MAX_ENDPOINT)
            {
                gatt_endpoint_t *uapp = &gatt_uapp[cfm->context];
                uapp->gattId = cfm->gattId;

                /* send response to micro app */
                profile_mgr_gatt_uapp_reg_unreg_resp(uapp->ep_id, statusOk, UAPP_GATT_APP_REG_RESP);
            }
#ifdef DEBUG_BUILD_LOGS
            else
                OFFLOAD_MGR_LOGL("Invalid index for gatt_uapp");
#endif
        }
        break;
        case GATT_OFFLOAD_CONN_INFO_CFM:
        {
            GattOffloadConnInfoCfm *cfm = (GattOffloadConnInfoCfm *)message;
            ustack_cb_ctx_t *ctx = (ustack_cb_ctx_t *)cfm->context;
            if (!ctx || !ctx->data)
            {
                BT_PAL_ASSERT_FATAL_NO_ARGS("");
                return;
            }

            offload_instance_t *offload = ctx->data;

#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("gattId:%d, rc %d rs %d, btConnId %d", \
                        cfm->gattId, cfm->resultCode, cfm->resultSupplier, cfm->btConnId);
#endif
            if (cfm->resultCode == GATT_OFFLOAD_RESULT_SUCCESS)
            {
                gatt_svc_t *svc = offload->state_ctx->data;
                svc->btConnId = cfm->btConnId;
                ctx->operation = USTACK_ENABLE_SERVICE;
                profile_mgr_gatt_ustack_enable_svc(offload, ctx, NULL);
            }
            else
            {
                offload_mgr_profile_offload_cfm_cb(offload->offload_instance_id, GATT_OFFLOAD_STATUS_INTERNAL_ERROR);
                bt_pal_free(ctx);
            }
        }
        break;
        case GATT_OFFLOAD_ENABLE_SERVICE_CFM:
        {
            GattOffloadEnableServiceCfm *cfm = (GattOffloadEnableServiceCfm *)message;
            ustack_cb_ctx_t *ctx = (ustack_cb_ctx_t *)cfm->context;
            if (!ctx || !ctx->data)
            {
                BT_PAL_ASSERT_FATAL_NO_ARGS("");
                return;
            }
            offload_instance_t *offload = ctx->data;

#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("gattId:%d, rc %d rs %d, offloadId %d", \
                        cfm->gattId, cfm->resultCode, cfm->resultSupplier, cfm->offloadId);
#endif
            int status = (cfm->resultCode == GATT_OFFLOAD_RESULT_SUCCESS)?\
                GATT_OFFLOAD_STATUS_SUCCESS:GATT_OFFLOAD_STATUS_INTERNAL_ERROR;

            if (ctx->operation == USTACK_ENABLE_SERVICE)
            {
                gatt_svc_t *svc = offload->state_ctx->data;
                svc->offloadId = cfm->offloadId;
                offload_mgr_profile_offload_cfm_cb(offload->offload_instance_id, status);
            }
            else
            {
                offload_mgr_profile_unoffload_cfm_cb(offload->offload_instance_id, status);
            }
            bt_pal_free(ctx);
        }
        break;
        case GATT_OFFLOAD_DISABLE_SERVICE_CFM:
        {
            GattOffloadDisableServiceCfm *cfm = (GattOffloadDisableServiceCfm *)message;
            ustack_cb_ctx_t *ctx = (ustack_cb_ctx_t *)cfm->context;
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("gattId:%d, rc %d rs %d, btConnId %d", \
                cfm->gattId, cfm->resultCode, cfm->resultSupplier, cfm->btConnId);
#endif
            if (!ctx || !ctx->data)
            {
                BT_PAL_ASSERT_FATAL_NO_ARGS("");
                return;
            }

            if (ctx->operation == USTACK_DISABLE_SERVICE) /* triggered as part of unregister service for single session */
            {
                offload_instance_t *offload = (offload_instance_t *)ctx->data;
                offload_mgr_profile_unoffload_cfm_cb(offload->offload_instance_id, 0);
            }
            else /* triggered as part of clear service sequence for set of sessions */
            {
                int32_t acl_handle = (int32_t)ctx->data;

                /* All sessions for this acl_handle is already moved to Idle state.
                 * So send response to HAL for completion of clear_service sequence */
                send_bt_gatt_hal_clear_services_rsp(acl_handle);
            }
            bt_pal_free(ctx);
        }
        break;
        case GATT_OFFLOAD_ERROR_IND:
        {
            GattOffloadErrorInd *ind = (GattOffloadErrorInd *)message;
            int32_t acl_handle = CSR_BT_CONN_ID_INVALID;
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("btConnId %d cid %d errCode %d", ind->btConnId, ind->cid, ind->errCode);
#endif
            if (find_acl_handle_by_btconn_id(ind->btConnId, &acl_handle) == TRUE)
            {
                gatt_error_report error_report = {
                    .aclConnectionHandle = acl_handle,
                    .localCid = ind->cid,
                    .error = ind->errCode,
                };
                proto_encode_and_frame(gatt_error_report_fields, &error_report, BT_OFFLOAD_GATT_ERROR_REPORT, NULL);
                Offload_Mgr_SendDataToBTGattHal(offload_mgr_transport_shim_buf,
                                                  offload_mgr_transport_shim_encoded_buf_len);
#ifdef DEBUG_BUILD_LOGS
                OFFLOAD_MGR_LOGL("ERR sent to HAL aclHandle %d error %d", acl_handle, ind->errCode);
#endif
            }
        }
        break;
        default:
        {
            send_ustack_events_to_uapp(message);
        }
        break;
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_uapp_register
===========================================================================*/
/**
 * @brief Registers a MicroApp (UAPP) to handle GATT offload.
 * @details
 *   - Allocates endpoint slot and sends registration request.
 *   - Sends response if resources are insufficient.
 * @param endpoint_id Endpoint ID.
 */
void profile_mgr_gatt_uapp_register(uint64_t endpoint_id)
{
    uint8_t i, j;
    for (i=0; i<GATT_OFFLOAD_MAX_ENDPOINT; i++)
    {
        if (gatt_uapp[i].ep_id == GATT_OFFLOAD_INVALID_ENDPOINT_ID)
        {
            gatt_uapp[i].ep_id = endpoint_id;
            CsrBtGattRegisterReqSend(PM_GATT_APP_HANDLE, i);
            return;
        }
    }

    /* send response since GATT_MAX_UAPPS has reached */
    profile_mgr_gatt_uapp_reg_unreg_resp(endpoint_id, statusResourceExhausted , UAPP_GATT_APP_REG_RESP);
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_uapp_unregister
===========================================================================*/
/**
 * @brief Unregisters a MicroApp (UAPP).
 * @details
 *   - Finds and unregisters endpoint, clears data.
 *   - Sends response to MicroApp.
 * @param endpoint_id Endpoint ID.
 */
void profile_mgr_gatt_uapp_unregister(uint64_t endpoint_id)
{
    for (uint8_t i=0; i<GATT_OFFLOAD_MAX_ENDPOINT; i++)
    {
        if (gatt_uapp[i].ep_id == endpoint_id)
        {
            CsrBtGattUnregisterReqSend(gatt_uapp[i].gattId);
            memset (&gatt_uapp[i], 0, sizeof(gatt_endpoint_t));

            /* send response to uapp */
            profile_mgr_gatt_uapp_reg_unreg_resp(endpoint_id, statusOk, UAPP_GATT_APP_UNREG_RESP);
            return;
        }
    }
    /* send response since endpoint has not registered before */
    profile_mgr_gatt_uapp_reg_unreg_resp(endpoint_id, statusInvalidParameter, UAPP_GATT_APP_UNREG_RESP);
}

static void profile_mgr_gatt_update_pending_op(gatt_session_t *session, uint16_t opcode, uint16_t handle, uint32_t context)
{
    switch(opcode)
    {
        case UAPP_GATT_CLIENT_READ_CHAR_REQ:
        case UAPP_GATT_CLIENT_WRITE_CHAR_REQ:
        case UAPP_GATT_CLIENT_WRITE_CHAR_CMD:
        case UAPP_GATT_SERVER_NOTIFICATION_REQ:
        {
            if (session)
            {
                session->tx_pending_op = opcode;
                session->tx_pending_handle = handle;
                session->uapp_ctx = context;
            }
        }
        break;
        default:
            break;
    }
}

/*===========================================================================
FUNCTION      profile_mgr_gatt_uapp_char_evt
===========================================================================*/
/**
 * @brief Handles characteristic events from MicroApp (UAPP).
 * @details
 *   - Decodes event and dispatches read/write/notification requests to Micro stack.
 * @param endpointId Endpoint ID.
 * @param opcode Operation code.
 * @param evt Pointer to event data.
 * @param len Length of event data.
 */
void profile_mgr_gatt_uapp_char_evt(uint64_t endpointId, uint16_t opcode, void *evt, uint16_t len)
{
    int32_t session_id = GATT_UAPP_EVT_GET_SESSION_ID(evt);
    uint16_t handle = GATT_UAPP_EVT_GET_HANDLE(evt);
    uint32_t context = GATT_UAPP_EVT_GET_UAPP_CTX(evt);
#ifdef DEBUG_BUILD_LOGS
        OFFLOAD_MGR_LOGL("Uapp_Op %d session_id %d handle %d context %x", opcode, session_id, handle, context);
#endif

    offload_instance_t *offload = offload_instance_find_by_gatt_session_id(session_id);
    if (!offload)
    {
        OFFLOAD_MGR_LOGH("Invalid sessionId of epId");
        //Cannot send response back to AM. So ignore at this point of time.
        return;
    }
    gatt_endpoint_t * gatt_ep = find_gatt_endpoint(ENDPT_MATCH_EP_ID, offload->ep_id.id, 0);
    gatt_svc_t *gatt_svc = offload->state_ctx->data;
    gatt_session_t *session = offload->offload_data.gatt_session;

    if (!gatt_ep || !gatt_svc || !session)
    {
        OFFLOAD_MGR_LOGH("Invalid sessionId of epId or session or gatt_svc");
        //Cannot send response back to AM. So ignore at this point of time.
        return;
    }

    if (!gatt_handle_exists_in_session(session, handle))
    {
        OFFLOAD_MGR_LOGH("Handle %d does not belong to session %d", handle, session_id);
        // Send Response back to Uapp as Invalid Parameter 
        profile_mgr_gatt_send_error_rsp_to_uapp(opcode, gatt_ep, session, 
            handle, statusInvalidParameter, context);
        return;
    }

    if (session->tx_pending_op != GATT_OFFLOAD_PENDING_OP_NONE && 
        (opcode == UAPP_GATT_CLIENT_READ_CHAR_REQ || 
        opcode == UAPP_GATT_CLIENT_WRITE_CHAR_REQ ||
        opcode == UAPP_GATT_CLIENT_WRITE_CHAR_CMD ||
        opcode == UAPP_GATT_SERVER_NOTIFICATION_REQ))
    {
        OFFLOAD_MGR_LOGH("Pending operation exists with opcode %d handle %d in the session",
            session->tx_pending_op, session->tx_pending_handle);
        // Send Response back to Uapp as Invalid Operation
        profile_mgr_gatt_send_error_rsp_to_uapp(opcode, gatt_ep, session, 
            handle, statusInvalidOperation, context);
        return;
    }

    profile_mgr_gatt_update_pending_op(session, opcode, handle, context);
    
    /* Send messages to Micro Stack */
    switch(opcode)
    {
        case UAPP_GATT_CLIENT_READ_CHAR_REQ:
        {
            CsrBtGattReadReqSend(gatt_ep->gattId, gatt_svc->btConnId, handle, 0);
        }
        break;
        case UAPP_GATT_CLIENT_WRITE_CHAR_REQ:
        case UAPP_GATT_CLIENT_WRITE_CHAR_CMD:
        {
            uapp_gatt_char_write_req_t *req = (uapp_gatt_char_write_req_t *)evt;
#ifdef DEBUG_BUILD_LOGS
            OFFLOAD_MGR_LOGL("CsrBtGattWriteReqSend attr_handle %d cmd %d val_len %d", \
                req->handle, req->writeWithoutRsp, req->val_len);
#endif
            uint8_t *val = NULL;
            if (req->val_len > 0)
            {
                if (req->val_len > (gatt_svc->mtu-3))
                {
                    req->val_len = gatt_svc->mtu-3;
                }
                val = bt_pal_malloc(req->val_len);
                if (!val)
                {
                    BT_PAL_ASSERT_FATAL_NO_ARGS("");
                    return;
                }
                memcpy(val, &req->val[0], req->val_len);
            }
            if (req->writeWithoutRsp == FALSE)
            {
                CsrBtGattWriteReqSend(gatt_ep->gattId, gatt_svc->btConnId, req->handle, 0, req->val_len, val);
            }
            else
            {
                CsrBtGattWriteCmdReqSend(gatt_ep->gattId, gatt_svc->btConnId, req->handle, req->val_len, val);
            }
        }
        break;
        case UAPP_GATT_SERVER_READ_CHAR_IND_RESP:
        {
            uapp_gatt_char_read_rsp_t *rsp = (uapp_gatt_char_read_rsp_t *)evt;
#ifdef DEBUG_BUILD_LOGS            
            OFFLOAD_MGR_LOGL("CsrBtGattDbReadAccessResSend attr_handle %d error_code %d val_len %d", \
                rsp->handle, rsp->result, rsp->val_len);
#endif
            uint8_t *val = NULL;
            if (rsp->val_len > 0)
            {
                if (rsp->val_len > (gatt_svc->mtu-1))
                {
                    rsp->val_len = gatt_svc->mtu-1;
                }
                val = bt_pal_malloc(rsp->val_len);
                if (!val)
                {
                    BT_PAL_ASSERT_FATAL_NO_ARGS("");
                    return;
                }
                memcpy(val, &rsp->val[0], rsp->val_len);
            }
            CsrBtGattDbReadAccessResSend(gatt_ep->gattId, gatt_svc->btConnId,
                rsp->handle, rsp->result, rsp->val_len, val);
        }
        break;
        case UAPP_GATT_SERVER_WRITE_CHAR_IND_RESP:
        {
            uapp_gatt_char_write_rsp_t *rsp = (uapp_gatt_char_write_rsp_t *)evt;
#ifdef DEBUG_BUILD_LOGS            
            OFFLOAD_MGR_LOGL("CsrBtGattDbWriteAccessResSend attr_handle %d error_code %d", \
                rsp->handle, rsp->result);
#endif
            CsrBtGattDbWriteAccessResSend(gatt_ep->gattId, gatt_svc->btConnId, rsp->handle, rsp->result);
        }
        break;
        case UAPP_GATT_SERVER_NOTIFICATION_REQ:
        {
            uapp_gatt_char_notif_ind_t *ind = (uapp_gatt_char_notif_ind_t *)evt;
#ifdef DEBUG_BUILD_LOGS            
            OFFLOAD_MGR_LOGL("CsrBtGattNotificationEventReqSend attr_handle %d val_len %d", \
                ind->handle, ind->val_len);
#endif
            uint8_t *val = NULL;
            if (ind->val_len > 0)
            {
                if (ind->val_len > (gatt_svc->mtu-3))
                {
                    ind->val_len = gatt_svc->mtu-3;
                }
                val = bt_pal_malloc(ind->val_len);
                if (!val)
                {
                    BT_PAL_ASSERT_FATAL_NO_ARGS("");
                    return;
                }
                memcpy(val, &ind->val[0], ind->val_len);
            }
            CsrBtGattNotificationEventReqSend(gatt_ep->gattId, gatt_svc->btConnId, 
                ind->handle, ind->val_len, val);
        }
        break;
    }
    
}

/*===========================================================================
FUNCTION      gatt_session_mgr_init
===========================================================================*/
/**
 * @brief Initializes the GATT session manager.
 * @details
 *   - Registers client functions for offload manager.
 *   - Initializes GATT profile manager.
 */
void gatt_session_mgr_init(void) 
{
    offload_mgr_client_fn_t fns = {
        .clear_offload_data = profile_mgr_gatt_clear_offload_data,
        .store_offload_data = NULL,
        .offload_req_to_profile = profile_mgr_gatt_offload_to_ustack,
        .unoffload_req_to_profile = profile_mgr_gatt_unoffload_to_ustack,
        .offload_req_to_microapp = profile_mgr_gatt_offload_to_uapp,
        .unoffload_req_to_microapp = profile_mgr_gatt_unoffload_to_uapp,
        .send_offload_resp_to_hal = profile_mgr_gatt_hal_offload_rsp,
        .send_unoffload_resp_to_hal = profile_mgr_gatt_hal_unoffload_rsp,
        .offload_to_microapp_success_cb = profile_mgr_gatt_offload_completed,
        .unoffload_from_microapp_success_cb = NULL,
    };

    offload_mgr_client_register(OFFLOAD_INSTANCE_TYPE_GATT, &fns);

    profile_mgr_gatt_init();
}

/*===========================================================================
FUNCTION      gatt_session_mgr_deinit
===========================================================================*/
/**
 * @brief Deinitializes the GATT session manager.
 * @details
 *   - Deinitializes GATT profile manager.
 *   - Deregisters client functions from offload manager.
 */
void gatt_session_mgr_deinit(void) 
{
    profile_mgr_gatt_deinit();
    offload_mgr_client_deregister(OFFLOAD_INSTANCE_TYPE_GATT);
}

