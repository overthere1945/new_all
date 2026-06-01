/*************************************************************************
 * @file     lpai_bt_gatt_client_app.c
 * @brief    LPAI BT GATT Client Micro App source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/** Program Specific Header file Inclusions */
#include <zephyr/sys/printk.h>
#include "lpai_bt_gatt_client_app.h"
#include "bt_utilities.h"

#ifdef CLIENT_APP_BULK_TRANSFER
/**
 * @struct client_app_bulk_transfer
 * @brief Structure for GATT micro-app bulk transfer information.
 */
typedef struct client_app_bulk_transfer {
    int32_t sessionId; /**< session ID */
    uint16_t attHandle; /*< Attribute Handle */
    uint16_t pktCnt;  /**< Packet counter */
    uint16_t valLen; /*< Attribute value len */
    bool enable; /*< Flag for enabling bulk transfer */
} client_app_bulk_transfer_t;

/**
 * @brief Variable used for TX bulk data trasfer.
 */
static client_app_bulk_transfer_t cl_tx_bulk_transfer = {0};

/**
 * @brief Variable used for RX bulk data trasfer.
 */
static client_app_bulk_transfer_t cl_rx_bulk_transfer = {0};

#endif /*CLIENT_APP_BULK_TRANSFER */


/**
 * @brief DeInitializes the GATT Client Microapp Structures.
 *
 * This function resets the necessary configurations, data structures, and
 * resources required to start the Gatt Client Gatt Structures.
 *
 * @note This function does not take any parameters and does not return a value.
 */
void gatt_client_uapp_deinit()
{
    memset(&cl_tx_bulk_transfer,0,sizeof(client_app_bulk_transfer_t));
    memset(&cl_rx_bulk_transfer,0,sizeof(client_app_bulk_transfer_t));
}

/**
 * @brief Event handler for GATT client micro-app events.
 *
 * Handles various GATT QAPI events received by the GATT client application.
 *
 * @param endpointId  Identifier for the endpoint (application/micro-app).
 * @param opcode      Event operation code from the GATT QAPI.
 * @param data        Pointer to event data, type depends on opcode.
 * @param data_len    Length of the event data.
 *
 * @return None
 */
void client_uapp_evt_handler(uint64_t endpointId, uint16_t opcode, void *data, uint16_t data_len);

/**
 * @brief Register and initialize the GATT client application.
 *
 * Prepares the EndPoint details and registers the application endpoint
 * with the GATT QAPI using the provided event handler.
 *
 * @return None
 */
void gatt_client_app_init()
{
    endPointDetails_t endpoint = {0};

	/*Add End Point Details*/
	endpoint.endPointId.hubId = GATT_CLIENT_APP_HUB_ID;
	endpoint.endPointId.epId = GATT_CLIENT_APP_EP_ID;
	memscpy(endpoint.name,ENDPT_NAME_MAX_LEN, GATT_CLIENT_APP_EP_NAME,
        sizeof(GATT_CLIENT_APP_EP_NAME));
	endpoint.endPointService.majorVersion = 0x01;
	endpoint.endPointService.minorVersion = 0x01;
	memscpy(endpoint.endPointService.serviceDescriptor,ENDPT_SERVICE_DESC_MAX_LEN, 
        GATT_CLIENT_APP_EP_SVC_NAME,sizeof(GATT_CLIENT_APP_EP_SVC_NAME));

    qapi_gatt_app_register(endpoint, client_uapp_evt_handler);
}

/**
 * @brief Function to send GATT Client Write Request or Command to peer device.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attrHandle  GATT characteristic handle.
 * @param valLen     GATT characteristic value len.
 * @param write_cmd  Request is write request or write without response (write_cmd).
 *
 * @return qapi_gatt_result_t
 */
qapi_gatt_result_t gatt_client_app_send_write_req(int32_t sessionId, uint16_t attrHandle, uint16_t valLen, bool write_cmd)
{
    qapi_bt_gatt_write_char_req_t req = {.sessionId = sessionId, .attrHandle=attrHandle, .val_len=valLen, .writeWithoutRsp=write_cmd};
    if (valLen > 0)
    {
        req.val = malloc(valLen);
        if (!req.val)
        {
            printk("CLIENT_APP malloc failed\n");
            return QAPI_BT_GATT_RESULT_INSUFF_RESOURCES;
        }
        memset(req.val, 0xA5, valLen);
    }
    qapi_gatt_result_t ret = qapi_gatt_client_write_char_req(req);
    if (req.val)
        free(req.val);
    return ret;
}

#ifdef CLIENT_APP_BULK_TRANSFER
/**
 * @brief Function to initiate Tx bulk transfer for GATT Client app. 
 * This function sends writeWithoutResponse to peer device for pktCnt times.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attHandle  GATT characteristic handle.
 * @param pktCnt  Packet Counter to repeat bulk transfer.
 * @param valLen     GATT characteristic value len.
 *
 * @return None
 */
void gatt_client_app_tx_bulk_transfer(int32_t sessionId, uint16_t attHandle, uint16_t pktCnt, uint16_t valLen)
{
    if (cl_tx_bulk_transfer.enable)
    {
        printk("CLIENT_APP Bulk transfer in progress\n");
        return;
    }
    cl_tx_bulk_transfer.enable = true;
    cl_tx_bulk_transfer.pktCnt = pktCnt;
    cl_tx_bulk_transfer.sessionId = sessionId;
    cl_tx_bulk_transfer.attHandle = attHandle;
    cl_tx_bulk_transfer.valLen = valLen;
    qapi_gatt_result_t ret = gatt_client_app_send_write_req(sessionId, attHandle, valLen, true);
    if (ret != QAPI_BT_GATT_RESULT_SUCCESS)
    {
        printk("CLIENT_APP Bulk transfer failed to start %d\n", ret);
        memset(&cl_tx_bulk_transfer, 0, sizeof(cl_tx_bulk_transfer));
    }
}

/**
 * @brief Function to initiate Rx bulk transfer for GATT Client app. 
 * This function receives Notifications from peer device for pktCnt times.
 *
 * @param sessionId  GATT offload session identifier.
 * @param attHandle  GATT characteristic handle.
 * @param pktCnt  Packet Counter to repeat bulk transfer.
 *
 * @return None
 */
void gatt_client_app_rx_bulk_transfer(int32_t sessionId, uint16_t attHandle, uint16_t pktCnt)
{
    if (cl_rx_bulk_transfer.enable)
    {
        printk("CLIENT_APP Rx Bulk transfer started\n");
        return;
    }
    cl_rx_bulk_transfer.enable = true;
    cl_rx_bulk_transfer.pktCnt = pktCnt;
    cl_rx_bulk_transfer.sessionId = sessionId;
    cl_rx_bulk_transfer.attHandle = attHandle;
}

#endif /* CLIENT_APP_BULK_TRANSFER*/

/**
 * @brief Callback for handling GATT QAPI events in the micro app.
 *
 * Processes different GATT client events (e.g., activation/deactivation,
 * read/write, notifications) using the opcode to branch between event types.
 *
 * @param endpointId  Identifier for the endpoint (application/micro-app).
 * @param opcode      Event operation code from the GATT QAPI.
 * @param data        Pointer to event data, type depends on opcode.
 * @param data_len    Length of the event data.
 *
 * @return None
 */
void client_uapp_evt_handler(uint64_t endpointId, uint16_t opcode, void *data, uint16_t data_len)
{
    switch(opcode)
    {
        case QAPI_BT_GATT_UAPP_ACTIVATE_CFM:
        {
            qapi_bt_gatt_app_activate_cfm_t *evt = (qapi_bt_gatt_app_activate_cfm_t *)data;
            printk("CLIENT_APP activated result: %d\n", evt->result);
        }
        break;
        case QAPI_BT_GATT_UAPP_DEACTIVATE_CFM:
        {
            qapi_bt_gatt_app_deactivate_cfm_t *evt = (qapi_bt_gatt_app_deactivate_cfm_t *)data;
            printk("CLIENT_APP deactivated result: %d\n", evt->result);
        }
        break;
        case QAPI_BT_GATT_UAPP_SERVICE_REGISTERED:
        {
            qapi_bt_gatt_service_registered_t *evt = (qapi_bt_gatt_service_registered_t *)data;
            printk("CLIENT_APP offloaded: sessionId: %d, Mtu %d numChars %d\n",\
                evt->sessionId, evt->attMtu, evt->numChars);
            //bt_log_byte_stream(evt->serviceUuid, GATT_OFFLOAD_UUID_BYTE_LEN);
            for (uint16_t i=0; i<evt->numChars; i++)
            {
                printk("handle=%d prop=%d \n", evt->characteristics[i].handle, evt->characteristics[i].properties);
                bt_log_byte_stream(evt->characteristics[i].uuid, GATT_OFFLOAD_UUID_BYTE_LEN);
            }
        }
        break;
        case QAPI_BT_GATT_UAPP_SERVICE_UNREGISTERED:
        {
            qapi_bt_gatt_service_unregistered_t *evt = (qapi_bt_gatt_service_unregistered_t *)data;
            printk("CLIENT_APP unoffloaded: sessionId %d\n", evt->sessionId);
#ifdef CLIENT_APP_BULK_TRANSFER
            if (cl_tx_bulk_transfer.enable == true && cl_tx_bulk_transfer.sessionId == evt->sessionId)
            {
                memset(&cl_tx_bulk_transfer, 0, sizeof(cl_tx_bulk_transfer));
            }
            if (cl_rx_bulk_transfer.enable == true && cl_rx_bulk_transfer.sessionId == evt->sessionId)
            {
                memset(&cl_rx_bulk_transfer, 0, sizeof(cl_rx_bulk_transfer));
            }
#endif
        }
        break;
        case QAPI_BT_GATT_UAPP_CLIENT_CHAR_READ_RSP:
        {
            qapi_bt_gatt_read_char_rsp_t *evt = (qapi_bt_gatt_read_char_rsp_t *)data;
            printk("CLIENT_APP Read_Rsp sessionId %d Handle %d res %d val_len %d\n",\
                evt->sessionId, evt->attrHandle,evt->result, evt->val_len);
            if (evt->val_len && evt->val)
            {
                //bt_log_byte_stream(evt->val, evt->val_len);
            }

            /* place holder to process characteristic value that is read */
        }
        break;
        case QAPI_BT_GATT_UAPP_CLIENT_CHAR_WRITE_RSP:
        {
            qapi_bt_gatt_write_char_rsp_t *evt = (qapi_bt_gatt_write_char_rsp_t *)data;
            printk("CLIENT_APP write_rsp sessionId %d Handle %d res %d\n",\
                            evt->sessionId, evt->attrHandle,evt->result);
#ifdef CLIENT_APP_BULK_TRANSFER
            if (cl_tx_bulk_transfer.enable && cl_tx_bulk_transfer.pktCnt > 0 && 
                cl_tx_bulk_transfer.sessionId == evt->sessionId && cl_tx_bulk_transfer.attHandle == evt->attrHandle)
            {
                cl_tx_bulk_transfer.pktCnt--;
                if (cl_tx_bulk_transfer.pktCnt == 0)
                {
                    printk("******** CLIENT_APP Tx Bulk Transfer completed ***********\n");
                    memset(&cl_tx_bulk_transfer, 0, sizeof(cl_tx_bulk_transfer));
                }
                else
                {
                    qapi_gatt_result_t ret = gatt_client_app_send_write_req(cl_tx_bulk_transfer.sessionId, cl_tx_bulk_transfer.attHandle, 
                        cl_tx_bulk_transfer.valLen, 1);
                    if (ret != QAPI_BT_GATT_RESULT_SUCCESS)
                    {
                        printk("CLIENT_APP Bulk transfer failed %d pkt_cnt %d\n", ret, cl_tx_bulk_transfer.pktCnt);
                        memset(&cl_tx_bulk_transfer, 0, sizeof(cl_tx_bulk_transfer));
                    }
                }
            }
#endif /* CLIENT_APP_BULK_TRANSFER */
        }
        break;
        case QAPI_BT_GATT_UAPP_CLIENT_CHAR_NOTIFICATION_IND:
        {
            qapi_bt_gatt_char_notif_t *evt = (qapi_bt_gatt_char_notif_t *)data;
            printk("CLIENT_APP Notif: sessionId %d Handle %d val_len %d\n ",\
                evt->sessionId, evt->attrHandle, evt->val_len);
            if (evt->val_len && evt->val)
            {
                //bt_log_byte_stream(evt->val, evt->val_len);
            }
            /* place holder to process characteristic value that is received */

#ifdef CLIENT_APP_BULK_TRANSFER
            if (cl_rx_bulk_transfer.enable == true && cl_rx_bulk_transfer.pktCnt > 0 && 
                cl_rx_bulk_transfer.sessionId == evt->sessionId && cl_rx_bulk_transfer.attHandle ==  evt->attrHandle)
            {
                cl_rx_bulk_transfer.pktCnt--;
                if (cl_rx_bulk_transfer.pktCnt == 0)
                {
                    printk("************* CLIENT_APP Rx Bulk Transfer completed ***************** \n");
                    memset(&cl_rx_bulk_transfer, 0, sizeof(cl_rx_bulk_transfer));
                }
            }
#endif /* CLIENT_APP_BULK_TRANSFER */
            /* place holder to send confirmation for indication */
            qapi_bt_gatt_char_notif_rsp_t rsp = {.result=0};
            qapi_gatt_result_t ret = qapi_gatt_client_notif_rsp(rsp);
            printk("CLIENT_APP Notif Resp ret %d\n", ret);
        }
        break;
		default:
			break;
    }
}

