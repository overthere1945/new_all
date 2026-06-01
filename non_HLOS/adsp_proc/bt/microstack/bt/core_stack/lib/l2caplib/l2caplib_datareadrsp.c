/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_DATAREAD_RSP

    Build and send an L2CA_DATAREAD_RSP primitive to L2CAP.
    See \c L2CA_DATAREAD_RSP_T for more details.
*/
void L2CA_DataReadRsp(l2ca_cid_t cid, uint16_t packets)
{
    L2CA_DATAREAD_RSP_T *prim = pnew(L2CA_DATAREAD_RSP_T);
    
    prim->type = L2CA_DATAREAD_RSP;
    prim->cid = cid;
    prim->packets = packets;

    L2CA_PutMsg(prim);
}
