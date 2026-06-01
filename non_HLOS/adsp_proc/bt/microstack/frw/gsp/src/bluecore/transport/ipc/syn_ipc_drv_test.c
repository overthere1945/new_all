/******************************************************************************
Copyright (c) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#include "csr_synergy.h"

#include "syn_ipc_drv.h"
#ifndef SYN_IPC_UNIT_TEST
#include "bt_lpass_test_ext_glink.h"
#endif
#ifdef SYN_IPC_STUB
#include "csr_types.h"
#include "csr_sched.h"
#include "syn_ipc_internal.h"
#include "syn_qapi_ipc_x86.h"
#include <stdlib.h>

#if (CSR_HOST_PLATFORM == Q6_HOST)
#include <string.h>
#include "glink.h"
#include "stringl/stringl.h"
glink_err_type bt_test_iface_glink_tx(uint8_t channel_idx, uint8_t *data, uint16_t len);
#endif


extern uint8_t *btHciGetBufCb(void *arg, const qapi_ipc_msg_t *ipcMsg);
extern void btHciRxMsgCb(void *arg, const qapi_ipc_msg_t * ipcMsg, const uint8 *payload);
extern void customRxMsgCb(void *arg, const qapi_ipc_msg_t * ipcMsg, const uint8 *payload);
extern uint8_t *customGetBufCb(void *arg, const qapi_ipc_msg_t *ipcMsg);
void SynIpcSendAckTest(qapi_ipc_msg_t * ipcMsg);

void qcli_ipc_tx(qapi_ipc_tx_msg_t * tx_msg);

boolean qapi_IPC_RegisterClient_Test(
    qapi_ipc_client_t client,
    qapi_ipc_client_cb_t * client_cb)
{
    CSR_UNUSED(client);
    CSR_UNUSED(client_cb);
    return TRUE;
}

boolean qapi_IPC_DeRegisterClient_Test(qapi_ipc_client_t client)
{
    CSR_UNUSED(client);
    return TRUE;
}

uint8_t * qapi_IPC_malloc_Test(size_t size)
{
    return malloc(size);
}

void qapi_IPC_free_Test(uint8_t * ptr)
{
    free(ptr);
}

void SynIpcHciRxMsgTest(uint16_t msg_len, uint8_t *msg)
{
    uint8_t *data;
    qapi_ipc_msg_t ipc_msg;
    ipc_msg.msg_len = msg_len;

    data = btHciGetBufCb(NULL, &ipc_msg);
    if (data != NULL)
    {
        SynMemCpyS(data, msg_len, msg, msg_len);
        btHciRxMsgCb(NULL, &ipc_msg, data);    
    }
}

void SynIpcCustomRxMsgTest(uint8_t msgid, uint16_t msg_len, uint8_t *msg)
{
    uint8_t *data = NULL;
    qapi_ipc_msg_t ipc_msg;
    ipc_msg.msg_len = msg_len;
    ipc_msg.msg_hdr = QAPI_IPC_BUILD_MSG_HEADER(QAPI_IPC_CLIENT_BT_HOST_CUSTOM_E, msgid);

    if (msg_len != 0)
    {
        data = customGetBufCb(NULL, &ipc_msg);
        SynMemCpyS(data, msg_len, msg, msg_len);
    }

    customRxMsgCb(NULL, &ipc_msg, data);    
}

void SynIpcSendAckTest(qapi_ipc_msg_t * ipcMsg)
{
    ipcMsg->msg_hdr |= QAPI_IPC_ACK_MASK;

    if (QAPI_IPC_GET_CLIENT_ID(ipcMsg->msg_hdr) == QAPI_IPC_CLIENT_BT_HCI_E)
    {
        btHciRxMsgCb(NULL, ipcMsg, NULL);
    }
    else
    {
        customRxMsgCb(NULL, ipcMsg, NULL);
    }
}


#if (CSR_HOST_PLATFORM == Q6_HOST)
void hci_glink_rx(void *data, size_t len)
{
    SynIpcHciRxMsgTest(len, (uint8_t *)data);
}
#endif


#ifdef SYN_IPC_UNIT_TEST

bool gEnableNocp = FALSE;

#define HCI_ACL_HANDLE_MASK                 0x0FFF

#define SYNIPC_GET_UINT16(_data) (uint16_t) (((uint8_t *) (_data))[0] | ((uint8_t *) (_data))[1] << 8)
#define SYNIPC_SET_UINT16(_data, _value)                               \
{                                                                      \
    ((uint8_t *) _data)[0] = (uint8_t) ((_value) & 0x00FF);            \
    ((uint8_t *) _data)[1] = (uint8_t) ((_value) >> 8);                \
}

void SynIpcConfigureNocp(bool enableNocp)
{
    gEnableNocp = enableNocp;
}


qapi_ipc_err_status_t qapi_IPC_TxMsg_Test(qapi_ipc_tx_msg_t * tx_msg)
{
    int i;
    qapi_ipc_err_status_t status = QAPI_IPC_STATUS_SUCCESS_E;
    CsrBool sendNocp=FALSE;
    CsrUint8 nocp[]={0x04, 0x13, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00};

    uint8_t * buff = tx_msg->msg.payload;

    CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "IPC Msg Sent"));

    for (i=0; i < tx_msg->msg.msg_len; i++)
    {
        CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "%02X", buff[i]));
    }

    if (buff[0] == 2)
    {
        CsrUint16 hciHandle      = (SYNIPC_GET_UINT16(&buff[1]) & HCI_ACL_HANDLE_MASK);
                
        SYNIPC_SET_UINT16(&nocp[4], hciHandle);
        sendNocp = TRUE;
    }

    /* Send ACK internally */
    SynIpcSendAckTest(&(tx_msg->msg));


    if (gEnableNocp && sendNocp)
    {
        CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "Send NOCP internally "));

        SynIpcHciRxMsgTest(sizeof(nocp), nocp);
    }

    return status;
}
#else

qapi_ipc_err_status_t qapi_IPC_TxMsg_Test(qapi_ipc_tx_msg_t * tx_msg)
{
    qapi_ipc_err_status_t status = QAPI_IPC_STATUS_SUCCESS_E;

#if (CSR_HOST_PLATFORM == Q6_HOST)
    uint8_t * buff = malloc(tx_msg->msg.msg_len);
    if (buff != NULL)
    {
        int i;
        SynMemCpyS(buff, tx_msg->msg.msg_len, tx_msg->msg.payload, tx_msg->msg.msg_len);

        CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "IPC Msg Sent"));

        for (i=0; i < CSRMIN(50, tx_msg->msg.msg_len); i++)
        {
            CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "%02X", buff[i]));
        }

        if (bt_offload_test_glink_hcicmd_tx(buff, tx_msg->msg.msg_len) == GLINK_STATUS_SUCCESS)
        {
            /* Send ACK internally */
            SynIpcSendAckTest(&(tx_msg->msg));
        }
        else
        {
            CSR_LOG_TEXT_INFO((SynIpcLto, SYN_IPC_LTSO_GEN, "GLINK TX failure"));
            free(buff);
            status = QAPI_IPC_STATUS_INVALID_PARAM_E;
        }     
    }
    else
    {
        status = QAPI_IPC_STATUS_INVALID_PARAM_E;
    }
#else
    qcli_ipc_tx(tx_msg);
#endif
 
    return status;
}
#endif

#endif /* SYN_IPC_STUB */

