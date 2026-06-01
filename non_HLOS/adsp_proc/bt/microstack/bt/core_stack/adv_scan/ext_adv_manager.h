/*!
Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
        All rights reserved

\file   ext_adv_manager.h

\brief  Extended Advertising (EA)
*/

#ifndef _EXT_ADV_MANAGER_H_
#define _EXT_ADV_MANAGER_H_
#include "ms_adv_scan_prim.h"
#include "csr_message_queue.h"
#include "dm_private.h"
#include "bluetooth.h"
#include "csr_gsched.h"
#include "csr_bt_usr_config.h"
#include "hci.h"
#include "csr_pmem_common.h"
#include "csr_msg_transport.h"
#include "dm_hci_interface.h"

#define ADV_HANDLE_FOR_LEGACY_API ((uint8_t) 0)


typedef CsrUint8 MsAdvScanStateGlobal;


typedef struct
{
    void                        *recvMsgP;
    adv_scan_prim_t             lockMsg;
    CsrMessageQueueType         *saveQueue;
    MsAdvScanStateGlobal           globalState;
    CsrSchedQid                     appHandle;
}msAdvScanInstanceData_t;
extern msAdvScanInstanceData_t  msAdvScanData;

/* Used to store information about a registered adv_set.
   NOTE: Advertising is assumed to be advertising if any of the advertising bits are
         set in field advertising. */
typedef struct
{
    uint8_t   info;            /* Bits defined above */
    uint8_t   advertising;     /* Bits defined above */

    /* Used when a connection is established on slave */
    uint8_t   ownAddrType;   /* Updated from HCI_ULP_EXT_ADV_SET_PARAMS
                                  or HCI_ULP_EXT_ADV_SET_PARAMS_V2 when sent
                                  successfully to controller */
    BD_ADDR_T ownRandomAddr; /* Stores last written or generated random address when
                                  HCI_ULP_EXT_ADV_SET_RANDOM_ADDR sent successfully to
                                  controller */

    uint32_t  flags;           /* Reserved for future use, always should be set to 0 */
} EXT_ADV_REGISTER_HANDLE_T;



/* This advertising handle value is used to say that an adv_handle is invalid. */
#define MS_EXT_ADV_HANDLE_INVALID ((uint8_t) 255)

/*** Bit settings for field flags ***/
/* bit 0      - Is any advertising set advertising? (true if set)
   bit 1 - 15 - Unspecified maybe any value. */
#define MS_EXT_ADVERTISING_ENABLED_FLAG  ((uint16_t) 0x0001)

/* Masks for field flags */
#define MS_EXT_ADVERTISING_ENABLED_MASK  ((uint16_t) 0x0001)

#define INVALID_MAX_ADV_DATA_LEN 0xFFFF
#define EXT_ADV_ENABLED ((uint32_t) 1)

/*** Bits that can be set in advertising field ***/
/* Set when advertising successfully enabled in controller */
#define ADV_ENABLED_CTRL         ((uint8_t) 0x01)

/* Advertising enable request waiting for controller confirmation */
#define ADV_CTRL_PENDING_ENABLE  ((uint8_t) 0x02)

/*** Bits that can be set in info field ***/
/* Set when advertising set is registered by application */
#define ADV_SET_REGISTERED       ((uint8_t) 0x01)

/* Set when Scan data is set for the adv_set */
#define ADV_SET_SCAN_DATA_IS_SET     ((uint8_t) 0x80)

/* Set when advertising filter policy uses the white list and is successfully
   configured in controller */
#define ADV_SET_POLICY_WHITELIST ((uint8_t) 0x04)

#define MS_ADV_SCAN_STATE_CHANGE(_var, _state)                            \
    do                                                                  \
    {                                                                   \
        CSR_LOG_TEXT_DEBUG((MsAdvScanLto,                                  \
                           MS_ADV_SCAN_LTSO_STATE,                        \
                           #_var" state: %d -> %d",                     \
                           (_var),                                      \
                           (_state)));                                  \
        (_var) = (_state);                                              \
    } while (0)

#define MS_ADV_SCAN_STATE_NOT_READY                   ((MsAdvScanStateGlobal) 0)
#define MS_ADV_SCAN_STATE_READY                        ((MsAdvScanStateGlobal) 1)

#define IS_MS_ADV_SCAN_MODULE_READY()  (msAdvScanData.globalState == MS_ADV_SCAN_STATE_READY)

extern int8_t lowestAdvHandleMicroStack;
#define MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle)     (advHandle - lowestAdvHandleMicroStack)
#define MAP_ADV_INDEX_TO_ADV_HANDLE(index)         (index + lowestAdvHandleMicroStack)
#define IS_ADV_HANDLE_VALID(advHandle)             ((advHandle >= lowestAdvHandleMicroStack) && (MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle) < maxSupportedAdvSets))

extern uint8_t maxSupportedAdvSets;
extern EXT_ADV_REGISTER_HANDLE_T appRegHandles[MS_EXT_ADV_MAX_ADV_HANDLES];

typedef void (*MS_EXT_ADV_ENABLE_CALLBACK_T)(uint8_t status, uint8_t adv_handle);
typedef void (*MS_EXT_ADV_MAX_DATA_LENGTH_CALLBACK_T)(uint8_t status, uint16_t max_length);
extern void MsExtAdvRegisterAppAdvSetReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtAdvUnregisterAppAdvSetReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtAdvSetParamsReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtAdvScanLocalQueueHandler(void);
extern void MsExtAdvManagerInit(void);
extern void MsExtAdvSendSetParamsV2Cfm(uint8_t status, CsrUint8          advHandle, int8_t selected_tx_pwr);
extern void MsExtAdvSetRandomAddrDone(uint8_t status, uint8_t adv_handle, BD_ADDR_T *addr);


extern void MsExtAdvMultiEnableReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtAdvSetsInfoReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtAdvSetRandomAddrReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtAdvSetScanRespDataReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtAdvSetDataReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsExtAdvReadMaxAdvDataLenReqHandler(msAdvScanInstanceData_t *msAdvScanData);
extern void MsAdvScanFreeDownstreamMessageContents(void *message);

extern void MsExtAdvSetDataCfmHandler(uint8_t status, uint8_t advHandle);
extern void MsExtAdvSetScanRespDataCfmHandler(uint8_t status, uint8_t advHandle);
extern void MsExtAdvReadMaxAdvDataLenCfmHandler(uint8_t status, uint16           maxAdvDataLen);
extern void MsExtAdvMultiEnableCommandDone(uint8_t status);
extern void MsExtAdvTerminatedIndHandler(void *msg);
extern void MsExtAdvRemoveAdvSetCommandDone(uint8_t status, uint8_t adv_handle);
extern void MsExtAdvUpdateAdvState(uint8_t status, HCI_ULP_EXT_ADV_ENABLE_T *req);
extern void MsExtAdvReadMaxLenCommandDone(uint8_t status, uint16_t max_len);
extern void MsExtAdvSetLowestAdvHandle(uint8_t lowestAdvHandle);
extern void MsExtAdvSetParamsCommandDone(HCI_ULP_EXT_ADV_SET_PARAMS_T *req, uint8_t status);
extern void MsExtAdvSetScanDataDone(uint8_t advHandle, uint8_t dataLen);


#endif /* _EXT_ADV_MANAGER_H_ */
