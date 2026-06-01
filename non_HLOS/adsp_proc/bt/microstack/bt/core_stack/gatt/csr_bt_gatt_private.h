#ifndef _CSR_BT_GATT_PRIVATE_H_
#define _CSR_BT_GATT_PRIVATE_H_
/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_message_queue.h"
#include "csr_log_text_2.h"
#include "csr_bt_util.h"
#include "csr_bt_tasks.h"
#include "att_prim.h"
#include "attlib.h"
#include "tbdaddr.h"
#include "l2cap_prim.h"
#include "l2caplib.h"

#include "csr_bt_gatt_config.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_utils.h"
#include "csr_bt_gatt_private_prim.h"
#include "csr_bt_uuids.h"
#include "csr_list.h"
#include "csr_bt_gatt_main.h"

#include "csr_bt_gatt_private_utils.h"

#include "csr_bt_gatt_sef.h"
#include "csr_bt_gatt_att_sef.h"
#include "csr_bt_gatt_sef.h"
#include "csr_bt_gatt_upstream.h"


#ifdef GATT_DATA_LOGGER
#include "gatt_data_logger_prim.h"
#endif

#ifdef GATT_OFFLOAD
#include "gatt_offload.h"
#include "socket_offload.h"
#endif

/* Maximum number of connections supported by connection manager is 5 */
#define GATT_MAX_CONNECTIONS 5

#define SERVICE_CHANGED_INDICATION_IDLE      0x00
#define SERVICE_CHANGED_INDICATION_SERVED    0x01
#define SERVICE_CHANGED_INDICATION_DISABLED  0x02

#ifdef __cplusplus
extern "C" {
#endif

/* Empty */

#ifdef __cplusplus
}
#endif

#endif
