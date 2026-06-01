/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_proto_utils.h
===========================================================================*/
/**
 * @file offload_mgr_proto_utils.h
 * @brief Common proto utilities used by the offload framework.
 *
 * @details This file contains utility functions and definitions that are commonly used 
 *          within the offload framework for handling proto messages.
 */

#ifndef OFFLOAD_MGR_PROTO_UTILS_H
#define OFFLOAD_MGR_PROTO_UTILS_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "offload_mgr_transport_shim.h"
#include "stdint.h"
#include "offload_mgr_log.h"
#include "pb.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"

/*===========================================================================
                        MACRO DEFINITIONS
===========================================================================*/
#define LE_FIRST_BYTE(x) ((x) & 0xFF)
#define LE_SECOND_BYTE(x) ((x >> 8) & 0xFF)


/*===========================================================================
                        PUBLIC FUNCTION DECLARATIONS
===========================================================================*/
/*===========================================================================
FUNCTION      encode__bytes
===========================================================================*/
/**
 * @brief Encodes bytes for proto messages.
 *
 * @param[in,out] stream    Pointer to the output stream.
 * @param[in]     field     Pointer to the field descriptor.
 * @param[in]     arg       Pointer to the argument containing the bytes to encode.
 * @return       true if encoding is successful, false otherwise.
 * @sideeffects  None.
 */
bool encode__bytes(pb_ostream_t * stream, const pb_field_t * field, void * const * arg);

/*===========================================================================
FUNCTION      decode__bytes
===========================================================================*/
/**
 * @brief Decodes bytes from proto messages.
 *
 * @param[in,out] stream    Pointer to the input stream.
 * @param[in]     field     Pointer to the field descriptor.
 * @param[out]    arg       Pointer to the argument where the decoded bytes will be stored.
 * @return       true if decoding is successful, false otherwise.
 * @sideeffects  None.
 */
bool decode__bytes(pb_istream_t * stream, const pb_field_t * field, void ** arg);



#endif /* OFFLOAD_MGR_PROTO_UTILS_H */
