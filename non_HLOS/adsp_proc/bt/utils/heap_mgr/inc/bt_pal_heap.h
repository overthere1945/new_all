/*=============================================================================
  @file bt_pal_heap.h

  @brief Bluetooth Platform Abstraction Layer (PAL) - Heap Management Header

*******************************************************************************
* Copyright (c) 2025 Qualcomm Technologies, Inc.  All Rights Reserved
* Qualcomm Technologies Confidential and Proprietary.
********************************************************************************/
#ifndef BT_PAL_HEAP_H
#define BT_PAL_HEAP_H
/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "stdbool.h"
#include "island_mem.h"
#include "string.h"
#include "memheap.h"
/*=============================================================================
                        TYPE DEFINITIONS
=============================================================================*/
/**
 * @brief Structure to manage Bluetooth PAL heap memory.
 *
 * This structure holds metadata and handles required for managing
 * memory allocation and deallocation in the Bluetooth Platform Abstraction Layer.
 */
 typedef struct 
 {
   bool      is_initialized; /**< flag to check for heap initialized */
 
   island_mem_pool_t pool_handle; /**< Handle to the memory pool used for allocations */
 } bt_pal_heap_mgr_t;
 

/*=============================================================================
                        MACRO DEFINITIONS
=============================================================================*/

/*=============================================================================
                    GLOBAL FUNCTION DECLARATIONS
=============================================================================*/ 
/*===========================================================================
FUNCTION      bt_pal_calloc
===========================================================================*/
/**
 * @brief Allocates and zero-initializes memory for an array.
 *
 * This function allocates memory for an array of `count` elements, each of
 * `size` bytes, and initializes all bytes in the allocated storage to zero.
 * The memory is allocated from either the TCM pool or the standard heap,
 * depending on the platform configuration.
 *
 * @param[in] count Number of elements to allocate.
 * @param[in] size  Size of each element in bytes.
 *
 * @return Pointer to the allocated and zero-initialized memory block on success,
 *         or NULL on failure.
 */
 void *bt_pal_calloc(size_t count, size_t size);

/*===========================================================================
FUNCTION      bt_pal_realloc
===========================================================================*/
/**
 * @brief Reallocates memory for Bluetooth operations.
 *
 * This function changes the size of the memory block pointed to by `ptr` to
 * `size` bytes. The contents will be unchanged up to the minimum of the old
 * and new sizes. If the new size is larger, the added memory will not be initialized.
 * The memory is reallocated using either the TCM pool or the standard heap,
 * depending on the platform configuration.
 *
 * @param[in] ptr  Pointer to the previously allocated memory block.
 * @param[in] size New size of the memory block in bytes.
 *
 * @return Pointer to the reallocated memory block on success, or NULL on failure.
 */
 void *bt_pal_realloc(void *ptr, size_t size);

/*===========================================================================
FUNCTION      bt_pal_malloc
===========================================================================*/
/**
 *
 * @brief This function allocates a memory block of the specified size using either
 * the TCM pool or the standard heap, depending on the compile-time configuration.
 *
 * @param[in] size Size of the memory block to allocate (in bytes).
 *
 * @return Pointer to the allocated memory block on success, or NULL on failure.
 */
 void* bt_pal_malloc(size_t size);

/*===========================================================================
FUNCTION      bt_pal_free
===========================================================================*/
/**
 * @brief This function handles dynamic memory deallocation for Bluetooth PAL layer.
 *
 * @param[in] ptr Pointer to the memory block to be freed.
 */
/*===========================================================================*/
void bt_pal_free(void *ptr);

/*===========================================================================
FUNCTION      bt_pal_heap_mgr_init
===========================================================================*/
/**
 * @brief Initializes the Bluetooth PAL heap manager.
 *
 * This function sets up the memory buffer used for Bluetooth operations.
 * Depending on the platform configuration, it uses either a statically allocated
 * TCM pool or dynamically allocated memory. It also initializes the buffer to zero
 * and sets internal metadata such as buffer size and end pointer.
 *
 * @return `true` if initialization is successful, `false` otherwise.
 *
 */
 bool bt_pal_heap_mgr_init(void);

/*===========================================================================
FUNCTION      bt_pal_get_current_mem_usage
===========================================================================*/
/**
 * @brief Returns the current memory usage of the BT PAL heap, as number of bytes.
 * It queries the usage from the underlying island pool heap.
 *
 * @return Current memory usage in bytes. Returns 0 if the heap manager is not initialized
 * or if the internal heap record cannot be retrieved.
 */
unsigned long bt_pal_get_current_mem_usage(void);

#endif /* BT_PAL_HEAP_H */
