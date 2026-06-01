/**************************************************************************
 * @file     lpai_bt_le_adv.h
 * @brief    LPAI BT LE Advertisement header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_LE_ADV_H
#define LPAI_BT_LE_ADV_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lpai_bt_app_mgr_client_interface.h"
#include "lpai_bt_le_scan.h"

/*===========================================================================
                        MACRO DEFINITIONS
===========================================================================*/
#define MAX_ADVERTISERS_SUPPORTED 0x02



/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

/**
 * @brief Advertiser Info Context Maintained on AWM
 * @param idxInUse  Bool to identify if index is in use
 * @param advHandle Advertisement Set Handle
 */
typedef struct{
    bool idxInUse;
    uint8_t advHandle;
}leAdvertiserCtx_t;



/**
 * @brief Scanner App Context Information along with context information for  Scanners Supported
 * @param isActive  Bool to specify if app is active
 * @param appHandle Application Handle Stored
 * @param advCtx    Advertisement Set Context
 */
typedef struct advAppCtx{
    bool isActive;
    uint16_t appHandle;
    leAdvertiserCtx_t advCtx[MAX_ADVERTISERS_SUPPORTED];
}advAppCtx_t;


/**
 * @enum le_adv_cmds_t
 * LE ADV Commands and Response for Communication between AAWM adn ADSP
 */
typedef enum{
    START_ADV_APP = 0x5000,
    REGISTER_ADV_SET ,
    UNREGISTER_ADV_SET ,
    SET_ADV_PARAMS,
    SET_ADV_DATA,
    SET_RANDOM_ADDRESS,
    START_ADVERTISEMENT ,
    STOP_ADVERTISEMENT,
    STOP_LE_ADV_APP,
    START_ADV_APP_RSP,
    REGISTER_ADV_SET_CFM,
    UNREGISTER_ADV_SET_CFM,
    SET_ADV_PARAMS_CFM,
    SET_ADV_DATA_CFM,
    SET_RANDOM_ADDRESS_CFM,
    START_ADVERTISEMENT_CFM,
    STOP_ADVERTISEMENT_CFM,
    ADV_CMD_MAX,
}le_adv_cmds_t;


/**
 * @brief Structure to Register an advertising set.
 * @param appHandle Application Handle
 */
typedef struct registerAdvSet{
    uint16_t appHandle;
}registerAdvSet_t;

/**
 * @brief Structure to UnRegister an advertising set.
 * @param appHandle Application Handle
 * @param advHandle Registered Advertisement Handle to be unregistered
 */
typedef struct unregisterAdvSet{
    uint16_t appHandle;
    uint8_t advHandle;
}unregisterAdvSet;
 

/**
 * @brief Structure to Enable Advertisment for an advertising set.
 * @param appHandle Application Handle
 * @param advHandle Registered Advertisement Handle for which advertisement is to be enabled
 */
typedef struct enableAdvertisement{
    uint16_t appHandle;
    uint8_t advHandle;
}enableAdvertisement_t;
 

/**
 * @brief Structure to Disable Advertisment for an advertising set.
 * @param appHandle Application Handle
 * @param advHandle Registered Advertisement Handle for which advertisement is to be disabled
 */
typedef struct disbaleAdvertisment{
    uint16_t appHandle;
    uint8_t advHandle;
}disbaleAdvertisment_t;


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
 * @brief Structure to Set Advertisement Parameters for an Registered Advertisement Set
 * @param appHandle          Application Handle
 * @param advHandle          Advertisement Set Handle for which parameters are set
 * @param advEventProperties Advertisement Type
 * @param intervalMin        Minimum Interval Parameter
 * @param intervalMax        Maximum Interval Parameter
 * @param advTxPower         Tx Power Parameter
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
 * @brief Structure to set Random Address for an Advertisement Set.
 * @param appHandle Application handle
 * @param advHandle Advertisement Set Handle for which address is set
 * @param deviceAddress Device Address to be Set
 */
typedef struct setRandomAddress{
    uint16_t appHandle;
    uint8_t advHandle;
    uint8_t type;
    bd_addr_t deviceAddress;
}setRandomAddress_t;


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
 * @brief Structure to confirm the setting of advertisement data.
 * @param resultCode Result code for setting of advertisement data for an advertising set
 * @param advHandle Advertising handle
 */
typedef struct setAdvDataCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}setAdvDataCfm_t;


/**
 * @brief Structure to confirm setting of Random Address for an Advertisement Set.
 * @param resultCode Result code for unregistration of an advertising set
 * @param advHandle  Advertising handle
 * @param deviceAddress Device Address which is set
 */
typedef struct setRandomAddressCfm{
    uint16_t  resultCode;
    uint8_t   advHandle;
    bd_addr_t  randomAddr;
}setRandomAddressCfm_t;


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

/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/

/**
 * @brief Initialization Function for LE Scan App
 * @param[in] None
 * @return    None
 */
void le_adv_init();


/**
 * @brief Initialization Function for LE Scan App
 * @param[in] None
 * @return    None
 */
void le_adv_deinit();
 

/**
 * @brief Le Advertisement Callback Function registered with App Manager on AWM
 * This function is invoked each time a advetisement event is received on AWM from ADSP
 * @param[in] eventId         Event Identifier to specify type of event
 * @param[in] appDataLen      Length of the Data received along with the event
 * @param[in] appData         Pointer to actual data specific for each event
 * @param[in] proto_encoded   Bool to specifiy if data is proto encoded or not
 * @return    None
 */
void leAdvCb(uint16_t eventId , uint16_t appDataLen , void *appData,bool proto_encoded);

/**
 * @brief This method is used register LE Advertisement APP on ADSP. On successful registartion app is activated and ready for use on AWM
 * @param[in] None
 * @return    None
 */
void register_le_advApp(void);

/**
 * @brief This method is used unregister LE Advertisement APP on ADSP. On successful unregistartion app is deactivated and can be only used once activated again
 * @param[in] None
 * @return    None
 */
void unregister_le_advApp(void);

/**
 * @brief This method is used register an Advetisement Set on ADSP
 * @param[in] None
 * @return    None
 */
void register_advSet(void);


/**
 * @brief This method is used register an Advetisement Set on ADSP
 * @param[in] advHandle Advertisement Handle which needs to be unregistered
 * @return    None
 */
void unregister_advSet(uint8_t advHandle);


/**
 * @brief This method is used to set advertisement params for a registered adv set
 * @param[in] advParams Advertisment Parameters Structure whith values for adv params
 * @return    None
 */
void set_advParams(setAdvParams_t advParams);

/**
 * @brief This method is used to set advertisement params for a registered adv set
 * @param[in] advData  Advertisement Data which is to be set for a particular advertisement set
 * @return    None
 */
void set_advData(setAdvData_t advData);


/**
 * @brief This method is used to set random address for an registered adv set
 * @param[in] advData  Advertisement Data which is to be set for a particular advertisement set
 * @return    None
 */
void set_random_address(setRandomAddress_t randomAddr);

/**
 * @brief This method is used to disbale advertising for an advertisment set
 * @param[in] advHandle   Advertisement Handle for which advertisement is to be disbaled
 * @return    None
 */
void disbale_advertisment(uint8_t advHandle);

/**
 * @brief This method is used to enable advertising for an advertisment set
 * @param[in] advHandle   Advertisement Handle for which advertisement is to be enabled
 * @return    None
 */
void enable_avertisement(uint8_t advHandle);



#endif /**LPAI_BT_LE_ADV_H */