/*==========================================================================
Description
  It has implementation for IPC logging mechanism.

# Copyright (c) 2017,2021 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

===========================================================================*/
#pragma once

#include <hidl/HidlSupport.h>
#define UART_LOG_PREFIX "ramdump_bt_uart_ftrace_"
#define SPI_LOG_PREFIX "ramdump_bt_spi_ftrace_"

#define TRANSPORT_LOG_PATH_BUF_SIZE (255)
#define COMPLETE_TRANSPORT_LOGS (0xFFFFFFFF)

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

class TransportLogs {
 private:
  void *logger_;
  bool DumpTransportLogs(const char *, const char *, long);

 public:
  const unsigned long TRANSPORT_LOG_MAX_SIZE = COMPLETE_TRANSPORT_LOGS;
  const unsigned long TRANSPORT_LOG_MAX_READ_PER_ITERATION = 16 * 1024;

  void DumpLogs();
  TransportLogs() {};
};

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
