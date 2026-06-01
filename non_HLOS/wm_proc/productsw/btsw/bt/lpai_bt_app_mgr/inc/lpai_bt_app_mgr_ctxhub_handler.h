/**************************************************************************
 * @file     lpai_bt_app_mgr_ctxhub_handler.h
 * @brief    LPAI BT APP Manager ADSP Interface Handler File.
 * 			 This file contains all the type declarations and opcodes for messages
 * 			 exhanges between AWM and Context Hub HAL on APSS
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_APP_MGR_CTXHUB_HANDLER_H
#define LPAI_BT_APP_MGR_CTXHUB_HANDLER_H


/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "stdint.h"
#include "stdbool.h"
#include "glink.h"


/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

/**
 * @enum ctx_hub_msgs_t
 * Message Opcodes for Messages exchanged between End Points on AWM and Context Hub HAL
 * on APPS for context information exchange
 */
typedef enum {
    CTXHUB_ENDPOINT_OPEN_SESSION_REQ = 0x00C2,
    CTXHUB_ENDPOINT_OPEN_SESSION_COMPLETE_CB,
    CTXHUB_ENDPOINT_CLOSE_SESSION_CB,
    CTXHUB_ENDPOINT_CLOSE_SESSION_REQ,
    CTXHUB_ENDPOINT_SEND_MSG_REQ,
    CTXHUB_ENDPOINT_MSG_RECEIVED_CB,
    CTXHUB_ENDPOINT_SEND_MSG_DELIVERY_STATUS_REQ,
    CTXHUB_ENDPOINT_MSG_DELIVERY_STATUS_RECEIVED_CB,
    CTXHUB_ENDPOINT_STARTED,
    CTXHUB_ENDPOINT_STOPPED,
}ctx_hub_msgs_t;


/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/

/**
 * @brief  Method to Handle Events for End Points registered with LPAI BT App Manager on AWM
 * The Method is invoked each time an event is received from APSS for exchange of context information
 * between end points on AWM and Context Hub HAL on APSS
 * @param[in]   dataBuff  Data Buffer with information related to End Points on AWM
 * @param[in]   len       Length of incoming data in the Data Buffer from APSS
 * @param[out]  None
 * @return      None
 */
void lpai_bt_app_mgr_ctxhub_evt_handler(const uint8_t* dataBuff , uint16_t len);



/**
 * @brief Method for end points to send their response to APSS.The client only needs to send application data which is intended
 * for the receiving entity on APSS. The additional task of adding the appropriate header to the application data/response
 * is handled by the Bt App Manager.
 * Any memory if dynamically allocated will need to be freed by the Client Applcaition using the interface API.
 * Success is returned if the data was successfully sent over Glink.
 * In case of any failures , the retry decision lies with the Client Application
 * @param[in]   endPointId  Identity of the End Point Sending the Message
 * @param[in]   opcode      Message Opcode to identify the Message
 * @param[in]   dataLen     Length of the data to be send to APSS
 * @param[in]   appDataBuf  Data Buffer containing the Application Data
 * @param[out]  None
 * @return      glink_err_type Return Type to indicate if the data was sent successfully , failure otherwise
 */
glink_err_type lpai_bt_appmgr_send_endpt_msg_apss(uint64_t endPointId , uint16_t opcode , uint16_t dataLen , const void *appDataBuf);



#endif /* LPAI_BT_APP_MGR_CTXHUB_HANDLER_H */
