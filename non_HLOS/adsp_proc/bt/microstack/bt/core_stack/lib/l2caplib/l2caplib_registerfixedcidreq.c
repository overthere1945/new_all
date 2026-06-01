/*******************************************************************************

Copyright (C) 2009 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_REGISTER_FIXED_CID_REQ

    Build and send a L2CA_REGISTER_FIXED_CID_REQ primitive to L2CAP.
    See \c L2CA_REGISTER_FIXED_CID_REQ_T for more details.
*/
#ifdef INSTALL_L2CAP_FIXED_CHANNEL_SUPPORT
void L2CA_RegisterFixedCidReq(phandle_t phandle,
                              l2ca_cid_t fixed_cid,
                              L2CA_CONFIG_T *config,
                              context_t reg_ctx)
{
    L2CA_REGISTER_FIXED_CID_REQ_T *prim = zpnew(L2CA_REGISTER_FIXED_CID_REQ_T);

    prim->type = L2CA_REGISTER_FIXED_CID_REQ;
    prim->phandle = phandle;
    prim->fixed_cid = fixed_cid;
    prim->reg_ctx = reg_ctx;
    qbl_memscpy(&(prim->config),
            sizeof(L2CA_CONFIG_T),
            config,
            sizeof(L2CA_CONFIG_T));

    L2CA_PutMsg(prim);
       
}
#endif
