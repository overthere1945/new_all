/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_REGISTER_REQ

    Build and send an L2CA_REGISTER_REQ primitive to L2CAP.
*/
void L2CA_RegisterReq(psm_t psm,
                      phandle_t phandle,
                      uint16_t mode_mask,
                      uint16_t flags,
                      context_t reg_ctx)
{
    L2CA_REGISTER_REQ_T *prim = zpnew(L2CA_REGISTER_REQ_T);

    prim->type = L2CA_REGISTER_REQ;
    prim->psm_local = psm;
    prim->phandle = phandle;
    prim->mode_mask = mode_mask;
    prim->flags = flags;
    prim->reg_ctx = reg_ctx;

    L2CA_PutMsg(prim);
}
#endif 
