/*************************************************************************
 * @file     lpai_bt_app_mgr_ctxhub_handler.c
 * @brief    LPAI BT App Manager Context Hub Handler source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <zephyr/sys/printk.h>
#include "lpai_bt_app_mgr_ctxhub_handler.h"
#include "lpai_bt_app_mgr_client_interface.h"
#include "lpai_bt_app_mgr.h"
#include "lpai_bt_app_mgr_adsp_handler.h"



/*===========================================================================
                        EXTERNAL DATA DECLARATIONS
===========================================================================*/
extern appMgrContext_t appMgrCtx;



/*===========================================================================
                        PUBLIC/GLOBAL DATA DEFINITIONS
===========================================================================*/


/*===========================================================================
                        FUNCTION DEFINITIONS
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
void lpai_bt_app_mgr_ctxhub_evt_handler(const uint8_t* dataBuff , uint16_t len)
{
    /**Todo : Add Handling Here for Context Hub Information Message Handling:  */
    if(dataBuff == NULL)
    {
        printk("No Handling to be performed as data is not available");
    }
    else
    {
        uint16_t event = dataBuff[0] | (dataBuff[1] << 8);
        uint64_t endPointId = 0xAA;
        switch(event)
        {
            case CTXHUB_ENDPOINT_OPEN_SESSION_REQ:
            {
                
            }
            break;

            case CTXHUB_ENDPOINT_CLOSE_SESSION_REQ:
            {
                
            }
            break;

            case CTXHUB_ENDPOINT_SEND_MSG_REQ:
            {
                
            }
            break;

            case UAPP_DATA_RX_IND:
            {
                
            }
            break;
        }

    }
}



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
glink_err_type lpai_bt_appmgr_send_endpt_msg_apss(uint64_t endPointId , uint16_t opcode , uint16_t dataLen , const void *appDataBuf)
{
    /**Todo : Add Handling Here for Context Hub Information Exchange Message Exchange :  */
    glink_err_type result = GLINK_STATUS_FAILURE;
    return result;

}
