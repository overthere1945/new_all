/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#if !defined(DISABLE_L2CAP_CONNECTION_FSM_SUPPORT) || \
     defined(INSTALL_L2CAP_LECOC_CB)
/*! \brief Send L2CA_DISCONNECT_REQ

    Build and send an L2CA_DISCONNECT_REQ primitive to L2CAP.

    See \c L2CA_DISCONNECT_REQ_T for more details.
*/
void L2CA_DisconnectReq(l2ca_cid_t cid)
{
    L2CA_DISCONNECT_REQ_T *prim = zpnew(L2CA_DISCONNECT_REQ_T);

    prim->type = L2CA_DISCONNECT_REQ;
    prim->cid = cid;

    L2CA_PutMsg(prim);
}
#endif /* #if !defined(DISABLE_L2CAP_CONNECTION_FSM_SUPPORT) ||
               defined(INSTALL_L2CAP_LECOC_CB) */
