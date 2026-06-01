/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#if defined (INSTALL_ATT_MODULE) && defined (INSTALL_ATT_BREDR)

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_connect_rsp
 *
 *  DESCRIPTION
 *      Build and send an ATT_CONNECT_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_connect_rsp(
    phandle_t phandle,
    uint16_t cid,
    l2ca_conn_result_t  response,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_CONNECT_RSP_T *prim = zpnew(ATT_CONNECT_RSP_T);

    prim->type = ATT_CONNECT_RSP;
    prim->phandle = phandle;
    prim->cid = cid;
    prim->response = response;

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
