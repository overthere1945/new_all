/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a contribution.
 *
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BluetoothChannelSoundingSession.h"
#include "BluetoothChannelSoundingAlgo.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "vendor.qti.bcs@1.0-bcssession"

namespace aidl::android::hardware::bluetooth::ranging::impl {
std::shared_ptr <BluetoothChannelSoundingAlgo> bcs_algo = BluetoothChannelSoundingAlgo::Get();
CsChannelSoundingProcedureData channelSoundingProcedureData;

BluetoothChannelSoundingSession::BluetoothChannelSoundingSession(
    std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback,
    Reason reason) {
  callback_ = callback;
  ALOGI("%s", __func__);
  bcs_algo = BluetoothChannelSoundingAlgo::Get();
  if (bcs_algo == nullptr) {
    ALOGI("%s: failed to get BluetoothChannelSoundingAlgo instance", __func__);
    goto failed;
  }

  if (bcs_algo->InitializeAlgoHandles(callback) == false) {
    ALOGI("%s: failed to get open or read the algo lib", __func__);
    goto failed;
  }

  callback_->onOpened(reason);
  return;

failed:
  callback_->onOpenFailed(reason);
  return;
}

ndk::ScopedAStatus BluetoothChannelSoundingSession::getVendorSpecificReplies(
    std::optional<
        std::vector<std::optional<VendorSpecificData>>>* /*_aidl_return*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::getSupportedResultTypes(
    std::vector<ResultType>* _aidl_return) {
  std::vector<ResultType> supported_result_types = {ResultType::RESULT_METERS};
  *_aidl_return = supported_result_types;
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::isAbortedProcedureRequired(
    bool* _aidl_return) {
  *_aidl_return = false;
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::writeRawData(
    const ChannelSoudingRawData& params) {

  RangingResult ranging_result;
  ranging_result.resultMeters = 0.0;

  CsMeasurement mAlgoMeasurement;

  int steps = params.stepChannels.size();
  int mode = params.stepMode.size();
  ALOGV("%s: total no of steps %d and total no of mode %d", __func__, steps, mode);
  ALOGV("%s: params.initiatorData.packetRssiDbm %d", __func__, params.initiatorData.packetRssiDbm->size());
  ALOGV("%s: params.initiatorData.packetQuality %d", __func__, params.initiatorData.packetQuality->size());
  ALOGV("%s: params.initiatorData.measuredFreqOffset %d", __func__, params.initiatorData.measuredFreqOffset->size());
  ALOGV("%s: params.initiatorData.frequencyCompensation %d", __func__, params.frequencyCompensation.size());
  ALOGV("%s: params.numAntennaPaths %d", __func__, params.numAntennaPaths);

  ALOGV("%s: params.reflectorData.packetQuality %d", __func__, params.reflectorData.packetQuality->size());
  ALOGV("%s: params.reflectorData.packetRssiDbm %d", __func__, params.reflectorData.packetRssiDbm->size());

  mAlgoMeasurement.N = steps;
  mAlgoMeasurement.timeUs = (Uint32)params.timestampMs;
  mAlgoMeasurement.init = (CsResult*) new CsResult[steps];
  mAlgoMeasurement.refl = (CsResult*) new CsResult[steps];
  mAlgoMeasurement.channel = (Int8 *) new Int8[steps];
  mAlgoMeasurement.mode = (Int8 *) new Int8[steps];
  mAlgoMeasurement.NAP = (Uint8)params.numAntennaPaths;

  uint8_t NAP = params.numAntennaPaths;
  uint8_t frequencyCompensationlen = params.frequencyCompensation.size();
  uint8_t initmeasuredFreqOffsetlen = params.initiatorData.measuredFreqOffset->size();
  uint8_t initpacketRssiDbmlen = params.initiatorData.packetRssiDbm->size() ;
  uint8_t initpacketQualitylen = params.initiatorData.packetQuality->size();
  uint8_t reflpacketQualitylen = params.reflectorData.packetQuality->size();
  uint8_t reflpacketRssiDbmlen = params.reflectorData.packetRssiDbm->size();
  uint8_t initToaTodlen = params.toaTodInitiator->size();
  uint8_t reflToaTodlen = params.todToaReflector->size();


  int32_t *frequencyCompensation = (int32_t*)params.frequencyCompensation.data();
  int32_t *initmeasuredFreqOffset = (int32_t*)params.initiatorData.measuredFreqOffset->data();
  int8_t *initpacketRssiDbm = (int8_t *)params.initiatorData.packetRssiDbm->data();
  uint8_t *initpacketQuality = (uint8_t *)params.initiatorData.packetQuality->data();
  uint8_t *reflpacketQuality = (uint8_t *)params.reflectorData.packetQuality->data();
  int8_t *reflpacketRssiDbm = (int8_t *)params.reflectorData.packetRssiDbm->data();
  int *initToaTod = (int *)params.toaTodInitiator->data();
  int *reflToaTod = (int *)params.todToaReflector->data();

  std::vector<ComplexNumber> init_tonePcts;
  std::vector<ComplexNumber> refl_tonePcts;
  std::vector<uint8_t> init_toneQualityIndicator;
  std::vector<uint8_t> refl_toneQualityIndicator;
  std::vector<int8_t> antennaPermutationIndex;
  std::vector<int8_t> init_toneExtensionAntennaIndex;
  std::vector<int8_t> refl_toneExtensionAntennaIndex;
  uint8_t initTonePctLen = params.initiatorData.stepTonePcts->size();
  uint8_t reflTonePctLen = params.reflectorData.stepTonePcts->size();

  ALOGV("%s initTonePctLen: %d, reflTonePctLen: %d", __func__, initTonePctLen, reflTonePctLen);
 
  for (uint8_t i=0; params.initiatorData.stepTonePcts && params.initiatorData.stepTonePcts->size() != 0 && i<params.initiatorData.stepTonePcts->at(0)->tonePcts.size(); i++) {
    for (uint8_t j=0;j<initTonePctLen;j++) {
      auto tonePcts_ = params.initiatorData.stepTonePcts->at(j)->tonePcts.at(i);
      ALOGV("%s init tonePcts_.real %f tonePcts_.imaginary %f", __func__, tonePcts_.real, tonePcts_.imaginary);
      init_tonePcts.push_back(tonePcts_);
    }
  }

  for (uint8_t i=0;params.reflectorData.stepTonePcts && params.reflectorData.stepTonePcts->size() != 0 && i<params.reflectorData.stepTonePcts->at(0)->tonePcts.size(); i++) {
    for (uint8_t j=0;j<reflTonePctLen;j++) {
      auto tonePcts_ = params.reflectorData.stepTonePcts->at(j)->tonePcts.at(i);
      ALOGV("%s init tonePcts_.real %f tonePcts_.imaginary %f", __func__, tonePcts_.real, tonePcts_.imaginary);
      refl_tonePcts.push_back(tonePcts_);
    }
  }
  for (auto StepTonePct : *params.initiatorData.stepTonePcts) {
    for (auto toneQualityIndicator_: StepTonePct->toneQualityIndicator) {
      init_toneQualityIndicator.push_back(toneQualityIndicator_);
    }
    init_toneExtensionAntennaIndex.push_back(StepTonePct->toneExtensionAntennaIndex);
  }

  for (auto StepTonePct : *params.reflectorData.stepTonePcts) {
    for (auto toneQualityIndicator_: StepTonePct->toneQualityIndicator) {
      refl_toneQualityIndicator.push_back(toneQualityIndicator_);
    }
    refl_toneExtensionAntennaIndex.push_back(StepTonePct->toneExtensionAntennaIndex);
  }

  for (auto antennaPermutationIndex_ : *params.initiatorData.vendorSpecificCsSingleSidedata) {
    antennaPermutationIndex.push_back(antennaPermutationIndex_);
  }

  ALOGV("%s: init_toneQualityIndicator.len %d", __func__, init_toneQualityIndicator.size());
  ALOGV("%s: init_tonePcts len %d", __func__, init_tonePcts.size());
  ALOGV("%s: refl_toneQualityIndicator.len %d", __func__, init_toneQualityIndicator.size());
  ALOGV("%s: refl_tonePcts len %d", __func__, refl_tonePcts.size());
  ALOGV("%s: antennaPermutationIndex %d", __func__, antennaPermutationIndex.size());
  ALOGV("%s: params antennaPermutationIndex %d", __func__, params.initiatorData.vendorSpecificCsSingleSidedata->size());
  ALOGV("%s: initToaTodlen %d reflToaTodlen %d", __func__,initToaTodlen, reflToaTodlen);
  ALOGV("%s: init_toneExtensionAntennaIndex %d refl_toneExtensionAntennaIndex %d", __func__,
          init_toneExtensionAntennaIndex.size(), refl_toneExtensionAntennaIndex.size());

  uint8_t init_toneExtensionAntennaIndex_len = init_toneExtensionAntennaIndex.size();
  uint8_t refl_toneExtensionAntennaIndex_len = refl_toneExtensionAntennaIndex.size();
  uint8_t mode0 = 0, mode1 = 0 ,mode2 = 0, mode3 = 0;

  uint16_t init_pct = 0,refl_pct = 0;
  for (uint8_t i = 0; i <steps; i++) {
    mAlgoMeasurement.channel[i]= params.stepChannels[i];
    mAlgoMeasurement.mode[i] = (Int8)params.stepMode[i];
    if (params.stepMode[i] == ModeType:: ZERO) {
     /* mode 0 initiator */
     {
     if (mode0 < initmeasuredFreqOffsetlen) {
       mAlgoMeasurement.init[i].mode0.freqOff = (int16_t)initmeasuredFreqOffset[mode0];
     }
     if ((mode0 + mode1) < initpacketRssiDbmlen) {
       mAlgoMeasurement.init[i].mode0.rssi = (Int8)initpacketRssiDbm[mode0 + mode1];
     }
     if ((mode0+mode1) < initpacketQualitylen) {
       mAlgoMeasurement.init[i].mode0.aaQual =  (Int8)initpacketQuality[mode0 + mode1];
     }
     if (mode0 < frequencyCompensationlen) {
       mAlgoMeasurement.freqCmp  = (Int16)frequencyCompensation[mode0];
     }
     }
     /* reflector */
     {
     if ((mode0+mode1) < reflpacketRssiDbmlen)
       mAlgoMeasurement.refl[i].mode0.rssi = (Int8)reflpacketRssiDbm[mode0 + mode1];
     if ((mode0+mode1) < reflpacketQualitylen)
       mAlgoMeasurement.refl[i].mode0.aaQual =  (Int8)reflpacketQuality[mode0 + mode1];
     }
     mode0++;
    } else if (params.stepMode[i] == ModeType::ONE) {
      /* mode 1 initiator */
      {
     if ((mode1+mode0) < initpacketRssiDbmlen) {
       mAlgoMeasurement.init[i].mode1.rssi = (Int8)initpacketRssiDbm[mode0+mode1];
     }
     if ((mode1+mode0) < initpacketQualitylen) {
       mAlgoMeasurement.init[i].mode1.aaQual =  (Int8)initpacketQuality[mode0+mode1];
     }
     if (mode1 < initToaTodlen) {
       mAlgoMeasurement.init[i].mode1.timeEst =  (Int16)initToaTod[mode1];
     }
      }
      /* mode 1 reflector */
      {
         if ((mode1+mode0) < reflpacketRssiDbmlen)
       mAlgoMeasurement.refl[i].mode1.rssi = (Int8)reflpacketRssiDbm[mode0+mode1];
     if ((mode1+mode0) < reflpacketQualitylen)
       mAlgoMeasurement.refl[i].mode1.aaQual =  (Int8)reflpacketQuality[mode0 + mode1];
         if (mode1 < reflToaTodlen) {
       mAlgoMeasurement.refl[i].mode1.timeEst =  (Int16)reflToaTod[mode1];
     }
      }
      mode1++;
    } else if (params.stepMode[i] == ModeType:: TWO) {
      /* mode 2 initiator */
      {
     if (mode2 < init_toneExtensionAntennaIndex_len) {
       mAlgoMeasurement.init[i].mode2.antPerm = (Uint8)init_toneExtensionAntennaIndex[mode2];
     }
     //mAlgoMeasurement.init[i].mode2.pctQual =  &init_toneQualityIndicator[mode2];
     Int16 *pct = (Int16 *) new Int16[sizeof(Int16) * NAP*2];
     Uint8 *pct_quality = (Uint8 *) new Uint8[sizeof(Uint8) * NAP];
     /* real & * imaganiary */
     for (int k=0, j=0; j < NAP *2;) {
           ComplexNumber pcts = init_tonePcts[init_pct];
       pct_quality[k] = init_toneQualityIndicator[init_pct];
       pct[j] = (Int16)(pcts.real * 2048);
       j = j+1;
       pct[j] = (Int16)(pcts.imaginary *2048);
       j = j+1;
       init_pct++;
       k++;
     }
     mAlgoMeasurement.init[i].mode2.pctQual = pct_quality;
     mAlgoMeasurement.init[i].mode2.pct = pct;
     init_pct += 1;
    }
      /* mode 2 reflector */
      {
     if (mode2 < refl_toneExtensionAntennaIndex_len) {
       mAlgoMeasurement.refl[i].mode2.antPerm = (Uint8)refl_toneExtensionAntennaIndex[mode2];
     }
    // mAlgoMeasurement.refl[i].mode2.pctQual =  &refl_toneQualityIndicator[mode2];
     /* real & * imaganiary */
     Int16 *pct = (Int16 *) new Int16[sizeof(Int16) * NAP * 2];
     Uint8 *pct_quality = (Uint8 *) new Uint8[sizeof(Uint8) * NAP];
         for (int k=0, j =0 ; j <  NAP*2;)
     {
           ComplexNumber pcts = refl_tonePcts[refl_pct];
       pct_quality[k] = refl_toneQualityIndicator[refl_pct];
       pct[j] = (Int16)(pcts.real * 2048);
       j = j+1;
       pct[j] = (Int16)(pcts.imaginary *2048);
       j = j+1;
       refl_pct++;
       k++;
     }
     mAlgoMeasurement.refl[i].mode2.pct = pct;
     mAlgoMeasurement.refl[i].mode2.pctQual = pct_quality;
     refl_pct +=1;
      }
      mode2++;
    } else if (params.stepMode[i] == ModeType:: THREE) {  
      /* mode 3 initiator */
      {
     if (mode3 < init_toneExtensionAntennaIndex_len) {
       mAlgoMeasurement.init[i].mode3.antPerm = (Uint8)init_toneExtensionAntennaIndex[mode3];
     }
     //mAlgoMeasurement.init[i].mode3.pctQual =  &init_toneQualityIndicator[mode3];
     Int16 *pct = (Int16 *) new Int16[sizeof(Int16) * NAP*3];  //assuming no of antenna paths= 3
     Uint8 *pct_quality = (Uint8 *) new Uint8[sizeof(Uint8) * NAP];
     /* real & * imaganiary */
     for (int k=0, j=0; j < NAP *2;) {
           ComplexNumber pcts = init_tonePcts[init_pct];
       pct_quality[k] = init_toneQualityIndicator[init_pct];
       pct[j] = (Int16)(pcts.real * 2048);
       j = j+1;
       pct[j] = (Int16)(pcts.imaginary *2048);
       j = j+1;
       init_pct++;
       k++;
     }
     mAlgoMeasurement.init[i].mode3.pctQual = pct_quality;
     mAlgoMeasurement.init[i].mode3.pct = pct;
     init_pct += 1;
     if ((mode1+mode0+mode3) < initpacketRssiDbmlen) {
       mAlgoMeasurement.init[i].mode3.rssi = (Int8)initpacketRssiDbm[mode0+mode1+mode3];
     }
     if ((mode1+mode0 + mode3) < initpacketQualitylen) {
       mAlgoMeasurement.init[i].mode3.aaQual =  (Int8)initpacketQuality[mode0+mode1 + mode3];
     }
     if (mode3 < initToaTodlen) {
       mAlgoMeasurement.init[i].mode3.timeEst =  (Int16)initToaTod[mode3];
     }
    }
      /* mode 3 reflector */
      {
     if (mode3 < refl_toneExtensionAntennaIndex_len) {
       mAlgoMeasurement.refl[i].mode3.antPerm = (Uint8)refl_toneExtensionAntennaIndex[mode3];
     }
    // mAlgoMeasurement.refl[i].mode3.pctQual =  &refl_toneQualityIndicator[mode2];
     /* real & * imaganiary */
     Int16 *pct = (Int16 *) new Int16[sizeof(Int16) * NAP * 3];
     Uint8 *pct_quality = (Uint8 *) new Uint8[sizeof(Uint8) * NAP];
         for (int k=0, j =0 ; j <  NAP*2;)
     {
           ComplexNumber pcts = refl_tonePcts[refl_pct];
       pct_quality[k] = refl_toneQualityIndicator[refl_pct];
       pct[j] = (Int16)(pcts.real * 2048);
       j = j+1;
       pct[j] = (Int16)(pcts.imaginary *2048);
       j = j+1;
       refl_pct++;
       k++;
     }
     mAlgoMeasurement.refl[i].mode3.pct = pct;
     mAlgoMeasurement.refl[i].mode3.pctQual = pct_quality;
     refl_pct +=1;
     if ((mode1+mode0+mode3) < reflpacketRssiDbmlen)
       mAlgoMeasurement.refl[i].mode3.rssi = (Int8)reflpacketRssiDbm[mode0 + mode1 + mode3];
     if ((mode1+mode0+mode3) < reflpacketQualitylen)
       mAlgoMeasurement.refl[i].mode3.aaQual =  (Int8)reflpacketQuality[mode0 + mode1 + mode3];
         if (mode3 < reflToaTodlen) {
          mAlgoMeasurement.refl[i].mode3.timeEst =  (Int16)reflToaTod[mode3];
        }
      }
      mode3++;
    }
  }

  ALOGV("%s init_pct%d refl_pct %d", __func__, init_pct, refl_pct);
  bcs_algo->csIfAddMeasurement(&mAlgoMeasurement);

  init_pct = refl_pct = 0;
  for (uint8_t i = 0; i <steps; i++) {
    if (params.stepMode[i] == ModeType:: TWO) {
      delete mAlgoMeasurement.init[i].mode2.pct;
      delete mAlgoMeasurement.refl[i].mode2.pct;
      delete mAlgoMeasurement.init[i].mode2.pctQual;
      delete mAlgoMeasurement.refl[i].mode2.pctQual;
    }
  }

  delete mAlgoMeasurement.init;
  delete mAlgoMeasurement.refl;
  delete mAlgoMeasurement.channel;
  delete mAlgoMeasurement.mode;

 return ::ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothChannelSoundingSession::close(Reason in_reason) {
  ALOGI("%s", __func__);

  if (bcs_algo) {
    bcs_algo->DeinitializeAlgoHandles();
  }

  callback_->onClose(in_reason);
  return ::ndk::ScopedAStatus::ok();
}
#ifdef BCS_HAL_V2
ndk::ScopedAStatus BluetoothChannelSoundingSession::writeProcedureData(
   const ChannelSoundingProcedureData& in_procedureData) {

  
  int initiatorSubeventResultDataSize = in_procedureData.initiatorSubeventResultData.size();
  int reflectorSubeventResultDataSize = in_procedureData.reflectorSubeventResultData.size();

  channelSoundingProcedureData.procedureCounter = in_procedureData.procedureCounter;
  channelSoundingProcedureData.procedureSequence = in_procedureData.procedureSequence;
  channelSoundingProcedureData.initiatorSelectedTxPower = in_procedureData.initiatorSelectedTxPower;
  channelSoundingProcedureData.reflectorSelectedTxPower = in_procedureData.reflectorSelectedTxPower;
  channelSoundingProcedureData.initiatorSubeventResultDataSize = (int32_t) initiatorSubeventResultDataSize;
  channelSoundingProcedureData.reflectorSubeventResultDataSize = (int32_t) reflectorSubeventResultDataSize;
  channelSoundingProcedureData.initiatorProcedureAbortReason = (int8_t) in_procedureData.initiatorProcedureAbortReason;
  channelSoundingProcedureData.reflectorProcedureAbortReason = (int8_t) in_procedureData.reflectorProcedureAbortReason;

  channelSoundingProcedureData.initiatorSubeventResultData = new CsSubeventResultData[initiatorSubeventResultDataSize];
  channelSoundingProcedureData.reflectorSubeventResultData = new CsSubeventResultData[reflectorSubeventResultDataSize];

  /* Initiator Subevent Result Data parsing*/

  for (int i=0;i<initiatorSubeventResultDataSize;i++) {

    std::vector<SubeventResultData> initiatorSubeventResultData = in_procedureData.initiatorSubeventResultData;
    int64_t steps = initiatorSubeventResultData[i].stepData.size();
  
    channelSoundingProcedureData.initiatorSubeventResultData[i].startAclConnEventCounter = initiatorSubeventResultData[i].startAclConnEventCounter;
    channelSoundingProcedureData.initiatorSubeventResultData[i].frequencyCompensation = initiatorSubeventResultData[i].frequencyCompensation;
    channelSoundingProcedureData.initiatorSubeventResultData[i].referencePowerLevelDbm = initiatorSubeventResultData[i].referencePowerLevelDbm;
    channelSoundingProcedureData.initiatorSubeventResultData[i].numAntennaPaths = initiatorSubeventResultData[i].numAntennaPaths;
    channelSoundingProcedureData.initiatorSubeventResultData[i].subeventAbortReason = (int8_t) initiatorSubeventResultData[i].subeventAbortReason;
    channelSoundingProcedureData.initiatorSubeventResultData[i].stepDataSize = steps;
    channelSoundingProcedureData.initiatorSubeventResultData[i].timestampNanos = initiatorSubeventResultData[i].timestampNanos;

    channelSoundingProcedureData.initiatorSubeventResultData[i].stepData = new CsStepData[steps];

    for (int64_t j=0;j<steps;j++) {
      vector<StepData> stepData = initiatorSubeventResultData[i].stepData;
      ModeType mode = stepData[j].stepMode;

      channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepChannel = stepData[j].stepChannel;
      channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepMode = (int8_t)mode;

      if (mode == ModeType::ZERO) {
        CsModeZeroData modeZeroData;
        modeZeroData.packetQuality = stepData[j].stepModeData.get<ModeData::modeZeroData>().packetQuality;
        modeZeroData.packetRssiDbm = stepData[j].stepModeData.get<ModeData::modeZeroData>().packetRssiDbm;
        modeZeroData.packetAntenna = stepData[j].stepModeData.get<ModeData::modeZeroData>().packetAntenna;
        modeZeroData.initiatorMeasuredFreqOffset = stepData[j].stepModeData.get<ModeData::modeZeroData>().initiatorMeasuredFreqOffset;
        channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepModeData.modeZeroData = modeZeroData;
      } else if (mode == ModeType::ONE) {
        CsModeOneData modeOneData;
        modeOneData.packetQuality = stepData[j].stepModeData.get<ModeData::modeOneData>().packetQuality;
        modeOneData.packetNadm = (int8_t)stepData[j].stepModeData.get<ModeData::modeOneData>().packetNadm;
        modeOneData.packetRssiDbm = stepData[j].stepModeData.get<ModeData::modeOneData>().packetRssiDbm;
        modeOneData.rttToaTodData.toaTodInitiator = stepData[j].stepModeData.get<ModeData::modeOneData>().rttToaTodData.get<RttToaTodData::toaTodInitiator>();
        modeOneData.packetAntenna = stepData[j].stepModeData.get<ModeData::modeOneData>().packetAntenna;
        modeOneData.packetPct1.iSample = stepData[j].stepModeData.get<ModeData::modeOneData>().packetPct1->iSample;
        modeOneData.packetPct1.qSample = stepData[j].stepModeData.get<ModeData::modeOneData>().packetPct1->qSample;
        modeOneData.packetPct2.iSample = stepData[j].stepModeData.get<ModeData::modeOneData>().packetPct2->iSample;
        modeOneData.packetPct2.qSample = stepData[j].stepModeData.get<ModeData::modeOneData>().packetPct2->qSample;
        channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepModeData.modeOneData = modeOneData;
      } else if (mode == ModeType::TWO) {
        CsModeTwoData modeTwoData;
        int32_t tonePctIQSampleSize = stepData[j].stepModeData.get<ModeData::modeTwoData>().tonePctIQSamples.size();
        int32_t toneQualityIndicatorsSize = stepData[j].stepModeData.get<ModeData::modeTwoData>().toneQualityIndicators.size();
        modeTwoData.antennaPermutationIndex = stepData[j].stepModeData.get<ModeData::modeTwoData>().antennaPermutationIndex;
        modeTwoData.tonePctIQSampleSize = tonePctIQSampleSize;
        modeTwoData.toneQualityIndicatorsSize = toneQualityIndicatorsSize;
        modeTwoData.tonePctIQSamples = new CsPctIQSample[tonePctIQSampleSize];
        modeTwoData.toneQualityIndicators = new uint8_t[toneQualityIndicatorsSize];
        for (int32_t k=0;k<tonePctIQSampleSize;k++) {
          modeTwoData.tonePctIQSamples[k].iSample = stepData[j].stepModeData.get<ModeData::modeTwoData>().tonePctIQSamples[k].iSample;
          modeTwoData.tonePctIQSamples[k].qSample = stepData[j].stepModeData.get<ModeData::modeTwoData>().tonePctIQSamples[k].qSample;
        }
        for (int32_t k=0;k<toneQualityIndicatorsSize;k++) {
          modeTwoData.toneQualityIndicators[k] = (uint8_t)stepData[j].stepModeData.get<ModeData::modeTwoData>().toneQualityIndicators[k];
        }
        channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepModeData.modeTwoData = modeTwoData;
      } else if (mode == ModeType::THREE) {
        /*Mode one data for mode 3*/
        CsModeOneData modeOneData;
        modeOneData.packetQuality = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetQuality;
        modeOneData.packetNadm = (int8_t)stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetNadm;
        modeOneData.packetRssiDbm = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetRssiDbm;
        modeOneData.rttToaTodData.toaTodInitiator = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.rttToaTodData.get<RttToaTodData::toaTodInitiator>();
        modeOneData.packetAntenna = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetAntenna;
        modeOneData.packetPct1.iSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetPct1->iSample;
        modeOneData.packetPct1.qSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetPct1->qSample;
        modeOneData.packetPct2.iSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetPct2->iSample;
        modeOneData.packetPct2.qSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetPct2->qSample;
        /*Mode two data for mode 3*/
        CsModeTwoData modeTwoData;
        int32_t tonePctIQSampleSize = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.tonePctIQSamples.size();
        int32_t toneQualityIndicatorsSize = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.toneQualityIndicators.size();
        modeTwoData.antennaPermutationIndex = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.antennaPermutationIndex;
        modeTwoData.tonePctIQSampleSize = tonePctIQSampleSize;
        modeTwoData.toneQualityIndicatorsSize = toneQualityIndicatorsSize;
        modeTwoData.tonePctIQSamples = new CsPctIQSample[tonePctIQSampleSize];
        modeTwoData.toneQualityIndicators = new uint8_t[toneQualityIndicatorsSize];
        for (int32_t k=0;k<tonePctIQSampleSize;k++) {
          modeTwoData.tonePctIQSamples[k].iSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.tonePctIQSamples[k].iSample;
          modeTwoData.tonePctIQSamples[k].qSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.tonePctIQSamples[k].qSample;
        }
        for (int32_t k=0;k<toneQualityIndicatorsSize;k++) {
          modeTwoData.toneQualityIndicators[k] = (uint8_t)stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.toneQualityIndicators[k];
        }
        channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepModeData.modeThreeData.modeOneData = modeOneData;
        channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepModeData.modeThreeData.modeTwoData = modeTwoData;
      }


    }
  }

  /*Reflector subevent result parsing*/

  for (int i=0;i<reflectorSubeventResultDataSize;i++) {

    std::vector<SubeventResultData> reflectorSubeventResultData = in_procedureData.reflectorSubeventResultData;
    int64_t steps = reflectorSubeventResultData[i].stepData.size();
  
    channelSoundingProcedureData.reflectorSubeventResultData[i].startAclConnEventCounter = reflectorSubeventResultData[i].startAclConnEventCounter;
    channelSoundingProcedureData.reflectorSubeventResultData[i].frequencyCompensation = reflectorSubeventResultData[i].frequencyCompensation;
    channelSoundingProcedureData.reflectorSubeventResultData[i].referencePowerLevelDbm = reflectorSubeventResultData[i].referencePowerLevelDbm;
    channelSoundingProcedureData.reflectorSubeventResultData[i].numAntennaPaths = reflectorSubeventResultData[i].numAntennaPaths;
    channelSoundingProcedureData.reflectorSubeventResultData[i].subeventAbortReason = (int8_t) reflectorSubeventResultData[i].subeventAbortReason;
    channelSoundingProcedureData.reflectorSubeventResultData[i].stepDataSize = steps;
    channelSoundingProcedureData.reflectorSubeventResultData[i].timestampNanos = reflectorSubeventResultData[i].timestampNanos;

    channelSoundingProcedureData.reflectorSubeventResultData[i].stepData = new CsStepData[steps];

    for (int64_t j=0;j<steps;j++) {
      vector<StepData> stepData = reflectorSubeventResultData[i].stepData;
      ModeType mode = stepData[j].stepMode;

      channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepChannel = stepData[j].stepChannel;
      channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepMode = (int8_t)mode;

      if (mode == ModeType::ZERO) {
        CsModeZeroData modeZeroData;
        modeZeroData.packetQuality = stepData[j].stepModeData.get<ModeData::modeZeroData>().packetQuality;
        modeZeroData.packetRssiDbm = stepData[j].stepModeData.get<ModeData::modeZeroData>().packetRssiDbm;
        modeZeroData.packetAntenna = stepData[j].stepModeData.get<ModeData::modeZeroData>().packetAntenna;
        modeZeroData.initiatorMeasuredFreqOffset = stepData[j].stepModeData.get<ModeData::modeZeroData>().initiatorMeasuredFreqOffset;
        channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepModeData.modeZeroData = modeZeroData;
      } else if (mode == ModeType::ONE) {
        CsModeOneData modeOneData;
        modeOneData.packetQuality = stepData[j].stepModeData.get<ModeData::modeOneData>().packetQuality;
        modeOneData.packetNadm = (int8_t)stepData[j].stepModeData.get<ModeData::modeOneData>().packetNadm;
        modeOneData.packetRssiDbm = stepData[j].stepModeData.get<ModeData::modeOneData>().packetRssiDbm;
        modeOneData.rttToaTodData.todToaReflector = stepData[j].stepModeData.get<ModeData::modeOneData>().rttToaTodData.get<RttToaTodData::todToaReflector>();
        modeOneData.packetAntenna = stepData[j].stepModeData.get<ModeData::modeOneData>().packetAntenna;
        modeOneData.packetPct1.iSample = stepData[j].stepModeData.get<ModeData::modeOneData>().packetPct1->iSample;
        modeOneData.packetPct1.qSample = stepData[j].stepModeData.get<ModeData::modeOneData>().packetPct1->qSample;
        modeOneData.packetPct2.iSample = stepData[j].stepModeData.get<ModeData::modeOneData>().packetPct2->iSample;
        modeOneData.packetPct2.qSample = stepData[j].stepModeData.get<ModeData::modeOneData>().packetPct2->qSample;
        channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepModeData.modeOneData = modeOneData;
      } else if (mode == ModeType::TWO) {
        CsModeTwoData modeTwoData;
        int32_t tonePctIQSampleSize = stepData[j].stepModeData.get<ModeData::modeTwoData>().tonePctIQSamples.size();
        int32_t toneQualityIndicatorsSize = stepData[j].stepModeData.get<ModeData::modeTwoData>().toneQualityIndicators.size();
        modeTwoData.antennaPermutationIndex = stepData[j].stepModeData.get<ModeData::modeTwoData>().antennaPermutationIndex;
        modeTwoData.tonePctIQSampleSize = tonePctIQSampleSize;
        modeTwoData.toneQualityIndicatorsSize = toneQualityIndicatorsSize;

        modeTwoData.tonePctIQSamples = new CsPctIQSample[tonePctIQSampleSize];
        modeTwoData.toneQualityIndicators = new uint8_t[toneQualityIndicatorsSize];
        for (int32_t k=0;k<tonePctIQSampleSize;k++) {
          modeTwoData.tonePctIQSamples[k].iSample = stepData[j].stepModeData.get<ModeData::modeTwoData>().tonePctIQSamples[k].iSample;
          modeTwoData.tonePctIQSamples[k].qSample = stepData[j].stepModeData.get<ModeData::modeTwoData>().tonePctIQSamples[k].qSample;
        }
        for (int32_t k=0;k<toneQualityIndicatorsSize;k++) {
          modeTwoData.toneQualityIndicators[k] = (uint8_t)stepData[j].stepModeData.get<ModeData::modeTwoData>().toneQualityIndicators[k];
        }

        channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepModeData.modeTwoData = modeTwoData;
      } else if (mode == ModeType::THREE) {
        /*Mode one data for mode 3*/
        CsModeOneData modeOneData;
        modeOneData.packetQuality = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetQuality;
        modeOneData.packetNadm = (int8_t)stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetNadm;
        modeOneData.packetRssiDbm = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetRssiDbm;
        modeOneData.rttToaTodData.todToaReflector = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.rttToaTodData.get<RttToaTodData::todToaReflector>();
        modeOneData.packetAntenna = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetAntenna;
        modeOneData.packetPct1.iSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetPct1->iSample;
        modeOneData.packetPct1.qSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetPct1->qSample;
        modeOneData.packetPct2.iSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetPct2->iSample;
        modeOneData.packetPct2.qSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeOneData.packetPct2->qSample;
        /*Mode two data for mode 3*/
        CsModeTwoData modeTwoData;
        int32_t tonePctIQSampleSize = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.tonePctIQSamples.size();
        int32_t toneQualityIndicatorsSize = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.toneQualityIndicators.size();
        modeTwoData.antennaPermutationIndex = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.antennaPermutationIndex;
        modeTwoData.tonePctIQSampleSize = tonePctIQSampleSize;
        modeTwoData.toneQualityIndicatorsSize = toneQualityIndicatorsSize;
        modeTwoData.tonePctIQSamples = new CsPctIQSample[tonePctIQSampleSize];
        modeTwoData.toneQualityIndicators = new uint8_t[toneQualityIndicatorsSize];
        for (int32_t k=0;k<tonePctIQSampleSize;k++) {
          modeTwoData.tonePctIQSamples[k].iSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.tonePctIQSamples[k].iSample;
          modeTwoData.tonePctIQSamples[k].qSample = stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.tonePctIQSamples[k].qSample;
        }
        for (int32_t k=0;k<toneQualityIndicatorsSize;k++) {
          modeTwoData.toneQualityIndicators[k] = (uint8_t)stepData[j].stepModeData.get<ModeData::modeThreeData>().modeTwoData.toneQualityIndicators[k];
        }
        channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepModeData.modeThreeData.modeOneData = modeOneData;
        channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepModeData.modeThreeData.modeTwoData = modeTwoData;
      }
    }
  }

  bcs_algo->csIfAddMeasurementV2(&channelSoundingProcedureData);
  /* Free allocated algo memory */
  /* Free Initiator subevent result memory*/

  for (int i=0;i<initiatorSubeventResultDataSize;i++) {
    for (int j=0;j<in_procedureData.initiatorSubeventResultData[i].stepData.size();j++) {
      if (in_procedureData.initiatorSubeventResultData[i].stepData[j].stepMode == ModeType::TWO) {
         delete channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepModeData.modeTwoData.tonePctIQSamples;
         delete channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepModeData.modeTwoData.toneQualityIndicators;
      } else if (in_procedureData.initiatorSubeventResultData[i].stepData[j].stepMode == ModeType::THREE) {
        delete channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepModeData.modeThreeData.modeTwoData.tonePctIQSamples;
        delete channelSoundingProcedureData.initiatorSubeventResultData[i].stepData[j].stepModeData.modeThreeData.modeTwoData.toneQualityIndicators;
      }
    }
    delete channelSoundingProcedureData.initiatorSubeventResultData[i].stepData;
  }
  delete channelSoundingProcedureData.initiatorSubeventResultData;

  /*Free reflector subevent result memory*/

  for (int i=0;i<reflectorSubeventResultDataSize;i++) {
    for (int j=0;j<in_procedureData.reflectorSubeventResultData[i].stepData.size();j++) {
      if (in_procedureData.reflectorSubeventResultData[i].stepData[j].stepMode == ModeType::TWO) {
         delete channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepModeData.modeTwoData.tonePctIQSamples;
         delete channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepModeData.modeTwoData.toneQualityIndicators;
      } else if (in_procedureData.reflectorSubeventResultData[i].stepData[j].stepMode == ModeType::THREE) {
        delete channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepModeData.modeThreeData.modeTwoData.tonePctIQSamples;
        delete channelSoundingProcedureData.reflectorSubeventResultData[i].stepData[j].stepModeData.modeThreeData.modeTwoData.toneQualityIndicators;
      }
    }
    delete channelSoundingProcedureData.reflectorSubeventResultData[i].stepData;
  }
  delete channelSoundingProcedureData.reflectorSubeventResultData;


  return ::ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothChannelSoundingSession::updateChannelSoundingConfig(
   const Config& in_config) {

  channelSoundingProcedureData.csConfigParam.modeType = (int8_t)in_config.modeType;
  channelSoundingProcedureData.csConfigParam.subModeType = (int8_t)in_config.subModeType;
  channelSoundingProcedureData.csConfigParam.rttType = (int32_t)in_config.rttType;
  for (int i=0;i<10;i++) {
    channelSoundingProcedureData.csConfigParam.channelMap[i] = in_config.channelMap[i];
  }
  channelSoundingProcedureData.csConfigParam.minMainModeSteps = in_config.minMainModeSteps;
  channelSoundingProcedureData.csConfigParam.maxMainModeSteps = in_config.maxMainModeSteps;
  channelSoundingProcedureData.csConfigParam.mainModeRepetition = in_config.mainModeRepetition;
  channelSoundingProcedureData.csConfigParam.mode0Steps = in_config.mode0Steps;
  channelSoundingProcedureData.csConfigParam.role = (int32_t)in_config.role;
  channelSoundingProcedureData.csConfigParam.csSyncPhyType = (int8_t)in_config.csSyncPhyType;
  channelSoundingProcedureData.csConfigParam.channelSelectionType = (int8_t)in_config.channelSelectionType;
  channelSoundingProcedureData.csConfigParam.ch3cShapeType = (int8_t)in_config.ch3cShapeType;
  channelSoundingProcedureData.csConfigParam.ch3cJump = (int8_t)in_config.ch3cJump;
  channelSoundingProcedureData.csConfigParam.channelMapRepetition = (int8_t)in_config.channelMapRepetition;
  channelSoundingProcedureData.csConfigParam.tIp1TimeUs = in_config.tIp1TimeUs;
  channelSoundingProcedureData.csConfigParam.tIp2TimeUs = in_config.tIp2TimeUs;
  channelSoundingProcedureData.csConfigParam.tFcsTimeUs = in_config.tFcsTimeUs;
  channelSoundingProcedureData.csConfigParam.tPmTimeUs = in_config.tPmTimeUs;
  channelSoundingProcedureData.csConfigParam.tSwTimeUsSupportedByLocal = in_config.tSwTimeUsSupportedByLocal;
  channelSoundingProcedureData.csConfigParam.tSwTimeUsSupportedByRemote = in_config.tSwTimeUsSupportedByRemote;
  channelSoundingProcedureData.csConfigParam.bleConnInterval = in_config.bleConnInterval;

  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::updateProcedureEnableConfig(
   const ProcedureEnableConfig& in_procedureEnableConfig) {
  channelSoundingProcedureData.procedureEnableConfig.toneAntennaConfigSelection = in_procedureEnableConfig.toneAntennaConfigSelection;
  channelSoundingProcedureData.procedureEnableConfig.subeventLenUs = in_procedureEnableConfig.subeventLenUs;
  channelSoundingProcedureData.procedureEnableConfig.subeventsPerEvent = in_procedureEnableConfig.subeventsPerEvent;
  channelSoundingProcedureData.procedureEnableConfig.subeventInterval = in_procedureEnableConfig.subeventInterval;
  channelSoundingProcedureData.procedureEnableConfig.eventInterval = in_procedureEnableConfig.eventInterval;
  channelSoundingProcedureData.procedureEnableConfig.procedureInterval = in_procedureEnableConfig.procedureInterval;
  channelSoundingProcedureData.procedureEnableConfig.procedureCount = in_procedureEnableConfig.procedureCount;
  channelSoundingProcedureData.procedureEnableConfig.maxProcedureLen = in_procedureEnableConfig.maxProcedureLen;
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::updateBleConnInterval(
  int /*in_bleConnInterval*/) {
  return ::ndk::ScopedAStatus::ok();
}
#endif
}  // namespace aidl::android::hardware::bluetooth::ranging::impl
