/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_prepare_write_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_PREPARE_WRITE_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Takes ownership of block pointed to by value.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_prepare_write_req(
    phandle_t phandle,
    uint16_t cid,
    uint16_t handle,
    uint16_t offset,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_PREPARE_WRITE_REQ_T *prim = zpnew(ATT_PREPARE_WRITE_REQ_T);

    prim->type = ATT_PREPARE_WRITE_REQ;
    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;
    prim->offset = offset;
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
