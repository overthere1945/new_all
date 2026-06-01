/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
#pragma once
#include "cs_interface.h"
#include<memory>
#include <dlfcn.h>
#include <android-base/logging.h>
#include <utils/Log.h>
#include <errno.h>
#include <aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h>

using namespace std;
#define BCS_ALGO_LIB_PATH "/vendor/lib64/hw/qc_bcs_lib.so"
#define FLT_MAX_T 0x1.fffffep127f

namespace aidl::android::hardware::bluetooth::ranging::impl {
class BluetoothChannelSoundingAlgo
{
  public:
    BluetoothChannelSoundingAlgo();
    ~BluetoothChannelSoundingAlgo();
    static std::shared_ptr <BluetoothChannelSoundingAlgo> Get(void);
    bool InitializeAlgoHandles(std::shared_ptr<IBluetoothChannelSoundingSessionCallback>);
    bool DeinitializeAlgoHandles(void);
    bool csIfInit(Int32, CsConfig *);
    bool csIfAddMeasurement(CsMeasurement *);
    bool csIfAddMeasurementV2(CsChannelSoundingProcedureData *);
    Int32 LeLinkReference;
    bool isHalV2;
    struct ChannelSoundingParameters {
      Int16 LocationType;
      Int16 SightType;
    } ChannelSoundingParameters;
  private:
    void *gAlgoLibHandle;
    bool (*gCsIfInitHandle)(int, CsConfig *);
    bool (*gCsIfAddMeasurement)(int, CsMeasurement *);
    bool (*gCsIfCleanUp)(int);
    bool (*gCsIfGetEstimate)(int);
    bool (*gCsIfAddMeasurementV2)(int, CsChannelSoundingProcedureData *);
    bool (*gCsIfSetParam)(int, IfParamName, CsIfParam *);
    static void CsAlgoResultsCallback(CsEstimate *, int *);
    static std::shared_ptr<BluetoothChannelSoundingAlgo> main_instance;
};

}  // namespace aidl::android::hardware::bluetooth::ranging::impl

