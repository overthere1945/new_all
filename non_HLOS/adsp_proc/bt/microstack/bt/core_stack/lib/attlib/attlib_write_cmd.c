/*******************************************************************************

Copyright (C) 2016 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_write_cmd
 *
 *  DESCRIPTION
 *      Build and send an ATT_WRITE_CMD primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Ownership of block pointed to by value passes to this function.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_write_cmd(
    phandle_t phandle,
    context_t context,
    uint16_t cid,
    uint16_t handle,
    uint16_t flags,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_WRITE_CMD_T *prim = zpnew(ATT_WRITE_CMD_T);

    prim->type = ATT_WRITE_CMD;
    prim->phandle = phandle;
    prim->cid = cid;
    prim->handle = handle;
    prim->context = context;
    prim->flags = flags;
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
