#ifndef CSR_TM_BLUECORE_PRIVATE_LIB_H__
#define CSR_TM_BLUECORE_PRIVATE_LIB_H__
/*****************************************************************************

            Copyright (c) 2008-2024 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/
#include "csr_tm_bluecore_private_prim.h"
#include "csr_tm_bluecore_task.h"
#include "csr_msg_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

CsrTmBluecoreResetInd *CsrTmBluecoreResetInd_struct(void);
#define CsrTmBluecoreResetIndSend(){            \
        CsrTmBluecoreResetInd *msg__; \
        msg__ = CsrTmBluecoreResetInd_struct(); \
        CsrMsgTransport(CSR_TM_BLUECORE_IFACEQUEUE, CSR_TM_BLUECORE_PRIM, msg__);}

CsrTmBluecorePanicInd *CsrTmBluecorePanicInd_struct(void);
#define CsrTmBluecorePanicIndSend(){            \
        CsrTmBluecorePanicInd *msg__; \
        msg__ = CsrTmBluecorePanicInd_struct(); \
        CsrMsgTransport(CSR_TM_BLUECORE_IFACEQUEUE, CSR_TM_BLUECORE_PRIM, msg__);}

#ifdef PATCH_NVM_BUFFER_DOWNLOAD
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTmBluecorePatchSegmentDownloadReqSend
 *
 *  DESCRIPTION
 *      Request to initiate next patch segment download
 *
 *  PARAMETERS
 *      phandle : The identity of the calling process.
 *----------------------------------------------------------------------------*/
CsrTmBluecorePatchSegmentDownloadReq *CsrTmBluecorePatchSegmentDownloadReq_struct(CsrSchedQid phandle);
#define CsrTmBluecorePatchSegmentDownloadReqSend(_ph){ \
        CsrTmBluecorePatchSegmentDownloadReq *msg__; \
        msg__ = CsrTmBluecorePatchSegmentDownloadReq_struct(_ph); \
        CsrMsgTransport(CSR_TM_BLUECORE_IFACEQUEUE, CSR_TM_BLUECORE_PRIM, msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTmBlueCoreInitPatchNvmData
 *
 *  DESCRIPTION
 *      Initialise patch/NVM download data info.
 *
 *  PARAMETERS
 *      data : Pointer to structure data containing patch/NVM info.
 *----------------------------------------------------------------------------*/
void CsrTmBlueCoreInitPatchNvmData(void *data);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTmBlueCoreSetFtmMode
 *
 *  DESCRIPTION
 *      Set FTM Mode to avoid triggering NOP.
 *----------------------------------------------------------------------------*/
void CsrTmBlueCoreSetFtmMode();
#endif
void CsrTmBluecorePrivateFreeUpstreamMessageContents(CsrUint16 eventClass, void *message);
void CsrTmBluecorePrivateFreeDownstreamMessageContents(CsrUint16 eventClass, void *message);

#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_BLUECORE_PRIVATE_LIB_H__ */
