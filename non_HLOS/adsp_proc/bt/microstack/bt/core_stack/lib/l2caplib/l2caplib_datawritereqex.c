/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_DATAWRITE_REQ

    Build and send an L2CA_DATAWRITE_REQ primitive with context to L2CAP.
    See \c L2CA_DATAWRITE_REQ_T for more details.
*/
void L2CA_DataWriteReqEx(l2ca_cid_t cid,
                         uint16_t length,
                         void *p_data,
                         context_t context)
{
    L2CA_DATAWRITE_REQ_T *prim = pnew(L2CA_DATAWRITE_REQ_T);

    prim->type = L2CA_DATAWRITE_REQ;
    prim->cid = cid;
    prim->length = 0;
    prim->packets_ack = 0;
    prim->data = L2CA_MblkWrap(p_data, length);
    prim->req_ctx = context;

    L2CA_PutMsg(prim);
}
