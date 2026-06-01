LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := vendor.qti.hardware.bluetooth.finder-impl-qti
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
    BluetoothFinder.cpp

BT_HAL_DIR:= vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default

LOCAL_C_INCLUDES +=  $(BT_HAL_DIR)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/common/inc
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/build/include
LOCAL_HEADER_LIBRARIES := libdiag_headers

LOCAL_SHARED_LIBRARIES := \
    android.hardware.bluetooth.finder-V1-ndk \
    libbase \
    libbinder_ndk \
    libutils \
    liblog \
    libcutils \
    libhardware \
    android.hardware.bluetooth@1.0 \
    android.hardware.bluetooth@1.0-impl-qti \

LOCAL_VINTF_FRAGMENTS := bluetooth-finder.xml

ifeq ($(BOARD_HAS_QTI_BT_FMD),true)
LOCAL_CPPFLAGS += -DQTI_BT_FMD_SUPPORTED
endif # BOARD_HAS_QTI_BT_FMD

include $(BUILD_SHARED_LIBRARY)
include $(LOCAL_PATH)/fuzzer/Android.mk
