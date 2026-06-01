LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := vendor.qti.hardware.bluetooth.gatt-impl-qti
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
    bluetooth_gatt_offload_impl.cpp

BT_HAL_DIR:= $(QC_PROP_ROOT)/bluetooth/hidl_transport/bt/1.0/default/

LOCAL_C_INCLUDES +=  $(BT_HAL_DIR)
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/common/inc
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/build/include
LOCAL_HEADER_LIBRARIES := libdiag_headers

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libbinder_ndk \
    libutils \
    liblog \
    libcutils \
    libhardware \
    android.hardware.bluetooth@1.0 \
    android.hardware.bluetooth.glink_transport \


ifeq ($(TARGET_VENDOR_GATT_HAL), true)
    LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.gatt-V1-ndk
    LOCAL_CFLAGS += -DUSE_VENDOR_GATT_HAL=1
else
    LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.gatt-V1-ndk
    LOCAL_CFLAGS += -DUSE_VENDOR_GATT_HAL=0
endif

# compile proto library for google offload
ifeq ($(BOARD_HAS_GATT_OFFLOAD_ENABLED),true)
LOCAL_CPPFLAGS += -DGATT_OFFLOAD_ENABLED
LOCAL_SHARED_LIBRARIES += \
  gatt_offload_proto \
  libprotobuf-cpp-full

endif

ifeq ($(TARGET_VENDOR_GATT_HAL), true)
LOCAL_VINTF_FRAGMENTS := bluetooth_vendor_qti_gatt_offload.xml
else
LOCAL_VINTF_FRAGMENTS := bluetooth_gatt_offload.xml
endif

include $(BUILD_SHARED_LIBRARY)
