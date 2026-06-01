#ifndef MBLK_H__
#define MBLK_H__
/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_mblk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define mblk_size_t CsrMblkSize

#define mblk_map(_mblk, _mblk_offset, _mblk_size)           \
    CsrMblkMap((const CsrMblk *) _mblk,                     \
               (CsrMblkSize) _mblk_offset,                  \
               (CsrMblkSize) _mblk_size)

#define mblk_unmap(_mblk, _mapped_data)                     \
    CsrMblkUnmap((const CsrMblk *) _mblk,                   \
                 (void *) _mapped_data)

#define mblk_get_length(_mblk)                              \
    CsrMblkGetLength((const CsrMblk *) _mblk)

#define mblk_malloc_create(_block_ptr, _block_Size)         \
    CsrMblkMallocCreate((void **) _block_ptr,               \
                        (CsrMblkSize) _block_Size)

#define mblk_duplicate_create(_dup_mblk,                    \
                              _dup_offset,                  \
                              _dup_length)                  \
    CsrMblkDuplicateCreate((CsrMblk *) _dup_mblk,           \
                           (CsrMblkSize) _dup_offset,       \
                           (CsrMblkSize) _dup_length)

#define mblk_inc_refcount(_mblk, _inc)                      \
    CsrMblkIncRefcount((CsrMblk *) _mblk, (CsrUint8) _inc)

#define mblk_read_head(_mblk, _data, _size_left)            \
    CsrMblkReadHead((CsrMblk **) _mblk,                     \
                    (void *) _data,                         \
                    (CsrMblkSize) _size_left)

#define mblk_read_tail(_mblk, _data, _size_left)            \
    CsrMblkReadTail((CsrMblk **) _mblk,                     \
                    (void *) _data,                         \
                    (CsrMblkSize) _size_left)

#define mblk_add_head(_head, _chain)                        \
    CsrMblkAddHead((CsrMblk *) _head, (CsrMblk *) _chain)

#define mblk_add_tail(_tail, _chain)                        \
    CsrMblkAddTail((CsrMblk *) _tail, (CsrMblk *) _chain)

#define mblk_duplicate_region(_mblk,                        \
                              _dup_offset,                  \
                              _dup_size)                    \
    CsrMblkDuplicateRegion((CsrMblk *) _mblk,               \
                           (CsrMblkSize) _dup_offset,       \
                           (CsrMblkSize) _dup_size)

#define mblk_iterate_region(_mblk,                          \
                            _mblk_offset,                   \
                            _mblk_size,                     \
                            _itr_func,                      \
                            _itr_data)                      \
    CsrMblkIterateRegion((const CsrMblk *) _mblk,           \
                         (CsrMblkSize) _mblk_offset,        \
                         (CsrMblkSize) _mblk_size,          \
                         (CsrBool (*)(const void *,         \
                                      CsrMblkSize,          \
                                      CsrMblkSize,          \
                                      void *)) _itr_func,   \
                         (void *) _itr_data)

#define mblk_read_head16(_mblk, _value)                     \
    CsrBtMblkReadHead16((CsrMblk **) _mblk,                 \
                        (CsrUint16 *) _value)

#define mblk_coalesce_to_pmalloc(_mblk)                     \
    CsrMblkCoalesceToPmalloc((CsrMblk **) _mblk)

#define mblk_copy_to_memory(_mblk,                          \
                            _mblk_offset,                   \
                            _mblk_size,                     \
                            _mem_ptr)                       \
    CsrMblkCopyToMemory((const CsrMblk *) _mblk,            \
                        (CsrUint16) _mblk_offset,           \
                        (CsrMblkSize) _mblk_size,           \
                        (CsrUint8 *) _mem_ptr)

#define mblk_discard_head(_mblk, _size_left)                \
    CsrMblkDiscardHead((CsrMblk **) _mblk,                  \
                       (CsrMblkSize) _size_left)

#define mblk_discard_tail(_mblk, _size_left)                \
    CsrMblkDiscardTail((CsrMblk **) _mblk,                  \
                       (CsrMblkSize) _size_left)

#define mblk_split(_mblk, _split_offset)                    \
    CsrMblkSplit((CsrMblk *) _mblk,                         \
                 (CsrMblkSize) _split_offset)

#define mblk_read_head8s CsrBtMblkReadHead8s

#define mblk_read_head8(_mblk, _value)                      \
    CsrBtMblkReadHead8((CsrMblk **) _mblk,                  \
                       (CsrUint8 *) _value)

#define mblk_destroy(_mblk) CsrMblkDestroy((CsrMblk *) _mblk)

#define mblk_data_create(_block, _block_size, _free_block)  \
    CsrMblkDataCreate((void *) _block,                      \
                      (CsrMblkSize) _block_size,            \
                      (CsrBool) _free_block)

#define mblk_set_destructor(_mblk, _destructor)             \
    CsrMblkSetDestructor((CsrMblk *) _mblk,                 \
                         (void (*)(CsrMblk *)) _destructor)

#define mblk_data_create_meta(_block,                       \
                              _block_size,                  \
                              _free_block,                  \
                              _metadata,                    \
                              _metasize)                    \
    CsrMblkDataCreateMeta((void *) _block,                  \
                          (CsrMblkSize) _block_size,        \
                          (CsrBool) _free_block,            \
                          (const void *) _metadata,         \
                          (CsrUint16) _metasize)

#define mblk_get_metadata(_mblk)                            \
    CsrMblkGetMetadata((CsrMblk *) _mblk)

#ifdef __cplusplus
}
#endif

#endif
