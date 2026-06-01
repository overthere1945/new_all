/*******************************************************************************

Copyright (C) 2010 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#include INC_DIR(l2cap,l2cap_cidme.h)

/*! \brief Get Bluetooth address from CID.

    Returns the Bluetooth Device Address of the device with the specified
    Connection Identifier

    \param cid Connection ID
    \param addrt Pointer to Bluetooth Address structure to initialise.
    \return TRUE if the cid has a valid Bluetooth Device Address otherwise FALSE
*/
#if defined(INSTALL_ATT_MODULE)
bool_t L2CA_GetBdAddrFromCid(uint16_t cid,
                             TYPED_BD_ADDR_T *addrt)
{
    CIDCB_T *cidcb = CIDME_GetCidcb(cid);

    if (cidcb == NULL)
        return FALSE;

    tbdaddr_copy(addrt, CH_GET_TBDADDR(cidcb->chcb));
    return TRUE;
}
#endif /* INSTALL_ATT_MODULE */
