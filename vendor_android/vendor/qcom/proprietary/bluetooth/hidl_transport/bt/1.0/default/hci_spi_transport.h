/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <hidl/HidlSupport.h>
#include "hci_transport.h"
#include "wake_lock.h"
#include "uart_utils.h"
#include "health_info_log.h"

#define BT_SPI_DEVICE "/dev/spibt"

#define BT_ENDPT                                         0x00

#define HCI_BT_CMD                                       0x01
#define HCI_BT_DATA                                      0x02
#define HCI_BT_EVENT                                     0x04

#define BT_WRITE_RETRY_CNT  4
#define BT_SPI_DOORBELL_PENDING 0x00

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

class HciSpiTransport : public HciTransport {
 public:
  HciSpiTransport(HealthInfoLog* theHealthInfo) {
    ctrl_fd_ = -1;
    data_fd_ = -1;
    health_info = theHealthInfo;
  };
  virtual bool Init(BluetoothSocType soc_type) override;
  bool Init(BluetoothSocType soc_type, bool need_reload);
  static HciTransport* Get();
  virtual int GetCtrlFd() override;
  virtual int GetDataFd() override;
  virtual bool CleanUp(void) override;
  int Read(unsigned char* buf, size_t len) override;
  int Write(HciPacketType type, const uint8_t *buf, int len) override;
  int Write(const uint8_t *buf, int len) override;
  int WriteWithOptions(HciPacketType type, const uint8_t *buf, int len,
    uint32_t options);
  void Disconnect(bool) override;
  void Disconnect(bool, bool);
  int Read(unsigned char *protInd, unsigned char *hostId, unsigned char* buf, size_t len);
  int SetSocToCrash(uint8_t crash_code);

 private:
  int WriteSafely(const uint8_t *buf, int len);
  HealthInfoLog* health_info;
  void DeInitTransport(void);
  bool GetRxState();

};

/*
* Xfer status
*/
enum xfer_status {
	SUCCESS = 0,
	FAILURE = 1,
	TIMEOUT = 6,
	OTHER,
	INVALID_STATUS = -EINVAL,
};
/*
* command type: enum aligned with q2spi
*/
enum cmd_type {
	DATA_READ = 2,
	USER_DATA_WRITE = 3,
	HRF_WRITE = 5,
	SRESET = 6,
};
/*
* priority type
*/
enum priority_type {
	NORMAL = 0,
	HIGH,
};
/*
* spi client request
*/
struct spi_client_request {
	void *data_buff;
#ifdef IS_LW_VARIANT
	uint32_t unused;
#endif
	uint32_t data_len;
	uint8_t end_point;
	uint8_t proto_ind;
	enum cmd_type cmd;
	enum xfer_status status;
	uint8_t flow_id;
	uint32_t reserved[20];
};
/*
* spi user request
*/
struct spi_request {
	void *data_buff;
#ifdef IS_LW_VARIANT
	uint32_t unused;
#endif
	enum cmd_type cmd;
	uint32_t addr;
	uint8_t end_point;
	uint8_t proto_ind;
	uint32_t data_len;
	enum priority_type priority;
	uint8_t flow_id;
	bool sync;
	uint32_t reserved[20];
};

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android

