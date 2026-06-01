/*******************************************************************************

Copyright (C) 2009 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "rfcomm_lib.h"
#ifndef RFCOMM_MULTI_THREAD
/*#include INC_DIR(rfcomm,rfcomm_private.h) */
#endif
#include INC_DIR(mblk,mblk.h)

#ifdef INSTALL_RFCOMM_MODULE

#ifndef QBLUESTACK_REFACTOR

/* We cannot use INC_DIR macro until B-287884 is fixed */
#ifdef BLUESTACK_HOST_IS_APPS
#include INC_DIR(pmalloc,pmalloc.h)
#else
#include "qbl_adapter_pmalloc.h"
#endif

#endif

#ifdef BUILD_FOR_HOST

/*! \brief Build and send an RFC_INIT_REQ primitive to RFCOMM
*/ 
void rfc_init_req(phandle_t phandle)
{
    RFC_INIT_REQ_T *p_prim = zpnew(RFC_INIT_REQ_T);

    p_prim->type = RFC_INIT_REQ;
    p_prim->phandle = phandle;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_REGISTER_REQ primitive to RFCOMM 
           loc_serv_chan_req should be either RFC_INVALID_SERV_CHANNEL to allow
           automatic allocation of server channel ids or any other value to
           request a specific value. NB if the requested value is already in use
           then the next available automatic one will be allocated.
*/ 
void rfc_register_req(phandle_t phandle,
                      uint8_t   flags,
                      context_t context,
                      uint8_t   loc_serv_chan_req)
{
    RFC_REGISTER_REQ_T *p_prim = pnew(RFC_REGISTER_REQ_T);

    p_prim->type = RFC_REGISTER_REQ;
    p_prim->phandle = phandle;
    p_prim->flags = flags;
    p_prim->context = context;
    p_prim->loc_serv_chan_req = loc_serv_chan_req;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_UNREGISTER_REQ primitive to RFCOMM
*/ 
void rfc_unregister_req(uint8_t loc_serv_chan)
{
    RFC_UNREGISTER_REQ_T *p_prim = pnew(RFC_UNREGISTER_REQ_T);

    p_prim->type = RFC_UNREGISTER_REQ;
    p_prim->loc_serv_chan = loc_serv_chan;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_CLIENT_CONNECT_REQ primitive to RFCOMM
*/ 
void rfc_client_connect_req(phandle_t   phandle,
                            BD_ADDR_T   *p_bd_addr,
                            uint8_t     rem_serv_chan,
                            uint8_t     flags,
                            context_t   context,
                            uint16_t    client_security_chan,
                            uint16_t    max_payload_size,
                            uint8_t     priority,
                            uint16_t    total_credits,
                            l2ca_controller_t   remote_l2cap_control, 
                            l2ca_controller_t   local_l2cap_control,  
                            uint16_t    reserved_length,
                            uint16_t    *reserved,
                            uint8_t     modem_signal,  
                            uint8_t     break_signal,     
                            uint16_t    msc_timeout)
{
    RFC_CLIENT_CONNECT_REQ_T *p_prim = pnew(RFC_CLIENT_CONNECT_REQ_T);

    QBL_UNUSED(reserved_length);
    QBL_UNUSED(reserved);

    p_prim->type = RFC_CLIENT_CONNECT_REQ;
    p_prim->phandle = phandle;
    p_prim->bd_addr = *p_bd_addr;
    p_prim->rem_serv_chan = rem_serv_chan;
    p_prim->flags = flags;
    p_prim->context = context;
    p_prim->client_security_chan = client_security_chan;
    p_prim->max_payload_size = max_payload_size;
    p_prim->priority = priority;
    p_prim->total_credits = total_credits;
    p_prim->remote_l2cap_control = remote_l2cap_control;
    p_prim->local_l2cap_control = local_l2cap_control;
    p_prim->reserved_length = 0;
    p_prim->reserved = NULL;
    p_prim->modem_signal = modem_signal;
    p_prim->break_signal = break_signal;
    p_prim->msc_timeout = msc_timeout;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_SERVER_CONNECT_RSP primitive to RFCOMM
*/ 
void rfc_server_connect_rsp(uint8_t     flags,
                            uint16_t    conn_id,
                            RFC_RESPONSE_T   response,
                            uint16_t    max_payload_size,
                            uint8_t     priority,
                            uint16_t    total_credits,
                            l2ca_controller_t   remote_l2cap_control, 
                            l2ca_controller_t   local_l2cap_control,
                            uint8_t     modem_signal,  
                            uint8_t     break_signal,     
                            uint16_t    msc_timeout)
{
    RFC_SERVER_CONNECT_RSP_T *p_prim = pnew(RFC_SERVER_CONNECT_RSP_T);

    p_prim->type = RFC_SERVER_CONNECT_RSP;
    p_prim->flags = flags;
    p_prim->conn_id = conn_id;
    p_prim->response = response;
    p_prim->max_payload_size = max_payload_size;
    p_prim->priority = priority;
    p_prim->total_credits = total_credits;
    p_prim->remote_l2cap_control = remote_l2cap_control;
    p_prim->local_l2cap_control = local_l2cap_control;
    p_prim->modem_signal = modem_signal;
    p_prim->break_signal = break_signal;
    p_prim->msc_timeout = msc_timeout;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_DISCONNECT_REQ primitive to RFCOMM
*/ 
void rfc_disconnect_req(uint16_t conn_id)
{
    RFC_DISCONNECT_REQ_T *p_prim = pnew(RFC_DISCONNECT_REQ_T);

    p_prim->type = RFC_DISCONNECT_REQ;
    p_prim->conn_id = conn_id;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_DISCONNECT_RSP primitive to RFCOMM
*/ 
void rfc_disconnect_rsp(uint16_t conn_id)
{
    RFC_DISCONNECT_RSP_T *p_prim = pnew(RFC_DISCONNECT_RSP_T);

    p_prim->type = RFC_DISCONNECT_RSP;
    p_prim->conn_id = conn_id;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_PORTNEG_REQ primitive to RFCOMM
*/ 
void rfc_portneg_req(uint16_t conn_id,
                     bool_t request,
                     RFC_PORTNEG_VALUES_T *p_port_pars)
{
    RFC_PORTNEG_REQ_T *p_prim = zpnew(RFC_PORTNEG_REQ_T);

    p_prim->type = RFC_PORTNEG_REQ;
    p_prim->conn_id = conn_id;
    p_prim->request = request;
    if (NULL != p_port_pars)
    {
        p_prim->port_pars = *p_port_pars;
    }

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_PORTNEG_RSP primitive to RFCOMM 
*/ 
void rfc_portneg_rsp(uint16_t conn_id,
                     RFC_PORTNEG_VALUES_T *p_port_pars)
{
    RFC_PORTNEG_RSP_T *p_prim = pnew(RFC_PORTNEG_RSP_T);

    p_prim->type = RFC_PORTNEG_RSP;
    p_prim->conn_id = conn_id;
    p_prim->port_pars = *p_port_pars;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_MODEM_STATUS_REQ primitive to RFCOMM
*/ 
void rfc_modem_status_req(uint16_t conn_id,
                          uint8_t modem_signal,
                          uint8_t break_signal)
{
    RFC_MODEM_STATUS_REQ_T *p_prim = pnew(RFC_MODEM_STATUS_REQ_T);

    p_prim->type = RFC_MODEM_STATUS_REQ;
    p_prim->conn_id = conn_id;
    p_prim->modem_signal = modem_signal;
    p_prim->break_signal = break_signal;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_FC_REQ primitive to RFCOMM 
*/ 
void rfc_fc_req(BD_ADDR_T *p_bd_addr,
                RFC_FC_T  fc)
{
    RFC_FC_REQ_T *p_prim = pnew(RFC_FC_REQ_T);

    p_prim->type = RFC_FC_REQ;
    p_prim->bd_addr = *p_bd_addr;
    p_prim->fc = fc;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_DATAWRITE_REQ primitive to RFCOMM
*/ 
void rfc_datawrite_req(uint16_t conn_id,
                       uint16_t payload_length,
                       MBLK_T   *payload,
                       uint16_t rx_credits)
{
    RFC_DATAWRITE_REQ_T *p_prim = pnew(RFC_DATAWRITE_REQ_T);

    p_prim->type = RFC_DATAWRITE_REQ;
    p_prim->conn_id = conn_id;
    p_prim->payload_length = payload_length;
    p_prim->payload = payload;
    p_prim->rx_credits = rx_credits;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_DATAREAD_RSP primitive to RFCOMM
*/ 
void rfc_dataread_rsp(uint16_t conn_id)
{
    RFC_DATAREAD_RSP_T *p_prim = pnew(RFC_DATAREAD_RSP_T);

    p_prim->type = RFC_DATAREAD_RSP;
    p_prim->conn_id = conn_id;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_LINESTATUS_REQ primitive to RFCOMM
*/ 
void rfc_linestatus_req(uint16_t conn_id,
                        uint8_t error_flag,
                        RFC_LINE_STATUS_T line_status)
{
    RFC_LINESTATUS_REQ_T *p_prim = pnew(RFC_LINESTATUS_REQ_T);

    p_prim->type = RFC_LINESTATUS_REQ;
    p_prim->conn_id = conn_id;
    p_prim->error_flag = error_flag;
    p_prim->line_status = line_status;

    RFC_PutMsg(p_prim);
}

/*! \brief Build and send an RFC_TEST_REQ primitive to RFCOMM
*/ 
void rfc_test_req(BD_ADDR_T  *p_bd_addr,
                  uint16_t test_data_length,
                  MBLK_T *test_data)
{
    RFC_TEST_REQ_T *p_prim = pnew(RFC_TEST_REQ_T);

    p_prim->type = RFC_TEST_REQ;
    p_prim->bd_addr = *p_bd_addr;
    p_prim->test_data_length = test_data_length;
    p_prim->test_data = test_data;

    RFC_PutMsg(p_prim);
}

#ifdef INSTALL_AMP_SUPPORT

/*! \brief Build and send an RFC_L2CA_MOVE_CHANNEL_REQ primitive to RFCOMM
*/ 
void rfc_l2ca_move_channel_request(BD_ADDR_T           *p_bd_addr,
                                   l2ca_controller_t   remote_control,
                                   l2ca_controller_t   local_control)
{
    RFC_L2CA_MOVE_CHANNEL_REQ_T *p_prim = pnew(RFC_L2CA_MOVE_CHANNEL_REQ_T);

    p_prim->type = RFC_L2CA_MOVE_CHANNEL_REQ;
    p_prim->bd_addr = *p_bd_addr;
    p_prim->local_control = local_control;
    p_prim->remote_control = remote_control;

    RFC_PutMsg(p_prim);

}

/*! \brief Build and send an RFC_L2CA_MOVE_CHANNEL_RSP primitive to RFCOMM
*/ 
void rfc_l2ca_move_channel_response(BD_ADDR_T           *p_bd_addr,
                                    l2ca_identifier_t   identifier,
                                    RFC_RESPONSE_T      status)
{
    RFC_L2CA_MOVE_CHANNEL_RSP_T *p_prim = pnew(RFC_L2CA_MOVE_CHANNEL_RSP_T);

    p_prim->type = RFC_L2CA_MOVE_CHANNEL_RSP;
    p_prim->bd_addr = *p_bd_addr;
    p_prim->identifier = identifier;
    p_prim->status = status;

    RFC_PutMsg(p_prim);

}

#endif

/*! \brief Build and send an RFC_CONFIG_REQ primitive to RFCOMM
*/ 
void rfc_config_req(uint16_t conn_id, uint16_t mux_disc_delay)
{
    RFC_CONFIG_REQ_T *p_prim = pnew(RFC_CONFIG_REQ_T);

    p_prim->type = RFC_CONFIG_REQ;
    p_prim->conn_id = conn_id;
    p_prim->mux_disc_delay = mux_disc_delay;

    RFC_PutMsg(p_prim);
}

#endif

/*! \brief Override phandle & connection context for the given conn_id & it's mux.

    Normally used when the "p_mux" & "p_dlc" are allocated as part of 
    unmarhsalling and not as a result of Rfcomm client connection request from 
    the application. 

    Note: This function returns FALSE for RFCOMM_MULTI_THREAD.
*/
bool_t rfc_overrider_context(uint16_t conn_id, phandle_t phandle, context_t context)
{
    QBL_UNUSED(conn_id);
    QBL_UNUSED(phandle);
    QBL_UNUSED(context);
    return FALSE;
}

#endif /* INSTALL_RFCOMM_MODULE */

/*! \brief Free an up or down stream RFCOMM primitive
 
    Taking into account any special considerations for destroying data
    associated with the prim.
 
    \param p_uprim - pointer to the primitive to be destroyed
*/
void rfc_free_primitive(RFCOMM_UPRIM_T *p_uprim)
{
    if (NULL == p_uprim)
    {
        return;
    }

    switch (p_uprim->type)
    {
        case RFC_CLIENT_CONNECT_REQ:
        {
            RFC_CLIENT_CONNECT_REQ_T *rfc_prim;

            rfc_prim = (RFC_CLIENT_CONNECT_REQ_T *) p_uprim;
            bpfree(rfc_prim->reserved);
            break;
        }

        case RFC_DATAWRITE_REQ:
        {
            RFC_DATAWRITE_REQ_T *rfc_prim;

            rfc_prim = (RFC_DATAWRITE_REQ_T *) p_uprim;
            mblk_destroy((MBLK_T*) rfc_prim->payload);
            break;
        }

        case RFC_TEST_REQ:
        {
            RFC_TEST_REQ_T *rfc_prim;

            rfc_prim = (RFC_TEST_REQ_T *) p_uprim;
            mblk_destroy(rfc_prim->test_data);
            break;
        }

        case RFC_DATAREAD_IND:
        {
            RFC_DATAREAD_IND_T *rfc_prim;

            rfc_prim = (RFC_DATAREAD_IND_T *) p_uprim;
            mblk_destroy(rfc_prim->payload);
            break;
        }

        case RFC_TEST_CFM:
        {
            RFC_TEST_CFM_T *rfc_prim;

            rfc_prim = (RFC_TEST_CFM_T *) p_uprim;
            mblk_destroy(rfc_prim->test_data);
            break;
        }

        default:
            break;
    }

    primfree(RFCOMM_PRIM, p_uprim);
}

/*============================================================================*
    Private Function Implementations
 *============================================================================*/
/* None */

/*============================================================================*
    End Of File
 *============================================================================*/
