/*
 *  Copyright (c) 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved..
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#pragma once

#ifndef XM_WIFI_IF_H
#define XM_WIFI_IF_H

#pragma once

#include <stdint.h>
#include "xpan_utils.h"
#include "xm_main.h"

namespace xpan {
namespace implementation {

class XMWifiIf
{
  public:
   XMWifiIf();
   ~XMWifiIf();
   WifiDrvStatus InitWifiIf();
   bool DeInitWifiIf();
   bool PostMessage(xm_ipc_msg_t *);
   void WiFiIpcMsgHandler(xm_ipc_msg_t *);
   static void CbWiFiAcsResults(int freq, uint8_t status);
   static void CbWiFiTwtEvent(void*);
   static void CbSapPowerState(void *);
   static void CbWlanSsrEvent(uint8_t status);
   static void WifiAudioTransportSwtich(uint8_t, uint8_t, uint8_t);
   static void WifiAPAvb(uint16_t, uint8_t);
   static void WifiAPAvbRsp(bdaddr_t, uint8_t);
   static void CbChannelSwitchStarted(void *);
   static void CbDrvreadyEvent(uint8_t);
   static void CbCountryCodeUpdate(char *);
   void UpdateWifiDrvStatus(void);
   static void CbPossibleToCreateApIface();

  private:
   std::thread wifi_event_handling_thread;
   std::thread xm_wifi_if_thread;
   void XMWiFiThreadRoutine(void);
   static void usr_handler(int);
   void UseCaseUpdate(xm_ipc_msg_t *, uint8_t);
   void EnableAcs(xm_ipc_msg_t *);
   void EnableStats(xm_ipc_msg_t *);
   bool HostMacAddress(bdaddr_t);
   void SapPowerState(xm_ipc_msg_t *);
   void SapState(xm_ipc_msg_t *);
   void CreateSapInterface(xm_ipc_msg_t *);
   void StartWifiInfThread(void);
   void CbUseCaseUpdateStatus(uint8_t);
   void RegisterStationInterface(xm_ipc_msg_t *);
   void UnRegisterStationInterface(void);
   void WifiTransportSwitchReq(xm_ipc_msg_t *);
   void WifiBearerIndication(xm_ipc_msg_t *);
   void WifiTransportSwitchRsp(xm_ipc_msg_t *);
   void SetApAvailableReq(xm_ipc_msg_t *);
   void CancelApAvailable(xm_ipc_msg_t *);
   void UpdateConnectedEbDetails(xm_ipc_msg_t *);
   void UpdateDisConnectedEbDetails(xm_ipc_msg_t *);
   void TeardownTwt(xm_ipc_msg_t *);
   std::mutex xm_wifi_if_mtx;
   std::queue <xm_ipc_msg_t *> xm_wifi_if_workqueue;
   std::atomic_bool xm_wifi_if_thread_running{false};
   std::atomic_bool is_wifi_if_thread_busy{false};
   static std::mutex dialog_id_mtk;
   int write_fd;
   int read_fd;
};

} // namespace implementation
} // namespace xpan

#endif
