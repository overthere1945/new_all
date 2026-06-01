/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#ifndef VSC_LIB_H
#define VSC_LIB_H

#include <vsdm_prim.h>
#include "csr_prim_defs.h"
#include "csr_bt_addr.h"
#include "hci_prim.h"
#include "csr_bt_profiles.h"
#include "csr_bt_util.h"
#include "csr_pmem.h"
#include <bluetooth.h>

#include "qbl_adapter_scheduler.h"

/* search_string="VscPrim" */
/* conversion_rule="UPPERCASE_START_AND_REMOVE_UNDERSCORES" */

typedef CsrPrim     VscPrim;
typedef CsrUint16   VscResult;
typedef CsrUint8    VscSourcetType;
typedef CsrUint8    VscPhyType;
typedef CsrUint8    VscScCodType;

#define VSC_STATUS_SUCCESS                         0x0000

#define VSC_RESULT_SUCCESS                         0x0000
#define VSC_RESULT_INVALID_PARAM                   0x0001
#define VSC_RESULT_INPROGRESS                      0x0002
#define VSC_RESULT_FAIL                            0x0003


#define VSC_QLM_SUPP_FET_SIZE                      (16)
#define VSC_QLL_SUPP_FET_SIZE                      (8)
#define VSC_MAX_NO_OF_COMPIDS                      (4)

#define VSC_PRIM_DOWN                              (VSDM_PRIM_BASE)
#define VSC_PRIM_UP                                (VSDM_PRIM_BASE | 0x0080)
#define VSC_PRIM_MAX                               (VSDM_PRIM_BASE | 0x00FF)

/* (VscPrim) is removed from those prims for which autogen of serializer/dissector is not applicable */

#define VSC_PRIM_DOWNSTREAM_HIGHEST                (ENUM_VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ)
#define VSC_PRIM_DOWNSTREAM_COUNT                  (VSC_PRIM_DOWNSTREAM_HIGHEST - VSC_PRIM_DOWN + 1)

#define VSC_PRIM_UPSTREAM_LOWEST                   (0x0080)

/* upstream primitives */
#define VSC_REGISTER_CFM                           ((VscPrim) 0x0000 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_READ_LOCAL_QLM_SUPP_FEATURES_CFM       ((VscPrim) 0x0001 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_READ_REMOTE_QLM_SUPP_FEATURES_CFM      ((VscPrim) 0x0002 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_WRITE_SC_HOST_SUPP_OVERRIDE_CFM        ((VscPrim) 0x0003 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_READ_SC_HOST_SUPP_OVERRIDE_CFM         ((VscPrim) 0x0004 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_QLM_CONNECTION_COMPLETE_IND            ((VscPrim) 0x0005 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_QCM_PHY_CHANGE_IND                     ((VscPrim) 0x0006 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM    ((VscPrim) 0x0007 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_READ_SC_HOST_SUPP_COD_OVERRIDE_CFM     ((VscPrim) 0x0008 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_SET_QHS_HOST_MODE_CFM                  ((VscPrim) 0x0009 + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_SET_WBM_FEATURES_CFM                   ((VscPrim) 0x000A + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_CONVERT_RPA_TO_IA_CFM                  ((VscPrim) 0x000B + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_INCOMING_PAGE_IND                      ((VscPrim) 0x000C + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM   ((VscPrim) 0x000D + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_SET_STREAMING_MODE_CFM                 ((VscPrim) 0x000E + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_QLL_CONNECTION_COMPLETE_IND            ((VscPrim) 0x000F + VSC_PRIM_UPSTREAM_LOWEST)
#define VSC_READ_REMOTE_QLL_SUPP_FEATURES_CFM      ((VscPrim) 0x0010 + VSC_PRIM_UPSTREAM_LOWEST)

#define VSC_HOUSE_CLEANING                         ((VscPrim) 0x0011 + VSC_PRIM_UPSTREAM_LOWEST)

#define VSC_PRIM_UPSTREAM_HIGHEST                  (0x0011 + VSC_PRIM_UPSTREAM_LOWEST)

/*
 * Type definition used to specify phy type.
 */

#define VSC_PHY_TYPE_BREDR       ((VscPhyType)0x00)
#define VSC_PHY_TYPE_QHS         ((VscPhyType)0x01)

/*
 * Type definition used to specify source type.
 */

#define VSC_SOURCE_TYPE_LOCAL   ((VscSourcetType)0x00)
#define VSC_SOURCE_TYPE_REMOTE  ((VscSourcetType)0x01)

/*
 * Type definition used to specify secure connections(SC) host support CoD
 * bit value.type.
 */

#define VSC_SC_HOST_SUPP_DISABLE_COD      ((VscScCodType)0x00)
#define VSC_SC_HOST_SUPP_ENABLE_COD       ((VscScCodType)0x01)

/*
 * Type definitions to specify the QHS host mode host is interested on
 * indicated transport.
 */
typedef CsrUint8 VscQhsTransport;

#define VSC_TRANSPORT_TYPE_BREDR        ((VscQhsTransport) 0x00)
#define VSC_TRANSPORT_TYPE_LE           ((VscQhsTransport) 0x01)
#define VSC_TRANSPORT_TYPE_LE_ISOC      ((VscQhsTransport) 0x02)

typedef CsrUint8 VscQhsHostMode;

/* For All transports */
#define VSC_QHS_HOST_MODE_DISABLED      ((VscQhsHostMode) 0x00)

/* Only for TRANSPORT_TYPE_BREDR */
#define VSC_QHS_HOST_MODE_BREDR_ENABLE  ((VscQhsHostMode) 0x01)

/* Only for TRANSPORT_TYPE_LE */
#define VSC_QHS_HOST_MODE_LE2M          ((VscQhsHostMode) 0x01)
#define VSC_QHS_HOST_MODE_LE1M_OR_LE2M  ((VscQhsHostMode) 0x02)

/* For All transports*/
#define VSC_QHS_HOST_MODE_HOST_AWARE    ((VscQhsHostMode) 0x03)

/* Type definitions to specify streaming mode */
#define VSC_STREAMING_MODE_NORMAL    ((uint8_t)0x00)
#define VSC_STREAMING_MODE_APTX      ((uint8_t)0x01)
#define VSC_STREAMING_MODE_GAMING    ((uint8_t)0x02)

typedef struct
{
    VscPrim type;
}VscHouseCleaning;

typedef struct
{
    VscPrim     type;       /* Always VSC_REGISTER_CFM */
    VscResult   result;     /* Result code - uses VSC_RESULT range */
}VscRegisterCfm;


typedef struct
{
    VscPrim     type;                                     /* Always VSC_READ_LOCAL_QLM_SUPP_FEATURES_CFM */
    hci_return_t status;                                  /* Status */
    uint8_t     qlmpSuppFeatures[VSC_QLM_SUPP_FET_SIZE];  /* QLMP supported features */
}VscReadLocalQlmSuppFeaturesCfm;

typedef struct
{
    VscPrim         type;                                      /* Always VSC_READ_REMOTE_QLM_SUPP_FEATURES_CFM */
    hci_return_t    status;                                    /* Status */
    CsrBdAddr       bdAddr;                                    /* Bluetooth device address */
    uint8_t         qlmpSuppFeatures[VSC_QLM_SUPP_FET_SIZE];   /* QLMP supported features */
}VscReadRemoteQlmSuppFeaturesCfm;

typedef struct
{
    VscPrim         type;                                      /* Always VSC_READ_REMOTE_QLL_SUPP_FEATURES_CFM */
    hci_return_t    status;                                    /* Status */
    CsrBtTypedAddr  tpAddrt;                                   /* Bluetooth device address */
    uint8_t         qllSuppFeatures[VSC_QLL_SUPP_FET_SIZE];    /* QLL supported features */
}VscReadRemoteQllSuppFeaturesCfm;

typedef struct
{
    VscPrim                 type;       /* Always VSC_QLM_CONNECTION_COMPLETE_IND */
    hci_return_t            status;     /* Status */
    hci_connection_handle_t handle;     /* QLM Connection handle */
    CsrBdAddr               bdAddr;     /* Bluetooth device address */
}VscQlmConnectionCompleteInd;

typedef struct
{
    VscPrim                 type;       /* Always VSC_QLL_CONNECTION_COMPLETE_IND */
    hci_return_t            status;     /* Status */
    hci_connection_handle_t handle;     /* QLL Connection handle */
    CsrBtTypedAddr          tpAddrt;    /* Bluetooth device address */
}VscQllConnectionCompleteInd;

typedef struct
{
   VscPrim                  type;       /* Always VSC_QCM_PHY_CHANGE_IND */
   hci_return_t             status;     /* Status */
   hci_connection_handle_t  handle;     /* QLM Connection handle */
   CsrBdAddr                bdAddr;     /* Bluetooth device address */
   VscPhyType               phy;        /* Type of phy, either BR/EDR or QHS */
   VscSourcetType           source;     /* Type of source, either local or remote */
}VscQcmPhyChangeInd;

typedef struct
{
   VscPrim      type;             /* Always VSC_INCOMING_PAGE_IND */
   hci_return_t status;           /* Status */
   CsrBdAddr    bdAddr;           /* Bluetooth device address */
   uint24_t     classOfDevice;    /* Class of device */
}VscIncomingPageInd;

typedef struct
{
    VscPrim      type;       /* Always VSC_WRITE_SC_HOST_SUPP_OVERRIDE_CFM */
    hci_return_t status;     /* Status */
}VscWriteScHostSuppOverrideCfm;

typedef struct
{
    VscPrim      type;                                      /* Always VSC_READ_SC_HOST_SUPP_OVERRIDE_CFM */
    hci_return_t status;                                    /* Status */
    uint8_t      numCompIDs;                                /* Number of compIDs */
    uint16_t     compID[VSC_MAX_NO_OF_COMPIDS];             /* compIDs to apply host mode override values */
    uint8_t      minLmpVersion[VSC_MAX_NO_OF_COMPIDS];      /* min_lmpVersion associated with compIDs */
    uint16_t     minLmpSubVersion[VSC_MAX_NO_OF_COMPIDS];   /* min_lmpSubVersion associated with compIDs */
}VscReadScHostSuppOverrideCfm;

typedef struct
{
    VscPrim      type;       /* Always VSC_WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM */
    hci_return_t status;     /* Status */
}VscWriteScHostSuppCodOverrideCfm;

typedef struct
{
    VscPrim         type;           /* Always VSC_READ_SC_HOST_SUPP_COD_OVERRIDE_CFM */
    hci_return_t    status;         /* Status */
    uint8_t         bitNumber;      /* Bit position in the Class of Device */
    VscScCodType    value;          /* Value of the bit_number */
}VscReadScHostSuppCodOverrideCfm;

typedef struct
{
    VscPrim      type;       /* Always VSC_SET_QHS_HOST_MODE_CFM */
    hci_return_t status;     /* Status */
}VscSetQhsHostModeCfm;

typedef struct
{
    VscPrim      type;       /* Always VSC_SET_WBM_FEATURES_CFM */
    hci_return_t status;     /* Status */
}VscSetWbmFeaturesCfm;

typedef struct
{
    VscPrim         type;               /* Always VSC_CONVERT_RPA_TO_IA_CFM */
    hci_return_t    status;             /* Status */
    CsrBtTypedAddr  identityAddress;    /* Resolved identity address if successful */
    uint8_t         priv_mode;          /* Privacy mode of address entry if present */
}VscConvertRpaToIaCfm;

typedef struct
{
    VscPrim      type;       /* Always VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM */
    hci_return_t status;     /* Status */
}VscWriteTruncatedPageScanEnableCfm;

typedef struct
{
    VscPrim      type;      /* Always VSC_SET_STREAMING_MODE_CFM */
    hci_return_t status;    /* Status */
    TP_BD_ADDR_T tp_addrt;  /* Address of the remote device */
}VscSetStreamingModeCfm;

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscRegister
 *
 *  DESCRIPTION
 *      Registers with VSC library to receive indications.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *----------------------------------------------------------------------------*/
void VscRegister(phandle_t phandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscDeregister
 *
 *  DESCRIPTION
 *      Deregister from receiving VSC indications.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *----------------------------------------------------------------------------*/
void VscDeregister(phandle_t phandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscRegisterReqSend
 *
 *  DESCRIPTION
 *      Registers with VSDM service by sending VSDM_REGISTER_REQ response from
 *      Bluestack handled within the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *----------------------------------------------------------------------------*/
void VscRegisterReqSend(phandle_t phandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadLocalQlmSuppFeaturesReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ, application is expected
 *      to receive VscReadLocalQlmSuppFeaturesCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *----------------------------------------------------------------------------*/
void VscReadLocalQlmSuppFeaturesReqSend(phandle_t phandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadRemoteQlmSuppFeaturesReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ, application is expected to
 *      receive VscReadRemoteQlmSuppFeaturesCfm from the library
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        handle:             connection handle
 *        bdaddr:             bluetooth device address
 *----------------------------------------------------------------------------*/
void VscReadRemoteQlmSuppFeaturesReqSend(phandle_t phandle, CsrBdAddr bdaddr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscWriteScHostSuppOverrideReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ, application is expected to
 *      receive VscWriteScHostSuppOverrideCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        numCompIds:         number of compIDs
 *        compId:             compIDs to apply host mode override values
 *        minLmpVersion:      min_lmpVersion associated with compIDs
 *        minLmpSubVersion:   min_lmpSubVersion associated with compIDs
 *----------------------------------------------------------------------------*/
void VscWriteScHostSuppOverrideReqSend(phandle_t phandle, uint8_t numCompIds, uint16_t compId[], uint8_t minLmpVersion[], uint16_t minLmpSubVersion[]);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadScHostSuppOverrideReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ, application is expected to
 *      receive VscReadScHostSuppOverrideCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *----------------------------------------------------------------------------*/
void VscReadScHostSuppOverrideReqSend(phandle_t phandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscWriteScHostSuppCodOverrideReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_WRITE_SC_HOST_COD_OVERRIDE_REQ, application is expected to
 *      receive VscWriteScHostSuppCodOverrideCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        bitNumber:          bit position in the class of device(0 to 23)
 *        enable:             enable or disable SC based on class of device
 *----------------------------------------------------------------------------*/
void VscWriteScHostSuppCodOverrideReqSend(phandle_t phandle, uint8_t bitNumber, VscScCodType enable);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadScHostSuppCodOverrideReqSend
 *
 *  DESCRIPTION
 *      Sends VSD_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ, application is expected to
 *      receive VscReadScHostSuppCodOverrideCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *----------------------------------------------------------------------------*/
void VscReadScHostSuppCodOverrideReqSend(phandle_t phandle);


/*----------------------------------------------------------------------------*
 *  NAME
 *      VscSetQhsHostModeReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_SET_QHS_HOST_MODE_REQ, application is expected to
 *      receive VscSetQhsHostModeCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        transport:          transport type
 *        qhdHostMode:        QHS mode type host want to set
 *----------------------------------------------------------------------------*/
void VscSetQhsHostModeReqSend(phandle_t phandle, VscQhsTransport transport, VscQhsHostMode qhsHostMode);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscSetWbmFeaturesReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_SET_WBM_FEATURES_REQ, application is expected to
 *      receive VscSetWbmFeaturesCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        enableMask:         enable or disable WBM features
 *----------------------------------------------------------------------------*/
void VscSetWbmFeaturesReqSend(phandle_t phandle, hci_connection_handle_t conn_handle, enable_bit_mask_t enableMask);


/*----------------------------------------------------------------------------*
 *  NAME
 *      VscConvertRpaToIaReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_CONVERT_RPA_TO_IA_REQ, application is expected to
 *      receive VscConvertRpaToIaCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        rpa:                resolvable private address
 *----------------------------------------------------------------------------*/
void VscConvertRpaToIaReqSend(phandle_t phandle, CsrBdAddr rpa);


/*----------------------------------------------------------------------------*
 *  NAME
 *      VscWriteTruncatedPageScanEnableReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ, application is expected to
 *      receive VscWriteTruncatedPageScanEnableCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        enable:             enable truncated page scan
 *----------------------------------------------------------------------------*/
void VscWriteTruncatedPageScanEnableReqSend(phandle_t phandle, uint8_t enable);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscSetStreamingModeReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_SET_STREAMING_MODE_REQ, application is expected to
 *      receive VscSetStreamingModeCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        tp_addrt:           address of remote device
 *        streaming_mode:     the type of streaming mode to be set
 *----------------------------------------------------------------------------*/
void VscSetStreamingModeReqSend(phandle_t phandle, TP_BD_ADDR_T *tp_addrt, uint8_t streaming_mode);

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadRemoteQllSuppFeaturesReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ, application is expected to
 *      receive VscReadRemoteQllSuppFeaturesCfm from the library
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        handle:             connection handle
 *        tpAddrt:            address of remote device
 *----------------------------------------------------------------------------*/
void VscReadRemoteQllSuppFeaturesReqSend(phandle_t phandle, CsrBtTypedAddr tpAddrt);


typedef union
{
    /* Shared */
    VscPrim                                 type;

    /* Upstream */
    VscRegisterCfm                          vsc_register_cfm;
    VscReadLocalQlmSuppFeaturesCfm          vsc_read_local_qlm_supp_features_cfm;
    VscReadRemoteQlmSuppFeaturesCfm         vsc_read_remote_qlm_supp_features_cfm;
    VscWriteScHostSuppOverrideCfm           vsc_write_sc_host_supp_override_cfm;
    VscReadScHostSuppOverrideCfm            vsc_read_sc_host_supp_override_cfm;
    VscQlmConnectionCompleteInd             vsc_qlm_connection_complete_ind;
    VscQcmPhyChangeInd                      vsc_qcm_phy_change_ind;
    VscWriteScHostSuppCodOverrideCfm        vsc_write_sc_host_supp_cod_override_cfm;
    VscReadScHostSuppCodOverrideCfm         vsc_read_sc_host_supp_cod_override_cfm;
    VscSetQhsHostModeCfm                    vsc_set_qhs_host_mode_cfm;
    VscSetWbmFeaturesCfm                    vsc_set_wbm_features_cfm;
    VscConvertRpaToIaCfm                    vsc_convert_rpa_to_ia_cfm;
    VscIncomingPageInd                      vsc_incoming_page_ind;
    VscWriteTruncatedPageScanEnableCfm      vsc_write_incoming_page_scan_enable_cfm;
    VscSetStreamingModeCfm                  vsc_set_streaming_mode_cfm;
    VscQllConnectionCompleteInd             vsc_qll_connection_complete_ind;
    VscReadRemoteQllSuppFeaturesCfm         vsc_read_remote_qll_supp_features_cfm;
    VscHouseCleaning                        vsc_house_cleaning;
} VscUprim;

#endif /* VSC_LIB_H */
