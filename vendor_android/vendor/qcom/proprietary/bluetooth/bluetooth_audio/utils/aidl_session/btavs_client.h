/*****************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 * Not a contribution.
 * Copyright 1999-2012 Broadcom Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************/

#ifndef BTAVSCLIENT_H
#define BTAVSCLIENT_H

#include <cstdint>
#include <vector>
#include <algorithm>
#include <thread>
#include <map>
#include <mutex>
#include <functional>
#include <string>
#include <fstream>
#include <iterator>
#include <optional>
#include "../../../btavs_provider/BtAVsProviderIf.h"
#include <cutils/properties.h>
#include <android-base/logging.h>
#include <log/log.h>
#include <system/audio.h>
#include <hardware/audio.h>

using bluetooth::btavsprovider::BtAVsProviderIf;

#define STREAM_TO_UINT8(u8, p) \
  {                            \
    (u8) = (uint8_t)(*(p));    \
    (p) += 1;                  \
  }
#define JSTREAM_TO_UINT8(u8, p) \
  {                             \
    (u8) = (uint8_t)(*(p));     \
  }
#define STREAM_TO_UINT16(u16, p)                                  \
  {                                                               \
    (u16) = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); \
    (p) += 2;                                                     \
  }
#define STREAM_TO_ARRAY16(a, p)        \
  {                                    \
    int ijk;                           \
    uint8_t* _pa = (uint8_t*)(a) + 15; \
    for (ijk = 0; ijk < 16; ijk++)     \
      *_pa-- = *(p)++;                 \
  }
#define UINT8_TO_STREAM(p, u8) \
  { *(p)++ = (uint8_t)(u8); }
#define ARRAY8_TO_STREAM(p, a)        \
  {                                   \
    int ijk;                          \
    for (ijk = 0; ijk < 8; ijk++)     \
      *(p)++ = (uint8_t)(a)[7 - ijk]; \
  }
#define STREAM_TO_ARRAY(a, p, len)    \
  {                                   \
    int ijk;                          \
    for (ijk = 0; ijk < (len); ijk++) \
      ((uint8_t*)(a))[ijk] = *(p)++;  \
  }
#define UINT16_TO_STREAM(p, u16)    \
  {                                 \
    *(p)++ = (uint8_t)(u16);        \
    *(p)++ = (uint8_t)((u16) >> 8); \
  }
#define STREAM_TO_UINT24(u32, p)                                      \
  {                                                                   \
    (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + \
             ((((uint32_t)(*((p) + 2)))) << 16));                     \
    (p) += 3;                                                         \
  }
#define JSTREAM_TO_UINT32(u32, p)                                     \
  {                                                                   \
    (u32) = ((((uint32_t)(*(p))) << 24) +                             \
            ((((uint32_t)(*((p) + 1)))) << 16) +                      \
            ((((uint32_t)(*((p) + 2)))) << 8) +                       \
            (((uint32_t)(*((p) + 3)))));                              \
  }
#define JSTREAM_TO_UINT64(u64, p)                                     \
  {                                                                   \
    (u64) = ((((uint64_t)(*(p))) << 56) +                             \
            ((((uint64_t)(*((p) + 1)))) << 48) +                      \
            ((((uint64_t)(*((p) + 2)))) << 40) +                      \
            ((((uint64_t)(*((p) + 3)))) << 32) +                      \
            ((((uint64_t)(*((p) + 4)))) << 24) +                      \
            ((((uint64_t)(*((p) + 5)))) << 16) +                      \
            ((((uint64_t)(*((p) + 6)))) << 8) +                       \
            (((uint64_t)(*((p) + 7)))));                              \
  }
#define JSTREAM_TO_UINT48(u64, p)                                     \
  {                                                                   \
    (u64) = ((((uint64_t)(*(p))) << 40) +                             \
            ((((uint64_t)(*((p) + 1)))) << 32) +                      \
            ((((uint64_t)(*((p) + 2)))) << 24) +                      \
            ((((uint64_t)(*((p) + 3)))) << 16) +                      \
            ((((uint64_t)(*((p) + 4)))) << 8) +                       \
            (((uint64_t)(*((p) + 5)))));                              \
  }
#define STREAM_TO_UINT48(u64, p)                                      \
  {                                                                   \
    (u64) = (((uint64_t)(*(p))) + ((((uint64_t)(*((p) + 1)))) << 8) + \
             ((((uint64_t)(*((p) + 2)))) << 16) +                     \
             ((((uint64_t)(*((p) + 3)))) << 24) +                     \
             ((((uint64_t)(*((p) + 4)))) << 32) +                     \
             ((((uint64_t)(*((p) + 5)))) << 40));                     \
    (p) += 6;                                                         \
  }

#define BTM_QBCE_QLE_HCI_SUPPORTED(x) ((x)[3] & 0x10)
#define BTM_QBCE_QCM_HCI_SUPPORTED(x) ((x)[3] & 0x20)

#define SOC_ADD_ON_FEATURES_MAX_SIZE 245

#define BD_FEATURES_LEN 8

#define QLL_LOCAL_SUPPORTED_FEATURES_MAX_SIZE 8

#define HCI_VS_GET_ADDON_FEATURES_SUPPORT 0x001D

#define HCI_VS_QBCE_OCF 0x0051
#define HCI_VS_QCONTROLLER_A2DP 0x000A

#define QBCE_READ_LOCAL_QLL_SUPPORTED_FEATURES 0x0B
#define QBCE_READ_REMOTE_QLL_SUPPORTED_FEATURE 0x0C
#define QBCE_READ_REMOTE_QLL_SUPPORTED_FEATURE_LEN 3

#define QBCE_QLL_CONNECTION_COMPLETE 0x09
#define QBCE_VS_EXTENDED_METADATA 0xFE
#define QBCE_VS_CODEC_SETTINGS 0x18
#define QBCE_VS_SET_CIG_METADATA 0x3D
#define QBCE_SET_CIG_CONTEXT_TYPE 0x3C
#define QBCE_SET_QHS_HOST_MODE 0x01
#define QBCE_SET_QLL_EVENT_MASK 0x10
#define QBCE_SET_QLM_EVENT_MASK 0x0F
#define QBCE_SET_ENCODER_LIMITS 0x24

#define A2DP_PASSTHRU_CODEC_INFO 0x13

#define LTV_TYPE_STREAM_INDICATION 0x01
#define LTV_LEN_STREAM_INDICATION 0x01
#define LTV_TYPE_BAP_TIMEOUT_INDICATION 0x02
#define LTV_LEN_BAP_TIMEOUT_INDICATION 0x06
#define LTV_TYPE_MIN_FT 0X00
#define LTV_TYPE_MIN_BIT_RATE 0X01
#define LTV_TYPE_MIN_MAX_ERROR_RESILIENCE 0X02
#define LTV_TYPE_LATENCY_MODE 0X03
#define LTV_TYPE_MAX_FT 0X04
#define LTV_LEN_MIN_FT 0X01
#define LTV_LEN_MIN_BIT_RATE 0X01
#define LTV_LEN_MIN_MAX_ERROR_RESILIENCE 0X01
#define LTV_LEN_LATENCY_MODE 0X01
#define LTV_LEN_MAX_FT 0X01

#define QHS_TRANSPORT_LE_ISO 2
#define QHS_TRANSPORT_LE 1
#define QHS_TRANSPORT_BREDR 0
#define QHS_BREDR_MASK 0x01
#define QHS_LE_MASK 0x02
#define QHS_HOST_MODE_HOST_AWARE 3
#define QHS_HOST_DISABLE_ALL 4

#define AIRPLANE_MODE_STATE_CHANGED 3
#define ADAPTER_STATE_CHANGED 4
#define LE_AUDIO_ACTIVE_DEVICE_CHANGED 5
#define LE_AUDIO_CODEC_CONFIG_CHANGED 7
#define ACL_DISCONNECTED 8
#define ACL_CONNECTED 9

#define ADAPTER_STATE_OFF 10
#define ADAPTER_STATE_ON 12
#define ADAPTER_STATE_TURNING_OFF 13

#define DEVICE_DISCONNECTED 0
#define DEVICE_CONNECTED 2

#define CODEC_TYPE_APTX_ADAPTIVE_R4 4

#define AUDIO_DEVICE_MAX_ADDRESS_LEN 32
#define AUDIO_ATTRIBUTES_TAGS_MAX_SIZE 256

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
  BT_SOC_TYPE_DEFAULT = 0x0000,
  BT_SOC_TYPE_SMD = BT_SOC_TYPE_DEFAULT,
  BT_SOC_TYPE_AR3K,
  BT_SOC_TYPE_ROME,
  BT_SOC_TYPE_CHEROKEE,
  BT_SOC_TYPE_HASTINGS,
  BT_SOC_TYPE_MOSELLE,
  BT_SOC_TYPE_HAMILTON,
  BT_SOC_TYPE_GANGES,
  BT_SOC_TYPE_ORNE,
  BT_SOC_TYPE_CONGO,
  BT_SOC_TYPE_RESERVED
} bt_soc_type_t;

typedef enum : uint8_t {
  HCI_SUCCESS = 0x00,
} tHCI_ERROR_CODE;

enum class LeAudioContextType : uint16_t {
  UNINITIALIZED = 0x0000,
  UNSPECIFIED = 0x0001,
  CONVERSATIONAL = 0x0002,
  MEDIA = 0x0004,
  GAME = 0x0008,
  INSTRUCTIONAL = 0x0010,
  VOICEASSISTANTS = 0x0020,
  LIVE = 0x0040,
  SOUNDEFFECTS = 0x0080,
  NOTIFICATIONS = 0x0100,
  RINGTONE = 0x0200,
  ALERTS = 0x0400,
  EMERGENCYALARM = 0x0800,
  RFU = 0x1000,
};

typedef struct {
  uint8_t as_array[SOC_ADD_ON_FEATURES_MAX_SIZE];
} bt_device_soc_add_on_features_t;

typedef struct {
  uint8_t as_array[BD_FEATURES_LEN];
} remote_qll_features_t;

typedef struct {
  uint8_t as_array[QLL_LOCAL_SUPPORTED_FEATURES_MAX_SIZE];
} bt_device_qll_local_supported_features_t;

typedef struct {
  const uint16_t opcode;
  const uint16_t param_len;
  const uint8_t* p_param_buf;
} VSC_CMPL;

typedef struct {
  const uint8_t as_array[8];
} bt_event_mask_t;

struct le_audio_device_t {
  uint8_t mode;
  uint16_t delay;
  uint64_t bdAddr;
  bool isActive;
  bool isLexSupported;
  bool isLexConfigured;
  bool isLexTransportReady;
  uint16_t connHandle;
  remote_qll_features_t qll_features;
  bool operator==(const struct le_audio_device_t& other) const {
      return other.mode == mode && other.delay == delay &&
             other.bdAddr == bdAddr && other.isActive == isActive &&
             other.isLexSupported == isLexSupported &&
             other.isLexConfigured == isLexConfigured &&
             other.isLexTransportReady == isLexTransportReady;
  }
};

typedef struct le_audio_device_t le_audio_device;

typedef struct {
  uint8_t cis_count;
  uint16_t context_type;
  std::vector<uint16_t> cis_handles;
} cig_context_metadata;

class CacheManager {
private:
    std::string filePath;
    std::string localQllFeaturesPath;
    std::string addOnFeaturesPath;

public:
    CacheManager(const std::string& devices, const std::string& localQllfeatures,
        const std::string& addOnFeatures) : filePath(devices),
        localQllFeaturesPath(localQllfeatures), addOnFeaturesPath(addOnFeatures) {}

    void putDevices(const std::vector<le_audio_device>& devices);
    void putLocalQllFeatures(const bt_device_qll_local_supported_features_t& local_qll_features_);
    void putSoCAddOnFeatures(const bt_device_soc_add_on_features_t& soc_add_on_features_);
    std::vector<uint8_t> getLocalQllFeatures();
    std::vector<uint8_t> getAddOnFeatures();
    std::vector<le_audio_device> getDevices();
    void clear();

};


class BtAVsClient {
public:
  BtAVsClient(const BtAVsClient&) = delete;
  BtAVsClient& operator=(const BtAVsClient&) = delete;
  static BtAVsClient& GetInstance();
  LeAudioContextType GetContextType(const tSESSION_TYPE session_type, void* data);
  void UpdateA2dpCodecModeOrData(const std::vector<uint8_t>& payload);
  void UpdateContextType(cig_context_metadata& data);
  void UpdateCodecSettings (const uint32_t& cig_id, const uint32_t& cis_id,
        const std::vector<uint8_t>& payload);
  void Init();
  bool IsLeXTransportAvailable();
  bt_device_soc_add_on_features_t GetSoCAddOnFeatures(uint8_t& len);
  bt_device_qll_local_supported_features_t GetLocalQLLSupportedFeatures(uint8_t& len);
  remote_qll_features_t GetRemoteQLLFeatures();
  uint8_t GetMode();
  uint16_t GetDelay();
  bool IsLeXDevice();
  bool IsLeXSelected();
  void UpdateVAContext(const tSESSION_TYPE session_type, void* data);
  void SetCurrentA2dpAudioMode(bool is_gaming_mode);
  void SetCurrentA2dpLatencyMode(bool is_low_latency_mode);
  bool GetCurrentA2dpAudioMode();
  void SetLosslessMode(bool is_lossless_mode);
  bool GetLosslessMode();
  void SetLosslessCapability(bool is_lossless_capability);
  bool GetLosslessCapability();
  uint8_t GetQhsSupportMask();
  int GetLdacBitRate();
  void SetLdacBitRate(int bitrate);
  void RegisterAudioSetting(std::function<void()> cb);
  void UpdateCigMetadata(const uint8_t metadata_type, const uint8_t stream_ind,
       const std::vector<uint8_t> bd_addr);
  bool IsAdapterStateOn();
private:
  BtAVsClient();
  void InitializeInterface();
  std::optional<std::vector<uint8_t>> Find(uint8_t type, std::map<uint8_t, std::vector<uint8_t>>& values);
  bool Parse(const uint8_t* p_value, uint8_t len, std::map<uint8_t, std::vector<uint8_t>>& values);
  static void UpdateVSEvent (int32_t ofc, const std::vector<uint8_t>& p);
  static void UpdateSysEvent (int32_t evt, const std::vector<uint8_t>& p);
  static void UpdateVSCommandStatus (int32_t ofc, int32_t status);
  static void UpdateVSCommandComplete (int32_t ofc, const std::vector<uint8_t>& params);
  void ParseControllerAddonFeaturesResponse(VSC_CMPL* p_data);
  void ParseVsQbceCommandCompleteResponse(VSC_CMPL* p_data);
  void ConfigQHS();
  void ReadVendorAddOnFeatures();
  void UpdateCodecSettings(const uint8_t* p);
  void ReadRemoteSupportedQllFeaturesCompleteEvt(const uint8_t* p);
  void QllConnectionCompleteEvt(const uint8_t* p);
  bt_soc_type_t ConvertSocNameToBTSocType(const char* soc_name);
  LeAudioContextType AudioContentToLeAudioContext(
            audio_content_type_t content_type, audio_usage_t usage);
  LeAudioContextType AudioSourceToLeAudioContext(audio_source_t source);
  void UpdateVAAudioContent(
       audio_content_type_t& content_type, audio_usage_t& usage);
  void UpdateVAAudioSource(audio_source_t& source);


  static BtAVsClient* instance;
  static std::mutex mtx;
  const char AUDIO_ATTRIBUTES_TAGS_SEPARATOR = ';';
  const bt_event_mask_t QBCE_QLM_AND_QLL_EVENT_MASK = {
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x4A}};
  const LeAudioContextType context_priority_list[11] = {
      /* Highest priority first */
      LeAudioContextType::CONVERSATIONAL,
      LeAudioContextType::RINGTONE,
      LeAudioContextType::LIVE,
      LeAudioContextType::VOICEASSISTANTS,
      LeAudioContextType::GAME,
      LeAudioContextType::MEDIA,
      LeAudioContextType::EMERGENCYALARM,
      LeAudioContextType::ALERTS,
      LeAudioContextType::INSTRUCTIONAL,
      LeAudioContextType::NOTIFICATIONS,
      LeAudioContextType::SOUNDEFFECTS,
  };
  std::function<void()> audio_setting_cb;
  std::vector<le_audio_device> leADevices = {};
  uint64_t leaActiveDevice = 0;
  bool isLeXDevice = false;
  uint8_t adapter_state;
  CacheManager cache_{"/data/vendor/audio/devices.bin", "/data/vendor/audio/localQllFeatures.bin", "/data/vendor/audio/addOnFeatures.bin"};
  std::map<uint64_t, uint32_t> device_codec_map = {};
  bt_device_soc_add_on_features_t soc_add_on_features = {};
  bt_device_qll_local_supported_features_t local_qll_features = {};
  tBTAVS_MANAGER_API intf = {};
  char qhs_value[PROPERTY_VALUE_MAX] = "0";
  uint8_t qhs_support_mask = 0;
  uint8_t soc_add_on_features_length = 0;
  uint8_t qll_local_supported_features_length = 0;
  uint16_t product_id = 0, response_version = 0;
  bool is_in_a2dp_gaming;
  bool is_in_a2dp_low_latency;
  bool get_latency_mode;
  bool is_lossless_mode_supported;
  int current_ldac_bit_rate = 0;
  bool is_lossless_capability_supported;
};

#endif // BTAVSCLIENT_H
