/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

#ifndef ATT_FLAT_DB_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_add_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_ADD_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_add_req(
    phandle_t phandle,
    att_attr_t *attrs,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_ADD_REQ_T *prim = zpnew(ATT_ADD_REQ_T);

    prim->type = ATT_ADD_REQ;
    prim->phandle = phandle;
    prim->attrs = attrs;

    if (pp_prim)
    {
        *pp_prim = (ATT_UPRIM_T *) prim;
    }
    else
    {
        ATT_PutMsg(prim);
    }
}
#endif /* !ATT_FLAT_DB_SUPPORT */

#endif /* INSTALL_ATT_MODULE */
