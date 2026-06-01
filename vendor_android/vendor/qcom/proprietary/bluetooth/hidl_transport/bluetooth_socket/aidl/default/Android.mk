LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := vendor.qti.hardware.bluetooth.socket-impl-qti
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
    bluetooth_socket_impl.cpp

BT_HAL_DIR:= vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default

LOCAL_C_INCLUDES +=  $(BT_HAL_DIR)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/common/inc
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/build/include
LOCAL_HEADER_LIBRARIES := libdiag_headers

LOCAL_SHARED_LIBRARIES := \
    android.hardware.bluetooth.socket-V1-ndk \
    libbase \
    libbinder_ndk \
    libutils \
    liblog \
    libcutils \
    libhardware \
    android.hardware.bluetooth@1.0 \
    android.hardware.bluetooth.glink_transport \

# compile proto library for google offload
ifeq ($(BOARD_HAS_GOOGLE_OFFLOAD_ENABLED),true)
LOCAL_CPPFLAGS += -DGOOGLE_OFFLOAD_ENABLED
LOCAL_SHARED_LIBRARIES += \
  google_offload_proto \
  libprotobuf-cpp-full \

endif

LOCAL_VINTF_FRAGMENTS := bluetooth_socket.xml

include $(BUILD_SHARED_LIBRARY)
