/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#if defined(INSTALL_ATT_MODULE) && defined(INSTALL_EATT)

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_read_multi_var_rsp
 *
 *  DESCRIPTION
 *      Build and send an ATT_READ_MULTI_VAR_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      This will take ownership of the block pointed to by value.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_read_multi_var_rsp(
    phandle_t phandle,
    uint16_t cid,
    att_result_t result,
    uint16_t error_handle,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_READ_MULTI_VAR_RSP_T *prim = zpnew(ATT_READ_MULTI_VAR_RSP_T);

    prim->type = ATT_READ_MULTI_VAR_RSP;
    prim->phandle = phandle;
    prim->cid = cid;
    prim->result = result;
    prim->error_handle = error_handle;
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

#endif
