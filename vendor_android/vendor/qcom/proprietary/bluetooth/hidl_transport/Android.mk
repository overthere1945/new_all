LOCAL_DIR_PATH:= $(call my-dir)
ifeq ($(BOARD_HAVE_BLUETOOTH_QCOM),true)

LOCAL_PATH := $(LOCAL_DIR_PATH)
TMP_LOCAL_PATH := $(LOCAL_DIR_PATH)
LOC_PATH := $(LOCAL_DIR_PATH)
ifeq ($(BOARD_HAVE_QTI_BT_SERVICE_VER_1_1), true)
include $(LOCAL_PATH)/bt/1.1/default/Android.mk
else
ifeq ($(BOARD_HAVE_QTI_BT_AIDL_SERVICE), true)
include $(LOC_PATH)/bt/aidl/default/Android.mk
endif
endif
include $(TMP_LOCAL_PATH)/bt/1.0/default/Android.mk

ifeq ($(TARGET_USE_QTI_BT_SAR),true)
LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/bluetooth_sar/1.1/default/Android.mk
endif # TARGET_USE_QTI_BT_SAR

ifeq ($(TARGET_USE_QTI_BT_SAR_AIDL),true)
LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/bluetooth_sar/aidl/default/Android.mk
endif # TARGET_USE_QTI_BT_SAR_AIDL

LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/bttpi/default/Android.mk

ifeq ($(TARGET_USE_QTI_BT_LMP_EVENT), true)
LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/btlmp_event/default/Android.mk
endif # TARGET_USE_QTI_BT_LMP_EVENT

ifeq ($(TARGET_USE_QTI_BT_CONFIGSTORE),true)
LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/btconfigstore/1.0/default/Android.mk

TMP_LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(TMP_LOCAL_PATH)/btconfigstore/2.0/default/Android.mk
endif # TARGET_USE_QTI_BT_CONFIGSTORE

ifeq ($(BOARD_HAS_QTI_BT_FMD), true)
LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/finder/default/Android.mk
endif #BOARD_HAS_QTI_BT_FMD

ifeq ($(BOARD_HAVE_QTI_BCS), true)
LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/ranging/default/Android.mk
endif #BOARD_HAVE_QTI_BCS

ifeq ($(BOARD_HAS_GOOGLE_OFFLOAD_ENABLED), true)
LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/bluetooth_socket/aidl/default/Android.mk

# change(add)-hyungchul-20260512-0001: HLOS LECoC socket offload bridge/add test CLI build
# - Define libqc_lecoc_socket_offload_bridge and qc_lecoc_offload_cli in qc_lecoc_offload_bridge/Android.mk.
# - This include registers the module definitions with the Android build system.
# - To have qc_lecoc_offload_cli included in the vendor image automatically, add qc_lecoc_offload_cli to PRODUCT_PACKAGES as well.
LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/qc_lecoc_offload_bridge/Android.mk
endif

ifeq ($(BOARD_HAS_GATT_OFFLOAD_ENABLED), true)
LOCAL_PATH := $(LOCAL_DIR_PATH)
include $(LOCAL_PATH)/bluetooth_gatt/aidl/default/Android.mk
endif

endif # BOARD_HAVE_BLUETOOTH_QCOM

ifeq ($(BOARD_HAVE_QCOM_FM),true)
LOCAL_PATH := $(LOCAL_DIR_PATH)
ifeq ($(BOARD_HAVE_QTI_FM_AIDL_SERVICE), true)
include $(LOCAL_PATH)/fm/aidl/default/Android.mk
endif
endif # BOARD_HAVE_QCOM_FM
