/******************************************************************************
Copyright (c) 2009-2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CSR_HCI_UTIL_H__
#define CSR_HCI_UTIL_H__

#include "csr_synergy.h"
#include "csr_hci_handler.h"
#include "csr_tm_bluecore_transport.h"
#include "csr_util.h"
#include "csr_hci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define used in in- and decoding of tunneled HCI messages.
   An unfragmented package is both start and end */
#define CSR_HCI_FRAGMENT_END        (0x80)
#define CSR_HCI_FRAGMENT_START      (0x40)
#define CSR_HCI_UNFRAGMENTED        (CSR_HCI_FRAGMENT_START | CSR_HCI_FRAGMENT_END)

#define CHANNEL_ID_DEBUG    (20)
#define CHANNEL_ID_MASK     (0x3F)

/*Refer to 7.7.16 in corev4.2 spec*/
#define CSR_HCI_EV_HARDWARE_ERROR                       ((CsrUint8) 0x10)
#define CSR_HCI_EV_PARAM_LEN_H4_LINK_SYNC_FAILURE       ((CsrUint8) 0x01)
/*CSR defined code for H4 sync loss*/
#define CSR_HCI_EV_H4_LINK_SYNC_FAILURE                 ((CsrUint8) 0xFE)

#define CSR_HCI_CMD_PACKET              (0x01)
#define CSR_HCI_CMD_RESET               (0x0003)
#define CSR_HCI_OGF                     (0x03)
#define CSR_HCI_RESET_OPCODE            ((CsrUint16) ((CSR_HCI_CMD_RESET & 0x03ff) | (CSR_HCI_OGF << 10)))
#define CSR_HCI_RESET_PAYLOAD_LEN       4
#define CSR_HCI_RESET_CMD_PARAMS        0

#define CSR_HCI_SET_XAP_UINT16(_data, _value)                           \
    {                                                                       \
        ((CsrUint8 *) _data)[0] = (CsrUint8) ((_value) & 0x00FF);                \
        ((CsrUint8 *) _data)[1] = (CsrUint8) (((_value) & 0xFF00) >> 8);        \
    }

#define CSR_HCI_GET_XAP_UINT16(_data) (CsrUint16) (((CsrUint8 *) (_data))[0] | ((CsrUint8 *) (_data))[1] << 8)

void CsrHciRegisterHandle(csrHciListType **main_list, CsrUint16 handle, CsrSchedQid queueId);
void CsrHciUnregisterHandle(csrHciListType **main_list, CsrUint16 handle, CsrSchedQid queueId);
void CsrHciUnregisterAllHandles(csrHciListType **main_list);

void CsrHciSendRegisterEventHandlerCfm(CsrSchedQid phandle);
void CsrHciSendRegisterAclHandlerCfm(CsrSchedQid phandle, CsrUint16 aclHandle);
void CsrHciSendUnregisterAclHandlerCfm(CsrSchedQid phandle, CsrUint16 aclHandle);
void CsrHciSendRegisterScoHandlerCfm(CsrSchedQid phandle, CsrUint16 scoHandle);
void CsrHciSendUnregisterScoHandlerCfm(CsrSchedQid phandle, CsrUint16 scoHandle);
void CsrHciSendRegisterVendorSpecificEventHandlerCfm(CsrSchedQid phandle, CsrUint8 channel);
void CsrHciSendUnregisterVendorSpecificEventHandlerCfm(CsrSchedQid phandle, CsrUint8 channel);
void CsrHciSendAclDataInd(CsrUint16 handlePlusFlags, CsrSchedQid phandle, CsrMblk *data, CsrBool functionPotential);
void CsrHciSendScoDataInd(CsrUint16 handlePlusFlags, CsrSchedQid phandle, CsrMblk *data);
void CsrHciSendEventInd(CsrSchedQid phandle, CsrUint8 *payload, CsrUint16 length);
void CsrHciSendVendorSpecificEventInd(CsrSchedQid phandle, CsrUint8 channel, CsrMblk *data);
void CsrHciSendResetTransportCfm(CsrSchedQid phandle, CsrBool restarted);
void CsrPeriSendRegisterEventHandlerCfm(CsrSchedQid phandle);
void CsrPeriSendEventInd(CsrSchedQid phandle, CsrUint8 *payload, CsrUint16 length);

csrHciListType *CsrHciGetHandler(csrHciListType *list, CsrUint16 handle);
CsrSchedQid CsrHciCheckHandlers(csrHciListType *list, CsrUint16 handle);

void CsrHciInsertAclHeader(CsrUint8 *payload, CsrUint16 handlePlusFlags, CsrUint16 length);
void CsrHciInsertScoHeader(CsrUint8 *payload, CsrUint16 handlePlusFlags, CsrUint16 length);
void CsrHciExtractAclHeader(CsrUint8 *payload, CsrUint16 *handlePlusFlags, CsrUint16 *length);
void CsrHciExtractScoHeader(CsrUint8 *payload, CsrUint16 *handlePlusFlags, CsrUint16 *length);

void CsrHciReassembleVendorSpecificEvents(CsrHciInstData *inst,
                                          CsrUint8 channel,
                                          CsrUint8 fragment,
                                          CsrUint8 event,
                                          CsrMblk *data);
void CsrHciSendVendorSpecificEventIndExt(CsrSchedQid phandle,
                                         CsrUint8 channel,
                                         CsrUint8 event,
                                         CsrMblk* data);

CsrBool CsrHciChannelLog(CsrUint8 channel);

#ifndef CSR_BLUECORE_ONOFF
void CsrHciSaveMessage(CsrHciInstData *inst);
void CsrHciRestoreSavedMessages(CsrHciInstData *inst);

#ifdef ENABLE_SHUTDOWN
void CsrHciDiscardSavedMessages(CsrHciInstData *inst);
#endif /* ENABLE_SHUTDOWN */
#endif /* CSR_BLUECORE_ONOFF */

#ifdef CSR_USE_QCA_CHIP
void CsrHciSendTmResetInd(CsrHciInstData *inst);
#else
void CsrHciSendTmNopInd(CsrHciInstData *inst, CsrUint8 *payload, CsrUint16 payloadLength);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_HCI_UTIL_H__ */
