/*==============================================================================*/
/*
* Copyright (c) 2024 - 2026 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      lpai_transport.h
===========================================================================*/
/**
 * @file lpai_transport.h
 * @brief Public Header File used by Lpai Glink Transport module.
 *
 * @details 
 * This file contains the public APIs used by the Lpai Glink Transport module.
 *
 * @note 
 * 1. The LPAI_TRANSPORT_THREAD_ENABLED macro makes the Lpai transport run through 
 *    a separate thread. When disabled, lpai_transport is serviced by the offload_manager
 *    thread and will not execute from a separate thread.
 */

#ifndef _LPAI_TRANSPORT_H_
#define _LPAI_TRANSPORT_H_


/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "glink.h"
#include "qurt_thread.h"
#include "qurt_signal.h"
#include <stdbool.h>
#include <stdint.h>
#include "lpai_transport_log.h"


/** enable this macro to use transport from separate thread */
// #define LPAI_TRANSPORT_THREAD_ENABLED 

/*===========================================================================
                        MACRO DEFINITIONS
===========================================================================*/
#define LPAI_TRANSPORT_THREAD_STACK_SIZE        1024
#define LPAI_TRANSPORT_THREAD_STACK_PRIORITY    62


/*===========================================================================
                      EXTERN VARS AND APIS
===========================================================================*/
extern const char *lpai_transport_thread_name;

/*===========================================================================
                    ENUM DEFINITIONS
===========================================================================*/

/**
 * @brief Enum for transport link indices.
 */
typedef enum {
  LPAI_TRANSPORT_AWM_LINK_IDX = 0, /**< AWM link index */
  LPAI_TRANSPORT_HLOS_LINK_IDX,    /**< HLOS link index */
  LPAI_TRANSPORT_LINKS_MAX         /**< Maximum number of links */
} LPAI_TRANSPORT_LINKS_NUM;

/**
* @brief Enum for AWM channel indices.
*/
typedef enum {
  LPAI_TRANSPORT_AWM_APP_MGR_CHNL_IDX = 0, /**< AWM application manager channel index */
  LPAI_TRANSPORT_AWM_GLINK_CHNLS_MAX       /**< Maximum number of AWM GLink channels */
} LPAI_TRANSPORT_AWM_CHNL_IDXS;

/**
* @brief Enum for HLOS channel indices.
*/
typedef enum {
  LPAI_TRANSPORT_HLOS_BT_SOCKET_CHNL_IDX = 0, /**< HLOS Bluetooth socket channel index */
  LPAI_TRANSPORT_HLOS_CONTEXT_HUB_CHNL_IDX,   /**< HLOS context hub channel index */
  LPAI_TRANSPORT_HLOS_HCI_CHNL_IDX,           /**< HLOS HCI channel index */
  LPAI_TRANSPORT_HLOS_BT_GATT_CHNL_IDX,       /**< HLOS Bluetooth GATT channel index */
#ifdef ENABLE_WLAN_TSF_GPIO
  LPAI_TRANSPORT_HLOS_WLAN_GPIO_CHNL_IDX,     /**< HLOS WLAN GPIO channel index */
#endif
  LPAI_TRANSPORT_HLOS_GLINK_CHNLS_MAX         /**< Maximum number of HLOS GLink channels */
} LPAI_TRANSPORT_HLOS_CHNL_IDXS;

/*===========================================================================
                    GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
FUNCTION      LPAI_Transport_Init
===========================================================================*/
/**
  function to init the memory and data structures required for Lpai Transport
  @param[in]    None   
  @return       None.
*/
void LPAI_Transport_Init(void);

/*===========================================================================
FUNCTION      LPAI_Transport_Start
===========================================================================*/
/**
  Function to start the transport thread
  @param[in]      *thread_name        Thread Name
  @param[in]      thread_priority     Thread Priority
  @param[in]      thread_stack_size   Thread Stack Size 
  @return         qurt_thread_t       Thread Id of the spawed thread
*/
qurt_thread_t LPAI_Transport_Start( char *thread_name,
       const int thread_priority,
       const int thread_stack_size);


bool LPAI_Transport_Disable (qurt_signal_t* signal);

/*===========================================================================
FUNCTION      LPAI_Transport_Start
===========================================================================*/
/**
  Function to send data on particular link and channel
  @param[in]      link_idx         Link Index
  @param[in]      chnl_idx         Channel index
  @param[in]      ptr              Pointer to data 
  @return         size             Size of the data
  @sideeffects  None.
*/
glink_err_type LPAI_Transport_Send_Data_On_Chnl(uint8_t link_idx, uint8_t chnl_idx,
                                                const void *ptr , size_t size);

/*===========================================================================
FUNCTION      LPAI_Transport_Handle_Event
===========================================================================*/
/**
  Function to send data on particular link and channel
  @param[in]      data        data ptr
  @return         None
*/
void LPAI_Transport_Handle_Event(void *data);

#endif /* _LPAI_TRANSPORT_H_ */
