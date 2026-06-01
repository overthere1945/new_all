/******************************************************************************
 Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef QBL_ADAPTER_TIMER_H__
#define QBL_ADAPTER_TIMER_H__

#include "qbl_adapter_types.h"

/*! Timed events */
extern void timer_start(tid *p_id,
                        TIME delay,
                        void (*fn)(uint16_t, void*),
                        uint16_t fniarg,
                        void *fnvarg);

extern void timer_cancel(tid *p_id);

#endif
