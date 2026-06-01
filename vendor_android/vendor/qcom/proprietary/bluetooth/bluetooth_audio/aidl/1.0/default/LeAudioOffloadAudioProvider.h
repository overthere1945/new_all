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

#pragma once

#include <map>

#include "BluetoothAudioProvider.h"
#include "aidl/android/hardware/bluetooth/audio/LeAudioAseConfiguration.h"
#include "aidl/android/hardware/bluetooth/audio/MetadataLtv.h"
#include "aidl/android/hardware/bluetooth/audio/SessionType.h"
#include "aidl/android/hardware/bluetooth/audio/ConfigurationFlags.h"
#include <cutils/properties.h>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using LeAudioAseConfigurationSetting =
    IBluetoothAudioProvider::LeAudioAseConfigurationSetting;
using AseDirectionRequirement = IBluetoothAudioProvider::
    LeAudioConfigurationRequirement::AseDirectionRequirement;
using AseDirectionConfiguration = IBluetoothAudioProvider::
    LeAudioAseConfigurationSetting::AseDirectionConfiguration;
using AseQosDirectionRequirement = IBluetoothAudioProvider::
    LeAudioAseQosConfigurationRequirement::AseQosDirectionRequirement;
using LeAudioAseQosConfiguration =
    IBluetoothAudioProvider::LeAudioAseQosConfiguration;
using LeAudioBroadcastConfigurationSetting =
    IBluetoothAudioProvider::LeAudioBroadcastConfigurationSetting;
using ::aidl::android::hardware::bluetooth::audio::AudioConfiguration;
using ::aidl::android::hardware::bluetooth::audio::LeAudioConfiguration;

constexpr uint16_t kLeAudioVendorCompanyIdQualcomm = 0x000A;
constexpr uint16_t kLeAudioCodingFormatAptxLe = 0x0001;
constexpr uint16_t kLeAudioCodingFormatAptxLeX = 0x01AD;

constexpr uint8_t kLeAudioCodecAptxLeTypeSamplingFreq = 0x81;
constexpr uint8_t kLeAudioCodecAptxLeTypeAudioChannelAllocation = 0x83;

constexpr uint8_t kLeAudioCodecLC3QSupportedFeaturesMetadataType = 0x10;
constexpr uint8_t kLeAudioCodecAptxLeSupportedFeaturesMetadataType = 0x11;

constexpr uint8_t kLeAudioCodecLC3QSupportedFeaturesMetadataLen = 0x0B;
constexpr uint8_t kLeAudioCodecAptxLeSupportedFeaturesMetadataLen = 0x0B;

constexpr uint8_t kLeAudioCodecAptxLeSupportedDualAck = 0x80;
constexpr uint16_t kLeAudioCodecAptxLeMpcEnable = 0x100;

constexpr uint8_t kCodingFormatLc3 = 0x01;
constexpr uint8_t kCodingFormatLc3q = 0x02;
constexpr uint8_t kCodingFormatAptxLe = 0x04;
constexpr uint8_t kCodingFormatAptxLeX = 0x08;

class LeAudioOffloadAudioProvider : public BluetoothAudioProvider {
 public:
  LeAudioOffloadAudioProvider();

  bool isValid(const SessionType& sessionType) override;

  ndk::ScopedAStatus startSession(
      const std::shared_ptr<IBluetoothAudioPort>& host_if,
      const AudioConfiguration& audio_config,
      const std::vector<LatencyMode>& latency_modes, DataMQDesc* _aidl_return);
  ndk::ScopedAStatus setCodecPriority(const CodecId& in_codecId,
                                      int32_t in_priority) override;
  ndk::ScopedAStatus getLeAudioAseConfiguration(
      const std::optional<std::vector<
          std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
          in_remoteSinkAudioCapabilities,
      const std::optional<std::vector<
          std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
          in_remoteSourceAudioCapabilities,
      const std::vector<
          IBluetoothAudioProvider::LeAudioConfigurationRequirement>&
          in_requirements,
      std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting>*
          _aidl_return) override;
  ndk::ScopedAStatus getLeAudioAseQosConfiguration(
      const IBluetoothAudioProvider::LeAudioAseQosConfigurationRequirement&
          in_qosRequirement,
      IBluetoothAudioProvider::LeAudioAseQosConfigurationPair* _aidl_return)
      override;
  ndk::ScopedAStatus getLeAudioAseDatapathConfiguration(
      const std::optional<IBluetoothAudioProvider::StreamConfig>& in_sinkConfig,
      const std::optional<IBluetoothAudioProvider::StreamConfig>& in_sourceConfig,
      IBluetoothAudioProvider::LeAudioDataPathConfigurationPair* _aidl_return) override;
  ndk::ScopedAStatus onSourceAseMetadataChanged(
      IBluetoothAudioProvider::AseState in_state, int32_t in_cigId,
      int32_t in_cisId,
      const std::optional<std::vector<std::optional<MetadataLtv>>>& in_metadata)
      override;
  ndk::ScopedAStatus onSinkAseMetadataChanged(
      IBluetoothAudioProvider::AseState in_state, int32_t in_cigId,
      int32_t in_cisId,
      const std::optional<std::vector<std::optional<MetadataLtv>>>& in_metadata)
      override;
  ndk::ScopedAStatus getLeAudioBroadcastConfiguration(
      const std::optional<std::vector<
          std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
          in_remoteSinkAudioCapabilities,
      const IBluetoothAudioProvider::LeAudioBroadcastConfigurationRequirement&
          in_requirement,
      LeAudioBroadcastConfigurationSetting* _aidl_return) override;
  MetadataLtv::VendorSpecific getVendorSpecificMetadataBySessionType(
    SessionType session_type_, const AudioConfiguration& audio_config) override;

 private:
  ndk::ScopedAStatus onSessionReady(DataMQDesc* _aidl_return) override;
  std::map<CodecId, uint32_t> codec_priority_map_;
  std::vector<LeAudioBroadcastConfigurationSetting> broadcast_settings;

  // Private matching function definitions
  bool isMatchedValidCodec(CodecId cfg_codec, CodecId req_codec);
  void processCodecMetadata(
       const std::optional<std::vector<std::optional<MetadataLtv>>>& cfg_metadata,
             std::optional<std::vector<std::optional<MetadataLtv>>> caps_metadata,
             MetadataLtv::VendorSpecific& cfg_vendor_metadata,
             MetadataLtv::VendorSpecific& caps_vendor_metadata,
             AudioContext& caps_preferred_AudioContexts,
             AudioContext& caps_streaming_AudioContexts,
             bool& valid_vendor_specific_metadata_exist);
  bool isMatchedValidVendorCodecSpecificMetadata(CodecId cfg_codec,
                                        MetadataLtv::VendorSpecific& cfg_metadata,
                                        MetadataLtv::VendorSpecific caps_metadata,
                                        AudioContext& preferred_audio_contexts,
                                        AudioContext& setting_context);
  bool filterCapabilitiesMatchedContext(
      AudioContext& setting_context,
      const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities);
  bool isMatchedSamplingFreq(
      CodecSpecificConfigurationLtv::SamplingFrequency& cfg_freq,
      CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies&
          capability_freq);
  bool isMatchedFrameDuration(
      CodecSpecificConfigurationLtv::FrameDuration& cfg_fduration,
      CodecSpecificCapabilitiesLtv::SupportedFrameDurations&
          capability_fduration);
  bool isMatchedAudioChannel(
      CodecSpecificConfigurationLtv::AudioChannelAllocation& cfg_channel,
      CodecSpecificCapabilitiesLtv::SupportedAudioChannelCounts&
          capability_channel);
  bool isMatchedCodecFramesPerSDU(
      CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU& cfg_frame_sdu,
      CodecSpecificCapabilitiesLtv::SupportedMaxCodecFramesPerSDU&
          capability_frame_sdu);
  bool isMatchedOctetsPerCodecFrame(
      CodecSpecificConfigurationLtv::OctetsPerCodecFrame& cfg_octets,
      CodecSpecificCapabilitiesLtv::SupportedOctetsPerCodecFrame&
          capability_octets);
  bool isCapabilitiesMatchedCodecConfiguration(
      std::vector<CodecSpecificConfigurationLtv>& codec_cfg,
      std::vector<CodecSpecificCapabilitiesLtv> codec_capabilities);
  bool filterMatchedAseConfiguration(
      LeAudioAseConfiguration& setting_cfg,
      const LeAudioAseConfiguration& requirement_cfg);
  bool isMatchedBISConfiguration(
      LeAudioBisConfiguration bis_cfg,
      const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities);
  void filterCapabilitiesAseDirectionConfiguration(
      std::vector<std::optional<AseDirectionConfiguration>>&
          direction_configurations,
      const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities,
      std::vector<std::optional<AseDirectionConfiguration>>&
          valid_direction_configurations, AudioContext& setting_context);
  void filterRequirementAseDirectionConfiguration(
      std::optional<std::vector<std::optional<AseDirectionConfiguration>>>&
          direction_configurations,
      const std::vector<std::optional<AseDirectionRequirement>>& requirements,
      std::optional<std::vector<std::optional<AseDirectionConfiguration>>>&
          valid_direction_configurations,
      bool isExact, std::optional<ConfigurationFlags> flags, uint8_t direction,
      const AudioContext& context);
  std::optional<LeAudioAseConfigurationSetting>
  getCapabilitiesMatchedAseConfigurationSettings(
      IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
      const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities,
      uint8_t direction);
  std::optional<LeAudioAseConfigurationSetting>
  getRequirementMatchedAseConfigurationSettings(
      IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
      const IBluetoothAudioProvider::LeAudioConfigurationRequirement&
          requirement,
      bool isExact);
  bool isMatchedQosRequirement(LeAudioAseQosConfiguration setting_qos,
                               AseQosDirectionRequirement requirement_qos);
  std::optional<LeAudioBroadcastConfigurationSetting>
  getCapabilitiesMatchedBroadcastConfigurationSettings(
      LeAudioBroadcastConfigurationSetting& setting,
      const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities);
  void getBroadcastSettings();
  std::optional<LeAudioAseQosConfiguration> getDirectionQosConfiguration(
      uint8_t direction,
      const IBluetoothAudioProvider::LeAudioAseQosConfigurationRequirement&
          qosRequirement,
      std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>&
          ase_configuration_settings,
      bool isExact, bool isMatchedFlag);
  bool isSubgroupConfigurationMatchedContext(
      AudioContext requirement_context,
      IBluetoothAudioProvider::BroadcastQuality quality,
      LeAudioBroadcastSubgroupConfiguration configuration);
  std::optional<std::pair<
      std::string, IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>
  matchWithRequirement(
      std::vector<
          std::pair<std::string,
                    IBluetoothAudioProvider::LeAudioAseConfigurationSetting>>&
          matched_ase_configuration_settings,
      const IBluetoothAudioProvider::LeAudioConfigurationRequirement&
          requirements,
      bool isMatchContext, bool isExact, bool isMatchFlags);
  void populateVendorConfigDataPathPayload(LeAudioAseConfiguration& choosed_cfg,
                          const LeAudioConfiguration::StreamMap& stream_map,
                          const AudioContext& Choosed_context,
                          std::optional<LeAudioDataPathConfiguration>& data_path,
                          uint8_t direction);
  bool IsValidVendorMetadataExistInChoosedConfig(
                 IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
                 uint8_t direction);
  bool IsDualAckSupportedConfigSelected(
                 IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
                 uint8_t direction);
  void cachePacsVendorMetadataMatchedWithChoosedConfigByDirection(uint8_t direction,
        const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities);
  void checkMatchedPacsForChoosedConfig(
      const std::optional<std::vector<
          std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
          remoteSinkAudioCapabilities,
      const std::optional<std::vector<
          std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
          remoteSourceAudioCapabilities,
      IBluetoothAudioProvider::LeAudioAseConfigurationSetting& choosed_config);
  std::vector<uint8_t> negotiateAndSelectVendorCodecVersion(CodecId choosed_codec,
            const MetadataLtv::VendorSpecific choosed_config_vendor_Metadata,
                        const MetadataLtv::VendorSpecific pacs_vendor_Metadata);
  AudioContext getCurrentActiveContext();
  std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting>
                      getCachedAseConfigSetting(const AudioContext& context);
  MetadataLtv::VendorSpecific getValidVendorMetadataInCachedConfig(
      IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
      const MetadataLtv::VendorSpecific pacs_vendor_Metadata, uint8_t direction);
  MetadataLtv::VendorSpecific
  negotiatePacsAndCfgToSelectVendorSpecificMetadata(CodecId choosed_codec,
                 const MetadataLtv::VendorSpecific choosed_config_vendor_Metadata,
                 const MetadataLtv::VendorSpecific pacs_vendor_Metadata);
  std::vector<uint16_t> getUniqueCisHandles();
  void prepareMatchedCodecIdBitmask(const AudioContext& context,
                                    LeAudioAseConfiguration& setting_cfg);;
  void PrepareCodecBitmask(CodecId cfg_codec, const AudioContext& context);
  uint8_t getSelectedConfigCodecPriority(
                IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
                uint8_t direction);
  uint8_t getLocalCodecPriority(CodecId cfg_codec, const AudioContext& context);
};

class LeAudioOffloadOutputAudioProvider : public LeAudioOffloadAudioProvider {
 public:
  LeAudioOffloadOutputAudioProvider();
};

class LeAudioOffloadInputAudioProvider : public LeAudioOffloadAudioProvider {
 public:
  LeAudioOffloadInputAudioProvider();
};

class LeAudioOffloadBroadcastAudioProvider
    : public LeAudioOffloadAudioProvider {
 public:
  LeAudioOffloadBroadcastAudioProvider();
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
