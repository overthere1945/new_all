/*******************************************************************************

Copyright (C) 2010 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef _L2CAPLIBPRIVATE_H_
#define _L2CAPLIBPRIVATE_H_

#include <string.h>
#include "l2caplib.h"
#include INC_DIR(common,common.h)
#include INC_DIR(mblk,mblk.h)
#include INC_DIR(tbdaddr,tbdaddr.h)

#include "qbl_adapter_pmalloc.h"

/***********************/




/***********************/

#ifdef __cplusplus
extern "C" {
#endif

/* While any users of L2CAP are not yet MBLK-enabled, we need to wrap
   any incoming payload data pointers into a MBLK to please L2CAP.

   Note: This macro repeats 'data' in its expansion and so should be
   considered unsafe. */
#define L2CA_MblkWrap(data, length) \
    ((length) == 0 ? (data) : mblk_data_create((data), (length), TRUE))

#ifdef __cplusplus
}
#endif

#endif /* _L2CAPLIBPRIVATE_H_ */
