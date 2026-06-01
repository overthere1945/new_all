#ifndef QAPI_BT_LE_SCAN_H
#define QAPI_BT_LE_SCAN_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lpai_bt_app_mgr_client_interface.h"
#include "lpai_bt_le_scan.h"


/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/

#define QAPI_REG_SCANNER_CFM      0x01
#define QAPI_UNREG_SCANNER_CFM    0x02
#define QAPI_ENABLE_SCANNER_CFM   0x03
#define QAPI_DISABLE_SCANNER_CFM  0x04
#define QAPI_EXT_ADV_REPORT_IND   0x05


/*===========================================================================
                      Type Definitions
===========================================================================*/

/**
 * @struct qapi_registerScannerCfm
 * Register Scanner Command Confirm Structure received on AWM from ADSP
 */
typedef struct qapi_registerScannerCfm{
    uint16_t type;
    uint16_t resultCode;
    uint8_t scanHandle;
}qapi_registerScannerCfm_t;


/**
 * @struct qapi_unregisterScannerCfm_t
 * Unregister Scanner Command Confirm Structure received on AWM from ADSP
 */
typedef struct qapi_unregisterScannerCfm{
    uint16_t type;
    uint16_t resultCode;
    uint8_t scanHandle;
}qapi_unregisterScannerCfm_t;



/**
 * @struct qapi_enableScannerCfm_t
 * Response for Enable/Disable Scan Request for a Scanner
 */
typedef struct qapi_enableScannerCfm{
    uint16_t type;
    uint16_t resultCode;
    uint8_t scanHandle;
}qapi_enableScannerCfm_t;


/**
 * @struct extAdvReportInd_t
 * Extended Advertisement Indication Report Received from ADSP once scan is enabled
 */
typedef struct qapi_extAdvReportInd{
    uint16_t type;
    uint16_t eventType;
    uint8_t advSid;
    int8_t txPower;
    int8_t rssi;
    typed_bd_addr_t currentAddr;
    typed_bd_addr_t directAddr;
}qapi_extAdvReportInd_t;



/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/


/**
 * @brief This method is used by client on AWM to register a scanner on ADSP
 * @param[in] advFilter     Adv Filter Value to be used while registration of a scanner 
 * @return    None
 */
void qapi_register_scanner(uint16_t advFilter);

/**
 * @brief This method is used unregister an registerd scanner
 * @param[in] scanHandle  Scan Handle for which unregistration is to be performed
 * @return    None
 */
void qapi_unregister_scanner(uint8_t scanHandle);

/**
 * @brief This method is used to enable or disable scan for a particular scanner
 * @param[in] enable       Bool to specify if scan is to be enabled or disabled
 * @param[in] scanHandle   Scan Handle for which operation is to be performed
 * @param[in] duration     Duaration for which scan is to be enabled , ignored when scan is disabled
 * @return    None
 */
void qapi_enable_scan(bool enable , uint8_t scanHandle , uint16_t duration);



/**
 * @brief Handler Function to Handle LE SCAN QAPI EVENTS
 * @param[in] opcode       Opcode to identify Incoming Event
 * @param[in] appDataLen   Data Length of the Incoming Event
 * @param[in] appData      Pointer to Actual Data of the incoming event
 * @return    None
 */
void qapi_lescan_evt_handler(uint16_t opcode,uint16_t appDataLen , void *appData);



#endif /**QAPI_BT_LE_SCAN_H */