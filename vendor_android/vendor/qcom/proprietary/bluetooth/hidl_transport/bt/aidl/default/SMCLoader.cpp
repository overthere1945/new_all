/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "SMCLoader.h"
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include "SMCLoaderCallback.h"
#include <utils/Log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "android.hardware.bluetooth.smcloader"
#define SMC_WAIT_RETRY_INTERVAL_MS 100000
#define SMC_WAIT_RETRY_COUNT 5

std::shared_ptr<ISMCLoader> smc_loader_hal = NULL;
SMCLoader* SMCLoader::instance = NULL;

const static char kSMCLoaderServiceName[] = "default";
static AIBinder_DeathRecipient* SMCdeathRecipient = nullptr;

static binder_status_t registerSMCHAL_deathRecipient(::ndk::SpAIBinder& binder);

static void onSMCbinderDied(void* cookie){
    ALOGE("SMC Binder died");

    smc_loader_hal = NULL;
    unsigned char retry = 0;
    const std::string smc_instance = std::string() + ISMCLoader::descriptor
        + "/" + kSMCLoaderServiceName;

    /* We should not be blocking on the binder call. Lets keep a count for
     * retry mechanism until BT HAL binds to SMC HAL. Bail out if it fails.
    */

    while(retry < SMC_WAIT_RETRY_COUNT) {
        AIBinder* SMCservice = AServiceManager_checkService(smc_instance.c_str());
        if(SMCservice == nullptr) {
            retry++;
            usleep(SMC_WAIT_RETRY_INTERVAL_MS);
        } else {
            ::ndk::SpAIBinder binder = ndk::SpAIBinder(SMCservice);
            /* Update binder ptr with new instance */
            smc_loader_hal = ISMCLoader::fromBinder(binder);
            registerSMCHAL_deathRecipient(binder);
            return;
        }
    }
    if(retry == SMC_WAIT_RETRY_COUNT) {
        ALOGE("Failed to link to death recipient after %d retries", retry);
        /* to-do: handle a case where BT HAL fails to bind to SMC HAL */
    }
    return;
}

static binder_status_t registerSMCHAL_deathRecipient(::ndk::SpAIBinder& binder) {

    ALOGI("registerSMCHAL_deathRecipient");

    //delete the previous instance of death recipient before registering another one
    AIBinder_DeathRecipient_delete(SMCdeathRecipient);
    SMCdeathRecipient = AIBinder_DeathRecipient_new(&onSMCbinderDied);

    binder_status_t status = AIBinder_linkToDeath(binder.get(),
        SMCdeathRecipient, nullptr);

    if (status == STATUS_OK) {
        ALOGI("Successfully linked to death recipient");
    } else {
        ALOGE("Failed to link to death recipient");
    }
    return status;
}

void SMCLoader::registerSMCHAL(){

    ALOGI("registerSMCHAL");

    const std::string smc_instance = std::string() + ISMCLoader::descriptor + "/" + kSMCLoaderServiceName;
    ::ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_waitForService(smc_instance.c_str()));

    smc_loader_hal = ISMCLoader::fromBinder(binder);
    if (smc_loader_hal == NULL)
    {
        ALOGE("******** HAL object is NULL ********\n");
    } else {
        ALOGI("SMC HAL Registered Successfully");
        registerSMCHAL_deathRecipient(binder);
    }
}

void SMCLoader::registerSMCLoaderCallback(){
    auto aidl_smc_cb_ = SMCLoaderCallback::getInstance();
    auto cback_status = smc_loader_hal->registerSMCLoaderCallback(aidl_smc_cb_);
    if (!cback_status.isOk()) {
        ALOGE("Failed to registerSMCLoaderCallback");
    }else{
        ALOGI("registerSMCLoaderCallback Successfully");
    }
}

SMCBootState SMCLoader::getSMCBootState(){
    if (smc_loader_hal == NULL){
        ALOGE("******** HAL object is NULL ********\n");
    }else{
        auto hidl_status = smc_loader_hal->getSMCBootState(&smcBootState);
        if (!hidl_status.isOk()) {
          ALOGE("Unable to call getSMCBootState()");
        }else{
            ALOGI("smcBootState is :: %d",smcBootState);
        }
    }
    SMCBootState localSmcBootState = smcBootState;
    return localSmcBootState;
}

void SMCLoader::enableQCCSubsys(){
    if (smc_loader_hal == NULL){
        ALOGE("******** HAL object is NULL ********\n");
    }else{
        Status _aidl_return;
        smc_loader_hal->enableQCCSubsys(QCCSubSysType::QCC_SUBSYS_BT_UWB, &_aidl_return);
        if(_aidl_return != Status::SUCCESS){
            ALOGE("Failed with %d \n", _aidl_return);
        }
    }
}

void SMCLoader::disableQCCSubsys(){
    if (smc_loader_hal == NULL){
        ALOGE("******** HAL object is NULL ********\n");
    }else{
        Status _aidl_return;
        smc_loader_hal->disableQCCSubsys(QCCSubSysType::QCC_SUBSYS_BT_UWB, &_aidl_return);
        if(_aidl_return != Status::SUCCESS){
            ALOGE("Failed with %d \n", _aidl_return);
        }
    }
}

SMCLoader* SMCLoader::getInstance() {
    if (instance == nullptr) {
            instance = new SMCLoader();
    }
    return instance;
}

SMCLoader::SMCLoader() {
    ALOGI("SMCLoader constructor");
}

#ifdef IS_SMC_HAL_V2
int SMCLoader::prepareFMDEntry(){
    Status _aidl_return = Status::ERROR;
    if (smc_loader_hal == NULL){
        ALOGE("******** HAL object is NULL ********\n");
    }else{
        smc_loader_hal->prepareFMDEntry(&_aidl_return);
        if(_aidl_return != Status::SUCCESS){
            ALOGE("Failed with %d \n", _aidl_return);
        }
    }
    return static_cast<int>(_aidl_return);
}

int SMCLoader::enterFMD(){
    Status _aidl_return = Status::ERROR;
    if (smc_loader_hal == NULL){
        ALOGE("******** HAL object is NULL ********\n");
    }else{
        smc_loader_hal->enterFMD(&_aidl_return);
        if(_aidl_return != Status::SUCCESS){
            ALOGE("Failed with %d \n", _aidl_return);
        }
    }
    return static_cast<int>(_aidl_return);
}
#endif