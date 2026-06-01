/**************************************************************************
 * @file     lpai_bt_hal_mgr.h
 * @brief    LPAI BT HAL Manager header file.
 *           Contains Interface for Encoding Decoding Messages Intended from APSS and to be sent to APSS
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_HAL_MGR_H
#define LPAI_BT_HAL_MGR_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "pb.h"
#include <stdint.h>
#include <stdbool.h>
#include "lpai_bt_app_mgr_client_interface.h"


/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/
/**
 * @struct lpai_proto_out_buff
 * Structure to Store Information about protocol buffer which stores information of encoding deatils 
 */
typedef struct{
    uint8_t *out_buff;   /**< Pointer to Output Buffer for Storing Encoded String */
    uint16_t size;      /**< Size of the Encoded Buffer After Encoding */
}lpai_proto_out_buff;


/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
/**
 * @brief Encodes the End Point Discovery Response to be Sent from AWM to ADSP
 *        when it is requested from APSS .
 * @param[in]  out_buff  Pointer to Output Buffer where encoded string is stored after encoding is performed
 * @param[in]  appMgrCtx Pointer to App Manager Context Structure which store information related to end points
 * @param[out] out_buff  Output Buffer Pointer which stores encoded string
 * @return     bool      true if Encoding is successful, false otherwise
 */
bool encode_end_point_discovery_rsp(lpai_proto_out_buff *out_buff , appMgrContext_t *appMgrCtx);


/**
 * @brief Encodes the End Point Details for each end point during end point discovery Response.
 * @param[in]  stream  Protocol Buffer Output Stream
 * @param[in]  field   Protocol Buffer Fields requied for encoding particular structures
 * @param[in]  arg     Pointer to argument passed for a function which encodes callback fields
 * @return     bool    true if Encoding is successful, false otherwise
 */
bool encode_end_point_details(pb_ostream_t * stream, const pb_field_t * field, void * const * arg );


#endif /**LPAI_BT_HAL_MGR_H */