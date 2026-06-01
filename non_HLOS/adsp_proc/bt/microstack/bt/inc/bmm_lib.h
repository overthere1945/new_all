/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#ifndef __BMM_LIB_H__
#define __BMM_LIB_H__

#include "bluetooth.h"
#include "csr_prim_defs.h"
#include "bmm_prim.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 * API for getting Micro stack socket capabilities
 * Includes RFCOMM and LECOC capabilities
 * num of offloaded connections is set to zero if offload is not supported
 * for a specific protocol in Microstack
 * Offload framework can tune the frame size/mtu and report this to primary stack
 * based on platform memory.
******************************************************************************/
BmmResultCode BmmSocketGetCapabilities(BmmSocketCapabilities *cap);


/******************************************************************************
 * API for subscribing events from Micro stack by setting appropriate event mask
 * Micro stack returns various events based on the subscription done from the app.
 * E.g. if an application is subscribed to BMM_EVENT_MASK_SUBSCRIBE_INITIALIZED
 * it will receive BMM_INITIALIZED_IND, as and when the event takes place.  
 * Micro stack returns BMM_SET_EVENT_MASK_CFM once events are configured
 * NOTE :
 * 1. Set eventMask to zero to disable all the events
 * 2. Application is expected to enable all required masks in one go
******************************************************************************/
void BmmSetEventMaskReqSend(BmmSchedQid pHandle, BmmEventMask eventMask);


/******************************************************************************
 * API for offloading RFCOMM socket to Micro stack
 * This should be sent once RFCOMM offloaded connection is successful
 * Micro stack returns BMM_SOCKET_RFCOMM_OFFLOAD_CFM once socket is configured
 * Micro stack generates connId if offload procedure is successful
 * All further communication on this channel is via connId
 * 
 * NOTE :
 * 1. If offload framework is not able to open the socket with offload app, 
 *    then Offload framework is expected to send Close Socket to Micro stack
 * 2. Primary stack is responsible for setting initialRxCredits to zero as part
 *    of RFCOMM connection procedure for offload channel
 * 3. App is expected to issue set socket parameters(credits) once offload app
 *    is successfully configured. This enables Rx data path.
 * 4. Mux initiator is set to 1 if primary stack initiated Multiplexer session 
 *    for RFCOMM, otherwise set to 0
 * 5. Offload framework is expected to send valid context params
 * 6. Context received in REQ is returned in CFM
******************************************************************************/
void BmmSocketRfcommOffloadReqSend(BmmSocketId   socketId,
                                   BmmSchedQid   pHandle,
                                   BmmHciHandle  hciHandle, 
                                   BmmL2capCid   localCid,
                                   BmmL2capCid   remoteCid,
                                   BmmL2capMtu   localMtu,
                                   BmmL2capMtu   remoteMtu,
                                   uint8         dlci,
                                   uint16        initialRxCredits,
                                   uint16        initialTxCredits,
                                   uint16        maxFrameSize,
                                   uint16        muxInitiator,
                                   BmmContext    context);

/******************************************************************************
 * API for offloading LECOC socket to Micro stack
 * This should be sent once LECOC offloaded connection is successful
 * Micro stack returns BMM_SOCKET_LECOC_OFFLOAD_CFM once socket is configured
 * Micro stack generates connId if offload procedure is successful
 * All further communication on this channel is via connId
 * 
 * NOTE :
 * 1. If offload framework is not able to open the socket with offload app, 
 *    then Offload framework is expected to send Close Socket to Micro stack
 * 2. Primary stack is responsible for setting initialRxCredits to zero as part
 *    of LECOC connection procedure for offload channel
 * 3. App is expected to issue set socket parameters(credits) once offload app
 *    is successfully configured
 * 4. If remote device sends large MTU/MPS, Offload framework should cap these 
 *    values to avoid memory issues in Tx path.
 * 5. Offload framework is expected to send valid context params
 * 6. Context received in REQ is returned in CFM
******************************************************************************/
void BmmSocketLecocOffloadReqSend(BmmSocketId   socketId,
                                  BmmSchedQid   pHandle,
                                  BmmHciHandle  hciHandle, 
                                  BmmL2capCid   localCid,
                                  BmmL2capCid   remoteCid,
                                  BmmL2capMtu   localMtu,
                                  BmmL2capMtu   remoteMtu,
                                  BmmL2capMps   localMps,
                                  BmmL2capMps   remoteMps,
                                  uint16        initialRxCredits,
                                  uint16        initialTxCredits,
                                  BmmContext    context);

/******************************************************************************
 * API for closing LECOC/RFCOMM socket to Micro stack
 * Micro stack releases the socket and its resources
 * Any pending data request will be rejected with failure
 * Micro stack returns BMM_SOCKET_CLOSE_CFM
******************************************************************************/
void BmmSocketCloseReqSend(BmmConnId connId, BmmSchedQid pHandle);


/******************************************************************************
 * API for setting socket parameters for offloaded socket
 * param identifies the configurable option 
 * value is the value set for the param
 * Micro stack returns BMM_SOCKET_SET_PARAMS_CFM
 * 
 * NOTE :
 * 1. BMM_SOCK_PREFERRED_RX_CREDITS is used for releasing the credits to remote 
 *    device. This is preferred Rx credit, Micro stack can override this value 
 *    This can be called once offload app is initialised after open socket.
 *    This enables Rx data path
******************************************************************************/
void BmmSocketSetParamsReqSend(BmmConnId      connId, 
                               BmmSchedQid    pHandle,
                               BmmSocketParam param,
                               uint32         value,
                               BmmContext     context);


/******************************************************************************
 * API for sending data on offloaded LECOC/RFCOMM socket
 * 
 * NOTE :
 * 1. Application should not send data more than configured max frame size/MTU
 * 2. Application needs to wait for CFM before sending next data
 * 3. data will be freed by Micro Stack.
 * 4. Context received in REQ is sent back in BMM_SOCKET_DATA_CFM
******************************************************************************/
void BmmSocketDataReqSend(BmmConnId    connId,
                          BmmSchedQid  pHandle,    
                          uint16       dataLen,
                          uint8        *data,
                          BmmContext   context);


/******************************************************************************
 * API for sending response to incoming data on offloaded RFCOMM/LECOC socket
 * 
 * NOTE :
 * 1. Context received in IND is sent back in RSP
******************************************************************************/
void BmmSocketDataRspSend(BmmConnId connId, BmmContext context);


/******************************************************************************
  API to prepare Micro stack before stopping. This API needs to be called
  before calling the actual stop. this function can be called in the following scenarios
  case a) when a user explicitly requests a BT OFF, in which case the application needs to
  deinitialize any operations (disconnection of active links, deactivation of registered
  services, deregister profiles, active inquiry/scans, advertising etc) that are already
  in progress before this API is called by the application.
  Application needs to deactivate, deregister profiles etc before calling MicroStackStop.

  This API can be called with 3 different reasons :
      1. BMM_STOP_REASON_SSR  - For SSR case
      2. BMM_STOP_REASON_USER - For user enforced stop
      3. BMM_STOP_REASON_FMD  - For FMD case


  Micro stack returns BMM_PREPARE_STOP_CFM with following possible reasons :
      1. BMM_RESULT_SUCCESS      - Success case
      2. BMM_RESULT_STOP_FAILURE - Failure in prepare stop

  **NOTE: the application shouldn't call any BT APIs after this.
******************************************************************************/
void BmmPrepareStopReqSend(BmmSchedQid     pHandle,
                           BmmStopReason   reason);

/******************************************************************************
 * API for freeing contents of all BMM primitives
******************************************************************************/
void BmmFreePrimitive(void *msg);

#ifdef __cplusplus
}
#endif

#endif /* __BSM_LIB_H__ */

