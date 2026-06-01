/*******************************************************************************

Copyright (C) 2009 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifdef INSTALL_ATT_MODULE
#ifndef    __ATT_H__
#define    __ATT_H__

#include "qbl_adapter_types.h"
#include INC_DIR(l2caplib,l2caplib.h)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BLUESTACK_HOST_IS_APPS
/* att.c */
extern void att_handler(void **gash);
#endif

#ifdef INSTALL_STREAM_MODULE
/****************************************************************************/
/*! \brief Gets the number of available application credits for stream and  
 *         whether this connection has a mandatory credit or not.
 *      
 *         Application credits indicates number of ATT notification and command 
 *         that can be sent on this ATT connection. 
 *
 * \param cid        Connection ID to check for credits
 * \param mandatory  Pointer to boolean to receive notice that this connection
 *                   has no outstanding messages and should therefore be
 *                   permitted to send an ATT cmd/notification regardless of 
 *                   credits remaining.
 *
 * \return Number of available app credits for stream, or returns zero if 
 *         no connection is found with given cid.
 */
extern uint16 att_get_available_app_credit_for_stream(l2ca_cid_t cid, 
                                                        bool *mandatory);

/****************************************************************************/
/*! \brief Get MTU of a specified ATT connection
 *
 * \param cid        Connection ID
 * 
 * \return MTU of specified connection, or 0 if no connection is found with
 *         given cid.
 */
extern uint16 att_get_mtu(l2ca_cid_t cid);

/****************************************************************************/
/*! \brief Determine if specified ATT connection exists
 *
 * \param cid        Connection ID
 * 
 * \return TRUE if connection exists, otherwise FALSE
 */
extern bool att_conn_exists(l2ca_cid_t cid);

#endif /* INSTALL_STREAM_MODULE */


#ifdef __cplusplus
}
#endif

#endif    /* __ATT_H__ */
#endif  /* INSTALL_ATT_MODULE */
