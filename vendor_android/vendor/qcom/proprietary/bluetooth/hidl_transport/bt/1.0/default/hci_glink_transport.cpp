/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "hci_glink_transport.h"
#include <cutils/properties.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <utils/Log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.bluetooth@1.0-hci_glink_transport"

HciGlinkTransport::HciGlinkTransport() : mFd(-1) {
    ALOGI("HciGlinkTransport ctor");
}

HciGlinkTransport::~HciGlinkTransport() {
    ALOGI("HciGlinkTransport dtor");
    if (-1 != mFd) {
        ::close(mFd);
        mFd = -1;
    }
    bt_pdr_cb = nullptr;
}

int HciGlinkTransport::open(const std::string& dev) {
    int tries = MAX_CONN_RETRIES;
    mDeviceName = dev;
    ALOGI("open '%s'", mDeviceName.c_str());
    int ret = 0;
    char glink_logging[PROPERTY_VALUE_MAX];

    do {
        mFd = ::open(mDeviceName.c_str(), O_RDWR);
        --tries;

        if (mFd > 0) {
            ALOGI("open: %s: success fd=%d", dev.c_str(), mFd);
            ret = property_get("persist.vendor.qcom.bluetooth.glink.logging", glink_logging,"false");
            if (strcmp(glink_logging, "true") == 0)
                isGlinkLogging = true;
            else
                isGlinkLogging = false;
            break;
        }

        ALOGE("open: %s: open error(%s)", dev.c_str(), strerror(errno));

        if (-ETIMEDOUT == errno) {
            ALOGE("open: %s: ETIMEDOUT", dev.c_str());
            sleep(1);
        } else {
            ALOGE("open: %s: giving up", dev.c_str());
            break;
        }

    } while(-ETIMEDOUT == errno && tries > 0 );

    return mFd;
}

int HciGlinkTransport::close() {
    ALOGI("close :: fd = %d",mFd);
    if (mFd >= 0)  {
        ::close(mFd);
        mFd = -1;
    }
    bt_pdr_cb = nullptr;
    return 0;
}

int HciGlinkTransport::poll(int wakeup_fd, int timeout) {
  ssize_t rc = 0;
  struct pollfd poll_fd[2];

  // wait for Rx data available in fd, for 'timeout' secs
  poll_fd[0].fd = mFd;
  poll_fd[0].events = POLLIN | POLLHUP;

  poll_fd[1].fd = wakeup_fd;
  poll_fd[1].events = POLLIN;

  do {
      rc = ::poll(poll_fd, 2, timeout * 1000);
      if(isGlinkLogging) {
        ALOGE("%s returns %d, errno: %d ",__func__,(int)rc, errno);
      }
  } while (rc == -1 && errno == EINTR);

  if(rc > 0) {
    if (poll_fd[0].revents & POLLIN) {
      return 0;
    }

    if (poll_fd[0].revents & POLLHUP) {
      ALOGE("poll: SSR detected");
      if(bt_pdr_cb != nullptr) {
        bt_pdr_cb();
      }
      else {
        ALOGE("%s: bt_pdr client is not registered", __func__);
      }
    }

    if (poll_fd[1].revents & POLLIN) {
      char buf[1];
      size_t n = ::read(wakeup_fd, buf, 1);
      ALOGD("n is :: %d buf is :: %c",n, buf[0]);
      if (n == 0) {
          ALOGD("Wake-up pipe closed (EOF)");
          return -2; // Fail-Safe : Not breaking Poll on Glink FD
      }else if (n == 1 && buf[0] == '0') {
          ALOGD("Wake-up signal received, Exit Poll");
      }else{
          ALOGD("Unexpected wake-up signal: n=%zd, buf=%d", n, buf[0]);
          return -2; // Fail-Safe : Not breaking Poll on Glink FD
      }
    }
  }

  return -1;
}

int HciGlinkTransport::read(uint8_t *data, size_t size) {
  ssize_t rc = 0;

  if (mFd < 0) {
    return -1;
  }

  rc = ::read(mFd, data, size);
  if (rc < 0) {
    if (errno != EAGAIN) {
      ALOGE("read: Read error: %s, rc %d", strerror(errno), (int)rc);
      return -1;
    }
  }
  else if (rc == 0) {
    ALOGE("read: Zero length packet received or hardware connection went off");
  }

  if(isGlinkLogging) {
    std::ostringstream hstr;
    for (int i=0; i< rc; ++i)
    {
      hstr  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(data[i]);
    }

    ALOGD("read string: read [%s]", hstr.str().c_str());

    for(int i=1; i<=8;i++)
    {
      ALOGD("last bytes=%d",data[size-i]);
    }
    ALOGD("read: read %d of %d bytes", (int)rc, (int)size);
  }
  return rc;
}

int HciGlinkTransport::write(uint8_t *buf, size_t buflen, size_t *bytes_written) {
  size_t bytes_written_out = 0;
  int rc = 0;

  if (bytes_written) {
    *bytes_written = 0;
  }

  if (mFd < 0) {
    ALOGE("write: invalid fd");
    return -1;
  }

  if (buflen <= 0) {
    ALOGE("write: nothing to do");
    return 0;
  }

  while (bytes_written_out < buflen) {

    if(isGlinkLogging)
      ALOGD("write: writing %d bytes", (int)(buflen-bytes_written_out));

    rc = ::write (mFd, buf+bytes_written_out, buflen-bytes_written_out);

    if(isGlinkLogging)
      ALOGD("write: wrote %d bytes", rc);

    if (rc < 0) {
      ALOGE("write: Write returned failure %d errno=%d", rc, errno);
      return -1;
    }

    if(isGlinkLogging) {
      int offset = bytes_written_out;
      std::ostringstream hstr;
      for (int i=0; i< rc; ++i)
      {
        hstr  << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(buf[i+offset]);
      }
      ALOGD("write: wrote [%s]", hstr.str().c_str());
    }
    bytes_written_out += rc;
    if(isGlinkLogging)
      ALOGD("write: total written %d bytes", bytes_written_out);
  };

  if (bytes_written) {
    *bytes_written = bytes_written_out;
  }
  return 0;
}

void HciGlinkTransport::register_bt_pdr_cb(void (*bt_pdr_ptr)(void)) {
  ALOGE("%s: called", __func__);
  bt_pdr_cb = bt_pdr_ptr;
}
