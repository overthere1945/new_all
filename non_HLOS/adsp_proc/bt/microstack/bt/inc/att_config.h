/*******************************************************************************

Copyright (C) 2019 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifndef _ATT_CONFIG_H_
#define _ATT_CONFIG_H_


#ifdef __cplusplus
extern "C" {
#endif

/*
 * These configuration paramters are defined to a basic ATT requirement and
 * Build for Host configurations can use a suitable number through their
 * configurations.
 */

/* Credits related configuration for downstream flood defence */
/*----------------------------------------------------------------------------*/
/* MTU less than or equal to ATT_CREDIT_BAND_SMALL is a small band MTU.
 * MTU greater than ATT_CREDIT_BAND_SMALL is considered as large band.*/
#ifndef ATT_CREDIT_BAND_SMALL
#define ATT_CREDIT_BAND_SMALL           48
#endif

/*
 * ATT should not be writing cmds and notifications indescriminately and 
 * hence shall be restricted to a defined limit. The defines below
 * help achieve a restricted writing of l2cap data.
 * Maximum number of avilable L2CAP data write credits that 
 * ATT can have on all ATT links with a given MTU BAND
 * This is mainly for the flood defense, applicable to ATT notifications
 * and ATT commands PDU
 * NOTE : Platform can override these with appropriate values based on
 * platform memory and usecase requirement.
 */
#ifndef ATT_MAX_CREDIT_FOR_SMALL_BAND_MTU
/* Here 12 (configurable) is the max no ATT cmd/notification PDUs for small band 
 * prim path, and 
 * 3(ATT_NUM_CREDITS_PER_PRIM) is the no of ATT credits per prim */
#define ATT_MAX_CREDIT_FOR_SMALL_BAND_MTU           (12 * 3)
#endif

#ifndef ATT_MAX_CREDIT_FOR_LARGE_BAND_MTU
#define ATT_MAX_CREDIT_FOR_LARGE_BAND_MTU           (3 * 3)
#endif


#ifdef __cplusplus
}
#endif 


#endif

