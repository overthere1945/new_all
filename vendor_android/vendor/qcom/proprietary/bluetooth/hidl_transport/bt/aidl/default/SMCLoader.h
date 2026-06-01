/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <aidl/vendor/qti/hardware/smcloader/ISMCLoader.h>
#include <aidl/vendor/qti/hardware/smcloader/ISMCLoaderCallback.h>
#include <aidl/vendor/qti/hardware/smcloader/SMCBootState.h>
#include <aidl/vendor/qti/hardware/smcloader/QCCSubSysType.h>
#include <aidl/vendor/qti/hardware/smcloader/Status.h>
#include <mutex>

#define SMCLOADER_RETRY_TIMEOUT (120000)

using ::aidl::vendor::qti::hardware::smcloader::ISMCLoader;
using ::aidl::vendor::qti::hardware::smcloader::ISMCLoaderCallback;
using ::aidl::vendor::qti::hardware::smcloader::SMCBootState;
using ::aidl::vendor::qti::hardware::smcloader::QCCSubSysType;
using ::aidl::vendor::qti::hardware::smcloader::Status;

class SMCLoader {
 public:
  static SMCLoader* getInstance();
  void registerSMCHAL();
  void registerSMCLoaderCallback();
  void enableQCCSubsys();
  void disableQCCSubsys();
  SMCBootState getSMCBootState();
#ifdef IS_SMC_HAL_V2
  int prepareFMDEntry();
  int enterFMD();
#endif
 private:
  static SMCLoader* instance;
  SMCBootState smcBootState = SMCBootState::QCOM_SMC_BEFORE_POWERUP;
  SMCLoader();
};
