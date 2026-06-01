/*!
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
        All rights reserved

\file   ext_adv_manager.h

\brief  Extended Scanning (ES)
*/

#ifndef _EXT_SCAN_MANAGER_H_
#define _EXT_SCAN_MANAGER_H_
#include "ms_adv_scan_prim.h"
#include "csr_message_queue.h"
#include "dm_private.h"
#include "bluetooth.h"
#include "csr_gsched.h"
#include "csr_bt_usr_config.h"
#include "hci.h"
#include "csr_pmem_common.h"
#include "csr_msg_transport.h"
#include "dm_hci_interface.h"
#include "ext_adv_manager.h"
#include "csr_bt_config_microstack.h"
/* The following defines say how the filter duplicates will work. The controller's
   DID cache will be enabled when set ON or a period set.  Setting a scan period will
   mean the DID cache will be flushed at the start of each scan period. */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_OFF HCI_ULP_FILTER_DUPLICATES_DISABLED
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_ON HCI_ULP_FILTER_DUPLICATES_ENABLED
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_1SEC_PERIOD 2   /* 1.28 seconds   */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_2SEC_PERIOD 3   /* 2.56 seconds   */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_5SEC_PERIOD 4   /* 5.12 seconds   */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_10SEC_PERIOD 5  /* 10.24 seconds  */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_30SEC_PERIOD 6  /* 30.72 seconds  */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_1MIN_PERIOD 7   /* 60.16 seconds  */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_2MIN_PERIOD 8   /* 120.32 seconds */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_5MIN_PERIOD 9   /* 300.80 seconds */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_10MIN_PERIOD 10 /* 600.32 seconds */

/* Used for single scan (Duration non-zero but Period zero).

   Period parameter in the HCI_ULP_EXT_SCAN_ENABLE command will be set to zero.
   Only the duration parameter is considered from the above 
   configurations (DM_ULP_EXT_SCAN_FILTER_DUPLICATE_xxx).

   Note: 1) Shall be OR'ed with one of the DM_ULP_EXT_SCAN_FILTER_DUPLICATE_xxx value
            greater than DM_ULP_EXT_SCAN_FILTER_DUPLICATE_ON (non-zero duration).
         2) DM_ULP_EXT_SCAN_TIMEOUT_IND shall be sent at the end of
            single scan in the controller.
         3) Disables all the other scanners if active and DM_ULP_EXT_SCAN_CTRL_SCAN_INFO_IND 
            shall be sent.
         4) Filter duplicate shall be enabled.

   Example: To perform single scan for a duration of 5.12 seconds. Set,
            filter_duplicates = ( DM_ULP_EXT_SCAN_FILTER_DUPLICATE_5SEC_PERIOD |
                                  DM_ULP_EXT_SCAN_FILTER_DUPLICATE_MASK_PERIOD)
   */
#define DM_ULP_EXT_SCAN_FILTER_DUPLICATE_MASK_PERIOD 0x80

#define DM_ULP_EXT_SCAN_MAX_REG_AD_TYPES 10

/* AD Structure flags filter part of flags field (bits 0-1) */
#define DM_ULP_EXT_SCAN_AD_FLAGS_MASK        3 /* Flags AD Structure mask */

#define DM_ULP_EXT_SCAN_AD_FLAGS_NO_FILTER   0 /* Flags AD Structure filter disabled */
#define DM_ULP_EXT_SCAN_AD_FLAGS_PRESENT     1 /* Only report if flags AD type present */
#define DM_ULP_EXT_SCAN_AD_FLAGS_GEN_AND_LIM 2 /* Only report if flags general or limited bit set */
#define DM_ULP_EXT_SCAN_AD_FLAGS_LIM         3 /* Only report if flags limited bit set */

/* Valid values for adv_filter field */
#define DM_ULP_EXT_SCAN_ADV_FILTER_NONE 0
#define DM_ULP_EXT_SCAN_ADV_FILTER_BLOCK_ALL 1
#define DM_ULP_EXT_SCAN_ADV_FILTER_LEGACY 2
#define DM_ULP_EXT_SCAN_ADV_FILTER_EXTENDED 3
#define DM_ULP_EXT_SCAN_ADV_FILTER_ASSOCIATED_PERIODIC 4

/* Valid values for ad_structure_filter field */
#define DM_ULP_EXT_SCAN_AD_STRUCT_FILTER_NONE 0

/* Sub fields setting for adv_filter or ad_structure_filter when not used */
#define DM_ULP_EXT_SCAN_SUB_FIELD_INVALID 0

/* Used to calculate number of dynamic array pointers in field ad_structure_info.
   It is declared here rather than hci.h as ad_structure_info in DM_ULP_EXT_SCAN_REGISTER_SCANNER_REQ
   and DM_ULP_PERIODIC_SCAN_START_FIND_TRAINS_REQ does not go to controller */
#define DM_ULP_AD_STRUCT_INFO_LENGTH         255
#define DM_ULP_AD_STRUCT_INFO_BYTES_PER_PTR  HCI_VAR_ARG_POOL_SIZE
#define DM_ULP_AD_STRUCT_INFO_BYTE_PTRS      ((DM_ULP_AD_STRUCT_INFO_LENGTH + DM_ULP_AD_STRUCT_INFO_BYTES_PER_PTR -1) / DM_ULP_AD_STRUCT_INFO_BYTES_PER_PTR)




#define MS_EXT_SCAN_HANDLE_INVALID 0xFF





#define DM_ULP_EXT_SCAN_MAX_SCANNERS 5
#define DM_ULP_EXT_SCAN_FOREVER 0
#define DM_ULP_MAX_EXT_SCAN_DURATION 3600 /* seconds */





/*----------------------------------------------------------------------------*
 * Bit definitions used for extended advertising reports adv_data_info field
 *----------------------------------------------------------------------------*/
/* AD Structures chain validity (Bits 0-1 in adv_data_info field) */
#define DM_ULP_EXT_SCAN_AD_STRUCTS_MASK      3 /* AD Structure validity mask */

#define DM_ULP_EXT_SCAN_AD_STRUCTS_OK        0 /* Valid chain of AD Structures */
#define DM_ULP_EXT_SCAN_AD_STRUCTS_ZERO_LEN  1 /* AD structure chain was terminated with
                                                  a 0 length AD Structure */
#define DM_ULP_EXT_SCAN_AD_STRUCTS_LEN_ERROR 2 /* AD Structure length check failed */

/* Flags AD Structure present (Bit 7 in adv_data_info field) */
#define DM_ULP_EXT_SCAN_AD_FLAGS_IS_PRESENT  0x80 /* If set ad_flags field is valid */




typedef struct
{
    dm_prim_t type;
    phandle_t phandle;

    /* The presence of 1 or more scan_handles indicates that the advertising report
       matches 1 or more scanner's filters */
    uint8_t num_of_scan_handles;
    uint8_t scan_handles[DM_ULP_EXT_SCAN_MAX_SCANNERS];

    /* Refer to BLUETOOTH CORE SPECIFICATION Version 5.2: LE Extended Advertising Report
       event */
    uint16_t event_type;
    uint16_t primary_phy;
    uint16_t secondary_phy;
    uint8_t adv_sid;
    uint8_t current_addr_type;
    uint8_t permanent_addr_type;
    uint8_t direct_addr_type;
    BD_ADDR_T current_addr;
    BD_ADDR_T permanent_addr;
    BD_ADDR_T direct_addr;
    int8_t tx_power;
    int8_t rssi;
    uint16_t periodic_adv_interval;

    uint8_t adv_data_info;      /* Refer to adv_data_info field defines above */
    uint8_t ad_flags;           /* The flags from Flags AD Structure */

    /* Advertising data */
    uint16_t adv_data_len;      /* Length of adv data (must be 0 to indicate MBLK) */
    MBLK_T *adv_data;
} MS_EXT_SCAN_FILTERED_ADV_REPORT_IND_T;
extern msAdvScanInstanceData_t  msAdvScanData;

extern void MsExtScanManagerDeInit(void);
extern void MsExtScanManagerInit(void);
extern void MsExtScanGlobalParamGetReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtScanGlobalParamSetReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtScanRegisterScannertReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtScanUnregisterScannerReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtScanEnableScannerReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtScanGetCtrlScanInfoReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtScanAdvertisingReportEvent(MS_EXT_SCAN_FILTERED_ADV_REPORT_IND_T *advReport);
void MsExtScanSetScanParamsDone(uint8_t status);
void MsExtScanEnableScanDone(uint8_t status);
#endif /* _EXT_SCAN_MANAGER_H_ */
