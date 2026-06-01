#BT
ifeq ($(BOARD_HAVE_QTI_BT_LAZY_SERVICE), true)
ifeq ($(BOARD_HAVE_QTI_BT_SERVICE_VER_1_1), true)
BT_HAL_VER = android.hardware.bluetooth@1.1-service-qti-lazy
else
BT_HAL_VER = android.hardware.bluetooth@1.0-service-qti-lazy
endif
else
ifeq ($(BOARD_HAVE_QTI_BT_SERVICE_VER_1_1), true)
BT_HAL_VER = android.hardware.bluetooth@1.1-service-qti
else
BT_HAL_VER = android.hardware.bluetooth@1.0-service-qti
endif
endif

ifeq ($(BOARD_HAVE_QTI_BT_AIDL_SERVICE), true)
BT_HAL_VER = android.hardware.bluetooth@aidl-service-qti
endif

ifeq ($(BOARD_HAVE_QTI_BT_SERVICE_VER_1_1), true)
BT_HAL = android.hardware.bluetooth@1.1-impl-qti
endif

ifeq ($(BOARD_HAVE_QTI_BT_AIDL_SERVICE), true)
BT_HAL = android.hardware.bluetooth@aidl-impl-qti
BT_PACKAGES += android.hardware.bluetooth-V1-ndk
endif

ifeq ($(BOARD_HAS_GOOGLE_OFFLOAD_ENABLED), true)
$(warning "BOARD_HAS_GOOGLE_OFFLOAD_ENABLED = $(BOARD_HAS_GOOGLE_OFFLOAD_ENABLED)")
PRODUCT_PACKAGES += google_offload_proto
PRODUCT_PACKAGES += android.hardware.bluetooth.socket-V1-ndk
PRODUCT_PACKAGES += vendor.qti.hardware.socket-impl-qti
PRODUCT_PACKAGES += bluetooth_socket.xml
endif
ifeq ($(BOARD_HAS_GATT_OFFLOAD_ENABLED), true)
$(warning "BOARD_HAS_GATT_OFFLOAD_ENABLED = $(BOARD_HAS_GATT_OFFLOAD_ENABLED)")

ifeq ($(TARGET_VENDOR_GATT_HAL), true)
PRODUCT_PACKAGES += gatt_offload_proto
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth.gatt-V1-ndk
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth.gatt-impl-qti
PRODUCT_PACKAGES += bluetooth_vendor_qti_gatt_offload.xml
else
PRODUCT_PACKAGES += gatt_offload_proto
PRODUCT_PACKAGES += android.hardware.bluetooth.gatt-V1-ndk
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth.gatt-impl-qti
PRODUCT_PACKAGES += bluetooth_gatt_offload.xml
endif

endif

ifeq ($(BOARD_HAS_QTI_BT_FMD), true)
BTFMD_PACKAGES += android.hardware.bluetooth.finder-V1-ndk
BTFMD_PACKAGES += vendor.qti.hardware.bluetooth.finder-impl-qti
PRODUCT_PACKAGES += bluetooth-finder.xml
endif

ifeq ($(BOARD_HAVE_QTI_BCS), true)
BTFMD_PACKAGES += android.hardware.bluetooth.ranging-V1-ndk
BTFMD_PACKAGES += android.hardware.bluetooth.ranging@aidl-impl-qti
PRODUCT_PACKAGES += bcs-ranging.xml
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.bluetooth_le.channel_sounding.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.channel_sounding.xml
endif

BT_PACKAGES += android.hardware.bluetooth@1.0-impl-qti
BT_PACKAGES += vendor.qti.hardware.bluetooth_audio@2.0-impl
BT_PACKAGES += vendor.qti.hardware.bluetooth_audio@2.1-impl
BT_PACKAGES += android.hardware.bluetooth.audio-impl-qti
BT_PACKAGES += android.hardware.bluetooth.audio_sw
#PRODUCT_PACKAGES += android.hardware.bluetooth.a2dp@2.0-impl
#PRODUCT_PACKAGES += com.qualcomm.qti.bluetooth_audio@1.0.vendor
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth_audio@2.0.vendor
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth_audio@2.1.vendor
BT_PACKAGES += btaudio_offload_if
BT_PACKAGES += libbthost_if_sink
BT_PACKAGES += audio.bluetooth.default
BT_PACKAGES += audio.bluetooth_qti.default
BT_PACKAGES += libbluetooth_audio_session_qti
BT_PACKAGES += libbluetooth_audio_session_qti_2_1
BT_PACKAGES += libbluetooth_audio_session
BT_PACKAGES += libbt-hidlclient

ifeq ($(TARGET_USE_QTI_BT_CONFIGSTORE), true)
BT_PACKAGES += vendor.qti.hardware.btconfigstore@1.0.vendor
BT_PACKAGES += vendor.qti.hardware.btconfigstore@1.0-impl
BT_PACKAGES += vendor.qti.hardware.btconfigstore@2.0.vendor
BT_PACKAGES += vendor.qti.hardware.btconfigstore@2.0-impl
endif

BT_PACKAGES += libbtnv
BT_PACKAGES_DEBUG += btconfig
BT_PACKAGES_DEBUG += socketClient
ifeq ($(TARGET_USES_QTI_BTFTM), true)
BT_PACKAGES_DEBUG += btftmdaemon
endif
BT_PACKAGES += $(BT_HAL)
BT_PACKAGES += $(BT_HAL_VER)
#BTSAR
ifeq ($(TARGET_USE_QTI_BT_SAR), true)
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth_sar@1.0.vendor
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth_sar@1.0-impl
SAR_PACKAGES += vendor.qti.hardware.bluetooth_sar@1.1.vendor
SAR_PACKAGES += vendor.qti.hardware.bluetooth_sar@1.1-impl
endif

#BTSAR
ifeq ($(TARGET_USE_QTI_BT_SAR_AIDL), true)
SAR_PACKAGES += vendor.qti.hardware.bluetooth_sar-V1-ndk
SAR_PACKAGES += vendor.qti.hardware.bluetooth_sar@aidl-impl
PRODUCT_PACKAGES += bluetooth_sar.xml
endif

#BTTPI
BTTPI_PACKAGES += vendor.qti.hardware.bttpi-V1-ndk
BTTPI_PACKAGES += vendor.qti.hardware.bttpi-impl
PRODUCT_PACKAGES += bttpi-saidl

#BTLMP_EVENT
ifeq ($(TARGET_USE_QTI_BT_LMP_EVENT), true)
BTLMP_EVENT_PACKAGES += android.hardware.bluetooth.lmp_event-V1-ndk
BTLMP_EVENT_PACKAGES += android.hardware.bluetooth.lmp_event-impl-qti
PRODUCT_PACKAGES += btlmp_event-saidl
endif

#BTDUN
ifeq ($(TARGET_USE_BT_DUN), true)
ifeq ($(TARGET_HAS_DUN_HIDL_FEATURE), true)
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth_dun@1.0.vendor
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth_dun@1.0-impl
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth_dun@1.0-service
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth_dun@1.0-service.rc
PRODUCT_PACKAGES += vendor.qti.hardware.bluetooth_dun@1.0-service.disable.rc
endif #TARGET_HAS_DUN_HIDL_FEATURE
endif #TARGET_USE_BT_DUN

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

ifeq ($(TARGET_HAS_QSH_BLE_FEATURE), true)
#QSH BLE
QSH_BLE_DIR := vendor/qcom/proprietary/bluetooth/qsh_ble
qsh_ble_config_files := $(wildcard $(QSH_BLE_DIR)/config/common/*.json)
qsh_ble_config_files += $(wildcard $(QSH_BLE_DIR)/config/$(TARGET_BOARD_PLATFORM)/*.json)
qsh_ble_copied_config_files := \
    $(call copy-files,$(qsh_ble_config_files),$(TARGET_COPY_OUT_VENDOR)/etc/sensors/config)
PRODUCT_COPY_FILES += $(qsh_ble_copied_config_files)
#$(warning qsh_ble_copied_config_files=$(qsh_ble_copied_config_files))
# temporarily disable libqsh_ble_pb
#PRODUCT_PACKAGES += libqsh_ble_pb
#QSH_BLE = libqsh_ble_pb
endif

#FM
ifeq ($(BOARD_HAVE_QCOM_FM), true)
ifeq ($(BOARD_HAVE_QTI_FM_AIDL_SERVICE), true)
FM_PACKAGES += vendor.qti.hardware.fm-impl
FM_PACKAGES += vendor.qti.hardware.fm-V1-ndk
endif
FM_PACKAGES += fmconfig
FM_PACKAGES += fmfactorytest
FM_PACKAGES += fmfactorytestserver
FM_PACKAGES += fm_qsoc_patches
FM_PACKAGES += ftm_fm_lib
FM_PACKAGES += vendor.qti.hardware.fm@1.0.vendor
FM_PACKAGES_DEBUG += hal_ss_test_manual
FM_PACKAGES += init.qti.fm.sh
endif

BTFM_PACKAGES = $(BT_PACKAGES)
BTFM_PACKAGES += $(SAR_PACKAGES)
BTFM_PACKAGES += $(BTTPI_PACKAGES)
BTFM_PACKAGES += $(BTFMD_PACKAGES)
BTFM_PACKAGES += $(BTLMP_EVENT_PACKAGES)
#BTFM_PACKAGES += $(QSH_BLE)
BTFM_PACKAGES += $(FM_PACKAGES)
BTFM_PACKAGES_DEBUG = $(BT_PACKAGES_DEBUG)
BTFM_PACKAGES_DEBUG += $(FM_PACKAGES_DEBUG)

PRODUCT_PACKAGES += $(BTFM_PACKAGES)
PRODUCT_PACKAGES_DEBUG += $(BTFM_PACKAGES_DEBUG)

#WIPOWER
ifeq ($(BOARD_USES_WIPOWER), true)
PRODUCT_PACKAGES += vendor.qti.hardware.wipower@1.0_vendor
PRODUCT_PACKAGES += vendor.qti.hardware.wipower@1.0-impl
endif #BOARD_USES_WIPOWER


#BT/FM/WIPOWER PROPERTIES

ifeq ($(TARGET_BOARD_PLATFORM), msmnile) # msmnile specific defines
ifeq ($(TARGET_BOARD_TYPE), auto)
PRODUCT_PROPERTY_OVERRIDES += vendor.qcom.bluetooth.soc=rome
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=rome
else
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptive
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=true
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
endif
endif

ifeq ($(TARGET_BOARD_PLATFORM), sdm845) # SDM845 specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=true
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
endif


ifeq ($(TARGET_BOARD_PLATFORM), sdmshrike)  # Poipu/8195 specific defines
ifeq ($(TARGET_BOARD_TYPE), auto)
PRODUCT_PROPERTY_OVERRIDES += vendor.qcom.bluetooth.soc=rome
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=rome
endif
endif

ifeq ($(TARGET_BOARD_PLATFORM), sm6150)  # Talos/sm6150 specific defines
ifeq ($(TARGET_BOARD_TYPE), auto)
PRODUCT_PROPERTY_OVERRIDES += vendor.qcom.bluetooth.soc=rome
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=rome
else
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=true
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
endif
endif

ifeq ($(TARGET_BOARD_PLATFORM), sdm710)  # sdm710/Warlock specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=true
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), kona)  # kona/sm8250 specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=hastings
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptiver2
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#A2dp Multicast support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_mcast_test.enabled=false
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=false
#HearingAid support
PRODUCT_PROPERTY_OVERRIDES += persist.sys.fflag.override.settings_bluetooth_hearing_aid=true
endif

ifeq ($(TARGET_BOARD_PLATFORM), lahaina) # lahaina specific defines
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptiver2
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#A2dp Multicast support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_mcast_test.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=true
#HearingAid support
PRODUCT_PROPERTY_OVERRIDES += persist.sys.fflag.override.settings_bluetooth_hearing_aid=true
endif

ifeq ($(TARGET_BOARD_PLATFORM), taro) # waipio specific defines
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptiver2
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#A2dp Multicast support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_mcast_test.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=true
#HearingAid support
PRODUCT_PROPERTY_OVERRIDES += persist.sys.fflag.override.settings_bluetooth_hearing_aid=true
endif

ifeq ($(TARGET_BOARD_PLATFORM), kalama) # kailua specific defines
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptiver2
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#A2dp Multicast support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_mcast_test.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=true
#HearingAid support
PRODUCT_PROPERTY_OVERRIDES += persist.sys.fflag.override.settings_bluetooth_hearing_aid=true
#Lossless over Aptx Adaptive LE support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.lossless_aptx_adaptive_le.enabled=false
#Basic Audio Profile (BAP) broadcast assist role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.assist.enabled=true
#Basic Audio Profile (BAP) broadcast source role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.source.enabled=true
#Basic Audio Profile (BAP) unicast client role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.unicast.client.enabled=true
#Volume Control Profile (VCP) controller role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.vcp.controller.enabled=true
#Coordinated Set Identification Profile (CSIP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.csip.set_coordinator.enabled=true
#Media Control Profile (MCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.mcp.server.enabled=true
#Media Control Profile (CCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.ccp.server.enabled=true
#Hearing Aid Profile (HAP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.hap.client.enabled=true
endif

ifeq ($(TARGET_BOARD_PLATFORM), pineapple) # lanai specific defines
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptiver2
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#A2dp Multicast support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_mcast_test.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=true
#HearingAid support
PRODUCT_PROPERTY_OVERRIDES += persist.sys.fflag.override.settings_bluetooth_hearing_aid=true
#Lossless over Aptx Adaptive LE support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.lossless_aptx_adaptive_le.enabled=true
#Basic Audio Profile (BAP) broadcast assist role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.assist.enabled=true
#Basic Audio Profile (BAP) broadcast source role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.source.enabled=true
#Basic Audio Profile (BAP) unicast client role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.unicast.client.enabled=true
#Volume Control Profile (VCP) controller role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.vcp.controller.enabled=true
#Coordinated Set Identification Profile (CSIP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.csip.set_coordinator.enabled=true
#Media Control Profile (MCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.mcp.server.enabled=true
#Media Control Profile (CCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.ccp.server.enabled=true
#Hearing Aid Profile (HAP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.hap.client.enabled=true
#DUAL Mode Transport support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.dualmode_transport_support=true
endif

ifeq ($(TARGET_BOARD_PLATFORM), sun) # pakala specific defines
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=true
#HearingAid support
PRODUCT_PROPERTY_OVERRIDES += persist.sys.fflag.override.settings_bluetooth_hearing_aid=true
#Lossless over Aptx Adaptive LE support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.lossless_aptx_adaptive_le.enabled=true
#Basic Audio Profile (BAP) broadcast assist role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.assist.enabled=true
#Basic Audio Profile (BAP) broadcast source role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.source.enabled=true
#Basic Audio Profile (BAP) unicast client role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.unicast.client.enabled=true
#Volume Control Profile (VCP) controller role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.vcp.controller.enabled=true
#Coordinated Set Identification Profile (CSIP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.csip.set_coordinator.enabled=true
#Media Control Profile (MCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.mcp.server.enabled=true
#Media Control Profile (CCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.ccp.server.enabled=true
#Hearing Aid Profile (HAP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.hap.client.enabled=true
#DUAL Mode Transport support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.dualmode_transport_support=true
#Leaudio_offload
PRODUCT_PROPERTY_OVERRIDES += persist.bluetooth.leaudio_offload.disabled=false
#Leaudio allow multiple context
PRODUCT_PROPERTY_OVERRIDES +=  persist.bluetooth.leaudio.allow.multiple.context=false
#disable BT Config store
PRODUCT_PROPERTY_OVERRIDES +=  ro.vendor.bluetooth.btconfigstore=false
#FMD default Header Value
#length of Header(1B), Remaining Bytes are Data of Header
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.fmd_header=8:02:01:06:18:16:AA:FE:40
#AdvInterval(2B),KeyDataTimeout(4B),TxPwr(1B),AdvKeyDataSize(1B),RestartKeys(1B)
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.fmd_config_info=80:0C:60:90:0F:00:00:14:01
endif

ifeq ($(TARGET_BOARD_PLATFORM), parrot) # Netrani specific defines
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptiver2
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#A2dp Multicast support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_mcast_test.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=true
#HearingAid support
PRODUCT_PROPERTY_OVERRIDES += persist.sys.fflag.override.settings_bluetooth_hearing_aid=true
#QC CSIP enable
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.csip_qti=true
#disable BT Config store
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.btconfigstore=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), trinket) # trinket specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=true
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), lito) # lito specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptiver2
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=false
#HearingAid support
PRODUCT_PROPERTY_OVERRIDES += persist.sys.fflag.override.settings_bluetooth_hearing_aid=true
endif

ifeq ($(TARGET_BOARD_PLATFORM), bengal) # bengal specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), holi) # holi specific defines
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), blair) # blair specific defines
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), atoll) # atoll specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptive
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), sdm660)  # sdm660
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), msm8998) # MSM8998 specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=true
endif

ifeq ($(TARGET_BOARD_PLATFORM), msm8937) # msm8937 specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=pronto
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=false
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), msm8953) # MSM8953 specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=pronto
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=false
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), msm8909) # MSM8909 specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=pronto
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=false
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), msm8952) # MSM8952 specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=pronto
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=false
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
endif

ifeq ($(TARGET_BOARD_PLATFORM), crow) # camano specific defines
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac-aptxadaptiver2
#aac frame control support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_frm_ctl.enabled=true
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#A2dp Multicast support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_mcast_test.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=true
#HearingAid support
PRODUCT_PROPERTY_OVERRIDES += persist.sys.fflag.override.settings_bluetooth_hearing_aid=true
#Basic Audio Profile (BAP) broadcast assist role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.assist.enabled=true
#Basic Audio Profile (BAP) broadcast source role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.source.enabled=true
#Basic Audio Profile (BAP) unicast client role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.unicast.client.enabled=true
#Volume Control Profile (VCP) controller role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.vcp.controller.enabled=true
#Coordinated Set Identification Profile (CSIP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.csip.set_coordinator.enabled=true
#Media Control Profile (MCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.mcp.server.enabled=true
#Call Control Profile (CCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.ccp.server.enabled=true
endif

ifeq ($(BOARD_HAS_QTI_BT_XPAN), true) #XPAN is enabled
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qti.btadvaudio.target.support.xpan=true
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.btstack.qhs_enable=true
endif

ifeq ($(TARGET_BOARD_PLATFORM), pitti) # Kalpeni specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=cherokee
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=true
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=true
endif

ifeq ($(TARGET_BOARD_PLATFORM), vienna) # vienna specific defines
#Bluetooth SOC type
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.soc=themisto
#Glink Packet Logging
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.glink.logging=false
#split a2dp support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.enable.splita2dp=true
#a2dp offload capability
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_offload_cap=sbc-aptx-aptxtws-aptxhd-aac-ldac
#Embedded wipower mode
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.wipower=false
#TWS+ state support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.twsp_state.enabled=false
#Scrambling support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.scram.enabled=false
#AAC VBR support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aac_vbr_ctl.enabled=false
#AptX Adaptive R2.1 support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.aptxadaptiver2_1_support=true
#Leaudio_offload
PRODUCT_PROPERTY_OVERRIDES += persist.bluetooth.leaudio_offload.disabled=false
#Basic Audio Profile (BAP) broadcast assist role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.assist.enabled=true
#Basic Audio Profile (BAP) broadcast source role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.broadcast.source.enabled=true
#Basic Audio Profile (BAP) unicast client role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.bap.unicast.client.enabled=true
#Volume Control Profile (VCP) controller role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.vcp.controller.enabled=true
#Coordinated Set Identification Profile (CSIP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.csip.set_coordinator.enabled=true
#Media Control Profile (MCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.mcp.server.enabled=true
#Call Control Profile (CCP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.ccp.server.enabled=true
#Hearing Aid Profile (HAP) is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.hap.client.enabled=true
#avrcp Target role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.avrcp.target.enabled=true
#avrcp Controller role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.avrcp.controller.enabled=true
#Leaudio_offload
PRODUCT_PROPERTY_OVERRIDES += ro.bluetooth.leaudio_offload.supported=true
#disable BT Config store
PRODUCT_PROPERTY_OVERRIDES += ro.vendor.bluetooth.btconfigstore=false
#Display flag for LE Audio option
PRODUCT_PROPERTY_OVERRIDES += persist.display_le_audio_toggle=true
#HFP Client role is enabled
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.hfp.hf.enabled=true
#PAN NAP support
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.pan.nap.enabled=false
#PAN PANU
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.pan.panu.enabled=false

ifeq ($(TARGET_SUPPORTS_WEAR_ANDROID), true) #for law target only
#PBAP Client support
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.pbap.client.enabled=true
#MAP client support
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.map.client.enabled=true
#A2DP Sink Split support
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.qcom.bluetooth.a2dp_sink_offload.enabled=true
endif

#SAP support
PRODUCT_PROPERTY_OVERRIDES += bluetooth.profile.sap.server.enabled=false
#Disable LDAC Codec
PRODUCT_PROPERTY_OVERRIDES += bluetooth.a2dp.source.ldac_priority.config=-1
endif
