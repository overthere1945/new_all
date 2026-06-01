/******************************************************************************
 Copyright (c) 2020-2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
******************************************************************************/

#include <qurt_timer.h>
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_time.h"
#if (CSR_HOST_PLATFORM == QCC5100_HOST)
#include "qapi_rtc.h"
#endif
#include "csr_log_text_2.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeGet
 *
 *  DESCRIPTION
 *      Returns the current microseconds system time in two 32bit high/low parts.
 *      The microseconds low part is the function return value. The high part is
 *      incremented on low part wraps and returned as output parameter high.
 *
 *  NOTE
 *      A NULL pointer may be provided as high parameter. In this case the
 *      function just returns the low part and leaves the high parameter
 *      unchanged.
 *
 *  RETURNS
 *      CsrTime - the 32 bit low part system time in microseconds.
 *----------------------------------------------------------------------------*/
#if (CSR_HOST_PLATFORM == QCC5100_HOST)
CsrTime CsrTimeGet(CsrTime *high)
{
    CsrUint64 time;
    CsrTime low;

    qurt_time_t ticks = qurt_timer_get_ticks();

    /* Convert the time to micro seconds */
    time = ticks * qurt_timer_convert_ticks_to_time(1, QURT_TIME_MSEC) * 1000;

    if (high != NULL)
    {
        *high = (CsrTime) ((time >> 32) & 0xFFFFFFFF);
    }

    low = (CsrTime) (time & 0xFFFFFFFF);

    return low;
}
#else
CsrTime CsrTimeGet(CsrTime *high)
{
    CsrTime low=0;
    unsigned long long int nticks;
    unsigned long long int usec;

#ifdef MICROSTACK_ENABLE_ISLAND_MODE
    nticks = qurt_sysclock_get_hw_ticks();
#else
    nticks = qurt_timer_get_ticks();
#endif
    usec = QURT_TIMER_TIMETICK_TO_US(nticks);

    if (high != NULL)
    {
        *high = (CsrTime) ((usec >> 32) & 0xFFFFFFFF);
    }

    low = (CsrTime) (usec & 0xFFFFFFFF);

    return low;
}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimeUtcGet
 *
 *  DESCRIPTION
 *      Get the current system wallclock timestamp in UTC.
 *      Specifically, if tod is non-NULL, the contents will be set to the
 *      number of seconds (plus any fraction of a second in milliseconds)
 *      since January 1st 1970.  If low is non-NULL, the contents will be
 *      set to the low 32 bit part of the current system time in microseconds,
 *      as would have been returned by CsrTimeGet(). If high is non-NULL, the
 *      contents will be set to the high 32 bit part of the current system
 *      time, as would be returned in the high output parameter of CsrTimeGet().
 *
 *  NOTE
 *      NULL pointers may be provided for both low and high parameters.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
#if (CSR_HOST_PLATFORM == QCC5100_HOST)
void CsrTimeUtcGet(CsrTimeUtc *tod, CsrTime *low, CsrTime *high)
{
    uint64 ms;
    CsrTime highval = 0;
    CsrTime lowval;
    qapi_Status_t status = QAPI_OK;

    lowval = CsrTimeGet(&highval);

    if (high != NULL)
    {
        *high = highval;
    }

    if (low != NULL)
    {
        *low = lowval;
    }

    if (tod != NULL)
    {
        if((status = qapi_Core_RTC_GPS_Epoch_Get(&ms)) != QAPI_OK)
        {
            tod->sec = 0;
            tod->msec = 0;

            CSR_LOG_TEXT_INFO((BT_FRW, 0, "qapi_Core_RTC_GPS_Epoch_Get failed %x", status));

            return;
        }

        tod->sec = (CsrUint32) ms/1000;
        tod->msec = ms % 1000;
    }
}
#else
void CsrTimeUtcGet(CsrTimeUtc *tod, CsrTime *low, CsrTime *high)
{
    CsrTime highval = 0;
    CsrTime lowval;

    lowval = CsrTimeGet(&highval);

    if (high != NULL)
    {
        *high = highval;
    }

    if (low != NULL)
    {
        *low = lowval;
    }
#if 0
    if (tod != NULL)
    {
        struct timespec now;

        clock_gettime(CLOCK_BOOTTIME, &now);

        tod->sec = (CsrUint32) now.tv_sec;
        tod->msec = now.tv_nsec/1000000;
    }
#endif
}
#endif

