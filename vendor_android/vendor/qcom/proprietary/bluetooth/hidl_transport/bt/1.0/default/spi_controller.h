/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <hidl/HidlSupport.h>
#include "controller.h"
#include "hci_internals.h"
#include "state_info.h"
#include "health_info_log.h"
#include "hci_spi_transport.h"
#include "notify_signal.h"

#define MAX_INVALID_BYTES 2
#define RX_THREAD_USAGE_TIMEOUT    (1500)
#define RX_THREAD_SCHEDULING_DELAY (50)
#define MAX_CONTINUOUS_RXTHREAD_STUCK (5)
#define PERI_WARM_RESET_DEFAULT_TIMEOUT_IN_MILLI_SECS 500
#define QCC_DISABLE_SUBSYS_TIMEOUT 50

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {


// singleton class
class SpiController : public Controller {
 public:
  // used to get the instance_ controller class
  virtual bool Init(PacketReadCallback packet_read_cb) override;
  // send packet is used to send the data. protocol type, actual packet types are expected.
  virtual size_t SendPacket(HciPacketType packet_type, const uint8_t *data, size_t length) override;
  virtual bool Cleanup(void) override;
  virtual SocAddOnFeatures_t* GetAddOnFeatureList() override;
  virtual uint64_t GetChipVersion() override;
  void OnDataReady(int fd);
  void ResetInvalidByteCounter();
  virtual int WaitforFwDownloadCmpl();
  virtual bool Disconnect();
  virtual void WaitforCrashdumpFinish();
  virtual void SignalCrashDumpFinish();
  virtual void UnlockControllerMutex();
  void StartSocCrashWaitTimer();
  void StartSocCrashWaitTimer(int ssrtimeout);
  bool StopSocCrashWaitTimer();
  void SendBqrRiePacket();
  int bt_kernel_panic(void);
  void ReportSocFailure(bool dumped_spi_log, PrimaryReasonCode reason,
                        bool cleanupSocCrashWaitTimer, bool cleanupIbs);
  static void SocCrashWaitTimeout(union sigval sig);
  void SendCrashPacket();
  void StartRxThreadTimer();
  void StopRxThreadTimer();
  double GetRxThreadSchedTSDiff();
  void SsrCleanup(PrimaryReasonCode reason);
  PrimaryReasonCode GetPreviousReason();
  bool IsBqrRieEnabled();
  void SetCleanupStatusDuringSSR() override;
  int ResetBaudrate(void);
  void periWarmResetTriggered();
  size_t SendPacketWithOptions(HciPacketType packet_type,
    const uint8_t *data, size_t length, uint32 options);
  SpiController(BluetoothSocType soc_type, BluetoothSecSocType sec_soc_type);
  ~SpiController();
  std::mutex ibs_clean_up_lock;
  NotifySignal *notifysignal_;
#ifdef IS_PERI_ENABLED
  enum SSRTypes {
    SSR_MODE_UNKNOWN = 0,
    NON_CHAINED_BT,
    CHAINED_PERI_AND_BT,
  };
  int mModulesInvovledInSSR = SSR_MODE_UNKNOWN;
  void setModulesInvolvedInSSR(SSRTypes ssr_type);
  int getModulesInvolvedInSSR();
#endif
  int SendActivationSSCommand(byte ssId , HciActivate action);
  void handleQ6SSR();
  bool isQ6SsrOngoing();

 private:
  BtState *state_info_;
  struct timeval time_wk_lockacq_rel_;
  static ThreadTimer recovery_timer_state_machine_;
  ThreadTimer rx_timer_state_machine_;
  std::condition_variable cv;
  std::mutex cv_m;
  bool DevInReset(int);
  int CheckDigPinConnectivity();
  static void RecoveryStuckTimeout(union sigval);
  void StartRecoveryStuckTimer();
  void StopRecoveryStuckTimer();
  void CleanupRecoveryStuckTimer();
  void HciTransportCleanup(bool);
  void Signalfwdwnldstate(void);
  bool GetCleanupStatus(void) override;
  void CleanupSocCrashWaitTimer();
  PrimaryReasonCode GetPrimaryReason();
  int TriggerSocCrashdump(uint8_t crash_code, PrimaryReasonCode reason);
  bool IsSecondaryReasonValid();
  void SetRxThreadTimerState(TimerState);
  bool ResetForceSsrTriggeredIfNoCleanup();
  bool IsSoCCrashNotNeeded(PrimaryReasonCode reason);
  void SendSpecialBuffer(PrimaryReasonCode reason);
  TimerState GetRxThreadTimerState();
  void updateCrashReasonAndDelayList();
  void CheckForBQRRootInflammationBit(const unsigned char* buf);
  static void RxThreadTimeOut(union sigval);
  bool CheckCleanupStatusDuringSSR();
  bool command_is_get_dbg_info(const unsigned char* buf, int len);
  using PeriWarmResetTimeoutCallback = std::function<void()>;
  void StartPeriWarmResetTimeout(int milli_seconds, PeriWarmResetTimeoutCallback peri_warm_reset_timeout_cb);
  void PeriWarmResetTimeout();
  static void *peri_warm_reset_timer_function(void* arg);
  typedef struct {
    int milli_seconds;
    std::function<void()> callback;
  } TimerData;

  HciPacketType hci_packet_type_{HCI_PACKET_TYPE_UNKNOWN};
  uint8_t host_id_{BT_ENDPT};
  BluetoothSocType soc_type_;
  BluetoothSecSocType sec_soc_type_;
  HealthInfoLog* health_info;
  int invalid_bytes_counter_;
  uint64_t chipset_ver_;
  uint64_t chipset_peri_ver_;
  std::mutex fwdwld_mtx;
  std::mutex controller_mutex;
  std::mutex check_cleanup_mutex_;
  std::condition_variable fwdwld_cv;
  hidl_vec<uint8_t> *packet_;
  PrimaryReasonCode prv_reason;
  bool is_cmd_timeout;
  bool is_invalid_pkt_from_soc;
  bool soc_crashed;
  bool is_soc_wakeup_failure;
  bool is_nmi_interrupt_triggered;
  //NotifySignal *notifysignal_;
  bool is_bqr_rie_enabled_; /* BQR Root Inflammation Event */
  bool is_bqr_rie_sent_already;
  bool is_spurious_wake;
  bool force_special_byte_enabled_;
  bool is_warm_reset_triggered;
  bool ss_failed_to_notify;
  int soc_crash_wait_timer_state_;
  bool cleanup_status_ssr_;
  timer_t soc_crash_wait_timer_;
  bool out_of_sync_;
  uint8_t inv_bytes[TX_RX_PKT_ASC_SIZE];
};

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
