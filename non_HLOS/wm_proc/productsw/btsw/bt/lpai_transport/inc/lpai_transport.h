#ifndef _LPAI_TRANSPORT_H_
#define _LPAI_TRANSPORT_H_

/**
 * @file lpai_transport.h
 *
 * @brief lpai_transport Public Header File
 */

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "glink.h"
#include "qurt_thread.h"
//#include "qapi_types.h"
#include "qurt_signal.h"
//#define bool_t qbool_t
#include <stdbool.h>
/*===========================================================================
                    GLOBAL FUNCTION DEFINITIONS
===========================================================================*/


/*===========================================================================
FUNCTION      LPAI_Transport_Init
===========================================================================*/
/**

  function to init the memory and data structures required for Lpai Transport

  @param[in]    None   

  @return       None.

  @sideeffects  None.
*/
/*=========================================================================*/
void LPAI_Transport_Init();


/*===========================================================================
FUNCTION      LPAI_Transport_Start
===========================================================================*/
/**

  Function to start the transport thread

  @param[in]      *thread_name        Thread Name

  @param[in]      thread_priority     Thread Priority

  @param[in]      thread_stack_size   Thread Stack Size 

  @return         qurt_thread_t       Thread Id of the spawed thread

  @sideeffects  None.
*/
/*=========================================================================*/
qurt_thread_t LPAI_Transport_Start(const uint8_t *thread_name,
       uint16_t thread_priority,
       uint32_t thread_stack_size);


bool LPAI_Transport_Disable (qurt_signal_t* signal);


/*===========================================================================
FUNCTION      LPAI_Transport_SendDataFromMicroAppsToQ6
===========================================================================*/
/**

  Function to start the transport thread

  @param[in]      *ptr                Pointer to data

  @param[in]      size                Size of the data

  @return         glink_err_type      Return code from glink

  @sideeffects  None.
*/
/*=========================================================================*/
glink_err_type LPAI_Transport_SendDataFromMicroAppsToQ6(const void *ptr , size_t size);


/*===========================================================================
FUNCTION      LPAI_Transport_SendDataFromMicroAppsToHLOS
===========================================================================*/
/**

  Send data to HLOS

  @param[in]      *ptr                Pointer to data

  @param[in]      size                Size of the data

  @return         glink_err_type      Return code from glink

  @sideeffects  None.
*/
/*=========================================================================*/
glink_err_type LPAI_Transport_SendDataFromMicroAppsToHLOS(const void *ptr , size_t size);

#endif /* _LPAI_TRANSPORT_H_ */
