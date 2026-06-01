/******************************************************************************

    Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
    All rights reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.

    Copyright (c) 2018, The Linux Foundation. All rights reserved.
    Redistribution and use in source and binary forms, with or without
    modification, are permitted (subject to the limitations in the
    disclaimer below) provided that the following conditions are met:

       * Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.

       * Redistributions in binary form must reproduce the above
         copyright notice, this list of conditions and the following
         disclaimer in the documentation and/or other materials provided
         with the distribution.

       * Neither the name of The Linux Foundation nor the names of its
         contributors may be used to endorse or promote products derived
         from this software without specific prior written permission.

    NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
    GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
    HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
    IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
    OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
    IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#define LOG_TAG "A2dpOffloadCodecAptxAdaptive"
#define HCI_SPLIT_A2DP_SOURCE_Tx_Split_APTX_ADAPTIVE_SUPPORTED(x) ((x)[3] & 0x80)

#include <algorithm>
#include <android-base/logging.h>
#include <cutils/properties.h>

#include "A2dpBits.h"
#include "A2dpOffloadCodecAptxAdaptive.h"

#include "btavs_client.h"

namespace aidl::android::hardware::bluetooth::audio {

/**
 * APTX ADAPTIVE Local Capabilities
 */

enum : bool {
  kEnableSamplingFrequency44100 = true,
  kEnableSamplingFrequency48000 = true,
  kEnableSamplingFrequency96000 = true,
};

enum : bool {
  kEnableChannelModeTwsPlus = true,
  kEnableChannelModeTwsMono = true,
  kEnableChannelModeJointStereo = true,
  kEnableChannelModeTwsStereo = true,
  kEnableChannelModeStereo = true,
  kEnableChannelModeMono = false,
};

enum : int {
  kBitdepth = 24,
};

enum : int {
  kTtpLowLatency0 = 80,
  kTtpLowLatency1 = 100,
  kTtpHighQuality0 = 100,
  kTtpHighQuality1 = 100,
  kTtpTws0 = 255,
  kTtpTws1 = 255,
};

enum : int {
  kCapVersion = 1,
};

enum : int {
  kCodecFeaturesV20 = 0x0000000F,
  kCodecFeaturesV21 = 0x0300000F,
  kCodecFeaturesV22 = 0x1700000F,
};

enum : int {
  kSetUpPreference1 = 2,
  kSetUpPreference2 = 3,
  kSetUpPreference3 = 3,
  kSetUpPreference4 = 3,
};

enum : int {
  kEoC = 0x00AA,
};

/**
 * Aptx Adaptive Signaling format Vendor Specific Value of [A2DP - 4.7 Vendor Specific A2DP Codec]
 */

// clang-format off

constexpr A2dpBits::Range kSamplingFrequency   (  0,  3 );
constexpr A2dpBits::Range kChannelMode         ( 10, 15 );
constexpr A2dpBits::Range kTtpLL0              ( 16, 23 );
constexpr A2dpBits::Range kTtpLL1              ( 24, 31 );
constexpr A2dpBits::Range kTtpHQ0              ( 32, 39 );
constexpr A2dpBits::Range kTtpHQ1              ( 40, 47 );
constexpr A2dpBits::Range kTtpTWS0             ( 48, 55 );
constexpr A2dpBits::Range kTtpTWS1             ( 56, 63 );
constexpr A2dpBits::Range kCapExtVersion       ( 72, 79 );
constexpr A2dpBits::Range kSupportedFeatures   ( 80, 111);
constexpr A2dpBits::Range kSetUpPref1          (112, 119);
constexpr A2dpBits::Range kSetUpPref2          (120, 127);
constexpr A2dpBits::Range kSetUpPref3          (128, 135);
constexpr A2dpBits::Range kSetUpPref4          (136, 143);
constexpr A2dpBits::Range kEndOfCapabilities   (144, 159);

// Below is EoC for Aptx Adaptive R1
constexpr A2dpBits::Range kEndOfCapabilitiesV1 ( 80, 95 );
constexpr size_t kCapabilitiesSize = 272/8;
// size 34
// clang-format on

enum {
  kSamplingFrequency96000 = kSamplingFrequency.first,
  kSamplingFrequency44100,
  kSamplingFrequency96000_1,
  kSamplingFrequency48000,
};

enum {
  kChannelModeTwsPlus = kChannelMode.first,
  kChannelModeTwsMono,
  kChannelModeJointStereo,
  kChannelModeTwsStereo,
  kChannelModeStereo,
  kChannelModeMono,
};

/**
 * Aptx Adaptive Conversion functions
 */

static int GetSamplingFrequencyBit(int32_t sampling_frequency) {
  switch (sampling_frequency) {
    case 44100:
      return kSamplingFrequency44100;
    case 48000:
      return kSamplingFrequency48000;
    case 96000:
      return kSamplingFrequency96000;
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
    case kSamplingFrequency96000:
    case kSamplingFrequency96000_1:
      return 96000;
    default:
      return 0;
  }
}

static int GetChannelModeBit(ChannelMode channel_mode) {
  switch (channel_mode) {
    case ChannelMode::STEREO:
      return kChannelModeStereo|kChannelModeJointStereo;
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
    case kChannelModeJointStereo:
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

static bool isAptxAdaptiveSplitTxSupported() {
  uint8_t soc_add_on_features_len_ = 0;
  BtAVsClient& instance_ = BtAVsClient::GetInstance();
  bool is_split_tx_enabled = false;
  bt_device_soc_add_on_features_t soc_add_on_features_ =
                    instance_.GetSoCAddOnFeatures(soc_add_on_features_len_);
  LOG(INFO) << "SoC Add On Features Length " << soc_add_on_features_len_;
  is_split_tx_enabled =
      HCI_SPLIT_A2DP_SOURCE_Tx_Split_APTX_ADAPTIVE_SUPPORTED(soc_add_on_features_.as_array);
  LOG(INFO) << "APTX-ADAPTIVE Split Tx Support " << is_split_tx_enabled;
  return is_split_tx_enabled;
}

static bool isAptxAdaptive2_1_Supported() {
  char adaptive_value[255] = {'\0'};
  property_get("persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support",
                                                            adaptive_value, "false");
  if (!(strcmp(adaptive_value,"true"))) {
    LOG(INFO) << "Aptx-adaptive R2.1 is supported";
    return true;
  }
  return false;
}

static bool isAptxAdaptive2_2_Supported() {
  char adaptive_value[255] = {'\0'};
  property_get("persist.vendor.qcom.bluetooth.aptxadaptiver2_2_support",
                                                            adaptive_value, "false");
  if (!(strcmp(adaptive_value,"true"))) {
    LOG(INFO) << "Aptx-adaptive R2.2 is supported";
    return true;
  }
  return false;
}

/**
 * APTX ADAPTIVE Class implementation
 */

A2dpOffloadCodecAptxAdaptive::A2dpOffloadCodecAptxAdaptive()
    : A2dpOffloadCodec(info_),
      info_({.name = "APTX-ADAPTIVE"}) {
  auto Vendor_AptxAdaptive = CodecId::Vendor();
  Vendor_AptxAdaptive.codecId = A2DP_APTX_ADAPTIVE_CODEC_ID_BLUETOOTH;
  Vendor_AptxAdaptive.id = A2DP_APTX_ADAPTIVE_VENDOR_ID;
  info_.id = Vendor_AptxAdaptive;
  info_.transport.set<CodecInfo::Transport::Tag::a2dp>();
  auto& a2dp_info = info_.transport.get<CodecInfo::Transport::Tag::a2dp>();

  /* --- Setup Capabilities --- */

  a2dp_info.capabilities.resize(kCapabilitiesSize);
  std::fill(begin(a2dp_info.capabilities), end(a2dp_info.capabilities), 0);

  auto capabilities = A2dpBits(a2dp_info.capabilities);

  if (isAptxAdaptive2_2_Supported())
    capabilities.set(kSamplingFrequency44100, kEnableSamplingFrequency44100);
  capabilities.set(kSamplingFrequency48000, kEnableSamplingFrequency48000);
  if (isAptxAdaptive2_2_Supported() || isAptxAdaptive2_1_Supported()) {
    capabilities.set(kSamplingFrequency96000, kEnableSamplingFrequency96000);
    capabilities.set(kSamplingFrequency96000_1, kEnableSamplingFrequency96000);
  }

  capabilities.set(kChannelModeMono, kEnableChannelModeMono);
  capabilities.set(kChannelModeStereo, kEnableChannelModeStereo);
  capabilities.set(kChannelModeJointStereo, kChannelModeJointStereo);

  capabilities.set(kTtpLL0, kTtpLowLatency0);
  capabilities.set(kTtpLL1, kTtpLowLatency1);
  capabilities.set(kTtpHQ0, kTtpHighQuality0);
  capabilities.set(kTtpHQ1, kTtpHighQuality1);
  capabilities.set(kTtpTWS0, kTtpTws0);
  capabilities.set(kTtpTWS1, kTtpTws1);

  capabilities.set(kCapExtVersion, kCapVersion);

  uint32_t aptx_feature_capabilities = kCodecFeaturesV20;
  if (isAptxAdaptive2_1_Supported())
    aptx_feature_capabilities = kCodecFeaturesV21;
  if (isAptxAdaptive2_2_Supported())
    aptx_feature_capabilities = kCodecFeaturesV22;

  capabilities.set(kSupportedFeatures, aptx_feature_capabilities);

  capabilities.set(kSetUpPref1, kSetUpPreference1);
  capabilities.set(kSetUpPref2, kSetUpPreference2);
  capabilities.set(kSetUpPref3, kSetUpPreference3);
  capabilities.set(kSetUpPref4, kSetUpPreference4);

  capabilities.set(kEndOfCapabilities, kEoC);

  /* --- Setup Sampling Frequencies --- */

  auto& sampling_frequency = a2dp_info.samplingFrequencyHz;

  for (auto v : {44100, 48000, 96000})
    if (capabilities.get(GetSamplingFrequencyBit(int32_t(v))))
      sampling_frequency.push_back(v);

  /* --- Setup Channel Modes --- */

  auto& channel_modes = a2dp_info.channelMode;

  for (auto v : {ChannelMode::MONO, ChannelMode::STEREO})
    if (capabilities.get(GetChannelModeBit(v))) channel_modes.push_back(v);

  /* --- Setup Bitdepth --- */

  a2dp_info.bitdepth.push_back(kBitdepth);
}

A2dpStatus A2dpOffloadCodecAptxAdaptive::ParseConfiguration(
    const std::vector<uint8_t>& configuration,
    CodecParameters* codec_parameters, AptxAdaptiveParameters* aptxadaptive_parameters) const {
  auto& a2dp_info = info.transport.get<CodecInfo::Transport::Tag::a2dp>();

  if (configuration.size() != a2dp_info.capabilities.size())
    return A2dpStatus::BAD_LENGTH;

  auto config = A2dpBits(configuration);
  auto lcaps = A2dpBits(a2dp_info.capabilities);

  /* --- Check Sampling Frequency --- */

  int sampling_frequency = config.find_first_active_bit(kSamplingFrequency);
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

  bool is_in_lossless_mode = (codec_parameters->samplingFrequencyHz == 44100);
  BtAVsClient& instance_ = BtAVsClient::GetInstance();
  instance_.SetLosslessMode(is_in_lossless_mode);

  codec_parameters->lossless = is_in_lossless_mode;

  codec_parameters->vendorSpecificParameters = {}; // TO-DO

  if (aptxadaptive_parameters) {
  }

  return A2dpStatus::OK;
}

bool A2dpOffloadCodecAptxAdaptive::BuildConfiguration(
    const std::vector<uint8_t>& remote_capabilities,
    const std::optional<CodecParameters>& hint,
    const AudioContext audio_context,
    std::vector<uint8_t>* configuration) const {
  auto& a2dp_info = info.transport.get<CodecInfo::Transport::Tag::a2dp>();

  LOG(INFO) << "APTX-ADAPTIVE Local Caps: ";
  for (auto local : a2dp_info.capabilities)
    LOG(INFO) << static_cast<int>(local);

  LOG(INFO) << "APTX-ADAPTIVE Remote Caps: ";
  for (auto remote : remote_capabilities)
    LOG(INFO) << static_cast<int>(remote);

  if (remote_capabilities.size() != a2dp_info.capabilities.size()) return false;

  bool is_source_split_tx_supported = isAptxAdaptiveSplitTxSupported();

  auto lcaps = A2dpBits(a2dp_info.capabilities);
  auto rcaps = A2dpBits(remote_capabilities);

  configuration->resize(a2dp_info.capabilities.size());
  std::fill(begin(*configuration), end(*configuration), 0);
  auto config = A2dpBits(*configuration);

  bool is_gaming_mode = (audio_context.bitmask == AudioContext::GAME);
  LOG(INFO) << "Current Gaming mode: " << is_gaming_mode;

  /* --- Select Sampling Frequency --- */

  bool is_lossless_capability = is_source_split_tx_supported &&
      lcaps.get(kSamplingFrequency44100) && rcaps.get(kSamplingFrequency44100);
  BtAVsClient& instance_ = BtAVsClient::GetInstance();
  instance_.SetLosslessCapability(is_lossless_capability);

  auto sf_hint = hint ? GetSamplingFrequencyBit(hint->samplingFrequencyHz) : -1;

  LOG(INFO) << "sf_hint: " << sf_hint;
  if (sf_hint >= 0 && lcaps.get(sf_hint) && rcaps.get(sf_hint)) {
    if (sf_hint == kSamplingFrequency44100 && !is_gaming_mode && is_source_split_tx_supported)
      config.set(kSamplingFrequency44100);

    else if ((sf_hint == kSamplingFrequency96000 || sf_hint == kSamplingFrequency96000_1)
        && !is_gaming_mode && is_source_split_tx_supported) {
      config.set(kSamplingFrequency96000);
      config.set(kSamplingFrequency96000_1);
    }

    else if (sf_hint == kSamplingFrequency48000)
      config.set(kSamplingFrequency48000);

    else
      config.set(sf_hint);
  }

  else if (sf_hint == 0 && lcaps.get(kSamplingFrequency96000_1) &&
      rcaps.get(kSamplingFrequency96000_1) && !is_gaming_mode && is_source_split_tx_supported) {
    config.set(kSamplingFrequency96000);
    config.set(kSamplingFrequency96000_1);
  }
  else if (lcaps.get(kSamplingFrequency44100) &&
      rcaps.get(kSamplingFrequency44100) && !is_gaming_mode && is_source_split_tx_supported)
    config.set(kSamplingFrequency44100);

  else if (lcaps.get(kSamplingFrequency48000) &&
      rcaps.get(kSamplingFrequency48000))
    config.set(kSamplingFrequency48000);

  else if (lcaps.get(kSamplingFrequency96000) &&
      rcaps.get(kSamplingFrequency96000) && !is_gaming_mode && is_source_split_tx_supported) {
    config.set(kSamplingFrequency96000);
    config.set(kSamplingFrequency96000_1);
  }

  else if (lcaps.get(kSamplingFrequency96000_1) &&
      rcaps.get(kSamplingFrequency96000_1) && !is_gaming_mode && is_source_split_tx_supported) {
    config.set(kSamplingFrequency96000);
    config.set(kSamplingFrequency96000_1);
  }

  else {
    LOG(INFO) << ": Unable to Set APTX-ADAPTIVE frequency";
    return false;
  }

  /* --- Select Channel Mode --- */

  auto cm_hint = hint ? GetChannelModeBit(hint->channelMode) : -1;

  if (cm_hint >= 0 && lcaps.get(cm_hint) && rcaps.get(cm_hint))
    config.set(cm_hint);
  else if (lcaps.get(kChannelModeStereo) && rcaps.get(kChannelModeStereo))
    config.set(kChannelModeStereo);
  else if (lcaps.get(kChannelModeJointStereo) && rcaps.get(kChannelModeJointStereo))
    config.set(kChannelModeJointStereo);
  else if (lcaps.get(kChannelModeMono) && rcaps.get(kChannelModeMono))
    config.set(kChannelModeMono);
  else {
    LOG(INFO) << ": Unable to Set APTX-ADAPTIVE Channel Mode";
    return false;
  }

  config.set(kTtpLL0, kTtpLowLatency0);
  config.set(kTtpLL1, kTtpLowLatency1);
  config.set(kTtpHQ0, kTtpHighQuality0);
  config.set(kTtpHQ1, kTtpHighQuality1);
  config.set(kTtpTWS0, kTtpTws0);
  config.set(kTtpTWS1, kTtpTws1);

  /* --- Select Codec Version Mode --- */

  int negotaiated_codec_version =
      std::min(lcaps.get(kCapExtVersion), rcaps.get(kCapExtVersion));

  config.set(kCapExtVersion, negotaiated_codec_version);

  /* --- Select LeftOver Codec Params Payload based on Codec Version --- */

  if (negotaiated_codec_version != 0) { // Aptx Adaptive R2.x
    uint8_t local_aptx_feature_byte_17 = (lcaps.get(kSupportedFeatures) & 0xFF000000) >> 24;
    uint8_t remote_aptx_feature_byte_17 = (rcaps.get(kSupportedFeatures) & 0xFF000000) >> 24;

    if (is_source_split_tx_supported &&
        (isAptxAdaptive2_1_Supported() || isAptxAdaptive2_2_Supported()))
      local_aptx_feature_byte_17 |= A2DP_APTX_ADAPTIVE_SOURCE_SPILT_TX_SUPPORTED;

    uint8_t negotiated_aptx_feature_byte_17 =
      (((local_aptx_feature_byte_17 >> 4) | (remote_aptx_feature_byte_17 >> 4)) << 4) |
      ((local_aptx_feature_byte_17 & remote_aptx_feature_byte_17) & 0x0F);

    uint32_t final_negotiated_aptx_features =
        (rcaps.get(kSupportedFeatures) & 0x00FFFFFF) | (negotiated_aptx_feature_byte_17 << 24);

    LOG(INFO) << ": local_aptx_feature_byte_17 " << static_cast<int>(local_aptx_feature_byte_17)
              << ": remote_aptx_feature_byte_17 " << static_cast<int>(remote_aptx_feature_byte_17)
              << ": negotiated_aptx_feature_byte_17 " << static_cast<int>(negotiated_aptx_feature_byte_17);

    config.set(kSupportedFeatures, final_negotiated_aptx_features);

    config.set(kSetUpPref1, kSetUpPreference1);
    config.set(kSetUpPref2, kSetUpPreference2);
    config.set(kSetUpPref3, kSetUpPreference3);
    config.set(kSetUpPref4, kSetUpPreference4);

    config.set(kEndOfCapabilities, kEoC);
  } else { // Aptx Adaptive R1.x
    config.set(kEndOfCapabilitiesV1, kEoC);
  }

  return true;
}

}  // namespace aidl::android::hardware::bluetooth::audio
