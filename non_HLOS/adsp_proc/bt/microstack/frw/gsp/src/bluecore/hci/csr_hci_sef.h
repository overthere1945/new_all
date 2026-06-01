#ifndef CSR_HCI_SEF_H__
#define CSR_HCI_SEF_H__

/******************************************************************************
Copyright (c) 2009-2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_hci_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This function is used to store the HCI task
 * instance data in a global static such that
 * we can directly process packets from the bgint
 * handler in which they are passed to us from
 * the lower layer.
 */
void CsrHciInstanceRegister(CsrHciInstData *inst);

/* CSR_HCI downstream handler functions */
void CsrHciRegisterEventHandlerReqHandler(CsrHciInstData *inst);
void CsrHciRegisterAclHandlerReqHandler(CsrHciInstData *inst);
void CsrHciUnregisterAclHandlerReqHandler(CsrHciInstData *inst);
void CsrHciRegisterScoHandlerReqHandler(CsrHciInstData *inst);
void CsrHciUnregisterScoHandlerReqHandler(CsrHciInstData *inst);
void CsrHciRegisterVendorSpecificEventHandlerReqHandler(CsrHciInstData *inst);
void CsrHciUnregisterVendorSpecificEventHandlerReqHandler(CsrHciInstData *inst);
void CsrHciCommandReqHandler(CsrHciInstData *inst);
void CsrHciAclDataReqHandler(CsrHciInstData *inst);
void CsrHciScoDataReqHandler(CsrHciInstData *inst);
void CsrHciVendorSpecificCommandReqHandler(CsrHciInstData *inst);
void CsrHciResetTransportReqHandler(CsrHciInstData *inst);
void CsrHciSendIsoPacket(CsrUint8 *theData);
void CsrPeriCommandReqHandler(CsrHciInstData *inst);
void CsrPeriRegisterEventHandlerReqHandler(CsrHciInstData *inst);

void CsrHciSendEventIndToRegisteredHciEventHandler(CsrUint8 *payload, CsrUint16 length);

#ifdef CSR_TARGET_PRODUCT_WEARABLE
void CsrHciBlockTransmission(void);
#endif

/* Prototypes from csr_hci_free_down.c */
void CsrHciFreeDownstreamMessageContents(CsrUint16 eventClass, void *message);

#ifdef __cplusplus
}
#endif

#endif /* CSR_HCI_SEF_H__ */
