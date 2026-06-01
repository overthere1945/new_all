#ifndef CSR_QVSC_PRIVATE_LIB_H__
#define CSR_QVSC_PRIVATE_LIB_H__
/******************************************************************************
 Copyright (c) 2016-2018 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_qvsc_private_prim.h"
#include "csr_qvsc_lib.h"
#include "csr_msg_transport.h"
#include "csr_qvsc_task.h"
#include "csr_macro.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrQvscTlvDownloadReqSend
 *
 *  DESCRIPTION
 *      TLV download procedure.
 *
 *  PARAMETERS
 *      phandle         : The identity of the calling process.
 *      config          : Response behavior to be expected from controller.
 *      payloadLength   : TLV payload length
 *      payload         : TLV payload.
 *----------------------------------------------------------------------------*/
#define CsrQvscTlvDownloadReqSend(_phandle, _config, _payloadLength, _payload)  \
    do                                                                          \
    {                                                                           \
        CsrQvscTlvDownloadReq *_req = CsrPmemZalloc(sizeof(*_req));             \
        _req->type = CSR_QVSC_TLV_DOWNLOAD_REQ;                                 \
        _req->phandle = _phandle;                                               \
        _req->config = _config;                                                 \
        _req->payloadLength = _payloadLength;                                   \
        _req->payload = _payload;                                               \
        CsrMsgTransport(CSR_QVSC_IFACEQUEUE, CSR_QVSC_PRIM, _req);              \
    } while (0)


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrQvscKcsDownloadReqSend
 *
 *  DESCRIPTION
 *      KCS download procedure.
 *
 *  PARAMETERS
 *      phandle         : The identity of the calling process.
 *      config          : Download response behavior to be expected from controller.
 *      processorId     : DSP processor ID
 *      payloadLength   : TLV payload length
 *      payload         : TLV payload.
 *----------------------------------------------------------------------------*/
#define CsrQvscKcsDownloadReqSend(_phandle,                                     \
                                  _config,                                      \
                                  _processorId,                                 \
                                  _payloadLength,                               \
                                  _payload)                                     \
    do                                                                          \
    {                                                                           \
        CsrQvscKcsDownloadReq *_req = CsrPmemZalloc(sizeof(*_req));             \
        _req->type = CSR_QVSC_KCS_DOWNLOAD_REQ;                                 \
        _req->phandle = _phandle;                                               \
        _req->config = _config;                                                 \
        _req->processorId = _processorId;                                       \
        _req->payloadLength = _payloadLength;                                   \
        _req->payload = _payload;                                               \
        CsrMsgTransport(CSR_QVSC_IFACEQUEUE, CSR_QVSC_PRIM, _req);              \
    } while (0)


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrQvscKcsRemoveReqSend
 *
 *  DESCRIPTION
 *      KCS remove procedure.
 *
 *  PARAMETERS
 *      phandle         : The identity of the calling process.
 *      kcsBundleId     : KCS bundle ID.
 *----------------------------------------------------------------------------*/
#define CsrQvscKcsRemoveReqSend(_phandle, _kcsBundleId)                         \
    do                                                                          \
    {                                                                           \
        CsrQvscKcsRemoveReq *_req = CsrPmemZalloc(sizeof(*_req));               \
        _req->type = CSR_QVSC_KCS_REMOVE_REQ;                                   \
        _req->phandle = _phandle;                                               \
        _req->kcsBundleId = _kcsBundleId;                                       \
        CsrMsgTransport(CSR_QVSC_IFACEQUEUE, CSR_QVSC_PRIM, _req);              \
    } while (0)

void CsrQvscPrivateFreeUpstreamMessageContents(CsrUint16 eventClass, void *message);

#ifdef __cplusplus
}
#endif

#endif /* CSR_QVSC_PRIVATE_LIB_H__ */

