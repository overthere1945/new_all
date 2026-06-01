/******************************************************************************
 Copyright (c) 2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CSR_QVSC_PRIVATE_PRIM_H_
#define CSR_QVSC_PRIVATE_PRIM_H_

#include "csr_sched.h"
#include "csr_types.h"
#include "csr_qvsc_prim.h"
#include "csr_result.h"

#ifdef __cplusplus
extern "C" {
#endif


/* search_string="CsrQvscPrim" */
/* conversion_rule="UPPERCASE_START_AND_REMOVE_UNDERSCORES" */

#define CSR_QVSC_RESULT_SUCCESS                     ((CsrResult) 0)
#define CSR_QVSC_RESULT_TIMEOUT                     ((CsrResult) 1)
#define CSR_QVSC_RESULT_MISC                        ((CsrResult) 2)
#define CSR_QVSC_RESULT_HCI_STATUS_MIN              0x0100

/* Downstream */
#define CSR_QVSC_TLV_DOWNLOAD_REQ                   ((CsrQvscPrim) (0x0001 + CSR_QVSC_PRIM_DOWNSTREAM_HIGHEST))
#define CSR_QVSC_KCS_DOWNLOAD_REQ                   ((CsrQvscPrim) (0x0002 + CSR_QVSC_PRIM_DOWNSTREAM_HIGHEST))
#define CSR_QVSC_KCS_REMOVE_REQ                     ((CsrQvscPrim) (0x0003 + CSR_QVSC_PRIM_DOWNSTREAM_HIGHEST))

#define CSR_QVSC_PRIVATE_PRIM_DOWNSTREAM_HIGHEST    (0x0003 + CSR_QVSC_PRIM_DOWNSTREAM_HIGHEST)
#define CSR_QVSC_PRIVATE_PRIM_DOWNSTREAM_COUNT      (CSR_QVSC_PRIVATE_PRIM_DOWNSTREAM_HIGHEST - CSR_QVSC_PRIM_DOWNSTREAM_HIGHEST)
#define CSR_QVSC_ALL_PRIM_DOWNSTREAM_COUNT          (CSR_QVSC_PRIM_DOWNSTREAM_COUNT + CSR_QVSC_PRIVATE_PRIM_DOWNSTREAM_COUNT)

/* Upstream */
#define CSR_QVSC_TLV_DOWNLOAD_CFM                   ((CsrQvscPrim) (0x0001 + CSR_QVSC_PRIM_UPSTREAM_HIGHEST))
#define CSR_QVSC_KCS_DOWNLOAD_CFM                   ((CsrQvscPrim) (0x0002 + CSR_QVSC_PRIM_UPSTREAM_HIGHEST))
#define CSR_QVSC_KCS_REMOVE_CFM                     ((CsrQvscPrim) (0x0003 + CSR_QVSC_PRIM_UPSTREAM_HIGHEST))

#define CSR_QVSC_PRIVATE_PRIM_UPSTREAM_HIGHEST      (0x0003 + CSR_QVSC_PRIM_UPSTREAM_HIGHEST)
#define CSR_QVSC_PRIVATE_PRIM_UPSTREAM_COUNT        (CSR_QVSC_PRIVATE_PRIM_UPSTREAM_HIGHEST - CSR_QVSC_PRIM_UPSTREAM_HIGHEST)
#define CSR_QVSC_ALL_PRIM_UPSTREAM_COUNT            (CSR_QVSC_PRIM_UPSTREAM_COUNT + CSR_QVSC_PRIVATE_PRIM_UPSTREAM_COUNT)

#define CSR_QVSC_PRIVATE_PRIM                       CSR_QVSC_PRIM
/* ---------- End of Primitives ----------*/

typedef CsrUint8 CsrQvscTlvDnldConfig;
/* Download response configurations */
#define CSR_QVSC_TLV_DOWNLOAD_CONFIG_DEFAULT        ((CsrQvscTlvDnldConfig) 0)   /* Both vendor specific events and command complete are received */
#define CSR_QVSC_TLV_DOWNLOAD_CFG_BIT_SKIP_CC       1   /* Command complete are not received */
#define CSR_QVSC_TLV_DOWNLOAD_CFG_BIT_SKIP_VS       2   /* Vendor specific events are not received */

#define CSR_QVSC_KCS_BUNDLE_ID_INVALID              0

typedef struct
{
    CsrQvscPrim             type;                       /* Primitive/message identity */
    CsrSchedQid             phandle;                    /* Application handle */
    CsrQvscTlvDnldConfig    config;
    CsrUint16               payloadLength;
    CsrUint8               *payload;
} CsrQvscTlvDownloadReq;

typedef struct
{
    CsrQvscPrim             type;                       /* Primitive/message identity */
    CsrResult               result;
} CsrQvscTlvDownloadCfm;

typedef struct
{
    CsrQvscPrim             type;                       /* Primitive/message identity */
    CsrSchedQid             phandle;                    /* Application handle */
    CsrQvscTlvDnldConfig    config;
    CsrUint16               processorId;
    CsrUint16               payloadLength;
    CsrUint8               *payload;
} CsrQvscKcsDownloadReq;

typedef struct
{
    CsrQvscPrim             type;                       /* Primitive/message identity */
    CsrUint16               kcsBundleId;
    CsrResult               result;
} CsrQvscKcsDownloadCfm;

typedef struct
{
    CsrQvscPrim             type;                       /* Primitive/message identity */
    CsrSchedQid             phandle;                    /* Application handle */
    CsrUint16               kcsBundleId;
} CsrQvscKcsRemoveReq;

typedef struct
{
    CsrQvscPrim             type;                       /* Primitive/message identity */
    CsrResult               result;
} CsrQvscKcsRemoveCfm;

#ifdef __cplusplus
}
#endif

#endif /* CSR_QVSC_PRIVATE_PRIM_H_ */

