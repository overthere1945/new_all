LOCAL_PATH:= $(call my-dir)



include $(CLEAR_VARS)

LOCAL_SRC_FILES:=     \
    src/hidl_client.cpp    \
    src/client_reader.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc

LOCAL_CFLAGS += -Wno-unused-variable
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-macro-redefined

LOCAL_MODULE:= libbt-hidlclient
LOCAL_MODULE_SUFFIX:= .so
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libhidlbase
ifneq ($(BOARD_ANT_WIRELESS_DEVICE),)
ifneq ($(TARGET_BOARD_PLATFORM),monaco)
LOCAL_SHARED_LIBRARIES += com.dsi.ant@1.0
LOCAL_CPPFLAGS += -DQCOM_ANT_SUPPORTED
endif
endif
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.fm@1.0
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libbinder
LOCAL_SHARED_LIBRARIES += libbinder_ndk
ifeq ($(BOARD_HAVE_QTI_BT_AIDL_SERVICE), true)
LOCAL_CPPFLAGS += -DCONFIG_BT_AIDL_SUPPORTED
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth-V1-ndk
else
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth@1.0
endif

LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
