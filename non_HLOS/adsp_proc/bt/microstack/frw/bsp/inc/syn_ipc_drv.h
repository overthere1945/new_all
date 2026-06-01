/******************************************************************************
Copyright (c) 2020 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #2 $
******************************************************************************/
#ifndef SYN_IPC_DRV_H__
#define SYN_IPC_DRV_H__
    
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_sched.h"
#include "syn_ipc.h"
#include "syn_ipc_mem.h"

#ifdef __cplusplus
    extern "C" {
#endif

/*****************************************************************************

    NAME
        SynIpcDrvDataRx

    DESCRIPTION
        This callback type is used for the callback that is registerd in the
        SynIpcDrvRegister function and called by the SynIpcDrvRx function.
        Please refer to the description of these functions for further
        information.

    PARAMETERS
        data - pointer to the data.
        dataLength - the number of contiguous data bytes available.

    RETURNS
        The number of data bytes consumed from the data pointer.

*****************************************************************************/
typedef CsrUint32 (*SynIpcDrvDataRx)(const CsrUint8 *data, CsrUint32 dataLength);

typedef CsrUint8 SynIpcTransport;
#define SYN_IPC_TRANSPORT_HCI    (SynIpcTransport)1
#define SYN_IPC_TRANSPORT_CUSTOM (SynIpcTransport)2

/*****************************************************************************

    NAME
        SynIpcDrvOpen

    DESCRIPTION
        This function is called to open the IPC BT Host client and retrieve 
        a handle to it, which is subsequently used in functions that accept 
        a IPC handle.

    PARAMETERS
        trans - Use SYN_IPC_TRANSPORT_HCI to send/receive HCI messages
        with BTSS. SYN_IPC_TRANSPORT_CUSTOM to register so as to send/receive 
        custom messages with BTSS.

    RETURNS
        A IPC handle or NULL if unable to open the IPC BT Host client.
        NULL is considered an invalid handle and shall not be used 
        in any subsequent calls.

*****************************************************************************/
void *SynIpcDrvOpen(SynIpcTransport trans);

/*****************************************************************************

    NAME
        SynIpcDrvClose

    DESCRIPTION
        This function is called to close the IPC instance that was previously
        opened with SynIpcDrvOpen. When the call returns, the handle is no
        longer valid and shall not be used in any subsequent calls.

        SynIpcDrvStop must be called before calling this function, if the
        IPC driver is in the running state (i.e. SynIpcDrvStart has been
        called).

    PARAMETERS
        handle - handle of the IPC driver.

*****************************************************************************/
void SynIpcDrvClose(void *handle);

/*****************************************************************************

    NAME
        SynIpcDrvStart

    DESCRIPTION
        This function is called to start the IPC driver.

    PARAMETERS
        handle - handle of the IPC driver.

    RETURNS
        TRUE if the IPC driver was successfully started and FALSE otherwise.

*****************************************************************************/
CsrBool SynIpcDrvStart(void *handle);

 /*****************************************************************************

    NAME
        SynIpcDrvSetBtRadioOnState

    DESCRIPTION
        This function is called to notify the IPC driver of the state of BTSS.

        During BT ON this function is called with TRUE after the transport is initialized AND BTSS is
        ON, and during BT OFF usecase, this function is called with FALSE after BTSS is off and before 
        the transport is deinitialized.

    PARAMETERS
        btRadioState - state of the BT radio

    RETURNS
        none

*****************************************************************************/
void SynIpcDrvSetBtRadioOnState(CsrBool btRadioState);
 

/*****************************************************************************

    NAME
        SynIpcDrvStop

    DESCRIPTION
        This function is called to stop the IPC driver. It undoes all
        actions performed by SynIpcDrvStart, freeing any allocated resources.

        This function must be called before calling SynIpcDrvClose, if the
        driver is in the running state (i.e. SynIpcDrvStart has been
        called)

    PARAMETERS
        handle - handle of the IPC driver.

    RETURNS
        TRUE if IPC driver was successfully stopped and FALSE otherwise.

*****************************************************************************/
CsrBool SynIpcDrvStop(void *handle);

/*****************************************************************************

    NAME
        SynIpcDrvAlloc

    DESCRIPTION
        This function allocates memory to which the app could fill the
        data.

    PARAMETERS
        size   - total amount of memory to be allocated.

    RETURNS
        Pointer to requested memory. NULL if the allocation failed.

*****************************************************************************/
CsrUint8* SynIpcDrvAlloc(CsrUint16 size);

/*****************************************************************************

    NAME
        SynIpcDrvTx

    DESCRIPTION
        Buffer the supplied data for transmission. If the free space in the
        transmit buffer is less than length of the specified data, the
        implementation shall either attempt to buffer as many bytes as
        possible and return the actual number of bytes buffered in the numSent
        variable, or simply set numSent to 0 and in both cases return FALSE.

    PARAMETERS
        handle - handle of the IPC driver.
        msgId - type of message. Refer SynIpcMsgId for details.
        data - pointer to data.
        dataLength - number of bytes to buffer for transmission.

    RETURNS
        Returns TRUE if all the supplied data has been buffered (numSent is
        equal to dataLength), else return FALSE.

*****************************************************************************/
CsrBool SynIpcDrvTx(void *handle, SynIpcMsgId msgId, const CsrUint8 *data, CsrUint32 dataLength);

typedef SynIpcMemReadCallback SynIpcDrvReadCallback;

/*****************************************************************************

    NAME
        SynIpcDrvLowLevelTransportRx

    DESCRIPTION
        Invocation of this will provide one complete packet received from the 
        ipc via the the callback.

    PARAMETERS
        handle - handle of the IPC driver.
        context - application specific context.
        cb - callback through which one complete buffer will be delivered to app.

    RETURNS
        None.

*****************************************************************************/
void SynIpcDrvLowLevelTransportRx(void *handle, void *context, SynIpcDrvReadCallback cb);

/*****************************************************************************

    NAME
        SynIpcDrvRegister

    DESCRIPTION
        Register a callback function and background interrupt handle to
        provide the IPC driver a means for indicating reception of data to
        the upper layer.

    PARAMETERS
        handle - handle of the IPC device.
         rxBgintHandle - this background interrupt is triggered when data is
            available in the receive buffer.
        txBgintHandle - this background interrupt is triggered when ipc
            acknowledged the previous packet which was sent to btss.
        customTxBgintHandle - this background interrupt is triggered when ack is
            received for the previously sent custom packet.

*****************************************************************************/
void SynIpcDrvRegister(void *handle,  
                       CsrSchedBgint rxBgintHandle, 
                       CsrSchedBgint txBgintHandle,
                       CsrSchedBgint customTxBgintHandle);
    



#ifdef __cplusplus
    }
#endif
    
#endif

