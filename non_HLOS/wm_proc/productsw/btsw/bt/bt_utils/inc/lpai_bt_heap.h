/**************************************************************************
 * @file     lpai_bt_heap.h
 * @brief    LPAI BT HEAP Manager header file.
 * 
 * This file contains the declarations for all the heap initializations and heap memory allocation API
 * which are exposed to BT Apps 
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_HEAP_H
#define LPAI_BT_HEAP_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>

/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

/**
 * @enum btsw_heap_type_t
 * Heap Type identifiers for Identification of Heaps used for BTSW
 */
typedef enum{
    BTSW_COMMON_HEAP,
    BTSW_HEAP_MAX,
}btsw_heap_type_t;

/**
 * @struct btsw_heap_info_t
 * Structure to Store Information about each Heap Region
 */
typedef struct{
    struct k_heap bt_heap;
    btsw_heap_type_t heap_type;  /**< ID of heap memory region */
    size_t size;                /**< Total Size of Memory Region */
    void *heap_start_addr;     /**< Starting address of memory region */
}btsw_heap_info_t;



/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/

/**
 * @brief Allocates the chunk of memory block for requested number of bytes
 *        from the requested heap memory section type passed.
 * @param[in] heap_type The heap type to allocate from.
 *                      Should be one among pdtsw_heap_type_t enum values only
 * @param[in] num_bytes The number of bytes to be allocated.
 * @return Returns pointer to allocated memory if operation was success,
 *         otherwise NULL.
 * @note This API does not perform any memory alignment.
 */
void* bt_heap_alloc(btsw_heap_type_t heap_type , size_t bytes);


/**
 * @brief Allocates the chunk of memory block for requested number of bytes
 *        from the requested heap memory section type passed & initializes all bytes to zero.
 * @param[in] heap_type The heap type to allocate from.
 *                      Should be one among pdtsw_heap_type_t enum values only
 * @param[in] num_obj The number of objects to allocate.
 * @param[in] size_obj The size of each object to allocate.
 * @return Returns pointer to allocated memory if operation was success,
 *         otherwise NULL.
 * @note This API does not perform any memory alignment.
 */
void* bt_heap_calloc(btsw_heap_type_t heap_type , size_t num , size_t size);


/**
 * @brief Allocates the chunk of memory block for requested number of bytes
 *        from the requested heap memory section type passed.
 * @param[in] heap_type The heap type to allocate from.
 *                      Should be one among pdtsw_heap_type_t enum values only
 * @param[in] num_bytes The number of bytes to be allocated.
 * @param[in] align     Number of bytes by which to align.
 *                      Mandatory: Alignment in bytes, must be a power of two.
 * @return Returns pointer to allocated memory if operation was success, otherwise NULL.
 */
void* bt_heap_aligned_alloc(btsw_heap_type_t heap_type,size_t align, size_t bytes);


/**
 * @brief Reallocates the chunk of memory block for requested number of bytes
 *        from the requested heap memory section type passed.
 * @param[in] heap_type The heap type to allocate from.
 *                      Should be one among pdtsw_heap_type_t enum values only
 * @param[in] mem_p Pointer to original memory location which needs to be rellocated.
 * @param[in] num_bytes The number of bytes to be allocated during reallocation.
 * @return Returns pointer to allocated memory if operation was success,
 *         otherwise NULL.
 * @note This API does not perform any memory alignment.
 */
void* bt_heap_realloc(btsw_heap_type_t heap_type , void *heap_ptr, size_t bytes);


/**
 * @brief Frees the chunk of memory block.
 * @param[in] heap_type The heap type to allocate from.
 *                      Should be one among pdtsw_heap_type_t enum values only
 * @param[in] mem_p Pointer to memory location to be freed.
 * @return None
 */
void bt_heap_free(btsw_heap_type_t heap_type , void *heap_ptr);


#endif /**LPAI_BT_HEAP_H */