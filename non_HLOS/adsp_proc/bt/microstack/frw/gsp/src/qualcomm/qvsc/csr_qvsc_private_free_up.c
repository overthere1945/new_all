#ifndef EXCLUDE_CSR_QVSC_PRIVATE_MODULE
/*****************************************************************************
 Copyright (c) 2018 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
*****************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_autogen.h"
#include "csr_qvsc_private_lib.h"
#include "csr_qvsc_private_prim.h"

void CsrQvscPrivateFreeUpstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_QVSC_PRIVATE_PRIM)
    {
        CsrQvscPrim *prim = (CsrQvscPrim *) message;
        switch (*prim)
        {
            default:
            {
                break;
            }
        } /* End switch */
    } /* End if */
    else
    {
        /* Unknown primitive type, exception handling */
    }
}
#endif /* EXCLUDE_CSR_QVSC_PRIVATE_MODULE */
