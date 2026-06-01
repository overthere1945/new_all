/*!
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
        All rights reserved

\file   hci_cmd_arb.h

\brief  HCI command arbitration
*/

#ifndef _HCI_CMD_ARB_H_
#define _HCI_CMD_ARB_H_

#include "csr_message_queue.h"
#include "dm_private.h"
#include "bluetooth.h"
#include "csr_gsched.h"
#include "csr_bt_usr_config.h"
#include "hci.h"
#include "csr_pmem_common.h"
#include "csr_msg_transport.h"
#include "dm_hci_interface.h"
#include "hci_cmd_arb_prim.h"
#include "csr_hci_prim.h"
#include "hci.h"


typedef CsrUint8 HciCmdArbGlobal;


typedef struct
{
    void                        *recvMsgP;
    HciCmdArbGlobal             globalState;
    HciCmdArbPrim               lockMsgMicroStack;
    HciCmdArbPrim               lockMsgPrimaryStack;
    uint8                       lockMsgPrimaryStackCount;
    hci_op_code_t               lockMsgMicroStackCmdOpcode;
    HciCmdArbPrim               lockMsgSplArbLogic;


    CsrMessageQueueType         *saveMicroStackQueue;
    CsrMessageQueueType         *savePrimaryStackQueue;

}hciCmdArbInstanceData_t;

extern hciCmdArbInstanceData_t  hciCmdArbData;
extern CsrSchedQid HCI_CMD_ARB_IFACEQUEUE;

void HciCmdArbInit(void **gash);
extern void HciCmdArbIfaceHandler(void **gash);
extern HciArbCmdReq *HciArbCommandReq_struct(uint8 sender, CsrUint16 payloadLength, CsrUint8 *payload);
extern HciPeriArbCmdReq *HciPeriArbCommandReq_struct(uint8 sender, CsrUint16 payloadLength, CsrUint8 *payload);
extern HciArbHouseCleaningReq *HciArbHouseCleaningReq_struct(uint8 sender);
extern HciArbVsCmdReq *HciArbVsCommandReq_struct(uint8 sender, void *data);
extern void HciCmdArbLockQueue(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData);
extern bool IsHciCmdArbQueueLocked(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData);
extern void HciCmdArbUnlockQueue(uint8 sender, hciCmdArbInstanceData_t *hciCmdArbData);

extern bool IsMicroStackWaitingForAnyResponseEvent(void);
extern bool HasPrimaryStackTakenTheHCICmdArbLock(void);

#define HciCommandArbCmdSend(sender, _payloadLength, _payload){ \
        HciArbCmdReq *__msg; \
        __msg = HciArbCommandReq_struct(sender, _payloadLength, _payload); \
        CsrMsgTransport(HCI_CMD_ARB_IFACEQUEUE, HCI_CMD_ARB_PRIM, __msg); \
}

#define HciPeriCommandArbCmdSend(sender, _payloadLength, _payload){ \
        HciPeriArbCmdReq *__msg; \
        __msg = HciPeriArbCommandReq_struct(sender, _payloadLength, _payload); \
        CsrMsgTransport(HCI_CMD_ARB_IFACEQUEUE, HCI_CMD_ARB_PRIM, __msg); \
}

#define HciCommandArbHouseCleaningSend(sender){ \
        HciArbHouseCleaningReq *__msg; \
        __msg = HciArbHouseCleaningReq_struct(sender); \
        CsrMsgTransport(HCI_CMD_ARB_IFACEQUEUE, HCI_CMD_ARB_PRIM, __msg); \
}

#define HciCommandArbVSCmdSend(sender, data){ \
        HciArbVsCmdReq *__msg; \
        __msg = HciArbVsCommandReq_struct(sender, data); \
        CsrMsgTransport(HCI_CMD_ARB_IFACEQUEUE, HCI_CMD_ARB_PRIM, __msg); \
}

#endif /* _HCI_CMD_ARB_H_ */
