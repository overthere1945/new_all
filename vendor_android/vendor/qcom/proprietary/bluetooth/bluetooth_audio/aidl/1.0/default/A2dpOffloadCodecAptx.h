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

#pragma once

#include "A2dpOffloadCodec.h"

#define A2DP_APTX_VENDOR_ID 0x0000004F
#define A2DP_APTX_CODEC_ID_BLUETOOTH 0x0001

namespace aidl::android::hardware::bluetooth::audio {

struct AptxParameters : public CodecParameters {
};

class A2dpOffloadCodecAptx : public A2dpOffloadCodec {
  CodecInfo info_;

  A2dpStatus ParseConfiguration(const std::vector<uint8_t>& configuration,
                                CodecParameters* codec_parameters,
                                AptxParameters* aptx_parameters) const;

 public:
  A2dpOffloadCodecAptx();

  A2dpStatus ParseConfiguration(
      const std::vector<uint8_t>& configuration,
      CodecParameters* codec_parameters) const override {
    return ParseConfiguration(configuration, codec_parameters, nullptr);
  }

  A2dpStatus ParseConfiguration(const std::vector<uint8_t>& configuration,
                                AptxParameters* aptx_parameters) const {
    return ParseConfiguration(configuration, aptx_parameters, aptx_parameters);
  }

  bool BuildConfiguration(const std::vector<uint8_t>& remote_capabilities,
                          const std::optional<CodecParameters>& hint,
                          const AudioContext audio_context,
                          std::vector<uint8_t>* configuration) const override;
};

}  // namespace aidl::android::hardware::bluetooth::audio
