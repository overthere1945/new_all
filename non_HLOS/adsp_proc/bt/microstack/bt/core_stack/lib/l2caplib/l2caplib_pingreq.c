/*******************************************************************************

Copyright (C) 2009 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_PING_REQ

    Build and send an L2CA_PING_REQ primitive to L2CAP.
    See \c L2CA_PING_REQ_T for more details.
*/
void L2CA_PingReq(const BD_ADDR_T *p_bd_addr,
                  phandle_t phandle,
                  void *p_data,
                  uint16_t length,
                  context_t req_ctx,
                  l2ca_conflags_t flags)
{
    L2CA_PING_REQ_T *prim = zpnew(L2CA_PING_REQ_T);

    prim->type = L2CA_PING_REQ;
    bd_addr_copy(&prim->bd_addr, p_bd_addr);
    prim->length = length;

    if(prim->length > 0)
        prim->data = p_data;

    prim->phandle = phandle;
    prim->req_ctx = req_ctx;
    prim->flags = flags;

    L2CA_PutMsg(prim);
}
#endif 
