#ifndef CSR_BT_CSR_BT_TASKS_H__
#define CSR_BT_CSR_BT_TASKS_H__
/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #13 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_sched.h"
#include "csr_bt_profiles.h"
#include "csr_bt_gatt_prim.h"

#ifdef CSR_LOG_ENABLE
#include "csr_bt_log_version.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CAA)
#define DEFAULT_PRIORITY 2
#endif


/* Corestack: DM */
#define ADV_SCAN_IFACEQUEUE_PRIM DM_PRIM
#if defined(CSR_BT_RUN_TASK_ADV_SCAN) && (CSR_BT_RUN_TASK_ADV_SCAN == 1)
extern void MsAdvScanInit(void **gash);
extern void MsAdvScanIfaceHandler(void **gash);
#define ADV_SCAN_INIT MsAdvScanInit
#define ADV_SCAN_TASK MsAdvScanIfaceHandler
#ifdef ENABLE_SHUTDOWN
extern void adv_scan_deinit(void **gash);
#define ADV_SCAN_DEINIT adv_scan_deinit
#else
#define ADV_SCAN_DEINIT NULL
#endif
#else
#define ADV_SCAN_INIT NULL
#define ADV_SCAN_DEINIT NULL
#define ADV_SCAN_TASK NULL
#endif

#if defined(CSR_BT_RUN_TASK_HCI_CMD_ARB) && (CSR_BT_RUN_TASK_HCI_CMD_ARB == 1)
extern void HciCmdArbInit(void **gash);
extern void HciCmdArbIfaceHandler(void **gash);
#define HCI_CMD_ARB_INIT HciCmdArbInit
#define HCI_CMD_ARB_TASK HciCmdArbIfaceHandler
#ifdef ENABLE_SHUTDOWN
extern void hci_cmd_arb_deinit(void **gash);
#define HCI_CMD_ARB_DEINIT hci_cmd_arb_deinit
#else
#define HCI_CMD_ARB_DEINIT NULL
#endif
#else
#define HCI_CMD_ARB_INIT NULL
#define HCI_CMD_ARB_DEINIT NULL
#define HCI_CMD_ARB_TASK NULL
#endif

#if defined(CSR_BT_RUN_TASK_HCI_ATT_ARB) && (CSR_BT_RUN_TASK_HCI_ATT_ARB == 1)
extern void HciAttArbInit(void **gash);
extern void HciAttArbIfaceHandler(void **gash);
#define HCI_ATT_ARB_INIT HciAttArbInit
#define HCI_ATT_ARB_TASK HciAttArbIfaceHandler
#ifdef ENABLE_SHUTDOWN
extern void hci_att_arb_deinit(void **gash);
#define HCI_ATT_ARB_DEINIT hci_att_arb_deinit
#else
#define HCI_ATT_ARB_DEINIT NULL
#endif
#else
#define HCI_ATT_ARB_INIT NULL
#define HCI_ATT_ARB_DEINIT NULL
#define HCI_ATT_ARB_TASK NULL
#endif


/* Corestack: DM-HCI */
#define DM_HCI_IFACEQUEUE_PRIM DM_PRIM
#if defined(CSR_BT_RUN_TASK_DM_HCI) && (CSR_BT_RUN_TASK_DM_HCI == 1)
extern void dm_hci_init (void **gash);
extern void CsrBtDmHciHandler(void **gash);
#define DM_HCI_INIT dm_hci_init
#define DM_HCI_TASK CsrBtDmHciHandler
#ifdef ENABLE_SHUTDOWN
extern void dm_hci_deinit(void **gash);
#define DM_HCI_DEINIT dm_hci_deinit
#else
#define DM_HCI_DEINIT NULL
#endif
#else
#define DM_HCI_DEINIT NULL
#define DM_HCI_TASK NULL
#endif

/* Corestack: SDP */
#define SDP_IFACEQUEUE_PRIM SDP_PRIM
#if defined(CSR_BT_RUN_TASK_SDP) && (CSR_BT_RUN_TASK_SDP == 1)
extern void init_sdp(void **gash);
extern void sdp_sda_handler(void **gash);
#define SDP_INIT init_sdp
#define SDP_TASK sdp_sda_handler
#ifdef ENABLE_SHUTDOWN
extern void deinit_sdp_sda(void **gash);
#define SDP_DEINIT deinit_sdp_sda
#else
#define SDP_DEINIT NULL
#endif
#else
#define SDP_INIT NULL
#define SDP_DEINIT NULL
#define SDP_TASK NULL
#endif

/* Corestack: L2CAP */
#define L2CAP_IFACEQUEUE_PRIM L2CAP_PRIM
#if defined(CSR_BT_RUN_TASK_L2CAP) && (CSR_BT_RUN_TASK_L2CAP == 1)
extern void L2CAP_Init(void **gash);
extern void L2CAP_InterfaceHandler(void **gash);
#define L2CAP_INIT L2CAP_Init
#define L2CAP_TASK L2CAP_InterfaceHandler
#ifdef ENABLE_SHUTDOWN
extern void L2CAP_Deinit(void **gash);
#define L2CAP_DEINIT L2CAP_Deinit
#else
#define L2CAP_DEINIT NULL
#endif
#else
#define L2CAP_INIT NULL
#define L2CAP_DEINIT NULL
#define L2CAP_TASK NULL
#endif

/* Corestack: RFCOMM */
#define RFCOMM_IFACEQUEUE_PRIM RFCOMM_PRIM
#if defined(CSR_BT_RUN_TASK_RFCOMM) && (CSR_BT_RUN_TASK_RFCOMM == 1) && !defined(EXCLUDE_CSR_BT_RFC_MODULE)
extern void rfc_init(void **gash);
extern void rfc_iface_handler(void **gash);
#define RFCOMM_INIT rfc_init
#define RFCOMM_TASK rfc_iface_handler
#ifdef ENABLE_SHUTDOWN
extern void rfc_deinit(void **gash);
#define RFCOMM_DEINIT rfc_deinit
#else
#define RFCOMM_DEINIT NULL
#endif
#else
#define RFCOMM_INIT NULL
#define RFCOMM_DEINIT NULL
#define RFCOMM_TASK NULL
#endif

/* Corestack: SDP-L2CAP */
#define SDP_L2CAP_IFACEQUEUE_PRIM SDP_PRIM
#define SDP_L2CAP_INIT NULL
#if defined(CSR_BT_RUN_TASK_SDP_L2CAP) && (CSR_BT_RUN_TASK_SDP_L2CAP == 1)
extern void sdp_l2cap_handler(void **gash);
#define SDP_L2CAP_TASK sdp_l2cap_handler
#ifdef ENABLE_SHUTDOWN
extern void sdp_l2cap_deinit(void **gash);
#define SDP_L2CAP_DEINIT sdp_l2cap_deinit
#else
#define SDP_L2CAP_DEINIT NULL
#endif
#else
#define SDP_L2CAP_TASK NULL
#define SDP_L2CAP_DEINIT NULL
#endif

/* Corestack: ATT */
#define ATT_IFACEQUEUE_PRIM ATT_PRIM
#if defined(CSR_BT_RUN_TASK_ATT) && (CSR_BT_RUN_TASK_ATT == 1) && defined(CSR_BT_LE_ENABLE)
extern void att_init(void **gash);
extern void att_handler(void **gash);
#define ATT_INIT att_init
#define ATT_TASK att_handler
#ifdef ENABLE_SHUTDOWN
extern void att_deinit(void **gash);
#define ATT_DEINIT att_deinit
#else
#define ATT_DEINIT NULL
#endif
#else
#define ATT_INIT NULL
#define ATT_DEINIT NULL
#define ATT_TASK NULL
#endif

/* Profiles: CM */
#define CSR_BT_CM_IFACEQUEUE_PRIM CSR_BT_CM_PRIM
#if !defined(EXCLUDE_CM_MODULE) && defined(CSR_BT_RUN_TASK_CM) && (CSR_BT_RUN_TASK_CM == 1)
extern void CsrBtCmInit(void **gash);
extern void CsrBtCmHandler(void **gash);
#define CSR_BT_CM_INIT CsrBtCmInit
#define CSR_BT_CM_HANDLER CsrBtCmHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtCmDeinit(void **gash);
#define CSR_BT_CM_DEINIT CsrBtCmDeinit
#else
#define CSR_BT_CM_DEINIT NULL
#endif
#else
#define CSR_BT_CM_INIT NULL
#define CSR_BT_CM_DEINIT NULL
#define CSR_BT_CM_HANDLER NULL
#endif

/* Profile: SC */
#define CSR_BT_SC_IFACEQUEUE_PRIM CSR_BT_SC_PRIM
#if !defined(EXCLUDE_CM_MODULE) && defined(CSR_BT_RUN_TASK_SC) && (CSR_BT_RUN_TASK_SC == 1)
extern void CsrBtScInit(void **gash);
extern void CsrBtScHandler(void **gash);
#define CSR_BT_SC_INIT CsrBtScInit
#define CSR_BT_SC_HANDLER CsrBtScHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtScDeinit(void **gash);
#define CSR_BT_SC_DEINIT CsrBtScDeinit
#else
#define CSR_BT_SC_DEINIT NULL
#endif
#else
#define CSR_BT_SC_INIT NULL
#define CSR_BT_SC_DEINIT NULL
#define CSR_BT_SC_HANDLER NULL
#endif

/* Synergy AG */
CsrUint16 CsrBtAgGetTaskId(void);
#define CSR_BT_AG_IFACEQUEUE_PRIM CSR_BT_USER_PRIM
extern void CsrBtAgInit(void **gash);
extern void CsrBtAgHandler(void **gash);
#define CSR_BT_AG_INIT CsrBtAgInit
#define CSR_BT_AG_HANDLER CsrBtAgHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtAGDeinit(void **gash);
#define CSR_BT_AG_DEINIT CsrBtAGDeinit
#else
#define CSR_BT_AG_DEINIT NULL
#endif

/* Profile: DG */
#define CSR_BT_DG_IFACEQUEUE_PRIM CSR_BT_DG_PRIM
#if !defined(EXCLUDE_CSR_BT_DG_MODULE) && defined(CSR_BT_RUN_TASK_DG) && (CSR_BT_RUN_TASK_DG == 1)
extern void CsrBtDgInit(void **gash);
extern void CsrBtDgHandler(void **gash);
#define CSR_BT_DG_INIT CsrBtDgInit
#define CSR_BT_DG_HANDLER CsrBtDgHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtDgDeinit(void **gash);
#define CSR_BT_DG_DEINIT CsrBtDgDeinit
#else
#define CSR_BT_DG_DEINIT NULL
#endif
#else
#define CSR_BT_DG_INIT NULL
#define CSR_BT_DG_DEINIT NULL
#define CSR_BT_DG_HANDLER NULL
#endif

/* Profile: AT */
#define CSR_BT_AT_IFACEQUEUE_PRIM CSR_BT_AT_PRIM
#if !defined(EXCLUDE_CSR_BT_AT_MODULE) && defined(CSR_BT_RUN_TASK_AT) && (CSR_BT_RUN_TASK_AT == 1)
extern void CsrBtAtInit(void **gash);
extern void CsrBtAtHandler(void **gash);
#define CSR_BT_AT_INIT CsrBtAtInit
#define CSR_BT_AT_HANDLER CsrBtAtHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtAtDeinit(void **gash);
#define CSR_BT_AT_DEINIT CsrBtAtDeinit
#else
#define CSR_BT_AT_DEINIT NULL
#endif
#else
#define CSR_BT_AT_INIT NULL
#define CSR_BT_AT_DEINIT NULL
#define CSR_BT_AT_HANDLER NULL
#endif

/* Profile: DUNC */
#define CSR_BT_DUNC_IFACEQUEUE_PRIM CSR_BT_DUNC_PRIM
#if !defined(EXCLUDE_CSR_BT_DUNC_MODULE) && defined(CSR_BT_RUN_TASK_DUNC) && (CSR_BT_RUN_TASK_DUNC == 1)
extern void CsrBtDuncInit(void **gash);
extern void CsrBtDuncHandler(void **gash);
#define CSR_BT_DUNC_INIT CsrBtDuncInit
#define CSR_BT_DUNC_HANDLER CsrBtDuncHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtDuncDeinit(void **gash);
#define CSR_BT_DUNC_DEINIT CsrBtDuncDeinit
#else
#define CSR_BT_DUNC_DEINIT NULL
#endif
#else
#define CSR_BT_DUNC_INIT NULL
#define CSR_BT_DUNC_DEINIT NULL
#define CSR_BT_DUNC_HANDLER NULL
#endif

/* Profile: OPS */
#define CSR_BT_OPS_IFACEQUEUE_PRIM CSR_BT_OPS_PRIM
#if !defined(EXCLUDE_CSR_BT_OPS_MODULE) && defined(CSR_BT_RUN_TASK_OPS) && (CSR_BT_RUN_TASK_OPS == 1)
extern void CsrBtOpsInit(void **gash);
extern void CsrBtOpsHandler(void **gash);
#define CSR_BT_OPS_INIT CsrBtOpsInit
#define CSR_BT_OPS_HANDLER CsrBtOpsHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtOpsDeinit(void **gash);
#define CSR_BT_OPS_DEINIT CsrBtOpsDeinit
#else
#define CSR_BT_OPS_DEINIT NULL
#endif
#else
#define CSR_BT_OPS_INIT NULL
#define CSR_BT_OPS_DEINIT NULL
#define CSR_BT_OPS_HANDLER NULL
#endif

/* Profile: OPC */
#define CSR_BT_OPC_IFACEQUEUE_PRIM CSR_BT_OPC_PRIM
#if !defined(EXCLUDE_CSR_BT_OPC_MODULE) && defined(CSR_BT_RUN_TASK_OPC) && (CSR_BT_RUN_TASK_OPC == 1)
extern void CsrBtOpcInit(void **gash);
extern void CsrBtOpcHandler(void **gash);
#define CSR_BT_OPC_INIT CsrBtOpcInit
#define CSR_BT_OPC_HANDLER CsrBtOpcHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtOpcDeinit(void **gash);
#define CSR_BT_OPC_DEINIT CsrBtOpcDeinit
#else
#define CSR_BT_OPC_DEINIT NULL
#endif
#else
#define CSR_BT_OPC_INIT NULL
#define CSR_BT_OPC_DEINIT NULL
#define CSR_BT_OPC_HANDLER NULL
#endif

/* Profile: MAPC */
#define CSR_BT_MAPC_EXTRA_IFACEQUEUE_PRIM CSR_BT_MAPC_PRIM
#define CSR_BT_MAPC_IFACEQUEUE_PRIM CSR_BT_MAPC_PRIM
#if !defined(EXCLUDE_CSR_BT_MAPC_MODULE) && defined(CSR_BT_RUN_TASK_MAPC) && (CSR_BT_RUN_TASK_MAPC == 1)
extern void CsrBtMapcInit(void **gash);
extern void CsrBtMapcHandler(void **gash);
#define CSR_BT_MAPC_INIT CsrBtMapcInit
#define CSR_BT_MAPC_HANDLER CsrBtMapcHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtMapcDeinit(void **gash);
#define CSR_BT_MAPC_DEINIT CsrBtMapcDeinit
#else
#define CSR_BT_MAPC_DEINIT NULL
#endif
#else
#define CSR_BT_MAPC_INIT NULL
#define CSR_BT_MAPC_DEINIT NULL
#define CSR_BT_MAPC_HANDLER NULL
#endif

/* Profile: MAPS */
#define CSR_BT_MAPS_EXTRA_IFACEQUEUE_PRIM CSR_BT_MAPS_PRIM
#define CSR_BT_MAPS_IFACEQUEUE_PRIM CSR_BT_MAPS_PRIM
#if !defined(EXCLUDE_CSR_BT_MAPS_MODULE) && defined(CSR_BT_RUN_TASK_MAPS) && (CSR_BT_RUN_TASK_MAPS == 1)
extern void CsrBtMapsInit(void **gash);
extern void CsrBtMapsHandler(void **gash);
#define CSR_BT_MAPS_INIT CsrBtMapsInit
#define CSR_BT_MAPS_HANDLER CsrBtMapsHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtMapsDeinit(void **gash);
#define CSR_BT_MAPS_DEINIT CsrBtMapsDeinit
#else
#define CSR_BT_MAPS_DEINIT NULL
#endif
#else
#define CSR_BT_MAPS_INIT NULL
#define CSR_BT_MAPS_DEINIT NULL
#define CSR_BT_MAPS_HANDLER NULL
#endif

/* Profile: SYNCC */
#define CSR_BT_SYNCC_IFACEQUEUE_PRIM CSR_BT_SYNCC_PRIM
#if !defined(EXCLUDE_CSR_BT_SYNCC_MODULE) && defined(CSR_BT_RUN_TASK_SYNCC) && (CSR_BT_RUN_TASK_SYNCC == 1)
extern void CsrBtSynccInit(void **gash);
extern void CsrBtSynccHandler(void **gash);
#define CSR_BT_SYNCC_INIT CsrBtSynccInit
#define CSR_BT_SYNCC_HANDLER CsrBtSynccHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtSynccDeinit(void **gash);
#define CSR_BT_SYNCC_DEINIT CsrBtSynccDeinit
#else
#define CSR_BT_SYNCC_DEINIT NULL
#endif
#else
#define CSR_BT_SYNCC_INIT NULL
#define CSR_BT_SYNCC_DEINIT NULL
#define CSR_BT_SYNCC_HANDLER NULL
#endif

/* Profile: SYNCS */
#define CSR_BT_SYNCS_IFACEQUEUE_PRIM CSR_BT_SYNCS_PRIM
#if !defined(EXCLUDE_CSR_BT_SYNCS_MODULE) && defined(CSR_BT_RUN_TASK_SYNCS) && (CSR_BT_RUN_TASK_SYNCS == 1)
extern void CsrBtSyncsInit(void **gash);
extern void CsrBtSyncsHandler(void **gash);
#define CSR_BT_SYNCS_INIT CsrBtSyncsInit
#define CSR_BT_SYNCS_HANDLER CsrBtSyncsHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtSyncsDeinit(void **gash);
#define CSR_BT_SYNCS_DEINIT CsrBtSyncsDeinit
#else
#define CSR_BT_SYNCS_DEINIT NULL
#endif
#else
#define CSR_BT_SYNCS_INIT NULL
#define CSR_BT_SYNCS_DEINIT NULL
#define CSR_BT_SYNCS_HANDLER NULL
#endif

/* Profile: SMLC */
#define CSR_BT_SMLC_IFACEQUEUE_PRIM CSR_BT_SMLC_PRIM
#if !defined(EXCLUDE_CSR_BT_SMLC_MODULE) && defined(CSR_BT_RUN_TASK_SMLC) && (CSR_BT_RUN_TASK_SMLC == 1)
extern void CsrBtSmlcInit(void **gash);
extern void CsrBtSmlcHandler(void **gash);
#define CSR_BT_SMLC_INIT CsrBtSmlcInit
#define CSR_BT_SMLC_HANDLER CsrBtSmlcHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtSmlcDeinit(void **gash);
#define CSR_BT_SMLC_DEINIT CsrBtSmlcDeinit
#else
#define CSR_BT_SMLC_DEINIT NULL
#endif
#else
#define CSR_BT_SMLC_INIT NULL
#define CSR_BT_SMLC_DEINIT NULL
#define CSR_BT_SMLC_HANDLER NULL
#endif

/* Profile: SMLS */
#define CSR_BT_SMLS_IFACEQUEUE_PRIM CSR_BT_SMLS_PRIM
#if !defined(EXCLUDE_CSR_BT_SMLS_MODULE) && defined(CSR_BT_RUN_TASK_SMLS) && (CSR_BT_RUN_TASK_SMLS == 1)
extern void CsrBtSmlsInit(void **gash);
extern void CsrBtSmlsHandler(void **gash);
#define CSR_BT_SMLS_INIT CsrBtSmlsInit
#define CSR_BT_SMLS_HANDLER CsrBtSmlsHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtSmlsDeinit(void **gash);
#define CSR_BT_SMLS_DEINIT CsrBtSmlsDeinit
#else
#define CSR_BT_SMLS_DEINIT NULL
#endif
#else
#define CSR_BT_SMLS_INIT NULL
#define CSR_BT_SMLS_DEINIT NULL
#define CSR_BT_SMLS_HANDLER NULL
#endif

/* Profile: IWU */
#define CSR_BT_IWU_IFACEQUEUE_PRIM CSR_BT_IWU_PRIM
#if !defined(EXCLUDE_CSR_BT_IWU_MODULE) && defined(CSR_BT_RUN_TASK_IWU) && (CSR_BT_RUN_TASK_IWU == 1)
extern void CsrBtIwuInit(void **gash);
extern void CsrBtIwuHandler(void **gash);
#define CSR_BT_IWU_INIT CsrBtIwuInit
#define CSR_BT_IWU_HANDLER CsrBtIwuHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtIwuDeinit(void **gash);
#define CSR_BT_IWU_DEINIT CsrBtIwuDeinit
#else
#define CSR_BT_IWU_DEINIT NULL
#endif
#else
#define CSR_BT_IWU_INIT NULL
#define CSR_BT_IWU_DEINIT NULL
#define CSR_BT_IWU_HANDLER NULL
#endif

/* Profile: SPP */
#define CSR_BT_SPP_EXTRA_IFACEQUEUE_PRIM CSR_BT_SPP_PRIM
#define CSR_BT_SPP_IFACEQUEUE_PRIM CSR_BT_SPP_PRIM
#if !defined(EXCLUDE_CSR_BT_SPP_MODULE) && defined(CSR_BT_RUN_TASK_SPP) && (CSR_BT_RUN_TASK_SPP == 1)
extern void CsrBtSppInit(void **gash);
extern void CsrBtSppHandler(void **gash);
#define CSR_BT_SPP_INIT CsrBtSppInit
#define CSR_BT_SPP_HANDLER CsrBtSppHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtSppDeinit(void **gash);
#define CSR_BT_SPP_DEINIT CsrBtSppDeinit
#else
#define CSR_BT_SPP_DEINIT NULL
#endif
#else
#define CSR_BT_SPP_INIT NULL
#define CSR_BT_SPP_DEINIT NULL
#define CSR_BT_SPP_HANDLER NULL
#endif

/* Profile: HFG */
#define CSR_BT_HFG_IFACEQUEUE_PRIM CSR_BT_HFG_PRIM
#if !defined(EXCLUDE_CSR_BT_HFG_MODULE) && defined(CSR_BT_RUN_TASK_HFG) && (CSR_BT_RUN_TASK_HFG == 1) && !defined(EXCLUDE_CSR_BT_SCO_MODULE)
extern void CsrBtHfgInit(void **gash);
extern void CsrBtHfgHandler(void **gash);
#define CSR_BT_HFG_INIT CsrBtHfgInit
#define CSR_BT_HFG_HANDLER CsrBtHfgHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtHfgDeinit(void **gash);
#define CSR_BT_HFG_DEINIT CsrBtHfgDeinit
#else
#define CSR_BT_HFG_DEINIT NULL
#endif
#else
#define CSR_BT_HFG_INIT NULL
#define CSR_BT_HFG_DEINIT NULL
#define CSR_BT_HFG_HANDLER NULL
#endif

/* Profile: HF */
#define CSR_BT_HF_IFACEQUEUE_PRIM CSR_BT_HF_PRIM
#if !defined(EXCLUDE_CSR_BT_HF_MODULE) && defined(CSR_BT_RUN_TASK_HF) && (CSR_BT_RUN_TASK_HF == 1) && !defined(EXCLUDE_CSR_BT_SCO_MODULE)
extern void CsrBtHfInit(void **gash);
extern void CsrBtHfHandler(void **gash);
#define CSR_BT_HF_INIT CsrBtHfInit
#define CSR_BT_HF_HANDLER CsrBtHfHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtHfDeinit(void **gash);
#define CSR_BT_HF_DEINIT CsrBtHfDeinit
#else
#define CSR_BT_HF_DEINIT NULL
#endif
#else
#define CSR_BT_HF_INIT NULL
#define CSR_BT_HF_DEINIT NULL
#define CSR_BT_HF_HANDLER NULL
#endif

/* Profile: PAC */
#define CSR_BT_PAC_IFACEQUEUE_PRIM CSR_BT_PAC_PRIM
#if !defined(EXCLUDE_CSR_BT_PAC_MODULE) && defined(CSR_BT_RUN_TASK_PAC) && (CSR_BT_RUN_TASK_PAC == 1)
extern void CsrBtPacInit(void **gash);
extern void CsrBtPacHandler(void **gash);
#define CSR_BT_PAC_INIT CsrBtPacInit
#define CSR_BT_PAC_HANDLER CsrBtPacHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtPacDeinit(void **gash);
#define CSR_BT_PAC_DEINIT CsrBtPacDeinit
#else
#define CSR_BT_PAC_DEINIT NULL
#endif
#else
#define CSR_BT_PAC_INIT NULL
#define CSR_BT_PAC_DEINIT NULL
#define CSR_BT_PAC_HANDLER NULL
#endif

/* Profile: PAS */
#define CSR_BT_PAS_IFACEQUEUE_PRIM CSR_BT_PAS_PRIM
#if !defined(EXCLUDE_CSR_BT_PAS_MODULE) && defined(CSR_BT_RUN_TASK_PAS) && (CSR_BT_RUN_TASK_PAS == 1)
extern void CsrBtPasInit(void **gash);
extern void CsrBtPasHandler(void **gash);
#define CSR_BT_PAS_INIT CsrBtPasInit
#define CSR_BT_PAS_HANDLER CsrBtPasHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtPasDeinit(void **gash);
#define CSR_BT_PAS_DEINIT CsrBtPasDeinit
#else
#define CSR_BT_PAS_DEINIT NULL
#endif
#else
#define CSR_BT_PAS_INIT NULL
#define CSR_BT_PAS_DEINIT NULL
#define CSR_BT_PAS_HANDLER NULL
#endif

/* Profile: FTS */
#define CSR_BT_FTS_IFACEQUEUE_PRIM CSR_BT_FTS_PRIM
#if !defined(EXCLUDE_CSR_BT_FTS_MODULE) && defined(CSR_BT_RUN_TASK_FTS) && (CSR_BT_RUN_TASK_FTS == 1)
extern void CsrBtFtsInit(void **gash);
extern void CsrBtFtsHandler(void **gash);
#define CSR_BT_FTS_INIT CsrBtFtsInit
#define CSR_BT_FTS_HANDLER CsrBtFtsHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtFtsDeinit(void **gash);
#define CSR_BT_FTS_DEINIT CsrBtFtsDeinit
#else
#define CSR_BT_FTS_DEINIT NULL
#endif
#else
#define CSR_BT_FTS_INIT NULL
#define CSR_BT_FTS_DEINIT NULL
#define CSR_BT_FTS_HANDLER NULL
#endif

/* Profile: BNEP */
#define CSR_BT_BNEP_IFACEQUEUE_PRIM CSR_BT_BNEP_PRIM
#if !defined(EXCLUDE_CSR_BT_BNEP_MODULE) && defined(CSR_BT_RUN_TASK_BNEP) && (CSR_BT_RUN_TASK_BNEP == 1)
extern void CsrBtBnepInit(void **gash);
extern void CsrBtBnepHandler(void **gash);
#define CSR_BT_BNEP_INIT CsrBtBnepInit
#define CSR_BT_BNEP_HANDLER CsrBtBnepHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtBnepDeinit(void **gash);
#define CSR_BT_BNEP_DEINIT CsrBtBnepDeinit
#else
#define CSR_BT_BNEP_DEINIT NULL
#endif
#else
#define CSR_BT_BNEP_INIT NULL
#define CSR_BT_BNEP_DEINIT NULL
#define CSR_BT_BNEP_HANDLER NULL
#endif

/* Profile: BSL */
#define CSR_BT_BSL_IFACEQUEUE_PRIM CSR_BT_BSL_PRIM
#if !defined(EXCLUDE_CSR_BT_BSL_MODULE) && defined(CSR_BT_RUN_TASK_BSL) && (CSR_BT_RUN_TASK_BSL == 1)
extern void CsrBtBslInit(void **gash);
extern void CsrBtBslHandler(void **gash);
#define CSR_BT_BSL_INIT CsrBtBslInit
#define CSR_BT_BSL_HANDLER CsrBtBslHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtBslDeinit(void **gash);
#define CSR_BT_BSL_DEINIT CsrBtBslDeinit
#else
#define CSR_BT_BSL_DEINIT NULL
#endif
#else
#define CSR_BT_BSL_INIT NULL
#define CSR_BT_BSL_DEINIT NULL
#define CSR_BT_BSL_HANDLER NULL
#endif

/* Profile: IP */
#define CSR_BT_IP_IFACEQUEUE_PRIM CSR_BT_IP_PRIM
#if defined(CSR_BT_RUN_TASK_IP) && (CSR_BT_RUN_TASK_IP == 1)
extern void CsrBtIpInit(void ** gash);
extern void CsrBtIpHandler(void ** gash);
#define CSR_BT_IP_INIT CsrBtIpInit
#define CSR_BT_IP_TASK CsrBtIpHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtIpDeinit(void ** gash);
#define CSR_BT_IP_DEINIT CsrBtIpDeinit
#else
#define CSR_BT_IP_DEINIT NULL
#endif
#else
#define CSR_BT_IP_INIT NULL
#define CSR_BT_IP_DEINIT NULL
#define CSR_BT_IP_TASK NULL
#endif

/* Profile: ICMP */
#define CSR_BT_ICMP_IFACEQUEUE_PRIM CSR_BT_ICMP_PRIM
#if defined(CSR_BT_RUN_TASK_ICMP) && (CSR_BT_RUN_TASK_ICMP == 1)
extern void CsrBtIcmpInit(void ** gash);
extern void CsrBtIcmpHandler(void ** gash);
#define CSR_BT_ICMP_INIT CsrBtIcmpInit
#define CSR_BT_ICMP_TASK CsrBtIcmpHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtIcmpDeinit(void ** gash);
#define CSR_BT_ICMP_DEINIT CsrBtIcmpDeinit
#else
#define CSR_BT_ICMP_DEINIT NULL
#endif
#else
#define CSR_BT_ICMP_INIT NULL
#define CSR_BT_ICMP_DEINIT NULL
#define CSR_BT_ICMP_TASK NULL
#endif

/* Profile: UDP */
#define CSR_BT_UDP_IFACEQUEUE_PRIM CSR_BT_UDP_PRIM
#if defined(CSR_BT_RUN_TASK_UDP) && (CSR_BT_RUN_TASK_UDP == 1)
extern void CsrBtUdpInit(void ** gash);
extern void CsrBtUdpHandler(void ** gash);
#define CSR_BT_UDP_INIT CsrBtUdpInit
#define CSR_BT_UDP_HANDLER CsrBtUdpHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtUdpDeinit(void ** gash);
#define CSR_BT_UDP_DEINIT CsrBtUdpDeinit
#else
#define CSR_BT_UDP_DEINIT NULL
#endif
#else
#define CSR_BT_UDP_INIT NULL
#define CSR_BT_UDP_DEINIT NULL
#define CSR_BT_UDP_HANDLER NULL
#endif

/* Profile: DHCP */
#define CSR_BT_DHCP_IFACEQUEUE_PRIM CSR_BT_DHCP_PRIM
#if defined(CSR_BT_RUN_TASK_DHCP) && (CSR_BT_RUN_TASK_DHCP == 1)
extern void CsrBtDhcpInit(void ** gash);
extern void CsrBtDhcpHandler(void ** gash);
#define CSR_BT_DHCP_INIT CsrBtDhcpInit
#define CSR_BT_DHCP_TASK CsrBtDhcpHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtDhcpDeinit(void ** gash);
#define CSR_BT_DHCP_DEINIT CsrBtDhcpDeinit
#else
#define CSR_BT_DHCP_DEINIT NULL
#endif
#else
#define CSR_BT_DHCP_INIT NULL
#define CSR_BT_DHCP_DEINIT NULL
#define CSR_BT_DHCP_TASK NULL
#endif

/* Profile: TFTP */
#define CSR_BT_TFTP_IFACEQUEUE_PRIM CSR_BT_TFTP_PRIM
#if defined(CSR_BT_RUN_TASK_TFTP) && (CSR_BT_RUN_TASK_TFTP == 1)
extern void CsrBtTftpInit(void ** gash);
extern void CsrBtTftpHandler(void ** gash);
#define CSR_BT_TFTP_INIT CsrBtTftpInit
#define CSR_BT_TFTP_HANDLER CsrBtTftpHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtTftpDeinit(void ** gash);
#define CSR_BT_TFTP_DEINIT CsrBtTftpDeinit
#else
#define CSR_BT_TFTP_DEINIT NULL
#endif
#else
#define CSR_BT_TFTP_INIT NULL
#define CSR_BT_TFTP_DEINIT NULL
#define CSR_BT_TFTP_HANDLER NULL
#endif

/* Helper for tester/tester: Ctrl */
#define CSR_BT_CTRL_IFACEQUEUE_PRIM CSR_BT_CTRL_PRIM
#if defined(CSR_BT_RUN_TASK_CTRL) && (CSR_BT_RUN_TASK_CTRL == 1)
extern void CsrBtCtrlInit(void ** gash);
extern void CsrBtCtrlHandler(void ** gash);
#define CSR_BT_CTRL_INIT CsrBtCtrlInit
#define CSR_BT_CTRL_TASK CsrBtCtrlHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtCtrlDeinit(void ** gash);
#define CSR_BT_CTRL_DEINIT CsrBtCtrlDeinit
#else
#define CSR_BT_CTRL_DEINIT NULL
#endif
#else
#define CSR_BT_CTRL_INIT NULL
#define CSR_BT_CTRL_DEINIT NULL
#define CSR_BT_CTRL_TASK NULL
#endif

/* Profile: BIPC */
#define CSR_BT_BIPC_IFACEQUEUE_PRIM CSR_BT_BIPC_PRIM
#if !defined(EXCLUDE_CSR_BT_BIPC_MODULE) && defined(CSR_BT_RUN_TASK_BIPC) && (CSR_BT_RUN_TASK_BIPC == 1)
extern void CsrBtBipcInit(void **gash);
extern void CsrBtBipcHandler(void **gash);
#define CSR_BT_BIPC_INIT CsrBtBipcInit
#define CSR_BT_BIPC_HANDLER CsrBtBipcHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtBipcDeinit(void **gash);
#define CSR_BT_BIPC_DEINIT CsrBtBipcDeinit
#else
#define CSR_BT_BIPC_DEINIT NULL
#endif
#else
#define CSR_BT_BIPC_INIT NULL
#define CSR_BT_BIPC_DEINIT NULL
#define CSR_BT_BIPC_HANDLER NULL
#endif

/* Profile: BIPS */
#define CSR_BT_BIPS_EXTRA_IFACEQUEUE_PRIM CSR_BT_BIPS_PRIM
#define CSR_BT_BIPS_IFACEQUEUE_PRIM CSR_BT_BIPS_PRIM
#if !defined(EXCLUDE_CSR_BT_BIPS_MODULE) && defined(CSR_BT_RUN_TASK_BIPS) && (CSR_BT_RUN_TASK_BIPS == 1)
extern void CsrBtBipsInit(void **gash);
extern void CsrBtBipsHandler(void **gash);
#define CSR_BT_BIPS_INIT CsrBtBipsInit
#define CSR_BT_BIPS_HANDLER CsrBtBipsHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtBipsDeinit(void **gash);
#define CSR_BT_BIPS_DEINIT CsrBtBipsDeinit
#else
#define CSR_BT_BIPS_DEINIT NULL
#endif
#else
#define CSR_BT_BIPS_INIT NULL
#define CSR_BT_BIPS_DEINIT NULL
#define CSR_BT_BIPS_HANDLER NULL
#endif

/* Profile: FTC */
#define CSR_BT_FTC_IFACEQUEUE_PRIM CSR_BT_FTC_PRIM
#if !defined(EXCLUDE_CSR_BT_FTC_MODULE) && defined(CSR_BT_RUN_TASK_FTC) && (CSR_BT_RUN_TASK_FTC == 1)
extern void CsrBtFtcInit(void **gash);
extern void CsrBtFtcHandler(void **gash);
#define CSR_BT_FTC_INIT CsrBtFtcInit
#define CSR_BT_FTC_HANDLER CsrBtFtcHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtFtcDeinit(void **gash);
#define CSR_BT_FTC_DEINIT CsrBtFtcDeinit
#else
#define CSR_BT_FTC_DEINIT NULL
#endif
#else
#define CSR_BT_FTC_INIT NULL
#define CSR_BT_FTC_DEINIT NULL
#define CSR_BT_FTC_HANDLER NULL
#endif

/* Profile: PPP */
#define CSR_BT_PPP_IFACEQUEUE_PRIM CSR_BT_PPP_PRIM
#if !defined(EXCLUDE_CSR_BT_PPP_MODULE) && defined(CSR_BT_RUN_TASK_PPP) && (CSR_BT_RUN_TASK_PPP == 1)
extern void CsrBtPppInit(void ** gash);
extern void CsrBtPppHandler(void ** gash);
#define CSR_BT_PPP_INIT CsrBtPppInit
#define CSR_BT_PPP_HANDLER CsrBtPppHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtPppDeinit(void ** gash);
#define CSR_BT_PPP_DEINIT CsrBtPppDeinit
#else
#define CSR_BT_PPP_DEINIT NULL
#endif
#else
#define CSR_BT_PPP_INIT NULL
#define CSR_BT_PPP_DEINIT NULL
#define CSR_BT_PPP_HANDLER NULL
#endif

/* Profile: BPPC */
#define CSR_BT_BPPC_IFACEQUEUE_PRIM CSR_BT_BPPC_PRIM
#if !defined(EXCLUDE_CSR_BT_BPPC_MODULE) && defined(CSR_BT_RUN_TASK_BPPC) && (CSR_BT_RUN_TASK_BPPC == 1)
extern void CsrBtBppcInit(void **gash);
extern void CsrBtBppcHandler(void **gash);
#define CSR_BT_BPPC_INIT CsrBtBppcInit
#define CSR_BT_BPPC_HANDLER CsrBtBppcHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtBppcDeinit(void **gash);
#define CSR_BT_BPPC_DEINIT CsrBtBppcDeinit
#else
#define CSR_BT_BPPC_DEINIT NULL
#endif
#else
#define CSR_BT_BPPC_INIT NULL
#define CSR_BT_BPPC_DEINIT NULL
#define CSR_BT_BPPC_HANDLER NULL
#endif

/* Profile: AV */
#define CSR_BT_AV_IFACEQUEUE_PRIM CSR_BT_AV_PRIM
#if !defined(EXCLUDE_CSR_BT_AV_MODULE) && defined(CSR_BT_RUN_TASK_AV) && (CSR_BT_RUN_TASK_AV == 1)
extern void CsrBtAvInit(void **gash);
extern void CsrBtAvHandler(void **gash);
#define CSR_BT_AV_INIT CsrBtAvInit
#define CSR_BT_AV_HANDLER CsrBtAvHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtAvDeinit(void **gash);
#define CSR_BT_AV_DEINIT CsrBtAvDeinit
#else
#define CSR_BT_AV_DEINIT NULL
#endif
#else
#define CSR_BT_AV_INIT NULL
#define CSR_BT_AV_DEINIT NULL
#define CSR_BT_AV_HANDLER NULL
#endif

/* Profile: AVRCP */
#define CSR_BT_AVRCP_IFACEQUEUE_PRIM CSR_BT_AVRCP_PRIM
#if !defined(EXCLUDE_CSR_BT_AVRCP_MODULE) && defined(CSR_BT_RUN_TASK_AVRCP) && (CSR_BT_RUN_TASK_AVRCP == 1)
extern void CsrBtAvrcpInit(void **gash);
extern void CsrBtAvrcpHandler(void **gash);
#define CSR_BT_AVRCP_INIT CsrBtAvrcpInit
#define CSR_BT_AVRCP_HANDLER CsrBtAvrcpHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtAvrcpDeinit(void **gash);
#define CSR_BT_AVRCP_DEINIT CsrBtAvrcpDeinit
#else
#define CSR_BT_AVRCP_DEINIT NULL
#endif
#else
#define CSR_BT_AVRCP_INIT NULL
#define CSR_BT_AVRCP_DEINIT NULL
#define CSR_BT_AVRCP_HANDLER NULL
#endif

/* Profile: AVRCP_IMG */
#define CSR_BT_AVRCP_IMAGING_IFACEQUEUE_PRIM CSR_BT_AVRCP_IMAGING_PRIM
#if !defined(EXCLUDE_CSR_BT_AVRCP_IMAGING_MODULE) && defined(CSR_BT_RUN_TASK_AVRCP_IMAGING) && (CSR_BT_RUN_TASK_AVRCP_IMAGING == 1)
extern void CsrBtAvrcpImagingInit(void **gash);
extern void CsrBtAvrcpImagingHandler(void **gash);
#define CSR_BT_AVRCP_IMAGING_INIT CsrBtAvrcpImagingInit
#define CSR_BT_AVRCP_IMAGING_HANDLER CsrBtAvrcpImagingHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtAvrcpImagingDeinit(void **gash);
#define CSR_BT_AVRCP_IMAGING_DEINIT CsrBtAvrcpImagingDeinit
#else
#define CSR_BT_AVRCP_IMAGING_DEINIT NULL
#endif
#else
#define CSR_BT_AVRCP_IMAGING_INIT NULL
#define CSR_BT_AVRCP_IMAGING_DEINIT NULL
#define CSR_BT_AVRCP_IMAGING_HANDLER NULL
#endif

/* Profile: SAPS */
#define CSR_BT_SAPS_IFACEQUEUE_PRIM CSR_BT_SAPS_PRIM
#if !defined(EXCLUDE_CSR_BT_SAPS_MODULE) && defined(CSR_BT_RUN_TASK_SAPS) && (CSR_BT_RUN_TASK_SAPS == 1)
extern void CsrBtSapsInit(void **gash);
extern void CsrBtSapsHandler(void **gash);
#define CSR_BT_SAPS_INIT CsrBtSapsInit
#define CSR_BT_SAPS_HANDLER CsrBtSapsHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtSapsDeinit(void **gash);
#define CSR_BT_SAPS_DEINIT CsrBtSapsDeinit
#else
#define CSR_BT_SAPS_DEINIT NULL
#endif
#else
#define CSR_BT_SAPS_INIT NULL
#define CSR_BT_SAPS_DEINIT NULL
#define CSR_BT_SAPS_HANDLER NULL
#endif

/* Profile: SAPC */
#define CSR_BT_SAPC_IFACEQUEUE_PRIM CSR_BT_SAPC_PRIM
#if !defined(EXCLUDE_CSR_BT_SAPC_MODULE) && defined(CSR_BT_RUN_TASK_SAPC) && (CSR_BT_RUN_TASK_SAPC == 1)
extern void CsrBtSapcInit(void **gash);
extern void CsrBtSapcHandler(void **gash);
#define CSR_BT_SAPC_INIT CsrBtSapcInit
#define CSR_BT_SAPC_HANDLER CsrBtSapcHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtSapcDeinit(void **gash);
#define CSR_BT_SAPC_DEINIT CsrBtSapcDeinit
#else
#define CSR_BT_SAPC_DEINIT NULL
#endif
#else
#define CSR_BT_SAPC_INIT NULL
#define CSR_BT_SAPC_DEINIT NULL
#define CSR_BT_SAPC_HANDLER NULL
#endif

/* Profile: GNSS_CLIENT */
#define CSR_BT_GNSS_CLIENT_IFACEQUEUE_PRIM CSR_BT_GNSS_CLIENT_PRIM
#if !defined(EXCLUDE_CSR_BT_GNSS_CLIENT_MODULE) && defined(CSR_BT_RUN_TASK_GNSS_CLIENT) && (CSR_BT_RUN_TASK_GNSS_CLIENT == 1)
extern void CsrBtGnssClientInit(void **gash);
extern void CsrBtGnssClientHandler(void **gash);
#define CSR_BT_GNSS_CLIENT_INIT CsrBtGnssClientInit
#define CSR_BT_GNSS_CLIENT_HANDLER CsrBtGnssClientHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtGnssClientDeinit(void **gash);
#define CSR_BT_GNSS_CLIENT_DEINIT CsrBtGnssClientDeinit
#else
#define CSR_BT_GNSS_CLIENT_DEINIT NULL
#endif
#else
#define CSR_BT_GNSS_CLIENT_INIT NULL
#define CSR_BT_GNSS_CLIENT_DEINIT NULL
#define CSR_BT_GNSS_CLIENT_HANDLER NULL
#endif

/* Profile: GNSS_SERVER */
#define CSR_BT_GNSS_SERVER_IFACEQUEUE_PRIM CSR_BT_GNSS_SERVER_PRIM
#if !defined(EXCLUDE_CSR_BT_GNSS_SERVER_MODULE) && defined(CSR_BT_RUN_TASK_GNSS_SERVER) && (CSR_BT_RUN_TASK_GNSS_SERVER == 1)
extern void CsrBtGnssServerInit(void **gash);
extern void CsrBtGnssServerHandler(void **gash);
#define CSR_BT_GNSS_SERVER_INIT CsrBtGnssServerInit
#define CSR_BT_GNSS_SERVER_HANDLER CsrBtGnssServerHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtGnssServerDeinit(void **gash);
#define CSR_BT_GNSS_SERVER_DEINIT CsrBtGnssServerDeinit
#else
#define CSR_BT_GNSS_SERVER_DEINIT NULL
#endif
#else
#define CSR_BT_GNSS_SERVER_INIT NULL
#define CSR_BT_GNSS_SERVER_DEINIT NULL
#define CSR_BT_GNSS_SERVER_HANDLER NULL
#endif

/* Profiles: SD */
#define CSR_BT_SD_IFACEQUEUE_PRIM CSR_BT_SD_PRIM
#if defined(CSR_BT_RUN_TASK_SD) && (CSR_BT_RUN_TASK_SD == 1)
extern void CsrBtSdInit(void **gash);
extern void CsrBtSdHandler(void **gash);
#define CSR_BT_SD_INIT CsrBtSdInit
#define CSR_BT_SD_HANDLER CsrBtSdHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtSdDeinit(void **gash);
#define CSR_BT_SD_DEINIT CsrBtSdDeinit
#else
#define CSR_BT_SD_DEINIT NULL
#endif
#else
#define CSR_BT_SD_INIT NULL
#define CSR_BT_SD_DEINIT NULL
#define CSR_BT_SD_HANDLER NULL
#endif

/* Profile: BPPS */
#define CSR_BT_BPPS_IFACEQUEUE_PRIM CSR_BT_BPPS_PRIM
#if !defined(EXCLUDE_CSR_BT_BPPS_MODULE) && defined(CSR_BT_RUN_TASK_BPPS) && (CSR_BT_RUN_TASK_BPPS == 1)
extern void CsrBtBppsInit(void **gash);
extern void CsrBtBppsHandler(void **gash);
#define CSR_BT_BPPS_INIT CsrBtBppsInit
#define CSR_BT_BPPS_HANDLER CsrBtBppsHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtBppsDeinit(void **gash);
#define CSR_BT_BPPS_DEINIT CsrBtBppsDeinit
#else
#define CSR_BT_BPPS_DEINIT NULL
#endif
#else
#define CSR_BT_BPPS_INIT NULL
#define CSR_BT_BPPS_DEINIT NULL
#define CSR_BT_BPPS_HANDLER NULL
#endif

/* Profile: HCRP */
#define CSR_BT_HCRP_IFACEQUEUE_PRIM CSR_BT_HCRP_PRIM
#if !defined(EXCLUDE_CSR_BT_HCRP_MODULE) && defined(CSR_BT_RUN_TASK_HCRP) && (CSR_BT_RUN_TASK_HCRP == 1)
extern void CsrBtHcrpInit(void **gash);
extern void CsrBtHcrpHandler(void **gash);
#define CSR_BT_HCRP_INIT CsrBtHcrpInit
#define CSR_BT_HCRP_HANDLER CsrBtHcrpHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtHcrpDeinit(void **gash);
#define CSR_BT_HCRP_DEINIT CsrBtHcrpDeinit
#else
#define CSR_BT_HCRP_DEINIT NULL
#endif
#else
#define CSR_BT_HCRP_INIT NULL
#define CSR_BT_HCRP_DEINIT NULL
#define CSR_BT_HCRP_HANDLER NULL
#endif

/* Profile: HIDH */
#define CSR_BT_HIDH_IFACEQUEUE_PRIM CSR_BT_HIDH_PRIM
#if !defined(EXCLUDE_CSR_BT_HIDH_MODULE) && defined(CSR_BT_RUN_TASK_HIDH) && (CSR_BT_RUN_TASK_HIDH == 1)
extern void CsrBtHidhInit(void **gash);
extern void CsrBtHidhHandler(void **gash);
#define CSR_BT_HIDH_INIT CsrBtHidhInit
#define CSR_BT_HIDH_HANDLER CsrBtHidhHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtHidhDeinit(void **gash);
#define CSR_BT_HIDH_DEINIT CsrBtHidhDeinit
#else
#define CSR_BT_HIDH_DEINIT NULL
#endif
#else
#define CSR_BT_HIDH_INIT NULL
#define CSR_BT_HIDH_DEINIT NULL
#define CSR_BT_HIDH_HANDLER NULL
#endif

/* Profile: HIDD */
#define CSR_BT_HIDD_IFACEQUEUE_PRIM CSR_BT_HIDD_PRIM
#if !defined(EXCLUDE_CSR_BT_HIDD_MODULE) && defined(CSR_BT_RUN_TASK_HIDD) && (CSR_BT_RUN_TASK_HIDD == 1)
extern void CsrBtHiddInit(void **gash);
extern void CsrBtHiddHandler(void **gash);
#define CSR_BT_HIDD_INIT CsrBtHiddInit
#define CSR_BT_HIDD_HANDLE CsrBtHiddHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtHiddDeinit(void **gash);
#define CSR_BT_HIDD_DEINIT CsrBtHiddDeinit
#else
#define CSR_BT_HIDD_DEINIT NULL
#endif
#else
#define CSR_BT_HIDD_INIT NULL
#define CSR_BT_HIDD_HANDLE NULL
#define CSR_BT_HIDD_DEINIT NULL
#endif

/* Profile: JSR82 */
#define CSR_BT_JSR82_IFACEQUEUE_PRIM CSR_BT_JSR82_PRIM
#if !defined(EXCLUDE_CSR_BT_JSR82_MODULE) && defined(CSR_BT_RUN_TASK_JSR82) && (CSR_BT_RUN_TASK_JSR82 == 1)
extern void CsrBtJsr82Init(void **gash);
extern void CsrBtJsr82Handler(void **gash);
#define CSR_BT_JSR82_HANDLER CsrBtJsr82Handler
#define CSR_BT_JSR82_INIT CsrBtJsr82Init
#ifdef ENABLE_SHUTDOWN
extern void CsrBtJsr82Deinit(void **gash);
#define CSR_BT_JSR82_DEINIT CsrBtJsr82Deinit
#else
#define CSR_BT_JSR82_DEINIT NULL
#endif
#else
#define CSR_BT_JSR82_INIT NULL
#define CSR_BT_JSR82_HANDLER NULL
#define CSR_BT_JSR82_DEINIT NULL
#endif

/* Profile: MCAP */
#define CSR_BT_MCAP_IFACEQUEUE_PRIM CSR_BT_MCAP_PRIM
#if !defined(EXCLUDE_CSR_BT_MCAP_MODULE) && defined(CSR_BT_RUN_TASK_MCAP) && (CSR_BT_RUN_TASK_MCAP == 1)
extern void CsrBtMcapInit(void **gash);
extern void CsrBtMcapHandler(void **gash);
#define CSR_BT_MCAP_HANDLER CsrBtMcapHandler
#define CSR_BT_MCAP_INIT CsrBtMcapInit
#ifdef ENABLE_SHUTDOWN
extern void CsrBtMcapDeinit(void **gash);
#define CSR_BT_MCAP_DEINIT CsrBtMcapDeinit
#else
#define CSR_BT_MCAP_DEINIT NULL
#endif
#else
#define CSR_BT_MCAP_INIT NULL
#define CSR_BT_MCAP_HANDLER NULL
#define CSR_BT_MCAP_DEINIT NULL
#endif

/* Profile: HDP */
#define CSR_BT_HDP_IFACEQUEUE_PRIM CSR_BT_HDP_PRIM
#if !defined(EXCLUDE_CSR_BT_HDP_MODULE) && defined(CSR_BT_RUN_TASK_HDP) && (CSR_BT_RUN_TASK_HDP == 1)
extern void CsrBtHdpInit(void **gash);
extern void CsrBtHdpHandler(void **gash);
#define CSR_BT_HDP_HANDLER CsrBtHdpHandler
#define CSR_BT_HDP_INIT CsrBtHdpInit
#ifdef ENABLE_SHUTDOWN
extern void CsrBtHdpDeinit(void **gash);
#define CSR_BT_HDP_DEINIT CsrBtHdpDeinit
#else
#define CSR_BT_HDP_DEINIT NULL
#endif
#else
#define CSR_BT_HDP_INIT NULL
#define CSR_BT_HDP_HANDLER NULL
#define CSR_BT_HDP_DEINIT NULL
#endif

/* Protocol: AMPM */
#define CSR_BT_AMPM_IFACEQUEUE_PRIM CSR_BT_AMPM_PRIM
#if defined(CSR_BT_RUN_TASK_AMPM) && (CSR_BT_RUN_TASK_AMPM == 1)
extern void CsrBtAmpmInit(void **gash);
extern void CsrBtAmpmInterfaceHandler(void **gash);
#define CSR_BT_AMPM_INIT CsrBtAmpmInit
#define CSR_BT_AMPM_HANDLER CsrBtAmpmInterfaceHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtAmpmDeinit(void **gash);
#define CSR_BT_AMPM_DEINIT CsrBtAmpmDeinit
#else
#define CSR_BT_AMPM_DEINIT NULL
#endif
#else
#define CSR_BT_AMPM_INIT NULL
#define CSR_BT_AMPM_DEINIT NULL
#define CSR_BT_AMPM_HANDLER NULL
#endif

/* Profile: PHDC Manager */
#define CSR_BT_PHDC_MGR_IFACEQUEUE_PRIM CSR_BT_PHDC_MGR_PRIM
#if !defined(EXCLUDE_CSR_BT_PHDC_MGR_MODULE) && defined(CSR_BT_RUN_TASK_PHDC_MGR) && (CSR_BT_RUN_TASK_PHDC_MGR == 1)
extern void CsrBtPhdcMgrInit(void **gash);
extern void CsrBtPhdcMgrHandler(void **gash);
#define CSR_BT_PHDC_MGR_HANDLER                 CsrBtPhdcMgrHandler
#define CSR_BT_PHDC_MGR_INIT                    CsrBtPhdcMgrInit
#ifdef ENABLE_SHUTDOWN
extern void CsrBtPhdcMgrDeinit(void **gash);
#define CSR_BT_PHDC_MGR_DEINIT                  CsrBtPhdcMgrDeinit
#else
#define CSR_BT_PHDC_MGR_DEINIT  NULL
#endif
#else
#define CSR_BT_PHDC_MGR_INIT    NULL
#define CSR_BT_PHDC_MGR_HANDLER NULL
#define CSR_BT_PHDC_MGR_DEINIT  NULL
#endif

/* Profile: PHDC Agent */
#define CSR_BT_PHDC_AG_IFACEQUEUE_PRIM CSR_BT_PHDC_AG_PRIM
#if !defined(EXCLUDE_CSR_BT_PHDC_AG_MODULE) && defined(CSR_BT_RUN_TASK_PHDC_AG) && (CSR_BT_RUN_TASK_PHDC_AG == 1)
extern void CsrBtPhdcAgentInit(void **gash);
extern void CsrBtPhdcAgentHandler(void **gash);
#define CSR_BT_PHDC_AG_HANDLER                 CsrBtPhdcAgentHandler
#define CSR_BT_PHDC_AG_INIT                    CsrBtPhdcAgentInit
#ifdef ENABLE_SHUTDOWN
extern void CsrBtPhdcAgentDeinit(void **gash);
#define CSR_BT_PHDC_AG_DEINIT                  CsrBtPhdcAgentDeinit
#else
#define CSR_BT_PHDC_AG_DEINIT  NULL
#endif
#else
#define CSR_BT_PHDC_AG_INIT    NULL
#define CSR_BT_PHDC_AG_HANDLER NULL
#define CSR_BT_PHDC_AG_DEINIT  NULL
#endif

/* Protocol: GATT */
#define CSR_BT_GATT_IFACEQUEUE_PRIM CSR_BT_GATT_PRIM
#if defined(CSR_BT_RUN_TASK_GATT) && (CSR_BT_RUN_TASK_GATT == 1)
extern void CsrBtGattInit(void **gash);
extern void CsrBtGattInterfaceHandler(void **gash);
#define CSR_BT_GATT_INIT CsrBtGattInit
#define CSR_BT_GATT_HANDLER CsrBtGattInterfaceHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtGattDeinit(void **gash);
#define CSR_BT_GATT_DEINIT CsrBtGattDeinit
#else
#define CSR_BT_GATT_DEINIT NULL
#endif
#else
#define CSR_BT_GATT_INIT NULL
#define CSR_BT_GATT_DEINIT NULL
#define CSR_BT_GATT_HANDLER NULL
#endif

/* Proximity server service */
#define CSR_BT_PROX_SRV_IFACEQUEUE_PRIM CSR_BT_PROX_SRV_PRIM
#if defined(CSR_BT_RUN_TASK_PROX_SRV) && (CSR_BT_RUN_TASK_PROX_SRV == 1)
extern void CsrBtProxSrvInit(void **gash);
extern void CsrBtProxSrvHandler(void **gash);
#define CSR_BT_PROX_SRV_INIT CsrBtProxSrvInit
#define CSR_BT_PROX_SRV_HANDLER CsrBtProxSrvHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtProxSrvDeinit(void **gash);
#define CSR_BT_PROX_SRV_DEINIT CsrBtProxSrvDeinit
#else
#define CSR_BT_PROX_SRV_DEINIT NULL
#endif
#else
#define CSR_BT_PROX_SRV_INIT NULL
#define CSR_BT_PROX_SRV_DEINIT NULL
#define CSR_BT_PROX_SRV_HANDLER NULL
#endif

/* LE server */
#define CSR_BT_LE_SRV_IFACEQUEUE_PRIM CSR_BT_LE_SRV_PRIM
#if defined(CSR_BT_RUN_TASK_LE_SRV) && (CSR_BT_RUN_TASK_LE_SRV == 1)
extern void CsrBtLeSrvInit(void **gash);
extern void CsrBtLeSrvHandler(void **gash);
#define CSR_BT_LE_SRV_INIT CsrBtLeSrvInit
#define CSR_BT_LE_SRV_HANDLER CsrBtLeSrvHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtLeSrvDeinit(void **gash);
#define CSR_BT_LE_SRV_DEINIT CsrBtLeSrvDeinit
#else
#define CSR_BT_LE_SRV_DEINIT NULL
#endif
#else
#define CSR_BT_LE_SRV_INIT NULL
#define CSR_BT_LE_SRV_DEINIT NULL
#define CSR_BT_LE_SRV_HANDLER NULL
#endif

/* LE services */
#define CSR_BT_LE_SVC_IFACEQUEUE_PRIM CSR_BT_LE_SVC_PRIM
#if defined(CSR_BT_RUN_TASK_LE_SVC) && (CSR_BT_RUN_TASK_LE_SVC == 1)
extern void CsrBtLeSvcInit(void **gash);
extern void CsrBtLeSvcHandler(void **gash);
#define CSR_BT_LE_SVC_INIT CsrBtLeSvcInit
#define CSR_BT_LE_SVC_HANDLER CsrBtLeSvcHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtLeSvcDeinit(void **gash);
#define CSR_BT_LE_SVC_DEINIT CsrBtLeSvcDeinit
#else
#define CSR_BT_LE_SVC_DEINIT NULL
#endif
#else
#define CSR_BT_LE_SVC_INIT NULL
#define CSR_BT_LE_SVC_DEINIT NULL
#define CSR_BT_LE_SVC_HANDLER NULL
#endif

/* thermometer server service */
#define CSR_BT_THERM_SRV_IFACEQUEUE_PRIM CSR_BT_THERM_SRV_PRIM
#if defined(CSR_BT_RUN_TASK_THERM_SRV) && (CSR_BT_RUN_TASK_THERM_SRV == 1)
extern void CsrBtThermSrvInit(void **gash);
extern void CsrBtThermSrvHandler(void **gash);
#define CSR_BT_THERM_SRV_INIT CsrBtThermSrvInit
#define CSR_BT_THERM_SRV_HANDLER CsrBtThermSrvHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtThermSrvDeinit(void **gash);
#define CSR_BT_THERM_SRV_DEINIT CsrBtThermSrvDeinit
#else
#define CSR_BT_THERM_SRV_DEINIT NULL
#endif
#else
#define CSR_BT_THERM_SRV_INIT NULL
#define CSR_BT_THERM_SRV_DEINIT NULL
#define CSR_BT_THERM_SRV_HANDLER NULL
#endif


/* HID over GATT host */
#define CSR_BT_HOGH_IFACEQUEUE_PRIM CSR_BT_HOGH_PRIM
#if defined(CSR_BT_RUN_TASK_HOGH) && (CSR_BT_RUN_TASK_HOGH == 1)
extern void CsrBtHoghInit(void **gash);
extern void CsrBtHoghHandler(void **gash);
#define CSR_BT_HOGH_INIT CsrBtHoghInit
#define CSR_BT_HOGH_HANDLER CsrBtHoghHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtHoghDeinit(void **gash);
#define CSR_BT_HOGH_DEINIT CsrBtHoghDeinit
#else
#define CSR_BT_HOGH_DEINIT NULL
#endif
#else
#define CSR_BT_HOGH_INIT NULL
#define CSR_BT_HOGH_DEINIT NULL
#define CSR_BT_HOGH_HANDLER NULL
#endif

/* Proximity Monitor host */
#define CSR_BT_PXPM_IFACEQUEUE_PRIM CSR_BT_PXPM_PRIM
#if defined(CSR_BT_RUN_TASK_PXPM) && (CSR_BT_RUN_TASK_PXPM == 1)
extern void CsrBtPxpmInit(void **gash);
extern void CsrBtPxpmHandler(void **gash);
#define CSR_BT_PXPM_INIT CsrBtPxpmInit
#define CSR_BT_PXPM_HANDLER CsrBtPxpmHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtPxpmDeinit(void **gash);
#define CSR_BT_PXPM_DEINIT CsrBtPxpmDeinit
#else
#define CSR_BT_PXPM_DEINIT NULL
#endif
#else
#define CSR_BT_PXPM_INIT NULL
#define CSR_BT_PXPM_DEINIT NULL
#define CSR_BT_PXPM_HANDLER NULL
#endif

#define CSR_BT_LPM_IFACEQUEUE_PRIM CSR_BT_LPM_PRIM
#if defined(CSR_BT_RUN_TASK_LPM) && (CSR_BT_RUN_TASK_LPM == 1)
extern void CsrBtLpmInit(void **gash);
extern void CsrBtLpmHandler(void **gash);
#define CSR_BT_LPM_INIT CsrBtLpmInit
#define CSR_BT_LPM_HANDLER CsrBtLpmHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtLpmDeinit(void **gash);
#define CSR_BT_LPM_DEINIT CsrBtLpmDeinit
#else
#define CSR_BT_LPM_DEINIT NULL
#endif
#else
#define CSR_BT_LPM_INIT NULL
#define CSR_BT_LPM_DEINIT NULL
#define CSR_BT_LPM_HANDLER NULL
#endif

/* TPMP Monitor host */
#define CSR_BT_TPM_IFACEQUEUE_PRIM CSR_BT_TPM_PRIM
#if defined(CSR_BT_RUN_TASK_TPM) && (CSR_BT_RUN_TASK_TPM == 1)
extern void CsrBtTpmInit(void **gash);
extern void CsrBtTpmHandler(void **gash);
#define CSR_BT_TPM_INIT CsrBtTpmInit
#define CSR_BT_TPM_HANDLER CsrBtTpmHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtTpmDeinit(void **gash);
#define CSR_BT_TPM_DEINIT CsrBtTpmDeinit
#else
#define CSR_BT_TPM_DEINIT NULL
#endif
#else
#define CSR_BT_TPM_INIT NULL
#define CSR_BT_TPM_DEINIT NULL
#define CSR_BT_TPM_HANDLER NULL
#endif

/* Application */
#define CSR_BT_EXTRA_IFACEQUEUE_PRIM CSR_BT_USER_PRIM
#if defined(CSR_BT_RUN_TASK_EXTRA_TASK) && (CSR_BT_RUN_TASK_EXTRA_TASK == 1)
extern void CsrBtExtraTaskInit(void **gash);
extern void CsrBtExtraTaskHandler(void **gash);
#define CSR_BT_EXTRA_TASK_INIT CsrBtExtraTaskInit
#define CSR_BT_EXTRA_TASK_HANDLER CsrBtExtraTaskHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtExtraTaskDeinit(void **gash);
#define CSR_BT_EXTRA_TASK_DEINIT CsrBtExtraTaskDeinit
#else
#define CSR_BT_EXTRA_TASK_DEINIT NULL
#endif
#else
#define CSR_BT_EXTRA_TASK_INIT NULL
#define CSR_BT_EXTRA_TASK_DEINIT NULL
#define CSR_BT_EXTRA_TASK_HANDLER NULL
#endif

/* Application */
#define TESTQUEUE_PRIM CSR_BT_USER_PRIM
#if !defined(EXCLUDE_TEST_MODULE) && defined(CSR_BT_RUN_TASK_TEST_TASK) && (CSR_BT_RUN_TASK_TEST_TASK == 1)
extern void CsrBtAppInit(void ** gash);
extern void CsrBtAppHandler(void ** gash);
#define CSR_BT_TEST_INIT CsrBtAppInit
#define CSR_BT_TEST_HANDLER CsrBtAppHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtAppDeinit(void ** gash);
#define CSR_BT_TEST_DEINIT CsrBtAppDeinit
#else
#define CSR_BT_TEST_DEINIT NULL
#endif
#else
#define CSR_BT_TEST_INIT NULL
#define CSR_BT_TEST_DEINIT NULL
#define CSR_BT_TEST_HANDLER NULL
#endif

/* Audio Stream Manager */
#define CSR_BT_ASM_IFACEQUEUE_PRIM CSR_BT_ASM_PRIM
#if !defined(EXCLUDE_CSR_BT_ASM_MODULE) && defined(CSR_BT_RUN_TASK_ASM) && (CSR_BT_RUN_TASK_ASM == 1)
extern void CsrBtAsmInit(void **gash);
extern void CsrBtAsmHandler(void **gash);
#define CSR_BT_ASM_INIT CsrBtAsmInit
#define CSR_BT_ASM_HANDLER CsrBtAsmHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtAsmDeinit(void **gash);
#define CSR_BT_ASM_DEINIT CsrBtAsmDeinit
#else
#define CSR_BT_ASM_DEINIT NULL
#endif
#else
#define CSR_BT_ASM_INIT NULL
#define CSR_BT_ASM_DEINIT NULL
#define CSR_BT_ASM_HANDLER NULL
#endif

/* Battery Server  */
#define CSR_BT_BAS_SERVER_IFACEQUEUE_PRIM BAS_SERVER_PRIM
extern void CsrBtBasInit(void **gash);
extern void CsrBtBasHandler(void **gash);
#define CSR_BT_BAS_SERVER_INIT CsrBtBasInit
#define CSR_BT_BAS_SERVER_HANDLER CsrBtBasHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtBasDeinit(void **gash);
#define CSR_BT_BAS_SERVER_DEINIT CsrBtBasDeinit
#else
#define CSR_BT_BAS_SERVER_DEINIT NULL
#endif

/* Qualcomm Snapdragon Sound Server */
#define CSR_BT_QSS_SERVER_IFACEQUEUE_PRIM QSS_SERVER_PRIM
extern void gattQssServerTaskInit(void** gash);
extern void gattQssServerMsgHandler(void** gash);
#define CSR_BT_QSS_SERVER_INIT gattQssServerTaskInit
#define CSR_BT_QSS_SERVER_HANDLER gattQssServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gattQssServerTaskDeinit(void** gash);
#define CSR_BT_QSS_SERVER_DEINIT gattQssServerTaskDeinit
#else
#define CSR_BT_QSS_SERVER_DEINIT NULL
#endif

/* Device Info Server  */
#define CSR_BT_DIS_SERVER_IFACEQUEUE_PRIM DIS_SERVER_PRIM
extern void CsrBtDisInit(void **gash);
extern void CsrBtDisHandler(void **gash);
#define CSR_BT_DIS_SERVER_INIT CsrBtDisInit
#define CSR_BT_DIS_SERVER_HANDLER CsrBtDisHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtDisDeinit(void **gash);
#define CSR_BT_DIS_SERVER_DEINIT CsrBtDisDeinit
#else
#define CSR_BT_DIS_SERVER_DEINIT NULL
#endif

/* Gatt Gap Proxy */
#define CSR_BT_GGPROXY_IFACEQUEUE_PRIM CSR_BT_GGPROXY_PRIM
#if !defined(EXCLUDE_CSR_BT_GGPROXY_MODULE) && defined(CSR_BT_RUN_TASK_GGPROXY) && (CSR_BT_RUN_TASK_GGPROXY == 1)
extern void CsrBtGgProxyInit(void **gash);
extern void CsrBtGgProxyHandler(void **gash);
#define CSR_BT_GGPROXY_INIT CsrBtGgProxyInit
#define CSR_BT_GGPROXY_HANDLER CsrBtGgProxyHandler
#ifdef ENABLE_SHUTDOWN
extern void CsrBtGgProxyDeinit(void **gash);
#define CSR_BT_GGPROXY_DEINIT CsrBtGgProxyDeinit
#else
#define CSR_BT_GGPROXY_DEINIT NULL
#endif
#else
#define CSR_BT_GGPROXY_INIT NULL
#define CSR_BT_GGPROXY_DEINIT NULL
#define CSR_BT_GGPROXY_HANDLER NULL
#endif

#define CSR_BT_BAP_IFACEQUEUE_PRIM BAP_PRIM
extern void bapClientInit(void **gash);
extern void bapClientHandler(void **gash);
#define CSR_BT_BAP_CLIENT_INIT bapClientInit
#define CSR_BT_BAP_CLIENT_HANDLER bapClientHandler
#ifdef ENABLE_SHUTDOWN
extern void bapClientDeinit(void **gash);
#define CSR_BT_BAP_CLIENT_DEINIT bapClientDeinit
#else
#define CSR_BT_BAP_CLIENT_DEINIT NULL
#endif

#define CSR_BT_VCP_IFACEQUEUE_PRIM VCP_PRIM
extern void vcp_init(void **gash);
extern void vcpMsgHandler(void **gash);
#define CSR_BT_VCP_INIT vcp_init
#define CSR_BT_VCP_HANDLER vcpMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void vcp_deinit(void **gash);
#define CSR_BT_VCP_DEINIT vcp_deinit
#else
#define CSR_BT_VCP_DEINIT NULL
#endif


#define CSR_BT_MICP_IFACEQUEUE_PRIM MICP_PRIM
extern void micp_init(void **gash);
extern void micpMsgHandler(void **gash);
#define CSR_BT_MICP_INIT micp_init
#define CSR_BT_MICP_HANDLER micpMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void micp_deinit(void **gash);
#define CSR_BT_MICP_DEINIT micp_deinit
#else
#define CSR_BT_MICP_DEINIT NULL
#endif


#define CSR_BT_CCP_IFACEQUEUE_PRIM CCP_PRIM
extern void ccp_init(void **gash);
extern void ccpMsgHandler(void **gash);
#define CSR_BT_CCP_INIT ccp_init
#define CSR_BT_CCP_HANDLER ccpMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void ccp_deinit(void **gash);
#define CSR_BT_CCP_DEINIT ccp_deinit
#else
#define CSR_BT_CCP_DEINIT NULL
#endif


#define CSR_BT_ASCS_CLIENT_IFACEQUEUE_PRIM ASCS_CLIENT_PRIM
extern void gatt_ascs_client_init(void **gash);
extern void AscsClientMsgHandler(void **gash);
#define CSR_BT_ASCS_CLIENT_INIT gatt_ascs_client_init
#define CSR_BT_ASCS_CLIENT_HANDLER AscsClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattAscsClientDeInit(void **gash);
#define CSR_BT_ASCS_CLIENT_DEINIT GattAscsClientDeInit
#else
#define CSR_BT_ASCS_CLIENT_DEINIT NULL
#endif


#define CSR_BT_PACS_CLIENT_IFACEQUEUE_PRIM PACS_CLIENT_PRIM
extern void GattPacsClientInit(void** gash);
extern void PacsClientMsgHandler(void** gash);
#define CSR_BT_PACS_CLIENT_INIT GattPacsClientInit
#define CSR_BT_PACS_CLIENT_HANDLER PacsClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gattPacsClientDeinit(void** gash);
#define CSR_BT_PACS_CLIENT_DEINIT gattPacsClientDeinit
#else
#define CSR_BT_PACS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_VCS_CLIENT_IFACEQUEUE_PRIM VCS_CLIENT_PRIM
extern void gatt_vcs_client_init(void **gash);
extern void gattVcsClientMsgHandler(void **gash);
#define CSR_BT_VCS_CLIENT_INIT gatt_vcs_client_init
#define CSR_BT_VCS_CLIENT_HANDLER gattVcsClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattVcsClientDeInit(void **gash);
#define CSR_BT_VCS_CLIENT_DEINIT GattVcsClientDeInit
#else
#define CSR_BT_VCS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_MICS_CLIENT_IFACEQUEUE_PRIM MICS_CLIENT_PRIM
extern void GattMicsClientInit(void **gash);
extern void gattMicsClientMsgHandler(void **gash);
#define CSR_BT_MICS_CLIENT_INIT GattMicsClientInit
#define CSR_BT_MICS_CLIENT_HANDLER gattMicsClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattMicsClientDeInit(void **gash);
#define CSR_BT_MICS_CLIENT_DEINIT GattMicsClientDeInit
#else
#define CSR_BT_MICS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_VOCS_CLIENT_IFACEQUEUE_PRIM VOCS_CLIENT_PRIM
extern void gatt_vocs_client_init(void **gash);
extern void gattVocsClientMsgHandler(void **gash);
#define CSR_BT_VOCS_CLIENT_INIT gatt_vocs_client_init
#define CSR_BT_VOCS_CLIENT_HANDLER gattVocsClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattVocsClientDeInit(void **gash);
#define CSR_BT_VOCS_CLIENT_DEINIT GattVocsClientDeInit
#else
#define CSR_BT_VOCS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_AICS_CLIENT_IFACEQUEUE_PRIM AICS_CLIENT_PRIM
extern void gatt_aics_client_init(void **gash);
extern void gattAicsClientMsgHandler(void **gash);
#define CSR_BT_AICS_CLIENT_INIT gatt_aics_client_init
#define CSR_BT_AICS_CLIENT_HANDLER gattAicsClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattAicsClientDeInit(void **gash);
#define CSR_BT_AICS_CLIENT_DEINIT GattAicsClientDeInit
#else
#define CSR_BT_AICS_CLIENT_DEINIT NULL
#endif


#define CSR_BT_GATT_SRVC_DISC_IFACEQUEUE_PRIM GATT_SRVC_DISC_PRIM
extern void GattServiceDiscoveryInit(void** gash);
extern void GattServiceDiscoveryMsgHandler(void** gash);
#define CSR_BT_GATT_SRVC_DISC_INIT GattServiceDiscoveryInit
#define CSR_BT_GATT_SRVC_DISC_HANDLER GattServiceDiscoveryMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattServiceDiscoveryDeinit(void** gash);
#define CSR_BT_GATT_SRVC_DISC_DEINIT GattServiceDiscoveryDeinit
#else
#define CSR_BT_GATT_SRVC_DISC_DEINIT NULL
#endif

#define CSR_BT_PACS_SERVER_IFACEQUEUE_PRIM PACS_SERVER_PRIM
extern void gatt_pacs_server_init(void** gash);
extern void pacs_server_msg_handler(void** gash);
#define CSR_BT_PACS_SERVER_INIT gatt_pacs_server_init
#define CSR_BT_PACS_SERVER_HANDLER pacs_server_msg_handler
#ifdef ENABLE_SHUTDOWN
extern void gatt_pacs_server_deinit(void** gash);
#define CSR_BT_PACS_SERVER_DEINIT gatt_pacs_server_deinit
#else
#define CSR_BT_PACS_SERVER_DEINIT NULL
#endif

#ifndef EXCLUDE_CSR_BT_HIDS_SERVER_MODULE
#define CSR_BT_HIDS_SERVER_IFACEQUEUE_PRIM HIDS_SERVER_PRIM
extern void gattHidsServerInit(void** gash);
extern void gattHidsServerMsgHandler(void** gash);
#define CSR_BT_HIDS_SERVER_INIT gattHidsServerInit
#define CSR_BT_HIDS_SERVER_HANDLER gattHidsServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gatt_hids_server_deinit(void** gash);
#define CSR_BT_HIDS_SERVER_DEINIT gatt_hids_server_deinit
#else
#define CSR_BT_HIDS_SERVER_DEINIT NULL
#endif
#endif

#define CSR_BT_VCS_SERVER_IFACEQUEUE_PRIM VCS_SERVER_PRIM
extern void gatt_vcs_server_init(void** gash);
extern void vcs_server_msg_handler(void** gash);
#define CSR_BT_VCS_SERVER_INIT gatt_vcs_server_init
#define CSR_BT_VCS_SERVER_HANDLER vcs_server_msg_handler
#ifdef ENABLE_SHUTDOWN
extern void gatt_vcs_server_deinit(void** gash);
#define CSR_BT_VCS_SERVER_DEINIT gatt_vcs_server_deinit
#else
#define CSR_BT_VCS_SERVER_DEINIT NULL
#endif

#define CSR_BT_BAP_SERVER_IFACEQUEUE_PRIM BAP_SERVER_PRIM
extern void BapServerInit(void** gash);
extern void BapServerMsgHandler(void** gash);
#define CSR_BT_BAP_SERVER_INIT BapServerInit
#define CSR_BT_BAP_SERVER_HANDLER BapServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void BapServerDeinit(void** gash);
#define CSR_BT_BAP_SERVER_DEINIT BapServerDeinit
#else
#define CSR_BT_BAP_SERVER_DEINIT NULL
#endif

#define CSR_BT_WMAS_SERVER_IFACEQUEUE_PRIM WMAS_SERVER_PRIM
extern void gatt_wmas_server_init(void** gash);
extern void wmas_server_msg_handler(void** gash);
#define CSR_BT_WMAS_SERVER_INIT gatt_wmas_server_init
#define CSR_BT_WMAS_SERVER_HANDLER wmas_server_msg_handler
#ifdef ENABLE_SHUTDOWN
extern void gatt_wmas_server_deinit(void** gash);
#define CSR_BT_WMAS_SERVER_DEINIT gatt_wmas_server_deinit
#else
#define CSR_BT_WMAS_SERVER_DEINIT NULL
#endif

#define CSR_BT_ASCS_SERVER_IFACEQUEUE_PRIM ASCS_SERVER_PRIM
extern void gatt_ascs_server_task_init(void** gash);
extern void ascs_server_msg_handler(void** gash);
#define CSR_BT_ASCS_SERVER_INIT gatt_ascs_server_task_init
#define CSR_BT_ASCS_SERVER_HANDLER ascs_server_msg_handler
#ifdef ENABLE_SHUTDOWN
extern void gatt_ascs_server_task_deinit(void** gash);
#define CSR_BT_ASCS_SERVER_DEINIT gatt_ascs_server_task_deinit
#else
#define CSR_BT_ASCS_SERVER_DEINIT NULL
#endif

#define CSR_BT_TBS_SERVER_IFACEQUEUE_PRIM TBS_SERVER_PRIM
extern void gattTbsServerInit(void** gash);
extern void tbsServerMsgHandler(void** gash);
#define CSR_BT_TBS_SERVER_INIT gattTbsServerInit
#define CSR_BT_TBS_SERVER_HANDLER tbsServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gattTbsServerDeinit(void** gash);
#define CSR_BT_TBS_SERVER_DEINIT gattTbsServerDeinit
#else
#define CSR_BT_TBS_SERVER_DEINIT NULL
#endif

#define CSR_BT_CSIP_IFACEQUEUE_PRIM CSIP_PRIM
extern void CsipInit(void **gash);
extern void csipMsgHandler(void **gash);
#define CSR_BT_CSIP_INIT CsipInit
#define CSR_BT_CSIP_HANDLER csipMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void CsipDeinit(void **gash);
#define CSR_BT_CSIP_DEINIT CsipDeinit
#else
#define CSR_BT_CSIP_DEINIT NULL
#endif

#define CSR_BT_CSIS_CLIENT_IFACEQUEUE_PRIM CSIS_CLIENT_PRIM
extern void gattCsisClientInit(void **gash);
extern void gattCsisClientMsgHandler(void **gash);
#define CSR_BT_CSIS_CLIENT_INIT gattCsisClientInit
#define CSR_BT_CSIS_CLIENT_HANDLER gattCsisClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattCsisClientDeInit(void **gash);
#define CSR_BT_CSIS_CLIENT_DEINIT GattCsisClientDeInit
#else
#define CSR_BT_CSIS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_BASS_CLIENT_IFACEQUEUE_PRIM BASS_CLIENT_PRIM
extern void gatt_bass_client_init(void **gash);
extern void gattBassClientMsgHandler(void **gash);
#define CSR_BT_BASS_CLIENT_INIT gatt_bass_client_init
#define CSR_BT_BASS_CLIENT_HANDLER gattBassClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattBassClientDeInit(void **gash);
#define CSR_BT_BASS_CLIENT_DEINIT GattBassClientDeInit
#else
#define CSR_BT_BASS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_TBS_CLIENT_IFACEQUEUE_PRIM TBS_CLIENT_PRIM
extern void gatt_tbs_client_init(void **gash);
extern void gattTbsClientMsgHandler(void **gash);
#define CSR_BT_TBS_CLIENT_INIT gatt_tbs_client_init
#define CSR_BT_TBS_CLIENT_HANDLER gattTbsClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattTbsClientDeInit(void **gash);
#define CSR_BT_TBS_CLIENT_DEINIT GattTbsClientDeInit
#else
#define CSR_BT_TBS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_CAS_SERVER_IFACEQUEUE_PRIM CAS_SERVER_PRIM
extern void gatt_cas_server_init(void** gash);
extern void cas_server_msg_handler(void** gash);
#define CSR_BT_CAS_SERVER_INIT gatt_cas_server_init
#define CSR_BT_CAS_SERVER_HANDLER cas_server_msg_handler
#ifdef ENABLE_SHUTDOWN
extern void gatt_cas_server_deinit(void** gash);
#define CSR_BT_CAS_SERVER_DEINIT gatt_cas_server_deinit
#else
#define CSR_BT_CAS_SERVER_DEINIT NULL
#endif

#define CSR_BT_CSIS_SERVER_IFACEQUEUE_PRIM CSIS_SERVER_PRIM
extern void gatt_csis_server_init(void** gash);
extern void csis_server_msg_handler(void** gash);
#define CSR_BT_CSIS_SERVER_INIT gatt_csis_server_init
#define CSR_BT_CSIS_SERVER_HANDLER csis_server_msg_handler
#ifdef ENABLE_SHUTDOWN
extern void gatt_csis_server_deinit(void** gash);
#define CSR_BT_CSIS_SERVER_DEINIT gatt_csis_server_deinit
#else
#define CSR_BT_CSIS_SERVER_DEINIT NULL
#endif

#define CSR_BT_BASS_SERVER_IFACEQUEUE_PRIM BASS_SERVER_PRIM
extern void gatt_bass_server_init(void** gash);
extern void GattBassServerMsgHandler(void** gash);
#define CSR_BT_BASS_SERVER_INIT gatt_bass_server_init
#define CSR_BT_BASS_SERVER_HANDLER GattBassServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gatt_bass_server_deinit(void** gash);
#define CSR_BT_BASS_SERVER_DEINIT gatt_bass_server_deinit
#else
#define CSR_BT_BASS_SERVER_DEINIT NULL
#endif

#define CSR_BT_MCS_CLIENT_IFACEQUEUE_PRIM MCS_CLIENT_PRIM
extern void gattMcsClientInit(void **gash);
extern void gattMcsClientMsgHandler(void **gash);
#define CSR_BT_MCS_CLIENT_INIT gattMcsClientInit
#define CSR_BT_MCS_CLIENT_HANDLER gattMcsClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gattMcsClientDeInit(void **gash);
#define CSR_BT_MCS_CLIENT_DEINIT gattMcsClientDeInit
#else
#define CSR_BT_MCS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_MCP_IFACEQUEUE_PRIM MCP_PRIM
extern void mcpInit(void **gash);
extern void mcpMsgHandler(void **gash);
#define CSR_BT_MCP_INIT mcpInit
#define CSR_BT_MCP_HANDLER mcpMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void mcpDeInit(void **gash);
#define CSR_BT_MCP_DEINIT mcpDeInit
#else
#define CSR_BT_MCP_DEINIT NULL
#endif

#define CSR_BT_MCS_SERVER_IFACEQUEUE_PRIM MCS_SERVER_PRIM
extern void gattMcsServerInit(void** gash);
extern void gattMcsServerMsgHandler(void** gash);
#define CSR_BT_MCS_SERVER_INIT gattMcsServerInit
#define CSR_BT_MCS_SERVER_HANDLER gattMcsServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gatt_mcs_server_deinit(void** gash);
#define CSR_BT_MCS_SERVER_DEINIT gatt_mcs_server_deinit
#else
#define CSR_BT_MCS_SERVER_DEINIT NULL
#endif

#define CSR_BT_GMAS_SERVER_IFACEQUEUE_PRIM GMAS_SERVER_PRIM
extern void gattGmasServerInit(void** gash);
extern void gattGmasServerMsgHandler(void** gash);
#define CSR_BT_GMAS_SERVER_INIT gattGmasServerInit
#define CSR_BT_GMAS_SERVER_HANDLER gattGmasServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gattGmasServerDeinit(void** gash);
#define CSR_BT_GMAS_SERVER_DEINIT gattGmasServerDeinit
#else
#define CSR_BT_GMAS_SERVER_DEINIT NULL
#endif

#define CSR_BT_GMAS_CLIENT_IFACEQUEUE_PRIM GMAS_CLIENT_PRIM
extern void gattGmasClientInit(void **gash);
extern void gattGmasClientMsgHandler(void **gash);
#define CSR_BT_GMAS_CLIENT_INIT gattGmasClientInit
#define CSR_BT_GMAS_CLIENT_HANDLER gattGmasClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gattGmasClientDeInit(void **gash);
#define CSR_BT_GMAS_CLIENT_DEINIT gattGmasClientDeInit
#else
#define CSR_BT_GMAS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_GMAP_CLIENT_IFACEQUEUE_PRIM GMAP_CLIENT_PRIM
extern void gmapClientInit(void **gash);
extern void gmapClientMsgHandler(void **gash);
#define CSR_BT_GMAP_CLIENT_INIT gmapClientInit
#define CSR_BT_GMAP_CLIENT_HANDLER gmapClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gmapClientDeInit(void **gash);
#define CSR_BT_GMAP_CLIENT_DEINIT gmapClientDeInit
#else
#define CSR_BT_GMAP_CLIENT_DEINIT NULL
#endif

#define CSR_BT_MICS_SERVER_IFACEQUEUE_PRIM MICS_SERVER_PRIM
extern void gatt_mics_server_init(void** gash);
extern void mics_server_msg_handler(void** gash);
#define CSR_BT_MICS_SERVER_INIT gatt_mics_server_init
#define CSR_BT_MICS_SERVER_HANDLER mics_server_msg_handler
#ifdef ENABLE_SHUTDOWN
extern void gatt_mics_server_deinit(void** gash);
#define CSR_BT_MICS_SERVER_DEINIT gatt_mics_server_deinit
#else
#define CSR_BT_MICS_SERVER_DEINIT NULL
#endif


#define CSR_BT_TPS_SERVER_IFACEQUEUE_PRIM TPS_SERVER_PRIM
extern void GattTpsServerTaskInit(void** gash);
extern void TpsServerMsgHandler(void** gash);
#define CSR_BT_TPS_SERVER_INIT GattTpsServerTaskInit
#define CSR_BT_TPS_SERVER_HANDLER TpsServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattTpsServerTaskDeinit(void** gash);
#define CSR_BT_TPS_SERVER_DEINIT GattTpsServerTaskDeinit
#else
#define CSR_BT_TPS_SERVER_DEINIT NULL
#endif

#define CSR_BT_TDS_SERVER_IFACEQUEUE_PRIM TDS_SERVER_PRIM
extern void GattTdsServerTaskInit(void** gash);
extern void TdsServerMsgHandler(void** gash);
#define CSR_BT_TDS_SERVER_INIT GattTdsServerTaskInit
#define CSR_BT_TDS_SERVER_HANDLER TdsServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattTdsServerTaskDeinit(void** gash);
#define CSR_BT_TDS_SERVER_DEINIT GattTdsServerTaskDeinit
#else
#define CSR_BT_TDS_SERVER_DEINIT NULL
#endif

#define CSR_BT_TDS_CLIENT_IFACEQUEUE_PRIM TDS_CLIENT_PRIM
extern void gattTdsClientInit(void** gash);
extern void gattTdsClientMsgHandler(void** gash);
#define CSR_BT_TDS_CLIENT_INIT gattTdsClientInit
#define CSR_BT_TDS_CLIENT_HANDLER gattTdsClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gattTdsClientDeInit(void** gash);
#define CSR_BT_TDS_CLIENT_DEINIT gattTdsClientDeInit
#else
#define CSR_BT_TDS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_CHP_SEEKER_IFACEQUEUE_PRIM CHP_SEEKER_PRIM
extern void chpSeekerInit(void** gash);
extern void chpSeekerMsgHandler(void** gash);
#define CSR_BT_CHP_SEEKER_INIT chpSeekerInit
#define CSR_BT_CHP_SEEKER_HANDLER chpSeekerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void chpSeekerDeInit(void** gash);
#define CSR_BT_CHP_SEEKER_DEINIT chpSeekerDeInit
#else
#define CSR_BT_CHP_SEEKER_DEINIT NULL
#endif

#define CSR_BT_TMAS_CLIENT_IFACEQUEUE_PRIM TMAS_CLIENT_PRIM
extern void gattTmasClientInit(void **gash);
extern void gattTmasClientMsgHandler(void **gash);
#define CSR_BT_TMAS_CLIENT_INIT gattTmasClientInit
#define CSR_BT_TMAS_CLIENT_HANDLER gattTmasClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gattTmasClientDeInit(void **gash);
#define CSR_BT_TMAS_CLIENT_DEINIT gattTmasClientDeInit
#else
#define CSR_BT_TMAS_CLIENT_DEINIT NULL
#endif

#define CSR_BT_TMAP_CLIENT_IFACEQUEUE_PRIM TMAP_CLIENT_PRIM
extern void tmapClientInit(void **gash);
extern void tmapClientMsgHandler(void **gash);
#define CSR_BT_TMAP_CLIENT_INIT tmapClientInit
#define CSR_BT_TMAP_CLIENT_HANDLER tmapClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void tmapClientDeInit(void **gash);
#define CSR_BT_TMAP_CLIENT_DEINIT tmapClientDeInit
#else
#define CSR_BT_TMAP_CLIENT_DEINIT NULL
#endif

#define CSR_BT_TMAS_SERVER_IFACEQUEUE_PRIM TMAS_SERVER_PRIM
extern void gattTmasServerInit(void** gash);
extern void gattTmasServerMsgHandler(void** gash);
#define CSR_BT_TMAS_SERVER_INIT gattTmasServerInit
#define CSR_BT_TMAS_SERVER_HANDLER gattTmasServerMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void gattTmasServerDeinit(void** gash);
#define CSR_BT_TMAS_SERVER_DEINIT gattTmasServerDeinit
#else
#define CSR_BT_TMAS_SERVER_DEINIT NULL
#endif

#define CSR_BT_CAP_CLIENT_IFACEQUEUE_PRIM CAP_CLIENT_PRIM
extern void CapClientTaskInit(void** gash);
extern void CapClientMsgHandler(void** gash);
#define CSR_BT_CAP_CLIENT_INIT CapClientTaskInit
#define CSR_BT_CAP_CLIENT_HANDLER CapClientMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void GattCapClientTaskDeinit(void** gash);
#define CSR_BT_CAP_CLIENT_DEINIT GattCapClientTaskDeinit
#else
#define CSR_BT_CAP_CLIENT_DEINIT NULL
#endif

#define CSR_BT_PBP_IFACEQUEUE_PRIM PBP_PRIM
extern void pbpInit(void **gash);
extern void pbpMsgHandler(void **gash);
#define CSR_BT_PBP_INIT pbpInit
#define CSR_BT_PBP_HANDLER pbpMsgHandler
#ifdef ENABLE_SHUTDOWN
extern void pbpDeInit(void **gash);
#define CSR_BT_PBP_DEINIT pbpDeInit
#else
#define CSR_BT_PBP_DEINIT NULL
#endif

extern void CsrBtGattClientUtilInit(void **gash);
extern void CsrBtGattClientUtilHandler(void **gash);
#define CSR_BT_GATT_CLIENT_UTIL_INIT CsrBtGattClientUtilInit
#define CSR_BT_GATT_CLIENT_UTIL_HANDLER CsrBtGattClientUtilHandler

#ifdef INSTALL_CTM
extern void CtmInit(void** gash);
extern void CtmHandler(void** gash);
#define CTM_INIT CtmInit
#define CTM_HANDLER CtmHandler
#ifdef ENABLE_SHUTDOWN
extern void CtmDeinit(void** gash);
#define CTM_DEINIT CtmDeinit
#else
#define CTM_DEINIT NULL
#endif
#endif

#if defined(CSR_BT_RUN_TASK_BMM) && (CSR_BT_RUN_TASK_BMM == 1)
extern void BmmInit(void **gash);
extern void BmmHandler(void **gash);
#define BMM_INIT BmmInit
#define BMM_TASK BmmHandler
#ifdef ENABLE_SHUTDOWN
extern void BmmDeinit(void **gash);
#define BMM_DEINIT BmmDeinit
#else
#define BMM_DEINIT NULL
#endif
#else
#define BMM_INIT NULL
#define BMM_DEINIT NULL
#define BMM_TASK NULL
#endif


/* CSR_BT queue definitions */
#ifdef CSR_TARGET_PRODUCT_VM

/*Profiles: Vsc*/
extern void vscInit(void **gash);
extern void vscHandler(void **gash);

#ifdef CSR_BT_APPLICATION_SECURITY_HANDLER
#undef CSR_BT_APPLICATION_SECURITY_HANDLER
#endif
#define CSR_BT_APPLICATION_SECURITY_HANDLER GET_EXT_TASK_ID(APP_TRAP_TASK_NULL)
#else
extern CsrSchedQid ADV_SCAN_IFACEQUEUE;
#endif

extern CsrSchedQid DM_HCI_IFACEQUEUE;
extern CsrSchedQid L2CAP_IFACEQUEUE;
extern CsrSchedQid RFCOMM_IFACEQUEUE;
extern CsrSchedQid SDP_L2CAP_IFACEQUEUE;
extern CsrSchedQid SDP_IFACEQUEUE;
extern CsrSchedQid ATT_IFACEQUEUE;

extern CsrSchedQid CSR_BT_CM_IFACEQUEUE;
extern CsrSchedQid CSR_BT_SC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_HF_IFACEQUEUE;
extern CsrSchedQid CSR_BT_GATT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_AV_IFACEQUEUE;
extern CsrSchedQid CSR_BT_AVRCP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_PAC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_SPP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_HIDD_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BAS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_QSS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_DIS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_HFG_IFACEQUEUE;
extern CsrSchedQid CSR_BT_VSADV_SCAN_IFACEQUEUE;

#ifndef CSR_TARGET_PRODUCT_VM
extern CsrSchedQid CSR_BT_BNEP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_IP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_ICMP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_UDP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_DHCP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_TFTP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_CTRL_IFACEQUEUE;
extern CsrSchedQid CSR_BT_PPP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_DG_IFACEQUEUE;
extern CsrSchedQid CSR_BT_AT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_OPS_IFACEQUEUE;
extern CsrSchedQid CSR_BT_OPC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_SYNCS_IFACEQUEUE;
extern CsrSchedQid CSR_BT_SYNCC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_IWU_IFACEQUEUE;
extern CsrSchedQid CSR_BT_SPP_EXTRA_IFACEQUEUE;
extern CsrSchedQid CSR_BT_FAX_IFACEQUEUE;
extern CsrSchedQid CSR_BT_FTS_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BSL_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BIPC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_FTC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BPPC_IFACEQUEUE;
#ifdef CSR_BT_INSTALL_AVRCP_COVER_ART
extern CsrSchedQid CSR_BT_AVRCP_IMAGING_IFACEQUEUE;
#endif
extern CsrSchedQid CSR_BT_SAPS_IFACEQUEUE;
extern CsrSchedQid CSR_BT_SAPC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_SD_IFACEQUEUE;
extern CsrSchedQid CSR_BT_HIDH_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BPPS_IFACEQUEUE;
extern CsrSchedQid CSR_BT_HCRP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BIPS_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BIPS_EXTRA_IFACEQUEUE;
extern CsrSchedQid CSR_BT_SMLC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_SMLS_IFACEQUEUE;
extern CsrSchedQid CSR_BT_DUNC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_JSR82_IFACEQUEUE;
extern CsrSchedQid CSR_BT_HIDD_IFACEQUEUE;
extern CsrSchedQid CSR_BT_PAS_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MCAP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_HDP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MAPC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MAPC_EXTRA_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MAPS_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MAPS_EXTRA_IFACEQUEUE;
extern CsrSchedQid CSR_BT_AMPM_IFACEQUEUE;
extern CsrSchedQid CSR_BT_PHDC_MGR_IFACEQUEUE;
extern CsrSchedQid CSR_BT_PHDC_AG_IFACEQUEUE;
extern CsrSchedQid CSR_BT_PROX_SRV_IFACEQUEUE;
extern CsrSchedQid CSR_BT_THERM_SRV_IFACEQUEUE;
extern CsrSchedQid CSR_BT_GNSS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_GNSS_SERVER_IFACEQUEUE;
extern CsrSchedQid TESTQUEUE;
extern CsrSchedQid CSR_BT_EXTRA_IFACEQUEUE;
extern CsrSchedQid CSR_BT_ASM_IFACEQUEUE;
extern CsrSchedQid CSR_BT_HOGH_IFACEQUEUE;
extern CsrSchedQid CSR_BT_PXPM_IFACEQUEUE;
extern CsrSchedQid CSR_BT_LE_SRV_IFACEQUEUE;
extern CsrSchedQid CSR_BT_LE_SVC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_LPM_IFACEQUEUE;
extern CsrSchedQid CSR_BT_TPM_IFACEQUEUE;
#ifndef EXCLUDE_CSR_BT_GGPROXY_MODULE
extern CsrSchedQid CSR_BT_GGPROXY_IFACEQUEUE;
#endif
#endif /* !CSR_TARGET_PLATFORM_VM */

extern CsrSchedQid CSR_BT_BAP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BAP_SERVER_IFACEQUEUE;

#ifndef EXCLUDE_CSR_BT_HIDS_SERVER_MODULE
extern CsrSchedQid CSR_BT_HIDS_SERVER_IFACEQUEUE;
#endif

extern CsrSchedQid CSR_BT_ASCS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_PACS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_GATT_CLIENT_UTIL_IFACEQUEUE;
extern CsrSchedQid CSR_BT_GATT_SRVC_DISC_IFACEQUEUE;
extern CsrSchedQid CSR_BT_VCP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_VCS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_VOCS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_AICS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_PACS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_VCS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_ASCS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MCS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MICS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MICP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MICS_CLIENT_IFACEQUEUE;



extern CsrSchedQid CSR_BT_TPS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_TDS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_TDS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_CHP_SEEKER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_CSIS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_CSIP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_TBS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BASS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_TBS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_CSIS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_BASS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MCS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_MCP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_CCP_IFACEQUEUE;
extern CsrSchedQid CSR_BT_CAP_CLIENT_IFACEQUEUE;

#ifdef INSTALL_CTM
extern CsrSchedQid CSR_BT_CTM_IFACEQUEUE;
#endif

extern CsrSchedQid BMM_IFACEQUEUE;

extern CsrSchedQid CSR_BT_TMAS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_TMAP_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_TMAS_CLIENT_IFACEQUEUE;

extern CsrSchedQid CSR_BT_PBP_IFACEQUEUE;


extern CsrSchedQid CSR_BT_GMAS_SERVER_IFACEQUEUE;
extern CsrSchedQid CSR_BT_GMAS_CLIENT_IFACEQUEUE;
extern CsrSchedQid CSR_BT_GMAP_CLIENT_IFACEQUEUE;

#ifdef ADK_GATT
typedef Task AppTask;
typedef TaskData AppTaskData;
typedef MessageId MsgId;
typedef Message Msg;
typedef gatt_status_t status_t;
#else
typedef CsrSchedQid AppTask;
typedef AppTask AppTaskData;
typedef CsrPrim MsgId;
typedef void* Msg;
typedef CsrBtResultCode status_t;
#endif


#ifdef __cplusplus
}
#endif

#endif
