/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_DATAWRITE_ABORT_REQ

    Build and send a L2CA_DATAWRITE_ABORT_REQ primitive to L2CAP.
    See \c L2CA_DATAWRITE_ABORT_REQ_T for more details.
*/ 
void L2CA_DataWriteAbortReq(l2ca_cid_t cid)
{
    L2CA_DATAWRITE_ABORT_REQ_T *prim = pnew(L2CA_DATAWRITE_ABORT_REQ_T);

    prim->type = L2CA_DATAWRITE_ABORT_REQ;
    prim->cid = cid;

    L2CA_PutMsg(prim);
}
