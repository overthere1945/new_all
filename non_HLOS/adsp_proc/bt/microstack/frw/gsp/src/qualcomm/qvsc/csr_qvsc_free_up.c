#ifndef EXCLUDE_CSR_QVSC_MODULE
/*****************************************************************************
 Copyright (c) 2017-2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
*****************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_autogen.h"
#include "csr_qvsc_lib.h"
#include "csr_qvsc_prim.h"

void CsrQvscFreeUpstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_QVSC_PRIM)
    {
        CsrQvscPrim *prim = (CsrQvscPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_QVSC_CFM
            case CSR_QVSC_CFM:
            {
                CsrQvscCfm *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_QVSC_CFM */

#ifndef EXCLUDE_CSR_QVSC_EVENT_IND
            case CSR_QVSC_EVENT_IND:
            {
                CsrQvscEventInd *p = message;
                CsrPmemFree(p->event);
                p->event = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_QVSC_EVENT_IND */
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
