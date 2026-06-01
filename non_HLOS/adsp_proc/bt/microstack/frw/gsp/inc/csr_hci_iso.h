#ifndef CSR_HCI_ISO_H__
#define CSR_HCI_ISO_H__

#include "csr_synergy.h"
/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/


#include "csr_types.h"


#ifdef __cplusplus
extern "C" {
#endif

#define NUMBER_OF_ISO_CREDITS 8

typedef struct CsrHciIsoTxEventInfoStructure
{
    CsrUint16 infoLength;
    CsrUint8 *info;
} CsrHciIsoTxEventInfo;

typedef void (*CsrHciIsoHandlerFuncType)(CsrUint8 *);
/* callback called when controller acks when a packet is sent over the air */
typedef void (*CsrHciIsoTxEventFuncType)(CsrUint16 handle);

typedef struct CsrHciIsoHandleStructure
{
    CsrUint16                        isoHandle;
    CsrHciIsoHandlerFuncType         isoHandlerFunc;
    /*
    CsrHciIsoTxEventFuncType         isoTxEventFunc;
    */
    struct CsrHciIsoHandleStructure *next;
} CsrHciIsoHandleStructure;

CsrBool CsrHciRegisterIsoHandle(CsrUint16 theIsoHandle,
                                          CsrHciIsoHandlerFuncType theFunctionPtr);

void CsrHciRegisterIsoTxEventFunction(CsrHciIsoTxEventFuncType theTxEventFunctionPtr);
/* API for unregistering the Tx call back when not required */
void CsrHciUnRegisterIsoTxEventFunction(void);
/* API for clearing of the Queue */
void CsrHciClearIsoData(void);

void CsrHciAddIsoData(CsrUint8 *theData);
void CsrHciSendIsoData(void);
void CsrHciDeRegisterIsoHandle(CsrUint16 theIsoHandle);
void CsrHciIsoInit(void);
CsrBool CsrHciLookForIsoHandle(const CsrUint8 *theBuf);
void CsrHciIsoHandleNumOfCompPkts(CsrUint8 *pkt);
void CsrHciIsoStatsPrint(void);

#ifdef __cplusplus
}
#endif

#endif /* !CSR_HCI_ISO_H__ */
