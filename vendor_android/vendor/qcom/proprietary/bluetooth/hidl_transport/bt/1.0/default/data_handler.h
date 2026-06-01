/*
 * Copyright (c) 2017 - 2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */
//
// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once
#include <hidl/HidlSupport.h>
#include "hci_transport.h"
#include "hci_uart_transport.h"
#include <thread>
#include "controller.h"
#include "uart_controller.h"
#include "mct_controller.h"
#include "spi_controller.h"
#include "wake_lock.h"
#include "diag_interface.h"
#ifdef IS_PERI_ENABLED
#include "notify_signal.h"
#endif
#include <mutex>
#ifdef SECURE_PERIPHERAL_ENABLED
extern "C" {
#include "CPeripheralAccessControl.h"
#include "IPeripheralState.h"
#include "peripheralStateUtils.h"
}
#endif

//#ifdef PASSTHROUGH_ENABLED
#include "hci_glink_transport.h"
#include "g_offload_constants.h"
//#endif

#define OPCODE_LEN (7)
#define HCICMD_BUFF_SIZE (64)
#define TIME_STAMP_LEN (13)
#define HCI_WRITE_BD_ADDRESS_LENGTH 9
#define HCI_WRITE_BD_ADDRESS_OFFSET 3
#define MIN_CLIENTS_ACTIVE 1
#define HOST_ADD_ON_FEATURE_MASK_MAX_LENGTH 0xFF
#define PCS_NOTIFICATION_SIGNAL  (0xFF)

#define QSH_BLE_SNOOP_ENABLE_PROPERTY "persist.vendor.service.bt.qsh_ble.enablesnoop"
#define HCI_VS_OFFLOAD_HOST_COMM_CMD_OCF 0x3C
#define HCI_VS_OFFLOAD_HOST_SET_CONFIG_SUB_OPCODE 0
#define HCI_VS_OFFLOAD_HOST_MAX_CONFIG_LEN 0x18
#define HCI_VS_OFFLOAD_HOST_BTSNOOP_BIT_MASK 0x01
#define BT_CLOSE_TIMEOUT 30000

//Calibration Macros
#define START_CALIBRATION_TIME              1000
#define HCI_VS_RADIO_READ_CAL_NVM_DATA     (0xFC3B)

#ifdef IS_PERI_ENABLED
// Peri commands
#define HCI_VS_GENERAL_OPCODE_PERI         (0xFFF1)
#define HCI_PERI_EVT_SS_ACTIVATE_COMPLETE  (0x02)
#endif

typedef struct {
  uint8_t feat_mask_len;
  uint8_t features[HOST_ADD_ON_FEATURE_MASK_MAX_LENGTH];
} HostAddOnFeatures_t;

#ifdef USER_DEBUG
#define COMMAND_RESET_UART_FLOW_ON \
  { 0x00, 0xFD, 0x00 }
#define COMMAND_GET_CTS_STATUS \
  { 0x10, 0xFD, 0x00 }
#define FRAME_GET_CTS_SOC_STATUS \
  { 0x0e, 0x06, 0x01, 0x0c, 0xfc, 0x00, 0x3a }
#define COMMAND_GET_SOC_CTS_STATUS \
  { 0x0c, 0xfc, 0x01, 0x39 }
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#ifdef QTI_BT_FMD_SUPPORTED
#define ADV_KEYS_DATA_SIZE 5120
typedef struct {
  uint16_t adv_interval;
  uint8_t no_of_keys;
  uint16_t length;
  uint8_t state;
  bool fmd_mode_progress;
  bool fmd_enable_sent;
} fmd_mode_info_t;

typedef struct {
  bool prop_read;
  uint8_t header_length;
  uint8_t tx_pwr;
  uint8_t restart_keys;
  uint8_t per_pkt_key_length;
  uint8_t adv_interval[2];
  uint8_t advData_Timeout[4];
  uint8_t config_header[20];
} fmd_config_header_t;

#define HCI_VSC_FMD_ADV_DATA_HEADER_SIZE 5
#define HCI_VSC_FMD_ADV_DATA_PKT_LEN 249
#define FMD_TOTAL_ADV_DATA_KEY_SIZE 5080
#define LE_MAX_ADV_INTERVAL 0x7D00
#define MAX_KEY_ROTATION_TIMER_VALUE 0x5265C00
#define MAX_FMD_CONFIG_HEADER_LENGTH 11
#endif

#ifdef QTI_BT_FMD_SUPPORTED
typedef struct {
  uint8_t fmdOperation;
  uint8_t socFwVer;
  int16_t rebootStatus;
  int16_t fmd_stop_counter;
} fmdOperationStruct;
#endif

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_vec;

using DataReadCallback = std::function<void(HciPacketType, const hidl_vec<uint8_t>*)>;
using InitializeCallback = std::function<void(bool success)>;
using OnRegisterCallback = std::function<void(bool status)>;

enum InitStatusType {
  INIT_STATUS_IDLE = 0,
  INIT_STATUS_INITIALIZING = 1,
  INIT_STATUS_FAILED = 2,
  INIT_STATUS_SUCCESS = 3
};

enum HciCommand {
  HCI_RESET_CMD = 1,
  SOC_PRE_SHUTDOWN_CMD,
  HCI_WRITE_BD_ADDRESS,
  HCI_SET_OFFLOAD_HOST_CONFIG_CMD,
  HCI_ACTIVATE_SS_CMD,
  HCI_SET_BAUDRATE_CMD,
  HCI_READ_CAL_NVM_DATA,
#ifdef QTI_BT_FMD_SUPPORTED
  HCI_SET_FMD_CONFIG_PARAMS_CMD,
  HCI_WRITE_FMD_ADV_DATA_CMD,
  HCI_SET_FMD_MODE_CMD,
  HCI_SET_FMD_CONFIG_PARAMS_V2_CMD,
#endif
#ifdef PASSTHROUGH_ENABLED
  HCI_VSC_PERI_BT_SPI_TXP_SWITCH,
  HCI_VSC_PERI_BT_UART_TXP_SWITCH
#endif
};

typedef struct {
  ProtocolType type;
  bool is_pending_init_cb;
  InitializeCallback init_cb;
  DataReadCallback data_read_cb;
} ProtocolCallbacksType;

typedef struct {
  char hcicmd_timestamp[HCICMD_BUFF_SIZE];
  char opcode[OPCODE_LEN];
} HciCmdTimestamp;
// it is the interface to the all external class like BT and FM and ANT.

typedef struct {
  TimerState timer_state;
  timer_t timer_id;
} InitTimer;

enum class CAL_STATUS: uint8_t {
  SOC_CALIBRATION_SUCCESSFULL =     0x00,
  ERR_UNSUPPORTED_FEATURE_PARAM =   0x11,
  ERR_INVALID_HCI_COMMAND_PARAM =   0x12,
  ERR_NO_NEW_CAL_DATA =             0xF0,
  ERR_NO_VALID_CAL_DATA =           0xF1,
  CALIBRATION_STATUS_IDLE =         0xFF,
};

enum HalStatus{
  HAL_SUCCESS = 0x00,
  HAL_ALREADY_INITIALIZED,
  HAL_UNABLE_TO_OPEN_INTERFACE,
  HAL_HARDWARE_INITIALIZATION_ERROR,
  HAL_UNKNOWN
};

#ifdef QTI_BT_FMD_SUPPORTED
enum FmdOperation {
  ENABLE_FMD,
  DISABLE_FMD,
  UPDATE_SOC_VER
};
#endif

class DataHandler {
 public:
  // Function gives the SPI Client driver file descriptor
  int GetSpiDriverFd(void);
  bool UpdateSpiDriverFd(void);

  // Function Close the SPI Client driver file descriptor
  void CloseSpiDriverFd(void);

  // Functions to set and get the status of FTM
  static void setFtmStatus(bool status);
  static bool getFtmStatus(void);

  // with this APIs respective modules will register with their dataread cllback.
  // callback implementaiton varies from BT to FM.
  static HalStatus Init(ProtocolType type, InitializeCallback init_cb,
      DataReadCallback data_read_cb);
  static void CleanUp (ProtocolType type);
  static DataHandler* Get();
  BluetoothSocType GetSocType();
  BluetoothSecSocType GetSecSocType();
  bool IsSocAlwaysOnEnabled();
  static int32_t NotifyEvent(const uint32_t peripheral, const uint8_t state);

  /*Sends command and wait for event*/
  bool sendCommandWait(HciCommand cmd, ProtocolType type);

  // this is used to send the actual packet.
  size_t SendData(ProtocolType type, HciPacketType packet_type, const uint8_t* data, size_t length);
  struct timeval time_hci_cmd_arrived_;
  HciCmdTimestamp last_hci_cmd_timestamp_;

  void AddHciCommandTag(char* dest_tag_str,  struct timeval& time_val, char * opcode);
  static bool CheckSignalCaughtStatus();
  void SetSignalCaught();
  bool SendBTSarData(const uint8_t* data, size_t length, DataReadCallback event_cb);
  bool SendBtTpiData(const uint8_t* data, size_t length, DataReadCallback event_cb);
  void registerTpiAsyncEventCb(DataReadCallback event_cb);
  void unRegisterTpiAsyncEventCb();
#ifdef QTI_BT_LMP_EVENT_SUPPORTED
  void registerBtLmpAsyncEventCb(const uint8_t* data, size_t length,
                                 DataReadCallback event_cb,
                                 OnRegisterCallback register_cb);
  void unregisterBtLmpAsyncEventCb(const uint8_t* data, size_t length);
#endif
  bool isBtTpiEventFlag;
  uint16_t cmd_opcode;
  uint8_t  cmd_subopcode;
  uint16_t sar_cmd_opcode;
  uint8_t sar_cmd_subopcode;
  SocAddOnFeatures_t* GetSoCAddOnFeatures();
  HostAddOnFeatures_t* GetHostAddOnFeatures();
  uint64_t GetChipVersion();
  unsigned int GetRxthreadStatus(ProtocolType);
  unsigned int GetClientStatus(ProtocolType);
  void SetRxthreadStatus(bool, ProtocolType);
  void SetClientStatus(bool, ProtocolType);
  void StartSocCrashWaitTimer();
  void SendBqrRiePacket();
  int IsXMEMEnabled();
  void StopInitTimer();
  int GetInitStatus();
  void logCalibrationData();
  bool IsHciPacketValid(HciPacketType type);
  ProtocolType GetProtocol(HciPacketType pkt_type);
  Controller* GetControllerRef();
#ifdef XPAN_SUPPORTED
  void QHciInitialized(bool);
  void OnPacketReadyFromQHci(HciPacketType type,
          const hidl_vec<uint8_t>*hidl_data, bool from_soc);
  void UpdateRingBufferFromQHci
          (HciPacketType packet_type, const uint8_t *data, int length);
#endif

#ifdef USER_DEBUG
  void SetTransport(HciUartTransport *);
  int  HandleFlowControl(userial_vendor_ioctl_op_t op);
  void SendCTSStatusToClient(unsigned char status);
  int  GetCTSLineStatus();
  bool command_is_get_rts_status(const unsigned char* buf, int len);
  bool command_is_get_cts_status(const unsigned char* buf, int len);
  bool command_is_reset_uart_flow(const unsigned char* buf, int len);
#endif
  void StackTimeoutTriggered();
  void LogPwrSrcsUartFlowCtrl();
  bool CheckForUartFailureStatus();
  void KillInitThread();
  std::thread::id GetInitThreadId();
  void XMInitialized(bool);
  BluetoothTransportType getTransportType();
#ifdef QTI_BT_FMD_SUPPORTED
  bool IsFmdModeEnabled();
  void SetFmdMode(bool enable);
  void sendEids(uint8_t *key_data, uint16_t length);
  bool IsFmdModeEnabledSendToSoc();
  void GetFmdMode(bool enable);
  void CheckAndSetFmdMode();
  void GenerateKeyData();
  bool IsShutdownFMDTriggered();
  void UserSetFmdMode(bool enable);
  bool GetFmdMode();
  void SendFmdCmdsToSoc();
  void GetFmdHeaderDataInfo();
  void GetFmdConfigInfo();
  int fmdOperationNotifyToSPIDriver(enum FmdOperation operation);
  const char* GetFmdOperationString(uint8_t operation);
  void GetFmdPropValues();
  void SetDefaultFmdConfigParams();
  void FmdWaitForKeys(int time_out);
  void FmdFinderKeysReceived();
  void logFmdCmd(const uint8_t *data, int length);
#endif
  Controller *controller_ = nullptr;
  void calSignalToClose();
  bool closeWaitForCal(int time_out);
  CAL_STATUS getCalibrationStatus();
  CAL_STATUS soc_cal_status;
  void startCalibrationThread();
  void updateSocCalibrationSupportStatus(char status_byte);
  bool isSocSupportCal;
  bool getFarmeType();
  void updateCalFarmeType(void);
#ifdef PASSTHROUGH_ENABLED
  void sendPassthroughDisable();
  void setIsPassthroughEnabled(bool isPassthroughEnableSuccess);
  bool getIsPassthroughEnabled();
#endif

 private:
  DataHandler();
  static void data_handler_exit_cb(int s);
  static void usr2_handler(int);
  HalStatus Open(ProtocolType type, InitializeCallback init_cb, DataReadCallback data_read_cb);
  bool Close(ProtocolType type);
  static int data_service_setup_sighandler(void);
  static void data_service_sighandler(int signum);
  static int block_signal(int signum,bool block);
  BluetoothSocType GetSocTypeInt();
  bool isProtocolInitialized(ProtocolType type);
  void OnPacketReady(ProtocolType , HciPacketType, const hidl_vec<uint8_t>*, bool from_soc = true);
  void InternalOnPacketReady(ProtocolType , HciPacketType,
                             const hidl_vec<uint8_t>*, bool from_soc = true);
  bool isAwaitedEvent(const uint8_t *buff, uint16_t len);
  int pwr_drv_Fd = -1;
#ifdef IS_PERI_ENABLED
  bool CheckPeripheralCoreActivityCode(SecondaryReasonCode activity_code);
  bool isAwaitedPeriEvent(const uint8_t *buff, uint16_t len);
  bool isAwaitedPeriNtf(const uint8_t *buff, uint16_t len);
  bool isAwaitedPeriBDEvt(const uint8_t *, uint16_t);
#ifdef PASSTHROUGH_ENABLED
  bool isAwaiterPeriSPITxpSwitchEvt(const uint8_t *, uint16_t);
#endif
  bool ChangeSoCbd(void);
  bool DeactivatePeri(void);
  bool isBatteryLevelCritical(void);
#endif
  void UpdateRingBuffer(HciPacketType packet_type, const uint8_t *data, int length);
  bool isBTSarEvent(HciPacketType type, const uint8_t* data);
  bool isBtTpiEvent(HciPacketType type, const uint8_t* data);
  bool isBtTpiAsyncEvent(HciPacketType type, const uint8_t* data);
  bool isBtTpiAsyncVSEvent(HciPacketType type, const uint8_t* data);

#ifdef QTI_BT_LMP_EVENT_SUPPORTED
  bool isBtLmpAsyncEvent(const uint8_t* data);
  bool isBtLmpRegisterEvent(const uint8_t* data);
#endif
  bool isProtocolAdded(ProtocolType type);
  static void InitTimeOut(union sigval);
  static void InitiateShutdown(union sigval);
  void StartInitTimer();
  static void StartShutdownTimer();
  TimerState GetInitTimerState();
  void SetInitTimerState(TimerState);
  void SetAppropriateLog(HciCommand, bool, ProtocolType);
  void SetHostAddOnFeatures(uint64_t chip_ver);
  void SetScramblingFeature(uint64_t chip_ver);
  const char * convertChipVerToStr(uint64_t chip_ver);
#ifdef BT_CP_CONNECTED
  bool GetXMState(void);
  bool UpdateCopVersion(CoPVerSupported);
#endif
  void SetOffloadHostConfig(ProtocolType);
  bool MakeHciCmdForSetOffloadHostConfig(uint8_t *cmd, size_t *len);
#ifdef BTOFF_DELAY_SUPPORT
  static bool checkBTCloseDelay(ProtocolType type);
  static bool BTCloseDelay(ProtocolType);
  static bool isBtOffDelaySupport();
  static int getBTOffDelayTimeoutValue();
  static int getBtOffTimerState();
  static void setProtocolCallback(InitializeCallback init_cb, DataReadCallback data_read_cb);
  static void BtOffDelayTimeout(union sigval);
  static void StartBTOffTimer();
  static void StopBTOffTimer();
#endif

  virtual ~DataHandler() = default;
#ifdef XPAN_SUPPORTED
  bool GetQHciState(void);
  void XpanInitModules(void);
#endif
#ifdef QTI_BT_FMD_SUPPORTED
  bool IsSocAcceptedFmdInfo(const hidl_vec <uint8_t> *hidl_data);
#endif

  static void CleanupThreadTimeOut(union sigval sig);
  TimerState GetCleanupThreadTimerState();
  void SetCleanupThreadTimerState(TimerState);
  void StartCleanupTimer();
  void StopCleanupThreadTimer();

  static bool caught_signal;
  DiagInterface diag_interface_;
#ifdef USER_DEBUG
  HciUartTransport *uart_transport_;
#endif
  Logger *logger_;
  std::mutex property_cal_tread_mutex_;
#ifdef IS_PERI_ENABLED
  NotifySignal *notifysignal_;
#endif
  std::mutex internal_mutex_;
  static int init_status_;
  std::thread init_thread_;
  BluetoothSocType soc_type_;
  BluetoothSecSocType sec_soc_type_;
  std::map<ProtocolType , ProtocolCallbacksType *> protocol_info_;
  DataReadCallback btsar_event_cb = nullptr;
  DataReadCallback bttpi_event_cb = nullptr;
  DataReadCallback bttpi_asyncevent_cb = nullptr;
#ifdef QTI_BT_LMP_EVENT_SUPPORTED
  DataReadCallback btlmp_asyncevent_cb = nullptr;
  OnRegisterCallback btlmp_register_cb = nullptr;
#endif
  double GetInitCloseTSDiff();
  InitTimer init_timer_ = {TIMER_NOT_CREATED, 0};
  static std::mutex init_timer_mutex_;
  bool is_xmem_read_;
  int xmem_prop_val_;
  HostAddOnFeatures_t host_add_on_features;
  std::thread::id init_thread_id;
  bool is_init_thread_killed;
  std::thread prop_reset_thread;
  bool is_last_client_cleanup_in_progress;
#ifdef XPAN_SUPPORTED
  bool qhci_initialized;
#endif
  std::mutex property_reset_mutex_;
#ifdef BT_SECURE_PERIPHERAL_ENABLED
  static uint8_t currSecureState;
  static void *periContext;
  static timer_t shutdown_timer_id;
#endif
  static bool secureEvent;
#ifdef BT_CP_CONNECTED
  bool xpan_manager_initialized;
#endif
#ifdef PASSTHROUGH_ENABLED
  HciGlinkTransport* hci_arbiter_transport = nullptr;
  std::vector<uint8_t> build_switch_spi_to_ipc_vsc();
  std::vector<uint8_t> build_switch_uart_to_spi_vsc();
  std::string build_passthrough_enable_msg();
  std::string build_passthrough_disable_msg();
  void processHciArbiterRx();
  void sendPacketOnHCIArbiterGlink(HciPacketType packet_type,const uint8_t *data,
    size_t length);
  bool is_uart_flow_off_successful = false;
  std::unique_ptr<std::thread> glink_rx_thread;
  std::atomic<bool> running_arbiter_ch_{false};
  bool is_passthrough_enable_success = false;
  static pthread_cond_t passthrough_enable_disable_cond;
  static pthread_mutex_t passthrough_enable_disable_lock;
  int wakeup_pipe_fd_[2];
  static void bt_pdr_handler(void);
#endif
#ifdef QTI_BT_FMD_SUPPORTED
  bool fmd_mode;
#endif
  static bool offload_host_config_sent;
  ThreadTimer cleanup_timer_state_machine_;
  BluetoothTransportType transport_type_;
};

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
