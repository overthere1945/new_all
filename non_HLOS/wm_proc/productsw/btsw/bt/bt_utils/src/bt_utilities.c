#include "bt_utilities.h"
#include "qapi_bt_utilities.h"
#include "stdbool.h"
#include <inttypes.h>
#include "lpai_bt_app_mgr_client_interface.h"

#define BT_UTILS_APP_HANDLE 0x7828


bool encode_bytes(pb_ostream_t * stream, const pb_field_t * field, void * const * arg)
{
    char * str = (char *)*arg;
    size_t size = 0;
    if (!pb_encode_tag_for_field(stream, field))
    {
        return false;
    }
    if (str != NULL)
    {
        size = strlen(str);
    }
    return pb_encode_string(stream, (uint8_t *) str, size);
}

bool decode_bytes(pb_istream_t * stream, const pb_field_t * field, void ** arg)
{
    if(!arg)
        return false;
    if (!pb_read(stream, *arg, stream->bytes_left))
    {
        printk("Failed bytes decode\n");
        return false;
    }
        return true;
}

bool encode_string(pb_ostream_t * stream, const pb_field_t * field, void * const * arg)
{
    char *str = (char *)*arg;
    if (!pb_encode_tag_for_field(stream, field))
    {
        return false;
    }
    return pb_encode_string(stream, (uint8_t *) str, strlen(str));
}

bool decode_string(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    //argument check
    if(!arg)
        return false;

    //argument check
    if(!(*arg))
        return false;

    if (!pb_read(stream, *arg, stream->bytes_left))
        return false;

    return true;
}
void bt_log_byte_stream(uint8_t * str, uint16_t len)
{
    if (!str || !len)
        return;
    for (uint16_t i=0; i<len; i++)
        printk("%x ", str[i]);
    printk("\n");
}

/**
 * @brief Bt Utils Callback Function registered with App Manager on AWM
 * This function is invoked each time a bt utils event is received on AWM from ADSP
 * @param[in] eventId         Event Identifier to specify type of event
 * @param[in] appDataLen      Length of the Data received along with the event
 * @param[in] appData         Pointer to actual data specific for each event
 * @param[in] proto_encoded   Bool to specifiy if data is proto encoded or not
 * @return    None
 */
void btUtilsCb(uint16_t eventId , uint16_t appDataLen , void *appData,bool proto_encoded)
{
	printk("Event Id : %d\n",eventId);
    switch(eventId)
    {
        case BT_UTILS_TIME_SYNC_RSP:
        {

			bt_utils_timesync_rsp_t *rsp = (bt_utils_timesync_rsp_t *)appData;
            printk("\n BT_UTILS_TIME_SYNC_RSP status: %x\n", rsp->status);
			
			if (rsp->status == 0)
			{
                printk("\n subOpcode: %x\n config: %x", rsp->subOpcode, rsp->config); 
                printk("\n BtClock: %" PRIu64 " \n AppClock: %" PRIu64 "", rsp->btClock, rsp->appClock);
				printk("\n connHandle: %x\n evtCounter: %x\n timeOffset: %x\n interval: %x\n", rsp->connHandle, rsp->evtCounter, rsp->timeOffset, rsp->interval);
			}

        }
        break;
		default:
		break;
	} 
}

void bt_utils_app_init(void)
{
    app_registration_status_code_t result = lpai_bt_app_mgr_register_microapp(BT_UTILS_APP_HANDLE,btUtilsCb);
    printk("bt utils App Registration Status : %d\n",result);
}

/**
 * @brief Exposes Timesync functionality.
 * 
 * @param req Notification structure.
 * @return success.
 */
bool qapi_bt_utils_timesync_req(uint16_t connHandle, uint8_t config)
{
	bt_utils_timesync_req_t req = {.connHandle = connHandle, .config = config};
    lpai_bt_appmgr_send_microapp_msg_adsp
        (BT_UTILS_TIME_SYNC , sizeof(bt_utils_timesync_req_t) , &req);
		
	return true;
}

#if defined(CONFIG_LPAI_BTSW_ENABLE_ISLAND_TEST)
void qapi_bt_utils_enter_exit_island_req(bool action , uint8_t *secret_pw)
{
    /*Enter Island */
    static bool island_entry_enforced = false;
    uint8_t entry = action;
    uint8_t glink_result;
    
    const char *expected_pw = "amaterasu-sectumsempra";
    size_t expected_len = strlen(expected_pw);

    if(!secret_pw || strlen((char*)secret_pw) != expected_len || 
       memcmp(secret_pw, expected_pw, expected_len) != 0)
    {
        printk("Get Sealed with Totsuka Blade for Eternity\n");
        return;
    }

    if(action == true && island_entry_enforced == false)
    {
       glink_result = lpai_bt_appmgr_send_microapp_msg_adsp
            (BT_UTILS_ENTER_ISLAND , sizeof(entry) , &entry);
        
        if(glink_result == 0)
        {
            island_entry_enforced = true;
        }
    }
    else if(action == false && island_entry_enforced == true)
    {
        glink_result = lpai_bt_appmgr_send_microapp_msg_adsp
            (BT_UTILS_EXIT_ISLAND , sizeof(entry) , &entry);

        if(glink_result == 0)
        {
            island_entry_enforced = false;
        }
        
    }
}
#endif