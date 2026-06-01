/*
 * Copyright (c) 2022, 2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 */

/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/properties.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/uio.h>
#include <termios.h>

#include "log/log.h"

#include "bluetooth_hci.h"
#include "data_handler.h"
#include "soc_properties.h"
#include "SMCLoader.h"
#include "SMCLoaderCallback.h"
#include "logger.h"
#include "ring_buffer.h"
#include <utils/Log.h>
#include <aidl/vendor/qti/hardware/smcloader/ISMCLoader.h>
#include <aidl/vendor/qti/hardware/smcloader/SMCBootState.h>
#include <aidl/vendor/qti/hardware/smcloader/QCCSubSysType.h>
#include <aidl/vendor/qti/hardware/smcloader/Status.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "android.hardware.bluetooth.aidl_service"

using android::hardware::bluetooth::V1_0::implementation::DataHandler;
using android::hardware::bluetooth::V1_0::implementation::HalStatus;
using android::hardware::bluetooth::V1_0::implementation::Logger;
using android::hardware::bluetooth::V1_0::implementation::
    RX_POST_STACK_ACL_CALL_BACK;
using android::hardware::bluetooth::V1_0::implementation::
    RX_POST_STACK_EVT_CALL_BACK;
using android::hardware::bluetooth::V1_0::implementation::
    RX_POST_STACK_ISO_CALL_BACK;
using android::hardware::bluetooth::V1_0::implementation::
    RX_PRE_STACK_ACL_CALL_BACK;
using android::hardware::bluetooth::V1_0::implementation::
    RX_PRE_STACK_EVT_CALL_BACK;
using android::hardware::bluetooth::V1_0::implementation::
    RX_PRE_STACK_ISO_CALL_BACK;
using android::hardware::bluetooth::V1_0::implementation::
    RX_PRE_STACK_SCO_CALL_BACK;
using android::hardware::bluetooth::V1_0::implementation::
    RX_POST_STACK_SCO_CALL_BACK;
using ::aidl::vendor::qti::hardware::smcloader::ISMCLoader;
using ::aidl::vendor::qti::hardware::smcloader::ISMCLoaderCallback;
using ::aidl::vendor::qti::hardware::smcloader::SMCBootState;
using ::aidl::vendor::qti::hardware::smcloader::QCCSubSysType;
using ::aidl::vendor::qti::hardware::smcloader::Status;

namespace aidl::android::hardware::bluetooth::impl {

void OnDeath(void *cookie);

class BluetoothDeathRecipient {
public:
  BluetoothDeathRecipient(BluetoothHci *hci) : mHci(hci) {}

  void LinkToDeath(const std::shared_ptr<IBluetoothHciCallbacks> &cb) {
    ALOGI("%s", __func__);
    mCb = cb;
    clientDeathRecipient_ = AIBinder_DeathRecipient_new(OnDeath);
    auto linkToDeathReturnStatus = AIBinder_linkToDeath(
        mCb->asBinder().get(), clientDeathRecipient_, this /* cookie */);

    if (linkToDeathReturnStatus != STATUS_OK) {
      ALOGE("%s: Unable to link to death recipient(%d)", __func__,
            linkToDeathReturnStatus);
    }
  }

  void UnlinkToDeath(const std::shared_ptr<IBluetoothHciCallbacks> &cb) {
    ALOGI("%s", __func__);
    if (cb != mCb) {
      ALOGE("Unable to unlink mismatched pointers");
    } else if (mCb != nullptr && clientDeathRecipient_ ) {
      ALOGI("%s unlink death recipient", __func__);
      AIBinder_unlinkToDeath(mCb->asBinder().get(), clientDeathRecipient_, this);
      AIBinder_DeathRecipient_delete(clientDeathRecipient_);
      mCb = nullptr;
      clientDeathRecipient_ = nullptr;
    }
  }

  void serviceDied() {
    ALOGE("%s", __func__);
    if (mCb != nullptr && !AIBinder_isAlive(mCb->asBinder().get())) {
      ALOGE("Bluetooth remote service has died");
    } else {
      ALOGE("BluetoothDeathRecipient::serviceDied called but service not dead");
      return;
    }

    DataHandler *data_handler = DataHandler::Get();
    if (data_handler != nullptr)
      data_handler->StackTimeoutTriggered();

    has_died_ = true;
    mHci->close();
  }
  BluetoothHci *mHci;
  std::shared_ptr<IBluetoothHciCallbacks> mCb;
  AIBinder_DeathRecipient *clientDeathRecipient_;
  bool getHasDied() const { return has_died_; }

private:
  bool has_died_{false};
};

void OnDeath(void *cookie) {
  ALOGE("%s", __func__);
  auto *death_recipient = static_cast<BluetoothDeathRecipient *>(cookie);
  death_recipient->serviceDied();
}

BluetoothHci::BluetoothHci() {
  ALOGI("%s", __func__);
  mDeathRecipient = std::make_shared<BluetoothDeathRecipient>(this);
}

ndk::ScopedAStatus BluetoothHci::initialize_aidl(
    const std::shared_ptr<IBluetoothHciCallbacks> &cb) {
  ALOGI("BluetoothHci::initialize_aidl()");
  int ret = 0;
  bool is_themisto = false;
  bool cont_init = false;
  char soc_name[PROPERTY_VALUE_MAX] = {'\0'};
  setVendorPropertiesDefault();
  ret = property_get("persist.vendor.qcom.bluetooth.soc", soc_name, "pronto");

  // TODO: Change to Themisto on real hardware
  if (!strncasecmp(soc_name, "themisto", sizeof("themisto"))) {
      ALOGI("%s:: Bluetooth soc type set to: themisto, ret: %d",__func__, ret);
      is_themisto = true;
  }else{
      ALOGI("%s:: Bluetooth soc type set to: %s, ret: %d",__func__, soc_name, ret);
      cont_init = true;
  }
  if (cb == nullptr) {
    ALOGE("%s: Received NULL callback from BT client", __func__);
    return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }
  std::shared_ptr<IBluetoothHciCallbacks> event_cb_tmp;
  event_cb_tmp = cb;

  auto rc = DataHandler::Init(
      TYPE_BT,
      [this, event_cb_tmp](bool status)
      {
        if (event_cb_tmp != nullptr)
        {
          ALOGI("%s: Set callbacks received from BT client inorder "
                "to provide status and data through them",
                __func__);
          event_cb_ = event_cb_tmp;
          ALOGE("SERVER - Value of AIDL callback is: %p", event_cb_tmp.get());
        }
        if (event_cb_ != nullptr)
        {
          auto hidl_client_status = event_cb_tmp->initializationComplete(
              status ? Status::SUCCESS : Status::HARDWARE_INITIALIZATION_ERROR);
          if (!hidl_client_status.isOk())
          {
            ALOGE("Client dead, callback initializationComplete failed");
          }
        }
      },
      [this,
       event_cb_tmp](HciPacketType type,
                     const ::android::hardware::hidl_vec<uint8_t> *packet)
      {
        DataHandler *data_handler = DataHandler::Get();
        if (event_cb_tmp == nullptr)
        {
          ALOGE("BluetoothHci: event_cb_tmp is null");
          if (data_handler)
            data_handler->SetClientStatus(false, TYPE_BT);
          return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
        }
        /* Skip calling client callback when client is dead */
        if (data_handler && (data_handler->GetClientStatus(TYPE_BT) == false))
        {
          ALOGV("%s: Skip calling client callback when client is dead",
                __func__);
          return ndk::ScopedAStatus::fromServiceSpecificError(
              STATUS_FAILED_TRANSACTION);
        }
        Logger::Get()->UpdateRxTimeStamp();
        switch (type)
        {
        case HCI_PACKET_TYPE_EVENT:
        {
#ifdef DUMP_RINGBUF_LOG
          Logger::Get()->UpdateRxEventTag(RX_PRE_STACK_EVT_CALL_BACK);
#endif
          auto hidl_client_status = event_cb_tmp->hciEventReceived(*packet);
          if (!hidl_client_status.isOk())
          {
            ALOGE("Client dead, callback hciEventReceived failed");
            if (data_handler)
              data_handler->SetClientStatus(false, TYPE_BT);
          }
#ifdef DUMP_RINGBUF_LOG
          Logger::Get()->UpdateRxEventTag(RX_POST_STACK_EVT_CALL_BACK);
#endif
        }
        break;
        case HCI_PACKET_TYPE_ACL_DATA:
        {
#ifdef DUMP_RINGBUF_LOG
          Logger::Get()->UpdateRxEventTag(RX_PRE_STACK_ACL_CALL_BACK);
#endif
          auto hidl_client_status = event_cb_tmp->aclDataReceived(*packet);
          if (!hidl_client_status.isOk())
          {
            ALOGE("Client dead, callback aclDataReceived failed");
            if (data_handler)
              data_handler->SetClientStatus(false, TYPE_BT);
          }
#ifdef DUMP_RINGBUF_LOG
          Logger::Get()->UpdateRxEventTag(RX_POST_STACK_ACL_CALL_BACK);
#endif
        }
        break;
        case HCI_PACKET_TYPE_ISO_DATA:
        {
#ifdef DUMP_RINGBUF_LOG
          Logger::Get()->UpdateRxEventTag(RX_PRE_STACK_ISO_CALL_BACK);
#endif
          auto hidl_client_status = event_cb_tmp->isoDataReceived(*packet);
          if (!hidl_client_status.isOk())
          {
            ALOGE("Client dead, callback aclDataReceived failed");
            if (data_handler)
              data_handler->SetClientStatus(false, TYPE_BT);
          }
#ifdef DUMP_RINGBUF_LOG
          Logger::Get()->UpdateRxEventTag(RX_POST_STACK_ISO_CALL_BACK);
#endif
        }
        break;
        case HCI_PACKET_TYPE_SCO_DATA:
        {
#ifdef DUMP_RINGBUF_LOG
          Logger::Get()->UpdateRxEventTag(RX_PRE_STACK_SCO_CALL_BACK);
#endif
          auto hidl_client_status = event_cb_tmp->scoDataReceived(*packet);
          if (!hidl_client_status.isOk())
          {
            ALOGE("Client dead, callback scoDataReceived failed");
            if (data_handler)
              data_handler->SetClientStatus(false, TYPE_BT);
          }
#ifdef DUMP_RINGBUF_LOG
          Logger::Get()->UpdateRxEventTag(RX_POST_STACK_SCO_CALL_BACK);
#endif
        }
        break;
        default:
          ALOGE("%s Unexpected event type %d", __func__, type);
          return ndk::ScopedAStatus::fromServiceSpecificError(
              STATUS_FAILED_TRANSACTION);
        }
        return ndk::ScopedAStatus::ok();
      });

  if (rc)
  {
    auto hidl_status = cb->initializationComplete((rc == HalStatus::HAL_ALREADY_INITIALIZED) ? Status::ALREADY_INITIALIZED : Status::HARDWARE_INITIALIZATION_ERROR);
    if (!hidl_status.isOk())
    {
      ALOGE("Client dead, callback initializationComplete failed");
    }
    ALOGE("BluetoothHci: error INITIALIZATION_ERROR");
  }
  else if (!rc && (cb != nullptr))
  {
    ALOGI("%s: linking to deathRecipient", __func__);
    mDeathRecipient->LinkToDeath(cb);
  }
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus
BluetoothHci::initialize(const std::shared_ptr<IBluetoothHciCallbacks> &cb) {
  ALOGI("BluetoothHci: Using initialize_aidl.");
  return initialize_aidl(cb);
}

ndk::ScopedAStatus BluetoothHci::close() {
  ALOGI("BluetoothHci::close()");
  if (event_cb_ != nullptr)
    mDeathRecipient->UnlinkToDeath(event_cb_);
  DataHandler::CleanUp(TYPE_BT);
  ALOGW("BluetoothHci::close, finish cleanup");
  DataHandler::setFtmStatus(false);

  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus
BluetoothHci::sendHciCommand(const std::vector<uint8_t> &packet) {
  sendDataToController(HCI_PACKET_TYPE_COMMAND, packet);
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus
BluetoothHci::sendAclData(const std::vector<uint8_t> &packet) {
  sendDataToController(HCI_PACKET_TYPE_ACL_DATA, packet);
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus
BluetoothHci::sendScoData(const std::vector<uint8_t> &packet) {
  sendDataToController(HCI_PACKET_TYPE_SCO_DATA, packet);
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus
BluetoothHci::sendIsoData(const std::vector<uint8_t> &packet) {
  sendDataToController(HCI_PACKET_TYPE_ISO_DATA, packet);
  return ndk::ScopedAStatus::ok();
}

void BluetoothHci::sendDataToController(HciPacketType type,
                                        const std::vector<uint8_t> &v) {
  DataHandler *data_handler = DataHandler::Get();

  if (data_handler != nullptr) {
    data_handler->SendData(TYPE_BT, type, v.data(), v.size());
  }
  else{
       //For Debugging
       for(const auto& byte : v)
         ALOGD("[%s] data: %x and size: %d", __func__ , byte, v.size());
       if(v[2] == 0x05 && v.size() == 3){
         DataHandler::setFtmStatus(true);
         return;
     }
  }
}

} // namespace aidl::android::hardware::bluetooth::impl
