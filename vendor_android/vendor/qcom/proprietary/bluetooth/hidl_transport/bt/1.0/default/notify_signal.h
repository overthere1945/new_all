/*==========================================================================
Description
 It has implementation for Signaling Power driver class

# Copyright (c) 2023 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

===========================================================================*/

#pragma once

#include <cutils/properties.h>
#include <hidl/HidlSupport.h>
#include <hci_uart_transport.h>
#include <logger.h>
#include "power_manager.h"

#include <stdint.h>

#define REGISTER_BT_PID     5

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {
// Sub-System SSR status

typedef enum {
  WAITING_FOR_SOC_ACCESS = 0,
  ACCESS_GRANTED = 1,
  SSR_ON_OTHER_CLIENT = 2,
  SSR_ON_OTHER_CLINET_COMPLETED = 3,
} SoCAccessWaitState;

typedef enum {
  SUB_STATE_IDLE = 0,
  SSR_ON_BT,
  BT_SSR_COMPLETED,
  SSR_ON_UWB,
  UWB_SSR_COMPLETED,
  UWB_SSR_STATE_UNKNOWN,
} ssr_states;

typedef enum {
  SOC_ACCESS_GRANTED = 0,
  SOC_ACCESS_DENIED = 1,
  SOC_ACCESS_RELEASED = 2,
  SOC_ACCESS_DISALLOWED = -1,
} SoCAccessState;

typedef enum {
    Q6_INVALID_STATE = 0,
    Q6_SSR_ON,
    Q6_SSR_COMPLETED,
} Q6SsrState_t;

typedef enum {
  RAMDUMP_FAILED = 0x01,
  SMC_SSR_ON,
  SMC_SSR_COMPLETED,
} SMC_Event_t;

class NotifySignal {
  public:
    NotifySignal();
    ~NotifySignal();
    static NotifySignal *Get(void);
    void RegSigIOCallBack(void);
    static void SigIOSignalHandler(int signum, siginfo_t *info, void *unused);
    static void SPIDriverSignalHandler(int uwb_ssr_state);
    static const int notify_signal_cmd_ = 0xbfe2;
    static const int soc_access_cmd = 0xbfe4;
    static const int warm_reset_cmd = 0xbff0;
    bool RegisterService(void);
    int GetSubSystemSsrStatus(void);
    bool getUWBSsrStatus(void);
    bool NotifyDriver(int SsrState);
    bool notifySubsystem;
    SoCAccessState RequestSoCAccess(void);
    SoCAccessState ReleaseSoCAccess(void);
    bool warmReset(void);
    bool WaitForSoCAccess();
 protected:
 private:
   static NotifySignal *instance_;
   static void SoCAccessSigHandler(SoCAccessWaitState);
   static void UpdateSoCAccessState(SoCAccessWaitState);
   static void Q6NotificationSigHandler(Q6SsrState_t);
   static void SMC_NotificationSigHandler(SMC_Event_t);
};


} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
