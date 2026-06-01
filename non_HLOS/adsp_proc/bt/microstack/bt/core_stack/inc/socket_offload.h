/*******************************************************************************
Copyright (C) 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifndef __SOCKET_OFFLOAD_H__
#define __SOCKET_OFFLOAD_H__

#include "qbl_adapter_types.h"
#include "rfcomm_prim.h"

#include INC_DIR(bluestack,bluetooth.h)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_SOCKET_OFFLOAD

/******************************************************************************
  L2CAOFFLOAD_RfcommConn : Function to set the RFCOMM context info
******************************************************************************/
l2ca_cid_t L2CAOFFLOAD_RfcommConn(hci_connection_handle_t hci_handle,
                                  uint16_t  local_cid,
                                  uint16_t  remote_cid,
                                  uint16_t  local_mtu,
                                  uint16_t  remote_mtu,
                                  context_t context,
                                  phandle_t phandle);


/******************************************************************************
  L2CAOFFLOAD_LecocConn : Update L2CAP conn for LECOC conn
******************************************************************************/
l2ca_cid_t L2CAOFFLOAD_LecocConn(hci_connection_handle_t hci_handle,
                                 uint16_t  local_cid,
                                 uint16_t  remote_cid,
                                 uint16_t  local_mtu,
                                 uint16_t  remote_mtu,
                                 uint16_t  local_mps,
                                 uint16_t  remote_mps,
                                 uint16_t  initial_rx_credits,
                                 uint16_t  initial_tx_credits,
                                 context_t context,
                                 phandle_t phandle);


/******************************************************************************
  L2CAOFFLOAD_AttConn : Update L2CAP conn context for ATT fixed channel
******************************************************************************/
l2ca_cid_t L2CAOFFLOAD_AttConn(hci_connection_handle_t hci_handle, 
                               uint16_t mtu,
                               context_t context);


/******************************************************************************
  rfcomm_offload_dlci : Function to offload RFCOMM DLCI
******************************************************************************/
uint16_t rfcomm_offload_dlci(uint8_t dlci,
                             uint16_t tx_credits,
                             uint16_t max_frame_size,
                             uint16_t flags,
                             l2ca_cid_t cid,
                             uint16_t l2cap_mtu,
                             phandle_t phandle);


/******************************************************************************
  rfcomm_disconnect_dlci : Function to disconnect RFCOMM DLCI
******************************************************************************/
void rfcomm_disconnect_dlci(uint16_t conn_id);

/******************************************************************************
  att_offload_conn : Update ATT fixed channel information
******************************************************************************/
void att_offload_conn(TYPED_BD_ADDR_T *addrt,
                      l2ca_cid_t cid,
                      uint16_t mtu);

/******************************************************************************
  L2CAOFFLOAD_DisconnectCid : Disconnect CID in L2CAP
******************************************************************************/
void L2CAOFFLOAD_DisconnectCid(hci_connection_handle_t hci_handle, uint16_t remote_cid);

#endif

#ifdef __cplusplus
}
#endif

#endif 
