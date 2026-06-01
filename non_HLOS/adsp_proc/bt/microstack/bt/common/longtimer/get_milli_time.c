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
	get_milli_time  -  get a 32 bit millisecond time
*/
MILLITIME get_milli_time(void)
{
    longtimer t;

    get_long_milli_time(&t);
    return (MILLITIME) t.lsw;
}

/* BCHS_EXPORT_POINT_END */
