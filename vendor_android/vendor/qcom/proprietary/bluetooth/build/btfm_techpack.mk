include vendor/qcom/proprietary/bluetooth/build/bt-vendor-proprietary-product.mk
ifeq ($(TARGET_USES_QTI_UWB), true)
include vendor/qcom/proprietary/uwb/build/uwb-vendor-proprietary-product.mk
endif
include vendor/qcom/opensource/bt-kernel/bt_kernel_vendor_board.mk
.PHONY: btfm_tp btfm_tp_package btfm_tp_dlkm
btfm_tp: btfm_tp_package btfm_tp_dlkm
btfm_tp_dlkm: $(BT_KERNEL_DRIVER)
ifeq ($(TARGET_USES_QTI_UWB), true)
btfm_tp_package: $(BTFM_PACKAGES) $(BTFM_PACKAGES_DEBUG) $(UWB_PACKAGES) $(UWB_PACKAGES_DEBUG)
else
btfm_tp_package: $(BTFM_PACKAGES) $(BTFM_PACKAGES_DEBUG)
endif
$(warning "BTFM Techpack configuration = $(btfm_tp_package)")
