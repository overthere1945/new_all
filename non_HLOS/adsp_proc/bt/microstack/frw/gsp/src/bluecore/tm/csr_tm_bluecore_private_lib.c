/*****************************************************************************

            Copyright (c) 2008-2024 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/
#include "csr_synergy.h"
#include "csr_tm_bluecore_private_prim.h"
#include "csr_tm_bluecore_private_lib.h"
#include "csr_pmem.h"

CsrTmBluecoreResetInd *CsrTmBluecoreResetInd_struct(void)
{
    CsrTmBluecoreResetInd *prim;
    prim = (CsrTmBluecoreResetInd *) CsrPmemAlloc(sizeof(CsrTmBluecoreResetInd));
    prim->type = CSR_TM_BLUECORE_RESET_IND;
    return prim;
}

CsrTmBluecorePanicInd *CsrTmBluecorePanicInd_struct(void)
{
    CsrTmBluecorePanicInd *prim;
    prim = (CsrTmBluecorePanicInd *) CsrPmemAlloc(sizeof(CsrTmBluecorePanicInd));
    prim->type = CSR_TM_BLUECORE_PANIC_IND;
    return prim;
}
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
CsrTmBluecorePatchSegmentDownloadReq *CsrTmBluecorePatchSegmentDownloadReq_struct(CsrSchedQid phandle)
{
    CsrTmBluecorePatchSegmentDownloadReq *prim;

    prim = (CsrTmBluecorePatchSegmentDownloadReq *) CsrPmemAlloc(sizeof(CsrTmBluecorePatchSegmentDownloadReq));
    prim->type = CSR_TM_BLUECORE_PATCH_SEGMENT_DOWNLOAD_REQ;
    prim->phandle = phandle;
    return prim;
}
#endif
