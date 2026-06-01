#ifndef CSR_RESULT_H__
#define CSR_RESULT_H__
/*****************************************************************************
 Copyright (c) 2009-2018, The Linux Foundation.
 All rights reserved.
*****************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef CsrUint16 CsrResult;
#define CSR_RESULT_SUCCESS  ((CsrResult) 0x0000)
#define CSR_RESULT_FAILURE  ((CsrResult) 0xFFFF)

#ifdef __cplusplus
}
#endif

#endif
