#ifndef QBL_ADAPTER_LOGGING_H__
#define QBL_ADAPTER_LOGGING_H__
/******************************************************************************
 Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HYDRA) && !defined(CAA)

/*
 * Log an Error.
 *
 * Do not abuse this error level. They can not be disabled at run time.
 */
#define L0_DBG_MSG(x)
#define L0_DBG_MSG1(x, a)
#define L0_DBG_MSG2(x, a, b)
#define L0_DBG_MSG3(x, a, b, c)
#define L0_DBG_MSG4(x, a, b, c, d)

/*
 * Log a warning.
 */
#define L1_DBG_MSG(x)
#define L1_DBG_MSG1(x, a)
#define L1_DBG_MSG2(x, a, b)
#define L1_DBG_MSG3(x, a, b, c)
#define L1_DBG_MSG4(x, a, b, c, d)

/*
 * Log a major event.
 */
#define L2_DBG_MSG(x)
#define L2_DBG_MSG1(x, a)
#define L2_DBG_MSG2(x, a, b)
#define L2_DBG_MSG3(x, a, b, c)
#define L2_DBG_MSG4(x, a, b, c, d)

/*
 * Log a minor event.
 */
#define L3_DBG_MSG(x)
#define L3_DBG_MSG1(x, a)
#define L3_DBG_MSG2(x, a, b)
#define L3_DBG_MSG3(x, a, b, c)
#define L3_DBG_MSG4(x, a, b, c, d)

/*
 * Log minor module level event.
 */
#define L4_DBG_MSG(x)
#define L4_DBG_MSG1(x, a)
#define L4_DBG_MSG2(x, a, b)
#define L4_DBG_MSG3(x, a, b, c)
#define L4_DBG_MSG4(x, a, b, c, d)

/*
 * Log minutiae
 */
#define L5_DBG_MSG(x)
#define L5_DBG_MSG1(x, a)
#define L5_DBG_MSG2(x, a, b)
#define L5_DBG_MSG3(x, a, b, c)
#define L5_DBG_MSG4(x, a, b, c, d)

#endif

#ifdef __cplusplus
}
#endif
#endif

