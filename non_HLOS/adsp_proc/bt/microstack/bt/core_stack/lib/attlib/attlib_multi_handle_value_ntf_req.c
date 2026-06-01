/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#if defined(INSTALL_ATT_MODULE) && defined(INSTALL_EATT)

/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_multi_handle_value_ntf_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_MULTI_HANDLE_VALUE_NTF_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Takes ownership of block pointed to by value.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_multi_handle_value_ntf_req(
    phandle_t phandle,
    context_t context,
    uint16_t cid,
    uint16_t size_value,
    uint8_t *value,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_MULTI_HANDLE_VALUE_NTF_REQ_T *prim = zpnew(ATT_MULTI_HANDLE_VALUE_NTF_REQ_T);

    prim->type = ATT_MULTI_HANDLE_VALUE_NTF_REQ;
    prim->phandle = phandle;
    prim->context = context;
    prim->cid = cid;
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
