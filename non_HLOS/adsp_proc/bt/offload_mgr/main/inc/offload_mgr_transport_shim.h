/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_transport_shim.h
===========================================================================*/
/**
 * @file offload_mgr_transport_shim.h
 * @brief Header file for the transport shim layer.
 *
 * @details This file contains the definitions and declarations for the transport shim layer.
 *           Handles the mapping between transport APIs and Offload Manager events. 
 */

#ifndef OFFLOAD_MGR_TRANSPORT_SHIM_H
#define OFFLOAD_MGR_TRANSPORT_SHIM_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "lpai_transport.h"
#include "bt_pal_heap.h"
#include "bt_pal_assert.h"
#include <string.h>

/*===========================================================================
                      MACRO DEFINITIONS
===========================================================================*/
/**
 * @brief Length of the transport shim buffer.
 */
#define OFFLOAD_MGR_TRANSPORT_SHIM_BUF_LEN (8 * 512U)

/**
 * @brief Length of the transport shim header.
 */
#define OFFLOAD_MGR_TRANSPORT_SHIM_HDR_LEN  6U

/*===========================================================================
                      EXTERN DECLARATIONS
===========================================================================*/
/**
 * @brief Buffer for transport shim.
 */
extern uint8_t offload_mgr_transport_shim_buf[OFFLOAD_MGR_TRANSPORT_SHIM_BUF_LEN];

/**
 * @brief Length of the encoded buffer.
 */
extern size_t offload_mgr_transport_shim_encoded_buf_len;


/*===========================================================================
                    GLOBAL FUNCTION DECLARATIONS
===========================================================================*/
/*===========================================================================
FUNCTION      Offload_Mgr_SendDataToAppMgr
===========================================================================*/
/**
 * @brief Sends data to the Application Manager.
 *
 * @param[in] ptr Pointer to data, owned by application.
 *                transport will copy this data in its own buffers. 
 * @param[in] size Size of the data.
 * @return glink_err_type Return code from glink.
 * @sideeffects None.
 */
/*=========================================================================*/
static inline glink_err_type Offload_Mgr_SendDataToAppMgr(const void *ptr , size_t size) 
{
       return LPAI_Transport_Send_Data_On_Chnl(LPAI_TRANSPORT_AWM_LINK_IDX, 
                  LPAI_TRANSPORT_AWM_APP_MGR_CHNL_IDX, ptr, size);
}


/*===========================================================================
FUNCTION      Offload_Mgr_SendDataToBTSocketHal
===========================================================================*/
/**
 * @brief Sends data to the BT Socket HAL.
 *
 * @param[in] ptr Pointer to data, owned by application.
 *                transport will copy this data in its own buffers. 
 * @param[in] size Size of the data.
 * @return glink_err_type Return code from glink.
 * @sideeffects None.
 */
/*=========================================================================*/
static inline glink_err_type Offload_Mgr_SendDataToBTSocketHal(const void *ptr , size_t size) 
{
       return LPAI_Transport_Send_Data_On_Chnl(LPAI_TRANSPORT_HLOS_LINK_IDX, 
                  LPAI_TRANSPORT_HLOS_BT_SOCKET_CHNL_IDX, ptr, size);
}

/*===========================================================================
FUNCTION      Offload_Mgr_SendDataToBTGattHal
===========================================================================*/
/**
 * @brief Sends data to the BT GATT HAL.
 *
 * @param[in] ptr Pointer to data, owned by application.
 *                transport will copy this data in its own buffers. 
 * @param[in] size Size of the data.
 * @return glink_err_type Return code from glink.
 * @sideeffects None.
 */
/*=========================================================================*/
static inline glink_err_type Offload_Mgr_SendDataToBTGattHal(const void *ptr , size_t size) 
{
       return LPAI_Transport_Send_Data_On_Chnl(LPAI_TRANSPORT_HLOS_LINK_IDX, 
                  LPAI_TRANSPORT_HLOS_BT_GATT_CHNL_IDX, ptr, size);
}

/*===========================================================================
FUNCTION      Offload_Mgr_SendDataToContextHubHal
===========================================================================*/
/**
 * @brief Sends data to the Context Hub HAL.
 *
 * @param[in] ptr Pointer to data, owned by application.
 *                transport will copy this data in its own buffers. 
 * @param[in] size Size of the data.
 * @return glink_err_type Return code from glink.
 * @sideeffects None.
 */
/*=========================================================================*/
static inline glink_err_type Offload_Mgr_SendDataToContextHubHal(const void *ptr , size_t size) 
{
       return LPAI_Transport_Send_Data_On_Chnl(LPAI_TRANSPORT_HLOS_LINK_IDX, 
                  LPAI_TRANSPORT_HLOS_CONTEXT_HUB_CHNL_IDX, ptr, size);
}

/*===========================================================================
FUNCTION      Offload_Mgr_SendDataToHci
===========================================================================*/
/**
 * @brief Sends data to the HCI transport.
 *
 * @param[in] ptr Pointer to data, owned by application.
 *                transport will copy this data in its own buffers. 
 * @param[in] size Size of the data.
 * @return glink_err_type Return code from glink.
 * @sideeffects None.
 */
/*=========================================================================*/
static inline glink_err_type Offload_Mgr_SendDataToHCI(const void *ptr , size_t size) 

{
       return LPAI_Transport_Send_Data_On_Chnl(LPAI_TRANSPORT_HLOS_LINK_IDX, 
                  LPAI_TRANSPORT_HLOS_HCI_CHNL_IDX, ptr, size);
}

/*===========================================================================
FUNCTION      Offload_Mgr_Post_Transport_Event
===========================================================================*/
/**
 * @brief Posts a transport event to the Offload Manager.
 *
 * @param[in] link_id Link ID.
 * @param[in] channel_id Channel ID.
 * @param[in] blob Pointer to the data blob.
 * @param[in] len Length of the data blob.
 */
/*=========================================================================*/
void Offload_Mgr_Post_Transport_Event(uint8_t link_id, uint8_t channel_id, const uint8_t * blob, size_t len);

/*===========================================================================
FUNCTION      Offload_Mgr_Post_Microstack_Evt
===========================================================================*/
/**
 * @brief Posts a Microstack event to the Offload Manager.
 *
 * @param[in] blob Pointer to the data blob.
 * @param[in] len Length of the data blob.
 */
void Offload_Mgr_Post_Microstack_Evt(uint8_t *blob, uint16_t len);

/*===========================================================================
FUNCTION      BT_Offload_HciArbiter_Init
===========================================================================*/
/**
 * @brief Initializes the HCI Arbiter.
 */
void BT_Offload_HciArbiter_Init(void);

/*===========================================================================
FUNCTION      BT_Offload_HciArbiter_DeInit
===========================================================================*/
/**
 * @brief De-Initializes the HCI Arbiter.
 */
void BT_Offload_HciArbiter_DeInit(void);


/*===========================================================================
FUNCTION    Offload_Mgr_Update_Header
===========================================================================*/
/**
 * @brief Updates the header in the output buffer.
 *
 * @param[out]   out_buf    Pointer to the output buffer.
 * @param[in]    opcode     Opcode to be included in the header.
 * @param[in]    length     Length of the data to be included in the header.
 * @return     void.
 * @sideeffects  None.
 */
void Offload_Mgr_Update_Header(uint8_t *out_buf, uint16_t opcode, uint16_t length);
#endif /* OFFLOAD_MGR_TRANSPORT_SHIM_H */
