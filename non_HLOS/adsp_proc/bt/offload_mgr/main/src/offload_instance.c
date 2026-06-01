/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_instance.c
===========================================================================*/
/**
 * @file offload_instance.c
 * @brief Offload instance management functions.
 *
 * This file contains functions for managing offload instances, including finding
 * instances by socket ID, pseudo ID, and instance ID.
 */

 /*============================================================================*
                           INCLUDE FILES
*============================================================================*/
#include "offload_instance.h"


/**
 * @brief Array of offload instances.
 *
 * This array stores all the offload instances, with a maximum size of
 * MAX_OFFLOAD_INSTANCES.
 */
offload_instance_t offload_instances[MAX_OFFLOAD_INSTANCES];


/*===========================================================================
							FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
FUNCTION      offload_instance_find_by_socket_id
===========================================================================*/
/**
 * @brief Find an offload instance by socket ID.
 *
 * This function searches for an offload instance with the given socket ID and
 * returns a pointer to the instance if found.
 *
 * @param socket_id The socket ID to search for.
 * @return A pointer to the offload instance if found, or NULL if not found.
 */
const offload_instance_t* offload_instance_find_by_socket_id(socket_id_t socket_id) {
    offload_instance_t *offload_instance = NULL;
    for (int i = 0; i < MAX_OFFLOAD_INSTANCES; i++) {
        if (offload_instances[i].offload_data.socket_data.socket_id == socket_id) {
            return &offload_instances[i];
        }
    }
    return NULL; /* todo */
}

/*===========================================================================
FUNCTION      offload_instance_find_by_pseudo_id
===========================================================================*/
/**
 * @brief Find an offload instance by pseudo ID.
 *
 * This function searches for an offload instance with the given pseudo ID and
 * returns a pointer to the instance if found.
 *
 * @param pseudo_id The pseudo ID to search for.
 * @return A pointer to the offload instance if found, or NULL if not found.
 */
const offload_instance_t* offload_instance_find_by_pseudo_id(uint32_t pseudo_id) {
    offload_instance_t *offload_instance = NULL;
    for (int i = 0; i < MAX_OFFLOAD_INSTANCES; i++) {
        if (offload_instances[i].offload_data.socket_data.pseudo_id == pseudo_id) {
            return &offload_instances[i];
        }
    }
    return NULL;
}

/*===========================================================================
FUNCTION      offload_instance_find_id_by_socket_id
===========================================================================*/
/**
 * @brief Find the instance ID of an offload instance by socket ID.
 *
 * This function searches for an offload instance with the given socket ID and
 * returns the instance ID if found.
 *
 * @param socket_id The socket ID to search for.
 * @return The instance ID if found, or 0 if not found.
 */

const offload_instance_id_t offload_instance_find_id_by_socket_id(socket_id_t socket_id) {
    for (int i = 0; i < MAX_OFFLOAD_INSTANCES; i++) {
        if (offload_instances[i].offload_data.socket_data.socket_id == socket_id) {
            return offload_instances[i].offload_instance_id;
        }
    }
    return 0; // Return an invalid ID if not found
}

/*===========================================================================
FUNCTION      offload_instance_find_by_id
===========================================================================*/
/**
 * @brief Find an offload instance by instance ID.
 *
 * This function searches for an offload instance with the given instance ID and
 * returns a pointer to the instance if found.
 *
 * @param id The instance ID to search for.
 * @return A pointer to the offload instance if found, or NULL if not found.
 */
const offload_instance_t* offload_instance_find_by_id(offload_instance_id_t id) {
    for (int i = 0; i < MAX_OFFLOAD_INSTANCES; i++) {
        if (offload_instances[i].offload_instance_id == id) {
            return &offload_instances[i];
        }
    }
    return NULL;
}

/*===========================================================================
FUNCTION      offload_instance_find_id_by_gatt_session_id
===========================================================================*/
/**
 * @brief Finds the offload instance ID by GATT session ID.
 * @details Searches all offload instances for a GATT type instance containing the specified session ID.
 *          Returns the instance ID if found, otherwise returns MAX_OFFLOAD_INSTANCES as an invalid ID.
 * @param[in] session_id GATT session ID to search for.
 * @return offload_instance_id_t Instance ID if found, otherwise MAX_OFFLOAD_INSTANCES.
 */
const offload_instance_id_t offload_instance_find_id_by_gatt_session_id(gatt_session_id_t session_id) 
{
    for (int i = 0; i < MAX_OFFLOAD_INSTANCES; i++) 
    {
        if (offload_instances[i].offload_instance_type  == OFFLOAD_INSTANCE_TYPE_GATT 
            && offload_instances[i].offload_data.gatt_session)
        {
            if (offload_instances[i].offload_data.gatt_session->session_id == session_id)
            {
                return offload_instances[i].offload_instance_id;
            }
        }
    }
    return MAX_OFFLOAD_INSTANCES; // Return an invalid ID if not found
}

/*===========================================================================
FUNCTION      offload_instance_find_by_gatt_session_id
===========================================================================*/
/**
 * @brief Finds the offload instance by GATT session ID.
 * @details Searches all offload instances for a GATT type instance containing the specified session ID.
 *          Returns a pointer to the instance if found, otherwise returns NULL.
 * @param[in] session_id GATT session ID to search for.
 * @return const offload_instance_t* Pointer to the instance if found, otherwise NULL.
 */
const offload_instance_t* offload_instance_find_by_gatt_session_id(gatt_session_id_t session_id) 
{
    for (int i = 0; i < MAX_OFFLOAD_INSTANCES; i++) 
    {
        if (offload_instances[i].offload_instance_type  == OFFLOAD_INSTANCE_TYPE_GATT 
            && offload_instances[i].offload_data.gatt_session)
        {
            if (offload_instances[i].offload_data.gatt_session->session_id == session_id)
            {
                return &offload_instances[i];
            }
        }
    }
    return NULL; /* todo */
}


