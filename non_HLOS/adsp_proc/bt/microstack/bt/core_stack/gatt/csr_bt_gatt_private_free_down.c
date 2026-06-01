/******************************************************************************
 Copyright (c) 2012-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_BT_GATT_PRIVATE_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_bt_autogen.h"
#include "csr_bt_gatt_private_lib.h"
#include "csr_bt_gatt_private_prim.h"

void CsrBtGattPrivateFreeDownstreamMessageContents(CsrUint16 eventClass, void *message);

void CsrBtGattPrivateFreeDownstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_GATT_PRIVATE_PRIM)
    {
        CsrBtGattPrim *prim = (CsrBtGattPrim *) message;
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
#endif /* EXCLUDE_CSR_BT_GATT_PRIVATE_MODULE */
