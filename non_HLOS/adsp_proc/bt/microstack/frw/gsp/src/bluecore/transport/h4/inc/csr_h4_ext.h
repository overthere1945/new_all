#ifndef CSR_H4_EXTENSION_H_
#define CSR_H4_EXTENSION_H_
/******************************************************************************
 Copyright (c) 2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_transport.h"


#ifdef __cplusplus
extern "C" {
#endif


/* H4 status */
typedef CsrUint8 CsrH4Status;
#define CSR_H4_STATUS_TX_QUEUE_EMPTY            ((CsrH4Status) 1)   /* Transmit queue is empty */
#define CSR_H4_STATUS_TX_QUEUE_READY            ((CsrH4Status) 2)   /* Transmit queue has HCI packet to send */
#define CSR_H4_STATUS_RX_HCI                    ((CsrH4Status) 3)   /* An HCI packet received from controller */


/* H4 sync loss */
typedef CsrUint8 CsrH4SyncLoss;
#define CSR_H4_SYNC_LOSS_TYPE_INVALID_HDR       ((CsrH4SyncLoss) 0)
#define CSR_H4_SYNC_LOSS_TYPE_HW_ERROR          ((CsrH4SyncLoss) 1)
#define CSR_H4_SYNC_LOSS_TYPE_READ_TIMEOUT      ((CsrH4SyncLoss) 2)
#define CSR_H4_SYNC_LOSS_NO_RESPONSE            ((CsrH4SyncLoss) 3)
#define CSR_H4_SYNC_LOSS_WAKE_FAIL              ((CsrH4SyncLoss) 4)


/* Extended data handler callback
 *
 * This callback function is called when H4 detects unknown H4 header.
 * The H4 extension protocol module must read data from writePtr.
 *
 * If protocol expects more data for the packet it must update the writePtr to
 * an appropriately sized payload holder and return the number of bytes for H4
 * to read.
 * In such case, H4 would read requested number of bytes into writePtr and call
 * this function again.
 *
 * When protocol does not expects more data for the packet, it must return 0.
 * At this point H4 would go back to reading H4 packets, until it reads another
 * unknown H4 header.
 *
 * Extension protocol may choose to call Sync loss handler if received bytes are
 * not in expected format.
 *
 * Note: First unknown byte from controller, which may possibly be extension
 * protocol data, is not passed to the extension protocol when Synergy is
 * configured to ignore stray bytes (i.e. when CSR_H4_ALLOW_STRAY_BYTES is set
 * to a value other than 0).
 *
 * Arguments:
 *      inst        - [IN] Instance of extension protocol.
 *      writePtr    - [IN, OUT] Double pointer to the buffer holding read
 *                    extended bytes. Memory pointed to by *writePtr must not be
 *                    released or modified.
 *
 * Returns: Number of bytes, further, expected by extension protocol.
 * */
typedef CsrUint16 (*CsrH4RxExtDataHandler)(void *inst, CsrUint8 * const *writePtr);


/* H4 status handler callback
 *
 * H4 calls this callback to inform extension module to inform of its restart
 * and TX queue status.
 *
 * Arguments:
 *      inst        - [IN] Instance of extension protocol.
 *      status      - [IN] H4 status.
 *      opaque      - [IN] Data accompanying status, if any.
 * */
typedef void (*CsrH4StatusHandler)(void *inst, CsrH4Status status, void *opaque);


/* UART clock vote
 *
 * Extension protocols inform H4 of its power state using this function.
 * If supported by platform, H4 may put UART clock into low power state.
 *
 * Arguments:
 *      powerState  - [IN] Power state to set.
 *
 * Returns: TRUE if power state change was successful, else FALSE.
 * */
extern CsrBool CsrH4SetPowerState(CsrBool powerState);


/* Start transmission
 *
 * Function to initiate transmission of queued H4 packets to the controller.
 * Extension protocol must call this function when it wants to allow transmission
 * of H4 packets.
 *
 * Once initiated, H4 transmits all the queued data. At the end of transmissions
 * H4 updates the extension protocol with EMPTY tx queue status. See CsrH4TxQueueStatusHandler
 * */
extern void CsrH4StartTransmit(void);


/* Send extension data
 *
 * Function to send extension protocol packets to controller. These packets are
 * prioritised over normal H4 packets. However it does not interrupt any
 * partially sent packet.
 * Also packets are queued behind any already queued packets belonging to same
 * channel. This makes sure that packets belonging to same channels are not
 * transmitted out of order.
 *
 * This function also takes care of initiating transmission, if not initiated
 * already. When calling this function, extension protocol is not required to
 * separately initiate transmission by calling CsrH4StartTransmit().
 *
 * Arguments:
 *      msg         - [IN] Extension data to transmit.
 * */
extern void CsrH4SendExtData(TXMSG *msg);


/* Synchronisation loss handler
 *
 * Synchronisation handler may be called when extension protocol detects
 * synchronisation loss from which it cannot recover.
 * Calling this function may result in panic depending upon H4 configuration.
 * Stray bytes may not always result in panic if user has specified a non-zero
 * value for CSR_H4_ALLOW_STRAY_BYTES.
 *
 * Arguments:
 *      reason      - [IN] Reason of synchronisation loss.
 * */
extern void CsrH4SyncLossHandler(CsrH4SyncLoss reason);


/* Register extension protocol
 *
 * This function registers extension protocol with H4. This function must be
 * called after initialization of H4 instance (CsrTmBlueCoreH4Init()) but before
 * H4 transport has been started by transport manager.
 *
 * Arguments:
 *      transportType   - [IN] Transport type of the extension protocol.
 *      inst            - [IN] Instance of extended protocol.
 *      rxHandler       - [IN] Callback function to receive extension data.
 *      statusHandler   - [IN] Callback function to receive H4 statuses.
 * */
extern void CsrH4RegisterExtension(CsrUint16 transportType,
                                   void *inst,
                                   CsrH4RxExtDataHandler rxHandler,
                                   CsrH4StatusHandler statusHandler);


#ifdef __cplusplus
}
#endif

#endif /* CSR_H4_EXTENSION_H_ */
