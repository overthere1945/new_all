/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_read_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_READ_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_read_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t handle,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_READ_REQ_T *prim = zpnew(ATT_READ_REQ_T);

    prim->type = ATT_READ_REQ;
    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;

    if (pp_prim)
    {
        *pp_prim = (ATT_UPRIM_T *) prim;
    }
    else
    {
        ATT_PutMsg(prim);
    }
}

#endif /* INSTALL_ATT_MODULE */
