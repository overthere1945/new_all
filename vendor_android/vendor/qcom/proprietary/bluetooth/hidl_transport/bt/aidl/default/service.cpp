/*
 * Copyright (c) 2022, 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 */

/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bluetooth_hci.h"
#include <aidl/android/hardware/bluetooth/IBluetoothHci.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android-base/properties.h>
#include <cutils/properties.h>
#include <hidl/HidlSupport.h>
#include <hidl/LegacySupport.h>
#include <hwbinder/ProcessState.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef QTI_BT_SOFTSKU_SUPPORTED
#include <initsoftsku.h>
#endif
#include "SMCLoader.h"

#ifdef QCOM_FM_SUPPORTED
#include <vendor/qti/hardware/fm/1.0/IFmHci.h>
#include <FmHci.h>
#include <FmIoctlHci.h>
#endif
#include <hidl/HidlTransportSupport.h>


#ifdef QTI_BT_FMD_SUPPORTED
#include "BluetoothFinder.h"
#include <aidl/android/hardware/bluetooth/finder/IBluetoothFinder.h>
#endif

#ifdef QTI_BT_CONFIGSTORE_SUPPORTED
#include <vendor/qti/hardware/btconfigstore/2.0/IBTConfigStore.h>
#endif

#include "bttpi.h"

#ifdef QTI_BT_LMP_EVENT_SUPPORTED
#include <bt_lmp_event.h>
#endif

#ifdef QTI_BT_XPANPROVIDER_SUPPORTED
#include "XpanProviderService.h"
#endif

#ifdef GOOGLE_OFFLOAD_ENABLED
#include "bluetooth_socket_impl.h"
#include <aidl/android/hardware/bluetooth/socket/IBluetoothSocket.h>
#endif
#ifdef GATT_OFFLOAD_ENABLED
#include "bluetooth_gatt_offload_impl.h"

#if USE_VENDOR_GATT_HAL
#include <aidl/vendor/qti/hardware/bluetooth/gatt/IBluetoothGatt.h>
#else
#include <aidl/android/hardware/bluetooth/gatt/IBluetoothGatt.h>
#endif

#endif

#ifdef QTI_BT_QCV_SUPPORTED
#include "soc_properties.h"
extern "C" {
#include "libsoc_helper.h"
}
#endif

#ifdef QTI_VND_FWK_DETECT_SUPPORTED
#ifdef __cplusplus
extern "C" {
#include "vndfwk-detect.h"
}
#endif
#endif

#ifdef QTI_BT_SAR_SUPPORTED
#include <aidl/vendor/qti/hardware/bluetooth_sar/IBluetoothSar.h>
#include "BluetoothSar.h"
#endif

using ::aidl::android::hardware::bluetooth::impl::BluetoothHci;
using android::OK;
using ::android::hardware::configureRpcThreadpool;
using android::hardware::defaultPassthroughServiceImplementation;
using ::android::hardware::joinRpcThreadpool;
using android::hardware::registerPassthroughServiceImplementation;
using ::aidl::vendor::qti::hardware::smcloader::ISMCLoader;

#ifdef QTI_BT_XPANPROVIDER_SUPPORTED
using aidl::vendor::qti::hardware::bluetooth::xpanprovider::XpanProviderService;
#endif

#ifdef QTI_BT_CONFIGSTORE_SUPPORTED
using IBTConfigStore_V2_0 =
    ::vendor::qti::hardware::btconfigstore::V2_0::IBTConfigStore;
#endif

#ifdef QTI_BT_SAR_SUPPORTED
using aidl::vendor::qti::hardware::bluetooth_sar::BluetoothSar;
#endif

#ifdef QTI_BT_FMD_SUPPORTED
using ::aidl::android::hardware::bluetooth::finder::impl::BluetoothFinder;
#endif

#ifdef GOOGLE_OFFLOAD_ENABLED
using ::aidl::android::hardware::bluetooth::socket::IBluetoothSocket;
using ::aidl::android::hardware::bluetooth::socket::impl::BluetoothSocketImpl;
#endif
#ifdef GATT_OFFLOAD_ENABLED

#if USE_VENDOR_GATT_HAL
using ::aidl::vendor::qti::hardware::bluetooth::gatt::IBluetoothGatt;
using ::aidl::vendor::qti::hardware::bluetooth::gatt::impl::BluetoothGattImpl;
#else
using ::aidl::android::hardware::bluetooth::gatt::IBluetoothGatt;
using ::aidl::android::hardware::bluetooth::gatt::impl::BluetoothGattImpl;
#endif

#endif

#ifdef QCOM_FM_SUPPORTED
using ::aidl::vendor::qti::hardware::fm::impl::FmHci;
using vendor::qti::hardware::fm::V1_0::IFmHci;
#endif

#ifdef QTI_BT_LMP_EVENT_SUPPORTED
using aidl::android::hardware::bluetooth::lmp_event::BluetoothLmpEvent;
#endif

#ifdef QTI_BT_BCS_SUPPORTED
#include "BluetoothChannelSounding.h"
using ::aidl::android::hardware::bluetooth::ranging::impl::BluetoothChannelSounding;
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "android.hardware.bluetooth.aidl_service"

int main(int /* argc */, char ** /* argv */) {
  bool isVendorEnhancedFramework;
  ALOGE("Bluetooth HAL starting");
  // Initialize Mmap size the moment the process starts
  android::hardware::ProcessState::initWithMmapSize((size_t)(256144));

  if (!ABinderProcess_setThreadPoolMaxThreadCount(1)) {
    ALOGE("failed to set thread pool max thread count");
    return 1;
  }
  ABinderProcess_startThreadPool();
  char prop_value[PROPERTY_VALUE_MAX];
  bool isTpiSupported = false;
  property_get("persist.vendor.qcom.bluetooth.tpi_supported", prop_value, "1");
  if (!strcmp(prop_value, "1")) {
    ALOGI("BTTPI: TPI is supported");
    isTpiSupported = true;
  } else
    ALOGI("BT TPI service not supported");

#ifdef QTI_BT_XPANPROVIDER_SUPPORTED
  char xpan_prop[PROPERTY_VALUE_MAX];
  bool isXpanSupported = false;
  property_get("persist.vendor.qti.btadvaudio.target.support.xpan", xpan_prop,
               "false");
  if (!strcmp(xpan_prop, "true")) {
    ALOGI("XPAN supported");
    isXpanSupported = true;
  } else {
    ALOGI("XPAN not supported");
  }
#endif

  configureRpcThreadpool(1, true /* caller Will Join*/);

#ifdef QTI_BT_QCV_SUPPORTED
  setVendorPropertiesDefault();
#endif

#ifdef BT_CP_CONNECTED
  ALOGI("calling isCpsupported");
  isCpsupported();
#endif

#ifdef QTI_VND_FWK_DETECT_SUPPORTED
  /* 0: Pure AOSP for both system and odm
     1: Pure AOSP for system and QC Value-adds for odm
     2: QC value-adds for system and Pure AOSP for odm
     3: QC value-adds for both system and odm
  */
  int vendorEnhancedInfo = getVendorEnhancedInfo();
  ALOGI("vendorEnhancedInfo: %d", vendorEnhancedInfo);
  isVendorEnhancedFramework = (vendorEnhancedInfo & 1) ? true : false;
#else
  isVendorEnhancedFramework = false;
#endif

  ALOGI("isVendorEnhancedFramework: %d", isVendorEnhancedFramework);
  ALOGI("Registering BT Service");

  android::status_t status;

  std::shared_ptr<BluetoothHci> service =
      ndk::SharedRefBase::make<BluetoothHci>();
  std::string instance = std::string() + BluetoothHci::descriptor + "/default";
  auto result =
      AServiceManager_addService(service->asBinder().get(), instance.c_str());
  if (result == STATUS_OK) {
    ALOGE("Register BT AIDL service!");
  } else {
    ALOGE("Could not register as a service!");
  }

#ifdef QCOM_FM_SUPPORTED
#if 0
  if (isVendorEnhancedFramework) {
    ALOGI("Registering FM Service");
    status = registerPassthroughServiceImplementation<IFmHci>();
    if (status != OK)
      ALOGI("Error while registering FM service: %d", status);
  }
#endif
  char soc_name[PROPERTY_VALUE_MAX] = {'\0'};
  property_get("persist.vendor.qcom.bluetooth.soc", soc_name, "pronto");
  ALOGE("%s: persist.vendor.qcom.bluetooth.soc prop: %s", __func__, soc_name);
  if(strncmp(soc_name, "cherokee", strlen("cherokee")) == 0) {
    std::shared_ptr<FmHci> fm_service = ndk::SharedRefBase::make<FmHci>();
    std::string fm_instance = std::string() + FmHci::descriptor + "/default";
    auto fm_result =
        AServiceManager_addService(fm_service->asBinder().get(), fm_instance.c_str());
    if (fm_result == STATUS_OK) {
      ALOGE("FM register as a service successful!");
    } else {
      ALOGE("Could not register as a service!");
    }
  } else {
    std::shared_ptr<FmIoctlHal> fm_service = ndk::SharedRefBase::make<FmIoctlHal>(soc_name);
    std::string fm_instance = std::string() + FmIoctlHal::descriptor + "/default";
    auto fm_result =
        AServiceManager_addService(fm_service->asBinder().get(), fm_instance.c_str());
    if (fm_result == STATUS_OK) {
      ALOGE("FM register as a service successful!");
    } else {
      ALOGE("Could not register as a service!");
    }
  }
#endif


#ifdef QTI_BT_SAR_SUPPORTED
  ALOGI("Registering BT SAR Service");
  std::shared_ptr<BluetoothSar> sarService = ndk::SharedRefBase::make<BluetoothSar>();
  std::string sarInstance = std::string() + BluetoothSar::descriptor + "/default";
  auto sarResult =
      AServiceManager_addService(sarService->asBinder().get(), sarInstance.c_str());
  if (sarResult == STATUS_OK) {
    ALOGE("Register BT SAR AIDL service!");
  } else {
    ALOGE("Could not register BT SAR as a service!");
  }
#endif

#ifdef QTI_BT_CONFIGSTORE_SUPPORTED
  if (isVendorEnhancedFramework) {
    ALOGI("Registering BT Config store Service");
    status = registerPassthroughServiceImplementation<IBTConfigStore_V2_0>();
    if (status != OK) {
      ALOGI("Error while registering BT Configstore 2.0 service: %d", status);
    }
  }
#endif

  if (isTpiSupported) {
    ALOGI("Registering BT TPI service");

    std::shared_ptr<BtTpi> mAidlService = ndk::SharedRefBase::make<BtTpi>();
    const std::string instance = std::string() + BtTpi::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(
        mAidlService->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
      ALOGE("BtTpi::Error registering AIDL service [%s]", instance.c_str());
    } else {
      ALOGI("BtTpi::Success registering AIDL service[%s]", instance.c_str());
    }
  }

#ifdef QTI_BT_LMP_EVENT_SUPPORTED
  bool isBtLmpEventSupported = android::base::GetBoolProperty(
      "persist.vendor.qcom.bluetooth.lmp_event_supported", true);
  ALOGI("BtLmpEvent: Lmp Event is %s",
      isBtLmpEventSupported ? "supported" : "not supported");
  if (isBtLmpEventSupported) {
    ALOGI("BtLmpEvent: Registering BT LMP Event service");
    std::shared_ptr<BluetoothLmpEvent> mAidlService =
      ndk::SharedRefBase::make<BluetoothLmpEvent>();
    const std::string instance =
      std::string() + BluetoothLmpEvent::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(
        mAidlService->asBinder().get(), instance.c_str()
    );
    if (status != STATUS_OK) {
      ALOGE("BtLmpEvent: Error registering AIDL Service [%s]", instance.c_str());
    } else {
      ALOGI("BtLmpEvent: Success registering AIDL Service [%s] status [%d]",
          instance.c_str(), status);
    }
  }
#endif

#ifdef QTI_BT_XPANPROVIDER_SUPPORTED
  if (isXpanSupported) {
    ALOGI("XPAN: Registering XPAN AIDL Service ..");

    std::shared_ptr<XpanProviderService> mAidlService =
        ndk::SharedRefBase::make<XpanProviderService>();
    const std::string instance =
        std::string() + XpanProviderService::descriptor + "/default";

    binder_status_t bstatus = AServiceManager_addService(
        mAidlService->asBinder().get(), instance.c_str());

    if (bstatus != STATUS_OK) {
      ALOGE("XpanProvider: Error registering AIDL service [%s]",
            instance.c_str());
    } else {
      ALOGI("XpanProviderService: Successfully registered AIDL service[%s]",
            instance.c_str());
    }
  }
#endif

#ifdef QTI_BT_FMD_SUPPORTED
  ALOGI("Registering FMD AIDL Service ..");
  std::shared_ptr<BluetoothFinder> mAidlService =
        ndk::SharedRefBase::make<BluetoothFinder>();
  instance = std::string() + BluetoothFinder::descriptor + "/default";
  if (AServiceManager_isDeclared(instance.c_str())) {
    auto result = AServiceManager_addService(mAidlService->asBinder().get(), instance.c_str());
    if (result != STATUS_OK) {
      ALOGE("Could not register FMD as a service!");
    } else {
        ALOGI("Registered FMD AIDL service!");
    }
  } else {
    ALOGE("Could not register FMD as a service because it's not declared.");
  }
#endif

#ifdef QTI_BT_SOFTSKU_SUPPORTED
  status = InitSoftSku();
  ALOGI("InitSoftSku: status = %d", status);
#endif

#ifdef QTI_BT_BCS_SUPPORTED
  ALOGI("Registering BluetoothChannelSounding Service ..");
  std::shared_ptr<BluetoothChannelSounding> mAidlService1=
        ndk::SharedRefBase::make<BluetoothChannelSounding>();
  instance = std::string() + BluetoothChannelSounding::descriptor + "/default";
  if (AServiceManager_isDeclared(instance.c_str())) {
    auto result = AServiceManager_addService(mAidlService1->asBinder().get(), instance.c_str());
    if (result != STATUS_OK) {
      ALOGE("Could not register as a service!");
    } else {
      ALOGI("BluetoothChannelSounding: Successfully registered AIDL service[%s]",
            instance.c_str());
    }
  } else {
    ALOGE("Could not register as a service because it's not declared.");
  }
#endif

  SMCLoader* smc_loader = SMCLoader::getInstance();
  smc_loader->registerSMCHAL();

#ifdef GOOGLE_OFFLOAD_ENABLED
  ALOGI("Registering Bluetooth Socket AIDL Service ..");
  std::shared_ptr<BluetoothSocketImpl> bluetoothSocket = ndk::SharedRefBase::make<BluetoothSocketImpl>();
  const std::string bluetooth_socket_offload = std::string() + IBluetoothSocket::descriptor + "/default";
  if (bluetoothSocket) {
    auto result = AServiceManager_addService(bluetoothSocket->asBinder().get(), bluetooth_socket_offload.c_str());
    if (result != STATUS_OK) {
      ALOGE("Could not register google offload as a service!");
    } else {
        ALOGI("Registered google offload AIDL service!");
    }
  } else {
    ALOGE("Could not register google offload as a service because it's not declared.");
  }
#endif
#ifdef GATT_OFFLOAD_ENABLED  
  ALOGI("Registering Bluetooth GATT AIDL Service ..");
  std::shared_ptr<BluetoothGattImpl> bluetoothGatt = ndk::SharedRefBase::make<BluetoothGattImpl>();
  const std::string bluetooth_gatt_offload = std::string() + IBluetoothGatt::descriptor + "/default";
  ALOGD("bluetooth_gatt_offload value is %s", bluetooth_gatt_offload.c_str());
  if (AServiceManager_isDeclared(bluetooth_gatt_offload.c_str())) {
   //if(bluetoothGatt) {
     ALOGD("bluetooth_gatt_offload binder %p", bluetoothGatt->asBinder().get());
    auto result = AServiceManager_addService(bluetoothGatt->asBinder().get(), bluetooth_gatt_offload.c_str());
    if (result != STATUS_OK) {
      ALOGE("Could not register gatt offload as a service!");
    } else {
      ALOGI("Registered gatt offload AIDL service!");
    }
  } else {
    ALOGE("Could not register gatt offload as a service because it's not declared.");
  }
#endif

  //ALOGI("Main, joinRpcThreadpool for HIDL");
  //joinRpcThreadpool();
  ABinderProcess_joinThreadPool();
  ALOGI("Main, joinRpcThreadpool for AIDL");

  return 0;
}
