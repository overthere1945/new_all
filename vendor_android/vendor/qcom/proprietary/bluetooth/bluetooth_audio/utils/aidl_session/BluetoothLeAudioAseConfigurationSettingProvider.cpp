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

#define STREAM_TO_UINT8(u8, p) \
  {                            \
    (u8) = (uint8_t)(*(p));    \
    (p) += 1;                  \
  }
#define STREAM_TO_UINT16(u16, p)                                  \
  {                                                               \
    (u16) = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); \
    (p) += 2;                                                     \
  }
#define STREAM_TO_UINT32(u32, p)                                      \
  {                                                                   \
    (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + \
             ((((uint32_t)(*((p) + 2)))) << 16) +                     \
             ((((uint32_t)(*((p) + 3)))) << 24));                     \
    (p) += 4;                                                         \
  }
#define STREAM_TO_ARRAY(a, p, len)    \
  {                                   \
    int ijk;                          \
    for (ijk = 0; ijk < (len); ijk++) \
      ((uint8_t*)(a))[ijk] = *(p)++;  \
  }

#define LOG_TAG "BTAudioAseConfigAidl"

#include "BluetoothLeAudioAseConfigurationSettingProvider.h"

#include <aidl/android/hardware/bluetooth/audio/AudioConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/AudioContext.h>
#include <aidl/android/hardware/bluetooth/audio/BluetoothAudioStatus.h>
#include <aidl/android/hardware/bluetooth/audio/CodecId.h>
#include <aidl/android/hardware/bluetooth/audio/CodecSpecificCapabilitiesLtv.h>
#include <aidl/android/hardware/bluetooth/audio/CodecSpecificConfigurationLtv.h>
#include <aidl/android/hardware/bluetooth/audio/ConfigurationFlags.h>
#include <aidl/android/hardware/bluetooth/audio/LeAudioAseConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/Phy.h>
#include <android-base/logging.h>

#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

/* Internal structure definition */
std::map<std::string,
         std::tuple<std::vector<std::optional<AseDirectionConfiguration>>,
                    std::vector<std::optional<AseDirectionConfiguration>>,
                    ConfigurationFlags>>
    configurations_;

std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>
    ase_configuration_settings_;

constexpr uint8_t kIsoDataPathHci = 0x00;
constexpr uint8_t kIsoDataPathPlatformDefault = 0x01;
constexpr uint8_t kIsoDataPathDisabled = 0xFF;

constexpr uint8_t kLeAudioDirectionSink = 0x01;
constexpr uint8_t kLeAudioDirectionSource = 0x02;
constexpr uint8_t kLeAudioDirectionBoth =
    kLeAudioDirectionSink | kLeAudioDirectionSource;

/* Sampling Frequencies */
constexpr uint8_t kLeAudioSamplingFreq8000Hz = 0x01;
constexpr uint8_t kLeAudioSamplingFreq11025Hz = 0x02;
constexpr uint8_t kLeAudioSamplingFreq16000Hz = 0x03;
constexpr uint8_t kLeAudioSamplingFreq22050Hz = 0x04;
constexpr uint8_t kLeAudioSamplingFreq24000Hz = 0x05;
constexpr uint8_t kLeAudioSamplingFreq32000Hz = 0x06;
constexpr uint8_t kLeAudioSamplingFreq44100Hz = 0x07;
constexpr uint8_t kLeAudioSamplingFreq48000Hz = 0x08;
constexpr uint8_t kLeAudioSamplingFreq88200Hz = 0x09;
constexpr uint8_t kLeAudioSamplingFreq96000Hz = 0x0A;
constexpr uint8_t kLeAudioSamplingFreq176400Hz = 0x0B;
constexpr uint8_t kLeAudioSamplingFreq192000Hz = 0x0C;
constexpr uint8_t kLeAudioSamplingFreq384000Hz = 0x0D;

/* Frame Durations */
constexpr uint8_t kLeAudioCodecFrameDur7500us = 0x00;
constexpr uint8_t kLeAudioCodecFrameDur10000us = 0x01;

/* Audio Allocations */
constexpr uint32_t kLeAudioLocationNotAllowed = 0x00000000;
constexpr uint32_t kLeAudioLocationFrontLeft = 0x00000001;
constexpr uint32_t kLeAudioLocationFrontRight = 0x00000002;
constexpr uint32_t kLeAudioLocationFrontCenter = 0x00000004;
constexpr uint32_t kLeAudioLocationLowFreqEffects1 = 0x00000008;
constexpr uint32_t kLeAudioLocationBackLeft = 0x00000010;
constexpr uint32_t kLeAudioLocationBackRight = 0x00000020;
constexpr uint32_t kLeAudioLocationFrontLeftOfCenter = 0x00000040;
constexpr uint32_t kLeAudioLocationFrontRightOfCenter = 0x00000080;
constexpr uint32_t kLeAudioLocationBackCenter = 0x00000100;
constexpr uint32_t kLeAudioLocationLowFreqEffects2 = 0x00000200;
constexpr uint32_t kLeAudioLocationSideLeft = 0x00000400;
constexpr uint32_t kLeAudioLocationSideRight = 0x00000800;
constexpr uint32_t kLeAudioLocationTopFrontLeft = 0x00001000;
constexpr uint32_t kLeAudioLocationTopFrontRight = 0x00002000;
constexpr uint32_t kLeAudioLocationTopFrontCenter = 0x00004000;
constexpr uint32_t kLeAudioLocationTopCenter = 0x00008000;
constexpr uint32_t kLeAudioLocationTopBackLeft = 0x00010000;
constexpr uint32_t kLeAudioLocationTopBackRight = 0x00020000;
constexpr uint32_t kLeAudioLocationTopSideLeft = 0x00040000;
constexpr uint32_t kLeAudioLocationTopSideRight = 0x00080000;
constexpr uint32_t kLeAudioLocationTopBackCenter = 0x00100000;
constexpr uint32_t kLeAudioLocationBottomFrontCenter = 0x00200000;
constexpr uint32_t kLeAudioLocationBottomFrontLeft = 0x00400000;
constexpr uint32_t kLeAudioLocationBottomFrontRight = 0x00800000;
constexpr uint32_t kLeAudioLocationFrontLeftWide = 0x01000000;
constexpr uint32_t kLeAudioLocationFrontRightWide = 0x02000000;
constexpr uint32_t kLeAudioLocationLeftSurround = 0x04000000;
constexpr uint32_t kLeAudioLocationRightSurround = 0x08000000;

constexpr uint32_t kLeAudioLocationAnyLeft =
    kLeAudioLocationFrontLeft | kLeAudioLocationBackLeft |
    kLeAudioLocationFrontLeftOfCenter | kLeAudioLocationSideLeft |
    kLeAudioLocationTopFrontLeft | kLeAudioLocationTopBackLeft |
    kLeAudioLocationTopSideLeft | kLeAudioLocationBottomFrontLeft |
    kLeAudioLocationFrontLeftWide | kLeAudioLocationLeftSurround;

constexpr uint32_t kLeAudioLocationAnyRight =
    kLeAudioLocationFrontRight | kLeAudioLocationBackRight |
    kLeAudioLocationFrontRightOfCenter | kLeAudioLocationSideRight |
    kLeAudioLocationTopFrontRight | kLeAudioLocationTopBackRight |
    kLeAudioLocationTopSideRight | kLeAudioLocationBottomFrontRight |
    kLeAudioLocationFrontRightWide | kLeAudioLocationRightSurround;

constexpr uint32_t kLeAudioLocationStereo =
    kLeAudioLocationFrontLeft | kLeAudioLocationFrontRight;

/* Octets Per Frame */
constexpr uint16_t kLeAudioCodecFrameLen30 = 30;
constexpr uint16_t kLeAudioCodecFrameLen40 = 40;
constexpr uint16_t kLeAudioCodecFrameLen60 = 60;
constexpr uint16_t kLeAudioCodecFrameLen80 = 80;
constexpr uint16_t kLeAudioCodecFrameLen100 = 100;
constexpr uint16_t kLeAudioCodecFrameLen120 = 120;

/* Helper map for matching various sampling frequency notations */
const std::map<uint8_t, CodecSpecificConfigurationLtv::SamplingFrequency>
    sampling_freq_map = {
        {kLeAudioSamplingFreq8000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ8000},
        {kLeAudioSamplingFreq16000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000},
        {kLeAudioSamplingFreq24000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000},
        {kLeAudioSamplingFreq32000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ32000},
        {kLeAudioSamplingFreq44100Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ44100},
        {kLeAudioSamplingFreq48000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000},
        {kLeAudioSamplingFreq96000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000}};

/* Helper map for matching various frame durations notations */
const std::map<uint8_t, CodecSpecificConfigurationLtv::FrameDuration>
    frame_duration_map = {
        {kLeAudioCodecFrameDur7500us,
         CodecSpecificConfigurationLtv::FrameDuration::US7500},
        {kLeAudioCodecFrameDur10000us,
         CodecSpecificConfigurationLtv::FrameDuration::US10000}};

/* Helper map for matching various audio channel allocation notations */
std::map<uint32_t, uint32_t> audio_channel_allocation_map = {
    {kLeAudioLocationNotAllowed,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::NOT_ALLOWED},
    {kLeAudioLocationFrontLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_LEFT},
    {kLeAudioLocationFrontRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_RIGHT},
    {kLeAudioLocationFrontCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_CENTER},
    {kLeAudioLocationLowFreqEffects1,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         LOW_FREQUENCY_EFFECTS_1},
    {kLeAudioLocationBackLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BACK_LEFT},
    {kLeAudioLocationBackRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BACK_RIGHT},
    {kLeAudioLocationFrontLeftOfCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         FRONT_LEFT_OF_CENTER},
    {kLeAudioLocationFrontRightOfCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         FRONT_RIGHT_OF_CENTER},
    {kLeAudioLocationBackCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BACK_CENTER},
    {kLeAudioLocationLowFreqEffects2,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         LOW_FREQUENCY_EFFECTS_2},
    {kLeAudioLocationSideLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::SIDE_LEFT},
    {kLeAudioLocationSideRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::SIDE_RIGHT},
    {kLeAudioLocationTopFrontLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_FRONT_LEFT},
    {kLeAudioLocationTopFrontRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_FRONT_RIGHT},
    {kLeAudioLocationTopFrontCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_FRONT_CENTER},
    {kLeAudioLocationTopCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_CENTER},
    {kLeAudioLocationTopBackLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_BACK_LEFT},
    {kLeAudioLocationTopBackRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_BACK_RIGHT},
    {kLeAudioLocationTopSideLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_SIDE_LEFT},
    {kLeAudioLocationTopSideRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_SIDE_RIGHT},
    {kLeAudioLocationTopBackCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_BACK_CENTER},
    {kLeAudioLocationBottomFrontCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         BOTTOM_FRONT_CENTER},
    {kLeAudioLocationBottomFrontLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BOTTOM_FRONT_LEFT},
    {kLeAudioLocationBottomFrontRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BOTTOM_FRONT_RIGHT},
    {kLeAudioLocationFrontLeftWide,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_LEFT_WIDE},
    {kLeAudioLocationFrontRightWide,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_RIGHT_WIDE},
    {kLeAudioLocationLeftSurround,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::LEFT_SURROUND},
    {kLeAudioLocationRightSurround,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::RIGHT_SURROUND},
};

constexpr uint16_t kLeAudioCodingFormatAptxLe = 0x0001;
constexpr uint16_t kLeAudioCodingFormatAptxLeX = 0x01AD;
constexpr uint16_t kLeAudioVendorCompanyIdQcom = 0x000A;

constexpr uint8_t kLeAudioCodecAptxLeSupportedDualAck = 0x80;

bool is_aptx_adaptive_le_supported_ = false;
bool is_enhanced_le_gaming_supported_ = false;
bool is_aptx_adaptive_lex_supported_ = false;
bool is_aptx_adaptive_le_dual_ack_supported_ = false;

// Set configuration and scenario files with fallback default
static const std::vector<
    std::pair<const char* /*schema*/, const char* /*content*/>>
    kLeAudioSetConfigs = {{"/vendor/etc/aidl/le_audio/"
                           "aidl_audio_set_configurations.bfbs",
                           "/vendor/etc/aidl/le_audio/"
                           "aidl_audio_set_configurations.json"},

                          {"/vendor/etc/aidl/le_audio/"
                           "aidl_audio_set_configurations.bfbs",
                           "/vendor/etc/aidl/le_audio/"
                           "aidl_default_audio_set_configurations_qti.json"}};

static const std::vector<
    std::pair<const char* /*schema*/, const char* /*content*/>>
    kLeAudioSetScenarios = {{"/vendor/etc/aidl/le_audio/"
                             "aidl_audio_set_scenarios.bfbs",
                             "/vendor/etc/aidl/le_audio/"
                             "aidl_audio_set_scenarios.json"},

                            {"/vendor/etc/aidl/le_audio/"
                             "aidl_audio_set_scenarios.bfbs",
                             "/vendor/etc/aidl/le_audio/"
                             "aidl_default_audio_set_scenarios_qti.json"}};

static const std::vector<
    std::pair<const char* /*schema*/, const char* /*content*/>>
    kVendorLeAudioSetScenarios = {
                            {"/vendor/etc/aidl/le_audio/"
                             "aidl_audio_set_scenarios.bfbs",
                             "/vendor/etc/aidl/le_audio/"
                             "aidl_default_vendor_audio_set_scenarios_qti.json"}};


/* Implementation */

std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>
AudioSetConfigurationProviderJson::GetLeAudioAseConfigurationSettings() {
  AudioSetConfigurationProviderJson::LoadAudioSetConfigurationProviderJson();
  return ase_configuration_settings_;
}

void AudioSetConfigurationProviderJson::
    LoadAudioSetConfigurationProviderJson() {
  if (configurations_.empty() || ase_configuration_settings_.empty()) {
    ase_configuration_settings_.clear();
    configurations_.clear();
    BtAVsClient& instance_ = BtAVsClient::GetInstance();
    instance_.RegisterAudioSetting([]{
      LOG(DEBUG) << ": Clearing configuration";
      ase_configuration_settings_.clear();
      configurations_.clear();
    });
    if (!instance_.IsAdapterStateOn()) {
      LOG(ERROR) << __func__ << ": Unable to load le audio set configuration files : No BT Adapter!";
      return;
    }
    bool loaded = false;
    bool is_row_config =
      property_get_bool("persist.vendor.qcom.bluetooth.is_row_config", true);
    LOG(DEBUG) << ": is_row_config: " << is_row_config;
    if (is_row_config) {
      loaded = LoadContent(kLeAudioSetConfigs, kLeAudioSetScenarios,
                              CodecLocation::ADSP);
    } else {
      loaded = LoadContent(kLeAudioSetConfigs, kVendorLeAudioSetScenarios,
                             CodecLocation::ADSP);
    }
    if (!loaded)
      LOG(ERROR) << ": Unable to load le audio set configuration files.";
  } else
    LOG(INFO) << ": Reusing loaded le audio set configuration";
}

const le_audio::CodecSpecificConfiguration*
AudioSetConfigurationProviderJson::LookupCodecSpecificParam(
    const flatbuffers::Vector<flatbuffers::Offset<
        le_audio::CodecSpecificConfiguration>>* flat_codec_specific_params,
    le_audio::CodecSpecificLtvGenericTypes type) {
  auto it = std::find_if(
      flat_codec_specific_params->cbegin(), flat_codec_specific_params->cend(),
      [&type](const auto& csc) { return (csc->type() == type); });
  return (it != flat_codec_specific_params->cend()) ? *it : nullptr;
}

const le_audio::CodecSpecificConfiguration*
AudioSetConfigurationProviderJson::LookupVendorCodecSpecificParam(
    const flatbuffers::Vector<flatbuffers::Offset<
        le_audio::CodecSpecificConfiguration>>* flat_codec_specific_params,
    le_audio::AptxVendorCodecSpecificLtvGenericTypes type) {
  auto it = std::find_if(
      flat_codec_specific_params->cbegin(), flat_codec_specific_params->cend(),
      [&type](const auto& csc) { return (csc->type() == type); });
  return (it != flat_codec_specific_params->cend()) ? *it : nullptr;
}

void AudioSetConfigurationProviderJson::populateAudioChannelAllocation(
    CodecSpecificConfigurationLtv::AudioChannelAllocation&
        audio_channel_allocation, uint32_t audio_location) {
  audio_channel_allocation.bitmask = 0;
  for (auto [allocation, bitmask] : audio_channel_allocation_map) {
    if (audio_location & allocation)
      audio_channel_allocation.bitmask |= bitmask;
  }
  LOG(DEBUG) << ": audio_channel_allocation.bitmask: "
             << audio_channel_allocation.bitmask;
}

void AudioSetConfigurationProviderJson::populateConfigurationData(
    LeAudioAseConfiguration& ase,
    const flatbuffers::Vector<
        flatbuffers::Offset<le_audio::CodecSpecificConfiguration>>*
        flat_codec_specific_params) {
  uint8_t sampling_frequency = 0;
  uint8_t frame_duration = 0;
  uint32_t audio_channel_allocation = 0;
  uint16_t octets_per_codec_frame = 0;
  uint8_t codec_frames_blocks_per_sdu = 0;

  auto param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_SAMPLING_FREQUENCY);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT8(sampling_frequency, ptr);
  }

  param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_FRAME_DURATION);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT8(frame_duration, ptr);
  }

  param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::
          CodecSpecificLtvGenericTypes_SUPPORTED_AUDIO_CHANNEL_ALLOCATION);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT32(audio_channel_allocation, ptr);
  }

  param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_OCTETS_PER_CODEC_FRAME);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT16(octets_per_codec_frame, ptr);
  }

  param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::
          CodecSpecificLtvGenericTypes_SUPPORTED_CODEC_FRAME_BLOCKS_PER_SDU);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT8(codec_frames_blocks_per_sdu, ptr);
  }

  LOG(DEBUG) << ": sampling_frequency: " << unsigned(sampling_frequency)
             << ", frame_duration: " << unsigned(frame_duration)
             << ", audio_channel_allocation: " << unsigned(audio_channel_allocation)
             << ", octets_per_codec_frame: " << unsigned(octets_per_codec_frame)
             << ", codec_frames_blocks_per_sdu: " << unsigned(codec_frames_blocks_per_sdu);

  // Make the correct value
  ase.codecConfiguration = std::vector<CodecSpecificConfigurationLtv>();

  auto sampling_freq_it = sampling_freq_map.find(sampling_frequency);
  if (sampling_freq_it != sampling_freq_map.end())
    ase.codecConfiguration.push_back(sampling_freq_it->second);
  auto frame_duration_it = frame_duration_map.find(frame_duration);
  if (frame_duration_it != frame_duration_map.end())
    ase.codecConfiguration.push_back(frame_duration_it->second);

  CodecSpecificConfigurationLtv::AudioChannelAllocation channel_allocation;
  populateAudioChannelAllocation(channel_allocation, audio_channel_allocation);
  ase.codecConfiguration.push_back(channel_allocation);

  auto octet_structure = CodecSpecificConfigurationLtv::OctetsPerCodecFrame();
  octet_structure.value = octets_per_codec_frame;
  ase.codecConfiguration.push_back(octet_structure);

  auto frame_sdu_structure =
      CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU();
  frame_sdu_structure.value = codec_frames_blocks_per_sdu;
  ase.codecConfiguration.push_back(frame_sdu_structure);
}

void AudioSetConfigurationProviderJson::populateVendorCodecSpecificConfigurationData(
    LeAudioAseConfiguration& ase,
    const flatbuffers::Vector<flatbuffers::Offset<le_audio::CodecSpecificConfiguration>>*
        flat_codec_specific_params, uint16_t max_sdu, uint16_t iso_interval) {
  uint8_t sampling_frequency = 0;
  uint32_t audio_channel_allocation = 0;
  uint8_t frame_duration = 0;
  uint16_t octets_per_codec_frame = 0;

  LOG(DEBUG) << ": maxSdu: " << (unsigned)max_sdu
             << ", iso_interval: " << (unsigned)iso_interval;

  octets_per_codec_frame = max_sdu;
  if (iso_interval == 7500) {
    frame_duration = 0;
  } else if (iso_interval == 10000){
    frame_duration = 1;
  }
  //frame_duration = iso_interval;
  LOG(DEBUG) << ": frame_duration: " << (unsigned)frame_duration;

  auto param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_SAMPLING_FREQUENCY);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT8(sampling_frequency, ptr);
  }

  param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::
          CodecSpecificLtvGenericTypes_SUPPORTED_AUDIO_CHANNEL_ALLOCATION);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT32(audio_channel_allocation, ptr);
  }

  LOG(DEBUG) << ": sampling_frequency: " << unsigned(sampling_frequency)
             << ", audio_channel_allocation: " << unsigned(audio_channel_allocation);

  if (ase.codecId.has_value() &&
      ase.codecId.value().getTag() == CodecId::Tag::vendor) {
    auto vc = ase.codecId.value().get<CodecId::Tag::vendor>();

    //Below check is for filling of vendorCodecConfiguration
    LOG(DEBUG) << ": vc.codecId: " << vc.codecId;
    if (vc.codecId == kLeAudioCodingFormatAptxLe ||
        vc.codecId == kLeAudioCodingFormatAptxLeX) {
      param = LookupVendorCodecSpecificParam(
          flat_codec_specific_params,
          le_audio::AptxVendorCodecSpecificLtvGenericTypes_SUPPORTED_SAMPLING_FREQUENCY);
      if (param) {
        auto ptr = param->compound_value()->value()->data();
        STREAM_TO_UINT8(sampling_frequency, ptr);
      }

      param = LookupVendorCodecSpecificParam(
          flat_codec_specific_params,
          le_audio::
              AptxVendorCodecSpecificLtvGenericTypes_SUPPORTED_AUDIO_CHANNEL_ALLOCATION);
      if (param) {
        auto ptr = param->compound_value()->value()->data();
        STREAM_TO_UINT32(audio_channel_allocation, ptr);
      }

      LOG(DEBUG) << ": Vendor specific sampling_frequency: " << unsigned(sampling_frequency)
                 << ", audio_channel_allocation: " << unsigned(audio_channel_allocation);

      //Fill both ase.codecConfiguration as well as ase.vendorCodecConfiguration
      auto VendorCodecConfig = std::vector<uint8_t>();
      ase.codecConfiguration = std::vector<CodecSpecificConfigurationLtv>();

      auto sampling_freq_it = sampling_freq_map.find(sampling_frequency);
      sampling_freq_it = sampling_freq_map.find(sampling_frequency);
      if (sampling_freq_it != sampling_freq_map.end()) {
        //Fill codecConfiguration ase
        ase.codecConfiguration.push_back(sampling_freq_it->second);
      }

      CodecSpecificConfigurationLtv::AudioChannelAllocation channel_allocation;
      populateAudioChannelAllocation(channel_allocation, audio_channel_allocation);
      ase.codecConfiguration.push_back(channel_allocation);

      auto octets = CodecSpecificConfigurationLtv::OctetsPerCodecFrame();
      octets.value = max_sdu;
      ase.codecConfiguration.push_back(octets);

      auto frameDuration = frame_duration_map.find(frame_duration);
      if (frameDuration != frame_duration_map.end())
        ase.codecConfiguration.push_back(frameDuration->second);

      for (auto const& param : *flat_codec_specific_params) {
        auto const value = param->compound_value()->value();
        std::vector<uint8_t> value_raw(value->data(), value->data() + value->size());
        VendorCodecConfig.push_back(value_raw.size() + 1);
        VendorCodecConfig.push_back(param->type());
        VendorCodecConfig.insert(VendorCodecConfig.end(), value_raw.begin(), value_raw.end());
      }

      ase.vendorCodecConfiguration =
              std::vector<uint8_t>(VendorCodecConfig.begin(), VendorCodecConfig.end());
    }
  }
}

void AudioSetConfigurationProviderJson::populateAseConfiguration(
    LeAudioAseConfiguration& ase,
    const le_audio::AudioSetSubConfiguration* flat_subconfig,
    const le_audio::QosConfiguration* qos_cfg,
    ConfigurationFlags& configurationFlags, CodecMetadataSetting metadata) {
  // Target latency
  switch (qos_cfg->target_latency()) {
    case le_audio::AudioSetConfigurationTargetLatency::
        AudioSetConfigurationTargetLatency_BALANCED_RELIABILITY:
      ase.targetLatency =
          LeAudioAseConfiguration::TargetLatency::BALANCED_LATENCY_RELIABILITY;
      break;
    case le_audio::AudioSetConfigurationTargetLatency::
        AudioSetConfigurationTargetLatency_HIGH_RELIABILITY:
      ase.targetLatency =
          LeAudioAseConfiguration::TargetLatency::HIGHER_RELIABILITY;
      break;
    case le_audio::AudioSetConfigurationTargetLatency::
        AudioSetConfigurationTargetLatency_LOW:
      ase.targetLatency = LeAudioAseConfiguration::TargetLatency::LOWER;
      configurationFlags.bitmask |= ConfigurationFlags::LOW_LATENCY;
      break;
    default:
      ase.targetLatency = LeAudioAseConfiguration::TargetLatency::UNDEFINED;
      break;
  };

  ase.targetPhy = Phy::TWO_M;
  // Making CodecId
  if (flat_subconfig->codec_id()->coding_format() ==
      (uint8_t)CodecId::Core::LC3) {
    //applies for LC3 and LC3Q as code foramt is LC3 for both codecs
    LOG(INFO) << ": LC3 codec config choosen to parse from json.";
    ase.codecId = CodecId::Core::LC3;
    // Codec configuration data
    populateConfigurationData(ase, flat_subconfig->codec_configuration());
  } else {
    //applies for Aptx Le and Aptx LeX
    auto vendorC = CodecId::Vendor();
    vendorC.codecId = flat_subconfig->codec_id()->vendor_codec_id();
    vendorC.id = flat_subconfig->codec_id()->vendor_company_id();
    ase.codecId = vendorC;
    LOG(INFO) << ": Vendor codec config choosen to parse from json.";
    LOG(DEBUG) << ": Vendor codec ID: " << (unsigned)vendorC.codecId;
    // Codec configuration data
    populateVendorCodecSpecificConfigurationData(ase,
                      flat_subconfig->codec_configuration(),
                      flat_subconfig->max_sdu(), flat_subconfig->iso_interval());
  }

  LOG(DEBUG) << ": vendor_company_id: " << (unsigned)metadata.vendor_company_id;
  if (metadata.vendor_company_id == kLeAudioVendorCompanyIdQcom) {
    LOG(INFO) << ": Filling vendor specific metadata.";
    auto codec_metadata = MetadataLtv::VendorSpecific();
    codec_metadata.companyId = metadata.vendor_company_id;
    codec_metadata.opaqueValue =
                std::vector<uint8_t>(metadata.vs_metadata.begin(),
                                          metadata.vs_metadata.end());
    /* will check later to see right way to assign*/
    //ase.metadata.value().push_back(codec_metadata);
    //ase.metadata.value().push_back(cfg_name);
    ase.metadata = {codec_metadata};
    return;
  }
}

void AudioSetConfigurationProviderJson::populateAseQosConfiguration(
    LeAudioAseQosConfiguration& qos, const le_audio::QosConfiguration* qos_cfg,
    LeAudioAseConfiguration& ase, uint8_t ase_channel_cnt, uint16_t max_sdu,
    uint16_t iso_interval) {
  std::optional<CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU>
      frameBlock = std::nullopt;
  std::optional<CodecSpecificConfigurationLtv::FrameDuration> frameDuration =
      std::nullopt;
  std::optional<CodecSpecificConfigurationLtv::OctetsPerCodecFrame> octet =
      std::nullopt;

  // Hack to put back allocation
  CodecSpecificConfigurationLtv::AudioChannelAllocation allocation =
      CodecSpecificConfigurationLtv::AudioChannelAllocation();
  if (ase_channel_cnt == 1) {
    allocation.bitmask |=
        CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_CENTER;

  } else {
    allocation.bitmask |=
        CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_LEFT |
        CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_RIGHT;
  }
  for (auto& cfg_ltv : ase.codecConfiguration) {
    auto tag = cfg_ltv.getTag();
    if (tag == CodecSpecificConfigurationLtv::codecFrameBlocksPerSDU) {
      frameBlock =
          cfg_ltv.get<CodecSpecificConfigurationLtv::codecFrameBlocksPerSDU>();
    } else if (tag == CodecSpecificConfigurationLtv::frameDuration) {
      frameDuration =
          cfg_ltv.get<CodecSpecificConfigurationLtv::frameDuration>();
    } else if (tag == CodecSpecificConfigurationLtv::octetsPerCodecFrame) {
      octet = cfg_ltv.get<CodecSpecificConfigurationLtv::octetsPerCodecFrame>();
    } else if (tag == CodecSpecificConfigurationLtv::audioChannelAllocation) {
      // Change to the old hack allocation
      cfg_ltv.set<CodecSpecificConfigurationLtv::audioChannelAllocation>(
          allocation);
    }
  }

  if (ase.codecId == CodecId::Core::LC3) {
    int frameBlockValue = 1;
    if (frameBlock.has_value()) frameBlockValue = frameBlock.value().value;
    LOG(DEBUG) << ": frameBlockValue: " << frameBlockValue
             << ", ase_channel_cnt: " << unsigned(ase_channel_cnt);

    // Populate maxSdu
    if (octet.has_value()) {
      qos.maxSdu = ase_channel_cnt * octet.value().value * frameBlockValue;
      LOG(INFO) << ": maxSdu: " << qos.maxSdu;
    }
    // Populate sduIntervalUs
    if (frameDuration.has_value()) {
      switch (frameDuration.value()) {
        case CodecSpecificConfigurationLtv::FrameDuration::US7500:
          qos.sduIntervalUs = 7500;
          break;
        case CodecSpecificConfigurationLtv::FrameDuration::US10000:
          qos.sduIntervalUs = 10000;
          break;
      }
      qos.sduIntervalUs *= frameBlockValue;
    }
  } else {
    /*auto vendorAptxLe = CodecId::Vendor();
    vendorAptxLe.codecId = kLeAudioCodingFormatAptxLe;
    vendorAptxLe.id = 0x000A;
    auto vendorAptxLeX = CodecId::Vendor();
    vendorAptxLeX.codecId = kLeAudioCodingFormatAptxLeX;
    vendorAptxLeX.id = 0x000A;*/
    if (ase.codecId.has_value() &&
        ase.codecId.value().getTag() == CodecId::Tag::vendor) {
      auto vendorCodec = ase.codecId.value().get<CodecId::Tag::vendor>();
      //Below check is for filling of vendorCodecConfiguration
      LOG(DEBUG) << ": vendorCodec: " << vendorCodec.codecId;
      if (vendorCodec.codecId == kLeAudioCodingFormatAptxLe ||
          vendorCodec.codecId == kLeAudioCodingFormatAptxLeX) {
        qos.maxSdu = max_sdu;
        qos.sduIntervalUs = iso_interval;
        LOG(INFO) << ": Vendor codec maxSdu: " << qos.maxSdu;
        LOG(INFO) << ": Vendor codec sduIntervalUs: " << qos.sduIntervalUs;
      }
    }
  }
  qos.maxTransportLatencyMs = qos_cfg->max_transport_latency();
  qos.retransmissionNum = qos_cfg->retransmission_number();
}

// Parse into AseDirectionConfiguration
AseDirectionConfiguration
AudioSetConfigurationProviderJson::SetConfigurationFromFlatSubconfig(
    const le_audio::AudioSetSubConfiguration* flat_subconfig,
    const le_audio::QosConfiguration* qos_cfg, CodecLocation location,
    ConfigurationFlags& configurationFlags, CodecMetadataSetting metadata) {
  AseDirectionConfiguration direction_conf;

  LeAudioAseConfiguration ase;
  LeAudioAseQosConfiguration qos;
  LeAudioDataPathConfiguration path;

  // Translate into LeAudioAseConfiguration
  populateAseConfiguration(ase, flat_subconfig, qos_cfg,
                           configurationFlags, metadata);

  // Translate into LeAudioAseQosConfiguration
  populateAseQosConfiguration(qos, qos_cfg, ase,
                              flat_subconfig->ase_channel_cnt(),
                              flat_subconfig->max_sdu(),
                              flat_subconfig->iso_interval());

  // Translate location to data path id
  switch (location) {
    case CodecLocation::ADSP:
      path.isoDataPathConfiguration.isTransparent = true;
      path.dataPathId = kIsoDataPathPlatformDefault;
      break;
    case CodecLocation::HOST:
      path.isoDataPathConfiguration.isTransparent = true;
      path.dataPathId = kIsoDataPathHci;
      break;
    case CodecLocation::CONTROLLER:
      path.isoDataPathConfiguration.isTransparent = false;
      path.dataPathId = kIsoDataPathPlatformDefault;
      break;
  }
  // Move codecId to iso data path
  path.isoDataPathConfiguration.codecId = ase.codecId.value();

  direction_conf.aseConfiguration = ase;
  direction_conf.qosConfiguration = qos;
  direction_conf.dataPathConfiguration = path;

  return direction_conf;
}

// Parse into AseDirectionConfiguration and the ConfigurationFlags
// and put them in the given list.
void AudioSetConfigurationProviderJson::processSubconfig(
    const le_audio::AudioSetSubConfiguration* subconfig,
    const le_audio::QosConfiguration* qos_cfg,
    std::vector<std::optional<AseDirectionConfiguration>>&
        directionAseConfiguration,
    CodecLocation location, ConfigurationFlags& configurationFlags,
    CodecMetadataSetting metadata) {
  auto ase_cnt = subconfig->ase_cnt();
  LOG(DEBUG) << __func__ << ": ase_cnt: " << (unsigned)ase_cnt;
  auto config =
      SetConfigurationFromFlatSubconfig(subconfig, qos_cfg, location,
                                        configurationFlags, metadata);
  directionAseConfiguration.push_back(config);
  // Put the same setting again.
  if (ase_cnt == 2) directionAseConfiguration.push_back(config);
}

// Comparing if 2 AseDirectionConfiguration is equal.
// Configuration are copied in, so we can remove some fields for comparison
// without affecting the result.
bool isAseConfigurationEqual(AseDirectionConfiguration cfg_a,
                             AseDirectionConfiguration cfg_b) {
  // Remove unneeded fields when comparing.
  cfg_a.aseConfiguration.metadata = std::nullopt;
  cfg_b.aseConfiguration.metadata = std::nullopt;
  return cfg_a == cfg_b;
}

void AudioSetConfigurationProviderJson::PopulateAseConfigurationFromFlat(
    const le_audio::AudioSetConfiguration* flat_cfg,
    std::vector<const le_audio::CodecConfiguration*>* codec_cfgs,
    std::vector<const le_audio::QosConfiguration*>* qos_cfgs,
    CodecLocation location,
    std::vector<std::optional<AseDirectionConfiguration>>&
        sourceAseConfiguration,
    std::vector<std::optional<AseDirectionConfiguration>>& sinkAseConfiguration,
    ConfigurationFlags& configurationFlags,
    std::vector<const le_audio::CodecSpecifcMetadata*>* metadata_cfgs) {
  if (flat_cfg == nullptr) {
    LOG(ERROR) << "flat_cfg cannot be null";
    return;
  }
  std::string codec_config_key = flat_cfg->codec_config_name()->str();
  auto* qos_config_key_array = flat_cfg->qos_config_name();
  auto* metadata_key_array = flat_cfg->codec_metadata_name();
  const le_audio::CodecSpecifcMetadata* metadata_sink_cfg = nullptr;
  const le_audio::CodecSpecifcMetadata* metadata_source_cfg = nullptr;

  if (metadata_key_array != nullptr) {
    if (metadata_key_array->size() > 0) {
      constexpr std::string_view default_metadata = "VND_AAR4_VS_Metadata";
      std::string metadata_sink_key(default_metadata);
      std::string metadata_source_key(default_metadata);
      metadata_sink_key = metadata_key_array->Get(0)->str();
      if (metadata_key_array->size() > 1) {
        metadata_source_key = metadata_key_array->Get(1)->str();
      } else {
        metadata_source_key = metadata_sink_key;
      }

      LOG(INFO) << ": Audio set config: " << flat_cfg->name()->c_str()
                << ", metadata_sink: " << metadata_sink_key.c_str()
                << ", metadata_source: " << metadata_source_key.c_str();

      for (auto i = metadata_cfgs->begin(); i != metadata_cfgs->end(); ++i) {
        if ((*i)->name()->str() == metadata_sink_key) {
          metadata_sink_cfg = *i;
          break;
        }
      }
      for (auto i = metadata_cfgs->begin(); i != metadata_cfgs->end(); ++i) {
        if ((*i)->name()->str() == metadata_source_key) {
          metadata_source_cfg = *i;
          break;
        }
      }
    }
  }

  uint8_t qll_supported_feat_len_ = 0;
  uint8_t soc_add_on_features_len_ = 0;
  bool is_apx_lossless_le_supported_ = false;
  bool is_qhs_enabled_locally_ = false;
  bool is_xpan_enabled_locally_ = false;

  BtAVsClient& instance_ = BtAVsClient::GetInstance();

  uint8_t qhs_support_mask_ = instance_.GetQhsSupportMask();
  LOG(INFO) << ": Extn: qhs_support_mask_: " << (unsigned)qhs_support_mask_;

  if (qhs_support_mask_ != 0) {
    LOG(INFO) << ": Extn: QHS is enabled on this target.";
    is_qhs_enabled_locally_ = true;
  } else {
    LOG(INFO) << ": Extn: QHS is disabled on this target.";
    if (metadata_sink_cfg != nullptr || metadata_source_cfg != nullptr) {
      LOG(INFO) << ": Extn: skip populating vendor codec config as qhs disabled";
      return;
    }
  }

  constexpr std::string_view default_qos = "QoS_Config_Balanced_Reliability";

  std::string qos_sink_key(default_qos);
  std::string qos_source_key(default_qos);

  /* We expect maximum two QoS settings. First for Sink and second for Source
   */
  if (qos_config_key_array->size() > 0) {
    qos_sink_key = qos_config_key_array->Get(0)->str();
    if (qos_config_key_array->size() > 1) {
      qos_source_key = qos_config_key_array->Get(1)->str();
    } else {
      qos_source_key = qos_sink_key;
    }
  }

  LOG(INFO) << "Audio set config " << flat_cfg->name()->c_str()
            << ": codec config " << codec_config_key.c_str()
            << ", qos_sink " << qos_sink_key.c_str()
            << ", qos_source " << qos_source_key.c_str();

  // Find the first qos config that match the name
  const le_audio::QosConfiguration* qos_sink_cfg = nullptr;
  for (auto i = qos_cfgs->begin(); i != qos_cfgs->end(); ++i) {
    if ((*i)->name()->str() == qos_sink_key) {
      qos_sink_cfg = *i;
      break;
    }
  }

  const le_audio::QosConfiguration* qos_source_cfg = nullptr;
  for (auto i = qos_cfgs->begin(); i != qos_cfgs->end(); ++i) {
    if ((*i)->name()->str() == qos_source_key) {
      qos_source_cfg = *i;
      break;
    }
  }

  CodecMetadataSetting metadata_sink;
  if (metadata_sink_cfg != nullptr) {
    metadata_sink.vendor_metadata_type = metadata_sink_cfg->type();
    auto ptr = metadata_sink_cfg->compound_value()->value()->data();
    int size = metadata_sink_cfg->compound_value()->value()->size();
    metadata_sink.vendor_company_id = metadata_sink_cfg->vendor_company_id();
    metadata_sink.vs_metadata.resize(size);
    STREAM_TO_ARRAY(metadata_sink.vs_metadata.data(), ptr, size);
  } else {
    LOG(ERROR) << "No matching metadata found.";
  }

  CodecMetadataSetting metadata_source;
  if (metadata_source_cfg != nullptr) {
     metadata_source.vendor_metadata_type = metadata_source_cfg->type();
     auto ptr = metadata_source_cfg->compound_value()->value()->data();
     int size = metadata_source_cfg->compound_value()->value()->size();
     metadata_source.vendor_company_id = metadata_source_cfg->vendor_company_id();
     metadata_source.vs_metadata.resize(size);
     STREAM_TO_ARRAY(metadata_source.vs_metadata.data(), ptr, size);
  } else {
    LOG(ERROR) << "No matching metadata found.";
  }

  // First codec_cfg with the same name
  const le_audio::CodecConfiguration* codec_cfg = nullptr;
  for (auto i = codec_cfgs->begin(); i != codec_cfgs->end(); ++i) {
    if ((*i)->name()->str() == codec_config_key) {
      codec_cfg = *i;
      break;
    }
  }

  bt_device_qll_local_supported_features_t qll_feature_list_ =
             instance_.GetLocalQLLSupportedFeatures(qll_supported_feat_len_);

  bt_device_soc_add_on_features_t soc_add_on_features_ =
                    instance_.GetSoCAddOnFeatures(soc_add_on_features_len_);

  bool is_dynamic_bn_over_qhs_ =
             QBCE_QLL_BN_VARIATION_BY_QHS_RATE(qll_feature_list_.as_array);
  bool is_dynamic_ft_change_supported_ =
             QBCE_QLL_FT_CHNAGE(qll_feature_list_.as_array);
  bool is_aptx_le_dual_ack_supported_ =
             QBCE_QLL_DUAL_ACK_SUPPORT(qll_feature_list_.as_array);

  if (property_get_bool(
      "persist.vendor.qcom.bluetooth.lossless_aptx_adaptive_le.enabled", false)) {
    LOG(INFO) << ": Extn: Aptx LE codec enabled at target level.";
    is_apx_lossless_le_supported_ = true;
  }

  is_aptx_adaptive_le_supported_ = is_dynamic_bn_over_qhs_ &&
                                   is_dynamic_ft_change_supported_ &&
                                   is_apx_lossless_le_supported_;

  is_aptx_adaptive_le_dual_ack_supported_ =
                                   is_aptx_adaptive_le_supported_ &&
                                   is_aptx_le_dual_ack_supported_;

  char value_xpan_prop[EXTN_PROPERTY_VALUE_MAX] = {'\0'};
  ALOGI("%s: Reading xpan property: adv_transport", __func__);
  property_get("persist.vendor.qcom.bluetooth.adv_transport",
                                             value_xpan_prop, "false");
  if (strcmp(value_xpan_prop, "true") == 0) {
    ALOGI("%s: Extn: XPAN is supported", __func__);
    is_xpan_enabled_locally_ = true;
  } else {
    ALOGI("%s: Extn: XPAN is not supported", __func__);
    is_xpan_enabled_locally_ = false;
  }

  is_aptx_adaptive_lex_supported_ =
       is_dynamic_ft_change_supported_ && is_xpan_enabled_locally_;

  is_enhanced_le_gaming_supported_ =
          QBCE_QLE_HCI_SUPPORTED(soc_add_on_features_.as_array) &&
          is_dynamic_ft_change_supported_ && is_qhs_enabled_locally_;

  LOG(INFO) << __func__
            << ": Extn: FT Changes allowed: " << is_dynamic_ft_change_supported_
            << ", BN Variation allowed: " << is_dynamic_bn_over_qhs_
            << ", Aptx LE Lossless enabled: " << is_apx_lossless_le_supported_
            << ", is_xpan_enabled_locally_" << is_xpan_enabled_locally_
            << ", is_aptx_adaptive_le_dual_ack_supported_: "
            << is_aptx_adaptive_le_dual_ack_supported_;

  LOG(INFO) <<__func__
            << ": Extn: Aptx LE supported: " << is_aptx_adaptive_le_supported_
            << ", is_aptx_adaptive_lex_supported_: " << is_aptx_adaptive_lex_supported_
            << ", Enhanced Gaming supported: " << is_enhanced_le_gaming_supported_;

  // Process each subconfig and put it into the correct list
  if (codec_cfg != nullptr && codec_cfg->subconfigurations()) {
    uint8_t sink_ase_chnl_count = 0;
    uint8_t source_ase_chnl_count = 0;
    /* Load subconfigurations */
    for (auto subconfig : *codec_cfg->subconfigurations()) {
      LOG(DEBUG) << __func__ << ": Vendor codec ID: "
                 << (unsigned)subconfig->codec_id()->vendor_codec_id();
      if (subconfig->codec_id()->vendor_codec_id() == kLeAudioCodingFormatAptxLeX) {
        if (is_aptx_adaptive_lex_supported_) {
          LOG(INFO) << __func__
                    << ": SoC supports Aptx-LeX over LE, continue to populate.";
        } else {
          LOG(INFO) << __func__
                    << ": SoC don't support Aptx-LeX  over LE, skip populating.";
          continue;
        }
      }

      if (subconfig->codec_id()->vendor_codec_id() == kLeAudioCodingFormatAptxLe) {
        if (is_aptx_adaptive_le_supported_) {
          bool dual_ack_supported_json_config = false;
          if (subconfig->direction() == kLeAudioDirectionSink) {
            LOG(DEBUG) << ": vendor_company_id: " << (unsigned)metadata_sink.vendor_company_id;
            if (metadata_sink.vendor_company_id == kLeAudioVendorCompanyIdQcom) {
              LOG(DEBUG) << ": vs_metadata[7]: " << (unsigned)metadata_sink.vs_metadata[7];
              if (metadata_sink.vs_metadata[7] == kLeAudioCodecAptxLeSupportedDualAck) {
                dual_ack_supported_json_config = true;
                LOG(INFO) << __func__ << ": dual ack supported for this config.";
              }
            }
          }

          LOG(INFO) << __func__
                    << ": dual_ack_supported_json_config: " << dual_ack_supported_json_config;
          if (dual_ack_supported_json_config) {
            if (is_aptx_adaptive_le_dual_ack_supported_) {
              LOG(INFO) << __func__
                  << ": SoC supports Aptx-Le dual ack over LE, continue to populate.";
            } else {
              LOG(INFO) << __func__
                << ": SoC don't support Aptx-Le dual ack over LE, skip populating.";
              continue;
            }
          }
          LOG(INFO) << __func__
                    << ": SoC supports Aptx-Le over LE, continue to populate.";
        } else {
          LOG(INFO) << __func__
                    << ": SoC don't support Aptx-Le over LE, skip populating.";
          continue;
        }
      }

      if (subconfig->direction() == kLeAudioDirectionSink) {
        processSubconfig(subconfig, qos_sink_cfg,
                         sinkAseConfiguration, location, configurationFlags,
                         metadata_sink);
        sink_ase_chnl_count = subconfig->ase_channel_cnt();
      } else {
        processSubconfig(subconfig, qos_source_cfg,
                         sourceAseConfiguration, location, configurationFlags,
                         metadata_source);
        source_ase_chnl_count = subconfig->ase_channel_cnt();
      }
    }

    // After putting all subconfig, check if it's an asymmetric configuration
    // and populate information for ConfigurationFlags
    if (!sinkAseConfiguration.empty() && !sourceAseConfiguration.empty()) {
      LOG(DEBUG) << __func__
                 << ": sinkAseConfiguration size: " << sinkAseConfiguration.size()
                 << " sourceAseConfiguration size: " << sourceAseConfiguration.size();
      if (sinkAseConfiguration.size() == sourceAseConfiguration.size()) {
        for (int i = 0; i < sinkAseConfiguration.size(); ++i) {
          if (sinkAseConfiguration[i].has_value() !=
              sourceAseConfiguration[i].has_value()) {
            // Different configuration: one is not empty and other is.
            configurationFlags.bitmask |=
                ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS;
          } else if (sinkAseConfiguration[i].has_value()) {
            // Both is not empty, comparing inner fields:
            if (!isAseConfigurationEqual(sinkAseConfiguration[i].value(),
                                         sourceAseConfiguration[i].value())) {
              configurationFlags.bitmask |=
                  ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS;
            }
          }
        }
      } else {
        /*==========================================================================
        Stereo headset:                     |CSIP headset :
        SInk:                               |SInk:
        Audio channel allocation: L|R ( 3)  |Audio channel allocation: L or R ( 1/2)
        channel count = 2                   |channel count = 1
        no of sink ASEs = 1                 |no of sink ASEs = 1
                                            |
        Src:                                |Src:
        Audio channel allocation: M         |Audio channel allocation: L or R ( 1/2)
        channel count = 1                   |channel count = 1
        no of Src ASEs = 1                  |no of Src ASEs = 1
        ============================================================================
        TWM headset :
        SInk:
        Audio channel allocation: L|R ( 3)
        channel count = 1
        no of sink ASEs = 2

        Src:
        Audio channel allocation: L|R (3)
        channel count = 1
        no of Src ASEs = 2
        ===========================================================================*/

        LOG(DEBUG) << __func__
             << ": sink_ase_chnl_count: " << (unsigned)sink_ase_chnl_count
             << ", source_ase_chnl_count: " << (unsigned)source_ase_chnl_count;
        // Different number of ASE, is a different configuration.
        configurationFlags.bitmask |=
             ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS;

        if ((source_ase_chnl_count == 1) &&
            (sinkAseConfiguration.size() > sourceAseConfiguration.size())) {
          LOG(DEBUG) << __func__ << ": Setting Mono MIC for TWM config";
          configurationFlags.bitmask |=
               ConfigurationFlags::MONO_MIC_CONFIGURATION;
        }
      }
    }
  } else {
    if (codec_cfg == nullptr) {
      LOG(ERROR) << "No codec config matching key " << codec_config_key.c_str()
                 << " found";
    } else {
      LOG(ERROR) << "Configuration '" << flat_cfg->name()->c_str()
                 << "' has no valid subconfigurations.";
    }
  }
}

bool AudioSetConfigurationProviderJson::LoadConfigurationsFromFiles(
    const char* schema_file, const char* content_file, CodecLocation location) {
  flatbuffers::Parser configurations_parser_;
  std::string configurations_schema_binary_content;
  bool ok = flatbuffers::LoadFile(schema_file, true,
                                  &configurations_schema_binary_content);
  LOG(INFO) << __func__ << ": Loading schema file " << schema_file;
  if (!ok) return ok;

  /* Load the binary schema */
  ok = configurations_parser_.Deserialize(
      (uint8_t*)configurations_schema_binary_content.c_str(),
      configurations_schema_binary_content.length());
  if (!ok) return ok;

  /* Load the content from JSON */
  std::string configurations_json_content;
  LOG(INFO) << __func__ << ": Loading json file " << content_file;
  ok = flatbuffers::LoadFile(content_file, false, &configurations_json_content);
  if (!ok) return ok;

  /* Parse */
  LOG(INFO) << __func__ << ": Parse JSON content";
  ok = configurations_parser_.Parse(configurations_json_content.c_str());
  if (!ok) return ok;

  /* Import from flatbuffers */
  LOG(INFO) << __func__ << ": Build flat buffer structure";
  auto configurations_root = le_audio::GetAudioSetConfigurations(
      configurations_parser_.builder_.GetBufferPointer());
  if (!configurations_root) return false;

  auto flat_qos_configs = configurations_root->qos_configurations();
  if ((flat_qos_configs == nullptr) || (flat_qos_configs->size() == 0))
    return false;

  LOG(DEBUG) << ": Updating " << flat_qos_configs->size()
             << " qos config entries.";
  std::vector<const le_audio::QosConfiguration*> qos_cfgs;
  for (auto const& flat_qos_cfg : *flat_qos_configs) {
    qos_cfgs.push_back(flat_qos_cfg);
  }

  auto flat_codec_configs = configurations_root->codec_configurations();
  if ((flat_codec_configs == nullptr) || (flat_codec_configs->size() == 0))
    return false;

  LOG(DEBUG) << ": Updating " << flat_codec_configs->size()
             << " codec config entries.";
  std::vector<const le_audio::CodecConfiguration*> codec_cfgs;
  for (auto const& flat_codec_cfg : *flat_codec_configs) {
    codec_cfgs.push_back(flat_codec_cfg);
  }

  auto flat_configs = configurations_root->configurations();
  if ((flat_configs == nullptr) || (flat_configs->size() == 0)) return false;

  LOG(DEBUG) << ": Updating " << flat_configs->size() << " config entries.";

  auto flat_metadata_configs = configurations_root->codec_specific_metadata();
  if ((flat_metadata_configs == nullptr) || (flat_metadata_configs->size() == 0))
    return false;

  LOG(DEBUG) << ": Updating " << flat_codec_configs->size()
             << " metadata config entries.";
  std::vector<const le_audio::CodecSpecifcMetadata*> metadata_cfgs;
  for (auto const& flat_metadata_cfg : *flat_metadata_configs) {
    metadata_cfgs.push_back(flat_metadata_cfg);
  }

  for (auto const& flat_cfg : *flat_configs) {
    // Create 3 vector to use
    std::vector<std::optional<AseDirectionConfiguration>> sourceAseConfiguration;
    std::vector<std::optional<AseDirectionConfiguration>> sinkAseConfiguration;
    ConfigurationFlags configurationFlags;
    PopulateAseConfigurationFromFlat(flat_cfg, &codec_cfgs, &qos_cfgs, location,
                                     sourceAseConfiguration, sinkAseConfiguration,
                                     configurationFlags, &metadata_cfgs);
    if (sourceAseConfiguration.empty() && sinkAseConfiguration.empty())
      continue;
    configurations_[flat_cfg->name()->str()] = std::make_tuple(
        sourceAseConfiguration, sinkAseConfiguration, configurationFlags);
  }

  return true;
}

bool AudioSetConfigurationProviderJson::LoadScenariosFromFiles(
    const char* schema_file, const char* content_file) {
  flatbuffers::Parser scenarios_parser_;
  std::string scenarios_schema_binary_content;
  bool ok = flatbuffers::LoadFile(schema_file, true,
                                  &scenarios_schema_binary_content);
  LOG(INFO) << __func__ << ": Loading file " << schema_file;
  if (!ok) return ok;

  /* Load the binary schema */
  ok = scenarios_parser_.Deserialize(
      (uint8_t*)scenarios_schema_binary_content.c_str(),
      scenarios_schema_binary_content.length());
  if (!ok) return ok;

  /* Load the content from JSON */
  LOG(INFO) << __func__ << ": Loading file " << content_file;
  std::string scenarios_json_content;
  ok = flatbuffers::LoadFile(content_file, false, &scenarios_json_content);
  if (!ok) return ok;

  /* Parse */
  LOG(INFO) << __func__ << ": Parse json content";
  ok = scenarios_parser_.Parse(scenarios_json_content.c_str());
  if (!ok) return ok;

  /* Import from flatbuffers */
  LOG(INFO) << __func__ << ": Build flat buffer structure";
  auto scenarios_root = le_audio::GetAudioSetScenarios(
      scenarios_parser_.builder_.GetBufferPointer());
  if (!scenarios_root) return false;

  auto flat_scenarios = scenarios_root->scenarios();
  if ((flat_scenarios == nullptr) || (flat_scenarios->size() == 0))
    return false;

  LOG(INFO) << __func__ << ": Turn flat buffer into structure";
  AudioContext media_context = AudioContext();
  media_context.bitmask =
      (AudioContext::ALERTS | AudioContext::INSTRUCTIONAL |
       AudioContext::NOTIFICATIONS | AudioContext::EMERGENCY_ALARM |
       AudioContext::UNSPECIFIED | AudioContext::MEDIA |
       AudioContext::SOUND_EFFECTS);

  AudioContext conversational_context = AudioContext();
  conversational_context.bitmask =
      (AudioContext::RINGTONE_ALERTS | AudioContext::CONVERSATIONAL);

  AudioContext live_context = AudioContext();
  live_context.bitmask = AudioContext::LIVE_AUDIO;

  AudioContext game_context = AudioContext();
  game_context.bitmask = AudioContext::GAME;

  AudioContext voice_assistants_context = AudioContext();
  voice_assistants_context.bitmask = AudioContext::VOICE_ASSISTANTS;

  bool row_config_ =
    property_get_bool("persist.vendor.qcom.bluetooth.is_row_config", true);
  bool voice_HD_ =
    property_get_bool("persist.vendor.qcom.bluetooth.is_voice_HD", false);
  bool is_remote_mora_ =
    property_get_bool("persist.vendor.qcom.bluetooth.is_remote_mora", true);
  bool gmap_particular_conf =
    property_get_bool("persist.vendor.qcom.bluetooth.gmap_particular_conf", false);

  LOG(DEBUG) << ": row_config_: " << row_config_
             << ", voice_HD_: " << voice_HD_
             << ", is_remote_mora_: " << is_remote_mora_;

  LOG(DEBUG) << "Updating " << flat_scenarios->size() << " scenarios.";
  for (auto const& scenario : *flat_scenarios) {
    if (!scenario->configurations()) continue;
    std::string scenario_name = scenario->name()->c_str();
    AudioContext context;
    if (scenario_name == "Media")
      context = AudioContext(media_context);
    else if (scenario_name == "Conversational")
      context = AudioContext(conversational_context);
    else if (scenario_name == "Live")
      context = AudioContext(live_context);
    else if (scenario_name == "Game")
      context = AudioContext(game_context);
    else if (scenario_name == "VoiceAssistants")
      context = AudioContext(voice_assistants_context);
    LOG(DEBUG) << "Scenario " << scenario->name()->c_str()
               << " configs: " << scenario->configurations()->size()
               << " context: " << context.toString();

    for (auto it = scenario->configurations()->begin();
         it != scenario->configurations()->end(); ++it) {
      auto config_name = it->str();

      if(!row_config_ && !is_remote_mora_ && (config_name ==
         "VND_Two-OneChan-SnkAse-Lc3_48_4_120octs_R5_L20-One-OneChan-SrcAse-Lc3_16_2_40octs_R5_L20")) {
        LOG(DEBUG) << ": skip pushing mora config for game";
        continue;
      }

      if (!row_config_ && voice_HD_ && (config_name ==
          "Two-OneChan-SnkAse-Lc3_16_2_40octs_R4_L10-Two-OneChan-SrcAse-Lc3_16_2_40octs_R4_L10")) {
        LOG(DEBUG) << ": Skip pushing non-HD Voice";
        continue;
      }

      /* This is specifically for GMAP/UGG/LLU/BV-103-C, we don't need such logic for other TCs
         This can also be used when someone wants to select particular config only for GMAP */
      if(gmap_particular_conf && config_name !=
         "Two-OneChan-SnkAse-Lc3_48_1-One-OneChan-SrcAse-Lc3_16_1_Low_Latency") {
        LOG(DEBUG) << ": Ignoring the config since we need particular config for GMAP";
        continue;
      }

      auto configuration = configurations_.find(config_name);
      if (configuration == configurations_.end()) continue;
      LOG(DEBUG) << "Getting configuration with name: " << config_name;
      auto [source, sink, flags] = configuration->second;
      // Each configuration will create a LeAudioAseConfigurationSetting
      // with the same {context, packing}
      // and different data
      LeAudioAseConfigurationSetting setting;
      setting.audioContext = context;
      //Paking to be checked
      setting.packing = IBluetoothAudioProvider::Packing::INTERLEAVED;
      setting.sourceAseConfiguration = source;
      setting.sinkAseConfiguration = sink;
      setting.flags = flags;
      // Add to list of setting
      LOG(DEBUG) << "Pushing configuration to list: " << config_name;
      ase_configuration_settings_.push_back({config_name, setting});
    }
  }

  return true;
}

bool AudioSetConfigurationProviderJson::LoadContent(
    std::vector<std::pair<const char* /*schema*/, const char* /*content*/>>
        config_files,
    std::vector<std::pair<const char* /*schema*/, const char* /*content*/>>
        scenario_files,
    CodecLocation location) {
  bool is_loaded_config = false;
  for (auto [schema, content] : config_files) {
    std::string s_config = content;
    LOG(INFO) << __func__ << ": json config file name " << content;
    if (s_config.find("qti") != std::string::npos) {
      LOG(INFO) << __func__ << ": qti json config file found.";
    } else {
      LOG(INFO) << __func__ << ": qti json config file not found, continue.";
      continue;
    }

    if (LoadConfigurationsFromFiles(schema, content, location)) {
      is_loaded_config = true;
      break;
    }
  }

  bool is_loaded_scenario = false;
  for (auto [schema, content] : scenario_files) {
    std::string s_scenarios = content;
    LOG(INFO) << __func__ << ": json scenarios file name " << content;
    if (s_scenarios.find("qti") != std::string::npos) {
      LOG(INFO) << __func__ << ": qti json scenarios file found.";
    } else {
      LOG(INFO) << __func__ << ": qti json scenarios file not found, continue";
      continue;
    }

    if (LoadScenariosFromFiles(schema, content)) {
      is_loaded_scenario = true;
      break;
    }
  }
  return is_loaded_config && is_loaded_scenario;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
