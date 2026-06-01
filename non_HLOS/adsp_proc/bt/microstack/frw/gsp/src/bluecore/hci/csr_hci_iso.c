/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_hci_iso.h"
#include "csr_hci_util.h"
#include "csr_hci_sef.h"
#include <stdio.h>

#ifdef HCI_ISO_DEBUG
#undef HCI_ISO_DEBUG
#define HCI_ISO_DEBUG
#endif

/* array for the IsoFunctionCalls */
static CsrHciIsoHandleStructure *IsoFunctionArray = NULL;

typedef struct isoPktQueue
{
    struct isoPktQueue *next;
    CsrUint8* pkt;
} isoPktQueue;

static isoPktQueue *pktQueue = NULL;
void hciIsoSendFromQueue(void);
CsrHciIsoTxEventFuncType isoTxEventFunc = NULL;

/***********************************************************************
CsrHciIsoInit
***********************************************************************/
void CsrHciIsoInit(void)
{
    CsrHciIsoHandleStructure *prev, *next = IsoFunctionArray;

    while (next)
    {
        prev = next;
        next = next->next;
        CsrPmemFree(prev);
    }
    IsoFunctionArray = NULL;
}

void CsrHciRegisterIsoTxEventFunction(CsrHciIsoTxEventFuncType theTxEventFunctionPtr)
{
    isoTxEventFunc = theTxEventFunctionPtr;
}

void CsrHciUnRegisterIsoTxEventFunction(void)
{
    isoTxEventFunc = NULL;
}

/***********************************************************************
registerIsoHandle
***********************************************************************/
CsrBool CsrHciRegisterIsoHandle(CsrUint16 theIsoHandle,
                                          CsrHciIsoHandlerFuncType theFunctionPtr)
{
    CsrHciIsoHandleStructure *next = CsrPmemAlloc(sizeof(CsrHciIsoHandleStructure));

    next->next = NULL;
    next->isoHandle = theIsoHandle;
    next->isoHandlerFunc = theFunctionPtr;

    if (IsoFunctionArray)
    {
        CsrHciIsoHandleStructure *prev;

        prev = IsoFunctionArray;

        while (prev->next)
        {
            prev = prev->next;
        }
        prev->next = next;
    }
    else
    {
        IsoFunctionArray = next;
    }

    return TRUE;
}

/***********************************************************************
CsrHciDeRegisterIsoHandle
***********************************************************************/
void CsrHciDeRegisterIsoHandle(CsrUint16 theIsoHandle)
{
    CsrHciIsoHandleStructure *prev, *next;

    prev = NULL;
    next = IsoFunctionArray;

    while (next)
    {
        if (next->isoHandle == theIsoHandle)
        {
            if (prev == NULL)
            {
                IsoFunctionArray = next->next;
            }
            else
            {
                prev->next = next->next;
            }
            CsrPmemFree(next);
            break;
        }
        prev = next;
        next = next->next;
    }
}

#define ISO_HDR_SIZE 4

/***********************************************************************
 *CsrHciLookForIsoHandle
 * Copy ISO data before sending to the application.
 *
 ***********************************************************************/
CsrBool CsrHciLookForIsoHandle(const CsrUint8 *theBuf)
{
    CsrHciIsoHandleStructure *next = IsoFunctionArray;
    CsrUint16 theIsoHandle;
    CsrUint16 isoDataLoadLength;

    theIsoHandle = CSR_HCI_GET_XAP_UINT16(theBuf) & 0x0fff;

    while (next)
    {
        if (next->isoHandle == theIsoHandle)
        {
            CsrUint8 *data;
            isoDataLoadLength = theBuf[2] + ((theBuf[3] & 0x3F) << 8);
            data = CsrPmemAlloc(isoDataLoadLength + ISO_HDR_SIZE);
            CsrMemCpy(data, theBuf, isoDataLoadLength + ISO_HDR_SIZE);
            next->isoHandlerFunc(data);
            return TRUE;
        }
        next = next->next;
    }
    return FALSE;
}

CsrUint16 isoCredits = NUMBER_OF_ISO_CREDITS;

#define HCI_ISO_STATS 
#ifdef HCI_ISO_STATS
static CsrUint64 isoTxCount = 0;
static CsrUint64 isoTxNocpCount = 0;
static CsrUint64 isoTxQueueEmptyCount = 0;
#endif

void CsrHciIsoHandleNumOfCompPkts(CsrUint8 *pkt)
{
    CsrUint8 numOfHandles;
    CsrUint8 i;
    CsrHciIsoHandleStructure *next = IsoFunctionArray;
    CsrUint16 handle;
    CsrUint16 numOfCompPkts;
    CsrBool isoCreditsReceived = FALSE;

    i = 2;
    numOfHandles = pkt[i];
    i++;
    for (;numOfHandles > 0;numOfHandles--)
    {
        handle = CSR_HCI_GET_XAP_UINT16(pkt+i);
        i += 2;
        numOfCompPkts = CSR_HCI_GET_XAP_UINT16(pkt+i);
        i += 2;
        while (next)
        {
            if (next->isoHandle == handle)
            {
                if (numOfCompPkts)
                {
#ifdef HCI_ISO_DEBUG
                    printf("\n>ISO NOCP EVT: handle: %x, nocp: %x, old credits: %x, new credits: %x\n", 
                            handle, 
                            numOfCompPkts, 
                            isoCredits, 
                            isoCredits + numOfCompPkts);
#endif /* HCI_ISO_DEBUG */
                    isoCredits += numOfCompPkts;
                    isoCreditsReceived = TRUE;
                }
            }
            next = next->next;
        }
    }
    CsrPmemFree(pkt);
#ifdef HCI_ISO_STATS
    if (isoCredits > 0 && !pktQueue)
    {
        /* We had credits but the queue is empty, lets count */
        isoTxQueueEmptyCount++;
    }
#endif /* HCI_ISO_STATS */
    hciIsoSendFromQueue();
    if ((isoCreditsReceived != FALSE) &&
         (isoTxEventFunc != NULL))
    {
#ifdef HCI_ISO_STATS
        isoTxNocpCount++;
#endif /* HCI_ISO_STATS */
        isoTxEventFunc(handle);
		/* Trying to flush the packets from the queue again to avoid avoid
		a situation in which we have credits and we dont have packets in 
		queue and we miss one cycle because of that.
		
		*/
		hciIsoSendFromQueue();
    }
}

void hciIsoSendFromQueue(void)
{

    if (isoCredits)
    {
        if (pktQueue)
        {
            isoPktQueue *cur = pktQueue;
#ifdef HCI_ISO_DEBUG
            printf("\n>ISO SEND FROM QUEUE: old credits: %x, new credits: %x\n", isoCredits, (isoCredits - 1));
#endif /* HCI_ISO_DEBUG */
            isoCredits--;

            CsrHciSendIsoPacket(cur->pkt);
            pktQueue = cur->next;
            CsrPmemFree(cur);
            hciIsoSendFromQueue();
        }
    }
    else
    {
#ifdef HCI_ISO_DEBUG
        printf("\n>ISO SEND FROM QUEUE: no credits\n");
#endif /* HCI_ISO_DEBUG */
    }
}
#ifdef HCI_ISO_STATS
void CsrHciIsoStatsPrint(void)
{
    printf("\n>ISO STATS: TxCount: %llu, Nocps: %llu, EmptyQ: %llu\n", isoTxCount, isoTxNocpCount, isoTxQueueEmptyCount);
}
#endif

/***********************************************************************
CsrHciSendIsoData
***********************************************************************/
void CsrHciSendIsoData(void)
{
    hciIsoSendFromQueue();
}

void CsrHciAddIsoData(CsrUint8 *theData)
{
    isoPktQueue *newPkt = CsrPmemAlloc(sizeof(*newPkt));

#ifdef HCI_ISO_STATS
    isoTxCount++;

#if 0
    /* try to print the stats every 100 packets */
    if ((isoTxCount % 100) == 0)
    {
        printf("\n>ISO STATS: TxCount: %d, Nocps: %d, EmptyQ: %d\n", isoTxCount, isoTxNocpCount, isoTxQueueEmptyCount);
    }
#endif
#endif /* HCI_ISO_STATS */

    newPkt->next = NULL;
    newPkt->pkt = theData;

    if (pktQueue)
    {
        isoPktQueue *prev;

        prev = pktQueue;

        while (prev->next)
        {
            prev = prev->next;
        }
        prev->next = newPkt;
    }
    else
    {
        pktQueue = newPkt;
    }
}

void CsrHciClearIsoData(void)
{
    isoPktQueue *current;
    isoPktQueue *next;

    current = pktQueue;
    while(current)
    {
         next = current->next;
         CsrPmemFree(current);
         current = next;
    }
    pktQueue = NULL;
    isoTxCount = 0;
    isoTxNocpCount = 0;
    isoTxQueueEmptyCount = 0;
}

