/*************************************************************************
 * @file     lpai_bt_app_mgr.c
 * @brief    LPAI BT App Manager Main source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include "lpai_bt_app_mgr_client_interface.h" 
#include "lpai_transport.h"
#include "lpai_bt_app_mgr.h"
#include "lpai_bt_app_mgr_rpc.h"


/*===========================================================================
                        PUBLIC/GLOBAL THREAD AND QUEUE DEFINITIONS
===========================================================================*/

K_MSGQ_DEFINE(lpai_bt_app_mgr_msgq , sizeof(appMgrMsgFormat_t),LPAI_BT_APP_MGR_MSGQ_SIZE,1);

K_THREAD_DEFINE(lpai_bt_app_mgr_thread_id , LPAI_BT_APP_MGR_THREAD_STACKS_SIZE,
    lpai_bt_app_mgr_thread_fn,NULL,NULL,NULL,
    LPAI_BT_APP_MGR_THREAD_PRIORITY,0,0);


/*===========================================================================
                        PUBLIC/GLOBAL DATA DEFINITIONS
===========================================================================*/
appMgrContext_t appMgrCtx;



/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/


/**
 * @brief Method to initialize the appropriate LPAI BT APP Manager. This can contain additional initialization
 * sequnces for end points which involves procedures for initialization of their internal structures as well as registration with BT App Manager
 * @param[in]  None
 * @param[out] None
 * @return     None
 */
void lpai_bt_app_mgr_init()
{
    
    lpai_bt_mgr_ctx_init();
    
    #ifdef CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER
        extern void register_dummy_app();
        register_dummy_app();
	    extern void start_ancs_app();
    	start_ancs_app();
    #endif

	extern void lecoc_app_init();
    lecoc_app_init();
    extern void rfcomm_app_init();
    rfcomm_app_init();

	/**Starting BT Utils App Initialization and Registration */
	extern void bt_utils_app_init();
	bt_utils_app_init();

}


/**
 * @brief Method to initialize the appropriate LPAI BT APP Manager Context Structure
 * @param[in]  None
 * @param[out] None
 * @return     None
 */
void lpai_bt_mgr_ctx_init()
{
    memset(&appMgrCtx,0,sizeof(appMgrContext_t));
    appMgrCtx.btStatus = BT_STATUS_OFF;
}

/**
 * @brief  Method to Post an Event with Data to Lpai Bt App Manager Message Queue
 * @param[in]   eventId  Event Identifier to Data Item to be sent to Queue
 * @param[in]   size     Size of Data Item
 * @param[in]   data     Pointer to Actual Data
 * @param[out]  None
 * @return      int   Result of Putting an Message to a Message Queue
 */
int lpai_bt_app_mgr_send_dataEvent(uint16_t eventId , uint16_t size , void *data)
{
    appMgrMsgFormat_t lpaiBtAppMsg = {eventId,size,data};

   // return k_msgq_put(&(lpai_app_mgr_main.lpai_bt_app_mgr_msgq),&lpaiBtAppMsg,K_NO_WAIT);
   /**Debug Code */
   return k_msgq_put(&lpai_bt_app_mgr_msgq,&lpaiBtAppMsg,K_NO_WAIT);
}

/**
 * @brief  Method to Post an Event to Lpai Bt App Manager Message Queue
 * @param[in]   eventId  Event Identifier to Data Item to be sent to Queue
 * @return      int   Result of Putting an Message to a Message Queue
 */
int lpai_bt_app_mgr_send_event(uint16_t eventId)
{
    appMgrMsgFormat_t lpaiBtAppMsg = {eventId,sizeof(eventId),NULL};

   return k_msgq_put(&lpai_bt_app_mgr_msgq,&lpaiBtAppMsg,K_NO_WAIT);

}



/**
 * @brief  Thread Handler Function for BT APP Manager
 * @param[in]   param1  Context Parameter 1 to Thread Handler Function
 * @param[in]   param2  Context Parameter 1 to Thread Handler Function
 * @param[in]   param3  Context Parameter 1 to Thread Handler Function
 * @param[out]  None
 * @return      None
 */
void lpai_bt_app_mgr_thread_fn(void *param1 , void *param2 , void *param3)
{
    appMgrMsgFormat_t appMgrMsg;
    lpai_bt_app_mgr_init();
    while(1)
    {
        k_msgq_get(&lpai_bt_app_mgr_msgq, &appMgrMsg,  K_FOREVER);

        switch(appMgrMsg.msgType)
        {
            case LPAI_BT_APP_MGR_ADSP_RX:
            {
                app_mgr_rpc_adsp_evt_handler(appMgrMsg.msgPtr,appMgrMsg.msgSize);
                free(appMgrMsg.msgPtr);
                appMgrMsg.msgPtr = NULL;
            }
            break;

            case LPAI_BT_APP_MGR_APSS_RX:
            {
                lpai_bt_app_mgr_ctxhub_evt_handler(appMgrMsg.msgPtr,appMgrMsg.msgSize);
                free(appMgrMsg.msgPtr);
                appMgrMsg.msgPtr = NULL;
            }
            break;

            case LPAI_BT_APP_MGR_AWM_INTERNAL_EVT:
            {
                free(appMgrMsg.msgPtr);
                appMgrMsg.msgPtr = NULL;
            }
            break;

            default:
                break;
        }

        /**Free Up any Memory if it requires to be freed */

    }

}




























