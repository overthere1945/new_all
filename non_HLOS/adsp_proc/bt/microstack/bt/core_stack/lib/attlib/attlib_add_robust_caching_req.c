/*******************************************************************************

Copyright (C) 2018 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE
/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_add_robust_caching_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_ADD_ROBUST_CACHING_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_add_robust_caching_req(
    phandle_t phandle,
    context_t context,
    TP_BD_ADDR_T *tp_addrt,
    uint16_t change_aware,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_ADD_ROBUST_CACHING_REQ_T *prim = zpnew(ATT_ADD_ROBUST_CACHING_REQ_T);

    prim->type = ATT_ADD_ROBUST_CACHING_REQ;

    tbdaddr_copy(&prim->tp_addrt.addrt, &tp_addrt->addrt);
    prim->tp_addrt.tp_type = tp_addrt->tp_type;
    prim->phandle = phandle;
    prim->context = context;
    prim->change_aware = change_aware;

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
