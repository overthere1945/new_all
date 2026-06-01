/*****************************************************************************

            Copyright (c) 2010-2015 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/

#include "csr_synergy.h"

/*!

\file   CsrMblkReadHead8.c

\brief  Read and remove a CsrUint8 from head of MBLK.

*/

#include "csr_mblk_private.h"
#include "csr_types.h"

/*! \brief Read and remove CsrUint8 from head of mblk.

    This is just a convenient wrapper for CsrMblkReadHead().

    \param mblk MBLK from which to read.
    \param value Pointer to return value that was read.
    \returns TRUE if successful, otherwise FALSE.
*/
CsrBool CsrMblkReadHead8(CsrMblk **mblk, CsrUint8 *value)
{
    return CsrMblkReadHead(mblk, value, 1) == 1;
}
