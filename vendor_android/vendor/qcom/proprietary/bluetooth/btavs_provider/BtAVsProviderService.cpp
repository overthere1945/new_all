/*****************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************/

#include <log/log.h>
#include <android/binder_auto_utils.h>
#include <android-base/logging.h>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "BtAVsProviderService.h"
#include "BtAVsProviderIf.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.hardware.bluetooth.btavsprovider"

using aidl::vendor::qti::hardware::bluetooth::btavsprovider::BtAVsProviderService;
using bluetooth::btavsprovider::BtAVsProviderIf;

tBTAVS_MANAGER_API *vs_intf = NULL;

namespace aidl {
namespace vendor {
namespace qti {
namespace hardware {
namespace bluetooth {
namespace btavsprovider {

TaskQueue::TaskQueue() : stop(false), worker(&TaskQueue::workerThread, this) {}

TaskQueue::~TaskQueue() {
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    stop = true;
  }
  condition.notify_all();
  worker.join();
}

void TaskQueue::enqueue(std::unique_ptr<Task> task) {
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    tasks.push(std::move(task));
  }
  condition.notify_one();
}

void TaskQueue::workerThread() {
  while (true) {
    std::unique_ptr<Task> task;
    {
      std::unique_lock<std::mutex> lock(queueMutex);
      condition.wait(lock, [this] { return stop || !tasks.empty(); });
      if (stop && tasks.empty()) return;
      task = std::move(tasks.front());
      tasks.pop();
    }
    task->execute();
  }
}

/* Used to register BtAVs app Callback */
::ndk::ScopedAStatus BtAVsProviderService::registerBtAVsCallbacks(const std::shared_ptr<::aidl::vendor::qti::hardware::bluetooth::btavsprovider::IBtAVsProviderCallback>& in_cb) {
  ALOGD("%s", __func__);
  btavsProviderCb = in_cb;

  AIBinder_DeathRecipient* deathRecipient = AIBinder_DeathRecipient_new(
      BtAVsProviderService::clientDeathRecipient);

  if (deathRecipient != NULL && in_cb != NULL) {
    auto status = AIBinder_linkToDeath(in_cb->asBinder().get(), deathRecipient,
                                       reinterpret_cast<void*>(this));
    if (status != STATUS_OK) {
      ALOGE("%s: Failed to register DeathRecipient with error(%d)", __func__, status);
      // no action needed
    }
  } else {
      ALOGE("%s: Failed ", __func__);
  }

  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BtAVsProviderService::updateVsCmdStatus(int32_t ofc, int32_t status) {
  ALOGD("%s", __func__);
  if (vs_intf && vs_intf->update_vs_cmd_status) {
    taskQueue.enqueue(std::make_unique<TaskType2>(vs_intf->update_vs_cmd_status, ofc, status));
  }
  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BtAVsProviderService::updateVsCmdComplete(int32_t ofc, const std::vector<uint8_t>& params) {
  ALOGD("%s", __func__);
  if (vs_intf && vs_intf->update_vs_cmd_cmpl) {
    taskQueue.enqueue(std::make_unique<TaskType1>(vs_intf->update_vs_cmd_cmpl, ofc, params));
  }
  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BtAVsProviderService::updateVsEvent(int32_t ofc, const std::vector<uint8_t>& data) {
  ALOGD("%s", __func__);
  if (vs_intf && vs_intf->update_vs_event) {
    taskQueue.enqueue(std::make_unique<TaskType1>(vs_intf->update_vs_event, ofc, data));
  }
  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BtAVsProviderService::updateSysEvent(int32_t ofc, const std::vector<uint8_t>& payload) {
  ALOGD("%s", __func__);
  if (vs_intf && vs_intf->update_sys_event) {
    taskQueue.enqueue(std::make_unique<TaskType1>(vs_intf->update_sys_event, ofc, payload));
  }
  return ::ndk::ScopedAStatus::ok();
}

void BtAVsProviderService::sendVSCommand(int32_t ofc, const std::vector<uint8_t>& params) {
  ALOGD("%s", __func__);

  if (btavsProviderCb == NULL) {
    ALOGE("%s: BtAVsProvider Client not bound to the service", __func__);
	return;
  }
  btavsProviderCb->btAVsCommandReceivedCb(ofc, params);
}

void BtAVsProviderService::btAVsRegisterEvent(const std::vector<uint8_t>& eventcodes) {
  ALOGD("%s", __func__);

  if (btavsProviderCb == NULL) {
    ALOGE("%s: BtAVsProvider Client not bound to the service", __func__);
	return;
  }
  btavsProviderCb->btAVsRegisterEventReceivedCb(eventcodes);
}

void BtAVsProviderService::btAVsUnRegisterEvent() {
  ALOGD("%s", __func__);

  if (btavsProviderCb == NULL) {
    ALOGE("%s: BtAVsProvider Client not bound to the service", __func__);
	return;
  }
  btavsProviderCb->btAVsUnRegisterEventReceivedCb();
}

BtAVsProviderService::BtAVsProviderService() {
  BtAVsProviderIf::Initialize(this);
}

BtAVsProviderService::~BtAVsProviderService() {
  btavsProviderCb = NULL;
}

/* BtAVs Profile death recipient */
void BtAVsProviderService::clientDeathRecipient(void* cookie) {
  ALOGD("%s: BtAVs Provider Client (Profile) died..", __func__);

  auto* svc = static_cast<BtAVsProviderService*>(cookie);
  svc->btavsProviderCb = NULL;
}

} // btavsprovider
} // bluetooth
} // hardware
} // qti
} // vendor
} // aidl

namespace bluetooth {
namespace btavsprovider {

class BtAVsProviderIfImpl;
BtAVsProviderIfImpl* instance = NULL;

class BtAVsProviderIfImpl: public BtAVsProviderIf {
  public:
    BtAVsProviderService* service_;

BtAVsProviderIfImpl(BtAVsProviderService* service) {
  service_ = service;
}

~BtAVsProviderIfImpl() {
  instance = NULL;
  service_ = NULL;
}

void RegisterBtAVsManager(tBTAVS_MANAGER_API *vsIf) {
  ALOGD("%s", __func__);
  vs_intf = vsIf;
  ALOGD("%s: vs_intf = %p", __func__, vs_intf);
}

void DeregisterBtAVsManager() {
  ALOGD("%s", __func__);
  vs_intf = NULL;
}

void sendVSCommand(int32_t ofc, const std::vector<uint8_t>& params) {
  ALOGD("%s", __func__);
  if (service_) {
    service_->sendVSCommand(ofc, params);
  }
}

void btAVsRegisterEvent(const std::vector<uint8_t>& eventcodes) {
  ALOGD("%s", __func__);
  if (service_) {
    service_->btAVsRegisterEvent(eventcodes);
  }
}

void btAVsUnRegisterEvent() {
  ALOGD("%s", __func__);
  if (service_) {
    service_->btAVsUnRegisterEvent();
  }
}
};
}
}

BtAVsProviderIf* BtAVsProviderIf::GetIf() {
  return instance;
}

void BtAVsProviderIf::Initialize(BtAVsProviderService* service) {
  ALOGI("%s", __func__);
  instance = new BtAVsProviderIfImpl(service);
}
