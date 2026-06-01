/******************************************************************************
 Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#ifndef __HCI_ARBITER_LIB_H__
#define __HCI_ARBITER_LIB_H__

#include "bluetooth.h"
#include "csr_prim_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Callback for notifying HCI Message to BT Middleware.
 *
 * @param   length  length of the HCI Message 
 * @param   data    pointer to HCI message. 
 *                  First byte includes the HCI packet indicator   

 * Memory is owned by BT Middleware in this case.

 */
typedef void (*HCI_ARBITER_MSG_CB)(uint16 length, uint8 *data);

/******************************************************************************
 * API to register callback for HCI message.
 *
 * @param  hciCb  callback function for handling HCI Message.
 *
 * The callback function should be short, and not block the Micro stack thread
 ******************************************************************************/
void HciArbiterRegisterHciMsgCb(HCI_ARBITER_MSG_CB hciCb);

/******************************************************************************
 * API to send HCI Message from primary stack. This is used by BT Middleware
 * This should include HCI packet indicator in first byte. e.g 1 for HCI command and 2 for ACL
 *
 * @param  length length of the HCI Message.
 * @param  payload pointer to HCI Message.
 *
 * Note : payload will be freed by Micro Stack.
 *        Primary stack is responsible for HCI flow control
 ******************************************************************************/
void HciArbiterSendHciMsg(uint16 length, uint8 *payload);


#ifdef __cplusplus
}
#endif

#endif /* __HCI_ARBITER_LIB_H__ */

