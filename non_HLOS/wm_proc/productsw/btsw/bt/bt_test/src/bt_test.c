/*************************************************************************
 * @file     lpai_bt_test.c
 * @brief    LPAI BT Test source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <lpai_bt_app_mgr_adsp_handler.h>
#include <lpai_bt_heap.h>
#include <lpai_bt_app_mgr.h>
#include "lpai_bt_state_mgr.h"
#include <lpai_bt_app_mgr_client_interface.h>
#include "qapi_bt_rfcomm_app.h"
#include "qapi_bt_lecoc_app.h"
#include "qapi_bt_le_scan.h"
#include "lpai_bt_le_adv.h"
#include "qapi_bt_le_adv.h"
#if defined(CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)
    #ifdef CONFIG_LPAI_BT_GATT_THROUGHPUT_MENU
    #define CLIENT_APP_BULK_TRANSFER
    #define SERVER_APP_BULK_TRANSFER
    #endif
#endif /* CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER */
#include "lpai_bt_audio_tone.h"




#define NUM_BASE 10
#define STRTOL_BASE 0


/* Subcommand: BT STATUS */
static int cmd_bt_status(const struct shell *shell, size_t argc, char **argv)
{
    uint16_t bt_status = lpai_get_current_bt_status();
    #if CONFIG_LPAI_BT_TEST_MENU
    shell_print(shell , "Bt Status is %d\n",bt_status);
    if(bt_status == BT_STATUS_ON)
    shell_print(shell , "BT is ON ! \n");
    else
        shell_print(shell , "BT is OFF ! \n"); 
    #endif
    return 0;
}

/* Subcommand: List End Points */
static int cmd_list_endpoints(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
     shell_print(shell, "List End Points \n");
     list_endpoint_details();
    #endif
    return 0;
}


static int rfcomm_cmd1(const struct shell *shell, size_t argc, char **argv)
{
    if(argc != 4)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <socketId><packet_size><num_packets> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid socketId: %s", argv[1]);
            #endif
            return -EINVAL;
        }

        long packetSize = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid packetSize %s", argv[2]);
            #endif
            return -EINVAL;
        }
        long packetNum = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
           #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "packetNum: %s", argv[3]);
            #endif
            return -EINVAL;
        }
        else
        {
            qapi_bt_rfcomm_test_tput_tx(0, socketId, packetSize,packetNum);
        }
    }
    
    return 0;
}

static int rfcomm_cmd2(const struct shell *shell, size_t argc, char **argv)
{
    
    if(argc != 4)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <socketId><dataLen><data> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid integer: %s", argv[1]);
            #endif
            return -EINVAL;
        }

        long dataLen = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid integer: %s", argv[2]);
            #endif
            return -EINVAL;
        }
        uint8_t *shell_data= argv[3];
        uint8_t *data = malloc(dataLen);
        if (data == NULL) 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid Memory Allocation: %s", argv[3]);
            #endif
            return -EINVAL;
        }
        else
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_print(shell,"socket Id : % " PRIu64 "\n",socketId);
            #endif
            memscpy(data,dataLen,shell_data,dataLen);
            qapi_bt_rfcomm_send_data(0 , socketId, dataLen, data);
            free(data);

        }
    }
    
    return 0;
}

static int rfcomm_cmd3(const struct shell *shell, size_t argc, char **argv)
{
   if(argc != 2)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_error(shell , "Usage : <socketId> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid socketId: %s", argv[1]);
            #endif
            return -EINVAL;
        }
        else
        {
            qapi_rfcomm_test_loopback(0, socketId);
        }
    }
    
    return 0;
}

static int rfcomm_cmd4(const struct shell *shell, size_t argc, char **argv)
{
     if(argc != 2)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <socketId> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid integer: %s", argv[1]);
            #endif
            return -EINVAL;
        }
        else
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_print(shell,"socket Id : % " PRIu64 "\n",socketId);
            #endif
            qapi_bt_rfcomm_close_socket(0, socketId);
        }
    }
    return 0;
}

static int rfcomm_cmd5(const struct shell *shell, size_t argc, char **argv)
{
     if(argc != 1)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <No input needed> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        qapi_bt_rfcomm_get_socketdetails();
    }
    return 0;
}

static int rfcomm_cmd6(const struct shell *shell, size_t argc, char **argv)
{
    if(argc != 4)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <socketId><packet_size><num_packets> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid socketId: %s", argv[1]);
            #endif
            return -EINVAL;
        }

        long packetSize = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid packetSize %s", argv[2]);
            #endif
            return -EINVAL;
        }
        long packetNum = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
           #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "packetNum: %s", argv[3]);
            #endif
            return -EINVAL;
        }
        else
        {
            qapi_bt_rfcomm_test_tput_rx(0, socketId, packetSize,packetNum);
        }
    }
    
    return 0;
}


static int lecoc_cmd1(const struct shell *shell, size_t argc, char **argv)
{
     if(argc != 4)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <socketId><dataLen> <data> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid integer: %s", argv[1]);
            #endif
            return -EINVAL;
        }

        long dataLen = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid integer: %s", argv[2]);
            #endif
            return -EINVAL;
        }
        uint8_t *shell_data= argv[3];
        uint8_t *data = malloc(dataLen);
        if (data == NULL) 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid Memory Allocation: %s", argv[3]);
            #endif
            return -EINVAL;
        }
        else
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_print(shell,"socket Id : % " PRIu64 "\n",socketId);
            #endif
            memscpy(data,dataLen,shell_data,dataLen);
            int rc = qapi_bt_lecoc_send_data(0, socketId, dataLen, data);
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_print(shell,"rc : %d\n", rc);
            #endif
            free(data);

        }
    }
    return 0;
}

static int lecoc_cmd2(const struct shell *shell, size_t argc, char **argv)
{
   if(argc != 4)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_error(shell , "Usage : <socketId><packetSize><packetNum> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid socketId: %s", argv[1]);
            #endif
            return -EINVAL;
        }

        long packetSize = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid packetSize %s", argv[2]);
            #endif
            return -EINVAL;
        }
        long packetNum = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
           #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "packetNum: %s", argv[3]);
            #endif
            return -EINVAL;
        }
        else
        {
            qapi_bt_lecoc_test_tput_tx(0, socketId, packetSize, packetNum);
        }
    }
    
    return 0;
}

static int lecoc_cmd3(const struct shell *shell, size_t argc, char **argv)
{
   if(argc != 2)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <socketId> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid socketId: %s", argv[1]);
            #endif
            return -EINVAL;
        }
        else
        {
            qapi_lecoc_test_loopback(0 , socketId );
        }
    }
    
    return 0;
}

static int lecoc_cmd4(const struct shell *shell, size_t argc, char **argv)
{
     if(argc != 2)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <socketId> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid integer: %s", argv[1]);
            #endif
            return -EINVAL;
        }
        else
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_print(shell,"socket Id : % " PRIu64 "\n",socketId);
            #endif
            qapi_bt_lecoc_close_socket(0, socketId);
        }
    }
    return 0;
}

static int lecoc_cmd5(const struct shell *shell, size_t argc, char **argv)
{
     if(argc != 1)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <No input needed> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        qapi_bt_lecoc_get_socketdetails();
    }
    return 0;
}

static int lecoc_cmd8(const struct shell *shell, size_t argc, char **argv)
{
   if(argc != 4)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_error(shell , "Usage : <socketId><packetSize><packetNum> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        uint64_t socketId = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid socketId: %s", argv[1]);
            #endif
            return -EINVAL;
        }

        long packetSize = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid packetSize %s", argv[2]);
            #endif
            return -EINVAL;
        }
        long packetNum = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
           #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "packetNum: %s", argv[3]);
            #endif
            return -EINVAL;
        }
        else
        {
            qapi_bt_lecoc_test_tput_rx(0, socketId, packetSize, packetNum);
        }
    }
    
    return 0;
}


static int lecoc_cmd6(const struct shell *shell, size_t argc, char **argv)
{
    
    if(argc != 4)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <enable><FilterDataLen><FliterData> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        bool enable = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid enable flag: %s", argv[1]);
            #endif
            return -EINVAL;
        }
        long dataLen = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid integer: %s", argv[2]);
            #endif
            return -EINVAL;
        }
        uint8_t *shell_data= argv[3];
        uint8_t *data = malloc(dataLen);
        if (data == NULL) 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid Memory Allocation: %s", argv[3]);
            #endif
            return -EINVAL;
        }
        else
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_print(shell,"qapi_bt_lecoc_audio_tone_enable \n");
            #endif
            memscpy(data,dataLen,shell_data,dataLen);
            #if defined(CONFIG_PDTSW_AUDIO_SERVICE_ENABLE)
                qapi_bt_lecoc_audio_tone_enable(enable,dataLen, data);
            #endif
            free(data);
        }
    }
    return 0;
}

static bool sanity_check(char *ptr, char *arg, const struct shell *shell)
{
    if (ptr == NULL || *ptr != '\0')
    {
    	#if CONFIG_LPAI_BT_TEST_MENU
    	shell_error(shell, "Invalid argument: %s", arg);
    	#endif
    	return false;
    }
	return true;
}

static int lecoc_cmd7(const struct shell *shell, size_t argc, char **argv)
{
	if(!(argc == 4 || argc == 2))
	{
		#if CONFIG_LPAI_BT_TEST_MENU
		shell_print(shell , "Usage : <enable:1/0> <FilterDataLen> <FliterData>\n"
					        "Example(enable): bluetooth lecoc lecoc_enable_display 1 5 Start\n"
					        "Example(disable): bluetooth lecoc lecoc_enable_display 0\n");
		#endif
        return -EINVAL;
	}
	#if !CONFIG_QC_M55_DISPLAY_ENABLE
		/* feature not enabled */
		#if CONFIG_LPAI_BT_TEST_MENU
		shell_error(shell, "Display feature not enabled\n");
		#endif
		return -EINVAL;
	#endif
	char *endptr;
	bool enable = strtoull(argv[1], &endptr, NUM_BASE);

	if(!sanity_check(endptr, argv[1], shell)) return -EINVAL;

	if(!enable) {
		qapi_bt_lecoc_display_enable(enable, 0, NULL);
		return 0;
	}

	long dataLen = strtol(argv[2], &endptr, NUM_BASE);
	if(!sanity_check(endptr, argv[2], shell)) return -EINVAL;

	uint8_t *shell_data = argv[3];
	uint8_t *data = malloc(dataLen);
	if (data == NULL)
	{
		#if CONFIG_LPAI_BT_TEST_MENU
		shell_error(shell, "Invalid Memory Allocation: %s", argv[3]);
		#endif
		return -EINVAL;
	}

	#if CONFIG_LPAI_BT_TEST_MENU
	shell_print(shell,"qapi_bt_lecoc_display_enable \n");
	#endif
	memscpy(data, dataLen, shell_data, dataLen);
	qapi_bt_lecoc_display_enable(enable, dataLen, data);
	free(data);
    return 0;
}


static int audio_tone_cmd1(const struct shell *shell, size_t argc, char **argv)
{

    if(argc != 1)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <No input needed> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        #if defined(CONFIG_PDTSW_AUDIO_SERVICE_ENABLE)
            qapi_bt_get_supported_tone_ids();
        #endif
    }
    return 0;
}

static int audio_tone_cmd2(const struct shell *shell, size_t argc, char **argv)
{
   if(argc != 3)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <enable 0\1><toneid> \n");
        #endif
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        bool enable = strtoull(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid enable flag: %s", argv[1]);
            #endif
            return -EINVAL;
        }

        long toneid = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            #if CONFIG_LPAI_BT_TEST_MENU
            shell_error(shell, "Invalid toneid %s", argv[2]);
            #endif
            return -EINVAL;
        }
        else
        {
            #if defined(CONFIG_PDTSW_AUDIO_SERVICE_ENABLE)
                qapi_bt_configure_audio_tone(enable,toneid);
            #endif
        }
    }
    
    return 0;
}

static int cmd_start_le_scan_app(const struct shell *shell, size_t argc, char **argv)
{
    register_leScanApp();
    return 0;
}

static int cmd_reg_le_scanner(const struct shell *shell, size_t argc, char **argv)
{
    if(argc != 2)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <adv_filter : Valid Values : 0 to 3> \n");
        #endif
        return -EINVAL;
    }
    char *endptr;
    uint16_t adv_filter = (uint16_t)strtol(argv[1], &endptr, STRTOL_BASE);
    #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Adv Filter : %d \n",adv_filter);
    #endif
     
    qapi_register_scanner(adv_filter);
    return 0;
}

static int cmd_unreg_le_scanner(const struct shell *shell , size_t argc , char **argv)
{
    if(argc != 2)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "Usage : <scanHandle> \n");
        #endif
        return -EINVAL;
    }
    char *endptr;
    uint8_t scanHandle = (uint8_t)strtol(argv[1],&endptr,STRTOL_BASE);
    qapi_unregister_scanner(scanHandle);
    return 0;
}

static int cmd_enable_disable_scanner(const struct shell *shell, size_t argc, char **argv)
{
    if(argc != 4)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "<Enable/Disable> <Scan_Handle> <duration> \n");
        #endif
        return -EINVAL;
    }
    char *endptr;
    bool enable = (uint8_t)strtol(argv[1], &endptr, STRTOL_BASE);
    uint8_t scanHandle = (uint8_t)strtol(argv[2], &endptr, STRTOL_BASE);
    uint16_t duration = (uint16_t)strtol(argv[3], &endptr, STRTOL_BASE);

    qapi_enable_scan(enable , scanHandle , duration);
    
    return 0;
}


static int cmd_stop_le_scan_app(const struct shell *shell , size_t argc , char **argv)
{
    unregister_leScanApp();
    return 0;
}


static int cmd_start_le_adv_app(const struct shell *shell, size_t argc, char **argv)
{
    register_le_advApp();
    return 0;
}

static int cmd_stop_le_adv_app(const struct shell *shell, size_t argc, char **argv)
{
    unregister_le_advApp();
    return 0;
}

static int cmd_reg_adv_set(const struct shell *shell, size_t argc, char **argv)
{
    qapi_register_advSet();
    return 0;
}

static int cmd_unreg_adv_set(const struct shell *shell, size_t argc, char **argv)
{
    if(argc != 2)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "<Adv Handle> \n");
        #endif
        return -EINVAL;
    }
    char *endptr;
    uint8_t advHandle = (uint8_t)strtol(argv[1], &endptr,STRTOL_BASE);
    qapi_unregister_advSet(advHandle);
    return 0;
}

static int cmd_set_adv_params(const struct shell *shell, size_t argc, char **argv)
{
    if(argc != 7)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "<Adv Handle> <advEventProperties <Valid Values : 0 and 64>> <minInterval <Valid Values : 0x20 to 0xFFFFFF> > <maxInterval <Valid Values : 0x20 to 0xFFFFFF> > <advTxPower : -127 to +20 > <ownAddrType <Valid < 0 : Public , 1: Random >> \n");
        #endif
        return -EINVAL;
    }
    setAdvParams_t advParams;
    char *endptr;

    advParams.advHandle = (uint16_t)strtol(argv[1], &endptr, STRTOL_BASE);
    advParams.advEventProperties = (uint32_t)strtol(argv[2], &endptr, STRTOL_BASE);
    advParams.intervalMin = (uint32_t)strtol(argv[3], &endptr, STRTOL_BASE);
    advParams.intervalMax = (uint32_t)strtol(argv[4], &endptr, STRTOL_BASE);
    advParams.advTxPower = (int32_t)strtol(argv[5], &endptr, STRTOL_BASE);
    advParams.ownAddrType = (int8_t)strtol(argv[6], &endptr, STRTOL_BASE);

    qapi_set_advParams(advParams);
    return 0;
}


static int cmd_set_adv_random_addr(const struct shell *shell , size_t argc , char **argv)
{
    if(argc != 6)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "<Adv Handle> <addrType <Valid : Static : 0> > <lap <Ex:0xa1b2c3> > <uap <Ex:0xd4> > <nap <Ex:0xe5f6>> \n");
        #endif
        return -EINVAL;
    }
    setRandomAddress_t randomAddr ;
    char *endptr;

    randomAddr.advHandle = (uint16_t)strtol(argv[1], &endptr, STRTOL_BASE);
    randomAddr.type = (uint32_t)strtol(argv[2], &endptr, STRTOL_BASE);
    randomAddr.deviceAddress.lap = (uint32_t)strtol(argv[3], &endptr, STRTOL_BASE);
    randomAddr.deviceAddress.uap = (uint32_t)strtol(argv[4], &endptr, STRTOL_BASE);
    randomAddr.deviceAddress.nap = (int32_t)strtol(argv[5], &endptr, STRTOL_BASE);
    qapi_set_random_address(randomAddr);
	
	return 0;
}


static int cmd_set_adv_data(const struct shell *shell , size_t argc , char **argv)
{

    setAdvData_t advData;
    char *endptr;

    if(argc != 4)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "<AdvHandle> <Service_uuid  <Ex:0xff01> > <Local Name <Max Len : 20> > \n");
        #endif
        return -EINVAL;
    }
    advData.advHandle = (uint16_t)strtol(argv[1], &endptr, STRTOL_BASE);
    advData.service_uuid = (uint16_t)strtol(argv[2], &endptr, STRTOL_BASE);
    //uint8_t *shell_data= argv[3];
    memscpy(advData.local_name,sizeof(advData.local_name),argv[3],strlen(argv[3]));
    if(strlen(argv[3]) > 20)
    {
        advData.local_name_len = 20;
    }
    else
    {
        advData.local_name_len = strlen(argv[3]);
    }
    qapi_set_advData(advData);
    
    return 0;

}

static int cmd_reg_start_adv(const struct shell *shell , size_t argc , char **argv)
{
    if(argc != 2)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "<Adv Handle to Enable> \n");
        #endif
        return -EINVAL;
    }
    char *endptr;
    uint8_t advHandle = (uint8_t)strtol(argv[1], &endptr, STRTOL_BASE);
    qapi_enable_avertisement(advHandle);
    return 0;
}

static int cmd_reg_stop_adv(const struct shell *shell, size_t argc, char **argv)
{
    if(argc != 2)
    {
        #if CONFIG_LPAI_BT_TEST_MENU
        shell_print(shell , "<Adv Handle to Disable> \n");
        #endif
        return -EINVAL;
    }
    char *endptr;
    uint8_t advHandle = (uint8_t)strtol(argv[1], &endptr, STRTOL_BASE);
    qapi_disbale_advertisment(advHandle);
    return 0;
}

#if defined(CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)

static int gatt_unoffload_req(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
    shell_print(shell, "gatt_unoffload_req");

    if(argc != 2)
    {
        shell_print(shell , "Usage : <sessionId> \n");
        return -EINVAL;
    }
    else
    {
        char *endptr;
        int32_t sessionId = strtol(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "sessionId Invalid integer: %s", argv[1]);
            return -EINVAL;
        }
        shell_print(shell , "sessionId : %d \n",sessionId);
        release_attributes(sessionId);
    }
    #endif
    return 0;
}

static int gatt_client_app_read_req(const struct shell *shell, size_t argc, char **argv)
{
#if CONFIG_LPAI_BT_TEST_MENU

    shell_print(shell, "gatt_client_app_read_req");

    if(argc != 3)
    {
        shell_print(shell , "Usage : <sessionId> <attrHandle> \n");
        return -EINVAL;
    }
    else
    {
        char *endptr;
        int32_t sessionId = strtol(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "sessionId Invalid integer: %s", argv[1]);
            return -EINVAL;
        }

        uint16_t attrHandle = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
             shell_error(shell, "attrHandle Invalid integer: %s", argv[2]);
            return -EINVAL;
        }

        shell_print(shell , "sessionId : %d , attrHandle %d \n",sessionId , attrHandle);
        extern void send_gatt_read_request(uint32_t session_id, uint16_t attribute_id);
        send_gatt_read_request(sessionId, attrHandle);
    }
    #endif
    return 0;
}

static int gatt_client_app_write_req(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
    shell_print(shell, "gatt_client_app_write_req");

    if(argc != 5)
    {
        shell_print(shell , "Usage : <sessionId> <attrHandle> <val_len> <write_cmd 0/1>\n");
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        int32_t sessionId = strtol(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "sessionId Invalid integer: %s", argv[1]);
            return -EINVAL;
        }

        uint16_t attrHandle = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "attrHandle Invalid integer: %s", argv[2]);
            return -EINVAL;
        }

        uint16_t val_len = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "val_len Invalid integer: %s", argv[3]);
            return -EINVAL;
        }

        uint8_t write_cmd = strtol(argv[4], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "write_cmd Invalid integer: %s", argv[4]);
            return -EINVAL;
        }

        shell_print(shell , "sessionId : %d , attrHandle %d val_len %d write_cmd %d\n",sessionId , attrHandle, val_len, write_cmd);

        send_gatt_write_request(sessionId, attrHandle, val_len, write_cmd);
        // shell_print(shell , "qapi_gatt_client_write_char_req ret %d\n", ret);
    }
    #endif
    return 0;
}

#ifdef CLIENT_APP_BULK_TRANSFER
static int gatt_client_tx_bulk_transfer(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
    shell_print(shell, "gatt_client_tx_bulk_transfer");

    if(argc != 5)
    {
        shell_print(shell , "Usage : <sessionId> <attrHandle> <pkt_cnt> <val_len> \n");
        return -EINVAL;
    }
    else
    {
        char *endptr;
        int32_t sessionId = strtol(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "sessionId Invalid integer: %s", argv[1]);
            return -EINVAL;
        }

        uint16_t attrHandle = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "attrHandle Invalid integer: %s", argv[2]);
            return -EINVAL;
        }

        uint16_t pktCnt = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "pktCnt Invalid integer: %s", argv[3]);
            return -EINVAL;
        }

        uint16_t val_len = strtol(argv[4], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "val_len Invalid integer: %s", argv[4]);
            return -EINVAL;
        }

        shell_print(shell , "sessionId : %d , attrHandle %d val_len %d pktCnt %d\n",sessionId , attrHandle, val_len, pktCnt);
        gatt_client_app_tx_bulk_transfer(sessionId, attrHandle, pktCnt, val_len);
    }
    #endif
    return 0;
}

static int gatt_client_rx_bulk_transfer(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
    shell_print(shell, "gatt_client_rx_bulk_transfer");

    if(argc != 5)
    {
        shell_print(shell , "Usage : <sessionId> <attrHandle> <pkt_cnt> <pkt_size> \n");
        return -EINVAL;
    }
    else
    {
        char *endptr;
        int32_t sessionId = strtol(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "sessionId Invalid integer: %s", argv[1]);
            return -EINVAL;
        }

        uint16_t attrHandle = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "attrHandle Invalid integer: %s", argv[2]);
            return -EINVAL;
        }

        uint16_t pktCnt = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "pktCnt Invalid integer: %s", argv[3]);
            return -EINVAL;
        }
        uint16_t pktSize = strtol(argv[4], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "pktCnt Invalid integer: %s", argv[4]);
            return -EINVAL;
        }

        shell_print(shell , "sessionId : %d , attrHandle %d pktCnt %d pktSize %d\n",sessionId , attrHandle, pktCnt, pktSize);
        gatt_client_app_rx_bulk_transfer(sessionId, attrHandle, pktCnt, pktSize); /** todo  */
    }
    #endif
    return 0;
}

static int last_throughput(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
    // shell_print(shell, "last_throughput");

    if(argc != 1)
    {
        shell_print(shell , "Usage :  \n");
        return -EINVAL;
    }
    else
    {
        print_last_throughput();
        // gatt_client_app_rx_bulk_transfer(sessionId, attrHandle, pktCnt, pktSize); /** todo  */
    }
    #endif
    return 0;
}

static int exit_throughput(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
    // shell_print(shell, "last_throughput");

    if(argc != 1)
    {
        shell_print(shell , "Usage :  \n");
        return -EINVAL;
    }
    else
    {
        exit_throughput_proc();
        // gatt_client_app_rx_bulk_transfer(sessionId, attrHandle, pktCnt, pktSize); /** todo  */
    }
    #endif
    return 0;
}

#endif /* CLIENT_APP_BULK_TRANSFER */


static int gatt_server_app_send_notif(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
    shell_print(shell, "gatt_server_app_send_notif");

    if(argc != 4)
    {
        shell_print(shell , "Usage : <sessionId> <attrHandle> <val_len>\n");
        return -EINVAL;
    }
    else
    {
        
        char *endptr;
        int32_t sessionId = strtol(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "sessionId Invalid integer: %s", argv[1]);
            return -EINVAL;
        }

        uint16_t attrHandle = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "attrHandle Invalid integer: %s", argv[2]);
            return -EINVAL;
        }

        uint16_t val_len = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "val_len Invalid integer: %s", argv[3]);
            return -EINVAL;
        }

        shell_print(shell , "sessionId : %d , attrHandle %d val_len %d\n",sessionId , attrHandle, val_len);

        gatt_server_app_notif_send(sessionId, attrHandle, val_len);
        // shell_print(shell , "qapi_gatt_server_send_char_notif ret %d\n", ret);

    }
    #endif
    return 0;
}

#ifdef SERVER_APP_BULK_TRANSFER
static int gatt_server_tx_bulk_transfer(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
    shell_print(shell, "gatt_server_bulk_transfer");

    if(argc != 5)
    {
        shell_print(shell , "Usage : <sessionId> <attrHandle> <pkt_cnt> <val_len>\n");
        return -EINVAL;
    }
    else
    {
        char *endptr;
        int32_t sessionId = strtol(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "sessionId Invalid integer: %s", argv[1]);
            return -EINVAL;
        }

        uint16_t attrHandle = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "attrHandle Invalid integer: %s", argv[2]);
            return -EINVAL;
        }

        uint16_t pktCnt = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "pktCnt Invalid integer: %s", argv[3]);
            return -EINVAL;
        }

        uint16_t val_len = strtol(argv[4], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "val_len Invalid integer: %s", argv[4]);
            return -EINVAL;
        }

        shell_print(shell , "sessionId : %d , attrHandle %d pkt_cnt %d val_len %d\n",sessionId , attrHandle, pktCnt, val_len);
        gatt_server_app_tx_bulk_transfer(sessionId, attrHandle, pktCnt, val_len);

    }
    #endif
    return 0;
}

static int gatt_server_rx_bulk_transfer(const struct shell *shell, size_t argc, char **argv)
{
    #if CONFIG_LPAI_BT_TEST_MENU
    shell_print(shell, "gatt_server_rx_bulk_transfer");

    if(argc != 5)
    {
        shell_print(shell , "Usage : <sessionId> <attrHandle> <pkt_cnt> <pkt_size>\n");
        return -EINVAL;
    }
    else
    {
        char *endptr;
        int32_t sessionId = strtol(argv[1], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "sessionId Invalid integer: %s", argv[1]);
            return -EINVAL;
        }

        uint16_t attrHandle = strtol(argv[2], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "attrHandle Invalid integer: %s", argv[2]);
            return -EINVAL;
        }

        uint16_t pktCnt = strtol(argv[3], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "pktCnt Invalid integer: %s", argv[3]);
            return -EINVAL;
        }
		uint16_t pktSize = strtol(argv[4], &endptr, NUM_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "pktCnt Invalid integer: %s", argv[4]);
            return -EINVAL;
        }

        shell_print(shell , "sessionId : %d, attrHandle %d pkt_cnt %d pkt_size %d\n",sessionId , attrHandle, pktCnt, pktSize);
        gatt_server_app_rx_bulk_transfer(sessionId, attrHandle, pktCnt, pktSize);

    }
    #endif
    return 0;
}
#endif
#endif /*CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER */

static int bt_utils_trigger_timesync(const struct shell *shell, size_t argc, char **argv)
{
#if CONFIG_LPAI_BT_TEST_MENU
    shell_print(shell, "bt_utils_trigger_timesync");

    if(argc != 3)
    {
        shell_print(shell , "Usage : <ConnHandle> <Config>\n");
        return -EINVAL;
    }
    else
    {
        char *endptr;
        uint16_t connHandle = strtol(argv[1], &endptr, STRTOL_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "ConnHandle Invalid integer: %s", argv[1]);
            return -EINVAL;
        }
        uint8_t config = strtol(argv[2], &endptr, STRTOL_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "Config Invalid integer: %s", argv[2]);
            return -EINVAL;
        }
        shell_print(shell , "ConnHandle : %d , Config %d\n",connHandle , config);
        qapi_bt_utils_timesync_req(connHandle, config);
    }

#endif
    return 0;
}

#if defined(CONFIG_LPAI_BTSW_ENABLE_ISLAND_TEST)
static int bt_utils_enter_exit_island(const struct shell *shell , size_t argc , char **argv)
{
    if(argc != 3)
    {
        shell_print(shell , "Usage : <enter_island :1  , exit_island : 0> <string : passwd>\n");
        return -EINVAL;
    }

    else
    {
        char *endptr;
        uint8_t action = strtol(argv[1], &endptr, STRTOL_BASE);
        if (*endptr != '\0') 
        {
            shell_error(shell, "Action Invalid: %s", argv[1]);
            return -EINVAL;
        }

        if (action > 1) 
        {
            shell_error(shell, "Action must be 0 (exit) or 1 (enter)");
            return -EINVAL;
        }

        if (!argv[2] || strlen(argv[2]) == 0) 
        {
            shell_error(shell, "Password cannot be empty");
            return -EINVAL;
        }

        uint8_t *secret_pw = argv[2];
        qapi_bt_utils_enter_exit_island_req(action,secret_pw);
        return 0;

    }
}
#endif




/* Define subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(rfcomm_menu,
    SHELL_CMD(rfcomm_tx, NULL, "RFCOMM TX", rfcomm_cmd2),
    SHELL_CMD(rfcomm_tx_throughput, NULL, "RFCOMM Tx Throughput", rfcomm_cmd1),
    SHELL_CMD(rfcomm_loopback_test, NULL, "RFCOMM Loopback", rfcomm_cmd3),
    SHELL_CMD(rfcomm_socket_close, NULL, "RFCOMM Socket Close", rfcomm_cmd4),
    SHELL_CMD(rfcomm_socket_details, NULL, "RFCOMM Socket Details", rfcomm_cmd5),
    SHELL_CMD(rfcomm_rx_throughput, NULL, "RFCOMM Rx Throughput", rfcomm_cmd6),
    SHELL_SUBCMD_SET_END
);

/* Define subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(lecoc_menu,
    SHELL_CMD(lecoc_tx, NULL, "LECOC TX", lecoc_cmd1),
    SHELL_CMD(lecoc_tx_tput, NULL, "LECOC TX Throughput", lecoc_cmd2),
    SHELL_CMD(lecoc_loopback, NULL, "LECOC Loopback", lecoc_cmd3),
    SHELL_CMD(lecoc_socket_close, NULL, "LECOC Socket Close", lecoc_cmd4),
    SHELL_CMD(lecoc_socket_details, NULL, "LECOC Socket details", lecoc_cmd5),
    SHELL_CMD(lecoc_enable_audiotone, NULL, "lecoc_enable_audiotone", lecoc_cmd6),
    SHELL_CMD(lecoc_enable_display, NULL, "lecoc_enable_display", lecoc_cmd7),
    SHELL_CMD(lecoc_rx_tput, NULL, "LECOC RX Throughput", lecoc_cmd8),
    SHELL_SUBCMD_SET_END
);

/* Define subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(audio_tone,
    SHELL_CMD(fetch_toneid, NULL, "Get Tone id", audio_tone_cmd1),
    SHELL_CMD(configure_audio, NULL, "Configure tone id", audio_tone_cmd2),
    SHELL_SUBCMD_SET_END
);

/* Define subcommands */
#if defined(CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)
SHELL_STATIC_SUBCMD_SET_CREATE(gatt_menu,
    SHELL_CMD(gatt_app_unoffload_req, NULL, "gatt_app_unoffload_req", gatt_unoffload_req),
    SHELL_CMD(gattClient_app_read_req, NULL, "gattClient_app_read_req", gatt_client_app_read_req),
    SHELL_CMD(gattClient_app_write_req, NULL, "gattClient_app_write_req", gatt_client_app_write_req),
#ifdef CLIENT_APP_BULK_TRANSFER
    SHELL_CMD(gattClient_tx_bulk_transfer, NULL, "gattClient_tx_bulk_transfer", gatt_client_tx_bulk_transfer),
    SHELL_CMD(gattClient_rx_bulk_transfer, NULL, "gattClient_rx_bulk_transfer", gatt_client_rx_bulk_transfer),
    SHELL_CMD(print_throughput, NULL, "last_throughput", last_throughput),
    SHELL_CMD(exit_throughput, NULL, "exit_throughput", exit_throughput),

#endif /* CLIENT_APP_BULK_TRANSFER */

    SHELL_CMD(gattServer_app_send_notif, NULL, "gattServer_app_send_notif", gatt_server_app_send_notif),
#ifdef SERVER_APP_BULK_TRANSFER
    SHELL_CMD(gattServer_tx_bulk_transfer, NULL, "gattServer_tx_bulk_transfer", gatt_server_tx_bulk_transfer),
    SHELL_CMD(gattServer_rx_bulk_transfer, NULL, "gattServer_rx_bulk_transfer", gatt_server_rx_bulk_transfer),
#endif
    SHELL_SUBCMD_SET_END
);
#endif /* CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER */

SHELL_STATIC_SUBCMD_SET_CREATE(utils_menu,
    SHELL_CMD(bt_utils_timesync, NULL, "Trigger timesync ", bt_utils_trigger_timesync),
    #if defined(CONFIG_LPAI_BTSW_ENABLE_ISLAND_TEST)
        SHELL_CMD(bt_utils_enterIsland, NULL, "Enter/Exit Island ADSP ", bt_utils_enter_exit_island),
    #endif
    SHELL_SUBCMD_SET_END
);



/* Define subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(cmd_menu,
    SHELL_CMD(status, NULL, "Respond with Bt Status", cmd_bt_status),
    SHELL_CMD(endpointsInfo, NULL, "Show End Point info", cmd_list_endpoints),
    SHELL_CMD(startScanApp,NULL,"Start Le Scan App",cmd_start_le_scan_app),
    SHELL_CMD(regScanner, NULL , "Register Scanner",cmd_reg_le_scanner),
    SHELL_CMD(enableScan, NULL , "Enable Scanner",cmd_enable_disable_scanner),
    SHELL_CMD(unregScanner , NULL , "Unregister Scanner",cmd_unreg_le_scanner),
    SHELL_CMD(startAdvApp,NULL,"Start Le Adv App",cmd_start_le_adv_app),
    SHELL_CMD(regAdvSet, NULL , "RegisterAdvSet",cmd_reg_adv_set),
    SHELL_CMD(unregAdvSet,NULL,"Unregister Adv Set",cmd_unreg_adv_set),
    SHELL_CMD(setAdvParams, NULL , "Set Advertisement Params",cmd_set_adv_params),
    SHELL_CMD(setAdvData,NULL,"Set Advertisement Data",cmd_set_adv_data),
    SHELL_CMD(setAdvRandomAddr,NULL,"Set Random Address",cmd_set_adv_random_addr),
    SHELL_CMD(enableAdv,NULL,"Enable Advertisement",cmd_reg_start_adv),
    SHELL_CMD(disableAdv, NULL , "Disable Advertisement",cmd_reg_stop_adv),
    SHELL_CMD(rfcomm,&rfcomm_menu,"RFCOMM Menu",NULL),
    SHELL_CMD(lecoc,&lecoc_menu,"LECOC Menu",NULL),
#if defined(CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD_GHEADER)
    SHELL_CMD(gatt,&gatt_menu,"GATT Menu",NULL),
#endif /* CONFIG_LPAI_BTSW_ENABLE_GATT_OFFLOAD */
	SHELL_CMD(audio_tone,&audio_tone,"Audio Tone Menu",NULL),
	SHELL_CMD(bt_utils,&utils_menu,"BT Utils Menu",NULL),
    SHELL_SUBCMD_SET_END
);

/* Register the main Bluetooth Command */
SHELL_CMD_REGISTER(bluetooth, &cmd_menu, "Bluetooth Test Commands", NULL);
