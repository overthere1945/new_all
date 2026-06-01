/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_disconnect_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_DISCONNECT_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_disconnect_req(
    phandle_t phandle,
    uint16_t cid,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_DISCONNECT_REQ_T *prim = zpnew(ATT_DISCONNECT_REQ_T);

    prim->type = ATT_DISCONNECT_REQ;
    prim->phandle = phandle;
    prim->cid = cid;

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
