/*==============================================================================
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*===============================================================================
*/
#include <android/binder_auto_utils.h>
#include "ranging/default/BluetoothChannelSounding.h"

#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <log/log.h>

using aidl::android::hardware::bluetooth::ranging::impl::BluetoothChannelSounding;

std::shared_ptr<BluetoothChannelSounding> service;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    service = ndk::SharedRefBase::make<BluetoothChannelSounding>();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if( size < 15 || service == nullptr ) return 0;

    FuzzedDataProvider provider(data, size);
    android::fuzzService(service->asBinder().get(),std::move(provider));

    return 0;
}
