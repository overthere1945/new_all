LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_USE_QTI_BT_LMP_EVENT), true)
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.bluetooth.lmp_event-impl-qti
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := \
    bt_lmp_event.cpp

BT_HAL_DIR := vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default

LOCAL_C_INCLUDES += $(BT_HAL_DIR)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/mink/inc/uid
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/mink/inc/interface
LOCAL_C_INCLUDES += vendor/qcom/proprietary/securemsm/peripheralStateUtils/inc

LOCAL_HEADER_LIBRARIES := libdiag_headers vendor_common_inc

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libcutils \
    libbase \
    libbinder_ndk \
    libhardware \
    libhidlbase \
    android.hardware.bluetooth.lmp_event-V1-ndk \
    android.hardware.bluetooth@1.0-impl-qti

ifeq ($(TARGET_USE_QTI_BT_LMP_EVENT), true)
LOCAL_CPPFLAGS += -DQTI_BT_LMP_EVENT_SUPPORTED
endif # TARGET_USE_QTI_BT_LMP_EVENT

LOCAL_VINTF_FRAGMENTS := bt_lmp_event-saidl.xml

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/fuzzer/Android.mk
endif # TARGET_USE_QTI_BT_LMP_EVENT
