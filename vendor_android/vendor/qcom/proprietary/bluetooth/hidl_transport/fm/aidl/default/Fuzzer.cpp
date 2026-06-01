/*==============================================================================
*  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
*  All Rights Reserved.
*  Confidential and Proprietary - Qualcomm Technologies, Inc.
*===============================================================================
*/

#include <string>
#include <FmHci.h>
#include <FmIoctlHci.h>

#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <log/log.h>

using aidl::vendor::qti::hardware::fm::impl::FmHci;

std::shared_ptr<FmHci> fm_service;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    fm_service = ndk::SharedRefBase::make<FmHci>();
     return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if( size < 12 || fm_service == nullptr) return 0;

    FuzzedDataProvider provider(data, size);
    android::fuzzService(fm_service->asBinder().get(),std::move(provider));
    return 0;
}


