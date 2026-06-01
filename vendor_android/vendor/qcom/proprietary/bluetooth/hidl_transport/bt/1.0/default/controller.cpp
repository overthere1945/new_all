/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <hidl/HidlSupport.h>
#include "power_manager.h"
#include "hci_transport.h"
#include "async_fd_watcher.h"
#include "hci_internals.h"
#include "hci_packetizer.h"
#include "logger.h"
#include "controller.h"
#include "data_handler.h"
#include "uart_controller.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

std::mutex cal_state_mutex_;
std::condition_variable cal_wait_cv;
static std::mutex cal_m;
static bool CalEvntRecvStatus;

void Controller :: updateCalEvntRecvStatus(bool status) {
  std::lock_guard<std::mutex> lock(cal_m);
  CalEvntRecvStatus = status;
}

void Controller::waitForCalDataToCollect() {

  std::unique_lock<std::mutex> lk(cal_m);

  if (CalEvntRecvStatus) {
    ALOGD("%s: SOC calibration data logged already, returing from here \n", __func__);
    return;
  }

  ALOGD("%s: Wait for %d ms to log calibration data\n", __func__, CALIBRATION_DATA_LOG_TIME);

  cal_wait_cv.wait_for (lk,
                       std::chrono::milliseconds(CALIBRATION_DATA_LOG_TIME),
                       [](){return (int)CalEvntRecvStatus;});

  if(CalEvntRecvStatus) {
    ALOGD("%s: Finished calibration data loging\n", __func__);
  } else {
    ALOGD("%s: Calibration data loging timed out\n", __func__);
    DataHandler *data_handler = DataHandler::Get();
    if (data_handler && data_handler->controller_ != nullptr) {
       if(data_handler->getTransportType() == BT_TRANSPORT_TYPE_SPI){
            SpiController * instance = (SpiController *)data_handler->controller_;
            instance->SsrCleanup(BT_HOST_REASON_SOC_CALIBRATION_DATA_RECV_TIMEOUT);
          }else{
            UartController * instance = (UartController *)data_handler->controller_;
            instance->SsrCleanup(BT_HOST_REASON_SOC_CALIBRATION_DATA_RECV_TIMEOUT);
          }

    }
  }
  return;
}

void Controller::SignalCalDataCollectFinish()
{
  std::lock_guard<std::mutex> lk(cal_m);
  ALOGD("%s: Notify to Calibration Thread\n", __func__);
  CalEvntRecvStatus = true;
  cal_wait_cv.notify_all();
}

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
