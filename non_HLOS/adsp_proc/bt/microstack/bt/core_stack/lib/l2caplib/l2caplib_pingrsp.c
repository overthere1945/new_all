/*******************************************************************************

Copyright (C) 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_PING_RSP

    Build and send an L2CA_PING_RSP primitive to L2CAP.
    See \c L2CA_PING_RSP_T for more details.
*/
void L2CA_PingRsp(const BD_ADDR_T *p_bd_addr,
                  void *p_data,
                  uint16_t length,
                  l2ca_identifier_t identifier)
{
    L2CA_PING_RSP_T *prim = zpnew(L2CA_PING_RSP_T);

    prim->type = L2CA_PING_RSP;
    bd_addr_copy(&prim->bd_addr, p_bd_addr);
    prim->length = length;

    if(prim->length > 0)
        prim->data = p_data;

    prim->identifier = identifier;

    L2CA_PutMsg(prim);
}
#endif 
