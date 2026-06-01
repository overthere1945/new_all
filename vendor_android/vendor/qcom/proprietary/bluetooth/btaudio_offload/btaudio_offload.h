/****************************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 *
 *****************************************************************************/
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

/*****************************************************************************
 *
 *  Filename:      btaudio_offload.h
 *
 *  Description:
 *
 *****************************************************************************/
#ifndef BT_HOST_IPC_H
#define BT_HOST_IPC_H
#include <system/audio.h>
#include "btavs_client.h"

/*****************************************************************************
**  Constants & Macros
******************************************************************************/
#define BT_AUDIO_HARDWARE_INTERFACE "libbthost"

#include <time.h>

#include <aidl/android/hardware/audio/common/SinkMetadata.h>
#include <aidl/android/hardware/audio/common/SourceMetadata.h>

// aidl session
#include <aidl/android/hardware/audio/common/SinkMetadata.h>
#include <aidl/android/hardware/audio/common/SourceMetadata.h>
#include <aidl/android/hardware/bluetooth/audio/BnBluetoothAudioPort.h>

#include <aidl/android/hardware/bluetooth/audio/AacCapabilities.h>
#include <aidl/android/hardware/bluetooth/audio/AacConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/AacObjectType.h>
#include <aidl/android/hardware/bluetooth/audio/AptxCapabilities.h>
#include <aidl/android/hardware/bluetooth/audio/AptxConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/AudioCapabilities.h>
#include <aidl/android/hardware/bluetooth/audio/AudioConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/AudioLocation.h>
#include <aidl/android/hardware/bluetooth/audio/BluetoothAudioStatus.h>
#include <aidl/android/hardware/bluetooth/audio/ChannelMode.h>
#include <aidl/android/hardware/bluetooth/audio/CodecCapabilities.h>
#include <aidl/android/hardware/bluetooth/audio/CodecConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/CodecId.h>
#include <aidl/android/hardware/bluetooth/audio/CodecType.h>
#include <aidl/android/hardware/bluetooth/audio/IBluetoothAudioPort.h>
#include <aidl/android/hardware/bluetooth/audio/IBluetoothAudioProvider.h>
#include <aidl/android/hardware/bluetooth/audio/IBluetoothAudioProviderFactory.h>
#include <aidl/android/hardware/bluetooth/audio/Lc3Capabilities.h>
#include <aidl/android/hardware/bluetooth/audio/Lc3Configuration.h>
#include <aidl/android/hardware/bluetooth/audio/LdacCapabilities.h>
#include <aidl/android/hardware/bluetooth/audio/LdacChannelMode.h>
#include <aidl/android/hardware/bluetooth/audio/LdacConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/LdacQualityIndex.h>
#include <aidl/android/hardware/bluetooth/audio/LeAudioCodecCapabilitiesSetting.h>
#include <aidl/android/hardware/bluetooth/audio/LeAudioCodecConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/LeAudioConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/LeAudioBroadcastConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/PcmCapabilities.h>
#include <aidl/android/hardware/bluetooth/audio/PcmConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/PresentationPosition.h>
#include <aidl/android/hardware/bluetooth/audio/SbcAllocMethod.h>
#include <aidl/android/hardware/bluetooth/audio/SbcCapabilities.h>
#include <aidl/android/hardware/bluetooth/audio/SbcChannelMode.h>
#include <aidl/android/hardware/bluetooth/audio/SbcConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/SessionType.h>
#ifdef AIDL_EXTENSION
#include <aidl/android/hardware/bluetooth/audio/extension/IBluetoothAudioProviderExt.h>
#include <aidl/android/hardware/bluetooth/audio/extension/SessionType.h>
#endif /* AIDL_EXTENSION */

using ::aidl::android::hardware::bluetooth::audio::CodecType;
using ::aidl::android::hardware::bluetooth::audio::CodecId;
using ::aidl::android::hardware::bluetooth::audio::PresentationPosition;
using ::aidl::android::hardware::bluetooth::audio::CodecConfiguration;
using ::aidl::android::hardware::bluetooth::audio::SbcConfiguration;
using ::aidl::android::hardware::bluetooth::audio::SbcChannelMode;
using ::aidl::android::hardware::bluetooth::audio::SbcAllocMethod;
using ::aidl::android::hardware::bluetooth::audio::AacConfiguration;
using ::aidl::android::hardware::bluetooth::audio::AptxConfiguration;
using ::aidl::android::hardware::bluetooth::audio::AptxAdaptiveConfiguration;
using ::aidl::android::hardware::bluetooth::audio::AptxAdaptiveChannelMode;
using ::aidl::android::hardware::bluetooth::audio::AptxAdaptiveCapabilities;
using ::aidl::android::hardware::bluetooth::audio::AptxAdaptiveInputMode;
using ::aidl::android::hardware::bluetooth::audio::AptxMode;
using ::aidl::android::hardware::bluetooth::audio::ChannelMode;
using ::aidl::android::hardware::bluetooth::audio::SessionType;

using ::aidl::android::hardware::bluetooth::audio::LdacChannelMode;
using ::aidl::android::hardware::bluetooth::audio::LdacConfiguration;
using ::aidl::android::hardware::bluetooth::audio::LdacQualityIndex;
using ::aidl::android::hardware::bluetooth::audio::AudioConfiguration;
using ::aidl::android::hardware::bluetooth::audio::BluetoothAudioStatus;
using ::aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using ::aidl::android::hardware::bluetooth::audio::LeAudioConfiguration;
using ::aidl::android::hardware::bluetooth::audio::LeAudioBroadcastConfiguration;
using ::aidl::android::hardware::bluetooth::audio::LeAudioCodecConfiguration;
using ::aidl::android::hardware::bluetooth::audio::A2dpStreamConfiguration;
using ::aidl::android::hardware::bluetooth::audio::LeAudioAseConfiguration;
using ::aidl::android::hardware::bluetooth::audio::CodecSpecificConfigurationLtv;
using ::aidl::android::hardware::bluetooth::audio::MetadataLtv;

#ifndef SESSION_TYPE
#define SESSION_TYPE
typedef enum {
    UNKNOWN,
    /** A2DP legacy that AVDTP media is encoded by Bluetooth Stack */
    A2DP_SOFTWARE_ENCODING_DATAPATH,
    /** The encoding of AVDTP media is done by HW and there is control only */
    A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
    /** Used when encoded by Bluetooth Stack and streaming to Hearing Aid */
    HEARING_AID_SOFTWARE_ENCODING_DATAPATH,
    /** Used when encoded by Bluetooth Stack and streaming to LE Audio device */
    LE_AUDIO_SOFTWARE_ENCODING_DATAPATH,
    /** Used when decoded by Bluetooth Stack and streaming to audio framework */
    LE_AUDIO_SOFTWARE_DECODED_DATAPATH,
    /** Encoding is done by HW an there is control only */
    LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
    /** Decoding is done by HW an there is control only */
    LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH,
    /** SW Encoding for LE Audio Broadcast */
    LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH,
    /** HW Encoding for LE Audio Broadcast */
    LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
    /** A2DP legacy that AVDTP media is decoded by Bluetooth Stack */
    A2DP_SOFTWARE_DECODING_DATAPATH,
    /** The decoding of AVDTP media is done by HW and there is control only */
    A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH,
    MAX_SESSION_TYPE
}tSESSION_TYPE;
#endif

typedef enum {
    CTRL_ACK_SUCCESS,
    CTRL_ACK_UNSUPPORTED,
    CTRL_ACK_FAILURE,
    CTRL_ACK_PENDING,
    CTRL_ACK_INCALL_FAILURE,
    CTRL_ACK_DISCONNECT_IN_PROGRESS,
    CTRL_SKT_DISCONNECTED,
    CTRL_ACK_UNKNOWN,
    CTRL_ACK_RECONFIGURATION,
} tCTRL_ACK;

typedef enum {
    STATUS_UNKNOWN,
    STATUS_SUCCESS,
    STATUS_UNSUPPORTED,
    STATUS_FAILURE,
    STATUS_RECONFIGURATION
} tBLUETOOTH_AUDIO_STATUS;

typedef enum {
    AUDIO_STATE_STANDBY,    /* allows write to autoresume */
    AUDIO_STATE_STARTING,
    AUDIO_STATE_STARTED,
    AUDIO_STATE_STOPPING,
    AUDIO_STATE_STOPPED,
    AUDIO_STATE_SUSPENDED, /* need explicit set param call to resume (suspend=false) */
    AUDIO_STATE_RECONFIG_SUSPENDED /* suspended due to reconfiguration from stack */
} tAUDIO_STATE;

#define  MAX_CODEC_CFG_SIZE  30

struct a2dp_config {
    uint32_t                rate;
    uint32_t                channel_flags;
    int                     format;
};

struct a2dp_stream_common {
    pthread_mutex_t         lock;
    pthread_mutex_t         ack_lock;
    unsigned char           ack_recvd;
    pthread_cond_t          ack_cond;
    tAUDIO_STATE            state;
    tCTRL_ACK               ack_status;
    bool                    reconfig_pending;
    uint8_t                 multicast;
    uint8_t                 num_conn_dev;
    AudioConfiguration      codec_cfg;
    AudioConfiguration      dec_codec_cfg;
    uint8_t                 session_ready;
    uint16_t                sink_latency;
    uint16_t                ctrl_key;
    SessionType             sessionType;
    std::function<void(const uint16_t&, const bool&, const BluetoothAudioStatus&)> control_result_cb;
    std::function<void(const uint16_t&)> session_changed_cb;
    std::function<void(const uint16_t&, const AudioConfiguration &audio_config)> audio_configuration_changed_cb;
    std::function<void(const uint16_t&, const uint32_t&, const uint32_t&, const std::vector<uint8_t>&)> metadata_changed_cb;
};

/* codec specific definitions */

#define A2DP_APTX_VENDOR_ID 0x0000004F
#define A2DP_APTX_CODEC_ID_BLUETOOTH 0x0001
#define A2DP_APTX_ADAPTIVE_VENDOR_ID 0x000000D7
#define A2DP_APTX_ADAPTIVE_CODEC_ID_BLUETOOTH 0x00AD
#define A2DP_APTX_HD_VENDOR_ID 0x000000D7
#define A2DP_APTX_HD_CODEC_ID_BLUETOOTH 0x0024
#define A2DP_LDAC_VENDOR_ID 0x0000012D
#define A2DP_LDAC_CODEC_ID_BLUETOOTH 0x00AA

#define CODEC_ID_SBC        0u             //0x0000000000;
#define CODEC_ID_AAC        2u             //0x0000000002;
#define CODEC_ID_APTX       16797695u      //0x0001004fff;
#define CODEC_ID_APTX_HD    604035071u     //0x002400d7ff;
#define CODEC_ID_LDAC       285204031u     //0x00aa012dff;
#define CODEC_ID_OPUS       16834815u      //0x000100e0ff;
#define CODEC_ID_APTX_AD    2902513663u    //0x00ad00d7ff;
#define CODEC_ID_LHDCV5     327307049727u  //0x4c35053aff;

/* the LOSC of SBC media codec capabilitiy */
#define A2DP_SBC_CODEC_LEN 6
/* the LOSC of AAC media codec capabilitiy */
#define A2DP_AAC_CODEC_LEN 8
/* the LOSC of LDAC media codec capabilitiy */
#define A2DP_LDAC_CODEC_LEN 10
/* the LOSC of APTX media codec capabilitiy */
#define A2DP_APTX_CODEC_LEN 9
/* the LOSC of APTX HD media codec capabilitiy */
#define A2DP_APTX_HD_CODEC_LEN 13
/* the LOSC of APTX ADAPTIVE media codec capabilitiy */
#define A2DP_APTX_ADAPTIVE_CODEC_LEN 42

#define SKIP_STANDARD_CODEC_TYPE_PAYLOAD 2
#define SKIP_VENDOR_CODEC_TYPE_PAYLOAD 8

#define A2D_SBC_SAMP_FREQ_16 0x80  /* b7:16  kHz */
#define A2D_SBC_SAMP_FREQ_32 0x40  /* b6:32  kHz */
#define A2D_SBC_SAMP_FREQ_44 0x20  /* b5:44.1kHz */
#define A2D_SBC_SAMP_FREQ_48 0x10  /* b4:48  kHz */

#define A2D_SBC_CHANNEL_MODE_MONO         0x08 /* b3: MONO */
#define A2D_SBC_CHANNEL_MODE_DUAL_MONO    0x04 /* b2: DUAL MONO */
#define A2D_SBC_CHANNEL_MODE_STEREO       0x02 /* b1: STEREO */
#define A2D_SBC_CHANNEL_MODE_JOINT_STEREO 0x01 /* b0: JOINT STEREO */

#define A2D_SBC_SAMP_FREQ_MSK    0xF0 /* b7-b4 sampling frequency */
#define A2D_SBC_CH_MD_MSK        0x0F
#define A2D_SBC_BLK_MASK         0xF0
#define A2D_SBC_SUBBAND_MASK     0x0C
#define A2D_SBC_ALLOC_MASK       0x03

#define A2D_SBC_BLOCKS_4         0x80    /* 4 blocks */
#define A2D_SBC_BLOCKS_8         0x40    /* 8 blocks */
#define A2D_SBC_BLOCKS_12        0x20    /* 12blocks */
#define A2D_SBC_BLOCKS_16        0x10    /* 16blocks */

#define A2D_SBC_SUBBAND_4        0x08    /* b3: 4 */
#define A2D_SBC_SUBBAND_8        0x04    /* b2: 8 */

#define A2D_SBC_ALLOC_MD_S       0x02    /* b1: SNR */
#define A2D_SBC_ALLOC_MD_L       0x01    /* b0: loundess */

#define A2D_SBC_CHANNEL_MONO         0x00
#define A2D_SBC_CHANNEL_DUAL_MONO    0x01
#define A2D_SBC_CHANNEL_STEREO       0x02
#define A2D_SBC_CHANNEL_JOINT_STEREO 0x03

#define A2D_LDAC_SAMPLING_FREQ_MASK 0x3F
#define A2D_LDAC_CHANNEL_MODE_MASK 0x07

#define A2D_LDAC_SAMPLING_FREQ_44100 0x20
#define A2D_LDAC_SAMPLING_FREQ_48000 0x10
#define A2D_LDAC_SAMPLING_FREQ_88200 0x08
#define A2D_LDAC_SAMPLING_FREQ_96000 0x04
#define A2D_LDAC_SAMPLING_FREQ_176400 0x02
#define A2D_LDAC_SAMPLING_FREQ_192000 0x01

#define A2D_LDAC_CHANNEL_STEREO       0x01
#define A2D_LDAC_CHANNEL_DUAL         0x02
#define A2D_LDAC_CHANNEL_MONO         0x04

#define A2D_APTX_SAMPLING_FREQ_MASK 0xF0
#define A2D_APTX_CHANNEL_MODE_MASK 0x0F

#define A2D_APTX_CHANNELS_STEREO 0x02
#define A2D_APTX_CHANNELS_MONO 0x01

#define A2D_APTX_ADAPTIVE_SAMPLING_FREQ_MASK 0xF0
#define A2D_APTX_ADAPTIVE_CHANNEL_MODE_MASK 0x3F

#define A2D_APTX_ADAPTIVE_SAMPLERATE_44100        (0x40)
#define A2D_APTX_ADAPTIVE_SAMPLERATE_48000        (0x10)
#define A2D_APTX_ADAPTIVE_SAMPLERATE_96000        (0xA0)
#define A2D_APTX_ADAPTIVE_SAMPLERATE_96000_1      (0x20)
#define A2D_APTX_ADAPTIVE_SAMPLERATE_96000_2      (0x80)
#define A2D_APTX_ADAPTIVE_SAMPLERATE_RESERVED     (0x80)

#define A2D_APTX_ADAPTIVE_CHANNELS_MONO          (0x01)
#define A2D_APTX_ADAPTIVE_CHANNELS_STEREO        (0x02)
#define A2D_APTX_ADAPTIVE_CHANNELS_TWS_STEREO    (0x04)
#define A2D_APTX_ADAPTIVE_CHANNELS_JOINT_STEREO  (0x08)
#define A2D_APTX_ADAPTIVE_CHANNELS_TWS_MONO      (0x10)
#define A2D_APTX_ADAPTIVE_CHANNELS_TWS_PLUS      (0x20)

#define A2DP_APTX_ADAPTIVE_CODEC_FEATURE_LEN 26
#define A2DP_APTX_ADAPTIVE_RESERVED_DATA (A2DP_APTX_ADAPTIVE_CODEC_LEN \
        - (A2DP_APTX_ADAPTIVE_CODEC_FEATURE_LEN + 2))

#define A2D_APTX_SAMPLING_FREQ_44100 0x20
#define A2D_APTX_SAMPLING_FREQ_48000 0x10

#define A2D_AAC_FRAME_PEAK_MTU       0  /* Configure peak MTU */
#define A2D_AAC_FRAME_PEAK_BITRATE   1  /* Configure peak bitrate */
#define A2D_AAC_VBR_SUPPORT   2  /* Configure AAC VBR support */
#define A2D_AAC_VBR_SIZE_CTL_STRUCT 1
#define A2D_AAC_VBR_ENABLE 1
#define A2D_AAC_VBR_DISABLE 0
#define A2D_AAC_MIN_BITRATE 32000
#define A2D_AAC_MAX_BITRATE 165000

// [Octet 1] Sampling Frequency - 8000 to 44100
#define A2D_AAC_SAMPLING_FREQ_MASK0 0xFF
#define A2D_AAC_SAMPLING_FREQ_8000 0x80
#define A2D_AAC_SAMPLING_FREQ_11025 0x40
#define A2D_AAC_SAMPLING_FREQ_12000 0x20
#define A2D_AAC_SAMPLING_FREQ_16000 0x10
#define A2D_AAC_SAMPLING_FREQ_22050 0x08
#define A2D_AAC_SAMPLING_FREQ_24000 0x04
#define A2D_AAC_SAMPLING_FREQ_32000 0x02
#define A2D_AAC_SAMPLING_FREQ_44100 0x01
// [Octet 2], [Bits 4-7] Sampling Frequency - 48000 to 96000
// NOTE: Bits offset for the higher-order octet 16-bit integer
#define A2D_AAC_SAMPLING_FREQ_MASK1 (0xF0 << 8)
#define A2D_AAC_SAMPLING_FREQ_48000 (0x80 << 8)
#define A2D_AAC_SAMPLING_FREQ_64000 (0x40 << 8)
#define A2D_AAC_SAMPLING_FREQ_88200 (0x20 << 8)
#define A2D_AAC_SAMPLING_FREQ_96000 (0x10 << 8)
// [Octet 2], [Bits 2-3] Channel Mode
#define A2D_AAC_CHANNEL_MODE_MASK 0x0C
#define A2D_AAC_CHANNEL_MODE_MONO 0x08
#define A2D_AAC_CHANNEL_MODE_STEREO 0x04

#define A2DP_DEFAULT_SINK_LATENCY 0

#define AUDIO_CODEC_TYPE_LC3                 721420288u //0x2B000000UL
#define AUDIO_CODEC_TYPE_APTX_ADAPTIVE_LE    805306368u //0x30000000UL
#define AUDIO_CODEC_TYPE_APTX_ADAPTIVE_R4    822083584u //0x31000000UL

constexpr uint16_t kLeAudioVendorCompanyIdQualcomm = 0x000A;
constexpr uint16_t kLeAudioCodingFormatAptxLe = 0x0001;
constexpr uint16_t kLeAudioCodingFormatAptxLeX = 0x01AD;

typedef struct {
    uint32_t subband;    /* 4, 8 */
    uint32_t blk_len;    /* 4, 8, 12, 16 */
    uint16_t sampling_rate; /*44.1khz,48khz*/
    uint8_t  channels;      /*0(Mono),1(Dual_mono),2(Stereo),3(JS)*/
    uint8_t  alloc;         /*0(Loudness),1(SNR)*/
    uint8_t  min_bitpool;   /* 2 */
    uint8_t  max_bitpool;   /*53(44.1khz),51 (48khz) */
    uint32_t bitrate;      /* 320kbps to 512kbps */
    uint32_t bits_per_sample;  /* 16 bit */
} audio_sbc_encoder_config_t;

/* Information about BT APTX encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct {
    uint16_t sampling_rate;
    uint8_t  channels;
    uint32_t bitrate;
    uint32_t bits_per_sample;
} audio_aptx_encoder_config_t;

struct bit_rate_level_map_t {
    uint32_t link_quality_level;
    uint32_t bitrate;
};

#define MAX_LEVELS 5

struct quality_level_to_bitrate_info {
    uint32_t num_levels;
    struct bit_rate_level_map_t bit_rate_level_map[MAX_LEVELS];
};

/* Information about BT LDAC encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct {
    uint32_t sampling_rate;
    uint32_t bitrate;
    uint16_t channel_mode;
    uint16_t mtu;
    uint32_t bits_per_sample;
    bool is_abr_enabled;
    struct quality_level_to_bitrate_info level_to_bitrate_map;
} audio_ldac_encoder_config_t;

/* Structure to control frame size of AAC encoded frames. */
struct aac_frame_size_control_t {
    /* Type of frame size control: MTU_SIZE / PEAK_BIT_RATE */
    uint32_t ctl_type;
    /* Control value
     * MTU_SIZE: MTU size in bytes
     * PEAK_BIT_RATE: Peak bitrate in bits per second.
     */
    uint32_t ctl_value;
};

struct aac_abr_control_t {
bool is_abr_enabled;
struct quality_level_to_bitrate_info level_to_bitrate_map;
};

/* Information about BT AAC encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct {
    uint32_t enc_mode; /* LC, SBR, PS */
    uint16_t format_flag; /* RAW, ADTS */
    uint16_t channels; /* 1-Mono, 2-Stereo */
    uint32_t sampling_rate;
    uint32_t bitrate;
    uint32_t bits_per_sample;
    struct aac_frame_size_control_t frame_ctl;
    uint8_t size_control_struct;
    struct aac_frame_size_control_t* frame_ptr_ctl;
    uint8_t abr_size_control_struct;
    struct aac_abr_control_t* abr_ptr_ctl;
} audio_aac_encoder_config_t;

//LC3 encoder config
typedef struct {
    uint32_t api_version;
    uint32_t sampling_freq;
    uint32_t max_octets_per_frame;
    uint32_t frame_duration;//7.5msec, 10msec
    uint32_t bit_depth;
    uint32_t num_blocks;
    uint8_t default_q_level;
    uint8_t vendor_specific[16];
    uint32_t mode;
} lc3_config_t;
typedef struct {
    uint32_t audio_location;
    uint8_t stream_id;
    uint8_t direction;
} lc3_stream_map_t;
typedef struct {
    lc3_config_t toAirConfig;
    uint8_t stream_map_size;
    lc3_stream_map_t* streamMapOut;
} lc3_encoder_config_t;//audio_lc3_encoder_config_t;
typedef struct {
    lc3_config_t fromAirConfig;
    uint32_t decoder_output_channel;
    uint8_t stream_map_size;
    lc3_stream_map_t* streamMapIn;
} lc3_decoder_config_t;//audio_lc3_decoder_config_t;
typedef struct {
  lc3_encoder_config_t enc_config;
  lc3_decoder_config_t dec_config;
  bool is_enc_config_set;
  bool is_dec_config_set;
} audio_lc3_codec_config_t;


typedef struct {
    uint32_t  sampling_rate;
    uint32_t  mtu;
    int32_t  channel_mode;
    uint32_t  min_sink_buffering_LL;
    uint32_t  max_sink_buffering_LL;
    uint32_t  min_sink_buffering_HQ;
    uint32_t  max_sink_buffering_HQ;
    uint32_t  min_sink_buffering_TWS;
    uint32_t  max_sink_buffering_TWS;
    uint32_t  aptx_mode;
    uint8_t  TTP_LL_low;
    uint8_t  TTP_LL_high;
    uint8_t  TTP_HQ_low;
    uint8_t  TTP_HQ_high;
    uint8_t  TTP_TWS_low;
    uint8_t  TTP_TWS_high;
    uint32_t bits_per_sample;
    uint32_t  input_mode;
    uint32_t  input_fade_duration;
    uint8_t  sink_capabilities[11];
} audio_aptx_adaptive_encoder_config_t;


int audio_stream_open(tSESSION_TYPE session_type);
int audio_stream_start(tSESSION_TYPE session_type);
int audio_stream_suspend(tSESSION_TYPE session_type);
int audio_stream_stop(tSESSION_TYPE session_type);
int audio_stream_close(tSESSION_TYPE session_type);
uint16_t audio_get_a2dp_sink_latency(tSESSION_TYPE session_type);
void * audio_get_codec_config(tSESSION_TYPE session_type, uint8_t* multicast_status, uint8_t* num_dev,
                              audio_format_t *codec_type);

typedef struct {
  uint32_t bitrate;
  uint32_t bitrate_mode; // 0 - unknown, 1 - avg, 2 - max
  uint32_t mtu;
} audio_sink_buffer_config_t;

typedef struct {
  audio_sink_buffer_config_t snk_buffer;
  uint32_t obj_type; /* LC*/
  uint16_t format_flag; /* LATM */
  uint16_t channels; /* 1-Mono, 2-Stereo */
  uint32_t sampling_rate;
  uint32_t bits_per_sample;
} audio_aac_decoder_config_t;

typedef struct {
  audio_sink_buffer_config_t snk_buffer;
  uint16_t sampling_rate; /*44.1khz,48khz*/
  uint8_t channels; /*0(Mono),1(Dual_mono),2(Stereo),3(JS)*/
  uint32_t bits_per_sample; /* 16 bit */
} audio_sbc_decoder_config_t;

static void btapoffload_port_init();

extern "C" int audio_start_stream(void);
extern "C" int audio_stop_stream(void);
extern "C" int audio_suspend_stream(void);
extern "C" int audio_stream_open(void);
extern "C" int audio_stream_close(void);
extern "C" int audio_stream_start(void);
extern "C" int audio_stream_stop(void);
extern "C" int audio_stream_suspend(void);
extern "C" void* audio_get_codec_config(uint8_t *mcast, uint8_t *num_dev, audio_format_t *codec_type);
extern "C" void audio_handoff_triggered(void);
extern "C" void clear_a2dpsuspend_flag(void);
extern "C" void* audio_get_next_codec_config(uint8_t idx, audio_format_t *codec_type);
extern "C" int audio_check_a2dp_ready(void);
extern "C" uint16_t audio_get_a2dp_sink_latency();
extern "C" uint16_t audio_sink_get_a2dp_latency();
extern "C" int wait_for_stack_response(uint8_t duration);
extern "C" bool audio_is_scrambling_enabled(void);
extern "C" void bt_audio_pre_init(void);

extern "C" int audio_sink_start_stream(void);
extern "C" int audio_sink_stop_stream(void);
extern "C" int audio_sink_suspend_stream(void);

extern "C" int audio_sink_stream_start(void);
extern "C" int audio_sink_stream_stop(void);
extern "C" int audio_sink_stream_suspend(void);

/* HIDL 2.2 APIs */
extern "C" void update_metadata(const tSESSION_TYPE session_type , void * metadata);
extern "C" int audio_start_stream_api(const tSESSION_TYPE session_type);
extern "C" int audio_stop_stream_api(const tSESSION_TYPE session_type);
extern "C" int audio_suspend_stream_api(const tSESSION_TYPE session_type);
extern "C" int audio_stream_open_api(const tSESSION_TYPE session_type);
extern "C" int audio_stream_close_api(const tSESSION_TYPE session_type);
extern "C" int audio_check_a2dp_ready_api(const tSESSION_TYPE session_type);
extern "C" void* audio_get_codec_config_api(const tSESSION_TYPE session_type, uint8_t *mcast, uint8_t *num_dev, audio_format_t *codec_type);
extern "C" uint16_t audio_sink_get_a2dp_latency_api(const tSESSION_TYPE session_type);
/* reconfig_cb: status = 0 (stream suspend), status = 1 (updateCodecConfig) */
extern "C" void register_reconfig_cb(int  (*reconfig_cb)(tSESSION_TYPE session_type, int status));
extern "C" void unregister_reconfig_cb(int (*reconfig_cb)(tSESSION_TYPE session_type, int status));
extern "C" int audio_stream_get_supported_latency_modes_api(const tSESSION_TYPE session_type,
                                                            size_t *num_modes,
                                                            size_t max_latency_modes,
                                                            uint32_t *modes);
extern "C" int audio_stream_set_latency_mode_api(const tSESSION_TYPE session_type, uint32_t mode);

extern "C" int audio_sink_check_a2dp_ready();
extern "C" int audio_sink_start_capture();
extern "C" int audio_sink_stop_capture();
extern "C" void *audio_get_decoder_config(audio_format_t *codec_type);
extern "C" int audio_sink_session_setup_complete(uint64_t system_latency);
#endif
