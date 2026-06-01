/*******************************************************************************
Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

\brief  This file handles L2CAP connection managment.
*******************************************************************************/

#include "l2cap_private.h"


/*! \brief Retrieve connection handle instance for HCI connection handle.

    This function returns the connection handle instance for the specified
    HCI connection handle. It walks down the linked list of CHCB instances
    looking for the first instance with matching HCI handle.

    \return Pointer to CHCB instance, or NULL if no matching instance exists.
*/

L2CAP_CHCB_T *CHME_GetConHandle(hci_connection_handle_t handle)
{
    DM_ACL_T *p_acl;

    if ((p_acl = dm_acl_find_by_handle(handle)) != NULL)
        return CH_GET_CHCB(p_acl);

    return NULL;
}

