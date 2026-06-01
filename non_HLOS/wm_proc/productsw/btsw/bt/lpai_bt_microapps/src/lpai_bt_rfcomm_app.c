/*************************************************************************
 * @file     lpai_bt_rfcomm_app.c
 * @brief    LPAI BT RFCOMM App source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/** Program Specific Header file Inclusions */
#include <stdlib.h>
#include <zephyr/sys/printk.h>
#include "lpai_bt_rfcomm_app.h"
#include "qapi_bt_rfcomm_app.h"
#include <zephyr/kernel.h>

rfcommApp_t rfcommAppInfo;
static uint64_t startTime;
static uint64_t endTime;

void rfcomm_app_add_end_point_details()
{
        /*Add End Point Details*/
        rfcommAppInfo.endPointDetails.endPointId.hubId = RFCOMM_ENDPOINT_HUB_ID;
        rfcommAppInfo.endPointDetails.endPointId.epId = RFCOMM_ENDPOINT_ID;
        memscpy(rfcommAppInfo.endPointDetails.name,sizeof("RFCOMM"),"RFCOMM",sizeof("RFCOMM"));
        rfcommAppInfo.endPointDetails.endPointService.majorVersion = 0x01;
        rfcommAppInfo.endPointDetails.endPointService.minorVersion = 0x01;
        memscpy(rfcommAppInfo.endPointDetails.endPointService.serviceDescriptor,sizeof("RFCOMM_SERVICE"),"RFCOMM_SERVICE",sizeof("RFCOMM_SERVICE"));

        rfcommAppInfo.protocolType = SOCKET_TYPE_RFCOMM;
        rfcommAppInfo.socketsInUse = 0x00;
        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
        {
                memset((rfcommAppInfo.socketDetails + idx), 0 , sizeof(socketDetails_t));
        }
}

/*
 * Register a rfcomm app.
 */
void rfcomm_app_init()
{
        rfcomm_app_add_end_point_details();
        /*Register End Point with BT App Manager*/
        if(lpai_bt_app_mgr_register_endpt_client(rfcomm_app_cb, rfcommAppInfo.endPointDetails)!= APP_REGISTRATION_SUCCESS)
        {
                printk("Rfcomm App callback registration Failed");
        }
}

/**
 * @brief Initializes the RFCOMM microApp.
 *
 * This function resets the necessary configurations, data structures, and
 * resources required to start the RFCOMM MicroApp.
 *
 * @note This function does not take any parameters and does not return a value.
 */
void rfcomm_app_deinit()
{
        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
        {
                memset((rfcommAppInfo.socketDetails + idx), 0 , sizeof(socketDetails_t));
        }
}

/**
 * @brief  Method to Check if the recieved BT Protocol Type matches with the protocol for a given End Point.
 * @param[in]   protocolType   Protocol Type to be Validated for an End Point
 * @param[out]  None
 * @return      bool            True if protocol matches , false otherwise
 */
static bool lpai_bt_app_mgr_validate_protocol_type(uint8_t protocolType )
{
    return ( rfcommAppInfo.protocolType == protocolType);
}

/*
 * Open a rfcomm socket.
 */
void rfcomm_socket_opened(void *appData)
{
        uint8_t protocolType = 0xFF ;
        uapp_socket_open_rsp_t socketOpenRsp;
        protocolType = ((uapp_socket_open_req_t*)appData)->socket_type;
        socketOpenRsp.socket_id = ((uapp_socket_open_req_t*)appData)->socket_id;
        if(lpai_bt_app_mgr_validate_protocol_type(protocolType))
        {
                if(rfcommAppInfo.socketsInUse < MAX_SOCKET_SUPPORTED)
                {
                        rfcommAppInfo.socketsInUse++;
                        socketOpenRsp.status = SOCKET_OPEN_SUCCESS;
                        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
                        {
                                if(rfcommAppInfo.socketDetails[idx].idxInUse == false)
                                {
                                        rfcommAppInfo.socketDetails[idx].idxInUse = true;
                                        rfcommAppInfo.socketDetails[idx].remoteMtu = ((uapp_socket_open_req_t*)appData)->socket_info.rfcomm_socket_info.remotemtu;
                                        rfcommAppInfo.socketDetails[idx].socketId = ((uapp_socket_open_req_t*)appData)->socket_id;
                                        lpai_bt_appmgr_send_endpt_msg_adsp(rfcommAppInfo.endPointDetails.endPointId.epId,UAPP_OPEN_SOCKET_RES,sizeof(socketOpenRsp),&socketOpenRsp,false);
                                        break;
                                }
                        }    
                }
                else
                {
                        printk("Maximum Number of Sockets Supported Exhausted\n");
                        socketOpenRsp.status = SOCKET_OPEN_FAILURE;
                        lpai_bt_appmgr_send_endpt_msg_adsp(rfcommAppInfo.endPointDetails.endPointId.epId,UAPP_OPEN_SOCKET_RES,sizeof(socketOpenRsp),&socketOpenRsp,false);
                }
        }
        else
        {
                printk("Open Socket Request Does not match with Protocol Type\n");
                socketOpenRsp.status = SOCKET_OPEN_PROTOCOL_MISMATCH;
                lpai_bt_appmgr_send_endpt_msg_adsp(rfcommAppInfo.endPointDetails.endPointId.epId,UAPP_OPEN_SOCKET_RES,sizeof(socketOpenRsp),&socketOpenRsp,false);
        }
}

/*
 * Close a rfcomm socket.
 */
void rfcomm_socket_closed(uint64_t socketId)
{
        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
        {
                if(rfcommAppInfo.socketDetails[idx].socketId == socketId)
                {
                        rfcommAppInfo.socketsInUse--;
                        memset((rfcommAppInfo.socketDetails + idx), 0 , sizeof(socketDetails_t));
                        return;
                }
        }
        printk("Close Socket Request Does not match with Socket id\n");
}
/*
 * rfcomm data Rx ind.
 */
void rfcomm_socket_data_rx_ind(uint64_t socketId, uint16_t appDataLen,void *appData)
{
        //  for(int i=0;i<appDataLen;i++)
        //  {
        //          printk("%02x ",((uint8_t*)appData)[i]);
        //  }
        //  printk("\n");
        //printk("rfcomm_rx_ind[%d]\n",appDataLen);
        uapp_data_rx_rsp_t rxRsp;
        rxRsp.socket_id = socketId;
        //lpai_bt_appmgr_send_endpt_msg_adsp(rfcommAppInfo.endPointDetails.endPointId.epId,UAPP_DATA_RX_RES,sizeof(rxRsp),&rxRsp,false);
}

/*
 * rfcomm data Tx req.
 */
void rfcomm_socket_data_tx_req(uint64_t socketId, uint16_t dataLen,void *data)
{
        uapp_data_tx_req_t *tx_req = malloc(sizeof(uapp_data_tx_req_t) + dataLen);
        if(tx_req != NULL)
        {
                tx_req->socket_id = socketId;
                tx_req->data_len =  dataLen;
                memscpy(tx_req->data,dataLen,data,dataLen);
                lpai_bt_appmgr_send_endpt_msg_adsp(rfcommAppInfo.endPointDetails.endPointId.epId, UAPP_DATA_TX_REQ, (sizeof(uapp_data_tx_req_t) + dataLen), tx_req,false);
                free(tx_req);
        }
}
/*
* data frame to be send for tx
*/
static void send_data_frame(uint16_t packetSize, uint8_t *data)
{
	for(int i=0;i<packetSize;i++)
        {
            data[i] = 0x61;
    	}

}

/*
QAPI for TX throughput testing
*/
qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_test_tput_tx(uint64_t EndpointId, uint64_t socketId, uint16_t packetSize, uint16_t numPackets)
{
		if(packetSize == 0)
		{
				printk("Error:packetSize should be greater than 0\n");
				return QAPI_BT_RFCOMM_INVALID_OPERATION;
		}
        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
        {
                if(rfcommAppInfo.socketDetails[idx].socketId == socketId)
                {
                        if(rfcommAppInfo.socketDetails[idx].remoteMtu > packetSize) 
                        {      
                                if(rfcommAppInfo.socketDetails[idx].pendingOperations == 0)
                                {
                                        rfcommAppInfo.socketDetails[idx].pendingOperations = TX_TPUT_ENABLE;
                                        //mtu check and operation allowed means
                                        if(rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt == 0)
                                        {
                                                rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt = numPackets;
                                                rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.txPacketSize = packetSize;
                                                uint8_t *data = malloc(packetSize);
                                                if(data != NULL)
                                                {
                                                        send_data_frame(packetSize, data);
                                                        startTime = k_uptime_get();
                                                        rfcomm_socket_data_tx_req(rfcommAppInfo.socketDetails[idx].socketId, packetSize, data);
                                                        free(data);
                                                }
                                        }
                                }
                                else
                                {
                                        printk("Multiple operations enabled\n");
                                        return QAPI_BT_RFCOMM_INVALID_OPERATION;
                                }
                        }
                        else
                        {
                            printk("MTU Size exceed\n");  
                            return QAPI_BT_RFCOMM_MTU_EXCEEDED;  
                        }
                        return QAPI_BT_RFCOMM_SUCCESS;
                }
        }
        return QAPI_BT_RFCOMM_FAILURE;
}

/*
QAPI for RX throughput testing
*/
qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_test_tput_rx(uint64_t EndpointId, uint64_t socketId, uint16_t packetSize, uint16_t numPackets)
{
		if(packetSize == 0)
		{
				printk("Error:packetSize should be greater than 0\n");
				return QAPI_BT_RFCOMM_INVALID_OPERATION;
		}
        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
        {
                if(rfcommAppInfo.socketDetails[idx].socketId == socketId)
                {
                        if(rfcommAppInfo.socketDetails[idx].remoteMtu > packetSize) 
                        {      
                                if(rfcommAppInfo.socketDetails[idx].pendingOperations == 0)
                                {
                                        rfcommAppInfo.socketDetails[idx].pendingOperations = RX_TPUT_ENABLE;
                                        rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt = numPackets;
                                        rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.txPacketSize = packetSize;
                                        rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt = 0;
                                }
                                else
                                {
                                        printk("Multiple operations enabled\n");
                                        return QAPI_BT_RFCOMM_INVALID_OPERATION;
                                }
                        }
                        else
                        {
                            printk("MTU Size exceed\n");  
                            return QAPI_BT_RFCOMM_MTU_EXCEEDED;  
                        }
                        return QAPI_BT_RFCOMM_SUCCESS;
                }
        }
        return QAPI_BT_RFCOMM_FAILURE;
}


/*
QAPI for data send data
*/
qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_send_data(uint64_t EndpointId, uint64_t socketId, uint16_t dataLen, uint8_t* data)
{
        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
        {
                if(rfcommAppInfo.socketDetails[idx].socketId == socketId)
                {
                        if(rfcommAppInfo.socketDetails[idx].remoteMtu > dataLen) 
                        {
                                if(rfcommAppInfo.socketDetails[idx].pendingOperations == 0)
                                {
                                        rfcommAppInfo.socketDetails[idx].pendingOperations = TX_ENABLE;

                                        uint8_t *userData = malloc(dataLen);
                                        if(userData != NULL)
                                        {
                                                memscpy(userData, dataLen, data, dataLen);
                                                rfcomm_socket_data_tx_req(rfcommAppInfo.socketDetails[idx].socketId, dataLen, userData);
                                                free(userData);
                                                break;
                                        }
                                }
                                else
                                {
                                        printk("Multiple operations enabled\n");
                                        return QAPI_BT_RFCOMM_INVALID_OPERATION;
                                }
                        }
                        else
                        {
                              printk("MTU Size exceed\n"); 
                              return QAPI_BT_RFCOMM_MTU_EXCEEDED; 
                        }
                        return QAPI_BT_RFCOMM_SUCCESS;
                }
        }
        return QAPI_BT_RFCOMM_FAILURE;
}

/*
QAPI for loopback test
*/
qapi_bt_rfcomm_status_code_t qapi_rfcomm_test_loopback(uint64_t EndpointId, uint64_t socketId)
{
        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
        {
                if(rfcommAppInfo.socketDetails[idx].socketId == socketId )
                {
                        if(rfcommAppInfo.socketDetails[idx].pendingOperations == 0)
                        {
                                rfcommAppInfo.socketDetails[idx].pendingOperations = LOOPBACK_ENABLE;
                        }
                        else
                        {
                                printk("Multiple operations enabled\n");
                                return QAPI_BT_RFCOMM_INVALID_OPERATION;
                        }
                        return QAPI_BT_RFCOMM_SUCCESS;
                }
        }
        return QAPI_BT_RFCOMM_FAILURE;
}

/*
QAPI to request socket close through shell
*/
qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_close_socket(uint64_t EndpointId, uint64_t socketId)
{
        qapiSocketCloseReq_t socketCloseReq;
        socketCloseReq.socketId = socketId;
        socketCloseReq.reason = 0x00;

        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
        {
                if(rfcommAppInfo.socketDetails[idx].socketId == socketId )
                {
                        lpai_bt_appmgr_send_endpt_msg_adsp(rfcommAppInfo.endPointDetails.endPointId.epId,UAPP_SOCKET_CLOSE_IND,sizeof(socketCloseReq),&socketCloseReq,false);
                        return QAPI_BT_RFCOMM_SUCCESS;
                }
        }
        return QAPI_BT_RFCOMM_FAILURE;
}
/*
QAPI to fetch the offloaded socket details
*/
qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_get_socketdetails()
{
        if(rfcommAppInfo.socketsInUse>0)
        {
                for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
                {
                        printk("Socket ID = %" PRIu64 "\n", rfcommAppInfo.socketDetails[idx].socketId);
                }
        }
        else
        {
                return QAPI_BT_RFCOMM_NO_SOCKET_DATA;
        }
        return QAPI_BT_RFCOMM_SUCCESS;
}

/*
 * Handle event received from BT app mgr
 */
void rfcomm_app_cb(uint64_t endPointId, uint16_t eventId , uint16_t appDataLen , void *appData, bool proto_encoded)
{
        switch(eventId)
        {
                case UAPP_OPEN_SOCKET_REQ:
                {
                        rfcomm_socket_opened(appData);
                        qapiSocketOpened_t socketOpened;
                        socketOpened.socketId = ((uapp_socket_open_req_t*)appData)->socket_id;
                        qapi_rfcomm_evt_handler(QAPI_BT_RFCOMM_SOCKET_OPENED, sizeof(socketOpened),&socketOpened);
                        break;
                }

                case UAPP_SOCKET_CLOSE_CMD:
                {
                        socketClosed_t socketClosed;
                        socketClosed.socketId = ((uapp_socket_close_cmd_t*)appData)->socket_id;
                        rfcomm_socket_closed(socketClosed.socketId);

                        qapi_rfcomm_evt_handler(QAPI_BT_RFCOMM_SOCKET_CLOSED, sizeof(socketClosed),&socketClosed);
                        break;
                }
                case UAPP_DATA_RX_IND:
                {
                        qapiDataRxInd_t *evt = (qapiDataRxInd_t*)appData;
                        qapiDataTxCfm_t dataTxCfm = {.socketId=evt->socketId};
                        //printk("Rx Indication Received for RFCOMM APP\n");

                        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
                        {
                                if(rfcommAppInfo.socketDetails[idx].socketId == evt->socketId)
                                {
                                        rfcomm_socket_data_rx_ind(evt->socketId, evt->dataLen, evt->data);
                                        qapi_rfcomm_evt_handler(QAPI_BT_RFCOMM_SOCKET_DATA_IND, sizeof(evt),evt);
                                        if(rfcommAppInfo.socketDetails[idx].pendingOperations == LOOPBACK_ENABLE)
                                        {
                                                printk("Rx Indication Received for rfcomm loopback\n");
                                                rfcomm_socket_data_tx_req(rfcommAppInfo.socketDetails[idx].socketId, evt->dataLen, evt->data);
                                                rfcommAppInfo.socketDetails[idx].pendingOperations = 0;
                                                qapi_rfcomm_evt_handler(QAPI_BT_RFCOMM_SOCKET_DATA_LOOPBACK, sizeof(*(evt)),evt);
                                        }
                                        else if (rfcommAppInfo.socketDetails[idx].pendingOperations == RX_TPUT_ENABLE)
                                        {
                                            //printk("rx Tput packet %d\n", rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt);
                                            //start the timer for 1st received packet
                                            if (rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt == 0)
                                            {
                                                startTime = k_uptime_get();
                                            }
                                            if(rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt < rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt)
                                            {
                                                rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt++;
                                            }
                                            if (rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt == rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt)
                                            {
                                                endTime = k_uptime_get();
                                                double tput = (rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt * rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.txPacketSize *  8)/((double)(endTime - startTime));
                                                printk("RFCOMM RX throught =  %.2f kbps\n", tput);
                                                rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt = rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.txPacketSize = rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt = 0;
                                                rfcommAppInfo.socketDetails[idx].pendingOperations = 0;
                                                dataTxCfm.status = QAPI_BT_RFCOMM_SUCCESS;
                                                qapi_rfcomm_evt_handler(QAPI_BT_RFCOMM_SOCKET_DATA_THROUGHPUT, sizeof(dataTxCfm),&dataTxCfm);
                                            }
                                        }
                                }
                        }
                        break;
                }
                case UAPP_DATA_TX_RES:
                {       
                        qapiDataTxCfm_t dataTxCfm;
                        dataTxCfm.socketId = ((uapp_data_tx_cfm_t*)appData)->socket_id;
                        for(uint8_t idx = 0; idx < MAX_SOCKET_SUPPORTED; idx++)
                        {
                                if(rfcommAppInfo.socketDetails[idx].socketId == dataTxCfm.socketId)
                                {
                                        if(rfcommAppInfo.socketDetails[idx].pendingOperations == TX_TPUT_ENABLE)
                                        {

                                                if(rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt < rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt)
                                                {
                                                        rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt++;
                                                        
                                                        uint8_t *data = malloc(rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.txPacketSize);
                                                        if(data != NULL)
                                                        {
                                                                send_data_frame(rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.txPacketSize, data);

                                                                rfcomm_socket_data_tx_req(rfcommAppInfo.socketDetails[idx].socketId, rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.txPacketSize, data);
                                                                free(data);
                                                        }
                                                }
                                                if(rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt == rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt)
                                                {
                                                        endTime = k_uptime_get();
                                                        double tput = (rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt * rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.txPacketSize *  8)/((double)(endTime - startTime));
                                                        printk("RFCOMM TX throught =  %.2f kbps\n", tput);
                                                        rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.curCnt = rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.txPacketSize = rfcommAppInfo.socketDetails[idx].txOperations.tputPktDetail.totalCnt = 0;
                                                        rfcommAppInfo.socketDetails[idx].pendingOperations = 0;
                                                        qapi_rfcomm_evt_handler(QAPI_BT_RFCOMM_SOCKET_DATA_THROUGHPUT, sizeof(dataTxCfm),&dataTxCfm);
                                                }
                                        }
                                        else if(rfcommAppInfo.socketDetails[idx].pendingOperations == LOOPBACK_ENABLE)
                                        {
                                                // rfcommAppInfo.socketDetails[idx].pendingOperations = 0;
                                                // qapi_rfcomm_evt_handler(QAPI_BT_RFCOMM_SOCKET_DATA_LOOPBACK, sizeof(dataTxCfm),&dataTxCfm);

                                        }
                                        else if(rfcommAppInfo.socketDetails[idx].pendingOperations == TX_ENABLE)
                                        {
                                                qapi_rfcomm_evt_handler(QAPI_BT_RFCOMM_SOCKET_DATA_TX_CFM, sizeof(dataTxCfm),&dataTxCfm);
                                                rfcommAppInfo.socketDetails[idx].pendingOperations = 0;
                                        }
                                }
                        }
                        break;
                }
                default:
                {
			printk("Undefined event\n");
                }
        }
}

void qapi_rfcomm_evt_handler(uint16_t opcode,uint16_t appDataLen , void *appData)
{
        switch(opcode)
        {
                case QAPI_BT_RFCOMM_SOCKET_OPENED:
                {
                        printk("QAPI_BT_RFCOMM_SOCKET_OPENED\n");
                        break;
                }
                case QAPI_BT_RFCOMM_SOCKET_CLOSED:
                {
                        printk("QAPI_BT_RFCOMM_SOCKET_CLOSED\n");
                        break;
                }
                case QAPI_BT_RFCOMM_SOCKET_DATA_TX_CFM:
                {
                        printk("QAPI_BT_RFCOMM_SOCKET_DATA_TX_CFM\n");
                        break;
                }
                case QAPI_BT_RFCOMM_SOCKET_DATA_LOOPBACK:
                {
                        printk("QAPI_BT_RFCOMM_SOCKET_DATA_LOOPBACK\n");
                        break;
                }
                case QAPI_BT_RFCOMM_SOCKET_DATA_THROUGHPUT:
                {
                        printk("QAPI_BT_RFCOMM_SOCKET_DATA_THROUGHPUT\n");
                        break;
                }
                case QAPI_BT_RFCOMM_SOCKET_DATA_IND:
                {
                        //printk("QAPI_BT_RFCOMM_SOCKET_DATA_IND\n");
                        /* TBD
                        integrate with Display
                        integrate with AUDIO*/
                        break;
                }
        }
}
