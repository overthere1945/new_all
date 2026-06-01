/*******************************************************************************

Copyright (C) 2019 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"
#include INC_DIR(l2cap,l2cap_auto.h)
#include INC_DIR(l2cap,l2cap_cidme.h)

/*! \brief Sets connection context and phandle in L2cap module.

    Updates the connection context value stored in CIDCB_T & L2CAUTO_INSTANCE_T
    for the specified connection identifier. Usually, this API is called
    in a device where the L2CAP connection stuctures are unmarshalled and the
    connection context is not updated using L2CA_AUTO_TP_CONNECT_REQ primitve.

    \param cid Conn id for which context and phandle are initialized.
    \param p_handle Application phandle to be updated for the specified cid.
    \param context Application task context to be updated for the specified cid.
    \return TRUE for successful initialization, othewise FALSE.
*/
#ifdef TRAPSET_BLUESTACK
bool_t L2CA_OverrideConnCtx(l2ca_cid_t cid, phandle_t p_handle, context_t context)
{
    CIDCB_T *cidcb;
    L2CAUTO_INSTANCE_T *instance;

    /* Look for cidcb and auto instance that matches the cid */
    if ((cid != L2CA_CID_INVALID) && 
        (cidcb = CIDME_GetCidcb(cid)) != NULL &&
        (instance = L2CAUTO_FindInstance(cid, NULL, 0, 0)) != NULL)
    {
        cidcb->context = context;
        cidcb->p_handle = p_handle;
#ifdef INSTALL_L2CAP_ROUTER_SUPPORT
        cidcb->p_handle_data = cidcb->p_handle;
#endif
        instance->con_ctx = context;
        instance->phandle = p_handle;
        return TRUE;
    }

    return FALSE;
}

#endif
