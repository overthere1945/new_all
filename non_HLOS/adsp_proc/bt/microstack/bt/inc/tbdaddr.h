/*!
        Copyright (C) 2010 - 2023 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file

\brief  Advertise the services of the TYPED_BD_ADDR_T library
*/

#ifndef __BREDRLE_ADDR_H__
#define __BREDRLE_ADDR_H__

#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)

#ifdef __cplusplus
extern "C" {
#endif

/* Random address type (GAP) definitions */
#define TBDADDR_RANDOM_NAP_TYPE_MASK ((uint16_t)0xC000)
#define TBDADDR_RANDOM_NAP_TYPE_STATIC ((uint16_t)0xC000)
#define TBDADDR_RANDOM_NAP_TYPE_PRIVATE_NONRESOLVABLE ((uint16_t)0x0000)
#define TBDADDR_RANDOM_NAP_TYPE_PRIVATE_RESOLVABLE ((uint16_t)0x4000)

/* Read from TYPED_BD_ADDR_T */
#define TBDADDR_TYPE(addrt) ((addrt).type)
#define TBDADDR_ADDR(addrt) ((addrt).addr)
#define TBDADDR_LAP(addrt) (TBDADDR_ADDR(addrt).lap)
#define TBDADDR_UAP(addrt) (TBDADDR_ADDR(addrt).uap)
#define TBDADDR_NAP(addrt) (TBDADDR_ADDR(addrt).nap)

/* Read from TP_BD_ADDR_T */
#define TPBDADDR_ADDRT(tp_addrt) ((tp_addrt).addrt)
#define TPBDADDR_TRANSPORT(tp_addrt) ((tp_addrt).tp_type)
#define TPBDADDR_ADDR(tp_addrt) (TPBDADDR_ADDRT(tp_addrt).addr)
#define TPBDADDR_TYPE(tp_addrt) (TPBDADDR_ADDRT(tp_addrt).type)
#define TPBDADDR_LAP(tp_addrt) (TPBDADDR_ADDR(tp_addrt).lap)
#define TPBDADDR_UAP(tp_addrt) (TPBDADDR_ADDR(tp_addrt).uap)
#define TPBDADDR_NAP(tp_addrt) (TPBDADDR_ADDR(tp_addrt).nap)

/* Write to TYPED_BD_ADDR_T */
#define TBDADDR_INVALIDATE(addrt) (TBDADDR_TYPE(addrt) = TBDADDR_INVALID)


/* Set TYPED_BD_ADDR_T type as Random */
#define TBDADDR_SET_TYPE_RANDOM(addrt) (TBDADDR_TYPE(addrt) = TBDADDR_RANDOM)

/* Test TYPED_BD_ADDR_T */
#define TBDADDR_IS_PUBLIC(addrt) (TBDADDR_TYPE(addrt) == TBDADDR_PUBLIC)
#define TBDADDR_IS_RANDOM(addrt) (TBDADDR_TYPE(addrt) == TBDADDR_RANDOM)
#define TBDADDR_IS_STATIC(addrt) \
        tbdaddr_is_static(&(addrt))
#define TBDADDR_IS_PRIVATE_RESOLVABLE(addrt) \
        tbdaddr_is_private_resolvable(&(addrt))
#define TBDADDR_IS_PRIVATE_NONRESOLVABLE(addrt) \
        tbdaddr_is_private_nonresolvable(&(addrt))
#define TBDADDR_IS_VALID(addrt) tbdaddr_is_valid(&(addrt))
#define TBDADDR_IS_PRIVATE(addrt) (TBDADDR_IS_PRIVATE_RESOLVABLE(addrt) \
        || TBDADDR_IS_PRIVATE_NONRESOLVABLE(addrt))
#define TBADDR_TYPE_IS_INVALID(addrt) (TBDADDR_TYPE(addrt) == TBDADDR_INVALID)

/* Copy functions */
extern void tbdaddr_copy(
        TYPED_BD_ADDR_T *dst,
        const TYPED_BD_ADDR_T *src);

extern void tbdaddr_copy_from_bd_addr(
        TYPED_BD_ADDR_T *dst,
        uint8_t src_type,
        const BD_ADDR_T *src);

extern void tbdaddr_copy_from_public_bd_addr(
        TYPED_BD_ADDR_T *dst,
        const BD_ADDR_T *src);

extern uint8_t tbdaddr_copy_to_bd_addr(
        BD_ADDR_T *dst,
        const TYPED_BD_ADDR_T *src);

/* Equality functions */
extern bool_t tbdaddr_eq(
        const TYPED_BD_ADDR_T *a,
        const TYPED_BD_ADDR_T *b);

extern bool_t tpbdaddr_eq(
        const TP_BD_ADDR_T *a,
        const TP_BD_ADDR_T *b);

extern bool_t tbdaddr_eq_bd_addr(
        const TYPED_BD_ADDR_T *addrt,
        uint8_t bd_addr_type,
        const BD_ADDR_T *bd_addr);

/* Testing functions */
extern bool_t tbdaddr_is_static(
        const TYPED_BD_ADDR_T *addrt);

extern bool_t tbdaddr_is_private_resolvable(
        const TYPED_BD_ADDR_T *addrt);

extern bool_t tbdaddr_is_private_nonresolvable(
        const TYPED_BD_ADDR_T *addrt);

extern bool_t tbdaddr_is_valid(
        const TYPED_BD_ADDR_T *addrt);

#ifdef __cplusplus
}
#endif

#endif  /* __BREDRLE_ADDR_H__ */
