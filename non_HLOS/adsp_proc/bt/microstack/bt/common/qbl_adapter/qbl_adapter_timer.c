/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#include "qbl_adapter_timer.h"

/*! Timed events */
void timer_start(tid *p_id, 
                 TIME delay,
                 void (*fn)(uint16_t, void*),
                 uint16_t fniarg,
                 void *fnvarg)
{
    tid id = timed_event_in(delay, fn, fniarg, fnvarg);

    /* TODO debug only */
    if (p_id == NULL)
        BLUESTACK_PANIC(PANIC_MYSTERY);

    timer_cancel(p_id);
    *p_id = id;
}

void timer_cancel(tid *p_id)
{
    /* TODO debug only */
    if (p_id == NULL)
        BLUESTACK_PANIC(PANIC_MYSTERY);

    if (*p_id != 0)
    {
        cancel_timed_event(*p_id, NULL, NULL);
        *p_id = 0;
    }
}

