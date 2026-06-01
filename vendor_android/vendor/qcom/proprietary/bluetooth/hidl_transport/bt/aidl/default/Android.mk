LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.bluetooth@aidl-impl-qti
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := \
        bluetooth_hci.cpp \


LOCAL_CFLAGS += -DDIAG_ENABLED
LOCAL_CFLAGS += -Werror=unused-variable
# disable below flag to disable IBS
LOCAL_CFLAGS += -DWCNSS_IBS_ENABLED
# Disable this flag for disabling wakelocks
LOCAL_CFLAGS += -DWAKE_LOCK_ENABLED

#ifeq ($(BOARD_HAVE_QTI_BT_LAZY_SERVICE),true)
#LOCAL_CFLAGS += -DLAZY_SERVICE
#endif

ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
LOCAL_CFLAGS += -DDUMP_IPC_LOG -DDUMP_RINGBUF_LOG -DDETECT_SPURIOUS_WAKE -DENABLE_HEALTH_TIMER
#LOCAL_CFLAGS += -DDUMP_HEALTH_INFO_TO_FILE
#LOCAL_C_INCLUDES += vendor/qcom/proprietary/bt/hci_qcomm_init
LOCAL_CFLAGS += -DENABLE_FW_CRASH_DUMP -DUSER_DEBUG
endif
ifeq ($(TARGET_HAS_BT_QCV_FOR_SPF), true)
LOCAL_CPPFLAGS += -DQTI_BT_QCV_SUPPORTED
endif #TARGET_HAS_BT_QCV_FOR_SPF

ifeq ($(TARGET_BOARD_AUTO),true)
LOCAL_CPPFLAGS += -DTARGET_BOARD_AUTO
endif # TARGET_BOARD_AUTO

ifeq ($(ENABLE_PERIPHERAL_STATE_UTILS), true)
LOCAL_CFLAGS += -DSECURE_PERIFHERAL_ENABLED
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/mink/inc/uid
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/mink/inc/interface
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/peripheralStateUtils/inc
endif
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bttpi/default/

LOCAL_C_INCLUDES += vendor/qcom/proprietary/qmi/inc
LOCAL_C_INCLUDES += vendor/qcom/proprietary/qmi/platform
LOCAL_C_INCLUDES += vendor/qcom/proprietary/qcril-qmi-services/
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default/
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/fm/aidl/default/

LOCAL_ADDITIONAL_DEPENDENCIES += \
$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SHARED_LIBRARIES := \
    libbinder_ndk \
    libbase \
    libcutils \
    libhidlbase \
    liblog \
    libutils \
    libdiag \
    libqmiservices \
    libqmi_cci \
    libbtnv \
    android.hardware.bluetooth-V1-ndk \
    android.hardware.bluetooth@1.0-impl-qti \
    libsoc_helper

ifeq ($(BOARD_HAS_QTI_BT_THEMISTO),true)

  ifeq ($(BOARD_HAVE_QTI_SMC_HAL_V2), true)
    SMC_LIB := vendor.qti.hardware.smcloader-V2-ndk
  else
    SMC_LIB := vendor.qti.hardware.smcloader-V1-ndk
  endif

LOCAL_SHARED_LIBRARIES += \
    android.hardware.bluetooth.smcloader \
    $(SMC_LIB) \
    vendor.qti.hardware.smcloader@1.0-impl \

endif

LOCAL_HEADER_LIBRARIES := libril-qc-qmi-services-headers libdiag_headers vendor_common_inc

include $(BUILD_SHARED_LIBRARY)


ifeq ($(BOARD_HAS_QTI_BT_THEMISTO),true)
#smc loader
include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
ifeq ($(BOARD_HAVE_QTI_SMC_HAL_V2), true)
  LOCAL_CPPFLAGS += -DIS_SMC_HAL_V2
endif

LOCAL_MODULE := android.hardware.bluetooth.smcloader

LOCAL_SRC_FILES := \
        SMCLoader.cpp \
        SMCLoaderCallback.cpp \

LOCAL_C_INCLUDES += vendor/qcom/proprietary/wear-interfaces/smcloader/
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bt/aidl/default/
LOCAL_EXPORT_C_INCLUDE_DIRS := vendor/qcom/proprietary/bluetooth/hidl_transport/bt/aidl/default/

LOCAL_SHARED_LIBRARIES := \
    $(SMC_LIB) \
    vendor.qti.hardware.smcloader@1.0-impl \
    liblog \
    libutils \
    libbinder_ndk \

include $(BUILD_SHARED_LIBRARY)
endif

#service

include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE := android.hardware.bluetooth@aidl-service-qti

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
LOCAL_INIT_RC := android.hardware.bluetooth@aidl-service-qti-debug.rc
else
LOCAL_INIT_RC := android.hardware.bluetooth@aidl-service-qti.rc
endif

ifeq ($(TARGET_USE_HCI_MANIFEST_FRAGMENT),true)
LOCAL_VINTF_FRAGMENTS := bluetooth_hci.xml
endif

LOCAL_SRC_FILES := \
  service.cpp

ifeq ($(BOARD_HAS_QTI_BT_CP), true)
LOCAL_CFLAGS += -DBT_CP_CONNECTED
endif
LOCAL_SHARED_LIBRARIES := \
  libbase \
  liblog \
  libcutils \
  libutils \
  libhidlbase \
  libbinder_ndk \
  android.hardware.bluetooth-V1-ndk \
  android.hardware.bluetooth@1.0-impl-qti \
  vendor.qti.hardware.fm-V1-ndk\
  android.hardware.bluetooth@aidl-impl-qti \
  vendor.qti.hardware.bttpi-V3-ndk \
  libqmi_cci \
  libqmi \
  libqmiservices

ifeq ($(BOARD_HAS_QTI_BT_THEMISTO),true)
LOCAL_SHARED_LIBRARIES += \
  $(SMC_LIB) \
  vendor.qti.hardware.smcloader@1.0-impl \
  android.hardware.bluetooth.smcloader \

LOCAL_C_INCLUDES += vendor/qcom/proprietary/wear-interfaces/smcloader/
endif

ifeq ($(BOARD_HAVE_QTI_SOFTSKU),true)
LOCAL_SHARED_LIBRARIES += \
  libqcbor \
  libminksocket_vendor
endif

ifeq ($(BOARD_HAVE_QTI_SOFTSKU),true)
LOCAL_STATIC_LIBRARIES := \
  lib_softsku
endif

ifeq ($(ENABLE_PERIPHERAL_STATE_UTILS), true)
LOCAL_CFLAGS += -DSECURE_PERIFHERAL_ENABLED
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/mink/inc/uid
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/mink/inc/interface
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/peripheralStateUtils/inc
endif
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default/
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bttpi/default/
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bluetooth_sar/aidl/default/
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/fm/aidl/default/

LOCAL_HEADER_LIBRARIES := libdiag_headers vendor_common_inc

ifeq ($(BOARD_HAVE_QTI_SOFTSKU),true)
LOCAL_HEADER_LIBRARIES += mink_headers
LOCAL_CPPFLAGS += -DQTI_BT_SOFTSKU_SUPPORTED
endif

ifeq ($(TARGET_HAS_BT_QCV_FOR_SPF), true)
LOCAL_CPPFLAGS += -DQTI_BT_QCV_SUPPORTED
LOCAL_C_INCLUDES += vendor/qcom/proprietary/qcv-utils/libsoc-helper/native/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES += \
$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SHARED_LIBRARIES += libsoc_helper
LOCAL_SRC_FILES +=../../1.0/default/soc_properties.cpp
endif #TARGET_HAS_BT_QCV_FOR_SPF

ifeq ($(BOARD_HAVE_QCOM_FM),true)
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.fm@1.0
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.fm-impl
LOCAL_CPPFLAGS += -DQCOM_FM_SUPPORTED
endif #BOARD_HAVE_QCOM_FM

ifeq ($(BOARD_HAS_QTI_BT_XPAN),true)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/xpan/xpan_provider
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.xpanprovider-V1-ndk
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.xpanprovider-impl-qti
LOCAL_CPPFLAGS += -DQTI_BT_XPANPROVIDER_SUPPORTED
endif # BOARD_HAS_QTI_BT_XPAN

LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bttpi-impl

ifeq ($(TARGET_USE_QTI_BT_LMP_EVENT), true)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/btlmp_event/default/
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.lmp_event-V1-ndk
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.lmp_event-impl-qti
LOCAL_CPPFLAGS += -DQTI_BT_LMP_EVENT_SUPPORTED
endif # TARGET_USE_QTI_BT_LMP_EVENT

ifeq ($(TARGET_USE_QTI_BT_CONFIGSTORE),true)
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.btconfigstore@2.0
LOCAL_CPPFLAGS += -DQTI_BT_CONFIGSTORE_SUPPORTED
endif # TARGET_USE_QTI_BT_CONFIGSTORE

ifeq ($(BOARD_HAS_QTI_BT_FMD),true)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/finder/default
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.finder-V1-ndk
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.finder-impl-qti
LOCAL_CPPFLAGS += -DQTI_BT_FMD_SUPPORTED
endif # BOARD_HAS_QTI_BT_FMD

ifeq ($(TARGET_USE_QTI_VND_FWK_DETECT),true)
LOCAL_SHARED_LIBRARIES += libqti_vndfwk_detect_vendor
LOCAL_CPPFLAGS += -DQTI_VND_FWK_DETECT_SUPPORTED
endif # TARGET_USE_QTI_VND_FWK_DETECT

ifeq ($(BOARD_HAVE_QTI_BCS),true)
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/hidl_transport/ranging/default
ifeq ($(BOARD_HAVE_QTI_BCS_V2), true)
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.ranging-V2-ndk
LOCAL_CFLAGS += -DBCS_HAL_V2
else
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.ranging-V1-ndk
endif
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.ranging@aidl-impl-qti
LOCAL_CPPFLAGS += -DQTI_BT_BCS_SUPPORTED
endif 

ifeq ($(TARGET_USE_QTI_BT_SAR_AIDL),true)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bluetooth_sar
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth_sar@aidl-impl
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth_sar-V1-ndk
LOCAL_CPPFLAGS += -DQTI_BT_SAR_SUPPORTED
endif # TARGET_USE_QTI_BT_SAR_AIDL

ifeq ($(BOARD_HAS_PASSTHROUGH_ENABLED), true)
LOCAL_CFLAGS += -DPASSTHROUGH_ENABLED
LOCAL_SHARED_LIBRARIES +=  android.hardware.bluetooth.glink_transport
endif

ifeq ($(BOARD_HAS_GOOGLE_OFFLOAD_ENABLED),true)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bluetooth_socket/aidl/default

LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.socket-V1-ndk
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.socket-impl-qti
LOCAL_CPPFLAGS += -DGOOGLE_OFFLOAD_ENABLED
endif
ifeq ($(BOARD_HAS_GATT_OFFLOAD_ENABLED), true)
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/hidl_transport/bluetooth_gatt/aidl/default

ifeq ($(TARGET_VENDOR_GATT_HAL), true)
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.gatt-V1-ndk
LOCAL_CFLAGS += -DUSE_VENDOR_GATT_HAL=1
else
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.gatt-V1-ndk
LOCAL_CFLAGS += -DUSE_VENDOR_GATT_HAL=0
endif
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.gatt-impl-qti

LOCAL_CPPFLAGS += -DGATT_OFFLOAD_ENABLED
endif

include $(BUILD_EXECUTABLE)
include $(LOCAL_PATH)/fuzzer/Android.mk
