#ifndef QBL_TYPES_H__
#define QBL_TYPES_H__
/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "qbl_adapter_types.h"

typedef int16_t Status;

#define STATUS_SUCCESS                        (0)
#define STATUS_ERROR                         (-1)
#define STATUS_ERROR_TIMEOUT                 (-2)
#define STATUS_ERROR_NO_MEMORY               (-3)
#define STATUS_SCHEDULER_ALREADY_INITIALISED (-4)

typedef struct  {
    uint16 val[3];
} util_uint48;

#endif

