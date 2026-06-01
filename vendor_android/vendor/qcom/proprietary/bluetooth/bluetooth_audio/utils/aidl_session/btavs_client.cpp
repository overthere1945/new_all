/*****************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************/

#include "btavs_client.h"
#include "device_generated.h"  // Generated from FlatBuffers schema
#include "flatbuffers/flatbuffers.h"

BtAVsClient* BtAVsClient::instance = nullptr;
std::mutex BtAVsClient::mtx;

void CacheManager::putDevices(const std::vector<le_audio_device>& devices) {
    flatbuffers::FlatBufferBuilder builder;

    // Convert C++ vector to FlatBuffer objects
    std::vector<flatbuffers::Offset<cache::Device>> fb_devices;
    for (const auto& dev : devices) {
        auto qll_vec = builder.CreateVector(dev.qll_features.as_array, BD_FEATURES_LEN);
        fb_devices.push_back(cache::CreateDevice(builder, dev.mode, dev.delay,
                                                 dev.bdAddr, dev.isActive, dev.isLexSupported,
                                                 dev.isLexConfigured, dev.isLexTransportReady,
                                                 qll_vec));
    }

    auto device_list = cache::CreateDeviceList(builder, builder.CreateVector(fb_devices));
    builder.Finish(device_list);

    std::string tempFilePath = filePath + ".tmp";

    // Strategy: Write to a temporary file first, then rename for atomicity.
    // This prevents a partially written or corrupted file from overwriting the good one.
    std::ofstream tempFile(tempFilePath, std::ios::binary | std::ios::trunc);
    if (!tempFile.is_open()) {
        ALOGE("Failed to open temporary file for writing: %s", tempFilePath.c_str());
        return;
    }

    tempFile.write(reinterpret_cast<const char*>(builder.GetBufferPointer()), builder.GetSize());

    if (tempFile.fail()) {
        ALOGE("Failed to write data to temporary file: %s", tempFilePath.c_str());
        tempFile.close();
        std::remove(tempFilePath.c_str());
        return;
    }

    tempFile.close();

    if (std::rename(tempFilePath.c_str(), filePath.c_str()) != 0) {
        ALOGE("Failed to rename temporary file '%s' to '%s'. Error: %s",
              tempFilePath.c_str(), filePath.c_str(), strerror(errno));
    } else {
        ALOGD("Successfully wrote and updated device cache to: %s", filePath.c_str());
    }
}

void CacheManager::putLocalQllFeatures(const bt_device_qll_local_supported_features_t& local_qll_features_) {
    flatbuffers::FlatBufferBuilder builder;

    auto local_vec = builder.CreateVector(local_qll_features_.as_array, QLL_LOCAL_SUPPORTED_FEATURES_MAX_SIZE);
    auto device_list = cache::CreateDeviceList(builder, 0, local_vec);
    builder.Finish(device_list);

    // Strategy: Write to a temporary file first, then rename for atomicity.
    // This prevents a partially written or corrupted file from overwriting the good one.
    std::string tempFilePath = localQllFeaturesPath + ".tmp";

    std::ofstream tempFile(tempFilePath, std::ios::binary | std::ios::trunc);
    if (!tempFile.is_open()) {
        ALOGE("Failed to open temporary file for writing: %s", tempFilePath.c_str());
        return;
    }

    tempFile.write(reinterpret_cast<const char*>(builder.GetBufferPointer()), builder.GetSize());

    if (tempFile.fail()) {
        ALOGE("Failed to write data to temporary file: %s", tempFilePath.c_str());
        tempFile.close();
        std::remove(tempFilePath.c_str());
        return;
    }

    tempFile.close();

    if (std::rename(tempFilePath.c_str(), localQllFeaturesPath.c_str()) != 0) {
        ALOGE("Failed to rename temporary file '%s' to '%s'. Error: %s",
              tempFilePath.c_str(), localQllFeaturesPath.c_str(), strerror(errno));
    } else {
        ALOGD("Successfully wrote and updated local QLL features cache to: %s", localQllFeaturesPath.c_str());
    }
}

void CacheManager::putSoCAddOnFeatures(const bt_device_soc_add_on_features_t& soc_add_on_features_) {
    flatbuffers::FlatBufferBuilder builder;

    auto local_vec = builder.CreateVector(soc_add_on_features_.as_array, SOC_ADD_ON_FEATURES_MAX_SIZE);
    auto device_list = cache::CreateDeviceList(builder, 0, local_vec);
    builder.Finish(device_list);

    // Strategy: Write to a temporary file first, then rename for atomicity.
    // This prevents a partially written or corrupted file from overwriting the good one.
    std::string tempFilePath = addOnFeaturesPath + ".tmp";

    std::ofstream tempFile(tempFilePath, std::ios::binary | std::ios::trunc);
    if (!tempFile.is_open()) {
        ALOGE("Failed to open temporary file for writing: %s", tempFilePath.c_str());
        return;
    }

    tempFile.write(reinterpret_cast<const char*>(builder.GetBufferPointer()), builder.GetSize());

    if (tempFile.fail()) {
        ALOGE("Failed to write data to temporary file: %s", tempFilePath.c_str());
        tempFile.close();
        std::remove(tempFilePath.c_str());
        return;
    }

    tempFile.close();

    if (std::rename(tempFilePath.c_str(), addOnFeaturesPath.c_str()) != 0) {
        ALOGE("Failed to rename temporary file '%s' to '%s'. Error: %s",
              tempFilePath.c_str(), addOnFeaturesPath.c_str(), strerror(errno));
    } else {
        ALOGD("Successfully wrote and updated SoC Add-On Features cache to: %s", addOnFeaturesPath.c_str());
    }
}

std::vector<uint8_t> CacheManager::getLocalQllFeatures() {
    std::ifstream file(localQllFeaturesPath, std::ios::binary);
    std::vector<uint8_t> features;
    if (file.is_open()) {
        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), {});
        if (buffer.empty()) {
            ALOGW("Cache file '%s' is empty. Returning empty features.", localQllFeaturesPath.c_str());
            return features;
        }
        flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
        if (!cache::VerifyDeviceListBuffer(verifier)) {
            ALOGE("Failed to verify FlatBuffer data from '%s'. Data might be corrupt or schema outdated. Returning empty features.", localQllFeaturesPath.c_str());
            return features;
        }
        auto device_list = flatbuffers::GetRoot<cache::DeviceList>(buffer.data());
        auto vec = device_list->localQllFeatures();
        if (vec) {
            features.assign(vec->begin(), vec->end());
        } else {
            ALOGW("FlatBuffer data from '%s' is valid, but 'localQllFeatures' field is missing or null. Returning empty features.", localQllFeaturesPath.c_str());
        }
    } else {
        ALOGE("Failed to open file for reading: %s", localQllFeaturesPath.c_str());
    }
    return features;
}

std::vector<uint8_t> CacheManager::getAddOnFeatures() {
    std::ifstream file(addOnFeaturesPath, std::ios::binary);
    std::vector<uint8_t> support;
    if (file.is_open()) {
        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), {});
        if (buffer.empty()) {
            ALOGW("Cache file '%s' is empty. Returning empty features.", addOnFeaturesPath.c_str());
            return support;
        }
        flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
        if (!cache::VerifyDeviceListBuffer(verifier)) {
            ALOGE("Failed to verify FlatBuffer data from '%s'. Data might be corrupt or schema outdated. Returning empty features.", addOnFeaturesPath.c_str());
            return support;
        }
        auto device_list = flatbuffers::GetRoot<cache::DeviceList>(buffer.data());
        auto vec = device_list->socAddOnFeatures();
        if (vec) {
            support.assign(vec->begin(), vec->end());
        } else {
            ALOGW("FlatBuffer data from '%s' is valid, but 'socAddOnFeatures' field is missing or null. Returning empty features.", addOnFeaturesPath.c_str());
        }
    } else {
        ALOGE("Failed to open file for reading: %s", addOnFeaturesPath.c_str());
    }
    return support;
}

std::vector<le_audio_device> CacheManager::getDevices() {
    std::ifstream file(filePath, std::ios::binary);
    std::vector<le_audio_device> devices;
    if (file.is_open()) {
        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), {});
        if (buffer.empty()) {
            ALOGW("Cache file '%s' is empty. Returning empty device list.", filePath.c_str());
            return devices;
        }
        flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
        if (!cache::VerifyDeviceListBuffer(verifier)) {
            ALOGE("Failed to verify FlatBuffer data from '%s'. Data might be corrupt or schema outdated. Returning empty device list.", filePath.c_str());
            return devices;
        }

        // Deserialize FlatBuffers data
        auto device_list = flatbuffers::GetRoot<cache::DeviceList>(buffer.data());
        auto fb_devices = device_list->devices();
        if (fb_devices) {
            for (size_t i = 0; i < fb_devices->size(); i++) {
                const auto* dev = fb_devices->Get(i);
                if (!dev) {
                    ALOGW("Skipping null device entry at index %zu in '%s'.", i, filePath.c_str());
                    continue;
                }
                le_audio_device device_t(dev->mode(), dev->delay(), dev->Addr(),
                                   dev->isActive(), dev->isSupported(),
                                   dev->isConfigured(), dev->isReady());
                auto remote_qll_features = dev->remoteQllFeatures();
                if (remote_qll_features && remote_qll_features->size() == BD_FEATURES_LEN) {
                    std::copy(remote_qll_features->begin(), remote_qll_features->end(),
                        device_t.qll_features.as_array);
                } else if (remote_qll_features && remote_qll_features->size() != BD_FEATURES_LEN) {
                    ALOGW("Remote QLL Features size mismatch for device at index %zu. Expected %d, got %zu.",
                          i, BD_FEATURES_LEN, remote_qll_features->size());
                } else {
                    ALOGW("Remote QLL Features missing for device at index %zu.", i);
                }
                devices.push_back(device_t);
            }
        } else {
            ALOGW("FlatBuffer data from '%s' is valid, but 'devices' field is null. Returning empty device list.", filePath.c_str());
        }
    } else {
        ALOGE("Failed to open file for reading: %s", filePath.c_str());
    }
    return devices;
}

void CacheManager::clear() {
    std::ofstream file(filePath, std::ofstream::out | std::ofstream::trunc);
    file.close();
}

// Default to ADAPTER_STATE_ON as the adapter is expected to be initialized in the ON state
// by the Bluetooth stack before BtAVsClient is instantiated as initialization happpens during
// openProvider which happens at startAudioSession or restartAudioSession. This adapter state
// is expected to change when BT is turned Off.
BtAVsClient::BtAVsClient() : audio_setting_cb(nullptr), adapter_state(ADAPTER_STATE_ON) {
  InitializeInterface();
}

BtAVsClient& BtAVsClient::GetInstance() {
    std::lock_guard<std::mutex> lock(mtx);
    if (instance == nullptr) {
        instance = new BtAVsClient();
    }
    return *instance;
}

void BtAVsClient::Init() {
  auto provider = BtAVsProviderIf::GetIf();
  if (provider) {
    provider->RegisterBtAVsManager(&intf);
    leADevices = cache_.getDevices();
    auto vec_qll = cache_.getLocalQllFeatures();
    if (vec_qll.size() <= QLL_LOCAL_SUPPORTED_FEATURES_MAX_SIZE)
      std::copy(vec_qll.begin(), vec_qll.end(), local_qll_features.as_array);
    auto vec_addon = cache_.getAddOnFeatures();
    if (vec_addon.size() <= SOC_ADD_ON_FEATURES_MAX_SIZE)
      std::copy(vec_addon.begin(), vec_addon.end(), soc_add_on_features.as_array);
  } else {
    ALOGE("%s: No BtAVsProvider service instance!", __func__);
  }
}

void BtAVsClient::InitializeInterface() {
  intf.update_vs_cmd_status = [](auto ofc, auto status){
    BtAVsClient::GetInstance().UpdateVSCommandStatus(ofc, status);
  };
  intf.update_vs_cmd_cmpl = [](auto ofc, auto params){
    BtAVsClient::GetInstance().UpdateVSCommandComplete(ofc, params);
  };
  intf.update_vs_event = [](auto ofc, auto data){
    BtAVsClient::GetInstance().UpdateVSEvent(ofc, data);
  };
  intf.update_sys_event = [](auto ofc, auto payload){
    BtAVsClient::GetInstance().UpdateSysEvent(ofc, payload);
  };
}

void BtAVsClient::UpdateVAContext(const tSESSION_TYPE session_type, void* data) {
  if (session_type == LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    uint16_t metadata = 0;
    for (auto i = 0; i < (*((const struct source_metadata*) data)).track_count; i++) {
      auto& t = (*((const struct source_metadata*) data)).tracks[i];
      UpdateVAAudioContent(t.content_type, t.usage);
    }
  } else if (session_type == LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    uint16_t metadata = 0;
    for (auto i = 0; i < (*((const struct sink_metadata*) data)).track_count; i++) {
      auto& t = (*((const struct sink_metadata*) data)).tracks[i];
      UpdateVAAudioSource(t.source);
    }
  }
}

LeAudioContextType BtAVsClient::GetContextType(const tSESSION_TYPE session_type, void* data) {
  if (session_type == LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
      session_type == A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    uint16_t metadata = 0;
    for (auto i = 0; i < (*((const struct source_metadata*) data)).track_count; i++) {
      auto t = (*((const struct source_metadata*) data)).tracks[i];
      metadata |= static_cast<uint16_t>(AudioContentToLeAudioContext(t.content_type, t.usage));
    }
    ALOGD("%s: metadata %d ", __func__, metadata);
    for (auto ct : context_priority_list) {
      if (static_cast<uint16_t>(ct) & metadata) {
        ALOGD("%s: context type %d ", __func__, ct);
        return ct;
      }
    }
  } else if (session_type == LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    uint16_t metadata = 0;
    for (auto i = 0; i < (*((const struct sink_metadata*) data)).track_count; i++) {
      auto t = (*((const struct sink_metadata*) data)).tracks[i];
      metadata |= static_cast<uint16_t>(AudioSourceToLeAudioContext(t.source));
    }
    for (auto ct : context_priority_list) {
      if ((static_cast<uint16_t>(ct) & metadata) != 0) {
        return ct;
      }
    }
  }
  return LeAudioContextType::UNSPECIFIED;
}

void BtAVsClient::UpdateContextType(cig_context_metadata& data) {
  auto adv_prop = std::string("true");//android::base::GetProperty("persist.vendor.service.bt.adv_transport", "false");
  if (adv_prop == "true") {
    std::vector<uint8_t> param;
    param.push_back(QBCE_SET_CIG_CONTEXT_TYPE);
    param.push_back(0x00);
    param.push_back(data.context_type & 0x00FF);
    param.push_back((data.context_type & 0xFF00) >> 8);
    param.push_back(data.cis_count);
    for (auto& cis_handle: data.cis_handles) {
      param.push_back(cis_handle & 0x00FF);
      param.push_back((cis_handle & 0xFF00) >> 8);
    }
    data.cis_count = 0;
    data.cis_handles.clear();
    BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF,
          std::vector<uint8_t>(param.data(), param.data() + param.size()));
  }
}

void BtAVsClient::UpdateCigMetadata(const uint8_t metadata_type, const uint8_t stream_ind,
       const std::vector<uint8_t> bd_addr) {
  auto adv_prop = std::string("true");//android::base::GetProperty("persist.vendor.service.bt.adv_transport", "false");
  if (adv_prop == "true") {
    std::vector<uint8_t> param;
    bool update = false;
    param.push_back(QBCE_VS_SET_CIG_METADATA);
    if (metadata_type == LTV_TYPE_STREAM_INDICATION) {
      param.push_back(LTV_TYPE_STREAM_INDICATION);
      param.push_back(LTV_LEN_STREAM_INDICATION);
      param.push_back(stream_ind);
      update = true;
    } else if (metadata_type == LTV_TYPE_BAP_TIMEOUT_INDICATION) {
      param.push_back(LTV_TYPE_BAP_TIMEOUT_INDICATION);
      param.push_back(LTV_LEN_BAP_TIMEOUT_INDICATION);
      param.insert(param.end(), bd_addr.begin(), bd_addr.end());
      update = true;
    }
    if (update) {
      BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF,
         std::vector<uint8_t>(param.data(), param.data() + param.size()));
    }
  }
}

void BtAVsClient::RegisterAudioSetting(std::function<void()> cb) {
  audio_setting_cb = cb;
}

void BtAVsClient::UpdateA2dpCodecModeOrData(const std::vector<uint8_t>& payload) {
  std::vector<uint8_t> param;
  param.push_back(A2DP_PASSTHRU_CODEC_INFO);
  param.insert(param.end(), payload.begin(), payload.end());
  BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QCONTROLLER_A2DP,
          std::vector<uint8_t>(param.data(), param.data() + param.size()));
}

void BtAVsClient::UpdateCodecSettings (const uint32_t& cig_id, const uint32_t& cis_id,
      const std::vector<uint8_t>& payload) {
  std::map<uint8_t, std::vector<uint8_t>> ltv_map;
  if (!Parse(payload.data(), payload.size(), ltv_map)) {
    return;
  }
  auto ltv = Find(QBCE_VS_EXTENDED_METADATA, ltv_map);
  if (ltv) {
    ALOGI("%s: size of codec settings stream %d", __func__, ltv.value().size());
    uint8_t values[19] = {0};
    uint8_t* p = &values[0];
    UINT8_TO_STREAM(p, QBCE_SET_ENCODER_LIMITS); //sub-opcode
    UINT8_TO_STREAM(p, (uint8_t)cig_id);
    UINT8_TO_STREAM(p, (uint8_t)cis_id);
    UINT8_TO_STREAM(p, ltv.value().size()); //numlimits
    UINT8_TO_STREAM(p, LTV_TYPE_MIN_FT);
    UINT8_TO_STREAM(p, LTV_LEN_MIN_FT);
    UINT8_TO_STREAM(p, ltv.value()[0]);
    UINT8_TO_STREAM(p, LTV_TYPE_MIN_BIT_RATE);
    UINT8_TO_STREAM(p, LTV_LEN_MIN_BIT_RATE);
    UINT8_TO_STREAM(p, ltv.value()[2]);
    UINT8_TO_STREAM(p, LTV_TYPE_MIN_MAX_ERROR_RESILIENCE);
    UINT8_TO_STREAM(p, LTV_LEN_MIN_MAX_ERROR_RESILIENCE);
    UINT8_TO_STREAM(p, ltv.value()[3]);
    UINT8_TO_STREAM(p, LTV_TYPE_LATENCY_MODE);
    UINT8_TO_STREAM(p, LTV_LEN_LATENCY_MODE);
    UINT8_TO_STREAM(p, ltv.value()[4]);
    UINT8_TO_STREAM(p, LTV_TYPE_MAX_FT);
    UINT8_TO_STREAM(p, LTV_LEN_MAX_FT);
    UINT8_TO_STREAM(p, ltv.value()[1]);
    BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF, std::vector<uint8_t>(values, values+sizeof(values)));
  }
}

std::optional<std::vector<uint8_t>> BtAVsClient::Find(uint8_t type, std::map<uint8_t, std::vector<uint8_t>>& values) {
  auto iter = std::find_if(values.cbegin(), values.cend(),
                           [type](const auto& value) { return value.first == type; });
  if (iter == values.cend()) {
    return std::nullopt;
  }
  return iter->second;
}

bool BtAVsClient::Parse(const uint8_t* p_value, uint8_t len, std::map<uint8_t, std::vector<uint8_t>>& values) {
  if (len > 0) {
    const auto p_value_end = p_value + len;
    while ((p_value_end - p_value) > 0) {
      uint8_t ltv_len;
      STREAM_TO_UINT8(ltv_len, p_value);
      if (ltv_len == 0) {
        continue;
      }
      if (p_value_end < (p_value + ltv_len)) {
            LOG(ERROR) << __func__ << "Invalid ltv_len: " << static_cast<int>(ltv_len);
        return false;
      }
      uint8_t ltv_type;
      STREAM_TO_UINT8(ltv_type, p_value);
      ltv_len -= sizeof(ltv_type);
      const auto p_temp = p_value;
      p_value += ltv_len;
      std::vector<uint8_t> ltv_value(p_temp, p_value);
      values.emplace(ltv_type, std::move(ltv_value));
    }
  }
  return true;
}

void BtAVsClient::UpdateSysEvent (int32_t evt, const std::vector<uint8_t>& p) {
  ALOGI("%s: enter %d %d\n", __func__, evt, p.size());
  switch(evt) {
    case ADAPTER_STATE_CHANGED: {
      uint32_t state = 0;
      JSTREAM_TO_UINT32(state, p.data());
      BtAVsClient::GetInstance().adapter_state = static_cast<uint8_t>(state);
      if (state == ADAPTER_STATE_ON) {
        if (BtAVsProviderIf::GetIf()) {
          std::vector<uint8_t> eventCodes;
          eventCodes.push_back(0xFF & HCI_VS_QBCE_OCF);
          BtAVsProviderIf::GetIf()->btAVsRegisterEvent(eventCodes);
          ALOGI("%s: Adapter turned On, reading vendor add-on features", __func__);
          BtAVsClient::GetInstance().ReadVendorAddOnFeatures();
        } else {
          ALOGE("%s: No BtAVsProvider service instance!", __func__);
        }
      } else if (state == ADAPTER_STATE_TURNING_OFF) {
        if (BtAVsProviderIf::GetIf()) {
          BtAVsProviderIf::GetIf()->btAVsUnRegisterEvent();
        } else {
          ALOGE("%s: No BtAVsProvider service instance!", __func__);
        }
        if (BtAVsClient::GetInstance().audio_setting_cb) {
          BtAVsClient::GetInstance().audio_setting_cb();
        }
        ALOGI("%s: Adapter turning Off", __func__);
        std::fill(BtAVsClient::GetInstance().soc_add_on_features.as_array,
          BtAVsClient::GetInstance().soc_add_on_features.as_array + SOC_ADD_ON_FEATURES_MAX_SIZE, 0);
        std::fill(BtAVsClient::GetInstance().local_qll_features.as_array,
          BtAVsClient::GetInstance().local_qll_features.as_array + QLL_LOCAL_SUPPORTED_FEATURES_MAX_SIZE, 0);
        BtAVsClient::GetInstance().cache_.putLocalQllFeatures(BtAVsClient::GetInstance().local_qll_features);
        BtAVsClient::GetInstance().cache_.putSoCAddOnFeatures(BtAVsClient::GetInstance().soc_add_on_features);
      }
      break;
    }
    case AIRPLANE_MODE_STATE_CHANGED:
      break;
    case LE_AUDIO_ACTIVE_DEVICE_CHANGED: {
      uint8_t lexSupport = 0;
      uint64_t bdAddr = 0;
      if (p.size() > 0) {
        JSTREAM_TO_UINT8(lexSupport, p.data());
        JSTREAM_TO_UINT48(bdAddr, p.data()+1);
      }
      auto isLexSupported = (lexSupport == 0) ? false : true;
      ALOGI("%s: bdAddr: %ld", __func__, bdAddr);
      auto it = std::find_if(BtAVsClient::GetInstance().leADevices.begin(), BtAVsClient::GetInstance().leADevices.end(), [](le_audio_device d) { return d.isActive; });
      if (it != BtAVsClient::GetInstance().leADevices.end()) {
        it->isActive = false;
      }
      if (bdAddr) {
        auto it = std::find_if(BtAVsClient::GetInstance().leADevices.begin(), BtAVsClient::GetInstance().leADevices.end(), [bdAddr](le_audio_device d) { return d.bdAddr == bdAddr; });
        if (it != BtAVsClient::GetInstance().leADevices.end()) {
          it->isActive = true;
          it->isLexSupported = isLexSupported;
        } else {
          ALOGI("%s: Adding device 0x%x", __func__, bdAddr);
          BtAVsClient::GetInstance().leADevices.push_back({0, 0, bdAddr, true, isLexSupported, false, false, 0});
        }
      }
      BtAVsClient::GetInstance().cache_.putDevices(BtAVsClient::GetInstance().leADevices);
      break;
    }
    case ACL_DISCONNECTED: {
      uint64_t bdAddr = 0;
      if (p.size() > 0) {
        JSTREAM_TO_UINT48(bdAddr, p.data());
        //cleanup for this device
        auto it = std::find_if(BtAVsClient::GetInstance().leADevices.begin(), BtAVsClient::GetInstance().leADevices.end(), [bdAddr](le_audio_device d) { return d.bdAddr == bdAddr; });
        if (it != BtAVsClient::GetInstance().leADevices.end()) {
          ALOGI("%s:: found LEA device to remove from LeADevices list.", __func__);
          BtAVsClient::GetInstance().leADevices.erase(std::remove(BtAVsClient::GetInstance().leADevices.begin(), BtAVsClient::GetInstance().leADevices.end(), (*it)), BtAVsClient::GetInstance().leADevices.end());
          BtAVsClient::GetInstance().cache_.putDevices(BtAVsClient::GetInstance().leADevices);
        }
      }
      break;
    }
    case ACL_CONNECTED: {
      uint64_t bdAddr = 0;
      uint32_t connHndl = 0;
      if (p.size() > 0) {
        JSTREAM_TO_UINT32(connHndl, p.data());
        JSTREAM_TO_UINT48(bdAddr, p.data()+4);
        auto it = std::find_if(BtAVsClient::GetInstance().leADevices.begin(), BtAVsClient::GetInstance().leADevices.end(), [bdAddr](le_audio_device d) { return d.bdAddr == bdAddr; });
        if (it != BtAVsClient::GetInstance().leADevices.end()) {
          ALOGI("%s: Reseting device 0x%x", __func__, bdAddr);
          BtAVsClient::GetInstance().leADevices.erase(std::remove(BtAVsClient::GetInstance().leADevices.begin(), BtAVsClient::GetInstance().leADevices.end(), (*it)), BtAVsClient::GetInstance().leADevices.end());
          BtAVsClient::GetInstance().cache_.putDevices(BtAVsClient::GetInstance().leADevices);
        }
        ALOGI("%s: Adding device 0x%x", __func__, bdAddr);
        BtAVsClient::GetInstance().leADevices.push_back({0, 0, bdAddr, false, false, false, false, (uint16_t)connHndl});
        BtAVsClient::GetInstance().cache_.putDevices(BtAVsClient::GetInstance().leADevices);
      }
      break;
    }
    case LE_AUDIO_CODEC_CONFIG_CHANGED: {
      uint32_t codecType = 0;
      uint64_t bdAddr = 0;
      JSTREAM_TO_UINT32(codecType, p.data());
      JSTREAM_TO_UINT48(bdAddr, p.data()+4);
      auto isLexConfigured = (codecType != CODEC_TYPE_APTX_ADAPTIVE_R4) ? false : true;
      auto it = std::find_if(BtAVsClient::GetInstance().leADevices.begin(), BtAVsClient::GetInstance().leADevices.end(), [bdAddr](le_audio_device d) { return d.bdAddr == bdAddr; });
      if (it != BtAVsClient::GetInstance().leADevices.end()) {
        it->isLexConfigured = isLexConfigured;
      } else {
        ALOGI("%s: Adding device 0x%x", __func__, bdAddr);
        BtAVsClient::GetInstance().leADevices.push_back({0, 0, bdAddr, false, false, isLexConfigured, false, 0});
      }
      BtAVsClient::GetInstance().cache_.putDevices(BtAVsClient::GetInstance().leADevices);
      break;
    }
    default:
      ALOGI("%s:: unknown evt: %d", __func__, evt);
      break;
  }
}

void BtAVsClient::UpdateVSCommandStatus (int32_t ofc, int32_t status) {
  ALOGI("%s: enter", __func__);
  ALOGI("%s: Unhandled Command Status", __func__);
}

void BtAVsClient::UpdateVSCommandComplete (int32_t ofc, const std::vector<uint8_t>& params) {
  ALOGI("%s: enter", __func__);
  if (ofc == HCI_VS_GET_ADDON_FEATURES_SUPPORT) {
    ALOGI("%s: VS Get Add-On features Support Command Complete", __func__);
    VSC_CMPL data = {.opcode = HCI_VS_GET_ADDON_FEATURES_SUPPORT,
                     .param_len = static_cast<uint16_t>(params.size()),
                     .p_param_buf = params.data()};
    BtAVsClient::GetInstance().ParseControllerAddonFeaturesResponse(&data);
  } else if (ofc == HCI_VS_QBCE_OCF) {
    ALOGI("%s: VS QBCE Command Complete", __func__);
    VSC_CMPL data = {.opcode = HCI_VS_QBCE_OCF,
                     .param_len = static_cast<uint16_t>(params.size()),
                     .p_param_buf = params.data()};
    BtAVsClient::GetInstance().ParseVsQbceCommandCompleteResponse(&data);
  }
}

void BtAVsClient::ParseVsQbceCommandCompleteResponse(VSC_CMPL* p_data) {
  uint8_t *stream, status, subcmd;
  uint16_t opcode, length;

  if (p_data && (stream = (uint8_t*)p_data->p_param_buf)) {
    opcode = p_data->opcode;
    length = p_data->param_len;
    STREAM_TO_UINT8(status, stream);

    if (status != HCI_SUCCESS) {
      ALOGE("%s: failed with status %d", __func__, status);
    }

    STREAM_TO_UINT8(subcmd, stream);
    ALOGI("%s: subcmd = %d", __func__, subcmd);

    if (subcmd == QBCE_READ_LOCAL_QLL_SUPPORTED_FEATURES) {
      STREAM_TO_ARRAY(local_qll_features.as_array, stream,
                      (int)sizeof(bt_device_qll_local_supported_features_t));
      qll_local_supported_features_length = length - 2;
      cache_.putLocalQllFeatures(local_qll_features);
    }
  }
}

void BtAVsClient::UpdateVSEvent (int32_t ofc, const std::vector<uint8_t>& p) {
  ALOGI("%s: enter", __func__);
  if (ofc == HCI_VS_QBCE_OCF) {
    ALOGI("%s: QBCE VS Event Received, msg = %d", __func__, p[0]);
    switch(p[0]) {
      case QBCE_QLL_CONNECTION_COMPLETE:
        BtAVsClient::GetInstance().QllConnectionCompleteEvt(p.data()+1);
        break;
      case QBCE_READ_REMOTE_QLL_SUPPORTED_FEATURE:
        BtAVsClient::GetInstance().ReadRemoteSupportedQllFeaturesCompleteEvt(p.data()+1);
        break;
      case QBCE_VS_CODEC_SETTINGS:
        BtAVsClient::GetInstance().UpdateCodecSettings(p.data()+1);
        break;
      default:
        ALOGI("%s:: unknown msg type: %d", __func__, p[0]);
        break;
    }
  }
}

void BtAVsClient::ParseControllerAddonFeaturesResponse(VSC_CMPL* p_data) {
  uint8_t *stream, status;
  uint16_t opcode, length;

  if (p_data && (stream = (uint8_t*)p_data->p_param_buf)) {
    opcode = p_data->opcode;
    length = p_data->param_len;
    STREAM_TO_UINT8(status, stream);

    if (status != HCI_SUCCESS) {
      ALOGE("%s: failed with status %d", __func__, status);
    }

    if (stream && (length > 8)) {
      STREAM_TO_UINT16(product_id, stream);
      STREAM_TO_UINT16(response_version, stream);

      soc_add_on_features_length = length - 5;
      STREAM_TO_ARRAY(soc_add_on_features.as_array, stream, soc_add_on_features_length);
      cache_.putSoCAddOnFeatures(soc_add_on_features);
    }

    ConfigQHS();
  }
}

void BtAVsClient::ConfigQHS() {
  if (!BtAVsProviderIf::GetIf()) {
    ALOGE("%s: No BtAVsProvider service instance!", __func__);
  }

  if (BTM_QBCE_QLE_HCI_SUPPORTED(soc_add_on_features.as_array)) {
    char qhs_iso[PROPERTY_VALUE_MAX] = "false";
    property_get("persist.vendor.qcom.bluetooth.qhs_enable", qhs_iso, "true");
    uint8_t cmd[3];
    uint8_t sub_cmd = QBCE_SET_QHS_HOST_MODE;

    memset(cmd, 0, 3);

    cmd[0] = sub_cmd;
    cmd[1] = QHS_TRANSPORT_LE_ISO;

    if (!strncmp("true", qhs_iso, 4)) {
      cmd[2] = QHS_HOST_MODE_HOST_AWARE;
    } else {
      cmd[2] = QHS_HOST_DISABLE_ALL;
    }
    BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF, std::vector<uint8_t>(cmd, cmd+sizeof(cmd))
                              /*qbce_set_qhs_host_mode_hci_cmd_complete*/);

    /* This property is for test/debug purpose only */
   qhs_support_mask = GetQhsSupportMask();
    if (qhs_support_mask != 0xFF) {
      if (qhs_support_mask & QHS_BREDR_MASK) {
        cmd[1] = QHS_TRANSPORT_BREDR;
        cmd[2] = QHS_HOST_MODE_HOST_AWARE;
      } else {
        cmd[1] = QHS_TRANSPORT_BREDR;
        cmd[2] = QHS_HOST_DISABLE_ALL;
      }
      BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF, std::vector<uint8_t>(cmd, cmd+sizeof(cmd))
                                /*qbce_set_qhs_host_mode_hci_cmd_complete*/);
      if (qhs_support_mask & QHS_LE_MASK) {
        cmd[1] = QHS_TRANSPORT_LE;
        cmd[2] = QHS_HOST_MODE_HOST_AWARE;
      } else {
        cmd[1] = QHS_TRANSPORT_LE;
        cmd[2] = QHS_HOST_DISABLE_ALL;
      }
      BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF, std::vector<uint8_t>(cmd, cmd+sizeof(cmd))
                                /*qbce_set_qhs_host_mode_hci_cmd_complete*/);
      if (qhs_support_mask & QHS_LE_MASK) {
        cmd[1] = QHS_TRANSPORT_LE_ISO;
        cmd[2] = QHS_HOST_MODE_HOST_AWARE;
      } else {
        cmd[1] = QHS_TRANSPORT_LE_ISO;
        cmd[2] = QHS_HOST_DISABLE_ALL;
      }
      BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF, std::vector<uint8_t>(cmd, cmd+sizeof(cmd))
                                /*qbce_set_qhs_host_mode_hci_cmd_complete*/);
    }
    uint8_t cmd_qll[9];
    uint8_t* stream = &cmd_qll[0];
    UINT8_TO_STREAM(stream, QBCE_SET_QLL_EVENT_MASK);
    ARRAY8_TO_STREAM(stream, (&QBCE_QLM_AND_QLL_EVENT_MASK)->as_array);
    BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF, std::vector<uint8_t>(cmd_qll, cmd_qll+sizeof(cmd_qll))
                              /*qbce_set_qll_event_mask_hci_cmd_complete*/);
  }

  if (BTM_QBCE_QCM_HCI_SUPPORTED(soc_add_on_features.as_array)) {
    uint8_t cmd_qlm[9];
    uint8_t* stream = &cmd_qlm[0];
    UINT8_TO_STREAM(stream, QBCE_SET_QLM_EVENT_MASK);
    ARRAY8_TO_STREAM(stream, (&QBCE_QLM_AND_QLL_EVENT_MASK)->as_array);
    BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF, std::vector<uint8_t>(cmd_qlm, cmd_qlm+sizeof(cmd_qlm))
                              /*qbce_set_qlm_event_mask_hci_cmd_complete*/);
  }

  if (BTM_QBCE_QLE_HCI_SUPPORTED(soc_add_on_features.as_array)) {
    uint8_t cmd[1];
    cmd[0] = QBCE_READ_LOCAL_QLL_SUPPORTED_FEATURES;
    BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF, std::vector<uint8_t>(cmd, cmd+sizeof(cmd))
                              /*parse_qll_read_local_supported_features_response*/);
  }
}

void BtAVsClient::ReadVendorAddOnFeatures() {
  int ret = 0;

  ALOGI("%s: Soc Add On", __func__);

  char soc_name[PROPERTY_VALUE_MAX] = {'\0'};

  ret = property_get("persist.vendor.qcom.bluetooth.soc", soc_name, "");
  ALOGI("%s:: Bluetooth soc type set to: %s, ret: %d", __func__, soc_name, ret);

  if (ret != 0) {
    bt_soc_type_t soc_type;
    soc_type = ConvertSocNameToBTSocType(soc_name);
    if (soc_type >= BT_SOC_TYPE_CHEROKEE && BtAVsProviderIf::GetIf()) {
      BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_GET_ADDON_FEATURES_SUPPORT, std::vector<uint8_t>()
                                /*parse_controller_addon_features_response*/);
    }
  }
}

void BtAVsClient::UpdateCodecSettings(const uint8_t* p) {
  ALOGI("%s: Codec Settings event received", __func__);
  uint16_t handle, delay = 0xFFFF;
  uint8_t types, mode = 0xFF;
  uint64_t bdAddr = 0xFFFFFFFFFFFFFFFF;

  STREAM_TO_UINT16(handle, p);
  STREAM_TO_UINT8(types, p);

  ALOGI("%s: handle = 0x%x, types = 0x%x", __func__, handle, types);

  while (types--) {
    uint8_t len = 0x00, type = 0xFF;
    STREAM_TO_UINT8(len, p);
    STREAM_TO_UINT8(type, p);
    ALOGI("%s: LTV type = 0x%x", __func__, type);
    if (len > 2 && type == 0x01) {
      STREAM_TO_UINT24(delay, p);
      auto it = std::find_if(leADevices.begin(), leADevices.end(), [](le_audio_device d) { return d.isActive; });
      if (it != leADevices.end()) {
        it->delay = delay;
        cache_.putDevices(leADevices);
      }
    } else if (len > 0 && type == 0x00) {
      STREAM_TO_UINT8(mode, p);
      auto it = std::find_if(leADevices.begin(), leADevices.end(), [](le_audio_device d) { return d.isActive; });
      if (it != leADevices.end()) {
        it->mode = mode;
        cache_.putDevices(leADevices);
      }
    } else if (len > 5 && type == 0x02) {
      STREAM_TO_UINT48(bdAddr, p);
      auto it = std::find_if(leADevices.begin(), leADevices.end(), [bdAddr](le_audio_device d) { return d.bdAddr == bdAddr; });
      if (it != leADevices.end()) {
        it->isLexTransportReady = true;
      } else {
        ALOGI("%s: Adding device 0x%x", __func__, bdAddr);
        leADevices.push_back({0, 0, bdAddr, false, true, false, true, handle});
      }
      cache_.putDevices(leADevices);
    }
  }
}

bt_device_qll_local_supported_features_t BtAVsClient::GetLocalQLLSupportedFeatures(uint8_t& len) {
  ALOGI("%s:", __func__);
  len = qll_local_supported_features_length;
  return local_qll_features;
}

bt_device_soc_add_on_features_t BtAVsClient::GetSoCAddOnFeatures(uint8_t& len) {
  ALOGI("%s:", __func__);
  len = soc_add_on_features_length;
  return soc_add_on_features;
}

bool BtAVsClient::IsAdapterStateOn() {
  ALOGI("%s: adapter_state = %d", __func__, adapter_state);
  return adapter_state == ADAPTER_STATE_ON;
}

bool BtAVsClient::IsLeXTransportAvailable() {
  auto it = std::find_if(leADevices.begin(), leADevices.end(), [](le_audio_device d) { return d.isActive; });
  if (it != leADevices.end()) {
    if (it->isLexTransportReady) {
      ALOGI("%s: LeX transport is available to stream.", __func__);
      return true;
    }
    ALOGI("%s: No LeX transport is available", __func__);
  } else {
    ALOGE("%s: No Active device is available", __func__);
  }
  return false;
}

bool BtAVsClient::IsLeXDevice() {
  ALOGI("%s:", __func__);
  auto it = std::find_if(leADevices.begin(), leADevices.end(), [](le_audio_device d) { return d.isActive; });
  if (it != leADevices.end()) {
    return it->isLexSupported;
  }
  ALOGE("%s: No Active device is available", __func__);
  return false;
}

bool BtAVsClient::IsLeXSelected() {
  ALOGI("%s:", __func__);
  auto it = std::find_if(leADevices.begin(), leADevices.end(), [](le_audio_device d) { return d.isActive; });
  if (it != leADevices.end()) {
    return it->isLexConfigured;
  }
  ALOGE("%s: No Active device is available", __func__);
  return false;
}

uint8_t BtAVsClient::GetMode() {
  ALOGI("%s:", __func__);
  auto it = std::find_if(leADevices.begin(), leADevices.end(), [](le_audio_device d) { return d.isActive; });
  if (it != leADevices.end()) {
    return it->mode;
  }
  ALOGE("%s: No Active device is available", __func__);
  return 0;
}

uint16_t BtAVsClient::GetDelay() {
  ALOGI("%s:", __func__);
  auto it = std::find_if(leADevices.begin(), leADevices.end(), [](le_audio_device d) { return d.isActive; });
  if (it != leADevices.end()) {
    return it->delay;
  }
  ALOGE("%s: No Active device is available", __func__);
  return 0;
}

remote_qll_features_t BtAVsClient::GetRemoteQLLFeatures() {
  ALOGI("%s:", __func__);
  remote_qll_features_t res = {};
  auto it = std::find_if(leADevices.begin(), leADevices.end(), [](le_audio_device d) { return d.isActive; });
  if (it != leADevices.end()) {
    return it->qll_features;
  }
  ALOGE("%s: No Active device is available", __func__);
  return res;
}

void BtAVsClient::SetCurrentA2dpAudioMode(bool is_gaming_mode) {
  ALOGI("%s: is_gaming_mode: %d", __func__, is_gaming_mode);
  is_in_a2dp_gaming = is_gaming_mode;
}

void BtAVsClient::SetCurrentA2dpLatencyMode(bool is_low_latency_mode) {
  ALOGI("%s: is_low_latency_mode: %d", __func__, is_low_latency_mode);
  is_in_a2dp_low_latency = is_low_latency_mode;
}

bool BtAVsClient::GetCurrentA2dpAudioMode() {
  get_latency_mode = is_in_a2dp_gaming || is_in_a2dp_low_latency;
  ALOGI("%s: get_latency_mode: %d", __func__, get_latency_mode);
  return get_latency_mode;
}

void BtAVsClient::SetLosslessMode(bool is_lossless_mode) {
  ALOGI("%s: is_lossless_mode: %d", __func__, is_lossless_mode);
  is_lossless_mode_supported = is_lossless_mode;
}

bool BtAVsClient::GetLosslessMode() {
  ALOGI("%s: is_lossless_mode: %d", __func__, is_lossless_mode_supported);
  return is_lossless_mode_supported;
}

void BtAVsClient::SetLosslessCapability(bool is_lossless_capability) {
  ALOGI("%s: is_lossless_capable: %d", __func__, is_lossless_capability);
  is_lossless_capability_supported = is_lossless_capability;
}

bool BtAVsClient::GetLosslessCapability() {
  ALOGI("%s: is_lossless_capable: %d", __func__, is_lossless_capability_supported);
  return is_lossless_capability_supported;
}

int BtAVsClient::GetLdacBitRate() {
  ALOGI("%s: current ldac bit rate: %d", __func__, current_ldac_bit_rate);
  return current_ldac_bit_rate;
}

void BtAVsClient::SetLdacBitRate(int bitrate) {
  ALOGI("%s: current ldac bit rate: %d", __func__, bitrate);
  current_ldac_bit_rate = bitrate;
}

uint8_t BtAVsClient::GetQhsSupportMask() {
  /* This property is for test/debug purpose only */
  property_get("persist.vendor.qcom.bluetooth.qhs_support", qhs_value, "255");
  ALOGI("%s: qhs property value= %s", __func__, qhs_value);
  qhs_support_mask = (uint8_t)atoi(qhs_value);
  ALOGI("%s: qhs support mask=%d", __func__, qhs_support_mask);
  return qhs_support_mask;
}

void BtAVsClient::ReadRemoteSupportedQllFeaturesCompleteEvt(const uint8_t* p) {
  ALOGI("%s: Read Remote Supported QLL Features event received", __func__);
  uint16_t handle;
  uint8_t status, param[3] = {0}, *p_param = param;

  STREAM_TO_UINT8(status, p);
  STREAM_TO_UINT16(handle, p);

  if (status != HCI_SUCCESS) {
    ALOGE("%s:: failed for handle: 0x%x, status 0x%x", __func__, handle, status);
    return;
  }
  auto it = std::find_if(leADevices.begin(), leADevices.end(), [handle](le_audio_device d) { return d.connHandle == handle; });
  if (it != leADevices.end()) {
    STREAM_TO_ARRAY(it->qll_features.as_array, p, BD_FEATURES_LEN);
    cache_.putDevices(leADevices);
  }
}

void BtAVsClient::QllConnectionCompleteEvt(const uint8_t* p) {
  uint16_t handle;
  uint8_t status, param[3] = {0}, *p_param = param;

  STREAM_TO_UINT8(status, p);
  STREAM_TO_UINT16(handle, p);
  handle = handle & 0x0FFF;

  if (status != HCI_SUCCESS) {
    ALOGE("%s:: failed for handle: 0x%x, status 0x%x", __func__, handle, status);
    return;
  }

  if (!BtAVsProviderIf::GetIf()) {
    ALOGE("%s: No BtAVsProvider service instance!", __func__);
    return;
  }

  UINT8_TO_STREAM(p_param, QBCE_READ_REMOTE_QLL_SUPPORTED_FEATURE);
  UINT16_TO_STREAM(p_param, handle);
  BtAVsProviderIf::GetIf()->sendVSCommand(HCI_VS_QBCE_OCF, std::vector<uint8_t>(param, param+QBCE_READ_REMOTE_QLL_SUPPORTED_FEATURE_LEN)
                            /*btm_ble_read_remote_supported_qll_features_status_cback*/);
}

bt_soc_type_t BtAVsClient::ConvertSocNameToBTSocType(const char* soc_name) {
  bt_soc_type_t soc_type;
  if (!strncasecmp(soc_name, "rome", sizeof("rome"))) {
    soc_type = BT_SOC_TYPE_ROME;
  } else if (!strncasecmp(soc_name, "cherokee", sizeof("cherokee"))) {
    soc_type = BT_SOC_TYPE_CHEROKEE;
  } else if (!strncasecmp(soc_name, "ath3k", sizeof("ath3k"))) {
    soc_type = BT_SOC_TYPE_AR3K;
  } else if (!strncasecmp(soc_name, "hastings", sizeof("hastings"))) {
    soc_type = BT_SOC_TYPE_HASTINGS;
  } else if (!strncasecmp(soc_name, "moselle", sizeof("moselle"))) {
    soc_type = BT_SOC_TYPE_MOSELLE;
  } else if (!strncasecmp(soc_name, "hamilton", sizeof("hamilton"))) {
    soc_type = BT_SOC_TYPE_HAMILTON;
  } else if (!strncasecmp(soc_name, "ganges", sizeof("ganges"))) {
    soc_type = BT_SOC_TYPE_GANGES;
  } else if (!strncasecmp(soc_name, "orne", sizeof("orne"))) {
    soc_type = BT_SOC_TYPE_ORNE;
  } else if (!strncasecmp(soc_name, "congo", sizeof("congo"))) {
    soc_type = BT_SOC_TYPE_CONGO;
  } else if (!strncasecmp(soc_name, "pronto", sizeof("pronto"))) {
    soc_type = BT_SOC_TYPE_DEFAULT;
  } else {
    ALOGI("not set, so using pronto");
    soc_type = BT_SOC_TYPE_DEFAULT;
  }

  ALOGI("soc_name: %s, soc_type = %04x", soc_name, soc_type);
  return soc_type;
}

LeAudioContextType BtAVsClient::AudioContentToLeAudioContext(
          audio_content_type_t content_type, audio_usage_t usage) {
  /* Check audio attribute usage of stream */
  switch (usage) {
    case AUDIO_USAGE_MEDIA:
      return LeAudioContextType::MEDIA;
    case AUDIO_USAGE_ASSISTANT:
    case AUDIO_USAGE_ASSISTANCE_ACCESSIBILITY:
      return LeAudioContextType::VOICEASSISTANTS;
    case AUDIO_USAGE_VOICE_COMMUNICATION:
    case AUDIO_USAGE_CALL_ASSISTANT:
      return LeAudioContextType::CONVERSATIONAL;
    case AUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING:
      if (content_type == AUDIO_CONTENT_TYPE_SPEECH) {
        return LeAudioContextType::CONVERSATIONAL;
      }

      if (content_type == AUDIO_CONTENT_TYPE_SONIFICATION) {
        return LeAudioContextType::RINGTONE;
      }

      return LeAudioContextType::SOUNDEFFECTS;
    case AUDIO_USAGE_GAME:
      return LeAudioContextType::GAME;
    case AUDIO_USAGE_NOTIFICATION:
      return LeAudioContextType::NOTIFICATIONS;
    case AUDIO_USAGE_NOTIFICATION_TELEPHONY_RINGTONE:
      return LeAudioContextType::RINGTONE;
    case AUDIO_USAGE_ALARM:
      return LeAudioContextType::ALERTS;
    case AUDIO_USAGE_EMERGENCY:
      return LeAudioContextType::EMERGENCYALARM;
    case AUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE:
      return LeAudioContextType::INSTRUCTIONAL;
    case AUDIO_USAGE_ASSISTANCE_SONIFICATION:
      return LeAudioContextType::SOUNDEFFECTS;
    default:
      break;
  }

  return LeAudioContextType::MEDIA;
}

void BtAVsClient::UpdateVAAudioContent(
       audio_content_type_t& content_type, audio_usage_t& usage) {
  if (AudioContentToLeAudioContext(content_type, usage) ==
       LeAudioContextType::VOICEASSISTANTS) {
    usage = AUDIO_USAGE_VOICE_COMMUNICATION;
  }
}

void BtAVsClient::UpdateVAAudioSource(
       audio_source_t& source) {
  if (AudioSourceToLeAudioContext(source) ==
      LeAudioContextType::VOICEASSISTANTS) {
    source = AUDIO_SOURCE_VOICE_COMMUNICATION;
  }
}

LeAudioContextType BtAVsClient::AudioSourceToLeAudioContext(
       audio_source_t source) {
  if (source == AUDIO_SOURCE_MIC ||
      source == AUDIO_SOURCE_CAMCORDER ) {
    return LeAudioContextType::LIVE;

  } else if (source == AUDIO_SOURCE_VOICE_COMMUNICATION ||
             source == AUDIO_SOURCE_VOICE_CALL) {
    return LeAudioContextType::CONVERSATIONAL;
  } else {
    return LeAudioContextType::VOICEASSISTANTS;
  }
}
