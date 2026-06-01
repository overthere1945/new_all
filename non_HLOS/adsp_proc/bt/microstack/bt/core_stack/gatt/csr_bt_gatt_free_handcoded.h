#ifndef CSR_BT_GATT_FREE_HANDCODED_H__
#define CSR_BT_GATT_FREE_HANDCODED_H__
/******************************************************************************
 Copyright (c) 2012-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXCLUDE_CSR_BT_GATT_MODULE

void CsrBtGattFreeHandcoded(void *message);

/* The following functions must be handcoded */
void CsrBtGattDbAccessWriteIndPrimFree(void *message);
void CsrBtGattDbAddReqPrimFree(void *message);
void CsrBtGattWriteReqPrimFree(void *message);
void CsrBtGattLongReadCfmPrimFree(void *message);

#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_FREE_HANDCODED_H__ */

