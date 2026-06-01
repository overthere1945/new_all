/*=============================================================================
  @file bt_pal_micro_heap.c

  This file contains Micro Heap Abstractions for BT PAL.

*******************************************************************************
* Copyright (c) 2025 Qualcomm Technologies, Inc.  All Rights Reserved
* Qualcomm Technologies Confidential and Proprietary.
********************************************************************************/
/*===========================================================================
                      INCLUDE FILES
===========================================================================*/

#include "umemheap_lite.h"
#include <stdlib.h>
#include <stdint.h>
#include "bt_pal_log.h"


/*=============================================================================
					MACRO DEFINITIONS
=============================================================================*/
#define BT_PAL_MEM_UHEAP_SIZE (100 * 1024U)


/*=============================================================================
					GLOBAL DATA DECLARATIONS
=============================================================================*/

/**
 * @brief Global Micro Heap Structure for BT PAL Operations
 */
mem_heap_type uheap_ptr;

/**
 * @brief Static Buffer for BT PAL Heap Abstraction for Micro Heap
 */
static uint8_t bt_uheap_buffer[BT_PAL_MEM_UHEAP_SIZE];


/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/
/**
 * @brief Initializes the Micro Heap for BT PAL.
 *
 * This function initializes Micro Heap for Bluetooth PAL layer using a static TCM Buffer.
 *
 * @param[in] None
 *
 * @return None
 *
 */
void bt_pal_uimage_heap_init(void)
{
   umem_init_heap_with_debug(&uheap_ptr, bt_uheap_buffer, BT_PAL_MEM_UHEAP_SIZE, 1);
}

/**
 * @brief Frees memory allocated for Bluetooth operations on Micro Heap.
 *
 * This function handles dynamic memory deallocation for Bluetooth PAL layer.
 *
 * @param[in] ptr Pointer to the memory block to be freed.
 *
 * @note If the pointer is NULL, the function returns immediately without
 *       performing any operation.
 *
 */
void bt_pal_umem_free(void *ptr)
{
    BT_PAL_LOGH("bt_pal_umem_free 0x%x", ptr);
    return umem_free(&uheap_ptr, ptr);
}

/**
 *
 * @brief This function allocates a memory block of the specified size using either
 * the TCM pool or the standard heap, depending on the compile-time configuration.
 *
 * @param[in] size Size of the memory block to allocate (in bytes).
 *
 * @return Pointer to the allocated memory block on success, or NULL on failure.
 *
 */
void *bt_pal_umem_malloc(uint16_t size)
{
    uint8_t *ptr = umem_malloc(&uheap_ptr , size);
    if (ptr != NULL)
    {
        BT_PAL_LOGH("bt_pal_umem_malloc ptr:0x%x sz:0x%x ", ptr, size);
    }
    return ptr;
}
