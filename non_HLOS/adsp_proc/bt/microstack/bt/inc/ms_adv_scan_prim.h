

/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #01 $
******************************************************************************/
#ifndef MS_ADV_SCAN_PRIM_H__
#define MS_ADV_SCAN_PRIM_H__

#include "csr_synergy.h"
#include "csr_sched.h"
#include "csr_bt_profiles.h"
#include "csr_types.h"
#include "csr_bt_util.h"
#include "csr_bt_addr.h"
#include "rfcomm_prim.h"
#if defined(HYDRA) || defined(CAA)
#include "hci.h"
#else
#include "hci_prim.h"
#endif
#include "l2cap_prim.h"

#include "dm_prim.h"
#include "csr_bt_result.h"
#include "csr_mblk.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef CsrPrim CsrBtMsPrim;

/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/
#define MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST              (0x0000)
#define MS_EXT_ADV_SCAN_HOUSE_CLEANING_REQ              ((CsrBtMsPrim) (0x0001 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_ADV_REGISTER_APP_ADV_SET_REQ             ((CsrBtMsPrim) (0x0002 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ           ((CsrBtMsPrim) (0x0003 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_ADV_SET_PARAMS_REQ                       ((CsrBtMsPrim) (0x0004 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_ADV_SET_DATA_REQ                         ((CsrBtMsPrim) (0x0005 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_ADV_SET_SCAN_RESP_DATA_REQ               ((CsrBtMsPrim) (0x0006 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ            ((CsrBtMsPrim) (0x0007 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_ADV_SET_RANDOM_ADDR_REQ                  ((CsrBtMsPrim) (0x0008 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_ADV_SETS_INFO_REQ                        ((CsrBtMsPrim) (0x0009 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_ADV_MULTI_ENABLE_REQ                     ((CsrBtMsPrim) (0x000A + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))

#define MS_EXT_SCAN_GET_GLOBAL_PARAMS_REQ               ((CsrBtMsPrim) (0x000B + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_SCAN_SET_GLOBAL_PARAMS_REQ               ((CsrBtMsPrim) (0x000C + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_SCAN_REGISTER_SCANNER_REQ                ((CsrBtMsPrim) (0x000D + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_SCAN_UNREGISTER_SCANNER_REQ              ((CsrBtMsPrim) (0x000E + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_SCAN_ENABLE_SCANNERS_REQ                 ((CsrBtMsPrim) (0x000F + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))
#define MS_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ              ((CsrBtMsPrim) (0x0010 + MS_PRIM_ADV_SCAN_DOWNSTREAM_LOWEST))

#define MS_PRIM_ADV_SCAN_DOWNSTREAM_HIGHEST             ((CsrBtMsPrim) (MS_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ))
/* **** */

/* **** */
#define MS_SEND_PRIM_UPSTREAM_LOWEST                   (0x0100 + CSR_PRIM_UPSTREAM)
#define MS_EXT_ADV_REGISTER_APP_ADV_SET_CFM            ((CsrBtMsPrim) (0x0001 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM          ((CsrBtMsPrim) (0x0002 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_ADV_SET_PARAMS_CFM                      ((CsrBtMsPrim) (0x0003 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_ADV_SET_DATA_CFM                        ((CsrBtMsPrim) (0x0004 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_ADV_SET_SCAN_RESP_DATA_CFM              ((CsrBtMsPrim) (0x0005 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM           ((CsrBtMsPrim) (0x0006 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_ADV_SET_RANDOM_ADDR_CFM                 ((CsrBtMsPrim) (0x0007 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_ADV_SETS_INFO_CFM                       ((CsrBtMsPrim) (0x0008 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_ADV_MULTI_ENABLE_CFM                    ((CsrBtMsPrim) (0x0009 + MS_SEND_PRIM_UPSTREAM_LOWEST))

#define MS_EXT_SCAN_GET_GLOBAL_PARAMS_CFM              ((CsrBtMsPrim) (0x000A + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_SCAN_SET_GLOBAL_PARAMS_CFM              ((CsrBtMsPrim) (0x000B + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_SCAN_REGISTER_SCANNER_CFM               ((CsrBtMsPrim) (0x000C + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_SCAN_UNREGISTER_SCANNER_CFM             ((CsrBtMsPrim) (0x000D + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_SCAN_ENABLE_SCANNERS_CFM                ((CsrBtMsPrim) (0x000E + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM             ((CsrBtMsPrim) (0x000F + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_SCAN_CTRL_SCAN_INFO_IND                 ((CsrBtMsPrim) (0x0010 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_SCAN_FILTERED_ADV_REPORT_IND            ((CsrBtMsPrim) (0x0011 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_SCAN_DURATION_EXPIRED_IND               ((CsrBtMsPrim) (0x0012 + MS_SEND_PRIM_UPSTREAM_LOWEST))
#define MS_EXT_ADV_TERMINATED_IND                      ((CsrBtMsPrim) (0x0013 + MS_SEND_PRIM_UPSTREAM_LOWEST))

#define MS_SEND_PRIM_UPSTREAM_HIGHEST                  ((CsrBtMsPrim) (MS_EXT_ADV_TERMINATED_IND))

/*******************************************************************************
 * End primitive definitions
 *******************************************************************************/


/*************************************************************************************
 Primitive typedefs
************************************************************************************/
typedef HCI_ULP_EXT_ADV_SET_DATA_T MS_HCI_ULP_EXT_ADV_SET_DATA_REQ_T;
typedef HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_T MS_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ_T;


/* The max length of extended advertising data */
#define MS_MAX_POTENTIAL_ADV_DATA_LEN 1650

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: MS_EXT_ADV_REGISTER_APP_ADV_SET_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
} MsExtAdvRegisterAppAdvSetReq;


typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: MS_EXT_ADV_REGISTER_APP_ADV_SET_CFM */
    CsrBtResultCode             resultCode;      /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                    advHandle;
} MsExtAdvRegisterAppAdvSetCfm;


typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: MS_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
} MsExtAdvUnregisterAppAdvSetReq;

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: MS_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM */
    CsrBtResultCode             resultCode;     /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                    advHandle;
} MsExtAdvUnregisterAppAdvSetCfm;

typedef CsrUint32                   MsExtAdvSetParamsMask;


typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_PARAMS_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
    CsrUint16                   advEventProperties;
    CsrUint32                   primaryAdvIntervalMin;
    CsrUint32                   primaryAdvIntervalMax;
    CsrUint8                    primaryAdvChannelMap;
    CsrUint8                    ownAddrType;
    TYPED_BD_ADDR_T             peerAddr;
    CsrUint8                    advFilterPolicy;
    CsrUint16                   primaryAdvPhy;
    CsrUint8                    secondaryAdvMaxSkip;
    CsrUint16                   secondaryAdvPhy;
    CsrUint16                   advSid;
    CsrInt8                     advTxPower;
    CsrUint8                    scanReqNotifyEnable;         /* Reserved for future use */
    CsrUint8                    primaryAdvPhyOptions;        /* Reserved for future use */
    CsrUint8                    secondaryAdvPhyOptions;      /* Reserved for future use */
    MsExtAdvSetParamsMask       featureMask;
} MsExtAdvSetParamsReq;

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_PARAMS_CFM */
    CsrBtResultCode             resultCode;     /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                    advHandle;
    int8_t                      selected_tx_pwr;
} MsExtAdvSetParamsCfm;

#define MS_EXT_ADV_DATA_LENGTH_MAX       251
#define MS_EXT_ADV_DATA_BLOCK_SIZE       32
#define MS_EXT_ADV_DATA_BYTES_PTRS_MAX   8

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_DATA_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
    CsrUint8                    operation;
    CsrUint8                    fragPreference;
    CsrUint8                    dataLength;
    CsrUint8                    *data[MS_EXT_ADV_DATA_BYTES_PTRS_MAX];
} MsExtAdvSetDataReq;

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_DATA_CFM */
    CsrBtResultCode             resultCode;     /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                    advHandle;
} MsExtAdvSetDataCfm;


#define MS_EXT_ADV_SCAN_RESP_DATA_LENGTH_MAX          251
#define MS_EXT_ADV_SCAN_RESP_DATA_BLOCK_SIZE          32
#define MS_EXT_ADV_SCAN_RESP_DATA_BYTES_PTRS_MAX      8

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
    CsrUint8                    operation;
    CsrUint8                    fragPreference;
    CsrUint8                    dataLength;
    CsrUint8                    *data[MS_EXT_ADV_SCAN_RESP_DATA_BYTES_PTRS_MAX];
} MsExtAdvSetScanRespDataReq;

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM */
    CsrBtResultCode             resultCode;     /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                    advHandle;
} MsExtAdvSetScanRespDataCfm;

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CM_DM_EXT_ADV_GET_ADDR_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
} MsExtAdvGetAddrReq;

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CM_DM_EXT_ADV_GET_ADDR_CFM */
    CsrBtResultCode             resultCode;     /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                    advHandle;
    TYPED_BD_ADDR_T             ownAddr;
} MsExtAdvGetAddrCfm;


typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
} MsExtAdvReadMaxAdvDataLenReq;


typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM */
    CsrBtResultCode             resultCode;     /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint16                   maxAdvDataLen;
} MsExtAdvReadMaxAdvDataLenCfm;


/* Options for generating or setting a random address for an advertising set.
   Used to set field action. */
#define MS_EXT_ADV_ADDRESS_WRITE_STATIC 0               /* Set static address */
#define MS_EXT_ADV_ADDRESS_WRITE_NON_RESOLVABLE 1       /* Set NRPA */
#define MS_EXT_ADV_ADDRESS_WRITE_RESOLVABLE 2           /* Set RPA */

typedef struct
{
    CsrBtMsPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_REQ */
    CsrSchedQid                 appHandle;     /* Destination phandle */
    CsrUint8                    advHandle;     /* Advertising set */
    CsrUint16                   action;        /* How to set random address */
    CsrBtDeviceAddr             randomAddr;    /* Random address to write */
} MsExtAdvSetRandomAddrReq;


typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM */
    CsrBtResultCode             resultCode;     /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                    advHandle;
    CsrBtDeviceAddr             randomAddr;
} MsExtAdvSetRandomAddrCfm;


/* Reasons why an advertising set has stopped advertising */
#define MS_EXT_ADV_TERMINATED_CONN          0   /* Connection established on adv set */
#define MS_EXT_ADV_TERMINATED_DURATION      1   /* Duration period has ended */
#define MS_EXT_ADV_TERMINATED_MAX_EVENT     2   /* Max event limit reached */

typedef struct
{
    CsrBtMsPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_TERMINATED_IND */
    CsrUint8                    advHandle;       /* Adv set terminated */
    CsrUint8                    reason;          /* Why it was terminated */
    CsrUint8                    eaEvents;        /* Completed extended advertising events */
    CsrUint8                    maxAdvSets;      /* Number of adv sets below */
    CsrUint32                   advBits;
} MsExtAdvTerminatedInd;


typedef struct
{
    CsrBtMsPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_SETS_INFO_REQ */
    CsrSchedQid                 appHandle;
} MsExtAdvSetsInfoReq;

typedef struct
{
    CsrUint8                    registered;
    CsrUint8                    advertising;
    CsrUint16                   info;
    uint8_t                     advHandle;
} MsExtAdvSetInfo;

typedef struct
{
    CsrBtMsPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_SETS_INFO_CFM */
    CsrUint16                   flags;
    CsrUint8                    numAdvSets;
    MsExtAdvSetInfo             advSets[MS_EXT_ADV_MAX_ADV_HANDLES];
} MsExtAdvSetsInfoCfm;

#define MS_EXT_ADV_N0_DURATION      0
#define MS_EXT_ADV_NO_MAX_EVENTS    0

typedef struct
{
    CsrUint8                    advHandle;
    CsrUint8                    maxEaEvents;
    CsrUint16                   duration;
} AdvEnableConfig;

typedef struct
{
    CsrBtMsPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_MULTI_ENABLE_REQ */
    CsrSchedQid                 appHandle;
    CsrUint8                    enable;        /* Start/stop advertising */
    CsrUint8                    numSets;       /* How many advertising set configs in prim */
    AdvEnableConfig              config[MS_EXT_ADV_MAX_ADV_HANDLES];
} MsExtAdvMultiEnableReq;



typedef struct
{
    CsrBtMsPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_MULTI_ENABLE_CFM */
    CsrBtResultCode             resultCode;     /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                    maxAdvSets;
    CsrUint32                   advBits;
} MsExtAdvMultiEnableCfm;

/* primitive type */
typedef uint16_t                 adv_scan_prim_t;


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_GET_GLOBAL_PARAMS_REQ */
    CsrSchedQid                appHandle;
} MsExtScanGetGlobalParamsReq;


typedef ES_SCANNING_PHY_T MsScanningPhy;

#define MS_SCAN_MAX_SCANNING_PHYS HCI_ULP_MAX_SCANNING_PHYS
#define MS_SCAN_LE_1M_PHY_BIT         0x00
#define MS_SCAN_LE_2M_PHY_BIT         0x01
#define MS_SCAN_LE_CODED_PHY_BIT      0x02

#define MS_SCAN_LE_1M_PHY_BIT_MASK    0x01
#define MS_SCAN_LE_2M_PHY_BIT_MASK    0x02
#define MS_SCAN_LE_CODED_PHY_BIT_MASK 0x04

typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_GET_GLOBAL_PARAMS_CFM */
    CsrBtResultCode            resultCode;      /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                   flags;
    CsrUint8                   ownAddressType;
    CsrUint8                   scanningFilterPolicy;
    CsrUint8                   filterDuplicates;
    CsrUint16                  scanningPhys;
    MsScanningPhy              phys[MS_SCAN_MAX_SCANNING_PHYS];
} MsExtScanGetGlobalParamsCfm;

typedef MsExtScanGetGlobalParamsCfm CmExtScanGetGlobalParamsCfm;

typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_SET_GLOBAL_PARAMS_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   flags;
    CsrUint8                   ownAddressType;
    CsrUint8                   scanningFilterPolicy;
    CsrUint8                   filterDuplicates;
    CsrUint16                  scanningPhy;
    MsScanningPhy              phys[MS_SCAN_MAX_SCANNING_PHYS];
} MsExtScanSetGlobalParamsReq;

typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_SET_GLOBAL_PARAMS_CFM */
    CsrBtResultCode            resultCode;      /* Result code - HCI Success in case of success and HCI error codes in case of failure */
} MsExtScanSetGlobalParamsCfm;


#define MS_SCAN_MAX_REG_AD_TYPES 10

/* Valid values for advFilter field */
#define MS_SCAN_ADV_FILTER_NONE 0
#define MS_SCAN_ADV_FILTER_BLOCK_ALL 1
#define MS_SCAN_ADV_FILTER_LEGACY 2
#define MS_SCAN_ADV_FILTER_EXTENDED 3
#define MS_SCAN_ADV_FILTER_ASSOCIATED_PERIODIC 4

/* Valid values for adStructureFilter field */
#define MS_SCAN_AD_STRUCT_FILTER_NONE 0

/* Sub fields setting for advFilter or adStructureFilter when not used */
#define MS_SCAN_SUB_FIELD_INVALID 0


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_REGISTER_SCANNER_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   flags;
    CsrUint16                  advFilter;
    CsrUint16                  advFilterSubField1;
    CsrUint32                  advFilterSubField2;
    CsrUint16                  adStructureFilter;
    CsrUint16                  adStructureFilterSubField1;
    CsrUint32                  adStructureFilterSubField2;
    CsrUint8                   numRegAdTypes; /* RFU */
    CsrUint8                   regAdTypes[MS_SCAN_MAX_REG_AD_TYPES]; /* RFU */
} MsExtScanRegisterScannerReq;


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_REGISTER_SCANNER_CFM */
    CsrBtResultCode            resultCode;      /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                   scanHandle;
} MsExtScanRegisterScannerCfm;


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_UNREGISTER_SCANNER_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   scanHandle;
} MsExtScanUnregisterScannerReq;


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_UNREGISTER_SCANNER_CFM */
    CsrSchedQid                appHandle;
    CsrBtResultCode            resultCode;  /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                   scanHandle;
} MsExtScanUnregisterScannerCfm;

typedef struct
{
    CsrUint8         scanHandle;
    CsrUint16        duration;
} MsScanners;

#define MS_SCAN_MAX_SCANNERS 5
#define MS_SCAN_FOREVER 0


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_ENABLE_SCANNERS_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   enable;
    CsrUint8                   numOfScanners;
    MsScanners                 scanners[MS_SCAN_MAX_SCANNERS];
} MsExtScanEnableScannersReq;


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_ENABLE_SCANNERS_CFM */
    CsrBtResultCode            resultCode;  /* Result code - HCI Success in case of success and HCI error codes in case of failure */
} MsExtScanEnableScannersCfm;


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ */
    CsrSchedQid                appHandle;
} MsExtScanGetCtrlScanInfoReq;

typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_TIMEOUT_IND */
} MsExtScanTimeoutInd;


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM */
    CsrBtResultCode            resultCode;      /* Result code - HCI Success in case of success and HCI error codes in case of failure */
    CsrUint8                   numOfEnabledScanners;
    CsrUint16                  duration;
    CsrUint16                  scanningPhys;
    MsScanningPhy              phys[MS_SCAN_MAX_SCANNING_PHYS];
} MsExtScanGetCtrlScanInfoCfm;


/* Reason for Controller Scan Info Indication */
#define MS_SCAN_REASON_GLOBALS_CHANGED   0
#define MS_SCAN_REASON_SCANNERS_ENABLED  1
#define MS_SCAN_REASON_SCANNERS_DISABLED 2

typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_CTRL_SCAN_INFO_IND */
    CsrUint8                   reason;
    CsrUint8                   controllerUpdated;
    CsrUint8                   numOfEnabledScanners;
    CsrUint8                   legacyScannerEnabled;
    CsrUint16                  duration;
    CsrUint16                  scanningPhys;
    MsScanningPhy              phys[MS_SCAN_MAX_SCANNING_PHYS];
} MsExtScanCtrlScanInfoInd;


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_FILTERED_ADV_REPORT_IND */
    CsrUint16                  eventType;    
    CsrUint16                  primaryPhy;
    CsrUint16                  secondaryPhy;
    CsrUint8                   advSid;
    TYPED_BD_ADDR_T            currentAddrt;
    TYPED_BD_ADDR_T            permanentAddrt;
    TYPED_BD_ADDR_T            directAddrt;
    CsrInt8                    txPower;
    CsrInt8                    rssi;
    CsrUint16                  periodicAdvInterval;
    CsrUint8                   advDataInfo;
    CsrUint8                   adFlags;
    CsrUint16                  dataLength;
    CsrUint8                   *data;
} MsExtScanFilteredAdvReportInd;


typedef struct
{
    CsrBtMsPrim                type;          /* Identity: MS_EXT_SCAN_DURATION_EXPIRED_IND */
    CsrUint8                   scanHandle;    
    CsrUint8                   scanHandleUnregistered;
} MsExtScanDurationExpiredInd;


/*------------------------------------------------------------------------
 *
 *      UNION OF       PRIMITIVES
 *
 *-----------------------------------------------------------------------*/
typedef union
{
    adv_scan_prim_t                                         type;
    MsExtAdvRegisterAppAdvSetReq                            msExtAdvRegisterAdvAppSetReq;
    MsExtAdvRegisterAppAdvSetCfm                            msExtAdvRegisterAdvAppSetCfm;
    MsExtAdvUnregisterAppAdvSetReq                          msExtAdvUnregisterAdvAppSetReq;
    MsExtAdvUnregisterAppAdvSetCfm                          msExtAdvUnregisterAdvAppSetCfm;
    MsExtAdvSetParamsReq                                    msExtAdvSetParamsReq;
    MsExtAdvSetParamsCfm                                    msExtAdvSetParamsCfm;
    MsExtAdvSetDataReq                                      msExtAdvSetDataReq;
    MsExtAdvSetDataCfm                                      msExtAdvSetDataCfm;
    MsExtAdvSetScanRespDataReq                              msExtAdvSetScanRespDataReq;
    MsExtAdvSetScanRespDataCfm                              msExtAdvSetScanRespDataCfm;
    MsExtAdvSetsInfoReq                                     msExtAdvSetsInfoReq;
    MsExtAdvSetsInfoCfm                                     msExtAdvSetsInfoCfm;
    MsExtAdvMultiEnableReq                                  msExtAdvMultiEnableReq;
    MsExtAdvMultiEnableCfm                                  msExtAdvMultiEnableCfm;
    MsExtAdvGetAddrReq                                      msExtAdvGetAddrReq;
    MsExtAdvGetAddrCfm                                      msExtAdvGetAddrCfm;
    MsExtAdvReadMaxAdvDataLenReq                            msExtAdvReadMaxAdvDataLenReq;
    MsExtAdvReadMaxAdvDataLenCfm                            msExtAdvReadMaxAdvDataLenCfm;
    MsExtAdvSetRandomAddrReq                                msExtAdvSetRandomAddrReq;
    MsExtAdvSetRandomAddrCfm                                msExtAdvSetRandomAddrCfm;
    MsExtAdvTerminatedInd                                   msExtAdvTerminateInd;
}ADV_SCAN_PRIM_T;

#ifdef __cplusplus
}
#endif

#endif /* ifndef MS_ADV_SCAN_PRIM_H__ */
