/******************************************************************************
 Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#include "csr_synergy.h"

#include <stdlib.h>

#include "csr_macro.h"
#include "err.h"
#include "csr_panic.h"
#include "platform/csr_panic_init.h"


static CsrPanicHandler panic = NULL;

CsrPanicInfoSt CsrPanicInfo;

void CsrPanicInit(CsrPanicHandler cb)
{
    panic = cb;
}

void CsrPanic(CsrUint8 tech, CsrUint16 reason, const char *p)
{
    CsrPanicInfo.tech = tech;
    CsrPanicInfo.reason = reason;

#if (CSR_HOST_PLATFORM == QCC5100_HOST)
    {
        ERR_FATAL_ID id = (tech == CSR_TECH_BT) ? 
                          ERR_FAULT_ID_BTHOST_BT_FAULT : ERR_FAULT_ID_BTHOST_FRW_FAULT;

        CSR_UNUSED(p);

        ERR_FATAL(id, reason, 0, 0);
    }
#else
        ERR_FATAL("BT Host Panic tech %d reason %d %s", tech, reason, p);        
#endif
}
