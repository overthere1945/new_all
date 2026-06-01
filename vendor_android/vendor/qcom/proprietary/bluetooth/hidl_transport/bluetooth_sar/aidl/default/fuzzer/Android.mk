include $(CLEAR_VARS)
LOCAL_MODULE := aidl_fuzzer_bt_sar
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true

LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default
LOCAL_C_INCLUDES += vendor/qcom/proprietary/common/inc


LOCAL_SRC_FILES := \
    fuzzer/fuzzer.cpp \

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    liblog \
    libutils \
    libcutils \
    libbinder \
    libdiag \
    libbinder_ndk \
    vendor.qti.hardware.bluetooth_sar-V1-ndk \
    vendor.qti.hardware.bluetooth_sar@aidl-impl \
    android.hardware.bluetooth@aidl-impl-qti \
    android.hardware.bluetooth@1.0-impl-qti \
    libsoc_helper \

LOCAL_STATIC_LIBRARIES += libbinder_random_parcel
include $(BUILD_FUZZ_TEST)