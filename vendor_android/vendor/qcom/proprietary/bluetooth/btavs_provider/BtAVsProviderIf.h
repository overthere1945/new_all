/*****************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************/

#ifndef BTAVS_PROVIDER_IF_H
#define BTAVS_PROVIDER_IF_H

#include "BtAVsProviderService.h"

typedef void (VS_UpdateVSCommandStatus) (int32_t ofc, int32_t status);
typedef void (VS_UpdateVSCommandComplete) (int32_t ofc, const std::vector<uint8_t>& params);
typedef void (VS_UpdateVSEvent) (int32_t ofc, const std::vector<uint8_t>& data);
typedef void (VS_UpdateSysEvent) (int32_t ofc, const std::vector<uint8_t>& payload);

/* BtAVs Manager interface API's */
typedef struct {
  VS_UpdateVSCommandStatus* update_vs_cmd_status;
  VS_UpdateVSCommandComplete* update_vs_cmd_cmpl;
  VS_UpdateVSEvent* update_vs_event;
  VS_UpdateSysEvent* update_sys_event;
} tBTAVS_MANAGER_API;

namespace bluetooth {
namespace btavsprovider {

class BtAVsProviderIf {
  public:
    virtual ~BtAVsProviderIf() = default;
	static void Initialize(aidl::vendor::qti::hardware::bluetooth::btavsprovider::BtAVsProviderService* service);
	static BtAVsProviderIf* GetIf();
	virtual void RegisterBtAVsManager(tBTAVS_MANAGER_API *vsIf) = 0;
	virtual void DeregisterBtAVsManager() = 0;
	virtual void sendVSCommand(int32_t ofc, const std::vector<uint8_t>& params) = 0;
	virtual void btAVsRegisterEvent(const std::vector<uint8_t>& eventcodes) = 0;
	virtual void btAVsUnRegisterEvent() = 0;
};

}
}

#endif
