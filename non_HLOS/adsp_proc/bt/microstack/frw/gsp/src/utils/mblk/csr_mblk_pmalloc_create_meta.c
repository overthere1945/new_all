/******************************************************************************
Copyright (c) 2010-2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

/*!

\file   mblk_pmalloc_create_meta.c

\brief  Create MBLK with a newly allocated data block.
*/

#include "csr_mblk_private.h"

#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_util.h"

#if 1
/*! \brief Create MBLK with new data block and meta-data

    This function attempts to create a MBLK with a newly allocated data block.

    \param block_ptr Pointer to data block pointer, set by this function.
    \param block_size Size of data block.
    \param metadata Pointer to metadata to copy to MBLK.
    \param metasize Size of metadata.
    \return Pointer to new MBLK.
*/
CsrMblk *CsrMblkMallocCreateMeta(void **block_ptr,
    CsrMblkSize block_size,
    const void *metadata,
    CsrUint16 metasize)
{
    CsrMblk *mblk;

    *block_ptr = NULL;
    mblk = CsrMblkPmallocNew(TRUE, metasize, block_size, NULL);
    if (mblk)
    {
        *block_ptr = mblk->u.pmalloc.data;
        SynMemCpyS(CsrMblkGetMetadata(mblk), metasize, metadata, metasize);
    }

    return mblk;
}

#endif
