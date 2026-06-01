/******************************************************************************
 Copyright (c) 2012-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#ifndef _CSR_BT_FEATURE_DEFAULT_H
#define _CSR_BT_FEATURE_DEFAULT_H

#define INSTALL_BT_STANDALONE_MODE

/* Maximum number of BR/EDR ACL connections allowed. */
#define NUM_OF_ACL_CONNECTION                         (4) 

/*--------------------------------------------------------------------------
 * Version info
 *--------------------------------------------------------------------------*/
/* #undef CSR_BT_RELEASE_TYPE_TEST */
#define CSR_BT_VERSION_MAJOR    21
#define CSR_BT_VERSION_MINOR    0
#define CSR_BT_VERSION_FIXLEVEL 0
#define CSR_BT_VERSION_BUILD    0
/* #undef CSR_BT_RELEASE_TYPE_ENG */
#ifdef CSR_BT_RELEASE_TYPE_ENG
#define CSR_BT_RELEASE_VERSION  "21.0.0.0"
#else
#define CSR_BT_RELEASE_VERSION  "21.0.0"
#endif

/****************************************************************************
 Csr Bt Component Versions
 ****************************************************************************/
#define CSR_BT_BT_VERSION CSR_BT_BLUETOOTH_VERSION_5P3


/****************************************************************************
 application defines
 ****************************************************************************/
/* #undef CSR_BT_APP_AMP_UWB */
/* #undef CSR_BT_APP_AMP_WIFI */
#define CSR_BT_INSTALL_INTERNAL_APP_DEPENDENCIES

/****************************************************************************
conversion from global flags to bt flags
 ****************************************************************************/

#ifdef CSR_BUILD_DEBUG
    #define DM_ACL_DEBUG
    #define INSTALL_L2CAP_DEBUG
#endif

/****************************************************************************
 Random address Types
 ****************************************************************************/
#ifdef CSR_BT_LE_ENABLE
#define RPA    1
#define NRPA   2
#define STATIC_ADDR 3
#endif


/*Reading root directory path to resolve paths for psr files to open*/
#define CSR_BT_TOPDIR          

#define EXCLUDE_CSR_BT_CM_MODULE
#define EXCLUDE_CSR_BT_SDC_MODULE
#define EXCLUDE_CSR_BT_SDS_MODULE
#define EXCLUDE_CSR_BT_AV_MODULE
#define EXCLUDE_CSR_BT_AVRCP_MODULE
#define EXCLUDE_CSR_BT_AVRCP_IMAGING_MODULE
#define EXCLUDE_CSR_BT_AT_MODULE
#define EXCLUDE_CSR_BT_BPPS_MODULE
/* #undef EXCLUDE_CSR_BT_BPPS_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_BPPC_MODULE
/* #undef EXCLUDE_CSR_BT_BPPC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_BIPC_MODULE
#define EXCLUDE_CSR_BT_BIPS_MODULE
#define EXCLUDE_CSR_BT_BNEP_MODULE
#define EXCLUDE_CSR_BT_BSL_MODULE
#define EXCLUDE_CSR_BT_CME_BH_FEATURE
#define EXCLUDE_CSR_BT_DG_MODULE
/* #undef EXCLUDE_CSR_BT_DHCP_MODULE */
#define EXCLUDE_CSR_BT_DUNC_MODULE
#define EXCLUDE_CSR_BT_FTC_MODULE
/* #undef EXCLUDE_CSR_BT_FTC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_FTS_MODULE
/* #undef EXCLUDE_CSR_BT_FTS_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_GNSS_CLIENT_MODULE
/* #undef EXCLUDE_CSR_BT_GNSS_CLIENT_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_GNSS_SERVER_MODULE
/* #undef EXCLUDE_CSR_BT_GNSS_SERVER_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_HCRP_MODULE
#define EXCLUDE_CSR_BT_HDP_MODULE
#define EXCLUDE_CSR_BT_HF_MODULE
#define EXCLUDE_CSR_BT_HF_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_HFG_MODULE
#define EXCLUDE_CSR_BT_HFG_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_HID_PARSER_MODULE
 #define EXCLUDE_CSR_BT_HIDD_MODULE
#define EXCLUDE_CSR_BT_HIDH_MODULE
#define EXCLUDE_CSR_BT_HOGH_MODULE
/* #undef EXCLUDE_CSR_BT_ICMP_MODULE */
/* #undef EXCLUDE_CSR_BT_IP_MODULE */
#define EXCLUDE_CSR_BT_IWU_MODULE
#define EXCLUDE_CSR_BT_JSR82_MODULE
#define EXCLUDE_CSR_BT_MAPC_MODULE
#define EXCLUDE_CSR_BT_MAPC_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_MAPS_MODULE
#define EXCLUDE_CSR_BT_MCAP_MODULE
#define EXCLUDE_CSR_BT_OPC_MODULE
/* #undef EXCLUDE_CSR_BT_OPC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_OPS_MODULE
/* #undef EXCLUDE_CSR_BT_OPS_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_PAC_MODULE
#define EXCLUDE_CSR_BT_PAS_MODULE
#define EXCLUDE_CSR_BT_PHDC_AG_MODULE
#define EXCLUDE_CSR_BT_PHDC_MGR_MODULE
#define EXCLUDE_CSR_BT_SAPC_MODULE
#define EXCLUDE_CSR_BT_SAPS_MODULE
/* #undef EXCLUDE_CSR_BT_SCO_MODULE */
#define EXCLUDE_CSR_BT_SD_SERVICE_RECORD_MODULE
#define EXCLUDE_CSR_BT_SMLC_MODULE
/* #undef EXCLUDE_CSR_BT_SMLC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_SMLS_MODULE
/* #undef EXCLUDE_CSR_BT_SMLS_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_SPP_MODULE
/* #undef EXCLUDE_CSR_BT_SPP_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_SYNCC_MODULE
/* #undef EXCLUDE_CSR_BT_SYNCC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_SYNCS_MODULE
/* #undef EXCLUDE_CSR_BT_SYNCS_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_PXPM_MODULE
#define EXCLUDE_CSR_BT_PROX_SRV_MODULE
#define EXCLUDE_CSR_BT_THERM_SRV_MODULE
#define EXCLUDE_CSR_BT_TPM_MODULE
#define EXCLUDE_CSR_BT_SC_MODULE
#define EXCLUDE_CSR_BT_SC_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_SD_MODULE
#define EXCLUDE_CSR_BT_SD_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_CM_MODULE
#define EXCLUDE_CSR_BT_CM_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_OPTIONAL_UTILS
#define EXCLUDE_CSR_BT_TPT_MODULE
/* #undef EXCLUDE_CSR_BT_UDP_MODULE */
#define EXCLUDE_CSR_BT_MDER_MODULE
/* #define EXCLUDE_CSR_BT_GOEP_20_MODULE */
#define EXCLUDE_CSR_BT_SBC_MODULE
#define EXCLUDE_CSR_BT_PPP_MODULE
/* #undef EXCLUDE_CSR_BT_PAN_MODULE */
/* #undef EXCLUDE_CSR_BT_GATT_MODULE_OPTIONAL */
/* #undef EXCLUDE_CSR_BT_GATT_MODULE_OPTIONAL2 */
#define EXCLUDE_CSR_BT_GATT_GAP_MODULE
/* #undef CSR_BT_LE_SIGNING_ENABLE */
/* #undef EXCLUDE_CSR_BT_L2CA_MODULE */
/* #undef EXCLUDE_CSR_BT_RFC_MODULE */
#define EXCLUDE_CSR_BT_VCARD_MODULE
/* #undef INSTALL_L2CAP_RAW_SUPPORT */
/* #undef CSR_DSPM_ENABLE */


/* #undef CSR_BT_BLUE_STACK_DEBUG */
/* #undef CSR_BT_CONFIG_L2CAP_FCS */
/* #undef CSR_BT_INSTALL_L2CAP_CONNLESS_SUPPORT */

/* #undef CSR_BT_SC_ONLY_MODE_ENABLE */

#ifdef CSR_BT_LE_ENABLE
/*-------------------------------------------------------------------------------------------------------* 
 * CTKD shall not be enabled when local random address is configured as NRPA or STATIC(static address).  *
 * 1. In case of NRPA: Synergy does not allow bonding, so keys on other transport cannot be generated.   *
 * 2. In case of Static address: Synergy distributes the same static address as its Identity address     *
 *    while pairing, not the public address, so again correct keys can't be generated.                   *
 *-------------------------------------------------------------------------------------------------------*/
#if (RPA == RPA)
#define CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA
#define CSR_BT_INSTALL_CTKD_SUPPORT
#elif (RPA == NRPA)
#define CSR_BT_LE_RANDOM_ADDRESS_TYPE_NRPA
#elif (RPA == STATIC_ADDR)
#define CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC
#endif /* CSR_BT_LE_RANDOM_ADDRESS_TYPE */

#define CSR_BT_INSTALL_DLE_SUPPORT

#endif /* CSR_BT_LE_ENABLE */






#define CsrBtGattLto GATT
#define SynIpcLto IPC_TRANS

#define CsrBtGattLto GATT
#define MsAdvScanLto ADVSCAN
#define hciCmdArbLto HCICMDARB
#define CsrSerialComLto SRCOM
#define hciAttArbLto HCIATTARB


#define CSR_BT_INSTALL_EXTENDED_ADVERTISING
#define INSTALL_SOCKET_OFFLOAD
#include "csr_bt_config_microstack.h"



#endif /* _CSR_BT_FEATURE_DEFAULT_H */
