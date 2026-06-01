/******************************************************************************
 Copyright (c) 2005-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef	__LONGTIMER_PRIVATE_H__
#define	__LONGTIMER_PRIVATE_H__

 
/* BCHS_EXPORT_POINT_START */

#include "optim.h"
#include "qbl_adapter_timer.h"
#include "longtimer.h"

typedef optim_uint64 longtimer;

/****************************************************************************
NAME
	get_long_milli_time  -  get a 64 bit millisecond time

FUNCTION
	Fills in the current millisecond time in the 64 bit integer pointed
	to by t.
*/

void get_long_milli_time(longtimer *t);

/* BCHS_EXPORT_POINT_END */

#endif	/* __LONGTIMER_PRIVATE_H__ */
