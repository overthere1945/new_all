/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <aidl/vendor/qti/hardware/smcloader/SMCBootState.h>
#include <aidl/vendor/qti/hardware/smcloader/QCCSubSysType.h>
#include <aidl/vendor/qti/hardware/smcloader/QCCSubSysEvent.h>
#include <aidl/vendor/qti/hardware/smcloader/BnSMCLoaderCallback.h>
#include <android/binder_interface_utils.h>

#ifdef IS_SMC_HAL_V2
#include <aidl/android/hardware/common/Ashmem.h>
using aidl::android::hardware::common::Ashmem;
#endif

using ::aidl::vendor::qti::hardware::smcloader::BnSMCLoaderCallback;
using ::aidl::vendor::qti::hardware::smcloader::SMCBootState;
using ::aidl::vendor::qti::hardware::smcloader::QCCSubSysType;
using ::aidl::vendor::qti::hardware::smcloader::QCCSubSysEvent;


extern pthread_cond_t smcLoaderEventCond;
extern pthread_mutex_t smcLoaderEventLock;

extern pthread_cond_t qccSubsysEventCond;
extern pthread_mutex_t qccSubsysEventLock;

class SMCLoaderCallback : public ::aidl::vendor::qti::hardware::smcloader::BnSMCLoaderCallback {

public:

  class InternalSMCCallbackHandler {
    public:
        virtual void onSMCLoaderEvent(SMCBootState in_bootStatus) = 0;
        virtual void onQCCSubSystemEvent(QCCSubSysType in_subsystem, QCCSubSysEvent in_event) = 0;
        virtual ~InternalSMCCallbackHandler() = default;
  };

  SMCLoaderCallback();
  static std::shared_ptr<SMCLoaderCallback> getInstance();
  void setInternalCallbackHandler(std::shared_ptr<InternalSMCCallbackHandler> handler);

private:
  ::ndk::ScopedAStatus onSMCLoaderEvent(::aidl::vendor::qti::hardware::smcloader::SMCBootState in_bootStatus) override;
  ::ndk::ScopedAStatus onQCCSubSystemEvent(QCCSubSysType in_subsystem, QCCSubSysEvent in_event) override;
  std::shared_ptr<InternalSMCCallbackHandler> mHandler;
#ifdef IS_SMC_HAL_V2
  ::ndk::ScopedAStatus onCustomDataReceived(const ::aidl::android::hardware::common::Ashmem& in_buffer) override;
#endif
};


