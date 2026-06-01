LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := aidl_fuzzer_bttpi
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := \
    fuzzer.cpp

BT_HAL_DIR:= vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default

LOCAL_C_INCLUDES +=  $(LOCAL_PATH)/../
LOCAL_C_INCLUDES +=  $(BT_HAL_DIR)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/mink/inc/uid
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/mink/inc/interface
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/peripheralStateUtils/inc

LOCAL_HEADER_LIBRARIES := libdiag_headers vendor_common_inc

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libutils \
    liblog \
    libbase \
    libcutils \
    libhardware \
    libbinder_ndk \
    android.hardware.bluetooth@1.0 \
    android.hardware.bluetooth@1.0-impl-qti \
    vendor.qti.hardware.bttpi-V3-ndk \
	vendor.qti.hardware.bttpi-impl \
	libbinder

LOCAL_STATIC_LIBRARIES += libbinder_random_parcel
include $(BUILD_FUZZ_TEST)