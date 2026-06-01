/******************************************************************************
 Copyright (c) 2008-2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CSR_TM_BLUECORE_TRANSPORT_H__
#define CSR_TM_BLUECORE_TRANSPORT_H__

#include "csr_synergy.h"
#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Generic transport structure */
struct CsrTmBlueCoreTransport
{
#ifdef CSR_HYDRA_SSD
    CsrBool   (*start)(void *);
#else
    CsrBool   (*start)(void);
#endif
    CsrBool   (*stop)(void);
    CsrUint16 (*query)(void);
    void      (*msgtx)(void *msg);
    void      (*msgrx)(void *msg);
    void      (*scotx)(void *scoData);
    CsrBool   (*driverRestart)(CsrUint8 baudId);
    void      (*restart)(void);
    void      (*close)(void);
    CsrBool   started;
    void      (*isotx)(void *isoData);
};

/** Check if the transport is started
 *
 * @param hdl Transport handle
 *
 * @return TRUE or FALSE
 */
#define CSR_TM_BLUECORE_TRANSPORT_STARTED(hdl) (((struct CsrTmBlueCoreTransport *) (hdl))->started)

/** Delete a transport ** Currently does nothing **
 *
 * @param hdl Transport handle
 */
void CsrTmBlueCoreTransportDelete(void *hdl);

/** Start a transport
 *
 * @param hdl Transport handle
 * @param service_handle - handle to hydra service
 * @return TRUE on success; FALSE otherwise
 */
#ifndef CSR_HYDRA_SSD
CsrBool CsrTmBlueCoreTransportStart(void *hdl);
#else
CsrBool CsrTmBlueCoreTransportStart(void *hdl,void *service_handle);
#endif

/** Stop a transport
 *
 * @param hdl Transport handle
 *
 * @return TRUE on success; FALSE otherwise
 */
CsrBool CsrTmBlueCoreTransportStop(void *hdl);

/** Query a transport type
 *
 * @param hdl Transport handle
 *
 * @return Type of transport
 */
CsrUint16 CsrTmBlueCoreTransportQuery(void *hdl);

/** Send a message
 *
 * @param hdl Transport handle
 *
 * @param msg Message to send
 */
void CsrTmBlueCoreTransportMsgTx(void *hdl, void *msg);

/** Receive a message (upper layer call back)
 *
 * @param hdl Transport handle
 *
 * @param msg Message received by transport
 */
void CsrTmBlueCoreTransportMsgRx(void *hdl, void *msg);

/** Send sco data
 *
 * @param hdl Transport handle
 *
 * @param scoData Sco data to send
 */
void CsrTmBlueCoreTransportScoTx(void *hdl, void *scoData);

/** Send LE Isochronous HCI Packet
 *
 * @param hdl Transport handle
 *
 * @param isoData - pointer to complete iso hci packet
 */
void CsrTmBlueCoreTransportIsoTx(void *hdl, void *isoData);

void CsrTmBlueCoreTransportRegisterMsgRx(void *hdl, void (*msgrx)(void *msg));

CsrBool CsrTmBlueCoreTransportDriverRestart(void *hdl, CsrUint8 baudId);

void CsrTmBlueCoreTransportRestart(void *hdl);

void CsrTmBlueCoreTransportClose(void *hdl);

void CsrTmBlueCoreTransportInit(void **gash, void *blueCoreTransportDescriptor);

#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_BLUECORE_TRANSPORT_H__ */
