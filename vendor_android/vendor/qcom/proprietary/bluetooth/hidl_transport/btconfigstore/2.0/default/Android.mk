LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := vendor.qti.hardware.btconfigstore@2.0-impl
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_SRC_FILES := \
    BTConfigStore.cpp \

BT_HAL_DIR:= $(QC_PROP_ROOT)/bluetooth/hidl_transport/bt/1.0/default
LOCAL_C_INCLUDES += $(BT_HAL_DIR)
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/common/inc
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/build/include
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/mink/inc/uid
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/mink/inc/interface
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/peripheralStateUtils/inc
ifeq ($(TARGET_HAS_BT_QCV_FOR_SPF), true)
LOCAL_CPPFLAGS += -DQTI_BT_QCV_SUPPORTED
endif

LOCAL_HEADER_LIBRARIES := libdiag_headers

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libcutils \
    libhidlbase \
    liblog \
    libutils \
    vendor.qti.hardware.btconfigstore@2.0 \
    android.hardware.bluetooth@1.0-impl-qti

include $(BUILD_SHARED_LIBRARY)
