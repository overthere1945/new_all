/*******************************************************************************

Copyright (C) 2010 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_read_by_type_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_READ_BY_TYPE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Ownership of block pointed to by uuid is not taken over by
 *      this function. Caller must dispose of it as appropraite.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_read_by_type_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t start,
    uint16_t end,
    att_uuid_type_t uuid_type,
    uint32_t *uuid,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_READ_BY_TYPE_REQ_T *prim = zpnew(ATT_READ_BY_TYPE_REQ_T);

    prim->type = ATT_READ_BY_TYPE_REQ;
    prim->phandle = phandle;
    prim->cid = cid;
    prim->start = start;
    prim->end = end;
    prim->uuid_type = uuid_type;

    qbl_memscpy(prim->uuid, 4*sizeof(uint32_t), uuid, 4*sizeof(uint32_t));

    if (pp_prim)
    {
        *pp_prim = (ATT_UPRIM_T *) prim;
    }
    else
    {
        ATT_PutMsg(prim);
    }
}

#endif /* INSTALL_ATT_MODULE */
