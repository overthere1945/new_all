/*=============================================================================
  @file bt_state_mgr.c

  This file contains bt state management implementations of BT PAL.

*******************************************************************************
* Copyright (c) 2012-2025 Qualcomm Technologies, Inc.  All Rights Reserved
* Qualcomm Technologies Confidential and Proprietary.
********************************************************************************/


/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "bt_state_mgr.h"
#include "bt_utils.h"
#include "endpt_mgr_rpc.h"
#include "endpt_mgr.h"
#include "socket_mgr.h"
#include "offload_mgr_client_interface.h"
#include "offload_mgr_thread.h"
#include "offload_mgr_sm.h"
#include "bt_pal_npa.h"
#include "qurt_island.h"
#include "uSleep_mode_trans.h"

/*=============================================================================
                        MACRO DEFINITIONS
=============================================================================*/
/**
 * @brief Using existing handle range from MicroStack to uniquely register the module.
 */
#define BT_STATE_MGR_HNDL                (BT_APP_HANDLE_START+1)

/**
 * @brief Offset value for BT enable mode.
 */
 #define BT_ENABLE_MODE_OFFSET           (6U)

/**
 * @brief Offset value for BT disable mode.
 */
 #define BT_DISABLE_MODE_OFFSET           (6U)

/**
* @brief Length of the BT (enable/disable) response.
*/
#define BT_ENABLE_DISABLE_RSP_LEN (8U)



/*=============================================================================
                    GLOBAL DATA DECLARATIONS
=============================================================================*/
/* TBD: Microstack returns the mode, then this event can be updated properly */
/**
 * @brief Global variable to store BT on mode.
 */
bt_on_mode_t bt_on_mode;

/*===========================================================================
                    LOCAL FUNCTION DEFINITIONS
===========================================================================*/
/**
 * @brief Translates bt_off_mode to BMMStopReason
 * 
 * @param bt_off_mode BT OFF mode configuration value
 * @return corresponding BmmStopReason value
 */
/*===========================================================================
FUNCTION      translate_bt_off_mode
===========================================================================*/
static inline BmmStopReason translate_bt_off_mode(uint8_t bt_off_mode)
{
    BmmStopReason StopReason;   
	if (bt_off_mode == BT_OFF_SSR)
	{
		StopReason =  BMM_STOP_REASON_SSR;
	}
	else if (bt_off_mode == BT_OFF_FMD)
	{
		StopReason =  BMM_STOP_REASON_FMD;
	}
	else /* BT_OFF_USER_OFFLOAD_MODE, BT_OFF_USER_PASSTHROUGH_MODE */
	{
		StopReason =  BMM_STOP_REASON_USER;
	}
    return StopReason;
}

/*===========================================================================
FUNCTION      post_bt_state_to_apss
===========================================================================*/
/**
 * @brief Posts BT state to APSS.
 *
 * @param state The state of BT (enable/disable).
 * @param status The status of the operation.
 */
static void post_bt_state_to_apss(btState_t state, uint8_t status)
{
    if (state == BT_ENABLE)
    {
        /* Opcode for BT-ENABLE */
        offload_mgr_transport_shim_buf[0] = 0x1;
        offload_mgr_transport_shim_buf[1] = 0x0f;
    }
    else // BT_DISABLE
    {
        /* Opcode for BT-DISABLE */
        offload_mgr_transport_shim_buf[0] = 0x2;
        offload_mgr_transport_shim_buf[1] = 0x0f;
    }

    /* Encoded information */
    offload_mgr_transport_shim_buf[2] = 0x1;
    offload_mgr_transport_shim_buf[3] = 0x0;

    /* Length */
    offload_mgr_transport_shim_buf[4] = 0x2;
    offload_mgr_transport_shim_buf[5] = 0x0;

    /* Status */
    offload_mgr_transport_shim_buf[6] = (status & 0xFF);
    offload_mgr_transport_shim_buf[7] = 0x0;

    Offload_Mgr_SendDataToHCI(offload_mgr_transport_shim_buf, BT_ENABLE_DISABLE_RSP_LEN);   
}

/*===========================================================================
FUNCTION      bt_state_mgr_microstack_cb
===========================================================================*/
/**
 * @brief Handles  MicroStack Callback for Init/De-Init of Stack and Arbiter.
 *
 * @param handle The handle for BT application.
 * @param group The event class group.
 * @param message The message received.
 */
static void bt_state_mgr_microstack_cb(BtAppHandle handle, BtEventClass group,
                                    		void *message)
{

    BmmPrim *type = (BmmPrim *)message;
    OFFLOAD_MGR_LOGH("bt_state_mgr_microstack_cb: %u\n", *type);

    switch(*type)
    {
        case BMM_INITIALIZED_IND:
        {
            OFFLOAD_MGR_LOGH("BMM_INITIALIZED_IND\n");

            BT_Offload_HciArbiter_Init();            
			/* 1. enable client interface based on mode */
            offload_mgr_client_interface_init((bool)bt_on_mode);

            if(bt_on_mode == BT_STATE_MGR_OFFLD_MODE)     
            {
				//2. Initalize/Enable State Machine for Socket
	            offload_mgr_sm_init();
                ;//3.Inform MicroAPP if Mode is Offload
                /* header */
                endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
                header->opcode = UAPP_BT_STATUS;
                header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(bt_status_msg_t);
                
                /* message */
                bt_status_msg_t *bt_status_msg = (bt_status_msg_t *)header->data;
                bt_status_msg->bt_status = BT_STATUS_ON;
                /* send message to endpoint */
                endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
            }

            /* TBD: To be posted to apss only on microapps are notified, @rbangar */
            post_bt_state_to_apss(BT_ENABLE, 0 /* Status is success */);
            bt_pal_mgr_allow_island();
            break;
        }
        case BMM_DEINITIALIZED_IND:
        {
            if (bt_on_mode == BT_STATE_MGR_OFFLD_MODE || bt_on_mode == BT_STATE_MGR_PASSTHROUGH_MODE)
            {
                post_bt_state_to_apss(BT_DISABLE, 0 /* Status is success */);
            }

            bt_pal_mgr_allow_island();

            if(bt_on_mode == BT_STATE_MGR_OFFLD_MODE || bt_on_mode == BT_STATE_MGR_OFFLD_MODE_SSR)     
            {
            // 3. Inform MicroAPP if Mode is Offload
                /* header */
                endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
                header->opcode = UAPP_BT_STATUS;
                header->data_len = ENDPT_MGR_RPC_HDR_SIZE + sizeof(bt_status_msg_t);
                
                /* message */
                bt_status_msg_t *bt_status_msg = (bt_status_msg_t *)header->data;
                bt_status_msg->bt_status = BT_STATUS_OFF;
                /* send message to endpoint */
                endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, header->data_len);
				/* deinit offload mgr sm and client interface */
                
                /* state machine is deinitialized at the end
                   to service pending socket close indications */
                offload_mgr_sm_deinit();
            }
			
			offload_mgr_client_interface_deinit(bt_on_mode);
            bt_state_mgr_deinit();

        }
         break;

        case BMM_PREPARE_STOP_CFM:
        {
            BmmPrepareStopCfm *cfm = (BmmPrepareStopCfm *)message;
            OFFLOAD_MGR_LOGH(" BMM_PREPARE_STOP_CFM: %d\n", cfm->resultCode);

            if (cfm->resultCode == BMM_RESULT_SUCCESS)
            {  
                BT_Offload_HciArbiter_DeInit();

                Offload_Mgr_Post_Event(OFFLOAD_MGR_MSG_MICROSTACK_PREP_STOP_DONE, NULL, 0);
            }
            else
            {
                ;/* TBD: what are the failure cases expected */
            }
        } 
        break;

        case BMM_BTSS_ERROR_IND:
        {
            BmmBtssErrorInd *Ind = (BmmBtssErrorInd *)message;
            OFFLOAD_MGR_LOGH(" BMM_BTSS_ERROR_IND: %d\n", Ind->reason);

            /* Themisto SSR, stop sending HCI packets downstream */
            BT_Offload_HciArbiter_DeInit();
        }
        break;
        
        default:
            break;
    }

    MicroStackFreePrimitive(group, message);
}

/*=============================================================================
                    GLOBAL FUNCTION DEFINITIONS
=============================================================================*/
/*===========================================================================
FUNCTION      bt_state_mgr_init
===========================================================================*/
/**
 * @brief Module Init.
 */
void bt_state_mgr_init(void)
{
     bt_on_mode = BT_STATE_MGR_DEFAULT_MODE;
}

/*===========================================================================
FUNCTION      bt_state_mgr_deinit
===========================================================================*/
/**
 * @brief Module De-Init.
 */
void bt_state_mgr_deinit(void)
{
    bt_on_mode = BT_STATE_MGR_DEFAULT_MODE;
    MicroStackDeregisterAppCb(BT_STATE_MGR_HNDL);
}

/*===========================================================================
FUNCTION      bt_state_on_handle
===========================================================================*/
/**
 * @brief BT ON handling Post SPI download and transport switch from HLOS.
 *
 * @param evt Event data.
 */
void bt_state_on_handle(uint8_t *evt)
{
    MicrostackConfig config = evt[BT_ENABLE_MODE_OFFSET];
    OFFLOAD_MGR_LOGH("bt_state_on_handle -> config %d\n", config);
    if ((config != BT_STATE_MGR_PASSTHROUGH_MODE) && (config != BT_STATE_MGR_OFFLD_MODE))
    {
        return;//TBD: err_fatal  
    }

    /**If in Island Mode , Exit it and restrict Island Entry */
    if (qurt_island_get_status())
    {
        uSleep_exit();
    }
    bt_pal_mgr_restrict_island();

    /* 1. Configure Logging */
    MicroStackConfigureHciLogging(BTHOST_HCI_ENABLE_SNOOP_LOGGING);

    MicroStackStart((const uint8_t *)"bt_micro_stack", BT_MICROSTACK_THREAD_PRIORITY, config);

    /* 2. Register an Application cb */
    MicroStackRegisterAppCb(BT_STATE_MGR_HNDL,bt_state_mgr_microstack_cb);

    /* 3. Set event mask to get BMM initialized & Deinitialized indication */
    BmmSetEventMaskReqSend(BT_STATE_MGR_HNDL,BMM_EVENT_MASK_SUBSCRIBE_INITIALIZED | BMM_EVENT_MASK_SUBSCRIBE_DEINITIALIZED |
                                             BMM_EVENT_MASK_SUBSCRIBE_BTSS_ERROR);

    bt_on_mode = config;
    OFFLOAD_MGR_LOGL("bt_state_on_handle -> done\n");
}

/*===========================================================================
FUNCTION      bt_state_mgr_microstack_stop
===========================================================================*/
/**
 * @brief BT OFF handling Post HLOS Turn OFF BT.
 */
void bt_state_mgr_microstack_stop(void)
{
    MicroStackStop();
}

/*===========================================================================
FUNCTION      bt_state_off_handle
===========================================================================*/
/**
 * @brief BT OFF handling Post HLOS Turn OFFs BT. HLOS event to bt-disable
 *
 * @param evt Event data.
 */
void bt_state_off_handle(uint8_t *evt)
{
    BmmStopReason StopReason;
    bt_off_mode_t bt_off_mode = evt[BT_DISABLE_MODE_OFFSET];
    
    OFFLOAD_MGR_LOGH("bt_state_off_handle -> bt_off_mode %d\n", bt_off_mode);
    if (bt_off_mode >= BT_OFF_MAX)
    {
        return;
    }

    /**If in Island Mode , Exit it and restrict Island Entry */
    if (qurt_island_get_status())
    {
        uSleep_exit();
    }
    bt_pal_mgr_restrict_island();

    StopReason = translate_bt_off_mode(bt_off_mode);
    BmmPrepareStopReqSend(BT_STATE_MGR_HNDL, StopReason);

    OFFLOAD_MGR_LOGL("bt_state_off_handle -> bmmStopReason %d\n", StopReason);
}

/*===========================================================================
FUNCTION      bt_state_check_and_mock_off
===========================================================================*/
/**
 * @brief Function to handle SSR scenarios due to HAL crashes. 
 * This function checks and trigger BT-off if not triggered.
 */
void bt_state_check_and_mock_off(void)
{
    OFFLOAD_MGR_LOGL("bt_state_check_and_mock_off bt_on_mode %d", bt_on_mode);

    /* When there is HLOS HAL crashes, all BT HAL Glink channels will be disconnected. In this case,
    bt_state_mgr need to check BT_DISABLE is received before Glink is disconnected. Otherwise it is considered as
    HAL crash and bt_state_mgr need to mock BT-Off sequence to clean-up on LPASS side. */
    if (bt_on_mode == BT_STATE_MGR_PASSTHROUGH_MODE || bt_on_mode == BT_STATE_MGR_OFFLD_MODE)
    {

        /**If in Island Mode , Exit it and restrict Island Entry */
        if (qurt_island_get_status())
        {
            uSleep_exit();
        }
        bt_pal_mgr_restrict_island();


        OFFLOAD_MGR_LOGL("SSR due to HAL crash --> trigger BT-Off\n");
        BmmPrepareStopReqSend(BT_STATE_MGR_HNDL, BMM_STOP_REASON_USER);

        /* update the mode with SSR */
        bt_on_mode = (bt_on_mode == BT_STATE_MGR_PASSTHROUGH_MODE)? 
            BT_STATE_MGR_PASSTHROUGH_MODE_SSR : BT_STATE_MGR_OFFLD_MODE_SSR;
    }
}

