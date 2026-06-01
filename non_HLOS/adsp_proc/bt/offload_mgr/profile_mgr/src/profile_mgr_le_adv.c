/*
 * Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      profile_mgr_le_adv.c
===========================================================================*/
/**
 * @file profile_mgr_le_adv.c
 * @brief Handles adv communication with Microstack.
 *
 * @details This file primarily handles:
 *          1. LE ADV profile communication with Microstack.
 *             - Start adv
 *             - Stop adv
 *             - Set adv Data
 */

/*============================================================================*
                                INCLUDE FILES
*============================================================================*/
#include "profile_mgr_le_adv.h"
#include <stringl.h>
#include "bt_pal_heap.h"

/*============================================================================*
                                MACRO DEFINITIONS
*============================================================================*/

/**
 * @brief Maximum number of advertising sets supported.
 */
#define MAX_ADV_SETS_SUPPORTED 0x02

/**
 * @brief Enables LE advertising.
 */
#define LE_ADV_ENABLE 0x01

/**
 * @brief Disables LE advertising.
 */
#define LE_ADV_DISABLE 0x00


/**
 * @brief UUID for Q6 LE advertising service.
 */
#define Q6_LE_ADV_SERVICE_UUID 0xFF05

/**
 * @brief AD Type for complete List of UUID16.
 */
#define AD_TYPE_COMPLETE_LIST_UUID16     0x03

/**
 * @brief AD Type for service data of type UUID16.
 */
#define AD_TYPE_SERVICE_DATA_UUID16      0x16

/**
 * @brief AD Type for manufacturer data.
 */
#define AD_TYPE_MANUFACTURER_DATA        0xFF

/**
 * @brief AD Type for Local Name.
 */
#define AD_TYPE_LOCAL_NAME               0x09 

/**
 * @brief AD Type for tx power.
 */
#define AD_TYPE_TX_POWER                 0x0A

/**
 * @brief AD Type Length for Service UUID.
 */
#define AD_LENGTH_COMPLETE_LIST_UUID16   3

/**
 * @brief AD Type Length for Service UUID.
 */
#define AD_LENGTH_SERVICE_DATA_UUID16    3

/**
 * @brief AD Type Length for Service UUID.
 */
#define AD_LENGTH_MANUFACTURER_DATA      3

/**
 * @brief MAX ADV DATA BLOCK PTRS
 */
#define MAX_ADV_DATA_PTRS       8

/**
 * @brief MAX ADV DATA BLOCK LENGTH
 */
#define MAX_ADV_DATA_BLOCK_LEN       32



/*============================================================================*
                            GLOBAL DATA DECLARATIONS
*============================================================================*/
static uint16_t advAppHandle = 0x00;

/*! Global Structure to Store Current Advertisement Request */
advAppCurrReq_t advCurrReq;

/*! GLobal Structure for Storing context info related to Advertisement Data Train */
static adv_data_train_info_t data_train_inst;

/*! Global Structure to Store Advertisement Context */
le_adv_context_t adv_ctx[MAX_ADV_SETS_SUPPORTED];

/*! Global Pointer to Set Advertisment Data */
uint8_t* le_adv_data[MAX_ADV_DATA_PTRS] = {(uint8_t*)NULL};




/*============================================================================*
                                FUNCTION DEFINITIONS
*============================================================================*/

/**
 * @brief Method to append byte value to an extended advertisement packet
 * @param[in] byte_value  Byte value to be added
 * @return None
 */
static void append_byte_data_train(uint8_t byte_value)
{
	if(data_train_inst.current_adv_block_idx != 31)
	{
		le_adv_data[data_train_inst.current_adv_block][data_train_inst.current_adv_block_idx] = byte_value;
		data_train_inst.current_adv_block_idx++;
	}
	else
	{
		le_adv_data[data_train_inst.current_adv_block][data_train_inst.current_adv_block_idx] = byte_value;
		data_train_inst.current_adv_block++;
		data_train_inst.current_adv_block_idx = 0;
	}
}

/**
 * @brief Method to append stream of bytes to an extended advertisement packet
 * @param[in] raw_bytes_len  Length of data which needs to be appended
 * @param[in] raw_bytes      Pointer to Raw Bytes which needs to be appended to extended advertisement packet
 * @return  None
 */
static void adv_pkt_append_raw_bytes(uint8_t raw_bytes_len , uint8_t* raw_bytes)
{
	uint8_t remaining_pkt_size = raw_bytes_len;
	uint8_t current_raw_bytes_idx = 0;
	while(remaining_pkt_size)
    {
        if(remaining_pkt_size > 32-data_train_inst.current_adv_block_idx)
        {
            for(int i = data_train_inst.current_adv_block_idx ; i<32;i++)
            {
                le_adv_data[data_train_inst.current_adv_block][i] = raw_bytes[current_raw_bytes_idx];
                OFFLOAD_MGR_LOGL("adv_train value is 0x%x",le_adv_data[data_train_inst.current_adv_block][i]);
                current_raw_bytes_idx++;
            }
            data_train_inst.current_adv_block++;
            remaining_pkt_size= remaining_pkt_size - 32 + data_train_inst.current_adv_block_idx;
            data_train_inst.current_adv_block_idx = 0;
        }
        else
        {
            for(int i = data_train_inst.current_adv_block_idx ; i<data_train_inst.current_adv_block_idx+remaining_pkt_size;i++)
            {
                le_adv_data[data_train_inst.current_adv_block][i] = raw_bytes[current_raw_bytes_idx];
                OFFLOAD_MGR_LOGL("adv_train value is 0x%x",le_adv_data[data_train_inst.current_adv_block][i]);
                current_raw_bytes_idx++;
            }
            data_train_inst.current_adv_block_idx+=remaining_pkt_size;
            remaining_pkt_size = 0;
        }
    }
}


/**
 * @brief Method to append AD Type SERVICE UUID to Extended Advertisement Packet
 * @param[in] service_uuid  Service UUID to be added
 * @return  None
 */
static void adv_pkt_append_service_uuid(uint16_t service_uuid)
{
	append_byte_data_train(AD_LENGTH_COMPLETE_LIST_UUID16);
	append_byte_data_train(AD_TYPE_COMPLETE_LIST_UUID16);
	append_byte_data_train((service_uuid)%(service_uuid >> 8));
	append_byte_data_train(service_uuid >> 8);
}


/**
 * @brief Method to append AD Type Local Name to Extended Advertisement Packet
 * @param[in] local_name_len  Local Name length
 * @param[in] local_name      pointer to local name
 * @return  None
 */
static void adv_pkt_append_local_name(uint16_t local_name_len , uint8 *local_name)
{
    append_byte_data_train(local_name_len + 1);
	append_byte_data_train(AD_TYPE_LOCAL_NAME);
    adv_pkt_append_raw_bytes(local_name_len,local_name);

}


/**
 * @brief Sets the advertisement data for a given application handle and advertising handle.
 * @param setAdvData_t Pointer to Data to be set.
 * @return void.
 */
void le_adv_set_advertisement_data(setAdvData_t *advData)
{
    bool mem_allocation = true;
    uint8_t adv_blk_idx = 0;
    uint16_t total_len = 0;
    
    total_len += (advData->local_name_len + 2);

    total_len += AD_LENGTH_COMPLETE_LIST_UUID16 + 1;

    data_train_inst.num_adv_blocks = (total_len/32)+((total_len%32)?1:0);

    OFFLOAD_MGR_LOGM("Total Len : %d , Num Blocks : %d\n",total_len,data_train_inst.num_adv_blocks);
    
    /**Array for Storing Pointers to Advertisement Data */
    for(;(adv_blk_idx<data_train_inst.num_adv_blocks && adv_blk_idx<MAX_ADV_DATA_PTRS);adv_blk_idx++)
	{
		le_adv_data[adv_blk_idx] = (uint8_t*)bt_pal_malloc(MAX_ADV_DATA_BLOCK_LEN *sizeof(uint8_t));
        if(le_adv_data[adv_blk_idx] == NULL)
        {
            mem_allocation = false;
            break;
        }
	}

    if(!mem_allocation)
    {
         for(int i=0;(i<adv_blk_idx);i++)
        {
            bt_pal_free(le_adv_data[i]);
        }
        endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
        setAdvDataCfm_t *req = (setAdvDataCfm_t *)header->data;	
        uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(setAdvDataCfm_t);		
        header->opcode = LE_ADV_SET_ADV_DATA_CFM;
        header->data_len = sizeof(setAdvDataCfm_t) ;
        header->endpoint_id = advAppHandle;
        header->command_type = NONOFFLOAD_APP_CMD;

        /* msg */
        req->advHandle = advData->advHandle;
        req->resultCode = 0x01;
        endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);
    }
    else
    {

        adv_pkt_append_service_uuid(advData->service_uuid);
        adv_pkt_append_local_name(advData->local_name_len,advData->local_name);

        OFFLOAD_MGR_LOGL("Setting Data for Advertisement \n");

        BmmExtAdvSetDataReqSend(advAppHandle,advData->advHandle,3U,1U,total_len,le_adv_data);
    }

}


/**
 * @brief Sets the advertisement data for a given application handle and advertising handle.
 * @param setAdvData_t Pointer to Data to be set.
 * @return void.
 */
void le_adv_set_random_address(setRandomAddress_t *randomAddr)
{
    BmmExtAdvSetRandomAddrReqSend(advAppHandle,
            randomAddr->advHandle,
        randomAddr->type,randomAddr->deviceAddress);
}


/**
 * @brief Callback function for LE advertising events.
 *
 * @param apphandle The application handle.
 * @param eventClass The class of the event.
 * @param message Pointer to the message associated with the event.
 * @return void.
 */
void le_adv_callback(BtAppHandle apphandle, BtEventClass eventClass, void *message)
{
    CsrPrim type = *((CsrPrim* )message);

    switch(type)
    {
        case MS_EXT_ADV_REGISTER_APP_ADV_SET_CFM:
        {
            MsExtAdvRegisterAppAdvSetCfm *adv_set_reg_cfm = (MsExtAdvRegisterAppAdvSetCfm*)message;
            /**Send back corrosponding message back to M55*/
            OFFLOAD_MGR_LOGM("MS_EXT_ADV_REGISTER_APP_ADV_SET_CFM");
            OFFLOAD_MGR_LOGM("Adv Handle : %d , result Code : %d\n",adv_set_reg_cfm->advHandle,adv_set_reg_cfm->resultCode);
            /* send socket open request to microapp */   
            
            endpoint_t ep_id = {
                .hub_id = ENDPT_MGR_HUB_ID_AWM,
                .id     = apphandle, /* random */
            };
			
			endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			registerAdvSetCfm_t *req = (registerAdvSetCfm_t *)header->data;		
            uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(registerAdvSetCfm_t);
			header->opcode = LE_ADV_REGISTER_ADV_SET_CFM;
			header->data_len = sizeof(registerAdvSetCfm_t) ;
			header->endpoint_id = apphandle;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
			req->advHandle = adv_set_reg_cfm->advHandle;
			req->resultCode = adv_set_reg_cfm->resultCode;
			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);


        }
        break;

        case MS_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM:
        {
            MsExtAdvUnregisterAppAdvSetCfm *adv_set_unreg_cfm = (MsExtAdvUnregisterAppAdvSetCfm*)message;
            OFFLOAD_MGR_LOGM("MS_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM");
            OFFLOAD_MGR_LOGM("Adv Handle : %d , result Code : %d\n",adv_set_unreg_cfm->advHandle,adv_set_unreg_cfm->resultCode);
            /* send socket open request to microapp */   
            
            endpoint_t ep_id = {
                .hub_id = ENDPT_MGR_HUB_ID_AWM,
                .id     = apphandle, /* random */
            };
			
			endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			unregisterAdvSetCfm *req = (unregisterAdvSetCfm *)header->data;	
            uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(unregisterAdvSetCfm);		
			header->opcode = LE_ADV_UNREGISTER_ADV_SET_CFM;
			header->data_len = sizeof(unregisterAdvSetCfm) ;
			header->endpoint_id = apphandle;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
			req->advHandle = adv_set_unreg_cfm->advHandle;
			req->resultCode = adv_set_unreg_cfm->resultCode;
			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);
        }
        break;

        case MS_EXT_ADV_SET_PARAMS_CFM:
        {
            MsExtAdvSetParamsCfm *adv_set_params_cfm = (MsExtAdvSetParamsCfm*)message;
            OFFLOAD_MGR_LOGM("Set Params result Code : %d for Adv Handle : %d\n",adv_set_params_cfm->resultCode,adv_set_params_cfm->advHandle);

            endpoint_t ep_id = {
                .hub_id = ENDPT_MGR_HUB_ID_AWM,
                .id     = apphandle, /* random */
            };
			
			endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			setAdvParamsCfm_t *req = (setAdvParamsCfm_t *)header->data;	
            uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(setAdvParamsCfm_t);		
			header->opcode = LE_ADV_SET_ADV_PARAMS_CFM;
			header->data_len = sizeof(setAdvParamsCfm_t) ;
			header->endpoint_id = apphandle;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
			req->advHandle = adv_set_params_cfm->advHandle;
            req->resultCode = adv_set_params_cfm->resultCode;
			req->selected_tx_pwr = adv_set_params_cfm->selected_tx_pwr;
			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);

        }
        break;

        case MS_EXT_ADV_SET_DATA_CFM:
        {
            MsExtAdvSetDataCfm *adv_set_data_cfm = (MsExtAdvSetDataCfm*)message;
            OFFLOAD_MGR_LOGM("Result Code for Setting Advertisement Data : %d for Adv Handle : %d \n",adv_set_data_cfm->resultCode,adv_set_data_cfm->resultCode);

            memset(&data_train_inst,0,sizeof(data_train_inst));

            endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			setAdvDataCfm_t *req = (setAdvDataCfm_t *)header->data;	
            uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(setAdvDataCfm_t);		
			header->opcode = LE_ADV_SET_ADV_DATA_CFM;
			header->data_len = sizeof(setAdvDataCfm_t) ;
			header->endpoint_id = apphandle;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
			req->advHandle = adv_set_data_cfm->advHandle;
			req->resultCode = adv_set_data_cfm->resultCode;
			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);

        }
        break;

        case MS_EXT_ADV_SET_RANDOM_ADDR_CFM:
        {
            MsExtAdvSetRandomAddrCfm *set_random_addr_cfm = (MsExtAdvSetRandomAddrCfm*)message;
            OFFLOAD_MGR_LOGM("Result Code for Setting Random Address : %d for Adv Handle : %d \n",set_random_addr_cfm->resultCode,set_random_addr_cfm->advHandle);
            OFFLOAD_MGR_LOGM("Address : lap %x , uap : %x , nap : %x", set_random_addr_cfm->randomAddr.lap,set_random_addr_cfm->randomAddr.uap,set_random_addr_cfm->randomAddr.nap);
            endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			setRandomAddressCfm_t *req = (setRandomAddressCfm_t *)header->data;	
            uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(setRandomAddressCfm_t);		
			header->opcode = LE_ADV_SET_RANDOM_ADDRESS_CFM;
			header->data_len = sizeof(setRandomAddressCfm_t) ;
			header->endpoint_id = apphandle;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
			req->advHandle = set_random_addr_cfm->advHandle;
			req->resultCode = set_random_addr_cfm->resultCode;
            memscpy(&(req->randomAddr),sizeof(req->randomAddr),&(set_random_addr_cfm->randomAddr),sizeof(set_random_addr_cfm->randomAddr));
			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);
        }
        break;

        case MS_EXT_ADV_MULTI_ENABLE_CFM:
        {
            MsExtAdvMultiEnableCfm *adv_enable_cfm = (MsExtAdvMultiEnableCfm*)message;
            /**Send the corrosponding message back to M55*/
            OFFLOAD_MGR_LOGM("MS_EXT_ADV_MULTI_ENABLE_CFM");
            OFFLOAD_MGR_LOGM("Result Code : %d , Adv Bits : %d\n",adv_enable_cfm->resultCode,adv_enable_cfm->advBits);

            endpoint_t ep_id = {
                .hub_id = ENDPT_MGR_HUB_ID_AWM,
                .id     = apphandle, /* random */
            };
			
			endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			startAdvCfm_t *req = (startAdvCfm_t *)header->data;	
            uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(startAdvCfm_t);
            if(adv_enable_cfm->advBits & (1 << advCurrReq.advHandle))
            {
                header->opcode = LE_ADV_START_ADVERTISEMENT_CFM;   
            }
            else
            {
                header->opcode = LE_ADV_STOP_ADVERTISEMENT_CFM;
            }
			header->data_len = sizeof(startAdvCfm_t);
			header->endpoint_id = apphandle;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
            req->advHandle = advCurrReq.advHandle;
            req->resultCode = adv_enable_cfm->resultCode;

			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);
            
        }
        break;

        default:
            break;
    }


}

/**
 * @brief Sets the advertisement Parameters for a given application handle and advertising handle.
 * @param advParams Pointer to Advertisement Parameters to be set.
 * @return void.
 */
void le_adv_handle_set_adv_params(setAdvParams_t *advParams)
{
    CsrUint8 ownAddrType = (advParams->ownAddrType == 1)?1:0;
    TYPED_BD_ADDR_T peerAddr;
    CsrUint8 advFilterPolicy = 0x00;
    CsrUint8 primaryAdvChannelMap = HCI_ULP_ADVERT_CHANNEL_MAX_VALUE;
    CsrUint16 primaryAdvPhy = HCI_ULP_EXT_ADV_1M_PHY;
    CsrUint8 secondaryAdvMaxSkip = 0x00;
    CsrUint16 secondaryAdvPhy = HCI_ULP_EXT_ADV_1M_PHY;
    CsrUint16 advSid = (advParams->advHandle - 0x02);
    peerAddr.type = 0x00;
    peerAddr.addr.lap = 0x000000;
    peerAddr.addr.nap = 0x00;
    peerAddr.addr.uap = 0x0000;
    
    BmmExtAdvSetParamsReqSend(advAppHandle,
                              advParams->advHandle,
                              advParams->advEventProperties,
                              advParams->intervalMin, 
                              advParams->intervalMax,
                              primaryAdvChannelMap,
                              ownAddrType,
                              peerAddr,
                              advFilterPolicy,
                              primaryAdvPhy,
                              secondaryAdvMaxSkip,
                              secondaryAdvPhy,
                              advSid,
                              advParams->advTxPower,
                              0x00,
                              0x00,
                              0x00,
                              0x00);  
}


/**
 * @brief Handles the start of an advertisement.
 * @param cmd Pointer to the command structure.
 * @return void.
 */
void le_adv_handle_stop_advertisement(void *cmd)
{
    disbaleAdvertisment_t *stop_advertisement = (disbaleAdvertisment_t*)cmd;
    AdvEnableConfig config_enable[1];
    config_enable[0].advHandle = stop_advertisement->advHandle;
    config_enable[0].duration = 0x00;
    config_enable[0].maxEaEvents = 0x01;
    /**Enable Advertisement */
    BmmExtAdvMultiEnableReqSend(advAppHandle,
                                LE_ADV_DISABLE,
                                1,
                                config_enable);
}




/**
 * @brief Handles LE advertising commands from AWM.
 * This function processes advertisement commands based on the provided opcode and length.
 * @param opcode The operation code of the command.
 * @param len The length of the command.
 * @param cmd Pointer to the command structure.
 * @return void.
 */
void le_adv_cmd_handler(uint16_t opcode , uint16_t len , void *cmd)
{
    switch(opcode)
    {
        case LE_ADV_START_APP:
        {
            /**Register an App with Host with given Callback */
            uint16_t appHandle = ((startAdvApp_t*)cmd)->appHandle;
            bool rc = microstack_register_app_callback(appHandle,le_adv_callback);
            OFFLOAD_MGR_LOGM("rc for START_ADV_APP : %d", rc);
            endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			startAdvAppRsp_t *rsp = (startAdvAppRsp_t *)header->data;	
            uint16_t total_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(startAdvAppRsp_t);	
			header->opcode = LE_ADV_START_APP_RSP;
			header->data_len = sizeof(startAdvAppRsp_t);
			header->endpoint_id = appHandle;
			header->command_type = NONOFFLOAD_APP_CMD;

			/* msg */
            rsp->appHandle = appHandle;
            rsp->status = rc;

			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, total_len);

            advAppHandle = (rc == true ) ? appHandle : 0x00;

        }
        break;
        case LE_ADV_REGISTER_ADV_SET:
        {
            registerAdvSet_t *reg_adv_set = (registerAdvSet_t*)cmd;
            OFFLOAD_MGR_LOGM("\n REGISTER_ADV_SET : App Handle : %d",reg_adv_set->appHandle);
             BmmExtAdvRegisterAppAdvSetReqSend(advAppHandle);
        }
        break;

        case LE_ADV_UNREGISTER_ADV_SET:
        {
            unregisterAdvSet *unreg_adv_set = (unregisterAdvSet*)cmd;
            OFFLOAD_MGR_LOGM("Unregister Advertisement Set\n");
            BmmExtAdvUnregisterAppAdvSetReqSend(unreg_adv_set->appHandle,unreg_adv_set->advHandle);
        }
        break;

        case LE_ADV_SET_ADV_PARAMS:
        {
            setAdvParams_t *set_adv_params = (setAdvParams_t*)cmd;
            OFFLOAD_MGR_LOGM("START_ADV : APP Handle : %x , ADV Handle : %d ADVEVENTPROPERTIES : %d \n",
                             set_adv_params->appHandle,set_adv_params->advHandle,set_adv_params->advEventProperties);
			OFFLOAD_MGR_LOGM("Min Interval : %d , Max Interval : %d , Tx Power : %d \n",set_adv_params->intervalMin,set_adv_params->intervalMax,set_adv_params->advTxPower);
            le_adv_handle_set_adv_params(cmd);

        }
        break;

        case LE_ADV_SET_ADV_DATA:
        {
            setAdvData_t *set_adv_data = (setAdvData_t*)cmd;
            OFFLOAD_MGR_LOGM("UUID : %x",set_adv_data->service_uuid);
            OFFLOAD_MGR_LOGM("Local Name Length : %d",set_adv_data->local_name_len);

            le_adv_set_advertisement_data(set_adv_data);
        }
        break;

        case LE_ADV_SET_RANDOM_ADDRESS:
        {
            setRandomAddress_t *randomAddr = (setRandomAddress_t*)cmd;
            le_adv_set_random_address(randomAddr);
        }
        break;
        
        case LE_ADV_START_ADVERTISEMENT:
        {
            enableAdvertisement_t *start_adv = (enableAdvertisement_t*)(cmd);
            advCurrReq.advHandle = start_adv->advHandle;
            advCurrReq.currReq = LE_ADV_START_ADVERTISEMENT;

            AdvEnableConfig config_enable[1];
                config_enable[0].advHandle = start_adv->advHandle;
                config_enable[0].duration = 0x00;
                config_enable[0].maxEaEvents = 0x00;
                /**Enable Advertisement */
                BmmExtAdvMultiEnableReqSend(advAppHandle,
                                            LE_ADV_ENABLE,
                                            1,
                                            config_enable);


        }
        break;

        case LE_ADV_STOP_ADVERTISEMENT:
        {
            disbaleAdvertisment_t *stop_adv = (disbaleAdvertisment_t*)cmd;
            OFFLOAD_MGR_LOGM("STOP_ADV : APP Handle : %x , ADV Handle : %d\n",stop_adv->appHandle,stop_adv->appHandle);
            advCurrReq.advHandle = stop_adv->advHandle;
            advCurrReq.currReq = LE_ADV_STOP_ADVERTISEMENT;
            le_adv_handle_stop_advertisement(cmd);            
        }
        break;

        case LE_ADV_STOP_APP:
        {
            uint16_t appHandle = *((uint16_t*)cmd);
            bool rc  = microstack_deregister_app_callback(appHandle);
            OFFLOAD_MGR_LOGL("Response for Stop Le ADV App : %d\n",rc);
        }
        break;

        default:
            OFFLOAD_MGR_LOGE("Unhandled Event Received : %d\n",opcode);
            break;
    }
}
