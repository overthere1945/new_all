/*******************************************************************************

Copyright (C) 2009 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "rfcomm_private.h"


#ifdef INSTALL_RFCOMM_MODULE

/*! \brief Handler function for RFCOMM primitives. 
 
     \param control_data - Pointer to rfcomm instance data.
*/
void rfc_iface_handler(void **control_data)
{
    uint16_t mi;
    void *rx_prim;
    RFC_CTRL_PARAMS_T  rfc_params = {NULL, NULL, NULL, NULL, 0};

    rfc_params.rfc_ctrl = (RFC_CTRL_T *)*(control_data);

    /* Get message from interface queue */
    if (!get_message(RFCOMM_IFACEQUEUE, &mi, &rx_prim))
        return;

    switch (mi)
    {
        case RFCOMM_PRIM: 
            rfc_host_handler(&rfc_params,
                             (RFCOMM_UPRIM_T *)rx_prim);
            break;

        case L2CAP_PRIM:
            rfc_l2cap_handler(&rfc_params,
                              (L2CA_UPRIM_T *)rx_prim);
            break;

        default:
            /* Unexpected primitive type.
            */ 
            break;
    }
}


#endif /* INSTALL_RFCOMM_MODULE */
