/*******************************************************************************

Copyright (C) 2010 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#include INC_DIR(l2cap,l2cap_cidme.h)

/*! \brief Get Transport Bluetooth Address type from CID.

    Returns the transport bluetooth address of the device with the specified
    Connection Identifier

    \param cid Channel ID
    \param tpaddrt Pointer to Transport Bluetooth Address structure to initialise.
    \return TRUE if the cid has a valid Transport Bluetooth Device Address 
            otherwise FALSE*/
#if defined(INSTALL_STREAM_MODULE) || defined(TRAPSET_BLUESTACK) || defined(INSTALL_SM_MODULE)
bool_t L2CA_GetTpBdAddrFromCid(uint16_t cid,
                               TP_BD_ADDR_T *tpaddrt)
{
    CIDCB_T *cidcb = CIDME_GetCidcb(cid);
    
    if (cidcb == NULL)
        return FALSE;

    tbdaddr_copy(&tpaddrt->addrt, CH_GET_TBDADDR(cidcb->chcb));
    tpaddrt->tp_type = CH_IS_ULP(cidcb->chcb) ? LE_ACL : BREDR_ACL;
    return TRUE;
}
#endif /* STREAM_MODULE || TRAPSET_BLUESTACK */
