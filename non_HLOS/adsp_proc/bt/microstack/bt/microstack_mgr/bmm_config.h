/******************************************************************************
Copyright (c) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/
#ifndef __BMM_CONFIG_H__
#define __BMM_CONFIG_H__

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMM_MAX_SUBSCRIPTIONS
#define BMM_MAX_SUBSCRIPTIONS    10
#endif

#ifndef BMM_LECOC_RX_APP_CREDIT
#define BMM_LECOC_RX_APP_CREDIT   1
#endif

#ifndef BMM_RFCOMM_RX_APP_CREDIT
#define BMM_RFCOMM_RX_APP_CREDIT  1
#endif 

#ifndef BMM_RFCOMM_INIT_CREDITS
#define BMM_RFCOMM_INIT_CREDITS   10
#endif 

#ifndef BMM_LECOC_INIT_CREDITS
#define BMM_LECOC_INIT_CREDITS    20
#endif

#ifndef BMM_LECOC_TX_WINDOW_SIZE
#define BMM_LECOC_TX_WINDOW_SIZE   8
#endif

#ifndef BMM_RFCOMM_TX_WINDOW_SIZE
#define BMM_RFCOMM_TX_WINDOW_SIZE  8
#endif

#ifndef BMM_SOCK_MAX_RFCOMM_CONN
#define BMM_SOCK_MAX_RFCOMM_CONN   2
#endif

#ifndef BMM_SOCK_MAX_LECOC_CONN
#define BMM_SOCK_MAX_LECOC_CONN    2
#endif

#ifndef BMM_SOCK_MAX_RFCOMM_FRAME_SIZE
#define BMM_SOCK_MAX_RFCOMM_FRAME_SIZE   990
#endif

#ifndef BMM_SOCK_MAX_LECOC_MTU
#define BMM_SOCK_MAX_LECOC_MTU   1974
#endif

/* Allows setting of credits to application only once during open socket offload 
*  Subsequent req for setting the credits will be rejected */
#define BMM_ALLOW_CREDITS_REQ_ONLY_DURING_INIT

#endif /* __BMM_CONFIG_H__ */
