/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#ifndef __HCI_ARBITER_PRIVATE_H__
#define __HCI_ARBITER_PRIVATE_H__

#include "bluetooth.h"
#include "csr_hci_prim.h"
#include "hci_arbiter_int.h"
#include "hci_arbiter_lib.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef uint8 HciArbProtocol;
#define HCI_ARB_RFCOMM_PROTOCOL            ((HciArbProtocol)0x01)
#define HCI_ARB_LECOC_PROTOCOL             ((HciArbProtocol)0x02)
#define HCI_ARB_GATT_PROTOCOL              ((HciArbProtocol)0x03)

#define HCI_ARB_HCI_CMD_TYPE               0x01
#define HCI_ARB_HCI_EVT_TYPE               0x04
#define HCI_ARB_HCI_ACL_TYPE               0x02
#define HCI_ARB_PERI_CMD_TYPE              0x31
#define HCI_ARB_PERI_EVT_TYPE              0x34
#define HCI_ARB_PERI_DATA_TYPE             0x32

#define ACL_SOURCE_PRIMARY_STACK           0xFFFF

#define HCI_PERI_SS_ACTIVATE_COMPLETE           0x02
#define HCI_PERI_COMMAND_STATUS                 0x00
#define BTSS_SUBSYSTEM                          0x01
#define BT_OFF_ACTION                           0x00

#define HCI_PERI_ACTIVATE_SS_CMD_LENGTH         7

#define PERI_COMMAND_STATUS_EVENT_LENGTH        10
#define PERI_ACTIVATE_COMPLETE_EVENT_LENGTH     8
#define PERI_COMMAND_STATUS_MSG_TYPE_OFFSET     5
#define PERI_COMMAND_STATUS_RESULT_OFFSET       6
#define PERI_ACTIVATE_COMPLETE_MSG_TYPE_OFFSET  5
#define PERI_ACTIVATE_COMPLETE_SUBSYSTEM_OFFSET 6
#define PERI_ACTIVATE_COMPLETE_ACTION_OFFSET    7

#define HCI_ARB_ATT_CID                         0x0004
#define HCI_ARB_MAX_FILTERID_COUNTER            256

#define HCI_ARB_DEBUG_DATA_LEN                  10

#define HCI_ARB_SET_UINT16(_data, _value)                      \
{                                                              \
    ((uint8 *) _data)[0] = (uint8) ((_value) & 0x00FF);        \
    ((uint8 *) _data)[1] = (uint8) ((_value) >> 8);            \
}

#define HCI_ARB_GET_UINT16(_data) (uint16) (((uint8 *) (_data))[0] | ((uint8 *) (_data))[1] << 8)

#define HCI_ARB_GET_HCI_HANDLE_FROM_FILTER_ID(filterId) (uint16)(filterId & 0xFFFF)
#define HCI_ARB_GET_PROTOCOL_FROM_FILTER_ID(filterId) (uint8)(filterId  >> 24)


extern CsrUint8 PeriCmdInternal;

typedef struct
{
    uint8 dlci;
}HciArbRfcommCtx;

typedef struct
{
    HciArbL2capCid remoteCid;
}HciArbLecocCtx;

typedef union
{
    HciArbRfcommCtx *rfcomm;
    HciArbLecocCtx *lecoc;
    HciArbGattCtx *gatt;
}HciArbProtocolInfo;

typedef struct HciArbRxFilter_tag
{
    HciArbProtocol     protocol;
    HciArbFilterId     filterId;
    HciArbL2capCid     localCid;
    HciArbProtocolInfo proto;
    struct HciArbRxFilter_tag *next;
}HciArbRxFilter;

typedef struct
{
    HciArbRxFilter *rxFilter;
    HciArbHciHandle hciHandle;
    PHYSICAL_TRANSPORT_T transport;
    CsrBtTypedAddr addrt;
    HciArbStack firstFragStack; /* destination stack for first fragment, 
                                 * subsequent fragments are sent to same stack */
    bool prepWriteOffloaded;    /* Set to TRUE when prepare write req for 
                                 * micro stack is received, execute write req 
                                 * is sent to micro stack if this is set */
} HciArbAclInst;

typedef struct
{
    bool appCtxRunning;
    uint16 length;
    uint8 data[10];

    uint16 count;               /* No of times init is called */
} HciArbDebug;

typedef struct
{
    bool passthroughMode;
    HCI_ARBITER_MSG_CB hciMsgCb;
    HCI_ARBITER_ATT_CB attSeqCb;
    uint8  filterIdCounter;
    HciArbDebug debug;
} HciArbInst;

extern HciArbInst gHciArb;

void HciArbRegisterCb(HCI_ARBITER_MSG_CB cb);

void HciArbSendHciMsgToPrimaryStack(uint16 length, uint8 *data);

void HciArbSendNocpToPrimaryStack(HciArbHciHandle handle, uint16 nocp);

void HciArbProcessAclData(CsrHciAclDataInd *prim);

bool HciArbIsPassthroughMode(void);

HciArbAclInst *HciArbGetInstFromHandle(HciArbHciHandle hciHandle);

void HciArbSetFirstFragmentStack(HciArbAclInst *aclInst, HciArbStack stack);

HciArbStack HciArbGetFirstFragmentStack(HciArbAclInst *aclInst);

#ifdef CONFIG_BT_TESTER
bool HciArbiterSkipTestCmd(uint16 length, uint8 *payload);
#endif

#define HCI_LOG_ENABLE

#if defined(HCI_LOG_ENABLE)
#define HCI_LOG_INFO(...) CSR_LOG_TEXT_INFO((HCI_ARB , 0, __VA_ARGS__))
#define HCI_LOG_DEBUG(...) CSR_LOG_TEXT_INFO((HCI_ARB , 0, __VA_ARGS__))
#define HCI_LOG_WARNING(...)  CSR_LOG_TEXT_WARNING((HCI_ARB , 0, __VA_ARGS__))
#define HCI_LOG_ERROR(...)  CSR_LOG_TEXT_ERROR((HCI_ARB , 0, __VA_ARGS__))
#define HCI_PANIC(code, str)  {CsrPanic(CSR_TECH_BT, code, str);} 
#else
#define HCI_LOG_INFO(...)
#define HCI_LOG_DEBUG(...)
#define HCI_LOG_WARNING(...)
#define HCI_LOG_ERROR(...)
#define HCI_PANIC(code, str)  {CsrPanic(CSR_TECH_BT, code, str);} 
#endif


#ifdef __cplusplus
}
#endif

#endif /* __HCI_ARBITER_PRIVATE_H__ */

