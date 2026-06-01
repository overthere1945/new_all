/*******************************************************************************

Copyright (C) 2008 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/
#ifndef _CSR_BT_COMMON_H_
#define _CSR_BT_COMMON_H_

#include <stdarg.h>
#include "qbl_adapter_types.h"
#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Timer IDs */
/* TODO, this conditionalisation should become unnecessary after B-62415 */
#if !defined(BUILD_FOR_HOST) || defined(BLUESTACK_HOST_IS_APPS)
typedef uint32_t tid_t;
#endif

/* 
 * Generate internal primitive ids corresponding to the public primitive
 * segmentation defined in interface file (i.e. app/bluestack/qbl_bluetooth.h)
 */
#define GEN_INT_PRIM_ID(prim_id) (((prim_id) << 8) | (prim_id))

#ifdef QBLUESTACK_HOST
#define HciClientEnabled() FALSE
#else
/*! \brief Check for presence of HCI client. Always TRUE for BCHS. */
#ifdef BUILD_FOR_HOST
#ifdef BLUESTACK_HOST_IS_APPS
/* On the Hydra Apps subsystem, the HCI transport is never up when the Apps
 * initialises its message queues.  We wait until the BT full stack service is
 * started before starting Bluestack properly */
#define HciClientEnabled() FALSE
#else /* BLUESTACK_HOST_IS_APPS */
#define HciClientEnabled() TRUE
#endif /* BLUESTACK_HOST_IS_APPS */
#else /* BUILD_FOR_HOST */
#define HciClientEnabled() onchip_hci_client()
#endif /* BUILD_FOR_HOST */
#endif /* QBLUESTACK_HOST */

#ifndef QBLUESTACK_HOST
/*! \brief Use of bitfields */
#ifndef BUILD_FOR_HOST
#define USE_BLUESTACK_BITFIELDS /*!< Always use them when on chip */
#endif

#if !defined(HYDRA) 
#ifdef USE_BLUESTACK_BITFIELDS
#undef BITFIELD
#define BITFIELD(type, name, size)  unsigned int name:size
#else
#define BITFIELD(type, name, size)  type name
#endif
#endif

#define TIMER_EXPIRED(id) ((id) = (tid)0)

/*! \brief Special realloc may be needed for host builds

     On chip we have memory pools and can do clever prealloc magic
     whereas on-host we may need to know the old size for
     reallocations to work properly, hence the "wos" version ("with
     old size")
*/
#if defined (BUILD_FOR_HOST) && !defined(BLUESTACK_HOST_IS_APPS)
extern void* host_xprealloc(void *ptr, size_t oldsize, size_t newsize);
#define bpxprealloc(ptr, oldsize, newsize) \
    host_xprealloc((ptr), (oldsize), (newsize))
#else
#define bpxprealloc(ptr, oldsize, newsize) \
    xprealloc((ptr), (newsize))
#endif
#endif

/* Very basic minimum/maximum macros */
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

/* Absolute macro */
#ifndef ABS
#define ABS(a)   (((a)<0) ? -(a) : (a))
#endif

#if defined(INSTALL_STREAM_MODULE) && defined(INSTALL_RFCOMM_MODULE)
/* Bluestack stream destructor list stuff
*/
typedef enum
{
    RFCOMM_DESTRUCTOR,
    L2CAP_DESTRUCTOR,
    DM_DESTRUCTOR

} BS_DESTRUCTOR_TYPE;

typedef struct bs_destructor_tag
{
    struct bs_destructor_tag  *p_next;
    BS_DESTRUCTOR_TYPE  type;
    uint16_t  stream_key;
    STREAMS   *stream;
    BD_ADDR_T bd_addr;

} BS_DESTRUCTOR_T;

extern void bs_add_to_destructor_list(BS_DESTRUCTOR_TYPE  type,
                                      uint16_t  stream_key,
                                      STREAMS   *stream,
                                      BD_ADDR_T *bd_addr);
extern void bs_remove_from_distructor_list(BS_DESTRUCTOR_TYPE  type,
                                           uint16_t  stream_key);
extern bool_t bs_get_bd_addr(BS_DESTRUCTOR_TYPE  type,
                             uint16_t  stream_key,
                             BD_ADDR_T *bd_addr);
#else
#define bs_add_to_destructor_list(type, key, stream, bd_addr) ((void)0)
#define bs_remove_from_distructor_list(type, key) ((void)key)
#define bs_get_bd_addr(type, key, bd_addr) (FALSE)
#endif

/* Read macros from a byte stream
  
   The standard macros are endian-safe and architecture-independent.
   Only define UINTR_OPTIMIZE_LITTLE_ENDIAN if you know that your architecture
   is little-endian and will support the macros this enables.*/

#ifdef UINTR_OPTIMIZE_LITTLE_ENDIAN

/* 8 bit numbers */
#define UINT8_R(ptr, offset) \
    (*((ptr) + (offset)))

/* 16 bit numbers */
#define UINT16_R(ptr, offset) \
    (*(uint16_t *)((ptr) + (offset)))

/* 32 bit numbers */
#define UINT32_R(ptr, offset) \
    (*(uint32_t *)((ptr) + (offset)))

#else
/* Note, for the XAP, non byte packed byte stream assumed as in c6066-TM-046a */

/* 8 bit numbers */
#define UINT8_R(ptr, offset) \
    ((uint8_t)(ptr)[(offset)])

/* 16 bit numbers */
#define UINT16_R(ptr, offset)                          \
    ((uint16_t)((uint16_t)(ptr)[0 + (offset)]       |  \
                (uint16_t)(ptr)[1 + (offset)] << 8))

/* 32 bit numbers */
#define UINT32_R(ptr, offset)                          \
    ((uint32_t)((uint32_t)(ptr)[0 + (offset)]       |  \
                (uint32_t)(ptr)[1 + (offset)] << 8  |  \
                (uint32_t)(ptr)[2 + (offset)] << 16 |  \
                (uint32_t)(ptr)[3 + (offset)] << 24))
#endif

/* Writing uints of various sizes into a uint8_t buffer. */
#define URW_TYPE_BIT_OFFSET     3
#define URW_TOTAL_BIT_OFFSET    5

#define URW_SIZE_MASK           ((1 << URW_TYPE_BIT_OFFSET) - 1)
#define URW_TYPE_MASK           \
                ((1 << URW_TOTAL_BIT_OFFSET) - URW_SIZE_MASK - 1)

#define URW_TYPE_uint8_t        (0 << URW_TYPE_BIT_OFFSET)
#define URW_TYPE_uint16_t       (1 << URW_TYPE_BIT_OFFSET)
#define URW_TYPE_UNDEFINED      (2 << URW_TYPE_BIT_OFFSET)
#define URW_TYPE_uint32_t       (3 << URW_TYPE_BIT_OFFSET)

#define URW_FORMAT_INDIVIDUAL(index, type, quantity) \
    (((quantity) | (URW_TYPE_ ## type)) << ((index)*URW_TOTAL_BIT_OFFSET))

#define URW_FORMAT(type1, quantity1, type2, quantity2, type3, quantity3) \
     (URW_FORMAT_INDIVIDUAL(0, type1, (quantity1)) \
    | URW_FORMAT_INDIVIDUAL(1, type2, (quantity2)) \
    | URW_FORMAT_INDIVIDUAL(2, type3, (quantity3)))

/* We write to the place pointed to by *buf and increment the pointer */
void write_uint8(uint8_t **buf, uint8_t val);
void write_uint16(uint8_t **buf, uint16_t val);
void write_uint32(uint8_t **buf, const uint32_t *p_val); /* NOTE: Pointer to uint32_t! */

/* Read uint8_t/uint16_t/uint32_t from buffer and increment the pointer */
uint8_t read_uint8(const uint8_t **buf);
uint16_t read_uint16(const uint8_t **buf);
uint32_t read_uint32(const uint8_t **buf);

/* Read or write various numbers of various sizes of uint_t into buffer.
   Use the URW_FORMAT macro for the format parameter. The pointer will
   be incremented automatically.

   While writing, the additional arguments should be uint8_t, uint16_t or
   uint32_t*, as specified by the format. Note the pointer to uint32_t, rather
   than uin32 itself. While reading, the additional arguments should be
   uint8_t*, uint16_t*, uint32_t*.
*/
/*lint -e1916 Ellipsis */
void read_uints(const uint8_t **buf, unsigned int format, ...);
/*lint -e1916 Ellipsis */
void write_uints(uint8_t **buf, unsigned int format, ...);

void bd_addr_copy(BD_ADDR_T *p_bd_addr_dest, const BD_ADDR_T *p_bd_addr_src);
void bd_addr_zero(BD_ADDR_T *p_bd_addr);
bool_t bd_addr_eq(const BD_ADDR_T *p_bd_addr_1, const BD_ADDR_T *p_bd_addr_2);

/* Safe functions availability */
#ifndef BLUESTACK_SAFE_FUNCTIONS_AVAILABLE
size_t qbl_memscpy(void *dst, size_t dst_size, const void* src, size_t copy_size);
size_t qbl_memsmove(void *dst, size_t dst_size, const void* src, size_t copy_size);
#endif

/* Bluestack pfree(). Safe to use even on vm_const. */
#ifdef BUILD_FOR_HOST
#define bpfree(d) pfree(d)
#else
void bpfree(void *ptr);
#endif

/* BCHS doesn't need vm_const_fetch() */
#ifndef QBLUESTACK_HOST
#ifdef BUILD_FOR_HOST
#define vm_const_fetch(ptr) (*(ptr))
#endif
#endif

/* Rationalise the various flavours of "UNUSED" */
#ifdef BLUESTACK_HOST_IS_APPS
#define QBL_UNUSED(x) ((void) (x))
#else
#ifdef QBLUESTACK_HOST
#define QBL_UNUSED(x) ((void)((x)))
#else
#define QBL_UNUSED(x) unused(x)
#endif
#endif
#ifndef PARAM_UNUSED
#define PARAM_UNUSED(x) QBL_UNUSED(x)
#endif



#ifdef __cplusplus
}
#endif 


#endif /* _CSR_BT_COMMON_H_ */

