/*******************************************************************************
Copyright (C) 2009 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef _RFCOMM_CONFIG_H_
#define _RFCOMM_CONFIG_H_

#include "qbl_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* RFCOMM timers
   RFC_CREDIT_RETURN_TIMER_VALUE :- Timeout after which any outstanding credits on
                                    a particular DLC will automatically be
                                    returned to the peer device
 
   WARNING: Altering the values of these timeouts could seriously compromise the
            functionality of RFCOMM!!
*/

#ifndef RFC_CREDIT_RETURN_TIMER_VALUE
#define RFC_CREDIT_RETURN_TIMER_VALUE         (500 *MILLISECOND)  /* 0.5 seconds */
#endif

/* This is the fraction of the number of allocated rx credits that must have
   been accumulated before automatically triggering a credit only data frame
   to be sent to the peer.
*/
#ifndef RFC_CREDIT_SEND_THRESHOLD
#define RFC_CREDIT_SEND_THRESHOLD  4     /* 1/4 of the total allocated */
#endif


#ifdef __cplusplus
}
#endif

#endif /* _RFCOMM_CONFIG_H_ */
