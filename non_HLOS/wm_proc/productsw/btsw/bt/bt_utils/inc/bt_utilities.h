/**************************************************************************
 * @file     bt_utilities.h
 * @brief    LPAI BT Utilities header file.
 * 
 * This file contains the declarations for all the proto encoder/decoder helper functions
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef BT_UTILITIES_H
#define BT_UTILITIES_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/

#include <pb_common.h>
#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>

/**
 * @enum bt_utils_cmds_t
 * LE ADV Commands and Response for Communication between AAWM adn ADSP
 */
typedef enum{
    BT_UTILS_TIME_SYNC = 0x7000,
    BT_UTILS_ENTER_ISLAND,
    BT_UTILS_EXIT_ISLAND,
	BT_UTILS_TIME_SYNC_RSP = 0x7200,
   
}bt_utils_cmds_t;

/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
bool encode_bytes(pb_ostream_t * stream, const pb_field_t * field, void * const * arg);

bool decode_bytes(pb_istream_t * stream, const pb_field_t * field, void ** arg);

bool encode_string(pb_ostream_t * stream, const pb_field_t * field, void * const * arg);

bool decode_string(pb_istream_t *stream, const pb_field_t *field, void **arg);

void bt_log_byte_stream(uint8_t * str, uint16_t len);

#endif /**BT_UTILITIES_H*/