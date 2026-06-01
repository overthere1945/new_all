/*************************************************************************
 * @file     lpai_bt_audio_tone.c
 * @brief    LPAI BT AUDIO TONE source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "lpai_bt_audio_tone.h"

qapi_get_tone_id_t supported_tone_id;
audio_tone_info_t audio_tone_ctx;

/* To fetch the available tone id*/

qapi_bt_audiotone_status_code_t qapi_bt_get_supported_tone_ids()
{
    qapi_as_global_client_cb_t global_cb_info;
    global_cb_info.evt_cb = audio_global_client_cb;
    if(qapi_audio_get_supported_tones(global_cb_info) == QAPI_AS_ERROR_TYPE_SUCCESS)
    {
        return QAPI_BT_ERROR_TYPE_SUCCESS;
    }
    return QAPI_BT_ERROR_TYPE_FAIL;
}


void audio_global_client_cb(qapi_at_client_cb_event evt, void* data, int32_t client_cookie)
{
    switch(evt)
    {
        case QAPI_AS_CLIENT_CB_EVENT_GET_SUPPORTED_TONE_IDS:
        {
            printk("QAPI_AS_CLIENT_CB_EVENT_GET_SUPPORTED_TONE_IDS \n");
            qapi_as_client_cb_event_getsupported_effects_t *tone_data = (qapi_as_client_cb_event_getsupported_effects_t *)data;
            supported_tone_id.num_tones = tone_data->num_tones;
            for(uint8_t idx = 0; idx < supported_tone_id.num_tones; idx++)
            {
                supported_tone_id.tone_id[idx] = (qapi_as_client_cb_event_getsupported_effects_t*)tone_data->tone_info[idx].tone_id;
                printk("tone id %d\n",supported_tone_id.tone_id[idx]);
            }
            break;
        }
    }
}

qapi_bt_audiotone_status_code_t qapi_bt_configure_audio_tone(bool enable, int32_t tone_id)
{
	if(enable == true && audio_tone_ctx.enable_audio_tone == true)
	{
        printk("audio tone already enabled\n");
		return QAPI_BT_ERROR_TYPE_FAIL;
	}
	else if (enable == false && audio_tone_ctx.enable_audio_tone == true && audio_tone_ctx.session_state != None )
	{ 
        if(audio_tone_ctx.session_state == Opened || audio_tone_ctx.session_state == Opening)
        {
            return audio_tone_session_close();
        }
        else if(audio_tone_ctx.session_state == Audio_playing)
        {
            return qapi_audio_tone_stop_play();
        } 
        return QAPI_BT_ERROR_TYPE_FAIL;
    }
	else
	{
        printk("audio tone configured\n");
        audio_tone_ctx.enable_audio_tone = enable;
        audio_tone_ctx.tone_id = tone_id;
        audio_tone_ctx.session_state = None;
        audio_tone_ctx.pending_notification = 0;
        return QAPI_BT_ERROR_TYPE_SUCCESS;
    }
    printk("no case\n");
    return QAPI_BT_ERROR_TYPE_FAIL;
}

void audio_tone_session_open()
{
    
    if(audio_tone_ctx.enable_audio_tone == true)
    {
        if(audio_tone_ctx.session_state == None)
        {
            qapi_as_player_device_t device_config;
            device_config.num_devices = 1;
            device_config.id[0] = QAPI_AS_PLAYER_DEVICE_OUT_SPEAKER;
            qapi_as_session_client_cb_t qapi_audio_session_client_cb;
            qapi_audio_session_client_cb.evt_cb = qapi_audio_session_cb;
            qapi_audio_session_client_cb.cookie = 10;
            if(qapi_audio_session_open(QAPI_AS_PLAYBACK_REQUEST_TYPE_NOTIFICATION, audio_tone_ctx.tone_id, &device_config, &qapi_audio_session_client_cb) == QAPI_AS_ERROR_TYPE_SUCCESS)
            {
                printk("audio session opened\n");
                audio_tone_ctx.session_state = Opening;
            }
        }
        else if(audio_tone_ctx.session_state == Opened)
        {
            audio_tone_start_play();
        }
        else{
            audio_tone_ctx.pending_notification++;
        }
    }
    else
    {
        printk("audio tone is disable");
    }
}


qapi_bt_audiotone_status_code_t audio_tone_session_close()
{
	/* This function shall be called only if user configures to disable audio tone. So cleanup needed.*/

    uint8_t error_code = QAPI_BT_ERROR_TYPE_FAIL;
    if(audio_tone_ctx.enable_audio_tone == true)
    {
        error_code = qapi_audio_session_close(audio_tone_ctx.session_id, audio_tone_ctx.session_cookie);
        printk("qapi_audio_session_close %d\n", error_code);
        memset(&audio_tone_ctx, 0 , sizeof(audio_tone_info_t));
        audio_tone_ctx.session_state = None;
    }
    return error_code;
}

void audio_tone_start_play()
{
    printk("audio_tone_start_play\n");
    if(audio_tone_ctx.enable_audio_tone == true)
    {
        if(qapi_audio_session_start(audio_tone_ctx.session_id, audio_tone_ctx.session_cookie) == QAPI_AS_ERROR_TYPE_SUCCESS)
        {
            audio_tone_ctx.session_state = Audio_playing;
        }
    }
}


qapi_bt_audiotone_status_code_t qapi_audio_tone_stop_play()
{
    if(audio_tone_ctx.enable_audio_tone == true)
    {
        uint8_t error_code = qapi_audio_session_stop(audio_tone_ctx.session_id, audio_tone_ctx.session_cookie);
        printk("qapi_audio_session_stop %d\n", error_code);
        if(error_code == QAPI_AS_ERROR_TYPE_SUCCESS)
        {
            /* Audio session will be closed and cleanup is done with response for stop */
            audio_tone_ctx.session_state = Opened;
            audio_tone_ctx.stop_in_progress = true;
        }
        else
        {
            /* Cleanup required in failure case */
            memset(&audio_tone_ctx, 0 , sizeof(audio_tone_info_t));
            audio_tone_ctx.session_state = None;
        }
        return error_code;
    }
    return QAPI_BT_ERROR_TYPE_FAIL;
}
void qapi_audio_session_cb(qapi_at_client_cb_event evt, void* data, int32_t client_cookie, int32_t session_id, int32_t session_cookie)
{
    printk("callback event= %d\n",evt);

    switch(evt)
    {
        case QAPI_AS_CLIENT_CB_EVENT_PLAYER_OPEN:
        {
            qapi_as_client_cb_event_player_open_t *evt = (qapi_as_client_cb_event_player_open_t* )data;
            printk("QAPI_AS_CLIENT_CB_EVENT_PLAYER_OPEN response = %d audio_err_code = %d\n", evt->response, evt->audio_err_code);
            qapi_audio_session_set_volume(session_id,session_cookie, 0.2);
            audio_tone_ctx.session_state = Opened;
            audio_tone_ctx.session_id = session_id;
            audio_tone_ctx.session_cookie = session_cookie;
            audio_tone_start_play();
            break;
        }
        case QAPI_AS_CLIENT_CB_EVENT_PLAYER_CLOSE:
        {
            qapi_as_client_cb_event_player_close_t *evt = (qapi_as_client_cb_event_player_close_t* )data;
            printk("QAPI_AS_CLIENT_CB_EVENT_PLAYER_CLOSE response = %d audio_err_code = %d\n", evt->response, evt->audio_err_code);
            break;
        }
        case QAPI_AS_CLIENT_CB_EVENT_PLAYER_START:
        {
            qapi_as_client_cb_event_player_start_t *evt = (qapi_as_client_cb_event_player_start_t*)data;
            printk("QAPI_AS_CLIENT_CB_EVENT_PLAYER_START response = %d audio_err_code = %d\n", evt->response, evt->audio_err_code);
            break;
        }
        case QAPI_AS_CLIENT_CB_EVENT_PLAYER_STOP:
        {
            qapi_as_client_cb_event_player_stop_t *evt = (qapi_as_client_cb_event_player_stop_t*)data;
            printk("QAPI_AS_CLIENT_CB_EVENT_PLAYER_STOP response = %d audio_err_code = %d\n", evt->response, evt->audio_err_code);
            audio_tone_session_close();
            break;
        }
        case QAPI_AS_CLIENT_CB_EVENT_PLAYER_SET_VOL:
        {
            break;
        }
        case QAPI_AS_CLIENT_CB_EVENT_PLAYER_GET_VOL:
        {
            break;
        }
        case QAPI_AS_CLIENT_CB_EVENT_PLAYER_STOPPED:
        {
            qapi_as_client_cb_event_player_stopped_t *evt = (qapi_as_client_cb_event_player_stopped_t*)data;
            printk("QAPI_AS_CLIENT_CB_EVENT_PLAYER_STOPPED reason = %d\n", evt->reason);

            /* Below check is to handle race condition of receiving this event while waiting for QAPI_AS_CLIENT_CB_EVENT_PLAYER_STOP */
            if (audio_tone_ctx.stop_in_progress == true)
            {
                audio_tone_session_close();
                return;
            }
            audio_tone_ctx.session_state = Opened;
            if(audio_tone_ctx.pending_notification > 0)
            {
                audio_tone_ctx.pending_notification--;
                audio_tone_start_play();
            }
            break;
        }
        case QAPI_AS_CLIENT_CB_EVENT_SESSION_TERMINATED:
        {
            qapi_as_client_cb_event_session_terminated_t *evt = (qapi_as_client_cb_event_session_terminated_t*)data;
            printk("QAPI_AS_CLIENT_CB_EVENT_SESSION_TERMINATED reason = %d\n", evt->reason);

            /* Below check is to handle race condition of receiving this event while waiting for QAPI_AS_CLIENT_CB_EVENT_PLAYER_STOP */
            if (audio_tone_ctx.stop_in_progress == true)
            {
                memset(&audio_tone_ctx, 0 , sizeof(audio_tone_info_t));
                audio_tone_ctx.session_state = None;
                return;
            }

			audio_tone_ctx.session_state = None;
			audio_tone_ctx.session_id = session_id;
			audio_tone_ctx.session_cookie = session_cookie;
			if (audio_tone_ctx.pending_notification > 0)
			{
				audio_tone_ctx.pending_notification--;
				audio_tone_session_open();
			}
        }
		break;
        case QAPI_AS_CLIENT_CB_EVENT_REQUESTS_ON_INVALID_COOKIE:
        {
            break;
        }
    }
}