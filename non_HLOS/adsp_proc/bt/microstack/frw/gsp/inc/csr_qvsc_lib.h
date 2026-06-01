#ifndef CSR_QVSC_LIB_H__
#define CSR_QVSC_LIB_H__
/******************************************************************************
 Copyright (c) 2016-2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_qvsc_prim.h"

#ifdef __cplusplus
extern "C" {
#endif


void CsrQvscMsgTransport(void *msg);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrQvscReqSend
 *
 *  DESCRIPTION
 *      Send a QCA Vendor Specific Command.
 *  PARAMETERS
 *      phandle       : The identity of the calling process. This must be set to
 *                      CSR_SCHED_QID_INVALID if no response is expected from
 *                      controller. Else it must be set to handle of calling task.
 *      ocf           : A 10 bit value. Range from 0x0000 till 0x03FF.
 *                      Refer the Chip specific HCI VSC document for list 
 *                      of commands and their associated OCF value.
 *                      Note: For vendor specific commands OGF is fixed to 
 *                            0x3f (6bits) as per the Bluetooth specification.
 *      payloadLength : The total length of the message measured in 8-bit integers.
 *      *payload      : The QVSC command payload.
 *----------------------------------------------------------------------------*/
void CsrQvscReqSend(CsrSchedQid phandle,
                    CsrUint16 ocf,
                    CsrUint16 payloadLength,
                    CsrUint8 *payload);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrQvscReqSend
 *
 *  DESCRIPTION
 *      Send a QCA Vendor Specific Command.
 *  PARAMETERS
 *      phandle       : The identity of the calling process. This must be set to
 *                      CSR_SCHED_QID_INVALID if no response is expected from
 *                      controller. Else it must be set to handle of calling task.
 *      ocf           : A 12 bit value. Range from 0x0000 till 0x0FFF.
 *                      Refer the Chip specific HCI VSC document for list
 *                      of commands and their associated OCF value.
 *                      Note: For vendor specific commands OGF is fixed to
 *                            0x3f (6bits) as per the Bluetooth specification.
 *      *payload      : M-block containing payload of QVSC command.
 *----------------------------------------------------------------------------*/
void CsrQvscMblkReqSend(CsrSchedQid phandle, CsrUint16 ocf, CsrMblk *payload);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrQvscSubscribeReqSend
 *
 *  DESCRIPTION
 *      Send a subscription request for HCI Event.
 *      As a confirmation calling process will receive
 *      CSR_QVSC_SUBSCRIBE_CFM with subscription ID. The Subcription ID can 
 *      subsequently used for unsubscription.
 *
 *      The HCI events are monitored and when the event data matches 
 *      the subscription data pattern, event is forwarded as CSR_QVSC_EVENT_IND 
 *      to subscribed process.
 *
 *  PARAMETERS
 *      phandle        : The identity of the calling process. This must be set 
 *                       to handle of calling task.
 *
 *      patternLen    : Length of subscription pattern. Range 0x01 - 0xFF.
 *
 *      *pattern      : The subscription data pattern.
 *----------------------------------------------------------------------------*/
void CsrQvscSubscribeReqSend(CsrSchedQid phandle,
                             CsrUint8 patternLen,
                             CsrUint8 *pattern);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrQvscUnsubscribeReqSend
 *
 *  DESCRIPTION
 *      Send a request to unsubscribe previously subscribed HCI event.
 *      The subscription data is removed and HCI event is not monitored.
 *
 *  PARAMETERS
 *      phandle        : The identity of the calling process. This must be set 
 *                       to handle of calling task.
 *
 *      subscriptionId : Subscription ID. Range 0x01 - 0xFF.
 *                       CSR_QVSC_SUBSCRIPTION_ID_INVALID can be used to unsusbcribe 
 *                       all the subscriptions for a particular phandle.
 *                       Subcription ID is unique for a phandle.
 *                  
 *----------------------------------------------------------------------------*/
void CsrQvscUnsubscribeReqSend(CsrSchedQid phandle, 
                               CsrQvscSubscrptionId subscriptionId);
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrQvscFreeUpstreamMessageContents
 *
 *  DESCRIPTION
 *      Deallocates the  payload in the QVSC upstream messages
 *
 *
 *    PARAMETERS
 *      eventClass :  Must be CSR_QVSC_PRIM,
 *      msg:          The message received from QVSC module
 *----------------------------------------------------------------------------*/
void CsrQvscFreeUpstreamMessageContents(CsrUint16 eventClass, void *message);

#ifdef __cplusplus
}
#endif

#endif

