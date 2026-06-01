/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#ifndef __HCI_ARBITER_INT_H__
#define __HCI_ARBITER_INT_H__

#include "bluetooth.h"
#include "csr_hci_prim.h"
#include "csr_bt_result.h"
#include "csr_bt_addr.h"
#include "hci_arbiter_prim.h"



#ifdef __cplusplus
extern "C" {
#endif

typedef uint8 HciArbStack;
#define HCI_ARB_PRIMARY_STACK              ((HciArbStack)0x00) 
#define HCI_ARB_MICRO_STACK                ((HciArbStack)0x01) 

typedef uint32 HciArbFilterId;
#define HCI_ARB_INVALID_FILTER_ID            ((HciArbFilterId)0x00)

typedef uint8 HciArbGattRole;
#define HCI_ARB_GATT_SERVER                  ((HciArbGattRole)0x00)
#define HCI_ARB_GATT_CLIENT                  ((HciArbGattRole)0x01)

#define HCI_ARB_RESULT_SUCCESS               ((CsrBtResultCode)0x0000) 
#define HCI_ARB_RESULT_INVALID_OPERATION     ((CsrBtResultCode)0x0001) 
#define HCI_ARB_RESULT_ACL_DOES_NOT_EXIST    ((CsrBtResultCode)0x0002)


typedef uint16   HciArbHciHandle;
typedef uint16   HciArbL2capCid;

typedef struct
{
    HciArbGattRole role;
    uint16         numHandles;
    uint16         attrHandle[1];
}HciArbGattCtx;

typedef HciArbStack (*HCI_ARBITER_ATT_CB)(HciArbHciHandle hciHandle, 
                                          HciArbGattRole role, 
                                          HciArbL2capCid cid);

/******************************************************************************
 * APIs for HCI Arbiter
 ******************************************************************************/
#ifdef INCLUDE_HCI_ARBITER
void HciArbInit(bool isPassthrough);
void HciArbDeinit(void);
CsrBtResultCode HciArbEnableRfcommFilter(HciArbHciHandle hciHandle,
                                         HciArbL2capCid cid,
                                         uint8 dlci,
                                         HciArbFilterId *filterId);
CsrBtResultCode HciArbEnableLecocFilter(HciArbHciHandle hciHandle,
                                        HciArbL2capCid localCid,
                                        HciArbL2capCid remoteCid,
                                        HciArbFilterId *filterId);
CsrBtResultCode HciArbEnableGattFilter(HciArbHciHandle hciHandle,
                                       HciArbGattCtx *ctx,
                                       HciArbFilterId *filterId);
void HciArbDisableFilter(HciArbFilterId filterId);
void * HciArbAclConnected(HciArbHciHandle hciHandle,
                          PHYSICAL_TRANSPORT_T transport,
                          CsrBtTypedAddr addrt);
void HciArbAclDisconnected(void *context);
CsrBtTypedAddr HciArbGetBtAddrFromHandle(HciArbHciHandle hciHandle);

void HciArbRegisterAttSequencingCb(HCI_ARBITER_ATT_CB attCb);

CsrBtResultCode HciArbValidateConnReq(HciArbHciHandle hciHandle);

void HciArbiterSendMsgToBmm(void *msg);
void HciArbiterBtOffIndSend(HciArbiterResultCode status);
void HciArbiterBtssCrashIndSend(HciArbiterBtssErrorCode reason);

void HciArbiterHostCleanup(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __HCI_ARBITER_INT_H__ */

