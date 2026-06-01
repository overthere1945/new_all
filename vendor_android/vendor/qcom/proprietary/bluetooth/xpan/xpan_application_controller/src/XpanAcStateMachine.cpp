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

#define LOG_TAG "vendor.qti.bluetooth.xpan_ac.XpanAcStateMachine"

using xpan::implementation::XpanQhciAcIf;
using xpan::implementation::XMXacIf;

/* Xpan Application Controller State Machine Implementation.*/
namespace xpan {
namespace ac {

XpanAcStateMachine::XpanAcStateMachine(XpanDevice *dev, bdaddr_t bd_addr, XpanEarbudRole role,
                                       TransportType transport) {
  mCurrentState = XPAN_IDLE;
  mPrevState = XPAN_IDLE;
  mEarbudRole = role;
  mBdAddr = bd_addr;
  mTransportFrom = transport;
  mSocketFd = -1;
  mDevice = dev;
  lmp = new XpanLmpManager();
  qhci = XpanQhciAcIf::GetIf();
  xm = XMXacIf::GetIf();
  mSocketHdlr = new XpanSocketHandler(mBdAddr, role);
  lmp->setXpanSocketHandler(mSocketHdlr);
  mBearerSwitchState = -1;
  mOutTcpConnectInProgress = false;

  if (mEarbudRole == ROLE_PRIMARY) {
    int slen = 12;
    const char *s = "[Primary-EB]";
    memcpy(R, s, slen);
  } else {
    int slen = 14;
    const char *s = "[Secondary-EB]";
    memcpy(R, s, slen);
  }

  char xpan_prop[PROPERTY_VALUE_MAX];
  isPingEnabled = false;
  property_get("persist.vendor.qcom.btadvaudio.target.support.xpan_ping", xpan_prop, "false");
  if (!strcmp(xpan_prop, "true")) {
    ALOGI("%s %s: LMP Ping enabled", R, __func__);
    isPingEnabled = true;
  } else {
    ALOGI("%s %s: LMP Ping not enabled", R, __func__);
  }
}

XpanAcStateMachine::~XpanAcStateMachine() {
  ALOGD("%s %s: BDADDR(%s), IP(%s) Role(%d)", R, __func__, mBdAddr.toString().c_str(),
       mIpAddr.toString().c_str(), mEarbudRole);

  if (mEarbudRole == ROLE_PRIMARY && xm) {
    xm->RegisterMdnsService(mBdAddr, MDNS_UNREGISTER);
  }

  if (mDnsQueryTimer) {
    ALOGD("%s %s: Stopping mDnsQueryTimer", R, __func__);
    stopTimer(mDnsQueryTimer);
    mDnsQueryTimer = NULL;
  }

  if (mFilteredScanTimer) {
    ALOGD("%s %s: Stopping mFilteredScanTimer", R, __func__);
    stopTimer(mFilteredScanTimer);
    mFilteredScanTimer = NULL;
  }

  if (mBearerPrefTimer) {
    ALOGD("%s %s: Stopping mBearerPrefTimer", R, __func__);
    stopTimer(mBearerPrefTimer);
    mBearerPrefTimer = NULL;
  }

  if (mSocketHdlr) {
    ALOGD("%s %s: cleaning up Socket Handler", R, __func__);
    delete mSocketHdlr;
    mSocketHdlr = NULL;
  }

  if (lmp) {
    ALOGD("%s %s: cleaning up LMP Manager", R, __func__);
    delete lmp;
    lmp = NULL;
  }

  mIsLeToApHaltedForSeb = false;
  mLeToApUrgencySet = false;
  mProceedPebForLeAp = false;

  mDevice = NULL;
}

bool XpanAcStateMachine::GetRemoteApDetails() {
  ALOGD("%s", __func__);

  char ip_prop[PROPERTY_VALUE_MAX];
  property_get("persist.vendor.qcom.btadvaudio.target.support.xpan_remote_ip", ip_prop, "0.0.0.0");
  ipaddr_t remote_ip;
  ipStringToIpAddr(ip_prop, &remote_ip);

  if (remote_ip.isEmpty()) {
    ALOGE("%s: Invalid ip. Connection will timeout", __func__);
    return false;
  }

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (xac) {
    XmUpdateRemoteApParams *params =
      (XmUpdateRemoteApParams *) malloc (sizeof(XmUpdateRemoteApParams));
    if (params == NULL) {
      ALOGE("%s: failed to allocate memory", __func__);
      return false;
    }
    memset(params, 0, sizeof(XmUpdateRemoteApParams));
    params->event = XM_UPDATE_REMOTE_AP_PARAMS_EVT;
    params->addr = mBdAddr;
    params->is_mdns_update = true;
    params->remote_tcp_port = 51236;
    params->num_of_earbuds = 1;
    params->mdns_uuid = mDevice->GetRemoteMdnsUuid();

    for (int i = 0; i < params->num_of_earbuds; i++) {
      params->role[i] = ROLE_PRIMARY;
      params->ip[i] = remote_ip;
    }
    xac->PostMessage((xac_handler_msg_t *)params, MSG_PRIORITY_DEFAULT);
  }
  return true;
}

void XpanAcStateMachine::XpanAcSmExecuteEvent(xac_handler_msg_t* msg) {
  switch(mCurrentState) {
    case XPAN_IDLE:
      XpanIdleStateHandler(msg);
      break;

    case XPAN_TCP_CONNECTING:
      XpanTcpConnectingStateHandler(msg);
      break;

    case XPAN_TCP_CONNECTED:
      XpanTcpConnectedStateHandler(msg);
      break;

    case XPAN_LMP_CONNECTING:
      XpanLmpConnectingStateHandler(msg);
      break;

    case XPAN_LMP_CONNECTED:
      XpanLmpConnectedStateHandler(msg);
      break;

    case XPAN_BEARER_SWITCH_PENDING:
      XpanBearerSwitchPendingStateHandler(msg);
      break;

    case XPAN_AP_ACTIVE:
      XpanApActiveStateHandler(msg);
      break;

    case XPAN_DISCONNECTING:
      XpanDisconectingStateHandler(msg);
  }
}

void XpanAcStateMachine::XpanIdleStateHandler(xac_handler_msg_t* msg) {
  XacEvent evt = msg->event;
  ALOGD("%s State: XPAN_IDLE, ProcessEvent = %s", R, xpan_event_str(msg->event));

  switch(evt) {
    case QHCI_CREATE_CONNECTION_EVT: {
         mTransitionType = XPAN_RECONNECTION_OUTGOING;
         mConnIsBg = msg->createConn.is_bg;
         if (mEarbudRole == ROLE_PRIMARY && !HandleInitReconnection(mConnIsBg)) {
           mConnIsBg = false;
           ALOGE("%s %s: Connection Initiation Failed", R, __func__);
           return;
         }
         setState(XPAN_TCP_CONNECTING, evt);
         } break;

    case QHCI_CREATE_CONNECTION_CANCEL_EVT:
         if (qhci && mEarbudRole == ROLE_PRIMARY) {
           qhci->ConnectionCompleteRes(mBdAddr, UNKNOWN_CONNECTION_IDENTIFIER);
         }
         break;

    case XM_BEARER_PREFERENCE_EVT:
         if (msg->bearerPreference.bearer != XPAN_AP) {
           ALOGE("%s: Ignore Bearer Preference to %d in IDLE State",
                 __func__, msg->bearerPreference.bearer);
           return;
         }
         mTransitionType = XPAN_LE_TO_AP_IDLE;
         mBearerPrefTimer = new XpanAcTimer("BearerPreferenceTimer", BearerPreferenceTimeout, this);
         mBearerPrefTimer->StartTimer(XPAN_AC_BEARER_PREFERENCE_TIMER);
         setState(XPAN_TCP_CONNECTING, evt);
         /* Initiate TCP Connection */
         mSocketHdlr->InitTcpConnection(mSocketHdlr, mIpAddr, mTcpPort);
         break;

    case XM_PREPARE_BEARER_EVT:
         if (msg->prepareBearer.bearer != XPAN_AP) {
           ALOGE("%s: Ignore prepare bearer to %d in IDLE State",
                 __func__, msg->bearerPreference.bearer);
           return;
         }
         mTransitionType = XPAN_LE_TO_AP_STREAMING;
         setState(XPAN_TCP_CONNECTING, evt);
         /* Initiate TCP Connection */
         mSocketHdlr->InitTcpConnection(mSocketHdlr, mIpAddr, mTcpPort);
         break;

    case XPAN_TCP_CONNECTED_EVT:
        mTransitionType = XPAN_RECONNECTION_INCOMING;
        /* Check for Incoming connection */
        if (msg->tcpConnected.isIncoming) {
          mTransitionType = XPAN_RECONNECTION_INCOMING;
        }
        break;

    default:
        ALOGE("%s %s: Unanticipated Event %d", R, __func__, evt);
  }

}

void XpanAcStateMachine::XpanTcpConnectingStateHandler(xac_handler_msg_t* msg) {
  XacEvent evt = msg->event;
  ALOGD("%s State: XPAN_TCP_CONNECTING, ProcessEvent = %s", R, xpan_event_str(msg->event));

  switch(evt) {
    case QHCI_CREATE_CONNECTION_EVT: {
         /* This event is received when user initiates connection while waiting
            for targeted adv / incoming TCP connection from EB */
         if (mConnIsBg && !msg->createConn.is_bg) {
           mConnIsBg = false;
           bool queryTriggered = GetRemoteApDetails();
           if (!queryTriggered) {
             queryTriggered = xm->TriggerMdnsQuery(mBdAddr, MDNS_QUERY_START);
             mDnsQueryTimer = new XpanAcTimer("MDnsQueryTimer", HandleMdnsQueryTimeout, this);
             mDnsQueryTimer->StartTimer(XPAN_AC_MDNS_QUERY_TIMEOUT);
           }
          } else {
            ALOGW("%s: Connection Details- Prev Connect IsBg = %d, current trigger = %d"
                  ". Already pending connection. Skip",
                  __func__, mConnIsBg, msg->createConn.is_bg);
          }
        } break;

    case QHCI_CREATE_CONNECTION_CANCEL_EVT: {
         /* Steps: (Handled in XPAN_DISCONNECTING)
           1. close fd for outgoing/incoming connection.
           2. Inform XP to stop MDNS query
           2. Callback to QHCI that connection is cancelled (XPAN_DISCONNECTING State).*/
         setState(XPAN_DISCONNECTING, evt);
         if (xm) {
          xm->TriggerMdnsQuery(mBdAddr, MDNS_QUERY_STOP);
          xm->RegisterMdnsService(mBdAddr, MDNS_UNREGISTER);
         }
         mSocketHdlr->StopArpRetry(true);
         XpanAcSmExecuteEvent(msg);
        } break;

    case QHCI_DISCONNECT_CONNECTION_EVT: {
         mOutTcpConnectInProgress = false;
         mSocketHdlr->StopArpRetry(true);
         /* Only needed for secondary when primary connection is done and secondary
            is in connecting state when disconnection came from QHCI*/
         if (mEarbudRole == ROLE_SECONDARY) {
          msg->event = QHCI_CREATE_CONNECTION_CANCEL_EVT;
          setState(XPAN_DISCONNECTING, evt);
          XpanAcSmExecuteEvent(msg);
         }
        } break;

    case XM_UPDATE_REMOTE_AP_PARAMS_EVT:
        /* Note: Received for Outgoing reconnection */
        /* Steps:
         * 1. Stop Mdns Timer
         * 2. Stop Listening if no pending connection
         * 3. Initiate Connection
         */
        if (mEarbudRole == ROLE_PRIMARY) {
          if (mDnsQueryTimer) {
            stopTimer(mDnsQueryTimer);
            mDnsQueryTimer = NULL;
          }
          if (mConnIsBg) {
            ALOGE("%s: Outgoing connection can't be established as BTHost"
                  " is waiting for incoming connection (targeted advertisement)", __func__);
            return;
          }
        }
        /* Applicable for both roles */
        HandleRemoteApDetailsUpdate(msg);
        break;

    case XM_MDNS_DISCOVERY_STATUS_EVT: {
         uint8_t status = msg->mdnsDiscoveryStatus.status;
         MDNS_OP state = msg->mdnsDiscoveryStatus.state;
         if ((state == MDNS_QUERY_START &&
               status == MDNS_DISCOVERY_FAILED_TO_START) ||
             (state == MDNS_QUERY_STOP)) {
           if (mDnsQueryTimer) {
             stopTimer(mDnsQueryTimer);
             mDnsQueryTimer = NULL;
           }
           if (XpanSocketHandler::IsListeningOnTcp()) {
             ALOGD("%s %s: Waiting for incoming connection now", R, __func__);
             mTransitionType = XPAN_RECONNECTION_INCOMING;
           }
         }
        } break;

    case XPAN_TCP_CONNECTED_EVT: {
         setState(XPAN_TCP_CONNECTED, evt);
         XpanTcpConnectedEvt *tcpConn = (XpanTcpConnectedEvt *)msg;
         mSocketFd = tcpConn->fd;
         mOutTcpConnectInProgress = false;
         if (!tcpConn->isIncoming) {
           /* Outgoing Connection */
           //if (tlsNotSupported) { // Init Lmp Host Connection
           mSocketHdlr->CloseListeningSocket(mBdAddr);
           XpanLmpOutgoingConnReq outLmpConnReq{XPAN_LMP_OUT_CONNECTION_REQ,
              mIpAddr, mTcpPort, mLocalUuid, BT_LE, {/* COD - Set in TCP Connected state */}};
           XpanAcSmExecuteEvent((xac_handler_msg_t *)&outLmpConnReq);
           /*else if (tlsSupported) {
             // Note: XpanLmpOutgoingConnReq to be initiated after TLS Established
                      event in TCP_CONNECTED State.

           }*/
         } else {
           /* Incoming Connection */
           /*
           if (tls_supported) {
             // Call TLS API to wait on Handshake completion
           } else {
             // Nothing to do: Wait for Incoming LMP Host Connection request
           }
           */
         }
        }
        break;

    case XPAN_TCP_CONNECT_FAILED: {
         if (mTransitionType == XPAN_RECONNECTION_OUTGOING) {
           mOutTcpConnectInProgress = false;
           if (XpanSocketHandler::IsListeningOnTcp() && mEarbudRole == ROLE_PRIMARY) {
             ALOGD("%s %s: Waiting for incoming connection now", R, __func__);
             mTransitionType = XPAN_RECONNECTION_INCOMING;
           } else {
             if (qhci && mEarbudRole == ROLE_PRIMARY) {
               qhci->ConnectionCompleteRes(mBdAddr, UNKNOWN_CONNECTION_IDENTIFIER);
             }
             setState(XPAN_IDLE, evt);
           }
         } else if (mTransitionType == XPAN_LE_TO_AP_IDLE && xm) {
           setState(XPAN_IDLE, evt);
           xm->BearerSwitchFailedRsp(mBdAddr, msg->tcpConnFailed.reason, mEarbudRole);
           HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
           UpdatePrimaryDisconnToSec();
           if (mEarbudRole == ROLE_SECONDARY) {
             ProgressForLeApInPeb();
           }
         } else if (mTransitionType == XPAN_LE_TO_AP_STREAMING && xm) {
           setState(XPAN_IDLE, evt);
           xm->BearerSwitchFailedRsp(mBdAddr, msg->tcpConnFailed.reason, mEarbudRole);
           if (mEarbudRole == ROLE_PRIMARY) {
             xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
           } else if (mEarbudRole == ROLE_SECONDARY) {
             ProgressForLeApInPeb();
           }
           UpdatePrimaryDisconnToSec();
         }
       }
       break;

    case QHCI_STREAM_STATUS_UPDATE_EVT: {
        HandleStreamStatusEventFromStack(msg);
       }
       break;

    case XM_BEARER_SWITCH_IND_EVT: {
         uint8_t reason = msg->bearerSwitchInd.reason;
         /* Check if streaming stopped during LMP Bearer switch */
         if (reason == XM_KP_PORT_CLOSED && HandleStreamStopDuringTransition()) {
           break;
         }
       }
    case XPAN_BEARER_PREFERENCE_TIMEOUT: {
         setState(XPAN_DISCONNECTING, evt);
         XpanAcSmExecuteEvent(msg);
         // This event is received for LE->AP idle case timeout only in this state
         if (mTransitionType == XPAN_LE_TO_AP_IDLE && xm) {
           HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
         }
         xm->BearerSwitchFailedRsp(mBdAddr, TCP_HANDSHAKE_FAILED, mEarbudRole);
       }
       break;

    /* Received only for Incoming connect request since TCP Connected event is skipped
     * as IP address or bd address of remote cant be matched during incoming tcp connection
     * request phase */
    case XPAN_LMP_INC_CONNECTION_REQ_EVT: {
        mSocketHdlr->CloseListeningSocket(mBdAddr);
        // Mark outgoing connection as false as it will be cancelled
        mOutTcpConnectInProgress = false;
        /* Set State to TCP_Connected first and Handle LMP Connect request */
        setState(XPAN_TCP_CONNECTED, evt);
        XpanAcSmExecuteEvent(msg);
       } break;

    case XPAN_REMOTE_DISCONNECTED_EVT: /* received when ip address resets for the role*/
    case XPAN_PRIMARY_DISCONNECTING_EVT:
    case XPAN_WIFI_AP_DISCONNECTED: {
        mSocketHdlr->StopArpRetry(true);
        /* Set State to Disconnecting for Socket cleanups */
        setState(XPAN_DISCONNECTING, evt);
        XpanAcSmExecuteEvent(msg);
        if (mTransitionType == XPAN_LE_TO_AP_IDLE) {
          xm->BearerSwitchFailedRsp(mBdAddr, TCP_HANDSHAKE_FAILED, mEarbudRole);
          HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
          if (mEarbudRole == ROLE_SECONDARY) {
            ProgressForLeApInPeb();
          }
        } else if (mTransitionType == XPAN_LE_TO_AP_STREAMING) {
          xm->BearerSwitchFailedRsp(mBdAddr, TCP_HANDSHAKE_FAILED, mEarbudRole);
          if (mEarbudRole == ROLE_PRIMARY) {
            xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
          } else if (mEarbudRole == ROLE_SECONDARY) {
            ProgressForLeApInPeb();
          }
        }
      } break;

    case XM_LE_TO_AP_URGENCY_EVT: {
        ALOGD("%s %s: Set urgency to %d", R, __func__, msg->leApUrgency.is_urgent);
        mLeToApUrgencySet = msg->leApUrgency.is_urgent;
        mSocketHdlr->StopArpRetry(true);
        XpanAcStateMachine *seb_sm = mDevice->GetStateMachine(ROLE_SECONDARY);
        if (seb_sm && seb_sm->GetState() == XPAN_TCP_CONNECTING) {
          XpanSocketHandler *seb_shdlr = seb_sm->GetSocketHandler();
          if (seb_shdlr) {
            seb_shdlr->StopArpRetry(true);
          }
        }
      } break;

    case QHCI_LE_LINK_CONNECTION_STATUS_EVT: {
        uint8_t state = msg->leLinkStatus.state;
        TransportType qhci_active_transport = msg->leLinkStatus.qhci_active_transport;
        if (state == 0 && qhci_active_transport == BT_LE &&
              (mTransitionType == XPAN_LE_TO_AP_IDLE ||
               mTransitionType == XPAN_LE_TO_AP_STREAMING)) {
          ALOGW("%s: LE Link disconnected while it was active transport for QHCI", __func__);
          /* Note - No need to update XM about prepare audio bearer failure as QHCI sends
           *        unprepare to xm on disconnection */
          setState(XPAN_DISCONNECTING, evt);
          XpanAcSmExecuteEvent(msg);
        }
      } break;

    default:
        ALOGE("%s %s: Unanticipated Event %d", R, __func__, evt);
  }
}

void XpanAcStateMachine::XpanTcpConnectedStateHandler(xac_handler_msg_t* msg) {
  XacEvent evt = msg->event;
  ALOGD("%s State: XPAN_TCP_CONNECTED, ProcessEvent = %s", R, xpan_event_str(msg->event));
  mdns_uuid_t rem_uuid;

  switch(evt) {
    case QHCI_CREATE_CONNECTION_CANCEL_EVT:
         /* TODO: close fd for outgoing connection */
         /* Cancel Secondary connection if attempted */
         /* Callback to QHCI that connection is cancelled */
         setState(XPAN_DISCONNECTING, evt);
         XpanAcSmExecuteEvent(msg);
         break;

    case XPAN_LMP_INC_CONNECTION_REQ_EVT: {
         mTransitionType = XPAN_RECONNECTION_INCOMING;
         /* Incoming Connection */
         rem_uuid = msg->incLmpConnReq.remote_mdns_uuid;
         XpanDevice *dev = GetDeviceInstance(rem_uuid);
         if (!dev) {
           /* Wouldn't execute this condition as MDNS uuid is already matched in
            * XPan Lmp Manager */
           ALOGE("%s: MDNS UUID didnt match. Received(%s)", __func__,
                 rem_uuid.toString().c_str());
           lmp->SendLmpNotAccepted(XPAN_LMP_CONN_REJECTED_INVALID_DEVICE);
           setState(XPAN_DISCONNECTING, evt);
           XpanAcSmExecuteEvent(msg);
           return;
         }

         /* Stop discovery in Xpan profile if Profile hast not discovered remote
            and but incoming connection has progressed */
         if (mDnsQueryTimer) {
           ALOGD("%s: Stop MDNS discovery and its timer", __func__);
           xm->TriggerMdnsQuery(mBdAddr, false);
           stopTimer(mDnsQueryTimer);
           mDnsQueryTimer = NULL;
         }

         /* Update device psm ip address , psm , fd etc*/
         mIpAddr = msg->incLmpConnReq.ip;
         mSocketFd = msg->incLmpConnReq.fd;
         mSocketHdlr->SetSocketFd(mSocketFd);
         // Send acceptance to remote and callback to QHCI
         lmp->SetIpAddress(mIpAddr);
         if (mConnIsBg) {
           ALOGD("%s: Send Targeted Adv to BTHOST", __func__);
           qhci->RemoteAvaiableForConn(mBdAddr);
           mTargetedConnTimer = new XpanAcTimer("TargetedConnTimer",
                                  TargetedConnectionTimeout, this);
           mTargetedConnTimer->StartTimer(XPAN_AC_TARGETED_CONN_TIMER);
         } else {
           lmp->SendLmpAccepted();
           setState(XPAN_LMP_CONNECTED, evt);
           if (mEarbudRole == ROLE_PRIMARY) {
             qhci->ConnectionCompleteRes(mBdAddr, XPAN_AC_SUCCESS);
           }
           HandleXpanLmpConnected();
         }
        }
        break;

    /* Note - Below even is received in this state only after targeted advertisement is
              sent to BT Host for bg connection*/
    case QHCI_CREATE_CONNECTION_EVT: {
         if (mConnIsBg) {
           mConnIsBg = false;
           lmp->SendLmpAccepted();
           setState(XPAN_LMP_CONNECTED, evt);
           if (mEarbudRole == ROLE_PRIMARY) {
             qhci->ConnectionCompleteRes(mBdAddr, XPAN_AC_SUCCESS);
           }
           /* Stopping timer that expects Create Connection for directed advertisement*/
           if (mTargetedConnTimer) {
             stopTimer(mTargetedConnTimer);
             mTargetedConnTimer = NULL;
           }
           HandleXpanLmpConnected();
         } else {
           ALOGE("%s: Unexpected Event, ignore", __func__);
         }
        } break;

    case XPAN_TARGETED_ADV_CONN_TIMEOUT: {
         if (mConnIsBg) {
           mConnIsBg = false;
           lmp->SendLmpNotAccepted(0xFF);
           setState(XPAN_DISCONNECTING, evt);
           XpanAcSmExecuteEvent(msg);
         } else {
           ALOGE("%s: Unexpected Event, ignore", __func__);
           if (mTargetedConnTimer) {
            stopTimer(mTargetedConnTimer);
            mTargetedConnTimer = NULL;
           }
         }
        } break;

    case XPAN_LMP_OUT_CONNECTION_REQ: {
         setState(XPAN_LMP_CONNECTING, evt);
         /* Handset Class of Device*/
         uint8_t cod[3] = { 0x0C,  /* Minor Device class - Smartphone*/
                            0x42,  /* Major Device Class: Phone (cellular, cordless,
                                  payphone, modem,...)*/
                            0x5A}; /* Service class*/
         lmp->SendLmpConnectionReq(BT_LE, XpanApplicationController::GetMdnsUuid(), cod);
         }
         break;

    case XPAN_REMOTE_DETAILS_NOT_FOUND_EVT: {
        /* Note: Incoming connection form unbonded device */
         setState(XPAN_DISCONNECTING, evt);
         XpanAcSmExecuteEvent(msg);
        /* TODO: close fd and state machine */
        }
        break;

    case XPAN_TCP_DISCONNECTED_EVT:
        setState(XPAN_DISCONNECTING, evt);
        XpanAcSmExecuteEvent(msg);
        break;

    case XPAN_PRIMARY_DISCONNECTING_EVT:
    case XPAN_BEARER_PREFERENCE_TIMEOUT: {
        setState(XPAN_DISCONNECTING, evt);
        XpanAcSmExecuteEvent(msg);
        // This event is received for LE->AP idle case timeout only in this state
        if (mTransitionType == XPAN_LE_TO_AP_IDLE && xm) {
          HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
        }
       }
       break;

    case XM_LE_TO_AP_URGENCY_EVT: {
        ALOGD("%s %s: Set urgency to %d", R, __func__, msg->leApUrgency.is_urgent);
        mLeToApUrgencySet = msg->leApUrgency.is_urgent;
      } break;

    default:
        ALOGE("%s %s: Unanticipated Event %d", R, __func__, evt);
  }
}

void XpanAcStateMachine::XpanLmpConnectingStateHandler(xac_handler_msg_t* msg) {
  XacEvent evt = msg->event;
  ALOGD("%s State: XPAN_LMP_CONNECTING, ProcessEvent = %s", R, xpan_event_str(msg->event));
  uint8_t status = XPAN_AC_SUCCESS;

  switch(evt) {
    /* QHCI_DISCONNECT_CONNECTION_EVT can come only for secondary in this state (not for primary) */
    case QHCI_DISCONNECT_CONNECTION_EVT:
    case QHCI_CREATE_CONNECTION_CANCEL_EVT:
         /* TODO: close fd for outgoing connection */
         /* Cancel Secondary connection if attempted */
         /* Callback to QHCI that connection is cancelled */
         setState(XPAN_DISCONNECTING, evt);
         XpanAcSmExecuteEvent(msg);
         break;

    case XPAN_LMP_CONNECTION_RES_EVT:
         status = msg->lmpConnectRsp.status;

         /* Failure response */
         if (status != XPAN_AC_SUCCESS) {
           setState(XPAN_DISCONNECTING, evt);
           XpanAcSmExecuteEvent(msg);
           if (mEarbudRole == ROLE_PRIMARY) {
             if (mTransitionType == XPAN_LE_TO_AP_IDLE) {
              xm->BearerSwitchFailedRsp(mBdAddr, LMP_CONNECTION_FAILURE, mEarbudRole);
              HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
             } else if (mTransitionType == XPAN_LE_TO_AP_STREAMING) {
              xm->BearerSwitchFailedRsp(mBdAddr, LMP_CONNECTION_FAILURE, mEarbudRole);
              xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
             }
           } else if (mEarbudRole == ROLE_SECONDARY) {
             xm->BearerSwitchFailedRsp(mBdAddr, LMP_CONNECTION_FAILURE, mEarbudRole);
             if (mTransitionType == XPAN_LE_TO_AP_IDLE) {
              HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
             }
             ProgressForLeApInPeb();
           }
         /* Success Response */
         } else {
           setState(XPAN_LMP_CONNECTED, evt);
           // Callback to QHCI and XM only from Primary SM
           if (mEarbudRole == ROLE_PRIMARY) {
             qhci->ConnectionCompleteRes(mBdAddr, XPAN_AC_SUCCESS);
           }
           HandleXpanLmpConnected();
         }
         break;

    case XPAN_REMOTE_DISCONNECTED_EVT:
    case XPAN_WIFI_AP_DISCONNECTED:
    case XPAN_TCP_DISCONNECTED_EVT:
        setState(XPAN_DISCONNECTING, evt);
        XpanAcSmExecuteEvent(msg);
        if (mTransitionType == XPAN_LE_TO_AP_IDLE)
          HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
        else if (mTransitionType == XPAN_LE_TO_AP_STREAMING) {
          if (mEarbudRole == ROLE_PRIMARY) {
            xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
          }
          xm->BearerSwitchFailedRsp(mBdAddr, LMP_CONNECTION_FAILURE, mEarbudRole);
        }
        break;

    case QHCI_STREAM_STATUS_UPDATE_EVT: {
        HandleStreamStatusEventFromStack(msg);
       }
       break;

    case XM_BEARER_SWITCH_IND_EVT: {
         uint8_t reason = msg->bearerSwitchInd.reason;
         /* Check if streaming stopped during LMP Bearer switch */
         if (reason == XM_KP_PORT_CLOSED && HandleStreamStopDuringTransition()) {
           break;
         }
       }
    case XPAN_PRIMARY_DISCONNECTING_EVT:
    case XPAN_BEARER_PREFERENCE_TIMEOUT: {
        setState(XPAN_DISCONNECTING, evt);
        XpanAcSmExecuteEvent(msg);
        // This event is received for LE->AP idle case timeout only in this state
        if (mTransitionType == XPAN_LE_TO_AP_IDLE && xm) {
          HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
        }
        xm->BearerSwitchFailedRsp(mBdAddr, LMP_CONNECTION_FAILURE, mEarbudRole);
       }
       break;

    case XM_LE_TO_AP_URGENCY_EVT: {
        ALOGD("%s %s: Set urgency to %d", R, __func__, msg->leApUrgency.is_urgent);
        mLeToApUrgencySet = msg->leApUrgency.is_urgent;
      } break;

    case QHCI_LE_LINK_CONNECTION_STATUS_EVT: {
        uint8_t state = msg->leLinkStatus.state;
        TransportType qhci_active_transport = msg->leLinkStatus.qhci_active_transport;
        if (state == 0 && qhci_active_transport == BT_LE &&
              (mTransitionType == XPAN_LE_TO_AP_IDLE ||
               mTransitionType ==XPAN_LE_TO_AP_STREAMING)) {
          ALOGW("%s: LE Link disconnected while it was active transport for QHCI", __func__);
          /* Note - No need to update XM about prepare audio bearer failure as QHCI sends unprepare
           *        to xm on disconnection */
          setState(XPAN_DISCONNECTING, evt);
          XpanAcSmExecuteEvent(msg);
        }
      } break;

    default:
        ALOGE("%s %s: Unanticipated Event %d", R, __func__, evt);
  }
}

void XpanAcStateMachine::XpanLmpConnectedStateHandler(xac_handler_msg_t* msg) {
  XacEvent evt = msg->event;
  ALOGD("%s State: XPAN_LMP_CONNECTED, ProcessEvent = %s", R, xpan_event_str(msg->event));

  switch(evt) {
    case QHCI_ENABLE_ENCRYPTION_EVT:
         /*TODO: TLS */
         break;

    case XM_INITIATE_LMP_BEARER_SWITCH_EVT:
         setState(XPAN_BEARER_SWITCH_PENDING, evt);
         // TODO: Initiate LMP procedure
         break;

    /* TODO: To be moved to TCP Connected State */
    case XPAN_TLS_ESTABLISHED_EVT:
         if (mTransitionType == XPAN_RECONNECTION_OUTGOING ||
             mTransitionType == XPAN_RECONNECTION_INCOMING) {
           // No need to perform bearer switch procedure
           setState(XPAN_AP_ACTIVE, evt);
         } else {
           if (mTransitionType == XPAN_LE_TO_AP_IDLE ||
               mTransitionType == XPAN_LE_TO_AP_STREAMING) {
             // below call gets triggered for both earbuds
             lmp->SendLmpPrepareBearerReq(LMP_TP_LE, LMP_TP_AP);
           }
         }
         break;

    case XPAN_LMP_REMOTE_PING_REQ_EVT:
        lmp->SendLmpPingRes();
        break;

    /* ACL data before transition comepleted (could be delayed due to congestion) */
    case XPAN_LMP_ACL_DATA_EVT: {
         std::vector<uint8_t> data(msg->data.data,
                                  (msg->data.data + msg->data.len));
         qhci->DataReceivedCb(mBdAddr, msg->data.llid, data);
        } break;

    case XPAN_REMOTE_DISCONNECTED_EVT:
    case XPAN_TCP_DISCONNECTED_EVT:
    case QHCI_DISCONNECT_CONNECTION_EVT:
        setState(XPAN_DISCONNECTING, evt);
        XpanAcSmExecuteEvent(msg);
        break;

    case XPAN_PRIMARY_DISCONNECTING_EVT:
    case XPAN_BEARER_PREFERENCE_TIMEOUT: {
        setState(XPAN_DISCONNECTING, evt);
        XpanAcSmExecuteEvent(msg);
        // This event is received for LE->AP idle case timeout only in this state
        if (mTransitionType == XPAN_LE_TO_AP_IDLE && xm) {
          HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
        }
       }
       break;

    case XM_LE_TO_AP_URGENCY_EVT: {
        ALOGD("%s %s: Set urgency to %d", R, __func__, msg->leApUrgency.is_urgent);
        mLeToApUrgencySet = msg->leApUrgency.is_urgent;
      } break;

    default:
        ALOGE("%s %s: Unanticipated Event %d", R, __func__, evt);
  }
}

void XpanAcStateMachine::XpanBearerSwitchPendingStateHandler(xac_handler_msg_t* msg) {
  XacEvent evt = msg->event;
  ALOGD("%s State: XPAN_BEAARER_SWITCH_PENDING, ProcessEvent = %s", R, xpan_event_str(msg->event));

  uint8_t status;

  switch(evt) {
    case XPAN_LMP_PREPARE_BEARER_RES_EVT:
         HandleLmpPrepareBearerRspEvt(msg);
         break;

    case XM_INITIATE_LMP_BEARER_SWITCH_EVT: {
           mOperation = XPAN_BEARER_SWITCH;
           // Init L2CAP pause. Pause QHCI traffic first
           TransportType pauseTransport = XPAN_AP;
           if (mTransitionType == XPAN_LE_TO_AP_IDLE ||
               mTransitionType == XPAN_LE_TO_AP_STREAMING) {
             pauseTransport = BT_LE;
           }
           qhci->L2capPauseUnpauseReq(mBdAddr, PAUSE, pauseTransport);
         } break;

    case QHCI_L2CAP_PAUSE_UNPAUSE_RES_EVT:
         /* QHCI response for pause/unpause */
         HandleQhciPauseUnpauseEvt(msg);
         break;

    case XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT:
         HandleLmpPauseUnpauseRspEvt(msg);
         break;

    case XPAN_LMP_BEARER_SWITCH_RES_EVT:
         HandleLmpBearerSwitchRspEvt(msg);
         break;

    case XM_BEARER_SWITCH_IND_EVT:
         HandleXmBearerSwitchInd(msg);
         break;


    case QHCI_STREAM_STATUS_UPDATE_EVT: {
        HandleStreamStatusEventFromStack(msg);
       }
       break;

          /* During AP-AP roaming only*/
    case XPAN_TCP_NEW_AP_TRANSPORT_STATUS:
         HandleNewTcpTransportReadyEvt(msg);
         break;

    case XPAN_RESET_SUPERVISION_EVT:
         /* resetting timers in AC main thread */
         XpanLmpManager::StartSupervisionTimer(lmp);
         break;

    case XPAN_LMP_REMOTE_PING_REQ_EVT:
        lmp->SendLmpPingRes();
        break;

    /* ACL data before l2cap pause-unpause was done */
    case XPAN_LMP_ACL_DATA_EVT: {
         std::vector<uint8_t> data(msg->data.data,
                                  (msg->data.data + msg->data.len));
         qhci->DataReceivedCb(mBdAddr, msg->data.llid, data);
        } break;

    case XPAN_WIFI_AP_DISCONNECTED:
    case XPAN_REMOTE_DISCONNECTED_EVT:
    case XPAN_TCP_DISCONNECTED_EVT:
        if (mTransitionType == XPAN_LE_TO_AP_IDLE) {
          HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
          if (mEarbudRole == ROLE_PRIMARY &&
              (mBearerSwitchState == XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ ||
               mBearerSwitchState == XPAN_LMP_BEARER_SWITCH_REQ)) {
            qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
          }
        } else if (mTransitionType == XPAN_LE_TO_AP_STREAMING && mEarbudRole == ROLE_PRIMARY) {
          xm->BearerSwitchFailedRsp(mBdAddr, BEARER_SWITCH_FAILURE, mEarbudRole);
          xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
          if (mBearerSwitchState == XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ ||
              mBearerSwitchState == XPAN_LMP_BEARER_SWITCH_REQ) {
            qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
          }
        } else if (mTransitionType == XPAN_AP_TO_LE_IDLE) {
          HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
        } else if (mTransitionType == XPAN_AP_TO_LE_STREAMING) {
          if (mEarbudRole == ROLE_PRIMARY) {
            xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
          } else if (mEarbudRole == ROLE_SECONDARY) {
            if (mIsApToLeHaltedForSeb) {
               mIsApToLeHaltedForSeb = false;
               ALOGD("Resuming AP to LE transition with LMP BS Procedure", __func__);
               qhci->L2capPauseUnpauseReq(mBdAddr, PAUSE, XPAN_AP);
            }
          }
        }
    case QHCI_DISCONNECT_CONNECTION_EVT:
        setState(XPAN_DISCONNECTING, evt);
        XpanAcSmExecuteEvent(msg);
        break;

    case XPAN_PRIMARY_DISCONNECTING_EVT:
        if (mTransitionType == XPAN_LE_TO_AP_STREAMING) {
          setState(XPAN_DISCONNECTING, evt);
          XpanAcSmExecuteEvent(msg);
        }
        break;

    case QHCI_SEND_ACL_DATA_EVT:
         lmp->SendAclData(msg->aclDataParams.llid, msg->aclDataParams.len, msg->aclDataParams.data);
         break;

    case XPAN_BEARER_PREFERENCE_TIMEOUT: {
        if (mTransitionType == XPAN_LE_TO_AP_IDLE) {
          if (mBearerSwitchState == XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ ||
              mBearerSwitchState == XPAN_LMP_PREPARE_BEARER_SWITCH_REQ ||
              mBearerSwitchState == XPAN_LMP_BEARER_SWITCH_REQ) {
            lmp->SendLmpCancelBearerSwitchInd(LMP_TP_LE, LMP_TP_AP, 0xFF);
            XpanAcStateMachine *sm = mDevice->GetStateMachine(ROLE_SECONDARY);
            if (sm && mEarbudRole == ROLE_PRIMARY) {
              sm->SendLmpCancelBearerSwitchInd(LMP_TP_LE, LMP_TP_AP, 0xFF);
            }
            if (mBearerSwitchState != XPAN_LMP_PREPARE_BEARER_SWITCH_REQ) {
              qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
            }
          }
          setState(XPAN_DISCONNECTING, evt);
          XpanAcSmExecuteEvent(msg);
          HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
          xm->BearerSwitchFailedRsp(mBdAddr, BEARER_SWITCH_FAILURE, mEarbudRole);
        } else if (mTransitionType == XPAN_AP_TO_LE_IDLE) {
          if (mBearerSwitchState == XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ ||
              mBearerSwitchState == XPAN_LMP_PREPARE_BEARER_SWITCH_REQ ||
              mBearerSwitchState == XPAN_LMP_BEARER_SWITCH_REQ) {
            lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP, LMP_TP_LE, 0xFF);
            XpanAcStateMachine *sm = mDevice->GetStateMachine(ROLE_SECONDARY);
            if (sm && mEarbudRole == ROLE_PRIMARY) {
              sm->SendLmpCancelBearerSwitchInd(LMP_TP_AP, LMP_TP_LE, 0xFF);
            }
            if (mBearerSwitchState != XPAN_LMP_PREPARE_BEARER_SWITCH_REQ) {
              qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
            }
          }
          setState(XPAN_AP_ACTIVE, evt);
          HandleBearerPreferenceCmpl(mBdAddr, BT_LE, XM_AC_BEARER_PREFERENCE_REJECTED);
        }
       }
       break;

    case XM_LE_TO_AP_URGENCY_EVT: {
        ALOGD("%s %s: urgency for LE->AP transition", R, __func__);
        mLeToApUrgencySet = msg->leApUrgency.is_urgent;
        if (mLeToApUrgencySet) {
          ProgressForLeApInPeb();
        }
      } break;

    case QHCI_LE_LINK_CONNECTION_STATUS_EVT: {
        uint8_t state = msg->leLinkStatus.state;
        TransportType qhci_active_transport = msg->leLinkStatus.qhci_active_transport;
        /* If disconnected, start LE scan from Profile*/
        if (state == 0 && (mTransitionType == XPAN_AP_TO_LE_IDLE ||
            mTransitionType ==XPAN_AP_TO_LE_STREAMING)) {
          TriggerFilteredScan(this);
        } else if (state == 0 && qhci_active_transport == BT_LE &&
            (mTransitionType == XPAN_LE_TO_AP_IDLE ||
              mTransitionType ==XPAN_LE_TO_AP_STREAMING)) {
          ALOGW("%s: LE Link disconnected while it was active transport for QHCI", __func__);
          /* Note - No need to update XM about prepare audio bearer failure as QHCI sends
           *        unprepare to xm on disconnection */
          setState(XPAN_DISCONNECTING, evt);
          XpanAcSmExecuteEvent(msg);
        }
      } break;

    default:
        ALOGE("%s %s: Unanticipated Event %d", R, __func__, evt);
  }

}

void XpanAcStateMachine::XpanApActiveStateHandler(xac_handler_msg_t* msg) {
  XacEvent evt = msg->event;
  ALOGD("%s State: XPAN_AP_ACTIVE, ProcessEvent = %s", R, xpan_event_str(msg->event));

  uint8_t bearer_from, bearer_to;

  switch (evt) {
    case XM_PREPARE_BEARER_EVT:
         bearer_to = msg->prepareBearer.bearer;
         mOperation = XPAN_BEARER_SWITCH;
         if (bearer_to == XPAN_AP) {
           mTransitionType = XPAN_AP_TO_AP_ROAMING_STREAMING;
           setState(XPAN_BEARER_SWITCH_PENDING, evt);
           lmp->SendLmpPrepareBearerReq(LMP_TP_AP, LMP_TP_AP);
           newTcpTransportReady = false;
           prepBearerApRoamingDone = false;
           // Create TCP Connection secure connection
           mSocketHdlr->InitiateRoamingPrep(mSocketHdlr, mIpRoaming, mTcpPortRoaming);
         } else if (bearer_to == BT_LE) {
           mBearerSwitchState = XPAN_LMP_PREPARE_BEARER_SWITCH_REQ;
           mTransitionType = XPAN_AP_TO_LE_STREAMING;
           setState(XPAN_BEARER_SWITCH_PENDING, evt);
           lmp->SendLmpPrepareBearerReq(LMP_TP_AP, LMP_TP_LE);
         }
         break;

    case XM_BEARER_PREFERENCE_EVT:
         bearer_to = msg->prepareBearer.bearer;
         mOperation = XPAN_BEARER_SWITCH;
         if (bearer_to == XPAN_AP) {
           mTransitionType = XPAN_AP_TO_AP_ROAMING_IDLE;
           setState(XPAN_BEARER_SWITCH_PENDING, evt);
           lmp->SendLmpPrepareBearerReq(LMP_TP_AP, LMP_TP_AP);
           // Create TCP Connection secure connection
           mSocketHdlr->InitiateRoamingPrep(mSocketHdlr, mIpRoaming, mTcpPortRoaming);
         } else if (bearer_to == BT_LE) {
           mTransitionType = XPAN_AP_TO_LE_IDLE;
           mBearerPrefTimer = new XpanAcTimer("BearerPreferenceTimer",
                                               BearerPreferenceTimeout, this);
           mBearerPrefTimer->StartTimer(XPAN_AC_BEARER_PREFERENCE_TIMER);
           setState(XPAN_BEARER_SWITCH_PENDING, evt);
           lmp->SendLmpPrepareBearerReq(LMP_TP_AP, LMP_TP_LE);
         }
         break;

    case XM_INITIATE_LMP_BEARER_SWITCH_EVT:
        /* Received for Primary earbud only in this state */
        bearer_to = msg->initiateLmpBearerSwitch.bearer;
        if (bearer_to == XPAN_AP) {
          // Prepare audio bearer response (bearer preference wont come while IDLE->Streaming)
          xm->PrepareAudioBearerRsp(mBdAddr, XM_SUCCESS);
        }
        break;

    case XM_BEARER_SWITCH_IND_EVT:
         /* For streaming intiated in middle of IDLE transition */
         if (msg->bearerSwitchInd.status == XM_SUCCESS &&
                 qhci->isQhciInitiatedSeamlessPending(mBdAddr)) {
           qhci->BearerSwitchInd(mBdAddr, XPAN_AP, XM_SUCCESS);
           xm->UnPauseCompleted(XPAN_AP);
         }
         break;

    case QHCI_SEND_ACL_DATA_EVT:
         lmp->SendAclData(msg->aclDataParams.llid, msg->aclDataParams.len, msg->aclDataParams.data);
         break;

    case QHCI_GET_REMOTE_VERSION_EVT: {
         mdns_uuid_t uuid = XpanApplicationController::GetLocalMdnsUuid();
         LocalVersionInfo info = XpanApplicationController::GetLocalVersion();
         uint8_t version = info.version;
         uint16_t subversion = info.subversion;
         uint16_t companyId = info.companyId;
         lmp->SendLmpVersionReq(uuid, version, companyId, subversion);
         } break;

    case QHCI_GET_REMOTE_LE_FEATURES_EVT: {
          mdns_uuid_t uuid = XpanApplicationController::GetLocalMdnsUuid();;
          uint64_t le_feat = XpanApplicationController::GetLocalLeFeatures();
          lmp->SendLmpLeFeatureReq(uuid, le_feat);
         } break;

    case XPAN_LMP_VERSION_RES_EVT: {
          qhci->RemoteVersionRes(mBdAddr, msg->versionRsp.version,
              msg->versionRsp.company_id, msg->versionRsp.subversion);
         } break;

    case XPAN_LMP_LE_FEATURE_RES_EVT: {
          qhci->RemoteLeFeaturesRes(mBdAddr, XPAN_AC_SUCCESS, msg->featRsp.le_features);
         } break;

    case XPAN_LMP_SWITCH_PRIMARY_EVT: {
         /* on current primary*/
         if (msg->primarySwitchReq.operation == PRIMARY_SWITCH_START) {
           mOperation = XPAN_ROLE_SWITCH;
           // Accept response sent once QHCI paused the traffic
         } else if (msg->primarySwitchReq.operation == PRIMARY_SWITCH_COMPLETE) {
           lmp->SendLmpAccepted();
           // Primary switch callback to XM
           if (qhci && xm && qhci->isStreamingActive(mBdAddr)) {
            xm->RoleSwitchInd(mMacAddr);
           }
           // Update log tag
           int slen = 12;
           const char *s = "[Primary-EB]";
           memset(R, 0, sizeof(R));
           memcpy(R, s, slen);
           XpanAcStateMachine *ssm = mDevice->GetStateMachine(ROLE_SECONDARY);
           if (ssm) {
             int slen1 = 14;
             const char *s1 = "[Secondary-EB]";
             memcpy(ssm->R, s1, slen1);
           }
           ALOGD("%s Role switch completed", __func__);
         } else if (msg->primarySwitchReq.operation == PRIMARY_SWITCH_CANCEL) {
           if (msg->primarySwitchReq.reject) {
             ALOGD("%s: Rejecting Primary Switch Operation", __func__);
             lmp->SendLmpNotAccepted(0xFF);
           } else {
             lmp->SendLmpAccepted();
           }
         }
        } break;

    case QHCI_L2CAP_PAUSE_UNPAUSE_RES_EVT:
         /* QHCI response for pause/unpause - for role switch in this state */
         HandleQhciPauseUnpauseEvt(msg);
         break;

    case XPAN_RESET_SUPERVISION_EVT:
         /* resetting timers in AC main thread */
         XpanLmpManager::StartSupervisionTimer(lmp);
         break;

    case XPAN_LMP_REMOTE_PING_REQ_EVT:
        lmp->SendLmpPingRes();
        break;

    case QHCI_CREATE_CONNECTION_CANCEL_EVT: {
          msg->event = QHCI_DISCONNECT_CONNECTION_EVT;
          setState(XPAN_DISCONNECTING, evt);
          XpanAcSmExecuteEvent(msg);
        } break;

    case XPAN_WIFI_AP_DISCONNECTED:
        /* Note - event refactored to remote disconnected so that qhci get to know
           disconnection complete instead of connection complete.
           Fallthrough - intentional  */
        msg->event = XPAN_REMOTE_DISCONNECTED_EVT;
    case XPAN_PRIMARY_DISCONNECTING_EVT:
    case XPAN_REMOTE_DISCONNECTED_EVT:
    case XPAN_TCP_DISCONNECTED_EVT:
    case QHCI_DISCONNECT_CONNECTION_EVT:
        setState(XPAN_DISCONNECTING, evt);
        XpanAcSmExecuteEvent(msg);
        break;

    case XPAN_LMP_ACL_DATA_EVT: {
         if (mEarbudRole == ROLE_PRIMARY) {
           std::vector<uint8_t> data(msg->data.data,
                                    (msg->data.data + msg->data.len));
           qhci->DataReceivedCb(mBdAddr, msg->data.llid, data);
         } else {
           ALOGE("%s %s: ACL Data received on Secondary Earbud. Ignore", R, __func__);
         }
        } break;

    case XPAN_LMP_VERSION_REQ_EVT: {
          LocalVersionInfo info = XpanApplicationController::GetLocalVersion();
          mdns_uuid_t uuid = XpanApplicationController::GetLocalMdnsUuid();
          lmp->SendLmpVersionRes(uuid, info.version, info.companyId,
                                         info.subversion);
        } break;

    case XPAN_LMP_LE_FEATURE_REQ_EVT: {
          uint64_t le_feat = XpanApplicationController::GetLocalLeFeatures();
          mdns_uuid_t uuid = XpanApplicationController::GetLocalMdnsUuid();
          lmp->SendLmpLeFeatureRes(uuid, le_feat);
        } break;

    case QHCI_LE_LINK_CONNECTION_STATUS_EVT: {
          uint8_t state = msg->leLinkStatus.state;
          TransportType qhci_active_transport = msg->leLinkStatus.qhci_active_transport;
          /* If disconnected, start LE scan from Profile*/
          if (state == 0 && qhci_active_transport == XPAN_AP) {
            TriggerFilteredScan(this);
          } else if (state == 0 && qhci_active_transport == BT_LE) {
            ALOGW("%s: LE Link disconnected while it was active transport for QHCI", __func__);
            setState(XPAN_DISCONNECTING, evt);
            XpanAcSmExecuteEvent(msg);
          }
        } break;

    case QHCI_STREAM_STATUS_UPDATE_EVT: {
          if (msg->streamStatus.status == STREAM_BAP_TIMEOUT) {
            mDevice->ScheduleExplicitConnection();
          }
        } break;

    default:
        ALOGE("%s %s: Unanticipated Event %d", R, __func__, evt);
  }

}

void XpanAcStateMachine::XpanDisconectingStateHandler(xac_handler_msg_t* msg) {
  XacEvent evt = msg->event;
  ALOGD("%s State: XPAN_DISCONNECTING, ProcessEvent = %s", R, xpan_event_str(msg->event));
  mApDetailsOkToSend = false;

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return;
  }

  switch (evt) {
    case XPAN_BEARER_PREFERENCE_TIMEOUT:
    case XPAN_WIFI_AP_DISCONNECTED:
    case XPAN_PRIMARY_DISCONNECTING_EVT:
    case QHCI_STREAM_STATUS_UPDATE_EVT: /* only for audio server crash*/
    case QHCI_CREATE_CONNECTION_CANCEL_EVT:
        mOutTcpConnectInProgress = false;
        HandleSocketClosure();
        XpanSocketHandler::CloseListeningSocket(mBdAddr);
        setState(XPAN_IDLE, XPAN_TCP_DISCONNECTED_EVT);
        if (mDevice) {
          mDevice->HandleEarbudDisconnectedEvt(mEarbudRole);
        }
        if (mTransitionType == XPAN_LE_TO_AP_IDLE && mBearerPrefTimer) {
          stopTimer(mBearerPrefTimer);
          mBearerPrefTimer = NULL;
        }
        if (qhci && mEarbudRole == ROLE_PRIMARY) {
          qhci->ConnectionCompleteRes(mBdAddr, UNKNOWN_CONNECTION_IDENTIFIER);
        }
        Cleanup();
        UpdatePrimaryDisconnToSec();
        break;

    case XPAN_LMP_CONNECTION_RES_EVT:
        HandleSocketClosure();
        if (qhci && mEarbudRole == ROLE_PRIMARY &&
            mTransitionType == XPAN_RECONNECTION_OUTGOING) {
          qhci->ConnectionCompleteRes(mBdAddr, UNKNOWN_CONNECTION_IDENTIFIER);
        }
        setState(XPAN_IDLE, XPAN_TCP_DISCONNECTED_EVT);
        Cleanup();
        UpdatePrimaryDisconnToSec();
        break;

    case QHCI_DISCONNECT_CONNECTION_EVT:
        HandleSocketClosure();
        if (xm && mEarbudRole == ROLE_PRIMARY) xm->OnCurrentTransportUpdated(mBdAddr, NONE);
        setState(XPAN_IDLE, XPAN_TCP_DISCONNECTED_EVT);
        if (mDevice) {
          mDevice->HandleEarbudDisconnectedEvt(mEarbudRole);
        }
        if (qhci && mEarbudRole == ROLE_PRIMARY) {
          qhci->DisconnectionCompleteRes(mBdAddr, LOCAL_USER_TERMINATED_CONNECION);
        }
        Cleanup();
        UpdatePrimaryDisconnToSec();
        break;

    /* Note - QHCI_LE_LINK_CONNECTION_STATUS_EVT needs cleanup only during LE->AP */
    case QHCI_LE_LINK_CONNECTION_STATUS_EVT: {
          if (mTransitionType != XPAN_LE_TO_AP_IDLE &&
              mTransitionType != XPAN_LE_TO_AP_STREAMING) {
            break;
          }
          /* intentional fallthrough */
        }
    case XPAN_TCP_DISCONNECTED_EVT:
    case XPAN_REMOTE_DISCONNECTED_EVT:
        HandleSocketClosure();
        if (xm && mEarbudRole == ROLE_PRIMARY) xm->OnCurrentTransportUpdated(mBdAddr, NONE);
        if (mDevice) {
          mDevice->HandleEarbudDisconnectedEvt(mEarbudRole);
        }
        if (qhci && mEarbudRole == ROLE_PRIMARY &&
            mPrevState >= XPAN_LMP_CONNECTED) {
          qhci->DisconnectionCompleteRes(mBdAddr, XPAN_CONNECTION_TIMEOUT);
        }
        setState(XPAN_IDLE, XPAN_TCP_DISCONNECTED_EVT);
        Cleanup();
        UpdatePrimaryDisconnToSec();
        break;

    case XM_BEARER_SWITCH_IND_EVT:
    case XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT:
    case XPAN_LMP_PREPARE_BEARER_RES_EVT:
    case XPAN_LMP_BEARER_SWITCH_RES_EVT: {
        HandleSocketClosure();
        if (xm && mEarbudRole == ROLE_PRIMARY) xm->OnCurrentTransportUpdated(mBdAddr, NONE);
        setState(XPAN_IDLE, XPAN_TCP_DISCONNECTED_EVT);
        if (qhci && mEarbudRole == ROLE_PRIMARY) {
          qhci->DisconnectionCompleteRes(mBdAddr, XPAN_CONNECTION_TIMEOUT);
        }
        Cleanup();

        //  used for State machine object removal (this will be anyway ignored in IDLE state)
        XmRemoteDisconnectedEvent *new_msg = (XmRemoteDisconnectedEvent *)malloc(sizeof(XmRemoteDisconnectedEvent));
        if (new_msg == NULL) {
          ALOGE("%s: failed to allocate memory", __func__);
          return;
        }
        new_msg->event = XPAN_REMOTE_DISCONNECTED_EVT;
        new_msg->addr = mBdAddr;
        new_msg->role = mEarbudRole;
        xac->PostMessage((xac_handler_msg_t *)new_msg, MSG_PRIORITY_DEFAULT);
        UpdatePrimaryDisconnToSec();
        } break;

    case XPAN_TARGETED_ADV_CONN_TIMEOUT: {
         HandleSocketClosure();
         setState(XPAN_IDLE, XPAN_TCP_DISCONNECTED_EVT);
         Cleanup();
        } break;

    default:
      ALOGW("%s %s: Disconnection for Unknown event(%d)", R, __func__, (uint8_t)evt);
  }
}

XacSmState XpanAcStateMachine::GetState() {
  return mCurrentState;
}

void XpanAcStateMachine::setState(XacSmState newState, XacEvent event) {
  ALOGD("%s %s: State Transition %s -> %s, Event %s", R, __func__, xpan_state_str(mCurrentState),
        xpan_state_str(newState), xpan_event_str(event));
  mPrevState = mCurrentState;
  mCurrentState = newState;

  UpdateQhciAboutFailedIdleTransition();
}

void XpanAcStateMachine::SetEarbudProperties(ipaddr_t ip, macaddr_t mac,
    uint32_t audio_loc, uint32_t tcp_port) {
  mIpAddr = ip;
  mMacAddr = mac;
  mAudioLoc = audio_loc;
  mTcpPort = tcp_port;
  lmp->SetIpAddress(mIpAddr);
}

void XpanAcStateMachine::SetRoamingDetails(ipaddr_t ip, macaddr_t mac,
                                               uint32_t tcp_port) {
  mIpRoaming = ip;
  mMacRoaming = mac;
  mTcpPortRoaming = tcp_port;
}

void XpanAcStateMachine::SetSocketFd(int fd) {
  mSocketFd = fd;
}

XpanLmpManager* XpanAcStateMachine::GetLmpManager() {
  return lmp;
}

XpanSocketHandler* XpanAcStateMachine::GetSocketHandler() {
  return mSocketHdlr;
}

void XpanAcStateMachine::UpdateEarbudRole(XpanEarbudRole role) {
  mEarbudRole = role;
}

void XpanAcStateMachine::SetTrigger(XpanSwitchTrigger trigger) {
  mTrigger = trigger;
}

void XpanAcStateMachine::SetIpAddress(ipaddr_t ip) {
  mIpAddr = ip;
}

void XpanAcStateMachine::HandleRemoteApDetailsUpdate(xac_handler_msg_t* msg) {
  /* Initiate TCP Connection irrespective of ROLE & trigger
   * and handle next steps after TCP_CONNECTED_EVT based on trigger */

  if (mOutTcpConnectInProgress) {
    ALOGE("%s: Connection already in Progress. Ignore this event", __func__);
    return;
  }

  if (mCurrentState == XPAN_TCP_CONNECTING /*&&
      mTransitionType == XPAN_RECONNECTION_OUTGOING*/) {
    ALOGD("%s %s: Initiate Outgoing TCP Connection", R, __func__);
    mSocketHdlr->InitTcpConnection(mSocketHdlr, mIpAddr, mTcpPort);
    mOutTcpConnectInProgress = true;
  }
}

void XpanAcStateMachine::TriggerFilteredScan(void *data) {
  XpanAcStateMachine *sm = (XpanAcStateMachine *)data;

  if (!sm) {
    ALOGE("%s: Invalid state machine instance", __func__);
    return;
  }

  XMXacIf* xm = XMXacIf::GetIf();
  if (!xm) {
    ALOGE("%s: XM is not initialized", __func__);
    return;
  }

  // start filtered scan in XP for AP->LE transition
  xm->StartFilteredScan(sm->GetAddr());
}

void XpanAcStateMachine::HandleMdnsQueryTimeout(void *data) {
  ALOGD("%s: Mdns Query Timeout. Stop MDNS Query and Wait for Incoming connection.", __func__);

  XpanAcStateMachine* sm = (XpanAcStateMachine *)data;
  if (!sm) {
    ALOGE("%s: Invalid AC Instance", __func__);
    return;
  }

  bdaddr_t addr = sm->GetAddr();
  ALOGD("%s: Device %s", __func__, ConvertRawBdaddress(addr));

  XMXacIf* xm = XMXacIf::GetIf();
  if (!xm) {
    ALOGE("%s: XM is not initialized", __func__);
    return;
  }

  // stop MDNS Query at Xpan Profile
  xm->TriggerMdnsQuery(addr, MDNS_QUERY_STOP);

  // Set now that the transition type is XPAN_RECONNECTION_INCOMING
  // i.e. Handset will wait for incoming connection
  if (XpanSocketHandler::IsListeningOnTcp()) {
    sm->SetTransitionType(XPAN_RECONNECTION_INCOMING);
  }
}

bool XpanAcStateMachine::CanProgressForLeApInPeb() {
  if (mEarbudRole != ROLE_PRIMARY) {
    ALOGW("%s: Intended from PEB alone", __func__);
    return false;
  }

  XpanAcStateMachine *ssm = mDevice->GetStateMachine(ROLE_SECONDARY);
  if (!ssm || (ssm->GetState() == XPAN_IDLE)) {
    ALOGD("%s: SEB SM not available. Progress with primary", __func__);
    return true;
  }

  ALOGW("%s: mLeToApUrgencySet = %d mProceedPebForLeAp = %d", __func__,
       mLeToApUrgencySet, mProceedPebForLeAp);

  return (mLeToApUrgencySet || mProceedPebForLeAp);
}

void XpanAcStateMachine::ProgressForLeApInPeb() {
  XpanAcStateMachine *psm = mDevice->GetStateMachine(ROLE_PRIMARY);
  if (!psm) {
    ALOGW("%s: PSM not available", __func__);
    return;
  }

  /* For Indication to progress from secondary */
  if (mEarbudRole == ROLE_SECONDARY) {
    psm->mProceedPebForLeAp = true;
  }

  if (psm->GetState() != XPAN_BEARER_SWITCH_PENDING) {
    ALOGW("%s: PSM not in Bearer switch pending state", __func__);
    return;
  }

  if (psm->mBearerSwitchState != XPAN_LMP_PREPARE_BEARER_SWITCH_REQ ||
      !psm->mIsLeToApHaltedForSeb) {
    ALOGW("%s: mBearerSwitchState = %s mIsLeToApHaltedForSeb = %d m", __func__,
          xpan_lmpop_str((XpanLmpOperation)psm->mBearerSwitchState), psm->mIsLeToApHaltedForSeb);
    return;
  }

  ALOGW("%s: mIsLeToApHaltedForSeb = %d mProceedPebForLeAp = %d "
        "mLeToApUrgencySet = %d", __func__, psm->mIsLeToApHaltedForSeb,
        psm->mProceedPebForLeAp, psm->mLeToApUrgencySet);

  /* Step 1: Send remote params indication to CP*/
  if (mTransitionType == XPAN_LE_TO_AP_STREAMING) {
    /* Secondary is done with prepare bearer request later */
    if (mEarbudRole == ROLE_SECONDARY) {
      if (mCurrentState == XPAN_BEARER_SWITCH_PENDING) {
        mDevice->UpdateEarbudConnectionStatusToCp(ROLE_SECONDARY, XPAN_EB_CONNECTED);
      } else if (mCurrentState == XPAN_DISCONNECTING || mCurrentState == XPAN_IDLE) {
        mDevice->UpdateEarbudConnectionStatusToCp(ROLE_PRIMARY, XPAN_EB_CONNECTED);
      }

    /* Urgency indication for LE->AP from earbud - Only primary is done*/
    } else if (mEarbudRole == ROLE_PRIMARY && psm->mIsLeToApHaltedForSeb) {
      mDevice->UpdateEarbudConnectionStatusToCp(ROLE_PRIMARY, XPAN_EB_CONNECTED);
    }
  }

  psm->mIsLeToApHaltedForSeb = false;
  psm->mLeToApUrgencySet = false;
  psm->mProceedPebForLeAp = false;

  /* Step 2: triger LMP Bearer switch procedure*/
  XpanApplicationController *xac = XpanApplicationController::Get();
    if (xac) {
      XmInitiateLmpBearerSwitch *initEvent =
          (XmInitiateLmpBearerSwitch *)malloc(sizeof(XmInitiateLmpBearerSwitch));

    if (initEvent == NULL) {
      ALOGE("%s: failed to allocate memory", __func__);
      return;
    }

   initEvent->event = XM_INITIATE_LMP_BEARER_SWITCH_EVT;
   initEvent->addr = mBdAddr;
   initEvent->bearer = XPAN_AP;
   xac->PostMessage((xac_handler_msg_t *)initEvent, false);
 }
}

void XpanAcStateMachine::BearerPreferenceTimeout(void *data) {
  ALOGD("%s: Bearer Preference procedure timeout.", __func__);

  XpanAcStateMachine* sm = (XpanAcStateMachine *)data;
  if (!sm) {
    ALOGE("%s: Invalid AC Instance", __func__);
    return;
  }

  XpanBearerPrefTimeout *msg =
        (XpanBearerPrefTimeout *) malloc(sizeof(XpanBearerPrefTimeout));
  if (msg == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return;
  }

  msg->event = (XacEvent)XPAN_BEARER_PREFERENCE_TIMEOUT;
  msg->addr = sm->GetAddr();
  msg->role = sm->GetRole();

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free(msg);
    return;
  }

  xac->PostMessage((xac_handler_msg_t *)msg, MSG_PRIORITY_DEFAULT);
}

void XpanAcStateMachine::TargetedConnectionTimeout(void *data) {
  ALOGD("%s: Targeted Adv Connection Initiation timeout.", __func__);

  XpanAcStateMachine* sm = (XpanAcStateMachine *)data;
  if (!sm) {
    ALOGE("%s: Invalid AC Instance", __func__);
    return;
  }

  XpanTargetedConnTimeout *msg =
        (XpanTargetedConnTimeout *) malloc(sizeof(XpanTargetedConnTimeout));
  if (msg == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    return;
  }

  msg->event = (XacEvent)XPAN_TARGETED_ADV_CONN_TIMEOUT;
  msg->addr = sm->GetAddr();
  msg->role = sm->GetRole();

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    free(msg);
    return;
  }

  xac->PostMessage((xac_handler_msg_t *)msg, MSG_PRIORITY_DEFAULT);
}


void XpanAcStateMachine::SetTransitionType(XpanTransitionType transition) {
  mTransitionType = transition;
}

bool XpanAcStateMachine::HandleInitReconnection(bool is_bg) {
  ALOGD("%s %s: %s is_bg = %d", R, __func__, ConvertRawBdaddress(mBdAddr), is_bg);
  bool queryTriggered = false, registrationDone = false, listening = false;

  /* Steps:
    1. Trigger MDNS Query.
       Start 30 sec timer.
    2. Start Listening on TCP port.
    3. Register MDNS Service.
 */

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s %s: Unexpected Error. AC instance not found", R, __func__);
    return false;
  }

  ipaddr_t local_ip = xac->GetLocalIpAddr();
  if (local_ip.isEmpty()) {
    ALOGE("%s %s: Local IP address is not set. Probably HS is not connected to any AP",
          R, __func__);
    /*Note: No response to QHCI to prevent create connection and failure response loop
     for backgound connection */
    //qhci->ConnectionCompleteRes(mBdAddr, XPAN_TCP_CONNECTION_FAILED);
    return false;
  }

  if (!is_bg) { queryTriggered = GetRemoteApDetails();}
  if (!queryTriggered && !is_bg) {
    queryTriggered = xm->TriggerMdnsQuery(mBdAddr, true);
    mDnsQueryTimer = new XpanAcTimer("MDnsQueryTimer", HandleMdnsQueryTimeout, this);
    mDnsQueryTimer->StartTimer(XPAN_AC_MDNS_QUERY_TIMEOUT);
  }

  ALOGD("%s %s: Starting to Listen for incoming connection for HS IP (%s)",
        R, __func__, local_ip.toString().c_str());
  listening = XpanSocketHandler::CreateTcpSocketForIncomingConnection(local_ip, mBdAddr);

  // Send Port details callback to Xpan Profile
  XMXacIf *xm = XMXacIf::GetIf();
  if (xm && listening) {
    xm->HandSetPortNumberRsp(XpanSocketHandler::GetTcpListeningPort(),
                             XpanSocketHandler::GetUdpPort(),
                             XpanSocketHandler::GetUdpTsfPort());
  }

  if (xm && listening) {
    ALOGD("%s %s: Register HS MDNS Service for remote to discover (listening = %d)",
          R, __func__, listening);
    registrationDone = xm->RegisterMdnsService(mBdAddr, MDNS_REGISTER);
  }

  return (registrationDone || queryTriggered);
}

void XpanAcStateMachine::HandleXpanLmpConnected() {
  ALOGD("%s %s: TransitionType = %d", R, __func__, mTransitionType);

  switch (mTransitionType) {
    case XPAN_RECONNECTION_OUTGOING:
    case XPAN_RECONNECTION_INCOMING: {
          if (isPingEnabled) {
            lmp->setSupervisionTimeout(XPAN_TCP_SUPERVISION_TIMEOUT, XPAN_PERIODIC_PING_TIMER);
            XpanLmpManager::StartSupervisionTimer(lmp);
            lmp->SendLmpLstoInd();
          }
          mApDetailsOkToSend = true;
          // Callback to Xpan Profile suggesting BT is connected over AP
          if (mEarbudRole == ROLE_PRIMARY && xm) {
            xm->OnCurrentTransportUpdated(mBdAddr, XPAN_AP);
            SetFilteredScanTimer();
            xm->RegisterMdnsService(mBdAddr, MDNS_UNREGISTER);
          }
          setState(XPAN_AP_ACTIVE, XPAN_LMP_CONNECTION_RES_EVT);
          if (mEarbudRole == ROLE_SECONDARY && qhci->isStreamingActive(mBdAddr)) {
            mDevice->UpdateEarbudConnectionStatusToCp(ROLE_SECONDARY, XPAN_EB_CONNECTED);
            lmp->setSupervisionTimeout(XPAN_TCP_SUPERVISION_TIMEOUT, XPAN_PERIODIC_PING_TIMER);
          }
        } break;

    case XPAN_LE_TO_AP_IDLE: {
          mBearerSwitchState = XPAN_LMP_PREPARE_BEARER_SWITCH_REQ;
          lmp->SendLmpPrepareBearerReq(LMP_TP_LE, LMP_TP_AP);
          setState(XPAN_BEARER_SWITCH_PENDING, XM_BEARER_PREFERENCE_EVT);
        } break;

    case XPAN_LE_TO_AP_STREAMING: {
          mBearerSwitchState = XPAN_LMP_PREPARE_BEARER_SWITCH_REQ;
          lmp->SendLmpPrepareBearerReq(LMP_TP_LE, LMP_TP_AP);
          setState(XPAN_BEARER_SWITCH_PENDING, XM_PREPARE_BEARER_EVT);
        } break;

    default:
        ALOGD("%s %s: LMP Connected in Unexpected Transition", R, __func__);
  }
}

void XpanAcStateMachine::HandleLmpPrepareBearerRspEvt(xac_handler_msg_t* msg) {
  uint8_t status = msg->prepareBearerRsp.status;
  XacEvent evt = msg->prepareBearerRsp.event;
  XpanAcStateMachine *ssm = NULL;

  ALOGD("%s %s: status = %d Transition type = %d", R, __func__, status, mTransitionType);

  /* Prepare Bearer failed case */
  if (status != XPAN_AC_SUCCESS) {
    switch (mTransitionType) {
      case XPAN_LE_TO_AP_IDLE:
      case XPAN_LE_TO_AP_STREAMING:
           /* Change state to XPAN_DISCONNECTING. Since remote rejected the procedure,
              entire procedure is needed to be restarted as needed */
           setState(XPAN_DISCONNECTING, evt);
           if (mEarbudRole == ROLE_PRIMARY) {
             XpanApplicationController *xac = XpanApplicationController::Get();
             XpanDevice* dev = xac ? xac->GetDevice(mBdAddr): NULL;
             if (dev) {
               ssm = dev->GetStateMachine(ROLE_SECONDARY);
               if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_LE,
                            LMP_TP_AP, evt);
             }
             if (mTransitionType == XPAN_LE_TO_AP_STREAMING)
               xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
             else {
              HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
             }
           } else if (mEarbudRole == ROLE_SECONDARY) {
             ALOGE("%s: SEB rejected LMP Prepare bearer. Cancel transition with PEB",
                   __func__);
             XpanApplicationController *xac = XpanApplicationController::Get();
             if (xac) {
               XmRemoteDisconnectedEvent *new_msg = (XmRemoteDisconnectedEvent *)
                   malloc(sizeof(XmRemoteDisconnectedEvent));
               if (new_msg == NULL) {
                 ALOGE("%s: failed to allocate memory", __func__);
                 return;
               }
               new_msg->event = XPAN_REMOTE_DISCONNECTED_EVT;
               new_msg->addr = mBdAddr;
               new_msg->role = ROLE_PRIMARY;
               xac->PostMessage((xac_handler_msg_t *)new_msg, MSG_PRIORITY_DEFAULT);
             }
           }
           XpanAcSmExecuteEvent(msg);
           break;

      case XPAN_AP_TO_LE_IDLE:
      case XPAN_AP_TO_LE_STREAMING:
      case XPAN_AP_TO_AP_ROAMING_IDLE:
      case XPAN_AP_TO_AP_ROAMING_STREAMING:
           /* keep state as XPAN AP */
           setState(XPAN_AP_ACTIVE, evt);
           if (mEarbudRole == ROLE_PRIMARY) {
             XpanApplicationController *xac = XpanApplicationController::Get();
             XpanDevice* dev = xac ? xac->GetDevice(mBdAddr): NULL;
             if (dev) {
               ssm = dev->GetStateMachine(ROLE_SECONDARY);
             }
             if (mTransitionType != XPAN_AP_TO_AP_ROAMING_IDLE &&
                 mTransitionType != XPAN_AP_TO_AP_ROAMING_STREAMING) {
               if (ssm) ssm->SendLmpCancelBearerSwitchInd(
                             LMP_TP_AP, LMP_TP_LE, 0xFF);
             } else {
               if (ssm) ssm->SendLmpCancelBearerSwitchInd(
                             LMP_TP_AP, LMP_TP_AP, 0xFF);
             }
             if (mTransitionType == XPAN_AP_TO_LE_STREAMING ||
                 mTransitionType == XPAN_AP_TO_AP_ROAMING_STREAMING) {
               xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
             } else {
              HandleBearerPreferenceCmpl(mBdAddr, BT_LE, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
             }
           } else if (mEarbudRole == ROLE_SECONDARY) {
             XpanAcStateMachine *psm = mDevice->GetStateMachine(ROLE_PRIMARY);
             if (psm && psm->GetState() == XPAN_BEARER_SWITCH_PENDING) {
               psm->setState(XPAN_AP_ACTIVE, evt);
               XpanLmpManager *p_lmp = psm->GetLmpManager();
               if (p_lmp) {
                 p_lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP, LMP_TP_LE, 0xFF);
               }
             }
           }
           break;

      default:
        ALOGE("%s %s: Unexpected Transition", R, __func__);
        // disconnect connection
        setState(XPAN_DISCONNECTING, evt);
    }
  } else {
    switch (mTransitionType) {
      case XPAN_LE_TO_AP_IDLE:
           /* Proceed with L2cap Pause Unpause. Send Pause to QHCI first and wait for CB */
           if (mEarbudRole == ROLE_PRIMARY) {
             if (CanProgressForLeApInPeb()) {
               mIsLeToApHaltedForSeb = false;
               mLeToApUrgencySet = false;
               mProceedPebForLeAp = false;
               ALOGD("%s: Start LMP Bearer Switch procedure", __func__);
               qhci->L2capPauseUnpauseReq(mBdAddr, PAUSE, BT_LE);
             } else {
               mIsLeToApHaltedForSeb = true;
               ALOGW("%s: Wait for SEB to prepare bearer (IDLE)", __func__);
             }
           } else if (mEarbudRole == ROLE_SECONDARY) {
             ProgressForLeApInPeb();
             mApDetailsOkToSend = true;
             XpanAcStateMachine *psm = mDevice->GetStateMachine(ROLE_PRIMARY);
             if (psm && psm->GetState() == XPAN_AP_ACTIVE) {
                SendLmpBearerSwitchCompleteInd(LMP_TP_LE, LMP_TP_AP);
             }
           }
           break;

      case XPAN_LE_TO_AP_STREAMING: {
           if (mEarbudRole == ROLE_PRIMARY && !CanProgressForLeApInPeb()) {
             mApDetailsOkToSend = true;
             mIsLeToApHaltedForSeb = true;
             ALOGW("%s: Wait for SEB to prepare bearer(STREAMING)", __func__);
             break;
           }

           mApDetailsOkToSend = true;

           if (mEarbudRole == ROLE_PRIMARY) {
             mDevice->UpdateEarbudConnectionStatusToCp(mEarbudRole, XPAN_EB_CONNECTED);
             XpanApplicationController *xac = XpanApplicationController::Get();
             if (xac) {
               XmInitiateLmpBearerSwitch *initEvent =
                   (XmInitiateLmpBearerSwitch *)malloc(sizeof(XmInitiateLmpBearerSwitch));
               if (initEvent == NULL) {
                 ALOGE("%s: failed to allocate memory", __func__);
                 return;
               }
               initEvent->event = XM_INITIATE_LMP_BEARER_SWITCH_EVT;
               initEvent->addr = mBdAddr;
               initEvent->bearer = XPAN_AP;
               xac->PostMessage((xac_handler_msg_t *)initEvent, false);
             }
           }

           if (mEarbudRole == ROLE_SECONDARY) {
             mDevice->UpdateEarbudConnectionStatusToCp(mEarbudRole, XPAN_EB_CONNECTED);
             ProgressForLeApInPeb();
             XpanAcStateMachine *psm = mDevice->GetStateMachine(ROLE_PRIMARY);
             if (psm && psm->GetState() == XPAN_AP_ACTIVE) {
                SendLmpBearerSwitchCompleteInd(LMP_TP_LE, LMP_TP_AP);
             }
           }
          }break;

      case XPAN_AP_TO_LE_IDLE:
      case XPAN_AP_TO_LE_STREAMING:
           /* Proceed with L2cap Pause Unpause, wait to QHCI callback.
              Note InitiateLmpBearerSwitch is not initiated in this scenario */
           if (mEarbudRole == ROLE_PRIMARY) {
             if (IsLmpPrepareBearerReqPendingInSeb()) {
               ALOGW("%s: Wait for SEB to respond to LMP Prepare Bearer Req", __func__);
             } else {
              qhci->L2capPauseUnpauseReq(mBdAddr, PAUSE, XPAN_AP);
             }
           } else if (mEarbudRole == ROLE_SECONDARY) {
             mBearerSwitchState = -1;
             if (mIsApToLeHaltedForSeb) {
                mIsApToLeHaltedForSeb = false;
                ALOGD("Resuming AP to LE transition with LMP BS Procedure", __func__);
                qhci->L2capPauseUnpauseReq(mBdAddr, PAUSE, XPAN_AP);
             }
           }
           break;

      case XPAN_AP_TO_AP_ROAMING_IDLE:
      case XPAN_AP_TO_AP_ROAMING_STREAMING:
           prepBearerApRoamingDone = true;
           /* Proceed with L2cap Pause Unpause (idle transition) & wait to QHCI callback */
           if (newTcpTransportReady && mTransitionType == XPAN_AP_TO_AP_ROAMING_IDLE &&
               mEarbudRole == ROLE_PRIMARY) {
            qhci->L2capPauseUnpauseReq(mBdAddr, PAUSE, XPAN_AP);
           } else if (newTcpTransportReady && mTransitionType == XPAN_AP_TO_AP_ROAMING_STREAMING) {
            mApDetailsOkToSend = true;
            // callback to XM with remote AP details
            mDevice->UpdateEarbudConnectionStatusToCp(mEarbudRole, XPAN_EB_CONNECTED);
           }
           break;

      default:
        ALOGE("%s %s: Unexpected Transition", R, __func__);
        // disconnect connection
        setState(XPAN_DISCONNECTING, evt);
    }
  }
}

void XpanAcStateMachine::HandleQhciPauseUnpauseEvt(xac_handler_msg_t* msg) {
  uint8_t action = msg->l2capPauseUnpauseRes.action; // pause/unpause
  uint8_t status = msg->l2capPauseUnpauseRes.status; // accepted/rejected
  XpanAcStateMachine *ssm = NULL;

  ALOGD("%s %s: action = %d, status = %d mOp = %d", R, __func__, action, status, mOperation);

  if (mOperation == XPAN_ROLE_SWITCH) {
    HandleXpanPrimarySwitchReq(msg, action);
    return;
  }

  /* If this is for unpause callback for Bearer Switch Cancel procedure, ignore the callback */
  if (action == UNPAUSE && mOperation == XPAN_BEARER_SWITCH_CANCEL) {
    //nothing to do, Ignore
    mOperation = XPAN_OP_NONE;
    return;
  }

  if (status == XPAN_AC_SUCCESS) {
    switch (mTransitionType) {
      case XPAN_LE_TO_AP_IDLE:
      case XPAN_LE_TO_AP_STREAMING: {
             /* Send L2CAP Pause/Unpause */
             if (action == PAUSE) {
               /* mOperation is set here for LE->AP IDLE. For Streaming case, its already set
                 thus overriden */
               mOperation = XPAN_BEARER_SWITCH;
               mBearerSwitchState = XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ;
               lmp->SendLmpL2capPauseUnpauseReq(PAUSE, LMP_TP_LE);
             } else if (action == UNPAUSE) {
               // nothing to do
             }
           } break;

      case XPAN_AP_TO_LE_IDLE:
      case XPAN_AP_TO_LE_STREAMING:
      case XPAN_AP_TO_AP_ROAMING_IDLE:
      case XPAN_AP_TO_AP_ROAMING_STREAMING:
           /* Send L2CAP Pause/Unpause */
           if (action == PAUSE) {
             mBearerSwitchState = XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ;
             lmp->SendLmpL2capPauseUnpauseReq(PAUSE, LMP_TP_AP);
           } else if (action == UNPAUSE) {
             // nothing to do
           }
           break;

      default:
        ALOGE("%s %s: Unexpected Transition", R, __func__);
    }
  } else {
    /* QHCI rejected pause-unpause */
    switch (mTransitionType) {
      case XPAN_LE_TO_AP_IDLE:
      case XPAN_LE_TO_AP_STREAMING:
           /*TODO: check its bearer switch procedure before sending cancel ind */
           /* Cancel bearer switch and send indication */
           mOperation = XPAN_BEARER_SWITCH_CANCEL;
           lmp->SendLmpCancelBearerSwitchInd(LMP_TP_LE, LMP_TP_AP, 0xFF);
           if (mEarbudRole == ROLE_PRIMARY) {
             XpanApplicationController *xac = XpanApplicationController::Get();
             XpanDevice* dev = xac ? xac->GetDevice(mBdAddr): NULL;
             if (dev) {
               ssm = dev->GetStateMachine(ROLE_SECONDARY);
             }
             if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_LE, LMP_TP_AP, 0xFF);
           }
           if (action == UNPAUSE) {
             // not a desired scenario. QHCI never rejects
             qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
           }
           break;

      case XPAN_AP_TO_LE_IDLE:
      case XPAN_AP_TO_LE_STREAMING:
           /*TODO: check its bearer switch procedure before sending cancel ind */
           lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP, LMP_TP_LE, 0xFF);
           if (mEarbudRole == ROLE_PRIMARY) {
             XpanApplicationController *xac = XpanApplicationController::Get();
             XpanDevice* dev = xac ? xac->GetDevice(mBdAddr): NULL;
             if (dev) {
               ssm = dev->GetStateMachine(ROLE_SECONDARY);
             }
             if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                        LMP_TP_LE, 0xFF);
           }
           if (action == UNPAUSE) {
             // not a desired scenario. QHCI never rejects
             qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
           }
           break;

      case XPAN_AP_TO_AP_ROAMING_IDLE:
      case XPAN_AP_TO_AP_ROAMING_STREAMING:
           /*TODO: check its bearer switch procedure before sending cancel ind */
           /* Proceed with L2cap Pause Unpause, wait to QHCI callback */
           lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                  LMP_TP_AP, 0xFF);
           if (mEarbudRole == ROLE_PRIMARY) {
             XpanApplicationController *xac = XpanApplicationController::Get();
             XpanDevice* dev = xac ? xac->GetDevice(mBdAddr): NULL;
             if (dev) {
               XpanAcStateMachine* ssm = dev->GetStateMachine(ROLE_SECONDARY);
             }
             if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                            LMP_TP_AP, 0xFF);
           }
           if (action == UNPAUSE) {
             // not a desired scenario. QHCI never rejects
             qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
           }
           break;

      default:
        ALOGE("%s %s: Unexpected Transition", R, __func__);
        // disconnect connection
        //setState(XPAN_DISCONNECTING, evt);
    }
  }
}

void XpanAcStateMachine::HandleLmpPauseUnpauseRspEvt(xac_handler_msg_t* msg) {
  uint8_t action = msg->l2cPauseUnpause.action; // pause/unpause
  uint8_t status = msg->l2cPauseUnpause.status; // accepted/rejected
  XpanAcStateMachine *ssm = NULL;
  XpanApplicationController *xac = NULL;
  XpanDevice* dev = NULL;

  xac = XpanApplicationController::Get();
  dev = xac ? xac->GetDevice(mBdAddr): NULL;
  if (dev) {
   ssm = dev->GetStateMachine(ROLE_SECONDARY);
  }

  ALOGD("%s %s: action = %d, status = %d mOp = %d", R, __func__, action, status, mOperation);

  if (status == XPAN_AC_SUCCESS) {
    switch (mTransitionType) {
      case XPAN_LE_TO_AP_IDLE:
      case XPAN_LE_TO_AP_STREAMING:
           if (action == PAUSE) {
            mBearerSwitchState = XPAN_LMP_BEARER_SWITCH_REQ;
            lmp->SendLmpBearerSwitchReq(LMP_TP_LE, LMP_TP_AP);
           } else if (action == UNPAUSE && mOperation == XPAN_BEARER_SWITCH) {
             // Send Bearer Switch Complete Ind to Secondary
             qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
             if (ssm)
               ssm->SendLmpBearerSwitchCompleteInd(LMP_TP_LE, LMP_TP_AP);
             /* Role check not required since LMP BS ops are done with primary */
             if (!qhci->isLeLinkconnected(mBdAddr)) {
               XpanAcStateMachine::TriggerFilteredScan(this);
             }
             if (mTransitionType == XPAN_LE_TO_AP_IDLE) {
               mApDetailsOkToSend = true;
               setState(XPAN_AP_ACTIVE, XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT);
               HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_SUCCESS);
               xm->OnCurrentTransportUpdated(mBdAddr, XPAN_AP);
               qhci->UpdateIdleTransitionStatus(mBdAddr, XPAN_AP, IDLE_TRANSITION_COMPLETED);
             } else if (mTransitionType == XPAN_LE_TO_AP_STREAMING) {
               // Send Message to QHCI to disconnect CIS as move to AP successful
               qhci->BearerSwitchInd(mBdAddr, XPAN_AP, XM_SUCCESS);
               setState(XPAN_AP_ACTIVE, XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT);
               xm->OnCurrentTransportUpdated(mBdAddr, XPAN_AP);
               xm->UnPauseCompleted(XPAN_AP);
             }
             mOperation = XPAN_OP_NONE;
             mBearerSwitchState = -1;
             if (isPingEnabled) {
               lmp->setSupervisionTimeout(XPAN_TCP_SUPERVISION_TIMEOUT, XPAN_PERIODIC_PING_TIMER);
               XpanLmpManager::StartSupervisionTimer(lmp);
               lmp->SendLmpLstoInd();
             }
           } else if (action == UNPAUSE && mOperation == XPAN_BEARER_SWITCH_CANCEL) {
             // Send BS cancel indication to both earbuds
             lmp->SendLmpCancelBearerSwitchInd(LMP_TP_LE, LMP_TP_AP, 0xFF);
             if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_LE, LMP_TP_AP,
                  msg->l2cPauseUnpause.event);
             //qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
             setState(XPAN_DISCONNECTING, msg->l2cPauseUnpause.event);
             XpanAcSmExecuteEvent(msg);
           }
           break;

      case XPAN_AP_TO_LE_IDLE:
      case XPAN_AP_TO_LE_STREAMING:
           if (action == PAUSE) {
            mBearerSwitchState = XPAN_LMP_BEARER_SWITCH_REQ;
            lmp->SendLmpBearerSwitchReq(LMP_TP_AP, LMP_TP_LE);
           } else if (action == UNPAUSE && mOperation == XPAN_BEARER_SWITCH) {
             // Send Bearer Switch Complete Ind to Secondary
             qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
             if (ssm)
               ssm->SendLmpBearerSwitchCompleteInd(LMP_TP_AP, LMP_TP_LE);
             if (mTransitionType == XPAN_AP_TO_LE_IDLE) {
               HandleBearerPreferenceCmpl(mBdAddr, BT_LE, XM_SUCCESS);
               setState(XPAN_DISCONNECTING, XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT);
               XpanAcSmExecuteEvent(msg);
               qhci->UpdateIdleTransitionStatus(mBdAddr, XPAN_AP, IDLE_TRANSITION_COMPLETED);
             } else if (mTransitionType == XPAN_AP_TO_LE_STREAMING) {
               // Send Message to QHCI that move to LE is successful
               qhci->BearerSwitchInd(mBdAddr, BT_LE, XM_SUCCESS);
               setState(XPAN_DISCONNECTING, XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT);
               XpanAcSmExecuteEvent(msg);
               xm->UnPauseCompleted(BT_LE);
             }
             mOperation = XPAN_OP_NONE;
             mBearerSwitchState = -1;
           } else if (action == UNPAUSE && mOperation == XPAN_BEARER_SWITCH_CANCEL) {
             // Send BS cancel indication to both earbuds
             lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP, LMP_TP_LE, 0xFF);
             if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                            LMP_TP_LE, 0xFF);
             qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
             setState(XPAN_AP_ACTIVE, XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT);
           }
           break;

       case XPAN_AP_TO_AP_ROAMING_IDLE:
       case XPAN_AP_TO_AP_ROAMING_STREAMING:
           if (action == PAUSE) {
            mBearerSwitchState = XPAN_LMP_BEARER_SWITCH_REQ;
            lmp->SendLmpBearerSwitchReq(LMP_TP_AP, LMP_TP_AP);
           } else if (action == UNPAUSE && mOperation == XPAN_BEARER_SWITCH) {
             UpdateApTransport();
             qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
             // Send Bearer Switch Complete Ind to Secondary
             if (ssm)
               ssm->SendLmpBearerSwitchCompleteInd(LMP_TP_AP, LMP_TP_AP);
             if (mTransitionType == XPAN_AP_TO_AP_ROAMING_IDLE) {
                setState(XPAN_AP_ACTIVE, XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT);
                HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_SUCCESS);
             } else if (mTransitionType == XPAN_AP_TO_AP_ROAMING_STREAMING) {
                setState(XPAN_AP_ACTIVE, XM_BEARER_SWITCH_IND_EVT);
                // prepare bearer switch rsp callback to XM sent already
             }
             mOperation = XPAN_OP_NONE;
             mBearerSwitchState = -1;
           } else if (action == UNPAUSE && mOperation == XPAN_BEARER_SWITCH_CANCEL) {
             // Send BS cancel indication to both earbuds
             lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                        LMP_TP_AP, 0xFF);
             if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                        LMP_TP_AP, 0xFF);
             qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
           }
           break;

      default:
        ALOGE("%s %s: Unexpected Transition", R, __func__);
    }
  } else {
    /* Remote rejected Lmp Pause-Unpause */
    mOperation = XPAN_BEARER_SWITCH_CANCEL;
    switch (mTransitionType) {
      case XPAN_LE_TO_AP_IDLE:
      case XPAN_LE_TO_AP_STREAMING:
           if (action == PAUSE) {
             qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
             lmp->SendLmpCancelBearerSwitchInd(LMP_TP_LE,
                                               LMP_TP_AP, 0xFF);
             if (mEarbudRole == ROLE_PRIMARY) {
               if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_LE, LMP_TP_AP,
                            msg->l2cPauseUnpause.event);
             }
             setState(XPAN_DISCONNECTING, msg->l2cPauseUnpause.event);
             if (mTransitionType == XPAN_LE_TO_AP_IDLE) {
               HandleBearerPreferenceCmpl(mBdAddr, XPAN_AP, XM_AC_BEARER_PREFERENCE_REJECTED);
             } else {
               xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
             }
             XpanAcSmExecuteEvent(msg);
           } else if (action == UNPAUSE) {
             /* note: Error in unpause -> Disconnect */
             setState(XPAN_DISCONNECTING, msg->l2cPauseUnpause.event); // primary
             XmRemoteDisconnectedEvent s_eb_disconn{XPAN_REMOTE_DISCONNECTED_EVT,
                                                    mBdAddr, ROLE_SECONDARY};
             // disconnect secondary
             if (dev) dev->RemoteDisconnectedEvent((xac_handler_msg_t *)&s_eb_disconn);
           }
           break;

      case XPAN_AP_TO_LE_IDLE:
      case XPAN_AP_TO_LE_STREAMING:
      case XPAN_AP_TO_AP_ROAMING_IDLE:
      case XPAN_AP_TO_AP_ROAMING_STREAMING:
           xac = XpanApplicationController::Get();
           dev = xac ? xac->GetDevice(mBdAddr): NULL;
           if (dev) {
             ssm = dev->GetStateMachine(ROLE_SECONDARY);
           }
           if (action == PAUSE) {
             if (mEarbudRole == ROLE_PRIMARY) {
               qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
               if (mTransitionType != XPAN_AP_TO_AP_ROAMING_IDLE &&
                   mTransitionType != XPAN_AP_TO_AP_ROAMING_STREAMING) {
                 lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                                                   LMP_TP_LE, 0xFF);
                 if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                                                            LMP_TP_LE, 0xFF);
                 if (mTransitionType == XPAN_AP_TO_LE_STREAMING) {
                   xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
                 }
               } else {
                 lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                          LMP_TP_AP, 0xFF);
                 if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                          LMP_TP_AP, 0xFF);
               }
             }
           } else if (action == UNPAUSE) {
             /* note: Error in unpause -> Disconnect */
             setState(XPAN_DISCONNECTING, msg->l2cPauseUnpause.event); // primary
             // disconnect secondary
             XmRemoteDisconnectedEvent s_eb_disconn{XPAN_REMOTE_DISCONNECTED_EVT,
                                                    mBdAddr, ROLE_SECONDARY};
             // secondary disconnect
             if (dev) dev->RemoteDisconnectedEvent((xac_handler_msg_t *)&s_eb_disconn);
           }
           break;

      default:
        ALOGE("%s %s: Unexpected Transition", R, __func__);
        // disconnect connection
        //setState(XPAN_DISCONNECTING, evt);
    }
  }
}

void XpanAcStateMachine::HandleLmpBearerSwitchRspEvt(xac_handler_msg_t* msg) {
  uint8_t status = msg->bearerSwitchRsp.status;

  if (status == XPAN_AC_SUCCESS) {
    switch (mTransitionType) {
      case XPAN_LE_TO_AP_IDLE:
      case XPAN_AP_TO_AP_ROAMING_IDLE:
           mBearerSwitchState = XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ;
           lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_AP);
           break;

      case XPAN_AP_TO_LE_IDLE:
           mBearerSwitchState = XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ;
           lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_LE);
           break;

      case XPAN_LE_TO_AP_STREAMING:
      case XPAN_AP_TO_AP_ROAMING_STREAMING:
      case XPAN_AP_TO_LE_STREAMING:
           // received only for primary so no role check required
           xm->PrepareAudioBearerRsp(mBdAddr, XM_SUCCESS);
           break;

      default:
        ALOGE("%s %s: Unexpected Transition", R, __func__);
    }
  } else {
    /* Remote rejected Bearer switch request
       Steps:
       1. Unpause previous transport (with QHCI and remote)
       2. Send Bearer Switch Cancel Indication to both buds
    */
    mOperation = XPAN_BEARER_SWITCH_CANCEL;
    switch (mTransitionType) {
      case XPAN_LE_TO_AP_IDLE:
      case XPAN_LE_TO_AP_STREAMING:
           qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
           xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
           lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_LE);
           break;

      case XPAN_AP_TO_LE_IDLE:
      case XPAN_AP_TO_LE_STREAMING:
      case XPAN_AP_TO_AP_ROAMING_IDLE:
      case XPAN_AP_TO_AP_ROAMING_STREAMING:
           qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
           xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
           lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_AP);
           break;

      default:
        ALOGE("%s %s: Unexpected Transition", R, __func__);
        // disconnect connection
        //setState(XPAN_DISCONNECTING, evt);
    }
  }
}

void XpanAcStateMachine::HandleXmBearerSwitchInd(xac_handler_msg_t* msg) {
  uint8_t status = msg->bearerSwitchInd.status;

  /* Get device & ssm instance*/
  XpanAcStateMachine *ssm = NULL;
  XpanApplicationController *xac = XpanApplicationController::Get();
  XpanDevice* dev = xac ? xac->GetDevice(mBdAddr): NULL;
  if (dev) {
   ssm = dev->GetStateMachine(ROLE_SECONDARY);
  }

  if (status == XPAN_AC_SUCCESS) {
    uint8_t reason = msg->bearerSwitchInd.reason;
    /* Check if streaming stopped during LMP Bearer switch */
    if (reason == XM_KP_PORT_CLOSED && HandleStreamStopDuringTransition()) {
      return;
    }
    /* Steps:
       1. Unpause ACL traffic
       2. Send Bearer Switch Complete Ind to secondary after unapuse response
    */
    switch(mTransitionType) {
      case XPAN_LE_TO_AP_STREAMING:
           mBearerSwitchState = XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ;
           lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_AP);
           break;

      case XPAN_AP_TO_LE_STREAMING:
           mBearerSwitchState = XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ;
           lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_LE);
           //qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
           break;

      case XPAN_AP_TO_AP_ROAMING_STREAMING:
           mBearerSwitchState = XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ;
           lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_AP);
           //qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
           break;

      default:
           ALOGD("%s %s: Bearer Switch indication for undesired transport %d",
                 R, __func__, mTransitionType);
    }
  } else {
    /* XM send Bearer switch failed indication */
    /* Determine at what point bearer switch failed and execute next steps
       accordingly.
     */
    /* Check if Bearer switch is failing during UNPAUSE operation */
    if (mBearerSwitchState == XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ &&
        lmp->GetCurrentPauseUnpauseAction() == UNPAUSE) {
      ALOGD("%s: mBearerSwitchState = %d, pause = %d", __func__,
            lmp->GetCurrentPauseUnpauseAction());
      HandleUnpauseNotCompleted();
    }
    /* Set that Bearer Switch Cancel procedure is initiated*/
     mOperation = XPAN_BEARER_SWITCH_CANCEL;

     /* Unset flags used for LE to AP transition together */
     if (mTransitionType == XPAN_LE_TO_AP_IDLE ||
         mTransitionType == XPAN_LE_TO_AP_STREAMING) {
       mIsLeToApHaltedForSeb = false;
       mLeToApUrgencySet = false;
       mProceedPebForLeAp = false;
     }

     uint8_t curLmpOperation = mBearerSwitchState;
     uint8_t pauseAction = 0/*TODO: lmp->getCurrentLmpOperation()*/;
     ALOGD("%s %s: bearer switch failed ind from XM while performing operation:%d",
           R, __func__, curLmpOperation);
     switch (curLmpOperation) {
       case XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ:
       case XPAN_LMP_BEARER_SWITCH_REQ:
            /* Unpause L2CAP traffic on previous transport and then bearer switch
               cancel indication to both is sent from Lmp pause/unpause handler */
            switch(mTransitionType) {
              case XPAN_LE_TO_AP_IDLE:
              case XPAN_LE_TO_AP_STREAMING:
                   {
                    qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, BT_LE);
                    xm->BearerSwitchFailedRsp(mBdAddr, BEARER_SWITCH_FAILURE, ROLE_PRIMARY);
                    lmp->CleanLmpCommandQueue();
                    lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_LE);
                    lmp->SendLmpCancelBearerSwitchInd(LMP_TP_LE, LMP_TP_AP, 0xFF);
                    XpanAcStateMachine *sm = mDevice->GetStateMachine(ROLE_SECONDARY);
                    if (sm && mEarbudRole == ROLE_PRIMARY) {
                      sm->SendLmpCancelBearerSwitchInd(LMP_TP_LE, LMP_TP_AP, 0xFF);
                     }
                    setState(XPAN_DISCONNECTING, msg->event);
                    XpanAcSmExecuteEvent(msg);
                    break;
                   }

              case XPAN_AP_TO_LE_IDLE:
              case XPAN_AP_TO_LE_STREAMING:
              case XPAN_AP_TO_AP_ROAMING_IDLE:
              case XPAN_AP_TO_AP_ROAMING_STREAMING:
                  lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_AP);
                  break;

              default:
                   ALOGE("%s %s: Undesired transition(%d). Ignore", R, __func__, mTransitionType);
            }
            break;

       case XPAN_LMP_PREPARE_BEARER_SWITCH_REQ:
            /* Just send bearer switch Cancel Indication as L2CAP Pause/Unpause
               is not even initiated */
            switch(mTransitionType) {
              case XPAN_LE_TO_AP_IDLE:
              case XPAN_LE_TO_AP_STREAMING:
                   xm->BearerSwitchFailedRsp(mBdAddr, BEARER_SWITCH_FAILURE, ROLE_PRIMARY);
                   lmp->SendLmpCancelBearerSwitchInd(LMP_TP_LE,
                          LMP_TP_AP, 0xFF);
                   if (ssm) ssm->SendLmpCancelBearerSwitchInd(LMP_TP_LE,
                          LMP_TP_AP, 0xFF);
                   setState(XPAN_DISCONNECTING, msg->event);
                   XpanAcSmExecuteEvent(msg);
                   break;

              case XPAN_AP_TO_LE_IDLE:
              case XPAN_AP_TO_LE_STREAMING:
                  lmp->SendLmpCancelBearerSwitchInd(
                      LMP_TP_AP, LMP_TP_LE, 0xFF);
                  if (ssm) ssm->SendLmpCancelBearerSwitchInd(
                          LMP_TP_AP, LMP_TP_LE, 0xFF);
                  setState(XPAN_AP_ACTIVE, msg->event);
                  break;

              case XPAN_AP_TO_AP_ROAMING_IDLE:
              case XPAN_AP_TO_AP_ROAMING_STREAMING:
                   lmp->SendLmpCancelBearerSwitchInd(
                        LMP_TP_AP, LMP_TP_AP, 0xFF);
                   if (ssm) ssm->SendLmpCancelBearerSwitchInd(
                      LMP_TP_AP, LMP_TP_AP, 0xFF);
                   setState(XPAN_AP_ACTIVE, msg->event);
                   break;
              default:
                   ALOGE("%s %s: Undesired transition(%d). Ignore", R, __func__, mTransitionType);
            }

       default:
            ALOGD("%s %s: undesired LMP operation. Nothing to be done.", R, __func__);
     }
  }
}

void XpanAcStateMachine::HandleStreamStatusEventFromStack(xac_handler_msg_t* msg) {
  ALOGD("%s: mTransitionType = %d, status = %d", __func__, mTransitionType,
        msg->streamStatus.status);
  if (mTransitionType != XPAN_LE_TO_AP_STREAMING &&
      mTransitionType != XPAN_AP_TO_LE_STREAMING) {
    ALOGI("%s: No action needed for non streaming transition type", __func__);
    return;
  }

  uint8_t status = msg->streamStatus.status;
  if (status == STREAM_STOPPED_BY_PAUSE) {
    HandleStreamStopDuringTransition();
  } else if (status == STREAM_AUDIO_SERVER_EXCEPTION) {
    if (mTransitionType == XPAN_LE_TO_AP_STREAMING) {
      xm->BearerSwitchFailedRsp(mBdAddr, BEARER_SWITCH_FAILURE, ROLE_PRIMARY);
      xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_AUDIO_SERVER_EXCEPTION);
      setState(XPAN_DISCONNECTING, msg->event);
      XpanAcSmExecuteEvent(msg);
    } else if (mTransitionType == XPAN_AP_TO_LE_STREAMING) {
      xm->PrepareAudioBearerRsp(mBdAddr, XM_AC_PREPARE_AUDIO_BEARER_FAILED);
      SendLmpCancelBearerSwitchInd(LMP_TP_AP, LMP_TP_LE, 0xFF);
      if (mBearerSwitchState != XPAN_LMP_PREPARE_BEARER_SWITCH_REQ) {
        qhci->L2capPauseUnpauseReq(mBdAddr, UNPAUSE, XPAN_AP);
      }
      setState(XPAN_AP_ACTIVE, msg->event);
      XpanAcStateMachine *ssm = mDevice->GetStateMachine(ROLE_SECONDARY);
      if (ssm) {
        ssm->setState(XPAN_AP_ACTIVE, msg->event);
        ssm->SendLmpCancelBearerSwitchInd(LMP_TP_AP, LMP_TP_LE, 0xFF);
      }
    }
  }
}

bool XpanAcStateMachine::HandleStreamStopDuringTransition() {
  ALOGD("%s %s: mBearerSwitchState = %d mTransitionType = %d", R, __func__, mBearerSwitchState,
        mTransitionType);
  if (mTransitionType == XPAN_LE_TO_AP_IDLE || mTransitionType == XPAN_AP_TO_LE_IDLE) {
    return true;
  } else if (mTransitionType == XPAN_LE_TO_AP_STREAMING) {
    ALOGD("%s %s: Transition type changed from XPAN_LE_TO_AP_STREAMING "
          "to XPAN_LE_TO_AP_IDLE. KP Port closed", R, __func__);
    mTransitionType = XPAN_LE_TO_AP_IDLE;
    mBearerPrefTimer = new XpanAcTimer("BearerPreferenceTimer", BearerPreferenceTimeout, this);
    mBearerPrefTimer->StartTimer(XPAN_AC_BEARER_PREFERENCE_TIMER);
    if (mBearerSwitchState == XPAN_LMP_BEARER_SWITCH_REQ)
      lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_AP);
    return true;
  } else if (mTransitionType == XPAN_AP_TO_LE_STREAMING) {
    ALOGD("%s %s: Transition type changed from XPAN_AP_TO_LE_STREAMING "
          "to XPAN_AP_TO_LE_IDLE. KP Port closed", R, __func__);
    mTransitionType = XPAN_AP_TO_LE_IDLE;
    mBearerPrefTimer = new XpanAcTimer("BearerPreferenceTimer", BearerPreferenceTimeout, this);
    mBearerPrefTimer->StartTimer(XPAN_AC_BEARER_PREFERENCE_TIMER);
    if (mBearerSwitchState == XPAN_LMP_BEARER_SWITCH_REQ)
      lmp->SendLmpL2capPauseUnpauseReq(UNPAUSE, LMP_TP_LE);
    return true;
  }
  return false;
}

void XpanAcStateMachine::HandleUnpauseNotCompleted() {
  if (mTransitionType != XPAN_LE_TO_AP_STREAMING &&
      mTransitionType != XPAN_AP_TO_LE_STREAMING) {
    return;
  }
  ALOGE("%s: unpause not completed but audio moved to new transport", __func__);

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return;
  }

  if (mTransitionType == XPAN_LE_TO_AP_STREAMING) {
    // Disconnect Active LE Link as Audio already transitioned to AP
    qhci->DisconnectLeLink(mBdAddr);
    // Initiate connection from Handset for this explictely initiated disconnection
    mDevice->ScheduleExplicitConnection();
  } else if (mTransitionType == XPAN_AP_TO_LE_STREAMING) {
    // Disconnect Active AP Link as Audio already transitioned to LE
    XmRemoteDisconnectedEvent *new_msg = (XmRemoteDisconnectedEvent *)malloc(sizeof(XmRemoteDisconnectedEvent));
    if (new_msg == NULL) {
      ALOGE("%s: failed to allocate memory", __func__);
      return;
    }
    new_msg->event = XPAN_REMOTE_DISCONNECTED_EVT;
    new_msg->addr = mBdAddr;
    new_msg->role = ROLE_PRIMARY;
    xac->PostMessage((xac_handler_msg_t *)new_msg, MSG_PRIORITY_DEFAULT);
  }
}


void XpanAcStateMachine::HandleNewTcpTransportReadyEvt(xac_handler_msg_t* msg) {
  uint8_t status = msg->newApTransportStatus.status;

  if (status == XPAN_AC_SUCCESS) {
    switch (mTransitionType) {
      case XPAN_AP_TO_AP_ROAMING_IDLE:
           newTcpTransportReady = true;
           /* Proceed with L2cap Pause Unpause (idle transition) & wait to QHCI callback */
           if (newTcpTransportReady && mTransitionType == XPAN_AP_TO_AP_ROAMING_IDLE) {
            qhci->L2capPauseUnpauseReq(mBdAddr, PAUSE, XPAN_AP);
           } else if (mTransitionType == XPAN_AP_TO_AP_ROAMING_STREAMING) {
             //TODO: callback to XM with remote AP details
           }
           break;

      case XPAN_AP_TO_AP_ROAMING_STREAMING:
           newTcpTransportReady = true;
           /* Proceed with L2cap Pause Unpause (idle transition) & wait to QHCI callback */
           if (prepBearerApRoamingDone && mTransitionType == XPAN_AP_TO_AP_ROAMING_IDLE) {
            qhci->L2capPauseUnpauseReq(mBdAddr, PAUSE, XPAN_AP);
           } else if (mTransitionType == XPAN_AP_TO_AP_ROAMING_STREAMING) {
             //TODO: callback to XM with remote AP details
           }
           break;

      default:
          ALOGE("%s %s: Unexpected transition (%d)", R, __func__, mTransitionType);
    }
  } else {
    switch (mTransitionType) {
      case XPAN_AP_TO_AP_ROAMING_IDLE:
           // TODO: Send Bearer preference failed rsp to XM
           lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                                             LMP_TP_AP, 0xFF);
           // roll back to previous AP transport
           setState(XPAN_AP_ACTIVE, XPAN_TCP_NEW_AP_TRANSPORT_STATUS);
           break;

      case XPAN_AP_TO_AP_ROAMING_STREAMING:
           // TODO: Send prepare bearer failed rsp to XM
           // sent to both earbuds
           lmp->SendLmpCancelBearerSwitchInd(LMP_TP_AP,
                                             LMP_TP_AP, 0xFF);
           // roll back to previous AP transport
           setState(XPAN_AP_ACTIVE, XPAN_TCP_NEW_AP_TRANSPORT_STATUS);
           break;

      default:
          ALOGE("%s %s: Unexpected transition (%d)", R, __func__, mTransitionType);
    }
  }
}

void XpanAcStateMachine::UpdateApTransport() {
  /* Update AP details to new transport details */
  mIpAddr = mIpRoaming;
  mMacAddr = mMacRoaming;
  mTcpPort = mTcpPortRoaming;

  /* Unset Roaming details */
  mIpRoaming = {};
  mMacAddr = {};
  mTcpPort = 0;

  /* Update IP address in LMP Manager */
  lmp->SetIpAddress(mIpAddr);

  /* Set new transport in socket handler */
  mSocketHdlr->HandleRoamingCompletion();
}

void XpanAcStateMachine::HandleXpanPrimarySwitchReq(xac_handler_msg_t* msg, uint8_t action) {
  uint8_t status = msg->l2capPauseUnpauseRes.status;
  ALOGD("%s: status = %d action = %d", __func__, status, action);

  if (status == XPAN_AC_SUCCESS) {
    if (action == PAUSE) {
      lmp->SendLmpAccepted();
    }
  }
}

void XpanAcStateMachine::HandleSocketClosure() {
  if (!mSocketHdlr) {
    ALOGE("%s %s: Socket Handler already cleared", R, __func__);
    return;
  }

  // close the connection
  mSocketHdlr->CloseConnectionSocket();

  mSocketFd = -1;
}

XpanDevice* XpanAcStateMachine::GetDeviceInstance(mdns_uuid_t uuid) {
  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s %s: Xpan Application Controller is not initialized.", R, __func__);
    return NULL;
  }

  XpanDevice *dev = xac->GetDeviceByMdnsUuid(uuid);
  if (dev) {
    return dev;
  }
  return NULL;
}

int XpanAcStateMachine::GetSocketFd() {
  if (mSocketHdlr) {
    return mSocketHdlr->GetTcpFd();
  }
  return -1;
}

bdaddr_t XpanAcStateMachine::GetAddr() {
  return mBdAddr;
}

ipaddr_t XpanAcStateMachine::GetIpAddr() {
   return mIpAddr;
}

XpanEarbudRole XpanAcStateMachine::GetRole() {
  return mEarbudRole;
}

XpanTransitionType XpanAcStateMachine::GetCurrentTransition() {
  return mTransitionType;
}

TransportType XpanAcStateMachine::GetTransportFrom() {
  return mTransportFrom;
}

bool XpanAcStateMachine::IsRemoteApDetailsOkToSend() {
  return mApDetailsOkToSend;
}

/*
 * Note - Used only for AP->LE transition so that PEB doesnt progress
 *        if SEB doesnt respond to LMP Prepare Bearer Request as it can
 *        lead to audio loss on SEB. This approach will fail AP->LE
 *        transition and continue music on AP transport.
 */
bool XpanAcStateMachine::IsLmpPrepareBearerReqPendingInSeb() {
  XpanAcStateMachine *seb_sm = mDevice->GetStateMachine(ROLE_SECONDARY);
  if (!seb_sm) {
    return false;
  }

  if (seb_sm->mBearerSwitchState == XPAN_LMP_PREPARE_BEARER_SWITCH_REQ) {
    seb_sm->mIsApToLeHaltedForSeb = true;
    return true;
  }

  return false;
}

bool XpanAcStateMachine::IsTargetedBgconn() {
  return mConnIsBg;
}

void XpanAcStateMachine::SendLmpCancelBearerSwitchInd(uint8_t bearer_from, uint8_t bearer_to,
                                                      uint16_t err) {
  if (mCurrentState == XPAN_BEARER_SWITCH_PENDING) {
    ALOGD("%s %s: Send LMP Bearer Switch Cancel indication to secondary"
          ", bearer_from = %d, bearer_to = %d", R, __func__, bearer_from, bearer_to);
    lmp->SendLmpCancelBearerSwitchInd(bearer_from, bearer_to, err);
    if (bearer_to == LMP_TP_AP && bearer_from == LMP_TP_LE) {
      setState(XPAN_DISCONNECTING, (XacEvent)err);
      XpanBearerSwitchResEvt msg;
      msg.event = (XacEvent)XPAN_LMP_CANCEL_BEARER_SWITCH;
      XpanAcSmExecuteEvent((xac_handler_msg_t *)&msg);

      if (mEarbudRole == ROLE_SECONDARY && mTransitionType == XPAN_LE_TO_AP_IDLE) {
        if (mBearerPrefTimer) {
          stopTimer(mBearerPrefTimer);
          mBearerPrefTimer = NULL;
        }
      }

    } else if (bearer_to == LMP_TP_LE && bearer_from == LMP_TP_AP) {
      setState(XPAN_AP_ACTIVE, (XacEvent)err);
      mIsApToLeHaltedForSeb = false;

      if (mEarbudRole == ROLE_SECONDARY && mTransitionType == XPAN_AP_TO_LE_IDLE) {
        if (mBearerPrefTimer) {
          stopTimer(mBearerPrefTimer);
          mBearerPrefTimer = NULL;
        }
      }
    }
  }
}

void XpanAcStateMachine::SendLmpBearerSwitchCompleteInd(uint8_t bearer_from, uint8_t bearer_to) {
  if (mCurrentState == XPAN_BEARER_SWITCH_PENDING) {
    ALOGD("%s %s: Secondary to send LMP Bearer Switch complete indication."
          ", bearer_from = %d, bearer_to = %d", R, __func__, bearer_from, bearer_to);

    /* mApDetailsOkToSend flag suggests that prepare bearer rsp is received from SEB
       for LE to AP */
    if (bearer_to == LMP_TP_AP && mApDetailsOkToSend) {
      lmp->SendLmpBearerSwitchCmplInd(bearer_from, bearer_to);
      setState(XPAN_AP_ACTIVE, XM_BEARER_SWITCH_IND_EVT);
      if (mEarbudRole == ROLE_SECONDARY && mTransitionType == XPAN_LE_TO_AP_IDLE) {
        stopTimer(mBearerPrefTimer);
        mBearerPrefTimer = NULL;
      }
      if (isPingEnabled && bearer_from == LMP_TP_LE) {
        lmp->setSupervisionTimeout(XPAN_TCP_SUPERVISION_TIMEOUT, XPAN_PERIODIC_PING_TIMER);
        XpanLmpManager::StartSupervisionTimer(lmp);
        lmp->SendLmpLstoInd();
      }
    } else if (bearer_to == LMP_TP_LE && bearer_from == LMP_TP_AP) {
      lmp->SendLmpBearerSwitchCmplInd(bearer_from, bearer_to);
      setState(XPAN_DISCONNECTING, XM_BEARER_SWITCH_IND_EVT);
      if (mEarbudRole == ROLE_SECONDARY && mTransitionType == XPAN_AP_TO_LE_IDLE) {
        stopTimer(mBearerPrefTimer);
        mBearerPrefTimer = NULL;
      }
      XpanL2capPauseUnpauseResEvt l2cPauseUnpause = {.event = XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT};
      XpanAcSmExecuteEvent((xac_handler_msg_t *)&l2cPauseUnpause);
    }
  }
}

void XpanAcStateMachine::HandleBearerPreferenceCmpl(bdaddr_t addr,
    TransportType transport, RspStatus status) {
    ALOGD("%s %s: BDADDR(%s), transport(%d) status(%d)", R, __func__,
           addr.toString().c_str(), transport, status);
  if (mBearerPrefTimer) {
    stopTimer(mBearerPrefTimer);
    mBearerPrefTimer = NULL;
  }

  xm = XMXacIf::GetIf();
  if (xm && mEarbudRole == ROLE_PRIMARY) {
    //xm->BearerPreferenceRsp(addr, transport, status);
  }
}

void XpanAcStateMachine::UpdatePrimaryDisconnToSec() {
  if (mEarbudRole != ROLE_PRIMARY) {
    return;
  }

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return;
  }

  XpanPrimaryDiscEvt *msg =
      (XpanPrimaryDiscEvt *)malloc(sizeof(XpanPrimaryDiscEvt));
  msg->event = XPAN_PRIMARY_DISCONNECTING_EVT,
  msg->addr = mBdAddr;

  xac->PostMessage((xac_handler_msg_t *)msg, MSG_PRIORITY_DEFAULT);
}

/* Updates QHCI about failed idle transition so that it can skip audio bearer transition
   to new bearer if stream start was received during idle transition */
void XpanAcStateMachine::UpdateQhciAboutFailedIdleTransition() {
  if (mEarbudRole != ROLE_PRIMARY) return;

  if (mTransitionType == XPAN_LE_TO_AP_IDLE) {
    if ((mCurrentState == XPAN_DISCONNECTING || mCurrentState == XPAN_IDLE) &&
        (mPrevState == XPAN_TCP_CONNECTING || mPrevState== XPAN_TCP_CONNECTED ||
         mPrevState== XPAN_LMP_CONNECTING  || mPrevState== XPAN_LMP_CONNECTED ||
         mPrevState== XPAN_BEARER_SWITCH_PENDING)) {
      qhci->UpdateIdleTransitionStatus(mBdAddr, XPAN_AP, IDLE_TRANSITION_FAILED);
    }
  } else if (mTransitionType == XPAN_AP_TO_LE_IDLE) {
    if (mPrevState == XPAN_BEARER_SWITCH_PENDING &&
        (mCurrentState == XPAN_AP_ACTIVE || mCurrentState == XPAN_DISCONNECTING)) {
      qhci->UpdateIdleTransitionStatus(mBdAddr, BT_LE, IDLE_TRANSITION_FAILED);
    }
  }
}


void XpanAcStateMachine::SetFilteredScanTimer() {
    mFilteredScanTimer = new XpanAcTimer("MFilterScanTimer", TriggerFilteredScan, this);
    mFilteredScanTimer->StartTimer(XPAN_AC_FILTERED_SCAN_TRIGGER_TIMER);
}

void XpanAcStateMachine::SetSupervisionTimer(uint16_t supervision_to, uint16_t interval) {
  lmp->setSupervisionTimeout(supervision_to, interval);
}

void XpanAcStateMachine::Cleanup() {
  ALOGD("%s %s: BDADDR(%s), IP(%s) Role(%d)", R, __func__, mBdAddr.toString().c_str(),
         mIpAddr.toString().c_str(), mEarbudRole);

  if (mDnsQueryTimer) {
    ALOGD("%s %s: Stopping mDnsQueryTimer", R, __func__);
    stopTimer(mDnsQueryTimer);
    mDnsQueryTimer = NULL;
  }

  if (mFilteredScanTimer) {
    ALOGD("%s %s: Stopping mFilteredScanTimer", R, __func__);
    stopTimer(mFilteredScanTimer);
    mFilteredScanTimer = NULL;
  }

  if (mTargetedConnTimer) {
    ALOGD("%s %s: Stopping mTargetedConnTimer", R, __func__);
    stopTimer(mTargetedConnTimer);
    mTargetedConnTimer = NULL;
  }

  if (mBearerPrefTimer) {
    ALOGD("%s %s: Stopping mBearerPrefTimer", R, __func__);
    stopTimer(mBearerPrefTimer);
    mBearerPrefTimer = NULL;
  }

  if (mSocketHdlr) {
    ALOGD("%s %s: cleaning up Socket Handler", R, __func__);
    delete mSocketHdlr;
    mSocketHdlr = NULL;
  }

  if (lmp) {
    ALOGD("%s %s: cleaning up Lmp Manager", R, __func__);
    delete lmp;
    lmp = NULL;
  }
}

/* To stop and delete Timer */
void stopTimer(XpanAcTimer *timer) {
  ALOGD("%s: Stop and Delete Timer", __func__);
  timer->StopTimer();
  timer->DeleteTimer();
  delete(timer);
}

} // namespace ac
} // namespace xpan
