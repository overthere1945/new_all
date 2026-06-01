LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.bluetooth.ranging@aidl-impl-qti
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
    BluetoothChannelSounding.cpp \
    BluetoothChannelSoundingSession.cpp \
    BluetoothChannelSoundingAlgo.cpp

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libbinder_ndk \
    libutils \
    libcutils \
    liblog \
    libhidlbase \
    qc_bcs_lib \

ifeq ($(BOARD_HAVE_QTI_BCS_V2), true)
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.ranging-V2-ndk
LOCAL_CFLAGS += -DBCS_HAL_V2
else
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.ranging-V1-ndk
endif

LOCAL_VINTF_FRAGMENTS := bcs-ranging.xml
include $(BUILD_SHARED_LIBRARY)
include $(LOCAL_PATH)/fuzzer/Android.mk
