/*******************************************************************************

Copyright (C) 2016 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\brief  L2CAP internal interface primitives.

        Internal primitives involved in setting up L2CAP connection on a
        specific transport.
        For LE transport, these are used to setup credit based connection
        oriented L2CAP channel.
*******************************************************************************/

#ifndef _BLUESTACK_L2CAP_INT_PRIM_H_
#define _BLUESTACK_L2CAP_INT_PRIM_H_

#include INC_DIR(bluestack,hci.h)
#ifdef __cplusplus
extern "C" {
#endif

/* L2CAP internal primitive segmentation & numbering */
#define L2CA_INT_PRIM_BASE                   0x0100
#define L2CA_INT_PRIM_MAX                    (L2CA_INT_PRIM_BASE | 0x00FF)
#define L2CA_INT_PRIM_DOWN                   L2CA_INT_PRIM_BASE
#define L2CA_INT_PRIM_UP                     L2CA_INT_PRIM_MAX


typedef enum l2cap_int_prim_tag
{
    /* Downstream primitives */
    ENUM_L2CA_INTERNAL_TP_CONNECT_REQ = L2CA_INT_PRIM_DOWN,
    ENUM_L2CA_INTERNAL_TP_CONNECT_RSP,
    ENUM_L2CA_INTERNAL_PASSTHROUGH_DATA_REQ,

    /* Upstream primitives */
    ENUM_L2CA_INTERNAL_TP_CONNECT_IND = L2CA_INT_PRIM_UP - 1,
    ENUM_L2CA_INTERNAL_TP_CONNECT_CFM = L2CA_INT_PRIM_UP
} L2CAP_INT_PRIM_T;


/* Downstream primitives */
#define L2CA_INTERNAL_TP_CONNECT_REQ        ((l2cap_prim_t)ENUM_L2CA_INTERNAL_TP_CONNECT_REQ)
#define L2CA_INTERNAL_TP_CONNECT_RSP        ((l2cap_prim_t)ENUM_L2CA_INTERNAL_TP_CONNECT_RSP)
#define L2CA_INTERNAL_PASSTHROUGH_DATA_REQ  ((l2cap_prim_t)ENUM_L2CA_INTERNAL_PASSTHROUGH_DATA_REQ)

/* Upstream primitives */
#define L2CA_INTERNAL_TP_CONNECT_IND        ((l2cap_prim_t)ENUM_L2CA_INTERNAL_TP_CONNECT_IND)
#define L2CA_INTERNAL_TP_CONNECT_CFM        ((l2cap_prim_t)ENUM_L2CA_INTERNAL_TP_CONNECT_CFM)


typedef struct
{
    l2cap_prim_t        type;             /*!< Always L2CA_INTERNAL_PASSTHROUGH_DATA_REQ */
    uint16_t            length;           /*!< Length of data, 0 for MBLKs */
    MBLK_T             *data;             /*!< Pointer to data/MBLK */
} L2CA_INTERNAL_PASSTHROUGH_DATA_REQ_T;



#ifdef __cplusplus
}
#endif

#endif

