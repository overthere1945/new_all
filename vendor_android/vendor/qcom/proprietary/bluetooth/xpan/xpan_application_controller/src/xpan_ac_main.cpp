/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc. 
 */

#include <log/log.h>
#include "xpan_ac_int.h"
#include <cutils/properties.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.bluetooth.xpan_ac.XpanApplicationController"

namespace xpan {
namespace ac {

XpanApplicationController*  XpanApplicationController::instance = NULL;
mdns_uuid_t XpanApplicationController::local_uuid;
std::thread XpanApplicationController::mXpanAcThread;
std::thread XpanApplicationController::mXpanRxThread;
std::mutex XpanApplicationController::mWqMutex;
std::deque <xac_handler_msg_t *> XpanApplicationController::mWorkQueue;
std::condition_variable XpanApplicationController::mWqNotifier;
std::atomic_bool XpanApplicationController::mIsMainThreadBusy;
std::atomic_bool XpanApplicationController::mIsMainThreadRunning;
uint64_t XpanApplicationController::le_features;
macaddr_t XpanApplicationController::localApBssid;
macaddr_t XpanApplicationController::localMacAddr;
ipaddr_t XpanApplicationController::localIpAddr;
uint32_t XpanApplicationController::center_freq;
bool XpanApplicationController::isXpanTlsEnabled;
bool XpanApplicationController::isXpanDtlsEnabled;
int XpanApplicationController::mUniqueId;

LocalVersionInfo XpanApplicationController::info;
uint8_t * XpanApplicationController::local_cod;
std::vector<QhciCreateConnection> XpanApplicationController::mStalledConnRequests;

XpanApplicationController::XpanApplicationController() {
  ALOGD("%s", __func__);

  localIpAddr = {};
  localApBssid = {};
  local_cod = (uint8_t *)malloc(3);
  if(local_cod) {
    local_cod[0] = 0xFF; /*TODO*/
    local_cod[1] = 0xFF;
    local_cod[2] = 0xFF;
  }

  devices.reserve(64);

  char xpan_prop_2[PROPERTY_VALUE_MAX];
  isXpanTlsEnabled = false;
  isXpanDtlsEnabled = false;
  property_get("persist.vendor.qcom.xpan.ac.enc.enabled", xpan_prop_2, "tls-dtls");
  if (!strcmp(xpan_prop_2, "tls")) {
    ALOGI("XpanAC TLS enabled");
    isXpanTlsEnabled = true;
  } else if (!strcmp(xpan_prop_2, "dtls")) {
    ALOGI("XpanAC DTLS enabled");
    isXpanDtlsEnabled = true;
  } else if (!strcmp(xpan_prop_2, "tls-dtls")) {
    ALOGI("XpanAC TLS & DTLS enabled");
    isXpanTlsEnabled = true;
    isXpanDtlsEnabled = true;
  } else {
    ALOGI("XpanAC TLS & DTLS not enabled");
  }
}

XpanApplicationController::~XpanApplicationController() {
  ALOGD("%s", __func__);

  free(local_cod);
  devices.clear();

  mStalledConnRequests.clear();
}

void XpanApplicationController::thread_task() {
  ALOGD("%s: XPAN Application Controller main thread started", __func__);

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (xac) {
    xac->ReadDevSecurityRecords();
  }
  std::shared_ptr<xpan::implementation::XpanQhciAcIf> qhci =
      xpan::implementation::XpanQhciAcIf::GetIf();
  if (qhci) {
    le_features = qhci->GetLocalLeFeatures();
    info.version = qhci->GetLocalLeVersion().version;
    info.companyId = qhci->GetLocalLeVersion().companyId;
    info.subversion = qhci->GetLocalLeVersion().subversion;
  }
  XpanApplicationController::XacMessageHandlerRoutine();
}

void XpanApplicationController::RxThreadTask() {
  ALOGD("%s: XPAN Application Controller Rx thread started", __func__);
  XpanSocketHandler::InitiateRxRoutine();
}

bool XpanApplicationController::Initialize() {
  if (instance) {
    ALOGD("%s: Application Controller already initialized", __func__);
    return true;
  }

  instance = new XpanApplicationController();

  // Xpan Ac Main thread for events from other modules and for Tx
  mXpanAcThread = std::thread(thread_task);

  // Xpan AC Rx thread
  mXpanRxThread = std::thread(RxThreadTask);

  if (!mXpanAcThread.joinable() || !mXpanRxThread.joinable()) return false;

  ALOGD("%s: Xpan AC threads initialized", __func__);
  return true;
}

bool XpanApplicationController::Deinitialize() {
  if (!std::atomic_exchange(&mIsMainThreadRunning, false)) {
    ALOGW("%s: main thread already stopped", __func__);
  }

  XpanSocketHandler::StopRxThreadRoutine();

  std::unique_lock<std::mutex> xac_lock(mWqMutex);
  while(!mWorkQueue.empty()) {
    xac_handler_msg_t *msg = mWorkQueue.front();
    mWorkQueue.pop_front();
  }
  xac_lock.unlock();

  /* notify inactive main thread to finish its execution */
  if (!mIsMainThreadBusy) {
    mWqNotifier.notify_all();
  }

  if (mXpanAcThread.joinable()) {
    //pthread_kill(mXpanAcThread.native_handle(), SIGUSR1);
    mXpanAcThread.join();
    ALOGI("%s: main thread cleaned", __func__);
  }

  if (mXpanRxThread.joinable()) {
    //pthread_kill(mXpanRxThread.native_handle(), SIGUSR1);
    mXpanRxThread.join();
    ALOGI("%s: Rx thread cleaned", __func__);
  }

  if (instance) {
    delete instance;
    instance = NULL;
  }

  ALOGD("%s: Completed", __func__);

  return true;
}

XpanApplicationController* XpanApplicationController::Get() {
  return instance;
}

void XpanApplicationController::XacMessageHandlerRoutine() {
  ALOGD("%s", __func__);

  if (std::atomic_exchange(&mIsMainThreadRunning, true)) {
    ALOGE("%s: Main Thread is already running..", __func__);
    return;
  }

  while (mIsMainThreadRunning) {
    std::atomic_exchange(&mIsMainThreadBusy, false);
    std::unique_lock<std::mutex> xac_lock(mWqMutex);

    if (mWorkQueue.empty()) {
      /* Wait on conditional variable only if queue is empty and
         main thread needs to be unblocked only on new event */
      mWqNotifier.wait(xac_lock);
    }
    xac_lock.unlock();
    std::atomic_exchange(&mIsMainThreadBusy, true);

    while(1) {
      mWqMutex.lock();

      if (mWorkQueue.empty()) {
        mWqMutex.unlock();
        break;
      } else {
        xac_handler_msg_t *data = mWorkQueue.front();
        mWorkQueue.pop_front();
        mWqMutex.unlock();
        XacMessageHandler(data);

      }
    }
  }
  /* Close the UDP port fds in main thread context in which those were created */
  XpanSocketHandler::CloseTsfRxUdpPort();
  XpanSocketHandler::CloseDataRxUdpPort();
  ALOGE("%s: Main thread finished executing", __func__);
}

bool XpanApplicationController::PostMessage(xac_handler_msg_t *msg, bool isHighPriority) {
  ALOGD("%s: Event =  (%s)", __func__, xpan_event_str(msg->event));

  if (!mIsMainThreadRunning) {
    ALOGE("%s: Main thread is not running", __func__);
    free(msg);
    return false;
  }

  mWqMutex.lock();

  if (isHighPriority) {
    mWorkQueue.push_front(msg);
  } else {
    mWorkQueue.push_back(msg);
  }

  mWqMutex.unlock();

  /* If there is no outstanding task in queue, notify main thread about new message */
  if (!mIsMainThreadBusy) {
    mWqNotifier.notify_all();
  }

  return true;
}

bool XpanApplicationController::RemoveMessage(XacEvent event) {
  ALOGD("%s: Event = %d", __func__, event);

  mWqMutex.lock();

  auto i = mWorkQueue.begin();
  while (i != mWorkQueue.end())
  {
    if ((*i)->event == event) {
      mWorkQueue.erase(i);
      continue;
    }
    i++;
  }

  mWqMutex.unlock();

  return true;
}

void XpanApplicationController::XacMessageHandler(xac_handler_msg_t *msg) {
  XpanDevice *device = NULL;

  XacEvent event = msg->event;
  ALOGD("%s: Event = (%s)", __func__, xpan_event_str(event));

  if (!instance) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free(msg);
    return;
  }

  std::shared_ptr<XpanQhciAcIf> qhci = XpanQhciAcIf::GetIf();

  switch (event) {
    case XM_PREPARE_BEARER_EVT:
         device = instance->CreateOrGetDevice(msg->prepareBearer.addr);
         if (!device) {
           ALOGE("%s: Device instance can not be created.", __func__);
           break;
         }
         device->PrepareBearer(msg);
         break;

    case XM_BEARER_PREFERENCE_EVT:
         device = instance->CreateOrGetDevice(msg->bearerPreference.addr);
         if (!device) {
           ALOGE("%s: Device instance can not be created.", __func__);
           break;
         }
         device->BearerPreference(msg);
         break;

    case XM_UPDATE_REMOTE_AP_PARAMS_EVT:
         device = instance->CreateOrGetDevice(msg->remoteApParams.addr);
         if (!device) {
           ALOGE("%s: Device instance can not be created.", __func__);
           break;
         }
         device->UpdateRemoteApParams(msg);
         break;

    case XM_REQUEST_REMOTE_AP_PARAMS_EVT:
         device = instance->GetDevice(msg->remoteApParams.addr);
         if (!device) {
           ALOGE("%s: Device instance can not be created.", __func__);
           break;
         }
         device->RequestRemoteApParams(msg);
         break;

    case XM_INITIATE_LMP_BEARER_SWITCH_EVT:
         device = instance->GetDevice(msg->initiateLmpBearerSwitch.addr);
         if (!device) {
           ALOGE("%s: Device instance not found", __func__);
           break;
         }
         device->InitiateLmpBearerSwitch(msg);
         break;

    case XPAN_REMOTE_DISCONNECTED_EVT:
         device = instance->GetDevice(msg->initiateLmpBearerSwitch.addr);
         if (!device) {
           ALOGE("%s: Device instance not found", __func__);
           break;
         }
         device->RemoteDisconnectedEvent(msg);
         break;

    case XM_BEARER_SWITCH_IND_EVT:
         device = instance->GetDevice(msg->bearerSwitchInd.addr);
         if (!device) {
           ALOGE("%s: Device instance not found", __func__);
           break;
         }
         device->BearerSwitchInd(msg);
         break;

    case XM_MDNS_DISCOVERY_STATUS_EVT:
         device = instance->GetDevice(msg->mdnsDiscoveryStatus.addr);
         if (!device) {
           ALOGE("%s: Device instance not found", __func__);
           break;
         }
         device->MdnsDiscoveryStatus(msg);
         break;

    case XM_UPDATE_BOND_STATE_EVT:
         instance->BondStateUpdated(msg->bondStateUpdate.addr,
             msg->bondStateUpdate.state);
         break;

    case XM_UPDATE_LOCAL_AP_DETAILS_EVT:
         instance->UpdateLocalApDetails(msg->localApDetails.local_mac_addr,
             msg->localApDetails.bssid, msg->localApDetails.ipAddr,
             msg->localApDetails.uuid, msg->localApDetails.freq);
         break;

    case XM_GET_LOCAL_LISTENING_PORTS_EVT: {
          int tcp_port = XpanSocketHandler::GetTcpListeningPort();
          int udp_port = XpanSocketHandler::GetUdpPort();
          int udp_tsf_port = XpanSocketHandler::GetUdpTsfPort();

          XMXacIf *xm = XMXacIf::GetIf();
          if (xm) {
            xm->HandSetPortNumberRsp(tcp_port, udp_port, udp_tsf_port);
          }
         } break;

    case XM_UPDATE_BONDED_XPAN_DEVICES_EVT:
         instance->LoadXpanBondedDevices(msg->xpanBondedDevices.numOfDevices,
                                         msg->xpanBondedDevices.devices);
         break;

    case XM_SET_FEATURE_MASK_EVT: {
         device = instance->GetDevice(msg->setFeatureMask.addr);
         if (!device) {
           ALOGE("%s: Device instance not found", __func__);
           break;
         }
         device->SetXpanSupportedFeature(msg);
        } break;

    case QHCI_CREATE_CONNECTION_EVT: {
         if (localIpAddr.isEmpty()) {
           ALOGD("%s: AP Details not received. Cache Request", __func__);
           CacheConnectionRequest(msg);
           break;
         }
         device = instance->CreateOrGetDevice(msg->createConn.addr);
         if (!device) {
           ALOGE("%s: Device can not be created.", __func__);
           if (qhci && !msg->createConn.is_bg) {
             qhci->ConnectionCompleteRes(msg->createConn.addr, UNKNOWN_CONNECTION_IDENTIFIER);
           }
           break;
         }
         device->CreateConnection(msg);
        } break;

    case QHCI_CREATE_CONNECTION_CANCEL_EVT:
         RemovePendingConnection(msg->createConnCancel.addr);
         device = instance->GetDevice(msg->createConnCancel.addr);
         if (!device) {
           if (qhci) {
             qhci->ConnectionCompleteRes(
                 msg->createConnCancel.addr, UNKNOWN_CONNECTION_IDENTIFIER);
           }
           break;
         }
         device->CreateConnectionCancel(msg);
         break;

    case QHCI_DISCONNECT_CONNECTION_EVT:
         RemovePendingConnection(msg->disconn.addr);
         device = instance->GetDevice(msg->disconn.addr);
         if (!device) {
           qhci->DisconnectionCompleteRes(msg->disconn.addr, XPAN_AC_SUCCESS);
           break;
         }
         device->DisconnectConnection(msg);
         break;

    case QHCI_GET_REMOTE_VERSION_EVT:
         device = instance->GetDevice(msg->getRemoteVersion.addr);
         if (!device) {
           // callback not needed
           break;
         }
         device->GetRemoteVersion(msg);
         break;

    case QHCI_GET_REMOTE_LE_FEATURES_EVT:
         device = instance->GetDevice(msg->getRemoteLeFeatures.addr);
         if (!device) {
           // callback not needed
           break;
         }
         device->GetRemoteLeFeatures(msg);
         break;

    case QHCI_SET_LOCAL_LE_FEATURES_EVT:
         XpanApplicationController::SetLocalLeFeatures(msg->setLocalLeFeat.le_features);
         /* Load devices and security records */
         // TODO: Uncomment post integration - Added for selectively enable/disable TLS-DTLS
         //if (IsTLSEnabled()) {
           XpanApplicationController::Get()->ReadDevSecurityRecords();
         //}
         break;

    case QHCI_SET_LOCAL_VERSION_EVT: {
         LocalVersionInfo ev = {msg->setLocalVersion.version,
            msg->setLocalVersion.companyId, msg->setLocalVersion.subversion};
         XpanApplicationController::SetLocalVersionInfo(ev);
        }break;

    case QHCI_ENABLE_ENCRYPTION_EVT:
         // TODO - Uncomment post integration tests
         if (IsTLSEnabled()) {
           device = instance->CreateOrGetDevice(msg->encryptionParams.addr);
           if (!device) {
             ALOGE("%s: Device not found", __func__);
             break;
           }
           device->EnableEncrption(msg);
         } else {
           ALOGE("%s: Ignore encyption as TLS is not enabled", __func__);
         }
         break;

    case XPAN_LMP_PREPARE_BEARER_RES_EVT:
         device = instance->GetDevice(msg->prepareBearerRsp.addr);
         if (!device) {
           ALOGE("%s: Invalid device", __func__);
           break;
         }
         device->HandleLmpPrepareBearerRes(msg);
         break;

    case XPAN_LMP_BEARER_SWITCH_RES_EVT:
         device = instance->GetDevice(msg->prepareBearerRsp.addr);
         if (!device) {
           ALOGE("%s: Invalid device", __func__);
           break;
         }
         device->HandleLmpBearerSwitchRes(msg);
         break;

    case XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT:
         device = instance->GetDevice(msg->prepareBearerRsp.addr);
         if (!device) {
           ALOGE("%s: Invalid device", __func__);
           break;
         }
         device->HandleLmpL2capPauseUnpauseRes(msg);
         break;

    case XPAN_LMP_SWITCH_PRIMARY_EVT:
        device = instance->GetDevice(msg->primarySwitchReq.addr);
        if (!device) {
           ALOGE("%s: Invalid request", __func__);
           break;
         }
         device->HandleRoleSwitchRequest(msg);
         break;

    case QHCI_SEND_ACL_DATA_EVT:
         device = instance->GetDevice(msg->aclDataParams.addr);
         if (!device) {
           ALOGE("%s: Invalid device %s for ACL data Tx", __func__,
                 ConvertRawBdaddress(msg->aclDataParams.addr));
           break;
         }
         device->SendAclData(msg);
         break;

    case QHCI_L2CAP_PAUSE_UNPAUSE_RES_EVT:
         device = instance->GetDevice(msg->l2capPauseUnpauseRes.addr);
         if (!device) {
           ALOGE("%s: Invalid device", __func__);
           break;
         }
         device->L2capPauseUnpauseRes(msg);
         break;

    case QHCI_LE_LINK_CONNECTION_STATUS_EVT: {
         device = instance->GetDevice(msg->aclDataParams.addr);
         if (!device) {
           ALOGE("%s: Invalid device %s for Link Status", __func__,
                 ConvertRawBdaddress(msg->aclDataParams.addr));
           break;
         }
         device->HandleLeLinkStatusEvt(msg);
        } break;

    case QHCI_STREAM_STATUS_UPDATE_EVT: {
         device = instance->GetDevice(msg->streamStatus.addr);
         if (!device) {
           ALOGE("%s: Invalid device %s for Link Status", __func__,
                 ConvertRawBdaddress(msg->streamStatus.addr));
           break;
         }
         device->HandleStreamStatusUpdate(msg);
        } break;

    case XPAN_LMP_CONNECTION_RES_EVT:
         device = instance->GetDevice(msg->lmpConnectRsp.addr);
         if (!device) {
           ALOGE("%s: Device instance not found", __func__);
           break;
         }
         device->HandleLmpConnectionRes(msg);
         break;

    case XPAN_LMP_INC_CONNECTION_REQ_EVT:
         device = instance->GetDevice(msg->incLmpConnReq.addr);
         if (!device) {
           ALOGE("%s: Device instance not found", __func__);
           break;
         }
         device->HandleLmpConnectionReq(msg);
         break;

    case XPAN_TCP_CONNECTED_EVT:
         device = instance->GetDevice(msg->tcpConnected.addr);
         if (!device) {
           ALOGE("%s: Device removed. Ignore event", __func__);
           break;
         }
         device->HandleTcpConnectedEvent(msg);
         break;


    case XPAN_TCP_CONNECT_FAILED:
         device = instance->GetDevice(msg->tcpConnFailed.addr);
         if (!device) {
           ALOGE("%s: Device removed. Ignore event", __func__);
           break;
         }
         device->HandleTcpFailedEvent(msg);
         break;

    case XPAN_LMP_EXECUTE_NEXT_CMD:
         {
           XpanLmpManager *lmp =
             XpanApplicationController::Get()->GetLmpManager(msg->executeNext.ip);
           if (!lmp) {
             ALOGE("%s: Couldn't find associated lmp manager. Ignore this response.", __func__);
             break;
           }
           lmp->LmpExecuteNextOp();
         }
         break;

    case XPAN_LMP_REMOTE_PING_REQ_EVT: {
          ipaddr_t ip = msg->pingReq.ip;

          XpanApplicationController *xac = XpanApplicationController::Get();
          if (!xac) {
            ALOGE("%s: Invalid Xac instance", __func__);
            break;
          }

          XpanDevice *dev = xac->GetDeviceByIpAddr(ip);
          if (!dev) {
            ALOGE("%s: No device found for ip: %s", __func__, ip.toString().c_str());
            break;
          }
          dev->HandleLmpPingRequestEvt(msg);
        }
        break;

    case XPAN_LMP_LE_FEATURE_REQ_EVT:
         device = instance->GetDevice(msg->featReq.addr);
         if (!device) {
           ALOGE("%s: Device removed. Ignore event", __func__);
           break;
         }
         device->HandleLmpLeFeatureReq(msg);
         break;

    case XPAN_LMP_VERSION_REQ_EVT:
         device = instance->GetDevice(msg->versionReq.addr);
         if (!device) {
           ALOGE("%s: Device removed. Ignore event", __func__);
           break;
         }
         device->HandleLmpVersionReq(msg);
         break;

    case XPAN_LMP_LE_FEATURE_RES_EVT:
         device = instance->GetDevice(msg->featRsp.addr);
         if (!device) {
           ALOGE("%s: Device removed. Ignore event", __func__);
           break;
         }
         device->HandleLmpVersionReq(msg);
         break;

    case XPAN_LMP_VERSION_RES_EVT:
         device = instance->GetDevice(msg->versionRsp.addr);
         if (!device) {
           ALOGE("%s: Device removed. Ignore event", __func__);
           break;
         }
         device->HandleLmpVersionReq(msg);
         break;

    case XPAN_LMP_ACL_DATA_EVT: {
         device = instance->GetDevice(msg->data.addr);
         if (!device) {
           ALOGE("%s: Device removed. Ignore event", __func__);
           break;
         }
         device->HandleAclDataEvt(msg);
        } break;

    case XPAN_TCP_DISCONNECTED_EVT: {
         device = instance->GetDevice(msg->tcpDisconn.addr);
         if (!device) {
           ALOGE("%s: Device removed. Ignore event", __func__);
           break;
         }
         device->HandleTcpDisconnectedEvent(msg);
        } break;

    case XPAN_WIFI_AP_DISCONNECTED: {
          device = instance->GetDevice(msg->apDisconn.addr);
          if (!device) {
            ALOGE("%s: Device removed. Ignore event", __func__);
            break;
          }
          device->HandleWifiApDisconnectedEvent(msg);
        } break;

    case XPAN_BEARER_PREFERENCE_TIMEOUT: {
         device = instance->GetDevice(msg->bearerPrefTimeout.addr);
          if (!device) {
            ALOGE("%s: Device removed. Ignore event", __func__);
            break;
          }
          device->HandleBearerPreferenceTimeout(msg);
        } break;

    case XPAN_PRIMARY_DISCONNECTING_EVT: {
         device = instance->GetDevice(msg->primaryDiscon.addr);
          if (!device) {
            ALOGE("%s: Device removed. Ignore event", __func__);
            break;
          }
          device->HandlePrimaryDisconnecting(msg);
        } break;

    case XPAN_TARGETED_ADV_CONN_TIMEOUT: {
          device = instance->GetDevice(msg->targetedConnTo.addr);
          if (!device) {
            ALOGE("%s: Device removed. Ignore event", __func__);
            break;
          }
          device->HandleTargetedAdvConnTimeout(msg);
        } break;

    case XPAN_RESET_SUPERVISION_EVT: {
          device = instance->GetDeviceByIpAddr(msg->resetSupervision.ip);
          if (!device) {
            ALOGE("%s: Device removed. Ignore event", __func__);
            break;
          }
          device->HandleResetSupervision(msg);
        } break;

    default:
      ALOGE("%s: Unhandled Event", __func__);
  }
  free(msg);
}

bool XpanApplicationController::RemovePendingConnection(bdaddr_t addr) {
  std::vector<QhciCreateConnection>::iterator it = mStalledConnRequests.begin();
  while (it != mStalledConnRequests.end()) {
    if (it->addr == addr) {
      it = mStalledConnRequests.erase(it);
      ALOGD("%s: removed existing request %s", __func__, addr.toString().c_str());
    } else {
      it++;
    }
  }
  return true;
}

bool XpanApplicationController::CacheConnectionRequest(xac_handler_msg_t *msg) {
  QhciCreateConnection connreq = {
    msg->createConn.event, msg->createConn.addr, msg->createConn.supervision_timeout,
    msg->createConn.is_bg
  };

  // To update connection req if any
  RemovePendingConnection(msg->createConn.addr);

  mStalledConnRequests.push_back(connreq);
  ALOGD("%s: Create Connection: %s is_bg = %d ", __func__,
        msg->createConn.addr.toString().c_str(), msg->createConn.is_bg);

  return true;
}

/* This API caches pending connections when there is event like wifi reset
   while waiting for connection. If BT connection remains pending, without
   this API, connection wont be retried if there is wifi reset while looking
   for connection from remote */
void XpanApplicationController::RecachePendingConnections() {
  for (int i = 0; i < devices.size(); i++) {
    XpanAcStateMachine *psm = devices[i].GetStateMachine(ROLE_PRIMARY);
    if (psm && (psm->GetState() == XPAN_TCP_CONNECTING ||
        psm->GetState() == XPAN_TCP_CONNECTED) &&
        (psm->GetCurrentTransition() == XPAN_RECONNECTION_INCOMING ||
         psm->GetCurrentTransition() == XPAN_RECONNECTION_OUTGOING)) {
      ALOGD("%s: Connection pending with %s. Cache it", __func__,
            devices[i].GetAddr().toString().c_str());
      QhciCreateConnection connreq = {
        QHCI_CREATE_CONNECTION_EVT, devices[i].GetAddr(), XPAN_TCP_SUPERVISION_TIMEOUT,
        psm->IsTargetedBgconn()
      };
      mStalledConnRequests.push_back(connreq);
    }
  }
}

bool XpanApplicationController::IsDevicePresent(bdaddr_t addr) {
  for (int i = 0; i < devices.size(); i++) {
    if (devices[i].GetAddr() == addr) {
      return true;
    }
  }
  return false;
}

XpanDevice* XpanApplicationController::GetDevice(bdaddr_t addr) {
  for (int i = 0; i < devices.size(); i++) {
    if (devices[i].GetAddr() == addr) {
      return &devices[i];
    }
  }
  return NULL;
}

XpanDevice* XpanApplicationController::CreateOrGetDevice(bdaddr_t addr) {
  for (int i = 0; i < devices.size(); i++) {
    if (devices[i].GetAddr() == addr) {
      return &devices[i];
    }
  }
  /* Create new instance */
  ALOGD("%s: Creating new instance for %s", __func__, ConvertRawBdaddress(addr));
  devices.emplace_back(addr);

  for (int i = 0; i < devices.size(); i++) {
    if (devices[i].GetAddr() == addr) {
      return &devices[i];
    }
  }
  return NULL;
}

mdns_uuid_t XpanApplicationController::GetLocalMdnsUuid() {
  return local_uuid;
}

ipaddr_t XpanApplicationController::GetLocalIpAddr() {
  return localIpAddr;
}

macaddr_t XpanApplicationController::GetLocalMacAddr() {
  return localMacAddr;
}

macaddr_t XpanApplicationController::GetLocalApBssid() {
  return localApBssid;
}

uint32_t XpanApplicationController::GetLocalApFrequency() {
  return center_freq;
}

mdns_uuid_t XpanApplicationController::GetMdnsUuid() {
  return local_uuid;
}

uint8_t * XpanApplicationController::GetCod() {
  return local_cod;
}

XpanDevice* XpanApplicationController::GetDeviceByMdnsUuid(mdns_uuid_t uuid) {
  for (int i = 0; i < devices.size(); i++) {
    if (devices[i].GetRemoteMdnsUuid() == uuid) {
      return &devices[i];
    }
  }
  return NULL;
}

XpanLmpManager* XpanApplicationController::GetLmpManager(ipaddr_t ip) {
  for (int i = 0; i < devices.size(); i++) {
    if (devices[i].GetIpAddr(ROLE_PRIMARY) == ip) {
      XpanAcStateMachine *psm = devices[i].GetStateMachine(ROLE_PRIMARY);
      if (psm) {
        return psm->GetLmpManager();
      }
    } else if (devices[i].GetIpAddr(ROLE_SECONDARY) == ip) {
      XpanAcStateMachine *ssm = devices[i].GetStateMachine(ROLE_SECONDARY);
      if (ssm) {
        return ssm->GetLmpManager();
      }
    }
  }
  return NULL;
}

XpanLmpManager* XpanApplicationController::GetLmpManager(int socket_fd) {
  for (int i = 0; i < devices.size(); i++) {
    XpanAcStateMachine *psm = devices[i].GetStateMachine(ROLE_PRIMARY);
    if (psm && psm->GetSocketFd() == socket_fd) {
      return psm->GetLmpManager();
    }

    XpanAcStateMachine *ssm = devices[i].GetStateMachine(ROLE_SECONDARY);
    if (ssm && ssm->GetSocketFd() == socket_fd) {
      return ssm->GetLmpManager();
    }
  }
  return NULL;
}

ipaddr_t XpanApplicationController::GetIpFromFd(int socket_fd) {
  for (int i = 0; i < devices.size(); i++) {
    XpanAcStateMachine *psm = devices[i].GetStateMachine(ROLE_PRIMARY);
    if (psm && psm->GetSocketFd() == socket_fd) {
      return psm->GetIpAddr();
    }

    XpanAcStateMachine *ssm = devices[i].GetStateMachine(ROLE_SECONDARY);
    if (ssm && ssm->GetSocketFd() == socket_fd) {
      return ssm->GetIpAddr();
    }
  }
  return {};
}

XpanDevice* XpanApplicationController::GetDeviceByIpAddr(ipaddr_t ip) {
  for (int i = 0; i < devices.size(); i++) {
    if (devices[i].GetIpAddr(ROLE_PRIMARY) == ip ||
        devices[i].GetIpAddr(ROLE_SECONDARY) == ip) {
      return &devices[i];
    }
  }
  return NULL;
}

bool XpanApplicationController::AddDevSecRecord(mdns_uuid_t uuid, bdaddr_t addr, uint8_t* psk,
    uint8_t *ltk, long feature_mask) {
  for (int i = 0; i < MAX_SUPPORTED_DEV; i++) {
    if (mSecRecords[i].used && mSecRecords[i].addr == addr) {
      ALOGD("%s: update device Security record", __func__);
      memcpy(&mSecRecords[i].ltk, ltk, sizeof(mSecRecords[i].ltk));
      memcpy(&mSecRecords[i].psk, psk, sizeof(mSecRecords[i].psk));
      mSecRecords[i].uuid = uuid;
      mSecRecords[i].feature_mask = feature_mask;
      // Persisting record in storage
      PersistDevSecurityRecords();
      return true;
    }
  }

  for (int i = 0; i < MAX_SUPPORTED_DEV; i++) {
    if (!mSecRecords[i].used) {
      mSecRecords[i].used = true;
      mSecRecords[i].uuid = uuid;
      mSecRecords[i].addr = addr;
      memcpy(&mSecRecords[i].ltk, ltk, sizeof(mSecRecords[i].ltk));
      memcpy(&mSecRecords[i].psk, psk, sizeof(mSecRecords[i].psk));
      mSecRecords[i].feature_mask = feature_mask;
      ALOGD("%s: Created device Security record for %s", __func__, addr.toString().c_str());
      // Persisting record in storage
      PersistDevSecurityRecords();
      return true;
    }
  }

  ALOGE("%s: Couldn't create or update any existing device security record", __func__);

  return false;
}

bool XpanApplicationController::RemoveDevSecRecord(bdaddr_t addr) {
  for (int i = 0; i < MAX_SUPPORTED_DEV; i++) {
    if (mSecRecords[i].used && mSecRecords[i].addr == addr) {
      //memset(&mSecRecords[i], 0, sizeof(XpanDevSecRecord));
      mSecRecords[i].used = false;
      mSecRecords[i].uuid = {};
      memset(&mSecRecords[i].psk, 0, sizeof(mSecRecords[i].psk));
      memset(&mSecRecords[i].ltk, 0, sizeof(mSecRecords[i].ltk));
      mSecRecords[i].addr = {};
      ALOGD("%s: removed Device Security record for %s", __func__, addr.toString().c_str());
      PersistDevSecurityRecords();
      return true;
    }
  }
  return true;
}

bool XpanApplicationController::PersistDevSecurityRecords() {
  FILE *fp = fopen(XPAN_SREC_FILEPATH, "wb");
  if (!fp) {
    ALOGE("%s: Failed to write device security records", __func__);
    return false;
  }

  size_t num_records =fwrite(&mSecRecords, sizeof(XpanDevSecRecord), MAX_SUPPORTED_DEV, fp);
  if (num_records != MAX_SUPPORTED_DEV) {
    ALOGE("%s: Failed to write required records", __func__);
    return false;
  }

  ALOGD("%s: records saved", __func__);
  fclose(fp);

  return true;
}

bool XpanApplicationController::ReadDevSecurityRecords() {
  if (!XpanApplicationController::IsTLSEnabled() &&
      !XpanApplicationController::IsDTLSEnabled()) {
    ALOGE("%s: TLS & DTLS is not enabled", __func__);
    return false;
  }

  FILE *fp = fopen(XPAN_SREC_FILEPATH, "rb");
  if (!fp) {
    ALOGE("%s: Failed to read device security records", __func__);
    return false;
  }

  size_t num_records =fread(&mSecRecords, sizeof(XpanDevSecRecord), MAX_SUPPORTED_DEV, fp);
  if (num_records != MAX_SUPPORTED_DEV) {
    ALOGE("%s: Failed to read required records", __func__);
    fclose(fp);
    return false;
  }

  for (int i = 0; i < (int)num_records; i++) {
    if (mSecRecords[i].used && !instance->IsDevicePresent(mSecRecords[i].addr)) {
      ALOGD("%s: Creating XpanDevice instance for %s", __func__,
            ConvertRawBdaddress(mSecRecords[i].addr));
      XpanDevice device(mSecRecords[i].addr);
      device.SetDeviceSecurityDetails(mSecRecords[i].uuid, mSecRecords[i].psk,
                                      mSecRecords[i].ltk, mSecRecords[i].feature_mask);
      devices.push_back(device);
    }
  }

  fclose(fp);

  return true;
}

uint8_t* XpanApplicationController::GetLtkFromSecRecord(bdaddr_t addr) {
  for (int i = 0; i < MAX_SUPPORTED_DEV; i++) {
    if (mSecRecords[i].used && mSecRecords[i].addr == addr) {
      return mSecRecords[i].ltk;
    }
  }

  ALOGD("%s: LTK not found for %s", __func__, addr.toString().c_str());
  return NULL;
}

mdns_uuid_t XpanApplicationController::GetMdnsUuidFromSecRecord(bdaddr_t addr) {
  for (int i = 0; i < MAX_SUPPORTED_DEV; i++) {
    if (mSecRecords[i].used && mSecRecords[i].addr == addr) {
      return mSecRecords[i].uuid;
    }
  }

  ALOGD("%s: UUID not found for %s", __func__, addr.toString().c_str());
  return {};
}

void XpanApplicationController::PrintKey(uint8_t *key, char *name) {
  char xpan_prop_2[PROPERTY_VALUE_MAX];
  bool VERBOSE = false;
  //default enabled for debugging
  property_get("persist.vendor.qcom.xpan.ac.enc.verbocity", xpan_prop_2, "true");
  if (!strcmp(xpan_prop_2, "true")) {
    VERBOSE = true;
  }

  if (VERBOSE) {
    ALOGD("%s: %s: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:" \
          "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", __func__, name,
          key[0],key[1],key[2],key[3],
          key[4],key[5],key[6],key[7],
          key[8],key[9],key[10],key[11],
          key[12],key[13],key[14],key[15]);
  }
}



void XpanApplicationController::LoadXpanBondedDevices(uint8_t no_of_dev, bdaddr_t *list) {
  for (int i = 0; i < no_of_dev; i++) {
    if (instance && !instance->IsDevicePresent(list[i])) {
      ALOGD("%s: Creating XpanDevice instance for %s", __func__, ConvertRawBdaddress(list[i]));
      XpanDevice device(list[i]);
      devices.push_back(device);
    } else {
      ALOGD("%s: XpanDevice %s already present", __func__, ConvertRawBdaddress(list[i]));
    }
  }
}

mdns_uuid_t XpanApplicationController::SetUuidOrder(mdns_uuid_t uuid) {
  mdns_uuid_t bt_ordered_uuid;

  /* MSBs */
  int msbIndex = 7;
  for (int i = 0; i < 8; i++) {
    bt_ordered_uuid.b[msbIndex - i] = uuid.b[i];
  }

  int lsbIndex = 15;
  for (int i = 8, j = 0; i < 16; i++, j++) {
    bt_ordered_uuid.b[lsbIndex - j] = uuid.b[i];
  }

  return bt_ordered_uuid;
}

void XpanApplicationController::UpdateLocalApDetails (macaddr_t mac, macaddr_t bssid,
                                                      ipaddr_t ipAddr, mdns_uuid_t uuid,
                                                      uint32_t freq) {
  ALOGD("%s: HS IP(%s), Mac Address (%s) BSSID (%s) MDNS UUID(%s) Center freq(%d)",
        __func__, ipAddr.toString().c_str(), mac.toString().c_str(),
        bssid.toString().c_str(), uuid.toString().c_str(), freq);

  ipaddr_t EMPTY_IP{};
  macaddr_t EMPTY_BSSID{};
  std::shared_ptr<XpanQhciAcIf> qhci = XpanQhciAcIf::GetIf();

  bool areDetailsSet = (localIpAddr == EMPTY_IP? false : true);
  bool isIpUpdated = !(localIpAddr == ipAddr);
  bool isHsRoaming = false;
  if (!(localMacAddr == EMPTY_BSSID)) {
    if (!(localApBssid == bssid)) {
      isHsRoaming = true;
    }
  }

  localMacAddr = mac;
  localIpAddr = ipAddr;
  localApBssid = bssid;
  local_uuid = SetUuidOrder(uuid);
  center_freq = freq;

  if (ipAddr.isEmpty()) {
    ALOGD("%s: HS Disconnected from AP", __func__);

    // Close UDP TSF Port
    XpanSocketHandler::CloseTsfRxUdpPort();
    // Close UDP Data Rx Port
    XpanSocketHandler::CloseDataRxUdpPort();

    // Close Listening Socket fd
    for (int i = 0; i < devices.size(); i++) {
      // this will take care of listening and outgoing connection sockets
      XpanWifiDisconnectedEvt *params =
        (XpanWifiDisconnectedEvt *) malloc (sizeof(XpanWifiDisconnectedEvt));
      params->event = XPAN_WIFI_AP_DISCONNECTED;
      params->addr = devices[i].GetAddr();
      PostMessage((xac_handler_msg_t *)params, false);
    }

    /* Cache pending requests so that those can be retriggered */
    RecachePendingConnections();
    // Note:- areDetailsSet flag is local so unsetting it is not required here

    return;
  }

  /* Check if current state is XPAN_AP_ACTIVE for any device.
   * If yes, give Handset AP to AP roaming indication to XM(->CP).
   */

  if (areDetailsSet && isHsRoaming) {
    XMXacIf* xm = XMXacIf::GetIf();
    if (!xm) {
      ALOGE("%s: Invalid Xpan Manager Interface", __func__);
      return;
    }

    for (int i = 0; i < devices.size(); i++) {
      XpanAcStateMachine *xac = devices[i].GetStateMachine(ROLE_PRIMARY);
      if (xac && xac->GetState() == XPAN_AP_ACTIVE &&
          qhci &&  qhci->isStreamingActive(devices[i].GetAddr())) {
        ALOGD("%s: HS AP Roaming case. Inform CP about roaming", __func__);
        xm->UpdateHsApBssidChanged(bssid);
        break;
      }
    }
  } else {
    if (areDetailsSet && !isIpUpdated && XpanSocketHandler::GetUdpPort() > 0 &&
        XpanSocketHandler::GetUdpTsfPort() > 0) {
      ALOGW("%s: local ap details update without wifi reset", __func__);
    // Create UDP TSF sync socket
    } else if (isIpUpdated) {
      if (areDetailsSet) {
        /* If its only IP update without other details reset*/
        XpanSocketHandler::CloseDataRxUdpPort();
        XpanSocketHandler::CloseTsfRxUdpPort();
      }
      XpanSocketHandler::CreateUpdSocketForTsf(localIpAddr);
      // Create UDP port for Audio Rx(voice) data as well
      XpanSocketHandler::CreateUdpSocketForIncomingData(localIpAddr);
      // Send Port details callnacl to Xpan Profile
      XMXacIf *xm = XMXacIf::GetIf();
      if (xm) {
        xm->HandSetPortNumberRsp(XpanSocketHandler::GetTcpListeningPort(),
                                 XpanSocketHandler::GetUdpPort(),
                                 XpanSocketHandler::GetUdpTsfPort());
      }
    }
  }

  if (!localIpAddr.isEmpty()) {
    std::vector<QhciCreateConnection>::iterator it = mStalledConnRequests.begin();
    while (it != mStalledConnRequests.end()) {
      ALOGD("%s: schehdule pending connection for %s", __func__, it->addr.toString().c_str());
      XpanDevice *device = instance->CreateOrGetDevice(it->addr);
      if (!device) {
        ALOGE("%s: Device can not be created.", __func__);
        std::shared_ptr<XpanQhciAcIf> qhci = XpanQhciAcIf::GetIf();
        if (qhci && !it->is_bg) {
          qhci->ConnectionCompleteRes(it->addr, UNKNOWN_CONNECTION_IDENTIFIER);
        }
        it = mStalledConnRequests.erase(it);
        continue;
      }
      device->CreateConnection((xac_handler_msg_t *)&(*it));
      it = mStalledConnRequests.erase(it);
    }
  }

  ALOGD("%s: Stalled connection count = %d", __func__, mStalledConnRequests.size());

}

void XpanApplicationController::BondStateUpdated (bdaddr_t addr, BondState state) {
  ALOGD("%s: %s: state = %d", __func__, ConvertRawBdaddress(addr), state);

  if (state == BOND_NONE || state == 0) {
    // Remote device from XPAN Devices list
    std::vector<XpanDevice>::iterator it = devices.begin();
    while (it != devices.end()) {
      if (it->GetAddr() == addr) {
        /*  Expected HCI DISCONNECT before BONE_NONE State for cleanups */
        // First remove device security record
        RemovePendingConnection(addr);
        RemoveDevSecRecord(addr);
        XpanTLSHandler::RemovePSKWithIdentity(it->GetRemoteMdnsUuid());
        it = devices.erase(it);
        ALOGD("%s: removed %s", __func__, addr.toString().c_str());
      } else {
        it++;
      }
    }
  }

  // debug logs -> to be removed later
  ALOGD("%s: Current Bonded Devices list as below", __func__);
  for (int i = 0; i < devices.size(); i++) {
    ALOGD("%s: %s", __func__, ConvertRawBdaddress(devices[i].GetAddr()));
  }
}

bool XpanApplicationController::IsBonded(bdaddr_t addr) {
  std::vector<XpanDevice>::iterator it = devices.begin();
  while (it != devices.end()) {
    if (it->GetAddr() == addr) {
      return true;
    } else {
      it++;
    }
  }
  return false;
}

void XpanApplicationController::SetLocalVersionInfo(LocalVersionInfo verinfo) {
  info = verinfo;
}

void XpanApplicationController::SetLocalLeFeatures (uint64_t le_feat) {
  le_features = le_feat;
}

LocalVersionInfo XpanApplicationController::GetLocalVersion() {
  return info;
}

uint64_t XpanApplicationController::GetLocalLeFeatures() {
  return le_features;
}

bool XpanApplicationController::IsTLSEnabled() {
  return isXpanTlsEnabled;
}

bool XpanApplicationController::IsDTLSEnabled() {
  return isXpanDtlsEnabled;
}

bool XpanApplicationController::IsWhcSupportingDevice(bdaddr_t addr) {
  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return false;
  }

  XpanDevice *device = xac->GetDevice(addr);
  if (!device) {
    ALOGE("%s: XpanDevice not found for %s", __func__, addr.toString().c_str());
    return false;
  }

  return (device->GetFeatureMask() & XPAN_FEATURE_AP_TOPOLOGY_SUPPORT);
}

XpanTLSHandler* XpanApplicationController::GetTLSHandler(ipaddr_t ip) {
  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return NULL;
  }

  XpanDevice *device = xac->GetDeviceByIpAddr(ip);
  if (!device) {
    ALOGE("%s: device not found for ip %s", __func__, ip.toString().c_str());
    return NULL;
  }

  XpanLmpManager* lmp = device->GetLmpManager(ip);
  if (!lmp) {
    ALOGE("%s: LmpManager not found for ip %s", __func__, ip.toString().c_str());
    return NULL;
  }

  XpanSocketHandler *shandler = lmp->GetSocketHandler();
  if (!shandler) {
    ALOGE("%s: SocketHandler not found for ip %s", __func__, ip.toString().c_str());
    return NULL;
  }

  XpanTLSHandler* tlsHandler = shandler->GetTLSHandler();
  if (!tlsHandler) {
    ALOGE("%s: TlsHandler not found for ip %s", __func__, ip.toString().c_str());
    return NULL;
  }

  return tlsHandler;
}

ipaddr_t XpanApplicationController::ChangeIpEndianness(ipaddr_t ip) {
  ipaddr_t ipLittleEndian;
  int i = 0;
  for (int j = IPv4_SIZE - 1; j >= 0; j--) {
    ipLittleEndian.ipv4[j] = ip.ipv4[i++];
  }

  return ipLittleEndian;
}

const char* xpan_event_str(XacEvent event) {
  switch(event) {
    case XM_PREPARE_BEARER_EVT:
         return "XM_PREPARE_BEARER_EVT";

    case XM_BEARER_PREFERENCE_EVT:
         return "XM_BEARER_PREFERENCE_EVT";

    case XM_UPDATE_REMOTE_AP_PARAMS_EVT:
         return "XM_UPDATE_REMOTE_AP_PARAMS_EVT";

    case XM_REQUEST_REMOTE_AP_PARAMS_EVT:
        return "XM_REQUEST_REMOTE_AP_PARAMS_EVT";

    case XM_INITIATE_LMP_BEARER_SWITCH_EVT:
         return "XM_INITIATE_LMP_BEARER_SWITCH_EVT";

    case XM_BEARER_SWITCH_IND_EVT:
         return "XM_BEARER_SWITCH_IND_EVT";

    case XM_MDNS_DISCOVERY_STATUS_EVT:
         return "XM_MDNS_DISCOVERY_STATUS_EVT";

    case XM_UPDATE_BOND_STATE_EVT:
         return "XM_UPDATE_BOND_STATE_EVT";

    case XM_UPDATE_LOCAL_AP_DETAILS_EVT:
         return "XM_UPDATE_LOCAL_AP_DETAILS_EVT";

    case XM_GET_LOCAL_LISTENING_PORTS_EVT:
         return "XM_GET_LOCAL_LISTENING_PORTS_EVT";

    case XM_UPDATE_BONDED_XPAN_DEVICES_EVT:
         return "XM_UPDATE_BONDED_XPAN_DEVICES_EVT";

    case XM_SET_FEATURE_MASK_EVT:
         return "XM_SET_FEATURE_MASK_EVT";

    case QHCI_CREATE_CONNECTION_EVT:
         return "QHCI_CREATE_CONNECTION_EVT";

    case QHCI_CREATE_CONNECTION_CANCEL_EVT:
         return "QHCI_CREATE_CONNECTION_CANCEL_EVT";

    case QHCI_DISCONNECT_CONNECTION_EVT:
         return "QHCI_DISCONNECT_CONNECTION_EVT";

    case QHCI_GET_REMOTE_VERSION_EVT:
         return "QHCI_GET_REMOTE_VERSION_EVT";

    case QHCI_GET_REMOTE_LE_FEATURES_EVT:
         return "QHCI_GET_REMOTE_LE_FEATURES_EVT";

    case QHCI_SET_LOCAL_VERSION_EVT:
         return "QHCI_SET_LOCAL_VERSION_EVT";

    case QHCI_SET_LOCAL_LE_FEATURES_EVT:
         return "QHCI_SET_LOCAL_LE_FEATURES_EVT";

    case QHCI_ENABLE_ENCRYPTION_EVT:
         return "QHCI_ENABLE_ENCRYPTION_EVT";

    case QHCI_SEND_ACL_DATA_EVT:
         return "QHCI_SEND_ACL_DATA_EVT";

    case QHCI_L2CAP_PAUSE_UNPAUSE_RES_EVT:
         return "QHCI_L2CAP_PAUSE_UNPAUSE_RES_EVT";

    case QHCI_STREAM_STATUS_UPDATE_EVT:
         return "QHCI_STREAM_STATUS_UPDATE_EVT";

    case XPAN_LMP_OUT_CONNECTION_REQ:
         return "XPAN_LMP_OUT_CONNECTION_REQ";

    case XPAN_LMP_INC_CONNECTION_REQ_EVT:
         return "XPAN_LMP_INC_CONNECTION_REQ_EVT";

    case XPAN_LMP_CONNECTION_RES_EVT:
         return "XPAN_LMP_CONNECTION_RES_EVT";

    case XPAN_LMP_VERSION_REQ_EVT:
         return "XPAN_LMP_VERSION_REQ_EVT";

    case XPAN_LMP_VERSION_RES_EVT:
         return "XPAN_LMP_VERSION_RES_EVT";

    case XPAN_LMP_LE_FEATURE_REQ_EVT:
         return "XPAN_LMP_LE_FEATURE_REQ_EVT";

    case XPAN_LMP_LE_FEATURE_RES_EVT:
         return "XPAN_LMP_LE_FEATURE_RES_EVT";

    case XPAN_LMP_REMOTE_PING_REQ_EVT:
         return "XPAN_LMP_REMOTE_PING_REQ_EVT";

    case XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT:
         return "XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT";

    case XPAN_LMP_PREPARE_BEARER_RES_EVT:
         return "XPAN_LMP_PREPARE_BEARER_RES_EVT";

    case XPAN_LMP_BEARER_SWITCH_RES_EVT:
         return "XPAN_LMP_BEARER_SWITCH_RES_EVT";

    case XPAN_LMP_SWITCH_PRIMARY_EVT:
         return "XPAN_LMP_SWITCH_PRIMARY_EVT";

    case XPAN_LMP_ACL_DATA_EVT:
         return "XPAN_LMP_ACL_DATA_EVT";

    case XPAN_TCP_CONNECTED_EVT:
         return "XPAN_TCP_CONNECTED_EVT";

    case XPAN_TCP_CONNECT_FAILED:
         return "XPAN_TCP_CONNECT_FAILED";

    case XPAN_TCP_DISCONNECTED_EVT:
         return "XPAN_TCP_DISCONNECTED_EVT";

    case XPAN_WIFI_AP_DISCONNECTED:
         return "XPAN_WIFI_AP_DISCONNECTED";

    case XPAN_REMOTE_DISCONNECTED_EVT:
         return "XPAN_REMOTE_DISCONNECTED_EVT";

    case XPAN_TCP_NEW_AP_TRANSPORT_STATUS:
         return "XPAN_TCP_NEW_AP_TRANSPORT_STATUS";

    case XPAN_REMOTE_DETAILS_NOT_FOUND_EVT:
         return "XPAN_REMOTE_DETAILS_NOT_FOUND_EVT";

    case XPAN_TLS_ESTABLISHED_EVT:
         return "XPAN_TLS_ESTABLISHED_EVT";

    case XPAN_LMP_EXECUTE_NEXT_CMD:
         return "XPAN_LMP_EXECUTE_NEXT_CMD";

    case XPAN_BEARER_PREFERENCE_TIMEOUT:
         return "XPAN_BEARER_PREFERENCE_TIMEOUT";

    case XPAN_PRIMARY_DISCONNECTING_EVT:
         return "XPAN_PRIMARY_DISCONNECTING_EVT";

    case XPAN_TARGETED_ADV_CONN_TIMEOUT:
         return "XPAN_TARGETED_ADV_CONN_TIMEOUT";

    case QHCI_LE_LINK_CONNECTION_STATUS_EVT:
         return "QHCI_LE_LINK_CONNECTION_STATUS_EVT";

    case XM_LE_TO_AP_URGENCY_EVT:
         return "XM_LE_TO_AP_URGENCY_EVT";

    case XPAN_RESET_SUPERVISION_EVT:
         return "XPAN_RESET_SUPERVISION_EVT";

    default:
          return "Unknown event";
  }
}

const char* xpan_lmpop_str(XpanLmpOperation op) {
  switch (op) {
    case XPAN_LMP_ACCEPTED:
         return "XPAN_LMP_ACCEPTED";

    case XPAN_LMP_NOT_ACCEPTED:
         return "XPAN_LMP_NOT_ACCEPTED";

    case XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ:
        return "XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ";

    case XPAN_LMP_SWITCH_PRIMARY_REQ:
        return "XPAN_LMP_SWITCH_PRIMARY_REQ";

    case XPAN_LMP_BEARER_SWITCH_REQ:
        return "XPAN_LMP_BEARER_SWITCH_REQ";

    case XPAN_LMP_HOST_CONNECTION_REQ:
        return "XPAN_LMP_HOST_CONNECTION_REQ";

    case XPAN_LMP_BEARER_SWITCH_COMPLETE_IND:
        return "XPAN_LMP_BEARER_SWITCH_COMPLETE_IND";

    case XPAN_LMP_PREPARE_BEARER_SWITCH_REQ:
        return "XPAN_LMP_PREPARE_BEARER_SWITCH_REQ";

    case XPAN_LMP_PING_REQ:
        return "XPAN_LMP_PING_REQ";

    case XPAN_LMP_PING_RSP:
        return "XPAN_LMP_PING_RSP";

    case XPAN_LMP_BURST_INTERVAL_REQ:
        return "XPAN_LMP_BURST_INTERVAL_REQ";

    case XPAN_LMP_BURST_INTERVAL_IND:
        return "XPAN_LMP_BURST_INTERVAL_IND";

    case XPAN_LMP_LSTO_IND:
        return "XPAN_LMP_LSTO_IND";

    case XPAN_LMP_NAME_REQ:
        return "XPAN_LMP_NAME_REQ";

    case XPAN_LMP_NAME_RSP:
        return "XPAN_LMP_NAME_RSP";

    case XPAN_LMP_BREDR_FEATURES_REQ:
        return "XPAN_LMP_BREDR_FEATURES_REQ";

    case XPAN_LMP_BREDR_FEATURES_RSP:
        return "XPAN_LMP_BREDR_FEATURES_RSP";

    case XPAN_LMP_LE_FEATURES_REQ:
        return "XPAN_LMP_LE_FEATURES_REQ";

    case XPAN_LMP_LE_FEATURES_RSP:
        return "XPAN_LMP_LE_FEATURES_RSP";

    case XPAN_LMP_VERSION_REQ:
        return "XPAN_LMP_VERSION_REQ";

    case XPAN_LMP_VERSION_RSP:
        return "XPAN_LMP_VERSION_RSP";

    case XPAN_LMP_CANCEL_BEARER_SWITCH:
        return "XPAN_LMP_CANCEL_BEARER_SWITCH";

    default:
        return "Unknown LMP Operation";
  }
}

const char* xpan_state_str(XacSmState state) {
  switch (state) {
    case XPAN_IDLE:
        return "XPAN_IDLE";

    case XPAN_TCP_CONNECTING:
        return "XPAN_TCP_CONNECTING";

    case XPAN_TCP_CONNECTED:
        return "XPAN_TCP_CONNECTED";

    case XPAN_LMP_CONNECTING:
        return "XPAN_LMP_CONNECTING";

    case XPAN_LMP_CONNECTED:
        return "XPAN_LMP_CONNECTED";

    case XPAN_BEARER_SWITCH_PENDING:
        return "XPAN_BEARER_SWITCH_PENDING";

    case XPAN_AP_ACTIVE:
        return "XPAN_AP_ACTIVE";

    case XPAN_DISCONNECTING:
        return "XPAN_DISCONNECTING";

    default:
        return "INVALID_STATE";
  }
}


} // namespace ac
} // namespace xpan
