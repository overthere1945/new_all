/**************************************************************************
 * @file     lpai_bt_le_adv.c
 * @brief    LPAI BT LE Advertisement Sournce file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "lpai_bt_le_scan.h"
#include <zephyr/sys/printk.h>
#include <stdbool.h>
#include "qapi_bt_le_scan.h"
#include "stringl.h"

/*===========================================================================
                        MACRO DEFINITIONS
===========================================================================*/
#define SCAN_APP_HANDLE 0x9055

#define SCAN_APP_CMD_RESULT_SUCCESS 0x00

/*===========================================================================
                        PUBLIC/GLOBAL DATA DEFINITIONS
===========================================================================*/
scanAppCtx_t leScanCtx;


/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/

/**
 * @brief Initialization Function for LE Scan App
 * @param[in] None
 * @return    None
 */
void le_scan_init()
{
    memset(&leScanCtx,0,sizeof(leScanCtx));
}

/**
 * @brief Deinitialization Function for LE Scan App
 * @param[in] None
 * @return None
 */
void le_scan_deinit()
{
    lpai_bt_app_mgr_unregister_microapp(SCAN_APP_HANDLE);
    memset(&leScanCtx,0,sizeof(leScanCtx));
}

/**
 * @brief Static Method to check if scan app is activated
 * @param[in] None
 * @return    None
 */
static bool get_scan_app_status()
{
    return leScanCtx.isActive;
}

/**
 * @brief Static Method to find index for storing scan handle
 * @param[in] None
 * @return    None
 */
static uint8_t find_available_scanner_idx()
{
    uint8_t idx = 0;
    for(idx = 0; idx < MAX_SCANNERS_SUPPORTED; idx++)
    {
        if(leScanCtx.scannerCtx[idx].idxInUse == false)
        {
            return idx;
        }
    }
    return idx;
}

/**
 * @brief Static Method to find index for a given scan handle in scanners context information
 * @param[in] scanHandle scan Handle for which index is required
 * @return    idx       Index where scan Handle is stored , Invalid value otherwise
 */
static uint8_t find_scanner_idx(uint8_t scanHandle)
{
    uint8_t idx;
    for(idx = 0; idx < MAX_SCANNERS_SUPPORTED; idx++)
    {
        if(leScanCtx.scannerCtx[idx].scanHandle == scanHandle)
        {
            return idx;
        }
    }
    return idx;
}

/**
 * @brief This method is used register LE Scan APP on ADSP. On successful registartion app is activated and ready for use on AWM
 * @param[in] None
 * @return    None
 */
void register_leScanApp()
{
    app_registration_status_code_t result = lpai_bt_app_mgr_register_microapp(SCAN_APP_HANDLE,leScanCb);

    uint16_t appHandle = SCAN_APP_HANDLE;
    printk("App Registration Status : %d\n",result);
    if(leScanCtx.isActive == false)
    {
        lpai_bt_appmgr_send_microapp_msg_adsp(START_LE_SCAN_APP , sizeof(appHandle) , &appHandle );
    }
    else
    {
        printk("Le Scan App Already Active");
    }
}

/**
 * @brief This method is used unregister LE Scan APP on ADSP. On successful unregistartion app is deactivated and can be only used once activated again
 * @param[in] None
 * @return    None
 */
void unregister_leScanApp()
{
    uint16_t appHandle = SCAN_APP_HANDLE;
    if(leScanCtx.isActive == true)
    {
        lpai_bt_app_mgr_unregister_microapp(SCAN_APP_HANDLE);
        lpai_bt_appmgr_send_microapp_msg_adsp(STOP_LE_SCAN_APP , sizeof(appHandle) , &appHandle );
        le_scan_init();
    }
    else
    {
        printk("Le Scan App Not Active Yet !");
    }
}

/**
 * @brief This method is used by client on AWM to register a scanner on ADSP
 * @param[in] advFilter     Adv Filter Value to be used while registration of a scanner 
 * @return    None
 */
void register_scanner(uint16_t advFilter)
{
    if(get_scan_app_status())
    {
        registerScanner_t regScanner;
        regScanner.adv_filter = advFilter;
        regScanner.appHandle = leScanCtx.appHandle;
        lpai_bt_appmgr_send_microapp_msg_adsp(REGISTER_SCANNER,sizeof(registerScanner_t),&regScanner);
    }
    else
    {
        printk("Scan App Not Active Yet . Please Start the APP !\n");
    }

}

/**
 * @brief This method is used register LE Scan APP on ADSP
 * @param[in] scanHandle  Scan Handle for which unregistration is to be performed
 * @return    None
 */
void unregister_scanner(uint8_t scanHandle)
{
    if(get_scan_app_status())
    {
        unregisterScanner_t unregScanner;
        uint8_t scannerIdx = find_scanner_idx(scanHandle);
        if(scannerIdx != MAX_SCANNERS_SUPPORTED)
        {
             unregScanner.appHandle = leScanCtx.appHandle;
            unregScanner.scanHandle = scanHandle;
            lpai_bt_appmgr_send_microapp_msg_adsp(UNREGISTER_SCANNER,sizeof(unregisterScanner_t),&unregScanner);
        }
        else
        {
            printk("Invalid Scan Handle Received for Unregistration . Please Check the Scan Handle\n");
        }
    }
    else
    {
        printk("Scan App Not Active Yet . Please Start the APP !\n");
    }
}

/**
 * @brief This method is used to enable or disable scan for a particular scanner
 * @param[in] enable       Bool to specify if scan is to be enabled or disabled
 * @param[in] scanHandle   Scan Handle for which operation is to be performed
 * @param[in] duration     Duaration for which scan is to be enabled , ignored when scan is disabled
 * @return    None
 */
void enable_scan(bool enable , uint8_t scanHandle , uint16_t duration)
{
    uint16_t opcode = (enable == true)?ENABLE_SCANNER:DISABLE_SCANNER;
    if(get_scan_app_status())
    {
        uint8_t scannerIdx = find_scanner_idx(scanHandle);
        if(scannerIdx != MAX_SCANNERS_SUPPORTED)
        {
            enableScan_t enableScan;
            enableScan.appHandle = leScanCtx.appHandle;

            enableScan.enable = enable;
            enableScan.scanHandle = scanHandle;
            enableScan.duration = duration;
            lpai_bt_appmgr_send_microapp_msg_adsp(opcode,sizeof(enableScan_t),&enableScan); 
        }
        else
        {
            printk("Invalid Scan Handle Received for enablement/disablement . Please Check the Scan Handle\n");
        }
    }   
    else
    {
        printk("Scan App Not Active Yet . Please Start the APP !\n");
    }
}


/**
 * @brief Le Scan Callback Function registered with App Manager on AWM
 * This function is invoked each time a scan event is received on AWM from ADSP
 * @param[in] eventId         Event Identifier to specify type of event
 * @param[in] appDataLen      Length of the Data received along with the event
 * @param[in] appData         Pointer to actual data specific for each event
 * @param[in] proto_encoded   Bool to specifiy if data is proto encoded or not
 * @return    None
 */
void leScanCb(uint16_t eventId , uint16_t appDataLen , void *appData,bool proto_encoded)
{
    printk("Handle Le Scan Events \n");
    printk("Event Id Received : %d\n",eventId);
    switch (eventId)
    {
        case START_SCAN_APP_RSP:
        {
            startScanAppRsp_t *scanRsp = (startScanAppRsp_t*)appData;
            printk("App Handle : %d , Result Code : %d\n",scanRsp->appHandle,scanRsp->status);
            if(scanRsp->status == true)
            {
                leScanCtx.appHandle = scanRsp->appHandle;
                leScanCtx.isActive = true;
            }
        }
        break;

        case REGISTER_SCANNER_CFM:
        {
            registerScannerCfm_t *registerScanner = (registerScannerCfm_t*)appData;

            uint8_t scannerIdx = find_available_scanner_idx();
            if(scannerIdx != MAX_SCANNERS_SUPPORTED && registerScanner->resultCode == SCAN_APP_CMD_RESULT_SUCCESS)
            {
                leScanCtx.scannerCtx[scannerIdx].scanHandle = registerScanner->scanHandle;
                leScanCtx.scannerCtx[scannerIdx].idxInUse = true;
                qapi_registerScannerCfm_t regScannerCfm = {.resultCode = registerScanner->resultCode,
                                                            .scanHandle = registerScanner->scanHandle
                                                        };
                qapi_lescan_evt_handler(QAPI_REG_SCANNER_CFM,sizeof(regScannerCfm),&regScannerCfm);

            }
            else if(scannerIdx == MAX_SCANNERS_SUPPORTED)
            {
                printk("No More Scanners Can be registered as limit reached");
            }
        }
        break;

        case UNREGISTER_SCANNER_CFM:
        {
            unregisterScannerCfm_t *unreg_scanner = (unregisterScannerCfm_t *)appData;
            uint8_t scannerIdx = find_scanner_idx(unreg_scanner->scanHandle);
            printk("Index for Scan Handle : %d is %d",unreg_scanner->scanHandle,scannerIdx);
            if(scannerIdx != MAX_SCANNERS_SUPPORTED && unreg_scanner->resultCode == SCAN_APP_CMD_RESULT_SUCCESS)
            {
                leScanCtx.scannerCtx[scannerIdx].scanHandle = 0x00;
                leScanCtx.scannerCtx[scannerIdx].idxInUse = false;
                qapi_unregisterScannerCfm_t unregScannerCfm = {.resultCode = unreg_scanner->resultCode,
                                                            .scanHandle = unreg_scanner->scanHandle
                                                        };
                qapi_lescan_evt_handler(QAPI_UNREG_SCANNER_CFM,sizeof(unregScannerCfm),&unregScannerCfm);
            }
            else
            {
                printk("Invalid Scan Handle Received for Unregistration\n");
            }
        }
        break;

        case ENABLE_SCANNER_CFM:
        {
            enableScannerCfm_t *enableScanner = (enableScannerCfm_t*)appData;
            qapi_enableScannerCfm_t enableScannerCfm = {.resultCode = enableScanner->resultCode,
                                                        .scanHandle = enableScanner->scanHandle
                                                        };
            qapi_lescan_evt_handler(QAPI_ENABLE_SCANNER_CFM,sizeof(enableScannerCfm),&enableScannerCfm);                                         

        }
        break;

        case DISABLE_SCANNER_CFM:
        {
            enableScannerCfm_t *enableScanner = (enableScannerCfm_t*)appData;
            qapi_enableScannerCfm_t enableScannerCfm = {.resultCode = enableScanner->resultCode,
                                                        .scanHandle = enableScanner->scanHandle
                                                        };
            qapi_lescan_evt_handler(QAPI_DISABLE_SCANNER_CFM,sizeof(enableScannerCfm),&enableScannerCfm); 
        }
        break;

        case EXT_ADV_REPORT_IND:
        {
            extAdvReportInd_t *extAdvReport = (extAdvReportInd_t*)appData;
            qapi_extAdvReportInd_t advreportInd ;
            memscpy(&advreportInd,sizeof(advreportInd),extAdvReport,sizeof(advreportInd));
            qapi_lescan_evt_handler(QAPI_EXT_ADV_REPORT_IND,sizeof(advreportInd),&advreportInd);
        }
        break;

        default:
             break;
    }
}

/**
 * @brief This method is used by client on AWM to register a scanner on ADSP
 * @param[in] advFilter     Adv Filter Value to be used while registration of a scanner 
 * @return    None
 */
void qapi_register_scanner(uint16_t advFilter)
{
    register_scanner(advFilter);
}

/**
 * @brief This method is used unregister an registerd scanner
 * @param[in] scanHandle  Scan Handle for which unregistration is to be performed
 * @return    None
 */
void qapi_unregister_scanner(uint8_t scanHandle)
{
    unregister_scanner(scanHandle);
}

/**
 * @brief This method is used to enable or disable scan for a particular scanner
 * @param[in] enable       Bool to specify if scan is to be enabled or disabled
 * @param[in] scanHandle   Scan Handle for which operation is to be performed
 * @param[in] duration     Duaration for which scan is to be enabled , ignored when scan is disabled
 * @return    None
 */
void qapi_enable_scan(bool enable , uint8_t scanHandle , uint16_t duration)
{
    enable_scan(enable,scanHandle,duration);
}


/**
 * @brief Handler Function to Handle LE SCAN QAPI EVENTS
 * @param[in] opcode       Opcode to identify Incoming Event
 * @param[in] appDataLen   Data Length of the Incoming Event
 * @param[in] appData      Pointer to Actual Data of the incoming event
 * @return    None
 */
void qapi_lescan_evt_handler(uint16_t opcode,uint16_t appDataLen , void *appData)
{
    switch(opcode)
    {
        case QAPI_REG_SCANNER_CFM:
        {
            qapi_registerScannerCfm_t *qapi_reg_scanner_cfm = (qapi_registerScannerCfm_t*)appData;
            printk("QAPI REG SCANNER_CFM \n");
            printk("Result Code : %d Scan Handle Received : %d \n",qapi_reg_scanner_cfm->resultCode , qapi_reg_scanner_cfm->scanHandle);
        }
        break;

        case QAPI_UNREG_SCANNER_CFM:
        {
            qapi_unregisterScannerCfm_t *qapi_unreg_scanner_cfm = (qapi_unregisterScannerCfm_t*)appData;
            printk("QAPI UNREG SCANNER_CFM \n");
            printk("Result Code : %d Scan Handle Received : %d \n",qapi_unreg_scanner_cfm->resultCode , qapi_unreg_scanner_cfm->scanHandle);
        }
        break;

        case QAPI_ENABLE_SCANNER_CFM:
        {
            qapi_enableScannerCfm_t *qapi_enable_scanner_cfm = (qapi_enableScannerCfm_t*)appData;
            printk("QAPI ENABLE SCANNER CFM \n");
            printk("Scanning Enabled for Scan Handle : %d with result code : %d\n",qapi_enable_scanner_cfm->scanHandle,qapi_enable_scanner_cfm->resultCode);
        }
        break;

        case QAPI_DISABLE_SCANNER_CFM:
        {
            qapi_enableScannerCfm_t *qapi_enable_scanner_cfm = (qapi_enableScannerCfm_t*)appData;
            printk("QAPI DISABLE SCANNER CFM \n");
            printk("Scanning Disabled for Scan Handle : %d with result code : %d\n",qapi_enable_scanner_cfm->scanHandle,qapi_enable_scanner_cfm->resultCode);
        }
        break;

        case QAPI_EXT_ADV_REPORT_IND:
        {
            qapi_extAdvReportInd_t *extAdvReport = (qapi_extAdvReportInd_t*)appData;
            printk("QAPI EXT REPORT IND \n");
            printk("Tx Power : %d , Rssi : %d\n",extAdvReport->txPower,extAdvReport->rssi);
            printk("Address Type : %d Address : UAP : %x , LAP : %x , NAP : %x\n",extAdvReport->currentAddr.type,extAdvReport->currentAddr.bd_addr.uap,extAdvReport->currentAddr.bd_addr.lap,extAdvReport->currentAddr.bd_addr.nap);
        }
        break;

        default:
            break;
    } 
}