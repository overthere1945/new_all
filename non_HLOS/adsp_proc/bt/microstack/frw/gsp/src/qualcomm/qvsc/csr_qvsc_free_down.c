/*****************************************************************************
 Copyright (c) 2017-2018 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
*****************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_QVSC_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_autogen.h"
#include "csr_qvsc_lib.h"
#include "csr_qvsc_prim.h"

void CsrQvscFreeDownstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_QVSC_PRIM)
    {
        CsrQvscPrim *prim = (CsrQvscPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_QVSC_REQ
            case CSR_QVSC_REQ:
            {
                CsrQvscReq *p = message;
                CsrMblkDestroy(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_QVSC_REQ */

#ifndef EXCLUDE_CSR_QVSC_SUBSCRIBE_REQ
            case CSR_QVSC_SUBSCRIBE_REQ:
            {
                CsrQvscSubscribeReq *p = message;
                CsrPmemFree(p->pattern);
                p->pattern = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_QVSC_SUBSCRIBE_REQ */

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
#endif /* EXCLUDE_CSR_QVSC_MODULE */
