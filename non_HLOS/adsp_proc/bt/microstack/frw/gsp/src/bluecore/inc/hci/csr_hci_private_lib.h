#ifndef CSR_HCI_PRIVATE_LIB_H__
#define CSR_HCI_PRIVATE_LIB_H__
/*****************************************************************************

            Copyright (c) 2011-2015 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/
#include "csr_hci_private_prim.h"
#include "csr_hci_task.h"
#include "csr_msg_transport.h"

#ifdef __cplusplus
extern "C" {
#endif
void CsrHciPrivateFreeUpstreamMessageContents(CsrUint16 eventClass, void *message);
void CsrHciPrivateFreeDownstreamMessageContents(CsrUint16 eventClass, void *message);

#ifdef __cplusplus
}
#endif

#endif /* CSR_HCI_PRIVATE_LIB_H__ */
