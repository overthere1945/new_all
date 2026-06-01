/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "attlib_private.h"

#ifdef INSTALL_ATT_MODULE

#ifdef ATT_FLAT_DB_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      attlib_add_db_req
 *
 *  DESCRIPTION
 *      Build and send an ATT_ADD_DB_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      This will take ownership of the block pointed to by db.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void attlib_add_db_req(
    phandle_t phandle,
    uint16_t size_db,
    uint16_t *db,
    ATT_UPRIM_T **pp_prim
    )
{
    ATT_ADD_DB_REQ_T *prim = zpnew(ATT_ADD_DB_REQ_T);

    prim->type = ATT_ADD_DB_REQ;
    prim->phandle = phandle;
    prim->size_db = size_db;
    prim->db = db;

    if (pp_prim)
    {
        *pp_prim = (ATT_UPRIM_T *) prim;
    }
    else
    {
        ATT_PutMsg(prim);
    }
}
#endif /* ATT_FLAT_DB_SUPPORT */

#endif /* INSTALL_ATT_MODULE */
