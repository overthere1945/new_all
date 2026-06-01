/*
*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* All rights reserved.
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

#define LOG_TAG "A2dpOffloadCodecAac"

#include <algorithm>
#include <android-base/logging.h>

#include "A2dpBits.h"
#include "A2dpOffloadCodecAac.h"

namespace aidl::android::hardware::bluetooth::audio {

/**
 * AAC Local Capabilities
 */

enum : bool {
  kEnableObjectTypeMpeg2AacLc = true,
  kEnableObjectTypeMpeg4AacLc = true,
};

enum : bool {
  kEnableSamplingFrequency44100 = true,
  kEnableSamplingFrequency48000 = false,
  kEnableSamplingFrequency88200 = false,
  kEnableSamplingFrequency96000 = false,
};

enum : bool {
  kEnableChannels1 = false,
  kEnableChannels2 = true,
};

enum : bool {
  kEnableVbrSupported = true,
};

enum : int {
  kBitrateAac = 165000,
};

enum : int {
  kBitdepth = 16,
};

/**
 * AAC Signaling format [A2DP - 4.5]
 */

// clang-format off

constexpr A2dpBits::Range kObjectType        (  0,  6 );
constexpr A2dpBits::Range kDrcEnable         (  7     );
constexpr A2dpBits::Range kSamplingFrequency (  8, 19 );
constexpr A2dpBits::Range kChannels          ( 20, 23 );
constexpr A2dpBits::Range kVbrSupported      ( 24     );
constexpr A2dpBits::Range kBitrate           ( 25, 47 );
constexpr size_t kCapabilitiesSize = 48/8;

// clang-format on

enum {
  kObjectTypeMpeg2AacLc = kObjectType.first,
  kObjectTypeMpeg4AacLc,
  kObjectTypeMpeg4AacLtp,
  kObjectTypeMpeg4AacScalable,
  kObjectTypeMpeg4AacHeV1,
  kObjectTypeMpeg4AacHeV2,
  kObjectTypeMpeg4AacEldV2
};

enum {
  kSamplingFrequency8000 = kSamplingFrequency.first,
  kSamplingFrequency11025,
  kSamplingFrequency12000,
  kSamplingFrequency16000,
  kSamplingFrequency22050,
  kSamplingFrequency24000,
  kSamplingFrequency32000,
  kSamplingFrequency44100,
  kSamplingFrequency48000,
  kSamplingFrequency64000,
  kSamplingFrequency88200,
  kSamplingFrequency96000
};

enum { kChannels1 = kChannels.first, kChannels2, kChannels51, kChannels71 };

/**
 * AAC Conversion functions
 */

static AacParameters::ObjectType GetObjectTypeEnum(int object_type) {
  switch (object_type) {
    case kObjectTypeMpeg2AacLc:
      return AacParameters::ObjectType::MPEG2_AAC_LC;
    case kObjectTypeMpeg4AacLc:
    default:
      return AacParameters::ObjectType::MPEG4_AAC_LC;
  }
}

static int GetSamplingFrequencyBit(int32_t sampling_frequency) {
  switch (sampling_frequency) {
    case 8000:
      return kSamplingFrequency8000;
    case 11025:
      return kSamplingFrequency11025;
    case 12000:
      return kSamplingFrequency12000;
    case 16000:
      return kSamplingFrequency16000;
    case 22050:
      return kSamplingFrequency22050;
    case 24000:
      return kSamplingFrequency24000;
    case 32000:
      return kSamplingFrequency32000;
    case 44100:
      return kSamplingFrequency44100;
    case 48000:
      return kSamplingFrequency48000;
    case 64000:
      return kSamplingFrequency64000;
    case 88200:
      return kSamplingFrequency88200;
    case 96000:
      return kSamplingFrequency96000;
    default:
      return -1;
  }
}

static int32_t GetSamplingFrequencyValue(int sampling_frequency) {
  switch (sampling_frequency) {
    case kSamplingFrequency8000:
      return 8000;
    case kSamplingFrequency11025:
      return 11025;
    case kSamplingFrequency12000:
      return 12000;
    case kSamplingFrequency16000:
      return 16000;
    case kSamplingFrequency22050:
      return 22050;
    case kSamplingFrequency24000:
      return 24000;
    case kSamplingFrequency32000:
      return 32000;
    case kSamplingFrequency44100:
      return 44100;
    case kSamplingFrequency48000:
      return 48000;
    case kSamplingFrequency64000:
      return 64000;
    case kSamplingFrequency88200:
      return 88200;
    case kSamplingFrequency96000:
      return 96000;
    default:
      return 0;
  }
}

static int GetChannelsBit(ChannelMode channel_mode) {
  switch (channel_mode) {
    case ChannelMode::MONO:
      return kChannels1;
    case ChannelMode::STEREO:
      return kChannels2;
    default:
      return -1;
  }
}

static ChannelMode GetChannelModeEnum(int channel_mode) {
  switch (channel_mode) {
    case kChannels1:
      return ChannelMode::MONO;
    case kChannels2:
      return ChannelMode::STEREO;
    default:
      return ChannelMode::UNKNOWN;
  }
}

/**
 * AAC Class implementation
 */

A2dpOffloadCodecAac::A2dpOffloadCodecAac()
    : A2dpOffloadCodec(info_),
      info_({.id = CodecId(CodecId::A2dp::AAC), .name = "AAC"}) {
  info_.transport.set<CodecInfo::Transport::Tag::a2dp>();
  auto& a2dp_info = info_.transport.get<CodecInfo::Transport::Tag::a2dp>();

  /* --- Setup Capabilities --- */

  a2dp_info.capabilities.resize(kCapabilitiesSize);
  std::fill(begin(a2dp_info.capabilities), end(a2dp_info.capabilities), 0);

  auto capabilities = A2dpBits(a2dp_info.capabilities);

  capabilities.set(kObjectTypeMpeg2AacLc, kEnableObjectTypeMpeg2AacLc);
  capabilities.set(kObjectTypeMpeg4AacLc, kEnableObjectTypeMpeg4AacLc);

  capabilities.set(kSamplingFrequency44100, kEnableSamplingFrequency44100);

  capabilities.set(kChannels1, kEnableChannels1);
  capabilities.set(kChannels2, kEnableChannels2);

  capabilities.set(kVbrSupported, kEnableVbrSupported);

  capabilities.set(kBitrate, kBitrateAac);

  /* --- Setup Sampling Frequencies --- */

  auto& sampling_frequency = a2dp_info.samplingFrequencyHz;

  for (auto v : {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
                 64000, 88200, 96000})
    if (capabilities.get(GetSamplingFrequencyBit(int32_t(v))))
      sampling_frequency.push_back(v);

  /* --- Setup Channel Modes --- */

  auto& channel_modes = a2dp_info.channelMode;

  for (auto v : {ChannelMode::MONO, ChannelMode::STEREO})
    if (capabilities.get(GetChannelsBit(v))) channel_modes.push_back(v);

  /* --- Setup Bitdepth --- */

  a2dp_info.bitdepth.push_back(kBitdepth);
}

A2dpStatus A2dpOffloadCodecAac::ParseConfiguration(
    const std::vector<uint8_t>& configuration,
    CodecParameters* codec_parameters, AacParameters* aac_parameters) const {
  auto& a2dp_info = info.transport.get<CodecInfo::Transport::Tag::a2dp>();

  if (configuration.size() != a2dp_info.capabilities.size())
    return A2dpStatus::BAD_LENGTH;

  auto config = A2dpBits(configuration);
  auto lcaps = A2dpBits(a2dp_info.capabilities);

  /* --- Check Object Type --- */

  int object_type = config.find_active_bit(kObjectType);
  if (object_type < 0) return A2dpStatus::INVALID_OBJECT_TYPE;
  if (!lcaps.get(object_type)) return A2dpStatus::NOT_SUPPORTED_OBJECT_TYPE;

  /* --- Check Sampling Frequency --- */

  int sampling_frequency = config.find_active_bit(kSamplingFrequency);
  if (sampling_frequency < 0) return A2dpStatus::INVALID_SAMPLING_FREQUENCY;
  if (!lcaps.get(sampling_frequency))
    return A2dpStatus::NOT_SUPPORTED_SAMPLING_FREQUENCY;

  /* --- Check Channels --- */

  int channels = config.find_active_bit(kChannels);
  if (channels < 0) return A2dpStatus::INVALID_CHANNELS;
  if (!lcaps.get(channels)) return A2dpStatus::NOT_SUPPORTED_CHANNELS;

  /* --- Check Bitrate --- */

  bool vbr = config.get(kVbrSupported);
  if (vbr && !lcaps.get(kVbrSupported)) return A2dpStatus::NOT_SUPPORTED_VBR;

  int bitrate = config.get(kBitrate);
  if (vbr && lcaps.get(kBitrate) && bitrate > lcaps.get(kBitrate))
    return A2dpStatus::NOT_SUPPORTED_BIT_RATE;

  /* --- Return --- */

  codec_parameters->channelMode = GetChannelModeEnum(channels);
  codec_parameters->samplingFrequencyHz =
      GetSamplingFrequencyValue(sampling_frequency);
  codec_parameters->bitdepth = kBitdepth;

  codec_parameters->minBitrate = vbr ? 0 : bitrate;
  codec_parameters->maxBitrate = bitrate;

  codec_parameters->vendorSpecificParameters = {}; // TO-DO

  if (aac_parameters)
    aac_parameters->object_type = GetObjectTypeEnum(object_type);

  return A2dpStatus::OK;
}

bool A2dpOffloadCodecAac::BuildConfiguration(
    const std::vector<uint8_t>& remote_capabilities,
    const std::optional<CodecParameters>& hint,
    const AudioContext audio_context,
    std::vector<uint8_t>* configuration) const {
  auto& a2dp_info = info_.transport.get<CodecInfo::Transport::Tag::a2dp>();

  LOG(INFO) << "AAC Local Caps: ";
  for (auto local : a2dp_info.capabilities)
    LOG(INFO) << static_cast<int>(local);

  LOG(INFO) << "AAC Remote Caps: ";
  for (auto remote : remote_capabilities)
    LOG(INFO) << static_cast<int>(remote);

  if (remote_capabilities.size() != a2dp_info.capabilities.size()) return false;

  auto lcaps = A2dpBits(a2dp_info.capabilities);
  auto rcaps = A2dpBits(remote_capabilities);

  configuration->resize(a2dp_info.capabilities.size());
  std::fill(begin(*configuration), end(*configuration), 0);
  auto config = A2dpBits(*configuration);

  /* --- Select Object Type --- */

  if (lcaps.get(kObjectTypeMpeg2AacLc) && rcaps.get(kObjectTypeMpeg2AacLc))
    config.set(kObjectTypeMpeg2AacLc);
  else if (lcaps.get(kObjectTypeMpeg4AacLc) && rcaps.get(kObjectTypeMpeg4AacLc))
    config.set(kObjectTypeMpeg4AacLc);
  else {
    LOG(INFO) << ": Unable to Set AAC Object Type:";
    return false;
  }

  /* --- Select Sampling Frequency --- */

  LOG(INFO) << "Selecting the sampling frequency";
  auto sf_hint = hint ? GetSamplingFrequencyBit(hint->samplingFrequencyHz) : -1;

  if (sf_hint >= 0 && lcaps.get(sf_hint) && rcaps.get(sf_hint))
    config.set(sf_hint);
  else if (lcaps.get(kSamplingFrequency44100) &&
           rcaps.get(kSamplingFrequency44100))
    config.set(kSamplingFrequency44100);
  else {
    LOG(INFO) << ": Unable to Set AAC frequency";
    return false;
  }

  /* --- Select Channels --- */

  auto ch_hint = hint ? GetChannelsBit(hint->channelMode) : -1;

  if (ch_hint >= 0 && lcaps.get(ch_hint) && rcaps.get(ch_hint))
    config.set(ch_hint);
  else if (lcaps.get(kChannels2) && rcaps.get(kChannels2))
    config.set(kChannels2);
  else if (lcaps.get(kChannels1) && rcaps.get(kChannels1))
    config.set(kChannels1);
  else {
    LOG(INFO) << ": Unable to Set AAC channel mode";
    return false;
  }

  /* --- Select Bitrate --- */

  if (!hint || hint->minBitrate == 0)
    config.set(kVbrSupported,
               lcaps.get(kVbrSupported) && rcaps.get(kVbrSupported));

  int32_t bitrate = lcaps.get(kBitrate);
  if (hint && hint->maxBitrate > 0 && bitrate)
    bitrate = std::min(hint->maxBitrate, bitrate);
  else if (hint && hint->maxBitrate > 0)
    bitrate = hint->maxBitrate;
  config.set(kBitrate, bitrate);

  return true;
}

}  // namespace aidl::android::hardware::bluetooth::audio
