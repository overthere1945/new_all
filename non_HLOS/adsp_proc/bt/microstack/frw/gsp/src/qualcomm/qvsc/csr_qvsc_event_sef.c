/******************************************************************************
 Copyright (c) 2017-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
 ******************************************************************************/

#include "csr_sched.h"
#include "csr_util.h"
#include "csr_types.h"
#include "csr_hci_prim.h"
#include "csr_log_text_2.h"
#include "csr_macro.h"
#include "csr_mblk.h"
#include "csr_result.h"
#include "csr_qvsc_private_prim.h"
#include "csr_qvsc_download.h"
#include "csr_qvsc_handler.h"

#ifdef CSR_FRW_INSTALL_BT_CRASH_DUMP

#ifdef ENABLE_CRASH_DEBUG
#include <stdio.h>
#endif /* ENABLE_CRASH_DEBUG */

#include "csr_file.h"
#include "csr_formatted_io.h"


/*
    RAM DUMP:

    eventPayload will be pointing the below packet format:

    1. First Event packet format is as below:

    1st Event:          { LENGTH |  SUB EVENT ID | MSG TYPE | SEQ NO | RESERVED | TOTAL LENGTH | PAYLOAD}
    Number of Bytes:    {   1    |      1        |    1     |   2    |   1      |   4          |variable}

    Here for 1st packet, seq no is always set to 0.

    2. Following Events packet format is as below:
    Following Events:   { LENGTH |  SUB EVENT ID | MSG TYPE | SEQ NO | PAYLOAD}
    Number of Bytes:    {   1    |      1        |    1     |   2    |variable}

    For last packet, seqno is always set to 0xFFFF regardless of total size of ram dump

*/


#define CRASH_SUB_EVENT_ID                                      0x01
#define CRASH_MSG_TYPE_RAMDUMP                                  0x08
#define CRASH_MSG_TYPE_MINIDUMP                                 0x09

#define CRASH_RAMDUMP_1ST_EVENT_PAYLOAD_INDEX                   10
#define CRASH_RAMDUMP_FOLLOWING_EVENT_PAYLOAD_INDEX             6

#define CRASH_RAMDUMP_1ST_EVENT_HEADER_SIZE                     9
#define CRASH_RAMDUMP_FOLLOWING_EVENT_HEADER_SIZE               5

#define CRASH_MINIDUMP_EVENT_PAYLOAD_INDEX                      3
#define CRASH_MINIDUMP_EVENT_HEADER_SIZE                        2
#endif /* ifdef CSR_FRW_INSTALL_BT_CRASH_DUMP */

#define CSR_QVSC_EV_CC_SIZE_MIN                             4
#define CSR_QVSC_EV_VS_SIZE_MIN                             3
#define CSR_QVSC_EV_VS_OFFSET_CLASS                         2

#ifdef CSR_DSPM_KCS_DOWNLOAD
/* Handles command complete event for KCS download request */
static void csrQvscEvCcKcsDownloadHandler(CsrQvscInstanceData* qvscInst,
                                          const CsrUint8 *data,
                                          CsrUint8 dataLen)
{
    CsrQvsStatus status = data[CSR_QVSC_EV_CC_CAP_DOWNLOAD_OFFSET_STATUS - CSR_HCI_EVENT_CODE_SIZE];
    CsrQvsSubOpcode subopcode = data[CSR_QVSC_EV_CC_CAP_DOWNLOAD_OFFSET_SUBOPCODE - CSR_HCI_EVENT_CODE_SIZE];

    switch (subopcode)
    {
        case CSR_QVSC_SUBOP_CAP_DOWNLOAD_ADD_KCS:
            if (status == CSR_QVS_STATUS_SUCCESS)
            {
                CsrQvscTlvDownloadInitiate(&qvscInst->tlvData); /* Download TLV */
            }
            else
            {
                CsrQvscKcsDownloadComplete(qvscInst,
                                           CSR_QVSC_RESULT_HCI_STATUS_MIN + status,
                                           CSR_QVSC_KCS_BUNDLE_ID_INVALID);
            }
            break;

        case CSR_QVSC_SUBOP_CAP_DOWNLOAD_REMOVE_KCS:
        {
            CsrResult result;

            if (status == CSR_QVS_STATUS_SUCCESS)
            {
                result = CSR_QVSC_RESULT_SUCCESS;
            }
            else
            {
                result = CSR_QVSC_RESULT_HCI_STATUS_MIN + status;
            }

            CsrQvscKcsRemoveComplete(qvscInst, result);
            break;
        }

        case CSR_QVSC_SUBOP_CAP_DOWNLOAD_COMPLETE:
        {
            CsrResult result;
            CsrUint16 kcsBundleId = CSR_QVSC_KCS_BUNDLE_ID_INVALID;

            if (status == CSR_QVS_STATUS_SUCCESS)
            {
                if (dataLen >= CSR_QVSC_EV_CC_CAP_DOWNLOAD_SIZE_MIN)
                {
                    const CsrUint8 *pKcsBundleId = &data[CSR_QVSC_EV_CC_CAP_DOWNLOAD_OFFSET_KCS_BUNDLE_ID - CSR_HCI_EVENT_CODE_SIZE];

                    result = CSR_QVSC_RESULT_SUCCESS;
                    kcsBundleId = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(pKcsBundleId);
                }
                else
                {
                    result = CSR_QVSC_RESULT_MISC;
                }
            }
            else
            {
                result = CSR_QVSC_RESULT_HCI_STATUS_MIN + status;
            }

            CsrQvscKcsDownloadComplete(qvscInst, result, kcsBundleId);
            break;
        }

        default:
            CSR_LOG_TEXT_WARNING((CsrQvscLto,
                                  CSR_QVSC_LTSO_GENERAL,
                                  "Unexpected subopcode (0x%02X) for KCS download response",
                                  subopcode));
            break;
    }
}

/* Handles TLV download response event when downloading KCS file */
static void csrQvscEvVsDownloadTlvKcsStateHandler(CsrQvscInstanceData *qvscInst,
                                                  CsrQvsStatus status)
{
    if (status == CSR_QVS_STATUS_SUCCESS)
    {
        if (CSR_MASK_IS_SET(qvscInst->tlvData.config,
                            CSR_QVSC_TLV_DOWNLOAD_CFG_BIT_SKIP_CC))
        {
            /* No need to wait for command complete event */
            CsrQvscTlvSegmentSend(&qvscInst->tlvData);
        }
        else
        {
            /* Wait for command complete event for sending next segment */
        }
    }
    else
    {
        /* Download failed. Wait for KCS download command complete event. */
        CSR_LOG_TEXT_WARNING((CsrQvscLto,
                              CSR_QVSC_LTSO_GENERAL,
                              "Download failed with status=0x%02X",
                              status));
    }
}
#endif /* CSR_DSPM_KCS_DOWNLOAD */

/* Handles TLV download response event when downloading any TLV file */
static void csrQvscEvVsDownloadTlvStateHandler(CsrQvscInstanceData *qvscInst,
                                               CsrQvsStatus status)
{
    if (status == CSR_QVS_STATUS_SUCCESS)
    {
        if (CSR_MASK_IS_SET(qvscInst->tlvData.config,
                            CSR_QVSC_TLV_DOWNLOAD_CFG_BIT_SKIP_VS))
        {
            /* Received TLV download response even as it was disabled.
             * This means that it must be end of TLV download */
            CsrQvscTlvDownloadComplete(qvscInst, CSR_QVSC_RESULT_SUCCESS);
        }
        else
        if (CSR_MASK_IS_SET(qvscInst->tlvData.config,
                            CSR_QVSC_TLV_DOWNLOAD_CFG_BIT_SKIP_CC))
        {
            /* No need to wait for command complete event */
            if (CsrQvscTlvSegmentSend(&qvscInst->tlvData))
            {
                /* Download is not complete, wait for next TLV download status event. */
            }
            else
            {
                /* Download is complete. */
                CsrQvscTlvDownloadComplete(qvscInst, CSR_QVSC_RESULT_SUCCESS);
            }
        }
        else
        {
            /* Wait for command complete event for sending next segment */
        }
    }
    else
    {
        /* Download failed. */
        CsrQvscTlvDownloadComplete(qvscInst, CSR_QVSC_RESULT_HCI_STATUS_MIN + status);
    }
}

/* Handles TLV download response event */
static void csrQvscEvVsDownloadTlvHandler(CsrQvscInstanceData *qvscInst,
                                          const CsrUint8 *data,
                                          CsrUint8 dataLen)
{
    if (dataLen >= CSR_QVSC_EV_VS_DOWNLOAD_RES_TLV_OFFSET_STATUS)
    {
        CsrQvsStatus status = data[CSR_QVSC_EV_VS_DOWNLOAD_RES_TLV_OFFSET_STATUS - CSR_HCI_EVENT_CODE_SIZE];

        switch (qvscInst->state)
        {
#ifdef CSR_DSPM_KCS_DOWNLOAD
            case CSR_QVSC_STATE_KCS:
                csrQvscEvVsDownloadTlvKcsStateHandler(qvscInst, status);
                break;
#endif /* CSR_DSPM_KCS_DOWNLOAD */

            case CSR_QVSC_STATE_TLV:
                csrQvscEvVsDownloadTlvStateHandler(qvscInst, status);
                break;

            default:
                /* This will never happen. */
                break;
        }
    }
    else
    {
        CSR_LOG_TEXT_WARNING((CsrQvscLto,
                              CSR_QVSC_LTSO_GENERAL,
                              "QVS download TLV event with unexpected length: %d",
                              dataLen));
    }
}

/* Handles download response events */
static void csrQvscEvVsDownloadHandler(CsrQvscInstanceData *qvscInst,
                                       const CsrUint8 *data,
                                       CsrUint8 dataLen)
{
    CsrQvsSubOpcode subopcode = data[CSR_QVSC_EV_VS_DOWNLOAD_OFFSET_SUBOPCODE - CSR_HCI_EVENT_CODE_SIZE];

    switch (subopcode)
    {
        case CSR_QVSC_EV_VS_DOWNLOAD_RES_TLV:
            csrQvscEvVsDownloadTlvHandler(qvscInst, data, dataLen);
            break;

        default:
            CSR_LOG_TEXT_WARNING((CsrQvscLto,
                                  CSR_QVSC_LTSO_GENERAL,
                                  "Unexpected QVS download response subopcode: 0x%02X",
                                  subopcode));
            break;
    }
}

/* Handles Command Complete events with NOP */
static void csrQvscEvNopHandler(CsrQvscInstanceData *qvscInst)
{
    if (CSR_MASK_IS_UNSET(qvscInst->tlvData.config,
                          CSR_QVSC_TLV_DOWNLOAD_CFG_BIT_SKIP_CC))
    {
        if (CsrQvscTlvSegmentSend(&qvscInst->tlvData))
        {
            /* Download is not complete, wait for next TLV download status event. */
        }
        else
        { /* Download is complete. */
            if (qvscInst->state == CSR_QVSC_STATE_TLV)
            {
                CsrQvscTlvDownloadComplete(qvscInst,
                CSR_QVSC_RESULT_SUCCESS);
            }
#ifdef CSR_DSPM_KCS_DOWNLOAD
            else if (qvscInst->state == CSR_QVSC_STATE_KCS)
            {
                /* We are currently using TLV download only for KCS download.
                 * Wait for KCS download command complete event. */
            }
#endif /* CSR_DSPM_KCS_DOWNLOAD */
            else
            {
                /* This should never happen */
                CSR_LOG_TEXT_WARNING((CsrQvscLto,
                                      CSR_QVSC_LTSO_GENERAL,
                                      "NOP received in unknown state: %d",
                                      qvscInst->state));
            }
        }
    }
    else
    {
        /* Unexpected NOP. Ignore */
        CSR_LOG_TEXT_WARNING((CsrQvscLto,
                              CSR_QVSC_LTSO_GENERAL,
                              "Unexpected NOP"));

    }
}

/* Handles Command Complete events */
static CsrBool csrQvscEvCcHandler(CsrQvscInstanceData *qvscInst,
                                  const CsrUint8 *data,
                                  CsrUint8 dataLen)
{
    if (dataLen >= CSR_QVSC_EV_CC_SIZE_MIN)
    {
        CsrUint16 opcode = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(&(data[CSR_QVSC_EV_CC_CAP_DOWNLOAD_OFFSET_OPCODE - CSR_HCI_EVENT_CODE_SIZE]));
        CsrBool processed = TRUE;

        switch (opcode)
        {
#ifdef CSR_DSPM_KCS_DOWNLOAD
            case CSR_QVSC_CAP_DOWNLOAD_ADD_KCS_REQ:
                csrQvscEvCcKcsDownloadHandler(qvscInst, data, dataLen);
                break;
#endif /* CSR_DSPM_KCS_DOWNLOAD */

            case CSR_HCI_NOP:
                csrQvscEvNopHandler(qvscInst);
                break;

            default:
                processed = FALSE;
                CSR_LOG_TEXT_WARNING((CsrQvscLto,
                                      CSR_QVSC_LTSO_GENERAL,
                                      "Unexpected command complete opcode: 0x%04X",
                                      opcode));
                break;
        }

        return (processed); 
    }
    else
    {
        return (FALSE);
    }


}

/* Handles vendor specific events */
static CsrBool csrQvscEvVsHandler(CsrQvscInstanceData *qvscInst,
                                  const CsrUint8 *data,
                                  CsrUint8 dataLen)
{
    if (dataLen >= CSR_QVSC_EV_VS_SIZE_MIN)
    {
        CsrUint8 class = data[CSR_QVSC_EV_VS_OFFSET_CLASS - CSR_HCI_EVENT_CODE_SIZE];
        CsrBool processed = TRUE;

        switch (class)
        {
            case CSR_QVSC_EV_VS_CLASS_DOWNLOAD:
                csrQvscEvVsDownloadHandler(qvscInst, data, dataLen);
                break;

            default:
                processed = FALSE;
                CSR_LOG_TEXT_WARNING((CsrQvscLto,
                                      CSR_QVSC_LTSO_GENERAL,
                                      "Unexpected QVS event class: 0x%02X",
                                      class));
                break;
        }

        return (processed);
    }
    else
    {
        return (FALSE);
    }

}

/* Handles HCI events for supported procedures */
static CsrBool csrQvsProcedureEventHandler(CsrQvscInstanceData *qvscInst,
                                           CsrUint8 event,
                                           CsrMblk *eventPayload)
{
    CsrUint8 dataLen;
    CsrUint8 *data;
    CsrBool processed = FALSE;

    dataLen = (CsrUint8) CsrMblkGetLength(eventPayload);
    data = CsrMblkMap(eventPayload, 0, dataLen);

    switch (event)
    {
        case CSR_HCI_EV_COMMAND_COMPLETE:
            processed = csrQvscEvCcHandler(qvscInst, data, dataLen);
            break;

        case 0:
        case CSR_HCI_EV_VENDOR_SPECIFIC:
            processed = csrQvscEvVsHandler(qvscInst, data, dataLen);
            break;

        default:
            CSR_LOG_TEXT_WARNING((CsrQvscLto,
                                  CSR_QVSC_LTSO_GENERAL,
                                  "Unexpected event: 0x%02X",
                                  event));
            break;
    }

    CsrMblkUnmap(eventPayload, data);
    if (processed)
    {
        CsrMblkDestroy(eventPayload);
    }

    return (processed);
}

/* Handles HCI events for individual commands */
static void csrQvcsRequestEventHandler(CsrQvscInstanceData *qvscInst,
                                       CsrUint8 event,
                                       CsrMblk *eventPayload)
{
    if (qvscInst->phandle != CSR_SCHED_QID_INVALID)
    {
        CsrUint16 dataLen = CsrMblkGetLength(eventPayload);
        CsrUint8 *data = CsrMblkMap(eventPayload, 0, dataLen);

        /* Exclude the data length byte */
        CsrQvscCfmSend(qvscInst->phandle,
                       CsrMemDup(&data[1], dataLen - 1),
                       dataLen - 1);

        CsrMblkUnmap(eventPayload, data);
    }
    else
    {
        CSR_LOG_TEXT_DEBUG((CsrQvscLto,
                            CSR_QVSC_LTSO_GENERAL,
                            "No application to handle the event: %d",
                            event));
    }
    CsrMblkDestroy(eventPayload);
    CsrQvscRestore(qvscInst);
}


#ifdef CSR_FRW_INSTALL_BT_CRASH_DUMP
static CsrBool handleDump(CsrQvscInstanceData *qvscInst,
                          CsrUint8             event,
                          CsrMblk             *eventPayload)
{
    CsrUint8* crashPtr;
    CsrUint8  crashDataLength;

    CsrResult res;
    static CsrFileHandle *fileHandle;
    CsrUint16 seqNo;
    CsrSize count;

    CsrUint8 *data;
    CsrUint8 dataLen;

    CsrCharString fileStr[128];
    CsrTime logTimeValLow;
    CsrTime logTimeValHigh;

    /* Get data and length */
    dataLen = (CsrUint8) CsrMblkGetLength(eventPayload);
    data = (CsrUint8 *) CsrMblkMap(eventPayload, 0, dataLen); 

    if ((!dataLen) || (dataLen < 3))
    {
        CsrMblkUnmap(eventPayload, data);
        return FALSE;
    }

    if ((data[1] == CRASH_SUB_EVENT_ID) && (data[2] == CRASH_MSG_TYPE_RAMDUMP))
    {
        seqNo = (CsrUint16)((0x00ff & data[3]) |  ((data[4] << 8) & 0xff00));

        if (seqNo == 0)
        {
            crashPtr = data + CRASH_RAMDUMP_1ST_EVENT_PAYLOAD_INDEX;
            crashDataLength = data[0] - CRASH_RAMDUMP_1ST_EVENT_HEADER_SIZE;

            logTimeValLow = CsrTimeGet(&logTimeValHigh);

            if (!CsrStrCmp(".",CSR_FRW_BT_CRASH_DUMP_PATH))
            {
                CsrSnprintf(fileStr, 128, "RAMDump-%08lu%08lu.bin", logTimeValHigh, logTimeValLow);
            }
            else
            {
                CsrSnprintf(fileStr, 128, CSR_FRW_BT_CRASH_DUMP_PATH);
                CsrSnprintf(fileStr + sizeof(CSR_FRW_BT_CRASH_DUMP_PATH) - 1, 128 - (sizeof(CSR_FRW_BT_CRASH_DUMP_PATH) - 1), 
                            "RAMDump-%08lu%08lu.bin", logTimeValHigh, logTimeValLow);
            }


            res = CsrFileOpen(&fileHandle, (const CsrUtf8String *)fileStr,
                              CSR_FILE_OPEN_FLAGS_CREATE | CSR_FILE_OPEN_FLAGS_WRITE_ONLY,
                              CSR_FILE_PERMS_USER_WRITE);
            if (res)
            {
                /* File open failed */
                CsrMblkUnmap(eventPayload, data);
                CsrMblkDestroy(eventPayload);
                return TRUE;
            }
            res = CsrFileWrite(crashPtr, crashDataLength, fileHandle,
                               &count);
            if (res)
            {
                /* Write failed */
                CsrMblkUnmap(eventPayload, data);
                CsrMblkDestroy(eventPayload);
                return TRUE;
            }

#ifdef ENABLE_CRASH_DEBUG
            printf("\nCreating File\n");
            printf("seq no: ");
#endif
        }
        else
        {
            crashPtr = data + CRASH_RAMDUMP_FOLLOWING_EVENT_PAYLOAD_INDEX;
            crashDataLength = data[0] - CRASH_RAMDUMP_FOLLOWING_EVENT_HEADER_SIZE;

            res = CsrFileWrite(crashPtr, crashDataLength, fileHandle,
                               &count);
            if (res)
            {
                /* Write failed */
                CsrMblkUnmap(eventPayload, data);
                CsrMblkDestroy(eventPayload);
                return TRUE;
            }
            if (seqNo == 0xffff)
            {
                CsrFileClose(fileHandle);
            }
        }
#ifdef ENABLE_CRASH_DEBUG
        printf("%04x ", seqNo);
#endif
        CsrMblkUnmap(eventPayload, data);
        CsrMblkDestroy(eventPayload);

        if (seqNo == 0xffff)
        {
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_CHIP_CRASHED, "BT Controller Crash");
        }

        return TRUE;
    }

    if ((data[1] == CRASH_SUB_EVENT_ID) && (data[2] == CRASH_MSG_TYPE_MINIDUMP))
    {
        crashPtr = data + CRASH_MINIDUMP_EVENT_PAYLOAD_INDEX;
        crashDataLength = data[0] - CRASH_MINIDUMP_EVENT_HEADER_SIZE;

        logTimeValLow = CsrTimeGet(&logTimeValHigh);

        if (!CsrStrCmp(".",CSR_FRW_BT_CRASH_DUMP_PATH))
        {
            CsrSnprintf(fileStr, 128, "MiniDump-%08lu%08lu.bin", logTimeValHigh, logTimeValLow);
        }
        else
        {
            CsrSnprintf(fileStr, 128, CSR_FRW_BT_CRASH_DUMP_PATH);
            CsrSnprintf(fileStr + sizeof(CSR_FRW_BT_CRASH_DUMP_PATH) - 1, 128 - (sizeof(CSR_FRW_BT_CRASH_DUMP_PATH) - 1),
                        "MiniDump-%08lu%08lu.bin", logTimeValHigh, logTimeValLow);
        }


        res = CsrFileOpen(&fileHandle, (const CsrUtf8String *)fileStr,
                          CSR_FILE_OPEN_FLAGS_CREATE | CSR_FILE_OPEN_FLAGS_WRITE_ONLY,
                          CSR_FILE_PERMS_USER_WRITE);
        if (res)
        {
            /* File open failed */
            CsrMblkUnmap(eventPayload, data);
            CsrMblkDestroy(eventPayload);
            return TRUE;
        }

        res = CsrFileWrite(crashPtr, crashDataLength, fileHandle,
                           &count);
        if (res)
        {
            /* Write failed */
            CsrMblkUnmap(eventPayload, data);
            CsrMblkDestroy(eventPayload);
            return TRUE;
        }

        CsrFileClose(fileHandle);
        return TRUE;
    }

    return FALSE;
}
#endif /* ifdef CSR_FRW_INSTALL_BT_CRASH_DUMP */

/* Handles subscribed unsolicited HCI Vendor Specific events  */
static CsrBool CsrQvscSubsEventHandler(CsrQvscInstanceData *qvscInst,
                                       CsrUint8 event,
                                       CsrMblk *eventPayload)
{
    CsrBool processed = FALSE;

    if (event == CSR_HCI_EV_VENDOR_SPECIFIC)
    {
#ifdef CSR_FRW_INSTALL_BT_CRASH_DUMP
        processed = handleDump(qvscInst, event, eventPayload);
#endif
        if (CsrCmnListElementGetFirst(&(qvscInst->subsList)))
        {
            CsrUint8 *data;
            CsrUint8 dataLen;
            CsrQvscUnsolicitedHciEvent hciEvent;

            /* Get data and length */
            dataLen = (CsrUint8) CsrMblkGetLength(eventPayload);
            data = (CsrUint8 *) CsrMblkMap(eventPayload, 0, dataLen); 

            /* data[0] is length field and it is not used for comparision.
             * Hence dataLen needs to be decremented by 1, 
             * dataLen -1 is used for comparison.
             */
            hciEvent.data = &data[1];
            hciEvent.dataLen = dataLen -1;
            hciEvent.processed = FALSE;

            /* Iterate through Subscription list and send event indication
             * if received event data and length matches.
             */
            CSR_QVSC_FIND_SUBS_ELEM_AND_SEND_IND(qvscInst->subsList, &hciEvent);
            CsrMblkUnmap(eventPayload, data);

            if (hciEvent.processed)
            {
                CsrMblkDestroy(eventPayload);
                processed = TRUE;
            }
        }
    }
    return (processed);
}

CsrBool CsrQvscHciVendorSpecificEventIndHandler(CsrQvscInstanceData *qvscInst)
{
    CsrHciVendorSpecificEventInd *ind = qvscInst->recvMsgP;
    CsrBool result = TRUE;
    
    switch (qvscInst->state)
    {
        case CSR_QVSC_STATE_IDLE:
            result = CsrQvscSubsEventHandler(qvscInst, ind->event, ind->data);
            break;

        case CSR_QVSC_STATE_REQ:
            /* Subscription events are given priority over other HCI events .
             * We check if received HCI event is a subscribed event and process it.
             * If it is not a subscribed event, then generic event handler is
             * invoked to process the event.
             */
            if (!CsrQvscSubsEventHandler(qvscInst, ind->event, ind->data))
            {
                csrQvcsRequestEventHandler(qvscInst, ind->event, ind->data);
            }
            break;

#ifdef CSR_DSPM_KCS_DOWNLOAD
        case CSR_QVSC_STATE_KCS:
#endif
        case CSR_QVSC_STATE_TLV:
            /* Procedural events are given priority over subscribed HCI events.
             * We check if received HCI event is a procedural event and process it.
             * If it is not a procedural event, then subscription event handler is
             * invoked to process the event.
             */
            if (!csrQvsProcedureEventHandler(qvscInst, ind->event, ind->data))
            {
                result = CsrQvscSubsEventHandler(qvscInst, ind->event, ind->data);
            }
            break;

        default:
            result = FALSE;
    }

    if (!result)
    {
        CSR_LOG_TEXT_ERROR((CsrQvscLto,
                            CSR_QVSC_LTSO_GENERAL,
                            "Unexpected event: 0x%04X received in state: %d",
                            ind->event,
                            qvscInst->state));
        CsrMblkDestroy(ind->data);
    }
    ind->data = NULL;

    return (result);
}

