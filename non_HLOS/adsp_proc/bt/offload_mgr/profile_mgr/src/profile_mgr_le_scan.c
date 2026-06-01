/*==============================================================================*/
/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      profile_mgr_le_scan.c
===========================================================================*/
/**
 * @file profile_mgr_le_scan.c
 * @brief Handles le scan communication with Microstack.
 *
 * @details This file primarily handles:
 *          1. LE SCAN profile communication with Microstack.
 *             - Start Scan
 *             - Stop Scan
 *             - Register and Unregister Scanner
 */

/*============================================================================*
                                INCLUDE FILES
*============================================================================*/
#include <stdint.h>
#include "profile_mgr_le_scan.h"
#include "stringl.h"

/*============================================================================*
                                MACRO DEFINITIONS
*============================================================================*/
#define RFU_FLAG 0x00

#define SCAN_REG_DEFAULT_PARAM 0x00

#define ENABLE_LE_SCAN 0x01

#define DISABLE_LE_SCAN 0x00

#define MAX_SCANNERS_SUPPORTED 0x02

/*============================================================================*
                            GLOBAL DATA DECLARATIONS
*============================================================================*/
static uint16_t scanAppHandle = 0x00;

/*! Global Structure to Store Le Scan Context */
scanAppCtx_t leScanCtx;

/*! Global Structure to Store Current Scanner Request */
scanAppCurrReq_t scanCurrReq;

/*============================================================================*
                                FUNCTION DEFINITIONS
*============================================================================*/

/**
 * @brief Method to set Global Scan Parameters
 * Currently , these parameters are set internally by the scan middleware and capability to set global scan paramters is not exposed out to the application
 * @param None.
 * @return None
 */
void set_global_scan_params(void)
{
    MsScanningPhy scanningPhy;
    scanningPhy.scan_type = 0x00;
    scanningPhy.scan_window = 0x3E8;
    scanningPhy.scan_interval = 0x1000;
    BmmExtScanSetGlobalParamsReqSend(scanAppHandle , RFU_FLAG ,  0x00 , 0x00 , 0x01 , 0x01 , &scanningPhy);
}


/**
 * @brief Registers a scan command.
 * This function registers a scan command to be executed.
 * @param cmd Pointer to the command structure.
 * @return void
 */
void register_le_scanner(void *cmd)
{
    registerScanner_t *reg_scanner = (registerScanner_t *)cmd;
    
    BmmExtScanRegisterScannerReqSend(scanAppHandle,RFU_FLAG,reg_scanner->adv_filter,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,NULL);
}


/**
 * @brief Callback function for handling scanning events.
 * This function is called to handle scan events.
 * @param apphandle Handle to the Bluetooth application.
 * @param eventClass Class of the event.
 * @param message Pointer to the message structure.
 * @return void
 */
void le_scan_callback(BtAppHandle apphandle, BtEventClass eventClass, void *message)
{
    CsrPrim type = *((CsrPrim* )message);
    switch(type)
    {
        case MS_EXT_SCAN_REGISTER_SCANNER_CFM:
        {
            MsExtScanRegisterScannerCfm *reg_scan_set_cfm = (MsExtScanRegisterScannerCfm *)message;            
            OFFLOAD_MGR_LOGM("MS_EXT_SCAN_REGISTER_SCANNER_CFM");
            OFFLOAD_MGR_LOGM("Scan Handle : %d , Result Code : %d \n",reg_scan_set_cfm->scanHandle,reg_scan_set_cfm->resultCode);
            endpoint_t ep_id = {
                .hub_id = ENDPT_MGR_HUB_ID_AWM,
                .id     = apphandle, /* random */
            };
			endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			registerScannerCfm_t *req = (registerScannerCfm_t *)header->data;			
			header->opcode = LE_SCAN_REGISTER_SCANNER_CFM;
            header->data_len = sizeof(registerScannerCfm_t);
			header->endpoint_id = ep_id.id;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
			req->resultCode = reg_scan_set_cfm->resultCode;
            req->scanHandle = reg_scan_set_cfm->scanHandle;
            req->type = reg_scan_set_cfm->type;

			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, ENDPT_MGR_RPC_HDR_SIZE + header->data_len);

        }
        break;
        
        case MS_EXT_SCAN_UNREGISTER_SCANNER_CFM:
        {
            MsExtScanUnregisterScannerCfm *unreg_scan_set_cfm  = (MsExtScanUnregisterScannerCfm *)message;
            OFFLOAD_MGR_LOGM("MS_EXT_SCAN_UNREGISTER_SCANNER_CFM");
            OFFLOAD_MGR_LOGM("Result code : %d\n",unreg_scan_set_cfm->resultCode);
            endpoint_t ep_id = {
                .hub_id = ENDPT_MGR_HUB_ID_AWM,
                .id     = apphandle, /* random */
            };
            endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			unregisterScannerCfm_t *req = (unregisterScannerCfm_t *)header->data;			
			header->opcode = LE_SCAN_UNREGISTER_SCANNER_CFM;
            header->data_len = sizeof(registerScannerCfm_t);
			header->endpoint_id = ep_id.id;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
			req->resultCode = unreg_scan_set_cfm->resultCode;
            req->scanHandle = unreg_scan_set_cfm->scanHandle; 
            req->type = unreg_scan_set_cfm->type;
			
            endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, ENDPT_MGR_RPC_HDR_SIZE + header->data_len);
        }
        break;
        
        case MS_EXT_SCAN_SET_GLOBAL_PARAMS_CFM:
        {
            MsExtScanSetGlobalParamsCfm *set_scan_params_cfm = (MsExtScanSetGlobalParamsCfm *)message;
            OFFLOAD_MGR_LOGM("MS_EXT_SCAN_SET_GLOBAL_PARAMS_CFM");
            OFFLOAD_MGR_LOGM("Result Code : %d\n",set_scan_params_cfm->resultCode);
            endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			startScanAppRsp_t *rsp = (startScanAppRsp_t *)header->data;	
            uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(startScanAppRsp_t);	
			header->opcode = LE_SCAN_START_APP_RSP;
			header->data_len = sizeof(startScanAppRsp_t);
			header->endpoint_id = scanAppHandle;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
            rsp->appHandle = scanAppHandle;
            rsp->status = true;
			
            endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);
        }
        break;
        
        case MS_EXT_SCAN_ENABLE_SCANNERS_CFM:
        {
            MsExtScanEnableScannersCfm *scan_enable_cfm = (MsExtScanEnableScannersCfm *)message;
            OFFLOAD_MGR_LOGM("MS_EXT_SCAN_ENABLE_SCANNERS_CFM");
            OFFLOAD_MGR_LOGM("Result Code : %d\n",scan_enable_cfm->resultCode);
            endpoint_t ep_id = {
                .hub_id = ENDPT_MGR_HUB_ID_AWM,
                .id     = apphandle, /* random */
            };
			
			
			endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			enableScannerCfm_t *req = (enableScannerCfm_t *)header->data;	
            OFFLOAD_MGR_LOGM("Current Request : %d\n",scanCurrReq.currReq);
			header->opcode = (scanCurrReq.currReq == LE_SCAN_ENABLE_SCANNER)?LE_SCAN_ENABLE_SCANNER_CFM : LE_SCAN_DISABLE_SCANNER_CFM;
            header->data_len = sizeof(enableScannerCfm_t);
			header->endpoint_id = ep_id.id;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
			req->type = scan_enable_cfm->type;
            req->resultCode = scan_enable_cfm->resultCode;
            req->scanHandle = scanCurrReq.scanHandle;

			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, ENDPT_MGR_RPC_HDR_SIZE + header->data_len);
        }
        break;

        case MS_EXT_SCAN_FILTERED_ADV_REPORT_IND:
        {
            MsExtScanFilteredAdvReportInd *adv_report_ind = (MsExtScanFilteredAdvReportInd*)message;
             OFFLOAD_MGR_LOGM("MS_EXT_SCAN_FILTERED_ADV_REPORT_IND");
             OFFLOAD_MGR_LOGM("Rssi : %d , Tx Power : %d\n",adv_report_ind->rssi,adv_report_ind->txPower);
             OFFLOAD_MGR_LOGM("Address Type : %d Address : UAP : %d , LAP : %d , NAP : %d\n",adv_report_ind->currentAddrt.type,adv_report_ind->currentAddrt.addr.uap,adv_report_ind->currentAddrt.addr.lap,adv_report_ind->currentAddrt.addr.nap);
             endpoint_t ep_id = {
                .hub_id = ENDPT_MGR_HUB_ID_AWM,
                .id     = apphandle, /* random */
            };

            endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			extAdvReportInd_t *req = (extAdvReportInd_t *)header->data;			
			header->opcode = LE_SCAN_EXT_ADV_REPORT_IND;
            header->data_len = sizeof(extAdvReportInd_t);
			header->endpoint_id = ep_id.id;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
			req->txPower = adv_report_ind->txPower;
            req->rssi = adv_report_ind->rssi;
            req->type = adv_report_ind->type;
            memscpy(&(req->currentAddr),sizeof(req->currentAddr),&(adv_report_ind->currentAddrt),sizeof(adv_report_ind->currentAddrt));
            memscpy(&(req->directAddr),sizeof(req->directAddr),&(adv_report_ind->directAddrt),sizeof(adv_report_ind->directAddrt));
			
            endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, ENDPT_MGR_RPC_HDR_SIZE + header->data_len);

            CsrPmemFree(adv_report_ind->data);
            adv_report_ind->data = NULL;
        }
        break;

        default:
            break;
    }


}


/**
 * @brief Handles LE scan commands from AWM.
 * This function processes scan commands based on the provided opcode and length.
 * @param opcode The operation code for the scan command.
 * @param len The length of the command.
 * @param cmd Pointer to the command structure.
 * @return void
 */
void le_scan_cmd_handler(uint16_t opcode , uint16_t len , void *cmd)
{
    switch(opcode)
    {
        case LE_SCAN_START_APP:
        {
            uint16_t appHandle = *((uint16_t*)cmd);
            bool rc = microstack_register_app_callback(appHandle,le_scan_callback);

            if(rc == true)
            {
                scanAppHandle = appHandle;
                set_global_scan_params();
            }
            else
            {
                endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
                startScanAppRsp_t *rsp = (startScanAppRsp_t *)header->data;	
                uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(startScanAppRsp_t);	
                header->opcode = LE_SCAN_START_APP_RSP;
                header->data_len = sizeof(startScanAppRsp_t);
                header->endpoint_id = appHandle;
                header->command_type = NONOFFLOAD_APP_CMD;

                /* msg */
                rsp->appHandle = appHandle;
                rsp->status = rc;

                endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);

            }
        }
        break;
        
        case LE_SCAN_REGISTER_SCANNER:
        {
            OFFLOAD_MGR_LOGM("Register Scanner Command Received \n");
            registerScanner_t *reg_scanner = (registerScanner_t *)cmd;
            BmmExtScanRegisterScannerReqSend(scanAppHandle,RFU_FLAG,reg_scanner->adv_filter,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,SCAN_REG_DEFAULT_PARAM,NULL);
        }
        break;
        
        case LE_SCAN_UNREGISTER_SCANNER:
        {
            unregisterScanner_t *unregScanner = (unregisterScanner_t *)cmd;
            BmmExtScanUnregisterScannerReqSend(scanAppHandle, unregScanner->scanHandle);
        }
        break;
        
        case LE_SCAN_ENABLE_SCANNER:
        {
            enableScan_t *scan_enable = (enableScan_t *)cmd;
            MsScanners scanners;
            scanners.duration = scan_enable->duration;
            scanners.scanHandle = scan_enable->scanHandle;
            /*Cache the Reques for Sending Back the Confirmation*/
            scanCurrReq.currReq = LE_SCAN_ENABLE_SCANNER;
            scanCurrReq.scanHandle = scan_enable->scanHandle;
            OFFLOAD_MGR_LOGL("Current Request : %d\n",scanCurrReq.currReq);
            BmmExtScanEnableScannersReqSend(scanAppHandle,scan_enable->enable, 0x01 , &scanners);
        }
        break;
        
        case LE_SCAN_DISABLE_SCANNER:
        {
           enableScan_t *scan_enable = (enableScan_t *)cmd;
           MsScanners scanners;
           scanners.duration = scan_enable->duration;
           scanners.scanHandle = scan_enable->scanHandle;
           /*Cache the Reques for Sending Back the Confirmation*/
           scanCurrReq.currReq = LE_SCAN_DISABLE_SCANNER;
           scanCurrReq.scanHandle = scan_enable->scanHandle;
           OFFLOAD_MGR_LOGL("Current Request : %d\n",scanCurrReq.currReq);
           BmmExtScanEnableScannersReqSend(scanAppHandle,DISABLE_LE_SCAN, 0x01 , &scanners);
        }
        break;

        case LE_SCAN_STOP_APP:
        {
            uint16_t appHandle = *((uint16_t*)cmd);
            bool rc  = microstack_deregister_app_callback(appHandle);
            OFFLOAD_MGR_LOGL("Response for Stop Le Scan App : %d\n",rc);
        }
        break;

        default:
            OFFLOAD_MGR_LOGE("Unhandled Event Received : %d\n",opcode);
            break;
    }
}
