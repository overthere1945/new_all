
/*************************************************************************
 * @file     lpai_bt_heap.c
 * @brief    LPAI BT Heap Manager source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "lpai_bt_heap.h"
#include <zephyr/kernel.h>
#include <string.h>

/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/
/**
 * @def LPAI_BT_COMMON_HEAP_SIZE
 * @brief Defines the BT Common Heap Size.
 *
 * This macro sets the heap size for common BT Heap in the System.
 */
#define LPAI_BT_COMMON_HEAP_SIZE 2048

/**
 * @def LPAI_BT_GLINK_HEAP_SIZE
 * @brief Defines the BT Glink Heap Size.
 *
 * This macro sets the heap size for BT Heap for Glink Operations in the System.
 */
#define LPAI_BT_GLINK_HEAP_SIZE 2048



/**
 * @def HEAP_INIT_POST_KERNEL_PRIORITY
 * @brief Define the Priority for Post Kernal Call For BTSW Heap Init
 * 
 * This macro defines the Priority for Post Kernal Call For BTSW Heap Init
 */
#define HEAP_INIT_PRIORITY 40


/*===========================================================================
                        STATIC/GLOBAL DATA DEFINITIONS
===========================================================================*/
static uint8_t btsw_common_heap[LPAI_BT_COMMON_HEAP_SIZE + LPAI_BT_GLINK_HEAP_SIZE];    /**< Memory Region for Common BT Heap */
//static uint8_t btsw_glink_heap[LPAI_BT_GLINK_HEAP_SIZE];     /**< Memory Region for BT GLink Heap */


/**
 * BTSW Heap Information Table
 */
static btsw_heap_info_t btsw_heap_info[BTSW_HEAP_MAX] = 
{
    {
       .heap_type =  BTSW_COMMON_HEAP,
       .size      =  LPAI_BT_COMMON_HEAP_SIZE + LPAI_BT_GLINK_HEAP_SIZE,
       .heap_start_addr = btsw_common_heap
    }
};

/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/


/**
 * @brief Method to initialize all Heap Structures used under BTSW
 * @param[in]  None
 * @return 0
 */
int btsw_heap_init()
{
    for(uint8_t idx=0; idx< BTSW_HEAP_MAX; idx++)
    {
        if(btsw_heap_info[idx].heap_start_addr != NULL && btsw_heap_info[idx].size > 0)
        {
            k_heap_init(&(btsw_heap_info[idx].bt_heap),btsw_heap_info[idx].heap_start_addr,btsw_heap_info[idx].size);
        }
    }
    return 0;
}


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
void* bt_heap_alloc(btsw_heap_type_t heap_type , size_t bytes)
{
    if(heap_type >= BTSW_HEAP_MAX)
    {
        return NULL;
    }
    
    return k_heap_alloc(&(btsw_heap_info[heap_type].bt_heap),bytes,K_NO_WAIT);
}


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
void* bt_heap_aligned_alloc(btsw_heap_type_t heap_type,size_t align, size_t bytes)
{
    if(heap_type >= BTSW_HEAP_MAX)
    {
        return NULL;
    }
    return k_heap_aligned_alloc(&(btsw_heap_info[heap_type].bt_heap), align, bytes, K_NO_WAIT);
}


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
void* bt_heap_realloc(btsw_heap_type_t heap_type , void *heap_ptr, size_t bytes)
{
    if(heap_type >= BTSW_HEAP_MAX || heap_ptr == NULL)
    {
        return NULL;
    }
    return k_heap_realloc(&(btsw_heap_info[heap_type].bt_heap), heap_ptr, bytes, K_NO_WAIT);
}


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
void* bt_heap_calloc(btsw_heap_type_t heap_type , size_t num , size_t size)
{
    if(heap_type >= BTSW_HEAP_MAX)
    {
        return NULL;
    }
    size_t num_bytes = num * size ;
    void *ptr = k_heap_alloc(&(btsw_heap_info[heap_type].bt_heap),num_bytes,K_NO_WAIT);
    if(ptr != NULL)
    {
        memset(ptr, 0, num_bytes);
    }
    return ptr;
}


/**
 * @brief Frees the chunk of memory block.
 * @param[in] heap_type The heap type to allocate from.
 *                      Should be one among pdtsw_heap_type_t enum values only
 * @param[in] mem_p Pointer to memory location to be freed.
 * @return None
 */
void bt_heap_free(btsw_heap_type_t heap_type , void *heap_ptr)
{
    if(heap_type >= BTSW_HEAP_MAX || (btsw_heap_info[heap_type].heap_start_addr) > heap_ptr)
    {
        return;
    }
    k_heap_free(&(btsw_heap_info[heap_type].bt_heap), heap_ptr);
    return;
}


SYS_INIT(btsw_heap_init, POST_KERNEL, HEAP_INIT_PRIORITY);