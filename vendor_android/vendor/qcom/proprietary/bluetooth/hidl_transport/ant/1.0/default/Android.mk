ifneq ($(BOARD_ANT_WIRELESS_DEVICE),)

LOCAL_PATH := $(call my-dir)
QC_PROP_ROOT ?= vendor/qcom/proprietary

include $(CLEAR_VARS)
LOCAL_MODULE := com.dsi.ant@1.0-impl
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_SRC_FILES := \
   AntHci.cpp \

BT_HAL_DIR:= $(QC_PROP_ROOT)/bluetooth/hidl_transport/bt/1.0/default
LOCAL_C_INCLUDES +=  $(BT_HAL_DIR)
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/mink/inc/uid
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/mink/inc/interface
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/peripheralStateUtils/inc

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libcutils \
    libhidlbase \
    liblog \
    libutils \
    com.dsi.ant@1.0 \
    android.hardware.bluetooth@1.0 \
    android.hardware.bluetooth@1.0-impl-qti \

LOCAL_HEADER_LIBRARIES := libdiag_headers vendor_common_inc

include $(BUILD_SHARED_LIBRARY)

endif # BOARD_ANT_WIRELESS_DEVICE defined
