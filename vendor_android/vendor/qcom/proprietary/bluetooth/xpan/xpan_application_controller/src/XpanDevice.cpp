/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc. 
 */

#include <log/log.h>
#include "xpan_ac_int.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.bluetooth.xpan_ac.XpanDevice"

using xpan::implementation::XpanQhciAcIf;
using xpan::implementation::XMXacIf;

/* Xpan Remote Device Instance */
namespace xpan {
namespace ac {

XpanDevice::XpanDevice(bdaddr_t address) {
  addr = address;
  mPrimaryDetails = {};
  mSecondaryDetails = {};
  mSupervisionTimeout = 10000;
  mPingTimeout = mSupervisionTimeout * 0.75;
  xm = XMXacIf::GetIf();
  qhci = XpanQhciAcIf::GetIf();
  mDnsUuid = {};// just for testing
  memset(&mLtk, 0, sizeof(mLtk));
  memset(&pPsk, 0, sizeof(pPsk));
}

XpanDevice::~XpanDevice() {
  // CLose listening tcp socket for this device if opened
  XpanSocketHandler::CloseListeningSocket(addr);

  addr = {};
  mPrimaryDetails = {};
  mSecondaryDetails = {};

  if (psm) {
    delete psm;
    psm = NULL;
  }

  if (ssm) {
    delete ssm;
    ssm = NULL;
  }

  if (pReinitConnTimer) {
    pReinitConnTimer->StopTimer();
    pReinitConnTimer = NULL;
  }

  xm = NULL;
}

void XpanDevice::SetDeviceSecurityDetails(mdns_uuid_t uuid, uint8_t *psk, uint8_t *ltk,
                                          long feature_mask) {
  mDnsUuid = uuid;
  memcpy(pPsk, psk, sizeof(pPsk));
  memcpy(mLtk, ltk, sizeof(mLtk));
  mFeatureMask = feature_mask;
  XpanApplicationController::PrintKey(mLtk, "LTK");
  XpanApplicationController::PrintKey(pPsk, "PSK");
  XpanTLSHandler::SetPSKWithIdentity(uuid, pPsk);
  ALOGD("%s: supported feature mask = %X", __func__, feature_mask);
}

void XpanDevice::SetIpAddress(XpanEarbudRole role, ipaddr_t ip) {
  if (role == ROLE_PRIMARY) {
    ALOGD("%s: Primary IP address set to %s", __func__, ip.toString().c_str());
    mPrimaryDetails.ip = ip;
  } else if(role == ROLE_SECONDARY) {
    ALOGD("%s: Secondary IP address set to %s", __func__, ip.toString().c_str());
    mSecondaryDetails.ip = ip;
  }
}

ipaddr_t XpanDevice::GetIpAddr(XpanEarbudRole role) {
  if (role == ROLE_PRIMARY) {
    return mPrimaryDetails.ip;
  } else if (role == ROLE_SECONDARY) {
    return mSecondaryDetails.ip;
  }

  return {-1, {}, {}};
}

bdaddr_t XpanDevice::GetAddr() {
  return addr;
}

mdns_uuid_t XpanDevice::GetRemoteMdnsUuid() {
  return mDnsUuid;
}

XpanEarbudRole XpanDevice::GetRoleByIpAddr(ipaddr_t ip) {
  if (ip == mPrimaryDetails.ip) {
    return ROLE_PRIMARY;
  } else if (ip == mSecondaryDetails.ip) {
    return ROLE_SECONDARY;
  }
  return ROLE_INVALID;
}

XpanAcStateMachine* XpanDevice::GetStateMachine(XpanEarbudRole role) {
  if (role == ROLE_PRIMARY) {
    return psm;
  } else if (role == ROLE_SECONDARY) {
    return ssm;
  }
  return NULL;
}

XpanLmpManager* XpanDevice::GetLmpManager(ipaddr_t ip) {
  XpanEarbudRole role =  this->GetRoleByIpAddr(ip);
  if (role == ROLE_PRIMARY && psm) {
    return psm->GetLmpManager();
  } else if (role == ROLE_SECONDARY && ssm) {
    return ssm->GetLmpManager();
  }
  return NULL;
}

XpanSocketHandler* XpanDevice::GetSocketHandler(XpanEarbudRole role) {
  if (role == ROLE_PRIMARY && psm) {
    return psm->GetSocketHandler();
  } else if (role == ROLE_SECONDARY && ssm) {
    return ssm->GetSocketHandler();
  }
  return NULL;
}

long XpanDevice::GetFeatureMask() {
  return mFeatureMask;
}

/* Xpan Manager Calls */
void XpanDevice::PrepareBearer (xac_handler_msg_t* msg) {
  TransportType bearer = msg->prepareBearer.bearer;
  bool urgency = msg->prepareBearer.urgency;

  ALOGD("%s: device address = %s, bearer = %d urgency = %d", __func__
        , ConvertRawBdaddress(msg->prepareBearer.addr)
        , bearer, urgency);

  /* Set urgency in primary state machine */
  if (psm && psm->GetState() != XPAN_IDLE &&
      psm->GetState() != XPAN_AP_ACTIVE && bearer == XPAN_AP) {
    ALOGD("%s with urgency = %d", __func__, urgency);
    XmLeApUrgencyEvent evt = {XM_LE_TO_AP_URGENCY_EVT, urgency};
    psm->XpanAcSmExecuteEvent((xac_handler_msg_t *)&evt);
    return;
  }

  if (bearer == NONE) {
    /* Unprepare bearer request */
    ALOGD("%s: Unprepare bearer request ", __func__);
    UpdateEarbudConnectionStatusToCp(ROLE_PRIMARY, XPAN_EB_DISCONNECTED);
    xm->PrepareAudioBearerRsp(addr, XM_SUCCESS);
    /* Set trigger as NONE to handle case if secondary EB comes out of case
       after Pause (as secondary doesnt need to start LMP BS procedure)*/
    if (psm) {
      psm->SetTrigger(QHCI_CONNECTION);
      psm->SetSupervisionTimer(XPAN_TCP_SUPERVISION_TIMEOUT, XPAN_PERIODIC_PING_TIMER);
    }
    if (ssm) {
      ssm->SetTrigger(QHCI_CONNECTION);
      ssm->SetSupervisionTimer(XPAN_TCP_SUPERVISION_TIMEOUT, XPAN_PERIODIC_PING_TIMER);
    }
    return;
  }

  if (bearer == XPAN_AP) {
    /* discard prepare bearer request if remote AP details are not received */
    if (!mPrimaryDetails.detailsSet || XpanApplicationController::GetLocalIpAddr().isEmpty()) {
      ALOGE("%s: Remote AP Details are not set. Prepare Bearer failed.", __func__);
      // Xpan Manager callback for bearer switch failure
      xm->PrepareAudioBearerRsp(addr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
      return;
    }

    if (!psm) {
      ALOGD("%s: Creating primary earbud state machine.", __func__);
      psm = new XpanAcStateMachine(this, msg->createConn.addr, ROLE_PRIMARY, BT_LE);
      if (mPrimaryDetails.detailsSet) {
        psm->SetEarbudProperties(mPrimaryDetails.ip, mPrimaryDetails.mac_addr,
                                 mPrimaryDetails.audio_location, mPrimaryDetails.tcp_port);
      }
    }

    // Check if its IDLE to STREAMING transition on AP transport
    if (psm->GetState() == XPAN_AP_ACTIVE && !mPrimaryRoaming.detailsSet) {
      ALOGD("%s: IDLE to STREAMING transition on AP transport", __func__);
      psm->SetTransitionType(XPAN_AP_IDLE_TO_STREAMING);
      if (psm && psm->GetState() == XPAN_AP_ACTIVE &&
          ssm && ssm->GetState() == XPAN_AP_ACTIVE) {
        UpdateEarbudConnectionStatusToCp(ROLE_SECONDARY, XPAN_EB_CONNECTED);
        psm->SetSupervisionTimer(XPAN_TCP_SUPERVISION_TIMEOUT, XPAN_PERIODIC_PING_TIMER);
        ssm->SetSupervisionTimer(XPAN_TCP_SUPERVISION_TIMEOUT, XPAN_PERIODIC_PING_TIMER);
      } else if ((psm && psm->GetState() == XPAN_AP_ACTIVE) &&
                 (!ssm || (ssm && ssm->GetState() != XPAN_AP_ACTIVE))) {
        // only primary connected
        UpdateEarbudConnectionStatusToCp(ROLE_PRIMARY, XPAN_EB_CONNECTED);
        psm->SetSupervisionTimer(XPAN_TCP_SUPERVISION_TIMEOUT, XPAN_PERIODIC_PING_TIMER);
      }
      return;
    }

    // Set trigger for LMP bearer switch only if it is triggered from remote bearer preference
    mTrigger = XM_PREPARE_AUDIO_BEARER;

    psm->SetTrigger(mTrigger);
    psm->XpanAcSmExecuteEvent(msg);

    if (!ssm && !mSecondaryDetails.detailsSet) {
      ALOGE("%s: Secondary EB details are not available yet. Create SSM later", __func__);
      return;
    }

    if (!ssm && mSecondaryDetails.detailsSet) {
      ALOGD("%s: Creating Secondary earbud state machine.", __func__);
      ssm = new XpanAcStateMachine(this, msg->createConn.addr, ROLE_SECONDARY, BT_LE);
      if (mSecondaryDetails.detailsSet) {
        ssm->SetEarbudProperties(mSecondaryDetails.ip, mSecondaryDetails.mac_addr,
                                 mSecondaryDetails.audio_location, mSecondaryDetails.tcp_port);
      }
    }

    if (ssm) {
      ssm->SetTrigger(mTrigger);
      ssm->XpanAcSmExecuteEvent(msg);
    }

    /* Remote sent Bearer Preference with urgency set */
    if (psm && urgency) {
      ALOGD("%s: Prepare Bearer with urgency set already", __func__);
      XmLeApUrgencyEvent evt = {XM_LE_TO_AP_URGENCY_EVT, urgency};
      psm->XpanAcSmExecuteEvent((xac_handler_msg_t *)&evt);
    }
  } else if (bearer == BT_LE) {
    if (!psm || (psm && psm->GetState() != XPAN_AP_ACTIVE)) {
      ALOGE("%s: Current transport is not XPAN_AP", __func__);
      // callback to XM as success. No impact to XAC
      xm->PrepareAudioBearerRsp(addr, XM_SUCCESS);
      return;
    }

    if (psm) {
      psm->XpanAcSmExecuteEvent(msg);
    }
    if (ssm) {
      ssm->XpanAcSmExecuteEvent(msg);
    }
  }
}

void XpanDevice::BearerPreference (xac_handler_msg_t* msg) {
  TransportType bearer = msg->bearerPreference.bearer;
  bool urgency = msg->bearerPreference.is_urgent;
  if (psm && psm->GetState() != XPAN_IDLE && bearer == XPAN_AP) {
    ALOGD("%s with urgency = %d", __func__, urgency);
    xm->BearerPreferenceRsp(addr, bearer, XM_SUCCESS);
    XmLeApUrgencyEvent evt = {XM_LE_TO_AP_URGENCY_EVT, urgency};
    psm->XpanAcSmExecuteEvent((xac_handler_msg_t *)&evt);
    return;
  }

  /* TODO: Robustness - First message itself is with urgency */

  mTrigger = XM_BEARER_PREFERENCE;

  ALOGD("%s: device address = %s, bearer = %d", __func__
        , ConvertRawBdaddress(msg->bearerPreference.addr)
        , bearer);

  /* Transition: BT_LE to AP */
  if (bearer == XPAN_AP) {
    /* discard prepare bearer request if remote AP details are not received */
    if (!mPrimaryDetails.detailsSet || XpanApplicationController::GetLocalIpAddr().isEmpty()) {
      ALOGE("%s: Remote/Local AP Details are not set. Bearer Preference failed.", __func__);
      // Xpan Manager callback for bearer switch failure
      //xm->BearerPreferenceRsp(addr, bearer, XM_AC_BEARER_PREFERENCE_REJECTED);
      return;
    }

    if (psm && psm->GetState() == XPAN_AP_ACTIVE) {
      ALOGD("%s: AP Roaming!! Ignore request", __func__);
      //xm->BearerPreferenceRsp(addr, XPAN_AP, XM_SUCCESS);
      return;
    }

    if (!psm) {
      ALOGD("%s: Creating primary earbud state machine.", __func__);
      psm = new XpanAcStateMachine(this, msg->createConn.addr, ROLE_PRIMARY, BT_LE);
      if (mPrimaryDetails.detailsSet) {
        psm->SetEarbudProperties(mPrimaryDetails.ip, mPrimaryDetails.mac_addr,
                                 mPrimaryDetails.audio_location, mPrimaryDetails.tcp_port);
      }
    }

    /* Update the QHCI module that IDLE transition has started.
     * This is done to handle stream start if triggered during idle transition.
     */
    if (qhci) {
      qhci->UpdateIdleTransitionStatus(addr, bearer, IDLE_TRANSITION_STARTED);
    }

    psm->SetTrigger(mTrigger);
    psm->XpanAcSmExecuteEvent(msg);

    if (!ssm && !mSecondaryDetails.detailsSet) {
      ALOGE("%s: Secondary EB details are not available yet. Create SSM later", __func__);
      return;
    }

    if (!ssm && mSecondaryDetails.detailsSet) {
      ALOGD("%s: Creating Secondary earbud state machine.", __func__);
      ssm = new XpanAcStateMachine(this, msg->createConn.addr, ROLE_SECONDARY, BT_LE);
      if (mSecondaryDetails.detailsSet) {
        ssm->SetEarbudProperties(mSecondaryDetails.ip, mSecondaryDetails.mac_addr,
                                 mSecondaryDetails.audio_location, mSecondaryDetails.tcp_port);
      }
    }

    if (ssm) {
      ssm->SetTrigger(mTrigger);
      ssm->XpanAcSmExecuteEvent(msg);
    }

  /* Transition: AP -> BT_LE */
  } else if (bearer == BT_LE) {
    if (!psm || (psm && psm->GetState() != XPAN_AP_ACTIVE)) {
      ALOGE("%s: Current transport is not XPAN_AP", __func__);
      // callback to XM with rejection
      //xm->BearerPreferenceRsp(addr, BT_LE, XM_AC_BEARER_PREFERENCE_REJECTED);
      return;
    }

    if (psm) {
      psm->XpanAcSmExecuteEvent(msg);
    }
    if (ssm) {
      ssm->XpanAcSmExecuteEvent(msg);
    }
  }
}

void XpanDevice::UpdateRemoteApParams (xac_handler_msg_t* msg) {
  uint8_t num_of_devices = msg->remoteApParams.num_of_earbuds;

  // check if roaming update
  if (num_of_devices != NONE_CONNECTED && !apBssid.isEmpty() &&
      !(apBssid == msg->remoteApParams.eb_bssid)) {
    if (DirectRoamingUpdateToCp(msg)) {
      return;
    }
  }

  if (num_of_devices != NONE_CONNECTED) {
    apBssid = msg->remoteApParams.eb_bssid;
  } else {
    apBssid = {};
  }

  for (int i = 0; i < num_of_devices; i++) {
    /* First remote details should always be primary details as per spec */
    if (msg->remoteApParams.role[i] == ROLE_PRIMARY) {
      ALOGD("%s: Details for Primary received/updated. isMdnsUpdate = %d", __func__,
            msg->remoteApParams.is_mdns_update);
      mPrimaryDetails.ip = msg->remoteApParams.ip[i];
      mPrimaryDetails.role = (XpanEarbudRole)msg->remoteApParams.role[i];
      mPrimaryDetails.tcp_port = msg->remoteApParams.remote_tcp_port;

      /* Below details to be set upon IP address notification only */
      //if (!msg->remoteApParams.is_mdns_update) {
        mPrimaryDetails.detailsSet = true;
        mPrimaryDetails.mac_addr = msg->remoteApParams.mac[i];
        mPrimaryDetails.audio_location = msg->remoteApParams.audio_loc[i];
        mPrimaryDetails.udp_data_port = msg->remoteApParams.remote_udp_port;
        mPrimaryDetails.udp_tsf_port = msg->remoteApParams.remote_udp_tsf_port;
      //}
      DisplayRemoteApDetails(ROLE_PRIMARY);
      mDnsUuid = XpanApplicationController::SetUuidOrder(msg->remoteApParams.mdns_uuid);
      ComputePskFromLtk();

      if (psm) {
        /* Note:-
           1. Reconnection: Remote details through mdns will be handled in
                            XPAN_TCP_CONNECTING State to establish tcp connection.
           2. Reconnection: Remote details through IP address notification would be
                            cached in.
           3. LE->AP : Details Cached in and on receiving Prepare Bearer or Bearer Preference
                       tcp connection is made.
         */
        psm->SetEarbudProperties(mPrimaryDetails.ip, mPrimaryDetails.mac_addr,
                                 mPrimaryDetails.audio_location, mPrimaryDetails.tcp_port);
        if (mTrigger == QHCI_CONNECTION && msg->remoteApParams.is_mdns_update) {
          psm->XpanAcSmExecuteEvent(msg);
        }
      }
    } else if (msg->remoteApParams.role[i] == ROLE_SECONDARY) {
      ALOGD("%s: Details for Secondary received/updated", __func__);
      mSecondaryDetails.detailsSet = true;
      mSecondaryDetails.mac_addr = msg->remoteApParams.mac[i];
      mSecondaryDetails.ip = msg->remoteApParams.ip[i];
      mSecondaryDetails.role =  msg->remoteApParams.role[i];
      mSecondaryDetails.audio_location = msg->remoteApParams.audio_loc[i];
      mSecondaryDetails.tcp_port = msg->remoteApParams.remote_tcp_port;
      mSecondaryDetails.udp_data_port = msg->remoteApParams.remote_udp_port;
      mSecondaryDetails.udp_tsf_port = msg->remoteApParams.remote_udp_tsf_port;
      DisplayRemoteApDetails(ROLE_SECONDARY);

      /* Determine if secondary earbud IP is received after primary
         in separate notification */
      uint8_t psmState = (psm ? psm->GetState() : XPAN_IDLE);
      if (!ssm && mSecondaryDetails.detailsSet
               && psmState != XPAN_IDLE && psmState != XPAN_DISCONNECTING) {
        ALOGD("%s: Creating Secondary earbud state machine. mTrigger = %d", __func__, mTrigger);
        uint8_t transport_from = psm->GetTransportFrom();
        ssm = new XpanAcStateMachine(this, msg->createConn.addr, ROLE_SECONDARY,
                                     (TransportType)transport_from);
        ssm->SetEarbudProperties(mSecondaryDetails.ip, mSecondaryDetails.mac_addr,
                                 mSecondaryDetails.audio_location,
                                 mSecondaryDetails.tcp_port);
        ssm->SetTrigger(mTrigger);

        if (mTrigger == QHCI_CONNECTION || psm->GetState() == XPAN_AP_ACTIVE) {
          ALOGD("%s: Creating Secondary earbud connection", __func__);
          QhciCreateConnection conn{QHCI_CREATE_CONNECTION_EVT, addr, 5000/*TODO*/};
          ssm->XpanAcSmExecuteEvent((xac_handler_msg_t *)&conn);
          ssm->XpanAcSmExecuteEvent(msg);
        } else if (mTrigger == XM_PREPARE_AUDIO_BEARER || mTrigger == XM_BEARER_PREFERENCE) {
          ALOGD("%s: Send PrepareBearer/BearerPreference (%d)"
                 " to secondary state machine.", __func__, mTrigger);
          /* Note:- mTriger is translated to transition type */
          XacEvent transitionEvent = (mTrigger == XM_PREPARE_AUDIO_BEARER ?
              XM_PREPARE_BEARER_EVT: XM_BEARER_PREFERENCE_EVT);
          XmPrepareBearer prepareBearer{(XacEvent)transitionEvent, msg->remoteApParams.addr,
                                        XPAN_AP};
          ssm->XpanAcSmExecuteEvent((xac_handler_msg_t *)&prepareBearer);

        }
      }
    }
  }

  /* If secondary is disconnected */
  if (num_of_devices == PRIMARY_CONNECTED && mSecondaryDetails.detailsSet) {
    ALOGD("%s: Secondary earbud disconnected (%s: ip address = %s, mac_address = %s)"
          , __func__, ConvertRawBdaddress(addr), mSecondaryDetails.ip.toString().c_str()
          , ConvertRawMacaddress(mSecondaryDetails.mac_addr));
    XmRemoteDisconnectedEvent s_eb_disconn{XPAN_REMOTE_DISCONNECTED_EVT, addr, ROLE_SECONDARY};
    RemoteDisconnectedEvent((xac_handler_msg_t *)&s_eb_disconn);
    mSecondaryDetails = {};

  /* If both are disconnected */
  } else if (num_of_devices == NONE_CONNECTED) {
    ALOGD("%s: Both primary and secondary disconnected.", __func__);
    XmRemoteDisconnectedEvent p_eb_disconn{XPAN_REMOTE_DISCONNECTED_EVT, addr, ROLE_PRIMARY};
    RemoteDisconnectedEvent((xac_handler_msg_t *)&p_eb_disconn);

    XmRemoteDisconnectedEvent s_eb_disconn{XPAN_REMOTE_DISCONNECTED_EVT, addr, ROLE_SECONDARY};
    RemoteDisconnectedEvent((xac_handler_msg_t *)&s_eb_disconn);

    mPrimaryDetails = {};
    mSecondaryDetails = {};
  }
}

void XpanDevice::RequestRemoteApParams(xac_handler_msg_t* msg) {
  bdaddr_t addr = msg->reqRemoteApParams.addr;
  ALOGD("%s: Device = %s", __func__, ConvertRawBdaddress(addr));

  macaddr_t local_mac = XpanApplicationController::GetLocalMacAddr();
  macaddr_t local_ap_bssid = XpanApplicationController::GetLocalApBssid();
  ipaddr_t ip_addr = XpanApplicationController::ChangeIpEndianness(
                        XpanApplicationController::GetLocalIpAddr());
  uint32_t remote_tcp_port = mPrimaryDetails.tcp_port;
  uint32_t remote_udp_port = mPrimaryDetails.udp_data_port;
  uint32_t remote_udp_tsf = XpanSocketHandler::GetUdpTsfPort();
  uint32_t local_udp_tsf = XpanSocketHandler::GetUdpTsfPort();
  uint32_t local_udp_data = XpanSocketHandler::GetUdpPort();
  bool is_dtls_enabled = XpanApplicationController::IsDTLSEnabled();

  std::vector<RemoteEbParams> ebParams;

  if (psm && psm->GetState() != XPAN_AP_ACTIVE) {
    ALOGW("%s: Primary is not connected.", __func__);
    return;
  }
  /* Add primary details as primary details are always there */
  RemoteEbParams ebPrim;
  ebPrim.eb_audio_loc = mPrimaryDetails.audio_location;
  ebPrim.eb_ip_addr =  XpanApplicationController::ChangeIpEndianness(mPrimaryDetails.ip);
  ebPrim.eb_mac_addr = mPrimaryDetails.mac_addr;
  ebPrim.role = mPrimaryDetails.role;
  ebPrim.eb_ap_bssid = apBssid;

  ebParams.push_back(ebPrim);

  /* Secondary Connected as well */
  if (ssm && ssm->GetState() == XPAN_AP_ACTIVE) {
    /* Add secondary details as well */
    RemoteEbParams ebSec;
    ebSec.eb_audio_loc = mSecondaryDetails.audio_location;
    ebSec.eb_ip_addr =  XpanApplicationController::ChangeIpEndianness(mSecondaryDetails.ip);
    ebSec.eb_mac_addr = mSecondaryDetails.mac_addr;
    ebSec.role = mSecondaryDetails.role;
    ebSec.eb_ap_bssid = apBssid;
    ebParams.push_back(ebSec);
  }

  xm->IpDetailsFetched(msg->reqRemoteApParams.usage, msg->reqRemoteApParams.context,
                      is_dtls_enabled, (is_dtls_enabled ? pPsk: NULL),
                      XpanApplicationController::GetLocalMdnsUuid().b,
                      local_ap_bssid, local_mac, ip_addr,
                      XpanApplicationController::GetLocalApFrequency(),
                      remote_udp_tsf, local_udp_tsf, remote_udp_port,
                      local_udp_data , ebParams);

}

void XpanDevice::InitiateLmpBearerSwitch (xac_handler_msg_t* msg) {
  TransportType bearer = msg->initiateLmpBearerSwitch.bearer;

  ALOGD("%s: device address = %s, bearer = %d", __func__
        , ConvertRawBdaddress(addr), bearer);

  if (psm) {
    psm->XpanAcSmExecuteEvent(msg);
  }
}

void XpanDevice::RemoteDisconnectedEvent (xac_handler_msg_t* msg) {
  if (msg->remoteDisconnectedEvent.role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
    delete psm;
    psm = NULL;
  } else if (msg->remoteDisconnectedEvent.role == ROLE_SECONDARY && ssm &&
             ssm->GetState() != XPAN_IDLE) {
    ssm->XpanAcSmExecuteEvent(msg);
    delete ssm;
    ssm = NULL;
  }
}

void XpanDevice::BearerSwitchInd (xac_handler_msg_t* msg) {
  if (psm) {
    psm->XpanAcSmExecuteEvent(msg);
  }

  /*if (ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }*/
}

void XpanDevice::MdnsDiscoveryStatus (xac_handler_msg_t* msg) {
  mdns_uuid_t EMPTY_UUID = {};

  MDNS_OP state= msg->mdnsDiscoveryStatus.state;

  if (msg->mdnsDiscoveryStatus.uuid == EMPTY_UUID) {
    ALOGW("%s: Invalid UUID received = %s", __func__,
          msg->mdnsDiscoveryStatus.uuid.toString().c_str());
  } else {
    mDnsUuid = XpanApplicationController::SetUuidOrder(msg->mdnsDiscoveryStatus.uuid);
    ALOGD("%s: State: %d status: %d. Setting device UUID to %s",
          __func__, state, msg->mdnsDiscoveryStatus.status, mDnsUuid.toString().c_str());
    if (state == MDNS_QUERY_START || state == MDNS_SERVICE_REGISTERED) {
      ComputePskFromLtk();
    }
  }

  if (psm) {
    psm->XpanAcSmExecuteEvent(msg);
  }
}

/* Handle earbud connected/disconnected event to send remote ap details to CP */
void XpanDevice::UpdateEarbudConnectionStatusToCp(XpanEarbudRole role,
                                                           XpanEbConnState state) {
  ALOGD("%s: Device = %s, role =  %d, state = %d", __func__,
        ConvertRawBdaddress(addr), role, state);

  DisplayRemoteApDetails(ROLE_PRIMARY);
  DisplayRemoteApDetails(ROLE_SECONDARY);

  macaddr_t local_mac = XpanApplicationController::GetLocalMacAddr();
  macaddr_t local_ap_bssid = XpanApplicationController::GetLocalApBssid();
  ipaddr_t ip_addr = XpanApplicationController::ChangeIpEndianness(
                        XpanApplicationController::GetLocalIpAddr());
  uint32_t remote_tcp_port = mPrimaryDetails.tcp_port;
  uint32_t remote_udp_port = mPrimaryDetails.udp_data_port;
  uint32_t remote_udp_tsf = XpanSocketHandler::GetUdpTsfPort();
  uint32_t local_udp_tsf = XpanSocketHandler::GetUdpTsfPort();
  uint32_t local_udp_data = XpanSocketHandler::GetUdpPort();
  bool is_dtls_enabled = XpanApplicationController::IsDTLSEnabled();

  std::vector<RemoteEbParams> ebParams;

  if (state == XPAN_EB_CONNECTED) {
    if (role == ROLE_SECONDARY && psm && !psm->IsRemoteApDetailsOkToSend()) {
      ALOGW("%s: Primary is not ready for transition. Wait for primary status.", __func__);
      return;
    }
    /* Add primary details as primary details are always there */
    RemoteEbParams ebPrim;
    ebPrim.eb_audio_loc = mPrimaryDetails.audio_location;
    ebPrim.eb_ip_addr =  XpanApplicationController::ChangeIpEndianness(mPrimaryDetails.ip);
    ebPrim.eb_mac_addr = mPrimaryDetails.mac_addr;
    ebPrim.role = mPrimaryDetails.role;
    ebPrim.eb_ap_bssid = apBssid;

    /* ******** Early streaming before readin properties *********************
       temporary workaround for the case when remote ap details are not known */
    /* TODO: Get all details like mac address, audio location, udp audio &tsf ports from
             MDNS */
    if (mPrimaryDetails.mac_addr.isEmpty()) {
      macaddr_t temp = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      ebPrim.eb_mac_addr = temp;
      ebPrim.eb_audio_loc = FRONT_LEFT;
      remote_udp_port = 48883; /* Fix port */
    }

    ebParams.push_back(ebPrim);

    /* Secondary Connected as well */
    if (role == ROLE_SECONDARY ||
        (ssm && mSecondaryDetails.detailsSet && ssm->IsRemoteApDetailsOkToSend())) {
      /* Add secondary details as well */
      RemoteEbParams ebSec;
      ebSec.eb_audio_loc = mSecondaryDetails.audio_location;
      ebSec.eb_ip_addr =  XpanApplicationController::ChangeIpEndianness(mSecondaryDetails.ip);
      ebSec.eb_mac_addr = mSecondaryDetails.mac_addr;
      ebSec.role = mSecondaryDetails.role;
      ebSec.eb_ap_bssid = apBssid;
      ebParams.push_back(ebSec);
    }

    xm->UpdateRemoteEbDetails(is_dtls_enabled, (is_dtls_enabled ? pPsk: NULL),
                              XpanApplicationController::GetLocalMdnsUuid().b,
                              local_ap_bssid, local_mac, ip_addr,
                              XpanApplicationController::GetLocalApFrequency(),
                              remote_udp_tsf, local_udp_tsf, remote_udp_port,
                              local_udp_data , ebParams);

  } else if (state == XPAN_EB_DISCONNECTED) {

    if (role == ROLE_SECONDARY && psm->GetState() == XPAN_AP_ACTIVE) {
      ALOGD("%s: Secondary disconnected. Primary State = %s", __func__,
            xpan_state_str(psm->GetState()));
      // Send only Primary Details
      RemoteEbParams ebPrim;
      ebPrim.eb_audio_loc = mPrimaryDetails.audio_location;
      ebPrim.eb_ip_addr = XpanApplicationController::ChangeIpEndianness(mPrimaryDetails.ip);
      ebPrim.eb_mac_addr = mPrimaryDetails.mac_addr;
      ebPrim.role = mPrimaryDetails.role;
      ebPrim.eb_ap_bssid = apBssid;
      ebParams.push_back(ebPrim);

      xm->UpdateRemoteEbDetails(is_dtls_enabled, (is_dtls_enabled ? pPsk: NULL),
                          XpanApplicationController::GetLocalMdnsUuid().b,
                          local_ap_bssid, local_mac, ip_addr,
                          XpanApplicationController::GetLocalApFrequency(),
                          remote_udp_tsf, local_udp_tsf, remote_udp_port,
                          local_udp_data , ebParams);
    } else if (role == ROLE_PRIMARY) {
      xm->UpdateRemoteEbDetails(is_dtls_enabled, (is_dtls_enabled ? pPsk: NULL),
                          XpanApplicationController::GetLocalMdnsUuid().b,
                          local_ap_bssid, local_mac, ip_addr,
                          XpanApplicationController::GetLocalApFrequency(),
                          remote_udp_tsf, local_udp_tsf, remote_udp_port,
                          local_udp_data , ebParams);
    }
  }
}

/* This event is received when one or both of the earbuds is disconnected.
 * The event should be given to CP if streaming is ongoing */
void XpanDevice::HandleEarbudDisconnectedEvt(XpanEarbudRole role) {
  if (role == ROLE_PRIMARY) {
    // Send Remote AP params indication with both disconnected
    if (qhci->isStreamingActive(addr)) {
      UpdateEarbudConnectionStatusToCp(role, XPAN_EB_DISCONNECTED);
    }

  } else if (role == ROLE_SECONDARY) {
    // If Primary is still connected, update secondary disconnected
    if (psm && (psm->GetState() == XPAN_AP_ACTIVE ||
                psm->GetState() == XPAN_BEARER_SWITCH_PENDING ||
                psm->GetState() == XPAN_LMP_CONNECTED)) {
      ALOGD("%s: Secondary disconnected. Update status to XM if streaming", __func__);
      // Send Remote AP params indication with sec disconnected
      if (qhci->isStreamingActive(addr)) {
        UpdateEarbudConnectionStatusToCp(role, XPAN_EB_DISCONNECTED);
      }
    }
  }
}

/*
 * This API will send update remote params to connectivity proxy if BSSID changed.
 * If other details like IP addresses are added/removed, remote AP params are
 * updated post connection/disconnection.
 * Returns true if BSSID changed and IP details are not added/removed
 * Returns false if IP details are added/removed.
 */
bool XpanDevice::DirectRoamingUpdateToCp(xac_handler_msg_t* msg) {
  uint8_t num_of_devices = msg->remoteApParams.num_of_earbuds;
  apBssid = msg->remoteApParams.eb_bssid;

  if (num_of_devices == BOTH_CONNECTED &&
      mPrimaryDetails.detailsSet && psm && psm->GetState() == XPAN_AP_ACTIVE &&
      mSecondaryDetails.detailsSet && ssm && ssm->GetState() == XPAN_AP_ACTIVE) {
    ALOGD("%s: Roaming update with both EB's connected", __func__);
    UpdateEarbudConnectionStatusToCp(ROLE_SECONDARY, XPAN_EB_CONNECTED);
    return true;

  } else if (num_of_devices == PRIMARY_CONNECTED &&
             mPrimaryDetails.detailsSet && psm && psm->GetState() == XPAN_AP_ACTIVE &&
             !mSecondaryDetails.detailsSet) {
    ALOGD("%s: Roaming update with only primary EB connected", __func__);
    UpdateEarbudConnectionStatusToCp(ROLE_PRIMARY, XPAN_EB_CONNECTED);
    return true;
  }

  return false;
}

void XpanDevice::ScheduleExplicitConnection() {
  ALOGD("%s: device = %s", __func__, addr.toString().c_str());
  if (pReinitConnTimer) {
    ALOGW("%s: Connection already scheduled", __func__);
    return;
  }

  pReinitConnTimer = new XpanAcTimer("ReinitConnTimer", TriggerExplicitConnection, this);
  pReinitConnTimer->StartTimer(XPAN_AC_REINIT_CONNECTION_TIMER);
}

void XpanDevice::TriggerExplicitConnection(void *data) {
  XpanDevice *current_device = (XpanDevice *)data;
  ALOGD("%s: device = %s", __func__, current_device->addr.toString().c_str());

  XMXacIf *xmif = XMXacIf::GetIf();
  if (xmif) {
    xmif->InitiateBluetoothConnection(current_device->addr, 0); /* reason for future use */
  }

  if (current_device->pReinitConnTimer) {
    current_device->pReinitConnTimer->StopTimer();
    current_device->pReinitConnTimer = NULL;
  }
}

void XpanDevice::CancelExplicitConnection() {
  if (pReinitConnTimer) {
    ALOGD("%s: device = %s", __func__, addr.toString().c_str());
    pReinitConnTimer->StopTimer();
    pReinitConnTimer = NULL;
  }
}

void XpanDevice::UpdateBondState (xac_handler_msg_t* msg) {
  BondState state = msg->bondStateUpdate.state;

  if (state == BOND_NONE) {
    XmRemoteDisconnectedEvent p_eb_disconn{XPAN_REMOTE_DISCONNECTED_EVT, addr, ROLE_PRIMARY};
    RemoteDisconnectedEvent((xac_handler_msg_t *)&p_eb_disconn);

    XmRemoteDisconnectedEvent s_eb_disconn{XPAN_REMOTE_DISCONNECTED_EVT, addr, ROLE_SECONDARY};
    RemoteDisconnectedEvent((xac_handler_msg_t *)&s_eb_disconn);
  }
}

/* The feature mask for the XPAN use cases as per local and remote support */
void XpanDevice::SetXpanSupportedFeature(xac_handler_msg_t* msg) {
  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Invalid xac instance", __func__);
    return;
  }

  mFeatureMask = msg->setFeatureMask.feature_mask;
  xac->AddDevSecRecord(mDnsUuid, addr, pPsk, mLtk, mFeatureMask);
}

/* QHCI Calls */
void XpanDevice::CreateConnection (xac_handler_msg_t* msg) {
  ALOGD("%s is_ta = %s",__func__, (msg->createConn.is_bg?"true":"false"));

  mTrigger = QHCI_CONNECTION;

  // Set supervision and ping timeout
  mSupervisionTimeout = msg->createConn.supervision_timeout;
  mPingTimeout = mSupervisionTimeout * 0.75;

  if (!msg->createConn.is_bg) {
    CancelExplicitConnection();
  }

  if (!psm) {
    ALOGD("%s: Creating primary earbud state machine.", __func__);
    psm = new XpanAcStateMachine(this, msg->createConn.addr, ROLE_PRIMARY
                                 , NONE/*current transport*/);
  }

  psm->XpanAcSmExecuteEvent(msg);

}

void XpanDevice::CreateConnectionCancel (xac_handler_msg_t* msg) {
  if (!psm) {
    qhci->ConnectionCompleteRes(addr, UNKNOWN_CONNECTION_IDENTIFIER);
    return;
  }

  psm->XpanAcSmExecuteEvent(msg);

  // Clear Primary State Machine
  if (psm) {
    delete psm;
    psm = NULL;
  }
}

void XpanDevice::DisconnectConnection (xac_handler_msg_t* msg) {
  if (!psm) {
    ALOGW("%s: Already disconnected", __func__);
    qhci->DisconnectionCompleteRes(addr, XPAN_AC_SUCCESS);
    return;
  }
  psm->XpanAcSmExecuteEvent(msg);

  if (ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }

  // Clear Primary State Machine
  if (psm) {
    delete psm;
    psm = NULL;
  }

  // Clear Secondary State Machine
  if (ssm) {
    delete ssm;
    ssm = NULL;
  }
}

void XpanDevice::HandleLmpConnectionRes (xac_handler_msg_t* msg) {
  /* LMP Connection response can be received for either primary or secondary */
  if (msg && msg->lmpConnectRsp.role == ROLE_PRIMARY) {
    if (!psm) {
      qhci->ConnectionCompleteRes(addr, XPAN_CONNECTION_FAILED_TO_ESTABLISH);
      return;
    }
    psm->XpanAcSmExecuteEvent(msg);
    if (msg->lmpConnectRsp.status != XPAN_AC_SUCCESS) {
      delete psm;
      psm = NULL;
    }
  } else if (msg && msg->lmpConnectRsp.role == ROLE_SECONDARY) {
    if (ssm) {
      ssm->XpanAcSmExecuteEvent(msg);
      if (msg->lmpConnectRsp.status != XPAN_AC_SUCCESS) {
        delete ssm;
        ssm = NULL;
      }

    }
  }
}

void XpanDevice::HandleLmpConnectionReq(xac_handler_msg_t* msg) {
  /* LMP Connection request should always be received from acting primary*/
  if (msg) {
    if (!psm) {
      ALOGE("%s: State machine for primary not initialized", __func__);
      return;
    }
    // Set Primary IP details
    mPrimaryDetails.ip = msg->incLmpConnReq.ip;
    mPrimaryDetails.role = ROLE_PRIMARY;
    psm->XpanAcSmExecuteEvent(msg);
  }
}

void XpanDevice::HandleLmpPingRequestEvt(xac_handler_msg_t* msg) {
  XpanEarbudRole role = GetRoleByIpAddr(msg->pingReq.ip);
  if (role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
  } else if(role == ROLE_SECONDARY && ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }
}

void XpanDevice::GetRemoteVersion (xac_handler_msg_t* msg) {
  if (!psm) {
    ALOGE("%s: Primary EB not connected", __func__);
    return;
  }
  psm->XpanAcSmExecuteEvent(msg);
}

void XpanDevice::GetRemoteLeFeatures (xac_handler_msg_t* msg) {
  if (!psm) {
    ALOGE("%s: Primary EB not connected", __func__);
    return;
  }
  psm->XpanAcSmExecuteEvent(msg);
}

bool XpanDevice::IsLeToApUrgencySet() {
  return mIsLeToApUrgencySet;
}

bool XpanDevice::IsLtkSet() {
  bool ltkIsSet = false;
  for (int j = 0; j < KEY_LEN; j++) {
    if (mLtk[j] != 0) {
      ltkIsSet = true;
      break;
    }
  }

  return ltkIsSet;
}

bool XpanDevice::IsNewLtk(uint8_t *cur, uint8_t *upd) {
  for (int j = 0; j < KEY_LEN; j++) {
    if (cur[j] != upd[j]) {
      return true;
    }
  }
  return false;
}

bool XpanDevice::IsPskComputed() {
  for (int i = 0; i < KEY_LEN; i++) {
    if (pPsk[i] != 0) {
      ALOGD("%s: Already computed", __func__);
      return true;
    }
  }
  return false;
}

bool XpanDevice::IsRemoteIpDetailsAvailable(TransportType bearer) {
  /* discard prepare bearer request if remote AP details are not received */
  if (bearer == XPAN_AP && (!mPrimaryDetails.detailsSet ||
    XpanApplicationController::GetLocalIpAddr().isEmpty())) {
    ALOGE("%s: Remote/Local AP details not present", __func__);
    return false;
  }
  return true;
}

bool XpanDevice::IsPskRecomputationNeeded() {
  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Invalid xac instance", __func__);
    return false;
  }

  uint8_t *cached_ltk = xac->GetLtkFromSecRecord(addr);
  if (!cached_ltk || (cached_ltk != NULL && IsNewLtk(mLtk, cached_ltk))) {
    ALOGI("%s: LTK updated", __func__);
    XpanApplicationController::PrintKey(mLtk, "LTK");
    return true;
  }

  mdns_uuid_t EMPTY_UUID = {};
  mdns_uuid_t cached_mdns_uuid = xac->GetMdnsUuidFromSecRecord(addr);
  if (mDnsUuid == EMPTY_UUID || !(mDnsUuid == cached_mdns_uuid)) {
    ALOGI("%s: UUID updated %s", __func__, mDnsUuid.toString().c_str());
    return true;
  }

  return false;
}

void XpanDevice::ComputePskFromLtk() {
  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Invalid xac instance", __func__);
    return;
  }

  if (!XpanApplicationController::IsTLSEnabled() &&
      !XpanApplicationController::IsDTLSEnabled()) {
    ALOGE("%s: TLS & DTLS is not enabled", __func__);
    return;
  }

  mdns_uuid_t EMPTY_UUID = {};

  /* Note - Allow computing PSK for the first time and let it get stored with EMPTY UUID
            so that LTK gets stored in DevSecRecord and can be fetched on reboot if there
            is no transition or reconnection before reboot after first pairing session.

  if (mDnsUuid == EMPTY_UUID) {
    ALOGE("%s: Delay computing PSK from LTK until remote MDNS UUID is available", __func__);
    return;
  }*/

  if (!IsPskRecomputationNeeded()) {
    ALOGI("%s: PSK already computed", __func__);
    XpanApplicationController::PrintKey(pPsk, "PSK");
    return;
  }

  ALOGD("%s: Computing PSK", __func__);

  bool res = XpanTLSHandler::generate_psk(mLtk, mDnsUuid, pPsk);
  if (!res) {
    ALOGE("%s: Invalid msg", __func__);
    return;
  }

  XpanApplicationController::PrintKey(pPsk, "PSK");

  xac->AddDevSecRecord(mDnsUuid, addr, pPsk, mLtk, mFeatureMask);
}

void XpanDevice::EnableEncrption (xac_handler_msg_t* msg) {
  std::shared_ptr<XpanQhciAcIf> qhci = XpanQhciAcIf::GetIf();
  //if (qhci) {
  //  qhci->EncryptionChangedRes(addr, 0, 1);
  //}

  if (!msg) {
    ALOGE("%s: Invalid msg", __func__);
    return;
  }

  if (!XpanApplicationController::IsTLSEnabled() &&
      !XpanApplicationController::IsDTLSEnabled()) {
    ALOGE("%s: TLS & DTLS is not enabled", __func__);
    return;
  }

  if (!IsLtkSet()|| IsNewLtk(mLtk, msg->encryptionParams.ltk)) {
    memcpy(&mLtk, msg->encryptionParams.ltk, KEY_LEN);
    XpanApplicationController::PrintKey(mLtk, "LTK");
    ComputePskFromLtk();
    return;
  }

  ALOGD("%s: PSK already computed", __func__);
}

void XpanDevice::SendAclData (xac_handler_msg_t* msg) {
  if (!psm) {
    ALOGE("%s: Primary EB not connected", __func__);
    return;
  }
  psm->XpanAcSmExecuteEvent(msg);
}

void XpanDevice::L2capPauseUnpauseRes (xac_handler_msg_t* msg) {
  if (!psm) {
    ALOGE("%s: Primary EB not connected. Ignore.", __func__);
    return;
  }
  psm->XpanAcSmExecuteEvent(msg);
}

void XpanDevice::HandleRoleSwitchRequest(xac_handler_msg_t* msg) {
  uint8_t operation = msg->primarySwitchReq.operation;
  std::shared_ptr<XpanQhciAcIf> qhci = XpanQhciAcIf::GetIf();

  ALOGD("%s: operation = %d", __func__, operation);

  /* Operation = Start */
  if (operation == PRIMARY_SWITCH_START) {
    /* Steps :
        1. Mark current operation to role switch procedure
        2. Trigger Pause QHCI and wait for cb
        3. On Cb, Send Lmp accepted.
     */
    if (!psm) {
      ALOGW("%s PEB is not in connected state", __func__);
      return;
    }
    if (!qhci || !ssm || ssm->GetState() != XPAN_AP_ACTIVE) {
      // Send Not Accepted
      ALOGE("%s: Secondary earbud is not in AP Active State. reject request", __func__);
      msg->primarySwitchReq.operation = PRIMARY_SWITCH_CANCEL;
      msg->primarySwitchReq.reject = true;
      psm->XpanAcSmExecuteEvent(msg);
      return;
    }

    psm->XpanAcSmExecuteEvent(msg);
    qhci->L2capPauseUnpauseReq(addr, PAUSE, XPAN_AP);

  /* Operation = Complete */
  } else if (operation == PRIMARY_SWITCH_COMPLETE) {
    // swap role and send acceptance
    XpanAcStateMachine *tmp = psm;
    psm = ssm;
    ssm = tmp;

    if (!psm) {
      ALOGW("%s new PEB is not in connected state", __func__);
      return;
    }
    // Set new role
    psm->UpdateEarbudRole(ROLE_PRIMARY);
    if (ssm) {
      ssm->UpdateEarbudRole(ROLE_SECONDARY);
    }

    // swap earbud properties
    RemoteDetails temp = mPrimaryDetails;
    mPrimaryDetails = mSecondaryDetails;
    mSecondaryDetails = temp;

    mPrimaryDetails.role = ROLE_PRIMARY;
    mSecondaryDetails.role = ROLE_SECONDARY;

    DisplayRemoteApDetails(ROLE_PRIMARY);
    DisplayRemoteApDetails(ROLE_SECONDARY);

    psm->XpanAcSmExecuteEvent(msg);
    qhci->L2capPauseUnpauseReq(addr, UNPAUSE, XPAN_AP);
  } else if (operation == PRIMARY_SWITCH_CANCEL) {
    if (!psm) {
      ALOGW("%s PEB is not in connected state", __func__);
      return;
    }
    psm->XpanAcSmExecuteEvent(msg);
    qhci->L2capPauseUnpauseReq(addr, UNPAUSE, XPAN_AP);
  }
}


void XpanDevice::DisplayRemoteApDetails(XpanEarbudRole role) {
  if (role == ROLE_PRIMARY) {
    ALOGD("%s: Role = %d, ip=%s, mac=%s, location=%d", __func__,
          mPrimaryDetails.role, mPrimaryDetails.ip.toString().c_str(),
          mPrimaryDetails.mac_addr.toString().c_str(),
          mPrimaryDetails.audio_location);
  } else if (role == ROLE_SECONDARY) {
    ALOGD("%s: Role = %d, ip=%s, mac=%s, location=%d", __func__,
          mSecondaryDetails.role, mSecondaryDetails.ip.toString().c_str(),
          mSecondaryDetails.mac_addr.toString().c_str(),
          mSecondaryDetails.audio_location);
  }
}

void XpanDevice::HandleTcpConnectedEvent(xac_handler_msg_t* msg) {
  ALOGD("%s", __func__);
  if (!msg) {
    ALOGE("%s: Invalid Event", __func__);
    return;
  }

  XpanTcpConnectedEvt *evt = (XpanTcpConnectedEvt *)msg;
  if (evt->role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
  } else if (evt->role == ROLE_SECONDARY && ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }
}

void XpanDevice::HandleTcpFailedEvent(xac_handler_msg_t* msg) {
  ALOGD("%s", __func__);
  if (!msg) {
    ALOGE("%s: Invalid Event", __func__);
    return;
  }

  XpanTcpConnectionFailedEvt *evt = (XpanTcpConnectionFailedEvt *)msg;
  if (evt->role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
    /*
    If outgoing connection fails and XAC is waiting for incoming
    connection, don't delete state machine
    */
    if(!(evt->isOutgoing && XpanSocketHandler::IsListeningOnTcp())){
      delete psm;
      psm = NULL;
    }
  } else if (evt->role == ROLE_SECONDARY && ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
    delete ssm;
    ssm = NULL;
  }
}

void XpanDevice::HandleWifiApDisconnectedEvent(xac_handler_msg_t* msg) {
  ALOGD("%s", __func__);
  if (!msg) {
    ALOGE("%s: Invalid Event", __func__);
    return;
  }

  if (psm) {
    psm->XpanAcSmExecuteEvent(msg);
  }

  if (ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }

  // Clear Primary State Machine
  if (psm) {
    delete psm;
    psm = NULL;
  }

  // Clear Secondary State Machine
  if (ssm) {
    delete ssm;
    ssm = NULL;
  }
}


void XpanDevice::HandleTcpDisconnectedEvent(xac_handler_msg_t* msg) {
  ALOGD("%s", __func__);
  if (!msg) {
    ALOGE("%s: Invalid Event", __func__);
    return;
  }

  XpanTcpDisconnectionEvt *evt = (XpanTcpDisconnectionEvt *)msg;
  if (evt->role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
    delete psm;
    psm = NULL;
  } else if (evt->role == ROLE_SECONDARY && ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
    delete ssm;
    ssm = NULL;
  }
}

void XpanDevice::HandleAclDataEvt(xac_handler_msg_t* msg) {
  XpanEarbudRole role = GetRoleByIpAddr(msg->data.ip);
  if (role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
  } else if(role == ROLE_SECONDARY && ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }
}

void XpanDevice::HandleLmpLeFeatureReq(xac_handler_msg_t* msg) {
  XpanEarbudRole role =  this->GetRoleByIpAddr(msg->featReq.ip);
  if (role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
  } else if (role == ROLE_SECONDARY && ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }
}

void XpanDevice::HandleLmpLeFeatureRes(xac_handler_msg_t* msg) {
  XpanEarbudRole role =  this->GetRoleByIpAddr(msg->featRsp.ip);
  if (role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
  } else if (role == ROLE_SECONDARY && ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }
}

void XpanDevice::HandleLmpVersionReq(xac_handler_msg_t* msg) {
  XpanEarbudRole role =  this->GetRoleByIpAddr(msg->versionReq.ip);
  if (role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
  } else if (role == ROLE_SECONDARY && ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }
}

void XpanDevice::HandleLmpVersionRes(xac_handler_msg_t* msg) {
  XpanEarbudRole role =  this->GetRoleByIpAddr(msg->versionRsp.ip);
  if (role == ROLE_PRIMARY && psm) {
    psm->XpanAcSmExecuteEvent(msg);
  } else if (role == ROLE_SECONDARY && ssm) {
    ssm->XpanAcSmExecuteEvent(msg);
  }
}

  void XpanDevice::HandleLmpPrepareBearerRes(xac_handler_msg_t* msg) {
    XpanEarbudRole role =  this->GetRoleByIpAddr(msg->prepareBearerRsp.ip);
    if (role == ROLE_PRIMARY && psm) {
      psm->XpanAcSmExecuteEvent(msg);
    } else if (role == ROLE_SECONDARY && ssm) {
      ssm->XpanAcSmExecuteEvent(msg);
    }
  }

  void XpanDevice::HandleLmpL2capPauseUnpauseRes(xac_handler_msg_t* msg) {
    if (!psm) {
      ALOGE("%s: PSM not initialized", __func__);
      return;
    }
    psm->XpanAcSmExecuteEvent(msg);
  }

  void XpanDevice::HandleLmpBearerSwitchRes(xac_handler_msg_t* msg) {
    if (!psm) {
      ALOGE("%s: PSM not initialized", __func__);
      return;
    }
    psm->XpanAcSmExecuteEvent(msg);
  }

  void XpanDevice::HandleBearerPreferenceTimeout(xac_handler_msg_t* msg) {
    XpanEarbudRole role = (XpanEarbudRole)msg->bearerPrefTimeout.role;
    if (role == ROLE_PRIMARY && psm) {
      psm->XpanAcSmExecuteEvent(msg);
      // Clear Primary State Machine
      if (psm) {
        delete psm;
        psm = NULL;
      }
    } else if (role == ROLE_SECONDARY && ssm) {
      ssm->XpanAcSmExecuteEvent(msg);
      // Clear Primary State Machine
      if (ssm) {
        delete ssm;
        ssm = NULL;
      }
    }
  }

  void XpanDevice::HandlePrimaryDisconnecting(xac_handler_msg_t* msg) {
    if (ssm) {
      ssm->XpanAcSmExecuteEvent(msg);
      delete ssm;
      ssm = NULL;
    }
  }

  void XpanDevice::HandleTargetedAdvConnTimeout(xac_handler_msg_t* msg) {
    XpanEarbudRole role = (XpanEarbudRole)msg->targetedConnTo.role;
    if (role == ROLE_PRIMARY && psm) {
      psm->XpanAcSmExecuteEvent(msg);
      // Clear Primary State Machine
      if (psm) {
        delete psm;
        psm = NULL;
      }
    }
  }

  void XpanDevice::HandleLeLinkStatusEvt(xac_handler_msg_t* msg) {
    if (!psm) {
      ALOGE("%s: PSM state machine not present", __func__);
      return;
    }

    XacSmState sm_state = psm->GetState();
    XpanTransitionType transition = psm->GetCurrentTransition();
    uint8_t conn_state = msg->leLinkStatus.state;
    TransportType qhci_active_transport = msg->leLinkStatus.qhci_active_transport;

    psm->XpanAcSmExecuteEvent(msg);

    if (conn_state == 0 && (
        ((sm_state == XPAN_TCP_CONNECTING || sm_state == XPAN_LMP_CONNECTING ||
          sm_state == XPAN_BEARER_SWITCH_PENDING) &&
         (transition == XPAN_LE_TO_AP_IDLE ||
          transition == XPAN_LE_TO_AP_STREAMING)) ||
        (sm_state == XPAN_AP_ACTIVE && qhci_active_transport == BT_LE))) {
      delete psm;
      psm = NULL;
    }


  }

  void XpanDevice::HandleStreamStatusUpdate(xac_handler_msg_t* msg) {
    if (psm) {
      psm->XpanAcSmExecuteEvent(msg);
    }
  }

  void XpanDevice::HandleResetSupervision(xac_handler_msg_t* msg) {
    XpanEarbudRole role = GetRoleByIpAddr(msg->resetSupervision.ip);

    if (role == ROLE_PRIMARY && psm) {
      psm->XpanAcSmExecuteEvent(msg);
    } else if (role == ROLE_SECONDARY && ssm) {
      ssm->XpanAcSmExecuteEvent(msg);
    }
  }

} // namespace ac
} // namespace xpan

