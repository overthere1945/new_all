/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <string>

// TG: TODO: Needs to be added in device tree
#define BT_SS_DATA_CH "/dev/glink_pkt_ss_bt_data"

#pragma once
class HciGlinkTransport {
public:
    HciGlinkTransport();
    ~HciGlinkTransport();
    int open(const std::string& dev);
    int close();

    // timeout in seconds, negative value means infinite timeout
    int poll(int wakeup_fd, int timeout);
    int read(uint8_t *data, size_t size);
    int write(uint8_t *buf, size_t buflen, size_t *bytes_written);
    void register_bt_pdr_cb(void (*)(void));

private:
    HciGlinkTransport(const HciGlinkTransport&) = delete;
    HciGlinkTransport& operator=(const HciGlinkTransport&) = delete;
    void (*bt_pdr_cb)(void) = nullptr;
    static HciGlinkTransport* instance;
    static const uint32_t MAX_CONN_RETRIES = 3;
    std::string mDeviceName;
    int mFd;
    bool isGlinkLogging;
};
