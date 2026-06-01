/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      profile_mgr_le_scan.h
===========================================================================*/
/**
 * @file profile_mgr_le_scan.h
 * @brief Public header file for LE SCAN profile.
 *
 * @details This file contains the public API definitions for the LE SCAN profile.
 */
#ifndef PROFILE_MGR_LE_SCAN_H
#define PROFILE_MGR_LE_SCAN_H

/*============================================================================*
                                INCLUDE FILES
*============================================================================*/
#include <stdint.h>
#include "endpt_mgr.h"
#include "endpt_mgr_rpc.h"
#include "ms_ext_scan_lib.h"
#include "offload_mgr_client_interface.h"

/*============================================================================*
                                MACRO DEFINITIONS
*============================================================================*/

/*! Maximum Scannaers Supported */
#define MAX_SCANNERS_SUPPORTED 0x02


/*===========================================================================
                        TYPE DEFINITINOS
===========================================================================*/

/**
 * @enum le_scan_cmds_t
 * LE SCAN Commands and Response for Communication between AAWM adn ADSP
 */
typedef enum{
    LE_SCAN_START_APP = 0x6000,
    LE_SCAN_REGISTER_SCANNER ,
    LE_SCAN_UNREGISTER_SCANNER,
    LE_SET_GLOBAL_SCAN_PARAMS,
    LE_SCAN_GET_GLOBAL_SCAN_PARAMS,
    LE_SCAN_ENABLE_SCANNER,
    LE_SCAN_DISABLE_SCANNER,
    LE_SCAN_STOP_APP,
    LE_SCAN_START_APP_RSP,
    LE_SCAN_REGISTER_SCANNER_CFM,
    LE_SCAN_UNREGISTER_SCANNER_CFM,
    LE_SCAN_SET_GLOBAL_SCAN_PARAMS_CFM,
    LE_SCAN_ENABLE_SCANNER_CFM,
    LE_SCAN_DISABLE_SCANNER_CFM,
    LE_SCAN_EXT_ADV_REPORT_IND,
    LE_SCAN_CMD_MAX,
}le_scan_cmds_t;


/**
 * @struct scannerInfo_t
 * Scanner Info Context Maintained on AWM
 */
typedef struct{
    bool idxInUse;
    uint8_t scanHandle;
}leScannerCtx_t;


/**
 * @struct scanAppCtx_t
 * Scanner App Context Information along with context information for  Scanners Supported
 */
typedef struct scanAppCtx{
    uint16_t appHandle;
    leScannerCtx_t scannerCtx[MAX_SCANNERS_SUPPORTED];
}scanAppCtx_t;


/**
 * @struct scanAppCurrReq_t
 * Structure to Maintain the recent request for enable/disable for a particulat scan Handle
 */
typedef struct{
    uint8_t scanHandle;
    uint16_t currReq;
}scanAppCurrReq_t;


/**
 * @struct startScanAppRsp_t
 * Structure to get Responsefrom ADSP for App Registration 
 * */
typedef struct startScanAppRsp{
    uint16_t appHandle;
    bool status;
}startScanAppRsp_t;


/**
 * @struct registerScanner_t
 * Register Scanner Command Structure used to register a Scanner on ADSP
 */
typedef struct registerScanner{
    uint16_t appHandle;
    uint16_t adv_filter;
}registerScanner_t;


/**
 * @struct registerScannerCfm_t
 * Register Scanner Command Confirm Structure received on AWM from ADSP
 */
typedef struct registerScannerCfm{
    uint16_t type;
    uint16_t resultCode;
    uint8_t scanHandle;
}registerScannerCfm_t;


/**
 * @struct unregisterScannerCfm_t
 * Unregister Scanner Command Confirm Structure received on AWM from ADSP
 */
typedef struct unregisterScannerCfm{
    uint16_t type;
    uint16_t resultCode;
    uint8_t scanHandle;
}unregisterScannerCfm_t;


/**
 * @struct unregisterScanner_t
 * Unregister Scanner Command Structure used to register a Scanner on ADSP
 */
typedef struct unregisterScanner{
    uint16_t appHandle;
    uint8_t scanHandle;
}unregisterScanner_t;


/**
 * @struct enableScan_t
 * Enable or Disable Scan for a Scanner with a given scan handle and duration
 */
typedef struct enableScan{
    uint16_t appHandle;
    bool enable;
    uint8_t scanHandle;
    uint16_t duration;
}enableScan_t;


/**
 * @struct enableScannerCfm_t
 * Response for Enable/Disable Scan Request for a Scanner
 */
typedef struct enableScannerCfm{
    uint16_t type;
    uint16_t resultCode;
    uint8_t scanHandle;
}enableScannerCfm_t;


/**
 * @struct bd_addr_t
 * Structure for storing the BD Address
 */
typedef struct bd_addr{
    uint32_t lap;   /*!< Lower Address Part 00..23 */
    uint8_t  uap;   /*!< upper Address Part 24..31 */
    uint16_t nap;   /*!< Non-significant    32..47 */
}bd_addr_t;


/**
 * @struct typed_bd_addr_t
 * Structure for storing the BD Address along with its type
 */
typedef struct typed_bd_addr{
    uint8_t type;
    bd_addr_t bd_addr;
}typed_bd_addr_t;


/**
 * @struct extAdvReportInd_t
 * Extended Advertisement Indication Report Received from ADSP once scan is enabled
 */
typedef struct extAdvReportInd{
    uint16_t type;
    uint16_t eventType;
    uint8_t advSid;
    int8_t txPower;
    int8_t rssi;
    typed_bd_addr_t currentAddr;
    typed_bd_addr_t directAddr;
}extAdvReportInd_t;


/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
/**
 * @brief Sets the scan parameters.
 * @param None.
 * @return None
 */
void set_global_scan_params(void);


/**
 * @brief Registers a scan command.
 * This function registers a scan command to be executed.
 * @param cmd Pointer to the command structure.
 * @return void
 */
void register_le_scanner(void *cmd);


/**
 * @brief Callback function for handling scanning events.
 * This function is called to handle scan events.
 * @param apphandle Handle to the Bluetooth application.
 * @param eventClass Class of the event.
 * @param message Pointer to the message structure.
 * @return void
 */
void le_scan_callback(BtAppHandle apphandle, BtEventClass eventClass, void *message);


/**
 * @brief Handles scan commands.
 * This function processes scan commands based on the provided opcode and length.
 * @param opcode The operation code for the scan command.
 * @param len The length of the command.
 * @param cmd Pointer to the command structure.
 * @return void
 */
void le_scan_cmd_handler(uint16_t opcode , uint16_t len , void *cmd);

#endif
