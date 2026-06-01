/*****************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************/

#ifndef BTAVS_PROVIDER_SERVICE_H
#define BTAVS_PROVIDER_SERVICE_H

#include <aidl/vendor/qti/hardware/bluetooth/btavsprovider/BnBtAVsProvider.h>

namespace aidl {
namespace vendor {
namespace qti {
namespace hardware {
namespace bluetooth {
namespace btavsprovider {

class Task {
  public:
    virtual ~Task() = default;
    virtual void execute() = 0;
};

class TaskType1 : public Task {
  public:
    TaskType1(std::function<void(uint32_t, std::vector<uint8_t>)> func, uint32_t arg1, std::vector<uint8_t> arg2)
      : func_(std::move(func)), arg1_(arg1), arg2_(std::move(arg2)) {}

    void execute() override {
      func_(arg1_, std::move(arg2_));
    }

  private:
    std::function<void(uint32_t, std::vector<uint8_t>)> func_;
    uint32_t arg1_;
    std::vector<uint8_t> arg2_;
};

class TaskType2 : public Task {
  public:
    TaskType2(std::function<void(uint32_t, uint32_t)> func, uint32_t arg1, uint32_t arg2)
      : func_(std::move(func)), arg1_(arg1), arg2_(arg2) {}

    void execute() override {
      func_(arg1_, arg2_);
    }

  private:
    std::function<void(uint32_t, uint32_t)> func_;
    uint32_t arg1_;
    uint32_t arg2_;
};

class TaskQueue {
  public:
    TaskQueue();
    ~TaskQueue();
    void enqueue(std::unique_ptr<Task> task);

  private:
    std::queue<std::unique_ptr<Task>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
    std::thread worker;

    void workerThread();
};

class BtAVsProviderService : public BnBtAVsProvider {
  public:
    std::shared_ptr<::aidl::vendor::qti::hardware::bluetooth::btavsprovider::IBtAVsProviderCallback> btavsProviderCb;

    BtAVsProviderService();
    ~BtAVsProviderService();

    /* BtAVsProvider AIDL HAL calls */
    ::ndk::ScopedAStatus registerBtAVsCallbacks(const std::shared_ptr<::aidl::vendor::qti::hardware::bluetooth::btavsprovider::IBtAVsProviderCallback>& cb) override;
    ::ndk::ScopedAStatus updateVsCmdStatus(int32_t ofc, int32_t status) override;
	::ndk::ScopedAStatus updateVsCmdComplete(int32_t ofc, const std::vector<uint8_t>& params) override;
	::ndk::ScopedAStatus updateVsEvent(int32_t ofc, const std::vector<uint8_t>& data) override;
	::ndk::ScopedAStatus updateSysEvent(int32_t ofc, const std::vector<uint8_t>& payload) override;

	void sendVSCommand(int32_t ofc, const std::vector<uint8_t>& params);
	void btAVsRegisterEvent(const std::vector<uint8_t>& eventcodes);
	void btAVsUnRegisterEvent();

    static void clientDeathRecipient(void* cookie);

  private:
    TaskQueue taskQueue;
};

} // btavsprovider
} // bluetooth
} // hardware
} // qti
} // vendor
} // aidl

#endif
