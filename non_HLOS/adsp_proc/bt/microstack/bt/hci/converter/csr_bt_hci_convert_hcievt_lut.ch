
/****************************************************************************
 Copyright (c) 2016-2026 Qualcomm Technologies International, Ltd.

 FILE:           
 csr_bt_hci_convert_hcievt_lut.ch

 DESCRIPTION:   
 Top level converter for _hci commands and events

 REVISION:      : #5 


****************************************************************************/
  /* This file is AUTO-GENERATED. DO NOT modify! */
static const pdufmt * const hcievt_lut[] = {
    PDUFMT_NULL,
    PDUFMT_NULL,                /* INQUIRY_COMPLETE */
    PDUFMT_NULL,                /* INQUIRY_RESULT */
    LUT_PDUFMT(Su8sSu8su8u16Su24sSu8sSu16sSu8sSu8s),/* CONN_COMPLETE */
    PDUFMT_NULL,                /* CONN_REQUEST */
    LUT_PDUFMT(Su8sSu8su8u16u8),/* DISCONNECT_COMPLETE */
    PDUFMT_NULL,                /* AUTH_COMPLETE */
    PDUFMT_NULL,                /* REMOTE_NAME_REQ_COMPLETE */
    PDUFMT_NULL,                /* ENCRYPTION_CHANGE */
    PDUFMT_NULL,                /* CHANGE_CONN_LINK_KEY_COMPLETE */
    PDUFMT_NULL,                /* MASTER_LINK_KEY_COMPLETE */
    PDUFMT_NULL,                /* READ_REM_SUPP_FEATURES_COMPLETE */
    PDUFMT_NULL,                /* READ_REMOTE_VER_INFO_COMPLETE */
    PDUFMT_NULL,                /* QOS_SETUP_COMPLETE */
    PDUFMT_NULL,                /* COMMAND_COMPLETE */
    LUT_PDUFMT(Su8sSu8su8Su8su16),/* COMMAND_STATUS */
};
static const pdufmt * const hcievt_ulp_lut[] = {
    PDUFMT_NULL,
    LUT_PDUFMT(Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu16sSu16sSu16sSu8s),
                                /* CONNECTION_COMPLETE */
    PDUFMT_NULL,                /* ADVERTISING_REPORT */
    PDUFMT_NULL,                /* CONNECTION_UPDATE_COMPLETE */
    PDUFMT_NULL,                /* READ_REMOTE_USED_FEATURES_COMPLETE */
    PDUFMT_NULL,                /* LONG_TERM_KEY_REQUEST */
    PDUFMT_NULL,                /* REMOTE_CONNECTION_PARAMETER_REQUEST */
    PDUFMT_NULL,                /* DATA_LENGTH_CHANGE */
    PDUFMT_NULL,                /* READ_LOCAL_P256_PUB_KEY_COMPLETE */
    PDUFMT_NULL,                /* GENERATE_DHKEY_COMPLETE */
    
                                LUT_PDUFMT(Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu24sSu8sSu16sSu24sSu8sSu16sSu16sSu16sSu16sSu8s),
                                /* ENHANCED_CONNECTION_COMPLETE */
    PDUFMT_NULL,                /* DIRECT_ADVERTISING_REPORT */
    PDUFMT_NULL,                /* PHY_UPDATE_COMPLETE */
    PDUFMT_NULL,                /* EXT_ADV_REPORT */
    PDUFMT_NULL,                /* PERIODIC_ADV_SYNC_EST */
    PDUFMT_NULL,                /* PERIODIC_ADV_REPORT */
    PDUFMT_NULL,                /* PERIODIC_ADV_SYNC_LOST */
    PDUFMT_NULL,                /* SCAN_TIMEOUT */
    LUT_PDUFMT(Su8sSu8sSu8sSu8sSu8su16Su8s),/* ADV_SET_TERMINATED */
};
