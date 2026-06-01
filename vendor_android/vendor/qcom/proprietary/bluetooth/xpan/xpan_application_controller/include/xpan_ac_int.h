/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc. 
 */

/* This file contains declarations internal to Xpan Application Controller Module */
//#pragma once

#ifndef XPAN_AC_INT
#define XPAN_AC_INT

#include <atomic>
#include <deque>
#include <signal.h>
#include <stdint.h>
#include <thread>
#include <vector>
#include "xpan_utils.h"
#include "qhci_ac_if.h"
#include "xm_xac_if.h"
#include "xpan_ac_tls_int.h"

using xpan::implementation::XpanQhciAcIf;
using xpan::implementation::XMXacIf;

namespace xpan {
namespace ac {

#define XPAN_AC_SUCCESS                 0x00
/* Unknown Connection Identifier - Used when connection cancelled before its established */
#define UNKNOWN_CONNECTION_IDENTIFIER   0x02
/* mapped to spec - Connection Timeout */
#define XPAN_CONNECTION_TIMEOUT         0x08
/* mapped to spec - Connection Failed to be Established */
#define XPAN_TCP_CONNECTION_FAILED      0x3E
#define XPAN_CONNECTION_FAILED_TO_ESTABLISH      0x3E  // intentionally marked to same value
/* mapped to spec - CONNECTION REJECTED DUE TO UNACCEPTABLE BD_ADDR (0x0F) */
#define XPAN_LMP_CONN_REJECTED_INVALID_DEVICE    0x0F
/* mapped to spec - Connection Rejected Due To Security Reasons */
#define XPAN_TLS_ESTABLISHEMENT_FAILED  0x0E
/* mapped to spec - Connection Already Exists */
#define XPAN_CONNECTION_ALREADY_EXISTS 0x0B
/* XPAN MDNS Query and registration Failed
   mapped to spec - Connection Rejected due to Limited Resources (0x0D)*/
#define XPAN_CONN_REJECTED_LIMITED_RSRC 0x0D
/* Remote user terminated connection */
#define REMOTE_USER_TERMINATED_CONNECTION 0x13
/* Local user terminated connection */
#define LOCAL_USER_TERMINATED_CONNECION 0x16
/* Unknown LMP Operation*/
#define XPAN_LMP_UNKNOWN_PDU 0x19
/* Invalid LMP Parameters */
#define XPAN_LMP_INVALID_PARAMS 0x1E

/* XPAN AC Error Codes (Range - 0xD0 to 0xFF )*/
#define INVALID_DATA_LENGTH 0xD0
#define XPAN_CONN_DOESNT_EXIST 0xD1
#define XPAN_INVALID_REQUEST 0xD2

/* Internal fd management message length */
#define FD_MSG_LEN 32

/* Time to receive connection from BT Host after target adv event */
#define XPAN_AC_TARGETED_CONN_TIMER 2000 // in ms

/* Periodic timer to send Ping request */
#define XPAN_PERIODIC_PING_TIMER 10500 // in ms

/* Supervision timeout for TCP link */
#define XPAN_TCP_SUPERVISION_TIMEOUT 30000 // in ms

/* Max Lmp Data Size received */
#define XPAN_CTR_DATA_LEN 1024

/* Message priority in AC thread context*/
#define MSG_PRIORITY_DEFAULT 0
#define MSG_PRIORITY_HIGH 1

/* totla number of earbuds */
#define NO_OF_EARBUDS 2

/* Key length for ltk, psk etc*/
#define KEY_LEN 16

/* Max Suported Xpan Connections */
#define MAX_SUPPORTED_DEV 10

/* Xpan AC Timer Timeout function */
using xpan_ac_notify_cb = std::function<void(void *)>;

/* XPAN AC MDNS Query Timeout - 32 Sec */
#define XPAN_AC_MDNS_QUERY_TIMEOUT 32000 // in ms

/* XPAN AC Filtered scan trigger timer - 2 sec after connection or LE->AP transition*/
#define XPAN_AC_FILTERED_SCAN_TRIGGER_TIMER 2000 // in ms

/* Time to complete bearer preference for idle transition */
#define XPAN_AC_BEARER_PREFERENCE_TIMER 25000 // in ms

/* Time to initiate outgoing LE connection explictely from phone */
#define XPAN_AC_REINIT_CONNECTION_TIMER 4000 // in ms

/* Default port number for XPAN TCP listening*/
#define XPAN_AC_LISTENING_TCP_PORT 64253

#define XPAN_SREC_FILEPATH "/data/vendor/bluetooth/dev_records.conf"

/* Xpan Device Security record */
typedef struct {
  bool used;
  bdaddr_t addr;
  mdns_uuid_t uuid;
  long feature_mask;
  uint8_t ltk[KEY_LEN];
  uint8_t psk[KEY_LEN];
} __attribute__((packed)) XpanDevSecRecord;

/* Xpan AC Timer Management */
class XpanAcTimer {
  private:
    std::string mName;
    xpan_ac_notify_cb mCallbackFunction;
    timer_t mTimer;
    bool mIsTimerCreated;
    bool mIsTimerStarted;
    uint32_t mTimeoutMs;
    uint32_t mPeriodicTimerMs;
    void *cb_data;

    void Create();

  public:

    XpanAcTimer(std::string name, xpan_ac_notify_cb cb, void *data);
    ~XpanAcTimer();

    bool StartTimer(uint32_t);
    void SetTimerInterval(uint32_t);
    bool StopTimer();
    bool DeleteTimer();
    static void TimerCallback(union sigval arg);
};

/* Transport Types used for LMP Bearer Switch procedures */
#define LMP_TP_BREDR 0x01
#define LMP_TP_LE    0x02
#define LMP_TP_P2P   0x04
#define LMP_TP_AP    0x08

/* Reasons for LE->AP transition failure before LMP BS procedure */
#define NO_ROUTE_TO_HOST 0x00
#define TCP_HANDSHAKE_FAILED 0x01
#define LMP_CONNECTION_FAILURE 0x02
#define BEARER_SWITCH_FAILURE 0x03


/* Role Switch LMP Operations*/
typedef enum {
  PRIMARY_SWITCH_START,
  PRIMARY_SWITCH_COMPLETE,
  PRIMARY_SWITCH_CANCEL,
} XPAN_PRIMARY_SWITCH_OP;

/* MDNS Discovery Status */
typedef enum {
  MDNS_STATUS_SUCCESS, // started or stopped successfully
  MDNS_DISCOVERY_FAILED_TO_START,
} MDNS_DISCOVERY_STATUS;

/* MDNS operation */
typedef enum {
  MDNS_QUERY_STOP,
  MDNS_QUERY_START,
  MDNS_UNREGISTER = 0,
  MDNS_REGISTER,
  MDNS_SERVICE_UNREGISTERED,
  MDNS_SERVICE_REGISTERED,
} MDNS_OP;

typedef enum {
  STREAM_STOPPED_BY_PAUSE = 1,
  STREAM_AUDIO_SERVER_EXCEPTION,
  STREAM_BAP_TIMEOUT,
} STREAM_STATUS;

/* XPAN AC Main Thread Events */
typedef enum {
  /* XPAN Manager triggered Events */
  XM_PREPARE_BEARER_EVT,
  XM_BEARER_PREFERENCE_EVT,
  XM_UPDATE_REMOTE_AP_PARAMS_EVT,
  XM_REQUEST_REMOTE_AP_PARAMS_EVT,
  XM_INITIATE_LMP_BEARER_SWITCH_EVT,
  XM_BEARER_SWITCH_IND_EVT,
  XM_MDNS_DISCOVERY_STATUS_EVT,
  XM_UPDATE_BOND_STATE_EVT,
  XM_UPDATE_LOCAL_AP_DETAILS_EVT,
  XM_GET_LOCAL_LISTENING_PORTS_EVT,
  XM_UPDATE_BONDED_XPAN_DEVICES_EVT,
  XM_LE_TO_AP_URGENCY_EVT,
  XM_SET_FEATURE_MASK_EVT,

  /* QHCI triggered events */
  QHCI_CREATE_CONNECTION_EVT,
  QHCI_CREATE_CONNECTION_CANCEL_EVT,
  QHCI_DISCONNECT_CONNECTION_EVT,
  QHCI_GET_REMOTE_VERSION_EVT,
  QHCI_GET_REMOTE_LE_FEATURES_EVT,
  QHCI_SET_LOCAL_VERSION_EVT,
  QHCI_SET_LOCAL_LE_FEATURES_EVT,
  QHCI_ENABLE_ENCRYPTION_EVT,
  QHCI_SEND_ACL_DATA_EVT,
  QHCI_L2CAP_PAUSE_UNPAUSE_RES_EVT,
  QHCI_LE_LINK_CONNECTION_STATUS_EVT,
  QHCI_STREAM_STATUS_UPDATE_EVT,

  /* LMP triggered events */
  XPAN_LMP_OUT_CONNECTION_REQ,
  XPAN_LMP_INC_CONNECTION_REQ_EVT,
  XPAN_LMP_CONNECTION_RES_EVT,
  XPAN_LMP_VERSION_REQ_EVT,
  XPAN_LMP_VERSION_RES_EVT,
  XPAN_LMP_LE_FEATURE_REQ_EVT,
  XPAN_LMP_LE_FEATURE_RES_EVT,
  XPAN_LMP_REMOTE_PING_REQ_EVT,
  XPAN_LMP_L2CAP_PAUSE_UNPAUSE_RES_EVT,
  XPAN_LMP_PREPARE_BEARER_RES_EVT,
  XPAN_LMP_BEARER_SWITCH_RES_EVT,
  XPAN_LMP_SWITCH_PRIMARY_EVT,
  XPAN_LMP_ACL_DATA_EVT,

  /* Local Xpan Application Controller Events */
  XPAN_TCP_CONNECTED_EVT,
  XPAN_TCP_CONNECT_FAILED,
  XPAN_TCP_DISCONNECTED_EVT,
  XPAN_REMOTE_DISCONNECTED_EVT,
  XPAN_WIFI_AP_DISCONNECTED,
  XPAN_TCP_NEW_AP_TRANSPORT_STATUS,
  XPAN_REMOTE_DETAILS_NOT_FOUND_EVT,
  XPAN_TLS_ESTABLISHED_EVT,
  XPAN_LMP_EXECUTE_NEXT_CMD,
  XPAN_BEARER_PREFERENCE_TIMEOUT,
  XPAN_PRIMARY_DISCONNECTING_EVT,
  XPAN_TARGETED_ADV_CONN_TIMEOUT,
  XPAN_RESET_SUPERVISION_EVT,
} XacEvent;

/* XPAN AC Main Thread Event Data Structures */
/* Prepare Audio Bearer Request frOM XM */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  TransportType bearer;
  bool urgency;
} __attribute__((packed)) XmPrepareBearer;

/* Bearer Preference Event for LMP Bearer Switch in IDLE State */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  TransportType bearer;
  bool is_urgent;
} __attribute__((packed)) XmBearerPreference;

/* Remote AP params Update on IP Notification or MDNS response */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  bool is_mdns_update;
  mdns_uuid_t mdns_uuid;
  macaddr_t eb_bssid;
  uint32_t remote_tcp_port;
  uint32_t remote_udp_port;
  uint32_t remote_udp_tsf_port;
  uint8_t num_of_earbuds;
  XpanEarbudRole role[NO_OF_EARBUDS];
  ipaddr_t ip[NO_OF_EARBUDS];
  macaddr_t mac[NO_OF_EARBUDS];
  uint32_t audio_loc[NO_OF_EARBUDS];
}  XmUpdateRemoteApParams;

/* Request Remote AP Params from XM */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
} __attribute__((packed)) XmRequestRemoteParams;

/* Request to get Remote AP Details from XM */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  uint32_t usage;
  void *context;
} __attribute__((packed)) XmReqRemoteApParams;


/* Initiate Lmp Bearer Switch Procedure Call from XM */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  TransportType bearer;
} __attribute__((packed)) XmInitiateLmpBearerSwitch;

/* Remote Disconnected event*/
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  XpanEarbudRole role;
} __attribute__((packed)) XmRemoteDisconnectedEvent;

/* XM- Bearer Switch Indication */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  TransportType bearer;
  uint8_t status;
  uint8_t reason;
} __attribute__((packed)) XmBearerSwitchInd;

/* MDNS Discovery Status from Profile */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  uint8_t status;
  mdns_uuid_t uuid;
  MDNS_OP state;
} __attribute__((packed)) XmMdnsDiscoveryStatus;

/* Bond State Updates from Profile */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  BondState state;
} __attribute__((packed)) XmUpdateBondState;

/* Local Ap details from Profile */
typedef struct {
  XacEvent event;
  macaddr_t local_mac_addr;
  macaddr_t bssid;
  ipaddr_t ipAddr;
  mdns_uuid_t uuid;
  uint32_t freq;
} __attribute__((packed)) XmUpdateLocalApDetails;

/* Local HS port request event */
typedef struct {
  XacEvent event;
} __attribute__((packed)) XmGetLocalListeningPorts;

/* Bonded Device list event from profile */
typedef struct {
  XacEvent event;
  uint8_t numOfDevices;
  bdaddr_t *devices;
} __attribute__((packed)) XmUpdateBondedXpanDevices;

/* Set-Unset urgency for LE->Ap transition */
typedef struct {
  XacEvent event;
  bool is_urgent;
} __attribute__((packed)) XmLeApUrgencyEvent;

/* Set-feature mask to be supported for the remote device */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  long feature_mask;
} __attribute__((packed)) XmSetFeatureMask;


/* Create Connection event*/
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  uint16_t supervision_timeout;
  bool is_bg = false;
} __attribute__((packed)) QhciCreateConnection;

/* Create Connaection cancel event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
} __attribute__((packed)) QhciCreateConnectionCancel;

/* Disconnect Connection event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
} __attribute__((packed)) QhciDisconnectConnection;

/* get remote LMP version event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
} __attribute__((packed)) QhciGetRemoteVersion;

/* Get remote LE features request event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
} __attribute__((packed)) QhciGetRemoteLeFeatures;

/* Set HS LMP version event */
typedef struct {
  XacEvent event;
  uint8_t version;
  uint16_t companyId;
  uint16_t subversion;
} __attribute__((packed)) QhciSetLocalVersionEvt;

/* local version details */
typedef struct {
  uint8_t version;
  uint16_t companyId;
  uint16_t subversion;
} __attribute__((packed)) LocalVersionInfo;

/* Set local LE features event */
typedef struct {
  XacEvent event;
  uint64_t le_features;
} __attribute__((packed)) QhciSetLocalLeFeaturesEvt;

/* Enable Encryption event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  uint8_t ltk[16];
} __attribute__((packed)) QhciEnableEncrption;

/* ACL data from BT Host */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  uint8_t llid;
  uint16_t len;
  uint8_t *data;
} __attribute__((packed)) QhciSendAclData;

/* Pause-Unpause reponse from QHCI */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  TransportType bearer;
  uint8_t action;
  uint8_t status;
} __attribute__((packed)) QhciL2capPauseUnpauseRes;

/* LE Link status event from QHCI */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  uint8_t state;
  TransportType qhci_active_transport;
} __attribute__((packed)) QhciLeLinkConnectionStatus;

/* stream status event from QHCI */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  uint8_t status;
} __attribute__((packed)) QhciStreamStatusEvt;

/* LMP Command Events */
/* LMP Connected Event*/
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  XpanEarbudRole role;
  uint8_t status;
} __attribute__((packed)) XpanLmpConnectionResEvt;

/* LMP Incoming Connection Request Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  int fd;
  uint32_t local_tcp_port;
  mdns_uuid_t remote_mdns_uuid;
  uint8_t link_type;
  uint8_t cod[3];
} __attribute__((packed)) XpanLmpIncomingConnReqEvt;

/* LMP Outgoing Connection Request */
typedef struct {
  XacEvent event;
  ipaddr_t ip;
  uint32_t remote_tcp_port;
  mdns_uuid_t local_mdns_uuid;
  uint8_t link_type;
  uint8_t cod[3];
} __attribute__((packed)) XpanLmpOutgoingConnReq;

/* LMP LE Features Request Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  mdns_uuid_t mdns_uuid;
  uint64_t le_features;
} __attribute__((packed)) XpanLmpFeatureReqEvt;

/* LMP LE Features Response Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  mdns_uuid_t mdns_uuid;
  uint64_t le_features;
} __attribute__((packed)) XpanLmpFeatureResEvt;

/* LMP version Request Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  mdns_uuid_t mdns_uuid;
  uint8_t version;
  uint16_t company_id;
  uint16_t subversion;
} __attribute__((packed)) XpanVersionReqEvt;

/* LMP LE Features Response Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  mdns_uuid_t mdns_uuid;
  uint8_t version;
  uint16_t company_id;
  uint16_t subversion;
} __attribute__((packed)) XpanVersionResEvt;

/* XPAN LMP PING Request Event (from remote) */
typedef struct {
  XacEvent event;
  ipaddr_t ip;
} __attribute__((packed)) XpanLmpRemotePingReqEvt;

/* LMP Pause Unpause Response Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  uint8_t action;
  uint8_t status;
} __attribute__((packed)) XpanL2capPauseUnpauseResEvt;

/* LMP Prepare Bearer Response Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  uint8_t status;
} __attribute__((packed)) XpanPrepareBearerResEvt;

/* LMP Bearer Switch Response Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  uint8_t status;
} __attribute__((packed)) XpanBearerSwitchResEvt;

/* LMP Primary Switch Request Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  uint8_t operation;
  bool reject = false;
} __attribute__((packed)) XpanPrimarySwitchEvt;

/* ACL data received over LMP */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  uint8_t llid;
  ipaddr_t ip;
  uint16_t len;
  uint8_t* data;
} __attribute__((packed)) XpanAclDataRecvdEvt;

/* Local Xpan Application Controller Events */
/* Event generated for TCP Connection */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  XpanEarbudRole role;
  int fd;
  bool isIncoming = false;
} __attribute__((packed)) XpanTcpConnectedEvt;

/* Event generated for TCP Connection failure */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  XpanEarbudRole role;
  uint8_t reason;
  bool isOutgoing = false;
} __attribute__((packed)) XpanTcpConnectionFailedEvt;

/* Event generated for TCP disconnection */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  XpanEarbudRole role;
  uint8_t status;
} __attribute__((packed)) XpanTcpDisconnectionEvt;

/* Event generated for TCP Connection failure */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
} __attribute__((packed)) XpanWifiDisconnectedEvt;


/* Event generated for TCP disconnection */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  XpanEarbudRole role;
  uint8_t status;
} __attribute__((packed)) XpanNewApTransportStatus;

/* Event generated after TLS session has been established */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  XpanEarbudRole role;
  uint8_t status;
} __attribute__((packed)) XpanTlsEstablishedEvt;

/* Event for undesired Incoming connection (unbonded device) */
/* Event generated for TCP disconnection */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  ipaddr_t ip;
  mdns_uuid_t mdns_uuid;
} __attribute__((packed)) XpanRemoteDetailsNotFound;

/* Execute next queued LMP Command if any */
typedef struct {
  XacEvent event;
  ipaddr_t ip;
} __attribute__((packed)) XpanLmpExecuteNextCmd;

/* Bearer Preference timeout event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  XpanEarbudRole role;
} __attribute__((packed)) XpanBearerPrefTimeout;

/* Primary Disconnected Event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
} __attribute__((packed)) XpanPrimaryDiscEvt;

/* Targeted adv Connection timeout event */
typedef struct {
  XacEvent event;
  bdaddr_t addr;
  XpanEarbudRole role;
} __attribute__((packed)) XpanTargetedConnTimeout;

typedef struct {
  XacEvent event;
  ipaddr_t ip;
} __attribute__((packed)) XpanResetSupervisionEvt;


/* List of events to be processed in XPAN AC thread context */
typedef union {
  XacEvent event;
  /* Xpan Manager Call Parameters*/
  XmPrepareBearer prepareBearer;
  XmBearerPreference bearerPreference;
  XmUpdateRemoteApParams remoteApParams;
  XmReqRemoteApParams reqRemoteApParams;
  XmInitiateLmpBearerSwitch initiateLmpBearerSwitch;
  XmRemoteDisconnectedEvent remoteDisconnectedEvent;
  XmBearerSwitchInd bearerSwitchInd;
  XmMdnsDiscoveryStatus mdnsDiscoveryStatus;
  XmUpdateBondState bondStateUpdate;
  XmUpdateLocalApDetails localApDetails;
  XmGetLocalListeningPorts getLocalListeningPorts;
  XmUpdateBondedXpanDevices xpanBondedDevices;
  XmLeApUrgencyEvent leApUrgency;
  XmSetFeatureMask setFeatureMask;

  /* QHCI Call Parameters */
  QhciCreateConnection createConn;
  QhciCreateConnectionCancel createConnCancel;
  QhciDisconnectConnection disconn;
  QhciGetRemoteVersion getRemoteVersion;
  QhciGetRemoteLeFeatures getRemoteLeFeatures;
  QhciSetLocalVersionEvt setLocalVersion;
  QhciSetLocalLeFeaturesEvt setLocalLeFeat;
  QhciEnableEncrption encryptionParams;
  QhciSendAclData aclDataParams;
  QhciL2capPauseUnpauseRes l2capPauseUnpauseRes;
  QhciLeLinkConnectionStatus leLinkStatus;
  QhciStreamStatusEvt streamStatus;

  /* Events from remote (LMP to application Controller) */
  XpanLmpConnectionResEvt lmpConnectRsp;
  XpanLmpOutgoingConnReq outLmpConnReq;
  XpanLmpIncomingConnReqEvt incLmpConnReq;
  XpanLmpFeatureReqEvt featReq;
  XpanLmpFeatureResEvt featRsp;
  XpanVersionReqEvt versionReq;
  XpanVersionResEvt versionRsp;
  XpanLmpRemotePingReqEvt pingReq;
  XpanL2capPauseUnpauseResEvt l2cPauseUnpause;
  XpanPrepareBearerResEvt prepareBearerRsp;
  XpanBearerSwitchResEvt bearerSwitchRsp;
  XpanPrimarySwitchEvt primarySwitchReq;
  XpanAclDataRecvdEvt data;

  /* Events from Socket Manager to AC*/
  XpanTcpConnectedEvt tcpConnected;
  XpanTcpConnectionFailedEvt tcpConnFailed;
  XpanNewApTransportStatus newApTransportStatus;
  XpanTcpDisconnectionEvt tcpDisconn;
  XpanWifiDisconnectedEvt apDisconn;
  XpanRemoteDetailsNotFound invalidRemote;
  XpanLmpExecuteNextCmd executeNext;
  XpanBearerPrefTimeout bearerPrefTimeout;
  XpanPrimaryDiscEvt primaryDiscon;
  XpanTargetedConnTimeout targetedConnTo;
  XpanResetSupervisionEvt resetSupervision;

} xac_handler_msg_t;

typedef struct {
  bool detailsSet = false;
  macaddr_t mac_addr;
  ipaddr_t ip;
  XpanEarbudRole role;
  uint32_t audio_location;
  uint32_t tcp_port;
  uint32_t udp_data_port;
  uint32_t udp_tsf_port;
  //state machine pointer
} __attribute__((packed)) RemoteDetails;

/* XPAN Application Controller State Machine States*/
typedef enum {
  XPAN_IDLE,
  XPAN_TCP_CONNECTING,
  XPAN_TCP_CONNECTED,
  XPAN_LMP_CONNECTING,
  XPAN_LMP_CONNECTED,
  XPAN_BEARER_SWITCH_PENDING,
  XPAN_AP_ACTIVE,
  XPAN_DISCONNECTING,
} XacSmState;

/* Trigger for the transition to XPAN_AP transport */
typedef enum {
  QHCI_CONNECTION,
  XM_PREPARE_AUDIO_BEARER,
  XM_BEARER_PREFERENCE,
} XpanSwitchTrigger;

/* Number of Earbuds Connected */
typedef enum {
  NONE_CONNECTED,
  PRIMARY_CONNECTED,
  BOTH_CONNECTED,
} XpanConnectedDevices;

/* XPAN WHC Transitions */
typedef enum {
  XPAN_LE_TO_AP_IDLE,
  XPAN_LE_TO_AP_STREAMING,
  XPAN_AP_TO_LE_IDLE,
  XPAN_AP_TO_LE_STREAMING,
  XPAN_AP_TO_AP_ROAMING_IDLE,
  XPAN_AP_TO_AP_ROAMING_STREAMING,
  XPAN_RECONNECTION_OUTGOING,
  XPAN_RECONNECTION_INCOMING,
  XPAN_AP_IDLE_TO_STREAMING,
} XpanTransitionType;

typedef enum {
  XPAN_OP_NONE,
  XPAN_HOST_CONNECTION,
  XPAN_DISCONNECTION,
  XPAN_ROLE_SWITCH,
  XPAN_BEARER_SWITCH,
  XPAN_BEARER_SWITCH_CANCEL,
} XpanOperation;

typedef enum {
  LE_L2CAP_CONT,
  LE_L2CAP_START,
  BREDR_L2CAP_CONT,
  BREDR_L2CAP_START,
  XPAN_LMP_MSG,
} XpanLlid;

typedef enum {
  XPAN_EB_DISCONNECTED,
  XPAN_EB_CONNECTED,
} XpanEbConnState;

/* TCP Role of the device */
typedef enum {
  XPAN_TCP_ROLE_UNKNOWN,
  XPAN_TCP_CLIENT,
  XPAN_TCP_SERVER,
} XpanTcpRole;

/* XPAN LMP Operation opcodes */
typedef enum {
  XPAN_LMP_ACCEPTED,
  XPAN_LMP_NOT_ACCEPTED,
  XPAN_LMP_L2CAP_PAUSE_UNPAUSE_REQ,
  XPAN_LMP_SWITCH_PRIMARY_REQ,
  XPAN_LMP_BEARER_SWITCH_REQ = 0X04,

  XPAN_LMP_HOST_CONNECTION_REQ = 0X07,
  XPAN_LMP_BEARER_SWITCH_COMPLETE_IND = 0X0A,
  XPAN_LMP_PREPARE_BEARER_SWITCH_REQ,
  XPAN_LMP_PING_REQ,
  XPAN_LMP_PING_RSP,
  XPAN_LMP_BURST_INTERVAL_REQ,
  XPAN_LMP_BURST_INTERVAL_IND,
  XPAN_LMP_LSTO_IND,
  XPAN_LMP_NAME_REQ,
  XPAN_LMP_NAME_RSP,
  XPAN_LMP_BREDR_FEATURES_REQ,
  XPAN_LMP_BREDR_FEATURES_RSP,
  XPAN_LMP_LE_FEATURES_REQ,
  XPAN_LMP_LE_FEATURES_RSP,
  XPAN_LMP_VERSION_REQ,
  XPAN_LMP_VERSION_RSP,
  XPAN_LMP_CANCEL_BEARER_SWITCH,
} XpanLmpOperation;

extern const char* xpan_event_str(XacEvent event);
extern const char* xpan_lmpop_str(XpanLmpOperation event);
extern const char* xpan_state_str(XacSmState state);

class XpanDevice;
class XpanSocketHandler;
class XpanTLSHandler;

/* Xpan LMP Command and event Manager */
class XpanLmpManager {
 private:
  struct LmpOp {
    int8_t opcode = -1;
    std::vector<uint8_t> data;
  };
  std::deque<LmpOp> mLmpCmdQue;
  static std::mutex mLmpOpLock;

  int8_t mCurrentLmpOp = -1;
  uint8_t l2cPauseUnpauseVal = 0xFF;
  uint32_t mSupervisionTimeout;
  uint32_t mPingTimeout;
  uint16_t mPingInterval;
  ipaddr_t mIpAddr;
  XpanSocketHandler *mSocketHdlr;

  /* Parser API's for remote LMP response */
  static uint8_t parseLmpConnectionReq(uint8_t *, uint16_t , ipaddr_t, int);
  static uint8_t parseLmpLeFeatureReq(uint8_t *, uint16_t , ipaddr_t);
  static uint8_t parseLmpLeFeatureRes(uint8_t *, uint16_t , ipaddr_t);
  static uint8_t parseLmpVersionReq(uint8_t *, uint16_t , ipaddr_t);
  static uint8_t parseLmpVersionRes(uint8_t *, uint16_t , ipaddr_t);
  static uint8_t parseLmpSwitchPrimaryReq(uint8_t *, uint16_t , ipaddr_t);
  static uint8_t parseLmpRemotePingReq(uint8_t *, uint16_t , ipaddr_t);
  static uint8_t HandleLmpAcceptReject(uint8_t *p, uint16_t len, ipaddr_t ip, uint8_t op);
  static uint8_t HandleReceivedL2capData(uint8_t *, uint8_t, uint16_t , ipaddr_t);

  void LmpExecute(int8_t opcode, std::vector<uint8_t> data);
  void LmpEnqueue(LmpOp op);

 public:
  XpanAcTimer *mPingTimer;
  XpanAcTimer *mSupervisionTimer;

  XpanLmpManager();
  ~XpanLmpManager();
  void setXpanSocketHandler(XpanSocketHandler *handler);
  void setSupervisionTimeout(uint16_t supervision_timeout, uint16_t interval);
  static uint8_t parseXpanEvent(uint8_t *p, uint16_t len, ipaddr_t, int fd);
  void SendLmpConnectionReq(uint8_t link_type, mdns_uuid_t uuid,
                            uint8_t *cod);
  void SendLmpAccepted();
  void SendLmpNotAccepted(uint16_t err_code);
  void SendLmpPrepareBearerReq(uint8_t bearer_from, uint8_t bearer_to);
  void SendLmpL2capPauseUnpauseReq(uint8_t pause, uint8_t transport_type);
  void SendLmpBearerSwitchReq(uint8_t bearer_from, uint8_t bearer_to);
  void SendLmpBearerSwitchCmplInd(uint8_t bearer_from, uint8_t bearer_to);
  void SendLmpCancelBearerSwitchInd(uint8_t bearer_from, uint8_t bearer_to,
                                    uint16_t err_code);
  void SendLmpPingReq();
  void SendLmpPingRes();
  void SendLmpLstoInd();
  void SendLmpLeFeatureReq(mdns_uuid_t uuid, uint64_t le_features);
  void SendLmpLeFeatureRes(mdns_uuid_t uuid, uint64_t le_features);
  void SendLmpVersionReq(mdns_uuid_t uuid, uint8_t version, uint16_t companyId,
                         uint16_t subversion);
  void SendLmpVersionRes(mdns_uuid_t uuid, uint8_t version, uint16_t companyId,
                         uint16_t subversion);
  void SendAclData(uint8_t llid, uint16_t len, uint8_t* data);

  /* Utility API's */
  void ResetLmpPingTimer();
  void CleanLmpCommandQueue();
  static const char* LmpOpString(uint8_t op);
  int8_t GetCurrentExeOp();
  uint8_t GetCurrentPauseUnpauseAction();
  XpanSocketHandler * GetSocketHandler();
  void SetL2capPauseUnpauseAction(uint8_t);
  void SetIpAddress(ipaddr_t ip);
  ipaddr_t GetIpAddress();
  void LmpExecuteNextOp();
  static void StartSupervisionTimer(XpanLmpManager *lmp);
  static void ResetPingTimer(ipaddr_t ip);
  static void HandleLmpPingTimeout(void *data);
  static void HandleSupervisionTimeout(void *data);
  void ConfigureSupervisionTimers();
};

/* To translate uint16_t to byte stream */
extern void addUint16ToData(std::vector<uint8_t> &data, uint16_t val);

/* To translate uint32_t to byte stream */
extern void addUint32ToData(std::vector<uint8_t> &data, uint32_t val);

/* To translate uint64_t to byte stream */
extern void addUint64ToData(std::vector<uint8_t> &data, uint64_t val);

extern void addUintArrToData(std::vector<uint8_t> &data, uint8_t* val, uint8_t len);

/* String to IP Address array */
extern bool ipStringToIpAddr(const char *str, ipaddr_t *addr);

/* API to Stop and Delete AC Timer */
extern void stopTimer(XpanAcTimer *timer);

/* Xpan AC Socket Handler */
class XpanSocketHandler {
  private:
    bdaddr_t mBdAddr;
    XpanEarbudRole mRole;
    int tcp_fd;
    int tcp_fd_; /* for pending connection before TCP Role is set */
    std::thread mTcpConnThread;
    int mConnectReqId;
    XpanTLSHandler *mTlsHandler;
    XpanTLSHandler *mTlsHandlerServer;
    static int tcp_lfd;
    static int tcp_lport;
    static int udp_data_fd;
    static int udp_data_port_rx;
    static int udp_tsf_fd;
    static int udp_tsf_port_rx;
    static int hdlr_pipe_fd[2];
    static int nfds;
    static bool mIsRxRoutineRunning;
    int tcp_fd_roaming;
    static std::vector<int> mReadFds;
    static std::vector<bdaddr_t> mListeners;
    static fd_set rfds;
    std::shared_ptr<XpanQhciAcIf> qhci;
    XpanTcpRole mTcpRole;
    bool mStopArpRetry;

    bool AddNewFd();
    bool RemoveFd();
    static int GetLastFd();
    static void HandleUpdateReadFds(char *msg, int len);
    static void HandleRemoteDisconnection(int fd);
    static bool ContainsListener(bdaddr_t addr);
    static bool AddListenerFor(bdaddr_t addr);
    static bool RemoveListenerFor(bdaddr_t addr);
    static bool SetSocketTosOption(int fd);
    static bool SetSocketIfOption(int fd);
    static bool DisableDelayedPacketTransfer(int fd);
    static void UpdateFdSetInRxThread(int action, int fd);

  public:
    static int acceptTempFd;
    XpanSocketHandler(bdaddr_t addr, XpanEarbudRole role);
    ~XpanSocketHandler();
    static void StopRxThreadRoutine();
    static void HandleRxThreadTermination();
    int GetTcpFd();
    void StopArpRetry(bool stop_arp_retry);
    void SetSocketFd(int);
    void SetTlsHandler(XpanTLSHandler *tls_handler);
    void SetServerTlsHandler(XpanTLSHandler *tls_handler);
    void SetTcpRole(XpanTcpRole role);
    XpanTLSHandler* GetTlsHandlerServer();
    static int GetUdpDataFd();
    static int GetUdpTsfFd();
    static int GetTcpListeningPort();
    static int GetUdpPort();
    static int GetUdpTsfPort();
    XpanTLSHandler* GetTLSHandler();
    XpanTcpRole GetTcpRole();
    int GetPendingConnectRequestId();
    bool SendData(uint8_t llid, std::vector<uint8_t> &data);

    /* Tx Handler APIs */
    bool InitiateTxRoutine();
    static bool ConnectTcp(XpanSocketHandler *sock, ipaddr_t remote_ip,
                           uint32_t tcp_port, int conn_req_id);
    bool InitTcpConnection(XpanSocketHandler *sock, ipaddr_t remote_ip,
                           uint32_t tcp_port);
    bool InitiateRoamingPrep(XpanSocketHandler *sock, ipaddr_t remote_ip,
                                  uint32_t tcp_port);
    static bool PrepareRoamingTransport(XpanSocketHandler *sock, ipaddr_t remote_ip,
                                              uint32_t tcp_port);
    static bool CreateTcpSocketForIncomingConnection(ipaddr_t ip_local, bdaddr_t addr);
    static bool IsListeningOnTcp();
    static bool IsValidPendingConnection(ipaddr_t, int );
    static bool AcceptIncomingTcpConnection();
    static bool CreateUdpSocketForIncomingData(ipaddr_t ip);
    static bool CreateUpdSocketForTsf(ipaddr_t ip);
    void CloseConnectionSocket();
    static void CloseListeningSocket(bdaddr_t addr);
    static void CloseTsfRxUdpPort();
    static void CloseDataRxUdpPort();
    static void CloseInternalPipeFds();

    /* Rx Handler API's */
    static bool InitiateRxRoutine();
    static void SetMasterFdForConnection(int fd);
    static void UpdateFdSet(bool add, int fd);
    static void ResetFdSet();
    static void HandleRxDateParsingFailure(int fd, uint8_t ret);
    void HandleRoamingCompletion();
    static int CheckSocketStatus(int fd);
    static void EnableTcpKeepAlive(int fd);
};

/* Xpan Application Controller State Machine. One instance per earbud role */
class XpanAcStateMachine {
 private:
  ipaddr_t mIpAddr, mIpRoaming;
  macaddr_t mMacAddr, mMacRoaming;
  bdaddr_t mBdAddr;
  XpanEarbudRole mEarbudRole;
  uint32_t mAudioLoc;
  uint32_t mTcpPort, mTcpPortRoaming;
  int mSocketFd;
  mdns_uuid_t mLocalUuid;
  XpanAcTimer *mDnsQueryTimer;
  XpanAcTimer *mFilteredScanTimer;
  XpanAcTimer *mBearerPrefTimer;
  XpanAcTimer *mTargetedConnTimer;
  XpanDevice *mDevice;
  XpanLmpManager *lmp;
  XpanSocketHandler *mSocketHdlr;
  std::shared_ptr<XpanQhciAcIf> qhci;
  XMXacIf *xm;

  bool newTcpTransportReady = false;
  bool prepBearerApRoamingDone = false;

  XacSmState mCurrentState;
  XacSmState mPrevState;
  bool mConnIsBg;
  TransportType mTransportFrom; /* Transport Transitioning From while creation */
  //TransportType mReqTransport;     /* New requested transport */
  XpanSwitchTrigger mTrigger;      /* module initiating transition */
  XpanTransitionType mTransitionType; /* Ongoing Transition type or
                                         transition responsible for current state */
  XpanOperation mOperation;        /* Current operation */
  bool mApDetailsOkToSend = false;
  bool isPingEnabled = false;
  uint8_t mBearerSwitchState = -1;
  bool mOutTcpConnectInProgress = false;
  bool mProceedPebForLeAp = false;
  bool mLeToApUrgencySet = false;
  bool mIsLeToApHaltedForSeb = false; // Used in PSM
  bool mIsApToLeHaltedForSeb = false; // Used in SSM

  void setState(XacSmState newState, XacEvent evt);

  /* State Handlers */
  void XpanIdleStateHandler(xac_handler_msg_t* msg);
  void XpanTcpConnectingStateHandler(xac_handler_msg_t* msg);
  void XpanTcpConnectedStateHandler(xac_handler_msg_t* msg);
  void XpanLmpConnectingStateHandler(xac_handler_msg_t* msg);
  void XpanLmpConnectedStateHandler(xac_handler_msg_t* msg);
  void XpanBearerSwitchPendingStateHandler(xac_handler_msg_t* msg);
  void XpanApActiveStateHandler(xac_handler_msg_t* msg);
  void XpanDisconectingStateHandler(xac_handler_msg_t* msg);

  /* State Machine Event Handlers */
  void HandleRemoteApDetailsUpdate(xac_handler_msg_t* msg);
  void HandleXpanLmpConnected();
  void HandleLmpPrepareBearerRspEvt(xac_handler_msg_t* msg);
  void HandleQhciPauseUnpauseEvt(xac_handler_msg_t* msg);
  void HandleLmpPauseUnpauseRspEvt(xac_handler_msg_t* msg);
  void HandleLmpBearerSwitchRspEvt(xac_handler_msg_t* msg);
  void HandleXmBearerSwitchInd(xac_handler_msg_t* msg);
  void HandleNewTcpTransportReadyEvt(xac_handler_msg_t* msg);
  void HandleXpanPrimarySwitchReq(xac_handler_msg_t* msg, uint8_t);
  bool HandleInitReconnection(bool);
  void HandleSocketClosure();
  bool HandleStreamStopDuringTransition();
  void HandleStreamStatusEventFromStack(xac_handler_msg_t* msg);
  void HandleUnpauseNotCompleted();

  void Cleanup();
  void UpdateQhciAboutFailedIdleTransition();

 public:
  XpanAcStateMachine(XpanDevice *, bdaddr_t, XpanEarbudRole, TransportType);
  ~XpanAcStateMachine();

  char R[15];

    /* Set Properties */
  void SetEarbudProperties(ipaddr_t, macaddr_t, uint32_t audio_loc,
                           uint32_t tcp_port);
  void SetRoamingDetails(ipaddr_t, macaddr_t, uint32_t tcp_port);
  void SetSocketFd(int fd);
  void UpdateEarbudRole(XpanEarbudRole role);
  void UpdateApTransport();
  void XpanAcSmExecuteEvent(xac_handler_msg_t* msg);
  XacSmState GetState();
  XpanDevice* GetDeviceInstance(mdns_uuid_t uuid);
  bdaddr_t GetAddr();
  ipaddr_t GetIpAddr();
  XpanEarbudRole GetRole();
  XpanTransitionType GetCurrentTransition();
  XpanLmpManager* GetLmpManager();
  XpanSocketHandler* GetSocketHandler();
  int GetSocketFd();
  TransportType GetTransportFrom();
  bool IsRemoteApDetailsOkToSend();
  bool IsLmpPrepareBearerReqPendingInSeb();
  bool IsTargetedBgconn();
  bool CanProgressForLeApInPeb();
  void ProgressForLeApInPeb();
  void SetLeToApUrgency();
  void SetTrigger(XpanSwitchTrigger trigger);
  void SetTransitionType(XpanTransitionType transition);
  void SetFilteredScanTimer();
  void SetSupervisionTimer(uint16_t supervision_to, uint16_t interval);
  void SetIpAddress(ipaddr_t ip);
  static void TriggerFilteredScan(void *data);
  static void BearerPreferenceTimeout(void *data);
  static void TargetedConnectionTimeout(void *data);
  static void HandleMdnsQueryTimeout(void *data);
  void UpdatePrimaryDisconnToSec();
  void HandleBearerPreferenceCmpl(bdaddr_t , TransportType, RspStatus);
  void SendLmpBearerSwitchCompleteInd(uint8_t bearer_from, uint8_t bearer_to);
  void SendLmpCancelBearerSwitchInd(uint8_t bearer_from, uint8_t bearer_to,
                                    uint16_t err);
  bool GetRemoteApDetails();
};

class XpanDevice {
 private:
  bdaddr_t addr;
  macaddr_t apBssid;
  mdns_uuid_t mDnsUuid;
  XpanSwitchTrigger mTrigger;
  RemoteDetails mPrimaryDetails;
  RemoteDetails mSecondaryDetails;
  RemoteDetails mPrimaryRoaming;
  RemoteDetails mSecRoaming;
  XpanAcStateMachine *psm;
  XpanAcStateMachine *ssm;
  std::shared_ptr<XpanQhciAcIf> qhci;
  XMXacIf *xm;
  uint16_t mSupervisionTimeout;
  uint16_t mPingTimeout;
  uint8_t mLtk[KEY_LEN] = {};
  uint8_t pPsk[KEY_LEN] = {};
  bool mIsLeToApUrgencySet = false;
  XpanAcTimer *pReinitConnTimer;
  long mFeatureMask = 0xFFFFFFFFFFFFFFFF;

 public:
  XpanDevice(bdaddr_t addr);
  ~XpanDevice();

  bdaddr_t GetAddr();
  ipaddr_t GetIpAddr(XpanEarbudRole role);
  mdns_uuid_t GetRemoteMdnsUuid();
  void DisplayRemoteApDetails(XpanEarbudRole role);
  XpanAcStateMachine *GetStateMachine(XpanEarbudRole role);
  XpanEarbudRole GetRoleByIpAddr(ipaddr_t ip);
  XpanLmpOperation GetCurrentExeLmpOperation(ipaddr_t ip);
  XpanLmpManager* GetLmpManager(ipaddr_t ip);
  XpanSocketHandler* GetSocketHandler(XpanEarbudRole role);
  long GetFeatureMask();
  bool IsLeToApUrgencySet();
  bool IsLtkSet();
  bool IsNewLtk(uint8_t *, uint8_t *);
  bool IsPskComputed();
  bool IsRemoteIpDetailsAvailable(TransportType bearer);
  void ComputePskFromLtk();
  bool IsPskRecomputationNeeded();
  void SetDeviceSecurityDetails(mdns_uuid_t , uint8_t *, uint8_t *, long);
  void SetIpAddress(XpanEarbudRole role, ipaddr_t ip);

  /* Xpan Manager Calls */
  void PrepareBearer (xac_handler_msg_t* msg);
  void BearerPreference (xac_handler_msg_t* msg);
  void UpdateRemoteApParams (xac_handler_msg_t* msg);
  void RequestRemoteApParams(xac_handler_msg_t* msg);
  void InitiateLmpBearerSwitch (xac_handler_msg_t* msg);
  void RemoteDisconnectedEvent (xac_handler_msg_t* msg);
  void BearerSwitchInd (xac_handler_msg_t* msg);
  void MdnsDiscoveryStatus (xac_handler_msg_t* msg);
  void UpdateBondState (xac_handler_msg_t* msg);
  void SetXpanSupportedFeature(xac_handler_msg_t* msg);

  /* QHCI Calls */
  void CreateConnection (xac_handler_msg_t* msg);
  void CreateConnectionCancel (xac_handler_msg_t* msg);
  void DisconnectConnection (xac_handler_msg_t* msg);
  void GetRemoteVersion (xac_handler_msg_t* msg);
  void GetRemoteLeFeatures (xac_handler_msg_t* msg);
  void EnableEncrption (xac_handler_msg_t* msg);
  void SendAclData (xac_handler_msg_t* msg);
  void L2capPauseUnpauseRes (xac_handler_msg_t* msg);

  /* Handler API's*/
  void HandleLmpPrepareBearerRes(xac_handler_msg_t* msg);
  void HandleLmpL2capPauseUnpauseRes(xac_handler_msg_t* msg);
  void HandleLmpBearerSwitchRes(xac_handler_msg_t* msg);
  void HandleRoleSwitchRequest(xac_handler_msg_t* msg);
  void HandleTcpConnectedEvent(xac_handler_msg_t* msg);
  void HandleTcpDisconnectedEvent(xac_handler_msg_t* msg);
  void HandleTcpFailedEvent(xac_handler_msg_t* msg);
  void HandleWifiApDisconnectedEvent(xac_handler_msg_t* msg);
  void HandleLmpConnectionRes (xac_handler_msg_t* msg);
  void HandleLmpConnectionReq(xac_handler_msg_t* msg);
  void HandleEarbudDisconnectedEvt(XpanEarbudRole role);
  void HandleLmpPingRequestEvt(xac_handler_msg_t* msg);
  void HandleLmpLeFeatureReq(xac_handler_msg_t* msg);
  void HandleLmpLeFeatureRes(xac_handler_msg_t* msg);
  void HandleLmpVersionReq(xac_handler_msg_t* msg);
  void HandleLmpVersionRes(xac_handler_msg_t* msg);
  void HandleAclDataEvt(xac_handler_msg_t* msg);
  void HandleBearerPreferenceTimeout(xac_handler_msg_t* msg);
  void HandlePrimaryDisconnecting(xac_handler_msg_t* msg);
  void HandleTargetedAdvConnTimeout(xac_handler_msg_t* msg);
  void HandleLeLinkStatusEvt(xac_handler_msg_t* msg);
  void HandleResetSupervision(xac_handler_msg_t* msg);
  void UpdateEarbudConnectionStatusToCp(XpanEarbudRole role,
                                        XpanEbConnState state);
  bool DirectRoamingUpdateToCp(xac_handler_msg_t* msg);
  void HandleStreamStatusUpdate(xac_handler_msg_t* msg);
  void ScheduleExplicitConnection();
  static void TriggerExplicitConnection(void *data);
  void CancelExplicitConnection();

};

class XpanApplicationController {
 private:
  static XpanApplicationController* instance;

  std::vector<XpanDevice> devices;
  static std::vector<QhciCreateConnection> mStalledConnRequests;
  static ipaddr_t localIpAddr;
  static macaddr_t localMacAddr;
  static macaddr_t localApBssid;
  static mdns_uuid_t local_uuid;
  static uint32_t center_freq;
  static uint8_t *local_cod;
  static uint64_t le_features;
  static LocalVersionInfo info;
  static std::thread mXpanAcThread;
  static std::thread mXpanRxThread;
  static std::mutex mWqMutex;
  static std::deque <xac_handler_msg_t *> mWorkQueue;
  static std::condition_variable mWqNotifier;
  static std::atomic_bool mIsMainThreadBusy;
  static std::atomic_bool mIsMainThreadRunning;
  static bool isXpanTlsEnabled;
  static bool isXpanDtlsEnabled;
  XpanDevSecRecord mSecRecords[MAX_SUPPORTED_DEV];

  static void SigHandler(int signum);
  static void XacMessageHandlerRoutine();
  static void thread_task();
  static void RxThreadTask();
  static void XacMessageHandler(xac_handler_msg_t *msg);
  XpanApplicationController();
  ~XpanApplicationController();
  XpanDevice* CreateOrGetDevice(bdaddr_t addr);

  void LoadXpanBondedDevices(uint8_t no_of_dev, bdaddr_t *devices);
  void UpdateLocalApDetails (macaddr_t mac, macaddr_t bssid,
                             ipaddr_t ipAddr, mdns_uuid_t uuid, uint32_t freq);
  void BondStateUpdated (bdaddr_t addr, BondState state);
  bool IsBonded(bdaddr_t addr);
  static void SetLocalVersionInfo(LocalVersionInfo info);
  static void SetLocalLeFeatures (uint64_t le_features);

 public:
  static int mUniqueId;
  static XpanApplicationController* Get();
  static bool Initialize();
  static bool Deinitialize();
  bool PostMessage(xac_handler_msg_t *msg, bool isHighPriority = false);
  bool RemoveMessage(XacEvent event);
  static bool CacheConnectionRequest(xac_handler_msg_t *msg);
  static bool RemovePendingConnection(bdaddr_t addr);
  void RecachePendingConnections();
  static mdns_uuid_t SetUuidOrder(mdns_uuid_t uuid);
  bool IsDevicePresent(bdaddr_t addr);

  /* Getter API's*/
  static mdns_uuid_t GetLocalMdnsUuid();
  static ipaddr_t GetLocalIpAddr();
  static macaddr_t GetLocalMacAddr();
  static macaddr_t GetLocalApBssid();
  static uint32_t GetLocalApFrequency();
  XpanDevice* GetDevice(bdaddr_t addr);
  XpanDevice* GetDeviceByMdnsUuid(mdns_uuid_t uuid);
  XpanDevice* GetDeviceByIpAddr(ipaddr_t ip);
  XpanLmpManager* GetLmpManager(ipaddr_t ip);
  XpanLmpManager* GetLmpManager(int socket_fd);
  ipaddr_t GetIpFromFd(int socket_fd);
  static uint64_t GetLocalLeFeatures();
  static LocalVersionInfo GetLocalVersion();
  static mdns_uuid_t GetMdnsUuid();
  uint8_t* GetLtkFromSecRecord(bdaddr_t addr);
  mdns_uuid_t GetMdnsUuidFromSecRecord(bdaddr_t addr);
  static uint8_t *GetCod();
  static ipaddr_t ChangeIpEndianness(ipaddr_t ip);
  static bool IsTLSEnabled();
  static bool IsDTLSEnabled();
  static bool IsWhcSupportingDevice(bdaddr_t addr);
  static XpanTLSHandler * GetTLSHandler(ipaddr_t ip);
  bool AddDevSecRecord(mdns_uuid_t, bdaddr_t, uint8_t psk[], uint8_t *, long );
  bool RemoveDevSecRecord(bdaddr_t);
  bool PersistDevSecurityRecords();
  bool ReadDevSecurityRecords();
  static void PrintKey(uint8_t *key, char *name);
};

} // namespace ac
} // namespace xpan

#endif
