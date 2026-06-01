/*
* Copyright (c) 2024 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
* Not a contribution.
*
* Copyright (C) 2022 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*
http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#define LOG_TAG "BTAudioProviderLeAudioHW"

#include "LeAudioOffloadAudioProvider.h"

#include <BluetoothAudioCodecs.h>
#include <BluetoothAudioSessionReport.h>
#include <android-base/logging.h>
#include <algorithm>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

constexpr uint8_t kLeAudioDirectionSink = 0x01;
constexpr uint8_t kLeAudioDirectionSource = 0x02;
constexpr uint8_t kIsoDataPathHci = 0x00;
constexpr uint8_t kIsoDataPathPlatformDefault = 0x01;
constexpr uint8_t kIsoCodingFormatVendorSpecific = 0xFF;

/* Vendor specific config LTV */
constexpr uint8_t LTV_TYPE_VER_NUM = 0X00;
constexpr uint8_t LTV_TYPE_FREQ = 0X01;
constexpr uint8_t LTV_TYPE_USE_CASE = 0X02;
constexpr uint8_t LTV_TYPE_CONN_HANDLE = 0X06;
constexpr uint8_t LTV_TYPE_CODEC_ID = 0X07;
constexpr uint8_t LTV_TYPE_MPC = 0X0B;

constexpr uint8_t LTV_LEN_VER_NUM = 0X02;
constexpr uint8_t LTV_LEN_CONN_HANDLE = 0X03;
constexpr uint8_t LTV_LEN_FREQ = 0X02;
constexpr uint8_t LTV_LEN_CODEC_ID = 0X06;
constexpr uint8_t LTV_LEN_USE_CASE = 0X02;
constexpr uint8_t LTV_LEN_MPC = 0X03;

constexpr uint8_t HQ_AUDIO_USE_CASE = 0X00;
constexpr uint8_t GAMING_NO_VBC_USE_CASE = 0X01;
constexpr uint8_t GAMING_VBC_USE_CASE = 0X02;
constexpr uint8_t VOICE_USE_CASE = 0X03;
constexpr uint8_t STEREO_REC_USE_CASE = 0X04;

std::map<AudioContext,
          std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>
          context_to_config_map;
std::map<uint8_t, MetadataLtv::VendorSpecific>directional_metadata_map;
AudioContext active_context;
std::vector<uint16_t> cis_hndles = {};
std::map<AudioContext, /*bitmask*/uint8_t> context_to_matched_codecId;

const std::map<CodecSpecificConfigurationLtv::SamplingFrequency, uint32_t>
    freq_to_support_bitmask_map = {
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ8000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ8000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ11025,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ11025},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ16000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ22050,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ22050},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ24000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ32000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ32000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ48000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ88200,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ88200},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ96000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ176400,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ176400},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ192000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ192000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ384000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ384000},
};

// Helper map from capability's tag to configuration's tag
std::map<CodecSpecificCapabilitiesLtv::Tag, CodecSpecificConfigurationLtv::Tag>
    cap_to_cfg_tag_map = {
        {CodecSpecificCapabilitiesLtv::Tag::supportedSamplingFrequencies,
         CodecSpecificConfigurationLtv::Tag::samplingFrequency},
        {CodecSpecificCapabilitiesLtv::Tag::supportedMaxCodecFramesPerSDU,
         CodecSpecificConfigurationLtv::Tag::codecFrameBlocksPerSDU},
        {CodecSpecificCapabilitiesLtv::Tag::supportedFrameDurations,
         CodecSpecificConfigurationLtv::Tag::frameDuration},
        {CodecSpecificCapabilitiesLtv::Tag::supportedAudioChannelCounts,
         CodecSpecificConfigurationLtv::Tag::audioChannelAllocation},
        {CodecSpecificCapabilitiesLtv::Tag::supportedOctetsPerCodecFrame,
         CodecSpecificConfigurationLtv::Tag::octetsPerCodecFrame},
};

const std::map<CodecSpecificConfigurationLtv::FrameDuration, uint32_t>
    fduration_to_support_fduration_map = {
        {CodecSpecificConfigurationLtv::FrameDuration::US7500,
         CodecSpecificCapabilitiesLtv::SupportedFrameDurations::US7500},
        {CodecSpecificConfigurationLtv::FrameDuration::US10000,
         CodecSpecificCapabilitiesLtv::SupportedFrameDurations::US10000},
};

std::map<int32_t, CodecSpecificConfigurationLtv::SamplingFrequency>
    sampling_freq_map = {
        {16000, CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000},
        {24000, CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000},
        {48000, CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000},
        {96000, CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000},
};

std::map<int32_t, CodecSpecificConfigurationLtv::FrameDuration>
    frame_duration_map = {
        {7500, CodecSpecificConfigurationLtv::FrameDuration::US7500},
        {10000, CodecSpecificConfigurationLtv::FrameDuration::US10000},
};

LeAudioOffloadOutputAudioProvider::LeAudioOffloadOutputAudioProvider()
    : LeAudioOffloadAudioProvider() {
  session_type_ = SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

LeAudioOffloadInputAudioProvider::LeAudioOffloadInputAudioProvider()
    : LeAudioOffloadAudioProvider() {
  session_type_ = SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH;
}

LeAudioOffloadBroadcastAudioProvider::LeAudioOffloadBroadcastAudioProvider()
    : LeAudioOffloadAudioProvider() {
  session_type_ =
      SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

LeAudioOffloadAudioProvider::LeAudioOffloadAudioProvider()
    : BluetoothAudioProvider() {}

bool LeAudioOffloadAudioProvider::isValid(const SessionType& sessionType) {
  return (sessionType == session_type_);
}

void removeMetadataFromResultAndSendToStack(
    IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting) {
  if (!setting.sinkAseConfiguration.has_value() &&
      !setting.sourceAseConfiguration.has_value()) {
    LOG(DEBUG) << ": No Sink or Source ase config exist.";
    return;
  }

  for (uint8_t direction : {kLeAudioDirectionSink, kLeAudioDirectionSource}) {
    LOG(INFO) << __func__ << ": direction = " << unsigned(direction);
    std::vector<
        std::optional<LeAudioAseConfigurationSetting::AseDirectionConfiguration>>*
        directionAseConfiguration = {};
    if (direction == kLeAudioDirectionSink) {
      if (setting.sinkAseConfiguration.has_value() &&
          !setting.sinkAseConfiguration.value().empty()) {
        directionAseConfiguration = &setting.sinkAseConfiguration.value();
      } else {
        LOG(INFO) << __func__ << ": No sink ase setting available";
        return;
      }
    } else {
      if (setting.sourceAseConfiguration.has_value() &&
          !setting.sourceAseConfiguration.value().empty()) {
        directionAseConfiguration = &setting.sourceAseConfiguration.value();
      } else {
        LOG(INFO) << __func__ << ": No source ase setting available";
        return;
      }
    }

    for (auto& aseConfiguration : *directionAseConfiguration) {
      if (aseConfiguration.has_value() &&
          aseConfiguration.value().aseConfiguration.metadata.has_value()) {
        for (auto& meta :
             aseConfiguration.value().aseConfiguration.metadata.value()) {
          if (meta.has_value()) {
            meta = {};
          }
        }
      }
    }
  }
}

void fillPacsVsmForEnableOp(
    IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting) {
  if (!setting.sinkAseConfiguration.has_value() &&
      !setting.sourceAseConfiguration.has_value()) {
    LOG(DEBUG) << ": No Sink or Source ase config exist.";
    return;
  }

  for (uint8_t direction : {kLeAudioDirectionSink, kLeAudioDirectionSource}) {
    LOG(INFO) << __func__ << ": direction = " << unsigned(direction);
    std::vector<
        std::optional<LeAudioAseConfigurationSetting::AseDirectionConfiguration>>*
        directionAseConfiguration = {};
    if (direction == kLeAudioDirectionSink) {
      if (setting.sinkAseConfiguration.has_value() &&
          !setting.sinkAseConfiguration.value().empty()) {
        directionAseConfiguration = &setting.sinkAseConfiguration.value();
      } else {
        LOG(INFO) << __func__ << ": No sink ase setting available";
        return;
      }
    } else {
      if (setting.sourceAseConfiguration.has_value() &&
          !setting.sourceAseConfiguration.value().empty()) {
        directionAseConfiguration = &setting.sourceAseConfiguration.value();
      } else {
        LOG(INFO) << __func__ << ": No source ase setting available";
        return;
      }
    }

    for (auto& aseConfiguration : *directionAseConfiguration) {
      if (aseConfiguration.has_value() &&
          aseConfiguration.value().aseConfiguration.metadata.has_value()) {
        for (auto& meta :
             aseConfiguration.value().aseConfiguration.metadata.value()) {
          if (meta.has_value() &&
              meta.value().getTag() == MetadataLtv::vendorSpecific) {
            if (meta.value().get<MetadataLtv::vendorSpecific>().companyId != 0) {
              LOG(DEBUG) << ": vaid vendor specific metadata exist.";
              meta.value().set<MetadataLtv::vendorSpecific>();
              const MetadataLtv::VendorSpecific pacs_vendor_Metadata =
                                              directional_metadata_map[direction];
              meta.value().set<MetadataLtv::vendorSpecific>(pacs_vendor_Metadata);
            }
          } else if (meta.has_value() &&
                     meta.value().getTag() == MetadataLtv::preferredAudioContexts) {
            LOG(DEBUG) << ": Remove preferredAudioContexts from metadata.";
            meta.value().set<MetadataLtv::preferredAudioContexts>();
          } else if (meta.has_value() &&
                     meta.value().getTag() == MetadataLtv::streamingAudioContexts) {
            LOG(DEBUG) << ": Remove streamingAudioContexts from metadata.";
            meta.value().set<MetadataLtv::streamingAudioContexts>();
          }
        }
      }
    }
  }
}

ndk::ScopedAStatus LeAudioOffloadAudioProvider::startSession(
    const std::shared_ptr<IBluetoothAudioPort>& host_if,
    const AudioConfiguration& audio_config,
    const std::vector<LatencyMode>& latency_modes, DataMQDesc* _aidl_return) {
  LOG(WARNING) << __func__ << " session type: " << toString(session_type_);
  if (session_type_ ==
      SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    if ( audio_config.getTag() != AudioConfiguration::leAudioBroadcastConfig) {
      LOG(WARNING) << __func__ << " - Invalid Audio Configuration="
                   << audio_config.toString();
      *_aidl_return = DataMQDesc();
      return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
  } else if (audio_config.getTag() != AudioConfiguration::leAudioConfig) {
    LOG(WARNING) << __func__ << " - Invalid Audio Configuration="
                 << audio_config.toString();
    *_aidl_return = DataMQDesc();
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  bool isLeAudioCodecExtEnabled =
    property_get_bool("persist.vendor.qcom.bluetooth.vsc_enabled", false);
  if (isLeAudioCodecExtEnabled) {
    BluetoothCodecExtensiblityFlags::kEnableLeAudioCodecExtensibility = true;
  }

  return BluetoothAudioProvider::startSession(host_if, audio_config,
                                              latency_modes, _aidl_return);
}

ndk::ScopedAStatus LeAudioOffloadAudioProvider::onSessionReady(
    DataMQDesc* _aidl_return) {
  BluetoothAudioSessionReport::OnSessionStarted(
      session_type_, stack_iface_, nullptr, *audio_config_, latency_modes_);
  *_aidl_return = DataMQDesc();
  return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus LeAudioOffloadAudioProvider::setCodecPriority(
    const CodecId& in_codecId, int32_t in_priority) {
  codec_priority_map_[in_codecId] = in_priority;
  return ndk::ScopedAStatus::ok();
};

void LeAudioOffloadAudioProvider::PrepareCodecBitmask(CodecId cfg_codec,
                                                      const AudioContext& context) {
  LOG(INFO) << __func__
            << ": cfg_codec Tag: " << toString(cfg_codec.getTag());
  uint8_t kCodecMask = 0;
  if (cfg_codec.getTag() == CodecId::Tag::vendor) {
    auto vc_cfg_codec = cfg_codec.get<CodecId::Tag::vendor>();

    if (vc_cfg_codec.codecId == kLeAudioCodingFormatAptxLeX) {
       kCodecMask = kCodingFormatAptxLeX;
    }

    if (vc_cfg_codec.codecId == kLeAudioCodingFormatAptxLe) {
      kCodecMask = kCodingFormatAptxLe;
    }
  } else if (cfg_codec.getTag() == CodecId::Tag::core) {
     kCodecMask = kCodingFormatLc3;
  }

  LOG(INFO) << __func__
            << ": context: " << context.bitmask << ", kCodecMask: " << (unsigned)kCodecMask;
  context_to_matched_codecId[context] |= kCodecMask;
}

bool LeAudioOffloadAudioProvider::isMatchedValidCodec(CodecId cfg_codec,
                                                      CodecId req_codec) {
  auto priority = codec_priority_map_.find(cfg_codec);
  if (priority != codec_priority_map_.end() && priority->second == -1) {
    LOG(INFO) << __func__ << ": Codec Priority set to -1, return false";
    return false;
  }

  LOG(INFO) << __func__
            << ": cfg_codec Tag: " << toString(cfg_codec.getTag())
            << ", req_codec Tag: " << toString(req_codec.getTag());
  if (cfg_codec.getTag() != req_codec.getTag()) {
    LOG(DEBUG) << ": Config and Capabilities Tags not matched";
    return false;
  }

  if (cfg_codec.getTag() == CodecId::Tag::vendor) {
    auto vc_cfg_codec = cfg_codec.get<CodecId::Tag::vendor>();
    LOG(DEBUG) << ": vc_cfg_codec.id: " << vc_cfg_codec.id;
    LOG(DEBUG) << ": vc_cfg_codec.codecId: " << vc_cfg_codec.codecId;
  }

  if (req_codec.getTag() == CodecId::Tag::vendor) {
    auto vc_req_codec = req_codec.get<CodecId::vendor>();
    LOG(DEBUG) << ": vc_req_codec.id: " << vc_req_codec.id;
    LOG(DEBUG) << ": vc_req_codec.codecId: " << vc_req_codec.codecId;
  }

  if (cfg_codec == req_codec) {
    LOG(INFO) << __func__ << ": codec matched, return true";
    return true;
  }
  LOG(INFO) << __func__ << ": codec not matched, return false";
  return false;
}

void LeAudioOffloadAudioProvider::processCodecMetadata(
       const std::optional<std::vector<std::optional<MetadataLtv>>>& cfg_metadata,
             std::optional<std::vector<std::optional<MetadataLtv>>> caps_metadata,
             MetadataLtv::VendorSpecific& cfg_vendor_metadata,
             MetadataLtv::VendorSpecific& caps_vendor_metadata,
             AudioContext& caps_preferred_AudioContexts,
             AudioContext& caps_streaming_AudioContexts,
             bool& valid_vendor_specific_metadata_exist) {

  LOG(INFO) << __func__ <<": cfg_metadata size: "<< cfg_metadata.value().size();

  for (auto &cfg_meta : cfg_metadata.value()) {
    if (cfg_meta.value().getTag() == MetadataLtv::vendorSpecific) {
      cfg_vendor_metadata = cfg_meta.value().get<MetadataLtv::vendorSpecific>();
      LOG(INFO) << __func__
            << ": cfg_vendor_metadata.companyId: "<< cfg_vendor_metadata.companyId;
      if (cfg_vendor_metadata.companyId != 0) {
        //No need to go for next vendorSpecific metadataLtv,
        //as it has  config name only to debug
        LOG(INFO) << __func__
                  << ": valid vendor codec specific Metadata exist.";
        valid_vendor_specific_metadata_exist = true;
        break;
      }
    }
  }

  LOG(INFO) << __func__ << ": valid_vendor_specific_metadata_exist: "
                        << valid_vendor_specific_metadata_exist;
  if (valid_vendor_specific_metadata_exist && caps_metadata.has_value()) {
    //auto caps_metadata = caps_metadata.value();
    LOG(INFO) << __func__ <<": caps_metadata size: "<< caps_metadata.value().size();

    //check for the match of vendor metadata.
    for (auto &caps_meta : caps_metadata.value()) {
      LOG(INFO) << __func__
                << ": caps_metadata Tag: " << toString(caps_meta.value().getTag());
      switch(caps_meta.value().getTag()) {
        case MetadataLtv::vendorSpecific: {
          caps_vendor_metadata = caps_meta.value().get<MetadataLtv::vendorSpecific>();
          LOG(INFO) << __func__
               << ": caps_vendor_metadata.companyId: "<< caps_vendor_metadata.companyId;
          break;
        }
        case MetadataLtv::preferredAudioContexts: {
          caps_preferred_AudioContexts =
               caps_meta.value().get<MetadataLtv::Tag::preferredAudioContexts>().values;
          LOG(INFO) << __func__
                    << ": caps_preferred_AudioContexts.bitmask: "
                    << caps_preferred_AudioContexts.bitmask;
          break;
        }
        case MetadataLtv::streamingAudioContexts: {
          caps_streaming_AudioContexts =
               caps_meta.value().get<MetadataLtv::Tag::streamingAudioContexts>().values;
          LOG(INFO) << __func__
                    << ": caps_preferred_AudioContexts.bitmask: "
                    << caps_streaming_AudioContexts.bitmask;
          break;
        }
      }
    }
  }

  return;
}


bool LeAudioOffloadAudioProvider::isMatchedValidVendorCodecSpecificMetadata(
                                        CodecId cfg_codec,
                                        MetadataLtv::VendorSpecific& cfg_metadata,
                                        MetadataLtv::VendorSpecific caps_metadata,
                                        AudioContext& preferred_audio_contexts,
                                        AudioContext& setting_context) {

  LOG(INFO) << __func__ <<": cfg_metadata.companyId: "<< cfg_metadata.companyId
                        <<", caps_metadata.companyId: "<< caps_metadata.companyId;
  switch(cfg_metadata.companyId) {
    case kLeAudioVendorCompanyIdQualcomm : {
      LOG(INFO) << __func__ <<": cfg_codec Tag: "<< toString(cfg_codec.getTag());
      switch (cfg_codec.getTag()) {
        case CodecId::Tag::core : {
          auto vc_cfg_codec = cfg_codec.get<CodecId::Tag::core>();
          LOG(INFO) << __func__ <<": vc_cfg_codec: " << (unsigned)vc_cfg_codec;
          if (vc_cfg_codec == CodecId::Core::LC3) {

            if (caps_metadata.companyId == 0) {
              LOG(INFO) << __func__
                    <<": There is no Caps vendorSpecific metadata LTV available, return false.";
              return false;
            }
            //Check for type
            uint8_t cap_vendor_metadata_type = caps_metadata.opaqueValue[1];
            uint8_t cfg_vendor_metadata_type = cfg_metadata.opaqueValue[0];

            LOG(INFO) << __func__
                      <<": cap_vendor_metadata_type: " << (unsigned)cap_vendor_metadata_type
                      <<", cfg_vendor_metadata_type: " << (unsigned)cfg_vendor_metadata_type;

            LOG(INFO) << __func__
                      <<": setting_context.bitmask: " << setting_context.bitmask
                      <<", preferred_audio_contexts.bitmask: "
                      << preferred_audio_contexts.bitmask;

            //Below check is for selecting preferred context type.
            if (caps_metadata.companyId == kLeAudioVendorCompanyIdQualcomm &&
                cap_vendor_metadata_type == kLeAudioCodecLC3QSupportedFeaturesMetadataType) {
              if (setting_context.bitmask != AudioContext::UNSPECIFIED &&
                  !(setting_context.bitmask & preferred_audio_contexts.bitmask)) {
                LOG(INFO) << __func__
                  << ": Setting context not part of Pac Preferred Audio context, return false.";
                return false;
              }
            }

            //LC3Q
            if (cfg_metadata.companyId == caps_metadata.companyId) {
              LOG(INFO) << __func__ << ": config and caps company Ids are matched.";
            } else {
              LOG(INFO) << __func__
                        << ": config and caps company Ids are not matched, return false.";
              return false;
            }

            //check for len
            uint8_t cap_vendor_metadata_len = caps_metadata.opaqueValue[0];

            /* included type as well into opaqueValue in json file as part of extension,
             * considered as total length*/
            uint8_t cfg_vendor_metadata_len = cfg_metadata.opaqueValue.size();

            LOG(INFO) << __func__
                      <<": cap_vendor_metadata_len: " << (unsigned)cap_vendor_metadata_len
                      <<", cfg_vendor_metadata_len: " << (unsigned)cfg_vendor_metadata_len;

            if (cap_vendor_metadata_len != cfg_vendor_metadata_len) {
              LOG(INFO) << __func__
                        << ": Length mismatch b/w caps and config vendor metadata, return false.";
              return false;
            }

            if (caps_metadata.companyId == kLeAudioVendorCompanyIdQualcomm &&
                cap_vendor_metadata_len != kLeAudioCodecLC3QSupportedFeaturesMetadataLen) {
              LOG(INFO) << __func__ << ": Invalid caps vendor metadata length, return false.";
              return false;
            }

            if (cap_vendor_metadata_type != cfg_vendor_metadata_type) {
              LOG(INFO) << __func__
                        << ": Type mismatch b/w caps and config vendor metadata, return false.";
              return false;
            }

            if (caps_metadata.companyId == kLeAudioVendorCompanyIdQualcomm &&
                cap_vendor_metadata_type != kLeAudioCodecLC3QSupportedFeaturesMetadataType) {
              LOG(INFO) << __func__ << ": Invalid caps vendor metadata type, return false.";
              return false;
            }

            uint8_t cap_decoder_version = caps_metadata.opaqueValue[3];
            uint8_t cfg_encoder_version = cfg_metadata.opaqueValue[2];

            LOG(INFO) << __func__ <<": cap_decoder_version: " << (unsigned)cap_decoder_version
                                  <<", cfg_encoder_version: " << (unsigned)cfg_encoder_version;
            if (cap_decoder_version < cfg_encoder_version) {
              LOG(INFO) << __func__
                        << ": Encoder version cfg couldn't be negotiated as corresponding peer"
                           " decoder couldn't be matched as"
                           " cap_decoder_version < cfg_encoder_version, return false";
              return false;
            }

            uint8_t cap_encoder_version = caps_metadata.opaqueValue[2];
            uint8_t cfg_decoder_version = cfg_metadata.opaqueValue[3];

            LOG(INFO) << __func__ <<": cap_encoder_version: " << (unsigned)cap_encoder_version
                                  <<", cfg_decoder_version: " << (unsigned)cfg_decoder_version;
            if (cap_encoder_version < cfg_decoder_version) {
              LOG(INFO) << __func__
                        << ": Decoder version cfg couldn't be negotiated as corresponding peer"
                           " encoder couldn't be matched as"
                           " pac_encoder_version < req_decoder_version, return false";
              return false;
            }
          }
          break;
        }

        case CodecId::Tag::vendor : {
          auto vc_codec_config = cfg_codec.get<CodecId::Tag::vendor>();
          LOG(DEBUG) << ": vc_codec_config.id: " << vc_codec_config.id;
          LOG(DEBUG) << ": vc_codec_config.codecId: " << vc_codec_config.codecId;
          switch (vc_codec_config.id) {
            case kLeAudioVendorCompanyIdQualcomm : {
              switch (vc_codec_config.codecId) {
                case kLeAudioCodingFormatAptxLeX : {
                  break;
                }

                case kLeAudioCodingFormatAptxLe : {
                  if (cfg_metadata.companyId == caps_metadata.companyId) {
                    LOG(INFO) << __func__ << ": config and caps company Ids are matched.";
                  } else {
                    LOG(INFO) << __func__ << ": config and caps company Ids are not matched, return false.";
                    return false;
                  }

                  //check for len
                  uint8_t cap_vendor_metadata_len = caps_metadata.opaqueValue[0];
                  /* included type as well into opaqueValue in json file as part of extension,
                   * considered as total length*/
                  uint8_t cfg_vendor_metadata_len = cfg_metadata.opaqueValue.size();

                  LOG(INFO) << __func__
                            <<": cap_vendor_metadata_len: " << (unsigned)cap_vendor_metadata_len
                            <<", cfg_vendor_metadata_len: " << (unsigned)cfg_vendor_metadata_len;

                  if (cap_vendor_metadata_len != cfg_vendor_metadata_len) {
                    LOG(INFO) << __func__
                              << ": Length mismatch b/w caps and config vendor metadata, return false.";
                    return false;
                  }

                  if (caps_metadata.companyId == kLeAudioVendorCompanyIdQualcomm &&
                      cap_vendor_metadata_len != kLeAudioCodecAptxLeSupportedFeaturesMetadataLen) {
                    LOG(INFO) << __func__ << ": Invalid caps vendor metadata length, return false.";
                    return false;
                  }

                  //Check for type
                  uint8_t cap_vendor_metadata_type = caps_metadata.opaqueValue[1];
                  uint8_t cfg_vendor_metadata_type = cfg_metadata.opaqueValue[0];

                  LOG(INFO) << __func__
                            <<": cap_vendor_metadata_type: " << (unsigned)cap_vendor_metadata_type
                            <<", cfg_vendor_metadata_type: " << (unsigned)cfg_vendor_metadata_type;

                  if (cap_vendor_metadata_type != cfg_vendor_metadata_type) {
                    LOG(INFO) << __func__
                              << ": Type mismatch b/w caps and config vendor metadata, return false.";
                    return false;
                  }

                  if (caps_metadata.companyId == kLeAudioVendorCompanyIdQualcomm &&
                      cap_vendor_metadata_type != kLeAudioCodecAptxLeSupportedFeaturesMetadataType) {
                    LOG(INFO) << __func__ << ": Invalid caps vendor metadata type, return false.";
                    return false;
                  }

                  uint8_t cap_codec_version = caps_metadata.opaqueValue[3];
                  uint8_t cfg_codec_version = cfg_metadata.opaqueValue[2];

                  LOG(INFO) << __func__ <<": cap_codec_version: " << (unsigned)cap_codec_version
                                        <<", cfg_codec_version: " << (unsigned)cfg_codec_version;
                  if (cap_codec_version < cfg_codec_version) {
                    LOG(INFO) << __func__
                              << ": Codec version unspported cap_codec_version < cfg_codec_version.";
                    return false;
                  }

                  uint8_t cap_vendor_dual_ack_support = caps_metadata.opaqueValue[7];
                  uint8_t cfg_vendor_dual_ack_support = cfg_metadata.opaqueValue[7];

                  LOG(INFO) << __func__
                            <<": cap_vendor_dual_ack_support: " << (unsigned)cap_vendor_dual_ack_support
                            <<", cfg_vendor_dual_ack_support: " << (unsigned)cfg_vendor_dual_ack_support;
                  if (cfg_vendor_dual_ack_support == kLeAudioCodecAptxLeSupportedDualAck) {
                    if (cap_vendor_dual_ack_support != cfg_vendor_dual_ack_support) {
                      LOG(INFO) << __func__ << ": dual_ack is not supported.";
                      return false;
                    }
                  }
                  break;
                }
              }
              break;
            }
          }
          break;
        }
      }
      break;
    }
  }
  LOG(INFO) << __func__ << ": Caps and Config vendor codec Metadata matched, return true.";
  return true;
}

bool LeAudioOffloadAudioProvider::filterCapabilitiesMatchedContext(
    AudioContext& setting_context,
    const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities) {
  // If has no metadata, assume match
  if (!capabilities.metadata.has_value()) return true;

  for (auto metadata : capabilities.metadata.value()) {
    if (!metadata.has_value()) continue;
    if (metadata.value().getTag() == MetadataLtv::Tag::preferredAudioContexts) {
      // Check all pref audio context to see if anything matched
      auto& prefer_context =
          metadata.value()
              .get<MetadataLtv::Tag::preferredAudioContexts>()
              .values;
      LOG(INFO) << __func__ << ": setting_context.bitmask: " << setting_context.bitmask
            << ", prefer_context.bitmask : " << prefer_context.bitmask;
      if (setting_context.bitmask & prefer_context.bitmask) {
        // New mask with matched capability
        setting_context.bitmask &= prefer_context.bitmask;
        return true;
      }
    }
  }

  return false;
}

bool LeAudioOffloadAudioProvider::isMatchedSamplingFreq(
    CodecSpecificConfigurationLtv::SamplingFrequency& cfg_freq,
    CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies&
        capability_freq) {
  auto p = freq_to_support_bitmask_map.find(cfg_freq);
  if (p != freq_to_support_bitmask_map.end()) {
    if (capability_freq.bitmask & p->second) {
      LOG(INFO) << __func__ << ": Sampling freq matched. ";
      return true;
    }
  }
  return false;
}

bool LeAudioOffloadAudioProvider::isMatchedFrameDuration(
    CodecSpecificConfigurationLtv::FrameDuration& cfg_fduration,
    CodecSpecificCapabilitiesLtv::SupportedFrameDurations&
        capability_fduration) {
  auto p = fduration_to_support_fduration_map.find(cfg_fduration);
  if (p != fduration_to_support_fduration_map.end())
    if (capability_fduration.bitmask & p->second) {
      LOG(INFO) << __func__ << ": Frame duration matched. ";
      return true;
    }
  return false;
}

int getCountFromBitmask(int bitmask) {
  return std::bitset<32>(bitmask).count();
}

bool LeAudioOffloadAudioProvider::isMatchedAudioChannel(
    CodecSpecificConfigurationLtv::AudioChannelAllocation& cfg_channel,
    CodecSpecificCapabilitiesLtv::SupportedAudioChannelCounts&
        capability_channel) {
  int count = getCountFromBitmask(cfg_channel.bitmask);
  LOG(INFO) << __func__ << ": count: " << count << ", bitmask: " << capability_channel.bitmask;
  if (count == 1 &&
      !(capability_channel.bitmask &
        CodecSpecificCapabilitiesLtv::SupportedAudioChannelCounts::ONE))
    return false;
  if (count == 2 &&
      !(capability_channel.bitmask &
        CodecSpecificCapabilitiesLtv::SupportedAudioChannelCounts::TWO))
    return false;
  LOG(INFO) << __func__ << ": Audio Channel matched. ";
  return true;
}

bool LeAudioOffloadAudioProvider::isMatchedCodecFramesPerSDU(
    CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU& cfg_frame_sdu,
    CodecSpecificCapabilitiesLtv::SupportedMaxCodecFramesPerSDU&
        capability_frame_sdu) {
  LOG(INFO) << __func__ << ": cfg_frame_sdu.value: " << cfg_frame_sdu.value
                        << ", capability_frame_sdu.value : " << capability_frame_sdu.value;
  if (cfg_frame_sdu.value <= capability_frame_sdu.value) {
    LOG(INFO) << __func__ << ": Codec Frames per sdu within limits.";
    return true;
  }
  return false;
}

bool LeAudioOffloadAudioProvider::isMatchedOctetsPerCodecFrame(
    CodecSpecificConfigurationLtv::OctetsPerCodecFrame& cfg_octets,
    CodecSpecificCapabilitiesLtv::SupportedOctetsPerCodecFrame&
        capability_octets) {
  LOG(INFO) << __func__ << ": cfg_octets.value: " << cfg_octets.value
            << ", capability_octets.min : " << capability_octets.min
            << ", capability_octets.max: " << capability_octets.max;
  if (cfg_octets.value >= capability_octets.min &&
      cfg_octets.value <= capability_octets.max) {
    LOG(INFO) << __func__ << ": Octets per Codec Frames within limits.";
    return true;
  }
  return false;
}

bool LeAudioOffloadAudioProvider::isCapabilitiesMatchedCodecConfiguration(
    std::vector<CodecSpecificConfigurationLtv>& codec_cfg,
    std::vector<CodecSpecificCapabilitiesLtv> codec_capabilities) {
  // Convert all codec_cfg into a map of tags -> correct data
  std::map<CodecSpecificConfigurationLtv::Tag, CodecSpecificConfigurationLtv>
      cfg_tag_map;
  for (auto codec_cfg_data : codec_cfg) {
    cfg_tag_map[codec_cfg_data.getTag()] = codec_cfg_data;
    LOG(INFO) << __func__
              << ": codec_cfg Tag: " << toString(codec_cfg_data.getTag());
  }

  LOG(INFO) << __func__ << ": codec_capabilities size: " << codec_capabilities.size();
  for (auto& codec_capability : codec_capabilities) {
    auto cfg = cfg_tag_map.find(cap_to_cfg_tag_map[codec_capability.getTag()]);
    // If capability has this tag, but our configuration doesn't
    // Then we will assume it is matched
    if (cfg == cfg_tag_map.end()) {
      LOG(INFO) << __func__ << ": cap to config tag not matched, continue.";
      continue;
    }

    switch (codec_capability.getTag()) {
      case CodecSpecificCapabilitiesLtv::Tag::supportedSamplingFrequencies: {
        if (!isMatchedSamplingFreq(
                cfg->second.get<
                    CodecSpecificConfigurationLtv::Tag::samplingFrequency>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedSamplingFrequencies>())) {
          LOG(INFO) << __func__ << ": Sampling freq not matched.";
          return false;
        }
        break;
      }

      case CodecSpecificCapabilitiesLtv::Tag::supportedFrameDurations: {
        if (!isMatchedFrameDuration(
                cfg->second
                    .get<CodecSpecificConfigurationLtv::Tag::frameDuration>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedFrameDurations>())) {
          LOG(INFO) << __func__ << ": Frame duration not matched.";
          return false;
        }
        break;
      }

      case CodecSpecificCapabilitiesLtv::Tag::supportedAudioChannelCounts: {
        if (!isMatchedAudioChannel(
                cfg->second.get<CodecSpecificConfigurationLtv::Tag::
                                    audioChannelAllocation>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedAudioChannelCounts>())) {
          LOG(INFO) << __func__ << ": Audio Channel count not matched.";
          return false;
        }
        break;
      }

      case CodecSpecificCapabilitiesLtv::Tag::supportedMaxCodecFramesPerSDU: {
        if (!isMatchedCodecFramesPerSDU(
                cfg->second.get<CodecSpecificConfigurationLtv::Tag::
                                    codecFrameBlocksPerSDU>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedMaxCodecFramesPerSDU>())) {
          LOG(INFO) << __func__ << ": Max Codec Frames per sdu not matched.";
          return false;
        }
        break;
      }

      case CodecSpecificCapabilitiesLtv::Tag::supportedOctetsPerCodecFrame: {
        if (!isMatchedOctetsPerCodecFrame(
                cfg->second.get<
                    CodecSpecificConfigurationLtv::Tag::octetsPerCodecFrame>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedOctetsPerCodecFrame>())) {
          LOG(INFO) << __func__ << ": Octets per codecframe not matched.";
          return false;
        }
        break;
      }
    }
  }

  return true;
}

bool LeAudioOffloadAudioProvider::filterMatchedAseConfiguration(
    LeAudioAseConfiguration& setting_cfg,
    const LeAudioAseConfiguration& requirement_cfg) {
  // Check matching for codec configuration <=> requirement ASE codec
  // Also match if no CodecId requirement
  if (requirement_cfg.codecId.has_value()) {
    if (!setting_cfg.codecId.has_value()) {
      LOG(INFO) << __func__ << ": setting config codec id doesn't exist, return";
      return false;
    }
    if (!isMatchedValidCodec(setting_cfg.codecId.value(),
                             requirement_cfg.codecId.value())) {
      LOG(INFO) << __func__ << ": setting and requirement codec id not matched, return";
      return false;
    }
  }

  //LOG(INFO) << __func__ << ": requirement targetLatency: " << requirement_cfg.targetLatency
  //                      << ", setting targetLatency: " << setting_cfg.targetLatency;

  if (requirement_cfg.targetLatency != LeAudioAseConfiguration::TargetLatency::UNDEFINED &&
      setting_cfg.targetLatency != requirement_cfg.targetLatency) {
    LOG(INFO) << __func__ << ": setting and requirement targetLatency not matched, return";
    return false;
  }
  // Ignore PHY requirement

  // Check all codec configuration
  std::map<CodecSpecificConfigurationLtv::Tag, CodecSpecificConfigurationLtv> cfg_tag_map;

  LOG(INFO) << __func__
     << ": setting codecConfiguration size: " << setting_cfg.codecConfiguration.size()
     << ", requirement codecConfiguration size: " << requirement_cfg.codecConfiguration.size();

  for (auto cfg : setting_cfg.codecConfiguration)
    cfg_tag_map[cfg.getTag()] = cfg;

  for (auto requirement_cfg : requirement_cfg.codecConfiguration) {
    // Directly compare CodecSpecificConfigurationLtv
    auto cfg = cfg_tag_map.find(requirement_cfg.getTag());
    // Config not found for this requirement, cannot match
    if (cfg == cfg_tag_map.end()) {
      LOG(INFO) << __func__ << ": Config not found for this requirement, cannot match";
      return false;
    }

    // Ignore matching for audio channel allocation
    // since the rule is complicated. Match outside instead
    if (requirement_cfg.getTag() == CodecSpecificConfigurationLtv::Tag::audioChannelAllocation) {
      LOG(INFO) << __func__ << ": Ignore matching with audio channel allocation, continue";
      continue;
    }

    if (cfg->second != requirement_cfg) {
      LOG(INFO) << __func__ << ": setting and required config not matched, return.";
      return false;
    }
  }
  // Ignore vendor configuration and metadata requirement

  return true;
}

void LeAudioOffloadAudioProvider::prepareMatchedCodecIdBitmask(const AudioContext& context,
                                                      LeAudioAseConfiguration& setting_cfg) {
  if (!setting_cfg.codecId.has_value()) {
    LOG(INFO) << __func__ << ": setting config codec id doesn't exist, return";
    return;
  }
  PrepareCodecBitmask(setting_cfg.codecId.value(), context);
}

bool LeAudioOffloadAudioProvider::isMatchedBISConfiguration(
    LeAudioBisConfiguration bis_cfg,
    const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities) {
  if (!isMatchedValidCodec(bis_cfg.codecId, capabilities.codecId)) {
    return false;
  }
  if (!isCapabilitiesMatchedCodecConfiguration(
          bis_cfg.codecConfiguration, capabilities.codecSpecificCapabilities)) {
    return false;
  }
  return true;
}

void LeAudioOffloadAudioProvider::filterCapabilitiesAseDirectionConfiguration(
    std::vector<std::optional<AseDirectionConfiguration>>&
        direction_configurations,
    const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities,
    std::vector<std::optional<AseDirectionConfiguration>>&
        valid_direction_configurations, AudioContext& setting_context) {

  LOG(INFO) << __func__ << ": direction_configurations size: "
            << direction_configurations.size();

  for (auto direction_configuration : direction_configurations) {
    if (!direction_configuration.has_value()) {
      LOG(INFO) << __func__ << ": There is no direction_configuration, continue";
      continue;
    }

    if (!direction_configuration.value().aseConfiguration.codecId.has_value()) {
      LOG(INFO) << __func__ << ": There is no valid codec in direction_configuration, continue";
      continue;
    }

    if (!isMatchedValidCodec(direction_configuration.value().aseConfiguration.codecId.value(),
                             capabilities.codecId)) {
      LOG(INFO) << __func__ << ": codec not matched, continue.";
      continue;
    }

    // Check matching for codec configuration <=> codec capabilities
    if (!isCapabilitiesMatchedCodecConfiguration(
            direction_configuration.value().aseConfiguration.codecConfiguration,
            capabilities.codecSpecificCapabilities)) {
      LOG(INFO) << __func__ << ": dircetion config and capabilities config not matched";
      continue;
    }

    if (direction_configuration.value().aseConfiguration.metadata.has_value()) {
      MetadataLtv::VendorSpecific cfg_vendor_metadata = MetadataLtv::VendorSpecific();
      MetadataLtv::VendorSpecific caps_vendor_metadata = MetadataLtv::VendorSpecific();
      AudioContext caps_preferred_AudioContexts = AudioContext();
      AudioContext caps_streaming_AudioContexts = AudioContext();
      auto cfg_id = direction_configuration.value().aseConfiguration.codecId.value();

      //If LC3Q is there then below would set.
      bool valid_vendor_specific_metadata_exist = false;

      processCodecMetadata(direction_configuration.value().aseConfiguration.metadata,
                                 capabilities.metadata, cfg_vendor_metadata, caps_vendor_metadata,
                                 caps_preferred_AudioContexts, caps_streaming_AudioContexts,
                                 valid_vendor_specific_metadata_exist);

      LOG(INFO) << __func__ << ": valid_vendor_specific_metadata_exist: "
                            << valid_vendor_specific_metadata_exist;

      //Checking for match of json setting vendorspecificMetadata
      //and Pacs capability vendorMetadata
      if (valid_vendor_specific_metadata_exist &&
          !isMatchedValidVendorCodecSpecificMetadata(cfg_id, cfg_vendor_metadata,
                       caps_vendor_metadata, caps_preferred_AudioContexts, setting_context)) {
        LOG(INFO) << __func__
                  << ": dircetion metadata and capabilities metadata not matched, continue";
        continue;
      }
    }

    valid_direction_configurations.push_back(direction_configuration);
  }
}

int getLeAudioAseConfigurationAllocationBitmask(LeAudioAseConfiguration cfg) {
  for (auto cfg_ltv : cfg.codecConfiguration) {
    if (cfg_ltv.getTag() ==
        CodecSpecificConfigurationLtv::Tag::audioChannelAllocation) {
      return cfg_ltv
          .get<CodecSpecificConfigurationLtv::Tag::audioChannelAllocation>()
          .bitmask;
    }
  }
  return 0;
}

std::optional<AseDirectionConfiguration> findValidMonoConfig(
    std::vector<AseDirectionConfiguration>& valid_direction_configurations,
    int bitmask) {
  LOG(DEBUG) << __func__ << ": bitmask: " << bitmask;
  LOG(DEBUG) << __func__ << ": valid_direction_configurations size: "
                         << valid_direction_configurations.size();
  for (auto& cfg : valid_direction_configurations) {
    int cfg_bitmask =
        getLeAudioAseConfigurationAllocationBitmask(cfg.aseConfiguration);
    LOG(DEBUG) << __func__ << ": cfg_bitmask: " << cfg_bitmask;
    if (getCountFromBitmask(cfg_bitmask) <= 1) {
      // Modify the bitmask to be the same as the requirement
      for (auto& codec_cfg : cfg.aseConfiguration.codecConfiguration) {
        if (codec_cfg.getTag() ==
            CodecSpecificConfigurationLtv::Tag::audioChannelAllocation) {
          LOG(DEBUG) << __func__ << ": Set audioChannelAllocation bitmask.";
          codec_cfg
              .get<CodecSpecificConfigurationLtv::Tag::audioChannelAllocation>()
              .bitmask = bitmask;
          return cfg;
        }
      }
    }
  }
  return std::nullopt;
}

void populateVendorAudioChannelAllocation(
                        std::optional<AseDirectionConfiguration>& config) {
  //Filling vendorCodecConfiguration with AudioChannelAllocation
  auto cfg_CodecId = config.value().aseConfiguration.codecId.value();
  LOG(INFO) << __func__
            << ": cfg_CodecId Tag: " << toString(cfg_CodecId.getTag());
  if (cfg_CodecId.getTag() == CodecId::Tag::vendor) {
    auto cfg_codec_id = cfg_CodecId.get<CodecId::Tag::vendor>();
    LOG(DEBUG) << ": cfg_codec_id.id: " << cfg_codec_id.id;
    LOG(DEBUG) << ": cfg_codec_id.codecId: " << cfg_codec_id.codecId;
    if (cfg_codec_id.id == kLeAudioVendorCompanyIdQualcomm &&
        (cfg_codec_id.codecId == kLeAudioCodingFormatAptxLe ||
         cfg_codec_id.codecId == kLeAudioCodingFormatAptxLeX)) {

      auto VenCodecConfig = std::vector<uint8_t>();
      auto codecConfig = std::vector<CodecSpecificConfigurationLtv>();
      codecConfig = config.value().aseConfiguration.codecConfiguration;

      for (auto& codec_cfg : codecConfig) {
        if (codec_cfg.getTag() == CodecSpecificConfigurationLtv::Tag::audioChannelAllocation) {
          uint32_t channel_allocation =
            codec_cfg.get<CodecSpecificConfigurationLtv::Tag::audioChannelAllocation>().bitmask;
          LOG(DEBUG) << ": channel_allocation: " << (unsigned)channel_allocation;
          uint8_t len = sizeof(channel_allocation) + 1;
          uint8_t type = kLeAudioCodecAptxLeTypeAudioChannelAllocation;

          VenCodecConfig.push_back(len);
          VenCodecConfig.push_back(type);
          uint8_t* allocationBytes = reinterpret_cast<uint8_t*>(&channel_allocation);
          VenCodecConfig.insert(VenCodecConfig.end(), allocationBytes,
            allocationBytes + sizeof(uint32_t));
          config.value().aseConfiguration.vendorCodecConfiguration.value().insert(
                          config.value().aseConfiguration.vendorCodecConfiguration.value().end(),
                          VenCodecConfig.begin(), VenCodecConfig.end());
        }
      }
    }
  }
}

std::vector<AseDirectionConfiguration> getValidConfigurationsFromAllocation(
    int req_allocation_bitmask,
    std::vector<AseDirectionConfiguration>& valid_direction_configurations,
    bool isExact) {
  // Prefer the same allocation_bitmask
  int channel_count = getCountFromBitmask(req_allocation_bitmask);

  LOG(DEBUG) << __func__ << ": isExact: " << isExact;
  LOG(DEBUG) << __func__ << ": channel_count: " << channel_count;

  bool pts_gmap = property_get_bool("persist.vendor.qcom.bluetooth.pts_gmap", false);
  LOG(DEBUG) << __func__ << " pts_gmap: " << pts_gmap;
  if(!isExact && pts_gmap) {
    LOG(INFO) << __func__ << ": GMAP is enabled, go for exact matching only ";
    return {};
  }
  if (isExact) {
    for (auto& cfg : valid_direction_configurations) {
      int cfg_bitmask =
          getLeAudioAseConfigurationAllocationBitmask(cfg.aseConfiguration);
      if (cfg_bitmask == req_allocation_bitmask) {
        LOG(DEBUG) << __func__
            << ": Found an exact match for the requirement allocation of " << cfg_bitmask;
        return {cfg};
      }
    }
    return {};
  }
  // Not using exact match strategy
  if (channel_count <= 1) {
    // Mono requirement matched if cfg is a mono config
    auto cfg = findValidMonoConfig(valid_direction_configurations,
                                   req_allocation_bitmask);
    if (cfg.has_value()) {
      LOG(DEBUG) << __func__ << ": add valid mono config.";
      populateVendorAudioChannelAllocation(cfg);
      return {cfg.value()};
    }
  } else {
    // Stereo requirement returns 2 mono configs
    // that has a combined bitmask equal to the stereo config
    std::vector<AseDirectionConfiguration> temp;
    for (int bit = 0; bit < 32; ++bit)
      if (req_allocation_bitmask & (1 << bit)) {
        auto cfg =
            findValidMonoConfig(valid_direction_configurations, (1 << bit));
        if (cfg.has_value()) {
          LOG(DEBUG) << __func__ << ": push valid mono config.";
          populateVendorAudioChannelAllocation(cfg);
          temp.push_back(cfg.value());
        }
      }
    LOG(DEBUG) << __func__ << ": temp ase size: " << temp.size();
    if (temp.size() == channel_count) return temp;
  }
  return {};
}

// Check and filter each index to see if it's a match.
void LeAudioOffloadAudioProvider::filterRequirementAseDirectionConfiguration(
    std::optional<std::vector<std::optional<AseDirectionConfiguration>>>&
        direction_configurations,
    const std::vector<std::optional<AseDirectionRequirement>>& requirements,
    std::optional<std::vector<std::optional<AseDirectionConfiguration>>>&
        valid_direction_configurations,
    bool isExact, std::optional<ConfigurationFlags> flags, uint8_t direction,
    const AudioContext& context) {
  if (!direction_configurations.has_value()) return;

  if (!valid_direction_configurations.has_value()) {
    valid_direction_configurations =
        std::vector<std::optional<AseDirectionConfiguration>>();
  }

  LOG(INFO) << __func__ << ": requirements size: " << requirements.size();
  LOG(INFO) << __func__ << ": direction config size: " << direction_configurations->size();
  LOG(INFO) << __func__ << ": isExact: " << isExact;
  LOG(INFO) << __func__ << ": direction: " << (unsigned)direction;

  bool pts_gmap = property_get_bool("persist.vendor.qcom.bluetooth.pts_gmap", false);
  LOG(DEBUG) << __func__ << " pts_gmap: " << pts_gmap;
  if(!isExact && pts_gmap) {
    LOG(INFO) << __func__ << ": GMAP is enabled, go for exact matching only ";
    return;
  }
  if (isExact) {
    // Exact matching process
    // Need to respect the number of device
    for (int i = 0; i < requirements.size(); ++i) {
      auto requirement = requirements[i];
      auto direction_configuration = direction_configurations.value()[i];

      LOG(INFO) << __func__ << ": requirement: " << requirement->toString();
      LOG(INFO) << __func__ << ": direction_configuration: " << direction_configuration->toString();

      if (!direction_configuration.has_value()) {
        LOG(INFO) << __func__ << ": No config for this direction ";
        valid_direction_configurations = std::nullopt;
        return;
      }

      auto cfg = direction_configuration.value();
      if (!filterMatchedAseConfiguration(cfg.aseConfiguration,
                                         requirement.value().aseConfiguration)) {
        LOG(INFO) << __func__ << ": No filtered config for this direction ";
        valid_direction_configurations = std::nullopt;
        return;  // No way to match
      }

      // For exact match, we require this direction to have the same allocation.
      // If stereo, need stereo.
      // If mono, need mono (modified to the correct required allocation)
      auto req_allocation_bitmask = getLeAudioAseConfigurationAllocationBitmask(
          requirement.value().aseConfiguration);
      int req_channel_count = getCountFromBitmask(req_allocation_bitmask);
      int cfg_bitmask =
          getLeAudioAseConfigurationAllocationBitmask(cfg.aseConfiguration);
      int cfg_channel_count = getCountFromBitmask(cfg_bitmask);

      LOG(INFO) << __func__ << ": req_allocation_bitmask: " << req_allocation_bitmask
                            << ": req_channel_count: " << req_channel_count
                            << ": cfg_bitmask: " << cfg_bitmask
                            << ": cfg_channel_count: " << cfg_channel_count;
      if (req_channel_count <= 1) {
        // MONO case, is a match if also mono, modify to the same allocation
        if (cfg_channel_count > 1) {
          valid_direction_configurations = std::nullopt;
          return;  // Not a match
        }
        // Modify the bitmask to be the same as the requirement
        for (auto& codec_cfg : cfg.aseConfiguration.codecConfiguration) {
          if (codec_cfg.getTag() ==
              CodecSpecificConfigurationLtv::Tag::audioChannelAllocation) {
            LOG(INFO) << __func__ << ": set codecconfig audioChannelAllocation bitmask";
            codec_cfg
                .get<CodecSpecificConfigurationLtv::Tag::audioChannelAllocation>()
                .bitmask = req_allocation_bitmask;
            break;
          }
        }
      } else {
        // STEREO case, is a match if same allocation
        if (req_allocation_bitmask != cfg_bitmask) {
          LOG(INFO) << __func__ << ": stereo case no match of req and config allocation";
          valid_direction_configurations = std::nullopt;
          return;  // Not a match
        }
      }
      // Push to list if valid
      valid_direction_configurations.value().push_back(cfg);
      LOG(DEBUG) << ": Exact match choosed audio_context: " << (unsigned)context.bitmask;
      prepareMatchedCodecIdBitmask(context, cfg.aseConfiguration);
    }
  } else {
    // Loose matching process
    for (auto& requirement : requirements) {
      if (!requirement.has_value()) {
        LOG(INFO) << __func__ << ": No requirement exist, continue";
        continue;
      }
      auto req_allocation_bitmask = getLeAudioAseConfigurationAllocationBitmask(
          requirement.value().aseConfiguration);
      auto req_channel_count = getCountFromBitmask(req_allocation_bitmask);

      LOG(INFO) << __func__ << ": req_allocation_bitmask: " << req_allocation_bitmask
                            << ": req_channel_count: " << req_channel_count;

      auto temp = std::vector<AseDirectionConfiguration>();

      for (auto direction_configuration : direction_configurations.value()) {
        if (!direction_configuration.has_value()) {
          LOG(INFO) << __func__ << ": No config found, in loose match. continue";
          continue;
        }
        if (!filterMatchedAseConfiguration(direction_configuration.value().aseConfiguration,
                                           requirement.value().aseConfiguration)) {
          LOG(INFO) << __func__ << ": No filtered match found, in loose match. continue";
          continue;
        }
        // Valid if match any requirement.
        temp.push_back(direction_configuration.value());
      }

      if (flags.has_value()) {
        LOG(INFO) << __func__ << ": configuration flags mask: " << flags.value().bitmask;
        if (direction == kLeAudioDirectionSource && direction_configurations->size() == 1 &&
            ((flags.value().bitmask & (ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS |
                          ConfigurationFlags::MONO_MIC_CONFIGURATION)) ==
               (ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS |
                                   ConfigurationFlags::MONO_MIC_CONFIGURATION))) {
           //For TWM Mono MIC config setting below req_allocation_bitmask to 1
           req_allocation_bitmask = 1;
           req_channel_count = getCountFromBitmask(req_allocation_bitmask);
        }
      }

      // Get the best matching config based on channel allocation
      auto total_cfg_channel_count = 0;
      auto req_valid_configs = getValidConfigurationsFromAllocation(
          req_allocation_bitmask, temp, isExact);
      LOG(INFO) << __func__ << ": vaid config size: " << req_valid_configs.size();
      // Count and check required channel counts
      for (auto& cfg : req_valid_configs) {
        total_cfg_channel_count += getCountFromBitmask(
            getLeAudioAseConfigurationAllocationBitmask(cfg.aseConfiguration));
        valid_direction_configurations.value().push_back(cfg);
        LOG(DEBUG) << ": Lose match choosed audio_context: " << (unsigned)context.bitmask;
        prepareMatchedCodecIdBitmask(context, cfg.aseConfiguration);
      }
      LOG(INFO) << __func__ << ": total_cfg_channel_count: " << total_cfg_channel_count;
      if (total_cfg_channel_count != req_channel_count) {
        LOG(INFO) << __func__ << ": Not valid config, return null";
        valid_direction_configurations = std::nullopt;
        return;
      }
    }
  }
}

/* Get a new LeAudioAseConfigurationSetting by matching a setting with a
 * capabilities. The new setting will have a filtered list of
 * AseDirectionConfiguration that matched the capabilities */
std::optional<LeAudioAseConfigurationSetting>
LeAudioOffloadAudioProvider::getCapabilitiesMatchedAseConfigurationSettings(
    IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
    const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities,
    uint8_t direction) {
  // Create a new LeAudioAseConfigurationSetting and return
  LeAudioAseConfigurationSetting filtered_setting{
      .audioContext = setting.audioContext,
      .sinkAseConfiguration = setting.sinkAseConfiguration,
      .sourceAseConfiguration = setting.sourceAseConfiguration,
      .flags = setting.flags,
      .packing = setting.packing,
  };

  LOG(INFO) << __func__ << ": direction = " << unsigned(direction);
  //LOG(INFO) << __func__ << ": setting: " << setting.toString();
  //LOG(INFO) << __func__ << ": filtered_setting: " << filtered_setting.toString();

  // Get a list of all matched AseDirectionConfiguration
  // for the input direction
  std::vector<std::optional<AseDirectionConfiguration>>* direction_configuration = nullptr;
  if (direction == kLeAudioDirectionSink) {
    if (!filtered_setting.sinkAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sinkAseConfiguration available";
      return std::nullopt;
    }
    direction_configuration = &filtered_setting.sinkAseConfiguration.value();
  } else {
    if (!filtered_setting.sourceAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sourceAseConfiguration available";
      return std::nullopt;
    }
    direction_configuration = &filtered_setting.sourceAseConfiguration.value();
  }

  std::vector<std::optional<AseDirectionConfiguration>> valid_direction_configuration;
  filterCapabilitiesAseDirectionConfiguration(*direction_configuration, capabilities,
                                  valid_direction_configuration, setting.audioContext);

  // No valid configuration for this direction
  if (valid_direction_configuration.empty()) {
    LOG(INFO) << __func__ << ": valid_direction_configuration is empty.";
    return std::nullopt;
  }

  // Create a new LeAudioAseConfigurationSetting and return
  // For other direction will contain all settings
  if (direction == kLeAudioDirectionSink) {
    filtered_setting.sinkAseConfiguration = valid_direction_configuration;
    LOG(INFO) << __func__ << ": filtered_setting sink ase config available.";
  } else {
    filtered_setting.sourceAseConfiguration = valid_direction_configuration;
    LOG(INFO) << __func__ << ": filtered_setting source ase config available.";
  }

  return filtered_setting;
}

/* Get a new LeAudioAseConfigurationSetting by matching a setting with a
 * requirement. The new setting will have a filtered list of
 * AseDirectionConfiguration that matched the requirement */
std::optional<LeAudioAseConfigurationSetting>
LeAudioOffloadAudioProvider::getRequirementMatchedAseConfigurationSettings(
    IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
    const IBluetoothAudioProvider::LeAudioConfigurationRequirement& requirement,
    bool isExact) {
  // Create a new LeAudioAseConfigurationSetting to return
  // Make context the same as the requirement
  LeAudioAseConfigurationSetting filtered_setting{
      .audioContext = requirement.audioContext,
      .packing = setting.packing,
      .flags = setting.flags,
  };

  // The number of AseDirectionRequirement in the requirement
  // is the number of device.

  // The exact matching process is as follow:
  // 1. Setting direction has the same number of cfg (ignore when null require)
  // 2. For each index, it's a 1-1 filter / mapping.

  LOG(INFO) << __func__ << ": isExact: " << isExact;
  bool pts_gmap = property_get_bool("persist.vendor.qcom.bluetooth.pts_gmap", false);
  LOG(DEBUG) << __func__ << " pts_gmap: " << pts_gmap;
  if(!isExact && pts_gmap) {
    LOG(INFO) << __func__ << ": GMAP is enabled, go for exact matching only ";
    return std::nullopt;
  }
  if (isExact) {
    if (requirement.sinkAseRequirement.has_value()) {
      LOG(INFO) << __func__ << ": sink ase: requirement size: "
                            << requirement.sinkAseRequirement.value().size()
                            << ", setting size: "
                            << setting.sinkAseConfiguration.value().size();
    }

    if (requirement.sinkAseRequirement.has_value() &&
        requirement.sinkAseRequirement.value().size() !=
            setting.sinkAseConfiguration.value().size()) {
      LOG(INFO) << __func__ << ": sink ase, requirement and setting not matched: ";
      return std::nullopt;
    }

    if (requirement.sourceAseRequirement.has_value()) {
      LOG(INFO) << __func__ << ": source ase: requirement size: "
                              << requirement.sourceAseRequirement.value().size()
                              << ", setting size: "
                              << setting.sourceAseConfiguration.value().size();
    }

    if (requirement.sourceAseRequirement.has_value() &&
        requirement.sourceAseRequirement.value().size() !=
            setting.sourceAseConfiguration.value().size()) {
      LOG(INFO) << __func__ << ": source ase, requirement and setting not matched: ";
      return std::nullopt;
    }
    if(pts_gmap && (!requirement.sinkAseRequirement.has_value() &&
                        setting.sinkAseConfiguration.value().size() > 0)) {
      LOG(INFO) << __func__
                << ": GMAP is enabled, if no sink ase in requirement,"
                << " then don't select config with sink ";
      return std::nullopt;
    }
    if(pts_gmap && (!requirement.sourceAseRequirement.has_value() &&
                        setting.sourceAseConfiguration.value().size() > 0)) {
      LOG(INFO) << __func__
                << ": GMAP is enabled, if no source ase in requirement,"
                << " then don't select config with source ";
      return std::nullopt;
    }
    if(pts_gmap && (setting.sinkAseConfiguration.value().size() <= 0 &&
                        requirement.sinkAseRequirement.has_value())) {
      LOG(INFO) << __func__
                << ": GMAP is enabled, if no sink ase in setting,"
                << " then don't match with requirement with sink ";
      return std::nullopt;
    }
    if(pts_gmap && (setting.sourceAseConfiguration.value().size() <= 0 &&
                        requirement.sourceAseRequirement.has_value())) {
      LOG(INFO) << __func__
                << ": GMAP is enabled, if no source ase in setting,"
                << " then don't match with requirement with source ";
      return std::nullopt;
    }
  }

  if (requirement.sinkAseRequirement.has_value()) {
    LOG(INFO) << __func__ << ": sink ase requirement exist, so filter now";
    filterRequirementAseDirectionConfiguration(setting.sinkAseConfiguration,
                                               requirement.sinkAseRequirement.value(),
                                               filtered_setting.sinkAseConfiguration,
                                               isExact, filtered_setting.flags,
                                               kLeAudioDirectionSink,
                                               requirement.audioContext);
    if (!filtered_setting.sinkAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filterd setting of sink ase requirement and setting. ";
      return std::nullopt;
    }
  }

  if (requirement.sourceAseRequirement.has_value()) {
    LOG(INFO) << __func__ << ": source ase requirement exist, so filter now";
    filterRequirementAseDirectionConfiguration(setting.sourceAseConfiguration,
                                               requirement.sourceAseRequirement.value(),
                                               filtered_setting.sourceAseConfiguration,
                                               isExact, filtered_setting.flags,
                                               kLeAudioDirectionSource,
                                               requirement.audioContext);
    if (!filtered_setting.sourceAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filterd setting of source ase requirement and setting. ";
      return std::nullopt;
    }
  }

  return filtered_setting;
}

std::optional<std::pair<
    std::string, IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>
LeAudioOffloadAudioProvider::matchWithRequirement(
    std::vector<std::pair<
        std::string, IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>&
        matched_ase_configuration_settings,
    const IBluetoothAudioProvider::LeAudioConfigurationRequirement& requirement,
    bool isMatchContext, bool isExact, bool isMatchFlags) {

  LOG(INFO) << __func__ << ": Trying to match for the requirement "
            << requirement.toString();
  LOG(INFO) << __func__ << ": matched_ase_configuration_settings size "
            << matched_ase_configuration_settings.size();
  LOG(INFO) << __func__ << ": isMatchFlags: " << isMatchFlags
                        << ", isMatchContext: " << isMatchContext
                        << ", isExact: " << isExact;
  // Don't have to match with flag if requirements don't have flags.
  auto requirement_flags_bitmask = 0;
  if (isMatchFlags) {
    if (!requirement.flags.has_value()) {
      LOG(DEBUG) << __func__ << ": requirement configflags didn't set, return null.";
      return std::nullopt;
    }
    requirement_flags_bitmask = requirement.flags.value().bitmask;
  }
  for (auto& [setting_name, setting] : matched_ase_configuration_settings) {
    // Try to match context in metadata.
    if (isMatchContext) {
      LOG(WARNING) << __func__
                   << ": setting audio context bitmask: " << setting.audioContext.bitmask
                   << ", requirement audio context bitmask: " << requirement.audioContext.bitmask;
      if ((setting.audioContext.bitmask & requirement.audioContext.bitmask) !=
          requirement.audioContext.bitmask) {
        LOG(DEBUG) << __func__ << ": Setting and requirement context not matched, continue";
        continue;
      }
      LOG(DEBUG) << __func__
                 << ": Setting with matched context: name: " << setting_name
                 << ", setting: " << setting.toString();
    }

    // Try to match configuration flags
    if (isMatchFlags) {
      if (!setting.flags.has_value()) {
        LOG(DEBUG) << __func__ << ": Settings configflags didn't set, continue.";
        continue;
      }

      LOG(WARNING) << __func__
                   << ": setting audio context bitmask: " << setting.flags.value().bitmask
                   << ", requirement flags bitmask: " << requirement_flags_bitmask;
      if ((setting.flags.value().bitmask & requirement_flags_bitmask) !=
                                                   requirement_flags_bitmask) {
        LOG(DEBUG) << __func__
                   << ": Setting and requirement configFlags not matched, continue";
        continue;
      }
      LOG(DEBUG) << __func__
                 << ": Setting with matched flags: name: " << setting_name
                 << ", setting: " << setting.toString();
    }

    auto filtered_ase_configuration_setting =
        getRequirementMatchedAseConfigurationSettings(setting, requirement, isExact);
    if (filtered_ase_configuration_setting.has_value()) {
      LOG(INFO) << __func__ << ": Result found: name: " << setting_name
                << ", setting: "
                << filtered_ase_configuration_setting.value().toString();
      // Found a matched setting, ignore other settings
      return {{setting_name, filtered_ase_configuration_setting.value()}};
    }
  }
  // If cannot satisfy this requirement, return nullopt
  LOG(WARNING) << __func__ << ": Cannot match the requirement " << requirement.toString()
                           << ", match context = " << isMatchContext;
  return std::nullopt;
}

uint8_t convertAudioContextToUseCase(const AudioContext& choosed_audio_context) {
  LOG(DEBUG) << ": choosed audio_context: " << (unsigned)choosed_audio_context.bitmask;
  switch (choosed_audio_context.bitmask) {
    case AudioContext::UNSPECIFIED:
    case AudioContext::MEDIA:
    case AudioContext::INSTRUCTIONAL:
    case AudioContext::ALERTS:
    case AudioContext::SOUND_EFFECTS:
    case AudioContext::NOTIFICATIONS:
    case AudioContext::EMERGENCY_ALARM:
      return HQ_AUDIO_USE_CASE;
    case AudioContext::CONVERSATIONAL:
    case AudioContext::VOICE_ASSISTANTS:
    case AudioContext::RINGTONE_ALERTS:
      return VOICE_USE_CASE;
    case AudioContext::GAME:
      return GAMING_VBC_USE_CASE;
    case AudioContext::LIVE_AUDIO:
      return STEREO_REC_USE_CASE;
    default:
      return HQ_AUDIO_USE_CASE;
  }
}

std::vector<uint16_t> LeAudioOffloadAudioProvider::getUniqueCisHandles() {
  LOG(INFO) << __func__
            << ": cis_hndles size: "<< cis_hndles.size();
  std::set<uint16_t> s(cis_hndles.begin(), cis_hndles.end());
  cis_hndles.assign(s.begin(), s.end());
  LOG(INFO) << __func__
            << ": After removing duplicates cis_hndles size: "<< cis_hndles.size();
  return cis_hndles;
}

AudioContext LeAudioOffloadAudioProvider::getCurrentActiveContext() {
  LOG(INFO) << __func__
            << ": active_context: "<< active_context.toString();
  return active_context;
}

std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting>
LeAudioOffloadAudioProvider::getCachedAseConfigSetting(const AudioContext& context) {

  LOG(INFO) << __func__ << ": SessionType=" << toString(session_type_)
            << ": current_active_context: "<< context.toString();
  return context_to_config_map[context];
}

MetadataLtv::VendorSpecific LeAudioOffloadAudioProvider::
negotiatePacsAndCfgToSelectVendorSpecificMetadata(CodecId selected_codec,
               const MetadataLtv::VendorSpecific choosed_config_vendor_Metadata,
               const MetadataLtv::VendorSpecific pacs_vendor_Metadata) {
  //Below are for just ref for Aptx LE
  //cfg VendorSpecific{companyId: 10, opaqueValue: [17, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0]}
  //pac VendorSpecific{companyId: 10, opaqueValue: [11, 17, 0, 2, 40, 1, 0, 0, 0, 0, 0, 0]}
  std::vector<uint8_t> vendor_metadata = {};
  MetadataLtv::VendorSpecific temp = pacs_vendor_Metadata;
  if (selected_codec.getTag() == CodecId::Tag::vendor) {
    auto codec = selected_codec.get<CodecId::Tag::vendor>();
    LOG(DEBUG) << ": codec.id: " << codec.id;
    LOG(DEBUG) << ": codec.codecId: " << codec.codecId;
    auto dut_codec_version = choosed_config_vendor_Metadata.opaqueValue[2];
    auto peer_codec_version = pacs_vendor_Metadata.opaqueValue[3];
    auto negotiated_codec_version = 0;
    negotiated_codec_version = std::min(dut_codec_version, peer_codec_version);
    LOG(INFO) << __func__
              << ": dut_codec_version = " << unsigned(dut_codec_version)
              << ", peer_codec_version = " << unsigned(peer_codec_version)
              << ", negotiated_codec_version = " << unsigned(negotiated_codec_version);
    temp.opaqueValue[3] = negotiated_codec_version;
  } else {
    LOG(DEBUG) << ": Choosing LC3Q negotiated encoder and decoder versions.";
    auto dut_encoder_version = choosed_config_vendor_Metadata.opaqueValue[1];
    auto dut_decoder_version = choosed_config_vendor_Metadata.opaqueValue[2];
    auto peer_encoder_version = pacs_vendor_Metadata.opaqueValue[2];
    auto peer_decoder_version = pacs_vendor_Metadata.opaqueValue[3];
    auto negotiated_encoder_version = 0, negotiated_decoder_version = 0;
    negotiated_encoder_version = std::min(dut_encoder_version, peer_decoder_version);
    negotiated_decoder_version = std::min(dut_decoder_version, peer_encoder_version);
    LOG(INFO) << __func__
              << ": dut_encoder_version = " << unsigned(dut_encoder_version)
              << ", dut_decoder_version = " << unsigned(dut_decoder_version)
              << ", peer_encoder_version = " << unsigned(peer_encoder_version)
              << ", peer_decoder_version = " << unsigned(peer_decoder_version)
              << ", negotiated_encoder_version = " << unsigned(negotiated_encoder_version)
              << ", negotiated_decoder_version = " << unsigned(negotiated_decoder_version);
    temp.opaqueValue[2] = negotiated_encoder_version;
    temp.opaqueValue[3] = negotiated_decoder_version;
  }
  return temp;
}

MetadataLtv::VendorSpecific LeAudioOffloadAudioProvider::
getVendorSpecificMetadataBySessionType(SessionType session_type_,
                                       const AudioConfiguration& audio_config) {
  LOG(INFO) << __func__ << ": audio_config tag: " << toString(audio_config.getTag());
  MetadataLtv::VendorSpecific cached_vendor_metadata = MetadataLtv::VendorSpecific();
  AudioContext streaming_audio_context = AudioContext();
  uint8_t direction = 0;
  if (audio_config.getTag() == AudioConfiguration::leAudioConfig) {
    auto &audioconfig =
                  audio_config.get<AudioConfiguration::leAudioConfig>();
    LOG(INFO) << __func__ << ": streamMap size: " << audioconfig.streamMap.size();
    for (int i = 0; i < audioconfig.streamMap.size(); ++i) {
      auto &le_audio_direction_ase_config = audioconfig.streamMap[i].aseConfiguration;
      if (le_audio_direction_ase_config.has_value() &&
              le_audio_direction_ase_config.value().metadata.has_value()) {
        for (auto &cfg_meta : le_audio_direction_ase_config.value().metadata.value()) {
          if (cfg_meta.value().getTag() == MetadataLtv::streamingAudioContexts) {
            streaming_audio_context =
               cfg_meta.value().get<MetadataLtv::Tag::streamingAudioContexts>().values;
          }
        }
      }
    }
  }

  //AudioContext current_active_context = getCurrentActiveContext();
  LOG(INFO) << __func__
            << ": streaming_audio_context: "<< streaming_audio_context.toString();

  if (session_type_ == SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    direction = kLeAudioDirectionSink;
  } else if (session_type_ == SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    direction = kLeAudioDirectionSource;
  }
  LOG(INFO) << __func__ << ": direction=" << (unsigned)direction;

  auto cached_active_config = getCachedAseConfigSetting(streaming_audio_context);
  for (auto& cfg : cached_active_config) {
    const MetadataLtv::VendorSpecific pacs_vendor_Metadata =
                                              directional_metadata_map[direction];
    cached_vendor_metadata =
           getValidVendorMetadataInCachedConfig(cfg, pacs_vendor_Metadata, direction);
  }
  return cached_vendor_metadata;
}

void LeAudioOffloadAudioProvider::checkMatchedPacsForChoosedConfig(
    const std::optional<std::vector<
        std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
        remoteSinkAudioCapabilities,
    const std::optional<std::vector<
        std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
        remoteSourceAudioCapabilities,
    IBluetoothAudioProvider::LeAudioAseConfigurationSetting& choosed_config) {

  if (remoteSinkAudioCapabilities.has_value()) {
    for (auto& capability : remoteSinkAudioCapabilities.value()) {
      if (!capability.has_value()) {
        LOG(INFO) << __func__ << ": Sink capability not exist, continue.";
        continue;
      }
      LOG(INFO) << __func__ << ": Sink capability: " << capability->toString();

      auto filtered_ase_configuration_setting =
                         getCapabilitiesMatchedAseConfigurationSettings(
                               choosed_config, capability.value(), kLeAudioDirectionSink);
      if (filtered_ase_configuration_setting.has_value()) {
        LOG(DEBUG) << __func__
                   << ": Caching pacs Vendor specific Metadata of choosed config."
                   << filtered_ase_configuration_setting.value().toString();
        cachePacsVendorMetadataMatchedWithChoosedConfigByDirection(
                                         kLeAudioDirectionSink, capability.value());
        break;
      }
    }
  }

  if (remoteSourceAudioCapabilities.has_value()) {
    for (auto& capability : remoteSourceAudioCapabilities.value()) {
      if (!capability.has_value()) {
        LOG(INFO) << __func__ << ": Source capability not exist, continue.";
        continue;
      }
      LOG(INFO) << __func__ << ": Source capability: " << capability->toString();

      auto filtered_ase_configuration_setting =
                         getCapabilitiesMatchedAseConfigurationSettings(
                               choosed_config, capability.value(), kLeAudioDirectionSource);
      if (filtered_ase_configuration_setting.has_value()) {
        LOG(DEBUG) << __func__
                   << ": Caching pacs Vendor specific Metadata of choosed config."
                   << filtered_ase_configuration_setting.value().toString();
        cachePacsVendorMetadataMatchedWithChoosedConfigByDirection(
                                     kLeAudioDirectionSource, capability.value());
        break;
      }
    }
  }
}

void LeAudioOffloadAudioProvider::
cachePacsVendorMetadataMatchedWithChoosedConfigByDirection(uint8_t direction,
        const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities) {
  LOG(INFO) << __func__ <<": direction: " << (unsigned)direction;
  if (capabilities.metadata.has_value()) {
    LOG(INFO) << __func__ <<": caps_metadata size: "
                          << capabilities.metadata.value().size();
    for (auto caps_meta : capabilities.metadata.value()) {
      MetadataLtv::VendorSpecific caps_vendor_metadata = MetadataLtv::VendorSpecific();
      directional_metadata_map[direction] = {};
      LOG(INFO) << __func__
                << ": caps_metadata Tag: " << toString(caps_meta.value().getTag());
      if (caps_meta.value().getTag() == MetadataLtv::Tag::vendorSpecific) {
        caps_vendor_metadata = caps_meta.value().get<MetadataLtv::Tag::vendorSpecific>();
        LOG(INFO) << __func__
                << ": caps_vendor_metadata.companyId: "<< caps_vendor_metadata.companyId;
        directional_metadata_map[direction] = caps_vendor_metadata;
        LOG(INFO) << __func__
                << ": cached vendor metadata, return";
        return;
      }
    }
  }
}

uint8_t LeAudioOffloadAudioProvider::getSelectedConfigCodecPriority(
  IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting, uint8_t direction) {

  uint8_t priority = 0;

  // Create a new LeAudioAseConfigurationSetting and return
  LeAudioAseConfigurationSetting config {
      .audioContext = setting.audioContext,
      .sinkAseConfiguration = setting.sinkAseConfiguration,
      .sourceAseConfiguration = setting.sourceAseConfiguration,
      .flags = setting.flags,
      .packing = setting.packing,
  };

  // Get a list of all matched AseDirectionConfiguration
  // for the input direction
  std::vector<std::optional<AseDirectionConfiguration>>* direction_configurations = nullptr;
  if (direction == kLeAudioDirectionSink) {
    if (!config.sinkAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sinkAseConfiguration available";
      return priority;
    }
    direction_configurations = &config.sinkAseConfiguration.value();
  } else {
    if (!config.sourceAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sourceAseConfiguration available";
      return priority;
    }
    direction_configurations = &config.sourceAseConfiguration.value();
  }

  LOG(INFO) << __func__ << ": direction_configurations size: "
                        << direction_configurations->size();

  for (auto direction_configuration : *direction_configurations) {
    if (!direction_configuration.has_value()) {
      LOG(INFO) << __func__ << ": There is no direction_configuration, continue";
      continue;
    }

    if (!direction_configuration.value().aseConfiguration.codecId.has_value()) {
      LOG(INFO) << __func__ << ": There is no valid codec in direction_configuration, continue";
      continue;
    }

    priority = getLocalCodecPriority(
                    direction_configuration.value().aseConfiguration.codecId.value(),
                    setting.audioContext);
  }

  LOG(INFO) << __func__ << ": priority= " << (unsigned)priority;
  return priority;
}

uint8_t LeAudioOffloadAudioProvider::getLocalCodecPriority(CodecId cfg_codec,
                                                     const AudioContext& context) {
  LOG(INFO) << __func__
            << ": cfg_codec Tag: " << toString(cfg_codec.getTag());
  uint8_t mask = 0;
  if (cfg_codec.getTag() == CodecId::Tag::vendor) {
    auto vc_cfg_codec = cfg_codec.get<CodecId::Tag::vendor>();

    if (vc_cfg_codec.codecId == kLeAudioCodingFormatAptxLeX) {
       mask = context_to_matched_codecId[context] & kCodingFormatAptxLeX;
    }

    if (vc_cfg_codec.codecId == kLeAudioCodingFormatAptxLe) {
      mask = context_to_matched_codecId[context] & kCodingFormatAptxLe;
    }
  } else if (cfg_codec.getTag() == CodecId::Tag::core) {
     mask = context_to_matched_codecId[context] & kCodingFormatLc3;
  }

  LOG(INFO) << __func__ << ": mask: " << (unsigned)mask;
  return mask;
}

MetadataLtv::VendorSpecific LeAudioOffloadAudioProvider::
getValidVendorMetadataInCachedConfig(
  IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
  const MetadataLtv::VendorSpecific pacs_vendor_Metadata, uint8_t direction) {

  MetadataLtv::VendorSpecific cfg_vendor_metadata = MetadataLtv::VendorSpecific();

  // Create a new LeAudioAseConfigurationSetting and return
  LeAudioAseConfigurationSetting choosed_config{
      .audioContext = setting.audioContext,
      .sinkAseConfiguration = setting.sinkAseConfiguration,
      .sourceAseConfiguration = setting.sourceAseConfiguration,
      .flags = setting.flags,
      .packing = setting.packing,
  };

  // Get a list of all matched AseDirectionConfiguration
  // for the input direction
  std::vector<std::optional<AseDirectionConfiguration>>* direction_configurations = nullptr;
  if (direction == kLeAudioDirectionSink) {
    if (!choosed_config.sinkAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sinkAseConfiguration available";
      return cfg_vendor_metadata;
    }
    direction_configurations = &choosed_config.sinkAseConfiguration.value();
  } else {
    if (!choosed_config.sourceAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sourceAseConfiguration available";
      return cfg_vendor_metadata;
    }
    direction_configurations = &choosed_config.sourceAseConfiguration.value();
  }

  LOG(INFO) << __func__ << ": direction_configurations size: "
                        << direction_configurations->size();

  for (auto direction_configuration : *direction_configurations) {
    bool is_chosen_config_has_vendor_metadata = false;
    if (!direction_configuration.has_value()) {
      LOG(INFO) << __func__ << ": There is no direction_configuration, continue";
      continue;
    }

    if (!direction_configuration.value().aseConfiguration.codecId.has_value()) {
      LOG(INFO) << __func__ << ": There is no valid codec in direction_configuration, continue";
      continue;
    }

    if (direction_configuration.value().aseConfiguration.metadata.has_value()) {
      for (auto &cfg_meta : direction_configuration.value().aseConfiguration.metadata.value()) {
        if (cfg_meta.value().getTag() == MetadataLtv::vendorSpecific) {
          cfg_vendor_metadata = cfg_meta.value().get<MetadataLtv::vendorSpecific>();
          LOG(INFO) << __func__
                << ": cfg_vendor_metadata.companyId: "<< cfg_vendor_metadata.companyId;
          if (cfg_vendor_metadata.companyId != 0) {
            //No need to go for next vendorSpecific metadataLtv,
            //as it has  config name only to debug
            LOG(INFO) << __func__
                      << ": valid vendor codec specific Metadata exist.";
            is_chosen_config_has_vendor_metadata = true;
            break;
          }
        }
      }
    }
    if (is_chosen_config_has_vendor_metadata) {
      cfg_vendor_metadata = negotiatePacsAndCfgToSelectVendorSpecificMetadata(
                       direction_configuration.value().aseConfiguration.codecId.value(),
                       cfg_vendor_metadata, pacs_vendor_Metadata);
      return cfg_vendor_metadata;
    }
  }
  return cfg_vendor_metadata;
}

bool LeAudioOffloadAudioProvider::IsValidVendorMetadataExistInChoosedConfig(
      IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting, uint8_t direction) {

  // Create a new LeAudioAseConfigurationSetting and return
  LeAudioAseConfigurationSetting choosed_config{
      .audioContext = setting.audioContext,
      .sinkAseConfiguration = setting.sinkAseConfiguration,
      .sourceAseConfiguration = setting.sourceAseConfiguration,
      .flags = setting.flags,
      .packing = setting.packing,
  };

  // Get a list of all matched AseDirectionConfiguration
  // for the input direction
  std::vector<std::optional<AseDirectionConfiguration>>* direction_configurations = nullptr;
  if (direction == kLeAudioDirectionSink) {
    if (!choosed_config.sinkAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sinkAseConfiguration available";
      return false;
    }
    direction_configurations = &choosed_config.sinkAseConfiguration.value();
  } else {
    if (!choosed_config.sourceAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sourceAseConfiguration available";
      return false;
    }
    direction_configurations = &choosed_config.sourceAseConfiguration.value();
  }

  LOG(INFO) << __func__ << ": direction_configurations size: "
                        << direction_configurations->size();

  for (auto direction_configuration : *direction_configurations) {
    bool is_chosen_config_has_vendor_metadata = false;
    if (!direction_configuration.has_value()) {
      LOG(INFO) << __func__ << ": There is no direction_configuration, continue";
      continue;
    }

    if (!direction_configuration.value().aseConfiguration.codecId.has_value()) {
      LOG(INFO) << __func__ << ": There is no valid codec in direction_configuration, continue";
      continue;
    }

    if (direction_configuration.value().aseConfiguration.metadata.has_value()) {
      for (auto &cfg_meta : direction_configuration.value().aseConfiguration.metadata.value()) {
        if (cfg_meta.value().getTag() == MetadataLtv::vendorSpecific) {
          auto cfg_vendor_metadata = cfg_meta.value().get<MetadataLtv::vendorSpecific>();
          LOG(INFO) << __func__
                << ": cfg_vendor_metadata.companyId: "<< cfg_vendor_metadata.companyId;
          if (cfg_vendor_metadata.companyId != 0) {
            //No need to go for next vendorSpecific metadataLtv,
            //as it has  config name only to debug
            LOG(INFO) << __func__
                      << ": valid vendor codec specific Metadata exist.";
            is_chosen_config_has_vendor_metadata = true;
            break;
          }
        }
      }
    }
    if (is_chosen_config_has_vendor_metadata) {
      return true;
    }
  }
  return false;
}

bool LeAudioOffloadAudioProvider::IsDualAckSupportedConfigSelected(
      IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting, uint8_t direction) {

  // Create a new LeAudioAseConfigurationSetting and return
  LeAudioAseConfigurationSetting choosed_config{
      .audioContext = setting.audioContext,
      .sinkAseConfiguration = setting.sinkAseConfiguration,
      .sourceAseConfiguration = setting.sourceAseConfiguration,
      .flags = setting.flags,
      .packing = setting.packing,
  };

  // Get a list of all matched AseDirectionConfiguration
  // for the input direction
  std::vector<std::optional<AseDirectionConfiguration>>* direction_configurations = nullptr;
  if (direction == kLeAudioDirectionSink) {
    if (!choosed_config.sinkAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sinkAseConfiguration available";
      return false;
    }
    direction_configurations = &choosed_config.sinkAseConfiguration.value();
  } else {
    if (!choosed_config.sourceAseConfiguration.has_value()) {
      LOG(INFO) << __func__ << ": No filtered_setting for sourceAseConfiguration available";
      return false;
    }
    direction_configurations = &choosed_config.sourceAseConfiguration.value();
  }

  LOG(INFO) << __func__ << ": direction_configurations size: "
                        << direction_configurations->size();

  for (auto direction_configuration : *direction_configurations) {
    bool is_dual_ack_config_selected = false;
    if (!direction_configuration.has_value()) {
      LOG(INFO) << __func__ << ": There is no direction_configuration, continue";
      continue;
    }

    if (!direction_configuration.value().aseConfiguration.codecId.has_value()) {
      LOG(INFO) << __func__ << ": There is no valid codec in direction_configuration, continue";
      continue;
    }

    if (direction_configuration.value().aseConfiguration.metadata.has_value()) {
      for (auto &cfg_meta : direction_configuration.value().aseConfiguration.metadata.value()) {
        if (cfg_meta.value().getTag() == MetadataLtv::vendorSpecific) {
          auto cfg_vendor_metadata = cfg_meta.value().get<MetadataLtv::vendorSpecific>();
          LOG(INFO) << __func__
                << ": cfg_vendor_metadata.companyId: "<< cfg_vendor_metadata.companyId
                << ", cfg_vendor_metadata.opaqueValue[7]: " << cfg_vendor_metadata.opaqueValue[7];
          if (cfg_vendor_metadata.companyId != 0 &&
              cfg_vendor_metadata.opaqueValue[7] == kLeAudioCodecAptxLeSupportedDualAck) {
            LOG(INFO) << __func__ << ": dual ack config has been selected";
            is_dual_ack_config_selected = true;
            break;
          }
        }
      }
    }
    if (is_dual_ack_config_selected) {
      return true;
    }
  }
  return false;
}

std::vector<uint8_t> LeAudioOffloadAudioProvider::
negotiateAndSelectVendorCodecVersion(CodecId choosed_codec,
               const MetadataLtv::VendorSpecific choosed_config_vendor_Metadata,
               const MetadataLtv::VendorSpecific pacs_vendor_Metadata) {
  //Below are for just ref for Aptx LE
  //cfg VendorSpecific{companyId: 10, opaqueValue: [17, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0]}
  //pac VendorSpecific{companyId: 10, opaqueValue: [11, 17, 0, 2, 40, 1, 0, 0, 0, 0, 0, 0]}
  std::vector<uint8_t> vendor_metadata = {};
  if (choosed_codec.getTag() == CodecId::Tag::vendor) {
    auto codec = choosed_codec.get<CodecId::Tag::vendor>();
    LOG(DEBUG) << ": codec.id: " << codec.id;
    LOG(DEBUG) << ": codec.codecId: " << codec.codecId;
    auto dut_codec_version = choosed_config_vendor_Metadata.opaqueValue[2];
    auto peer_codec_version = pacs_vendor_Metadata.opaqueValue[3];
    auto negotiated_codec_version = 0;
    negotiated_codec_version = std::min(dut_codec_version, peer_codec_version);
    LOG(INFO) << __func__
              << ": dut_codec_version = " << unsigned(dut_codec_version)
              << ", peer_codec_version = " << unsigned(peer_codec_version)
              << ", negotiated_codec_version = " << unsigned(negotiated_codec_version);
    vendor_metadata.push_back(negotiated_codec_version);
  } else {
    LOG(DEBUG) << ": Choosing LC3Q negotiated encoder and decoder versions.";
    auto dut_encoder_version = choosed_config_vendor_Metadata.opaqueValue[1];
    auto dut_decoder_version = choosed_config_vendor_Metadata.opaqueValue[2];
    auto peer_encoder_version = pacs_vendor_Metadata.opaqueValue[2];
    auto peer_decoder_version = pacs_vendor_Metadata.opaqueValue[3];
    auto negotiated_encoder_version = 0, negotiated_decoder_version = 0;
    negotiated_encoder_version = std::min(dut_encoder_version, peer_decoder_version);
    negotiated_decoder_version = std::min(dut_decoder_version, peer_encoder_version);
    LOG(INFO) << __func__ << ": dut_encoder_version = " << unsigned(dut_encoder_version)
                          << ", dut_decoder_version = " << unsigned(dut_decoder_version)
                          << ", peer_encoder_version = " << unsigned(peer_encoder_version)
                          << ", peer_decoder_version = " << unsigned(peer_decoder_version)
                          << ", negotiated_encoder_version = " << unsigned(negotiated_encoder_version)
                          << ", negotiated_decoder_version = " << unsigned(negotiated_decoder_version);
    vendor_metadata.push_back(negotiated_encoder_version);
    vendor_metadata.push_back(negotiated_decoder_version);
  }
  return vendor_metadata;
}

void LeAudioOffloadAudioProvider::populateVendorConfigDataPathPayload(
                          LeAudioAseConfiguration& choosed_cfg,
                          const LeAudioConfiguration::StreamMap& stream_map,
                          const AudioContext& Choosed_context,
                          std::optional<LeAudioDataPathConfiguration>& data_path,
                          uint8_t direction) {

  if (choosed_cfg.metadata.has_value()) {
    auto choosed_codec_vendor_metadata = MetadataLtv::VendorSpecific();
    bool is_choosed_cfg_has_valid_vendor_metadata = false;

    for (auto metadata : choosed_cfg.metadata.value()) {
      LOG(DEBUG) << __func__ << ": choosed config metadata size: "
                              << choosed_cfg.metadata.value().size();
      if (!metadata.has_value()) {
        LOG(ERROR) << __func__ << ": Metadata not exist., continue";
        continue;
      }

      LOG(DEBUG) << __func__ << ": choosed config metadata tag: "
                             << toString(metadata.value().getTag());
      if (metadata.value().getTag() == MetadataLtv::Tag::vendorSpecific) {
        choosed_codec_vendor_metadata =
                              metadata.value().get<MetadataLtv::vendorSpecific>();
        LOG(INFO) << __func__ << ": cfg_vendor_metadata.companyId: "
                              << choosed_codec_vendor_metadata.companyId;

        if (choosed_codec_vendor_metadata.companyId != 0) {
          is_choosed_cfg_has_valid_vendor_metadata = true;
          LOG(DEBUG) << ": Valid Metadata exist, proceed to populate vendorConfig.";
          break;
        }
      }
    }

    LOG(INFO) << __func__ << ": is_choosed_cfg_has_valid_vendor_metadata = "
                          << is_choosed_cfg_has_valid_vendor_metadata;
    CodecSpecificConfigurationLtv::SamplingFrequency choosed_cfg_sampling_freq =
                                    CodecSpecificConfigurationLtv::SamplingFrequency();
    if (is_choosed_cfg_has_valid_vendor_metadata) {
      for (auto codec_cfg : choosed_cfg.codecConfiguration) {
        LOG(DEBUG) << ": choosed config codec_config tag: " << toString(codec_cfg.getTag());
        if (codec_cfg.getTag() ==
              CodecSpecificConfigurationLtv::Tag::samplingFrequency) {
          choosed_cfg_sampling_freq =
                codec_cfg.get<CodecSpecificConfigurationLtv::samplingFrequency>();
          LOG(DEBUG) << ": choosed_cfg_sampling_freq: "
                     << (unsigned)choosed_cfg_sampling_freq;
        }
      }
    }

    //LeAudioDataPathConfiguration dataP = {};
    std::vector<uint8_t> configure_datapath = {};
    std::vector<uint8_t> vendor_negotated_version = {};
    bool is_aptx_le_dual_ack_supported = false;
    // Populate codec version number
    uint8_t codec_ver = 0;
    uint8_t len = LTV_LEN_VER_NUM;
    uint8_t type = LTV_TYPE_VER_NUM;
    //codec_ver = 2;//making hardcode as of now.

    LOG(INFO) << __func__ << ": direction = " << unsigned(direction);
    const MetadataLtv::VendorSpecific pacs_vendor_Metadata = directional_metadata_map[direction];
    vendor_negotated_version = negotiateAndSelectVendorCodecVersion(choosed_cfg.codecId.value(),
                                      choosed_codec_vendor_metadata, pacs_vendor_Metadata);

    uint8_t len_ = vendor_negotated_version.size();
    LOG(INFO) << __func__ << ": len_ = " << unsigned(len_);
    for (uint8_t i = 0; i < vendor_negotated_version.size(); i++) {
      LOG(INFO) << __func__ << ": vendor_negotated_version[" << unsigned(i)
                            << " ] = " << unsigned(vendor_negotated_version[i]);
    }

    if (vendor_negotated_version.empty()) {
      LOG(INFO) << __func__ << ": empty vendor_negotated_version";
      codec_ver = 0;
    } else {
      if (choosed_cfg.codecId.has_value()) {
        LOG(INFO) << __func__ << ": choosed_cfg Tag: "
                              << toString(choosed_cfg.codecId.value().getTag());
        if (choosed_cfg.codecId.value().getTag() == CodecId::Tag::vendor) {
          auto choosed_vc_cfg_codec = choosed_cfg.codecId.value().get<CodecId::Tag::vendor>();
          LOG(DEBUG) << ": chosen_vc_cfg_codec.id: " << choosed_vc_cfg_codec.id;
          LOG(DEBUG) << ": chosen_vc_cfg_codec.codecId: " << choosed_vc_cfg_codec.codecId;
          LOG(DEBUG) << ": pacs mps support: "
                     << (unsigned)pacs_vendor_Metadata.opaqueValue[7];
          codec_ver = vendor_negotated_version[0] & 0x0F;
          if (choosed_vc_cfg_codec.codecId == kLeAudioCodingFormatAptxLe &&
              pacs_vendor_Metadata.opaqueValue[7] == kLeAudioCodecAptxLeSupportedDualAck) {
            is_aptx_le_dual_ack_supported = true;
          }
        } else {
          if (direction == kLeAudioDirectionSink) {
            codec_ver = vendor_negotated_version[0];
          } else {
            codec_ver = vendor_negotated_version[1];
          }
        }
      }
    }
    LOG(INFO) << __func__ << ": codec_ver = " << unsigned(codec_ver);
    configure_datapath.insert(configure_datapath.end(), &len, &len + 1);
    configure_datapath.insert(configure_datapath.end(), &type, &type + 1);
    configure_datapath.insert(configure_datapath.end(), &codec_ver, &codec_ver + 1);

    len = LTV_LEN_FREQ;
    type = LTV_TYPE_FREQ;
    configure_datapath.insert(configure_datapath.end(), &len, &len + 1);
    configure_datapath.insert(configure_datapath.end(), &type, &type + 1);
    configure_datapath.insert(configure_datapath.end(), (uint8_t*)&choosed_cfg_sampling_freq,
                                                      (uint8_t*)&choosed_cfg_sampling_freq+ 1);

    // Populate usecase number
    uint8_t usecase = HQ_AUDIO_USE_CASE;
    len = LTV_LEN_USE_CASE;
    type = LTV_TYPE_USE_CASE;
    usecase = convertAudioContextToUseCase(Choosed_context);
    LOG(INFO) << __func__ << ": usecase: " << (unsigned)usecase;

    configure_datapath.insert(configure_datapath.end(), &len, &len + 1);
    configure_datapath.insert(configure_datapath.end(), &type, &type + 1);
    configure_datapath.insert(configure_datapath.end(), &usecase, &usecase + 1);

    // Populated codec ID
    uint16_t vendor_company_id = 0;
    uint16_t vendor_codec_id = 0;
    uint8_t coding_format = 0;
    if (choosed_cfg.codecId.has_value()) {
      LOG(INFO) << __func__ << ": choosed_cfg Tag: "
                            << toString(choosed_cfg.codecId.value().getTag());
      if (choosed_cfg.codecId.value().getTag() == CodecId::Tag::vendor) {
        auto choosed_vc_cfg_codec = choosed_cfg.codecId.value().get<CodecId::Tag::vendor>();
        LOG(DEBUG) << ": chosen_vc_cfg_codec.id: " << choosed_vc_cfg_codec.id;
        LOG(DEBUG) << ": chosen_vc_cfg_codec.codecId: " << choosed_vc_cfg_codec.codecId;
        coding_format = kIsoCodingFormatVendorSpecific;
        vendor_company_id = choosed_vc_cfg_codec.id;
        vendor_codec_id = choosed_vc_cfg_codec.codecId;
      } else {
        coding_format = (uint8_t)CodecId::Core::LC3;
      }
    }

    len = LTV_LEN_CODEC_ID;
    type = LTV_TYPE_CODEC_ID;
    configure_datapath.insert(configure_datapath.end(), &len, &len + 1);
    configure_datapath.insert(configure_datapath.end(), &type, &type + 1);
    configure_datapath.insert(configure_datapath.end(), &coding_format,
                                                           &coding_format + 1);
    configure_datapath.insert(configure_datapath.end(), vendor_company_id);
    configure_datapath.insert(configure_datapath.end(), vendor_company_id >> 8);
    configure_datapath.insert(configure_datapath.end(), vendor_codec_id);
    configure_datapath.insert(configure_datapath.end(), vendor_codec_id >> 8);


    // Populate connection handle
    uint16_t connection_handle = stream_map.streamHandle;
    LOG(INFO) << __func__ << ": connection_handle: " << (unsigned)connection_handle;
    len = LTV_LEN_CONN_HANDLE;
    type = LTV_TYPE_CONN_HANDLE;
    configure_datapath.insert(configure_datapath.end(), &len, &len + 1);
    configure_datapath.insert(configure_datapath.end(), &type, &type + 1);
    configure_datapath.insert(configure_datapath.end(), connection_handle);
    configure_datapath.insert(configure_datapath.end(), connection_handle >> 8);
    cis_hndles.push_back(connection_handle);
    //dataP.dataPathConfiguration.configuration =
    //      std::vector<uint8_t>(configure_datapath.begin(), configure_datapath.end());
    //data_path.value().dataPathConfiguration.configuration =
    //                       {dataP.dataPathConfiguration.configuration};

    //Populate mpc(multi peripheral cis(dual ack)) info.
    if (is_aptx_le_dual_ack_supported) {
      len = LTV_LEN_MPC;
      type = LTV_TYPE_MPC;
      uint16_t mpc_enable = kLeAudioCodecAptxLeMpcEnable;
      configure_datapath.insert(configure_datapath.end(), &len, &len + 1);
      configure_datapath.insert(configure_datapath.end(), &type, &type + 1);
      configure_datapath.insert(configure_datapath.end(), mpc_enable);
      configure_datapath.insert(configure_datapath.end(), mpc_enable >> 8);
    }

    data_path->dataPathConfiguration.configuration =
             std::vector<uint8_t>(configure_datapath.begin(), configure_datapath.end());

  }
}

ndk::ScopedAStatus LeAudioOffloadAudioProvider::getLeAudioAseDatapathConfiguration(
    const std::optional<IBluetoothAudioProvider::StreamConfig>& in_sinkConfig,
    const std::optional<IBluetoothAudioProvider::StreamConfig>& in_sourceConfig,
    IBluetoothAudioProvider::LeAudioDataPathConfigurationPair* _aidl_return) {

  LOG(DEBUG) << __func__ << ": getLeAudioAseDatapathConfiguration";
  if (!in_sinkConfig.has_value() && !in_sourceConfig.has_value()) {
    LOG(ERROR) << __func__ << ": Sink and Source config doesn't exist, return.";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }
  //LOG(INFO) << __func__ << ": in_sinkConfig size: " << in_sinkConfig.value().size();
  bool valid_sink_vendor_metadata_found = false;
  bool valid_source_vendor_metadata_found = false;
  IBluetoothAudioProvider::LeAudioDataPathConfigurationPair result;
  cis_hndles = {};
  if (in_sinkConfig.has_value()) {
    LOG(INFO) << __func__ << ": sink audioContext: "
                          << in_sinkConfig.value().audioContext.bitmask;
    auto chosen_config = context_to_config_map[in_sinkConfig.value().audioContext];
    LOG(INFO) << __func__ << ": chosen_config size: " << chosen_config.size();

    auto sink_streamMap = in_sinkConfig.value().streamMap;
    LOG(INFO) << __func__ << ": sink_streamMap size: " << sink_streamMap.size();
    for (auto& cfg : chosen_config) {
      valid_sink_vendor_metadata_found = false;
      valid_sink_vendor_metadata_found =
          IsValidVendorMetadataExistInChoosedConfig(cfg, kLeAudioDirectionSink);
      LOG(INFO) << __func__ << ": valid_sink_vendor_metadata_found: "
                            << valid_sink_vendor_metadata_found;
      if (valid_sink_vendor_metadata_found) {
        for (int i = 0; i < sink_streamMap.size(); ++i) {
          auto sink_stream_map = sink_streamMap[i];
          auto direction_configuration = cfg.sinkAseConfiguration.value()[i];
          LOG(INFO) << __func__ << ": direction_configuration: "
                                << direction_configuration->toString();

          if (!direction_configuration.has_value()) {
            LOG(INFO) << __func__ << ": No config for this direction, return";
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
          }

          //auto data_path_config =
          //       direction_configuration.value().dataPathConfiguration.value();
          std::optional<LeAudioDataPathConfiguration> sink_data_path_config =
                      direction_configuration.value().dataPathConfiguration.value();

          auto sink_ase_cfg = direction_configuration.value();
          populateVendorConfigDataPathPayload(sink_ase_cfg.aseConfiguration,
                                              sink_stream_map,
                                              in_sinkConfig->audioContext,
                                              sink_data_path_config,
                                              kLeAudioDirectionSink
                                              /*sink_ase_cfg.dataPathConfiguration*/);

          //result.inputConfig = sink_ase_cfg.dataPathConfiguration;
          //Fill into sink_ase_cfg as well, will check if it is really needed.
          sink_ase_cfg.dataPathConfiguration = sink_data_path_config;

          //overwriting with latest.
          result.inputConfig = sink_data_path_config;
        }
      } else {
        LOG(INFO) << __func__ << ": in sink direction LC3 codec selected.";
        for (int i = 0; i < sink_streamMap.size(); ++i) {
          auto sink_stream_map = sink_streamMap[i];
          cis_hndles.push_back(sink_stream_map.streamHandle);
        }
      }
    }
  } else {
    LOG(INFO) << __func__ << ": in_sinkConfig doesn't exist.";
  }

  //LOG(INFO) << __func__ << ": in_sourceConfig size: " << in_sourceConfig.value().size();
  if (in_sourceConfig.has_value()) {
    LOG(INFO) << __func__ << ": source audioContext: "
                          << in_sourceConfig.value().audioContext.bitmask;
    auto chosen_config = context_to_config_map[in_sourceConfig.value().audioContext];
    LOG(INFO) << __func__ << ": chosen_config size: " << chosen_config.size();

    auto source_streamMap = in_sourceConfig.value().streamMap;
    LOG(INFO) << __func__ << ": source_streamMap size: " << source_streamMap.size();
    for (auto& cfg : chosen_config) {
      valid_source_vendor_metadata_found = false;
      valid_source_vendor_metadata_found =
          IsValidVendorMetadataExistInChoosedConfig(cfg, kLeAudioDirectionSource);
      LOG(INFO) << __func__ << ": valid_source_vendor_metadata_found: "
                            << valid_source_vendor_metadata_found;
      if (valid_source_vendor_metadata_found) {
        for (int i = 0; i < source_streamMap.size(); ++i) {
          auto source_stream_map = source_streamMap[i];
          auto direction_configuration = cfg.sourceAseConfiguration.value()[i];
          LOG(INFO) << __func__ << ": direction_configuration: "
                                << direction_configuration->toString();

          if (!direction_configuration.has_value()) {
            LOG(INFO) << __func__ << ": No config for this direction, return";
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
          }

          //auto data_path_config =
          //       direction_configuration.value().dataPathConfiguration.value();
          std::optional<LeAudioDataPathConfiguration> source_data_path_config =
                      direction_configuration.value().dataPathConfiguration.value();

          auto source_ase_cfg = direction_configuration.value();
          populateVendorConfigDataPathPayload(source_ase_cfg.aseConfiguration,
                                              source_stream_map,
                                              in_sourceConfig->audioContext,
                                              source_data_path_config,
                                              kLeAudioDirectionSource
                                              /*source_ase_cfg.dataPathConfiguration*/);

          //result.outputConfig = source_ase_cfg.dataPathConfiguration;
          //Fill into source_ase_cfg as well, will check if it is really needed.
          source_ase_cfg.dataPathConfiguration = source_data_path_config;

          //overwriting with latest.
          result.outputConfig = source_data_path_config;
        }
      } else {
        LOG(INFO) << __func__ << ": in source direction LC3 codec selected.";
        for (int i = 0; i < source_streamMap.size(); ++i) {
          auto source_stream_map = source_streamMap[i];
          cis_hndles.push_back(source_stream_map.streamHandle);
        }
      }
    }
  } else {
    LOG(INFO) << __func__ << ": in_sourceConfig doesn't exist.";
  }

  LOG(INFO) << __func__ << ": " << result.toString();

  LOG(INFO) << __func__ << " for SessionType=" << toString(session_type_);
  std::vector<uint16_t> unique_cis_hndles_ = getUniqueCisHandles();
  BluetoothAudioSessionReport::ReportUniqueCishandles(session_type_, unique_cis_hndles_);


  if(in_sinkConfig.has_value() && !valid_sink_vendor_metadata_found) {
    LOG(INFO) << __func__ << " return as unsupported for LC3 snk";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
  }
  if(in_sourceConfig.has_value() &&
     !valid_sink_vendor_metadata_found && !valid_source_vendor_metadata_found) {
    LOG(INFO) << __func__ << " return as unsupported for LC3 src";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
  }

  * _aidl_return = result;
  return ndk::ScopedAStatus::ok();
}

// For each requirement, a valid ASE configuration will satify:
// - matched with the sink capability (if presented)
// - AND matched with the source capability (if presented)
// - and the setting need to pass the requirement
ndk::ScopedAStatus LeAudioOffloadAudioProvider::getLeAudioAseConfiguration(
    const std::optional<std::vector<
        std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
        in_remoteSinkAudioCapabilities,
    const std::optional<std::vector<
        std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
        in_remoteSourceAudioCapabilities,
    const std::vector<IBluetoothAudioProvider::LeAudioConfigurationRequirement>&
        in_requirements,
    std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting>*
        _aidl_return) {

  BluetoothCodecExtensiblityFlags::kEnableLeAudioCodecExtensibility = true;

  // Get all configuration settings
  std::vector<std::pair<
      std::string, IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>
      ase_configuration_settings =
          BluetoothAudioCodecs::GetLeAudioAseConfigurationSettings();

  if (!in_remoteSinkAudioCapabilities.has_value() &&
      !in_remoteSourceAudioCapabilities.has_value()) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  // Matched ASE configuration with ignored audio context
  std::vector<std::pair<
      std::string, IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>
      sink_matched_ase_configuration_settings;
  std::vector<std::pair<
      std::string, IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>
      matched_ase_configuration_settings;

  LOG(INFO) << __func__ << ": in_remoteSinkAudioCapabilities size: "
                        << in_remoteSinkAudioCapabilities->size();
  LOG(INFO) << __func__ << ": ase_configuration_settings size: "
                        << ase_configuration_settings.size();

  // A setting must match both source and sink.
  // First filter all setting matched with sink capability
  if (in_remoteSinkAudioCapabilities.has_value()) {
    for (auto& [setting_name, setting] : ase_configuration_settings) {

      LOG(DEBUG) << __func__ << ": filter with sink ase Setting: " << setting_name;
      for (auto& capability : in_remoteSinkAudioCapabilities.value()) {
        if (!capability.has_value()) {
          LOG(INFO) << __func__ << ": Sink capability not exist, continue.";
          continue;
        }

        LOG(INFO) << __func__ << ": Sink capability: " << capability->toString();
        auto filtered_ase_configuration_setting =
                           getCapabilitiesMatchedAseConfigurationSettings(
                                        setting, capability.value(), kLeAudioDirectionSink);
        if (filtered_ase_configuration_setting.has_value()) {
          LOG(DEBUG) << __func__
                     << ": pushing sink filtered Setting: " << setting_name;
          sink_matched_ase_configuration_settings.push_back(
              {setting_name, filtered_ase_configuration_setting.value()});
        }
      }
    }
  } else {
    LOG(INFO) << __func__ << ": in_remoteSinkAudioCapabilities not exist: ";
    sink_matched_ase_configuration_settings = ase_configuration_settings;
  }

  // Combine filter every source capability
  LOG(INFO) << __func__ << ": in_remoteSourceAudioCapabilities size: "
                        << in_remoteSourceAudioCapabilities->size();
  if (in_remoteSourceAudioCapabilities.has_value()) {
    for (auto& [setting_name, setting] : sink_matched_ase_configuration_settings) {

      LOG(DEBUG) << __func__ << ": source ase Setting: " << setting_name;
      for (auto& capability : in_remoteSourceAudioCapabilities.value()) {
        if (!capability.has_value()) {
          LOG(INFO) << __func__ << ": Source capability not exist, continue.";
          continue;
        }

        LOG(INFO) << __func__ << ": Source capability: " << capability->toString();
        auto filtered_ase_configuration_setting =
                           getCapabilitiesMatchedAseConfigurationSettings(
                                       setting, capability.value(), kLeAudioDirectionSource);
        if (filtered_ase_configuration_setting.has_value()) {
          LOG(DEBUG) << __func__
                     << ": pushing source filtered Setting: " << setting_name;
          matched_ase_configuration_settings.push_back(
              {setting_name, filtered_ase_configuration_setting.value()});
        }
      }
    }
  } else {
    LOG(INFO) << __func__ << ": in_remoteSourceAudioCapabilities not exist: ";
    matched_ase_configuration_settings = sink_matched_ase_configuration_settings;
  }

  std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting> result_no_name;

  std::vector<std::pair<
      std::string, IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>
      result;

  std::vector<std::pair<
      std::string, IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>
      temp_result;

  LOG(INFO) << __func__ << ": in_requirements size: " << in_requirements.size();
  for (auto& requirement : in_requirements) {
    // For each requirement, try to match with a setting.
    // If we cannot match, return an empty result.

    // Matching priority list:
    // Matched configuration flags, i.e. for asymmetric requirement.
    // Preferred context - exact match with allocation
    // Preferred context - loose match with allocation
    // Any context - exact match with allocation
    // Any context - loose match with allocation
    bool found = false;
    for (bool match_flag : {true, false}) {
      for (bool match_context : {true/*, false*/}) {
        for (bool match_exact : {true, false}) {
          LOG(INFO) << __func__
                    << ": going to match requiremet with match_flag: "<< match_flag
                    << ", match_context: " << match_context
                    << ", match_exact: " << match_exact;
          auto matched_setting =
              matchWithRequirement(matched_ase_configuration_settings,
                                   requirement, match_context, match_exact, match_flag);
          if (matched_setting.has_value()) {
            LOG(INFO) << __func__
                      << ": matched setting with match_flag, match_context and"
                         " match_exact is available, push to result.";
            temp_result.push_back(matched_setting.value());
            found = true;
            continue;
          }
        }
        if (found) break;
      }
      if (found) break;
    }

    if (temp_result.size() == 1)
      result.push_back(temp_result[0]);
    else if (temp_result.size() == 2) { // considering exactly 2 matches isexact: true/false
      // 1. Check codec priority if 2 matches found
      // 2. If priority is same then select exact match over loose one
      auto p0 = getSelectedConfigCodecPriority(temp_result[0].second, kLeAudioDirectionSink);
      auto p1 = getSelectedConfigCodecPriority(temp_result[1].second, kLeAudioDirectionSink);
      if (p0 > p1)
        result.push_back(temp_result[0]);
      else if (p1 > p0)
        result.push_back(temp_result[1]);
      else { //p0 == p1
        // When priorities are equal, prefer the first result which is likely from the exact match
        // since we try exact matches first in the outer loop
        result.push_back(temp_result[0]);
      }
    } else {
      // 1. If temp size 0 means no match mean found = false, no handling needed
      // 2. If temp size > 2 means match_flag loop logic also kick-in take care seperately // TO-DO
      if (temp_result.size() == 0) {
        LOG(INFO) << __func__ << ": No matches found in temp_result";
      } else if (temp_result.size() > 2) {
        LOG(INFO) << __func__ << ": Multiple matches found (" << temp_result.size()
                  << "), using first match for time being";
        // TO-DO : Flag based multimatch handling, currently going with first match for time being
        result.push_back(temp_result[0]);
      }
    }

    temp_result.clear();

    if (!found) {
      // Cannot find a match for this requirement
      // Immediately return
      LOG(ERROR) << __func__ << ": Cannot find any match for this requirement, exitting...";
      *_aidl_return = result_no_name;
      return ndk::ScopedAStatus::ok();
    }
  }

  LOG(INFO) << __func__ << ": result size: " << result.size();
  //Below to check for Dual ack support
  bool is_dual_ack_config_selected = false;
  for (auto& [setting_name, setting] : result) {
    LOG(INFO) << __func__ << ": name: " << setting_name
              << ", setting: " << setting.toString();
    result_no_name.push_back(setting);

    context_to_config_map[setting.audioContext] = {};
    active_context = AudioContext();

    context_to_config_map[setting.audioContext] = result_no_name;
    active_context = setting.audioContext;
    LOG(INFO) << __func__
              << ": active_context: "<< active_context.toString();
  }

  //Caching of vendor Metadata
  for (auto& choosed_config : result_no_name) {
    bool valid_vendor_metadata_found = false;
    valid_vendor_metadata_found =
        IsValidVendorMetadataExistInChoosedConfig(choosed_config, kLeAudioDirectionSink);
    LOG(INFO) << __func__ << ": valid_vendor_metadata_found: " << valid_vendor_metadata_found;
    if (valid_vendor_metadata_found) {
      checkMatchedPacsForChoosedConfig(in_remoteSinkAudioCapabilities,
                                       in_remoteSourceAudioCapabilities,
                                       choosed_config);
    }
  }

  for (auto& setting : result_no_name) {
    is_dual_ack_config_selected = IsDualAckSupportedConfigSelected(setting, kLeAudioDirectionSink);
    LOG(INFO) << __func__
              << ": is_dual_ack_config_selected: " << is_dual_ack_config_selected;
    if (!is_dual_ack_config_selected) {
      removeMetadataFromResultAndSendToStack(setting);
    } else {
      fillPacsVsmForEnableOp(setting);
    }
  }

  for (auto& finalsetting : result_no_name) {
    LOG(INFO) << __func__ << ": After removed metadata, setting: "
                          << finalsetting.toString();
  }

  *_aidl_return = result_no_name;

  return ndk::ScopedAStatus::ok();
};

bool LeAudioOffloadAudioProvider::isMatchedQosRequirement(
    LeAudioAseQosConfiguration setting_qos,
    AseQosDirectionRequirement requirement_qos) {
  if (setting_qos.retransmissionNum !=
      requirement_qos.preferredRetransmissionNum) {
    return false;
  }
  if (setting_qos.maxTransportLatencyMs >
      requirement_qos.maxTransportLatencyMs) {
    return false;
  }
  return true;
}

bool isValidQosRequirement(AseQosDirectionRequirement qosRequirement) {
  return ((qosRequirement.maxTransportLatencyMs > 0) &&
          (qosRequirement.presentationDelayMaxUs > 0) &&
          (qosRequirement.presentationDelayMaxUs >=
           qosRequirement.presentationDelayMinUs));
}

std::optional<LeAudioAseQosConfiguration>
LeAudioOffloadAudioProvider::getDirectionQosConfiguration(
    uint8_t direction,
    const IBluetoothAudioProvider::LeAudioAseQosConfigurationRequirement&
        qosRequirement,
    std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>&
        ase_configuration_settings,
    bool isExact, bool isMatchFlags) {
  auto requirement_flags_bitmask = 0;
  if (isMatchFlags) {
    if (!qosRequirement.flags.has_value()) return std::nullopt;
    requirement_flags_bitmask = qosRequirement.flags.value().bitmask;
  }

  std::optional<AseQosDirectionRequirement> direction_qos_requirement =
      std::nullopt;

  // Get the correct direction
  if (direction == kLeAudioDirectionSink) {
    direction_qos_requirement = qosRequirement.sinkAseQosRequirement.value();
  } else {
    direction_qos_requirement = qosRequirement.sourceAseQosRequirement.value();
  }

  for (auto& [setting_name, setting] : ase_configuration_settings) {
    // Context matching
    if ((setting.audioContext.bitmask & qosRequirement.audioContext.bitmask) !=
        qosRequirement.audioContext.bitmask)
      continue;
    LOG(DEBUG) << __func__
               << ": Setting with matched context: name: " << setting_name
               << ", setting: " << setting.toString();

    // Match configuration flags
    if (isMatchFlags) {
      if (!setting.flags.has_value()) continue;
      if ((setting.flags.value().bitmask & requirement_flags_bitmask) !=
          requirement_flags_bitmask)
        continue;
      LOG(DEBUG) << __func__
                 << ": Setting with matched flags: name: " << setting_name
                 << ", setting: " << setting.toString();
    }

    // Get a list of all matched AseDirectionConfiguration
    // for the input direction
    std::optional<std::vector<std::optional<AseDirectionConfiguration>>>
        direction_configuration = std::nullopt;
    if (direction == kLeAudioDirectionSink) {
      if (!setting.sinkAseConfiguration.has_value()) continue;
      direction_configuration.emplace(setting.sinkAseConfiguration.value());
    } else {
      if (!setting.sourceAseConfiguration.has_value()) continue;
      direction_configuration.emplace(setting.sourceAseConfiguration.value());
    }

    if (!direction_configuration.has_value()) {
      return std::nullopt;
    }

    // Collect all valid cfg into a vector
    // Then try to get the best match for audio allocation

    auto temp = std::vector<AseDirectionConfiguration>();

    for (auto& cfg : direction_configuration.value()) {
      if (!cfg.has_value()) continue;
      // If no requirement, return the first QoS
      if (!direction_qos_requirement.has_value()) {
        return cfg.value().qosConfiguration;
      }

      // If has requirement, return the first matched QoS
      // Try to match the ASE configuration
      // and QoS with requirement
      if (!cfg.value().qosConfiguration.has_value()) continue;
      if (filterMatchedAseConfiguration(
              cfg.value().aseConfiguration,
              direction_qos_requirement.value().aseConfiguration) &&
          isMatchedQosRequirement(cfg.value().qosConfiguration.value(),
                                  direction_qos_requirement.value())) {
        temp.push_back(cfg.value());
      }
    }
    LOG(WARNING) << __func__ << ": Got " << temp.size()
                 << " configs, start matching allocation";

    int qos_allocation_bitmask = getLeAudioAseConfigurationAllocationBitmask(
        direction_qos_requirement.value().aseConfiguration);
    // Get the best matching config based on channel allocation
    auto req_valid_configs =
        getValidConfigurationsFromAllocation(qos_allocation_bitmask, temp, isExact);
    if (req_valid_configs.empty()) {
      LOG(WARNING) << __func__
                   << ": Cannot find matching allocation for bitmask "
                   << qos_allocation_bitmask;

    } else {
      return req_valid_configs[0].qosConfiguration;
    }
  }

  return std::nullopt;
}

ndk::ScopedAStatus LeAudioOffloadAudioProvider::getLeAudioAseQosConfiguration(
    const IBluetoothAudioProvider::LeAudioAseQosConfigurationRequirement&
        in_qosRequirement,
    IBluetoothAudioProvider::LeAudioAseQosConfigurationPair* _aidl_return) {
  IBluetoothAudioProvider::LeAudioAseQosConfigurationPair result;

  // Get all configuration settings
  std::vector<std::pair<
      std::string, IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>
      ase_configuration_settings =
          BluetoothAudioCodecs::GetLeAudioAseConfigurationSettings();

  // Direction QoS matching
  // Only handle one direction input case
  if (in_qosRequirement.sinkAseQosRequirement.has_value()) {
    if (!isValidQosRequirement(in_qosRequirement.sinkAseQosRequirement.value()))
      return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    bool found = false;
    for (bool match_flag : {true, false}) {
      for (bool match_exact : {true, false}) {
        auto setting = getDirectionQosConfiguration(
            kLeAudioDirectionSink, in_qosRequirement,
            ase_configuration_settings, match_exact, match_flag);
        if (setting.has_value()) {
          found = true;
          result.sinkQosConfiguration = setting;
          break;
        }
      }
      if (found) break;
    }
  }
  if (in_qosRequirement.sourceAseQosRequirement.has_value()) {
    if (!isValidQosRequirement(
            in_qosRequirement.sourceAseQosRequirement.value()))
      return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    bool found = false;
    for (bool match_flag : {true, false}) {
      for (bool match_exact : {true, false}) {
        auto setting = getDirectionQosConfiguration(
            kLeAudioDirectionSource, in_qosRequirement,
            ase_configuration_settings, match_exact, match_flag);
        if (setting.has_value()) {
          found = true;
          result.sourceQosConfiguration = setting;
          break;
        }
      }
      if (found) break;
    }
  }

  *_aidl_return = result;
  return ndk::ScopedAStatus::ok();
};

ndk::ScopedAStatus LeAudioOffloadAudioProvider::onSinkAseMetadataChanged(
    IBluetoothAudioProvider::AseState in_state, int32_t in_cigId,
    int32_t in_cisId,
    const std::optional<std::vector<std::optional<MetadataLtv>>>& in_metadata) {
  std::vector<uint8_t> payload = {};
  auto metadata = in_metadata.value_or(std::vector<std::optional<MetadataLtv>>{});
  if (!metadata.empty()) {
    LOG(DEBUG) << __func__ << ": in_metadata has a value!";
    for (auto& meta : metadata) {
      if (meta.has_value() &&
          meta.value().getTag() == MetadataLtv::vendorSpecific) {
        auto k = meta.value().get<MetadataLtv::vendorSpecific>().opaqueValue;
        payload = std::vector(k.begin(), k.end());
        break;
      }
    }
  } else {
    LOG(DEBUG) << __func__ << ": in_metadata doesn't have any value!";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }
  BluetoothAudioSessionReport::ReportMetadataChanged(session_type_, in_cigId, in_cisId, payload);
  return ndk::ScopedAStatus::ok();
};

ndk::ScopedAStatus LeAudioOffloadAudioProvider::onSourceAseMetadataChanged(
    IBluetoothAudioProvider::AseState in_state, int32_t /*in_cigId*/,
    int32_t /*in_cisId*/,
    const std::optional<std::vector<std::optional<MetadataLtv>>>& in_metadata) {
  (void)in_state;
  (void)in_metadata;
  return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
};

LeAudioBroadcastConfigurationSetting getDefaultBroadcastSettingByProperty(
  IBluetoothAudioProvider::BroadcastQuality quality) {
  LeAudioBroadcastConfigurationSetting setting;
  char bis_prop_value[PROPERTY_VALUE_MAX] = {0};
  property_get("persist.vendor.qcom.bluetooth.bis_audio_config_setting", bis_prop_value, "");

  if (!strcmp(bis_prop_value, "16_2")) {
    setting.sduIntervalUs = 10000;
    setting.maxSduOctets = 40 * 2;
  } else if (!strcmp(bis_prop_value, "24_2")) {
    setting.sduIntervalUs = 10000;
    setting.maxSduOctets = 60 * 2;
  } else if (!strcmp(bis_prop_value, "48_1")) {
    setting.sduIntervalUs = 7500;
    setting.maxSduOctets = 75 * 2;
  } else if (!strcmp(bis_prop_value, "48_2")) {
    setting.sduIntervalUs = 10000;
    setting.maxSduOctets = 100 * 2;
  } else if (!strcmp(bis_prop_value, "48_3")) {
    setting.sduIntervalUs = 7500;
    setting.maxSduOctets = 90 * 2;
  } else if (!strcmp(bis_prop_value, "48_4")) {
    setting.sduIntervalUs = 10000;
    setting.maxSduOctets = 120 * 2;
  } else if (!strcmp(bis_prop_value, "48_5")) {
    setting.sduIntervalUs = 7500;
    setting.maxSduOctets = 117 * 2;
  } else if (!strcmp(bis_prop_value, "48_6")) {
    setting.sduIntervalUs = 10000;
    setting.maxSduOctets = 155 * 2;
  } else {
    if (quality == IBluetoothAudioProvider::BroadcastQuality::HIGH) {
      setting.sduIntervalUs = 10000;
      setting.maxSduOctets = 100 * 2;
    } else {
      setting.sduIntervalUs = 10000;
      setting.maxSduOctets = 40 * 2;
    }
  }
  setting.retransmitionNum =
      property_get_int32("persist.vendor.qcom.bluetooth.bis_rtn", 2);
  if (setting.sduIntervalUs == 7500) {
    setting.maxTransportLatencyMs =
        property_get_int32("persist.vendor.qcom.bluetooth.transport_latency", 45);
  } else {
    setting.maxTransportLatencyMs =
        property_get_int32("persist.vendor.qcom.bluetooth.transport_latency", 61);
  }
  LOG(INFO) << __func__ << ": Get Default Broadcast Setting by property"
            << ", sduIntervalUs: " << setting.sduIntervalUs
            << ", maxSduOctets: " << setting.maxSduOctets / 2
            << ", retransmitionNum: " << setting.retransmitionNum
            << ", maxTransportLatencyMs: " << setting.maxTransportLatencyMs;
  return setting;
}

LeAudioBroadcastConfigurationSetting getDefaultBroadcastSetting(
    int context_bitmask, IBluetoothAudioProvider::BroadcastQuality quality) {
  LeAudioBroadcastConfigurationSetting setting;
  setting.retransmitionNum = 2;
  setting.maxTransportLatencyMs = 61;
  setting.sduIntervalUs = 10000;
  setting.maxSduOctets = 40;

  if (property_get_bool("persist.vendor.qcom.bluetooth.bis_audio_config.enabled", false)) {
    return getDefaultBroadcastSettingByProperty(quality);
  }

  if (quality == IBluetoothAudioProvider::BroadcastQuality::HIGH) {
    LOG(INFO) << __func__ << ": High quality, returning high quality settings";
    setting.retransmitionNum = 2;
    setting.maxTransportLatencyMs = 61;
    setting.maxSduOctets = 200;
    LOG(INFO) << __func__ << ": retransmitionNum: " << setting.retransmitionNum
                << ", maxTransportLatencyMs: " << setting.maxTransportLatencyMs;
    return setting;
  }

  // Populate other settings base on context
  // TODO: Populate with better design
  if (context_bitmask & (AudioContext::LIVE_AUDIO | AudioContext::GAME)) {
    setting.retransmitionNum = 2;
    setting.maxTransportLatencyMs = 10;
    setting.maxSduOctets = 120;
  } else if (context_bitmask & (AudioContext::INSTRUCTIONAL)) {
    setting.retransmitionNum = 2;
    setting.maxTransportLatencyMs = 10;
    setting.maxSduOctets = 40;
  } else if (context_bitmask &
             (AudioContext::SOUND_EFFECTS | AudioContext::UNSPECIFIED)) {
    setting.retransmitionNum = 2;
    setting.maxTransportLatencyMs = 61;
    setting.maxSduOctets = 80;
  } else if (context_bitmask &
             (AudioContext::ALERTS | AudioContext::NOTIFICATIONS |
              AudioContext::EMERGENCY_ALARM)) {
    setting.retransmitionNum = 2;
    setting.maxTransportLatencyMs = 61;
    setting.maxSduOctets = 40;
  } else if (context_bitmask & AudioContext::MEDIA) {
    setting.retransmitionNum = 2;
    setting.maxTransportLatencyMs = 61;
    setting.maxSduOctets = 120;
  }

  return setting;
}
void modifySubBISConfigAllocation(
    IBluetoothAudioProvider::LeAudioSubgroupBisConfiguration& sub_bis_cfg,
    int allocation_bitmask) {
  for (auto& codec_cfg : sub_bis_cfg.bisConfiguration.codecConfiguration) {
    if (codec_cfg.getTag() ==
        CodecSpecificConfigurationLtv::audioChannelAllocation) {
      codec_cfg.get<CodecSpecificConfigurationLtv::audioChannelAllocation>()
          .bitmask = allocation_bitmask;
      break;
    }
  }
}
void modifySubgroupConfiguration(
    IBluetoothAudioProvider::LeAudioBroadcastSubgroupConfiguration&
        subgroup_cfg,
    int context_bitmask) {
  // STEREO configs
  // Split into 2 sub BIS config, each has numBis = 1
  if (context_bitmask & (AudioContext::LIVE_AUDIO | AudioContext::GAME |
                         AudioContext::SOUND_EFFECTS |
                         AudioContext::UNSPECIFIED | AudioContext::MEDIA)) {
    if (subgroup_cfg.bisConfigurations.size() == 1)
      subgroup_cfg.bisConfigurations.push_back(
          subgroup_cfg.bisConfigurations[0]);

    subgroup_cfg.bisConfigurations[0].numBis = 1;
    modifySubBISConfigAllocation(
        subgroup_cfg.bisConfigurations[0],
        CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_LEFT);

    subgroup_cfg.bisConfigurations[1].numBis = 1;
    modifySubBISConfigAllocation(
        subgroup_cfg.bisConfigurations[1],
        CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_RIGHT);
    return;
  }

  // MONO configs
  for (auto& sub_bis_cfg : subgroup_cfg.bisConfigurations) {
    sub_bis_cfg.numBis = 1;
    modifySubBISConfigAllocation(
        sub_bis_cfg,
        CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_CENTER);
  }
}

bool isSubgroupConfigurationMatchedByProperty(
    LeAudioBroadcastConfigurationSetting& setting,
    IBluetoothAudioProvider::LeAudioBroadcastSubgroupConfiguration configuration) {
  int preferred_octets_per_codec_frame = setting.maxSduOctets / 2;
  for (auto& sub_bis_cfg : configuration.bisConfigurations) {
    for (auto& codec_cfg : sub_bis_cfg.bisConfiguration.codecConfiguration) {
      if (codec_cfg.getTag() ==
          CodecSpecificConfigurationLtv::octetsPerCodecFrame) {
          LOG(INFO) << __func__
                    << ": CodecSpecificConfigurationLtv::octetsPerCodecFrame: "
                    << codec_cfg.get<CodecSpecificConfigurationLtv::octetsPerCodecFrame>().value;
        if (codec_cfg.get<CodecSpecificConfigurationLtv::octetsPerCodecFrame>()
            .value == preferred_octets_per_codec_frame) {
          LOG(INFO) << __func__
                    << ": Matched octetsPerCodecFrame: "
                    << preferred_octets_per_codec_frame;
          return true;
        }
      }
    }
  }
  LOG(INFO) << __func__
            << ": No matched config by property";
  return false;
}

void LeAudioOffloadAudioProvider::getBroadcastSettings() {
  if (!broadcast_settings.empty()) return;

  LOG(INFO) << __func__
            << ": Loading basic broadcast settings from provider info";

  std::vector<CodecInfo> db_codec_info =
      BluetoothAudioCodecs::GetLeAudioOffloadCodecInfo(
          SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
  broadcast_settings.clear();

  // Default value for population
  CodecSpecificConfigurationLtv::AudioChannelAllocation default_allocation;
  default_allocation.bitmask =
      CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_CENTER;
  CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU default_frame;
  default_frame.value = 1;

  for (auto& codec_info : db_codec_info) {
    if (codec_info.transport.getTag() != CodecInfo::Transport::leAudio)
      continue;
    auto& transport = codec_info.transport.get<CodecInfo::Transport::leAudio>();
    LeAudioBroadcastConfigurationSetting setting;
    setting.retransmitionNum = 4;
    setting.maxTransportLatencyMs = 60;
    setting.sduIntervalUs = 10000;
    setting.maxSduOctets = 40;
    // Default setting
    setting.numBis = 1;
    setting.phy = {Phy::TWO_M};
    // Populate BIS configuration info using codec_info
    LeAudioBisConfiguration bis_cfg;
    bis_cfg.codecId = codec_info.id;

    CodecSpecificConfigurationLtv::OctetsPerCodecFrame octets;
    octets.value = transport.bitdepth[0];

    bis_cfg.codecConfiguration = {
        sampling_freq_map[transport.samplingFrequencyHz[0]],
        octets,
        frame_duration_map[transport.frameDurationUs[0]],
        default_allocation,
        default_frame,
    };

    // Ignore bis_cfg.metadata

    // Add information to structure
    IBluetoothAudioProvider::LeAudioSubgroupBisConfiguration sub_bis_cfg;
    sub_bis_cfg.numBis = 1;
    sub_bis_cfg.bisConfiguration = bis_cfg;
    IBluetoothAudioProvider::LeAudioBroadcastSubgroupConfiguration sub_cfg;
    // Populate the same sub config
    sub_cfg.bisConfigurations = {sub_bis_cfg};
    setting.subgroupsConfigurations = {sub_cfg};

    broadcast_settings.push_back(setting);
  }

  LOG(INFO) << __func__
            << ": Done loading broadcast settings from provider info";
}

/* Get a new LeAudioAseConfigurationSetting by matching a setting with a
 * capabilities. The new setting will have a filtered list of
 * AseDirectionConfiguration that matched the capabilities */
std::optional<LeAudioBroadcastConfigurationSetting>
LeAudioOffloadAudioProvider::
    getCapabilitiesMatchedBroadcastConfigurationSettings(
        LeAudioBroadcastConfigurationSetting& setting,
        const IBluetoothAudioProvider::LeAudioDeviceCapabilities&
            capabilities) {
  std::vector<IBluetoothAudioProvider::LeAudioBroadcastSubgroupConfiguration>
      filter_subgroup;
  for (auto& sub_cfg : setting.subgroupsConfigurations) {
    std::vector<IBluetoothAudioProvider::LeAudioSubgroupBisConfiguration>
        filtered_bis_cfg;
    for (auto& bis_cfg : sub_cfg.bisConfigurations)
      if (isMatchedBISConfiguration(bis_cfg.bisConfiguration, capabilities)) {
        filtered_bis_cfg.push_back(bis_cfg);
      }
    if (!filtered_bis_cfg.empty()) {
      IBluetoothAudioProvider::LeAudioBroadcastSubgroupConfiguration
          subgroup_cfg;
      subgroup_cfg.bisConfigurations = filtered_bis_cfg;
      filter_subgroup.push_back(subgroup_cfg);
    }
  }
  if (filter_subgroup.empty()) return std::nullopt;

  // Create a new LeAudioAseConfigurationSetting and return
  LeAudioBroadcastConfigurationSetting filtered_setting(setting);
  filtered_setting.subgroupsConfigurations = filter_subgroup;

  return filtered_setting;
}

std::vector<CodecSpecificConfigurationLtv> getCodecRequirementBasedOnContext(
    int context_bitmask, IBluetoothAudioProvider::BroadcastQuality quality) {
  // Default requirement: lc3_stereo_16_2
  std::vector<CodecSpecificConfigurationLtv> requirement = {
      CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000,
      CodecSpecificConfigurationLtv::FrameDuration::US10000,
  };

  if (quality == IBluetoothAudioProvider::BroadcastQuality::HIGH) {
    LOG(INFO) << __func__
              << ": High quality, returning high quality requirement";
    requirement = {
        CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000,
        CodecSpecificConfigurationLtv::FrameDuration::US10000,
    };
    return requirement;
  }

  if (context_bitmask & (AudioContext::LIVE_AUDIO | AudioContext::GAME)) {
    // lc3_stereo_24_2_1
    requirement = {
        CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000,
        CodecSpecificConfigurationLtv::FrameDuration::US10000,
    };
  } else if (context_bitmask & (AudioContext::INSTRUCTIONAL)) {
    // lc3_mono_16_2
    requirement = {
        CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000,
        CodecSpecificConfigurationLtv::FrameDuration::US10000,
    };
  } else if (context_bitmask &
             (AudioContext::SOUND_EFFECTS | AudioContext::UNSPECIFIED)) {
    // lc3_stereo_16_2
    requirement = {
        CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000,
        CodecSpecificConfigurationLtv::FrameDuration::US10000,
    };
  } else if (context_bitmask &
             (AudioContext::ALERTS | AudioContext::NOTIFICATIONS |
              AudioContext::EMERGENCY_ALARM)) {
    // Default requirement: lc3_stereo_16_2
    requirement = {
        CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000,
        CodecSpecificConfigurationLtv::FrameDuration::US10000,
    };
  } else if (context_bitmask & AudioContext::MEDIA) {
    // Default requirement: lc3_stereo_16_2
    // Return the 48k requirement
    requirement = {
        CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000,
        CodecSpecificConfigurationLtv::FrameDuration::US10000,
    };
  }
  return requirement;
}

bool LeAudioOffloadAudioProvider::isSubgroupConfigurationMatchedContext(
    AudioContext requirement_context,
    IBluetoothAudioProvider::BroadcastQuality quality,
    LeAudioBroadcastSubgroupConfiguration configuration) {
  // Find any valid context metadata in the bisConfigurations
  // assuming the bis configuration in the same bis subgroup
  // will have the same context metadata
  std::optional<AudioContext> config_context = std::nullopt;

  auto codec_requirement =
      getCodecRequirementBasedOnContext(requirement_context.bitmask, quality);
  std::map<CodecSpecificConfigurationLtv::Tag, CodecSpecificConfigurationLtv>
      req_tag_map;
  for (auto x : codec_requirement) req_tag_map[x.getTag()] = x;

  for (auto& bis_cfg : configuration.bisConfigurations) {
    // Check every sub_bis_cfg to see which match
    for (auto& x : bis_cfg.bisConfiguration.codecConfiguration) {
      auto p = req_tag_map.find(x.getTag());
      if (p == req_tag_map.end()) continue;
      if (p->second != x) {
        LOG(WARNING) << __func__ << ": does not match for context "
                     << requirement_context.toString()
                     << ", cfg = " << x.toString();
        return false;
      }
    }
  }
  return true;
}

ndk::ScopedAStatus
LeAudioOffloadAudioProvider::getLeAudioBroadcastConfiguration(
    const std::optional<std::vector<
        std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
        in_remoteSinkAudioCapabilities,
    const IBluetoothAudioProvider::LeAudioBroadcastConfigurationRequirement&
        in_requirement,
    LeAudioBroadcastConfigurationSetting* _aidl_return) {
  if (in_requirement.subgroupConfigurationRequirements.empty()) {
    LOG(WARNING) << __func__ << ": Empty requirement";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  // Broadcast setting are from provider info
  // We will allow empty capability input, match all settings with requirements.
  getBroadcastSettings();
  std::vector<LeAudioBroadcastConfigurationSetting> filtered_settings;
  if (!in_remoteSinkAudioCapabilities.has_value() ||
      in_remoteSinkAudioCapabilities.value().empty()) {
    LOG(INFO) << __func__ << ": Empty capability, get all broadcast settings";
    filtered_settings = broadcast_settings;
  } else {
    for (auto& setting : broadcast_settings) {
      for (auto& capability : in_remoteSinkAudioCapabilities.value()) {
        if (!capability.has_value()) continue;
        auto filtered_setting =
            getCapabilitiesMatchedBroadcastConfigurationSettings(
                setting, capability.value());
        if (filtered_setting.has_value())
          filtered_settings.push_back(filtered_setting.value());
      }
    }
  }

  if (filtered_settings.empty()) {
    LOG(WARNING) << __func__ << ": Cannot match any remote capability";
    return ndk::ScopedAStatus::ok();
  }

  if (in_requirement.subgroupConfigurationRequirements.empty()) {
    LOG(INFO) << __func__ << ": Empty requirement";
    *_aidl_return = filtered_settings[0];
    return ndk::ScopedAStatus::ok();
  }

  // For each subgroup config requirement, find a suitable subgroup config.
  // Gather these suitable subgroup config in an array.
  // If the setting can satisfy all requirement, we can return the setting
  // with the filtered array.
  auto context_bitmask =
      in_requirement.subgroupConfigurationRequirements[0].audioContext.bitmask;
  auto quality = in_requirement.subgroupConfigurationRequirements[0].quality;
  LeAudioBroadcastConfigurationSetting return_setting =
      getDefaultBroadcastSetting(context_bitmask, quality);
  // Default setting
  return_setting.numBis = 0;
  return_setting.subgroupsConfigurations = {};

  LeAudioDataPathConfiguration path;
  path.isoDataPathConfiguration.isTransparent = true;
  path.dataPathId = kIsoDataPathPlatformDefault;

  // Each subreq, find a setting that match
  for (auto& sub_req : in_requirement.subgroupConfigurationRequirements) {
    bool is_setting_matched = false;
    bool bis_audio_config_enabled_prop_value =
        property_get_bool("persist.vendor.qcom.bluetooth.bis_audio_config.enabled", false);
    for (auto setting : filtered_settings) {
      bool is_matched = true;
      // Check if every sub BIS config satisfy
      for (auto& sub_group_config : setting.subgroupsConfigurations) {
        if (bis_audio_config_enabled_prop_value) {
          if (!isSubgroupConfigurationMatchedByProperty(
              return_setting, sub_group_config)) {
            is_matched = false;
            break;
          }
        } else {
          if (!isSubgroupConfigurationMatchedContext(
                  sub_req.audioContext, sub_req.quality, sub_group_config)) {
            is_matched = false;
            break;
          }
        }
        path.isoDataPathConfiguration.codecId =
            sub_group_config.bisConfigurations[0].bisConfiguration.codecId;
        // Also modify the subgroup config to match the context
        modifySubgroupConfiguration(sub_group_config, context_bitmask);
      }

      if (is_matched) {
        is_setting_matched = true;
        for (auto& sub_group_config : setting.subgroupsConfigurations)
          return_setting.subgroupsConfigurations.push_back(sub_group_config);
        break;
      }
    }

    if (bis_audio_config_enabled_prop_value) {
      if (is_setting_matched) {
        LOG(INFO) << __func__
                  << ": Broadcast settings matched by property";
        break;
      }
    }

    if (!is_setting_matched) {
      LOG(WARNING) << __func__
                   << ": Cannot find a setting that match requirement "
                   << sub_req.toString();
      return ndk::ScopedAStatus::ok();
    }
  }

  // Populate all numBis
  for (auto& sub_group_config : return_setting.subgroupsConfigurations) {
    for (auto& sub_bis_config : sub_group_config.bisConfigurations) {
      return_setting.numBis += sub_bis_config.numBis;
    }
  }
  return_setting.phy = std::vector<Phy>(return_setting.numBis, Phy::TWO_M);
  // Populate data path config
  return_setting.dataPathConfiguration = path;
  // TODO: Workaround for STEREO configs maxSduOctets being doubled
  if (context_bitmask & (AudioContext::LIVE_AUDIO | AudioContext::GAME |
                         AudioContext::SOUND_EFFECTS |
                         AudioContext::UNSPECIFIED | AudioContext::MEDIA)) {
    return_setting.maxSduOctets /= 2;
  }
  LOG(INFO) << __func__
            << ": Combined setting that match: " << return_setting.toString();
  *_aidl_return = return_setting;
  return ndk::ScopedAStatus::ok();
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
