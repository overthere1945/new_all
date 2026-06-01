#ifndef	__OPTIM_H__
#define	__OPTIM_H__

/******************************************************************************
 Copyright (c) 1999-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

/*******************************************************************************
FILE
        optim.h  -  advertise the services of the optim library

CONTAINS
        udiv3216  -  do a 32 by 16 bit division return quotient and remainder
        memset_nobc  -  set memory without using BC
        memset_nobc2  -  set memory in pairs of words without using BC

DESCRIPTION
    This file contains headers fo highly optimised routines available in
    optimbc01 or optimtest. optimbc01 will usually have hand crafted
    xap assembller, optimtest will have generic C routines for use with
    targets which haven't had hand crafted solutions.

MODIFICATION HISTORY
    1.1  9:nov:99   sms Created, uint3216 added.
    1.2  12:dec:99  cjo References to uint3216 changed to udiv3216.
    1.3  25:aug:00  sms Added memset_nobc and memset_nobc2.
    1.8  10:dec:01  pws     Changes for bc02 build.
    1.10 02:may:03  pws     B-452: version support rationalisation for bc3
    #11  01:oct:03  mm      B-1135: added L2CAP CRC generation helper.
    #12  16:apr:04  pws     B-2323: Merge back fruitbat changes, fixing:
                            B-1494: Added Drain2 support.
    #13  12:aug:04  mm  B-3653: Make memcpy_unpack more generic.
    #14  19:aug:04  mm  B-3736: Add get/put_le_uint32.
    #15  22:sep:04  bs01    B-4063: Added byte_swap.
    #16  26:oct:05  sms     B-10024: Added udiv6416.
    #18  30:may:07  sv01    B-24242: Port antenna diversity to mainline.
        ------------------------------------------------------------------
        --- This modification history is now closed. Do not update it. ---
        ------------------------------------------------------------------

********************************************************************************/

/* BCHS_EXPORT_POINT_START */

#include "qbl_adapter_types.h"
#include "bluetooth.h"
#include "qbl_types.h"

/****************************************************************************
NAME
	udiv3216  -  do a 32 by 16 bit division return quotient and remainder

RETURNS
	Return value is value (n/d).  If r is not NULL on entry then the
	remainder is written into r.
*/

extern uint32 udiv3216(uint16 *r, uint16 d, uint32 n);


/****************************************************************************
NAME
	memset_nobc  -  set memorywithout using BC

FUNCTION
	Like memset, but it doesn't use the XAP block copy instruction
*/

extern void memset_nobc(uint8 *dest, uint8 first_word, uint16 repeats);


/****************************************************************************
NAME
	memset_nobc2  -  set memory in pairs of words without using BC

FUNCTION
	Like memset, but you give it two words and it alternates them
	through memory.

	Note that this function does not use the XAP block copy instruction
*/

extern void memset_nobc2(uint8 *dest, uint8 first_word, uint8 second_word,
			 uint16 repeats);

/****************************************************************************
NAME
	memcpy_unpack  -  copy memory turning uint16 into 2 * uint8

FUNCTION
	Like memcpy but the source is a uint16 array and the destination
	is a uint8 array. The first word of the uint16 array goes into
	the first 2 words of the uint8 array (LSB first). This function
	guarantees to clear the upper 8 bits of the uint8 array.

	The length given is the length of the uint16 array.

RETURNS
	Unlike memcpy this function returns void.
*/

extern void memcpy_unpack(uint8 *dest, const uint16 *source, uint16 length);


/****************************************************************************
NAME
	memcpy_pack  -  copy memory turning 2 * uint8 into uint16

FUNCTION
	Like memcpy but the source is a uint8 array and the destination
	is a uint16 array. The first word of the uint16 array is built
	from the first 2 words of the uint8 array (LSB first).  If the
	uint8's contain set bits in their MSBs these are masked away.

	The length given is the length of the uint16 array.

RETURNS
	Unlike memcpy this function returns void.
*/

extern void memcpy_pack(uint16 *dest, const uint8 *source, uint16 length_words);


/****************************************************************************
NAME
	memcpy_unpack_be  -  copy memory turning uint16 into 2 * uint8

FUNCTION
	Like memcpy but the source is a uint16 array and the destination
	is a uint8 array. The first word of the uint16 array goes into
	the first 2 words of the uint8 array (MSB first). This function
	guarantees to clear the upper 8 bits of the uint8 array.

	The length given is the length of the uint16 array.

RETURNS
	Unlike memcpy this function returns void.
*/

extern void memcpy_unpack_be(uint8 *dest, const uint16 *source, uint16 length);


/****************************************************************************
NAME
	memcpy_pack_be  -  copy memory turning 2 * uint8 into uint16

FUNCTION
	Like memcpy but the source is a uint8 array and the destination
	is a uint16 array. The first word of the uint16 array is built
	from the first 2 words of the uint8 array (MSB first).  If the
	uint8's contain set bits in their MSBs these are masked away.

	The length given is the length of the uint16 array.

RETURNS
	Unlike memcpy this function returns void.
*/

extern void memcpy_pack_be(uint16 *dest, const uint8 *source, uint16 length);


/****************************************************************************
NAME
        cvsd_filter  -  apply CVSD correction filter

FUNCTION
        Applies a digital filter to provide some high frequency emphasis.
        This is useful when using CVSD with 16 bit linear host interface data.
*/

extern void cvsd_filter(uint8 *buffer, uint16 num_samples, uint16 filt_coef);


/****************************************************************************
NAME
        crc_correction  -  Helper for L2CAP CRC correction

FUNCTION
	This is a helper used in the L2CAP CRC generation.  I think
	(now bear with me here) that is performs a * b % POLY, but in
	that special maths used by CRC's (GF2).

	POLY is 0x8005, The L2CAP CRC polynomial.
*/

extern uint16 crc_correction(uint16 a, uint16 b);


/****************************************************************************
NAME
        saturate_add  -  Add two 32-bit numbers, saturating on overflow

FUNCTION
        Adds two 32-bit unsigned numbers together.  If there is a
        carry, the result is set to 0xffffffff.
*/

extern void saturate_add(uint32 increment, uint32 *sum);


/****************************************************************************
NAME
	get_le_uint32  -  get a uint32 from a uint8 array

FUNCTION
	This will read a 'little-endian' uint32 from the uint8 array.
	It correctly masks out any garbage that might be in the msb's.
*/

extern uint32 get_le_uint32(const uint8 *ptr);


/****************************************************************************
NAME
	put_le_uint32  -  put a uint32 into a uint8 array

FUNCTION
	This will write a 'little-endian' uint32 into the uint8 array.
	It correctly writes zero's into the msb's.
*/

extern void put_le_uint32(uint32 val, uint8 *ptr);

/****************************************************************************
NAME
	byte_swap  -  exchange the high and low bytes of a memory block

FUNCTION
        Exchange the high and low bytes of each word in a uint16 array
*/

extern void byte_swap(uint16 *data, uint16 words);


/****************************************************************************
NAME
        vector_set  -   set 'val' in 'vec' at bit position 'offset', 
  	                where 'val' is 'width' bits wide
*/
extern void vector_set(uint16 offset, uint16 width, uint16 *vec, uint16 val);

/****************************************************************************
NAME
        vector_get  -   Get 'width' bits from bit position 'offset' from 'vec'
*/
extern uint16 vector_get(uint16 offset, uint16 width, uint16 *val);


/****************************************************************************
TYPE
	optim_uint64  -  a 64 bit integer for the optim library

PURPOSE
	The only purpose of this type is to be divided quickly by a uint16
	using the function udiv6416.

NOTES
	The longtimer module relies on the internal workings of this
	structure. If you change it then update that module too.
*/

typedef struct  {
    uint32 msw;
    uint32 lsw;
} optim_uint64;


/****************************************************************************
NAME
	udiv6416  -  do a 64 by 16 bit division return quotient and remainder

RETURNS
 	Return value in AL is remainder, The result is written into *q.

	q may be NULL on entry in which case the quotient is discarded.

	q and n may point to the same block of memory in which case the
	division is done in place.
*/
extern uint16 udiv6416(optim_uint64 *q, uint16 d, const optim_uint64 *n);


/****************************************************************************
NAME
        multiply_skew  -  multiply by given parts per 2^20

FUNCTION
        Returns t * skew / 2^20, so skew is almost but not quite a parts
        per million value. The multiply of t * skew occurs in 48 bits, so
        always fits. 
*/
extern int32 multiply_skew(uint32 t, int16 skew);


/****************************************************************************
NAME
    call_with_stack_memory  -  Allocate stack memory and call me function

FUNCTION
    This function will allocate memory from stack address space of the size
    *len* in words and the allocated stack pointer is passed on to the me
    function.
    The first argument to the me function is the allocated stack pointer and
    second is an argurment which could be passed onto the me function.

    The allocated stack memory will be de-alloacted once the me function call
    returns to this function.

RETURNS
    Returns the uint16 value returned by me function.
*/
extern uint16 call_with_stack_memory(uint16 (*me)(void *, void *),
                                    size_t len, void *arg);


/****************************************************************************
NAME
	umult2424  -  do a 24 by 24 bit multiplication and return 48 bit result

RETURNS
	The 48 bit reults in r.
        
        r must NOT be NULL.
*/
extern void umult2424(uint32 a1, uint32 a2, util_uint48 *r);


/* BCHS_EXPORT_POINT_END */

#endif	/* __OPTIM_H__ */
