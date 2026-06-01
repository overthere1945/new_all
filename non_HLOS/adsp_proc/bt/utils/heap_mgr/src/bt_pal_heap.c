/*=============================================================================
  @file bt_pal_heap.c

  This file contains Heap management implementations of BT PAL.

*******************************************************************************
* Copyright (c) 2025 Qualcomm Technologies, Inc.  All Rights Reserved
* Qualcomm Technologies Confidential and Proprietary.
********************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "bt_pal_heap.h"
#include "bt_pal_assert.h"
#include "offload_mgr_log.h"
#include "stdlib.h"
/*=============================================================================
                        MACRO DEFINITIONS
=============================================================================*/

/**
 * @brief Defines the size of the Bluetooth PAL memory heap.
 *
 * This macro specifies the total heap size (in bytes) allocated for
 * runtime memory management. The size is set to 128 KB.
 */
#define BT_PAL_MEM_HEAP_SIZE (128 * 1024U)

/** @brief Macro for the TCM pool name */
#define BT_PAL_TCM_POOL_NAME "BT_ISLAND_LPASS_TCM_POOL"

/*=============================================================================
                    GLOBAL DATA DECLARATIONS
=============================================================================*/


/**
 * @brief Global heap manager instance for Bluetooth PAL memory operations.
 *
 * This instance is used to manage memory allocation, deallocation, and
 * pool registration for Bluetooth operations. It holds the buffer,
 * buffer size, end pointer, and memory pool handle.
 */

bt_pal_heap_mgr_t heap_mgr;

extern void *bt_pal_umem_malloc(uint16_t size);

extern void bt_pal_umem_free(void *ptr);

extern void bt_pal_uimage_heap_init(void);


/*===========================================================================
                    LOCAL FUNCTION DEFINITIONS
===========================================================================*/
/**
 * @brief Island heap test to check initial malloc. This also
 *        creates the heap.
 * @param pool_handle .
 *
 * @return true when heap test passes, false if it fails 
 */
static bool bt_pal_heap_test(island_mem_pool_t* pool_handle)
{
    size_t  test_size    = 4U;
    uint8_t *bt_test_ptr = NULL;

    bt_test_ptr = island_pool_heap_alloc(pool_handle, test_size);
    if (bt_test_ptr == NULL)
    {
        return false;
    }

    island_pool_heap_free(bt_test_ptr);
    return true;
}

/**
 * @brief Rounds up 'size' to the next smallest power of 2.
 *
 * @param size The input unsigned integer.
 * @return The smallest power of 2 that is >= size.
 */
static inline size_t ROUND_UP_TO_POWER_OF_2(size_t size)
{
    size--; // Handle cases where n is already a power of 2

    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    // For 64-bit unsigned long long or uint64_t, add:
    // size |= size >> 32;
    size++;

    return size;
}

/*=============================================================================
                    GLOBAL FUNCTION DEFINITIONS
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
 * @note if count or size is zero, a null pointer is returned silently by driver.
 *
 * @return Pointer to the allocated and zero-initialized memory block on success,
 *         or NULL on failure.
 */
void *bt_pal_calloc(size_t count, size_t size)
{
  if (!heap_mgr.is_initialized)
  {
    BT_PAL_ASSERT_FATAL("bt_pal_calloc: Heap manager not initialized! Cannot allocate.", 0, 0, 0);
  }

#ifndef ENABLE_BT_PAL_AMSS_HEAP
  return island_pool_heap_calloc(&heap_mgr.pool_handle, count, size);
#else
  return calloc(count, size);
#endif /* ENABLE_BT_PAL_AMSS_HEAP */

}
 
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
 * @note If ptr is NULL, the functions behaves exactly like bt_pal_malloc.
 * @note If ptr is not NULL and size is 0, the function behaves exactly like bt_pal_free.
 * @note If ptr is NULL and size if NULL, let us return gracefully returning NULL.
 * 
 * @return Pointer to the reallocated memory block on success, or NULL on failure.
 */
void *bt_pal_realloc(void *ptr, size_t size)
{
  if (!heap_mgr.is_initialized) 
  {
    BT_PAL_ASSERT_FATAL("bt_pal_realloc: Heap manager not initialized! Cannot reallocate.", 0, 0, 0);
  }

  if (ptr == NULL && size == 0)
  {
    return NULL;
  }

#ifndef ENABLE_BT_PAL_AMSS_HEAP
  return island_pool_heap_realloc(&heap_mgr.pool_handle, ptr, size);
#else
  return realloc(ptr, size);
#endif /* ENABLE_BT_PAL_AMSS_HEAP */
}
 
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
 *
 */
void* bt_pal_malloc(size_t size)
{
  if (!heap_mgr.is_initialized) 
  {
      BT_PAL_ASSERT_FATAL("bt_pal_malloc: Heap manager not initialized! Cannot allocate.", 0, 0, 0);
  }  

#ifndef ENABLE_BT_PAL_AMSS_HEAP
  //return island_pool_heap_alloc(&heap_mgr.pool_handle, size);
  return bt_pal_umem_malloc(size);
#else
  return malloc(size);
#endif /* ENABLE_BT_PAL_AMSS_HEAP */
}
 

/*===========================================================================
FUNCTION      bt_pal_free
===========================================================================*/
/**
 * @brief Frees memory allocated for Bluetooth operations.
 *
 * This function handles dynamic memory deallocation for Bluetooth PAL layer.
 *
 * @param[in] ptr Pointer to the memory block to be freed.
 *
 * @note If the pointer is NULL, the function returns immediately without
 *       performing any operation.
 *
 */
/*===========================================================================*/
void bt_pal_free(void *ptr)
{
  if (!heap_mgr.is_initialized) 
  {
    BT_PAL_ASSERT_FATAL("bt_pal_malloc: Heap manager not initialized! Cannot allocate.", 0, 0, 0);
  }
  
#ifndef ENABLE_BT_PAL_AMSS_HEAP
  //return island_pool_heap_free(ptr);
  return bt_pal_umem_free(ptr);
#else
  return free(ptr);
#endif /* ENABLE_BT_PAL_AMSS_HEAP */
}


//uint8_t *g_bt_pal_heap[BT_PAL_MEM_HEAP_SIZE] = {0};

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
 *
 * @return `true` if initialization is successful, `false` otherwise.
 *
 */
bool bt_pal_heap_mgr_init(void)
{
#ifndef ENABLE_BT_PAL_AMSS_HEAP
  size_t pool_config_size = BT_PAL_MEM_HEAP_SIZE; // Use this size to configure the pool
  const char* pool_name   = BT_PAL_TCM_POOL_NAME;
  size_t pool_name_len    = strlen(pool_name) + 1; // +1 for null terminator
  heap_mgr.is_initialized = false;

  //heap_mgr.heap_mem_ptr  = NULL;


  //island_heap_error_type bt_heap_err_type;

  /* 1. Register the memory pool */
  // if (!(island_register_mem_pool(&heap_mgr.pool_handle, (char*)pool_name, pool_name_len)))
  // {
  //   BT_PAL_ASSERT_FATAL("Failed to register pool '%s'", (uintptr_t)pool_name, 0, 0);
  //   return false; // Return false on registration failure
  // }

  // /* 2. Set the default heap size for the registered pool */
  // bt_heap_err_type = island_set_default_heap_size(&heap_mgr.pool_handle, ROUND_UP_TO_POWER_OF_2(pool_config_size));
  // if (bt_heap_err_type != H_SUCCESS)
  // {
  //   BT_PAL_ASSERT_FATAL("Failed to set default heap size (err: %d) for pool '%s'", (uintptr_t)bt_heap_err_type, (uintptr_t)pool_name, 0);
  //   return false; // Return false on setting default heap size failure
  // }

  /* 3. Validate heap, if test malloc fails, heap is not initialized. */
  //if (bt_pal_heap_test(&heap_mgr.pool_handle) == false) 
  //{
  //    BT_PAL_ASSERT_FATAL("bt_pal_heap_mgr_init: Heap initialization unsuccessful! Heap's test malloc failed", 0, 0, 0);
  //    return false;
  //}

  // /*heap_mgr.heap_mem_ptr*/ heap_mem_ptr = island_pool_heap_alloc(&heap_mgr.pool_handle, pool_config_size);
  // if (heap_mem_ptr == NULL)
  // {
  //   BT_PAL_ASSERT_FATAL("heap_mem_ptr is NULL", 0, 0, 0);
  //   return false;
  // }


  bt_pal_uimage_heap_init();

  heap_mgr.is_initialized = true;
  BT_PAL_LOGM("bt_pal_heap_mgr_init: Bluetooth PAL heap manager initialized successfully. TCM pool configured.");
#else
  heap_mgr.is_initialized = true;
  heap_mgr.pool_handle    = 0; //we use AMSS heap. No concept of pool handle
 #endif /* ENABLE_BT_PAL_AMSS_HEAP */

  return true;
}

/*===========================================================================
FUNCTION       bt_pal_get_current_mem_usage
===========================================================================*/
/**
 * @brief Returns the current memory usage of the BT PAL heap, as number of bytes.
 * It queries the usage from the underlying island memory pool by calling
 * `island_pool_get_current_mem_usage`.
 *
 * @return Current memory usage in bytes. Returns 0 if the heap manager is not initialized
 * or if the internal heap record cannot be retrieved.
 */
unsigned long bt_pal_get_current_mem_usage(void)
{
    if (!heap_mgr.is_initialized) 
    {
        BT_PAL_LOGM("bt_pal_get_current_mem_usage: Heap manager not initialized!", 0, 0, 0);
        return 0;
    }

    // Delegate the actual usage calculation to the external island_pool_get_current_mem_usage function
    return island_pool_get_current_mem_usage(&heap_mgr.pool_handle);
}
