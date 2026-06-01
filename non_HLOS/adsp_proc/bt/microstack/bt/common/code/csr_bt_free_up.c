/******************************************************************************
 Copyright (c) 2019 - 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_bt_profiles.h"
#ifndef EXCLUDE_CSR_BT_ATT_MODULE
#include "attlib.h"
#endif
#ifndef EXCLUDE_CSR_BT_AMPM_MODULE
#include "csr_bt_ampm_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_ASM_MODULE
#include "csr_bt_asm_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_AT_MODULE
#include "csr_bt_at_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_AV_MODULE
#include "csr_bt_av_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_IMAGING_MODULE
#include "csr_bt_avrcp_imaging_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE
#include "csr_bt_avrcp_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_BIPC_MODULE
#include "csr_bt_bipc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_BIPS_MODULE
#include "csr_bt_bips_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_BPPC_MODULE
#include "csr_bt_bppc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_BPPS_MODULE
#include "csr_bt_bpps_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_BSL_MODULE
#include "csr_bt_bsl_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_CM_MODULE
//#include "csr_bt_cm_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_DG_MODULE
#include "csr_bt_dg_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_DUNC_MODULE
#include "csr_bt_dunc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_FTC_MODULE
#include "csr_bt_ftc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_FTS_MODULE
#include "csr_bt_fts_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_GATT_MODULE
#include "csr_bt_gatt_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_GNSS_CLIENT_MODULE
#include "csr_bt_gnss_client_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_GNSS_SERVER_MODULE
#include "csr_bt_gnss_server_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_HCRP_MODULE
#include "csr_bt_hcrp_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_HDP_MODULE
#include "csr_bt_hdp_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_HF_MODULE
#include "csr_bt_hf_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_HFG_MODULE
#include "csr_bt_hfg_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_HIDD_MODULE
#include "csr_bt_hidd_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_HIDH_MODULE
#include "csr_bt_hidh_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_HOGH_MODULE
#include "csr_bt_hogh_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_JSR82_MODULE
#include "csr_bt_jsr82_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_MAPC_MODULE
#include "csr_bt_mapc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_MAPS_MODULE
#include "csr_bt_maps_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_OPC_MODULE
#include "csr_bt_opc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_OPS_MODULE
#include "csr_bt_ops_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_PAC_MODULE
#include "csr_bt_pac_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_PAS_MODULE
#include "csr_bt_pas_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_PHDC_AG_MODULE
#include "csr_bt_phdc_ag_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_PHDC_MGR_MODULE
#include "csr_bt_phdc_mgr_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_PROX_SRV_MODULE
#include "csr_bt_prox_srv_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_PXPM_MODULE
#include "csr_bt_pxpm_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SAPC_MODULE
#include "csr_bt_sapc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SAPS_MODULE
#include "csr_bt_saps_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SD_MODULE
#include "csr_bt_sd_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SMLC_MODULE
#include "csr_bt_smlc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SMLS_MODULE
#include "csr_bt_smls_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SPP_MODULE
#include "csr_bt_spp_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SYNCC_MODULE
#include "csr_bt_syncc_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SYNCS_MODULE
#include "csr_bt_syncs_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_THERM_SRV_MODULE
#include "csr_bt_therm_srv_lib.h"
#endif
#ifndef EXCLUDE_CSR_BT_DM_MODULE
#include "dmlib.h"
#endif
#ifndef EXCLUDE_CSR_BT_L2CAP_MODULE
#include "l2caplib.h"
#endif
#ifndef EXCLUDE_CSR_BT_RFCOMM_MODULE
#include "rfcommlib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SDC_MODULE
#include "sdclib.h"
#endif
#ifndef EXCLUDE_CSR_BT_SDS_MODULE
#include "sdslib.h"
#endif
#ifndef EXCLUDE_BMM_MODULE
#include "bmm_lib.h"
#endif
#ifndef EXCLUDE_CSR_QVSC_MODULE
#include "csr_qvsc_lib.h"
#endif

void *CsrBtFreeUpstreamMessageContents(CsrUint16 eventClass, void *message)
{
    switch (eventClass)
    {
#ifndef EXCLUDE_CSR_BT_ATT_MODULE
        case CSR_BT_ATT_PRIM:
            attlib_free(message);
            return NULL;
#endif
#ifndef EXCLUDE_CSR_BT_AMPM_MODULE
            case CSR_BT_AMPM_PRIM:
            CsrBtAmpmFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_ASM_MODULE
            case CSR_BT_ASM_PRIM:
            CsrBtAsmFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_AT_MODULE
        case CSR_BT_AT_PRIM:
            CsrBtAtFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_AV_MODULE
        case CSR_BT_AV_PRIM:
            CsrBtAvFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_IMAGING_MODULE
        case CSR_BT_AVRCP_IMAGING_PRIM:
            /* CsrBtAvrcpImagingFreeUpstreamMessageContents(eventClass, message); */
            break;
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE
        case CSR_BT_AVRCP_PRIM:
            CsrBtAvrcpFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_BIPC_MODULE
        case CSR_BT_BIPC_PRIM:
            CsrBtBipcFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_BIPS_MODULE
        case CSR_BT_BIPS_PRIM:
            CsrBtBipsFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_BPPC_MODULE
        case CSR_BT_BPPC_PRIM:
            CsrBtBppcFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_BPPS_MODULE
        case CSR_BT_BPPS_PRIM:
            CsrBtBppsFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_BSL_MODULE
        case CSR_BT_BSL_PRIM:
            CsrBtBslFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_CM_MODULE
        case CSR_BT_CM_PRIM:
            CsrBtCmFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_DG_MODULE
        case CSR_BT_DG_PRIM:
            CsrBtDgFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_DUNC_MODULE
        case CSR_BT_DUNC_PRIM:
            CsrBtDuncFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_FTC_MODULE
        case CSR_BT_FTC_PRIM:
            CsrBtFtcFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_FTS_MODULE
        case CSR_BT_FTS_PRIM:
            CsrBtFtsFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_GATT_MODULE
        case CSR_BT_GATT_PRIM:
            CsrBtGattFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_GNSS_CLIENT_MODULE
        case CSR_BT_GNSS_CLIENT_PRIM:
            CsrBtGnssClientFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_GNSS_SERVER_MODULE
        case CSR_BT_GNSS_SERVER_PRIM:
            CsrBtGnssServerFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_HCRP_MODULE
        case CSR_BT_HCRP_PRIM:
            CsrBtHcrpFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_HDP_MODULE
        case CSR_BT_HDP_PRIM:
            CsrBtHdpFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_HF_MODULE
        case CSR_BT_HF_PRIM:
            CsrBtHfFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_HFG_MODULE
        case CSR_BT_HFG_PRIM:
            CsrBtHfgFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_HIDD_MODULE
        case CSR_BT_HIDD_PRIM:
            CsrBtHiddFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_HIDH_MODULE
        case CSR_BT_HIDH_PRIM:
            CsrBtHidhFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_HOGH_MODULE
        case CSR_BT_HOGH_PRIM:
            CsrBtHoghFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_JSR82_MODULE
        case CSR_BT_JSR82_PRIM:
            CsrBtJsr82FreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_MAPC_MODULE
        case CSR_BT_MAPC_PRIM:
            CsrBtMapcFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_MAPS_MODULE
        case CSR_BT_MAPS_PRIM:
            CsrBtMapsFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_OPC_MODULE
        case CSR_BT_OPC_PRIM:
            CsrBtOpcFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_OPS_MODULE
        case CSR_BT_OPS_PRIM:
            CsrBtOpsFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_PAC_MODULE
        case CSR_BT_PAC_PRIM:
            CsrBtPacFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_PAS_MODULE
        case CSR_BT_PAS_PRIM:
            CsrBtPasFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_PHDC_AG_MODULE
        case CSR_BT_PHDC_AG_PRIM:
            CsrBtPhdcAgFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_PHDC_MGR_MODULE
        case CSR_BT_PHDC_MGR_PRIM:
            CsrBtPhdcMgrFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_PROX_SRV_MODULE
        case CSR_BT_PROX_SRV_PRIM:
            CsrBtProxSrvFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_PXPM_MODULE
        case CSR_BT_PXPM_PRIM:
            CsrBtPxpmFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SAPC_MODULE
        case CSR_BT_SAPC_PRIM:
            CsrBtSapcFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SAPS_MODULE
        case CSR_BT_SAPS_PRIM:
            CsrBtSapsFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SC_MODULE
        case CSR_BT_SC_PRIM:
            CsrBtScFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SD_MODULE
        case CSR_BT_SD_PRIM:
            CsrBtSdFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SMLC_MODULE
        case CSR_BT_SMLC_PRIM:
            CsrBtSmlcFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SMLS_MODULE
        case CSR_BT_SMLS_PRIM:
            CsrBtSmlsFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SPP_MODULE
        case CSR_BT_SPP_PRIM:
            CsrBtSppFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SYNCC_MODULE
        case CSR_BT_SYNCC_PRIM:
            CsrBtSynccFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SYNCS_MODULE
        case CSR_BT_SYNCS_PRIM:
            CsrBtSyncsFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_THERM_SRV_MODULE
        case CSR_BT_THERM_SRV_PRIM:
            CsrBtThermSrvFreeUpstreamMessageContents(eventClass, message);
            break;
#endif
#ifndef EXCLUDE_CSR_BT_DM_MODULE
        case CSR_BT_DM_PRIM:
            //dm_free_upstream_primitive(message);
            return NULL;
#endif
#ifndef EXCLUDE_CSR_BT_L2CAP_MODULE
        case CSR_BT_L2CAP_PRIM:
            L2CA_FreePrimitive(message);
            return NULL;
#endif
#ifndef EXCLUDE_CSR_BT_RFCOMM_MODULE
        case CSR_BT_RFCOMM_PRIM:
            rfc_free_primitive(message);
            return NULL;
#endif
#ifndef EXCLUDE_CSR_BT_SDC_MODULE
        case CSR_BT_SDC_PRIM:
            /* sdc_free_upstream_primitive(eventClass, message); */
            break;
#endif
#ifndef EXCLUDE_CSR_BT_SDS_MODULE
        case CSR_BT_SDS_PRIM:
            /* sds_free_upstream_primitive(eventClass, message); */
            break;
#endif

#ifndef EXCLUDE_BMM_MODULE
        case BMM_PRIM:
            BmmFreePrimitive(message);
        break;
#endif

#ifndef EXCLUDE_CSR_QVSC_MODULE
        case CSR_QVSC_PRIM:
            CsrQvscFreeUpstreamMessageContents(CSR_QVSC_PRIM, message);
        break;
#endif
        default:
            break;
    }

    return message;
}
