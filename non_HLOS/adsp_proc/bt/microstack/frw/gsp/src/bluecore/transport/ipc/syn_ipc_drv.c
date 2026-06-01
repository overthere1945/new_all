/******************************************************************************
Copyright (c) 2020 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #4 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_sched.h"
#include "syn_ipc_internal.h"
#if (CSR_HOST_PLATFORM == QCC5100_HOST)
#include "qapi_ipc.h"
#else
#include "syn_qapi_ipc_x86.h"
#endif
#include "syn_ipc_drv.h"
#include "csr_framework_ext.h"

#ifdef SYN_IPC_STUB
boolean qapi_IPC_RegisterClient_Test(qapi_ipc_client_t client, 
                                     qapi_ipc_client_cb_t * client_cb);
boolean qapi_IPC_DeRegisterClient_Test(qapi_ipc_client_t client);
uint8_t * qapi_IPC_malloc_Test(size_t size);
void qapi_IPC_free_Test(uint8_t * ptr);
qapi_ipc_err_status_t qapi_IPC_TxMsg_Test(qapi_ipc_tx_msg_t * tx_msg);

#define qapi_IPC_RegisterClient qapi_IPC_RegisterClient_Test
#define qapi_IPC_DeRegisterClient qapi_IPC_DeRegisterClient_Test
#define qapi_IPC_malloc qapi_IPC_malloc_Test
#define qapi_IPC_free qapi_IPC_free_Test
#define qapi_IPC_TxMsg qapi_IPC_TxMsg_Test
#endif

qapi_ipc_client_t btHciHandle = QAPI_IPC_CLIENT_BT_HCI_E;
qapi_ipc_client_t btCustomHandle = QAPI_IPC_CLIENT_BT_HOST_CUSTOM_E;

static CsrSchedBgint receiveBgint = CSR_SCHED_BGINT_INVALID;
static CsrSchedBgint transmitBgint = CSR_SCHED_BGINT_INVALID;
static CsrSchedBgint customTransmitBgint = CSR_SCHED_BGINT_INVALID;
qapi_ipc_msg_t synIpcMsg;
CsrBool ipcLogRegistered = FALSE;

static CsrBool      synIpcBtRadioOnState = TRUE;
static CsrMutexHandle synIpcMutexLock;
static CsrBool      synIpcMutexCreated = FALSE;

/* variable to store the number of Tx to IPC */
static uint16_t     synIpcTxCount = 0;
/* variable to store the number of acked Tx */
static uint16_t     synIpcTxAckedCount = 0;
/* variable to store the number of failed tx attempts */
static uint16_t     synIpcTxFailCount = 0;
/* variable to store the number of tx attempts dropped*/
static uint16_t     synIpcTxDropCount = 0;


#ifdef SYN_IPC_DEBUG_LOG_DATA
struct {
    CsrUint8 buffer[SYN_IPC_DEBUG_MEM_BUFFER_SIZE];
    CsrUint16 windex;
} txb;
#endif

/* Log Text */
CSR_LOG_TEXT_HANDLE_DEFINE(SynIpcDrvLto);

enum {
    SYN_IPC_DRV_PANIC_ISR_HCI_GB_ALLOC_FAIL = CSR_PANIC_FW_SYN_IPC_BASE,
    SYN_IPC_DRV_PANIC_ISR_HCI_GB_INVALID_PARAM = CSR_PANIC_FW_SYN_IPC_BASE+1,
    SYN_IPC_DRV_PANIC_ISR_HCI_RXM_WRITE_FAIL = CSR_PANIC_FW_SYN_IPC_BASE+2,
    SYN_IPC_DRV_PANIC_ISR_CUSTOM_GB_ALLOC_FAIL = CSR_PANIC_FW_SYN_IPC_BASE+3,
    SYN_IPC_DRV_PANIC_ISR_CUSTOM_GB_INVALID_PARAM = CSR_PANIC_FW_SYN_IPC_BASE+4,
    SYN_IPC_DRV_PANIC_ISR_CUSTOM_RXM_WRITE_FAIL = CSR_PANIC_FW_SYN_IPC_BASE+5,
};

/* 
3. what will be the max ask from the controller?
As per atherton logs, host buffer size for ACL is 1691 x 20. Even tough 1691
RFC MFS is 990 so it should be 990 for payload + 13 headear = 1003.
GATT MTU is 512 + 11 bytes header = 523 for notifications.
*/

uint8_t *btHciGetBufCb(void *arg, const qapi_ipc_msg_t *ipcMsg)
{
    uint8_t *payload = NULL;
#ifdef SYN_IPC_DEBUG_ADD_RX_WATER_MARK
    CsrUint8 wmSize = 2;
#else
    CsrUint8 wmSize = 0;
#endif

    CSR_UNUSED(arg);
    if (ipcMsg)
    {
        /* Additional two bytes for water mark */
        payload = (uint8_t *)SynIpcProtectedMemAlloc(ipcMsg->msg_len + wmSize);
        if (payload)
        {
#ifdef SYN_IPC_DEBUG_ADD_RX_WATER_MARK
            payload[0] = '@';
            payload[1] = '@';
#endif
            payload = payload + wmSize;
        }
        else
        {
            CsrPanic(CSR_TECH_FW, SYN_IPC_DRV_PANIC_ISR_HCI_GB_ALLOC_FAIL, NULL);
        }
    }
    else
    {
        /* This should never happen */
        CsrPanic(CSR_TECH_FW, SYN_IPC_DRV_PANIC_ISR_HCI_GB_INVALID_PARAM, NULL);
    }
    return payload;
}

void btHciRxMsgCb(void *arg, const qapi_ipc_msg_t * ipcMsg, const uint8 *payload)
{
    CsrBool status;
    CsrUint16 payloadSize = 0;
#ifdef SYN_IPC_DEBUG_ADD_RX_WATER_MARK
    CsrUint8 wmSize = 2;
#else
    CsrUint8 wmSize = 0;
#endif

    CSR_UNUSED(arg);

    if (ipcMsg == NULL)
    {
        return;
    }

    if (payload != NULL)
    {
        status = SynIpcMemWriteComplete((CsrUint8*)(payload - wmSize), &payloadSize);
        if (status != TRUE)
        {
            /* This should never happen */
            CsrPanic(CSR_TECH_FW, SYN_IPC_DRV_PANIC_ISR_HCI_RXM_WRITE_FAIL, NULL);
        }
#ifdef SYN_IPC_DEBUG_LOG_RX_DATA
        if (payloadSize)
        {
            SynMemCpyS(&txb.buffer[txb.windex], SYN_IPC_DEBUG_MEM_BUFFER_SIZE - txb.windex, payload - wmSize, payloadSize);
            txb.windex += payloadSize;
            txb.windex = txb.windex % SYN_IPC_DEBUG_MEM_BUFFER_SIZE;
        }
#endif
        CsrSchedBgintSet(receiveBgint);
    }
    else if (!QAPI_IPC_IS_RX_MSG(ipcMsg->msg_hdr))
    {
#ifdef SYN_IPC_DEBUG_LOG_TX_DATA
        txb.buffer[txb.windex] = '+';
        txb.windex++;
        txb.buffer[txb.windex] = '+';
        txb.windex++;
        txb.windex = txb.windex % SYN_IPC_DEBUG_MEM_BUFFER_SIZE;
#endif
        synIpcTxAckedCount++;

        /* Ack for previsouly sent message. Just free the allocated message */
        qapi_IPC_free(ipcMsg->payload);
        /* Ack for previsouly sent message. Set the tx bg interrupt so as to
            schedule the pending messages in the hci tx queue */
        CsrSchedBgintSet(transmitBgint);
    }
}

uint8_t *customGetBufCb(void *arg, const qapi_ipc_msg_t *ipcMsg)
{
    uint8_t *payload = NULL;

#ifdef SYN_IPC_DEBUG_ADD_RX_WATER_MARK
    CsrUint8 wmSize = 2;
#else
    CsrUint8 wmSize = 0;
#endif

    CSR_UNUSED(arg);
    if (ipcMsg)
    {
        /* +2 for water mark, +2 for storing the HCI pkt type & cust pkt type */
        payload = (uint8_t *)SynIpcProtectedMemAlloc(ipcMsg->msg_len + wmSize + 2);
        if (payload)
        {
#ifdef SYN_IPC_DEBUG_ADD_RX_WATER_MARK
            payload[0] = '!';
            payload[1] = '!';
#endif

            /* Packet type does not come from BTSS, so custom packet type is 
                added so as to the synergy ipc rx driver to recognize
                this against the normal hci event and acl data. */
            payload[wmSize] = SYN_IPC_HCI_PKT_CUSTOM;
            /* data[wmSize + 1] will be place holder to store the type of custom message received 
                in customRxMsgCb, so here give a pointer two places ahead to ipc driver */
            payload = payload + wmSize + 2;
        }
        else
        {
            CsrPanic(CSR_TECH_FW, SYN_IPC_DRV_PANIC_ISR_CUSTOM_GB_ALLOC_FAIL, NULL);

        }
    }
    else
    {
        /* This should never happen */
        CsrPanic(CSR_TECH_FW, SYN_IPC_DRV_PANIC_ISR_CUSTOM_GB_INVALID_PARAM, NULL);
    }
    return payload;
}

void customRxMsgCb(void *arg, const qapi_ipc_msg_t * ipcMsg, const uint8 *payload)
{
    CsrBool status;
    CsrUint8 *data;
    CsrUint16 payloadSize = 0;

#ifdef SYN_IPC_DEBUG_ADD_RX_WATER_MARK
    CsrUint8 wmSize = 2;
#else
    CsrUint8 wmSize = 0;
#endif

    CSR_UNUSED(arg);

    if (ipcMsg == NULL)
    {
        return;
    }

    if (QAPI_IPC_IS_RX_MSG(ipcMsg->msg_hdr) && (payload == NULL))
    {
        /* Internally call customGetBufCb() to store additional info 
           e.g custom packet type and water mark */
        payload = customGetBufCb(NULL, ipcMsg);
    }

    if (payload != NULL)
    {
        data = (CsrUint8*)(payload - wmSize - 2);
        data[wmSize + 1] = QAPI_IPC_GET_MSG_ID(ipcMsg->msg_hdr);
        status = SynIpcMemWriteComplete(data, &payloadSize);
        if (status != TRUE)
        {
            /* This should never happen */
            CsrPanic(CSR_TECH_FW, SYN_IPC_DRV_PANIC_ISR_CUSTOM_RXM_WRITE_FAIL, NULL);
        }
#ifdef SYN_IPC_DEBUG_LOG_RX_DATA
        if (payloadSize)
        {
            SynMemCpyS(&txb.buffer[txb.windex], SYN_IPC_DEBUG_MEM_BUFFER_SIZE - txb.windex, data, payloadSize);
            txb.windex += payloadSize;
            txb.windex = txb.windex % SYN_IPC_DEBUG_MEM_BUFFER_SIZE;
        }
#endif
        CsrSchedBgintSet(receiveBgint);
    }
    else if (!QAPI_IPC_IS_RX_MSG(ipcMsg->msg_hdr))
    {
#ifdef SYN_IPC_DEBUG_LOG_TX_DATA
        txb.buffer[txb.windex] = '<';
        txb.windex++;
        txb.buffer[txb.windex] = '<';
        txb.windex++;
        txb.windex = txb.windex % SYN_IPC_DEBUG_MEM_BUFFER_SIZE;
#endif

        /* Free the payload */
        qapi_IPC_free(ipcMsg->payload);
        /* Ack for previsouly sent message. Set the tx bg interrupt so as to
            schedule the pending tx messages in the custom tx queue */
        CsrSchedBgintSet(customTransmitBgint);
    }
}

void *SynIpcDrvOpen(SynIpcTransport trans)
{
    qapi_ipc_client_t *client = NULL;
    qapi_ipc_client_cb_t customCbSt;

    if (trans == SYN_IPC_TRANSPORT_HCI)
    {
        client = &btHciHandle;
        synIpcTxCount = 0;
        synIpcTxAckedCount = 0;
        synIpcTxFailCount = 0;
        synIpcTxDropCount = 0;
    }
    else if (trans == SYN_IPC_TRANSPORT_CUSTOM)
    {
        customCbSt.rx_get_buf.cb = customGetBufCb;
        customCbSt.rx_get_buf.cb_arg = NULL;
        customCbSt.rx_msg.cb = customRxMsgCb;
        customCbSt.rx_msg.cb_arg = NULL;
        customCbSt.cb_msg = &synIpcMsg;
        client = &btCustomHandle;
        if (qapi_IPC_RegisterClient(btCustomHandle, &customCbSt) != TRUE)
        {
            CsrPanic(CSR_TECH_FW, 0, "Custom register failed");
        }
    }

    if(synIpcMutexCreated == FALSE)
    {
        /* Initialise the ipc mutex */
        if(CsrMutexCreate(&synIpcMutexLock) == CSR_RESULT_SUCCESS)
        {
            synIpcMutexCreated = TRUE;
        }
    }

    if (ipcLogRegistered != TRUE)
    {
        CSR_LOG_TEXT_REGISTER(&SynIpcDrvLto, "IPC Drv", 0, NULL);
    }

    return client;
}

void SynIpcDrvClose(void *handle)
{
    if(synIpcMutexCreated == TRUE)
    {
        /* Initialise the ipc mutex */
        CsrMutexDestroy(&synIpcMutexLock);
        synIpcMutexCreated = FALSE;
    }

    CSR_UNUSED(handle);
}

CsrBool SynIpcDrvStart(void *handle)
{
    qapi_ipc_client_t *client = handle;
    qapi_ipc_client_cb_t btHciCbSt;

    btHciCbSt.rx_get_buf.cb = btHciGetBufCb;
    btHciCbSt.rx_get_buf.cb_arg = NULL;
    btHciCbSt.rx_msg.cb = btHciRxMsgCb;
    btHciCbSt.rx_msg.cb_arg = NULL;
    btHciCbSt.cb_msg = &synIpcMsg;

#ifdef SYN_IPC_DEBUG_LOG_DATA
    CsrMemSet(&txb, 0, sizeof(txb));
#endif

    SynIpcMemInit();

    return (CsrBool)qapi_IPC_RegisterClient(*client, &btHciCbSt);
}

void SynIpcDrvSetBtRadioOnState(CsrBool btRadioState)
{
    CsrMutexLock(&synIpcMutexLock);
    synIpcBtRadioOnState = btRadioState;
    CsrMutexUnlock(&synIpcMutexLock);
}

CsrBool SynIpcDrvStop(void *handle)
{
    qapi_ipc_client_t *client = handle;

    SynIpcMemDeInit();

    return (CsrBool)qapi_IPC_DeRegisterClient(*client);
}

CsrUint8* SynIpcDrvAlloc(CsrUint16 size)
{
    return (CsrUint8*)qapi_IPC_malloc((size_t)size);
}


#ifdef SYN_IPC_DEBUG_LOG_TX_DATA
static void logTxData(SynIpcMsgId msgId, CsrUint8 *data, CsrUint32 dataLength)
{
    if (msgId == SYN_IPC_MSG_BT_HCI)
    {
        txb.buffer[txb.windex] = '#';
        txb.windex++;
        txb.buffer[txb.windex] = '#';
    }
    else
    {
        txb.buffer[txb.windex] = '$';
        txb.windex++;
        txb.buffer[txb.windex] = '$';
    }
    txb.windex++;
    txb.buffer[txb.windex] = dataLength;
    txb.windex++;
    if (dataLength)
    {
        SynMemCpyS(&txb.buffer[txb.windex], SYN_IPC_DEBUG_MEM_BUFFER_SIZE - txb.windex, data, dataLength);
        txb.windex += dataLength;
    }
    txb.windex = txb.windex % SYN_IPC_DEBUG_MEM_BUFFER_SIZE;
}
#endif

CsrBool SynIpcDrvTx(void *handle, SynIpcMsgId msgId, const CsrUint8 *data, CsrUint32 dataLength)
{
    qapi_ipc_tx_msg_t txMsg;
    qapi_ipc_err_status_t status;
    qapi_ipc_client_t *client = handle;

    txMsg.msg.msg_hdr = QAPI_IPC_BUILD_MSG_HEADER(*client, msgId);
    txMsg.msg.msg_len = (uint16_t)dataLength;
    txMsg.msg.payload = (uint8_t *)data;
    txMsg.ring_type = QAPI_IPC_RING_TYPE_DEFAULT_E;

#ifdef SYN_IPC_DEBUG_LOG_TX_DATA
    logTxData(msgId, data, dataLength);
#endif

    CsrMutexLock(&synIpcMutexLock);
    if(synIpcBtRadioOnState == FALSE)
    {
        CsrMutexUnlock(&synIpcMutexLock);
        synIpcTxDropCount++;
        qapi_IPC_free((uint8_t *)data);
        CSR_LOG_TEXT_WARNING((SynIpcDrvLto, 0, "IPC Packet dropped"));
        return FALSE;
    }
    CsrMutexUnlock(&synIpcMutexLock);
    
    status = qapi_IPC_TxMsg(&txMsg);
    if (status != QAPI_IPC_STATUS_SUCCESS_E)
    {
        synIpcTxFailCount++;
        qapi_IPC_free((uint8_t *)data);

        if (status == QAPI_IPC_STATUS_RING_BUFFER_FULL_E)
        {
            CSR_LOG_TEXT_WARNING((SynIpcDrvLto, 0, "IPC Ring Buffer Full"));
        }
        else
        {
            CsrPanic(CSR_TECH_FW, (uint16_t)(CSR_PANIC_FW_QAPI_IPC_BASE|status), "IPC Send failed");
        }

        return FALSE;
    }
    synIpcTxCount++;

    return TRUE;
}

void SynIpcDrvLowLevelTransportRx(void *handle, void *context, SynIpcDrvReadCallback cb)
{
    CSR_UNUSED(handle);
    SynIpcMemRead(context, cb);
    if (SynIpcMemIsDataAvailable())
    {
         CsrSchedBgintSet(receiveBgint);
    }
}

void SynIpcDrvRegister(void *handle, 
                       CsrSchedBgint rxBgintHandle, 
                       CsrSchedBgint txBgintHandle,
                       CsrSchedBgint customTxBgintHandle)
{
    CSR_UNUSED(handle);
    receiveBgint = rxBgintHandle;
    transmitBgint = txBgintHandle;
    customTransmitBgint = customTxBgintHandle;
}

#if defined (CSR_PLATFORM_WINDOWS) || defined (CSR_PLATFORM_LINUX)
qapi_ipc_err_status_t qapi_IPC_TxMsg
(
    qapi_ipc_tx_msg_t * tx_msg
)
{
    CSR_UNUSED(tx_msg);

    return QAPI_IPC_STATUS_SUCCESS_E;
}

void qapi_IPC_free( uint8_t * ptr )
{
    CSR_UNUSED(ptr);
}

boolean qapi_IPC_RegisterClient
(
    qapi_ipc_client_t client,
    qapi_ipc_client_cb_t * client_cb
)
{
    return TRUE;
}

boolean qapi_IPC_DeRegisterClient
(
    qapi_ipc_client_t client
)
{
    return TRUE;
}

uint8_t * qapi_IPC_malloc
(
    size_t size
)
{
    return NULL;
}

#endif


