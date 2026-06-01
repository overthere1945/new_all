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

#include "FmHci.h"

#define LOG_TAG "vendor.qti.hardware.fm-impl"

#include <cutils/properties.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/uio.h>
#include <termios.h>

#include "log/log.h"

// TODO: Remove custom logging defines from PDL packets.
#undef LOG_INFO
#undef LOG_DEBUG

using aidl::vendor::qti::hardware::fm::Status;

namespace aidl::vendor::qti::hardware::fm::impl {
void OnDeath(void* cookie);

class FmDeathRecipient {
public:
    FmDeathRecipient(FmHci* hci) : mHci(hci) {}

    void LinkToDeath(const std::shared_ptr<IFmHciCallbacks>& cb)
    {
        mCb = cb;
        clientDeathRecipient_ = AIBinder_DeathRecipient_new(OnDeath);
        auto linkToDeathReturnStatus = AIBinder_linkToDeath(
                                           mCb->asBinder().get(), clientDeathRecipient_, this /* cookie */);
        LOG_ALWAYS_FATAL_IF(linkToDeathReturnStatus != STATUS_OK,
                            "Unable to link to death recipient");
    }

    void UnlinkToDeath(const std::shared_ptr<IFmHciCallbacks>& cb)
    {
        LOG_ALWAYS_FATAL_IF(cb != mCb, "Unable to unlink mismatched pointers");
    }

    void serviceDied()
    {
        if (mCb != nullptr && !AIBinder_isAlive(mCb->asBinder().get())) {
            ALOGE("Fm remote service has died");
        } else {
            ALOGE("FmDeathRecipient::serviceDied called but service not dead");
            return;
        }
        {
            std::lock_guard<std::mutex> guard(mHasDiedMutex);
            has_died_ = true;
        }
        HciPacketType packet_type = HCI_PACKET_TYPE_FM_CMD;
        const uint8_t data[] = {0x02, 0x4C, 0x00};
        int length = 3;
        DataHandler *data_handler = DataHandler::Get();
        if (data_handler != nullptr) {
            ALOGE("FM died sending fmOff command to Soc");
            data_handler->SendData(TYPE_FM,packet_type,data,length);
        }
        mHci->close();
    }
    FmHci* mHci;
    std::shared_ptr<IFmHciCallbacks> mCb;
    AIBinder_DeathRecipient* clientDeathRecipient_;
    bool getHasDied()
    {
        std::lock_guard<std::mutex> guard(mHasDiedMutex);
        return has_died_;
    }

private:
    std::mutex mHasDiedMutex;
    bool has_died_{false};
};

void OnDeath(void* cookie)
{
    auto* death_recipient = static_cast<FmDeathRecipient*>(cookie);
    death_recipient->serviceDied();
}
//struct fm_hal_t * FmIoctlHal :: fmhal = nullptr;

FmHci::FmHci() {
    mDeathRecipient = std::make_shared<FmDeathRecipient>(this);
}

ndk::ScopedAStatus FmHci::initialize(
    const std::shared_ptr<IFmHciCallbacks>& cb) {
    ALOGI(__func__);

    if (cb == nullptr) {
      ALOGE("cb == nullptr! -> Unable to call initializationComplete(ERR)");
      return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
    }

    mCb = cb;
    mDeathRecipient->LinkToDeath(mCb);

    auto rc = DataHandler::Init( TYPE_FM,
        [this](bool status) {
            auto init_status = mCb->initializationComplete(
                status ? Status::SUCCESS : Status::INITIALIZATION_ERROR);
            if (!init_status.isOk()) {
                ALOGE("Unable to call initializationComplete()");
            }
        },
        [this](HciPacketType type, const hidl_vec<uint8_t> *packet) {
            DataHandler *data_handler = DataHandler::Get();
            if (data_handler && (data_handler->GetClientStatus(TYPE_FM) == false)) {
              return;
            }
            switch (type) {
                case HCI_PACKET_TYPE_FM_EVENT:
                {
                    auto init_status = mCb->hciEventReceived(*packet);
                    if (!init_status.isOk()) {
                        ALOGE(" Unable to call hciEventReceived()");
                        if (data_handler)
                        data_handler->SetClientStatus(false, TYPE_FM);
                    }
                    break;
                }
                default:
                {
                    ALOGE("%s Unexpected event type %d", __func__, type);
                    break;
                }
            }
        });
    if (rc) {
        auto init_status = mCb->initializationComplete(Status::INITIALIZATION_ERROR);
        if (!init_status.isOk()) {
            ALOGE("Unable to call initializationComplete(ERR)");
        }
    }
    return ndk::ScopedAStatus::ok();
}


ndk::ScopedAStatus FmHci::close() {
    ALOGI(__func__);
    DataHandler::CleanUp(TYPE_FM);
    return ndk::ScopedAStatus::ok();
}


ndk::ScopedAStatus FmHci::sendHciCommand(
    const std::vector<uint8_t>& packet)
{
    sendDataToController(HCI_PACKET_TYPE_FM_CMD, packet);
    return ndk::ScopedAStatus::ok();
}

void FmHci::sendDataToController(HciPacketType type,
                                 const std::vector<uint8_t>& data) {
    DataHandler *data_handler = DataHandler::Get();

    if (data_handler != nullptr) {
        data_handler->SendData(TYPE_FM, type, data.data(), data.size());
    } else {
        ALOGI("sendDataToController: data_handler is null");
    }

}
}
