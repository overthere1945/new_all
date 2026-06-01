LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE :=  vendor.qti.hardware.fm-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := \
    FmHci.cpp\
    FmIoctlHci.cpp\
    FmIoctlsInterface.cpp

BT_HAL_DIR:= vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default

LOCAL_C_INCLUDES +=  $(BT_HAL_DIR)


LOCAL_HEADER_LIBRARIES := libdiag_headers vendor_common_inc

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libutils \
    liblog \
    libbase \
    libcutils \
    libhardware \
    libbinder_ndk \
    android.hardware.bluetooth@1.0-impl-qti \
    vendor.qti.hardware.fm-V1-ndk \
    libqmi_cci \
    libqmi \
    libqmiservices \


LOCAL_VINTF_FRAGMENTS := fm_hci.xml
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  vendor.qti.hardware.fm-aidl_fuzzer
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := \
    Fuzzer.cpp

BT_HAL_DIR:= vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default

LOCAL_C_INCLUDES +=  $(BT_HAL_DIR)


LOCAL_HEADER_LIBRARIES := libdiag_headers vendor_common_inc
LOCAL_STATIC_LIBRARIES += libbinder_random_parcel

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libutils \
    liblog \
    libbase \
    libcutils \
    libhardware \
    libbinder_ndk \
    libbinder\
    vendor.qti.hardware.fm-impl\
    vendor.qti.hardware.fm-V1-ndk \
    libqmi_cci \
    libqmi \
    libqmiservices \

include $(BUILD_FUZZ_TEST)
