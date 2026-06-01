#ifndef	__LONGTIMER_H__
#define	__LONGTIMER_H__

/******************************************************************************
 Copyright (c) 2005-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

/*******************************************************************************
FILE
        longtimer.h  -  advertise the services of the longtimer library

CONTAINS
    get_milli_time  -  get a 32 bit millisecond time
    get_deci_time  -  get a 32 bit decisecond time
    get_second_time  -  get a 32 bit second time
    millitime_to_time  -  convert a millisecond time into a system time
    decitime_to_time  -  convert a decisecond time into a system time
    secondtime_to_time  -  convert a second time into a system time

DESCRIPTION
    The longtimer library internally holds a 64 bit microsecond time.
    It provides routines to extract 32 bit times where the least
    significant bit corresponds to 1 millisecond, 1 decisecond (100 ms)
    or 1 second.

    All these times are synchronised to the system's microsecond time
    so a routine is not provided to extract a microsecond time. Use
    get_time() instead.

    Routines are also provide to find the system time corresponding to
    a time of the given resolution. Note that these system times are valid
    only within half an hour of the given time.

        Note:  The top 32 bits of the microsecond time will not begin updating
               until after the first call to a longtimer function.
               Typically, a caller will be comparing longtimer times and so
               this should not cause a problem.

MODIFICATION HISTORY
        #1    26:oct:05 sms  B-10024: Created.
        #2    14:feb:07 cph  B-22117: reworked to use timed events
        #3    16:feb:07 cph  B-22117: comment tweak
        ------------------------------------------------------------------
        --- This modification history is now closed. Do not update it. ---
        ------------------------------------------------------------------
********************************************************************************/

/* BCHS_EXPORT_POINT_START */

#include "qbl_adapter_scheduler.h"
#include "common.h"

/*
 * Typedefs for times in the different units. That way, when you put the
 * time into a structure you can give a hint about what the units are.
 */

typedef TIME MILLITIME;
typedef TIME DECITIME;
typedef TIME SECONDTIME;

/****************************************************************************
NAME
	get_milli_time  -  get a 32 bit millisecond time

RETURNS
	A 32 bit millisecond time.
*/

extern MILLITIME get_milli_time(void);


/****************************************************************************
NAME
	get_deci_time  -  get a 32 bit decisecond time

RETURNS
	A 32 bit decisecond (100 ms) time.
*/

extern DECITIME get_deci_time(void);


/****************************************************************************
NAME
	get_second_time  -  get a 32 bit second time

RETURNS
	A 32 bit second time.
*/

extern SECONDTIME get_second_time(void);


/****************************************************************************
NAME
	millitime_to_time  -  convert a millisecond time into a system time

RETURNS
	A 32 bit microsecond time that will be valid within half an hour of
	the time that was passed in.
*/

#define millitime_to_time(mt)  ((TIME) ((mt) * MILLISECOND))


/****************************************************************************
NAME
	decitime_to_time  -  convert a decisecond time into a system time

RETURNS
	A 32 bit microsecond time that will be valid within half an hour of
	the time that was passed in.
*/

#define decitime_to_time(mt)   ((TIME) ((mt) * (100 * MILLISECOND)))


/****************************************************************************
NAME
	secondtime_to_time  -  convert a second time into a system time

RETURNS
	A 32 bit microsecond time that will be valid within half an hour of
	the time that was passed in.
*/

#define secondtime_to_time(mt) ((TIME) ((mt) * SECOND))

/* BCHS_EXPORT_POINT_END */

#endif	/* __LONGTIMER_H__ */
