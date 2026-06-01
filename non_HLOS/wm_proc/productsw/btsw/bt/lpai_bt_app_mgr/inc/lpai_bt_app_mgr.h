/**************************************************************************
 * @file     lpai_bt_app_mgr.h
 * @brief    LPAI BT APP Manager header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_APP_MGR_H
#define LPAI_BT_APP_MGR_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "glink.h"


#ifdef __cplusplus
extern "C" {
#endif
/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/
/**
 * @def LPAI_BT_APP_MGR_THREAD_PRIORITY
 * @brief Defines the Thread Priority for LPAI BT APP manager.
 *
 * This macro sets the thread priority for LPAI BT App Manager in the System.
 */
#define LPAI_BT_APP_MGR_THREAD_PRIORITY 0x16

/**
 * @def LPAI_BT_APP_MGR_THREAD_STACKS_SIZE
 * @brief Defines the Stack Size for LPAI Bt App Manager Thread.
 *
 * This macro sets the thread stack size for LPAI BT App Manager Thread in the System.
 */
#define LPAI_BT_APP_MGR_THREAD_STACKS_SIZE 1536


/**
 * @def LPAI_BT_APP_MGR_MSGQ_SIZE
 * @brief Defines the Message Queue Size for Bt App Manager.
 *
 * This macro sets the maximum messages which can be stored by Lpai Bt App Manager Message Queue .
 */
#define LPAI_BT_APP_MGR_MSGQ_SIZE  15


/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

/**
 * @enum appMgrMsgId_t
 * Event identifiers for Events Sent and Received in Lpai Bt App Manager Message Queue
 */
typedef enum{
    LPAI_BT_APP_MGR_ADSP_RX,
    LPAI_BT_APP_MGR_APSS_RX,
    LPAI_BT_APP_MGR_AWM_INTERNAL_EVT,
}appMgrMsgId_t;

/**
 * @struct appMgrMsgFormat_t
 * Structure to Process Events while posting the data to Bt App Manager Message Queue and Processing it in the 
 * Thread Handler Function
 */
typedef struct appMgrMsgFormat{
    uint16_t msgType;
    uint16_t msgSize;
    const void *msgPtr;
}appMgrMsgFormat_t;




/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/


/**
 * @brief Method to initialize the appropriate LPAI BT APP Manager. This can contain additional initialization
 * sequnces for end points which involves procedures for initialization of their internal structures as well as registration with BT App Manager
 * @param[in]  None
 * @param[out] None
 * @return     None
 */
void lpai_bt_app_mgr_init();


/**
 * @brief Method to initialize the appropriate LPAI BT APP Manager Context Structure
 * @param[in]  None
 * @param[out] None
 * @return     None
 */
void lpai_bt_mgr_ctx_init();

/**
 * @brief  Thread Handler Function for BT APP Manager
 * @param[in]   param1  Context Parameter 1 to Thread Handler Function
 * @param[in]   param2  Context Parameter 1 to Thread Handler Function
 * @param[in]   param3  Context Parameter 1 to Thread Handler Function
 * @param[out]  None
 * @return      None
 */
void lpai_bt_app_mgr_thread_fn(void *param1 , void *param2 , void *param3);



/**
 * @brief  Method to Post an Event with Data to Lpai Bt App Manager Message Queue
 * @param[in]   eventId  Event Identifier to Data Item to be sent to Queue
 * @param[in]   size     Size of Data Item
 * @param[in]   data     Pointer to Actual Data
 * @param[out]  None
 * @return      int   Result of Putting an Message to a Message Queue
 */
int lpai_bt_app_mgr_send_dataEvent(uint16_t eventId , uint16_t size , void *data);



/**
 * @brief  Method to Post an Event to Lpai Bt App Manager Message Queue
 * @param[in]   eventId  Event Identifier to Data Item to be sent to Queue
 * @return      int   Result of Putting an Message to a Message Queue
 */
int lpai_bt_app_mgr_send_event(uint16_t eventId);

#ifdef __cplusplus
}
#endif

#endif /*LPAI_BT_APP_MGR_H */