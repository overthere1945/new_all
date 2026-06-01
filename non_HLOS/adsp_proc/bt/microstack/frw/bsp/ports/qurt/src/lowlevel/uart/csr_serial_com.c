/******************************************************************************
 Copyright (c) 2025-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_macro.h"
#include "csr_serial_com.h"
#include "csr_transport.h"
#include "csr_util.h"
#include "csr_log_text_2.h"

#include "qurt_thread.h"
#include "qurt_signal.h"
#include "qurt_mutex.h"
#include "uart.h"
#include "pram_mgr.h"
#ifdef UART_BUILD_V1
#include "uClock.h"
#endif
#include "csr_framework_ext.h"

#ifndef UART_BUILD_V1
#define ENABLE_SSC_GDSC_VOTE
#endif

#ifdef ENABLE_SSC_GDSC_VOTE
#include "Clock.h"
#endif

CsrSchedBgint rxBgint = CSR_SCHED_BGINT_INVALID;
CsrSchedBgint txBgint = CSR_SCHED_BGINT_INVALID;
CsrUartDrvDataRx rxData = NULL;

#define RX_CHUNK_SIZE 1024
#define TX_BUFFER_SIZE 4096

#ifdef ENABLE_SSC_GDSC_VOTE
#define BT_CORE_POWER_CLIENT_NAME     "bt_core_power"

#define CLOCK_HANDLE_INVALID   ((ClockHandle)0)           /* null pointer cast */
#define CLOCK_ID_INVALID       ((ClockIdType)0xFFFFFFFFu) /* invalid ID */
#endif

/* Log Text */
CSR_LOG_TEXT_HANDLE_DEFINE(CsrSerialComLto);

typedef struct {
    uart_handle uartHandle;
    CsrUint8 *tx_buffer;
    CsrUint32 tx_size;

    uart_descriptor tx_desc_local;
    CsrUint32 bytes_sent_total;

    CsrUint8 *rx_buffer;
    CsrUint32 rx_size;
    CsrUint32 rx_in;
    CsrUint32 rx_out;

    CsrUint32 bytes_received;

    CsrMutexHandle tx_mutex;
    CsrMutexHandle rx_mutex;
} uart_context;

uart_context uart_ctx;


typedef struct {
    uart_result result;
    CsrBool enable;
} uart_power_api_status;

uart_power_api_status  power_api_status;

#ifdef ENABLE_SSC_GDSC_VOTE

typedef struct bt_core_power_cfg_int
{
  ClockHandle handle;
  ClockIdType id;

} bt_core_power_cfg_int;

static bt_core_power_cfg_int bt_core_power_int = {
    .handle = CLOCK_HANDLE_INVALID,
    .id     = CLOCK_ID_INVALID,
};

#define BT_CORE_POWER_DOMAIN "lpass_aon_cc_lpass_ssc_gdsc"

#endif

uart_result openRet = -1;
uart_result receiveRet = -1;
uart_result transmitRet = -1;

CsrUint8 *psram_buffer = NULL;
CsrUint32 buffer_size = 0;
CsrUint8 uclockResult;
CsrUint8 *globalTxBuffer;

CsrBool rxQueued = FALSE;
CsrBool uartPowerModeOn;

CsrUint8 txConsecutiveFailureCount;

static CsrUint32 get_rx_free_space(void)
{
    if (uart_ctx.rx_in >= uart_ctx.rx_out)
    {
        return uart_ctx.rx_size - (uart_ctx.rx_in - uart_ctx.rx_out) - 1;
    }
    return uart_ctx.rx_out - uart_ctx.rx_in - 1;
}

static void advance_rx_in(CsrUint32 length)
{
    uart_ctx.rx_in = (uart_ctx.rx_in + length) % uart_ctx.rx_size;
}

static void advance_rx_out(CsrUint32 length)
{
    uart_ctx.rx_out = (uart_ctx.rx_out + length) % uart_ctx.rx_size;
}

static void setup_rx_buffer(void)
{
    CsrUint32 free_space = get_rx_free_space();

    if (free_space >= RX_CHUNK_SIZE)
    {
        uart_descriptor rx_desc;

        if (uart_ctx.rx_in + RX_CHUNK_SIZE <= uart_ctx.rx_size)
        {
            rx_desc.buf = uart_ctx.rx_buffer + uart_ctx.rx_in;
        }
        else
        {
            rx_desc.buf = uart_ctx.rx_buffer;
            uart_ctx.rx_in = 0;
            uart_ctx.rx_out = 0;
        }

        rx_desc.len = RX_CHUNK_SIZE;

        if (uartPowerModeOn)
        {
            if (!rxQueued)
            {
#ifdef UART_BUILD_V1
                receiveRet = uart_receive_v2(uart_ctx.uartHandle, &rx_desc, 1, NULL);
#else
                receiveRet = uart_receive(uart_ctx.uartHandle, rx_desc.buf, rx_desc.len, NULL);
#endif
                if (receiveRet == UART_SUCCESS)
                {
                    rxQueued = TRUE;
                }
                else
                {
                    CSR_LOG_TEXT_INFO((CsrSerialComLto, 0, "UART rcv failure: %d", receiveRet));
                }
            }
        }
    }
}

static void wakeup_callback(void* cb_data)
{
    CsrUint8 wakeUpByte = 0xFD;
    CsrMutexLock(&uart_ctx.rx_mutex);

    if (uartPowerModeOn == FALSE)
    {
        SynMemCpyS(uart_ctx.rx_buffer + uart_ctx.rx_in, 1, &wakeUpByte, 1);
        advance_rx_in(1);

        CsrSchedBgintSet(rxBgint);
    }

    CsrMutexUnlock(&uart_ctx.rx_mutex);
}

#ifdef ENABLE_SSC_GDSC_VOTE
static void bt_ssc_gdsc_power_ctrl(CsrBool enable)
{
    ClockResult clockResult;

    if (enable)
    {
        clockResult = Clock_Attach(&bt_core_power_int.handle, BT_CORE_POWER_CLIENT_NAME);

        if (clockResult != CLOCK_SUCCESS)
        {
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_CLOCK_API_FAILED, "Clock attach failed");
        }

        clockResult = Clock_GetId(bt_core_power_int.handle, BT_CORE_POWER_DOMAIN, &bt_core_power_int.id);

        if (clockResult != CLOCK_SUCCESS)
        {
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_CLOCK_API_FAILED, "Clock Get Id failed");
        }

        clockResult = Clock_Enable(bt_core_power_int.handle, bt_core_power_int.id);

        if (clockResult != CLOCK_SUCCESS)
        {
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_CLOCK_API_FAILED, "Clock Enable failed");
        }
    }
    else
    {
        clockResult = Clock_Disable(bt_core_power_int.handle, bt_core_power_int.id);

        if (clockResult != CLOCK_SUCCESS)
        {
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_CLOCK_API_FAILED, "Clock Disable failed");
        }

        clockResult = Clock_Detach(bt_core_power_int.handle);

        if (clockResult != CLOCK_SUCCESS)
        {
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_CLOCK_API_FAILED, "Clock Detach failed");
        }

        bt_core_power_int.handle = CLOCK_HANDLE_INVALID;
        bt_core_power_int.id     = CLOCK_ID_INVALID;
    }
}
#endif

void uart_tx_isr_cb(CsrUint32 num_bytes, void *cb_data)
{
    CsrMutexLock(&uart_ctx.tx_mutex);

    uart_ctx.bytes_sent_total = num_bytes;

    CsrSchedBgintSet(txBgint);

    CsrMutexUnlock(&uart_ctx.tx_mutex);
}

void uart_rx_isr_cb(CsrUint32 num_bytes, void *cb_data)
{
    CsrMutexLock(&uart_ctx.rx_mutex);

    rxQueued = FALSE;

    advance_rx_in(num_bytes);

    uart_ctx.bytes_received = num_bytes;

    CsrSchedBgintSet(rxBgint);

    CsrMutexUnlock(&uart_ctx.rx_mutex);
}


void *CsrUartDrvOpen(const CsrCharString *device,
                     CsrUint32 *baud,
                     CsrUint8 dataBits,
                     CsrUint8 parity,
                     CsrUint8 stopBits,
                     CsrBool flowControl,
                     const CsrUint8 *token)
{
    CSR_LOG_TEXT_REGISTER(&CsrSerialComLto, "SERIAL_COM", 0, NULL);

    uart_open_config config;

    CsrMutexCreate(&uart_ctx.tx_mutex);
    CsrMutexCreate(&uart_ctx.rx_mutex);

   /* Basic configuration */
    config.baud_rate = *baud;
    config.parity_mode = parity;
    config.num_stop_bits = stopBits;
    config.bits_per_char = dataBits;
    config.enable_loopback = FALSE;
    config.enable_flow_ctrl = flowControl;
    config.mode = UART_ISLAND_MODE;
    config.enable_wake_feature = TRUE;
    config.cb_type = UART_CALLBACK_TYPE_V1;
    config.tx_cb_isr = uart_tx_isr_cb; /* Called in IST context */
    config.rx_cb_isr = uart_rx_isr_cb; /* Called in IST context */
#ifndef UART_BUILD_V1
    config.is_uart_vfcp = FALSE;
#endif

#ifdef ENABLE_SSC_GDSC_VOTE
    bt_ssc_gdsc_power_ctrl(TRUE);
#endif

    if (!psram_buffer)
    {
#ifdef UART_BUILD_V1
        psram_buffer = (uint8 *) pram_acquire_partition("SENSORS", &buffer_size);
#else
        psram_buffer = (uint8 *) pram_acquire_partition("BT", &buffer_size);
#endif
    }

    uart_ctx.tx_buffer = psram_buffer;
    uart_ctx.tx_size = TX_BUFFER_SIZE;

    globalTxBuffer = psram_buffer;

    uart_ctx.rx_buffer = psram_buffer + TX_BUFFER_SIZE;
    uart_ctx.rx_size = buffer_size - TX_BUFFER_SIZE;
    uart_ctx.rx_in = 0;
    uart_ctx.rx_out = 0;
#ifdef UART_BUILD_V1
    uclockResult = uClock_EnablePowerdomain(CLOCK_POWERDOMAIN_SSC);
    openRet =  uart_open(&uart_ctx.uartHandle, UART_INSTANCE_02, &config);
#else
    openRet =  uart_open(&uart_ctx.uartHandle, UART_INSTANCE_03, &config);
#endif

    CSR_LOG_TEXT_INFO((CsrSerialComLto, 0, "CsrUartDrvOpen, handle : %x, openRet : %d", uart_ctx.uartHandle, openRet));

    if (openRet != UART_SUCCESS)
    {
        /* error condition */
    }

    uartPowerModeOn = TRUE;

    setup_rx_buffer(); /* Queue initial RX buffer */

    return &uart_ctx.uartHandle;
}

CsrBool CsrUartDrvStart(void *handle, CsrUint8 baudId)
{
    if (openRet == UART_SUCCESS)
    {
        return TRUE;
    }
    return FALSE;
}

CsrBool CsrUartDrvStop(void *handle)
{
    return TRUE;
}

CsrUint32 CsrUartDrvGetBaudrate(void *handle)
{
    CsrUint32 baudrate = 3000000;

    return baudrate;
}

CsrBool CsrUartDrvTx(void *handle, const CsrUint8 *data, CsrUint32 dataLength, CsrUint32 *numSent)
{
    CsrMutexLock(&uart_ctx.tx_mutex);

    if (uart_ctx.tx_buffer + dataLength >= uart_ctx.rx_buffer)
    {
        uart_ctx.tx_buffer = globalTxBuffer;
    }

    uart_ctx.tx_desc_local.len = dataLength;
    uart_ctx.bytes_sent_total= 0;
    uart_ctx.tx_desc_local.flags = 0;
    uart_ctx.tx_desc_local.buf = uart_ctx.tx_buffer;

    SynMemCpyS(uart_ctx.tx_desc_local.buf, dataLength, data, dataLength);

    CSR_LOG_TEXT_INFO((CsrSerialComLto, 0, "UART Tx, Len %d", dataLength));

    for(CsrUint32 i=0;i < dataLength && i < 20;i++)
    {
        CSR_LOG_TEXT_INFO((CsrSerialComLto, 0, "%x", data[i]));
    }

    transmitRet = uart_transmit(uart_ctx.uartHandle, uart_ctx.tx_desc_local.buf, uart_ctx.tx_desc_local.len, NULL);

    CSR_LOG_TEXT_INFO((CsrSerialComLto, 0, "UART Tx, Result : %d", transmitRet));

    if (transmitRet == 0)
    {
        *numSent = dataLength;
        uart_ctx.tx_buffer+=dataLength;
        txConsecutiveFailureCount = 0;
    }
    else
    {
        txConsecutiveFailureCount++;
        *numSent = 0;

        if (txConsecutiveFailureCount == 5)
        {
            CsrPanic(CSR_TECH_FW,CSR_PANIC_FW_H4_TX_FAILED, "UART TX FAIL 5 TIMES");
        }

    }

    CsrMutexUnlock(&uart_ctx.tx_mutex);

    if (!transmitRet)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


void CsrUartDrvRx(void *handle)
{
    /* Doesn't get called */
}

CsrUint32 CsrUartDrvLowLevelTransportRx(void *handle, CsrUint32 dataLength, CsrUint8 *data)
{
    CsrUint32 bytes_to_copy;

    CSR_LOG_TEXT_INFO((CsrSerialComLto, 0, "UART Rx, len : %d rx in : %d rx out : %d", dataLength, uart_ctx.rx_in, uart_ctx.rx_out));

    CsrMutexLock(&uart_ctx.rx_mutex);

    bytes_to_copy = (uart_ctx.rx_in >= uart_ctx.rx_out)
                                     ? uart_ctx.rx_in - uart_ctx.rx_out
                                     : uart_ctx.rx_size - uart_ctx.rx_out;

    if (bytes_to_copy > dataLength)
    {
        bytes_to_copy = dataLength;
    }

    SynMemCpyS(data, bytes_to_copy, uart_ctx.rx_buffer + uart_ctx.rx_out, bytes_to_copy);
    advance_rx_out(bytes_to_copy);

    if (uart_ctx.rx_in != uart_ctx.rx_out)
    {
        CSR_LOG_TEXT_DEBUG((CsrSerialComLto, 0, "UART Rx, BgInt set"));
        CsrSchedBgintSet(rxBgint);
    }
    else
    {
        CSR_LOG_TEXT_INFO((CsrSerialComLto, 0, "UART Rx, new buffer set rx in : %d rx out : %d", uart_ctx.rx_in, uart_ctx.rx_out));
        setup_rx_buffer();
    }

    CsrMutexUnlock(&uart_ctx.rx_mutex);

    return bytes_to_copy;
}

void CsrUartDrvClose(void *handle)
{
    uart_close(uart_ctx.uartHandle);

#ifdef ENABLE_SSC_GDSC_VOTE
    bt_ssc_gdsc_power_ctrl(FALSE);
#endif

    rxQueued = FALSE;
    txConsecutiveFailureCount = 0;
#ifdef UART_BUILD_V1
    uClock_DisablePowerDomain(CLOCK_POWERDOMAIN_SSC);
#endif
    CsrMutexDestroy(&uart_ctx.tx_mutex);
    CsrMutexDestroy(&uart_ctx.rx_mutex);
}

void CsrUartDrvRegister(void *handle, CsrUartDrvDataRx rxDataFn, CsrSchedBgint rxBgintHdl)
{
    rxData = rxDataFn;
    rxBgint = rxBgintHdl;
}

void CsrUartDrvRegisterTx(void *handle, CsrSchedBgint txBgintHandle)
{
    txBgint = txBgintHandle;
}


CsrBool CsrUartDrvSetPowerMode(void *handle, CsrBool enable)
{
    uart_result result;

    CSR_LOG_TEXT_DEBUG((CsrSerialComLto, 0, "Before power API call, enable : %d", enable));

    CsrMutexLock(&uart_ctx.rx_mutex);

    power_api_status.enable = enable;
    power_api_status.result = -1;

    if (enable)
    {
        result = uart_power_on(uart_ctx.uartHandle);

        if (result == UART_SUCCESS)
        {
            uartPowerModeOn = TRUE;
        }

        power_api_status.result = result;
        CSR_LOG_TEXT_DEBUG((CsrSerialComLto, 0, "After power API call, result : %d enable : %d", result, enable));

        if (uart_ctx.rx_in == uart_ctx.rx_out)
        {
            setup_rx_buffer();
        }
        else
        {
            CSR_LOG_TEXT_DEBUG((CsrSerialComLto, 0, "Rx buffer not queued rx in : %d rx out : %d", uart_ctx.rx_in, uart_ctx.rx_out));
        }
    }
    else
    {
        result = uart_power_off(uart_ctx.uartHandle, TRUE, wakeup_callback, NULL);

        if (result == UART_SUCCESS)
        {
            uartPowerModeOn = FALSE;
        }

        power_api_status.result = result;
        CSR_LOG_TEXT_DEBUG((CsrSerialComLto, 0, "After power API call, result : %d enable : %d", result, enable));
    }

    CsrMutexUnlock(&uart_ctx.rx_mutex);

    if (result == UART_SUCCESS)
    {
        return TRUE;
    }

    return FALSE;
}
