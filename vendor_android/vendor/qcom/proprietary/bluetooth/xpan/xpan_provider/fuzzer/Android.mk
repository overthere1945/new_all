include $(CLEAR_VARS)
LOCAL_MODULE := aidl_fuzzer_xpan_provider
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true

LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/xpan/common/include

LOCAL_SRC_FILES := \
    fuzzer/fuzzer.cpp \

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libbase \
    libhardware \
    libutils \
    libcutils \
    libbinder \
    libbinder_ndk \
    vendor.qti.hardware.bluetooth.xpanprovider-V1-ndk \
    vendor.qti.hardware.bluetooth.xpanprovider-impl-qti

LOCAL_STATIC_LIBRARIES += libbinder_random_parcel
include $(BUILD_FUZZ_TEST)