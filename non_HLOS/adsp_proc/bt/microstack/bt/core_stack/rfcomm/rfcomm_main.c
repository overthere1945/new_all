/*******************************************************************************
Copyright (C) 2009 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/


#include "rfcomm_private.h"


#ifdef INSTALL_RFCOMM_MODULE

/*! \brief System initialisation of RFCOMM
 
    \param control_data - Pointer to rfcomm instance data.
*/
void rfc_init(void **control_data)
{
    RFC_CTRL_PARAMS_T  rfc_params = {NULL, NULL, NULL, NULL, 0};
    *control_data = (void *)&rfc_ctrl_allocation;

    rfc_params.rfc_ctrl = (RFC_CTRL_T *)*(control_data);
    
    rfc_params.rfc_ctrl->phandle = BMM_IFACEQUEUE; 
    rfc_params.rfc_ctrl->use_streams = FALSE;
    rfc_params.rfc_ctrl->mux_list = NULL;
    rfc_params.rfc_ctrl->last_conn_id = RFC_CONN_ID_INITIALISER;    
}

#ifdef ENABLE_SHUTDOWN
/*! \brief System de-initialisation of RFCOMM
 
    \param control_data - Pointer to rfcomm instance data.
*/
void rfc_deinit(void **control_data)
{
    RFC_CTRL_PARAMS_T  rfc_params = {NULL, NULL, NULL, NULL, 0};
    uint16_t mi;
    RFCOMM_UPRIM_T *p_prim;

    rfc_params.rfc_ctrl = (RFC_CTRL_T *)*(control_data);

    /* Empty the scheduler queue.
    */ 
    while(get_message(RFCOMM_IFACEQUEUE, &mi, (void**)&p_prim))
    {
        switch (mi)
        {
            case RFCOMM_PRIM:
                rfc_free_primitive(p_prim);
                break;

            case L2CAP_PRIM:
                L2CA_FreePrimitive((L2CA_UPRIM_T *) p_prim);
                break;

            default:
                BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
                break;
        }
    }


    /* Go through each Mux and destroy the corresponding RFCOMM session.
    */ 
    while((rfc_params.p_mux = rfc_params.rfc_ctrl->mux_list) != NULL)
    {
        while ((rfc_params.p_dlc = rfc_params.p_mux->info.mux.p_dlcs) != NULL)
        {
            rfc_channel_destroy(&(rfc_params.p_mux->info.mux.p_dlcs),
                                &(rfc_params.p_dlc));
        }

        rfc_channel_destroy(&(rfc_params.rfc_ctrl->mux_list),
                            &(rfc_params.p_mux));
    }

    *control_data = NULL;
}
#endif



#endif /* INSTALL_RFCOMM_MODULE */
