/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_DISCONNECT_REQ
    
    Build and send an L2CA_DISCONNECT_REQ primitive to L2CAP.

    cid is set to 0 so we disconnect by connection context.
                  
    See \c L2CA_DISCONNECT_REQ_T for more details.
*/
void L2CA_DisconnectCtxReq(context_t con_ctx)
{
    L2CA_DISCONNECT_REQ_T *prim = zpnew(L2CA_DISCONNECT_REQ_T);
    prim->type = L2CA_DISCONNECT_REQ;
    prim->con_ctx = con_ctx;

    L2CA_PutMsg(prim);
}
#endif 
