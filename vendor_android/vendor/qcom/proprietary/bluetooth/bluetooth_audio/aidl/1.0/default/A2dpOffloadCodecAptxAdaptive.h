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

#pragma once

#include "A2dpOffloadCodec.h"

#define A2DP_APTX_ADAPTIVE_VENDOR_ID 0x000000D7
#define A2DP_APTX_ADAPTIVE_CODEC_ID_BLUETOOTH 0x00AD

#define A2DP_APTX_ADAPTIVE_SOURCE_SPILT_TX_SUPPORTED 0x04

namespace aidl::android::hardware::bluetooth::audio {

struct AptxAdaptiveParameters : public CodecParameters {
};

class A2dpOffloadCodecAptxAdaptive : public A2dpOffloadCodec {
  CodecInfo info_;

  A2dpStatus ParseConfiguration(const std::vector<uint8_t>& configuration,
                                CodecParameters* codec_parameters,
                                AptxAdaptiveParameters* aptxadaptive_parameters) const;

 public:
  A2dpOffloadCodecAptxAdaptive();

  A2dpStatus ParseConfiguration(
      const std::vector<uint8_t>& configuration,
      CodecParameters* codec_parameters) const override {
    return ParseConfiguration(configuration, codec_parameters, nullptr);
  }

  A2dpStatus ParseConfiguration(const std::vector<uint8_t>& configuration,
                                AptxAdaptiveParameters* aptxadaptive_parameters) const {
    return ParseConfiguration(configuration, aptxadaptive_parameters, aptxadaptive_parameters);
  }

  bool BuildConfiguration(const std::vector<uint8_t>& remote_capabilities,
                          const std::optional<CodecParameters>& hint,
                          const AudioContext audio_context,
                          std::vector<uint8_t>* configuration) const override;
};

}  // namespace aidl::android::hardware::bluetooth::audio
