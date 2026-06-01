/*****************************************************************************

      Copyright (c) 2025 Qualcomm Technologies International, Ltd.

      All Rights Reserved.

      Qualcomm Technologies International, Ltd. Confidential and Proprietary. 

*****************************************************************************/

#ifndef CSR_SERIAL_COM_H__
#define CSR_SERIAL_COM_H__

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_sched.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSR_TARGET_PRODUCT_WEARABLE
extern CsrUint8 txBlock;
#endif

/*****************************************************************************

    NAME
        CsrUartDrvDataRx

    DESCRIPTION
        This callback type is used for the callback that is registerd in the
        CsrUartDrvRegister function and called by the CsrUartDrvRx function.
        Please refer to the description of these functions for further
        information.

    PARAMETERS
        data - pointer to the data.
        dataLength - the number of contiguous data bytes available.

    RETURNS
        The number of data bytes consumed from the data pointer.

*****************************************************************************/
typedef CsrUint32 (*CsrUartDrvDataRx)(const CsrUint8 *data, CsrUint32 dataLength);


/*****************************************************************************

    NAME
        CsrUartDrvOpen

    DESCRIPTION
        This function is called to open the UART device and retrieve a handle
        to it, which is subsequently used in functions that accept a UART
        device handle.

    PARAMETERS
        device - a (platform dependent) string that identifies the UART device
            to open.
        baud - a pointer to a variable containing the desired baud rate to use.
            If the value specified is not supported by the UART device, the
            UART driver may, if possible, adjust the value to the nearest
            suitable rate (updating the baud variable accordingly), or, if
            possible, reject the request by returning NULL.
        dataBits - number of databit (valid values: 7 or 8).
        parity - parity bit: 0=None, 1=Odd, 2=Even.
        stopBits - number of stopbit (valid values: 1 or 2).
        flowControl - whether to enable hardware flow control (RTS/CTS).
        token - If this token is given, the UART driver shall only deliver
            data to the upper layer when this token is encountered in the RX
            datastream. This increases the efficieny by only triggering the
            upper layer processing when an entire data unit has been
            received. Pass NULL to disable this functionality.

    RETURNS
        A UART device handle or NULL if unable to open the specified device or
        if the arguments are invalid or unsupported. NULL is considered an
        invalid handle and shall not be used in any subsequent calls.

*****************************************************************************/
void *CsrUartDrvOpen(const CsrCharString *device,
                     CsrUint32 *baud,
                     CsrUint8 dataBits,
                     CsrUint8 parity,
                     CsrUint8 stopBits,
                     CsrBool flowControl,
                     const CsrUint8 *token);


/*****************************************************************************

    NAME
        CsrUartDrvClose

    DESCRIPTION
        This function is called to close the UART device that was previously
        opened with CsrUartDrvOpen. When the call returns, the handle is no
        longer valid and shall not be used in any subsequent calls.

        CsrUartDrvStop must be called before calling this function, if the
        UART driver is in the running state (i.e. CsrUartDrvStart has been
        called).

    PARAMETERS
        handle - handle of the UART device.

*****************************************************************************/
void CsrUartDrvClose(void *handle);

#define CSR_UART_BAUD_RATE_IGNORE   0xFF                        /* No change in baud rate */
#define CSR_UART_BAUD_RATE_COUNT    19                          /* The total number of available baud rates which are valid */
#define CSR_UART_BAUD_RATE_INVALID  0                           /* Invalid baud rate */
#define CSR_UART_BAUD_RATE_DEFAULT  115200                      /* The default baud rate */
#define CSR_UART_BAUD_ID_INVALID    CSR_UART_BAUD_RATE_COUNT    /* Invalid baud id */

/*****************************************************************************

    NAME
        CsrUartDrvGetBaudrateFromBaudId

    DESCRIPTION
        Obtain the baud rate from its corresponding baud id.

    PARAMETERS
        baudId - The baud id of the UART device.

    RETURNS
        The corresponding baud rate of the given baud id.
        Otherwise returns CSR_UART_BAUD_RATE_INVALID for an invalid baud id.

*****************************************************************************/
CsrUint32 CsrUartDrvGetBaudrateFromBaudId(CsrUint8 baudId);


/*****************************************************************************

    NAME
        CsrUartDrvGetBaudIdFromBaudrate

    DESCRIPTION
        Obtain the baud id from its corresponding baud rate.

    PARAMETERS
        baudrate - The baud rate of the UART device.

    RETURNS
        The corresponding baud id of the given baud rate.
        Otherwise returns CSR_UART_BAUD_ID_INVALID for an invalid baud rate.

*****************************************************************************/
CsrUint8 CsrUartDrvGetBaudIdFromBaudrate(CsrUint32 baudrate);


/*****************************************************************************

    NAME
        CsrUartDrvStart

    DESCRIPTION
        This function is called to start the UART driver.

        If hardware flow control is disabled, the state of the RTS pin is
        unknown on return, and should be set by calling CsrUartDrvRtsSet if
        needed.

    PARAMETERS
        handle - handle of the UART device.
        baudId - the baud id. This can be used by the UART driver
                 to determine the baud rate to use.

    RETURNS
        TRUE if the UART driver was successfully started and FALSE otherwise.

*****************************************************************************/
CsrBool CsrUartDrvStart(void *handle, CsrUint8 baudId);


/*****************************************************************************

    NAME
        CsrUartDrvStop

    DESCRIPTION
        This function is called to stop (close) the UART driver. It undoes all
        actions performed by CsrUartDrvStart, freeing any allocated resources.

        This function must be called before calling CsrUartDrvClose, if the
        UART driver is in the running state (i.e. CsrUartDrvStart has been
        called)

    PARAMETERS
        handle - handle of the UART device.

    RETURNS
        TRUE if UART driver was successfully stopped and FALSE otherwise.

*****************************************************************************/
CsrBool CsrUartDrvStop(void *handle);


/*****************************************************************************

    NAME
        CsrUartDrvRtsSet

    DESCRIPTION
        This function is called to manually set the output level of the RTS
        signal when hardware flow control is disabled.

        It can only be called when the UART driver is in the running state,
        i.e. after CsrUartDrvStart has been called and before CsrUartDrvStop
        has been called. Behaviour is undefined if this function is called
        on a UART device where hardware flow control is enabled.

        When hardware flow control is disabled, the state of the RTS pin is
        unknown when CsrUartDrvStart returns. This function should be called
        to set a specific state if needed.

        If hardware flow control is disabled and CsrUartDrvReconfigure is
        called without enabling hardware flow control, the state of the RTS pin
        shall be preserved as set by CsrUartDrvRtsSet. In all other cases the
        state of the RTS pin is unknown on return from CsrUartDrvReconfigure,
        and this function should be called to set a specific state if needed.

    PARAMETERS
        handle - handle of the UART device.
        state - determines whether the pin should be asserted (TRUE) or
            negated (FALSE). Specifically:

                +-------+--------+-----+-------------+
                | state | Logic  | TTL | RS232       |
                +-------+--------+-----+-------------+
                | TRUE  | 1/HIGH | 5V  | -3V to -25V |
                | FALSE | 0/LOW  | 0V  | +3V to +25V |
                +-------+--------+-----+-------------+

*****************************************************************************/
void CsrUartDrvRtsSet(void *handle, CsrBool state);


/*****************************************************************************

    NAME
        CsrUartDrvGetTxSpace

    DESCRIPTION
        Obtain amount of available free space in the UART driver transmit
        buffer. Should be able to buffer at least this number of bytes in a
        call to CsrUartDrvTx.

    PARAMETERS
        handle - handle of the UART device.

    RETURNS
        The number of bytes that can be written to the UART driver transmit
        buffer.

*****************************************************************************/
CsrUint32 CsrUartDrvGetTxSpace(void *handle);


/*****************************************************************************

    NAME
        CsrUartDrvTx

    DESCRIPTION
        Buffer the supplied data for transmission. If the free space in the
        transmit buffer is less than length of the specified data, the
        implementation shall either attempt to buffer as many bytes as
        possible and return the actual number of bytes buffered in the numSent
        variable, or simply set numSent to 0 and in both cases return FALSE.

    PARAMETERS
        handle - handle of the UART device.
        data - pointer to data.
        dataLength - number of bytes to buffer for transmission.
        numSent - the actual number of bytes buffered is returned in this
            variable.

    RETURNS
        Returns TRUE if all the supplied data has been buffered (numSent is
        equal to dataLength), else return FALSE.

*****************************************************************************/
CsrBool CsrUartDrvTx(void *handle, const CsrUint8 *data, CsrUint32 dataLength, CsrUint32 *numSent);


/*****************************************************************************

    NAME
        CsrUartDrvTxDrain

    DESCRIPTION
        Waits till tx data has been transmitted.

    PARAMETERS
        handle - handle of the UART device.

    RETURNS
        TRUE if successful, else FALSE.

*****************************************************************************/
CsrBool CsrUartDrvTxDrain(void *handle);


/*****************************************************************************

    NAME
        CsrUartDrvGetRxAvailable

    DESCRIPTION
        Obtain amount of data available in the UART driver receive buffer.
        Should be able to receive at least this number of bytes in a call to
        the CsrUartDrvRx or CsrUartDrvLowLevelTransportRx function.

    PARAMETERS
        handle - handle of the UART device.

    RETURNS
        The number of bytes that can be read from the UART driver receive
        buffer.

*****************************************************************************/
CsrUint32 CsrUartDrvGetRxAvailable(void *handle);


/*****************************************************************************

    NAME
        CsrUartDrvRx

    DESCRIPTION
        Calling this function will in turn cause the CsrUartDrvDataRx callback
        function registered in the call to CsrUartDrvRegister to be called.
        The CsrUartDrvDataRx callback function passes a pointer (and length)
        to the upper layer. The upper layer may either process this data
        in-place or copy it to a local buffer for later processing. The return
        value from the CsrUartDrvDataRx function indicates the number of bytes
        that was consumed (regardles of whether it was processed or copied) by
        the upper layer.

    PARAMETERS
        handle - handle of the UART device.

*****************************************************************************/
void CsrUartDrvRx(void *handle);


/*****************************************************************************

    NAME
        CsrUartDrvLowLevelTransportRx

    DESCRIPTION
        Copies up to the specified number of bytes from the receive buffer to
        the supplied data buffer.

    PARAMETERS
        handle - handle of the UART device.
        dataLength - the number of byte to retrieve.
        data - buffer to copy the data into.

    RETURNS
        The number of bytes actually copied. Note that this may be less than
        the length of the supplied buffer.

*****************************************************************************/
CsrUint32 CsrUartDrvLowLevelTransportRx(void *handle, CsrUint32 dataLength, CsrUint8 *data);


/*****************************************************************************

    NAME
        CsrUartDrvLowLevelTransportTxBufLevel

    DESCRIPTION
        Retrieve the number of bytes in the transmit buffer waiting to be
        transmitted.

    PARAMETERS
        handle - handle of the UART device.

    RETURNS
        The number of bytes in the transmit buffer waiting to be transmitted.

*****************************************************************************/
CsrUint32 CsrUartDrvLowLevelTransportTxBufLevel(void *handle);


/*****************************************************************************

    NAME
        CsrUartDrvGetBaudrate

    DESCRIPTION
        Retrieve the currently configured baud rate.

    PARAMETERS
        handle - handle of the UART device.

    RETURNS
        The currently configured baud rate.

*****************************************************************************/
CsrUint32 CsrUartDrvGetBaudrate(void *handle);


/*****************************************************************************

    NAME
        CsrUartDrvRegister

    DESCRIPTION
        Register a callback function and background interrupt handle to
        provide the UART driver a means for indicating reception of data to
        the upper layer.

    PARAMETERS
        handle - handle of the UART device.
        rxDataFn - this function is called when the upper layer calls the
            CsrUartDrvRx function to receive data.
        rxBgintHandle - this background interrupt is triggered when data is
            available in the receive buffer.

*****************************************************************************/
void CsrUartDrvRegister(void *handle, CsrUartDrvDataRx rxDataFn, CsrSchedBgint rxBgintHandle);

/*****************************************************************************

    NAME
        CsrUartDrvRegisterTx

    DESCRIPTION
        Register a background interrupt handle to provide the UART driver a
        means for indicating transmission of data to the upper layer.

    PARAMETERS
        handle - handle of the UART device.
        txBgintHandle - this background interrupt is triggered when data is
            sent down from UART.

*****************************************************************************/
void CsrUartDrvRegisterTx(void *handle, CsrSchedBgint txBgintHandle);


CsrBool CsrUartDrvSetPowerMode(void *handle, CsrBool enable);

#ifdef __cplusplus
}
#endif

#endif
