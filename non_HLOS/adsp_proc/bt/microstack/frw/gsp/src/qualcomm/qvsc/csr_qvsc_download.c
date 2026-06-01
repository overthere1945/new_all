/******************************************************************************
 Copyright (c) 2017-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
 ******************************************************************************/

#include "csr_qvsc_download.h"

#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_result.h"
#include "csr_qvsc_private_prim.h"
#include "csr_qvsc_handler.h"


/* Sends next TLV segment from TLV data. Returns number of bytes consumed. */
CsrUint8 CsrQvscTlvSegmentSend(CsrQvscTlvData *downloadInst)
{
    CsrUint8 tlvSegmentLength = 0;

    if (downloadInst->index < downloadInst->length)
    {
        CsrUint8 *body;
        CsrUint8 bodyLength;
        CsrUint16 remainingLength = downloadInst->length - downloadInst->index;

        tlvSegmentLength = (CsrUint8) CSRMIN(remainingLength, CSR_QVSC_TLV_SEGMENT_LENGTH_MAX);

        bodyLength = tlvSegmentLength + sizeof(CsrQvsSubOpcode) + sizeof(tlvSegmentLength);

        body = CsrPmemAlloc(bodyLength);

        body[0] = CSR_QVS_TLV_DOWNLOAD_REQ;
        body[1] = tlvSegmentLength;

        SynMemCpyS(&body[2], tlvSegmentLength, &downloadInst->payload[downloadInst->index], tlvSegmentLength);
        downloadInst->index += tlvSegmentLength;

        CsrQvscSendCommand(CSR_QVS_OCF_DOWNLOAD_COMMAND, body, bodyLength);
    }

    return (tlvSegmentLength);
}

/* Starts TLV download procedure */
void CsrQvscTlvDownloadInitiate(CsrQvscTlvData *downloadInst)
{
    if (CSR_MASK_IS_SET(downloadInst->config,
                        CSR_QVSC_TLV_DOWNLOAD_CFG_BIT_SKIP_CC) &&
        CSR_MASK_IS_SET(downloadInst->config,
                        CSR_QVSC_TLV_DOWNLOAD_CFG_BIT_SKIP_VS))
    {
        /* Responses are disabled, send all data back to back. */
        while (CsrQvscTlvSegmentSend(downloadInst));
    }
    else
    {
        /* Send one segment at a time, wait for response */
        CsrQvscTlvSegmentSend(downloadInst);
    }
}

/* Closes TLV download procedure */
void CsrQvscTlvDownloadComplete(CsrQvscInstanceData *qvscInst, CsrResult result)
{
    CsrQvscTlvDownloadCfmSend(qvscInst->phandle, result);
    CsrQvscRestore(qvscInst);
}

#ifdef CSR_DSPM_KCS_DOWNLOAD
/* Closes KCS download procedure */
void CsrQvscKcsDownloadComplete(CsrQvscInstanceData *qvscInst,
                                CsrResult result,
                                CsrUint16 kcsBundleId)
{
    CsrQvscKcsDownloadCfmSend(qvscInst->phandle, result, kcsBundleId);
    CsrQvscRestore(qvscInst);
}

/* Closes KCS remove procedure */
void CsrQvscKcsRemoveComplete(CsrQvscInstanceData *qvscInst, CsrResult result)
{
    CsrQvscKcsRemoveCfmSend(qvscInst->phandle, result);
    CsrQvscRestore(qvscInst);
}
#endif /* CSR_DSPM_KCS_DOWNLOAD */

/* TLV download request handler */
CsrBool CsrQvscTlvDownloadReqHandler(CsrQvscInstanceData *qvscInst, void *msg)
{
    if (qvscInst->state == CSR_QVSC_STATE_IDLE)
    {
        CsrQvscTlvDownloadReq *req = msg;
        CsrQvscTlvData *downloadInst = &qvscInst->tlvData;

        qvscInst->phandle = req->phandle;
        downloadInst->index = 0;
        downloadInst->config = req->config;
        downloadInst->length = req->payloadLength;
        downloadInst->payload = req->payload;
        req->payload = NULL; /* So that it is not released */

        CsrQvscTlvDownloadInitiate(downloadInst);

        CSR_QVSC_CHANGE_STATE(qvscInst, CSR_QVSC_STATE_TLV);

        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
#ifdef CSR_DSPM_KCS_DOWNLOAD
/* KCS download request handler */
CsrBool CsrQvscKcsDownloadReqHandler(CsrQvscInstanceData *qvscInst, void *msg)
{
    if (qvscInst->state == CSR_QVSC_STATE_IDLE)
    {
        CsrQvscKcsDownloadReq *req = msg;
        CsrUint8 *body;

        qvscInst->phandle = req->phandle;
        qvscInst->tlvData.index = 0;
        qvscInst->tlvData.config = req->config;
        qvscInst->tlvData.length = req->payloadLength;
        qvscInst->tlvData.payload = req->payload;
        req->payload = NULL; /* So that it is not released */

        body = CsrPmemZalloc(sizeof(CsrQvsSubOpcode) + sizeof(CsrUint16));
        body[0] = CSR_QVSC_SUBOP_CAP_DOWNLOAD_ADD_KCS;
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(req->processorId, &body[1]);

        CsrQvscSendCommand(CSR_QVS_OCF_DOWNLOAD_KCS_REQ,
                           body,
                           sizeof(CsrQvsSubOpcode) + sizeof(CsrUint16));

        CSR_QVSC_CHANGE_STATE(qvscInst, CSR_QVSC_STATE_KCS);

        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}

/* KCS download request handler */
CsrBool CsrQvscKcsRemoveReqHandler(CsrQvscInstanceData *qvscInst, void *msg)
{
    if (qvscInst->state == CSR_QVSC_STATE_IDLE)
    {
        CsrQvscKcsRemoveReq *req = msg;

        if (req->kcsBundleId == CSR_QVSC_KCS_BUNDLE_ID_INVALID)
        {
            /* Send success immediately. */
            CsrQvscKcsRemoveCfmSend(req->kcsBundleId, CSR_QVSC_RESULT_SUCCESS);
        }
        else
        {
            CsrUint8 *body = CsrPmemAlloc(sizeof(CsrQvsSubOpcode) + sizeof(CsrUint16));

            qvscInst->phandle = req->phandle;

            body[0] = CSR_QVSC_SUBOP_CAP_DOWNLOAD_REMOVE_KCS;
            CSR_COPY_UINT16_TO_LITTLE_ENDIAN(req->kcsBundleId, &body[1]);

            CsrQvscSendCommand(CSR_QVS_OCF_DOWNLOAD_KCS_REQ,
                               body,
                               sizeof(CsrQvsSubOpcode) + sizeof(CsrUint16));

            CSR_QVSC_CHANGE_STATE(qvscInst, CSR_QVSC_STATE_KCS);
        }
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}
#endif /* CSR_DSPM_KCS_DOWNLOAD */

