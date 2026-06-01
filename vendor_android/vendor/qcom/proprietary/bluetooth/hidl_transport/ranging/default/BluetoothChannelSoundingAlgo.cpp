/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "BluetoothChannelSoundingAlgo.h"
#include <cutils/properties.h>
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "vendor.qti.bcs@1.0-bcsalgo"

namespace aidl::android::hardware::bluetooth::ranging::impl {
std::shared_ptr<IBluetoothChannelSoundingSessionCallback> bcs_session_cb;
std::shared_ptr<BluetoothChannelSoundingAlgo> BluetoothChannelSoundingAlgo::main_instance = nullptr;

BluetoothChannelSoundingAlgo::BluetoothChannelSoundingAlgo()
{
}

BluetoothChannelSoundingAlgo::~BluetoothChannelSoundingAlgo()
{
}

std::shared_ptr<BluetoothChannelSoundingAlgo> BluetoothChannelSoundingAlgo::Get()
{
  if (!main_instance)
    main_instance.reset(new BluetoothChannelSoundingAlgo());

  return main_instance;
}

bool BluetoothChannelSoundingAlgo::InitializeAlgoHandles(std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback)
{
  bcs_session_cb = callback;
  isHalV2 = false;
#ifdef BCS_HAL_V2
  isHalV2 = true;
#endif
  ALOGI("%s: isHalV2: %s", __func__, isHalV2 ? "true" : "false");
  ALOGI("%s", __func__);
  gAlgoLibHandle = dlopen(BCS_ALGO_LIB_PATH, RTLD_NOW);

  if (!gAlgoLibHandle) {
    ALOGE("%s: %s : dlopen error csalgo: %s & %d", __func__,
           BCS_ALGO_LIB_PATH, strerror(errno), errno);
    return false;
  }

  *(void**)(&gCsIfInitHandle) = dlsym(gAlgoLibHandle, "csIfInit");

  if (!gCsIfInitHandle) {
    ALOGE("%s: dlopen error gCsIfInitHandle", __func__);
    dlclose(gAlgoLibHandle);
    return false;
  }

  *(void**)(&gCsIfSetParam) = dlsym(gAlgoLibHandle, "csIfSetParam");

  if (!gCsIfSetParam) {
    ALOGE("%s: dlopen error gCsIfSetParam", __func__);
    dlclose(gAlgoLibHandle);
    return false;
  }

  *(void**)(&gCsIfGetEstimate) = dlsym(gAlgoLibHandle, "csIfGetEstimate");

  if (!gCsIfGetEstimate) {
    ALOGE("%s: dlopen error gCsIfGetEstimate", __func__);
    dlclose(gAlgoLibHandle);
    return false;
  }

  if (isHalV2) {

    *(void**)(&gCsIfAddMeasurementV2) = dlsym(gAlgoLibHandle, "csIfAddMeasurementV2");

    if (!gCsIfAddMeasurementV2) {
      ALOGE("%s: dlopen error gCsIfAddMeasurementV2", __func__);
      dlclose(gAlgoLibHandle);
      return false;
    }
  } else {
    *(void**)(&gCsIfAddMeasurement) = dlsym(gAlgoLibHandle, "csIfAddMeasurement");

    if (!gCsIfAddMeasurement) {
      ALOGE("%s: dlopen error gCsIfAddMeasurement", __func__);
      dlclose(gAlgoLibHandle);
      return false;
    }
  }

  ALOGI("%s: successfully opened algo lib from %s", __func__, BCS_ALGO_LIB_PATH);
  return true;
}

bool BluetoothChannelSoundingAlgo::DeinitializeAlgoHandles(void)
{
  ALOGI("%s", __func__);
  dlclose(gAlgoLibHandle);
  return true;
}

bool BluetoothChannelSoundingAlgo::csIfInit(Int32 linkReference, CsConfig *config)
{
  ALOGI("%s linkReference %d", __func__, linkReference);
  if (!gAlgoLibHandle) {
    ALOGE("%s: %s : dlopen error csalgo: %s & %d", __func__,
           BCS_ALGO_LIB_PATH, strerror(errno), errno);
    return false;
  }

  if (!gCsIfInitHandle) {
    ALOGE("%s: dlopen error gCsIfInitHandle", __func__);
    return false;
  }

  LeLinkReference = linkReference;
  config->callback = CsAlgoResultsCallback;
  ALOGI("%s: ACL handler %d Location type: %d Sight Type: %d", __func__, LeLinkReference,
        ChannelSoundingParameters.LocationType, ChannelSoundingParameters.SightType);

  gCsIfInitHandle(linkReference, config);

  if (!gCsIfSetParam) {
    ALOGE("%s: dlopen error gCsIfSetParam", __func__);
    dlclose(gAlgoLibHandle);
    return false;
  }

  CsEnvironment csEnvironment;
  csEnvironment.location = static_cast<CsLocation_t>(ChannelSoundingParameters.LocationType);
  csEnvironment.sight = static_cast<CsSight_t>(ChannelSoundingParameters.SightType);
  
  gCsIfSetParam(linkReference, IFPARAM_ENV, &csEnvironment);

  return true;
}

bool BluetoothChannelSoundingAlgo::csIfAddMeasurement(CsMeasurement *params)
{
  if (!gAlgoLibHandle) {
    ALOGE("%s: %s : dlopen error csalgo: %s & %d", __func__,
           BCS_ALGO_LIB_PATH, strerror(errno), errno);
    return false;
  }

  if (!gCsIfAddMeasurement) {
    ALOGE("%s: dlopen error gCsIfAddMeasurement", __func__);
    return false;
  }
  
  gCsIfAddMeasurement(LeLinkReference, params);  
  gCsIfGetEstimate(LeLinkReference);
  return true;
}

bool BluetoothChannelSoundingAlgo::csIfAddMeasurementV2(CsChannelSoundingProcedureData *params) {
  if (!gAlgoLibHandle) {
    ALOGE("%s: %s : dlopen error csalgo: %s & %d", __func__,
           BCS_ALGO_LIB_PATH, strerror(errno), errno);
    return false;
  }

  if (!gCsIfAddMeasurementV2) {
    ALOGE("%s: dlopen error gCsIfAddMeasurementV2", __func__);
    return false;
  }
  if (!gCsIfGetEstimate) {
    ALOGE("%s: dlopen error gCsIfGetEstimate", __func__);
    return false;
  }
  gCsIfAddMeasurementV2(LeLinkReference, params);  
  gCsIfGetEstimate(LeLinkReference);
  return true;
}

void BluetoothChannelSoundingAlgo::CsAlgoResultsCallback(CsEstimate *estimates, int *linkRef) {
  RangingResult ranging_result;

  ranging_result.resultMeters =  (0.001)*(estimates->distEst1);
  ranging_result.confidenceLevel = (int8_t) estimates->distConf1;
  if (bcs_session_cb) {
    ALOGI("%s : distance %lf meters", __func__, estimates->distEst1*0.001);
    bcs_session_cb->onResult(ranging_result);
  }
}
}  // namespace aidl::android::hardware::bluetooth::ranging::impl
