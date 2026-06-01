#ifndef CSR_BT_RESULT_H__
#define CSR_BT_RESULT_H__
/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef CsrUint16 CsrBtResultCode;
#define CSR_BT_RESULT_CODE_SUCCESS          ((CsrBtResultCode) 0x0000)

typedef CsrUint16 CsrBtReasonCode;
typedef CsrUint16 CsrBtSupplier;

/*************************************************************************************
               Denotes which supplier the CsrBtResultCode originated from.
               Corresponding result codes can be found in the header file mentioned
               for each result supplier.
************************************************************************************/
#define CSR_BT_SUPPLIER_HCI                  ((CsrBtSupplier) (0x0000)) /* hci.h */
#define CSR_BT_SUPPLIER_DM                   ((CsrBtSupplier) (0x0001)) /* dm_prim.h */
#define CSR_BT_SUPPLIER_L2CAP_CONNECT        ((CsrBtSupplier) (0x0002)) /* l2cap_prim.h */
#define CSR_BT_SUPPLIER_L2CAP_MOVE           ((CsrBtSupplier) (0x0003)) /* l2cap_prim.h */
#define CSR_BT_SUPPLIER_L2CAP_DISCONNECT     ((CsrBtSupplier) (0x0004)) /* l2cap_prim.h */
#define CSR_BT_SUPPLIER_L2CAP_DATA           ((CsrBtSupplier) (0x0005)) /* l2cap_prim.h */
#define CSR_BT_SUPPLIER_L2CAP_MISC           ((CsrBtSupplier) (0x0006)) /* l2cap_prim.h */
#define CSR_BT_SUPPLIER_SDP_SDS              ((CsrBtSupplier) (0x0007)) /* sds_prim.h */
#define CSR_BT_SUPPLIER_SDP_SDC              ((CsrBtSupplier) (0x0008)) /* sdc_prim.h */
#define CSR_BT_SUPPLIER_SDP_SDC_OPEN_SEARCH  ((CsrBtSupplier) (0x0009)) /* sdc_prim.h */
#define CSR_BT_SUPPLIER_RFCOMM               ((CsrBtSupplier) (0x000A)) /* rfcomm_prim.h */
#define CSR_BT_SUPPLIER_BCCMD                ((CsrBtSupplier) (0x000B)) /* NA */
#define CSR_BT_SUPPLIER_CM                   ((CsrBtSupplier) (0x000C)) /* csr_bt_cm_prim.h */
#define CSR_BT_SUPPLIER_IRDA_OBEX            ((CsrBtSupplier) (0x000D)) /* csr_bt_obex.h */
#define CSR_BT_SUPPLIER_SPP                  ((CsrBtSupplier) (0x000E)) /* csr_bt_spp_prim.h */
#define CSR_BT_SUPPLIER_SD                   ((CsrBtSupplier) (0x000F)) /* csr_bt_sd_prim.h */
#define CSR_BT_SUPPLIER_HF                   ((CsrBtSupplier) (0x0010)) /* csr_bt_hf_prim.h */
#define CSR_BT_SUPPLIER_AVRCP                ((CsrBtSupplier) (0x0011)) /* csr_bt_avrcp_prim.h */
#define CSR_BT_SUPPLIER_AV                   ((CsrBtSupplier) (0x0014)) /* csr_bt_av_prim.h */
#define CSR_BT_SUPPLIER_BSL                  ((CsrBtSupplier) (0x0015)) /* csr_bt_bsl_prim.h */
#define CSR_BT_SUPPLIER_DUNC                 ((CsrBtSupplier) (0x0016)) /* csr_bt_dunc_prim.h */
#define CSR_BT_SUPPLIER_HDP                  ((CsrBtSupplier) (0x0017)) /* csr_bt_hdp_prim.h */
#define CSR_BT_SUPPLIER_HFG                  ((CsrBtSupplier) (0x0018)) /* csr_bt_hfg_prim.h */
#define CSR_BT_SUPPLIER_HIDD                 ((CsrBtSupplier) (0x0019)) /* csr_bt_hidd_prim.h */
#define CSR_BT_SUPPLIER_HIDH                 ((CsrBtSupplier) (0x001A)) /* csr_bt_hidh_prim.h */
#define CSR_BT_SUPPLIER_SAPC                 ((CsrBtSupplier) (0x001B)) /* csr_bt_sap_common.h */
#define CSR_BT_SUPPLIER_SAPS                 ((CsrBtSupplier) (0x001C)) /* csr_bt_sap_common.h */
#define CSR_BT_SUPPLIER_DG                   ((CsrBtSupplier) (0x001D)) /* csr_bt_dg_prim.h */
#define CSR_BT_SUPPLIER_HCRP                 ((CsrBtSupplier) (0x001E)) /* csr_bt_hcrp_prim.h */
#define CSR_BT_SUPPLIER_BNEP                 ((CsrBtSupplier) (0x001F)) /* csr_bt_bnep_prim.h */
#define CSR_BT_SUPPLIER_BNEP_CONNECT_CFM     ((CsrBtSupplier) (0x0020)) /* csr_bt_bnep_prim.h */
#define CSR_BT_SUPPLIER_SC                   ((CsrBtSupplier) (0x0021)) /* csr_bt_sc_prim.h */
#define CSR_BT_SUPPLIER_MCAP                 ((CsrBtSupplier) (0x0022)) /* csr_bt_mcap_private_prim.h */
#define CSR_BT_SUPPLIER_MCAP_PROTOCOL        ((CsrBtSupplier) (0x0023)) /* csr_bt_mcap_private_prim.h */
#define CSR_BT_SUPPLIER_OBEX_PROFILES        ((CsrBtSupplier) (0x0024)) /* csr_bt_obex.h */
#define CSR_BT_SUPPLIER_JSR82                ((CsrBtSupplier) (0x0025)) /* NA */
#define CSR_BT_SUPPLIER_AT                   ((CsrBtSupplier) (0x0026)) /* csr_bt_at_prim.h */
#define CSR_BT_SUPPLIER_A2DP                 ((CsrBtSupplier) (0x0027)) /* csr_bt_av_prim.h */
#define CSR_BT_SUPPLIER_AVCTP                ((CsrBtSupplier) (0x0028)) /* csr_bt_avrcp_prim.h */
#define CSR_BT_SUPPLIER_AVC                  ((CsrBtSupplier) (0x0029)) /* csr_bt_avrcp_prim.h */
#define CSR_BT_SUPPLIER_AMPM                 ((CsrBtSupplier) (0x002A)) /* csr_bt_ampm_prim.h */
#define CSR_BT_SUPPLIER_PHDC                 ((CsrBtSupplier) (0x002B)) /* csr_bt_phdc_mgr_prim.h */
#define CSR_BT_SUPPLIER_PHDC_AG              ((CsrBtSupplier) (0x002C)) /* csr_bt_phdc_ag_prim.h */
#define CSR_BT_SUPPLIER_ATT                  ((CsrBtSupplier) (0x002D)) /* att_prim.h */
#define CSR_BT_SUPPLIER_GATT                 ((CsrBtSupplier) (0x002E)) /* csr_bt_gatt_prim.h */
#define CSR_BT_SUPPLIER_PROX_SRV             ((CsrBtSupplier) (0x002F)) /* csr_bt_prox_srv_prim.h */
#define CSR_BT_SUPPLIER_THERM_SRV            ((CsrBtSupplier) (0x0030)) /* csr_bt_therm_srv_prim.h */
#define CSR_BT_SUPPLIER_GNSS_CLIENT          ((CsrBtSupplier) (0x0031)) /* csr_bt_gnss_client_prim.h */
#define CSR_BT_SUPPLIER_GNSS_SERVER          ((CsrBtSupplier) (0x0032)) /* csr_bt_gnss_server_prim.h */
#define CSR_BT_SUPPLIER_AVRCP_IMAGING        ((CsrBtSupplier) (0x0033)) /* csr_bt_avrcp_imaging_prim.h */
#define CSR_BT_SUPPLIER_HOGH                 ((CsrBtSupplier) (0x0034)) /* csr_bt_hogh_prim.h */
#define CSR_BT_SUPPLIER_PXPM                 ((CsrBtSupplier) (0x0035)) /* csr_bt_pxpm_prim.h */
#define CSR_BT_SUPPLIER_LE_SRV               ((CsrBtSupplier) (0x0036)) /* csr_bt_le_srv_prim.h */
#define CSR_BT_SUPPLIER_LE_SVC               ((CsrBtSupplier) (0x0037)) /* csr_bt_le_svc_private_prim.h */
#define CSR_BT_SUPPLIER_LPM                  ((CsrBtSupplier) (0x0038)) /* csr_bt_lpm_prim.h */
#define CSR_BT_SUPPLIER_TPM                  ((CsrBtSupplier) (0x0039)) /* csr_bt_tpm_prim.h */
#define CSR_BT_SUPPLIER_TD_DB                ((CsrBtSupplier) (0x003A)) /* csr_bt_td_db.h */
#define CSR_BT_SUPPLIER_GATT_OFFLOAD         ((CsrBtSupplier) (0x003B)) /* csr_bt_gatt_prim.h */

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_RESULT_H__ */
