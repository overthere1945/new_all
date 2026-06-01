/*******************************************************************************

Copyright (C) 2009 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifndef _RFCOMM_LIB_H_
#define _RFCOMM_LIB_H_


#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include "qbl_adapter_pmalloc.h"
#include "qbl_adapter_scheduler.h"
#include INC_DIR(common,common.h)
#include INC_DIR(bluestack,rfcomm_prim.h)
#include INC_DIR(mblk,mblk.h)

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Build and send an RFC_INIT_REQ primitive to RFCOMM
 
    \param phandle - protocol handle used to determine the default message
                     destination
*/ 
void rfc_init_req(phandle_t phandle);

/*! \brief Build and send an RFC_REGISTER_REQ primitive to RFCOMM
 
    \param phandle - protocol handle for callback
    \param flags - Additional options that can be set
    \param context - user specified context used in upstream server channel
                     prims.
    \param loc_serv_chan_req - Requested server channel number or
                               RFC_INVALID_SERV_CHANNEL to get auto generated
                               one.
*/ 
void rfc_register_req(phandle_t phandle,
                      uint8_t   flags,
                      context_t context,
                      uint8_t   loc_serv_chan_req);

/*! \brief Build and send an RFC_UNREGISTER_REQ primitive to RFCOMM
 
    \param loc_serv_chan - Local server channel to unregister
*/ 
void rfc_unregister_req(uint8_t loc_serv_chan);

/*! \brief Build and send an RFC_CLIENT_CONNECT_REQ primitive to RFCOMM
 
    \param phandle - protocol handle for callback
    \param p_bd_addr - pointer to the bluetooth address of the remote device to
                       connect to
    \param rem_serv_chan - server channel on the remote device to connect to
    \param flags - Additional options that can be set
    \param context - user specified context used in upstream server channel
                     prims.
    \param client_security_chan - Client chan id used for validating security
                                  access
    \param max_payload_size - Max data size to be used on the DLC
    \param priority - DLC priority
    \param total_credits - Total number of credits available to the peer (of max
                           payload size )
    \param remote_l2cap_control - AMP
    \param local_l2cap_control  - AMP
    \param reserved_length - For future enhancement and not currently used -
                             Value is ignored.
    \param reserved - For future enhancement and not currently used - Value is
                      ignored.
    \param modem_signal - Initial handshaking value. See modem status request
                          for details
    \param break_signal - Initial handshaking value. See modem status request
                          for details
    \param msc_timeout -  Initial handshaking value. This is the timeout in
                          milliseconds which will be allowed for the initial MSC
                          handshake prior to data being sent on the DLC.
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
                            uint16_t    msc_timeout);

/*! \brief Build and send an RFC_SERVER_CONNECT_RSP primitive to RFCOMM
 
    \param flags - Additional options that can be set
    \param conn_id - Unique connection identifier 
    \param response - Accept or decline the connection.
    \param max_payload_size - Max data size to be used on the DLC
    \param priority - DLC priority
    \param total_credits - Total number of credits available to the peer (of max
                           payload size )
    \param remote_l2cap_control - AMP
    \param local_l2cap_control  - AMP
    \param modem_signal - Initial handshaking value. See modem status request
                          for details
    \param break_signal - Initial handshaking value. See modem status request
                          for details
    \param msc_timeout -  Initial handshaking value. This is the timeout in
                          milliseconds which will be allowed for the initial MSC
                          handshake prior to data being sent on the DLC.
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
                            uint16_t    msc_timeout);

/*! \brief Build and send an RFC_DISCONNECT_REQ primitive to RFCOMM
 
    \param conn_id - Unique connection identifier 
*/ 
void rfc_disconnect_req(uint16_t conn_id);

/*! \brief Build and send an RFC_DISCONNECT_RSP primitive to RFCOMM
 
    \param conn_id - Unique connection identifier 
*/ 
void rfc_disconnect_rsp(uint16_t conn_id);

/*! \brief Build and send an RFC_PORTNEG_REQ primitive to RFCOMM
 
    \param conn_id - Unique connection identifier
    \param request - If TRUE then local host is requesting the remote side's
                     port parameters. In this case p_port_pars can be set to
                     NULL. If FALSE then the provided port parameters are to be
                     sent to the peer device.
    \param p_port_pars - Pointer to local port parameters. Must point to valid
                          values other than when issuing a request.
*/ 
void rfc_portneg_req(uint16_t conn_id,
                     bool_t request,
                     RFC_PORTNEG_VALUES_T *p_port_pars);

/*! \brief Build and send an RFC_PORTNEG_RSP primitive to RFCOMM
 
    \param conn_id - Unique connection identifier
    \param p_port_pars - Pointer to local port parameters. Must point to valid
                          values.
*/ 
void rfc_portneg_rsp(uint16_t conn_id,
                     RFC_PORTNEG_VALUES_T *p_port_pars);

/*! \brief Build and send an RFC_MODEM_STATUS_REQ primitive to RFCOMM
 
    \param conn_id - Unique connection identifier
    \param modem_signal
    \param break_signal
*/ 
void rfc_modem_status_req(uint16_t conn_id,
                          uint8_t modem_signal,
                          uint8_t break_signal);

/*! \brief Build and send an RFC_FC_REQ primitive to RFCOMM 
 
    \param p_bd_addr - pointer to the bluetooth address of the remote device
    \param fc - RFC_FC_ON or RFC_FC_OFF
*/ 
void rfc_fc_req(BD_ADDR_T *p_bd_addr,
                RFC_FC_T  fc);

/*! \brief Build and send an RFC_DATAWRITE_REQ primitive to RFCOMM
 
    \param conn_id - Unique connection identifier
    \param payload_length - length of payload
    \param payload - mblk containing the payload
    \param rx_credits - Normally the host would be expected to set rx_credits to 0, however by
                         having this field it does allow them to increase the
                         number of credits available to the peer for this
                         channel
*/ 
void rfc_datawrite_req(uint16_t conn_id,
                       uint16_t payload_length,
                       MBLK_T   *payload,
                       uint16_t rx_credits);

/*! \brief Build and send an RFC_DATAREAD_RSP primitive to RFCOMM
 
    \param conn_id - Unique connection identifier
*/ 
void rfc_dataread_rsp(uint16_t conn_id);

/*! \brief Build and send an RFC_LINESTATUS_REQ primitive to RFCOMM
 
    \param conn_id - Unique connection identifier
    \param error_flag - 0 if no errors, 1 otherwise
    \param line_status - Overun/Parity/Framing error
*/ 
void rfc_linestatus_req(uint16_t conn_id,
                        uint8_t error_flag,
                        RFC_LINE_STATUS_T line_status);

/*! \brief Build and send an RFC_TEST_REQ primitive to RFCOMM
 
    \param p_bd_addr - pointer to the bluetooth address of the remote device
    \param test_data_length - Length of test data to be transmitted
    \param test_data - Pointer to the test data to be transmitted
*/ 
void rfc_test_req(BD_ADDR_T  *p_bd_addr,
                  uint16_t test_data_length,
                  MBLK_T *test_data);

/*! \brief Free an up or down stream RFCOMM primitive
 
    \param p_uprim - pointer to the RFCOMM primitive to be freed 
*/ 
void rfc_free_primitive(RFCOMM_UPRIM_T *p_uprim);

#ifdef INSTALL_AMP_SUPPORT

/*! \brief Build and send an RFC_L2CA_MOVE_CHANNEL_REQ primitive to RFCOMM
 
    \param p_bd_addr - pointer to the bluetooth address of the remote device
    \param remote_control - Amp channel
    \param local_control - Amp channel
*/ 
void rfc_l2ca_move_channel_request(BD_ADDR_T           *p_bd_addr,
                                   l2ca_controller_t   remote_control,
                                   l2ca_controller_t   local_control);

/*! \brief Build and send an RFC_L2CA_MOVE_CHANNEL_RSP primitive to RFCOMM
 
    \param p_bd_addr - pointer to the bluetooth address of the remote device
    \param remote_control - Amp channel
    \param local_control - Amp channel
*/ 
void rfc_l2ca_move_channel_response(BD_ADDR_T           *p_bd_addr,
                                    l2ca_identifier_t   identifier,
                                    RFC_RESPONSE_T      status);
#endif 

/*! \brief Build and send an RFC_CONFIG_REQ primitive to RFCOMM
 
    \param conn_id - Unique connection identifier
    \param mux_disc_delay - Mux disconnection delay to be set that controls
                            mux disconnection instant post last DLCI closure.
*/ 
void rfc_config_req(uint16_t conn_id, uint16_t mux_disc_delay);

/*! \brief Override phandle & connection context for the given conn_id & it's mux.
 
    \param conn_id - conn_id to look for.
    \param phandle - Protocol handle used to identify the higher
                     entity to which primitives will be sent.
    \param context - host supplied context value.
*/ 
bool_t rfc_overrider_context(uint16_t conn_id, phandle_t phandle, context_t context);

#ifdef __cplusplus
}
#endif 

#endif

