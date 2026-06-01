/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#if defined(INSTALL_ATT_MODULE) && defined(INSTALL_EATT)

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_enhanced_unregister_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_ENHANCED_UNREGISTER_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Ownership of block pointed to by handles passes to this function.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_enhanced_unregister_req(
    phandle_t phandle,
    context_t context,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_ENHANCED_UNREGISTER_REQ_T *prim = zpnew(ATT_ENHANCED_UNREGISTER_REQ_T);

    prim->type = ATT_ENHANCED_UNREGISTER_REQ;
    prim->phandle = phandle;
    prim->context = context;

    if (pp_prim)
    {
        *pp_prim = (ATT_UPRIM_T *) prim;
    }
    else
    {
        ATT_PutMsg(prim);
    }
}

#endif
