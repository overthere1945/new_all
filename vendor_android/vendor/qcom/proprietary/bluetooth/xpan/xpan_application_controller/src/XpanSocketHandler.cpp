
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc. 
 */

#include <algorithm>
#include <arpa/inet.h>
#include <errno.h>
#include <cutils/properties.h>
#include <log/log.h>
#include <stdlib.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>

//       #include <sys/time.h>
//       #include <sys/types.h>

#include <unistd.h>
#include "xpan_ac_int.h"
#include "xpan_ac_tls_int.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.bluetooth.xpan_ac.XpanSocketHandler"

/* Xpan Socket Manager Implementation.*/

namespace xpan {
namespace ac {

int XpanSocketHandler::tcp_lfd;
int XpanSocketHandler::tcp_lport;
int XpanSocketHandler::udp_data_fd;
int XpanSocketHandler::udp_data_port_rx;
int XpanSocketHandler::udp_tsf_fd;
int XpanSocketHandler::udp_tsf_port_rx;
int XpanSocketHandler::hdlr_pipe_fd[2];
int XpanSocketHandler::nfds;
std::vector<int> XpanSocketHandler::mReadFds;
std::vector<bdaddr_t> XpanSocketHandler::mListeners;
fd_set XpanSocketHandler::rfds;
int XpanSocketHandler::acceptTempFd;
bool XpanSocketHandler::mIsRxRoutineRunning;
//std::mutex XpanSocketHandler::mRxRoutine;

XpanSocketHandler::XpanSocketHandler(bdaddr_t addr, XpanEarbudRole role) {
  tcp_fd = -1;
  mBdAddr = addr;
  mRole = role;
  qhci = XpanQhciAcIf::GetIf();
  mTlsHandler = new XpanTLSHandler();
  mTlsHandler->SetTcpRole((uint8_t)XPAN_TCP_CLIENT);
  mTcpRole = XPAN_TCP_ROLE_UNKNOWN;
}

XpanSocketHandler::~XpanSocketHandler() {
  ALOGD("%s", __func__);
  if (tcp_fd > 0) {
    close(tcp_fd);
  }

  tcp_fd = -1;

  delete mTlsHandler;
  mTlsHandler = NULL;
  mTcpRole = XPAN_TCP_ROLE_UNKNOWN;
  CloseListeningSocket(mBdAddr);
  if (mTcpConnThread.joinable()) {
    mTcpConnThread.join();
    ALOGD("%s Tcp Connection Thread joined", __func__);
  }
}

int XpanSocketHandler::GetTcpFd() {
  return tcp_fd;
}

int XpanSocketHandler::GetUdpDataFd() {
  return udp_data_fd;
}

int XpanSocketHandler::GetUdpTsfFd() {
  return udp_tsf_fd;
}

void XpanSocketHandler::StopRxThreadRoutine() {
  ALOGD("%s", __func__);
  std::string int_msg = "StopRxThreadRoutine";
  write(hdlr_pipe_fd[1], int_msg.c_str(), int_msg.length() + 1);
}

void XpanSocketHandler::HandleRxThreadTermination() {
  FD_ZERO(&rfds);
  mIsRxRoutineRunning = false;
  mReadFds.clear();
  mListeners.clear();
  nfds = 0;
}

ipaddr_t sockaddr_to_ip(int fd) {
  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return {};
  }

  ipaddr_t ip = xac->GetIpFromFd(fd);
  if (!ip.isEmpty()) {
    return ip;
  }

  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  int res = getpeername(fd, (struct sockaddr *)&addr, &addr_size);
  if (res != XPAN_AC_SUCCESS) {
    ALOGE("%s: Couldnt locate IP of remote. Ignore data", __func__);
    return {};
  }

  ip.type = IPv4;
  ip.ipv4[0] = (uint8_t)(addr.sin_addr.s_addr);
  ip.ipv4[1] = (uint8_t)(addr.sin_addr.s_addr >> 8);
  ip.ipv4[2] = (uint8_t)(addr.sin_addr.s_addr >> 16);
  ip.ipv4[3] = (uint8_t)(addr.sin_addr.s_addr >> 24);
  ALOGD("%s: Sockaddr_in IP (%s)", __func__, ip.toString().c_str());

  return ip;
}

int XpanSocketHandler::GetLastFd() {
  int maxFd = 0;
  for (int i = 0; i < mReadFds.size(); i++) {
    if (maxFd < mReadFds[i]) {
      maxFd = mReadFds[i];
    }
  }

  ALOGD("%s: max fd = %d", __func__, maxFd);//debug
  return maxFd;
}

int XpanSocketHandler::GetTcpListeningPort() {
  return tcp_lport;
}

int XpanSocketHandler::GetUdpPort() {
  return udp_data_port_rx;
}

int XpanSocketHandler::GetUdpTsfPort() {
  return udp_tsf_port_rx;
}

XpanTLSHandler* XpanSocketHandler::GetTLSHandler() {
  return mTlsHandler;
}

XpanTcpRole XpanSocketHandler::GetTcpRole() {
  return mTcpRole;
}

int XpanSocketHandler::GetPendingConnectRequestId() {
  return mConnectReqId;
}

XpanTLSHandler* XpanSocketHandler::GetTlsHandlerServer() {
  return mTlsHandlerServer;
}

void XpanSocketHandler::HandleUpdateReadFds(char *msg, int len) {
  if (!msg || (len < 5)) {
    ALOGE("%s: Invalid message. Ignore.", __func__);
    return;
  }

  /* Other thread writes message to the pipe to either add/remove
   * fd from read set used for select call.
   * Format of the message :-
   *      "Action-fd"
   *       Where, Action: add/remove
   *              fd: fd integer value
   */
  char add[] = "add", remove[] = "remove";
  char fd_act[2][20];
  char* tmp = NULL;
  char* token = strtok_r(msg, "-", &tmp);
  int idx = 0; // Index to token list
  while (token != NULL && idx < 2){
    size_t token_len = strlen(token) + 1; // extra byte needed by strlcpy for null terminator
    strlcpy(fd_act[idx], token, token_len); // Copy to token list
    idx++;
    token = strtok_r(NULL, "-", &tmp);
  }

  int fd = atoi(fd_act[1]);
  if (fd == 0) {
    ALOGE("%s: Invalid fd. Skip the value", __func__);
    return;
  }

  if (!strcmp(fd_act[0], add)) {
    ALOGD("%s: Add new fd(%d) to the set", __func__, fd);
    // Add new fd to the fd set
    UpdateFdSet(true, fd);
  } else if (!strcmp(fd_act[0], remove)) {
    ALOGD("%s: Remove fd(%d) from the set", __func__, fd);
    UpdateFdSet(false, fd);
  }
}

void XpanSocketHandler::UpdateFdSetInRxThread(int action, int fd) {
  if (action == 1) {
    /* add fd to I/O multiplexing Rx thread */
    std::string int_msg = "add-" + std::to_string(fd) + "*";
    ALOGD("%s: Add fd to I/O Mux FD_SET(msg-> %s)", __func__, int_msg.c_str());
    write(hdlr_pipe_fd[1], int_msg.c_str(), int_msg.length());
  } else if (action == 0) {
    /* remove the socket fd from I/O multiplexing Rx thread */
     std::string int_msg = "remove-" + std::to_string(fd) + "*";;
     ALOGD("%s: Remove fd from I/O Mux FD_SET(msg-> %s)", __func__, int_msg.c_str());
     write(hdlr_pipe_fd[1], int_msg.c_str(), int_msg.length() + 1);
  }
}

bool XpanSocketHandler::SendData(uint8_t llid, std::vector<uint8_t> &data) {
  if (XpanApplicationController::IsTLSEnabled()) {
    /* debug logs */
    /*for (int j=0; j < data.size(); j++) {
      ALOGD("%s: data[%d] = %02x", __func__, j, data[j]);
    }*/
    if (mTlsHandler) {
      int ret_val = 0;
      if (mTcpRole == XPAN_TCP_CLIENT) {
        ret_val = mTlsHandler->ClientSendData(data.data(), data.size());
      } else if (mTcpRole == XPAN_TCP_SERVER) {
        ret_val = mTlsHandler->ServerSendData(data.data(), data.size());
      } else {
        ALOGE("%s: Data not sent in unknown TCP role", __func__);
      }
      if (llid == LE_L2CAP_CONT || llid == LE_L2CAP_START) {
        // callback to QHCI (Number of packets completed)
        qhci->NumberOfPacketsCompleted(mBdAddr, 1);
      }
    } else {
      return false;
    }
    return true;
  }

  if (tcp_fd == -1) {
    ALOGE("%s: TCP Connection is not created yet", __func__);
    return false;
  }

  void *buf = (void *)data.data();
  size_t len = (size_t)data.size();

  ssize_t ret = send(tcp_fd, buf, len, 0);
  if (ret <= 0) {
    ALOGE("%s: Failed to write data error: %s", __func__, strerror(errno));
    return false;
  }

  if (llid == LE_L2CAP_CONT || llid == LE_L2CAP_START) {
    // callback to QHCI (Number of packets completed)
    qhci->NumberOfPacketsCompleted(mBdAddr, 1);
  }

  return true;
}

/* Tx Handler APIs */
bool XpanSocketHandler::InitiateTxRoutine() {
  return true;
}

/* Intermediate API to transfer context to new thread */
bool XpanSocketHandler::InitTcpConnection(XpanSocketHandler *sock, ipaddr_t remote_ip,
                       uint32_t tcp_port) {
  mConnectReqId = ++(XpanApplicationController::mUniqueId);
  mTcpConnThread = std::thread(XpanSocketHandler::ConnectTcp, sock, remote_ip, tcp_port,
                               mConnectReqId);
  ALOGD("%s: TCP Connect(Request Id:%d) initiated in other thread", __func__, mConnectReqId);
  return true;
}

/* Generates tokens based on delimiter and string */
std::vector<std::string> GetTokens(const char* p, char *msg) {
  std::vector<std::string> tokens;

  char* tmp = NULL;
  char* token = strtok_r(msg, p, &tmp);

  while (token != NULL){
    std::string tok(token);
    tokens.push_back(token);
    token = strtok_r(NULL, p, &tmp);
  }

  return tokens;
}

bool XpanSocketHandler::IsValidPendingConnection(ipaddr_t ip, int conn_req_id) {
  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: No valid Application Controller instance for Connect Req (%d)",
          __func__, conn_req_id);
    return false;
  }

  XpanDevice *device = xac->GetDeviceByIpAddr(ip);
  if (!device) {
    ALOGE("%s: No valid XpanDevice instance for Connect Req (%d)",
          __func__, conn_req_id);
    return false;
  }

  XpanEarbudRole role = device->GetRoleByIpAddr(ip);
  if (role == ROLE_INVALID) {
    ALOGE("%s: Invalid device role for Connect Req (%d)",
          __func__, conn_req_id);
    return false;
  }

  XpanSocketHandler *shandler = device->GetSocketHandler(role);
  if (!shandler) {
    ALOGE("%s: No valid Socket Handler for Connect Req (%d)",
          __func__, conn_req_id);
    return false;
  }

  int pendingConnId = shandler->GetPendingConnectRequestId();
  if (pendingConnId != conn_req_id) {
    ALOGW("%s: Pending Connection Req Id(%d), this request(%d). Ignore it",
          __func__, pendingConnId, conn_req_id);
    return false;
  }

  return true;
}


bool XpanSocketHandler::ConnectTcp(XpanSocketHandler *sock, ipaddr_t remote_ip,
                                   uint32_t tcp_port, int conn_req_id) {
  ALOGD("%s: IP = %s, port = %ld Request_Id =(%d)", __func__,
        remote_ip.toString().c_str(), tcp_port, conn_req_id);

  XpanSocketHandler *sock_hdlr = static_cast<XpanSocketHandler *>(sock);
  if (!sock_hdlr) {
    ALOGE("%s: Invalid socket Handler instance", __func__);
    return false;
  }

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return false;
  }

  XpanDevice *device = xac->GetDeviceByIpAddr(remote_ip);

  XpanTLSHandler* tlsHandler = XpanApplicationController::GetTLSHandler(remote_ip);
  if (XpanApplicationController::IsTLSEnabled()) {
    if (tlsHandler) {
      tlsHandler->InitTlsSessionParams();
    } else {
      ALOGW("%s: tlsHandler is null, return", __func__);
      return false;
    }
  } else {
    ALOGI("%s: TLS not enabled", __func__);
  }

  XpanTcpConnectionFailedEvt *evt;
  XpanTcpConnectedEvt *msg;
  struct sockaddr_in servaddr;
  socklen_t addrlen;
  sock_hdlr->tcp_fd_ = -1;

  uint8_t retry_count = 0, retval, MAX_RETRY_COUNT = 9 /* 20.7 sec */;
  int connect_err = 0;
  bool ret_val;

  do {
    /* create socket for client */
    sock_hdlr->tcp_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_hdlr->tcp_fd_ == -1) {
      ALOGE("%s: socket creation failed with server(%s:%d) failed: %s", __func__,
            remote_ip.toString().c_str(), tcp_port, strerror(errno));
      evt = (XpanTcpConnectionFailedEvt *)malloc(sizeof(XpanTcpConnectionFailedEvt));
      if (evt == NULL) {
        ALOGE("%s: failed to allocate memory", __func__);
        return false;
      }
      evt->event = XPAN_TCP_CONNECT_FAILED;
      evt->addr = sock_hdlr->mBdAddr;
      evt->ip = remote_ip;
      evt->role = sock_hdlr->mRole;
      evt->isOutgoing = true;
      xac->PostMessage((xac_handler_msg_t *)evt, MSG_PRIORITY_DEFAULT);
      return false;
    }

    SetSocketIfOption(sock_hdlr->tcp_fd_);

    if (XpanApplicationController::IsTLSEnabled())
      ret_val = tlsHandler->SetServerSocketFd(sock_hdlr->tcp_fd_);
    bzero(&servaddr, sizeof(servaddr));
    addrlen = sizeof(struct sockaddr_in);
    servaddr.sin_family = AF_INET ;
    servaddr.sin_port = htons(tcp_port);
    inet_pton(AF_INET, remote_ip.toString().c_str(), &servaddr.sin_addr.s_addr);

    retval = connect(sock_hdlr->tcp_fd_, (struct sockaddr *)&servaddr, addrlen);
    connect_err = errno;

    /* Note - TCP Connection to be retried again if connection with remote fails
              with No route to Host reason */
    if (retval != 0 && connect_err == EHOSTUNREACH) {
      retry_count++;
      if (retry_count < MAX_RETRY_COUNT && !sock_hdlr->mStopArpRetry) {
        ALOGE("%s: retry TCP connection for %s, stop arp retry = %d , retry_count = %d ",
              __func__, strerror(errno), sock_hdlr->mStopArpRetry, retry_count);
        close(sock_hdlr->tcp_fd_);
      } else {
        ALOGE("%s: Cancel ARP retries, stop arp retry = %d , retry_count = %d ",
              __func__, sock_hdlr->mStopArpRetry, retry_count);
      }
    } else {
      break;
    }
  } while (retry_count < MAX_RETRY_COUNT && !sock_hdlr->mStopArpRetry);

  sock_hdlr->mStopArpRetry = false;

  if (!IsValidPendingConnection(remote_ip, conn_req_id)) {
    if (sock_hdlr->tcp_fd_ > 0) {
      // Close opened socket
      ALOGE("%s: no valid socket handler, closing outgoing fd = %d", __func__, sock_hdlr->tcp_fd_);
      close(sock_hdlr->tcp_fd_);
      sock_hdlr->tcp_fd_ = -1;
    }
    return false;
  }

  if (retval != 0) {
    ALOGE("%s: connect with server(%s:%d) failed: %s", __func__,
          remote_ip.toString().c_str(), tcp_port, strerror(errno));
    evt = (XpanTcpConnectionFailedEvt *)malloc(sizeof(XpanTcpConnectionFailedEvt));
    if (evt == NULL) {
      ALOGE("%s: failed to allocate memory", __func__);
      if (sock_hdlr->tcp_fd_ > 0) {
        // Close opened socket
        ALOGE("%s: failed to allocate memory, closing outgoing fd = %d",
            __func__, sock_hdlr->tcp_fd_);
        close(sock_hdlr->tcp_fd_);
        sock_hdlr->tcp_fd_ = -1;
      }
      return false;
    }
    evt->event = XPAN_TCP_CONNECT_FAILED;
    evt->addr = sock_hdlr->mBdAddr;
    evt->ip = remote_ip;
    evt->role = sock_hdlr->mRole;
    evt->isOutgoing = true;
    evt->reason = (connect_err == EHOSTUNREACH ? NO_ROUTE_TO_HOST: TCP_HANDSHAKE_FAILED);
    xac->PostMessage((xac_handler_msg_t *)evt, MSG_PRIORITY_DEFAULT);
    if (sock_hdlr->tcp_fd_ > 0) {
      // Close opened socket
      ALOGE("%s: connect with server failed, closing outgoing fd = %d", __func__, sock_hdlr->tcp_fd_);
      close(sock_hdlr->tcp_fd_);
      sock_hdlr->tcp_fd_ = -1;
    }
    return false;
  }

  // Mark this connection with HS as TCP Client
  if (sock_hdlr->GetTcpRole() == XPAN_TCP_SERVER) {
    ALOGE("%s: Collision in TCP Connection. Allow incoming connection to proceed", __func__);
    if (sock_hdlr->tcp_fd_ > 0) {
      // Close opened socket
      ALOGE("%s: Collision in TCP Connection, closing outgoing fd = %d", __func__, sock_hdlr->tcp_fd_);
      close(sock_hdlr->tcp_fd_);
      sock_hdlr->tcp_fd_ = -1;
    }
    return false;
  }

  sock_hdlr->SetTcpRole(XPAN_TCP_CLIENT);
  ALOGD("%s: TCP Client role set", __func__);

  sock_hdlr->tcp_fd = sock_hdlr->tcp_fd_;
  /* Apply IP_TOS and SO_PRIORITY */
  SetSocketTosOption(sock_hdlr->tcp_fd);
  /* Disable Nagles algorithm */
  DisableDelayedPacketTransfer(sock_hdlr->tcp_fd);
  /* Enable TCP Keep Alive */
  EnableTcpKeepAlive(sock_hdlr->tcp_fd);

  if (XpanApplicationController::IsTLSEnabled() && device) {
    ret_val = tlsHandler->ClientEstablishTlsHS(device);
    if (!ret_val) {
      ALOGE("%s: TLS Client Handshake failed (%s:%d) failed: %s", __func__,
          remote_ip.toString().c_str(), tcp_port, strerror(errno));
      evt = (XpanTcpConnectionFailedEvt *)malloc(sizeof(XpanTcpConnectionFailedEvt));
      if (evt == NULL) {
        ALOGE("%s: failed to allocate memory", __func__);
        if (sock_hdlr->tcp_fd_ > 0) {
          // Close opened socket
          ALOGE("%s: failed to allocate memory, closing outgoing fd = %d",
              __func__, sock_hdlr->tcp_fd_);
          close(sock_hdlr->tcp_fd_);
          sock_hdlr->tcp_fd_ = -1;
        }
        return false;
      }
      evt->event = XPAN_TCP_CONNECT_FAILED;
      evt->addr = sock_hdlr->mBdAddr;
      evt->ip = remote_ip;
      evt->role = sock_hdlr->mRole;
      evt->isOutgoing = true;
      xac->PostMessage((xac_handler_msg_t *)evt, MSG_PRIORITY_DEFAULT);
      if (sock_hdlr->tcp_fd_ > 0) {
        // Close opened socket
        ALOGE("%s: TLS Client Handshake failed, closing outgoing fd = %d",
            __func__, sock_hdlr->tcp_fd_);
        close(sock_hdlr->tcp_fd_);
        sock_hdlr->tcp_fd_ = -1;
      }
      return false;
    }
  }
  /* add fd to I/O multiplexing Rx thread */
  std::string int_msg = "add-" + std::to_string(sock_hdlr->tcp_fd) + "*";
  ALOGD("%s: Add fd to I/O Mux fd set(msg-> %s)", __func__, int_msg.c_str());
  write(hdlr_pipe_fd[1], int_msg.c_str(), int_msg.length());

  /* Send TCP Connected Evt to State Machine */
  msg = (XpanTcpConnectedEvt *)malloc(sizeof(XpanTcpConnectedEvt));
  if (msg == NULL) {
    ALOGE("%s: failed to allocate memory", __func__);
    if (sock_hdlr->tcp_fd_ > 0) {
      // Close opened socket
      ALOGE("%s: failed to allocate memory, closing outgoing fd = %d", __func__, sock_hdlr->tcp_fd_);
      close(sock_hdlr->tcp_fd_);
      sock_hdlr->tcp_fd_ = -1;
    }
    return false;
  }

  // Unset pending connection
  sock_hdlr->tcp_fd_ = -1;

  msg->event = XPAN_TCP_CONNECTED_EVT;
  msg->addr = sock_hdlr->mBdAddr;
  msg->ip = remote_ip;
  msg->role = sock_hdlr->mRole;
  msg->fd = sock_hdlr->tcp_fd;
  msg->isIncoming = false;
  xac->PostMessage((xac_handler_msg_t *)msg, MSG_PRIORITY_DEFAULT);

  return true;
}

/* AP to AP Roaming transport preparation */
/* Intermediate API to transfer context to new thread */
bool XpanSocketHandler::InitiateRoamingPrep(XpanSocketHandler *sock, ipaddr_t remote_ip,
                       uint32_t tcp_port) {
  mTcpConnThread = std::thread(XpanSocketHandler::PrepareRoamingTransport, sock, remote_ip, tcp_port);
  mTcpConnThread.detach();
  ALOGD("%s: TCP Connect Initiated in other thread", __func__);
  return true;
}

bool XpanSocketHandler::PrepareRoamingTransport(XpanSocketHandler *sock, ipaddr_t remote_ip,
                                   uint32_t tcp_port) {
  ALOGD("%s: IP = %s, port = %ld", __func__, remote_ip.toString().c_str(), tcp_port);

  XpanSocketHandler *sock_hdlr = static_cast<XpanSocketHandler *>(sock);
  if (!sock_hdlr) {
    ALOGE("%s: Invalid socket Handler instance", __func__);
    return false;
  }

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return false;
  }

  XpanNewApTransportStatus *evt;
  struct sockaddr_in servaddr;
  socklen_t addrlen;

  /* create socket for client */
  sock_hdlr->tcp_fd_roaming = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_hdlr->tcp_fd_roaming == -1) {
    ALOGE("%s: socket creation failed with server(%s:%d) failed: %s", __func__,
          remote_ip.toString().c_str(), tcp_port, strerror(errno));
    evt = (XpanNewApTransportStatus *)malloc(sizeof(XpanNewApTransportStatus));
    if (evt == NULL) {
      ALOGE("%s: failed to allocate memory", __func__);
      return false;
    }
    evt->event = XPAN_TCP_NEW_AP_TRANSPORT_STATUS;
    evt->addr = sock_hdlr->mBdAddr;
    evt->ip = remote_ip;
    evt->role = sock_hdlr->mRole;
    evt->status = 0xFF;
    xac->PostMessage((xac_handler_msg_t *)evt, MSG_PRIORITY_DEFAULT);
    return false;
  }

  bzero(&servaddr, sizeof(servaddr));
  addrlen = sizeof(struct sockaddr_in);
  servaddr.sin_family = AF_INET ;
  servaddr.sin_port = htons(tcp_port);
  inet_pton(AF_INET, remote_ip.toString().c_str(), &servaddr.sin_addr.s_addr);

  int retval = connect(sock_hdlr->tcp_fd,(struct sockaddr *)&servaddr, addrlen);

  if (retval != 0) {
    ALOGE("%s: connect with server(%s:%d) failed: %s", __func__,
          remote_ip.toString().c_str(), tcp_port, strerror(errno));
    evt = (XpanNewApTransportStatus *)malloc(sizeof(XpanNewApTransportStatus));
    if (evt == NULL) {
      ALOGE("%s: failed to allocate memory", __func__);
      return false;
    }
    evt->event = XPAN_TCP_NEW_AP_TRANSPORT_STATUS;
    evt->addr = sock_hdlr->mBdAddr;
    evt->ip = remote_ip;
    evt->role = sock_hdlr->mRole;
    evt->status = 0xFF;
    xac->PostMessage((xac_handler_msg_t *)evt, MSG_PRIORITY_DEFAULT);
    return false;
  }

  /* add fd to I/O multiplexing Rx thread */
  std::string int_msg = "add-" + std::to_string(sock_hdlr->tcp_fd_roaming);
  ALOGD("%s: Add fd to I/O Mux fd set(msg-> %s)", __func__, int_msg.c_str());
  write(hdlr_pipe_fd[1], int_msg.c_str(), int_msg.length() + 1);

  /* Send TCP Connected Evt to State Machine */
  evt = (XpanNewApTransportStatus *)malloc(sizeof(XpanNewApTransportStatus));
  if (evt == NULL) {
      ALOGE("%s: failed to allocate memory", __func__);
      return false;
  }
  evt->event = XPAN_TCP_NEW_AP_TRANSPORT_STATUS;
  evt->addr = sock_hdlr->mBdAddr;
  evt->ip = remote_ip;
  evt->role = sock_hdlr->mRole;
  evt->status = (retval == 0 ? XPAN_AC_SUCCESS: 0xFF);
  xac->PostMessage((xac_handler_msg_t *)evt, MSG_PRIORITY_DEFAULT);

  return true;
}

void XpanSocketHandler::HandleRoamingCompletion() {
  // remove tcp fd from FD Set
  UpdateFdSet(false, tcp_fd);

  // Close previous socket connection
  int ret = shutdown(tcp_fd, SHUT_RDWR);  // Is this needed
  if (!ret) {
    ALOGD("%s: Shutdown successful", __func__);
  }
  close(tcp_fd);

  tcp_fd = tcp_fd_roaming;
  tcp_fd_roaming = -1;
}

bool XpanSocketHandler::CreateTcpSocketForIncomingConnection(ipaddr_t ip, bdaddr_t addr) {
  ALOGD("%s: For bd addr %s", __func__, addr.toString().c_str());

  if (IsListeningOnTcp()) {
    ALOGD("%s: Already Listening for inc connection", __func__);
    AddListenerFor(addr);
    return true;
  }

  struct sockaddr_in servaddr;
  int retval = -1;
  int option = 1;

  tcp_lfd = socket(AF_INET, SOCK_STREAM| SOCK_NONBLOCK, 0);
  if (tcp_lfd < 0) {
    ALOGE("%s: TCP Socket creation failed: %s", __func__, strerror(errno));
    return false;
  }

  if (setsockopt(tcp_lfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))) {
    ALOGW("%s: failed to reuse address for lfd(%d): %s", __func__,
          tcp_lfd, strerror(errno));
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET ;
  inet_pton(AF_INET, ip.toString().c_str(), &servaddr.sin_addr.s_addr);
  servaddr.sin_port = htons(XPAN_AC_LISTENING_TCP_PORT);

  if ((retval = bind(tcp_lfd, (sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
    ALOGE("%s: Failed to bind udp port for incoming data: %s", __func__, strerror(errno));
    close(tcp_lfd);
    tcp_lfd = -1;
    return false;
  }

  struct sockaddr_in sock_addr;
  socklen_t  addr_len = sizeof(sock_addr);
  if (getsockname(tcp_lfd, (struct sockaddr *)&sock_addr, &addr_len) == 0) {
    tcp_lport = ntohs(sock_addr.sin_port);
  }

  ALOGD("%s: TCP Socket is ready to receive TCP data on port %d",
        __func__, tcp_lport);

  retval = listen(tcp_lfd, 5); // 5 max incoming connection in queue allowed
  if (retval == -1) {
    ALOGE("%s: failed to start listening for tcp ip:%s port:%d error:%s",
          __func__, ip.toString().c_str(), tcp_lport, strerror(errno));
    close(tcp_lfd);
    tcp_lfd = -1;
    return false;
  }

  AddListenerFor(addr);

  /* add fd to I/O multiplexing Rx thread */
  std::string int_msg = "add-" + std::to_string(tcp_lfd);
  ALOGD("%s: Add fd to I/O Mux fd set(msg-> %s)", __func__, int_msg.c_str());
  write(hdlr_pipe_fd[1], int_msg.c_str(), int_msg.length() + 1);

  return true;
}

bool XpanSocketHandler::IsListeningOnTcp() {
  if (tcp_lfd <= 0) {
    return false;
  }
  return true;
}

bool XpanSocketHandler::AcceptIncomingTcpConnection() {
  struct sockaddr_in clientaddr;
  socklen_t addrlen = sizeof(struct sockaddr_in);

  int fd = accept4(tcp_lfd, (sockaddr *)&clientaddr, &addrlen, SOCK_NONBLOCK);
  if (fd == -1) {
    ALOGE("%s: Accept Connection Failed (%d - %s). Stop listening.",
          __func__, fd, strerror(errno));
    UpdateFdSetInRxThread(1, fd);
    tcp_lfd = -1;
    return false;
  }

  SetSocketTosOption(fd);
  /* Disable Nagles algorithm */
  DisableDelayedPacketTransfer(fd);
  /* Enable TCP Keep Alive */
  EnableTcpKeepAlive(fd);

  // Extract IP address (for IPv4)
  char ip_addr[INET_ADDRSTRLEN];
  if (clientaddr.sin_family != AF_INET) {
    ALOGE("%s: not a IP address. close socket.", __func__);
    close(fd);
    return false;
  }
  inet_ntop(AF_INET, &(clientaddr.sin_addr), ip_addr, INET_ADDRSTRLEN);
  acceptTempFd = fd;
  ALOGD("%s: Connection accepted for %s: fd = %d",__func__, ip_addr, acceptTempFd);


  if (XpanApplicationController::IsTLSEnabled()) {
    ipaddr_t remote_ip;
    remote_ip.type = IPv4;
    remote_ip.ipv4[0] = ip_addr[0];remote_ip.ipv4[1] = ip_addr[1];
    remote_ip.ipv4[2] = ip_addr[2];remote_ip.ipv4[3] = ip_addr[3];
    /*Set this method in Tls Handler as static as remote ip may not be known */
    XpanTLSHandler* tlsHandler = new XpanTLSHandler();
    tlsHandler->SetTcpRole((uint8_t)XPAN_TCP_SERVER);
    tlsHandler->SetRemoteIpAddress(remote_ip);
    //XpanTLSHandler* tlsHandler = XpanApplicationController::GetTLSHandler(remote_ip);

    if (!tlsHandler || !tlsHandler->InitTlsServerSession() ||
        !tlsHandler->ConfigureTlsServer()) {
      ALOGE("%s: SSL Config for TLS server Failed", __func__);
      close(fd);
      return false;
    }

    // set accepted connection fd in tls handler
    tlsHandler->SetClientSocketFd(fd);

    // Start TLS server role for handshake
    if (!tlsHandler->ServerEstablishTlsHS()) {
      ALOGE("%s: TLS Handshake Failed - %s", __func__, strerror(errno));
      close(fd);
      return false;
    }
  }

  UpdateFdSetInRxThread(1, fd);

  return true;
}

void XpanSocketHandler::CloseConnectionSocket() {
  /* first check if there is pending connection for this socket handler*/
  if (tcp_fd_ > 0) {
    int ret = shutdown(tcp_fd_, SHUT_RDWR);
    if (!ret) {
      ALOGD("%s: Shutdown successful for pending connection(fd:%d)", __func__, tcp_fd_);
    }

    ret = close(tcp_fd_);
    if (!ret) {
      ALOGD("%s: Close successful for pending connection(fd:%d)", __func__, tcp_fd_);
    }
    mTcpConnThread.join();
    ALOGD("%s: Connection thread terminated", __func__);
  }

  if (tcp_fd > 0) {
    int ret = shutdown(tcp_fd, SHUT_RDWR);  // Is this needed
    if (!ret) {
      ALOGD("%s: Shutdown successful", __func__);
    }

    ret = close(tcp_fd);
    if (!ret) {
      ALOGD("%s: socket close successful", __func__);
    }

    // remove the socket from I/O mux
    ALOGD("%s: Remove tcp_fd (%d from FD_SET)", __func__, tcp_fd);
    std::string int_msg = "remove-" + std::to_string(tcp_fd);
    write(hdlr_pipe_fd[1], int_msg.c_str(), int_msg.length() + 1);

  } else {
    ALOGE("%s: Socket already closed", __func__);
  }

  tcp_fd = -1;
  mTcpRole = XPAN_TCP_ROLE_UNKNOWN;
}

void XpanSocketHandler::CloseListeningSocket(bdaddr_t addr) {
  if (!ContainsListener(addr)) {
    ALOGE("%s: No incoming connection expected for %s", __func__, ConvertRawBdaddress(addr));
    return;
  }

  if (!XpanApplicationController::GetLocalIpAddr().isEmpty() &&
      !XpanApplicationController::GetLocalMacAddr().isEmpty()) {
    RemoveListenerFor(addr);
    if (mListeners.size() > 0) {
      ALOGD("%s: Other Incoming connections pending", __func__);
      return;
    }
  } else {
    ALOGE("%s: AP connection reset. Remove listening socket", __func__);
    mListeners.clear();
  }

  if (tcp_lfd > 0) {
    int ret = close(tcp_lfd);
    if (!ret) {
      ALOGD("%s: socket close successful"
            ". listening socket (fd %d)", __func__, tcp_lfd);
      std::string int_msg = "remove-" + std::to_string(tcp_lfd);
      write(hdlr_pipe_fd[1], int_msg.c_str(), int_msg.length() + 1);
    }
  } else {
    ALOGE("%s: Socket already closed", __func__);
  }

  tcp_lfd = -1;
  tcp_lport = 0;
}

bool XpanSocketHandler::CreateUdpSocketForIncomingData(ipaddr_t ip) {
  ALOGD("%s", __func__);

  struct sockaddr_in servaddr;
  int retval = -1;

  udp_data_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_data_fd < 0) {
    ALOGE("%s: Udp Socket creation failed (%s)", __func__, strerror(errno));
    return false;
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET ;
  inet_pton(AF_INET, ip.toString().c_str(), &servaddr.sin_addr.s_addr);
  servaddr.sin_port = 0; // gets port dynamically

  if ((retval = bind(udp_data_fd, (sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
    ALOGE("%s: Failed to bind udp port for incoming data: %s", __func__, strerror(errno));
    return false;
  }


  struct sockaddr_in sock_addr;
  socklen_t  addr_len = sizeof(sock_addr);
  if (getsockname(udp_data_fd, (struct sockaddr *)&sock_addr, &addr_len) == 0) {
      udp_data_port_rx = ntohs(sock_addr.sin_port);
  }

  ALOGD("%s: UDP Socket(fd:%d) is ready to receive audio data on port %d",
        __func__, udp_data_fd, udp_data_port_rx);

  return true;
}

bool XpanSocketHandler::CreateUpdSocketForTsf(ipaddr_t ip) {
  ALOGD("%s", __func__);
  struct sockaddr_in servaddr;
  int retval = -1;

  udp_tsf_fd = socket(AF_INET,SOCK_DGRAM, 0);
  if (udp_tsf_fd < 0) {
    ALOGE("%s: Udp Socket creation failed (%s)", __func__, strerror(errno));
    return false;
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET ;
  inet_pton(AF_INET, ip.toString().c_str(), &servaddr.sin_addr.s_addr);
  servaddr.sin_port = 0; // gets port dynamically

  if((retval = bind(udp_tsf_fd, (sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
    ALOGE("%s: Failed to bind udp port (%s)", __func__, strerror(errno));
    return false;
  }

  struct sockaddr_in sock_addr;
  socklen_t  addr_len = sizeof(sock_addr);
  if (getsockname(udp_tsf_fd, (struct sockaddr *)&sock_addr, &addr_len) == 0) {
      udp_tsf_port_rx = ntohs(sock_addr.sin_port);
  }

  ALOGD("%s: UDP Socket(fd:%d) is ready to receive audio tsf sync data on port %d",
        __func__, udp_tsf_fd, udp_tsf_port_rx);

  return true;
}

void XpanSocketHandler::CloseTsfRxUdpPort() {
  ALOGD("%s: fd to close %d", __func__, udp_tsf_fd);
  if (udp_tsf_fd > 0) {
    close(udp_tsf_fd);
    ALOGD("%s: fd %d closed", __func__, udp_tsf_fd);
    udp_tsf_fd = -1;
  }
}

void XpanSocketHandler::CloseDataRxUdpPort() {
  ALOGD("%s: fd to close %d ", __func__, udp_data_fd);
  if (udp_data_fd > 0) {
    close(udp_data_fd);
    ALOGD("%s: fd %d closed", __func__, udp_data_fd);
    udp_data_fd = -1;
  }
}

void XpanSocketHandler::CloseInternalPipeFds() {
  if (hdlr_pipe_fd[1] > 0) {
    ALOGD("%s: Closing Write pipe end %d", __func__, hdlr_pipe_fd[1]);
    close(hdlr_pipe_fd[1]);
    hdlr_pipe_fd[1] = -1;
  }

  if (hdlr_pipe_fd[0] > 0) {
    ALOGD("%s: Closing Read pipe end %d", __func__, hdlr_pipe_fd[0]);
    close(hdlr_pipe_fd[0]);
    hdlr_pipe_fd[0] = -1;
  }
}

void XpanSocketHandler::HandleRxDateParsingFailure(int fd, uint8_t ret) {
  ALOGD("%s: fd = %d return value = %d", __func__, fd, ret);

  std::vector<uint8_t> cmd_data;
  const uint16_t length = 3;
  cmd_data.push_back((uint8_t)(0xFF & XPAN_LMP_MSG));
  addUint16ToData(cmd_data, length);
  cmd_data.push_back((uint8_t)(0xFF & XPAN_LMP_NOT_ACCEPTED));
  addUint16ToData(cmd_data, (0xFFFF & ret));

  if (ret == XPAN_CONNECTION_ALREADY_EXISTS ||
      ret == XPAN_LMP_UNKNOWN_PDU ||
      ret == XPAN_LMP_INVALID_PARAMS) {
    void *buf = (void *)cmd_data.data();
    size_t len = (size_t)cmd_data.size();

    ssize_t ret_size = send(fd, buf, len, 0);
    if (ret_size <= 0) {
      ALOGE("%s: Failed to write data error: %s", __func__, strerror(errno));
    }
  }

  if (ret == XPAN_CONNECTION_ALREADY_EXISTS ||
      ret == XPAN_CONNECTION_FAILED_TO_ESTABLISH ||
      ret == XPAN_CONN_DOESNT_EXIST) {
    // remove fd frm fd set
    ALOGE("%s: remove fd %d from FD_SET(Invalid Lmp Connect Attempt)", __func__);
    UpdateFdSet(false, fd);  // remove fd from fd set
    close(fd); // close the socket
  }
}

void XpanSocketHandler::HandleRemoteDisconnection(int fd) {
  // Remote the FD from FD Set
  UpdateFdSet(false, fd);

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
    return;
  }

  ipaddr_t ip = xac->GetIpFromFd(fd);
  if (ip.isEmpty()) {
    ALOGE("%s: No IP associated with fd (%d)", __func__, fd);
    return;
  }

  XpanDevice *device = xac->GetDeviceByIpAddr(ip);
  if (!device) {
    ALOGE("%s: No associated device instance with ip (%s)",
          __func__, ip.toString().c_str());
    return;
  }

  bdaddr_t addr = device->GetAddr();
  XpanEarbudRole role = device->GetRoleByIpAddr(ip);

  // Send Remote Disconnected event to state machine
  XpanTcpDisconnectionEvt *msg =
    (XpanTcpDisconnectionEvt *)malloc(sizeof(XpanTcpDisconnectionEvt));
  
  if (msg == NULL) {
      ALOGE("%s: failed to allocate memory", __func__);
      return;
  }
  msg->event = XPAN_TCP_DISCONNECTED_EVT;
  msg->addr = addr;
  msg->ip = ip;
  msg->role = role;
  msg->status = LOCAL_USER_TERMINATED_CONNECION;

  xac->PostMessage((xac_handler_msg_t *)msg);
}

void XpanSocketHandler::ResetFdSet() {
  FD_ZERO(&rfds);

  for (int i = 0; i < mReadFds.size(); i++) {
    if (mReadFds[i] >= FD_SETSIZE || mReadFds[i] < 0 ||
        (fcntl(mReadFds[i], F_GETFD) < 0)) {
      ALOGE("%s: Invalid fd = %d", __func__, mReadFds[i]);
      UpdateFdSet(false, mReadFds[i]);
      continue;
    }
    FD_SET(mReadFds[i], &rfds);
  }
}

/* Rx Handler API's */
bool XpanSocketHandler::InitiateRxRoutine() {
  ALOGD("%s", __func__);
  int retval = -1;

  int status = pipe(hdlr_pipe_fd);
  if (status != XPAN_AC_SUCCESS) {
    ALOGE("%s: Pipe creation failed", __func__);
    return false;
  }

  ALOGD("%s: read fd = %d, write fd = %d", __func__,
        hdlr_pipe_fd[0], hdlr_pipe_fd[1]);

  mReadFds.clear();
  FD_ZERO(&rfds);
  FD_SET(hdlr_pipe_fd[0], &rfds);
  nfds++;
  mReadFds.push_back(hdlr_pipe_fd[0]);

  mIsRxRoutineRunning = true;

  do {
    ResetFdSet();
    int max_fd = GetLastFd();
    retval = select((max_fd + 1), &rfds, NULL, NULL, NULL);

    ALOGD("%s: select() -> retval = %d", __func__, retval);

    switch (retval) {
      case EBADF:
           ALOGE("%s: select Error: An invalid fd given in the set", __func__);
           break;

      case EINTR:
           ALOGE("%s: select Error: A signal was caught", __func__);
           break;

      case EINVAL:
           ALOGE("%s: select Error: nfds is negative or exceeds the limit"
                 "or Invalid timeout mentioned ", __func__);
           break;

      case ENOMEM:
           ALOGE("%s: Unable to allocate memory for internal tables", __func__);
           break;

      default: {
        for (int i = 0; i < nfds; i++) {
          if (mReadFds[i] >= FD_SETSIZE || mReadFds[i] < 0) {
            ALOGE("%s: Invalid fd = %d", __func__, mReadFds[i]);
            continue;
          }
          if (FD_ISSET(mReadFds[i],&rfds)) {
            /* Identify this fd */
            ALOGD("%s: Event on fd = %d", __func__, mReadFds[i]);

            /* fd for internal events like add/remove fd from select */
            if (mReadFds[i] == hdlr_pipe_fd[0]) {
              ALOGD("%s: Internal event to manage fds", __func__);
              // TODO: parse message to add/remove fd
              char msg[FD_MSG_LEN];
              int len = read(hdlr_pipe_fd[0], msg, FD_MSG_LEN);

              /* Check if this is Rx routine termination request */
              if (!strcmp(msg, "StopRxThreadRoutine")) {
                HandleRxThreadTermination();
                break;
              }

              std::vector<std::string> fdact_list = GetTokens("*", msg);
              for (int l = 0; l < fdact_list.size(); l++) {
                HandleUpdateReadFds(fdact_list[l].data(), fdact_list[l].length());
              }

            /* Incoming TCP connection */
            } else if (mReadFds[i] == tcp_lfd) {
              ALOGD("%s: Incoming connection event", __func__);
              AcceptIncomingTcpConnection();

            /* TCP data received */
            } else {
              uint8_t buffer[XPAN_CTR_DATA_LEN];
              ssize_t data_len = 0;
              if (XpanApplicationController::IsTLSEnabled()) {
                int read_status, data_cnt;
                read_status = ioctl(mReadFds[i], FIONREAD, &data_cnt);
                if (data_cnt == 0) {
                  int rval = CheckSocketStatus(mReadFds[i]);
                  if (rval < 0) {
                    HandleRemoteDisconnection(mReadFds[i]);
                  }
                  if (rval <= 0) continue; /*continue if select has more(>1) fds to process */
                }
                XpanApplicationController *xac = XpanApplicationController::Get();
                if (!xac) {
                  ALOGE("%s: Xpan Application Controller is not initialized.", __func__);
                  continue;
                }
                ipaddr_t ip = xac->GetIpFromFd(mReadFds[i]);
                if (ip.isEmpty()) {
                  ALOGE("%s: Couldn't find IP associated with fd %d", __func__, mReadFds[i]);
                  continue;
                }
                XpanTLSHandler *tlsHandler = xac->GetTLSHandler(ip);
                if (!tlsHandler) {
                  ALOGE("%s: Couldn't find TLS Handler associated with ip %s", __func__,
                        ip.toString().c_str());
                  continue;
                }
                if (tlsHandler->GetTcpRole() == XPAN_TCP_CLIENT) {
                  data_len = tlsHandler->ClientReadData(buffer);
                } else if (tlsHandler->GetTcpRole() == XPAN_TCP_SERVER) {
                  data_len = tlsHandler->ServerReadData(buffer);
                } else {
                  ALOGE("%s: Data not read in unknown TCP role", __func__);
                }
              } else {
                data_len = recv(mReadFds[i], &buffer, sizeof(buffer), 0);
              }
              ALOGD("%s: TCP data receieved. len = %d ", __func__, data_len);
              if (data_len <= 0) {
                HandleRemoteDisconnection(mReadFds[i]);
                // TODO: Possible disconnection
              } else {
                ipaddr_t ip = sockaddr_to_ip(mReadFds[i]);
                if (ip.isEmpty()) {
                  ALOGE("%s: not a valid ip address. Ignore data", __func__);
                  continue;
                }
                bool ret = XpanLmpManager::parseXpanEvent(buffer, data_len, ip, mReadFds[i]);
                if (ret != XPAN_AC_SUCCESS) {
                  HandleRxDateParsingFailure(mReadFds[i], ret);
                }
              }
            }
          }
        }
      }
    }
  } while (mIsRxRoutineRunning);
  /*close pipe fds in same context in which those were created */
  XpanSocketHandler::CloseInternalPipeFds();
  ALOGD("%s: Rx Thread Routine finished", __func__);

  return true;
}

void XpanSocketHandler::SetTlsHandler(XpanTLSHandler *tls_handler) {
  mTlsHandler = tls_handler;
}

void XpanSocketHandler::SetServerTlsHandler(XpanTLSHandler *tls_handler) {
  mTlsHandlerServer = tls_handler;
}

void XpanSocketHandler::SetTcpRole(XpanTcpRole role) {
  if (role != XPAN_TCP_CLIENT && role != XPAN_TCP_SERVER) {
    ALOGE("%s: Invalid role to set %d", __func__, role);
    return;
  }
  mTcpRole = role;
}

void XpanSocketHandler::SetMasterFdForConnection(int fd) {

}

/* Stops ARP retry if state machine has canceles connnection/transition procedure */
void XpanSocketHandler::StopArpRetry(bool stop_arp_retry) {
  ALOGD("%s: stop_arp_retry = %d role = %d", __func__, stop_arp_retry, mRole);
  mStopArpRetry = true;
}

/* Sets the tcp_fd after incoming connection is established */
void XpanSocketHandler::SetSocketFd(int fd) {
  tcp_fd = fd;
}

void XpanSocketHandler::UpdateFdSet(bool add, int fd) {
  ALOGD("%s: add = %d, fd = %d", __func__, add, fd);
  if (add) {
    if (fd >= FD_SETSIZE || fd < 0 || (fcntl(fd, F_GETFD) < 0)) {
      ALOGE("%s: Invalid fd = %d", __func__, fd);
      return;
    }
    nfds++;
    //add fd to the list
    mReadFds.push_back(fd);
  } else {
    if (std::find(mReadFds.begin(), mReadFds.end(), fd) != mReadFds.end()) {
      nfds--;
      //remove fd from list
      mReadFds.erase(std::remove(mReadFds.begin(), mReadFds.end(), fd), mReadFds.end());
    } else {
      ALOGD("%s: fd not present in the set", __func__);
    }
  }
}

bool XpanSocketHandler::ContainsListener(bdaddr_t addr) {
  std::vector<bdaddr_t>::iterator it = mListeners.begin();
  while (it != mListeners.end()) {
    if (*it == addr) {
      return true;
    } else {
      it++;
    }
  }
  return false;
}

bool XpanSocketHandler::AddListenerFor(bdaddr_t addr) {
  for (int i = 0; i < mListeners.size(); i++) {
    if (mListeners[i] == addr) {
      ALOGW("%s: Already listening for %s", __func__, addr.toString().c_str());
      return true;
    }
  }
  mListeners.push_back(addr);
  return true;
}

bool XpanSocketHandler::RemoveListenerFor(bdaddr_t addr) {
  std::vector<bdaddr_t>::iterator it = mListeners.begin();
  while (it != mListeners.end()) {
    if (*it == addr) {
      /* Remove entry */
      it = mListeners.erase(it);
      ALOGD("%s: removed %s", __func__, ConvertRawBdaddress(addr));
      return true;
    } else {
      it++;
    }
  }
  return false;
}

void XpanSocketHandler::EnableTcpKeepAlive(int fd) {
  char xpan_prop[PROPERTY_VALUE_MAX];
  property_get("persist.vendor.qcom.btadvaudio.target.support.xpan_ping", xpan_prop, "false");
  if (!strcmp(xpan_prop, "true")) {
    ALOGI("%s: LMP Ping enabled. TCP Keepalive is disabled", __func__);
    return;
  }

  /* default values to be used for TCP keep alive*/
  int keep_cnt = 20; // 20 keepalive probe
  int keep_int = 1;  // 1 sec between each packet
  int keep_idle = 10; // 10 sec before first keepalive packet

  /* If Keep alive has to be manually configured for test purposes */
  char xpan_prop1[PROPERTY_VALUE_MAX];
  property_get("persist.vendor.qcom.btadvaudio.target.support.xpan_tcp_keep_count", xpan_prop1, "20");
  if (strcmp(xpan_prop1, "20") && atoi(xpan_prop1) > 0) {
    keep_cnt = atoi(xpan_prop1);
  }

  char xpan_prop2[PROPERTY_VALUE_MAX];
  property_get("persist.vendor.qcom.btadvaudio.target.support.xpan_tcp_keep_int", xpan_prop2, "1");
  if (strcmp(xpan_prop2, "1") && atoi(xpan_prop2) > 0) {
    keep_int = atoi(xpan_prop2);
  }

  char xpan_prop3[PROPERTY_VALUE_MAX];
  property_get("persist.vendor.qcom.btadvaudio.target.support.xpan_tcp_keep_idle", xpan_prop3, "10");
  if (strcmp(xpan_prop3, "10") && atoi(xpan_prop3) > 0) {
    keep_idle = atoi(xpan_prop3);
  }

  ALOGD("%s: TCPKeepAlive(fd %d), keep_cnt = %d, keep_int=%d, keep_idle=%d",
        __func__, fd, keep_cnt, keep_int, keep_idle);

   if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &keep_cnt, sizeof(int)) < 0) {
     ALOGE("%s: failed to set keepalive count", __func__);
     return;
   }

   if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(int))) {
     ALOGE("%s: failed to set keepidle time", __func__);
     return;
   }

   if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_int, sizeof(int))) {
     ALOGE("%s: failed to set keepidle time", __func__);
     return;
   }

  int keepalive = 1;
  int status = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive , sizeof(keepalive ));
  if (status != 0) {
    ALOGW("%s: failed to enable TCP keepalive for fd %d", __func__, fd);
  }

  /* TCP timeout value if no ACK received for TCP data packet.
    Moreover, when used with the TCP keepalive (SO_KEEPALIVE) option,
    TCP_USER_TIMEOUT will override keepalive to determine when
    to close a connection due to keepalive failure.*/
  int tcp_timeout = (keep_idle * 1000) + (keep_int * keep_cnt * 1000);
  status = setsockopt(fd, IPPROTO_TCP , TCP_USER_TIMEOUT,
                      &tcp_timeout, sizeof(tcp_timeout));
  ALOGD("%s: TCP_USER_TIMEOUT(%d) for fd (%d) status = %d", __func__, tcp_timeout, fd, status);

}


/* This API assigns IP TOS (DSCP Expedited forwarding) at protocol level and
   SOL_SOCKET SO_PRIORITY as highest to a given XPAN TCP socket*/
bool XpanSocketHandler::SetSocketTosOption(int fd) {
  /* Setting DSCP Class Selector CS7 to XPAN Control traffic */
  int CS7_TOS = 0xE0;
  int getiptos = -1, getsoprio = -1;
  int rc = setsockopt(fd, IPPROTO_IP, IP_TOS,
                      &CS7_TOS, sizeof(int));
  socklen_t optlen = sizeof(int);
  getsockopt(fd, IPPROTO_IP, IP_TOS, (void *) &getiptos, &optlen);
  ALOGD("%s: socket option IP TOS = %d status = %d", __func__, getiptos, rc);

  int sockprio = 6;
  rc = setsockopt(fd, SOL_SOCKET, SO_PRIORITY, &sockprio, optlen);
  getsockopt(fd, SOL_SOCKET, SO_PRIORITY, (void *) &getsoprio, &optlen);
  ALOGD("%s: socket option SOL_SOCKET = %d status = %d", __func__, getsoprio, rc);

  return true;
}

/* This API check if Nagles algorith is enabled */
bool XpanSocketHandler::DisableDelayedPacketTransfer(int fd) {
  /* Check if Nagles Algorithm is enabled by deafult*/
  int tcpNoDelay;
  socklen_t tcpNoDelayLen = sizeof(tcpNoDelay);
  int res = getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &tcpNoDelay, &tcpNoDelayLen);
  if (res < 0) {
    ALOGE("%s: Unable to find TCP_NODELAY", __func__);
    return false;
  }

  ALOGD("%s: TCP_NODELAY is currently %s", __func__, (tcpNoDelay > 0 ? "enabled":"disabled"));
  if (!tcpNoDelay) {
    // Turn on TCP no delay (disable Nagles Algorithm)
    tcpNoDelay = 1;
    res = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&tcpNoDelay, sizeof(tcpNoDelay));
    if (res < 0) {
      ALOGE("%s: Unable to enable TCP_NODELAY", __func__);
      return false;
    }

    ALOGD("%s: TCP_NODELAY enabled", __func__);
  }

  return true;
}

/* This API assigns sets nterface for binding to socket as wlan0 */
bool XpanSocketHandler::SetSocketIfOption(int fd) {
  char *ifname = "wlan0"; // the name of the interface to bind to, e.g. "eth0"
  struct ifreq ifr; // the interface request structure
  // set the interface name in the ifr structure
  memset(&ifr, 0, sizeof(ifr));
  snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
  // set the socket option to bind to the device
  if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) != 0) {
    ALOGE("%s: failed to set socket option err = %s", __func__, strerror(errno));
    return false;
  } else {
    struct ifreq getintf;
    socklen_t optlen = sizeof(ifr);
    getsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, (void *) &getintf, &optlen);
    ALOGD("%s: socket interface set to = %s", __func__, getintf.ifr_name);
  }
  return true;
}

/* Checks if remote is unreachable
   Returns,
     value > 0, if remote is reachable and has data to read
     value < 0, if remote is unreachable
     value = 0, retry read again
   */
int XpanSocketHandler::CheckSocketStatus (int fd) {
  uint8_t buffer[XPAN_CTR_DATA_LEN];
  errno = 0;
  int ret = recv(fd, &buffer, sizeof(buffer), MSG_PEEK|MSG_DONTWAIT);
  ALOGD("%s: read count = %d, error = %d (%s)", __func__, ret, errno, strerror(errno));
  if (ret != 0) {
    if (ret == -1 && errno == EAGAIN) {
      return 0;
    }
    // If there is data to read, it returns positive value
    // If there is error with negetive value, return negetive val for disconnection
    return ret;
  }

  if (ret == 0) {
    switch (errno) {
      case EBADF:        /* Bad file descriptor */
      case ENOTCONN:     /* Transport endpoint is not connected */
      case ENOTSOCK:     /* Socket operation on non-socket */
      case ECONNREFUSED: /* Connection refused */
      case ECONNABORTED: /* Software caused connection abort */
      case ECONNRESET:   /* Connection reset by peer */
        ALOGE("%s: socket error: %s", __func__, strerror(errno));
        return -1;
      default:
        ALOGE("%s: other socket error: %s", __func__, strerror(errno));
    }
  }

  return ret;
}

bool ipStringToIpAddr(const char *str, ipaddr_t *addr) {
  if (!str || !addr) {
    ALOGE("%s: Invalid string to convert to ip address", __func__);
    return false;
  }

  //TODO: validate that the string is IP address
  if (sscanf(str, "%hu.%hu.%hu.%hu",
      &addr->ipv4[0], &addr->ipv4[1], &addr->ipv4[2], &addr->ipv4[3]) != 4) {
    ALOGE("%s: Error while parsing string to IP address", __func__);
    return false;
  }

  return true;
}

}
}
