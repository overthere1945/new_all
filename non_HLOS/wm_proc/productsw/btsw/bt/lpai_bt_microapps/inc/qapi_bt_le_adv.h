#ifndef QAPI_BT_LE_ADV_H
#define QAPI_BT_LE_ADV_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lpai_bt_app_mgr_client_interface.h"
#include "lpai_bt_le_adv.h"


/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/

#define QAPI_REGISTER_ADV_SET_CFM     0x01
#define QAPI_UNREGISTER_ADV_SET_CFM   0x02
#define QAPI_SET_ADV_PARAMS_CFM       0x03
#define QAPI_SET_ADV_DATA_CFM         0x04
#define QAPI_SET_RANDOM_ADDRESS_CFM   0x05
#define QAPI_START_ADVERTISEMENT_CFM  0x06
#define QAPI_STOP_ADVERTISEMENT_CFM   0x07



/*===========================================================================
                      Type Definitions
===========================================================================*/


/**
 * @brief Structure to confirm the registration of an advertising set.
 * @param resultCode Result code for registration of an advertising set
 * @param advHandle Advertising handle
 */
typedef struct qapi_registerAdvSetCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}qapi_registerAdvSetCfm_t;



/**
 * @brief Structure to confirm the unregistration of an advertising set.
 * @param resultCode Result code for unregistration of an advertising set
 * @param advHandle Advertising handle
 */
typedef struct qapi_unregisterAdvSetCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}qapi_unregisterAdvSetCfm;


/**
 * @brief Structure to confirm the setting of advertisement parameters.
 * @param resultCode Result code for unregistration of an advertising set
 * @param advHandle Advertising handle
 */
typedef struct qapi_setAdvParamsCfm{
    uint16_t resultCode;
    uint8_t  advHandle;
    int8_t   selected_tx_pwr;
}qapi_setAdvParamsCfm_t;


/**
 * @brief Structure to confirm the setting of advertisement data.
 * @param resultCode Result code for unregistration of an advertising set
 * @param advHandle Advertising handle
 */
typedef struct qapi_setAdvDataCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}qapi_setAdvDataCfm_t;


/**
 * @brief Structure to confirm setting of Random Address for an Advertisement Set.
 * @param resultCode Result code for unregistration of an advertising set
 * @param advHandle  Advertising handle
 * @param deviceAddress Device Address which is set
 */
typedef struct qapi_setRandomAddressCfm{
    uint16_t  resultCode;
    uint8_t   advHandle;
    bd_addr_t  randomAddr;
}qapi_setRandomAddressCfm_t;


/**
 * @brief Structure to confirm the start of an advertisement.
 * @param resultCode Result code of the start process
 * @param advBits advBits
 * @param maxAdvSets Maximum number of advertising sets
 */
typedef struct qapi_startAdvCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}qapi_startAdvCfm_t;
 

/**
 * @brief Structure to confirm the stop of an advertisement.
 * @param appHandle Application handle
 * @param status Status of the stop process
 */
typedef struct qapi_stopAdvCfm{
    uint16_t resultCode;
    uint8_t advHandle;
}qapi_stopAdvCfm_t;




/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
/**
 * @brief Handler Function to Handle LE ADV QAPI EVENTS
 * @param[in] opcode       Opcode to identify Incoming Event
 * @param[in] appDataLen   Data Length of the Incoming Event
 * @param[in] appData      Pointer to Actual Data of the incoming event
 * @return    None
 */
void qapi_leadv_evt_handler(uint16_t opcode,uint16_t appDataLen , void *appData);


/**
 * @brief This method is used register an Advetisement Set on ADSP
 * @param[in] None
 * @return    None
 */
void qapi_register_advSet(void);


/**
 * @brief This method is used register an Advetisement Set on ADSP
 * @param[in] advHandle Advertisement Handle which needs to be unregistered
 * @return    None
 */
void qapi_unregister_advSet(uint8_t advHandle);


/**
 * @brief This method is used to set advertisement params for a registered adv set
 * @param[in] advParams Advertisment Parameters Structure whith values for adv params
 * @return    None
 */
void qapi_set_advParams(setAdvParams_t advParams);

/**
 * @brief This method is used to set advertisement params for a registered adv set
 * @param[in] advData  Advertisement Data which is to be set for a particular advertisement set
 * @return    None
 */
void qapi_set_advData(setAdvData_t advData);


/**
 * @brief This method is used to set random address for an registered adv set
 * @param[in] advData  Advertisement Data which is to be set for a particular advertisement set
 * @return    None
 */
void qapi_set_random_address(setRandomAddress_t randomAddr);

/**
 * @brief This method is used to disbale advertising for an advertisment set
 * @param[in] advHandle   Advertisement Handle for which advertisement is to be disbaled
 * @return    None
 */
void qapi_disbale_advertisment(uint8_t advHandle);

/**
 * @brief This method is used to enable advertising for an advertisment set
 * @param[in] advHandle   Advertisement Handle for which advertisement is to be enabled
 * @return    None
 */
void qapi_enable_avertisement(uint8_t advHandle);

#endif /**QAPI_BT_LE_ADV_H */