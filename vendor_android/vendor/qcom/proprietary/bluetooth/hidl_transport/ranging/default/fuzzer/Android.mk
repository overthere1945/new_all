include $(CLEAR_VARS)
LOCAL_MODULE := aidl_fuzzer_bt_bcs
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true

LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport
LOCAL_SRC_FILES := \
    fuzzer/fuzzer.cpp \

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libcutils \
    libbinder \
    libbinder_ndk \
    android.hardware.bluetooth.ranging@aidl-impl-qti \

ifeq ($(BOARD_HAVE_QTI_BCS_V2), true)
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.ranging-V2-ndk
LOCAL_CFLAGS += -DBCS_HAL_V2
else
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.ranging-V1-ndk
endif

LOCAL_STATIC_LIBRARIES += libbinder_random_parcel
include $(BUILD_FUZZ_TEST)
