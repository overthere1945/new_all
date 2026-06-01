/*******************************************************************************
Copyright (C) 2024 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "dm_acl.h"
#include "hci_arbiter_int.h"


/* Linked list of ACL connections */
DM_ACL_T *p_acl_list = NULL;

DM_ACL_T *dm_acl_find_by_handle(hci_connection_handle_t handle)
{
    DM_ACL_T *p_acl;

    handle &= HCI_CONNECTION_HANDLE_MASK;

    for (p_acl = p_acl_list; p_acl != NULL && p_acl->handle != handle;
            p_acl = p_acl->p_next)
        ;

    return p_acl;
}


DM_ACL_T *dm_acl_find_by_tbdaddr_acl_type(const TYPED_BD_ADDR_T *const addrt, PHYSICAL_TRANSPORT_T phy_trnsprt)
{
    DM_ACL_T *p_acl;

    for (p_acl = p_acl_list; p_acl != NULL 
            && (!tbdaddr_eq(&p_acl->addrt, addrt) 
                || (p_acl->transport != phy_trnsprt));
               p_acl = p_acl->p_next)
        ;

    return p_acl;
}

bool_t dm_acl_is_ble(const DM_ACL_T *p_acl)
{
    return p_acl->transport == LE_ACL;
}

DM_ACL_T *dm_acl_new(const TYPED_BD_ADDR_T *const addrt,
                     hci_connection_handle_t handle,
                     PHYSICAL_TRANSPORT_T transport)
{
    DM_ACL_T *p_new_acl;
    L2CAP_CHCB_T * chcb;
    DM_AMP_ACL_TYPE_T type = (transport == BREDR_ACL) ? DM_AMP_ACL_TYPE_BR_EDR : DM_AMP_ACL_TYPE_LE;

    CSR_LOG_TEXT_INFO((DM, 0, "Create DM ACL Handle 0x%x", handle));

    /* Get a new connection list entry */
    if ((p_new_acl = zpnew(DM_ACL_T)) != NULL)
    {
        /* Add the connection to the list */
        p_new_acl->p_next = p_acl_list;
        p_acl_list = p_new_acl;

        p_new_acl->handle = handle;
        p_new_acl->transport  = transport;
        p_new_acl->addrt = *addrt;

        p_new_acl->arbiter = HciArbAclConnected(handle, transport, *addrt);

        chcb = &p_new_acl->dm_acl_client_l2cap;
        dm_amp_reserve_credit(&(chcb->queue), type);
    }

    return p_new_acl;
}

bool dm_acl_handle_is_ble(uint16 hciHandle)
{
    DM_ACL_T *p_acl = dm_acl_find_by_handle(hciHandle);

    return ((p_acl != NULL) && dm_acl_is_ble(p_acl)) ? TRUE : FALSE;
}

bool_t dm_acl_is_connected(const DM_ACL_T *const p_acl)
{
    return p_acl != NULL;
}


void dm_acl_free(DM_ACL_T *acl)
{
    DM_ACL_T **pp_acl, *p_acl;
    
    for (pp_acl = &p_acl_list; (p_acl = *pp_acl) != NULL; pp_acl = &p_acl->p_next)
    {
        /* Check if we have found instance to remove */
        if (p_acl == acl)
        {
            /* Remove instance from list and exit loop */
            *pp_acl = p_acl->p_next;
            break;
        }
    }

    if (acl != NULL)
    {
        CSR_LOG_TEXT_INFO((DM, 0, "Free DM ACL Handle 0x%x", acl->handle));
        HciArbAclDisconnected(acl->arbiter);
        pfree(acl);
    }
}



void *dm_acl_arb_find_by_handle(hci_connection_handle_t handle)
{
    DM_ACL_T *p_acl = dm_acl_find_by_handle(handle);
    return (p_acl != NULL) ? p_acl->arbiter : NULL;
}


