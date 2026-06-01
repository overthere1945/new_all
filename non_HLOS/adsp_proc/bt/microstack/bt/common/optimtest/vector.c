/******************************************************************************
 Copyright (c) 2005-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
#include "csr_synergy.h"
#ifndef CSR_TARGET_PRODUCT_WEARABLE
#include "optim.h"


/****************************************************************************
NAME
        vector_set  -   set 'val' into 'vec' at bit position 'index', 
  	                where 'val' is 'width' bits wide
*/
void vector_set(uint16 offset, uint16 width, uint16 *vec, uint16 val)
{
    uint16 word_offset = offset / 16;
    uint16 shift = offset % 16;
    uint16 width_mask = (1<<width) - 1;

    vec[word_offset] = 
        (val << shift) | (vec[word_offset] & (~(width_mask<<shift)));

    if ((shift + width) > 15)
    {
	uint16 done_bits = 16 - shift;
	vec[word_offset+1] = (val >> done_bits) | 
			     (vec[word_offset+1] & (~(width_mask >> done_bits)));
    }
}

/****************************************************************************
NAME
        vector_get  -   Get 'width' bits from bit position 'offset' from 'vec'
*/
uint16 vector_get(uint16 offset, uint16 width, uint16 *vec)
{
    uint16 word_offset = offset / 16;
    uint16 shift = offset % 16;
    uint16 shifted_word = (vec[word_offset] >> shift);
    uint16 width_mask = (1<<width) - 1;    

    if ((shift + width) < 16)
    {
	return shifted_word & width_mask;
    }
    else
    {
	return (shifted_word | (vec[word_offset+1] << (16 - shift)))
	       & width_mask;
    }
}


/****************************************************************************
NAME
        vector_test
*/
#if 0
void vector_test(void)
{
    uint16 test_bit_array[BCCMD_CHANNEL_BITMASK_UINT16S * 3];
    uint16 index;
    memset(test_bit_array, 0, sizeof(test_bit_array));
    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
       if (vector_get(index*3, 3, test_bit_array) != 0)
       {
           panic((panicid)0xfe);
       }
    }

    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
        vector_set(index*3, 3, test_bit_array, 0x5);
    }

    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
       if (vector_get(index*3, 3, test_bit_array) != 0x5)
       {
           panic((panicid)0xfd);
       }
    }

    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
        vector_set(index*3, 3, test_bit_array, 0x2);
    }

    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
       if (vector_get(index*3, 3, test_bit_array) != 0x2)
       {
           panic((panicid)0xfc);
       }
    }

    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
        vector_set(index*3, 3, test_bit_array, 0x7);
    }

    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
       if (vector_get(index*3, 3, test_bit_array) != 0x7)
       {
           panic((panicid)0xfb);
       }
    }


    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
        if (index & 1)
            continue;
        vector_set(index*3, 3, test_bit_array, 0x0);
    }

    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
        uint16 val;
        if (index & 1)
            val = 0x7;
        else
            val = 0x0;
       if (vector_get(index*3, 3, test_bit_array) != val)
       {
           panic((panicid)0xfa);
       }
    }

    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
        vector_set(index*3, 3, test_bit_array, index);
    }

    for (index = 0; index < (BCCMD_CHANNEL_BITMASK_UINT16S * 16); index++)
    {
       if (vector_get(index*3, 3, test_bit_array) != (index & 0x7))
       {
           panic((panicid)0xf9);
       }
    }
}
#endif
#endif
