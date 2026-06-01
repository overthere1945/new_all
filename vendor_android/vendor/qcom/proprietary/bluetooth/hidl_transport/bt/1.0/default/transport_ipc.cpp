/*==========================================================================
Description
  It has implementation for IPC logging mechanism.

# Copyright (c) 2017 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

===========================================================================*/
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <utils/Log.h>
#include "logger.h"
#include <sys/types.h>
#include <dirent.h>
#include "data_handler.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#ifdef BT_VER_1_1
#define LOG_TAG "vendor.qti.bluetooth@1.0-uart_ipc"
#else
#define LOG_TAG "vendor.qti.bluetooth@1.0-uart_ipc"
#endif

#define UART_IPC_LOG_COLLECTION_OLD_PATH  "/sys/kernel/debug/ipc_logging/msm_serial_hs/log"
#define IPC_LOG_COLLECTION_DIR  "/sys/kernel/debug/ipc_logging/"

#define UART_IPC_LOG_TX_FILE_NAME    "uart_tx/log"
#define UART_IPC_LOG_RX_FILE_NAME    "uart_rx/log"
#define UART_IPC_LOG_PWR_FILE_NAME    "uart_pwr/log"
#define UART_IPC_LOG_STATE_FILE_NAME    "uart_state/log"
#define UART_IPC_LOG_STATE_FILE_NAME_845    "uart_misc/log"

#define SPI_IPC_LOG_TX_RX_FILE_NAME "spi_tx_rx/log"

#define BT_SOC_CHEROKEE 1

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

#ifdef DUMP_IPC_LOG

void IpcLogs :: DumpTransportLogs()
{
  int fd;
  bool ret;
  char dpath[TRANSPORT_IPC_PATH_BUF_SIZE];
  char spath[TRANSPORT_IPC_PATH_BUF_SIZE];
  char ipc_log_dir[TRANSPORT_IPC_PATH_BUF_SIZE];
  char const *TX_path;
  if(DataHandler::Get()->getTransportType() == BT_TRANSPORT_TYPE_SPI){
    TX_path = (char *)"spi_tx";
  }else{
    TX_path = (char *)"uart_tx";
  }
  struct DIR* p_dir;
  struct dirent* p_dirent;

  ALOGD("%s: -->", __func__);

  logger_ = static_cast<void*>(Logger::Get());

  static_cast<Logger*>(logger_)->GetTransportDumpFilename(STATE_LOG,dpath);

  strlcpy(ipc_log_dir, IPC_LOG_COLLECTION_DIR, sizeof(ipc_log_dir));

  /* Finding the IPC log source location */
  p_dir = opendir(IPC_LOG_COLLECTION_DIR);
  if (p_dir == NULL) {
    ALOGE("%s: Unable to open the Dir %s err: %s (%d)", __func__, IPC_LOG_COLLECTION_DIR,
      strerror(errno), errno);
    return;
  }

  while ((p_dirent = readdir(p_dir)) != NULL) {
    char *end;
    if ((end = strstr(p_dirent->d_name, TX_path))) {
      *end = '\0';
      strlcat(ipc_log_dir, p_dirent->d_name, sizeof(ipc_log_dir));
      break;
    }
  }
  closedir(p_dir);

  if(p_dirent == NULL) {
    ALOGE("%s: Did not find IPC source log files", __func__);
    return;
  }

  strlcpy(spath, ipc_log_dir, sizeof(spath));
  ALOGI("%s: found IPC source log location: %s", __func__, spath);

  if(DataHandler::Get()->getTransportType() == BT_TRANSPORT_TYPE_SPI){
    strlcat(spath, SPI_IPC_LOG_TX_RX_FILE_NAME, sizeof(spath));
    fd = open(spath, O_RDONLY | O_NONBLOCK);
    if(fd >= 0){
      close(fd);
      ret = DumpIpcLogs(spath, dpath, SPI_IPC_LOG_TX_RX_MAX_SIZE);
      ALOGD("%s: SPI ipc Tx/Rx logs written at %s", __func__, dpath);
    }else{
      ALOGE("%s: Error opening source file (%s) error (%s)", __func__, spath, strerror(errno) );
    }
  }else{
    strlcat(spath, UART_IPC_LOG_STATE_FILE_NAME, sizeof(spath));

    if ((fd = open(spath, O_RDONLY | O_NONBLOCK)) < 0) {
        strlcpy(spath, ipc_log_dir, sizeof(spath));
        strlcat(spath, UART_IPC_LOG_STATE_FILE_NAME_845, sizeof(spath));
        fd = open(spath, O_RDONLY | O_NONBLOCK);
    }

    if (fd >= 0) {
      close(fd);
      ret = DumpIpcLogs(spath, dpath, UART_IPC_LOG_MISC_MAX_SIZE);
      ALOGD("%s: UART ipc State logs written at %s", __func__, dpath);

      static_cast<Logger*>(logger_)->GetTransportDumpFilename(RX_LOG,dpath);
      strlcpy(spath, ipc_log_dir, sizeof(spath));
      strlcat(spath, UART_IPC_LOG_RX_FILE_NAME, sizeof(spath));
      ret = DumpIpcLogs(spath, dpath, UART_IPC_LOG_TX_RX_MAX_SIZE);
      ALOGD("%s: UART ipc RX logs written at %s", __func__, dpath);

      static_cast<Logger*>(logger_)->GetTransportDumpFilename(TX_LOG,dpath);
      strlcpy(spath, ipc_log_dir, sizeof(spath));
      strlcat(spath, UART_IPC_LOG_TX_FILE_NAME, sizeof(spath));
      ret = DumpIpcLogs(spath, dpath, UART_IPC_LOG_TX_RX_MAX_SIZE);
      ALOGD("%s: UART ipc TX logs written at %s", __func__, dpath);

      static_cast<Logger*>(logger_)->GetTransportDumpFilename(POWER_LOG,dpath);
      strlcpy(spath, ipc_log_dir, sizeof(spath));
      strlcat(spath, UART_IPC_LOG_PWR_FILE_NAME, sizeof(spath));
      ret = DumpIpcLogs(spath, dpath, UART_IPC_LOG_PWR_MAX_SIZE);
      ALOGD("%s: UART ipc Power logs written at %s", __func__, dpath);
    } else {
      ALOGE("%s: Error opening source file (%s) error (%s)", __func__, spath, strerror(errno) );
      DumpIpcLogs((char *)UART_IPC_LOG_COLLECTION_OLD_PATH, dpath, 1024 * 22);
    }
  }
}

bool IpcLogs :: DumpIpcLogs(const char *spath, const char *dpath, long log_limit)
{
  int src_fd = -1;
  int dest_fd = -1;
  long logs_written = 0;
  long ret;
  char *buff;

  ALOGD("%s: dump to %s for max size %ld bytes", __func__, dpath, log_limit);

  if(DataHandler::Get()->getTransportType() == BT_TRANSPORT_TYPE_SPI){
    buff = new (std::nothrow)char[SPI_IPC_LOG_MAX_READ_PER_ITERATION];
    if (buff == NULL) {
    ALOGE("%s: Error allocating memory %ld bytes for log buffer", __func__,
             SPI_IPC_LOG_MAX_READ_PER_ITERATION);
    return false;
  }
  }else{
    buff = new (std::nothrow)char[UART_IPC_LOG_MAX_READ_PER_ITERATION];
    ALOGE("%s: Error allocating memory %ld bytes for log buffer", __func__,
             UART_IPC_LOG_MAX_READ_PER_ITERATION);
    return false;
  }

  src_fd = open(spath, O_RDONLY | O_NONBLOCK);
  if (src_fd < 0) {
    ALOGE("%s: Error opening source file (%s) error (%s)", __func__, spath, strerror(errno) );
    delete[] buff;
    return false;
  }

  do {
    if(DataHandler::Get()->getTransportType() == BT_TRANSPORT_TYPE_SPI){
      ret = read(src_fd, buff, SPI_IPC_LOG_MAX_READ_PER_ITERATION);
    }else{
      ret = read(src_fd, buff, UART_IPC_LOG_MAX_READ_PER_ITERATION);
    }
    if (ret <= 0) {
      ALOGE("%s: Finish reading src file: %ld (%s)", __func__, ret, strerror(errno) );
      break;
    } else if (dest_fd == -1) {
      dest_fd = open(dpath, O_CREAT | O_SYNC | O_WRONLY,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (dest_fd < 0) {
        ALOGE("%s: Error opening destination file: %s (%s)", __func__, dpath, strerror(errno) );
        delete[] buff;
        close(src_fd);
        return false;
      }
    }

    ret = write(dest_fd, buff, ret);
    if (ret <= 0) {
      ALOGE("%s: Error writing to dest file: %ld (%s)", __func__, ret, strerror(errno) );
      break;
    }

    ALOGI("%s: Read/Written size = %ld", __func__, ret);

    logs_written += ret;
    if (logs_written >= log_limit) {
      ALOGE("%s: Have pulled enough UART IPC logs", __func__);
      break;
    }
  } while (1);

  delete[] buff;
  close(src_fd);

  if (fsync(dest_fd) == -1) {
    ALOGE("%s: Error while synchronization of logs in :%s error code:%s", __func__,
          spath, strerror(errno));
  }

  close(dest_fd);

  return true;
}

#endif

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
