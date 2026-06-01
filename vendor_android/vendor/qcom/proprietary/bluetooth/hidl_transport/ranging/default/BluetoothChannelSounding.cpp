/*
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
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BluetoothChannelSounding.h"

#include "BluetoothChannelSoundingSession.h"
#include "BluetoothChannelSoundingAlgo.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "vendor.qti.bcs@1.0-bcs"

namespace aidl::android::hardware::bluetooth::ranging::impl {

BluetoothChannelSounding::BluetoothChannelSounding() {}
BluetoothChannelSounding::~BluetoothChannelSounding() {}

ndk::ScopedAStatus BluetoothChannelSounding::getVendorSpecificData(
    std::optional<
        std::vector<std::optional<VendorSpecificData>>>* /*_aidl_return*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSounding::getSupportedSessionTypes(
    std::optional<std::vector<SessionType>>* _aidl_return) {
  std::vector<SessionType> supported_session_types = {};
  *_aidl_return = supported_session_types;
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSounding::getMaxSupportedCsSecurityLevel(
    CsSecurityLevel* _aidl_return) {
  CsSecurityLevel security_level = CsSecurityLevel::NOT_SUPPORTED;
  *_aidl_return = security_level;
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSounding::openSession(
    const BluetoothChannelSoundingParameters& in_params,
    const std::shared_ptr<IBluetoothChannelSoundingSessionCallback>&
        in_callback,
    std::shared_ptr<IBluetoothChannelSoundingSession>* _aidl_return) {
  ALOGI("%s", __func__);
  if (in_callback == nullptr) {
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
        EX_ILLEGAL_ARGUMENT, "Invalid nullptr callback");
  }
  std::shared_ptr<BluetoothChannelSoundingSession> session = nullptr;
  session = ndk::SharedRefBase::make<BluetoothChannelSoundingSession>(
      in_callback, Reason::LOCAL_STACK_REQUEST);
  *_aidl_return = session;

  CsConfig mAlgoCsConfig;
  mAlgoCsConfig.thresPNoise = FLT_MAX_T;
  mAlgoCsConfig.thresDist = FLT_MAX_T;
  mAlgoCsConfig.pres.dest = 0;
  mAlgoCsConfig.pres.conf = 0;

  ALOGI("%s: ACL handler %d Location type: %d Sight Type: %d", __func__, 
        in_params.aclHandle, in_params.locationType, in_params.sightType);
  Int16 linkReference = (Int16) in_params.aclHandle;
  BluetoothChannelSoundingAlgo::Get()->ChannelSoundingParameters.LocationType =
    static_cast<Int16>(in_params.locationType);
  BluetoothChannelSoundingAlgo::Get()->ChannelSoundingParameters.SightType =
    static_cast<Int16>(in_params.sightType);
  BluetoothChannelSoundingAlgo::Get()->csIfInit(linkReference, &mAlgoCsConfig);
  return ::ndk::ScopedAStatus::ok();
}
#ifdef BCS_HAL_V2
ndk::ScopedAStatus BluetoothChannelSounding::getSupportedCsSecurityLevels(
   std::vector<CsSecurityLevel>* _aidl_return) {
  std::vector<CsSecurityLevel> supported_security_levels = {
      CsSecurityLevel::ONE, CsSecurityLevel::TWO, CsSecurityLevel::THREE,
      CsSecurityLevel::FOUR};
  *_aidl_return = supported_security_levels;
  return ::ndk::ScopedAStatus::ok();
}
#endif
}  // namespace aidl::android::hardware::bluetooth::ranging::impl
