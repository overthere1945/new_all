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

#ifdef GATT_DATA_LOGGER
#include "gatt_data_logger_prim.h"
#endif /* GATT_DATA_LOGGER */

void CsrBtGattFreeUpstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_GATT_PRIM)
    {
        CsrBtGattPrim *prim = (CsrBtGattPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_GATT_READ_USER_DESCRIPTION_CFM
            case CSR_BT_GATT_READ_USER_DESCRIPTION_CFM:
            {
                CsrBtGattReadUserDescriptionCfm *p = message;
                CsrPmemFree(p->usrDescription);
                p->usrDescription = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_READ_USER_DESCRIPTION_CFM */
#ifndef EXCLUDE_CSR_BT_GATT_READ_PROFILE_DEFINED_DESCRIPTOR_CFM
            case CSR_BT_GATT_READ_PROFILE_DEFINED_DESCRIPTOR_CFM:
            {
                CsrBtGattReadProfileDefinedDescriptorCfm *p = message;
                CsrPmemFree(p->value);
                p->value = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_READ_PROFILE_DEFINED_DESCRIPTOR_CFM */
#ifndef EXCLUDE_CSR_BT_GATT_READ_BY_UUID_IND
            case CSR_BT_GATT_READ_BY_UUID_IND:
            {
                CsrBtGattReadByUuidInd *p = message;
                CsrPmemFree(p->value);
                p->value = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_READ_BY_UUID_IND */
#ifndef EXCLUDE_CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_IND
            case CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_IND:
            {
                CsrBtGattDbAccessReadMultiVarInd *p = message;
                CsrPmemFree(p->attrHandles);
                p->attrHandles = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_DB_ACCESS_READ_MULTI_VAR_IND */
#ifndef EXCLUDE_CSR_BT_GATT_CLIENT_NOTIFICATION_IND
            case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
            {
                CsrBtGattClientNotificationInd *p = message;
                CsrPmemFree(p->value);
                p->value = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_CLIENT_NOTIFICATION_IND */
#ifndef EXCLUDE_CSR_BT_GATT_READ_MULTI_CFM
            case CSR_BT_GATT_READ_MULTI_CFM:
            {
                CsrBtGattReadMultiCfm *p = message;
                CsrPmemFree(p->value);
                p->value = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_READ_MULTI_CFM */
#ifndef EXCLUDE_CSR_BT_GATT_CLIENT_INDICATION_IND
            case CSR_BT_GATT_CLIENT_INDICATION_IND:
            {
                CsrBtGattClientIndicationInd *p = message;
                CsrPmemFree(p->value);
                p->value = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_CLIENT_INDICATION_IND */
#ifndef EXCLUDE_CSR_BT_GATT_READ_MULTI_VAR_CFM
            case CSR_BT_GATT_READ_MULTI_VAR_CFM:
            {
                CsrBtGattReadMultiVarCfm *p = message;
                CsrPmemFree(p->value);
                p->value = NULL;
                CsrPmemFree(p->handles);
                p->handles = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_READ_MULTI_VAR_CFM */
#ifndef EXCLUDE_CSR_BT_GATT_READ_AGGREGATE_FORMAT_CFM
            case CSR_BT_GATT_READ_AGGREGATE_FORMAT_CFM:
            {
                CsrBtGattReadAggregateFormatCfm *p = message;
                CsrPmemFree(p->handles);
                p->handles = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_READ_AGGREGATE_FORMAT_CFM */
#ifndef EXCLUDE_CSR_BT_GATT_READ_CFM
            case CSR_BT_GATT_READ_CFM:
            {
                CsrBtGattReadCfm *p = message;
                CsrPmemFree(p->value);
                p->value = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_GATT_READ_CFM */
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
