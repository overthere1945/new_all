# ==============================================================================
# 파일명: Android.mk
# 목적 및 기능:
# - qc_lecoc_offload_bridge native static library를 빌드한다.
# - qc_lecoc_offload_cli vendor executable을 빌드한다.
# - HLOS vendor Bluetooth stack에서 LECoC socket context를
#   android.hardware.bluetooth.socket AIDL service로 전달하기 위한 bridge 코드이다.
# ==============================================================================

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libqc_lecoc_socket_offload_bridge
LOCAL_VENDOR_MODULE := true
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := \
    QcLecocSocketOffloadBridge.cpp

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := \
    android.hardware.bluetooth.socket-V1-ndk \
    android.hardware.contexthub-V4-ndk \
    libbase \
    libbinder_ndk \
    liblog

LOCAL_CFLAGS += \
    -Wall \
    -Wextra \
    -Werror

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := qc_lecoc_offload_cli
LOCAL_VENDOR_MODULE := true
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := \
    QcLecocOffloadCli.cpp

LOCAL_STATIC_LIBRARIES := \
    libqc_lecoc_socket_offload_bridge

LOCAL_SHARED_LIBRARIES := \
    android.hardware.bluetooth.socket-V1-ndk \
    android.hardware.contexthub-V4-ndk \
    libbase \
    libbinder_ndk \
    liblog

LOCAL_CFLAGS += \
    -Wall \
    -Wextra \
    -Werror

include $(BUILD_EXECUTABLE)
