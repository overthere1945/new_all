/**************************************************************************
 * @file     lpai_bt_le_scan.h
 * @brief    LPAI BT LE SCAN header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_LE_SCAN_H
#define LPAI_BT_LE_SCAN_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lpai_bt_app_mgr_client_interface.h"

/*===========================================================================
                        MACRO DEFINITIONS
===========================================================================*/
#define MAX_SCANNERS_SUPPORTED 0x02


/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

/**
 * @enum le_scan_cmds_t
 * LE SCAN Commands and Response for Communication between AAWM adn ADSP
 */
typedef enum{
    START_LE_SCAN_APP = 0x6000,
    REGISTER_SCANNER ,
    UNREGISTER_SCANNER,
    SET_GLOBAL_SCAN_PARAMS,
    GET_GLOBAL_SCAN_PARAMS,
    ENABLE_SCANNER,
    DISABLE_SCANNER,
    STOP_LE_SCAN_APP,
    START_SCAN_APP_RSP,
    REGISTER_SCANNER_CFM,
    UNREGISTER_SCANNER_CFM,
    SET_GLOBAL_SCAN_PARAMS_CFM,
    ENABLE_SCANNER_CFM,
    DISABLE_SCANNER_CFM,
    EXT_ADV_REPORT_IND,
    SCAN_CMD_MAX,
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
    bool isActive;
    uint16_t appHandle;
    leScannerCtx_t scannerCtx[MAX_SCANNERS_SUPPORTED];
}scanAppCtx_t;


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
 * @struct unregisterScanner_t
 * Unregister Scanner Command Structure used to register a Scanner on ADSP
 */
typedef struct unregisterScanner{
    uint16_t appHandle;
    uint8_t scanHandle;
}unregisterScanner_t;


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
 * @brief Initialization Function for LE Scan App
 * @param[in] None
 * @return    None
 */
void le_scan_init();

/**
 * @brief Deinitialization Function for LE Scan App
 * @param[in] None
 * @return None
 */
void le_scan_deinit();


/**
 * @brief Le Scan Callback Function registered with App Manager on AWM
 * This function is invoked each time a scan event is received on AWM from ADSP
 * @param[in] eventId         Event Identifier to specify type of event
 * @param[in] appDataLen      Length of the Data received along with the event
 * @param[in] appData         Pointer to actual data specific for each event
 * @param[in] proto_encoded   Bool to specifiy if data is proto encoded or not
 * @return    None
 */
void leScanCb(uint16_t eventId , uint16_t appDataLen , void *appData,bool proto_encoded);

/**
 * @brief This method is used by client on AWM to register a scanner on ADSP
 * @param[in] advFilter     Adv Filter Value to be used while registration of a scanner 
 * @return    None
 */
void register_scanner(uint16_t advFilter);

/**
 * @brief This method is used unregister an registerd scanner
 * @param[in] scanHandle  Scan Handle for which unregistration is to be performed
 * @return    None
 */
void unregister_scanner(uint8_t scanHandle);

/**
 * @brief This method is used to enable or disable scan for a particular scanner
 * @param[in] enable       Bool to specify if scan is to be enabled or disabled
 * @param[in] scanHandle   Scan Handle for which operation is to be performed
 * @param[in] duration     Duaration for which scan is to be enabled , ignored when scan is disabled
 * @return    None
 */
void enable_scan(bool enable , uint8_t scanHandle , uint16_t duration);

/**
 * @brief This method is used register LE Scan APP on ADSP. On successful registartion app is activated and ready for use on AWM
 * @param[in] None
 * @return    None
 */
void register_leScanApp(void);


/**
 * @brief This method is used unregister LE Scan APP on ADSP. On successful unregistartion app is deactivated and can be only used once activated again
 * @param[in] None
 * @return    None
 */
void unregister_leScanApp(void);




#endif /** LPAI_BT_LE_SCAN_H */