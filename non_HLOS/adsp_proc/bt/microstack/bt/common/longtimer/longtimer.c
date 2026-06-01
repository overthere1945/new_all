/******************************************************************************
 Copyright (c) 2005-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

/* BCHS_EXPORT_POINT_START */
  
#include "longtimer_private.h"

static bool init; /* = FALSE */
static longtimer timer;

#define WAKEUP_PERIOD (10 * MINUTE)

/****************************************************************************
NAME
	update_long_timer  -  update the long timer to the current time
*/

static void update_long_timer(void)
{
    uint32 t;

    t = (uint32) get_time();
    if ((uint32) t < (uint32) timer.lsw) /* Note: unsigned comparison *not* time_lt() */
	timer.msw++;
    timer.lsw = t;
}

/****************************************************************************
NAME
	update_event  - timed event to check for clock wrap
*/

/* ARGSUSED */
static void update_event(uint16 m, void *p)
{
    QBL_UNUSED(m);
    QBL_UNUSED(p);

    update_long_timer();
    timed_event_in(WAKEUP_PERIOD,update_event,0,NULL);
}

/****************************************************************************
NAME
	get_long_milli_time  -  get a 64 bit millisecond time
*/

void get_long_milli_time(longtimer *t)
{
    if (!init)
    {
        /* initialisation.
           this is nice as we only start the timed event
           if someone is actually using longtimer! */
        init = TRUE;
        timer.lsw = get_time();
        timed_event_in(WAKEUP_PERIOD,update_event,0,NULL);
    }

    update_long_timer();
    (void) udiv6416(t, (uint16) MILLISECOND, &timer);
}

/* BCHS_EXPORT_POINT_END */
