#ifndef QBL_ADAPTER_MEMORY_H__
#define QBL_ADAPTER_MEMORY_H__
/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_pmem.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HYDRA) || defined(CAA)

#include "pmalloc.h"
#include "string.h"

#else
#define memset(a,b,c) CsrMemSet(a,b,c)
#define memcpy_unpack CsrBtMemCpyUnpack
#define memcpy_pack CsrBtMemCpyPack

#define zpmalloc(s) CsrPmemZalloc(s)
#define pmalloc CsrPmemAlloc
/*#define pfree(s) CsrPmemFree(s)*/

#define psizeof sizeof

#endif

#ifdef __cplusplus
}
#endif

#endif


