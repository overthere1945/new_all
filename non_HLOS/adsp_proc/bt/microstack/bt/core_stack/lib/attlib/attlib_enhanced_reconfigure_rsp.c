/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#if defined(INSTALL_ATT_MODULE) && defined(INSTALL_EATT)

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_enhanced_reconfigure_rsp
 *
 *  DESCRIPTION
 *      Build and send an ATT_ENHANCED_RECONFIGURE_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Ownership of block pointed to by handles passes to this function.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_enhanced_reconfigure_rsp(
    phandle_t phandle,
    uint16_t identifier,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_ENHANCED_RECONFIGURE_RSP_T *prim = zpnew(ATT_ENHANCED_RECONFIGURE_RSP_T);

    prim->type = ATT_ENHANCED_RECONFIGURE_RSP;
    prim->phandle = phandle;
    prim->identifier = identifier;

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
