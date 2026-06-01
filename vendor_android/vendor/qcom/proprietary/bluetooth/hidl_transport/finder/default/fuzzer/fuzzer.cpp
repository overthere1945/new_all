/*==============================================================================
*  Copyright (c) 2024 Qualcomm Technologies, Inc.
*  All Rights Reserved.
*  Confidential and Proprietary - Qualcomm Technologies, Inc.
*===============================================================================
*/
#include <android/binder_auto_utils.h>
#include "finder/default/BluetoothFinder.h"

#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <log/log.h>

using aidl::android::hardware::bluetooth::finder::impl::BluetoothFinder;

std::shared_ptr<BluetoothFinder> service;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    service = ndk::SharedRefBase::make<BluetoothFinder>();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if( size < 15 || service == nullptr ) return 0;

    FuzzedDataProvider provider(data, size);
    android::fuzzService(service->asBinder().get(),std::move(provider));

    return 0;
}