/*************************************************************************
 * @file     lpai_bt_gatt_server_app.c
 * @brief    LPAI BT GATT Server App source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/** Program Specific Header file Inclusions */
#include <zephyr/sys/printk.h>
#include "lpai_bt_gatt_server_app.h"
#include "bt_utilities.h"

/**
 * @brief Database for BT GATT server sessions.
 *
 * An array of session information blocks, each representing a GATT server session.
 * Stores characteristic handles, value buffers, and session state for up to
 * GATT_SERVER_APP_MAX_SESSIONS simultaneous connections.
 */
server_app_gatt_session_t server_uapp_gatt_db[GATT_SERVER_APP_MAX_SESSIONS];

#ifdef SERVER_APP_BULK_TRANSFER
/**
 * @struct server_app_bulk_transfer
 * @brief Structure for GATT micro-app bulk transfer information.
 */
typedef struct server_app_bulk_transfer {
    int32_t sessionId; /**< session ID */
    uint16_t attHandle; /*< Attribute Handle */
    uint16_t pktCnt;  /**< Packet counter */
    uint16_t valLen; /*< Attribute value len */
    bool enable; /*< Flag for enabling bulk transfer */
} server_app_bulk_transfer_t;

/**
 * @brief Variable used for TX bulk data trasfer.
 */
static server_app_bulk_transfer_t srv_tx_bulk_transfer = {0};

/**
 * @brief Variable used for RX bulk data trasfer.
 */
static server_app_bulk_transfer_t srv_rx_bulk_transfer = {0};

#endif /*SERVER_APP_BULK_TRANSFER */


/**
 * @brief DeInitializes the GATT Server Microapp Structures.
 *
 * This function resets the necessary configurations, data structures, and
 * resources required to start the Gatt Server Gatt Structures.
 *
 * @note This function does not take any parameters and does not return a value.
 */
void gatt_server_uapp_deinit()
{
    memset(&srv_tx_bulk_transfer,0,sizeof(server_app_bulk_transfer_t));
    memset(&srv_rx_bulk_transfer,0,sizeof(server_app_bulk_transfer_t));
    for(int i=0;i<GATT_SERVER_APP_MAX_SESSIONS;i++)
    {
        if(server_uapp_gatt_db[i].char_list != NULL)
        {
            free(server_uapp_gatt_db[i].char_list);
        }
        memset((server_uapp_gatt_db + i) ,0,sizeof(server_app_gatt_session_t));
    }

}

/**
 * @brief Handles events for the BT GATT server micro app.
 *
 * Acts as the main dispatcher for all incoming server events, including activation,
 * deactivation, service registration/unregistration, characteristic read/write, 
 * and notifications. Decodes the opcode and performs the required operations.
 *
 * @param[in] endpointId  Unique identifier for the endpoint.
 * @param[in] opcode      Opcode specifying the event type.
 * @param[in] data        Pointer to the event-specific data buffer.
 * @param[in] data_len    Size (in bytes) of the event data buffer.
 */
void server_uapp_evt_handler(uint64_t endpointId, uint16_t opcode, void *data, uint16_t data_len);

/**
 * @brief Initializes and registers the BT GATT server micro app.
 *
 * Sets up endpoint configuration, registers the server application with the GATT stack,
 * and resets the local session database for new operation.
 */
void gatt_server_app_init()
{
    endPointDetails_t endpoint = {0};

	/*Add End Point Details*/
	endpoint.endPointId.hubId = GATT_SERVER_APP_HUB_ID;
	endpoint.endPointId.epId = GATT_SERVER_APP_EP_ID;
	memscpy(endpoint.name,ENDPT_NAME_MAX_LEN, GATT_SERVER_APP_EP_NAME,
        sizeof(GATT_SERVER_APP_EP_NAME));
	endpoint.endPointService.majorVersion = 0x01;
	endpoint.endPointService.minorVersion = 0x01;
	memscpy(endpoint.endPointService.serviceDescriptor,ENDPT_SERVICE_DESC_MAX_LEN, 
        GATT_SERVER_APP_EP_SVC_NAME,sizeof(GATT_SERVER_APP_EP_SVC_NAME));
    qapi_gatt_app_register(endpoint, server_uapp_evt_handler);

    memset(server_uapp_gatt_db, 0, sizeof(server_uapp_gatt_db));
}

/**
 * @brief Adds a GATT session entry to the server database with associated characteristics.
 *
 * Allocates and initializes an entry in the local GATT session database, associates
 * the provided list of characteristics, and allocates value buffers of specified MTU.
 *
 * @param[in] session_id   Unique identifier for the session.
 * @param[in] char_list    Pointer to the array of GATT characteristics.
 * @param[in] num_chars    Number of characteristics in the list.
 * @param[in] mtu          Maximum transmission unit for value buffer allocation.
 */
static void gatt_db_add_handle(int32_t session_id, 
    const qapi_gatt_characteristic_t *char_list, uint16_t num_chars, uint16_t mtu)
{
    for(uint8_t i=0; i<GATT_SERVER_APP_MAX_SESSIONS; i++)
    {
        server_app_gatt_session_t *db = &server_uapp_gatt_db[i];
        if (db->sessionId == 0)
        {
            db->sessionId = session_id;
            db->num_chars = num_chars;
            db->char_list = malloc(sizeof(server_app_gatt_char_t) * num_chars);
            if (!db->char_list)
                return; // log/assert as needed
            for (uint8_t j=0; j<db->num_chars; j++)
            {
                db->char_list[j].handle = char_list[j].handle;
                db->char_list[j].val = malloc(mtu);
                db->char_list[j].val_len = 0;
                if (!db->char_list[j].val)
                {
                    // Cleanup previous allocations!
                    for(uint8_t k=0; k<j; k++) free(db->char_list[k].val);
                    free(db->char_list);
                    db->char_list = NULL;
                    return;
                }
            }
            break; // session created, exit
        }
    }
}

/**
 * @brief Deletes a GATT session and frees associated resources in the server database.
 *
 * Frees any buffers and characteristic structures allocated for the specified session,
 * removing its entry from the GATT server's local database.
 *
 * @param[in] session_id   Identifier of the session to be deleted.
 */
static void gatt_db_delete_handle(int32_t session_id)
{
    for(uint8_t i=0; i<GATT_SERVER_APP_MAX_SESSIONS; i++)
    {
        server_app_gatt_session_t *db = &server_uapp_gatt_db[i];
        if (db->sessionId == session_id)
        {
            for (uint8_t j=0; j<db->num_chars; j++)
            {
                if (db->char_list[j].val)
                {
                    free(db->char_list[j].val);
                    db->char_list[j].val = NULL;
                }
            }
            if (db->char_list)
            {
                free(db->char_list);
                db->char_list = NULL;
            }
            db->num_chars = 0;
            db->sessionId = 0;
            break;
        }
    }
}

/**
 * @brief Fetches or updates characteristic value buffers for a GATT session.
 *
 * If @p fetch is TRUE, retrieves the value buffer and its length for a given session and handle.
 * If @p fetch is FALSE, updates the buffer contents with the given value and length.
 *
 * @param[in]     sessionId  Session identifier.
 * @param[in]     handle     Characteristic handle.
 * @param[in,out] val_len    Pointer to value length (in/out).
 * @param[in,out] val        Pointer to value buffer (in/out).
 * @param[in]     fetch      Operation selector: TRUE for fetch (read), FALSE for update (write).
 */
static void gatt_db_fetch_update(int32_t sessionId, uint16_t handle, 
    uint16_t *val_len, uint8_t *val, bool fetch)
{
    for(uint8_t i=0; i<GATT_SERVER_APP_MAX_SESSIONS; i++)
    {
        server_app_gatt_session_t *db = &server_uapp_gatt_db[i];
        if (db->sessionId == sessionId)
        {
            for (uint8_t j=0; j<db->num_chars; j++)
            {
                if (db->char_list[j].handle == handle)
                {
                    if (*val_len <= QAPI_GATT_MTU_MAX && val)
                    {
                        if (fetch)
                        {
                            memscpy(val, QAPI_GATT_MTU_MAX, db->char_list[j].val, db->char_list[j].val_len);
                            *val_len = db->char_list[j].val_len;
                            printk("Fetching val_len %d\n", *val_len);
                        }
                        else
                        {
                            memscpy(db->char_list[j].val, QAPI_GATT_MTU_MAX, val, *val_len);
                            db->char_list[j].val_len = *val_len;
                            printk("Updating val_len %d\n", db->char_list[j].val_len);
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
}

/**
 * @brief Function to send GATT Server Characteristic Notfication to peer device.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attrHandle  GATT characteristic handle.
 * @param valLen     GATT characteristic value len.
 *
 * @return qapi_gatt_result_t
 */
qapi_gatt_result_t gatt_server_app_notif_send(int32_t sessionId, uint16_t attrHandle, uint16_t valLen)
{
    qapi_bt_gatt_char_notif_t req = {.sessionId = sessionId, .attrHandle=attrHandle, .val_len=valLen};

    if (valLen > 0)
    {
        req.val = malloc(valLen);
        if (!req.val)
        {
            printk("SRV_APP malloc failed\n");
            return QAPI_BT_GATT_RESULT_INSUFF_RESOURCES;
        }
        memset(req.val, 0xB5, valLen);
    }
    qapi_gatt_result_t ret = qapi_gatt_server_send_char_notif(req);
    if (req.val)
        free(req.val);
    return ret;
}

#ifdef SERVER_APP_BULK_TRANSFER
/**
 * @brief Function to initiate Tx bulk transfer for GATT Server app. 
 * This function sends GATT Notifications to peer device for pktCnt times.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attHandle  GATT characteristic handle.
 * @param pktCnt  Packet Counter to repeat bulk transfer.
 * @param valLen     GATT characteristic value len.
 *
 * @return None
 */
void gatt_server_app_tx_bulk_transfer(int32_t sessionId, uint16_t attHandle, uint16_t pktCnt, uint16_t valLen)
{
    if (srv_tx_bulk_transfer.enable)
    {
        printk("SRV_APP Tx Bulk transfer in progress\n");
        return;
    }
    srv_tx_bulk_transfer.enable = true;
    srv_tx_bulk_transfer.pktCnt = pktCnt;
    srv_tx_bulk_transfer.sessionId = sessionId;
    srv_tx_bulk_transfer.attHandle = attHandle;
    srv_tx_bulk_transfer.valLen = valLen;
    qapi_gatt_result_t ret = gatt_server_app_notif_send(sessionId, attHandle, valLen);
    if (ret != QAPI_BT_GATT_RESULT_SUCCESS)
    {
        printk("SRV_APP Bulk transfer failed to start %d\n", ret);
        memset(&srv_tx_bulk_transfer, 0, sizeof(srv_tx_bulk_transfer));
    }
}
/**
 * @brief Function to initiate Rx bulk transfer for GATT Server app. 
 * This function receives GATT writeWithoutResponse from peer device for pktCnt times.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attHandle  GATT characteristic handle.
 * @param pktCnt  Packet Counter to repeat bulk transfer.
 *
 * @return None
 */
void gatt_server_app_rx_bulk_transfer(int32_t sessionId, uint16_t attHandle, uint16_t pktCnt)
{
    if (srv_rx_bulk_transfer.enable)
    {
        printk("SRV_APP Rx Bulk transfer in progress\n");
        return;
    }
    srv_rx_bulk_transfer.enable = true;
    srv_rx_bulk_transfer.pktCnt = pktCnt;
    srv_rx_bulk_transfer.sessionId = sessionId;
    srv_rx_bulk_transfer.attHandle = attHandle;
}

#endif /* SERVER_APP_BULK_TRANSFER */


/**
 * @brief Handles events for the BT GATT server micro app.
 *
 * Acts as the main dispatcher for all incoming server events, including activation,
 * deactivation, service registration/unregistration, characteristic read/write,
 * and notifications. Decodes the opcode and performs event-specific operations.
 *
 * @param[in] endpointId  Unique identifier for the endpoint.
 * @param[in] opcode      Opcode specifying the event type.
 * @param[in] data        Pointer to the event-specific data buffer.
 * @param[in] data_len    Size (in bytes) of the event data buffer.
 */
void server_uapp_evt_handler(uint64_t endpointId, uint16_t opcode, void *data, uint16_t data_len)
{
    switch(opcode)
    {
        case QAPI_BT_GATT_UAPP_ACTIVATE_CFM:
        {
            qapi_bt_gatt_app_activate_cfm_t *evt = (qapi_bt_gatt_app_activate_cfm_t *)data;
            printk("SRV_APP activated res: %d", evt->result);
        }
        break;
        case QAPI_BT_GATT_UAPP_DEACTIVATE_CFM:
        {
            qapi_bt_gatt_app_deactivate_cfm_t *evt = (qapi_bt_gatt_app_deactivate_cfm_t *)data;
            printk("SRV_APP deactivated res: %d", evt->result);
        }
        break;
        case QAPI_BT_GATT_UAPP_SERVICE_REGISTERED:
        {
            qapi_bt_gatt_service_registered_t *evt = (qapi_bt_gatt_service_registered_t *)data;
            printk("SRV_APP offloaded: sessionId: %d, Mtu %d numChars %d\n", evt->sessionId, evt->attMtu, evt->numChars);
            //bt_log_byte_stream(evt->serviceUuid, GATT_OFFLOAD_UUID_BYTE_LEN);
            for (uint16_t i=0; i<evt->numChars; i++)
            {
                printk("handle=%d prop=%d\n", evt->characteristics[i].handle, evt->characteristics[i].properties);
                bt_log_byte_stream(evt->characteristics[i].uuid, GATT_OFFLOAD_UUID_BYTE_LEN);
            }
            gatt_db_add_handle(evt->sessionId, evt->characteristics, evt->numChars, evt->attMtu);
        }
        break;
        case QAPI_BT_GATT_UAPP_SERVICE_UNREGISTERED:
        {
            qapi_bt_gatt_service_unregistered_t *evt = (qapi_bt_gatt_service_unregistered_t *)data;
            printk("SRV_APP Unoffloaded: sessionId %d\n", evt->sessionId);
            gatt_db_delete_handle(evt->sessionId);
#ifdef SERVER_APP_BULK_TRANSFER
            if (srv_tx_bulk_transfer.enable == true && srv_tx_bulk_transfer.sessionId == evt->sessionId)
            {
                memset(&srv_tx_bulk_transfer, 0, sizeof(srv_tx_bulk_transfer));
            }

            if (srv_rx_bulk_transfer.enable == true && srv_rx_bulk_transfer.sessionId == evt->sessionId)
            {
                memset(&srv_rx_bulk_transfer, 0, sizeof(srv_rx_bulk_transfer));
            }
#endif

        }
        break;
        case QAPI_BT_GATT_UAPP_SERVER_CHAR_READ_REQ:
        {
            qapi_bt_gatt_read_char_req_t *evt = (qapi_bt_gatt_read_char_req_t *)data;
            printk("SRV_APP Read_req: sessionId %d Handle %d\n", evt->sessionId, evt->attrHandle);

            /* Fetch and Send response to Q6 MicroStack*/
            qapi_bt_gatt_read_char_rsp_t rsp = {
                .sessionId = evt->sessionId, 
                .attrHandle=evt->attrHandle, 
                .result = 0,};
			rsp.val = malloc(QAPI_GATT_MTU_MAX);
			if (!rsp.val)
				return;
            gatt_db_fetch_update(evt->sessionId, evt->attrHandle, &rsp.val_len, rsp.val, true);

            qapi_gatt_result_t ret = qapi_gatt_server_read_char_rsp(rsp);
			free(rsp.val);
            printk("SRV_APP Read_rsp ret %d", ret);
        }
        break;
        case QAPI_BT_GATT_UAPP_SERVER_CHAR_WRITE_REQ:
        {
            qapi_bt_gatt_write_char_req_t *evt = (qapi_bt_gatt_write_char_req_t *)data;
            printk("SRV_APP Write_req: sessionId %d Handle %d val_len %d \n", evt->sessionId, evt->attrHandle, evt->val_len);
            //bt_log_byte_stream(evt->val, evt->val_len);

            /* Update DB and Send response */
            gatt_db_fetch_update(evt->sessionId, evt->attrHandle, &evt->val_len, evt->val, false);

#ifdef SERVER_APP_BULK_TRANSFER
            if (srv_rx_bulk_transfer.enable == true && srv_rx_bulk_transfer.pktCnt > 0 &&
                srv_rx_bulk_transfer.sessionId == evt->sessionId && srv_rx_bulk_transfer.attHandle == evt->attrHandle)
            {
                srv_rx_bulk_transfer.pktCnt--;
                if (srv_rx_bulk_transfer.pktCnt == 0)
                {
                    printk("********** SRV_APP: Rx Bulk Transfer Completed ****************\n");
                    memset(&srv_rx_bulk_transfer, 0, sizeof(srv_rx_bulk_transfer));
                }
            }
#endif /*SERVER_APP_BULK_TRANSFER */

            qapi_bt_gatt_write_char_rsp_t rsp = {
                .sessionId = evt->sessionId, 
                .attrHandle=evt->attrHandle, 
                .result = 0,};
            qapi_gatt_result_t ret = qapi_gatt_server_write_char_rsp( rsp);
            printk("SRV_APP Write_rsp: ret %d\n", ret);
        }
        break;
        case QAPI_BT_GATT_UAPP_SERVER_CHAR_NOTIFICATION_RSP:
        {
            qapi_bt_gatt_char_notif_rsp_t *evt = (qapi_bt_gatt_char_notif_rsp_t *)data;
            printk("SRV_APP Notif_rsp:  result %d", evt->result);
#ifdef SERVER_APP_BULK_TRANSFER
            if (srv_tx_bulk_transfer.enable && srv_tx_bulk_transfer.pktCnt > 0)
            {
                srv_tx_bulk_transfer.pktCnt--;
                if (srv_tx_bulk_transfer.pktCnt == 0)
                {
                    printk("************** SRV_APP TX Bulk Transfer completed ****************\n");
                    memset(&srv_tx_bulk_transfer, 0, sizeof(srv_tx_bulk_transfer));
                }
                else
                {
                    qapi_gatt_result_t ret = gatt_server_app_notif_send(srv_tx_bulk_transfer.sessionId, srv_tx_bulk_transfer.attHandle, 
                        srv_tx_bulk_transfer.valLen);
                    if (ret != QAPI_BT_GATT_RESULT_SUCCESS)
                    {
                        printk("SRV_APP Bulk transfer failed %d pkt_cnt %d\n", ret, srv_tx_bulk_transfer.pktCnt);
                        memset(&srv_tx_bulk_transfer, 0, sizeof(srv_tx_bulk_transfer));
                    }
                }
            }
#endif /* SERVER_APP_BULK_TRANSFER */
        }
        break;
		default:
			break;
    }
}

