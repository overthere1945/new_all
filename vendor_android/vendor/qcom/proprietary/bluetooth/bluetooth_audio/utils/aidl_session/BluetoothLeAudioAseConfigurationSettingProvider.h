/*
* Copyright (c) 2024 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*
* Not a contribution.
*
* Copyright (C) 2023 The Android Open Source Project
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

#include <aidl/android/hardware/bluetooth/audio/IBluetoothAudioProvider.h>

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include "audio_set_configurations_generated.h"
#include "audio_set_scenarios_generated.h"
#include "btavs_client.h"
#include <cutils/properties.h>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using LeAudioAseConfigurationSetting =
    IBluetoothAudioProvider::LeAudioAseConfigurationSetting;
using AseDirectionConfiguration = IBluetoothAudioProvider::
    LeAudioAseConfigurationSetting::AseDirectionConfiguration;
using LeAudioAseQosConfiguration =
    IBluetoothAudioProvider::LeAudioAseQosConfiguration;
using LeAudioDataPathConfiguration =
    IBluetoothAudioProvider::LeAudioDataPathConfiguration;

enum class CodecLocation {
  HOST,
  ADSP,
  CONTROLLER,
};

struct CodecMetadataSetting {
  uint16_t vendor_company_id;
  int8_t vendor_metadata_type;
  std::vector<uint8_t> vs_metadata;
};

#define QBCE_QLL_FT_CHNAGE(x) ((x)[3] & 0x04)
#define QBCE_QLL_BN_VARIATION_BY_QHS_RATE(x) ((x)[3] & 0x08)
#define QBCE_QLE_HCI_SUPPORTED(x) ((x)[3] & 0x10)
#define QBCE_QLL_DUAL_ACK_SUPPORT(x) ((x)[4] & 0x40)

#define EXTN_PROPERTY_VALUE_MAX 92

class AudioSetConfigurationProviderJson {
 public:
  static std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>
  GetLeAudioAseConfigurationSettings();

 private:
  static void LoadAudioSetConfigurationProviderJson();

  static const le_audio::CodecSpecificConfiguration* LookupCodecSpecificParam(
      const flatbuffers::Vector<flatbuffers::Offset<
          le_audio::CodecSpecificConfiguration>>* flat_codec_specific_params,
      le_audio::CodecSpecificLtvGenericTypes type);

  static const le_audio::
     CodecSpecificConfiguration* LookupVendorCodecSpecificParam(
      const flatbuffers::Vector<flatbuffers::Offset<
          le_audio::CodecSpecificConfiguration>>* flat_codec_specific_params,
      le_audio::AptxVendorCodecSpecificLtvGenericTypes type);

  static void populateAudioChannelAllocation(
      CodecSpecificConfigurationLtv::AudioChannelAllocation&
          audio_channel_allocation,
      uint32_t audio_location);

  static void populateConfigurationData(
      LeAudioAseConfiguration& ase,
      const flatbuffers::Vector<
          flatbuffers::Offset<le_audio::CodecSpecificConfiguration>>*
          flat_codec_specific_params);

  static void populateVendorCodecSpecificConfigurationData(
      LeAudioAseConfiguration& ase,
      const flatbuffers::Vector<
          flatbuffers::Offset<le_audio::CodecSpecificConfiguration>>*
          flat_codec_specific_params,  uint16_t max_sdu, uint16_t iso_interval);

  static void populateAseConfiguration(
      LeAudioAseConfiguration& ase,
      const le_audio::AudioSetSubConfiguration* flat_subconfig,
      const le_audio::QosConfiguration* qos_cfg,
      ConfigurationFlags& configurationFlags, CodecMetadataSetting metadata);

  static void populateAseQosConfiguration(
      LeAudioAseQosConfiguration& qos,
      const le_audio::QosConfiguration* qos_cfg, LeAudioAseConfiguration& ase,
      uint8_t ase_channel_cnt, uint16_t max_sdu, uint16_t iso_interval);

  static AseDirectionConfiguration SetConfigurationFromFlatSubconfig(
      const le_audio::AudioSetSubConfiguration* flat_subconfig,
      const le_audio::QosConfiguration* qos_cfg, CodecLocation location,
      ConfigurationFlags& configurationFlags, CodecMetadataSetting metadata);

  static void processSubconfig(
      const le_audio::AudioSetSubConfiguration* subconfig,
      const le_audio::QosConfiguration* qos_cfg,
      std::vector<std::optional<AseDirectionConfiguration>>&
          directionAseConfiguration,
      CodecLocation location, ConfigurationFlags& configurationFlags,
      CodecMetadataSetting metadata);

  static void PopulateAseConfigurationFromFlat(
      const le_audio::AudioSetConfiguration* flat_cfg,
      std::vector<const le_audio::CodecConfiguration*>* codec_cfgs,
      std::vector<const le_audio::QosConfiguration*>* qos_cfgs,
      CodecLocation location,
      std::vector<std::optional<AseDirectionConfiguration>>&
          sourceAseConfiguration,
      std::vector<std::optional<AseDirectionConfiguration>>&
          sinkAseConfiguration,
      ConfigurationFlags& configurationFlags,
      std::vector<const le_audio::CodecSpecifcMetadata*>* metadata_cfgs);

  static bool LoadConfigurationsFromFiles(const char* schema_file,
                                          const char* content_file,
                                          CodecLocation location);

  static bool LoadScenariosFromFiles(const char* schema_file,
                                     const char* content_file);

  static bool LoadContent(
      std::vector<std::pair<const char* /*schema*/, const char* /*content*/>>
          config_files,
      std::vector<std::pair<const char* /*schema*/, const char* /*content*/>>
          scenario_files,
      CodecLocation location);
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
