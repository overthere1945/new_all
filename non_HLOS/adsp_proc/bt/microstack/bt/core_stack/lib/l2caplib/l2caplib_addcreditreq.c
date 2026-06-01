/*******************************************************************************

Copyright (C) 2017 - 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"


/*! \brief Send L2CA_ADD_CREDIT_REQ

    Build and send a L2CA_ADD_CREDIT_REQ primitive to L2CAP.
    See \c L2CA_ADD_CREDIT_REQ_T for more details.
*/
#ifdef INSTALL_L2CAP_LECOC_CB
void L2CA_AddCreditReq(l2ca_cid_t cid, context_t context, uint16_t credits)
{
    L2CA_ADD_CREDIT_REQ_T *prim = zpnew(L2CA_ADD_CREDIT_REQ_T);
    
    prim->type = L2CA_ADD_CREDIT_REQ;
    prim->cid = cid;
    prim->context = context;
    prim->credits = credits;

    L2CA_PutMsg(prim);
}
#endif
