/******************************************************************************
 Copyright (c) 2012-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_BT_GATT_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_bt_autogen.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_free_handcoded.h"
#include "csr_bt_gatt_sef.h"

void CsrBtGattFreeDownstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_GATT_PRIM)
    {
        CsrBtGattPrim *prim = (CsrBtGattPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_GATT_DB_ACCESS_RES
            case CSR_BT_GATT_DB_ACCESS_RES:
            {
                CsrBtGattDbAccessRes *p = message;
                CsrPmemFree(p->value);
                p->value = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_DB_ACCESS_RES */
#ifndef EXCLUDE_CSR_BT_GATT_READ_MULTI_VAR_REQ
            case CSR_BT_GATT_READ_MULTI_VAR_REQ:
            {
                CsrBtGattReadMultiVarReq *p = message;
                CsrPmemFree(p->handles);
                p->handles = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_READ_MULTI_VAR_REQ */
#ifndef EXCLUDE_CSR_BT_GATT_EVENT_SEND_REQ
            case CSR_BT_GATT_EVENT_SEND_REQ:
            {
                CsrBtGattEventSendReq *p = message;
                CsrPmemFree(p->value);
                p->value = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_EVENT_SEND_REQ */
#ifndef EXCLUDE_CSR_BT_GATT_READ_MULTI_REQ
            case CSR_BT_GATT_READ_MULTI_REQ:
            {
                CsrBtGattReadMultiReq *p = message;
                CsrPmemFree(p->handles);
                p->handles = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_READ_MULTI_REQ */
#ifndef EXCLUDE_CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_RSP
            case CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_RSP:
            {
                CsrBtGattDbAccessReadMultiVarRsp *p = message;
                CsrPmemFree(p->values);
                p->values = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_RSP */
            default:
            {
                CsrBtGattFreeHandcoded(prim);
                break;
            }
        } /* End switch */
    } /* End if */
    else
    {
        /* Unknown primitive type, exception handling */
    }
}
#endif /* EXCLUDE_CSR_BT_GATT_MODULE */
