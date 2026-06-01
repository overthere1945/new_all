/*****************************************************************************

      Copyright (c) 2010 - 2016 Qualcomm Technologies International, Ltd.

      All Rights Reserved.

      Qualcomm Technologies International, Ltd. Confidential and Proprietary. 

*****************************************************************************/

#ifndef CSR_HCI_PRIVATE_PRIM_H__
#define CSR_HCI_PRIVATE_PRIM_H__

#include "csr_hci_prim.h"

#ifdef __cplusplus
extern "C" {
#endif

/* search_string="CsrHciPrim" */
/* conversion_rule="UPPERCASE_START_AND_REMOVE_UNDERSCORES" */

#define CSR_HCI_PRIVATE_PRIM (CSR_HCI_PRIM)

#define CSR_HCI_NOP_IND ((CsrHciPrim) ((0x0000 + CSR_HCI_PRIM_UPSTREAM_COUNT) | CSR_PRIM_UPSTREAM))

#define CSR_HCI_RESET_IND   ((CsrHciPrim) ((0x0001 + CSR_HCI_PRIM_UPSTREAM_COUNT) | CSR_PRIM_UPSTREAM))

typedef struct
{
    CsrHciPrim  type;
    CsrSchedQid phandle;        /* Handle to send the forwarded NOP message to */
    CsrUint8   *payload;        /* Pointer to the forwarded NOP message */
    CsrUint16   payloadLength;  /* The length of the NOP message */
} CsrHciNopInd;

typedef struct
{
    CsrHciPrim  type;
    CsrSchedQid phandle;        /* Handle to send the forwarded RESET INDmessage to */
} CsrHciResetInd;

#ifdef __cplusplus
}
#endif


#endif /* CSR_HCI_PRIVATE_PRIM_H__ */
