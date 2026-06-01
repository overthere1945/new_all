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
#include "lpai_bt_le_adv.h"
#include <zephyr/sys/printk.h>
#include "qapi_bt_le_adv.h"
#include "stringl.h"


/*===========================================================================
                        MACRO DEFINITIONS
===========================================================================*/
#define ADV_APP_HANDLE 0x9054

#define ADV_APP_CMD_RESULT_SUCCESS 0x00

/*===========================================================================
                        PUBLIC/GLOBAL DATA DEFINITIONS
===========================================================================*/
advAppCtx_t leAdvCtx;

/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/
/**
 * @brief Initialization Function for LE Scan App
 * @param[in] None
 * @return    None
 */
void le_adv_init()
{
    memset(&leAdvCtx,0,sizeof(leAdvCtx));
}


/**
 * @brief Initialization Function for LE Scan App
 * @param[in] None
 * @return    None
 */
void le_adv_deinit()
{
    lpai_bt_app_mgr_unregister_microapp(ADV_APP_HANDLE);
    memset(&leAdvCtx,0,sizeof(leAdvCtx));
}

/**
 * @brief Static Method to check if scan app is activated
 * @param[in] None
 * @return    None
 */
static bool get_adv_app_status()
{
    return leAdvCtx.isActive;
}


/**
 * @brief Static Method to find index for storing adv set handle
 * @param[in] None
 * @return    None
 */
static uint8_t find_available_advertiser_idx()
{
    uint8_t idx = 0;
    for(idx = 0; idx < MAX_ADVERTISERS_SUPPORTED; idx++)
    {
        if(leAdvCtx.advCtx[idx].idxInUse == false)
        {
            return idx;
        }
    }
    return idx;
}

/**
 * @brief Static Method to find index for a given adv set handle in scanners context information
 * @param[in] advHandle adv set Handle for which index is required
 * @return    idx       Index where adv set Handle is stored , Invalid value otherwise
 */
static uint8_t find_advertiser_idx(uint8_t advHandle)
{
    uint8_t idx;
    for(idx = 0; idx < MAX_ADVERTISERS_SUPPORTED; idx++)
    {
        if(leAdvCtx.advCtx[idx].advHandle == advHandle)
        {
            return idx;
        }
    }
    return idx;
}


/**
 * @brief This method is used register LE Advertisement APP on ADSP. On successful registartion app is activated and ready for use on AWM
 * @param[in] None
 * @return    None
 */
void register_le_advApp()
{
    app_registration_status_code_t result = lpai_bt_app_mgr_register_microapp(ADV_APP_HANDLE,leAdvCb);

    uint16_t appHandle = ADV_APP_HANDLE;
    printk("App Registration Status : %d\n",result);
    if(leAdvCtx.isActive == false)
    {
        lpai_bt_appmgr_send_microapp_msg_adsp(START_ADV_APP , sizeof(appHandle) , &appHandle);
    }
    else
    {
        printk("Le Adv App Already Active");
    }
}


/**
 * @brief This method is used unregister LE Advertisement APP on ADSP. On successful unregistartion app is deactivated and can be only used once activated again
 * @param[in] None
 * @return    None
 */
void unregister_le_advApp()
{
    uint16_t appHandle = ADV_APP_HANDLE;
    if(leAdvCtx.isActive == true)
    {
        lpai_bt_app_mgr_unregister_microapp(ADV_APP_HANDLE);
        lpai_bt_appmgr_send_microapp_msg_adsp(STOP_LE_ADV_APP , sizeof(appHandle) , &appHandle );
        le_adv_init();
    }
    else
    {
        printk("Le Adv App Not Active Yet !");
    }
}


/**
 * @brief This method is used register an Advetisement Set on ADSP
 * @param[in] None
 * @return    None
 */
void qapi_register_advSet()
{
    if(get_adv_app_status())
    {
        registerAdvSet_t regAdvSet ;
        regAdvSet.appHandle = ADV_APP_HANDLE;

        lpai_bt_appmgr_send_microapp_msg_adsp(REGISTER_ADV_SET , sizeof(registerAdvSet_t) , &regAdvSet);
    }
    else
    {
        printk("Advertiser App not active yet . Please Start the App ! \n");
    }
}

/**
 * @brief This method is used register an Advetisement Set on ADSP
 * @param[in] advHandle Advertisement Handle which needs to be unregistered
 * @return    None
 */
void qapi_unregister_advSet(uint8_t advHandle)
{
    if(get_adv_app_status())
    {
        unregisterAdvSet unregAdvSet;
        uint8_t advSetIdx = find_advertiser_idx(advHandle);
        if(advSetIdx != MAX_ADVERTISERS_SUPPORTED)
        {
            unregAdvSet.advHandle = advHandle;
            unregAdvSet.appHandle = ADV_APP_HANDLE;

            lpai_bt_appmgr_send_microapp_msg_adsp(UNREGISTER_ADV_SET , sizeof(unregisterAdvSet) , &unregAdvSet);
        }
        else
        {
            printk("Invalid Advertiser Handle Recieved \n");
        }
    }
    else
    {
        printk("Advertiser App not active yet . Please Start the App ! \n");
    }
}


/**
 * @brief This method is used to set advertisement params for a registered adv set
 * @param[in] advParams Advertisment Parameters Structure whith values for adv params
 * @return    None
 */
void qapi_set_advParams(setAdvParams_t setAdvParams)
{
    if(get_adv_app_status())
    {
        setAdvParams_t advParams;
        uint8_t advSetIdx = find_advertiser_idx(setAdvParams.advHandle);
        if(advSetIdx != MAX_ADVERTISERS_SUPPORTED)
        {
            memscpy(&advParams,sizeof(advParams),&setAdvParams,sizeof(setAdvParams));
            advParams.appHandle = ADV_APP_HANDLE;
            lpai_bt_appmgr_send_microapp_msg_adsp(SET_ADV_PARAMS , sizeof(setAdvParams_t) , &advParams);
        }
        else
        {
            printk("Invalid Advertiser Handle Recieved \n");
        }
    }
    else
    {
        printk("Advertiser App not active yet . Please Start the App ! \n");
    }
}


/**
 * @brief This method is used to set advertisement params for a registered adv set
 * @param[in] advData  Advertisement Data which is to be set for a particular advertisement set
 * @return    None
 */
void qapi_set_advData(setAdvData_t setAdvData)
{
    if(get_adv_app_status())
    {
        setAdvData_t advData;
        uint8_t advSetIdx = find_advertiser_idx(setAdvData.advHandle);
        if(advSetIdx != MAX_ADVERTISERS_SUPPORTED)
        {
            memscpy(&advData,sizeof(setAdvData_t),&setAdvData,sizeof(setAdvData_t));
            advData.local_name_len = setAdvData.local_name_len;
            printk("Size of Local Name : %d\n",advData.local_name_len);
            lpai_bt_appmgr_send_microapp_msg_adsp(SET_ADV_DATA , sizeof(setAdvData_t) , &advData);
        }
        else
        {
            printk("Invalid Advertiser Handle Recieved \n");
        }
    }
    else
    {
        printk("Advertiser App not active yet . Please Start the App ! \n");
    }
}


/**
 * @brief This method is used to set random address for an registered adv set
 * @param[in] advData  Advertisement Data which is to be set for a particular advertisement set
 * @return    None
 */
void qapi_set_random_address(setRandomAddress_t setrandomAddr)
{
    if(get_adv_app_status())
    {
        setRandomAddress_t randomAddr;
        uint8_t advSetIdx = find_advertiser_idx(setrandomAddr.advHandle);
        if(advSetIdx != MAX_ADVERTISERS_SUPPORTED)
        {
            memscpy(&randomAddr,sizeof(setRandomAddress_t),&setrandomAddr,sizeof(setRandomAddress_t));
            randomAddr.appHandle = ADV_APP_HANDLE;
            lpai_bt_appmgr_send_microapp_msg_adsp(SET_RANDOM_ADDRESS , sizeof(setRandomAddress_t) , &randomAddr);
        }
        else
        {
            printk("Invalid Advertiser Handle Recieved \n");
        }
    }
    else
    {
        printk("Advertiser App not active yet . Please Start the App ! \n");
    }
}

/**
 * @brief This method is used to enable advertising for an advertisment set
 * @param[in] advHandle   Advertisement Handle for which advertisement is to be enabled
 * @return    None
 */
void qapi_enable_avertisement(uint8_t advHandle)
{
    if(get_adv_app_status())
    {
        enableAdvertisement_t startAdv;
        uint8_t advSetIdx = find_advertiser_idx(advHandle);
        if(advSetIdx != MAX_ADVERTISERS_SUPPORTED)
        {
            startAdv.advHandle = advHandle;
            startAdv.appHandle = ADV_APP_HANDLE;

            lpai_bt_appmgr_send_microapp_msg_adsp(START_ADVERTISEMENT , sizeof(enableAdvertisement_t) , &startAdv);
        }
        else
        {
            printk("Invalid Advertiser Handle Recieved \n");
        }
    }
    else
    {
        printk("Advertiser App not active yet . Please Start the App ! \n");
    }
}

/**
 * @brief This method is used to disbale advertising for an advertisment set
 * @param[in] advHandle   Advertisement Handle for which advertisement is to be disbaled
 * @return    None
 */
void qapi_disbale_advertisment(uint8_t advHandle)
{
    if(get_adv_app_status())
    {
        disbaleAdvertisment_t stopAdv;
        uint8_t advSetIdx = find_advertiser_idx(advHandle);
        if(advSetIdx != MAX_ADVERTISERS_SUPPORTED)
        {
            stopAdv.advHandle = advHandle;
            stopAdv.appHandle = ADV_APP_HANDLE;

            lpai_bt_appmgr_send_microapp_msg_adsp(STOP_ADVERTISEMENT , sizeof(disbaleAdvertisment_t) , &stopAdv);
        }
        else
        {
            printk("Invalid Advertiser Handle Recieved \n");
        }
    }
    else
    {
        printk("Advertiser App not active yet . Please Start the App ! \n");
    }
}


/**
 * @brief Le Advertisement Callback Function registered with App Manager on AWM
 * This function is invoked each time a advetisement event is received on AWM from ADSP
 * @param[in] eventId         Event Identifier to specify type of event
 * @param[in] appDataLen      Length of the Data received along with the event
 * @param[in] appData         Pointer to actual data specific for each event
 * @param[in] proto_encoded   Bool to specifiy if data is proto encoded or not
 * @return    None
 */
void leAdvCb(uint16_t eventId , uint16_t appDataLen , void *appData,bool proto_encoded)
{
    printk("Event Id : %d\n",eventId);
    switch(eventId)
    {
        case START_ADV_APP_RSP:
        {
            printk("App Started Event Received\n");
            startAdvAppRsp_t *rsp = (startAdvAppRsp_t*)appData;
            if(rsp->status == true)
            {
                leAdvCtx.appHandle = rsp->appHandle;
                leAdvCtx.isActive = true;
            }

        }
        break; 

        case REGISTER_ADV_SET_CFM:
        {
            registerAdvSetCfm_t *cfm = (registerAdvSetCfm_t*)appData;
            uint8_t advSetIdx = find_available_advertiser_idx();
            if(advSetIdx != MAX_ADVERTISERS_SUPPORTED && cfm->resultCode == ADV_APP_CMD_RESULT_SUCCESS)
            {
                leAdvCtx.advCtx[advSetIdx].advHandle = cfm->advHandle;
                leAdvCtx.advCtx[advSetIdx].idxInUse = true;
                qapi_registerAdvSetCfm_t regAdvSetCfm = {.resultCode = cfm->resultCode,
                                                            .advHandle = cfm->advHandle
                                                        };
                qapi_leadv_evt_handler(QAPI_REGISTER_ADV_SET_CFM,sizeof(regAdvSetCfm),&regAdvSetCfm);
            }
            else if(advSetIdx == MAX_ADVERTISERS_SUPPORTED)
            {
                printk("No More Advertisers Can be registered as limit reached");
            }
        }   
        break;
        
        case UNREGISTER_ADV_SET_CFM:
        {
            unregisterAdvSetCfm *unreg_cfm = (unregisterAdvSetCfm*)appData;
            uint8_t advSetIdx = find_advertiser_idx(unreg_cfm->advHandle);
            if(advSetIdx != MAX_ADVERTISERS_SUPPORTED)
            {
                leAdvCtx.advCtx[advSetIdx].advHandle = 0x00;
                leAdvCtx.advCtx[advSetIdx].idxInUse = false;
                qapi_unregisterAdvSetCfm unregAdvSetCfm = {.resultCode = unreg_cfm->resultCode,
                                                            .advHandle = unreg_cfm->advHandle
                                                        };
                qapi_leadv_evt_handler(QAPI_UNREGISTER_ADV_SET_CFM,sizeof(unregAdvSetCfm),&unregAdvSetCfm);
            }
            else
            {
                printk("Invalid Advertisement Handle Received for Unregistration\n");
            }
        }
        break;

        case SET_ADV_PARAMS_CFM:
        {
            setAdvParamsCfm_t *set_params_cfm = (setAdvParamsCfm_t*)appData;
            qapi_setAdvParamsCfm_t qapi_set_params_cfm = {.advHandle = set_params_cfm->advHandle, .resultCode = set_params_cfm->resultCode ,.selected_tx_pwr=set_params_cfm->selected_tx_pwr};
            qapi_leadv_evt_handler(QAPI_SET_ADV_PARAMS_CFM,sizeof(qapi_set_params_cfm),&qapi_set_params_cfm);

        }
        break;

        case SET_ADV_DATA_CFM:
        {
            setAdvDataCfm_t *set_data_cfm = (setAdvDataCfm_t*)appData;
            qapi_setAdvDataCfm_t qapi_set_data_cfm = {.advHandle = set_data_cfm->advHandle, .resultCode = set_data_cfm->resultCode};
            qapi_leadv_evt_handler(QAPI_SET_ADV_DATA_CFM,sizeof(qapi_set_data_cfm),&qapi_set_data_cfm);
        }
        break;

        case SET_RANDOM_ADDRESS_CFM:
        {
            setRandomAddressCfm_t *set_random_addr_cfm = (setRandomAddressCfm_t*)appData;
            qapi_setRandomAddressCfm_t qapi_random_addr_cfm ;
            memscpy(&qapi_random_addr_cfm,sizeof(qapi_random_addr_cfm),set_random_addr_cfm,sizeof(qapi_random_addr_cfm));
            qapi_leadv_evt_handler(QAPI_SET_RANDOM_ADDRESS_CFM,sizeof(qapi_random_addr_cfm),&qapi_random_addr_cfm);
        }
        break;

        case START_ADVERTISEMENT_CFM:
        {
            startAdvCfm_t *start_adv_cfm = (startAdvCfm_t*)appData;
            qapi_startAdvCfm_t qapi_start_adv_cfm = {.advHandle = start_adv_cfm->advHandle , .resultCode = start_adv_cfm->resultCode};
            qapi_leadv_evt_handler(QAPI_START_ADVERTISEMENT_CFM,sizeof(qapi_start_adv_cfm),&qapi_start_adv_cfm);
        }
        break;

        case STOP_ADVERTISEMENT_CFM:
        {
            stopAdvCfm_t *stop_adv_cfm = (stopAdvCfm_t*)appData;
            qapi_stopAdvCfm_t qapi_stop_adv_cfm = {.advHandle = stop_adv_cfm->advHandle , .resultCode = stop_adv_cfm->resultCode};
            qapi_leadv_evt_handler(QAPI_STOP_ADVERTISEMENT_CFM,sizeof(qapi_stop_adv_cfm),&qapi_stop_adv_cfm);
        }
        break;
       
        default:
            printk("Unhandled Event \n");
        break;
    }

}

/**
 * @brief Handler Function to Handle LE ADV QAPI EVENTS
 * @param[in] opcode       Opcode to identify Incoming Event
 * @param[in] appDataLen   Data Length of the Incoming Event
 * @param[in] appData      Pointer to Actual Data of the incoming event
 * @return    None
 */
void qapi_leadv_evt_handler(uint16_t opcode,uint16_t appDataLen , void *appData)
{
    switch(opcode)
    {
        case QAPI_REGISTER_ADV_SET_CFM:
        {
            qapi_registerAdvSetCfm_t *qapi_reg_advset_cfm = (qapi_registerAdvSetCfm_t*)appData;
            printk("QAPI REGISTER ADV SET CFM \n");
            printk("Result Code : %d ADV Handle Received : %d \n",qapi_reg_advset_cfm->resultCode , qapi_reg_advset_cfm->advHandle);
        }
        break;

        case QAPI_UNREGISTER_ADV_SET_CFM:
        {
            qapi_unregisterAdvSetCfm *qapi_unreg_adv_cfm = (qapi_unregisterAdvSetCfm*)appData;
            printk("QAPI UNREGISTER ADV SET CFM \n");
            printk("Result Code : %d ADV Handle Received : %d \n",qapi_unreg_adv_cfm->resultCode , qapi_unreg_adv_cfm->advHandle);
        }
        break;

        case QAPI_SET_ADV_PARAMS_CFM:
        {
            qapi_setAdvParamsCfm_t *qapi_set_params_cfm = (qapi_setAdvParamsCfm_t*)appData;
            printk("QAPI SET ADV PARAMS CFM \n");
            printk("Adv Handle : %d , Result Code : %d , Tx Power : %d \n",qapi_set_params_cfm->advHandle,qapi_set_params_cfm->resultCode,qapi_set_params_cfm->selected_tx_pwr);
        }
        break;

        case QAPI_SET_ADV_DATA_CFM:
        {
            qapi_setAdvDataCfm_t *qapi_set_data_cfm = (qapi_setAdvDataCfm_t*)appData;
            printk("QAPI SET ADV DATA CFM \n");
            printk("Adv Handle : %d , Result Code : %d\n",qapi_set_data_cfm->advHandle,qapi_set_data_cfm->resultCode);
        }
        break;

        case QAPI_SET_RANDOM_ADDRESS_CFM:
        {
            qapi_setRandomAddressCfm_t *qapi_set_random_addr_cfm = (qapi_setRandomAddressCfm_t*)appData;
            printk("QAPI SET RANDOM ADDRESS CFM \n");
            printk("Adv Handle : %d , Result Code : %d , Lap : %x , Uap : %x , Nap : %x ",qapi_set_random_addr_cfm->advHandle,
                qapi_set_random_addr_cfm->resultCode,qapi_set_random_addr_cfm->randomAddr.lap,qapi_set_random_addr_cfm->randomAddr.uap,qapi_set_random_addr_cfm->randomAddr.nap);
			
        }
        break;

        case QAPI_START_ADVERTISEMENT_CFM:
        {
            qapi_startAdvCfm_t *qap_start_adv = (qapi_startAdvCfm_t*)appData;
            printk("QAPI START ADVERTISEMENT CFM \n");
            printk("Advertiment Enable for Adv Handle : %d with result code : %d\n",qap_start_adv->advHandle,qap_start_adv->resultCode);
        }
        break;

        case QAPI_STOP_ADVERTISEMENT_CFM:
        {
            qapi_stopAdvCfm_t *qap_stop_adv = (qapi_stopAdvCfm_t*)appData;
            printk("QAPI STOP ADVERTISEMENT CFM \n");
            printk("Advertiment Disabled for Adv Handle : %d with result code : %d\n",qap_stop_adv->advHandle,qap_stop_adv->resultCode);
        }
        break;

        default:
            break;
    } 
}