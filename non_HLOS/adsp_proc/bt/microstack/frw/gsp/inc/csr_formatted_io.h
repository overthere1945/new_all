#ifndef CSR_FORMATTED_IO_H__
#define CSR_FORMATTED_IO_H__
/*****************************************************************************
 Copyright (c) 2009-2018, The Linux Foundation.
 All rights reserved.
*****************************************************************************/

#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_types.h"

CsrInt32 CsrSnprintf(CsrCharString *dest, CsrSize n, const CsrCharString *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
