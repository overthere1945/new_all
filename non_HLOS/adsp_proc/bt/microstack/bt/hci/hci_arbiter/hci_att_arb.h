/*!
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
        All rights reserved

\file   hci_att_arb.h

\brief  HCI command arbitration
*/

#ifndef _HCI_ATT_ARB_H_
#define _HCI_ATT_ARB_H_

#include "csr_message_queue.h"
#include "dm_private.h"
#include "bluetooth.h"
#include "csr_gsched.h"
#include "csr_bt_usr_config.h"
#include "hci.h"
#include "csr_pmem_common.h"
#include "csr_msg_transport.h"
#include "dm_hci_interface.h"
#include "hci_att_arb_prim.h"
#include "csr_hci_prim.h"
#include "hci.h"
#include "bluetooth.h"
#include "l2cap_types.h"


typedef struct
{
    void                        *recvMsgP;
}hciAttArbInstanceData_t;

extern hciAttArbInstanceData_t  hciAttArbData;
extern CsrSchedQid HCI_ATT_ARB_IFACEQUEUE;

void HciAttArbInit(void **gash);
extern void HciAttArbIfaceHandler(void **gash);
extern HciAttArbAttMsgReq *HciAttArbAttMsg_struct(AttMsgSender sender, 
                                            TXQE_T *txqe,
                                            L2CAP_CHCB_T *chcb,
                                            uint8_t priority,
                                            bool_t fromNhcp);

extern HciAttArbHouseCleaningReq *HciAttArbHouseCleaningReq_struct(AttMsgSender sender, L2CAP_CHCB_T *chcb);

extern bool HciAttArbLockQueue(AttMsgSender sender, CsrUint16 id, hciAttArbInstanceData_t *hciAttArbData, ATT_MSG_ACTION msgAction);
extern bool IsHciAttArbQueueLocked(AttMsgSender sender, hciAttArbInstanceData_t *hciAttArbData, L2CAP_CHCB_T *chcb);
extern void HciAttArbUnlockQueue(AttMsgSender sender,  L2CAP_CHCB_T *chcb, hciAttArbInstanceData_t *hciAttArbData, ATT_ROLE role);

extern bool IsMicroStackWaitingForAnyResponseEvent(void);
extern bool HasPrimaryStackTakenTheHCIAttArbLock(void);
extern void HciAttArbClearOffloadedGATTAclHandle(L2CAP_CHCB_T *chcb);
extern HciArbStack HciAttArbGetDestinationStack(HciArbHciHandle hciHandle, 
                                          HciArbGattRole role, 
                                          HciArbL2capCid cid);

extern AttMsgSender HciAttArbGetDestinationAndUnlockQueue(uint16 aclHandle, ATT_ROLE role, HciArbL2capCid cid);

#define HciAttArbAttMsgSend(sender, txqe, chcb, priority, fromNhcp){ \
        HciAttArbAttMsgReq *__msg; \
        __msg = HciAttArbAttMsg_struct(sender,  txqe, chcb, priority, fromNhcp); \
        CsrMsgTransport(HCI_ATT_ARB_IFACEQUEUE, HCI_ATT_ARB_PRIM, __msg); \
}


#define HciAttArbHouseCleaningSend(sender, chcb){ \
        HciAttArbHouseCleaningReq *__msg; \
        __msg = HciAttArbHouseCleaningReq_struct(sender, chcb); \
        CsrMsgTransport(HCI_ATT_ARB_IFACEQUEUE, HCI_ATT_ARB_PRIM, __msg); \
}



#endif /* _HCI_ATT_ARB_H_ */
