#ifndef CSR_BT_BLUESTACK_CONFIG_H_
#define CSR_BT_BLUESTACK_CONFIG_H_
/******************************************************************************
 Copyright (c) 2017-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef HYDRA

/* This header file is used to override Bluestack configuration parameters which
 * are required to be calibrated for Synergy but not exposed to application developers */

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configurable timeout multiplier to keep the idle link alive post pairing */
#ifdef DM_CONFIG_ACL_IDLE_TIMEOUT_LE_PAIRING
#undef DM_CONFIG_ACL_IDLE_TIMEOUT_LE_PAIRING
#endif
#define DM_CONFIG_ACL_IDLE_TIMEOUT_LE_PAIRING       0   /* Do not delay LE ACL disconnection after pairing */

/* Configurable timeout for disconnection of ACL after LE fixed(for now) channel */
#ifdef DM_CONFIG_ACL_IDLE_TIMEOUT_LE_TINY
#undef DM_CONFIG_ACL_IDLE_TIMEOUT_LE_TINY
#endif
#define DM_CONFIG_ACL_IDLE_TIMEOUT_LE_TINY          0   /* Do not delay LE ACL disconnection when
                                                           there are no stakeholders of the link */
/*---------------------------------------------------------------------------
 * Defines for ATT
 *---------------------------------------------------------------------------*/

/* ATT Credits configuration for downstream */

/* ATT maximum credits for small band MTU */
#ifdef ATT_MAX_CREDIT_FOR_SMALL_BAND_MTU
#undef ATT_MAX_CREDIT_FOR_SMALL_BAND_MTU
#endif

#ifdef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
#define ATT_MAX_CREDIT_FOR_SMALL_BAND_MTU   (16 * 3)
#else
#define ATT_MAX_CREDIT_FOR_SMALL_BAND_MTU   (0x7FFF)
#endif

/* ATT maximum credits for large band MTU */
#ifdef ATT_MAX_CREDIT_FOR_LARGE_BAND_MTU
#undef ATT_MAX_CREDIT_FOR_LARGE_BAND_MTU
#endif
#ifdef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
#define ATT_MAX_CREDIT_FOR_LARGE_BAND_MTU   (16*3)
#else
#define ATT_MAX_CREDIT_FOR_LARGE_BAND_MTU   (0x7FFF)
#endif

#include "dm_config.h"
#include "att_config.h"

#ifdef __cplusplus
}
#endif

#endif /* !HYDRA */

#endif /* BLUESTACK_CONFIG */
