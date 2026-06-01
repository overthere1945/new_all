/*****************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************/

#include <fuzzer/FuzzedDataProvider.h>
#include <fuzzbinder/libbinder_ndk_driver.h>
#include <android-base/logging.h>
#include <android/binder_interface_utils.h>

#include "BtAVsProviderService.h"

using ::aidl::vendor::qti::hardware::bluetooth::btavsprovider::BtAVsProviderService;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
auto binder = ::ndk::SharedRefBase::make<BtAVsProviderService>();
android::fuzzService(binder->asBinder().get(), FuzzedDataProvider(data, size));
return 0;
}
