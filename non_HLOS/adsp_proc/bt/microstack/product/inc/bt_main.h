/******************************************************************************
 Copyright (c) 2021 - 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#ifndef BT_MAIN_H__
#define BT_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_synergy.h"
#include <stdint.h>
#include "qurt_thread.h"
#ifdef ENABLE_EVENT_COUNTER_REPORTING
#include "bluetooth.h"
#endif

typedef uint16_t MicrostackConfig;
/* Bit mask to control the Microstack settings */
/* Bit 0 : set to 1 for passthrough mode, otherwise offload mode */
#define MICROSTACK_CONFIG_PASSTHROUGH_MODE_MASK    (MicrostackConfig)0x01
#define BTHOST_CONFIG_OFFLOAD_MODE_MASK            (MicrostackConfig)0x00

/* BTHOST LOGGING HCI PKT FLAG -  Bit 1: (Enable Snoop log) 
                                  Bit 2: (Enable enhanced logging) 
                                  Bit 3: (Enable logging of complete payload of ACL 
                                  data else the length will be truncated to 
                                  MAX_PAYLOAD_LEN_TO_INCLUDE) 
 * the other bits will be ignored. 
 * Note: Bit 2 and Bit 3 are only applicable if STANDALONE_DUT_PLUS flag is defined.
 */
#define BTHOST_HCI_ENABLE_SNOOP_LOGGING        (uint16_t)0x01
#define BTHOST_HCI_ACL_RX_ENH_PKT_FLAG         (uint16_t)0x02
#define BTHOST_HCI_ACL_RX_INC_FULL_PAYLOAD_LEN (uint16_t)0x04

/* Application Handle is the protocol handle which uniquely identifies the application.
 * 
 * Application registers a callback CB() for handle X using MicroStackRegisterAppCb(X, CB)
 * App callback CB() is called for any response/unsolicited events from BTHost.
 * 
 * Refer to individual module for the registration and the usage of handle. 
 * For e.g CsrBtGattRegister(X) is used by app for GATT registration.
 *
 */
#define BT_APP_HANDLE_START 0x7800     /*!< BT App Handle range, 0x7800 onwards */
typedef uint16_t BtAppHandle;


/* Event class/message group of Micro stack.
 * Refer csr_bt_profiles.h for supported eventclass. CSR_BT_XXX_PRIM format  
 */ 
typedef uint16_t BtEventClass;
#ifdef PATCH_NVM_BUFFER_DOWNLOAD
typedef enum
{
    OP_BT_PATCH_DOWNLOAD_COMPLETE,
    OP_BT_NVM_DOWNLOAD_COMPLETE,
    OP_BT_PATCH_DATA_GET,
    OP_BT_NVM_DATA_GET,
    OP_BT_DOWNLOAD_ERROR,
    OP_BT_BOOTSTRAP_COMPLETE
} bootstrap_callback_op;

typedef enum
{
    PATCH_DOWNLOAD_TYPE,
    NVM_DOWNLOAD_TYPE
} download_type;

typedef uint8_t patchNvmStatus;

#define BTHOST_DOWNLOAD_SUCCESS        (patchNvmStatus)0x00
#define BTHOST_DOWNLOAD_ERROR          (patchNvmStatus)0x01

typedef struct
{
    uint32_t lap;   /*!< Lower Address Part 00..23 */
    uint8_t  uap;   /*!< upper Address Part 24..31 */
    uint16_t nap;   /*!< Non-significant    32..47 */
} BT_BD_ADDR;
#endif

/******************************************************************************
  MicroStackStart: API to start Micro stack
******************************************************************************/
qurt_thread_t MicroStackStart(const uint8_t *threadName, uint16_t priority, MicrostackConfig config);


/******************************************************************************
  MicroStackStop: API to stop Micro stack.  As a result of this call, the application will receive 
  BMM_DEINITIALIZED_IND indication.once all the tasks have been deinitialzed.
  The application can then deregister the callback after receiving this event.
******************************************************************************/
void MicroStackStop(void);


/******************************************************************************
  MicroStackConfigureHciLogging: API to configure HCI logging. Refer the BTHOST_HCI_XXX flags 
  for configuration details above.
******************************************************************************/
void MicroStackConfigureHciLogging(uint16_t configFlags);


/******************************************************************************
 * Application callback function for response and other Bluetooth Stack events
 *
 * After processing the message, application is expected to free the memory by calling 
 * MicroStackFreePrimitive()
 *
 * Minimal functionality should be implemented in the callback without adding any latency.
 * Additional functionality which adds latency should be done in a separate thread.
 *
 * @param[out]  handle Application handle. 
 * @param[out]  eventClass  Primitive group
 * @param[out]  message pointer to received message
 ******************************************************************************/
typedef void (*BtHostAppCallback)(BtAppHandle handle, BtEventClass eventClass, void *message); 

/******************************************************************************
 * API for app callback registration
 *
 * Application can use this API to register a callback for an app handle.
 * This callback is called in response to a request and also unsolicited events from Blueooth Stack.
 * 
 * Application can register one or more callbacks with the Bluetooth stack.
 * 1. App can use one app handle to communicate with Bluetooth stack
 *    by registering a single callback for all the modules
 * 2. App can have module specific app handles e.g GATT 
 *    and then register a callback function for each app handle.  
 * 
 * Application is mandatorily expected to register atleast one callback.
 * For GATT based applications, application can use unique handle for each Gatt Register Req 
 * to register separate callback for each GattId.
 *
 * Returns TRUE if successful
 ******************************************************************************/
boolean MicroStackRegisterAppCb(BtAppHandle handle, BtHostAppCallback appCb);

/******************************************************************************
 * API for app deregistration
 *
 * Application can use this API to deregister the app callback.
 *
 ******************************************************************************/
void MicroStackDeregisterAppCb(BtAppHandle handle);


/******************************************************************************
 * API to free the memory for Bluetooth Stack events
 *
 * Application can use this API to free the received primitive and its associated contents.
 * If Application wants to use any pointers for further use, pointers inside the upstream 
 * message to be explicitly set to NULL before calling MicroStackFreePrimitive().
 *
 ******************************************************************************/
void MicroStackFreePrimitive(BtEventClass eventClass, void *message);


#ifdef PATCH_NVM_BUFFER_DOWNLOAD

/******************************************************************************
 * Callback function for getting the remaining patch or NVM payload. Client for the Patch and
 * NVM utility would have shared the total Patch and NVM file size as part of BtHostRegisterDownloadCB.
 * BT host will use this callback function to get the remaining patch and nvm data from the client.
 * This API can get called multiple times depending upon how much Patch and NVM payload is still
 * left to be downloaded based on the total Patch and NVM file sizes.
 * This function will also be used to indicate the result of operations and end of bootstrap procedure.
 *

 * Note : Buffer data should be multiple of 243 bytes (Max tlv segment length)
 *        except the last segment.
 *
 * @param  operation Operation type.
 * @param  offset  Data offset.
 * @param  bufferLength pointer to buffer length
 * @param  buffer double pointer to buffer data
 * @param  bufferToBeFreed pointer to a variable which tells if buffer is to be freed by Synergy BT Host
 * @param  status result of operation
 *
 * Relevance of parameters for each operation :
 *
 * OP_BT_PATCH_DATA_GET - All except status
 * OP_BT_NVM_DATA_GET - All except status
 * OP_BT_PATCH_DOWNLOAD_COMPLETE - Only operation
 * OP_BT_NVM_DOWNLOAD_COMPLETE - Only operation
 * OP_BT_DOWNLOAD_ERROR - Operation and status
 * OP_BT_BOOTSTRAP_COMPLETE - Only operation (Will always be sent to Patch Callback function)
 ******************************************************************************/

typedef boolean (*BT_HOST_PATCH_NVM_CB)(bootstrap_callback_op operation, size_t offset, size_t *bufferLength, uint8_t **buffer, uint8_t *bufferToBeFreed, patchNvmStatus status);

/******************************************************************************
 * Application can call this API to provide the full length of the patch/nvm file to be downloaded
 * and the corresponding callback function. The download will happen in segments.
 * BT host will call this callback function while starting the download procedure and the
 * application is expected to hand over first segment of the full file.
 * Once the first segments download is complete, this callback function will be triggered again
 * with OP_BT_XX_DOWNLOAD_GET to get the next segment from the application and so on
 * until the download is complete which will be indicated by OP_BT_XX_DOWNLOAD_COMPLETE.
 * In case of an error, OP_BT_DOWNLOAD_ERROR will be returned in the callback.
 *
 *  a) This should be called before MicroStackStart()
 *  b) This will be reset after MicroStackStop()
 *  c) NVM download is not applicable in FTM mode.
 *
 * Returns TRUE if successful
 ******************************************************************************/
boolean BtHostRegisterDownloadCB(download_type type, size_t length, BT_HOST_PATCH_NVM_CB patchNvmCb);

/******************************************************************************
 * API to set Bluetooth Device Address.
 *  a) This should be called before MicroStackStart()
 *  b) This will be reset after MicroStackStop()
 *  c) Not applicable in FTM mode
 *
 * Returns TRUE if successful
 ******************************************************************************/
boolean BtHostSetBDAddress(BT_BD_ADDR btDeviceAddr);

/**
 * Callback for notifying the HCI events in FTM mode.
 * Relevant only for FTM mode.
 *
 * @param   length  length of the hci event
 * @param   data    pointer to hci event message
 
 * Memory is owned by application in this case. 

 */
typedef void (*BT_HOST_FTM_HCI_CLIENT_CB)(uint16_t length, uint8_t *data);


/******************************************************************************
 * API to send HCI Commands when FTM mode is enabled.
 * Relevant only for FTM mode.
 *
 * @param  length length of the HCI command packet.
 * @param  payload  pointer to HCI command packet.
 *
 * Returns TRUE if successful
 *
 * Note : payload will be freed by Synergy.
 *
 ******************************************************************************/
boolean BtHostSendHciCmdFtmMode(uint16_t length, uint8_t *payload);

/******************************************************************************
 * API to enable FTM Mode and register callback for HCI message.
 *
 * @param  clientCb  callback function for handling HCI events in Ftm mode.
 *
 *  a) This should be called before MicroStackStart()
 *  b) This will be reset after MicroStackStop()
 *
 ******************************************************************************/
boolean BtHostSetFtmMode(BT_HOST_FTM_HCI_CLIENT_CB clientCb);
#endif

#ifdef __cplusplus
}
#endif

#endif
