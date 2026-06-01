/*******************************************************************************

Copyright (C) 2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief Panic interface.
 */

/**
 * \defgroup QBL_PANIC Panic Interface
 * @{
 */

#ifndef QBL_PANIC_H_
#define QBL_PANIC_H_

#include "qbl_macros.h"
#include "qbl_types.h"
#include "qbl_panic_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define qbl_panic(reason) BLUESTACK_PANIC(reason)

#ifdef __cplusplus
}
#endif

#endif /* QBL_PANIC_H_ */

/**@}*/
