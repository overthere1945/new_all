/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#if !defined(DISABLE_L2CAP_CONNECTION_FSM_SUPPORT) || \
     defined(INSTALL_L2CAP_LECOC_CB)
/*! \brief Send L2CA_DISCONNECT_RSP

    Build and send an L2CA_DISCONNECT_RSP primitive to L2CAP.
    See \c L2CA_DISCONNECT_RSP_T for more details.
*/
void L2CA_DisconnectRsp(l2ca_identifier_t identifier,
                        l2ca_cid_t cid)
{
    L2CA_DISCONNECT_RSP_T *prim = pnew(L2CA_DISCONNECT_RSP_T);

    prim->type = L2CA_DISCONNECT_RSP;
    prim->identifier = identifier;
    prim->cid = cid;

    L2CA_PutMsg(prim);
}
#endif /* #if !defined(DISABLE_L2CAP_CONNECTION_FSM_SUPPORT) ||
               defined(INSTALL_L2CAP_LECOC_CB) */
