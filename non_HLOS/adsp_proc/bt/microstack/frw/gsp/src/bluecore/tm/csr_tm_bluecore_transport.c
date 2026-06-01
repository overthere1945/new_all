/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_tm_bluecore_transport.h"

void CsrTmBlueCoreTransportDelete(void *hdl)
{
    /* do nothing */
}

#ifndef CSR_HYDRA_SSD
CsrBool CsrTmBlueCoreTransportStart(void *hdl)
#else
/*Can pass the tmBcInst itself as a void pointer and extract  service_handle to avoid having 2 
input params for CsrTmBlueCoreTransportStart fn. But i am going with this approach*/
CsrBool CsrTmBlueCoreTransportStart(void *hdl,void *service_handle)
#endif
{
    struct CsrTmBlueCoreTransport *p = hdl;
    CsrBool res = FALSE;
    if ((p) && (p)->start)
    {
#ifndef CSR_HYDRA_SSD
        res = (p)->start();
#else
        res = (p)->start(service_handle);
#endif 
        p->started = res;
    }
    return res;
}

CsrBool CsrTmBlueCoreTransportStop(void *hdl)
{
    struct CsrTmBlueCoreTransport *p = hdl;
    CsrBool res = FALSE;
    if ((p) && (p)->stop)
    {
        res = (p)->stop();
        p->started = !res;
    }
    return res;
}

CsrUint16 CsrTmBlueCoreTransportQuery(void *hdl)
{
    struct CsrTmBlueCoreTransport *p = hdl;
    CsrUint16 res = 0;
    if ((p) && (p)->query)
    {
        res = (p)->query();
    }
    return res;
}

void CsrTmBlueCoreTransportMsgTx(void *hdl, void *msg)
{
    struct CsrTmBlueCoreTransport *p = hdl;
    if ((p) && (p)->msgtx)
    {
        (p)->msgtx(msg);
    }
}

void CsrTmBlueCoreTransportMsgRx(void *hdl, void *msg)
{
    struct CsrTmBlueCoreTransport *p = hdl;
    if ((p) && (p)->msgrx)
    {
        (p)->msgrx(msg);
    }
}

void CsrTmBlueCoreTransportRegisterMsgRx(void *hdl, void (*msgrx)(void *msg))
{
    struct CsrTmBlueCoreTransport *p = hdl;
    if ((p) && (msgrx))
    {
        (p)->msgrx = msgrx;
    }
}

void CsrTmBlueCoreTransportScoTx(void *hdl, void *scoData)
{
    struct CsrTmBlueCoreTransport *p = hdl;
    if ((p) && (p)->scotx)
    {
        (p)->scotx(scoData);
    }
}

CsrBool CsrTmBlueCoreTransportDriverRestart(void *hdl, CsrUint8 baudId)
{
    struct CsrTmBlueCoreTransport *p = hdl;
    CsrBool res = FALSE;
    if ((p) && (p)->driverRestart)
    {
        res = (p)->driverRestart(baudId);
        p->started = res;
    }
    return res;
}

#ifndef CSR_TARGET_PRODUCT_WEARABLE
void CsrTmBlueCoreTransportRestart(void *hdl)
{
    struct CsrTmBlueCoreTransport *p = hdl;
    if ((p) && (p)->restart)
    {
        (p)->restart();
    }
}
#endif /* !CSR_TARGET_PRODUCT_WEARABLE */

void CsrTmBlueCoreTransportClose(void *hdl)
{
    struct CsrTmBlueCoreTransport *p = hdl;
    if ((p) && (p)->close)
    {
        (p)->close();
    }
}

void CsrTmBlueCoreTransportIsoTx(void *hdl, void *isoData)
{
    struct CsrTmBlueCoreTransport *p = hdl;
    if ((p) && (p)->isotx)
    {
        (p)->isotx(isoData);
    }
}
