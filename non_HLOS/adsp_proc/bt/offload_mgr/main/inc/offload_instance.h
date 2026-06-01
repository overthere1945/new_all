/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_instance.h
===========================================================================*/
/**
 * @file offload_instance.h
 * @brief This file has the primary offload_instance code.
 *
 * Operations available on offload_instance:
 * 1. get endpoint using socket id
 * 2. get pseudo id using socket id
 * 3. get offload_instance type using socket id
 * 4. This list can be expanded to read any other offload_instance properties.
 *
 * It is advised to not directly acquire the pointer to offload_instance. 
 * It is safe to use getters to access any of the offload properties properties.
 */


 #ifndef OFFLOAD_INSTANCE_H
 #define OFFLOAD_INSTANCE_H
 
 /*===========================================================================
                             INCLUDE FILES
 ===========================================================================*/
 #include "sm.h"
 #include "stdint.h"
 #include "socket_mgr.pb.h"
 #include "socket.h"
 #include "offload_mgr_sm.h"
 #include "profile_mgr_gatt.h"
 
 /*===========================================================================
                             MACRO DEFINITIONS
 ===========================================================================*/
 /** 
  * @def MAX_OFFLOAD_INSTANCES
  * @brief maximum number of offload_instances across offload manager
  */
 #define MAX_OFFLOAD_INSTANCES 9 /** 2 (lecoc sockets)
                                   + 2 (rfcomm sockets)
                                   + 5 (GATT_OFFLOAD_MAX_SESSIONS) */

 typedef enum
{
  OFFLOAD_INSTANCE_TYPE_SOCKET,
  OFFLOAD_INSTANCE_TYPE_GATT,
}
offload_instance_type_t;
 /*===========================================================================
                             TYPE DEFINITIONS
 ===========================================================================*/

 typedef uint8_t offload_instance_id_t;
 
 /**
  * @enum Socket_State
  * @brief Enumeration of offload_instance states.
  *
  * @details possible states of offload_instance. 
  *
  * @note The states are defined to handle different stages of the offload_instance open and close procedures.
  */
 typedef enum {
     Idle = (SMAnyState + 1),  /**< Idle state */
     OffloadReq,            /**< Offload instance open request */
     OffloadToMicrostackSuccess, /**< Offload open microstack success */
     UnOffloadReqToMicrostack,           /**< Unoffload request */
     UnOffloadReqFromHAL, /**< Unoffload request from HAL */

     OffloadFailure,        /**< Offload failure */
     OffloadToMicroappSuccess,/**< Offload procedure completes successfully */
     OffloadToMicroappFailure,/**< Offload to microapp failure */
     UnoffloadSuccess,             /**< Unoffload success */

 } Offload_State_t;
 
 /**
  * @struct endpoint_t
  * @brief Structure representing an endpoint.
  * @details This structure defines an endpoint with a hub ID and an ID. Ensure alignment with Google Proto.
  * @note The hub_id and id should be aligned with the corresponding fields in Google Proto.
  */
 typedef struct endpoint {
     uint64_t hub_id; /**< Hub ID, aligned with Google Proto */
     uint64_t id; /**< Endpoint ID */
 } endpoint_t;
 
 
typedef union offload_data{
    socket_data_t socket_data;
    gatt_session_t *gatt_session;
} offload_data_t;
 
 /**
  * @struct offload_instance_t
  * @brief Structure of a offload_instance.
  * @details Base offload_instance structure
  * @note state_ctx is present will be put in DDR and might not be 
  *       accessible in some low power modes.
  */
 typedef struct offload_instance {
     
     offload_instance_id_t offload_instance_id; /**< ID of the offload_instance */  
     offload_instance_type_t offload_instance_type; /**< Type of the offload_instance */
     Offload_State_t offload_state; /**< Current state of the offload_instance */
     endpoint_t ep_id; /**< Endpoint ID associated with the offload_instance */
     offload_data_t offload_data;

     offload_mgr_sm_state_ctx_t *state_ctx; /**< State context for the offload manager */
 } offload_instance_t;
 

 /*===========================================================================
                             GLOBAL FUNCTION DEFINITIONS
 ===========================================================================*/
 /*===========================================================================
 FUNCTION      offload_instance_find_by_pseudo_id
 ===========================================================================*/
 /**
  * @brief Find a offload_instance by its pseudo ID.
  *
  * @param[in] pseudo_id The pseudo ID of the offload_instance to find.
  * @return Pointer to the offload_instance_t structure if found, otherwise NULL.
  */
 const offload_instance_t* offload_instance_find_by_pseudo_id(uint32_t pseudo_id);
 const offload_instance_t* offload_instance_find_by_socket_id(socket_id_t socket_id);
 const offload_instance_id_t offload_instance_find_id_by_socket_id(socket_id_t socket_id);
 const offload_instance_t *offload_instance_find_by_id(offload_instance_id_t id);
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
const offload_instance_id_t offload_instance_find_id_by_gatt_session_id(gatt_session_id_t session_id);

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
const offload_instance_t* offload_instance_find_by_gatt_session_id(gatt_session_id_t session_id);
 
 /*===========================================================================
 FUNCTION      offload_instance_init
 ===========================================================================*/
 /**
  * @brief Initialize the offload instances.
  *
  * @details This function initializes the offload instances.
  */
 void offload_instance_init(void);
 
 
 #endif /* OFFLOAD_INSTANCE_H */
 
