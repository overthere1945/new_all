/*
 * Copyright (c) 2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <log/log.h>
#include "XpanAcXmIf.h"
#include "XpanAcQhciIf.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.bluetooth.xpan_ac.XpanAcXmIf"

namespace xpan {
namespace ac {

XpanAcXmIf* XpanAcXmIf::sInstance = NULL;

XpanAcXmIf::XpanAcXmIf() {
  ALOGD("%s: Instantiated.", __func__);

}

XpanAcXmIf::~XpanAcXmIf() {
  ALOGD("%s: Deintialized.", __func__);

}

XpanAcXmIf* XpanAcXmIf::GetIf() {
  if (!sInstance) {
    sInstance = new XpanAcXmIf();
  }
  return sInstance;
}

bool XpanAcXmIf::Initialize() {
  return XpanApplicationController::Initialize();
}

bool XpanAcXmIf::Deinitialize() {
  if (sInstance) {
    delete sInstance;
    sInstance = NULL;
    ALOGD("%s: XpanAcXmIf cleared", __func__);
  }

  XpanAcQhciIf::Deinitialize();

  return XpanApplicationController::Deinitialize();
}

bool XpanAcXmIf::PrepareBearer (bdaddr_t addr, TransportType bearer, bool urgency) {
  ALOGD("%s addr = %s bearer = %d urgency = %d", __func__, addr.toString().c_str(),
        bearer, urgency);

  XmPrepareBearer *params = (XmPrepareBearer *) malloc (sizeof(XmPrepareBearer));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_PREPARE_BEARER_EVT;
  params->addr = addr;
  params->bearer = bearer;
  params->urgency = urgency;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::CheckIpDetailsAvailibility (bdaddr_t addr, TransportType bearer, bool is_urgent) {
  ALOGD("%s addr = %s bearer = %d urgency = %d", __func__, addr.toString().c_str(),
        bearer, is_urgent);

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return false;
  }
  if (bearer == XPAN_AP) {
    XpanDevice* device = xac->GetDevice(addr);
    if (device && !device->IsRemoteIpDetailsAvailable(bearer)) {
      ALOGE("%s: Remote/Local AP details not present", __func__);
      return false;
    }
  }

  return true;
}

bool XpanAcXmIf::BearerPreference (bdaddr_t addr, TransportType bearer, bool is_urgent) {
  ALOGD("%s addr = %s bearer = %d urgency = %d", __func__, addr.toString().c_str(),
        bearer, is_urgent);

  std::shared_ptr<XpanQhciAcIf> qhci;
  qhci = XpanQhciAcIf::GetIf();

  if (bearer == BT_LE && qhci && !qhci->isLeLinkconnected(addr)) {
    ALOGE("%s: LE link not present", __func__);
    return false;
  }

  XmBearerPreference *params =
      (XmBearerPreference *) malloc (sizeof(XmBearerPreference));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_BEARER_PREFERENCE_EVT;
  params->addr = addr;
  params->bearer = bearer;
  params->is_urgent = is_urgent;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::RequestRemoteApParams (bdaddr_t addr, uint32_t usage, void *context) {
  ALOGD("%s: addr=%s, usage=%d", __func__, addr.toString().c_str(), usage);
  XmReqRemoteApParams *params =
      (XmReqRemoteApParams *) malloc (sizeof(XmReqRemoteApParams));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }

  params->event = XM_REQUEST_REMOTE_AP_PARAMS_EVT;
  params->addr = addr;
  params->usage = usage;
  params->context = context;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }
  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::UpdateRemoteApParams (tXPAN_Remote_Params ap_details) {
  ALOGD("%s", __func__);

  XmUpdateRemoteApParams *params =
      (XmUpdateRemoteApParams *) malloc (sizeof(XmUpdateRemoteApParams));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_UPDATE_REMOTE_AP_PARAMS_EVT;
  params->addr = ap_details.addr;
  params->is_mdns_update = ap_details.mdns;
  params->mdns_uuid = ap_details.mdnsUuid;
  params->remote_tcp_port = ap_details.portL2capTcp;
  params->remote_udp_port = ap_details.portUdpAudio;
  params->remote_udp_tsf_port = ap_details.portUdpReports;
  params->num_of_earbuds = ap_details.numOfEbs;

  if ((params->num_of_earbuds == 1 &&
         ap_details.vectorEbParams[0].role != ROLE_PRIMARY) ||
      (params->num_of_earbuds == 2 &&
         ap_details.vectorEbParams[0].role != ROLE_PRIMARY &&
         ap_details.vectorEbParams[1].role != ROLE_PRIMARY)) {
    ALOGE("%s: Remote Params Indication without Primary Role. Discard", __func__);
    free(params);
    return false;
  }

  for (int i = 0; i < params->num_of_earbuds; i++) {
    int role = ap_details.vectorEbParams[i].role;

    if (role != ROLE_PRIMARY && role != ROLE_SECONDARY) {
      free(params);
      ALOGE("%s: Invalid role. Discarding remote params indication", __func__);
      return false;
    }

    /* Sort the roles before processing in XpanDevice */
    params->role[role] = (XpanEarbudRole)ap_details.vectorEbParams[i].role;
    params->ip[role] = ap_details.vectorEbParams[i].ipAddr;
    params->mac[role] = ap_details.vectorEbParams[i].mac_addr;
    params->audio_loc[role] = ap_details.vectorEbParams[i].audioLocation;
    params->eb_bssid = ap_details.vectorEbParams[i].mac_bssid;
  }

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::InitiateLmpBearerSwitch (bdaddr_t addr, TransportType bearer) {
  ALOGD("%s", __func__);

  XmInitiateLmpBearerSwitch *params =
      (XmInitiateLmpBearerSwitch *) malloc (sizeof(XmInitiateLmpBearerSwitch));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_INITIATE_LMP_BEARER_SWITCH_EVT;
  params->addr = addr;
  params->bearer = bearer;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::RemoteDisconnectedEvent (bdaddr_t addr, XpanEarbudRole role) {
  ALOGD("%s", __func__);

  XmRemoteDisconnectedEvent *params =
      (XmRemoteDisconnectedEvent *) malloc (sizeof(XmRemoteDisconnectedEvent));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_BEARER_SWITCH_IND_EVT;
  params->addr = addr;
  params->role = role;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::BearerSwitchInd (bdaddr_t addr, TransportType bearer, uint8_t status,
    uint8_t reason) {
  ALOGD("%s", __func__);

  XmBearerSwitchInd *params =
      (XmBearerSwitchInd *) malloc (sizeof(XmBearerSwitchInd));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_BEARER_SWITCH_IND_EVT;
  params->addr = addr;
  params->bearer = bearer;
  params->status = status;
  params->reason = reason;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::MdnsDiscoveryStatus (bdaddr_t addr, uint8_t status,
    mdns_uuid_t uuid, uint8_t state) {
  ALOGD("%s", __func__);

  XmMdnsDiscoveryStatus *params =
      (XmMdnsDiscoveryStatus *) malloc (sizeof(XmMdnsDiscoveryStatus));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_MDNS_DISCOVERY_STATUS_EVT;
  params->addr = addr;
  params->status = status;
  params->uuid = uuid;
  params->state = (MDNS_OP)state;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::UpdateBondState (bdaddr_t addr, BondState state) {
  ALOGD("%s", __func__);

  XmUpdateBondState *params =
      (XmUpdateBondState *) malloc (sizeof(XmUpdateBondState));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_UPDATE_BOND_STATE_EVT;
  params->addr = addr;
  params->state = state;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::UpdateLocalApDetails (macaddr_t mac, macaddr_t bssid,
                                       ipaddr_t ipAddr, mdns_uuid_t uuid, uint32_t freq) {
  ALOGD("%s", __func__);

  XmUpdateLocalApDetails *params =
      (XmUpdateLocalApDetails *) malloc (sizeof(XmUpdateLocalApDetails));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_UPDATE_LOCAL_AP_DETAILS_EVT;
  params->local_mac_addr = mac;
  params->bssid = bssid;
  params->ipAddr = ipAddr;
  params->uuid = uuid;
  params->freq = freq;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::GetLocalListeningPorts () {
  ALOGD("%s", __func__);

  XmGetLocalListeningPorts *params =
      (XmGetLocalListeningPorts *) malloc (sizeof(XmGetLocalListeningPorts));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_GET_LOCAL_LISTENING_PORTS_EVT;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::UpdateBondedXpanDevices (uint8_t numOfDevices, bdaddr_t devices[]) {
  ALOGD("%s", __func__);

  XmUpdateBondedXpanDevices *params =
      (XmUpdateBondedXpanDevices *) malloc (sizeof(XmUpdateBondedXpanDevices));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_UPDATE_BONDED_XPAN_DEVICES_EVT;
  params->numOfDevices = numOfDevices;
  params->devices = (bdaddr_t *)malloc(sizeof(bdaddr_t) * numOfDevices);
  for (int i = 0; i < numOfDevices; i++) {
    memcpy(&params->devices[i], &devices[i], sizeof(bdaddr_t));
  }

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free (params);
    return false;
  }

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}

bool XpanAcXmIf::isTransportReadyForSwitch(bdaddr_t addr) {
  ALOGD("%s addr = %s", __func__, addr.toString().c_str());

   XpanApplicationController *xac = XpanApplicationController::Get();
   if (!xac) {
     ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
     return false;
   }

   XpanDevice* device = xac->GetDevice(addr);
   if (device) {
     XpanAcStateMachine *sm = device->GetStateMachine(ROLE_PRIMARY);
     if (sm &&
          (((sm->GetCurrentTransition() == XPAN_LE_TO_AP_IDLE ||
              sm->GetCurrentTransition() == XPAN_LE_TO_AP_STREAMING) &&
             (sm->GetState() == XPAN_TCP_CONNECTING ||
              sm->GetState() == XPAN_LMP_CONNECTING ||
              sm->GetState() == XPAN_BEARER_SWITCH_PENDING ||
              sm->GetState() == XPAN_DISCONNECTING)) ||
           ((sm->GetCurrentTransition() == XPAN_AP_TO_LE_IDLE ||
            sm->GetCurrentTransition() == XPAN_AP_TO_LE_STREAMING) &&
             (sm->GetState() == XPAN_BEARER_SWITCH_PENDING)))) {
        ALOGD("%s: transition(%d) already in progress. Current State(%s)", __func__,
              sm->GetCurrentTransition(), xpan_state_str(sm->GetState()));
        return false;
     }
   }

   return true;
}

/* Combined(local & remote) feature mask to be supported for a given XPAN device */
bool XpanAcXmIf::SetRemoteFeatureMask(bdaddr_t addr, long feature_mask) {
  ALOGD("%s addr = %s feature_mask = 0x%x", __func__, addr.toString().c_str(), feature_mask);

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return false;
  }

  XmSetFeatureMask *params =
        (XmSetFeatureMask *) malloc (sizeof(XmSetFeatureMask));
  if (params == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return false;
  }
  params->event = XM_SET_FEATURE_MASK_EVT;
  params->addr = addr;
  params->feature_mask = feature_mask;

  xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  return true;
}


} // namespace ac
} // namespace xpan
