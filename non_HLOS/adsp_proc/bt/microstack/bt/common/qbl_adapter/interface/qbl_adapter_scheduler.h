#ifndef QBL_ADAPTER_SCHEDULER_H__
#define QBL_ADAPTER_SCHEDULER_H__
/******************************************************************************
 Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #6 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_tasks.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GATT_DATA_LOGGER
void GattDataLoggerSendPrim(void );
#endif /* GATT_DATA_LOGGER */

#if defined(CSR_TARGET_PRODUCT_VM)
/* Use public interfaces */
void VmSendL2capPrim(void *prim);
void VmSendDmPrim(void *prim);
void VmSendDmPrim(void *prim);
void VmSendAttPrim(void *prim);
void VmSendRfcommPrim(void *prim);
void VmSendSdpPrim(void *prim);
void VmSendVsdmPrim(void *prim);

#define L2CA_PutMsg(prim) VmSendL2capPrim(prim)
#define DM_PutMsg(prim) VmSendDmPrim(prim)
#define DM_HCI_PutMsg(prim) VmSendDmPrim(prim)
#ifdef GATT_DATA_LOGGER
#define ATT_PutMsg(prim) { GattDataLoggerSendPrim(); \
                           VmSendAttPrim(prim);}
#else
#define ATT_PutMsg(prim) VmSendAttPrim(prim)
#endif /* GATT_DATA_LOGGER */
#define RFC_PutMsg(prim) VmSendRfcommPrim(prim)
#define SDP_PutMsg(prim) VmSendSdpPrim(prim)
#define VSC_PutMsg(prim) VmSendVsdmPrim(prim)
#else
#define L2CA_PutMsg(prim) put_message(L2CAP_IFACEQUEUE, L2CAP_PRIM, (prim))
#define DM_PutMsg(prim) put_message(ADV_SCAN_IFACEQUEUE, DM_PRIM, (prim))
#define DM_HCI_PutMsg(prim) put_message(DM_HCI_IFACEQUEUE, DM_PRIM, (prim))
#ifdef GATT_DATA_LOGGER
#define ATT_PutMsg(prim) {GattDataLoggerSendPrim(); \
                          put_message(ATT_IFACEQUEUE, ATT_PRIM, (prim));}
#else
#define ATT_PutMsg(prim) put_message(ATT_IFACEQUEUE, ATT_PRIM, (prim))
#endif /* GATT_DATA_LOGGER */
#define RFC_PutMsg(prim) put_message(RFCOMM_IFACEQUEUE, RFCOMM_PRIM, (prim))
#define SDP_PutMsg(prim) put_message(SDP_IFACEQUEUE, SDP_PRIM, (prim))
#define VSC_PutMsg(prim) put_message(VSADV_SCAN_IFACEQUEUE, VSDM_PRIM, (prim))
#endif

#ifdef __cplusplus
}
#endif

#endif
