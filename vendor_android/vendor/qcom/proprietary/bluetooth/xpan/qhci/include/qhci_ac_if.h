/*
 * Copyright (c) 2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

//#pragma once
#ifndef XPAN_QHCI_AC_IF
#define XPAN_QHCI_AC_IF

#include <stdint.h>
#include "xpan_utils.h"

namespace xpan {
namespace implementation {

typedef struct {
  uint8_t version;
  uint16_t companyId;
  uint16_t subversion;
} QHciLocalVer;

class XpanQhciAcIf {
 private:
  XpanQhciAcIf();


 public:
  static std::shared_ptr<XpanQhciAcIf> GetIf();
  void DeInit();
  ~XpanQhciAcIf();
  friend class QHci;

  bool CreateConnection(bdaddr_t addr,        uint16_t supervision_timeout,
                              bool is_bg);
  bool DisconnectConnection(bdaddr_t addr);
  bool GetRemoteVersion  (bdaddr_t addr);
  bool GetRemoteLeFeatures   (bdaddr_t addr);
  bool EnableEncrption(bdaddr_t addr,
                             const std::vector<uint8_t>& ltk,
                             bool is_rsp_needed);
  bool SendAclData(bdaddr_t addr, uint8_t llid,
                        const std::vector<uint8_t>& data);
  bool L2capPauseUnpauseRes(bdaddr_t addr, uint8_t pause,
                                    uint8_t status);
  bool UpdateLocalVersion(uint8_t version, uint16_t companyId,
                                 uint16_t subversion);
  bool UpdateLocalLeFeatures(uint64_t le_features);
  bool L2capPauseUnpauseReq (bdaddr_t bdAddr, uint8_t pause, TransportType transport);
  void BearerSwitchInd(bdaddr_t addr, TransportType transport, uint8_t status);
  bool ConnectionCompleteRes (bdaddr_t bdAddr, uint8_t status);
  bool DisconnectionCompleteRes (bdaddr_t bdAddr, uint8_t status);
  bool RemoteVersionRes (bdaddr_t bdAddr, uint8_t version, uint16_t companyId,
                                uint16_t subversion);
  bool RemoteLeFeaturesRes(bdaddr_t bdAddr, uint8_t status, uint64_t featureMask);
  bool EncryptionChangedRes(bdaddr_t bdAddr, uint8_t status, uint8_t encryptionEnabled);
  bool DataReceivedCb(bdaddr_t bdAddr,  uint8_t llid, const std::vector<uint8_t>& data);
  bool NumberOfPacketsCompleted(bdaddr_t bdAddr,  uint8_t noOfPacketsSent);
  bool isStreamingActive(bdaddr_t bd_addr);
  bool CreateConnectionCancel(bdaddr_t addr);
  bool RemoteAvaiableForConn(bdaddr_t bdAddr);
  void DisconnectLeLink(bdaddr_t bdaddr);
  bool isLeLinkconnected(bdaddr_t bdAddr);
  bool isQhciInitiatedSeamlessPending(bdaddr_t bdAddr);
  bool LeLinkUpdate(bdaddr_t bdAddr, bool link_status, TransportType type = XPAN_AP);
  QHciLocalVer GetLocalLeVersion();
  uint64_t GetLocalLeFeatures();
  void StreamStatusUpdate(bdaddr_t addr, uint8_t stream_status);
  void UpdateIdleTransitionStatus(bdaddr_t addr, TransportType transport_type,
                              uint8_t status);

  private:
  static std::shared_ptr<XpanQhciAcIf> qhci_ac_instance;
};

} // namespace ac
} // namespace xpan

#endif
