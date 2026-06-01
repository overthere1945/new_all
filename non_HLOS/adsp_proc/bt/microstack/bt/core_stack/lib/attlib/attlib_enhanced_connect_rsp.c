/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#if defined(INSTALL_ATT_MODULE) && defined(INSTALL_EATT)

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_enhanced_connect_rsp
 *
 *  DESCRIPTION
 *      Build and send an ATT_ENHANCED_CONNECT_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Ownership of block pointed to by handles passes to this function.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_enhanced_connect_rsp(
    phandle_t phandle,
    uint16_t  identifier,
    uint16_t  num_cid_success,
    l2ca_conn_result_t  response,
    uint16_t mtu,
    uint16_t initial_credits,
    att_priority_t priority,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_ENHANCED_CONNECT_RSP_T *prim = zpnew(ATT_ENHANCED_CONNECT_RSP_T);

    prim->type = ATT_ENHANCED_CONNECT_RSP;
    prim->phandle = phandle;
    prim->identifier = identifier;
    prim->num_cid_success = num_cid_success;
    prim->response = response;
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
