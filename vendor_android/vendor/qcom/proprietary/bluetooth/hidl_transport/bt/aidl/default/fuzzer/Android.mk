include $(CLEAR_VARS)
LOCAL_MODULE := aidl_fuzzer_bt_hci
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true

LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bt

LOCAL_SRC_FILES := \
    fuzzer/fuzzer.cpp \

LOCAL_SHARED_LIBRARIES := \
    libbinder_ndk \
    libbinder \
    libcutils \
    liblog \
    libutils \
    libdiag \
    android.hardware.bluetooth-V1-ndk \
    android.hardware.bluetooth@aidl-impl-qti \
    libsoc_helper

LOCAL_STATIC_LIBRARIES += libbinder_random_parcel
include $(BUILD_FUZZ_TEST)
