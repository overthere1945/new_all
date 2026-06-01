/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <unistd.h>
#include "hci_spi_transport.h"
#include <cutils/properties.h>
#include <utils/Log.h>
#include "data_handler.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#ifdef BT_VER_1_1
#define LOG_TAG "vendor.qti.bluetooth@1.1-spi_transport"
#else
#define LOG_TAG "vendor.qti.bluetooth@1.0-spi_transport"
#endif

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

bool HciSpiTransport::Init(BluetoothSocType soc_type, bool need_reload)
{
  bool status = false;

  ALOGD("%s:> soc_type: %d, need_reload: %d", __func__, soc_type, need_reload);

  this->soc_type_ = soc_type;

  switch (soc_type) {
    case BT_SOC_THEMISTO:
    {
      if(DataHandler::Get()->GetSpiDriverFd() < 0){
        ALOGD("%s: SPI Fd is Invalid. Open it");
        DataHandler::Get()->UpdateSpiDriverFd();
        ctrl_fd_ = DataHandler::Get()->GetSpiDriverFd();//open(BT_SPI_DEVICE, O_RDWR);
      }else{
        ALOGD("%s: SPI Fd is valid. Get access to it :: %d",__func__,DataHandler::Get()->GetSpiDriverFd());
        ctrl_fd_ = DataHandler::Get()->GetSpiDriverFd();
      }
      if (ctrl_fd_ == -1) {
		    ALOGE("%s: failed to open %s err %s", __func__, BT_SPI_DEVICE, strerror(errno));
		    status = false;
	    }else{
        ALOGE("%s: open successful %s",__func__,BT_SPI_DEVICE);
        status = true;
      }
    }
    break;
    default:
    {
      ALOGE("Unknown chip type: %d", soc_type);
    }
  }
  return status;
}

bool HciSpiTransport::Init(BluetoothSocType soc_type)
{
  return Init(soc_type, true);
}

// public functions
int HciSpiTransport::GetCtrlFd()
{
  return ctrl_fd_;
}

int HciSpiTransport::GetDataFd()
{
  return data_fd_;
}

bool HciSpiTransport::CleanUp()
{
  bool status = true;

  ALOGI("%s:> soc_type: %d", __func__, soc_type_);

  switch (soc_type_) {
    case BT_SOC_THEMISTO:
    {
      if(ctrl_fd_ >= 0) close(ctrl_fd_);
    }
    break;
    default:
    {
      ALOGE("Unknown chip type: %d", soc_type_);
    }
  }
  return status;
}

int HciSpiTransport::Read(unsigned char* buf, size_t len)
{
  int bytes_left, bytes_read = 0, read_offset;

  if (!len) {
    ALOGE("%s: read returned with len 0.", __func__);
    return 0;
  }

  bytes_left = len;
  read_offset = 0;

  do {
    bytes_read = TEMP_FAILURE_RETRY(read(ctrl_fd_, buf + read_offset, bytes_left));
    if (bytes_read < 0) {
      ALOGE("%s: Read error: %d (%s)", __func__, bytes_left, strerror(errno));
      return -1;
    } else if (bytes_read == 0) {
      ALOGE("%s: read returned 0, err = %s, read bytes: %d, expected: %d",
            __func__, strerror(errno), (unsigned int)(len - bytes_left),
            (unsigned int)len);
      return (len - bytes_left);
    } else {
      if (bytes_read < bytes_left) {
        ALOGV("Still there are %d bytes to read", bytes_left - bytes_read);
        bytes_left = bytes_left - bytes_read;
        read_offset = read_offset + bytes_read;
      } else {
        ALOGV("%s: done with read", __func__);
        break;
      }
    }
  } while (1);
  return len;
}

int HciSpiTransport::Read(unsigned char *protInd, unsigned char *hostId, unsigned char* buf, size_t len){
  int bytes_read = 0;
  struct spi_client_request clReq;

  if (len == 0) {
    ALOGV("%s: Invalid size buffer passed size:%d", __func__, len);
    return 0;
  }

  clReq.cmd = DATA_READ;
  clReq.data_buff = buf;

  bytes_read = read(ctrl_fd_, &clReq, sizeof(clReq));
  if(bytes_read == -1) {
    ALOGE("%s: Read error: Read returned EAGAIN. Read Again",__func__);
    usleep(10000);
    bytes_read = read(ctrl_fd_, &clReq, sizeof(clReq));
  }
  if (bytes_read < sizeof(clReq)) {
    ALOGE("%s: Read error: bytes_required:%d, bytes_read:%d (%s)",
          __func__, sizeof(clReq), bytes_read, strerror(errno));
    return -1;
  }

  if ((clReq.data_len == 0) || (clReq.data_len > len)) {
    if(len == HCI_MAX_EVENT_SIZE && clReq.proto_ind == LOG_BT_ACL_PACKET_TYPE){
       ALOGE("%s: SPI Read error. Waiting for Event Packet but got ACL Packet", __func__);
    }else{
       ALOGE("%s: SPI Read error. Received len:%d", __func__, clReq.data_len);
       return -1;
    }
  }

  *protInd = clReq.proto_ind;
  *hostId = clReq.end_point;

  return clReq.data_len;
}

int HciSpiTransport::Write(const uint8_t *buf, int len)
{
  std::unique_lock<std::mutex> guard(internal_mutex_);
  spi_request req;
  req.cmd = HRF_WRITE;
  req.addr = 0;
  req.end_point = BT_ENDPT;
  req.sync = 1;
  req.flow_id = 1;
  req.priority = HIGH;
  req.proto_ind = HCI_PACKET_TYPE_UNKNOWN;
  req.data_len = len;
  req.data_buff = (void *)buf;
  req.reserved[0] = 0;
#ifdef IS_LW_VARIANT
  req.unused = 0;
#endif

  //for (int i=0; i<len; i++){
   // ALOGE("buf[%d] is :: %02x\n", i, buf[i]);
  //}

  return WriteSafely((uint8_t*)&req, sizeof(req));
}
int HciSpiTransport::Write( HciPacketType type, const uint8_t *buf, int len)
{
  return WriteWithOptions(type, buf, len, SPI_DEFAULT_SLEEP_BIT);
}

int HciSpiTransport::WriteWithOptions( HciPacketType type, const uint8_t *buf,
  int len, uint32_t options)
{
  int ret = -1;
  std::unique_lock<std::mutex> guard(internal_mutex_);
  spi_request req;
  if(type == HCI_PACKET_TYPE_PERI_CMD){
    const uint8_t *peri_buf = buf+2;
    req.data_buff = (void *)peri_buf;
    req.data_len = len - 2;
  }else if(type == HCI_PACKET_TYPE_CMD_FROM_HAL){
    const uint8_t *bt_buf = buf+1;
    req.data_buff = (void *)bt_buf;
    req.data_len = len - 1;
  }else{
    req.data_buff = (void *)buf;
    req.data_len = len;
  }
  req.cmd = HRF_WRITE;
  req.addr = 0;
  req.end_point = BT_ENDPT;
  req.sync = 1;
  req.flow_id = 1;
  req.priority = HIGH;
  if(type ==  HCI_PACKET_TYPE_CMD_FROM_HAL){
    req.proto_ind = HCI_PACKET_TYPE_COMMAND;
  }else{
    req.proto_ind = type;
  }
  req.reserved[0] = options;

#ifdef IS_LW_VARIANT
  req.unused = 0;
#endif

  ret = WriteSafely((uint8_t*)&req, sizeof(req));

  if (ret != sizeof(req)){
      ALOGE("%s: SPI write failed(0x%x)", __func__, ret);
      return ret;
  }else{
      ret = len;
  }
  return len;
}

int HciSpiTransport::WriteSafely(const uint8_t *data, int length)
{
  int write_len = 0;
  int retry_cnt = 0;

  while (length > 0) {
    ssize_t ret =
      TEMP_FAILURE_RETRY(write(ctrl_fd_, data + write_len, length));


    if (ret < 0) {
      if (errno == EAGAIN) continue;
      ALOGE("%s error writing to SPI (%s)", __func__, strerror(errno));
      break;

    } else if (ret == 0) {
      // Nothing written :(
        if (retry_cnt > BT_WRITE_RETRY_CNT) {
            ALOGE("%s zero bytes written - something went wrong... tried %d times", __func__, retry_cnt);
            if (GetRxState()) {
              ALOGW("%s: Doorbell pending, returning", __func__);
              return BT_SPI_DOORBELL_PENDING;
            }
            break;
        }
        retry_cnt++;
    } else if (length != ret) {
      ALOGE("%s: Only %d bytes written - continue writing...", __func__, ret);
    }
    write_len += ret;
    length -= ret;
  }
  return write_len;
}

void HciSpiTransport::DeInitTransport(void)
{
  int ret;

  ALOGD("%s: ctrl_fd_ = %d", __func__, ctrl_fd_);
  if (!ctrl_fd_ || ctrl_fd_ == -1) {
    return;
  }

  internal_mutex_.lock();

  ALOGD("%s: Transport is being closed!", __func__);
  DataHandler::Get()->sendPassthroughDisable();

  if ((ret = close(ctrl_fd_)) < 0 ) {
    ALOGE("%s: Close returned Error: %d\n", __func__, ret);
  }

  ctrl_fd_ = -1;
  internal_mutex_.unlock();

  DataHandler::Get()->CloseSpiDriverFd();
}

void HciSpiTransport::Disconnect(bool need_reload, bool wakeup_needed) {
  DeInitTransport();
}

void HciSpiTransport::Disconnect(bool wakeup_needed) {
  return Disconnect(true, wakeup_needed);
}

bool HciSpiTransport::GetRxState() {
  struct timeval tv;
  fd_set rfds;

  FD_ZERO(&rfds);
  FD_SET(ctrl_fd_, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  int ret = select(ctrl_fd_ + 1, &rfds, NULL, NULL, &tv);
  if(ret > 0 && FD_ISSET(ctrl_fd_, &rfds)) {
    ALOGD("%s: something to read", __func__);
    return true;
  }
  return false;
}

int HciSpiTransport::SetSocToCrash(uint8_t crash_code)
{
  int len = Write(&crash_code, 1);
  if (len != 1)
    ALOGE("%s: Send failed with ret value: %d", __func__, len);
  return len;
}

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android

