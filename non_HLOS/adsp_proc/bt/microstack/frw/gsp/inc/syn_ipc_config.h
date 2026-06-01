/******************************************************************************
Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"

/* This percent of buffer when full will cause extension api to be invoked
   Range 0 - 100.
*/
#ifndef SYN_IPC_MAX_SPACE_PERCENT
#define SYN_IPC_MAX_SPACE_PERCENT           75
#endif

#ifndef SYN_IPC_MEM_BUFFER_SIZE
#define SYN_IPC_MEM_BUFFER_SIZE             25000
#endif

#ifndef SYN_IPC_MEM_DATA_POOL_SIZE
#define SYN_IPC_MEM_DATA_POOL_SIZE          500
#endif

#ifndef SYN_IPC_DEBUG_ADD_RX_WATER_MARK
#define SYN_IPC_DEBUG_ADD_RX_WATER_MARK
#endif

#ifdef SYN_IPC_DEBUG_LOG_TX_DATA
#define SYN_IPC_DEBUG_LOG_TX_DATA
#endif

#ifdef SYN_IPC_DEBUG_LOG_RX_DATA
#define SYN_IPC_DEBUG_LOG_RX_DATA
#endif

#if !defined (SYN_IPC_DEBUG_LOG_TX_DATA) && !defined (SYN_IPC_DEBUG_LOG_RX_DATA)
#undef SYN_IPC_DEBUG_LOG_DATA
#else
#define SYN_IPC_DEBUG_LOG_DATA
#define SYN_IPC_DEBUG_MEM_BUFFER_SIZE       10000
#endif

#ifdef SYN_IPC_STUB
#define SYN_IPC_STUB
#endif