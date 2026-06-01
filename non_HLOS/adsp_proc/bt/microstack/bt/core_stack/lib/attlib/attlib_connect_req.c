/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_connect_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_CONNECT_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_connect_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    L2CA_CONNECTION_T connection,
    l2ca_conflags_t flags,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_CONNECT_REQ_T *prim = zpnew(ATT_CONNECT_REQ_T);

    prim->type = ATT_CONNECT_REQ;
    prim->phandle = phandle;
    prim->flags = flags | L2CA_CONFLAG_ENUM(connection);

    if (addrt != NULL)
    {
        tbdaddr_copy(&prim->addrt, addrt);
    }
    else
    {
        prim->addrt.type = TBDADDR_INVALID;
    }

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
