/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      profile_mgr_le_adv.h
===========================================================================*/
/**
 * @file profile_mgr_le_adv.h
 * @brief Public header file for LE ADV profile.
 *
 * @details This file contains the public API definitions for the LE ADV profile.
 */
#ifndef PROFILE_MGR_LE_ADV_H
#define PROFILE_MGR_LE_ADV_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include "endpt_mgr.h"
#include "endpt_mgr_rpc.h"
#include "ms_ext_adv_lib.h"
#include "offload_mgr_client_interface.h"

/*===========================================================================
                        TYPE DEFINITINOS
===========================================================================*/

/**
 * @brief Enumeration of LE advertising commands.
 * LE ADV Commands and Responses Exchanged between ADSP and AWM
 */
typedef enum{
    LE_ADV_START_APP = 0x5000,
    LE_ADV_REGISTER_ADV_SET ,
    LE_ADV_UNREGISTER_ADV_SET ,
    LE_ADV_SET_ADV_PARAMS,
    LE_ADV_SET_ADV_DATA,
    LE_ADV_SET_RANDOM_ADDRESS,
    LE_ADV_START_ADVERTISEMENT ,
    LE_ADV_STOP_ADVERTISEMENT,
    LE_ADV_STOP_APP,
    LE_ADV_START_APP_RSP,
    LE_ADV_REGISTER_ADV_SET_CFM,
    LE_ADV_UNREGISTER_ADV_SET_CFM,
    LE_ADV_SET_ADV_PARAMS_CFM,
    LE_ADV_SET_ADV_DATA_CFM,
    LE_ADV_SET_RANDOM_ADDRESS_CFM,
    LE_ADV_START_ADVERTISEMENT_CFM,
    LE_ADV_STOP_ADVERTISEMENT_CFM,
    LE_ADV_CMD_MAX,
}le_adv_cmds_t;


/**
 * @struct scanAppCurrReq_t
 * Structure to Maintain the recent request for enable/disable for a particulat adv Set
 * This is helpful in sending back the correct response back to the application
 * to identify correctly if advertisement was enabled or disabled for a particular advertisement set
 */
typedef struct advAppCurrReq{
    uint8_t advHandle;
    uint16_t currReq;
}advAppCurrReq_t;


/**
 * @struct adv_data_train_info_t
 * Structure to Maintain the recent request for enable/disable for a particulat adv Set
 * This is helpful in sending back the correct response back to the application
 * to identify correctly if advertisement was enabled or disabled for a particular advertisement set
 */
typedef struct{
	uint8_t current_adv_block;
	uint8_t current_adv_block_idx;
	uint8_t num_adv_blocks;
}adv_data_train_info_t;


/**
 * @brief Structure to get App Handle for ADV App Registration with BT HOST 
 * @param appHandle Application Handle
 * */
typedef struct startAdvApp{
    uint16_t appHandle;
}startAdvApp_t;


/**
 * @brief Structure to register an advertising set.
 * @param appHandle Application handle
 */
typedef struct registerAdvSet{
    uint16_t appHandle;
}registerAdvSet_t;


/**
 * @brief Structure to unregister an advertising set.
 * @param appHandle Application handle
 * @param advHandle Advertising handle to be unregistered
 */
typedef struct unregisterAdvSet{
    uint16_t appHandle;
    uint16_t advHandle;
}unregisterAdvSet;
 

/**
 * @brief Structure to Enable Advertisment for an advertising set.
 * @param appHandle Application Handle
 * @param advHandle Adv Set Handle for which advertisement is to be enabled
 */
typedef struct enableAdvertisement{
    uint16_t appHandle;
    uint8_t advHandle;
}enableAdvertisement_t;


/**
 * @brief Structure to Disable Advertisment for an advertising set.
 * @param appHandle Application Handle
 * @param advHandle Adv Set Handle for which advertisement was disabled
 */
typedef struct disbaleAdvertisment{
    uint16_t appHandle;
    uint8_t advHandle;
}disbaleAdvertisment_t;


/**
 * @brief Structure to Set Advertisement Parameters for an Registered Advertisement Set
 * @param appHandle          Application Handle
 * @param advHandle          Advertisement Set Handle for which parameters are set
 * @param advEventProperties Advertisement Type
 * @param intervalMin        Minimum Interval Parameter
 * @param intervalMax        Maximum Interval Parameter
 * @param advTxPower         Tx Power Parameter
 * @param ownAddrType        Own Address Type
 */
typedef struct{
    uint16_t appHandle;
    uint8_t advHandle;
    uint16_t advEventProperties;
    uint32_t intervalMin;
    uint32_t intervalMax;
    int8_t advTxPower;
    int8_t ownAddrType;
}setAdvParams_t;


/**
 * @brief Structure to confirm the setting of advertisement parameters.
 * @param resultCode Result code for setting of advertisement parameters for an advertising set
 * @param advHandle Advertising handle
 * @param selected_tx_pwr Selected Tx Power
 */
typedef struct setAdvParamsCfm{
    uint16_t resultCode;
    uint8_t  advHandle;
    int8_t   selected_tx_pwr;
}setAdvParamsCfm_t;


/**
 * @brief Structure to Set Advertisement Data for an Registered Advertisement Set
 * @param appHandle          Application Handle
 * @param advHandle          Advertisement Set Handle for which parameters are set
 * @param service_uuid       Service UUID to be set in advertisement data
 * @param local_name         Local Name to be set in advertisement data 
 * @param local_name_len     Local Name Length
 */
typedef struct{
    uint16_t appHandle;
    uint8_t  advHandle;
    uint16_t service_uuid;
    uint8_t  local_name[20];
    uint8_t  local_name_len;
}setAdvData_t;


/**
 * @brief Structure to confirm the setting of advertisement data.
 * @param resultCode Result code for setting of advertisement data for an advertising set
 * @param advHandle Advertising handle
 */
typedef struct setAdvDataCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}setAdvDataCfm_t;


/**
 * @brief Structure to set Random Address for an Advertisement Set.
 * @param appHandle Application handle
 * @param advHandle Advertisement Set Handle for which address is set
 * @param deviceAddress Device Address to be Set
 */
typedef struct setRandomAddress{
    uint16_t appHandle;
    uint8_t advHandle;
    uint8_t type;
    CsrBtDeviceAddr deviceAddress;
}setRandomAddress_t;


/**
 * @brief Structure to confirm setting of Random Address for an Advertisement Set.
 * @param resultCode Result code for unregistration of an advertising set
 * @param advHandle  Advertising handle
 * @param deviceAddress Device Address which is set
 */
typedef struct setRandomAddressCfm{
    uint16_t  resultCode;
    uint8_t   advHandle;
    CsrBtDeviceAddr  randomAddr;
}setRandomAddressCfm_t;


/**
 * @brief Structure to get App Handle for ADV App Registration with BT HOST 
 * @param appHandle Application Handle
 * @param status Bool to Specify if App Registration was successful
 * */
typedef struct startAdvAppRsp{
    uint16_t appHandle;
    bool status;
}startAdvAppRsp_t;


/**
 * @brief Structure to confirm the registration of an advertising set.
 * @param resultCode Result code for registration of an advertising set
 * @param advHandle Advertising handle
 */
typedef struct registerAdvSetCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}registerAdvSetCfm_t;


/**
 * @brief Structure to confirm the unregistration of an advertising set.
 * @param resultCode Result code for unregistration of an advertising set
 * @param advHandle Advertising handle
 */
typedef struct unregisterAdvSetCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}unregisterAdvSetCfm;


/**
 * @brief Structure to confirm the start of an advertisement.
 * @param resultCode Result code of the start process
 * @param advHandle  Adv Set Handle used for enablement
 */
typedef struct startAdvCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}startAdvCfm_t;
 

/**
 * @brief Structure to confirm the start of an advertisement.
 * @param resultCode Result code of the start process
 * @param advHandle  Adv Set Handle used for disablement
 */
typedef struct stopAdvCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}stopAdvCfm_t;


/**
 * @brief Structure to define LE advertising parameters.
 * @param advEventProperties Advertising event properties
 * @param primaryAdvIntervalMin Minimum primary advertising interval
 * @param primaryAdvIntervalMax Maximum primary advertising interval
 * @param advTxPower Advertising transmit power
 */
typedef struct{
    uint16_t advEventProperties;
    uint32_t primaryAdvIntervalMin;
    uint32_t primaryAdvIntervalMax;
    int8_t advTxPower;
}le_adv_params_t;


/**
 * @brief Structure to define the context for LE advertising.
 * @param idx_in_use Indicates if the index is in use
 * @param advHandle Advertising handle
 * @param adv_paranms  Advertising parameters
 * @param current_adv_request Current advertising request
 */
typedef struct{
    bool idx_in_use;
    int8_t advHandle;
    le_adv_params_t adv_paranms;
    uint16_t current_adv_request;
}le_adv_context_t;


/**
 * @brief Sets the advertisement Parameters for a given application handle and advertising handle.
 * @param advParams Pointer to Advertisement Parameters to be set.
 * @return void.
 */
void le_adv_handle_set_adv_params(setAdvParams_t *advParams);


/**
 * @brief Sets the advertisement data for a given application handle and advertising handle.
 * @param setAdvData_t Pointer to Data to be set.
 * @return void.
 */
void le_adv_set_advertisement_data(setAdvData_t *advData);


/**
 * @brief Sets the advertisement data for a given application handle and advertising handle.
 * @param setAdvData_t Pointer to Data to be set.
 * @return void.
 */
void le_adv_set_random_address(setRandomAddress_t *randomAddr);


/**
 * @brief Callback function for LE advertising events.
 *
 * @param apphandle The application handle.
 * @param eventClass The class of the event.
 * @param message Pointer to the message associated with the event.
 * @return void.
 */
void le_adv_callback(BtAppHandle apphandle, BtEventClass eventClass, void *message);


/**
 * @brief Handles the start of an advertisement.
 * @param cmd Pointer to the command structure.
 * @return void.
 */
void le_adv_handle_start_advertisement(void *cmd);


/**
 * @brief Handles the stop of an advertisement.
 * @param cmd Pointer to the command structure.
 * @return void.
 */
void le_adv_handle_stop_advertisement(void *cmd);


/**
 * @brief Handles LE advertising commands from AWM.
 * This function processes advertisement commands based on the provided opcode and length.
 * @param opcode The operation code of the command.
 * @param len The length of the command.
 * @param cmd Pointer to the command structure.
 * @return void.
 */
void le_adv_cmd_handler(uint16_t opcode , uint16_t len , void *cmd);


#endif
