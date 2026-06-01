/*****************************************************************************

            Copyright (c) 2012-2015 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_TM_BLUECORE_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_autogen.h"
#include "csr_tm_bluecore_lib.h"
#include "csr_tm_bluecore_prim.h"

void CsrTmBluecoreFreeUpstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_TM_BLUECORE_PRIM)
    {
        CsrTmBlueCorePrim *prim = (CsrTmBlueCorePrim *) message;
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
#endif /* EXCLUDE_CSR_TM_BLUECORE_MODULE */
