/******************************************************************************
 Copyright (c) 2005-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

/* BCHS_EXPORT_POINT_START */

#include "longtimer_private.h"

/****************************************************************************
NAME
	get_second_time  -  get a 32 bit second time
*/
SECONDTIME get_second_time(void)
{
    longtimer t;

    get_long_milli_time(&t);
    (void) udiv6416(&t, 1000U, &t);

    return (SECONDTIME) t.lsw;
}

/* BCHS_EXPORT_POINT_END */
