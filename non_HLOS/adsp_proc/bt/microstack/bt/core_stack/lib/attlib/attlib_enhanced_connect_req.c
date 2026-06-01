/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#if defined(INSTALL_ATT_MODULE) && defined(INSTALL_EATT)

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_enhanced_connect_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_ENHANCED_CONNECT_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Ownership of block pointed to by handles passes to this function.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_enhanced_connect_req(
    phandle_t phandle,
    TP_BD_ADDR_T  *tp_addrt,
    l2ca_conflags_t flags,
    att_mode_t mode,
    uint16_t num_bearers,
    uint16_t mtu,
    uint16_t initial_credits,
    att_priority_t priority,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_ENHANCED_CONNECT_REQ_T *prim = zpnew(ATT_ENHANCED_CONNECT_REQ_T);

    prim->type = ATT_ENHANCED_CONNECT_REQ;
    prim->phandle = phandle;
    prim->tp_addrt = *tp_addrt;
    prim->flags = flags;
    prim->mode = mode;
    prim->num_bearers = num_bearers;
    prim->mtu = mtu;
    prim->initial_credits = initial_credits;
    prim->priority = priority;

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
