/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Send L2CA_UNREGISTER_REQ

    Build and send an L2CA_UNREGISTER_REQ primitive to L2CAP.
*/
void L2CA_UnRegisterReq(psm_t psm_local,
                        phandle_t phandle)
{
    L2CA_UNREGISTER_REQ_T *prim = pnew(L2CA_UNREGISTER_REQ_T);

    prim->type = L2CA_UNREGISTER_REQ;
    prim->psm_local = psm_local;
    prim->phandle = phandle;

    L2CA_PutMsg(prim);
}
#endif 
