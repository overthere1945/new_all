include $(CLEAR_VARS)
LOCAL_MODULE := aidl_fuzzer_bt_fmd
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true

LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default


LOCAL_SRC_FILES := \
    fuzzer/fuzzer.cpp \

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libcutils \
    libbinder \
    libbinder_ndk \
    android.hardware.bluetooth-V1-ndk \
    android.hardware.bluetooth.finder-V1-ndk \
    vendor.qti.hardware.bluetooth.finder-impl-qti \
    libsoc_helper \

LOCAL_STATIC_LIBRARIES += libbinder_random_parcel
include $(BUILD_FUZZ_TEST)