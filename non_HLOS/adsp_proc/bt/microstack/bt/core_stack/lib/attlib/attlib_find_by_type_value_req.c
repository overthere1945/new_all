/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_find_by_type_value_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_FIND_BY_TYPE_VALUE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      This will take ownership of the block pointed to by value.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_find_by_type_value_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t start,
    uint16_t end,
    uint16_t uuid,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_FIND_BY_TYPE_VALUE_REQ_T *prim = zpnew(ATT_FIND_BY_TYPE_VALUE_REQ_T);

    prim->type = ATT_FIND_BY_TYPE_VALUE_REQ;
    prim->phandle = phandle;
    prim->cid = cid;
    prim->start = start;
    prim->end = end;
    prim->uuid = uuid;
    prim->size_value = size_value;
    prim->value = value;

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
