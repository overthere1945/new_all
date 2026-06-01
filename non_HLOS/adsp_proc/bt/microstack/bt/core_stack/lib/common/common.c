/*******************************************************************************

Copyright (C) 2008 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/

#include <string.h>
#include <stdarg.h>
#include "qbl_adapter_types.h"
#include "qbl_adapter_panic.h"
#include INC_DIR(bluestack,bluetooth.h)
#include "common.h"

#ifndef BUILD_FOR_HOST
#include "vm_const.h"
#elif defined(BLUESTACK_HOST_IS_APPS)
#include "memory_map.h"
#endif


#if !defined(REALLY_ON_HOST) && (!defined(BUILD_FOR_HOST) || defined(BLUESTACK_HOST_IS_APPS))
/* Used as proxy for unit tests where this assert is not required.
 *
 * This is done to ensure that the opaque context parameter in the Bluestack
 * primitives can be used to hold a pointer in the current platform.
 */
COMPILE_TIME_ASSERT(sizeof(context_t) == sizeof(void *), \
                    context_t_cannot_hold_pointer);
#endif

#ifndef BUILD_FOR_HOST
#include "vm_const.h"
#endif

#ifndef BUILD_FOR_HOST
/*----------------------------------------------------------------------------*
 *  NAME
 *      bpfree
 *
 *  DESCRIPTION
 *      Bluestack pfree.
 *      Makes sure that we're not pointing at VM const space and then passes
 *      to pfree. So this is not for BCHS use.
 *----------------------------------------------------------------------------*/
void bpfree(void *ptr)
{
    if (vm_const_is_pmalloc(ptr))
        pfree(ptr);

}
#endif

/*! \brief Write a uint8_t to the place pointed to and increment the pointer. 

    \param buf Pointer to pointer to place in buffer.
    \param val Value to be written.
*/
void write_uint8(uint8_t **buf, uint8_t val)
{
    *((*buf)++) = val & 0xFF;
}

/*! \brief Read a uint8_t from the place pointed to and increment the pointer.
 
    \param buf Pointer to pointer to place in buffer.
    \returns Value read.
*/
uint8_t read_uint8(const uint8_t **buf)
{
    return 0xFF & *((*buf)++);
}

/*! \brief Write a uint16_t to the place pointed to and increment the pointer.
 
    \param buf Pointer to pointer to place in buffer.
    \param val Value to be written.
*/
void write_uint16(uint8_t **buf, uint16_t val)
{
    write_uint8(buf, (uint8_t)(val & 0xFF));
    write_uint8(buf, (uint8_t)(val >> 8));
}

/*! \brief Read a uint16_t from the place pointed to and increment the pointer.
 
    \param buf Pointer to pointer to place in buffer.
    \returns Value read.
*/
uint16_t read_uint16(const uint8_t **buf)
{
    uint16_t val_low = read_uint8(buf);
    uint16_t val_high = read_uint8(buf);

    return val_low | (val_high << 8);
}

/*! \brief Write a uint32_t to the place pointed to and increment the pointer.

    \param buf Pointer to pointer to place in buffer.
    \param p_val Pointer to value to be written.
*/
void write_uint32(uint8_t **buf, const uint32_t *p_val)
{
    write_uint16(buf, (uint16_t)(*p_val & 0xFFFF));
    write_uint16(buf, (uint16_t)(*p_val >> 16));
}

/*! \brief Read a uint32_t from the place pointed to and increment the pointer.
 
    \param buf Pointer to pointer to place in buffer.
    \returns Value read.
*/
uint32_t read_uint32(const uint8_t **buf)
{
    uint16_t val_low = read_uint16(buf);
    uint16_t val_high = read_uint16(buf);

    return val_low | (((uint32_t)val_high) << 16);
}

/*! \brief Read or write a sequence of types to/from a uint8_t buffer.
 
    The var args are read using the format argument. It's best to use
    the URW_FORMAT macro to create the format. However, the details
    are explained below.
   
    format is interpreted as three five-bit values. In each five-bit
    value, the least significant three bits determines the quantity,
    and the most significant two the type. So from LSB to MSB, format
    is as follows:
 
    Bits   0-2    3-4   5-7    8-9   10-12  13-14 15
           quant1 type1 quant2 type2 quant3 type3 0(reserved)
    
    The function starts with the least significant 5 bits, once it has
    decremented the quantity part to zero, it shifts right by 5 and then
    repeats until it runs out of non-zero bits.

    \param format Number and types of var args.
    \param buf Pointer to pointer to buffer. Type uint8_t**.
    \param ... variables to be written.
*/
void write_uints(uint8_t **buf, unsigned int format, ...)
{
    va_list ap;
    va_start(ap, format);

    /* Loop over the type/quantity pairs. */
    for(; format != 0; format >>= URW_TOTAL_BIT_OFFSET)
    {
        /* Loop through the quantity. */
        for(; (format & URW_SIZE_MASK) != 0; --format)
        {
            unsigned int type = (format & URW_TYPE_MASK);

            /* Read from var args and write to buffer. */
            if (type == URW_TYPE_uint32_t)
            {
                write_uint32(buf, va_arg(ap, uint32_t*));
            }
            else
            {
                unsigned int val = va_arg(ap, unsigned int);

                if (type == URW_TYPE_uint8_t)
                {
                    write_uint8(buf, (uint8_t)val);
                }
                else if (type == URW_TYPE_uint16_t)
                {
                    write_uint16(buf, (uint16_t)val);
                }
            }
        }
    }
    va_end(ap);
}

void read_uints(const uint8_t **buf, unsigned int format, ...)
{
    va_list ap;
    va_start(ap, format);

    /* Loop over the type/quantity pairs. */
    for (; format != 0; format >>= URW_TOTAL_BIT_OFFSET)
    {
        /* Loop through the quantity. */
        for(; (format & URW_SIZE_MASK) != 0; --format)
        {
            uint32_t ignore;
            void *p_val;

            /* Read address from var args. If it's NULL then we still
               proceed to read from buffer, but we ignore the result. 
               This ensures that the buffer pointer gets incremented. */
            if ((p_val = va_arg(ap, void*)) == NULL)
                p_val = (void*)&ignore;

            /* Read from buffer and write to address from var args. */
            switch (format & URW_TYPE_MASK)
            {
                case URW_TYPE_uint8_t:
                    *((uint8_t*)p_val) = read_uint8(buf);
                    break;

                case URW_TYPE_uint16_t:
                    *((uint16_t*)p_val) = read_uint16(buf);
                    break;

                case URW_TYPE_uint32_t:
                    *((uint32_t*)p_val) = read_uint32(buf);
                    break;
            }
        }
    }

    va_end(ap);
}

#ifndef BLUESTACK_SAFE_FUNCTIONS_AVAILABLE
size_t qbl_memscpy(void *dst, size_t dst_size, const void* src, size_t copy_size)
{
    copy_size = (dst_size <= copy_size) ? dst_size : copy_size;
    memcpy(dst, src, copy_size);
    return copy_size;
}

size_t qbl_memsmove(void *dst, size_t dst_size, const void* src, size_t copy_size)
{
    copy_size = (dst_size <= copy_size) ? dst_size : copy_size;
    memmove(dst, src, copy_size);
    return copy_size;
}
#endif
