#ifndef CSR_LOG_REGISTER_H__
#define CSR_LOG_REGISTER_H__

#include "csr_synergy.h"
/*****************************************************************************

            Copyright (c) 2009-2015 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/

#include "csr_types.h"
#include "csr_log.h"

#ifdef __cplusplus
extern "C" {
#endif

void CsrLogRegisterTechnology(const CsrLogTechInformation *techInfo);

#ifdef __cplusplus
}
#endif

#endif
