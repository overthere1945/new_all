/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      socket.h
===========================================================================*/
/**
 * @file socket.h
 * @brief This file has the primary socket code.
 */

#ifndef SOCKET_H
#define SOCKET_H

/*===========================================================================
                            INCLUDE FILES
===========================================================================*/
#include "bt_pal_heap.h"
#include "bt_pal_assert.h"
#include "profile_mgr_lecoc.h"
#include "profile_mgr_rfcomm.h"
#include "socket_mgr.pb.h"

/*===========================================================================
                            TYPE DEFINITIONS
===========================================================================*/
typedef uint64_t socket_id_t;

typedef enum
{
  SOCKET_TYPE_LECOC = google_offload_proto_Protocol_LE_COC,
  SOCKET_TYPE_RFCOMM = google_offload_proto_Protocol_RFCOMM,
} socket_type_t;

typedef union {
    lecoc_data_t lecoc_data;   /**< LE COC data */
    rfcomm_data_t rfcomm_data; /**< RFCOMM data */
}profile_data_t;

typedef struct socket_data
{
  socket_id_t socket_id;          /**< Socket ID */
  uint32_t pseudo_id;             /**< Pseudo identifier for internal use */
  socket_type_t socket_type;      /**< Socket type */
  profile_data_t profile_data;   /**< Profile data */
}socket_data_t;

#endif
