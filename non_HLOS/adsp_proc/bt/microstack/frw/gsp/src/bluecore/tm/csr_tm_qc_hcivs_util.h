/*****************************************************************************
Copyright (c) 2016-2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary. 

REVISION:      $Revision: #1 $
*****************************************************************************/

#ifndef CSR_TM_QC_HCIVS_UTIL_H__
#define CSR_TM_QC_HCIVS_UTIL_H__

#include "csr_synergy.h"
#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

CsrUint8 *CsrHciCommandBuild(CsrUint16 opCode,
    CsrUint8 *payload,
    CsrUint8 payloadLength,
    CsrUint16 *qcmdLength);

CsrUint8 *CsrQcHciVsCommandBuild(CsrUint16 opCode,
    CsrUint8 *payload,
    CsrUint8 payloadLength,
    CsrUint16 *qcmdLength);

CsrUint8 *CsrQcHciVsDownloadCmdBuild(CsrUint16 opCode, 
    CsrUint8 reqCmd,
    CsrUint8 *payload,
    CsrUint8 payloadLength,
    CsrUint16 *qcmdLength);

void csrSendQcHciVsData(CsrUint8 *payload, CsrUint16 payloadLength);
CsrUint8 *csrGetQcBinData(CsrQcBin *binVar, CsrUint32 *length);

#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_QC_HCIVS_UTIL_H__ */

