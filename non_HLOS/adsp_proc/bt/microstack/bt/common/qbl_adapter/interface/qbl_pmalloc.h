#ifndef QBL_PMALLOC_H__
#define QBL_PMALLOC_H__
/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#define qbl_zpmalloc(size)  CsrPmemZalloc(size)
#define qbl_pfree CsrPmemFree

#ifdef __cplusplus
}
#endif

#endif

