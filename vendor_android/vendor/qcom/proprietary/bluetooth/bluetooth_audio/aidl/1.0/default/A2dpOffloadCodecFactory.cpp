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

#define LOG_TAG "A2dpOffloadCodecFactory"

#include <algorithm>
#include <android-base/logging.h>
#include <cassert>

#include "A2dpOffloadCodecFactory.h"
#include "A2dpOffloadCodecAac.h"
#include "A2dpOffloadCodecAptx.h"
#include "A2dpOffloadCodecAptxAdaptive.h"
#include "A2dpOffloadCodecAptxHd.h"
#include "A2dpOffloadCodecLdac.h"
#include "A2dpOffloadCodecSbc.h"

#include "btavs_client.h"

namespace aidl::android::hardware::bluetooth::audio {

/**
 * Local Capabilities Configuration
 */

enum : bool {
  kEnableAptxAdaptive = true,
  kEnableLdac = true,
  kEnableAptxHd = true,
  kEnableAptx = true,
  kEnableAac = true,
  kEnableSbc = true,
};

/**
 * Class implementation
 */

A2dpOffloadCodecFactory::A2dpOffloadCodecFactory()
    : name("Offload"), codecs(ranked_codecs_) {
  ranked_codecs_.reserve(kEnableAac + kEnableSbc + kEnableLdac + kEnableAptx +
          kEnableAptxHd + kEnableAptxAdaptive);

  if (kEnableAptxAdaptive)
    ranked_codecs_.push_back(std::make_shared<A2dpOffloadCodecAptxAdaptive>());
  if (kEnableLdac)
    ranked_codecs_.push_back(std::make_shared<A2dpOffloadCodecLdac>());
  if (kEnableAptxHd)
    ranked_codecs_.push_back(std::make_shared<A2dpOffloadCodecAptxHd>());
  if (kEnableAptx)
    ranked_codecs_.push_back(std::make_shared<A2dpOffloadCodecAptx>());
  if (kEnableAac)
    ranked_codecs_.push_back(std::make_shared<A2dpOffloadCodecAac>());
  if (kEnableSbc)
    ranked_codecs_.push_back(std::make_shared<A2dpOffloadCodecSbc>());
}

std::shared_ptr<const A2dpOffloadCodec> A2dpOffloadCodecFactory::GetCodec(
    CodecId id) const {
  auto codec = std::find_if(begin(ranked_codecs_), end(ranked_codecs_),
                            [&](auto c) { return id == c->info.id; });

  return codec != end(ranked_codecs_) ? *codec : nullptr;
}

bool A2dpOffloadCodecFactory::GetConfiguration(
    const std::vector<A2dpRemoteCapabilities>& remote_capabilities,
    const A2dpConfigurationHint& hint, A2dpConfiguration* configuration) const {
  decltype(ranked_codecs_) codecs;

  codecs.reserve(ranked_codecs_.size());

  BtAVsClient& instance = BtAVsClient::GetInstance();
  bool is_gaming_mode = (hint.audioContext.bitmask == AudioContext::GAME);
  instance.SetCurrentA2dpAudioMode(is_gaming_mode);
  LOG(INFO) << __func__ << ": initialize lossless capability to false : ";
  instance.SetLosslessCapability(false);

  auto hinted_it = std::find_if(begin(ranked_codecs_), end(ranked_codecs_),
                  [&](auto c) { return hint.codecId == c->info.id; });



  // If we found the hinted codec, put it first and exclude it by id
  if (hinted_it != end(ranked_codecs_)) {
    const auto targetId = (*hinted_it)->info.id;
    codecs.push_back(*hinted_it);

    std::copy_if(begin(ranked_codecs_), end(ranked_codecs_),
               std::back_inserter(codecs),
               [&](auto c) { return c->info.id != targetId; });
  } else {
    // No hinted codec found: copy all
    std::copy(begin(ranked_codecs_), end(ranked_codecs_),
              std::back_inserter(codecs));
  }

  for (auto codec : codecs) {
    LOG(INFO) << __func__ << ": Trying Codec : " << codec->info.name;
    auto rc =
        std::find_if(begin(remote_capabilities), end(remote_capabilities),
                     [&](auto& rc__) { return codec->info.id == rc__.id; });

    if ((rc == end(remote_capabilities)) ||
        !codec->BuildConfiguration(rc->capabilities, hint.codecParameters,
                                   hint.audioContext, &configuration->configuration))
      continue;

    configuration->id = codec->info.id;
    LOG(INFO) << __func__ << ": Attempting Codec : " << codec->info.name;
    A2dpStatus status = codec->ParseConfiguration(configuration->configuration,
                                                  &configuration->parameters);
    LOG(INFO) << __func__ << ": Parse Status : " << static_cast<int>(status);
    assert(status == A2dpStatus::OK);

    configuration->remoteSeid = rc->seid;

    return true;
  }

  return false;
}

}  // namespace aidl::android::hardware::bluetooth::audio
