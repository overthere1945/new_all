/**
 * Copyright (c) 2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

/* Copyright (c) 2014, 2018 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <utils/Log.h>
#include <errno.h>

#include "xpancommand.h"
#include "qca-vendor_oem.h"

WifihalGeneric::WifihalGeneric(wifi_handle handle, int id, u32 vendor_id,
			       u32 subcmd)
		: WifiVendorCommand(handle, id, vendor_id, subcmd)
{
	memset(&mCallback, 0, sizeof(wifi_callbacks));
	freq_list = NULL;
	max_freq_list_size = 0;
	freq_list_size = NULL;
}

wifi_error WifihalGeneric::requestResponse()
{
	return WifiCommand::requestResponse(mMsg);
}

void WifihalGeneric::setWifiCallbackHandler(wifi_callbacks callback)
{
	mCallback = callback;
}

void WifihalGeneric::setCookiePointer(uint64_t *cookie_ptr)
{
	cookie = cookie_ptr;
}

void WifihalGeneric::setCookiePointer16(uint16_t *cookie_ptr16)
{
	cookie_u16 = cookie_ptr16;
}

void WifihalGeneric::setChanSwitchTsfPointer(uint64_t *tsf_ptr)
{
	channelSwitchTsf = tsf_ptr;
}

void WifihalGeneric::setXpanFeaturePtr(bool *enable)
{
	xpan_supported = enable;
}

void WifihalGeneric::setXpanUsableFreqs(int *freqs, int max_size, int *size)
{
	freq_list = freqs;
	max_freq_list_size = max_size;
	freq_list_size = size;
}

int WifihalGeneric::handleResponse(WifiEvent &reply)
{
	ALOGV("Got a Wi-Fi HAL module message from Driver");
	int i = 0;
	WifiVendorCommand::handleResponse(reply);

	// Parse the vendordata and get the attribute
	switch(mSubcmd)
	{
	case QCA_NL80211_VENDOR_SUBCMD_DO_ACS:
		{
			int freq = 0;
			if(mCallback.cb_wifi_acs_results) {
				(mCallback.cb_wifi_acs_results)(freq,ACS_STARTED);
				ALOGI("ACS Started");
			} else {
				ALOGI("callback not present");
			}

			break;
		}
	case QCA_NL80211_VENDOR_SUBCMD_DOZED_AP:
		{
			struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_DOZED_AP_MAX+ 1];
			nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_DOZED_AP_MAX,
				  (struct nlattr *)mVendorData,
				  mDataLen, NULL);
			if (!tb_vendor[QCA_WLAN_VENDOR_ATTR_DOZED_AP_COOKIE])
				return NL_SKIP;

			*cookie = nla_get_u64(tb_vendor[QCA_WLAN_VENDOR_ATTR_DOZED_AP_COOKIE]);

			break;

		}
	case QCA_NL80211_VENDOR_SUBCMD_HIGH_AP_AVAILABILITY:
		{
			struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_HIGH_AP_AVAILABILITY_MAX + 1];
			nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_HIGH_AP_AVAILABILITY_MAX,
				  (struct nlattr *)mVendorData,
				  mDataLen, NULL);

			if (!tb_vendor[QCA_WLAN_VENDOR_ATTR_HIGH_AP_AVAILABILITY_COOKIE])
				return NL_SKIP;

			*cookie_u16 = nla_get_u16(
					tb_vendor[QCA_WLAN_VENDOR_ATTR_HIGH_AP_AVAILABILITY_COOKIE]);

			break;
		}
	case QCA_NL80211_VENDOR_SUBCMD_TSF:
		{
			struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_TSF_MAX + 1];
			if (nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_TSF_MAX,
				  (struct nlattr *)mVendorData, mDataLen, NULL)) {
				ALOGE("%s: TSF response: nla_parse failed", __func__);
				return NL_SKIP;
			}

			if (tb_vendor[QCA_WLAN_VENDOR_ATTR_TSF_TIMER_VALUE]) {
				*channelSwitchTsf =
					nla_get_u64(tb_vendor[QCA_WLAN_VENDOR_ATTR_TSF_TIMER_VALUE]);
			} else {
				ALOGE("%s: TSF response with value is missing", __func__);
				return NL_SKIP;
			}

			break;
		}
	case QCA_NL80211_VENDOR_SUBCMD_GET_FEATURES:
		{
			struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_MAX + 1];
			struct nlattr *attr;
			u8 *flags;
			enum qca_wlan_vendor_features feature;
			int len, idx;

			nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_MAX,
				  (struct nlattr *)mVendorData, mDataLen, NULL);

			if (!tb_vendor[QCA_WLAN_VENDOR_ATTR_FEATURE_FLAGS])
				return NL_SKIP;

			attr = tb_vendor[QCA_WLAN_VENDOR_ATTR_FEATURE_FLAGS];
			feature = QCA_WLAN_VENDOR_FEATURE_ENHANCED_AUDIO_EXPERIENCE_OVER_WLAN;

			idx = feature/8;
			len = nla_len(attr);
			flags = (u8 *)malloc(len);
			if (!flags) {
				ALOGE("Could not allocate flags");
				return NL_SKIP;
			}
			memcpy(flags, nla_data(attr), len);
			*xpan_supported = (idx < len) && (flags[idx] & BIT(feature % 8));
			free(flags);

			break;
		}
	case QCA_NL80211_VENDOR_SUBCMD_USABLE_CHANNELS:
		{
			struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_USABLE_CHANNELS_MAX + 1];
			struct nlattr *attr;
			int rem, size = 0;

			if (nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_USABLE_CHANNELS_MAX,
				  (struct nlattr *)mVendorData, mDataLen, NULL)) {
				ALOGE("NL80211_ATTR_VENDOR_DATA parse error");
				return NL_SKIP;
			}

			if (!tb_vendor[QCA_WLAN_VENDOR_ATTR_USABLE_CHANNELS_CHAN_INFO]) {
				ALOGE("Usable channel info not found");
				return NL_SKIP;
			}

			for_each_nested_attribute(
					attr,
					tb_vendor[QCA_WLAN_VENDOR_ATTR_USABLE_CHANNELS_CHAN_INFO],
					rem) {
				struct nlattr *ch_info[QCA_WLAN_VENDOR_ATTR_CHAN_INFO_MAX + 1];

				if (size >= max_freq_list_size) {
					ALOGE("Got max channels %d completed", max_freq_list_size);
					break;
				}
				if (nla_parse_nested(ch_info, QCA_WLAN_VENDOR_ATTR_CHAN_INFO_MAX,
						attr, NULL)) {
					ALOGE("Failed to get usable channel info");
					return NL_SKIP;
				}

				if (!ch_info[QCA_WLAN_VENDOR_ATTR_CHAN_INFO_PRIMARY_FREQ]) {
					ALOGE("CHAN_INFO_PRIMARY_FREQ not found");
					return NL_SKIP;
				}
				freq_list[size] = nla_get_u32(ch_info[
							QCA_WLAN_VENDOR_ATTR_CHAN_INFO_PRIMARY_FREQ]);
				size++;
			}
			*freq_list_size = size;
			break;
		}
	default :
		ALOGE("%s: Wrong Wi-Fi HAL event received %d", __func__, mSubcmd);
	}
	return NL_SKIP;
}

WiFiConfigCommand::WiFiConfigCommand(wifi_handle handle,
				     int id, u32 vendor_id,
				     u32 subcmd)
	: WifiVendorCommand(handle, id, vendor_id, subcmd)
{

}

/* This function implements creation of Vendor command */
wifi_error WiFiConfigCommand::create()
{
	wifi_error ret = mMsg.create(NL80211_CMD_VENDOR, 0, 0);
	if (ret != WIFI_SUCCESS)
		return ret;

	/* Insert the oui in the msg */
	ret = mMsg.put_u32(NL80211_ATTR_VENDOR_ID, mVendor_id);
	if (ret != WIFI_SUCCESS)
		return ret;
	/* Insert the subcmd in the msg */
	ret = mMsg.put_u32(NL80211_ATTR_VENDOR_SUBCMD, mSubcmd);

	return ret;
}

/* This function implements creation of generic NL command */
wifi_error WiFiConfigCommand::create_generic(u8 cmdId)
{
	wifi_error ret = mMsg.create(cmdId, 0, 0);
	return ret;
}

/* Callback handlers registered for nl message send */
static int error_handler_wifi_config(struct sockaddr_nl *nla,
				     struct nlmsgerr *err,
				     void *arg)
{
	struct sockaddr_nl *tmp;
	int *ret = (int *)arg;
	tmp = nla;
	*ret = err->error;
	ALOGE("%s: Error code:%d (%s)", __FUNCTION__, *ret, strerror(-(*ret)));

	return NL_STOP;
}

/* Callback handlers registered for nl message send */
static int ack_handler_wifi_config(struct nl_msg *msg, void *arg)
{
	int *ret = (int *)arg;
	struct nl_msg * a;

	a = msg;
	*ret = 0;
	return NL_STOP;
}

/* Callback handlers registered for nl message send */
static int finish_handler_wifi_config(struct nl_msg *msg, void *arg)
{
	int *ret = (int *)arg;
	struct nl_msg * a;

	a = msg;
	*ret = 0;
	return NL_SKIP;
}

wifi_error WiFiConfigCommand::sendCommand()
{
	int status;
	wifi_error res = WIFI_SUCCESS;
	struct nl_cb *cb = NULL;

	pthread_mutex_lock(&mInfo->lock);

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		ALOGE("%s: Callback allocation failed",__FUNCTION__);
		res = WIFI_ERROR_OUT_OF_MEMORY;
		goto out;
	}

	status = nl_send_auto_complete(mInfo->nl_cmd, mMsg.getMessage());
	if (status < 0) {
		res = mapKernelErrortoWifiHalError(status);
		goto out;
	}
	status = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler_wifi_config, &status);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler_wifi_config,
		  &status);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler_wifi_config, &status);

	/* Err is populated as part of finish_handler. */
	while (status > 0) {
		nl_recvmsgs(mInfo->nl_cmd, cb);
	}

	if (status < 0) {
		res = mapKernelErrortoWifiHalError(status);
		goto out;
	}
out:
	nl_cb_put(cb);
	/* Cleanup the mMsg */
	mMsg.destroy();
	pthread_mutex_unlock(&mInfo->lock);
	return res;
}
