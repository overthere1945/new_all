/*
 *  Copyright (c) 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved..
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#ifndef XM_IPC_IF_H
#define XM_IPC_IF_H

#pragma once

#include <stdint.h>
#include <string.h>
#include <vector>
#include "xpan_utils.h"

namespace xpan {
namespace implementation {

typedef enum {
  XM_IPC_MSG_BASE = 1000,
  /* This message is used to send CoP version from DH to XM*/
  DH_XM_COP_VER_IND,
  /* This message is used to send CoP version from DH to CP*/
  XM_CP_COP_VER_IND,
  /* This message is used to send CP log level*/
  XM_CP_LOG_LVL,
  XM_KP_LOG_LVL,
  XM_XP_CREATE_SAP_INF_STATUS,
  XM_WIFI_TRANSPORT_SWITCH_REQ,
  XM_WIFI_TRANSPORT_SWITCH_COMPLETED,
  /* This message is used to notify that remote support XPAN or not*/
  QHCI_XM_REMOTE_SUPPORT_XPAN,
  XP_XM_TRANSPORT_ENABLED,
  XP_XM_BONDED_DEVICES_LIST,
  /* QHCI asks to prepare audio bearer for given transport*/
  QHCI_XM_PREPARE_AUDIO_BEARER_REQ,
  QHCI_XM_USECASE_UPDATE,
  XP_XM_PREPARE_AUDIO_BEARER_RSP,
  XP_WIFI_TRANSPORT_SWITCH_RSP,
  XP_WIFI_CONNECTED_EB_DETAILS,
  XP_WIFI_DISCONNECTED_EB_DETAILS,
  XP_TEARDOWN_TWT,
  XP_WIFI_SET_AP_AVB_REQ,
  XP_WIFI_CANCEL_AP_AVB,
  KP_XM_PREPARE_AUDIO_BEARER_RSP,
  KP_XM_ADSP_STATE_IND,
  KP_XM_USECASE_STATE_IND,
  KP_XM_PORT_STATE_IND,
  CP_XM_PREPARE_AUDIO_BEARER_RSP,
  XM_QHCI_PREPARE_AUDIO_BEARER_RSP,
  QHCI_CP_ENCODER_LIMIT_UPDATE,
  XP_XM_AUDIO_BEARER_SWITCHED,
  /* QHCI Asks to unprepare audio bearer for given transport */
  QHCI_XM_UNPREPARE_AUDIO_BEARER_REQ,
  XM_PREPARE_AUDIO_BEARER_TIMEOUT,
  XM_BEARER_PREFERENCE_TIMEOUT,
  XM_INITIALIZE_XPAN_MODULES,
  XP_XM_ENABLE_ACS,
  WIFI_XM_ACS_RESULTS,
  WIFI_XM_TWT_EVENT,
  WIFI_XM_SSR_EVENT,
  XP_XM_HOST_PARAMETERS,
  XP_XM_SAP_POWER_STATE,
  XP_XM_SAP_CURRENT_STATE,
  XP_XM_CREATE_SAP_INF,
  WIFI_XP_TRANSPORT_SWITCH_REQ,
  WIFI_XM_TRANSPORT_SWITCH_RSP,
  WIFI_XP_SET_AP_AVB_RSP,
  WIFI_XM_SAP_POWER_SAVE_STATE_RSP,
  WIFI_XM_USECASE_UPDATE_RSP,
  WIFI_XM_CSA,
  WIFI_XM_DRV_STATE,
  WIFI_XM_COUNTRY_CODE_UPDATE,
  WIFI_XM_POSSIBLE_TO_CREATE_AP_IFACE,
  XP_XM_TWT_SESSION_EST,
  XP_XM_TWT_SESSION_REQ,
  XP_XM_BEARER_PREFERENCE_REQ,
  CP_XM_DELAY_REPORTING,
  CP_XM_TRANSPORT_UPDATE,
  XAC_XM_TRANSPORT_UPDATE,
  /* Triggered to send the usecase to clients during seamless transtions */
  XM_WIFI_USECASE_UPDATE,
  XM_WIFI_STA_USECASE_UPDATE,
  XM_WIFI_ENABLE_STATS,
  /* Codec Either requested or accepted to switch transport */
  CP_XM_BEARER_SWITCH_REQ,
  CP_XM_ADSP_STATE_IND,
  QHCI_XM_AUDIO_STREAM_UPDATE,
  XAC_XP_CURRENT_TRANS_UPDATE,
  XAC_XP_MDNS_REQ,
  XP_XAC_MDNS_RSP,
  XP_XAC_BONDED_STATE_UPDATE,
  XP_XAC_LOCAL_AP_DETAILS,
  XP_XM_GET_WIFI_STATUS,
  XM_WIFI_UNREGISTER_REGISTER_STA,
  XM_WIFI_REGISTER_STA,
  XAC_XP_REGISTER_MDNS_SERVICE,
  XAC_XP_START_FILTERED_SCAN,
  XAC_CP_BURST_INT_REQ,
  XP_QHCI_CONNECT_LINK_REQ,
  QHCI_XP_CONNECT_LINK_RSP,
  XP_XAC_REMOTE_AP_DETAILS,
  XP_XAC_HANDSET_PORT_REQ,
  XP_XAC_HANDSET_PORT_RSP,
  XAC_XM_PREPARE_AUDIO_BEARER_RSP,
  XAC_CP_REMOTE_PARAMS_IND,
  XAC_CP_REMOTE_PARAMS_REQ,
  XAC_CP_REMOTE_PARAMS_IND_DURING_ROAMING,
  XP_WIFI_SCAN_STARTED,
  XAC_CP_ROLE_SWITCH_IND,
  XAC_CP_BSSID_CHANGE_IND,
  XAC_XP_BEARER_SWITCH_FAILED_RSP,
  XAC_INITIATE_BLUETOOTH_CONNECTION_IND,
  /* CP sharing timestamp and event logs */
  CP_XM_TIME_SYNC_IND,
  CP_XM_LOG_EVENT_IND,
  QHCI_XP_BEARER_PREFERENCE_REQ,
  XP_REMOTE_FEATURE_MASK,
} XmIpcEventId;

typedef struct {
  XmIpcEventId eventId;
  uint8_t len;
  uint8_t *data; 
} __attribute__((packed)) DhXmCopVerInd;

typedef struct {
  XmIpcEventId eventId;
  uint8_t len;
  uint8_t data; 
} __attribute__((packed)) XmLogLvl;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  bool is_supported; 
} __attribute__((packed)) QhciXmRemoteSupportXpan;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  TransportType transport;
  uint8_t reason;
} __attribute__((packed)) QhciXmRBearerPreference;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  TransportType type;
} __attribute__((packed)) QhciXmPrepareAudioBearerReq;

typedef struct {
  UseCaseType usecase;
  UseCaseType prev_usecase;
  macaddr_t LeftEb;
  macaddr_t RightEb;
  uint32_t twt_si;
  uint8_t usecase_orginator;
  uint8_t gaming_vbc_si;
  uint32_t right_earbud_offset;
} __attribute__((packed)) XmUseCaseReq;

typedef struct {
  XmIpcEventId eventId;
  UseCaseType usecase;
  macaddr_t LeftEb;
  macaddr_t RightEb;
  bdaddr_t bdaddr;
  uint8_t usecase_orginator;
  uint8_t gaming_vbc_si;
  uint32_t right_earbud_offset;
} __attribute__((packed)) QhciXmUseCase;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  TransportType type;
} __attribute__((packed)) QhciXmUnPrepareAudioBearerReq;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  TransportType type;
} __attribute__((packed)) QhciXmAudioUpdate;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  bool status;
} __attribute__((packed)) XmQhciUnPrepareAudioBearerRsp;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  RspStatus status;
} __attribute__((packed)) XmPrepareAudioBearerRsp;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  TransportType type;
  bool status;
  uint8_t reason;
} __attribute__((packed)) XmXpTransportEnabled;

typedef struct {
  XmIpcEventId eventId;
  uint8_t current_transport;
  uint8_t status;
} __attribute__((packed)) XmKpAudioBearerRsp;

typedef struct {
  XmIpcEventId eventId;
  uint32_t action;
} __attribute__((packed)) AdspStateInd;
 
typedef struct {
  XmIpcEventId eventId;
  uint8_t port_state;
} __attribute__((packed)) PortStateInd;

typedef struct {
  XmIpcEventId eventId;
  uint8_t current_transport;
  uint8_t status;
} __attribute__((packed)) XmCpAudioBearerRsp;

typedef struct {
  XmIpcEventId eventId;
  uint8_t transport;
  uint8_t stream_id;
} __attribute__((packed)) KpUseCaseStartInd;

typedef struct {
  XmIpcEventId eventId;
} __attribute__((packed)) XmTimeout;

typedef struct {
  XmIpcEventId eventId;
  bool is_xpan_supported;
} __attribute__((packed)) XmXpanInit;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  RspStatus status;
} __attribute__((packed)) XpXmBearerSwitch;

typedef struct {
  XmIpcEventId eventId;
  int *freq_list;
  int freq_list_size;
} __attribute__((packed)) XpEnableAcs;

typedef struct {
  XmIpcEventId eventId;
  uint8_t twt_event;
  int32_t wake_dur_us;
  int32_t wake_int_us;
  macaddr_t addr;
} __attribute__((packed)) XpTwtEvent;

typedef struct {
  XmIpcEventId eventId;
  int freq;
  uint8_t status;
} __attribute__((packed)) XpAcsResults;

typedef struct {
  XmIpcEventId eventId;
  macaddr_t macaddr;
  uint16_t Ethertype;
} __attribute__((packed)) XpHostParameters;

typedef struct {
  XmIpcEventId eventId;
  uint8_t state;
} __attribute__((packed)) XpCreateSapInf;

typedef struct {
  XmIpcEventId eventId;
  uint8_t req_state;
  uint8_t status;
} __attribute__((packed)) XpCreateSapInfStatus;

typedef struct {
  XmIpcEventId eventId;
  uint8_t dialog_id;
  uint8_t state;
} __attribute__((packed)) XpSapPowerStateParams;

typedef struct {
  XmIpcEventId eventId;
  uint16_t state;
} __attribute__((packed)) XpSapStatus;

typedef struct {
  XmIpcEventId eventId;
  uint8_t dialog_id;
  uint16_t power_save_bi_multiplier;
  uint64_t next_tsf;
} __attribute__((packed)) WiFiSapPowerStateParams;

typedef struct {
  XmIpcEventId eventId;
  uint8_t transport_type;
  uint8_t status;
  uint8_t reason;
} __attribute__((packed)) XmWiFiTransportSwitch;

typedef struct {
  XmIpcEventId eventId;
  uint8_t set_id;
  macaddr_t left_eb;
  macaddr_t right_eb;
} __attribute__((packed)) XpConnectedEbDetails;

typedef struct {
  XmIpcEventId eventId;
  macaddr_t left_eb;
  macaddr_t right_eb;
} __attribute__((packed)) XpDisConnectedEbDetails;

typedef struct {
  XmIpcEventId eventId;
  macaddr_t left_eb;
  macaddr_t right_eb;
} __attribute__((packed)) XpTeardownTwt;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t addr;
  uint32_t duration;
  uint8_t status;
} __attribute__((packed)) XpWifiSetApAvb;

typedef struct {
  XmIpcEventId eventId;
  uint8_t status;
} __attribute__((packed)) WiFiUseCaseUpdateRsp;

typedef struct {
  XmIpcEventId eventId;
  uint8_t status;
} __attribute__((packed)) WiFiSsrEvent;

typedef struct {
  XmIpcEventId eventId;
  uint8_t status;
} __attribute__((packed)) WiFiDrvState;

typedef struct {
  XmIpcEventId eventId;
  char *code;
} __attribute__((packed)) CountryCodeUpdate;

typedef struct {
  macaddr_t mac_addr;
  uint32_t interval;
  uint32_t peroid;
  uint32_t location;
} __attribute__((packed)) XPANTwtSessionParams;

typedef struct {
  macaddr_t mac_addr;
  uint8_t IpAddress[6];
  uint16_t audio_locations;
  uint8_t role;
}__attribute__((packed)) RemoteApParams;

typedef struct {
  XmIpcEventId eventId;
  uint8_t encryption;
  uint8_t psk[16];
  uint8_t identity[16];
  macaddr_t hs_ap_bssid;
  macaddr_t hs_mac_addr;
  ipaddr_t hs_ip_addr;
  uint32_t center_freq;
  uint16_t time_sync_tx_port;
  uint16_t time_sync_rx_port;
  uint16_t remote_udp_port;
  uint16_t rx_udp_port;
  uint16_t periodicity;
  uint8_t num_devices;
  RemoteEbParams *EbParams;
  uint8_t stream_id;
}__attribute__((packed)) CpRemoteApParams;

typedef struct {
  XmIpcEventId eventId;
  uint8_t num_devices;
  uint32_t right_earbud_offset;
  uint8_t vbc_si;
  XPANTwtSessionParams *params;
  uint8_t stream_id;
} __attribute__((packed)) XpTwtParameters;

typedef struct {
  uint8_t num_devices;
  uint32_t right_earbud_offset;
  uint8_t vbc_si;
  std::vector<XPANTwtSessionParams> params;
} __attribute__((packed)) TwtParameters;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  uint8_t transport;
  uint8_t requestor;
  uint8_t reason;
  bool urgency;
} __attribute__((packed)) XpBearerPreference;

typedef struct {
  XmIpcEventId eventId;
  uint32_t delay_reporting;
} __attribute__((packed)) CpDelayReporting;

typedef struct {
  XmIpcEventId eventId;
  bool enable;
  int interval;
  int interface_type;
} __attribute__((packed)) XmEnableStats;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  TransportType transport;
} __attribute__((packed)) XmTransportUpdate;

typedef struct {
  XmIpcEventId eventId;
  uint8_t numOfDevices;
  bdaddr_t *bdaddr;
} __attribute__((packed)) BondedDeviceList;

typedef struct {
  XmIpcEventId eventId;
  TransportType transport;
} __attribute__((packed)) CpBearerSwitchReq;

typedef struct {
  XmIpcEventId eventId;
  uint8_t num_limit;
  uint8_t *data;
} __attribute__((packed)) CpEncoderLimit;

typedef struct {
  bdaddr_t bdaddr;
  TransportType type;
} __attribute__((packed)) PrepareAudioBearerReq;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  uint8_t status;
  uint8_t state;
  mdns_uuid_t uuid;
} __attribute__((packed)) MdnsQuery;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  BondState state;
} __attribute__((packed)) BondStateUpdate;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  RspStatus status;
} __attribute__((packed)) ConnectLeLink;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
} __attribute__((packed)) bdaddr_holder;

typedef struct {
  XmIpcEventId eventId;
  macaddr_t macaddr;
  uint16_t bi_media;
  uint16_t bi_voice;
  uint8_t status;
} __attribute__((packed)) BurstInterval;

typedef struct {
  XmIpcEventId eventId;
  tXPAN_Remote_Params params;
} __attribute__((packed)) RemoteApDetails;

typedef struct {
  XmIpcEventId eventId;
  macaddr_t bssid;
  macaddr_t ap_mac_address;
  ipaddr_t ipaddr;
  mdns_uuid_t mdns_uuid;
  uint32_t center_freq;
} __attribute__((packed)) ApDetails;

typedef struct {
  XmIpcEventId eventId;
  macaddr_t macaddr;
}  __attribute__((packed)) RoleSwitchDetails;

typedef struct {
  XmIpcEventId eventId;
  macaddr_t hs_ap_bssid;
}__attribute__((packed)) BssidUpdateParams;

typedef struct {
  XmIpcEventId eventId;
  macaddr_t ap_mac_address;
}  __attribute__((packed)) RegisterStaInf;

typedef struct {
  XmIpcEventId eventId;
  int tcp_port;
  int udp_port;
  int udp_tsf_port;
} __attribute__((packed)) HandSetPort;

typedef struct {
  XmIpcEventId eventId;
  uint32_t freq;
  uint64_t tsf;
  uint8_t bw;
} __attribute__((packed)) SAPChannelParams;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  uint8_t state;
} __attribute__((packed)) XpWIfiScan;

typedef struct {
  XmIpcEventId eventId;
} __attribute__((packed)) PossibleToCreateApIFace;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  uint8_t reason;
  uint8_t role;
} __attribute__((packed)) XacBearerSwitchFailedRsp;

typedef struct {
  uint32_t ModuleInstId;
  uint8_t is_sink;
  uint16_t SubModuleId;
  uint32_t LogEventId;
} __attribute__((packed)) CpLogEvent;


typedef struct {
  uint32_t ModuleInstId;
  uint8_t is_sink;
  uint64_t timestamp;
} __attribute__((packed)) CpTimeSync;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  uint8_t reason;
} __attribute__((packed)) XacInitBluetoothConnection;

typedef struct {
  XmIpcEventId eventId;
  bdaddr_t bdaddr;
  uint64_t feature_mask;
} __attribute__((packed)) XpRemoteFeatureMask;

typedef union {
  XmIpcEventId eventId;
  /* This entry holds CoP version indication data */
  DhXmCopVerInd CoPInd;
  XmLogLvl Loglvl;
  QhciXmRemoteSupportXpan SupportsXpan;
  QhciXmRBearerPreference QhciBearerPreference;
  QhciXmPrepareAudioBearerReq AudioBearerReq;
  QhciXmUseCase UseCase;
  QhciXmUnPrepareAudioBearerReq UnPrepareAudioBearerReq;
  QhciXmAudioUpdate AudioUpdate;
  XmQhciUnPrepareAudioBearerRsp UnPrepareAudioBearerRsp;
  XmPrepareAudioBearerRsp AudioBearerRsp;
  XmXpTransportEnabled TransportEnabled;
  XmKpAudioBearerRsp KpAudioBearerRsp;
  AdspStateInd AdspState;
  PortStateInd PortState;
  KpUseCaseStartInd KpInd;
  XmCpAudioBearerRsp CpAudioBearerRsp;
  XpXmBearerSwitch   XpBearerSwitchInd;
  XmTimeout AudioBearerTimeout;
  XmXpanInit XpanInit;
  XpEnableAcs Acslist;
  XpAcsResults AcsResults;
  XpTwtEvent TwtEvent;
  XpHostParameters HostParams;
  XpCreateSapInf CreateSapInfParams;
  RegisterStaInf StationInf;
  XpCreateSapInfStatus CreateSapInfStatusParams;
  XpSapPowerStateParams SapPowerStateParams;
  XpSapStatus SapState;
  XmWiFiTransportSwitch WiFiTransportSwitch;
  XpConnectedEbDetails WiFiConnectedEbs;
  XpDisConnectedEbDetails WiFiDisConnectedEbs;
  XpTeardownTwt TeardownTwt;
  XmWiFiTransportSwitch WiFiBearerSwitchRsp;
  XpWifiSetApAvb SetApAvbReq;
  XpWifiSetApAvb SetApAvbRsp;
  XpWifiSetApAvb CancelApAvb;
  SAPChannelParams ChannelSwitchStarted;
  WiFiSapPowerStateParams SapPowerStateRsp;
  WiFiUseCaseUpdateRsp UseCaseUpdateRsp;
  WiFiSsrEvent SsrEvent;
  WiFiDrvState DrvState;
  CountryCodeUpdate CcUpdate;
  PossibleToCreateApIFace possibleToCreateApIface;
  XpTwtParameters TwtParams;
  XpBearerPreference BearerPreference;
  CpDelayReporting DelayReporting;
  CpEncoderLimit EncoderLimitParams;
  XmEnableStats EnableStats;
  XmTransportUpdate TransportUpdate;
  BondedDeviceList BondedDevies;
  CpBearerSwitchReq BearerSwitchReq;
  MdnsQuery MdnsReq;
  BondStateUpdate BondStateInd;
  MdnsQuery MdnsRsp;
  bdaddr_holder FilterScanInd;
  BurstInterval BurstIntReq;
  ConnectLeLink ConnectToLeLinkReq;
  ConnectLeLink ConnectToLeLinkRsp;
  RemoteApDetails EbParams;
  ApDetails ApParams;
  HandSetPort HandSetPortNumReq;
  HandSetPort HandSetPortNumRsp;
  CpRemoteApParams CpRemoteParamsInd;
  XpWIfiScan XpWifiScanStarted;
  RoleSwitchDetails RoleSwitchInd;
  BssidUpdateParams BssidUpdateInd;
  XacBearerSwitchFailedRsp xacBearerFailed;
  CpLogEvent CpLogEventInd;
  CpTimeSync CpTimeSyncInd;
  XacInitBluetoothConnection InitBluetoothConnection;
  XpRemoteFeatureMask RemoteFeatureMask;
} xm_ipc_msg_t;

typedef struct {
  XmIpcEventId eventId;
  ApiDirection orginator;
  uint8_t requestor;
  uint8_t waiting_for_rsp;
  uint8_t delayed_req;
  TransportType type;
  bool rx_bearer_ind;
  bool bearer_switch_rsp;
  bool stats_enabled;
  bool waiting_for_xac_transport_update;
} __attribute__((packed)) XmAudioBearerReq;

typedef struct {
  uint8_t orginator;
  TransportType reqtransport;
  bdaddr_t bdaddr;
  uint8_t requestor;
  bool wifi_accepted_switch;
  bool is_xpan_ap_roaming_started;
  bool is_xpan_ap_roaming_completed;
  bool urgency; // Cuurently, set for LE->AP only
} __attribute__((packed)) XmBearerPreferenceReq;

#define XM_IPC_MSG_SIZE sizeof(xm_ipc_msg_t)

#define XM_XP				1
#define XM_CP 				2
#define XM_KP				3
#define XM_QHCI				4
#define XM_XAC                          5
#define XM_SELF                         6
#define XM_WIFI                         7 
#define XM_ORG_INVALID                  8

#define XM_AUDIO_BEARER_TIMEOUT			(1000 * 1.5)
#define XM_SEAMLESS_AUDIO_BEARER_TIMEOUT	(1000 * 15)
#define XM_SEAMLESS_AUDIO_BEARER_AP_TIMEOUT	(1000 * 25)
#define XM_BEARER_PREF_TIMEOUT			(1000 * 20)
#define XM_XP_RETRY_TIMEOUT                     (100)
#define XM_ENCODER_LIMIT_PARAMS_SIZE    3
#define TWT_CONFIG_ENABLE 1

static inline char * OrginatorToString(uint8_t orginator)
{
  if (orginator == XM_XP)
    return "XP";
  else if (orginator == XM_CP)
    return "CP";
  else if (orginator == XM_KP)
    return "KP";
  else if (orginator == XM_QHCI)
    return "QHCI";
  else
    return "ORG_INVALID";
}

enum {
	WIFI_TRANS_SWITCH_STATUS_REQUEST = 0,
	WIFI_TRANS_SWITCH_STATUS_REJECTED = 1,
	WIFI_TRANS_SWITCH_STATUS_COMPLETED = 2,
}WifiTransportSwitchStatus;

typedef enum {
	WIFI_DRIVER_ACTIVE = 0,
	WIFI_DRIVER_NOT_ACTIVE = 1,
	WIFI_DRIVER_NOT_SUPPORTING_XPAN = 2,
	WIFI_DRIVER_SUPPORTING_XPAN = 3,
	WIFI_DRIVER_INIT_FAILED = 4,
} WifiDrvStatus;

static inline char * ConvertMsgtoString(XmIpcEventId eventId)
{ 
  if (eventId == DH_XM_COP_VER_IND)
    return "DH_XM_COP_VER_IND";
  if (eventId == XM_CP_COP_VER_IND)
    return "XM_CP_COP_VER_IND";
  else if (eventId == XM_CP_LOG_LVL)
    return "XM_CP_LOG_LVL";
  else if (eventId == XM_KP_LOG_LVL)
    return "XM_KP_LOG_LVL";
  else if (eventId == XM_XP_CREATE_SAP_INF_STATUS)
    return "XM_XP_CREATE_SAP_INF_STATUS";
  else if (eventId == QHCI_XM_REMOTE_SUPPORT_XPAN)
    return "QHCI_XM_REMOTE_SUPPORT_XPAN";
  else if (eventId == XP_XM_TRANSPORT_ENABLED)
    return "XP_XM_TRANSPORT_ENABLED";
  else if (eventId == XP_XM_BONDED_DEVICES_LIST)
    return "XP_XM_BONDED_DEVICES_LIST";
  else if (eventId == QHCI_XM_PREPARE_AUDIO_BEARER_REQ)
    return "QHCI_XM_PREPARE_AUDIO_BEARER_REQ";
  else if(eventId == QHCI_XM_USECASE_UPDATE)
    return "QHCI_XM_USECASE_UPDATE";
  else if (eventId == XP_XM_PREPARE_AUDIO_BEARER_RSP)
    return "XP_XM_PREPARE_AUDIO_BEARER_RSP";
  else if (eventId == XP_WIFI_TRANSPORT_SWITCH_RSP)
    return "XP_WIFI_TRANSPORT_SWITCH_RSP";
  else if (eventId == XP_WIFI_CONNECTED_EB_DETAILS)
    return "XP_WIFI_CONNECTED_EB_DETAILS";
  else if (eventId == XP_WIFI_DISCONNECTED_EB_DETAILS)
    return "XP_WIFI_DISCONNECTED_EB_DETAILS";
  else if (eventId == XP_TEARDOWN_TWT)
      return "XP_TEARDOWN_TWT";
  else if (eventId == KP_XM_PREPARE_AUDIO_BEARER_RSP)
    return "KP_XM_PREPARE_AUDIO_BEARER_RSP";
  else if (eventId == KP_XM_ADSP_STATE_IND)
    return "KP_XM_ADSP_STATE_IND";
  else if (eventId == KP_XM_USECASE_STATE_IND)
    return "KP_XM_USECASE_STATE_IND";
  else if (eventId == KP_XM_PORT_STATE_IND)
    return "KP_XM_PORT_STATE_IND";
  else if (eventId == CP_XM_PREPARE_AUDIO_BEARER_RSP)
    return "CP_XM_PREPARE_AUDIO_BEARER_RSP";
  else if (eventId == XM_QHCI_PREPARE_AUDIO_BEARER_RSP)
    return "XM_QHCI_PREPARE_AUDIO_BEARER_RSP";
  else if (eventId == QHCI_CP_ENCODER_LIMIT_UPDATE)
    return "QHCI_CP_ENCODER_LIMIT_UPDATE";
  else if (eventId == XP_XM_AUDIO_BEARER_SWITCHED)
    return "XP_XM_AUDIO_BEARER_SWITCHED";
  else if (eventId == QHCI_XM_UNPREPARE_AUDIO_BEARER_REQ)
    return "QHCI_XM_UNPREPARE_AUDIO_BEARER_REQ";
  else if (eventId == XM_PREPARE_AUDIO_BEARER_TIMEOUT)
     return "XM_PREPARE_AUDIO_BEARER_TIMEOUT";
  else if (eventId == XM_BEARER_PREFERENCE_TIMEOUT)
     return "XM_BEARER_PREFERENCE_TIMEOUT";
  else if (eventId == XM_INITIALIZE_XPAN_MODULES)
     return "XM_INITIALIZE_XPAN_MODULES";
  else if (eventId == XP_XM_ENABLE_ACS)
    return "XP_XM_ENABLE_ACS";
  else if (eventId == WIFI_XM_ACS_RESULTS)
    return "WIFI_XM_ACS_RESULTS";
  else if (eventId == WIFI_XM_TWT_EVENT)
    return "WIFI_XM_TWT_EVENT";
  else if (eventId == WIFI_XM_SSR_EVENT)
    return "WIFI_XM_SSR_EVENT";
  else if(eventId == XP_XM_HOST_PARAMETERS)
    return "XP_XM_HOST_PARAMETERS";
  else if (eventId == XP_XM_SAP_POWER_STATE)
    return "XP_XM_SAP_POWER_STATE";
  else if (eventId == XP_XM_SAP_CURRENT_STATE)
    return "XP_XM_SAP_CURRENT_STATE";
  else if (eventId == XP_XM_CREATE_SAP_INF)
    return "XP_XM_CREATE_SAP_INF";
  else if (eventId == XM_WIFI_TRANSPORT_SWITCH_COMPLETED)
    return "XM_WIFI_TRANSPORT_SWITCH_COMPLETED";
  else if (eventId == XM_WIFI_TRANSPORT_SWITCH_REQ)
    return "XM_WIFI_TRANSPORT_SWITCH_REQ";
  else if (eventId == WIFI_XM_TRANSPORT_SWITCH_RSP)
    return "WIFI_XM_TRANSPORT_SWITCH_RSP";
  else if (eventId == WIFI_XP_TRANSPORT_SWITCH_REQ)
    return "WIFI_XP_TRANSPORT_SWITCH_REQ";
  else if (eventId == WIFI_XM_SAP_POWER_SAVE_STATE_RSP)
    return "WIFI_XM_SAP_POWER_SAVE_STATE_RSP";
  else if (eventId == WIFI_XM_USECASE_UPDATE_RSP)
    return "WIFI_XM_USECASE_UPDATE_RSP";
  else if (eventId == XP_XM_TWT_SESSION_EST)
    return "XP_XM_TWT_SESSION_EST";
  else if (eventId == XP_XM_TWT_SESSION_REQ)
    return "XP_XM_TWT_SESSION_REQ";
  else if (eventId == XP_XM_BEARER_PREFERENCE_REQ)
    return "XP_XM_BEARER_PREFERENCE_REQ";
  else if (eventId == CP_XM_DELAY_REPORTING)
    return "CP_XM_DELAY_REPORTING";
  else if (eventId == CP_XM_TRANSPORT_UPDATE)
    return "CP_XM_TRANSPORT_UPDATE";
  else if (eventId == XAC_XM_TRANSPORT_UPDATE)
    return "CP_XM_TRANSPORT_UPDATE";
  else if (eventId == XM_WIFI_USECASE_UPDATE)
    return "XM_WIFI_USECASE_UPDATE";
  else if (eventId == XM_WIFI_ENABLE_STATS)
    return "XM_WIFI_ENABLE_STATS";
  else if (eventId == CP_XM_BEARER_SWITCH_REQ)
    return "CP_XM_BEARER_SWITCH_REQ";
  else if (eventId == CP_XM_ADSP_STATE_IND)
    return "CP_XM_ADSP_STATE_IND";
  else if (eventId == QHCI_XM_AUDIO_STREAM_UPDATE)
    return "QHCI_XM_AUDIO_STREAM_UPDATE";
  else if (eventId == XAC_XP_MDNS_REQ)
    return "XAC_XP_MDNS_REQ";
  else if (eventId == XP_XAC_MDNS_RSP)
    return "XP_XAC_MDNS_RSP";
  else if (eventId == XAC_XP_START_FILTERED_SCAN)
    return "XAC_XP_START_FILTERED_SCAN";
  else if (eventId ==XAC_INITIATE_BLUETOOTH_CONNECTION_IND)
    return "XAC_INITIATE_BLUETOOTH_CONNECTION_IND";
  else if (eventId == XAC_CP_BURST_INT_REQ)
    return "XAC_CP_BURST_INT_REQ";
  else if (eventId == XP_WIFI_SET_AP_AVB_REQ)
      return "XP_WIFI_SET_AP_AVB_REQ";
  else if (eventId == XP_WIFI_CANCEL_AP_AVB)
      return "XP_WIFI_CANCEL_AP_AVB";
  else if (eventId == WIFI_XP_SET_AP_AVB_RSP)
      return "WIFI_XP_SET_AP_AVB_RSP";
  else if (eventId == WIFI_XM_CSA)
      return "WIFI_XM_CSA";
  else if (eventId == WIFI_XM_DRV_STATE)
      return "WIFI_XM_DRV_STATE";
  else if (eventId == WIFI_XM_COUNTRY_CODE_UPDATE)
      return "WIFI_XM_COUNTRY_CODE_UPDATE";
  else if (eventId == WIFI_XM_POSSIBLE_TO_CREATE_AP_IFACE)
      return "WIFI_XM_POSSIBLE_TO_CREATE_AP_IFACE";
  else if (eventId == XM_WIFI_STA_USECASE_UPDATE)
      return "XM_WIFI_STA_USECASE_UPDATE";
  else if (eventId == XAC_XP_CURRENT_TRANS_UPDATE)
      return "XAC_XP_CURRENT_TRANS_UPDATE";
  else if (eventId == XP_XAC_BONDED_STATE_UPDATE)
      return "XP_XAC_BONDED_STATE_UPDATE";
  else if (eventId == XP_XAC_LOCAL_AP_DETAILS)
        return "XP_XAC_LOCAL_AP_DETAILS";
  else if (eventId == XP_XM_GET_WIFI_STATUS)
        return "XP_XM_GET_WIFI_STATUS";
  else if (eventId == XM_WIFI_UNREGISTER_REGISTER_STA)
        return "XM_WIFI_UNREGISTER_REGISTER_STA";
  else if (eventId == XM_WIFI_REGISTER_STA)
        return "XM_WIFI_REGISTER_STA";
  else if (eventId == XAC_XP_REGISTER_MDNS_SERVICE)
        return "XAC_XP_REGISTER_MDNS_SERVICE";
  else if (eventId == XP_QHCI_CONNECT_LINK_REQ)
        return "XP_QHCI_CONNECT_LINK_REQ";
  else if (eventId == QHCI_XP_CONNECT_LINK_RSP)
        return "QHCI_XP_CONNECT_LINK_RSP";
  else if (eventId == XP_XAC_REMOTE_AP_DETAILS)
        return "XP_XAC_REMOTE_AP_DETAILS";
  else if (eventId == XP_XAC_HANDSET_PORT_REQ)
        return "XP_XAC_HANDSET_PORT_REQ";
  else if (eventId == XP_XAC_HANDSET_PORT_RSP)
        return "XP_XAC_HANDSET_PORT_RSP";
  else if (eventId == XAC_XM_PREPARE_AUDIO_BEARER_RSP)
        return "XAC_XM_PREPARE_AUDIO_BEARER_RSP";
  else if (eventId == XAC_CP_REMOTE_PARAMS_IND)
        return "XAC_CP_REMOTE_PARAMS_IND";
  else if (eventId == XAC_CP_REMOTE_PARAMS_REQ)
        return "XAC_CP_REMOTE_PARAMS_REQ";
  else if (eventId == XAC_CP_REMOTE_PARAMS_IND_DURING_ROAMING)
        return "XAC_CP_REMOTE_PARAMS_IND_DURING_ROAMING";
  else if (eventId == XP_WIFI_SCAN_STARTED)
        return "XP_WIFI_SCAN_STARTED";
  else if (eventId == XAC_CP_ROLE_SWITCH_IND)
        return "XAC_CP_ROLE_SWITCH_IND";
  else if (eventId == XAC_CP_BSSID_CHANGE_IND)
        return "XAC_CP_BSSID_CHANGE_IND";
  else if (eventId == XAC_XP_BEARER_SWITCH_FAILED_RSP)
        return "XAC_XP_BEARER_SWITCH_FAILED_RSP";
  else if (eventId == QHCI_XP_BEARER_PREFERENCE_REQ)
    return "QHCI_XP_BEARER_PREFERENCE_REQ";
  else
    return "INVALID Event ID";
}

} // namespace implementation
} // namespace xpan
#endif
