#ifndef CSR_TM_BLUECORE_SEF_H__
#define CSR_TM_BLUECORE_SEF_H__

/******************************************************************************
 Copyright (c) 2008-2018 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_log_text_2.h"
#include "csr_tm_bluecore_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DECLARE(CsrTmBluecoreLto);

#ifndef CSR_USE_QCA_CHIP
#define CSR_BCCMD_PANIC_ARG_VARID             (0x6805)
void CsrTmBlueCoreBccmdMsgHandler(CsrTmBlueCoreInstanceData *tmBcInst);

#ifdef CSR_HYDRA_SSD
void CsrTmBlueCoreSsdMsgHandler(CsrTmBlueCoreInstanceData *tmBcInst);
#endif /* CSR_HYDRA_SSD */
#endif /* !CSR_USE_QCA_CHIP */

void CsrTmBlueCoreTmBlueCoreMsgHandler(CsrTmBlueCoreInstanceData *tmBcInst);
void CsrTmBlueCoreHciMsgHandler(CsrTmBlueCoreInstanceData *tmBcInst);
void CsrTmBlueCoreActivate(CsrTmBlueCoreInstanceData *tmBcInst);

#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_BLUECORE_SEF_H__*/
