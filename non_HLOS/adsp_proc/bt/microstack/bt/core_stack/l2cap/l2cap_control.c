/*******************************************************************************

Copyright (C) 2008 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

\brief  Miscellenous control functions for handling PSMs, fixed
        channels, connectionless rx/tx setup, etc.  We also control
        creation of CIDCBs for both incoming and outgoing connections.
*******************************************************************************/

#include "l2cap_private.h"

/*! \brief L2CAP master control block

    This is the only global variable left in L2CAP. It's
    auto-initialised to zero on-chip. When on-host, the MCB_Init()
    function makes sure it's zero-initialised.
*/
MCB_T mcb;


/*! \brief Initialise L2CAP control.

    This function is called to initialise the L2CAP control module.
*/
void MCB_Init(void)
{
#ifdef BUILD_FOR_HOST
    /* Zero initialise the only global variable we've got */
    memset(&(mcb), 0, sizeof(MCB_T));
#endif
}




