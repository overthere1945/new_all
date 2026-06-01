/**************************************************************************
 * @file     lpai_bt_app_mgr_adsp_handler.h
 * @brief    LPAI BT APP Manager ADSP Interface Handler File.
 * 			 This file contains all the type declarations and opcodes for messages
 * 			 exhanges between ADSP and AWM Subsystems
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_APP_MGR_ADSP_HANDLER_H
#define LPAI_BT_APP_MGR_ADSP_HANDLER_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "stdint.h"
#include "stdbool.h"
#include "glink.h"
#include "lpai_bt_state_mgr.h"

/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/

#define PROTO_BUFF_SIZE 512

#define UAPP_BT_STATUS 0x0F00
#define UAPP_OPEN_SOCKET_REQ 0x0A01
#define UAPP_OPEN_SOCKET_RES UAPP_OPEN_SOCKET_REQ
#define UAPP_SOCKET_CLOSE_CMD 0x0A02
#define UAPP_DATA_TX_REQ 0x0A03
#define UAPP_DATA_TX_RES UAPP_DATA_TX_REQ
#define UAPP_DATA_RX_IND 0x0A04
#define UAPP_DATA_RX_RES UAPP_DATA_RX_IND
#define UAPP_ENDPT_DISC_REQ 0x00C0
#define UAPP_ENDPT_DISC_RES 0x00C1
#define UAPP_SOCKET_CLOSE_IND 0x0A05

//GATT  Micro App Opcodes
#define UAPP_GATT_APP_REG_REQ                   0x0A10
#define UAPP_GATT_APP_REG_RESP                  UAPP_GATT_APP_REG_REQ
#define UAPP_GATT_APP_UNREG_REQ                 0x0A11
#define UAPP_GATT_APP_UNREG_RESP                UAPP_GATT_APP_UNREG_REQ
#define UAPP_GATT_REGISTER_SERVICE_REQ          0x0A12
#define UAPP_GATT_REGISTER_SERVICE_RSP          UAPP_GATT_REGISTER_SERVICE_REQ
#define UAPP_GATT_UNREGISTER_SERVICE_REQ        0x0A13
#define UAPP_GATT_UNREGISTER_SERVICE_RSP        UAPP_GATT_UNREGISTER_SERVICE_REQ
#define UAPP_GATT_UNREGISTER_SERVICE_IND        0x0A14
#define UAPP_GATT_CLIENT_READ_CHAR_REQ          0x0A15
#define UAPP_GATT_CLIENT_READ_CHAR_RESP         UAPP_GATT_CLIENT_READ_CHAR_REQ
#define UAPP_GATT_CLIENT_WRITE_CHAR_REQ         0x0A16
#define UAPP_GATT_CLIENT_WRITE_CHAR_RESP        UAPP_GATT_CLIENT_WRITE_CHAR_REQ
#define UAPP_GATT_CLIENT_NOTIFICATION_IND       0x0A18
#define UAPP_GATT_SERVER_READ_CHAR_IND          0x0A19
#define UAPP_GATT_SERVER_READ_CHAR_IND_RESP     UAPP_GATT_SERVER_READ_CHAR_IND
#define UAPP_GATT_SERVER_WRITE_CHAR_IND         0x0A1A
#define UAPP_GATT_SERVER_WRITE_CHAR_IND_RESP    UAPP_GATT_SERVER_WRITE_CHAR_IND
#define UAPP_GATT_SERVER_NOTIFICATION_REQ       0x0A1B
#define UAPP_GATT_SERVER_NOTIFICATION_RESP      UAPP_GATT_SERVER_NOTIFICATION_REQ 



/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

/**
 * @enum endpt_mgr_rpc_socket_type_t
 * Socket Type to indicate BT Protocol supported by End Point on AWM
 */
typedef enum {
    SOCKET_TYPE_LECOC = 0,     /**< LECOC PROTOCOL Identifier */
    SOCKET_TYPE_RFCOMM,       /**< RFCOMM PROTOCOL Identifier */
    SOCKET_TYPE_GATT,        /**< GATT PROTOCOL Identifier */
}endpt_mgr_rpc_socket_type_t;


/**
 * @enum message_format_t
 * Message Format Indicating if the message is proto encoded or sent as raw stream of bytes
 */
typedef enum {
    MESSAGE_FORMAT_RAW,     /**< Message is raw stream of bytes*/
    MESSAGE_FORMAT_PROTO,  /**< Message is proto encoded */
}message_format_t;


/**
 * @enum command_type_t
 * Command Type included while sending message to ADSP to identify is message is send from End Points or Microapps
 */
typedef enum {
    OFFLOAD_APP_CMD,          /**< Message is intended for End Point */
    NONOFFLOAD_APP_CMD,      /**< Message is intended for MicroApp */
}command_type_t;


/**
 * @enum socket_open_response_t
 * Response Codes to be sent from LPAI BT App Manager when a socket open request is received for end points on AWM from ADSP
 */
typedef enum{
	SOCKET_OPEN_SUCCESS,                /**< Socket Open Operation was Successful */
	SOCKET_OPEN_FAILURE,               /**< Socket Open Operation Failed */
	SOCKET_OPEN_PROTOCOL_MISMATCH,    /**< Socket Open Operation failed as correct BT Protocol was not received for intended end point */
	SOCKET_OPEN_ENDPOINT_NOT_FOUND,  /**< Socket Open Operation failed as invalid End Point Identifier was received */
}socket_open_response_t;


/**
 * @struct endpt_mgr_rpc_header_t
 * Header Format to be included before sending any message to ADSP and parsed when a message is received from ADSP for identifications of end points and microapps
 */
typedef struct endpt_mgr_rpc_header {
	uint16_t cmd_type;              /**< Command Type in header if the request/response is intended for End Points or MicroApps  */
	uint16_t opcode;               /**< Message Opcode for message exchange between ADSP and AWM */
	uint16_t message_format;      /**< Message Format to specify if incoming/outgoing application data is proto encoded or raw stream of bytes */
	uint16_t data_len;           /**< Lenght of the incoming application data*/
	uint64_t endpoint_id;       /**< End Point Identifier or Microapp identifer for which message is intended */
}endpt_mgr_rpc_header_t;


/**
 * @struct lecoc_channel_info_t
 * Socket Related Information for LECOC Socket Open Request
 */
typedef struct lecoc_channel_info {
    uint32_t remotemtu;           /**< Remote MTU */
    uint32_t remoteMps;          /**< Maximum Payload Size*/
}lecoc_channel_info_t;
   

/**
 * @struct gatt_channel_info_t
 * Socket Related Information for GATT Socket Open Request
 */
typedef struct gatt_channel_info {
    uint32_t remotemtu;           /**< Remote MTU */
}gatt_channel_info_t;
   

/**
 * @struct rfcomm_channel_info_t
 * Socket Related Information for RFCOMM Socket Open Request
 */
typedef struct rfcomm_channel_info {
    uint32_t remotemtu;            /**< Remote MTU */
    uint32_t  maxFrameSize;       /**< Maximum Frame Size */
}rfcomm_channel_info_t;
   

/**
 * @union socket_info_t
 * Socket Context Information for Each Socket Open Request
 */
typedef union socket_info {
 lecoc_channel_info_t lecoc_socket_info;      /**< LECOC Socket Information */
 gatt_channel_info_t  gatt_socket_info;      /**< GATT Socket Information */
 rfcomm_channel_info_t rfcomm_socket_info;  /**< RFCOMM Socket Information */
}socket_info_t;


/**
 * @struct uapp_socket_open_req_t
 * Socket Open Request received from ADSP for one of the end points registered with LPAI Bt App Manager on AWM for Offloading Socket Information
 */
typedef struct uapp_socket_open_req {
    uint64_t socket_id;            /**< Socket Identifier received from ADSP for Socket Open request to End Points */
    uint8_t socket_type;          /**< Socket Type to identify BT Protocol for which socket oepn request is received */
    socket_info_t socket_info;   /**< Socket Related Context Information for Specific Socket Open Request */
}uapp_socket_open_req_t;


/**
 * @struct uapp_socket_open_rsp_t
 * Socket Open Response sent from AWM to ADSP when a corrosponding socket open request is received
 */
typedef struct uapp_socket_open_rsp {
    uint64_t socket_id;           /**< Socket Identifier for Sending Socket Open Response */
    uint8_t status;              /**< Status send for Socket Open Request */
}uapp_socket_open_rsp_t;


/**
 * @struct uapp_socket_close_cmd_t
 * Socket Close Command received from ADSP for one of the end points registered with LPAI Bt App Manager on AWM for Clearing Offloaded Socket Information
 */
typedef struct uapp_socket_close_cmd {
    uint64_t socket_id;         /**< Socket Identifier for Close Command */
}uapp_socket_close_cmd_t;


/**
 * @struct uapp_data_rx_ind_t
 * Rx Indication Format for Receiving rx Indication from ADSP
 */
typedef struct uapp_data_rx_ind {
    uint64_t socket_id;         /**< Socket Identifier for Close Command */
    int data_len;              /**< Socket Identifier for Close Command */
    uint8_t data[];           /**< Socket Identifier for Close Command */
}uapp_data_rx_ind_t;


/**
 * @struct uapp_data_rx_rsp_t
 * End Point Data Rx response to ADSP for Rx Operation to End Point
 */
typedef struct uapp_data_rx_rsp {
    uint64_t socket_id;             /**< Socket Identifier Rx Indication Response from End Point */
}uapp_data_rx_rsp_t;


/**
 * @struct uapp_data_tx_req_t
 * End Point Data Tx request to perform Tx Operation from End Point
 */
typedef struct uapp_data_tx_req {
    uint64_t socket_id;            /**< Socket Identifier for Close Command */
    int data_len;                 /**< Length of Data for Tx Request */
    uint8_t data[];              /**< Data Pointer for Tx request */
}uapp_data_tx_req_t;


/**
 * @struct uapp_data_tx_cfm_t
 * End Point Data Tx response from ADSP for Tx Operation from End Point
 */
typedef struct uapp_data_tx_cfm {
    uint64_t socket_id;          /**< Socket Identifier for Tx Cfm for End Point */
    uint8_t status;             /**< Status for Tx Request Operation */
}uapp_data_tx_cfm_t;




/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/

/**
 * @brief  Method to Handle Events for End Points and MicroApps registered with LPAI BT App Manager on AWM
 * The Method is invoked each time an event is received from ADSP for any of BT realted activites taking
 * place across the system.
 * @param[in]   dataBuff  Data Buffer with information related to End Points or MicroApps on AWM
 * @param[in]   len       Length of incoming data in the Data Buffer from ADSP
 * @param[out]  None
 * @return      None
 */
void lpai_bt_app_mgr_adsp_evt_handler(const uint8_t* dataBuff, uint16_t len);



/**
 * @brief Method for end points to send their response to ADSP.The client only needs to send application data which is intended
 * for the receiving entity on ADSP. The additional task of adding the appropriate header to the application data/response
 * is handled by the Bt App Manager.
 * Any memory if dynamically allocated will need to be freed by the Client Applcaition using the interface API.
 * Success is returned if the data was successfully sent over Glink.
 * In case of any failures , the retry decision lies with the Client Application
 * @param[in]   endPointId  Identity of the End Point Sending the Message
 * @param[in]   opcode      Message Opcode to identify the Message
 * @param[in]   dataLen     Length of the data to be send to ADSP
 * @param[in]   appDataBuf  Data Buffer containing the Application Data
 * @param[out]  None
 * @return      glink_err_type Return Type to indicate if the data was sent successfully , failure otherwise
 */
glink_err_type lpai_bt_appmgr_send_endpt_msg_adsp(uint64_t endPointId , uint16_t opcode , uint16_t dataLen , const void *appDataBuf, bool proto_enabled);




/**
 * @brief Method for LPAI BT App Manager to send back the response for an End Point Discovery Request from ADSP
 * The information sent is maintained by the LPAI BT App Manager for each end point when it is initially registered
 * with the BT App Manager
 * Success is returned if the data was successfully sent over Glink.
 * In case of any failures , the retry decision lies with the BT App Manager
 * @param[in]   dataLen     Length of the data to be send to ADSP
 * @param[in]   appDataBuf  Data Buffer containing the End Point Discovery Response
 * @param[out]  None
 * @return      glink_err_type Return Type to indicate if the data was sent successfully , failure otherwise
 */
glink_err_type lpai_bt_appmgr_send_endpt_discovery_response_adsp(uint16_t dataLen , const void *appDataBuf);


/**
 * @brief Method for Microapps on AWM to send their response to ADSP.The client only needs to send application data which is intended
 * for the receiving entity on ADSP. The additional task of adding the appropriate header to the application data/response
 * is handled by the Bt App Manager.
 * Any memory if dynamically allocated will need to be freed by the MicroApp using the interface API.
 * Success is returned if the data was successfully sent over Glink.
 * In case of any failures , the retry decision lies with the Microapp
 * @param[in]   opcode      Message Opcode to identify the Message
 * @param[in]   dataLen     Length of the data to be send to ADSP
 * @param[in]   message     Data Buffer containing the Application Data
 * @param[out]  None
 * @return      glink_err_type Return Type to indicate if the data was sent successfully , failure otherwise
 */
glink_err_type lpai_bt_appmgr_send_microapp_msg_adsp(uint16_t opcode , uint16_t dataLen , void *message);




#endif
