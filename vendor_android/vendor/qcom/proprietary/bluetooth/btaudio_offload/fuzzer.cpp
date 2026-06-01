/*==============================================================================
*  Copyright (c) 2023 Qualcomm Technologies, Inc.
*  All Rights Reserved.
*  Confidential and Proprietary - Qualcomm Technologies, Inc.
*===============================================================================
*/

#include <fuzzer/FuzzedDataProvider.h>
#include <fuzzbinder/libbinder_ndk_driver.h>
#include <android-base/logging.h>
#include <android/binder_interface_utils.h>

#include "BluetoothAudioProviderFactory.h"

using ::aidl::android::hardware::bluetooth::audio::BluetoothAudioProviderFactory;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {

       auto binder = ::ndk::SharedRefBase::make<BluetoothAudioProviderFactory>();
       android::fuzzService(binder->asBinder().get(), FuzzedDataProvider(data, size));

      return 0;
}

