/******************************************************************************
Copyright (c) 2009-2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_hci_lib.h"
#include "csr_pmem.h"

/* --------------------------------------------------------------------
   Name
       CSR_HCI_REGISTER_EVENT_HANDLER_REQ

   Description
        Register HCI event handler with the CsrHci task

 * -------------------------------------------------------------------- */
CsrHciRegisterEventHandlerReq *CsrHciRegisterEventHandlerReq_struct(CsrSchedQid queueId)
{
    CsrHciRegisterEventHandlerReq *prim = pnew(CsrHciRegisterEventHandlerReq);

    prim->type = CSR_HCI_REGISTER_EVENT_HANDLER_REQ;
    prim->queueId = queueId;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_PERI_REGISTER_EVENT_HANDLER_REQ

   Description
        Register PERI event handler with the CsrHci task

 * -------------------------------------------------------------------- */
CsrPeriRegisterEventHandlerReq *CsrPeriRegisterEventHandlerReq_struct(CsrSchedQid queueId)
{
    CsrPeriRegisterEventHandlerReq *prim = pnew(CsrPeriRegisterEventHandlerReq);

    prim->type = CSR_PERI_REGISTER_EVENT_HANDLER_REQ;
    prim->queueId = queueId;

    return prim;
}


/* --------------------------------------------------------------------
   Name
       CSR_HCI_REGISTER_ACL_HANDLER_REQ

   Description
        Register HCI ACL handler with the CsrHci task

 * -------------------------------------------------------------------- */
CsrHciRegisterAclHandlerReq *CsrHciRegisterAclHandlerReq_struct(CsrSchedQid queueId, CsrUint16 aclHandle)
{
    CsrHciRegisterAclHandlerReq *prim = pnew(CsrHciRegisterAclHandlerReq);

    prim->type = CSR_HCI_REGISTER_ACL_HANDLER_REQ;
    prim->queueId = queueId;
    prim->aclHandle = aclHandle;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_HCI_UNREGISTER_ACL_HANDLER_REQ

   Description
        Unregister HCI ACL handler with the CsrHci task

 * -------------------------------------------------------------------- */
CsrHciUnregisterAclHandlerReq *CsrHciUnregisterAclHandlerReq_struct(CsrSchedQid queueId, CsrUint16 aclHandle)
{
    CsrHciUnregisterAclHandlerReq *prim = pnew(CsrHciUnregisterAclHandlerReq);

    prim->type = CSR_HCI_UNREGISTER_ACL_HANDLER_REQ;
    prim->queueId = queueId;
    prim->aclHandle = aclHandle;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_HCI_REGISTER_SCO_HANDLER_REQ

   Description
        Register HCI SCO handler with the CsrHci task

 * -------------------------------------------------------------------- */
CsrHciRegisterScoHandlerReq *CsrHciRegisterScoHandlerReq_struct(CsrSchedQid queueId, CsrUint16 scoHandle)
{
    CsrHciRegisterScoHandlerReq *prim = pnew(CsrHciRegisterScoHandlerReq);

    prim->type = CSR_HCI_REGISTER_SCO_HANDLER_REQ;
    prim->queueId = queueId;
    prim->scoHandle = scoHandle;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_HCI_UNREGISTER_SCO_HANDLER_REQ

   Description
        Unregister HCI SCO handler with the CsrHci task

 * -------------------------------------------------------------------- */
CsrHciUnregisterScoHandlerReq *CsrHciUnregisterScoHandlerReq_struct(CsrSchedQid queueId, CsrUint16 scoHandle)
{
    CsrHciUnregisterScoHandlerReq *prim = pnew(CsrHciUnregisterScoHandlerReq);

    prim->type = CSR_HCI_UNREGISTER_SCO_HANDLER_REQ;
    prim->queueId = queueId;
    prim->scoHandle = scoHandle;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_HCI_REGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_REQ

   Description
        Register Vendor Specific HCI event handler with the CsrHci task

 * -------------------------------------------------------------------- */
CsrHciRegisterVendorSpecificEventHandlerReq *CsrHciRegisterVendorSpecificEventHandlerReq_struct(CsrSchedQid queueId, CsrUint8 channel)
{
    CsrHciRegisterVendorSpecificEventHandlerReq *prim = pnew(CsrHciRegisterVendorSpecificEventHandlerReq);

    prim->type = CSR_HCI_REGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_REQ;
    prim->queueId = queueId;
    prim->channel = channel;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_HCI_UNREGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_REQ

   Description
        Unregister Vendor Specific HCI event handler with the CsrHci task

 * -------------------------------------------------------------------- */
CsrHciUnregisterVendorSpecificEventHandlerReq *CsrHciUnregisterVendorSpecificEventHandlerReq_struct(CsrSchedQid queueId, CsrUint8 channel)
{
    CsrHciUnregisterVendorSpecificEventHandlerReq *prim = pnew(CsrHciUnregisterVendorSpecificEventHandlerReq);

    prim->type = CSR_HCI_UNREGISTER_VENDOR_SPECIFIC_EVENT_HANDLER_REQ;
    prim->queueId = queueId;
    prim->channel = channel;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_HCI_COMMAND_REQ

   Description
        Send HCI command

 * -------------------------------------------------------------------- */
CsrHciCommandReq *CsrHciCommandReq_struct(CsrUint16 payloadLength, CsrUint8 *payload)
{
    CsrHciCommandReq *prim = pnew(CsrHciCommandReq);

    prim->type = CSR_HCI_COMMAND_REQ;
    prim->payloadLength = payloadLength;
    prim->payload = payload;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_PERI_COMMAND_REQ

   Description
        Send HCI command

 * -------------------------------------------------------------------- */
CsrPeriCommandReq *CsrPeriCommandReq_struct(CsrUint16 payloadLength, CsrUint8 *payload)
{
    CsrPeriCommandReq *prim = pnew(CsrPeriCommandReq);

    prim->type = CSR_PERI_COMMAND_REQ;
    prim->payloadLength = payloadLength;
    prim->payload = payload;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_HCI_ACL_DATA_REQ

   Description
        Send HCI ACL data

 * -------------------------------------------------------------------- */
CsrHciAclDataReq *CsrHciAclDataReq_struct(CsrUint16 handlePlusFlags, CsrMblk *data)
{
    CsrHciAclDataReq *prim = pnew(CsrHciAclDataReq);

    prim->type = CSR_HCI_ACL_DATA_REQ;
    prim->handlePlusFlags = handlePlusFlags;
    prim->data = data;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_HCI_SCO_DATA_REQ

   Description
        Send HCI SCO data

 * -------------------------------------------------------------------- */
CsrHciScoDataReq *CsrHciScoDataReq_struct(CsrUint16 handlePlusFlags, CsrMblk *data)
{
    CsrHciScoDataReq *prim = pnew(CsrHciScoDataReq);

    prim->type = CSR_HCI_SCO_DATA_REQ;
    prim->handlePlusFlags = handlePlusFlags;
    prim->data = data;

    return prim;
}

/* --------------------------------------------------------------------
   Name
       CSR_HCI_VENDOR_SPECIFIC_COMMAND_REQ

   Description
        Send Vendor Specific HCI command data

 * -------------------------------------------------------------------- */
CsrHciVendorSpecificCommandReq *CsrHciVendorSpecificCommandReq_struct(CsrUint8 channel, CsrMblk *data)
{
    CsrHciVendorSpecificCommandReq *prim = pnew(CsrHciVendorSpecificCommandReq);

    prim->type = CSR_HCI_VENDOR_SPECIFIC_COMMAND_REQ;
    prim->channel = channel;
    prim->data = data;

    return prim;
}


/* --------------------------------------------------------------------
   Name
       CSR_HCI_RESET_TRANSPORT_REQ

   Description
        Send Vendor Specific HCI command data

 * -------------------------------------------------------------------- */
CsrHciResetTransportReq *CsrHciResetTransportReq_struct(CsrSchedQid queueId,
                                                        CsrUint8 baudId)
{
    CsrHciResetTransportReq *prim = pnew(CsrHciResetTransportReq);

    prim->type = CSR_HCI_RESET_TRANSPORT_REQ;
    prim->queueId = queueId;
    prim->baudId = baudId;

    return (prim);
}
