/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "SMCLoaderCallback.h"
#include <utils/Log.h>

pthread_cond_t  smcLoaderEventCond;
pthread_mutex_t smcLoaderEventLock;

pthread_cond_t  qccSubsysEventCond;
pthread_mutex_t qccSubsysEventLock;

SMCLoaderCallback::SMCLoaderCallback() = default;

std::shared_ptr<SMCLoaderCallback> SMCLoaderCallback::getInstance() {
    static std::shared_ptr<SMCLoaderCallback> instance = ndk::SharedRefBase::make<SMCLoaderCallback>();
    return instance;
}

void SMCLoaderCallback::setInternalCallbackHandler(std::shared_ptr<InternalSMCCallbackHandler> handler) {
    mHandler = std::move(handler);
}

::ndk::ScopedAStatus SMCLoaderCallback::onSMCLoaderEvent(::aidl::vendor::qti::hardware::smcloader::SMCBootState in_bootStatus) {
    if(mHandler){
        mHandler->onSMCLoaderEvent(in_bootStatus);
    }
    switch (in_bootStatus) {
        case (::SMCBootState::QCOM_SMC_BEFORE_POWERUP): {
             ALOGI("******** QCOM_SMC_BEFORE_POWERUP Recieved ********\n");
        break;
        }
        case (::SMCBootState::QCOM_SMC_AFTER_POWERUP): {
             ALOGI("******** QCOM_SMC_AFTER_POWERUP Recieved ********\n");
             //Sending Signal on Conditional Variable - smcLoaderEventCond
             pthread_cond_signal(&smcLoaderEventCond);
        break;
        }
        case (::SMCBootState::QCOM_SMC_RAM_DUMP): {
             ALOGI("******** QCOM_SMC_RAM_DUMP Recieved ********\n");
        break;
        }
        case (::SMCBootState::QCOM_SMC_BEFORE_SHUTDOWN): {
             ALOGI("******** QCOM_SMC_BEFORE_SHUTDOWN Recieved ********\n");
        break;
        }
        case (::SMCBootState::QCOM_SMC_AFTER_SHUTDOWN): {
             ALOGI("******** QCOM_SMC_AFTER_SHUTDOWN Recieved ********\n");
        break;
        default:{
             ALOGI("******** Unexpected value. Ignore ********\n");
        break;
        }
        }
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SMCLoaderCallback::onQCCSubSystemEvent(QCCSubSysType in_subsystem, QCCSubSysEvent in_event) {
     if(mHandler){
        mHandler->onQCCSubSystemEvent(in_subsystem,in_event);
     }
     switch (in_subsystem) {
        case (::QCCSubSysType::QCC_SUBSYS_BT_UWB): {
            ALOGI("******** QCC_SUBSYS_BT_UWB Recieved ********\n");
            switch(in_event){
                case (::QCCSubSysEvent::POWER_UP_SUCCESS): {
                    ALOGI("******** POWER_UP_SUCCESS Recieved ********\n");
                    //Sending Signal on Conditional Variable - qccSubsysEventCond
                    pthread_cond_signal(&qccSubsysEventCond);
                break;
                }
                case (::QCCSubSysEvent::POWER_UP_FAIL): {
                    ALOGI("******** POWER_UP_FAIL Recieved ********\n");
                break;
                }
                case (::QCCSubSysEvent::POWER_DOWN_SUCCESS): {
                    ALOGI("******** POWER_DOWN_SUCCESS Recieved ********\n");
                    //Sending Signal on Conditional Variable - qccSubsysEventCond
                    pthread_cond_signal(&qccSubsysEventCond);
                break;
                }
                case (::QCCSubSysEvent::POWER_DOWN_FAIL): {
                    ALOGI("******** POWER_DOWN_FAIL Recieved ********\n");
                break;
                }
                case (::QCCSubSysEvent::RAMDUMP_AVAILABLE): {
                    ALOGI("******** RAMDUMP_AVAILABLE Recieved ********\n");
                break;
                default:{
                    ALOGI("******** Unexpected value. Ignore ********\n");
                break;
                }
                }
            }
            break;
        }
        case (::QCCSubSysType::QCC_SUBSYS_WLAN): {
            printf("******** QCC_SUBSYS_WLAN Recieved ********\n");
            break;
        }
        case (::QCCSubSysType::QCC_SUBSYS_GNSS): {
            printf("******** QCC_SUBSYS_GNSS Recieved ********\n");
            break;
        }

    }
    return ::ndk::ScopedAStatus::ok();
}

#ifdef IS_SMC_HAL_V2
::ndk::ScopedAStatus SMCLoaderCallback::onCustomDataReceived(const ::aidl::android::hardware::common::Ashmem& in_buffer) {
    ALOGI("******** CUSTOM_DATA Recieved ********\n");
    return ::ndk::ScopedAStatus::ok();
}
#endif

