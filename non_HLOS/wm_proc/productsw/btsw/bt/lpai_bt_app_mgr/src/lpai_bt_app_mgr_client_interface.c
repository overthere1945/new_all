/*************************************************************************
 * @file     lpai_bt_app_mgr_client_interface.c
 * @brief    LPAI BT App Manager Handler source file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <zephyr/sys/printk.h>
#include <stringl.h>
#include "lpai_bt_app_mgr_client_interface.h"



/*===========================================================================
                        EXTERNAL DATA DECLARATIONS
===========================================================================*/
extern appMgrContext_t appMgrCtx;

/*===========================================================================
                        PUBLIC/GLOBAL DATA DEFINITIONS
===========================================================================*/



/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/

/**
 * @brief Method for Client end points on AWM to register their Callbacks Succesfully with the LPAI BT APP Manager.
 * The registered client callbacks are then invoked once appropriate BT events intended for a specific client end point are received from
 * ADSP over Glink.The clients are also responsible for sending additional context information which is maintained by LPAI Bt App
 * Manager which is further used to identify the approriate client's callback to be invoked once an event intended for a specific client
 * is received from ADSP.
 * @param[in]  appCb                            Application Callback Function which is invoked once event is received for a specific client
 * @param[in]  appMetaData                      Application Client Additional Meta Data to be maintained by Bt App Manager.
 * @param[in]  endPointDetails                  Application End Point Details to currectly identify an endpoint and send information to ADSP once it is requested
 * @param[out] None
 * @return     app_registration_status_code_t   Application registration Status to client to indicate if registration was successful
 */
app_registration_status_code_t lpai_bt_app_mgr_register_endpt_client(EndPointAppCb appCb , endPointDetails_t endPointDetails)
{
    uint16_t size = sizeof(endPointDetails_t);
    if(appMgrCtx.numEndPointsRegisterd < (MAX_END_POINTS_SUPPORTED))
    {
        if(appCb != NULL)
        {
            for(uint8_t idx = 0; idx < MAX_END_POINTS_SUPPORTED; idx++)
            {
                if(appMgrCtx.appContext[idx].appCb == appCb && appMgrCtx.appContext[idx].endPointDetail.endPointId.epId == endPointDetails.endPointId.epId)
                {
                    return APP_REGISTRATION_ALREADY_REGISTERED;
                }
            }

            for(uint8_t idx = 0; idx < MAX_END_POINTS_SUPPORTED; idx++)
            {
                if(appMgrCtx.appContext[idx].appCb == NULL)
                {
                    appMgrCtx.appContext[idx].appCb = appCb;
                    memscpy(&(appMgrCtx.appContext[idx].endPointDetail),size,&endPointDetails,size);
                    appMgrCtx.numEndPointsRegisterd += 1;
                    return APP_REGISTRATION_SUCCESS;
                }
            }
        }
    }
    else
    {
        printk("Max Number of Apps Already registered");
        return APP_REGISTRATION_MAX_LIMIT_REACHED;
    }

    return APP_REGISTRATION_FAILURE;
}



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
app_registration_status_code_t lpai_bt_app_mgr_register_microapp(uint16_t appHandle , MicroAppCallback appCb)
{
	if(appMgrCtx.numMicroAppsRegistered < (MAX_MICROAPPS_SUPPORTED))
	{
		if(appCb != NULL)
		{
			for(uint8_t idx = 0; idx < MAX_MICROAPPS_SUPPORTED; idx++)
			{
				if(appMgrCtx.microAppContext[idx].appCb == appCb && appMgrCtx.microAppContext[idx].appHandle == appHandle)
				{
					 return APP_REGISTRATION_ALREADY_REGISTERED;
				}
			}

		}
		for(uint8_t idx = 0; idx < MAX_MICROAPPS_SUPPORTED; idx++)
		{
			if(appMgrCtx.microAppContext[idx].appCb == NULL)
			{
				appMgrCtx.microAppContext[idx].appHandle = appHandle;
				appMgrCtx.microAppContext[idx].appCb = appCb;
				appMgrCtx.numMicroAppsRegistered += 1;
				return APP_REGISTRATION_SUCCESS;
			}
		}
	}

	return APP_REGISTRATION_MAX_LIMIT_REACHED;
}


/**
 * @brief Method for Microapps on AWM to unregister with the LPAI BT APP Manager.
 * Client Microapp is unregistered successfully if the given app Handle exists with Bt App Manager
 * @param[in]  appHandle                        Application Handle to identify the application
 * @pram[out]  None
 * @return     None
 */
void lpai_bt_app_mgr_unregister_microapp(uint16_t appHandle)

{
	for(uint8_t idx = 0;idx < MAX_MICROAPPS_SUPPORTED;idx++)
	{
		if(appMgrCtx.microAppContext[idx].appHandle == appHandle)
		{
			appMgrCtx.microAppContext[idx].appHandle = 0x00;
			appMgrCtx.microAppContext[idx].appCb = NULL;
            appMgrCtx.numMicroAppsRegistered -= 1;
			printk("App Successfully Unregistered\n");
			return;
		}
	}
	printk("No App with the given app handle exists\n");
}




/**
 * @brief Method for Client to Query for Current BT Status of the System
 * @param[in] None
 * @return  bt_status_t  Current BT Status
 */

 bt_status_t lpai_get_current_bt_status()
 {
    return appMgrCtx.btStatus;
 }


 /**
 * @brief Method to List Registered End Point Details
 * @param[in] None
 * @return    None
 */
void list_endpoint_details()
{
    for(uint8_t idx = 0; idx < MAX_END_POINTS_SUPPORTED; idx++)
    {
        printk("End Point Id : % " PRIu64 "\t  End Point Name : %s \n",appMgrCtx.appContext[idx].endPointDetail.endPointId.epId,appMgrCtx.appContext[idx].endPointDetail.name);
    }
}