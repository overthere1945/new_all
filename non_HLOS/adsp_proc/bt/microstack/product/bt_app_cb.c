/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#include "csr_synergy.h"

#include "csr_sched_init.h"
#include "csr_tm_bluecore_task.h"
#include "csr_hci_task.h"
#include "bt_private.h"
#include "csr_bt_tasks.h"
#include "bt_main.h"

BtHostApiInstData BtHostApiInst;

/******************************************************************************
  BtHostAddAppHandleInfo: This function adds app handle info
******************************************************************************/
void BtHostAddAppHandleInfo(BtAppHandle handle, BtHostAppCallback appCallback)
{
    BtHostAppEntry *appEntry = CsrPmemAlloc(sizeof(BtHostAppEntry));

    appEntry->handle = handle;
    appEntry->appCallback = appCallback;

    appEntry->next = BtHostApiInst.appInfo;
    BtHostApiInst.appInfo = appEntry;
}

/******************************************************************************
  BtHostRemoveAppHandleInfo: This function removes app handle info
******************************************************************************/
void BtHostRemoveAppHandleInfo(BtAppHandle handle)
{
    BtHostAppEntry  *p;
    BtHostAppEntry  **pp;

    for (pp = &BtHostApiInst.appInfo;
         (p = *pp) != NULL ;
         pp = &(p->next) )
    {
        /* Look for a matching entry with handle */ 
        if (handle == p->handle)
        {
            /* Found the correct entry. */ 
            *pp = p->next;
            CsrPmemFree(p);
            return;
        }
    }
}

/******************************************************************************
  BtHostFindAppHandleInfo: This function find app handle info
******************************************************************************/
BtHostAppEntry *BtHostFindAppHandleInfo(BtAppHandle handle)
{
    BtHostAppEntry  *appEntry;

    for (appEntry = BtHostApiInst.appInfo; 
         (appEntry != NULL); 
         (appEntry = appEntry->next))
    {
        if (appEntry->handle == handle)
        {
            return appEntry;
        }
    }
    return NULL;
}

/******************************************************************************
  BtHostAddAppHandleInfo: This function adds app handle info
******************************************************************************/
boolean BtHostRegisterAppHandleInfo(BtAppHandle handle, BtHostAppCallback appCb)
{
    boolean ret = TRUE;

    if ((handle < BT_APP_HANDLE_START) || (appCb == NULL))
    {
        ret = FALSE;
    }
    else if (BtHostFindAppHandleInfo(handle) == NULL)
    {
        BtHostAddAppHandleInfo(handle, appCb);
    }

    return ret;
}

/******************************************************************************
  BtHostExternalAppCallBack: This function is called when Synergy sends
  a message to external application.
******************************************************************************/
CsrBool BtHostExternalAppCallBack(CsrSchedQid q, CsrUint16 mi, void *mv)
{
    BtHostAppEntry *appEntry;

    /* Call the platform/product specific callback */
    if ((BtHostApiInst.eventCallback != NULL) && (BtHostApiInst.eventCallback(q, mi, mv) == FALSE))
    {
        return TRUE;
    }

    if ((appEntry = BtHostFindAppHandleInfo(q)) != NULL)
    {
        CsrPrim type = mv?(*(CsrPrim *)mv):0xffff;

        BtHostApiInst.primType = type;
        BtHostApiInst.mi = mi; 
        BtHostApiInst.appCtxRunning = TRUE;
        appEntry->appCallback(q, mi, mv);
        BtHostApiInst.appCtxRunning = FALSE;
    }
    else
    {
        /* Application has not registered the callback */
        CsrPanic(CSR_TECH_BT, BT_PANIC_NO_APP_CALLBACK, "No app callback");
    }

    return TRUE;
}


/******************************************************************************
  BtHostExtCbInit:
******************************************************************************/
void BtHostExtCbInit(BtHostEventCallback eventCb)
{
    CsrSchedRegisterExternalSend(BtHostExternalAppCallBack);

    /* Platform/product specific callback can be registered by the application.
     * This function gets called for upstream messages
     * Each platform/product can add additional functionality for upstream messages */
    BtHostApiInst.eventCallback = eventCb;
}

