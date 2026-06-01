/****************************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 *
 ****************************************************************************/
/*****************************************************************************
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*     btaudio_sink_offload.cpp
 *
 *  Description:   Implements IPC interface between HAL and BT host
 *
 *****************************************************************************/
#include <time.h>
#include <unistd.h>
#include "btaudio_offload.h"
#include "btaudio_offload_qti.h"
#include "btaudio_offload_qti_2_1.h"
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <system/audio.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include "BluetoothAudioProviderFactory.h"
#include "aidl_session/BluetoothAudioSessionControl.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "btaudio_offload_sink"
#define FAIL_TRIAL_COUNT 10
/*****************************************************************************
**  Constants & Macros
******************************************************************************/
/* 3DH5 Max (1021) = 1005 + 12 (AVDTP header) + 4 (L2CAP header) */
#define MAX_3MBPS_A2DP_STREAM_MTU 1005
#define AAC_SAMPLE_SIZE  1024
#define AAC_LATM_HEADER  12
#define CASE_RETURN_STR(const) case const: return #const;
/*****************************************************************************
**  Local type definitions
******************************************************************************/
int  max_waittime = 4500; // in ms
static struct a2dp_stream_common audio_stream;
static bool config_thread_created = false;
static pthread_t thread_config_changed;
static std::mutex api_lock;
tSESSION_TYPE session_type = A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH;
using ::aidl::android::hardware::bluetooth::audio::BluetoothAudioProviderFactory;

using BluetoothAudioSessionControl =
        aidl::android::hardware::bluetooth::audio::BluetoothAudioSessionControl;
extern binder_status_t createIBluetoothAudioProviderFactory();
/*****************************************************************************
**  Static functions
******************************************************************************/
audio_aac_decoder_config_t aac_dec;
audio_sbc_decoder_config_t sbc_dec;

/*****************************************************************************
**   Miscellaneous helper functions
******************************************************************************/
static const char* dump_ctrl_ack(tCTRL_ACK ack)
{
    switch (ack) {
        CASE_RETURN_STR(CTRL_ACK_SUCCESS)
        CASE_RETURN_STR(CTRL_ACK_FAILURE)
        CASE_RETURN_STR(CTRL_ACK_INCALL_FAILURE)
        CASE_RETURN_STR(CTRL_ACK_PENDING)
        CASE_RETURN_STR(CTRL_ACK_DISCONNECT_IN_PROGRESS)
        CASE_RETURN_STR(CTRL_SKT_DISCONNECTED)
        CASE_RETURN_STR(CTRL_ACK_UNSUPPORTED)
        CASE_RETURN_STR(CTRL_ACK_UNKNOWN)
        CASE_RETURN_STR(CTRL_ACK_RECONFIGURATION)
    }
    return "UNKNOWN A2DP_CTRL_ACK";
}

static const char* dump_a2dp_hal_state(tAUDIO_STATE state)
{
    switch (state) {
        CASE_RETURN_STR(AUDIO_STATE_STARTING)
        CASE_RETURN_STR(AUDIO_STATE_STARTED)
        CASE_RETURN_STR(AUDIO_STATE_STOPPING)
        CASE_RETURN_STR(AUDIO_STATE_STOPPED)
        CASE_RETURN_STR(AUDIO_STATE_SUSPENDED)
        CASE_RETURN_STR(AUDIO_STATE_STANDBY)
        CASE_RETURN_STR(AUDIO_STATE_RECONFIG_SUSPENDED)
    }
    return "UNKNOWN A2DP_STATE";
}

tCTRL_ACK aidl_to_audio_status(BluetoothAudioStatus status) {
    tCTRL_ACK ret = CTRL_ACK_UNKNOWN;
    switch (status) {
        case BluetoothAudioStatus::UNKNOWN: ret = CTRL_ACK_UNKNOWN;
            break;
        case BluetoothAudioStatus::SUCCESS: ret = CTRL_ACK_SUCCESS;
            break;
        case BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION: ret = CTRL_ACK_UNSUPPORTED;
            break;
        case BluetoothAudioStatus::FAILURE: ret = CTRL_ACK_FAILURE;
            break;
        case BluetoothAudioStatus::RECONFIGURATION: ret = CTRL_ACK_RECONFIGURATION;
          break;
        default: ret = CTRL_ACK_UNKNOWN;
            break;
    }
    return ret;
}

/*****************************************************************************
**
** AUDIO DATA PATH
**
*****************************************************************************/
void stack_resp_cb (const uint16_t& cntrl_key __unused, const bool& start_resp,
                     const BluetoothAudioStatus&  status) {
  ALOGI("%s: status = %hhu", __func__, status);
  pthread_mutex_lock(&audio_stream.ack_lock);
  audio_stream.ack_status = aidl_to_audio_status(status);
  if (!audio_stream.ack_recvd) {
    audio_stream.ack_recvd = 1;
    pthread_cond_signal(&audio_stream.ack_cond);
  }
  pthread_mutex_unlock(&audio_stream.ack_lock);
}

void session_resp_cb (const uint16_t& cntrl_key)
{
  ALOGI("%s", __func__);
  pthread_mutex_lock(&audio_stream.ack_lock);
  ALOGI("%s: session ready  = %d", __func__, audio_stream.session_ready);
  if (audio_stream.session_ready || audio_stream.state == AUDIO_STATE_STARTED
                || audio_stream.state == AUDIO_STATE_STARTING)
  {
    audio_stream.state = AUDIO_STATE_STANDBY;
    audio_stream.session_ready = 0;
    ALOGI("%s:  end session session ready  = %d", __func__, audio_stream.session_ready);
  } else if ((!audio_stream.session_ready)) {
    audio_stream.session_ready = 1;
    audio_stream.state = AUDIO_STATE_STANDBY;
    memset(&audio_stream.codec_cfg, 0, sizeof(CodecConfiguration));
    audio_stream.codec_cfg = BluetoothAudioSessionControl::GetAudioConfig(audio_stream.sessionType);
  }
  ALOGD("%s: state = %s", __func__, dump_a2dp_hal_state(audio_stream.state));
  if (!audio_stream.ack_recvd) {
    audio_stream.ack_recvd = 1;
    pthread_cond_signal(&audio_stream.ack_cond);
  }
  pthread_mutex_unlock(&audio_stream.ack_lock);
}

static void btapoffload_port_init()
{
  ALOGI("%s", __func__);
  if (BluetoothAudioSessionControl::IsSessionReady(audio_stream.sessionType)) {
    audio_stream.state = AUDIO_STATE_STANDBY;
    audio_stream.session_ready = 1;
    memset(&audio_stream.codec_cfg, 0, sizeof(CodecConfiguration));
    audio_stream.codec_cfg = BluetoothAudioSessionControl::GetAudioConfig(audio_stream.sessionType);
  } else
    ALOGE("%s, bluetooth provider session is not avail", __func__);

  audio_stream.control_result_cb = stack_resp_cb;
  audio_stream.session_changed_cb = session_resp_cb;
  ::aidl::android::hardware::bluetooth::audio::PortStatusCallbacks cbacks = {
    .control_result_cb_ = audio_stream.control_result_cb,
    .session_changed_cb_ = audio_stream.session_changed_cb};
    audio_stream.ctrl_key = BluetoothAudioSessionControl::RegisterControlResultCback(
       audio_stream.sessionType, cbacks);
}

static void btapoffload_port_deinit(tSESSION_TYPE session_type, bool unreg) {
  ALOGI("%s session_type = %d, ctrl_key = %d", __func__, audio_stream.sessionType, audio_stream.ctrl_key);
  if (unreg)
    BluetoothAudioSessionControl::UnregisterControlResultCback(audio_stream.sessionType,audio_stream.ctrl_key);
  audio_stream.ctrl_key = 0;
}

void a2dp_stream_common_init(struct a2dp_stream_common *common)
{
  pthread_mutexattr_t lock_attr;
  ALOGI("%s", __func__);
  pthread_mutexattr_init(&lock_attr);
  pthread_mutexattr_settype(&lock_attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&common->lock, &lock_attr);
  pthread_mutexattr_destroy(&lock_attr);
  memset(common,0,sizeof(a2dp_stream_common));
  common->sink_latency = A2DP_DEFAULT_SINK_LATENCY;
  common->sessionType = SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH;
  btapoffload_port_init();
}

int wait_for_stack_response(int time_to_wait, tSESSION_TYPE session_type)
{
  ALOGD("%s", __func__);
  struct timespec now;
  int ret = 0;
  pthread_mutex_lock(&audio_stream.ack_lock);
  if (audio_stream.session_ready== 0) {
    ALOGE("%s: stack deinitialized", __func__);
    pthread_mutex_unlock(&audio_stream.ack_lock);
    return ret;
  }
  ALOGW("%s: entering conditional wait: ack_recvd = %d",
            __func__,audio_stream.ack_recvd);
  clock_gettime(CLOCK_REALTIME, &now);
  now.tv_sec += (time_to_wait / 1000);
  if (!audio_stream.ack_recvd) {
    pthread_cond_timedwait(&audio_stream.ack_cond,
                             &audio_stream.ack_lock, &now);
  }
  if (audio_stream.ack_recvd) {
    ALOGV("%s: ack received", __func__);
  }
  else {
    clock_gettime(CLOCK_REALTIME, &now);
    now.tv_nsec += ((time_to_wait % 1000) * 1000000);
    pthread_cond_timedwait(&audio_stream.ack_cond,
                           &audio_stream.ack_lock, &now);
  }
  pthread_mutex_unlock(&audio_stream.ack_lock);
  return ret;
}

static void* a2dp_codec_parser(AudioConfiguration *codec_cfg,
                               audio_format_t *codec_type,
                               uint32_t *sample_freq)
{
  CodecConfiguration *codecConfig =
        &codec_cfg->get<AudioConfiguration::a2dpConfig>();
  ALOGI("%s: codec_type = %x", __func__, codecConfig->codecType);
  if (codecConfig->codecType == CodecType::SBC) {
    memset(&sbc_dec, 0, sizeof(audio_sbc_decoder_config_t));
    SbcConfiguration *sbcConfig =
       &codecConfig->config.get<CodecConfiguration::CodecSpecific::sbcConfig>();
    switch (sbcConfig->sampleRateHz) {
      case 48000:
        sbc_dec.sampling_rate = 48000;
        break;
      case 44100:
        sbc_dec.sampling_rate = 44100;
        break;
      default:
        ALOGE("%s: SBC: unknown sampling rate:%d", __func__,
              sbcConfig->sampleRateHz);
        break;
    }
    switch (sbcConfig->channelMode) {
      case SbcChannelMode::STEREO:
        sbc_dec.channels = A2D_SBC_CHANNEL_STEREO;
        break;
      case SbcChannelMode::JOINT_STEREO:
        sbc_dec.channels = A2D_SBC_CHANNEL_JOINT_STEREO;
        break;
      case SbcChannelMode::DUAL:
        sbc_dec.channels = A2D_SBC_CHANNEL_DUAL_MONO;
        break;
      case SbcChannelMode::MONO:
        sbc_dec.channels = A2D_SBC_CHANNEL_MONO;
        break;
      default:
        ALOGE("%s: SBC: unknown channel mode:%u", __func__,
              (unsigned)sbcConfig->channelMode);
        break;
    }
    switch (sbcConfig->bitsPerSample) {
      case 16:
        sbc_dec.bits_per_sample = 16;
        break;
      case 24:
        sbc_dec.bits_per_sample = 24;
        break;
      case 32:
        sbc_dec.bits_per_sample = 32;
        break;
      default:
        ALOGE("%s: SBC: unknown bits per sample", __func__);
        break;
    }
    if (codecConfig->encodedAudioBitrate == 0) {
      ALOGW("%s: SBC: bitrate is zero", __func__);
      sbc_dec.snk_buffer.bitrate = 328000;
    } else if (codecConfig->encodedAudioBitrate >= 0x00000001 &&
               codecConfig->encodedAudioBitrate <= 0x00FFFFFF) {
      sbc_dec.snk_buffer.bitrate = codecConfig->encodedAudioBitrate;
    }
    ALOGI("%s: SBC: bitrate:%d i/p bitrate:%d", __func__, sbc_dec.snk_buffer.bitrate,
          codecConfig->encodedAudioBitrate);
    sbc_dec.snk_buffer.bitrate_mode = 2; // max
    sbc_dec.snk_buffer.mtu = codecConfig->peerMtu;
    *codec_type = AUDIO_FORMAT_SBC;

    if (sample_freq) *sample_freq = sbc_dec.sampling_rate;

    ALOGI("%s: SBC: done copying full codec config", __func__);
    return ((void *)(&sbc_dec));

  } else if (codecConfig->codecType == CodecType::AAC) {
    AacConfiguration *aacConfig =
      &codecConfig->config.get<CodecConfiguration::CodecSpecific::aacConfig>();
    memset(&aac_dec, 0, sizeof(audio_aac_decoder_config_t));
    aac_dec.obj_type = 0;
    aac_dec.format_flag = 4;
    switch (aacConfig->sampleRateHz) {
      case 44100:
        aac_dec.sampling_rate = 44100;
        break;
      case 48000:
        aac_dec.sampling_rate = 48000;
        break;
      case 88200:
        aac_dec.sampling_rate = 88200;
        break;
      case 96000:
        aac_dec.sampling_rate = 96000;
        break;
      default:
        ALOGE("%s: AAC: invalid sample rate:%d", __func__,
              aacConfig->sampleRateHz);
        break;
    }
    switch (aacConfig->channelMode) {
      case ChannelMode::MONO:
        aac_dec.channels = 1;
        break;
      case ChannelMode::STEREO:
        aac_dec.channels = 2;
        break;
      default:
        ALOGE("%s: AAC: unknown channel mode:%u", __func__,
              (unsigned)aacConfig->channelMode);
        break;
    }

    switch (aacConfig->bitsPerSample) {
      case 16:
        aac_dec.bits_per_sample = 16;
        break;
      case 24:
        aac_dec.bits_per_sample = 24;
        break;
      case 32:
        aac_dec.bits_per_sample = 32;
        break;
      default:
        ALOGE("%s: AAC: unknown bits per sample", __func__);
        break;
    }

    uint32_t bitrate_hal = 0;
    if (codecConfig->encodedAudioBitrate == 0) {
      ALOGW("%s: AAC: bitrate is zero", __func__);
    } else if (codecConfig->encodedAudioBitrate >= 0x00000001 &&
               codecConfig->encodedAudioBitrate <= 0x00FFFFFF) {
      bitrate_hal = codecConfig->encodedAudioBitrate;
    }

    *codec_type = AUDIO_FORMAT_AAC;
    ALOGI("%s: AAC HAL: bitrate:%" PRIu32 " peermtu:%" PRIu16, __func__,
          bitrate_hal, codecConfig->peerMtu);
    uint16_t mtu = codecConfig->peerMtu;
    if ((mtu == 0) || (mtu > MAX_3MBPS_A2DP_STREAM_MTU))
        mtu = MAX_3MBPS_A2DP_STREAM_MTU;

    aac_dec.snk_buffer.bitrate = bitrate_hal;
    aac_dec.snk_buffer.bitrate_mode = 2;   // MAX
    aac_dec.snk_buffer.mtu = mtu;   // MAX

    ALOGI("%s: AAC: done copying full codec config", __func__);
    return ((void *)(&aac_dec));
  }
  return NULL;
}

void bt_audio_pre_init() {
  ALOGD("%s: calling preinit", __func__);
  createIBluetoothAudioProviderFactory();
}

int audio_stream_open() {
  ALOGI("%s", __func__);
  a2dp_stream_common_init(&audio_stream);
  audio_stream.session_ready =
         BluetoothAudioSessionControl::IsSessionReady(audio_stream.sessionType);
  if (audio_stream.session_ready) {
    ALOGI("%s: success", __func__);
    return 0;
  }

  btapoffload_port_deinit(session_type, true);
  ALOGE("%s: failed", __func__);
  return -1;
}

int audio_stream_close() {
  ALOGI("%s", __func__);
  ALOGI("%s: session type: %d", __func__, session_type);
  tCTRL_ACK status = CTRL_ACK_UNKNOWN;
  std::unique_lock<std::mutex> lock(api_lock);
  if (audio_stream.state == AUDIO_STATE_STARTED ||
    audio_stream.state == AUDIO_STATE_STOPPING) {
    audio_stream.state = AUDIO_STATE_STOPPED;
    if (audio_stream.session_ready) {
      audio_stream.ack_status = CTRL_ACK_UNKNOWN;
      audio_stream.ack_recvd = 0;
      ALOGW("%s: suspending audio stream", __func__);
      auto ret = BluetoothAudioSessionControl::SuspendStream(audio_stream.sessionType);
      if (ret == false) {
        ALOGE("%s: client has died",__func__);
        lock.unlock();
        return -1;
      }
      pthread_mutex_lock(&audio_stream.ack_lock);
      status = audio_stream.ack_status;
      pthread_mutex_unlock(&audio_stream.ack_lock);
      ALOGI("%s: ack status = %s", __func__, dump_ctrl_ack(status));
      if (status == CTRL_ACK_UNKNOWN) {
        wait_for_stack_response(max_waittime, session_type);
        pthread_mutex_lock(&audio_stream.ack_lock);
        status = audio_stream.ack_status;
        pthread_mutex_unlock(&audio_stream.ack_lock);
      }
    }
  }
  btapoffload_port_deinit(session_type, true);
  lock.unlock();
  if (status == CTRL_ACK_UNKNOWN) {
    ALOGE("%s: failed to get ack from stack", __func__);
    return -1;
  } else {
    ALOGI("%s: return success", __func__);
    return 1;
  }
}


void *audio_get_decoder_config(audio_format_t *codec_type) {
  ALOGI("%s", __func__);
  return (a2dp_codec_parser(&audio_stream.codec_cfg, codec_type, NULL));
}

int audio_sink_check_a2dp_ready() {
  tSESSION_TYPE session_type = A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
  ALOGI("%s: state = %s", __func__, dump_a2dp_hal_state(audio_stream.state));
  int status;
  std::unique_lock<std::mutex> lock(api_lock);
  ALOGI("%s: session ready  = %d", __func__, audio_stream.session_ready);
  if (session_type != UNKNOWN && audio_stream.session_ready == 0) {
    ALOGI("%s: session restarted, do port init", __func__);
    audio_stream_open_api(session_type);
  }
  if (audio_stream.session_ready) {
    status = 1;
  } else {
      status = 0;
  }
  lock.unlock();
  return status;
}

int audio_sink_start_capture() {
  int retry_count = 0;
  int wait_time = 0;
  ALOGI("%s", __func__);
  //audio_stream_open();
  int ack_ret;
  tCTRL_ACK status = CTRL_ACK_SUCCESS;
  std::unique_lock<std::mutex> lock(api_lock);
  if (!audio_stream.session_ready &&
    BluetoothAudioSessionControl::IsSessionReady(audio_stream.sessionType)) {
    ALOGD("%s: session state not updated, calling port init",__func__);
    audio_stream_open_api(session_type);
  }
  if (audio_stream.session_ready) {
    ALOGD("%s: state = %s", __func__,
           dump_a2dp_hal_state(audio_stream.state));
    if (audio_stream.state == AUDIO_STATE_SUSPENDED) {
      ALOGW("%s: stream suspended", __func__);
      lock.unlock();
      return -1;
    } else if(audio_stream.state == AUDIO_STATE_STARTED) {
      ALOGW("%s: stream already started", __func__);
      lock.unlock();
      return CTRL_ACK_SUCCESS;
    }
    audio_stream.ack_status = CTRL_ACK_UNKNOWN;
    ALOGE("%s: issues Start stream to stack",__func__);
    audio_stream.ack_recvd = 0;
    auto ret = BluetoothAudioSessionControl::StartStream(audio_stream.sessionType);
    if (!ret) {
      ALOGE("%s: client has died",__func__);
      goto end;
    }
    audio_stream.state = AUDIO_STATE_STARTING;
    pthread_mutex_lock(&audio_stream.ack_lock);
    status = audio_stream.ack_status;
    pthread_mutex_unlock(&audio_stream.ack_lock);
    if (status == CTRL_ACK_UNKNOWN) {
      ack_ret = wait_for_stack_response(max_waittime,session_type);
      pthread_mutex_lock(&audio_stream.ack_lock);
      status = audio_stream.ack_status;
      pthread_mutex_unlock(&audio_stream.ack_lock);
      ALOGD("%s: status = %s", __func__, dump_ctrl_ack(status));
    }
    if (status == CTRL_ACK_SUCCESS) {
      ALOGI("%s: a2dp stream started successfully", __func__);
      audio_stream.state = AUDIO_STATE_STARTED;
      goto end;
    } else if(status == CTRL_ACK_FAILURE) {
      ALOGW("%s: a2dp stream start failed: status = %s", __func__,
               dump_ctrl_ack(status));
      audio_stream.state = AUDIO_STATE_STANDBY;
      retry_count = FAIL_TRIAL_COUNT;
      wait_time = 200;
    } else if(status == CTRL_ACK_UNSUPPORTED ||
          status == CTRL_ACK_DISCONNECT_IN_PROGRESS ||
          status == CTRL_ACK_UNKNOWN) {
      ALOGW("%s: a2dp stream start failed: status = %s", __func__,
               dump_ctrl_ack(status));
      audio_stream.state = AUDIO_STATE_STANDBY;
      goto end;
    } else if(status == CTRL_ACK_RECONFIGURATION) {
      ALOGW("%s: a2dp stream start failed: status = %s", __func__,
               dump_ctrl_ack(status));
      audio_stream.state = AUDIO_STATE_SUSPENDED;
      goto end;
    }
  } else {
    ALOGW("%s: session is not active", __func__);
    lock.unlock();
    return -1;
  }
end:
    lock.unlock();
    return status;
}


int audio_sink_stop_capture() {
  ALOGI("%s: session type: %d", __func__, session_type);
  int ret = 0;
  tCTRL_ACK status;

  if (audio_stream.state == AUDIO_STATE_SUSPENDED ||
    audio_stream.state == AUDIO_STATE_STANDBY ||
    audio_stream.state == AUDIO_STATE_STOPPED) {
    ALOGD("%s: stream in standby/suspended already",__func__);
    return 0;
  }
  std::unique_lock<std::mutex> lock(api_lock);
  if (audio_stream.session_ready) {
    ALOGD("%s: state = %s", __func__,
          dump_a2dp_hal_state(audio_stream.state));
    if (audio_stream.state != AUDIO_STATE_SUSPENDED &&
        audio_stream.state != AUDIO_STATE_STANDBY) {
      audio_stream.ack_recvd = 0;
      audio_stream.ack_status = CTRL_ACK_UNKNOWN;
      auto ret = BluetoothAudioSessionControl::SuspendStream(audio_stream.sessionType);
      if (ret == false) {
        ALOGE("%s: client has died",__func__);
        lock.unlock();
        return -1;
      }
      pthread_mutex_lock(&audio_stream.ack_lock);
      status = audio_stream.ack_status;
      pthread_mutex_unlock(&audio_stream.ack_lock);
      ALOGI("%s: ack status = %s", __func__, dump_ctrl_ack(status));
      if (status == CTRL_ACK_UNKNOWN) {
        audio_stream.state = AUDIO_STATE_STOPPING;
        wait_for_stack_response(max_waittime, session_type);
        pthread_mutex_lock(&audio_stream.ack_lock);
        status = audio_stream.ack_status;
        pthread_mutex_unlock(&audio_stream.ack_lock);
      }
      if (status == CTRL_ACK_SUCCESS) {
        ALOGD("%s: success", __func__);
        audio_stream.state = AUDIO_STATE_STANDBY;
        lock.unlock();
        return 0;
      } else {
        ALOGW("%s: failed", __func__);
        audio_stream.state = AUDIO_STATE_STOPPED;
        lock.unlock();
        return -1;
      }
    } else {
      ALOGD("%s: state is already stopped or in standby",__func__);
      lock.unlock();
      return 0;
    }
  } else {
    ret = -1;
    ALOGW("%s: session is not ready", __func__);
  }
  audio_stream.state = AUDIO_STATE_STOPPED;
  lock.unlock();
  return ret;

}

// create dummy metadata
sink_metadata_t* create_dummy_sink_metadata() {
  sink_metadata_t* sink = (sink_metadata_t*)malloc(1 * sizeof(sink_metadata_t));
  if (sink == NULL)
    return NULL;

  sink->track_count = 1;
  sink->tracks = (record_track_metadata_t*)malloc(1 * sizeof(record_track_metadata_t));
  if(sink->tracks == NULL) {
    free(sink);
    return NULL;
  }
  sink->tracks->gain = 140;
  return sink;
}

int audio_sink_session_setup_complete(uint64_t system_latency) {
  //TODO to pass it on to BT stack
  ALOGI("%s UpdateSinkMetadata", __func__);
  BluetoothAudioSessionControl::UpdateSinkMetadata(audio_stream.sessionType, *((sink_metadata*) create_dummy_sink_metadata()));
  return true;
}

int audio_stream_open_api(tSESSION_TYPE session_type) {
  if (audio_stream.ctrl_key != 0 &&
    audio_stream.session_ready) {
    ALOGW("%s: session is opened already", __func__);
    return 0;
  }
  return audio_stream_open();
}
