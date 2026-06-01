LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.bluetooth@1.0-impl-qti
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

ifeq ($(UART_BAUDRATE_3_0_MBPS),true)
LOCAL_CPPFLAGS := -DUART_BAUDRATE_3_0_MBPS
endif

LOCAL_SRC_FILES := \
    bluetooth_hci.cpp \
    bluetooth_address.cpp \
    async_fd_watcher.cpp \
    hci_packetizer.cpp \
    soc_properties.cpp \
    data_handler.cpp \
    uart_controller.cpp \
    patch_dl_manager.cpp \
    hci_uart_transport.cpp \
    power_manager.cpp \
    ibs_handler.cpp \
    logger.cpp \
    transport_ipc.cpp \
    transport_logs.cpp \
    ring_buffer.cpp \
    wake_lock.cpp \
    diag_interface.cpp \
    mct_controller.cpp \
    hci_mct_transport.cpp \
    nvm_tags_manager.cpp \
    health_info_log.cpp \
    state_info.cpp \
    controller.cpp

IS_PERI_ENABLED := $(or $(findstring true,$(BOARD_HAS_QTI_BT_GANGES)),$(findstring true,$(BOARD_HAS_QTI_BT_THEMISTO)))

ifeq ($(IS_PERI_ENABLED),true)
LOCAL_CPPFLAGS := -DIS_PERI_ENABLED
LOCAL_SRC_FILES += \
    hci_spi_transport.cpp \
    spi_controller.cpp \
    peri_patch_dl_manager.cpp \
    notify_signal.cpp
endif

ifeq ($(TARGET_SUPPORTS_WEAR_OS), true)
LOCAL_CPPFLAGS += -DIS_LW_VARIANT
endif

ifeq ($(TARGET_SUPPORTS_WEAR_ANDROID),true)
LOCAL_CPPFLAGS += -DIS_LAW_VARIANT
endif

ifeq ($(TARGET_USE_QTI_BT_LMP_EVENT), true)
LOCAL_CPPFLAGS += -DQTI_BT_LMP_EVENT_SUPPORTED
endif # TARGET_USE_QTI_BT_LMP_EVENT

ifeq ($(BOARD_HAS_QTI_BT_FMD),true)
LOCAL_CPPFLAGS += -DQTI_BT_FMD_SUPPORTED
endif # BOARD_HAS_QTI_BT_FMD

ifeq ($(BOARD_HAVE_QTI_SMC_HAL_V2), true)
  SMC_LIB := vendor.qti.hardware.smcloader-V2-ndk
  LOCAL_CFLAGS += -DIS_SMC_HAL_V2
else
  SMC_LIB := vendor.qti.hardware.smcloader-V1-ndk
endif

ifeq ($(BOARD_HAS_QTI_BT_FMD_SW_WAR_DISABLE),true)
LOCAL_CPPFLAGS += -DQTI_BT_FMD_SW_WAR_DISABLE
endif # BOARD_HAS_QTI_BT_FMD_SW_WAR_DISABLE

ifeq ($(BOARD_HAVE_QTI_BT_SERVICE_VER_1_1), true)
LOCAL_CFLAGS += -DBT_VER_1_1
endif

LOCAL_CFLAGS += -DDIAG_ENABLED
LOCAL_CFLAGS += -Werror=unused-variable

# disable below flag to disable IBS/OBS
ifeq ($(TARGET_USE_QTI_BT_OBS), true)
LOCAL_CFLAGS += -DWCNSS_OBS_ENABLED
LOCAL_SRC_FILES += obs_handler.cpp
else
LOCAL_CFLAGS += -DWCNSS_IBS_ENABLED
endif

# Disable this flag for disabling wakelocks
LOCAL_CFLAGS += -DWAKE_LOCK_ENABLED

ifeq ($(BOARD_HAVE_QTI_BT_LAZY_SERVICE),true)
LOCAL_CFLAGS += -DLAZY_SERVICE
endif

LOCAL_CFLAGS += -DDUMP_RINGBUF_LOG
ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
LOCAL_CFLAGS += -DDUMP_IPC_LOG -DDETECT_SPURIOUS_WAKE -DENABLE_HEALTH_TIMER
#LOCAL_CFLAGS += -DDUMP_HEALTH_INFO_TO_FILE
#LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bt/hci_qcomm_init
LOCAL_CFLAGS += -DENABLE_FW_CRASH_DUMP -DUSER_DEBUG
endif
ifeq ($(TARGET_HAS_BT_QCV_FOR_SPF), true)
LOCAL_CPPFLAGS += -DQTI_BT_QCV_SUPPORTED
endif #TARGET_HAS_BT_QCV_FOR_SPF

ifeq ($(TARGET_BOARD_AUTO),true)
LOCAL_CPPFLAGS += -DTARGET_BOARD_AUTO
endif # TARGET_BOARD_AUTO

ifeq ($(ENABLE_PERIPHERAL_STATE_UTILS), true)
LOCAL_CFLAGS += -DSECURE_PERIFHERAL_ENABLED
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/mink/inc/uid
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/mink/inc/interface
endif
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/hidl_transport/bttpi/default/

LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/qmi/inc
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/qmi/platform
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/qcril-qmi-services/
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_ADDITIONAL_DEPENDENCIES += \
$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libcutils \
    libhidlbase \
    liblog \
    libutils \
    libdiag \
    libqmiservices \
    libqmi_cci \
    libbtnv \
    android.hardware.bluetooth@1.0 \
    libsoc_helper \
    libssl \
    mbedtls_qti \
    libcrypto \
    libbinder_ndk \

ifeq ($(IS_PERI_ENABLED),true)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bt/aidl/default/
LOCAL_C_INCLUDES += vendor/qcom/proprietary/wear-interfaces/smcloader/
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.smcloader \
    $(SMC_LIB) \
    vendor.qti.hardware.smcloader@1.0-impl \

endif

ifeq ($(strip $(ENABLE_PERIPHERAL_STATE_UTILS)), true)
LOCAL_CFLAGS += -DBT_SECURE_PERIPHERAL_ENABLED
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/peripheralStateUtils/inc
LOCAL_SHARED_LIBRARIES +=  \
    libminksocket_vendor \
    libPeripheralStateUtils
LOCAL_HEADER_LIBRARIES += peripheralstate_headers
endif

ifeq ($(ENABLE_PERIPHERAL_STATE_UTILS), true)
LOCAL_SHARED_LIBRARIES += libminksocket_vendor \
                          libPeripheralStateUtils
endif

ifeq ($(BOARD_HAS_QTI_BT_XPAN), true)
LOCAL_CFLAGS += -DBT_XPAN_ENABLED
LOCAL_SHARED_LIBRARIES += libxpan_wifi_hal \
                          libbinder_ndk \
                          vendor.qti.hardware.bluetooth.xpanprovider-V1-ndk
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.xpanprovider-impl-qti
LOCAL_STATIC_LIBRARIES += libbtxpan_qhci
LOCAL_STATIC_LIBRARIES += vendor.qti.hardware.bluetooth.xpan_application_controller
endif

ifeq ($(BOARD_HAS_QTI_BT_XPAN), true)
LOCAL_CFLAGS += -DXPAN_SUPPORTED
LOCAL_STATIC_LIBRARIES += libbtxpan_qhci
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/xpan/xpan_manager/include
LOCAL_C_INCLUDES += $(TOP)/$(QC_PROP_ROOT)/bluetooth/xpan/qhci/include
LOCAL_C_INCLUDES += vendor/qcom/opensource/bt-external/include
LOCAL_C_INCLUDES += external/openssl/include
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/xpan/xpan_application_controller/include
endif

ifeq ($(BOARD_HAS_QTI_BT_CP), true)
LOCAL_CFLAGS += -DBT_CP_CONNECTED
LOCAL_STATIC_LIBRARIES += libbtxpanmanager
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/xpan/xpan_manager/include
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/xpan/common/include
endif

LOCAL_CFLAGS += -DBTOFF_DELAY_SUPPORT

LOCAL_HEADER_LIBRARIES := libril-qc-qmi-services-headers libdiag_headers vendor_common_inc

ifeq ($(BOARD_HAS_PASSTHROUGH_ENABLED), true)
LOCAL_CFLAGS += -DPASSTHROUGH_ENABLED

LOCAL_SHARED_LIBRARIES += \
  android.hardware.bluetooth.glink_transport

endif

ifeq ($(BOARD_HAS_GOOGLE_OFFLOAD_ENABLED), true)
LOCAL_CFLAGS += -DGOOGLE_OFFLOAD_ENABLED
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bluetooth_socket/aidl/default

LOCAL_SHARED_LIBRARIES += \
  google_offload_proto \
  libprotobuf-cpp-lite \
  vendor.qti.hardware.bluetooth.socket-impl-qti \
  android.hardware.bluetooth.socket-V1-ndk 
endif

ifeq ($(BOARD_HAS_GATT_OFFLOAD_ENABLED), true)
LOCAL_CFLAGS += -DGATT_OFFLOAD_ENABLED
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/hidl_transport/bluetooth_gatt/aidl/default

LOCAL_SHARED_LIBRARIES += \
  gatt_offload_proto \
  libprotobuf-cpp-lite \
  vendor.qti.hardware.bluetooth.gatt-impl-qti

ifeq ($(TARGET_VENDOR_GATT_HAL), true)
LOCAL_SHARED_LIBRARIES += \
  vendor.qti.hardware.bluetooth.gatt-V1-ndk
LOCAL_CFLAGS += -DUSE_VENDOR_GATT_HAL=1
else
LOCAL_SHARED_LIBRARIES += \
  android.hardware.bluetooth.gatt-V1-ndk
LOCAL_CFLAGS += -DUSE_VENDOR_GATT_HAL=0
endif
endif

include $(BUILD_SHARED_LIBRARY)


# Glink Transport Lib
ifeq ($(BOARD_HAS_PASSTHROUGH_ENABLED), true)

include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE := android.hardware.bluetooth.glink_transport

LOCAL_SRC_FILES += \
  hci_glink_transport.cpp

LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bt/1.0/default/

LOCAL_SHARED_LIBRARIES += \
  libcutils \
  libutils \
  liblog \

include $(BUILD_SHARED_LIBRARY)
endif

# compile service 1.0 if flag is set
ifneq ($(BOARD_HAVE_QTI_BT_SERVICE_VER_1_1), true)
ifneq ($(BOARD_HAVE_QTI_BT_AIDL_SERVICE), true)
include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

ifeq ($(BOARD_HAVE_QTI_BT_LAZY_SERVICE),true)
LOCAL_MODULE := android.hardware.bluetooth@1.0-service-qti-lazy
LOCAL_CPPFLAGS += -DLAZY_SERVICE
ifeq ($(TARGET_BOARD_PLATFORM), msm8937)
LOCAL_INIT_RC := lazy-hal-rc/msm8937_32go/android.hardware.bluetooth@1.0-service-qti-lazy.rc
else
LOCAL_INIT_RC := lazy-hal-rc/common/android.hardware.bluetooth@1.0-service-qti-lazy.rc
endif
else
LOCAL_MODULE := android.hardware.bluetooth@1.0-service-qti
ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
LOCAL_INIT_RC := android.hardware.bluetooth@1.0-service-qti-debug.rc
else
LOCAL_INIT_RC := android.hardware.bluetooth@1.0-service-qti.rc
endif
endif #BOARD_HAVE_QTI_BT_LAZY_SERVICE

LOCAL_SRC_FILES := \
  service.cpp

LOCAL_SHARED_LIBRARIES := \
  liblog \
  libcutils \
  libutils \
  libhidlbase \
  libbinder_ndk \
  android.hardware.bluetooth@1.0 \
  vendor.qti.hardware.bttpi-V3-ndk \
  libssl \
  mbedtls_qti \
  libcrypto \

ifeq ($(ENABLE_PERIPHERAL_STATE_UTILS), true)
LOCAL_CFLAGS += -DSECURE_PERIFHERAL_ENABLED
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/mink/inc/uid
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/mink/inc/interface
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/securemsm/peripheralStateUtils/inc
endif
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/hidl_transport/bttpi/default/
LOCAL_C_INCLUDES += external/openssl/include
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/xpan/xpan_application_controller/include
LOCAL_HEADER_LIBRARIES := libdiag_headers vendor_common_inc

ifeq ($(TARGET_HAS_BT_QCV_FOR_SPF), true)
LOCAL_CPPFLAGS += -DQTI_BT_QCV_SUPPORTED
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/qcv-utils/libsoc-helper/native/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES += \
$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SHARED_LIBRARIES += libsoc_helper
LOCAL_SRC_FILES += soc_properties.cpp
endif #TARGET_HAS_BT_QCV_FOR_SPF

ifeq ($(BOARD_HAVE_QCOM_FM),true)
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.fm@1.0
LOCAL_CPPFLAGS += -DQCOM_FM_SUPPORTED
endif #BOARD_HAVE_QCOM_FM

ifneq ($(filter msm8909 msm8937 msm8953 vienna monaco,$(TARGET_BOARD_PLATFORM)),)
else
ifeq ($(BOARD_ANT_WIRELESS_DEVICE),"qualcomm-hidl")
LOCAL_SHARED_LIBRARIES += com.dsi.ant@1.0
LOCAL_CPPFLAGS += -DQCOM_ANT_SUPPORTED
endif
endif

ifeq ($(TARGET_USE_QTI_BT_SAR),true)
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth_sar@1.0
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth_sar@1.1
LOCAL_CPPFLAGS += -DQTI_BT_SAR_SUPPORTED
endif # TARGET_USE_QTI_BT_SAR

LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bttpi-impl

ifeq ($(TARGET_USE_QTI_BT_CONFIGSTORE),true)
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.btconfigstore@1.0
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.btconfigstore@2.0
LOCAL_CPPFLAGS += -DQTI_BT_CONFIGSTORE_SUPPORTED
endif # TARGET_USE_QTI_BT_CONFIGSTORE

ifeq ($(BOARD_HAS_GOOGLE_OFFLOAD_ENABLED),true)
LOCAL_C_INCLUDES += vendor/qcom/proprietary/bluetooth/hidl_transport/bluetooth_socket/aidl/default

LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.socket-V1-ndk
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.socket-impl-qti
LOCAL_CPPFLAGS += -DGOOGLE_OFFLOAD_ENABLED
endif
ifeq ($(BOARD_HAS_GATT_OFFLOAD_ENABLED), true)
LOCAL_C_INCLUDES += $(QC_PROP_ROOT)/bluetooth/hidl_transport/bluetooth_gatt/aidl/default

ifeq ($(TARGET_VENDOR_GATT_HAL), true)
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.gatt-V1-ndk
else
LOCAL_SHARED_LIBRARIES += android.hardware.bluetooth.gatt-V1-ndk
endif
LOCAL_SHARED_LIBRARIES += vendor.qti.hardware.bluetooth.gatt-impl-qti
LOCAL_CPPFLAGS += -DGATT_OFFLOAD_ENABLED
endif
ifeq ($(TARGET_USE_QTI_VND_FWK_DETECT),true)
LOCAL_SHARED_LIBRARIES += libqti_vndfwk_detect_vendor
LOCAL_CPPFLAGS += -DQTI_VND_FWK_DETECT_SUPPORTED
endif # TARGET_USE_QTI_VND_FWK_DETECT

ifeq ($(TARGET_USE_WEAR_QC_BT_STACK),true)
LOCAL_CPPFLAGS += -DQTI_WEAR_QC_BT_STACK_SUPPORTED
endif # TARGET_USE_QTI_VND_FWK_DETECT

ifeq ($(TARGET_SUPPORTS_WEARABLES),true)
LOCAL_CPPFLAGS += -DQTI_TARGET_SUPPORT_WERABLES
endif # TARGET_SUPPORTS_WEARABLES




include $(BUILD_EXECUTABLE)
endif
endif
