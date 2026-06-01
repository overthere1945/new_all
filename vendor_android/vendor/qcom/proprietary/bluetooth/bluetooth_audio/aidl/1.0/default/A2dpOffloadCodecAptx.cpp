/*
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a contribution.
 *
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "A2dpOffloadCodecAptx"

#include <algorithm>
#include <android-base/logging.h>

#include "A2dpBits.h"
#include "A2dpOffloadCodecAptx.h"

namespace aidl::android::hardware::bluetooth::audio {

/**
 * APTX Local Capabilities
 */

enum : bool {
  kEnableSamplingFrequency44100 = true,
  kEnableSamplingFrequency48000 = true
};

enum : bool {
  kEnableChannelModeMono = false,
  kEnableChannelModeStereo = true
};

enum : int {
  kBitdepth = 16,
};

/**
 * Aptx Signaling format Vendor Specific Value of [A2DP - 4.7 Vendor Specific A2DP Codec]
 */

// clang-format off

constexpr A2dpBits::Range kSamplingFrequency (  2,  3 );
constexpr A2dpBits::Range kChannelMode       (  6,  7 );
constexpr size_t kCapabilitiesSize = 8/8;

// clang-format on

enum {
  kSamplingFrequency44100 = kSamplingFrequency.first,
  kSamplingFrequency48000
};

enum {
  kChannelModeStereo = kChannelMode.first,
  kChannelModeMono
};

/**
 * Aptx Conversion functions
 */

static int GetSamplingFrequencyBit(int32_t sampling_frequency) {
  switch (sampling_frequency) {
    case 44100:
      return kSamplingFrequency44100;
    case 48000:
      return kSamplingFrequency48000;
    default:
      return -1;
  }
}

static int32_t GetSamplingFrequencyValue(int sampling_frequency) {
  switch (sampling_frequency) {
    case kSamplingFrequency44100:
      return 44100;
    case kSamplingFrequency48000:
      return 48000;
    default:
      return 0;
  }
}

static int GetChannelModeBit(ChannelMode channel_mode) {
  switch (channel_mode) {
    case ChannelMode::STEREO:
      return kChannelModeStereo;
    case ChannelMode::MONO:
      return kChannelModeMono;
    default:
      return -1;
  }
}

static ChannelMode GetChannelModeEnum(int channel_mode) {
  switch (channel_mode) {
    case kChannelModeMono:
      return ChannelMode::MONO;
    case kChannelModeStereo:
      return ChannelMode::STEREO;
    default:
      return ChannelMode::UNKNOWN;
  }
}

static int32_t GetSamplingFrequencyValue(const A2dpBits& configuration) {
  return GetSamplingFrequencyValue(
      configuration.find_active_bit(kSamplingFrequency));
}

static int GetBitrate(const A2dpBits& configuration) {
  return 0;
}

/**
 * APTX Class implementation
 */

A2dpOffloadCodecAptx::A2dpOffloadCodecAptx()
    : A2dpOffloadCodec(info_),
      info_({.name = "APTX"}) {
  auto Vendor_Aptx = CodecId::Vendor();
  Vendor_Aptx.codecId = A2DP_APTX_CODEC_ID_BLUETOOTH;
  Vendor_Aptx.id = A2DP_APTX_VENDOR_ID;
  info_.id = Vendor_Aptx;
  info_.transport.set<CodecInfo::Transport::Tag::a2dp>();
  auto& a2dp_info = info_.transport.get<CodecInfo::Transport::Tag::a2dp>();

  /* --- Setup Capabilities --- */

  a2dp_info.capabilities.resize(kCapabilitiesSize);
  std::fill(begin(a2dp_info.capabilities), end(a2dp_info.capabilities), 0);

  auto capabilities = A2dpBits(a2dp_info.capabilities);

  capabilities.set(kSamplingFrequency44100, kEnableSamplingFrequency44100);
  capabilities.set(kSamplingFrequency48000, kEnableSamplingFrequency48000);

  capabilities.set(kChannelModeMono, kEnableChannelModeMono);
  capabilities.set(kChannelModeStereo, kEnableChannelModeStereo);

  /* --- Setup Sampling Frequencies --- */

  auto& sampling_frequency = a2dp_info.samplingFrequencyHz;

  for (auto v : {44100, 48000})
    if (capabilities.get(GetSamplingFrequencyBit(int32_t(v))))
      sampling_frequency.push_back(v);

  /* --- Setup Channel Modes --- */

  auto& channel_modes = a2dp_info.channelMode;

  for (auto v : {ChannelMode::MONO, ChannelMode::STEREO})
    if (capabilities.get(GetChannelModeBit(v))) channel_modes.push_back(v);

  /* --- Setup Bitdepth --- */

  a2dp_info.bitdepth.push_back(kBitdepth);
}

A2dpStatus A2dpOffloadCodecAptx::ParseConfiguration(
    const std::vector<uint8_t>& configuration,
    CodecParameters* codec_parameters, AptxParameters* aptx_parameters) const {
  auto& a2dp_info = info.transport.get<CodecInfo::Transport::Tag::a2dp>();

  if (configuration.size() != a2dp_info.capabilities.size())
    return A2dpStatus::BAD_LENGTH;

  auto config = A2dpBits(configuration);
  auto lcaps = A2dpBits(a2dp_info.capabilities);

  /* --- Check Sampling Frequency --- */

  int sampling_frequency = config.find_active_bit(kSamplingFrequency);
  if (sampling_frequency < 0) return A2dpStatus::INVALID_SAMPLING_FREQUENCY;
  if (!lcaps.get(sampling_frequency))
    return A2dpStatus::NOT_SUPPORTED_SAMPLING_FREQUENCY;

  /* --- Check Channel Mode --- */

  int channel_mode = config.find_active_bit(kChannelMode);
  if (channel_mode < 0) return A2dpStatus::INVALID_CHANNEL_MODE;
  if (!lcaps.get(channel_mode)) return A2dpStatus::NOT_SUPPORTED_CHANNEL_MODE;

  /* --- Return --- */

  codec_parameters->channelMode = GetChannelModeEnum(channel_mode);
  codec_parameters->samplingFrequencyHz =
      GetSamplingFrequencyValue(sampling_frequency);
  codec_parameters->bitdepth = kBitdepth;

  codec_parameters->vendorSpecificParameters = {}; // TO-DO

  if (aptx_parameters) {
  }

  return A2dpStatus::OK;
}

bool A2dpOffloadCodecAptx::BuildConfiguration(
    const std::vector<uint8_t>& remote_capabilities,
    const std::optional<CodecParameters>& hint,
    const AudioContext audio_context,
    std::vector<uint8_t>* configuration) const {
  auto& a2dp_info = info.transport.get<CodecInfo::Transport::Tag::a2dp>();

  LOG(INFO) << "APTX Local Caps: ";
  for (auto local : a2dp_info.capabilities)
    LOG(INFO) << static_cast<int>(local);

  LOG(INFO) << "APTX Remote Caps: ";
  for (auto remote : remote_capabilities)
    LOG(INFO) << static_cast<int>(remote);

  if (remote_capabilities.size() != a2dp_info.capabilities.size()) return false;

  auto lcaps = A2dpBits(a2dp_info.capabilities);
  auto rcaps = A2dpBits(remote_capabilities);

  configuration->resize(a2dp_info.capabilities.size());
  std::fill(begin(*configuration), end(*configuration), 0);
  auto config = A2dpBits(*configuration);

  /* --- Select Sampling Frequency --- */

  auto sf_hint = hint ? GetSamplingFrequencyBit(hint->samplingFrequencyHz) : -1;

  if (sf_hint >= 0 && lcaps.get(sf_hint) && rcaps.get(sf_hint))
    config.set(sf_hint);
  else if (lcaps.get(kSamplingFrequency48000) &&
           rcaps.get(kSamplingFrequency48000))
    config.set(kSamplingFrequency48000);
  else if (lcaps.get(kSamplingFrequency44100) &&
           rcaps.get(kSamplingFrequency44100))
    config.set(kSamplingFrequency44100);
  else {
    LOG(INFO) << ": Unable to Set APTX frequency";
    return false;
  }

  /* --- Select Channel Mode --- */

  auto cm_hint = hint ? GetChannelModeBit(hint->channelMode) : -1;

  if (cm_hint >= 0 && lcaps.get(cm_hint) && rcaps.get(cm_hint))
    config.set(cm_hint);
  else if (lcaps.get(kChannelModeStereo) && rcaps.get(kChannelModeStereo))
    config.set(kChannelModeStereo);
  else if (lcaps.get(kChannelModeMono) && rcaps.get(kChannelModeMono))
    config.set(kChannelModeMono);
  else {
    LOG(INFO) << ": Unable to Set APTX Channel Mode";
    return false;
  }

  return true;
}

}  // namespace aidl::android::hardware::bluetooth::audio
