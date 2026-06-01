#ifndef CSR_TM_BLUECORE_H4_H__
#define CSR_TM_BLUECORE_H4_H__

#include "csr_synergy.h"
/*****************************************************************************

            Copyright (c) 2009-2015 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void CsrTmBlueCoreH4Init(void **gash);
void CsrTmBlueCoreRegisterUartHandleH4(void *handle);

#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_BLUECORE_H4_H__ */

