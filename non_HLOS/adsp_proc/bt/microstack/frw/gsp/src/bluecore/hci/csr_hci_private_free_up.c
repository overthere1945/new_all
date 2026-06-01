/*****************************************************************************

            Copyright (c) 2011-2015 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_HCI_PRIVATE_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_autogen.h"
#include "csr_hci_private_lib.h"
#include "csr_hci_private_prim.h"

void CsrHciPrivateFreeUpstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_HCI_PRIVATE_PRIM)
    {
        CsrHciPrim *prim = (CsrHciPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_HCI_NOP_IND
            case CSR_HCI_NOP_IND:
            {
                CsrHciNopInd *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_HCI_NOP_IND */
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
#endif /* EXCLUDE_CSR_HCI_PRIVATE_MODULE */
