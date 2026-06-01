/******************************************************************************
 Copyright (c) 2012-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#include "csr_synergy.h"
#include "csr_msgconv.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_bt_gatt_free_handcoded.h"
#ifndef EXCLUDE_CSR_BT_GATT_MODULE
#include "csr_bt_gatt_prim.h"

void CsrBtGattFreeHandcoded(void *message)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim *) message;
    switch(*prim)
    {
#ifndef EXCLUDE_CSR_BT_GATT_DB_ACCESS_WRITE_IND
        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            CsrBtGattDbAccessWriteIndPrimFree(prim);
            break;
        }
#endif /* EXCLUDE_CSR_BT_GATT_DB_ACCESS_WRITE_IND */
#ifndef EXCLUDE_CSR_BT_GATT_DB_ADD_REQ
        case CSR_BT_GATT_DB_ADD_REQ:
        {
            CsrBtGattDbAddReqPrimFree(prim);
            break;
        }
#endif /* EXCLUDE_CSR_BT_GATT_DB_ADD_REQ */
#ifndef EXCLUDE_CSR_BT_GATT_WRITE_REQ
        case CSR_BT_GATT_WRITE_REQ:
        {
            CsrBtGattWriteReqPrimFree(prim);
            break;
        }
#endif /* EXCLUDE_CSR_BT_GATT_WRITE_REQ */
        case CSR_BT_GATT_LONG_READ_CFM:
        {
			CsrBtGattLongReadCfmPrimFree(prim);
			break;
        }
#ifdef GATT_OFFLOAD
        case GATT_OFFLOAD_ENABLE_SERVICE_REQ:
        {
            GattOffloadEnableServiceReq *msg = message;

            if (msg->service != NULL)
            {
                CsrPmemFree(msg->service);
                msg->service = NULL;
            }
            break;
        }
#endif /* GATT_OFFLOAD */
        default:
        {
            break;
        }
    }
}

#endif

