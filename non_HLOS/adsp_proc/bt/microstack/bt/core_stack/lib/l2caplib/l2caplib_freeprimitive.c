/*******************************************************************************

Copyright (C) 2009 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Discard a configuration block
  
    Free pointers from a configuration placeholder.
 */
void L2CA_FreeConfigPtrs(L2CA_CONFIG_T *config)
{
    if (config != NULL)
    {
        bpfree(config->unknown);
        bpfree(config->qos);
        bpfree(config->flow);
        bpfree(config->flowspec);

        config->unknown = NULL;
        config->unknown_length = 0;
        config->qos = NULL;
        config->flow = NULL;
        config->flowspec = NULL;
    }
}

/*! \brief Discard a transport specific configuration block
  
    Free pointers from a configuration placeholder.
 */
void L2CA_FreeTpConfigPtrs(L2CA_TP_CONFIG_T *tpconfig)
{
    if (tpconfig != NULL)
    {
        /* L2CA_TP_CONFIG_T aligns with L2CA_CONFIG_T structure we can reuse
         the existing method to free pointers */
        L2CA_FreeConfigPtrs( (L2CA_CONFIG_T *)tpconfig);
    }
}

/*! \brief Destroy L2CAP primitive

    Destroy an L2CAP primtive including any associated MBLKs or data blocks.
    
    \param prim Pointer to L2CAP primitive to destroy.
*/
void L2CA_FreePrimitive(L2CA_UPRIM_T *prim)
{
    if (prim == NULL)
        return;

    /* Handle any special cases. We must use bpree() in downstream
     * signals to deal with VM const data. On-host this is macrofied
     * to a normal pfree() */
    switch(prim->type)
    {
        case L2CA_DATAWRITE_REQ:
            mblk_destroy(prim->l2ca_datawrite_req.data);
            break;

        case L2CA_DATAREAD_IND:
            mblk_destroy(prim->l2ca_dataread_ind.data);
            break;

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
        case L2CA_CONFIG_REQ:
            L2CA_FreeConfigPtrs(&prim->l2ca_config_req.config);
            break;

        case L2CA_CONFIG_RSP:
            L2CA_FreeConfigPtrs(&prim->l2ca_config_rsp.config);
            break;

        case L2CA_CONFIG_CFM:
            L2CA_FreeConfigPtrs(&prim->l2ca_config_cfm.config);
            break;

        case L2CA_CONFIG_IND:
            L2CA_FreeConfigPtrs(&prim->l2ca_config_ind.config);
            break;
#endif 

#ifdef INSTALL_L2CAP_CONNLESS_SUPPORT
        case L2CA_MULTICAST_REQ:
            mblk_destroy(prim->l2ca_multicast_req.data);
            break;
#endif

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
        case L2CA_PING_REQ:
            bpfree(prim->l2ca_ping_req.data);
            break;

        case L2CA_PING_CFM:
            pfree(prim->l2ca_ping_cfm.data);
            break;

        case L2CA_PING_IND:
            pfree(prim->l2ca_ping_ind.data);
            break;

        case L2CA_PING_RSP:
            bpfree(prim->l2ca_ping_rsp.data);
            break;

        case L2CA_GETINFO_CFM:
            pfree(prim->l2ca_getinfo_cfm.info_data);
            break;
#endif 
        case L2CA_RAW_DATA_REQ:
            mblk_destroy(prim->l2ca_raw_data_req.data);
            break;

        case L2CA_RAW_DATA_IND:
            mblk_destroy(prim->l2ca_raw_data_ind.data);
            break;

#ifdef INSTALL_L2CAP_FIXED_CHANNEL_SUPPORT
        case L2CA_REGISTER_FIXED_CID_REQ:
            L2CA_FreeConfigPtrs(&prim->l2ca_register_fixed_cid_req.config);
            break;
#endif /* FIXED_CHANNEL_SUPPORT */

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
        case L2CA_AUTO_CONNECT_REQ:
            bpfree(prim->l2ca_auto_connect_req.conftab);
            break;

        case L2CA_AUTO_CONNECT_RSP:
            bpfree(prim->l2ca_auto_connect_rsp.conftab);
            break;

        case L2CA_AUTO_CONNECT_CFM:
            L2CA_FreeConfigPtrs(&prim->l2ca_auto_connect_cfm.config);
            break;
#endif 

        case L2CA_AUTO_TP_CONNECT_REQ:
            bpfree(prim->l2ca_auto_tp_connect_req.conftab);
            break;

        case L2CA_AUTO_TP_CONNECT_RSP:
            bpfree(prim->l2ca_auto_tp_connect_rsp.conftab);
            break;

        case L2CA_AUTO_TP_CONNECT_IND:
            pfree(prim->l2ca_auto_tp_connect_ind.conftab);
            break;

        case L2CA_AUTO_TP_CONNECT_CFM:
            L2CA_FreeTpConfigPtrs(&prim->l2ca_auto_tp_connect_cfm.config);
            pfree(prim->l2ca_auto_tp_connect_cfm.config_ext);
            break;
    }
    
    /* Free primitive memory block */
    primfree(L2CAP_PRIM, prim);
}
