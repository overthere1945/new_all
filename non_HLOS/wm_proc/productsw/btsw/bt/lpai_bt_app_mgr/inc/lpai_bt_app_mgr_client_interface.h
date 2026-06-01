/**************************************************************************
 * @file     lpai_bt_app_mgr.h
 * @brief    LPAI BT APP Manager header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_APP_MGR_CLIENT_INTERFACE_H
#define LPAI_BT_APP_MGR_CLIENT_INTERFACE_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "lpai_bt_app_mgr_rpc.h"
#include "lpai_bt_state_mgr.h"

/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/

/**
 * @def MAX_END_POINTS_SUPPORTED
 * @brief Defines the Maximum number of end points to be offloaded from MSM.
 *
 * This macro sets the Maximum number of end points to be offloaded from MSM to AWM in the System.
 */
#define MAX_END_POINTS_SUPPORTED  2

/**
 * @def MAX_MICROAPPS_SUPPORTED
 * @brief Defines the Maximum number of microapps supported on AWM.
 *
 * This macro sets the Maximum number of apps supported on AWM Subsystem.
 */
#define MAX_MICROAPPS_SUPPORTED 3

/**
 * @def ENDPT_SERVICE_DESC_MAX_LEN
 * @brief Defines the maximum length for Service Descriptor of an End Point.
 */
#define ENDPT_SERVICE_DESC_MAX_LEN    30

/**
 * @def ENDPT_NAME_MAX_LEN
 * @brief Defines the maximum length for End Point Name.
 */
#define ENDPT_NAME_MAX_LEN        30



/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

/**
 * Type Definition for Registration of a Callback Function for each end point.
 * The Callback is registered by each end point instance on AWM with the BT App Manager
 * and then invoked once an event is received from ADSP or HLOS Subsystem for a specific end point
 */
typedef void (*EndPointAppCb)
(
    uint64_t endPointId,	/* Endpoint Id for which event has received */
	uint16_t eventId,          /* Event Id received for the end point */
	uint16_t appDataLen,      /* Data Length for the incoming request for End Point */
	void 	 *appData ,      /* Pointer to the incoming data for End Point */
    bool     proto_encoded  /*  Field to identify if message is Proto Encoded */
);

/**
 * Type Definition for Registration of a Callback Function for each Microapp on AWM.
 * The Callback is registered by each Microapp instance on AWM with the BT App Manager
 * and then invoked once an event is received from ADSP or HLOS Subsystem for a specific Microapp
 */
typedef void (*MicroAppCallback)
(
	uint16_t  handle,          /* Identity of the MicroApp on AWM */
	uint16_t  eventClass,     /* Message Class of the incoming message for microapp on AWM */
	void      *message,      /* Pointer to the incoming data for MicroApp on AWM */
    bool     proto_encoded  /*  Field to identify if message is Proto Encoded */
);


/**
 * @enum app_registration_status_code_t
 * App Registration Process Response Codes for End Points and MicroApps on AWM
 */
typedef enum{
    APP_REGISTRATION_SUCCESS,                /**< App Registration was Successful */
    APP_REGISTRATION_FAILURE,               /**< App Registration Failed */
    APP_REGISTRATION_MAX_LIMIT_REACHED,    /**< App Registration Limit Reached */
    APP_REGISTRATION_APP_CB_NULL,         /**< App Registration Limit Reached */
    APP_REGISTRATION_ALREADY_REGISTERED, /**< App was already registerd */
}app_registration_status_code_t;

/**
 * @enum app_protocol_type_t
 * BT Protocol Types used for End Points on AWM
 */
typedef enum{
    LECOC,     /**< RFCOMM Protocol Type */
    RFCOMM,   /**< LECOC Protocol Type */
    GATT,    /**< GATT Protocol Type */
}app_protocol_type_t;


/**
 * @struct endPointId_t
 * Type definition for End Points on AWM
 */
typedef struct endPointId{
    uint64_t hubId;     /**< Identifies each subsystem uniquely */
    uint64_t epId;     /**< Identifies each end point on a specific hub(susystem) uniquely */
}endPointId_t;

/**
 * @struct endPointService_t
 * Type definition for Service Supported by End  Points on AWM
 */
typedef struct endPointService{
    char serviceDescriptor[ENDPT_SERVICE_DESC_MAX_LEN];   /**< Service Desciptor for Service Supported by End Point */
    int32_t majorVersion;        /**< Major Version for Service Supported by End Point */
    int32_t minorVersion;       /**< Minor Version for Service Supported by End Point */
}endPointService_t;

/**
 * @struct endPointDetails_t
 * Type definition for End Points Details to be managed by BT App Manager on AWM
 */
typedef struct endPointDetails{
    endPointId_t endPointId;              /**< End Point Details to identify each end point on AWM Subsystem */
    char         name[ENDPT_NAME_MAX_LEN];               /**< Name of the End Point on AWM */
    endPointService_t endPointService;  /**< End Point Service Details supported by End point on AWM */
}endPointDetails_t;


/**
 * @struct endPointAppContext_t
 * Type definition for App Context for managing End Point Details on AWM Subsystem
 */
typedef struct appContext{
    endPointDetails_t endPointDetail;   /**< End Point Details for end points registered which are sent to ADSP during discovery */
    EndPointAppCb     appCb;          /**< Application Callback for each point registered which is invoked when an event is received for specific end point */
    uint64_t sessionId;              /**< Session Id for each end point for app/end point context trasnfer with Context Hub HAL on HLOS */
    bool isActive;                  /**< Boolean to specify if an end point is active for further operations*/
}endPointAppContext_t;

/**
 * @struct microAppCtx_t
 * Type definition for Non Offloaded Microapps on AWM Subsystem.These Apps are used for communication with BT Host on ADSP without
 * any context information available to HLOS regarding the Microapps
 */
typedef struct microAppContext{
	uint16_t appHandle;            /**< Unique Application Handle used for communciation with BT Host on ADSP*/
	MicroAppCallback appCb;       /**< Application Callback for each point registered which is invoked when an event is received for specific Microapp */
}microAppContext_t;


/**
 * @struct appMgrContext_t
 * Type definition for BT App Manager on AWM which allows for communication of end points and microapps on AWM Subsystem
 * with other entities on ADSP and HLOS SubSystems and also stores the relevant information for these microapps and end points
 * for any further communcation and use cases.
 */
typedef struct appMgrContext{
    bt_status_t btStatus;                                                 /**< Boolean to specify BT Status */
    uint8_t numEndPointsRegisterd;                                  /**< Context information specifying number of offloaded end points registered with BT App Manager */
    uint8_t numMicroAppsRegistered;                                /**< Context information specifying number of microapps registered with BT App Manager */
    endPointAppContext_t appContext[MAX_END_POINTS_SUPPORTED];    /**< PlaceHolder for Storing App Contexts for Offloaded End Points */
    microAppContext_t microAppContext[MAX_MICROAPPS_SUPPORTED];  /**< PlaceHolder for Storing App Contexts for MicroApps */
}appMgrContext_t;


/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Method for Client end points on AWM to register their Callbacks Successfully with the LPAI BT APP Manager.
 * The registered client callbacks are then invoked once appropriate BT events intended for a specific client end point are received from
 * ADSP over Glink.The clients are also responsible for sending additional context information which is maintained by LPAI Bt App
 * Manager which is further used to identify the  appropriate client's callback to be invoked once an event intended for a specific client
 * is received from ADSP.
 * @param[in]  appCb                            Application Callback Function which is invoked once event is received for a specific client
 * @param[in]  endPointDetails                  Application End Point Details to correctly identify an endpoint and send information to ADSP once it is requested
 * @param[out] None
 * @return     app_registration_status_code_t   Application registration Status to client to indicate if registration was successful
 */
app_registration_status_code_t lpai_bt_app_mgr_register_endpt_client(EndPointAppCb appCb ,  endPointDetails_t endPointDetails);




/**
 * @brief Method for Microapps on AWM to register their Callbacks Successfully with the LPAI BT APP Manager.
 * The registered client callbacks are then invoked once appropriate BT events intended for a specific client end point are received from
 * ADSP over Glink.The clients are also responsible for sending an application identifier handle which is maintained by LPAI Bt App
 * Manager which is further used to identify the  appropriate client's callback to be invoked once an event intended for a specific client
 * is received from ADSP.
 * @param[in]  appHandle                        Application Handle to identify the application
 * @param[in]  appCb                            Application Callback Function which is invoked once event is received for a specific client
 * @param[out] None
 * @return     app_registration_status_code_t   Application registration Status to client to indicate if registration was successful
 */
app_registration_status_code_t lpai_bt_app_mgr_register_microapp(uint16_t appHandle , MicroAppCallback appCb);



/**
 * @brief Method for Microapps on AWM to unregister with the LPAI BT APP Manager.
 * Client Microapp is unregistered successfully if the given app Handle exists with Bt App Manager
 * @param[in]  appHandle                        Application Handle to identify the application
 * @pram[out]  None
 * @return     None
 */
void lpai_bt_app_mgr_unregister_microapp(uint16_t appHandle);



/**
 * @brief Method for Client to Query for Current BT Status of the System
 * @param[in] None
 * @return  bt_status_t  Current BT Status
 */

bt_status_t lpai_get_current_bt_status();


/**
 * @brief Method to List Registered End Point Details
 * @param[in] None
 * @return    None
 */
void list_endpoint_details();

#ifdef __cplusplus
}
#endif

#endif /*LPAI_BT_APP_MGR_CLIENT_INTERFACE_H */
