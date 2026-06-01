/*==============================================================================*/
/*
 * Copyright (c) 2025 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      bt_utils.c
===========================================================================*/
/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "uGPIOInt.h"
#include "bt_utils.h"
#include "bt_pal_log.h"
#include "csr_qvsc_lib.h"
#include "bt_main.h"
#include "bmm_lib.h"
#include "offload_mgr_client_interface.h"
#include "bt_pal_heap.h"
#include "endpt_mgr_rpc.h"
#include "endpt_mgr.h"
#include "stringl.h"
#include "island_user.h"
#include "qurt_island.h"
#include "uSleep_mode_trans.h"
#include "bt_pal_npa.h"

/*=============================================================================
                        MACRO DEFINITIONS
=============================================================================*/
/** @def BT_UTILS_APP_HANDLE
 *  @brief Application handle for BT Utils commands.
 */
#define BT_UTILS_APP_HANDLE 0x7828

/** @def TIME_SYNC_GPIO_NUM
 *  @brief GPIO number used for time synchronization.
 */
#define TIME_SYNC_GPIO_NUM (78U)

/** @def HCI_VS_CLOCK_MAPPING_ARRAY_SUB_OPC
 *  @brief Sub-opcode for HCI VS Clock Mapping Array command.
 */
#define HCI_VS_CLOCK_MAPPING_ARRAY_SUB_OPC (0x6U)

/** @def BT_TIMESYNC_CMD_LEN
 *  @brief Length of the Bluetooth time synchronization command payload.
 */
#define BT_TIMESYNC_CMD_LEN (5U)

/** @def BT_TIMESYNC_CMD_OPC
 *  @brief Opcode for the Bluetooth time synchronization command.
 */
#define BT_TIMESYNC_CMD_OPC (0xFD91)

/** @def BT_TIMESYNC_EVT_CMD_OPC_OFFSET1
 *  @brief Offset for the first byte of the event command opcode in the payload.
 */
#define BT_TIMESYNC_EVT_CMD_OPC_OFFSET1 (1U)

/** @def BT_TIMESYNC_EVT_CMD_OPC_OFFSET2
 *  @brief Offset for the second byte of the event command opcode in the payload.
 */
#define BT_TIMESYNC_EVT_CMD_OPC_OFFSET2 (2U)

/** @def BT_TIMESYNC_EVT_STATUS_OFFSET
 *  @brief Offset for the status byte in the event payload.
 */
#define BT_TIMESYNC_EVT_STATUS_OFFSET   (3U)

/** @def BT_TIMESYNC_EVT_STATUS_OFFSET
 *  @brief Offset for the status byte in the event payload.
 */
#define BT_TIMESYNC_GPIO_READ_DONE   (1U)

/** @def BT_TIMESYNC_EVT_STATUS_OFFSET
 *  @brief Offset for the status byte in the event payload.
 */
#define BT_TIMESYNC_GPIO_READ_FAILED   (2U)

/** @def APPS_CLOCK_SHARING_BY_GPIO_TOGGLE  
 *  @brief Config for GPIO toggle on BLE conn 
 */
#define APPS_CLOCK_SHARING_BY_GPIO_TOGGLE 0x2

/** @def APPS_CLOCK_SHARING_WITH_ISO_BY_GPIO_TOGGLE  
 *  @brief Config for GPIO toggle on ISOC conn
 */
#define APPS_CLOCK_SHARING_WITH_ISO_BY_GPIO_TOGGLE 0x3

/** @def BT_TS_WAIT_TIME_MICROSEC  
 *  @brief 100 millisec -> 100000microsec
 */
#define BT_TS_WAIT_TIME_MICROSEC   100000U


/*=============================================================================
                    GLOBAL DATA DECLARATIONS
=============================================================================*/
/**
 * @brief Global instance of Bluetooth utilities.
 */
bt_utils_t bt_utils;

/*===========================================================================
                    LOCAL FUNCTION DEFINITIONS
===========================================================================*/
/**
 * @brief GPIO interrupt notification callback for time synchronization.
 *
 * @param param Parameter passed by the interrupt system.
 */
static void timesync_gpio_interrupt_notify(uint32_t param) 
{
    uGPIOIntTimestampStatus status;

    int32_t result = uGPIOInt_TimestampRead(TIME_SYNC_GPIO_NUM, &status, &bt_utils.timesync.hw_timestamp);

    if (result != UGPIOINT_SUCCESS) 
    {
        qurt_signal_set(&bt_utils.timesync.signal, BT_TIMESYNC_GPIO_READ_FAILED);
        BT_PAL_LOGE("hwts_read_fail rc:%d", result);
        return;
    }

    qurt_signal_set(&bt_utils.timesync.signal, BT_TIMESYNC_GPIO_READ_DONE);
	
    return;
}

/**
 * @brief Deregisters the GPIO interrupt for time synchronization.
 */
static void bt_utils_deregister_gpio_isr(void)
{
    int32_t result = uGPIOInt_DeregisterInterrupt(TIME_SYNC_GPIO_NUM);

    if (result != UGPIOINT_SUCCESS) 
    {
        BT_PAL_LOGE("bt_ts_de-reg: rc:%d", result);
        return;
    }

    return;
}

/**
 * @brief Registers the GPIO interrupt for time synchronization.
 */
static void bt_utils_register_gpio_isr(void)
{
    int32_t result = uGPIOInt_RegisterInterrupt(
                            TIME_SYNC_GPIO_NUM, UGPIOINT_TRIGGER_RISING,
                            (uGPIOINTISR)timesync_gpio_interrupt_notify, NULL,
                            (UGPIOINTF_TIMESTAMP_EN | UGPIOINTF_ISLAND));

    if (result != UGPIOINT_SUCCESS) 
    {
        BT_PAL_LOGE("bt_ts_reg: rc:%d", result);
        return;
    }

    return;
}


/**
 * @brief Triggers a Bluetooth time synchronization command.
 *
 * @param connHandle Connection handle identifier.
 * @param config Configuration flags.
 */
static void bt_utils_trigger_timesync(uint16_t connHandle, uint8_t config)
{
    uint8_t numHandles = 1U; // Support 1 connection handle
    uint16_t iter = 0;

    BT_PAL_LOGH("bt_ts_req ch: %x cfg: %x", connHandle, config);

    /**If in Island Mode , Exit it and restrict Island Entry */
    if (qurt_island_get_status())
    {
        uSleep_exit();
    }
    bt_pal_mgr_restrict_island();
    
    bt_utils_register_gpio_isr();

    bt_pal_mgr_allow_island();

    uint8_t *payload = bt_pal_malloc(BT_TIMESYNC_CMD_LEN);
    if (payload == NULL)
    {
        /*TBD: Frame a negative rsp and return */
        return;
    }

    payload[iter++] = HCI_VS_CLOCK_MAPPING_ARRAY_SUB_OPC;
    payload[iter++] = config;
    payload[iter++] = numHandles;
    payload[iter++] = (uint8_t)connHandle & 0xFF;
    payload[iter] = (uint8_t)(connHandle >> 8U);
 
    CsrQvscReqSend(BT_UTILS_APP_HANDLE, BT_TIMESYNC_CMD_OPC, BT_TIMESYNC_CMD_LEN, payload);
	return;
}

/**
 * @brief wait for the hw timestamp to get populated.
 *
 * @param config Configuration flags.
 * @param status status from BTSS for fetching BT clock
 */
static void bt_ts_sync_wait(uint8_t config, uint8_t *status)
{
    int ret;
    unsigned int eventBits = 0;

    /* Not expecting hardware timestamps for other configs */
    if (!(config == APPS_CLOCK_SHARING_BY_GPIO_TOGGLE || 
        config == APPS_CLOCK_SHARING_WITH_ISO_BY_GPIO_TOGGLE))
    {
        return;
    }

    /* No need to wait if HW timestamp is already present */
    if (bt_utils.timesync.hw_timestamp != 0)
    {
        return;
    }

    qurt_signal_clear(&bt_utils.timesync.signal, BT_TIMESYNC_GPIO_READ_DONE | BT_TIMESYNC_GPIO_READ_FAILED);

    ret = qurt_signal_wait_timed(&bt_utils.timesync.signal, 
                            BT_TIMESYNC_GPIO_READ_DONE | BT_TIMESYNC_GPIO_READ_FAILED, 
                            QURT_SIGNAL_ATTR_WAIT_ANY, 
                            (unsigned int *)&eventBits, 
                            BT_TS_WAIT_TIME_MICROSEC);
    BT_PAL_LOGH("bt_ts_sync_wait st %d:", ret);
    if (bt_utils.timesync.hw_timestamp == 0)
    {
        *status = 0x1f; //unspecified error
    }

    return;
}

/**
 * @brief Callback for QVSC command responses.
 *
 * @param qvsc_prim Pointer to QVSC primitive response.
 */
static void qvsc_cb(void *qvsc_prim)
{
    
    CsrQvscCfm *cfm = (CsrQvscCfm *)qvsc_prim;
    uint16_t cmd_opcode = cfm->payload[BT_TIMESYNC_EVT_CMD_OPC_OFFSET1] | (cfm->payload[BT_TIMESYNC_EVT_CMD_OPC_OFFSET2] << 8U);    
    BT_PAL_LOGM("qvsc_cb opc: %x", cmd_opcode);

    switch (cmd_opcode)
    {
        case BT_TIMESYNC_CMD_OPC:
        {
            uint8_t status = cfm->payload[BT_TIMESYNC_EVT_STATUS_OFFSET];

            BT_PAL_LOGH("bt_ts_rsp st: %x", status);

            endpoint_t ep_id = {
                .hub_id = ENDPT_MGR_HUB_ID_AWM,
                .id     = 0, /* random */
            };
			endpt_mgr_rpc_header_t *header = endpt_mgr_rpc_get_default_header();
			bt_utils_timesync_rsp_t *rsp = (bt_utils_timesync_rsp_t *)header->data;			
			header->opcode = BT_UTILS_TIME_SYNC_RSP;
			header->endpoint_id = BT_UTILS_APP_HANDLE;
			header->command_type = NONOFFLOAD_APP_CMD;

            if (status == 0x00)
            {

                header->data_len = sizeof(bt_utils_timesync_rsp_t);
                memscpy(rsp, sizeof(bt_utils_timesync_rsp_t), &cfm->payload[3], cfm->payloadLength - 3);

                /* Wait for the HW timestamp to get captured */
                bt_ts_sync_wait(rsp->config, &status);
                rsp->appClock = bt_utils.timesync.hw_timestamp;

                BT_PAL_LOGH("AppClock: %x BtClock: %x\n", rsp->appClock, rsp->btClock);
				BT_PAL_LOGM("connHandle: %x evtCounter: %x timeOffset: %x interval: %x", rsp->connHandle, rsp->evtCounter, rsp->timeOffset, rsp->interval);
            }
            else
            {
                rsp->status = status;
                header->data_len = sizeof(uint8_t);
            }

			endpt_mgr_send_msg_to_endpoint(&endpt_mgr_hub_id_awm, header, ENDPT_MGR_RPC_HDR_SIZE + header->data_len);

            /**If in Island Mode , Exit it and restrict Island Entry */
            if (qurt_island_get_status())
            {
                uSleep_exit();
            }
            bt_pal_mgr_restrict_island();

            bt_utils_deregister_gpio_isr();

            bt_pal_mgr_allow_island();

            /* reset hw timestamp */
            bt_utils.timesync.hw_timestamp = 0;

            /*TBD: Clean-up needs to be removed once microstack api's exposed */
            bt_pal_free(cfm->payload);
            cfm->payload = NULL;
        }
        break;
        default:
        {
            ; //Do nothing
        }
        break;
    }
}

/**
 * @brief Microstack callback for Bluetooth utility events.
 *
 * @param handle Application handle.
 * @param eventClass Event class identifier.
 * @param message Pointer to event message.
 */
static void microstack_bt_utils_cb(BtAppHandle handle, BtEventClass eventClass, void *message)
{
    CsrPrim type = *((CsrPrim *)message);
    BT_PAL_LOGM("microstack_bt_utils_cb : %d", eventClass);
    switch (eventClass)
    {
        case CSR_QVSC_PRIM: 
        {
            qvsc_cb(message);
        }
        break;
        default:
        {
            BT_PAL_LOGE("Unhandled Event Received : %d\n",type);
        }
        break;
    }
}

/*=============================================================================
                    GLOBAL FUNCTION DEFINITIONS
=============================================================================*/
/**
 * @brief Handles BT Utils commands
 * This function processes bt utils commands based on the provided opcode and length.
 */
void bt_utils_cmd_handler(uint16_t opcode , uint16_t len , void *cmd)
{
    BT_PAL_LOGM("bt_utils_cmd_handler opc: %x", opcode);
    switch(opcode)
    {
        case BT_UTILS_TIME_SYNC:
        {
            bt_utils_timesync_req_t *req = (bt_utils_timesync_req_t *)cmd;
            bt_utils_trigger_timesync(req->connHandle, req->config);
        }
        break;

        case BT_UTILS_ENTER_ISLAND:
        {
            BT_PAL_LOGM("Enter Island Command Received\n");
            if (qurt_island_get_status())
            {
                uSleep_exit();
            }
#ifdef BT_ISLAND_PROXY_VOTE_SUPPORT
            island_mgr_remove_proxy(USLEEP_ISLAND_SNS);
            island_mgr_remove_proxy(USLEEP_ISLAND_AUD);
#endif
        }
        break;

        case BT_UTILS_EXIT_ISLAND:
        {
            BT_PAL_LOGM("Exit Island Command Received\n");
            if (qurt_island_get_status())
            {
                uSleep_exit();
            }
#ifdef BT_ISLAND_PROXY_VOTE_SUPPORT
            island_mgr_add_proxy(USLEEP_ISLAND_SNS);
            island_mgr_add_proxy(USLEEP_ISLAND_AUD);
#endif
        }
        break;

        default:
        {
            BT_PAL_LOGE("Unhandled Event Received : %d\n",opcode);
        }
        break;
    }
}

/**
 * @brief Initializes Bluetooth utilities.
 *
 * Registers GPIO interrupt and application callback, and initializes global data.
 */
void bt_utils_init(void)
{
    microstack_register_app_callback(BT_UTILS_APP_HANDLE, microstack_bt_utils_cb);

    memset(&bt_utils, 0x0, sizeof(bt_utils_t));

    qurt_signal_init(&bt_utils.timesync.signal);
}

/**
 * @brief Deinitializes Bluetooth utilities.
 *
 * Deregisters GPIO interrupt and application callback.
 */
void bt_utils_deinit(void)
{
    microstack_deregister_app_callback(BT_UTILS_APP_HANDLE);

    qurt_signal_destroy(&bt_utils.timesync.signal);
}