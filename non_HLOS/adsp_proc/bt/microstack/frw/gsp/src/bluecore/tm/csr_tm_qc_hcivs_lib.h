/******************************************************************************
 Copyright (c) 2016-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CSR_TM_QC_HCIVS_LIB_H__
#define CSR_TM_QC_HCIVS_LIB_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CSR_QSOC_MAX_BD_ADDRESS_SIZE                     0x06  /**< Length of BT Address */
#define CSR_QC_HCIVS_STD_HEADER_SIZE                     0x03
#define CSR_HCI_STD_HEADER_SIZE                          0x03

#define CSR_HCI_VENDOR_CMD_OGF                           0x3F
#define CSR_EDL_SET_BAUDRATE_CMD_OCF                     0x48

/* Vendor Specific command codes */
#define CSR_QSOC_EDL_CMD_OPCODE                          0xFC00
#define CSR_QSOC_NVM_ACCESS_OPCODE                       0xFC0B
#define CSR_QSOC_WARM_RESET_OPCODE                       0xFC47
#define CSR_QSOC_SET_BAUD_RATE_OPCODE                    0xFC48
#define CSR_QSOC_HOST_CTL_LOG_OPCODE                     0xFC17
#define CSR_HCI_RESET_OPCODE                             0x0C03

#define EDL_CMD_REQ_RES_EVT                              0x00
#define EDL_APP_VER_RES_EVT                              0x02
#define EDL_FW_HCI_REQ_RES_EVT                           0x00
#define EDL_FW_HCI_RES_PARAM_LEN                         0x01
#define CSR_QSOC_SET_BAUD_RATE_RESP                      0x92
#define VENDOR_EVENT_CLASS_NVM_ACCESS                    0x0B


/* For VENDOR_EVENT_CLASS_NVM_ACCESS related defines */
#define NVM_ACCESS_COMMAND_TYPE_OFFSET                  0x02
#define NVM_ACCESS_TAG_NUMBER_OFFSET                    0x03
#define NVM_ACCESS_HEADER_SIZE                          0x03
#define NVM_ACCESS_TAG_VALUE_OFFSET                     0x05

/* Command types */
#define NVM_ACCESS_COMMAND_TYPE_GET                     0x00
#define NVM_ACCESS_COMMAND_TYPE_SET                     0x01

/* Definitions for NVM access tag numbers */
#define NVM_ACCESS_TAG_NUMBER_DEBUG_CONTROL             0x26

#define DEBUG_CONTROL_INHIBIT_NOP_BIT4                  0x10

#define CSR_QSOC_PATCH_VER_REQ                           0x19
#define CSR_QSOC_PATCH_VER_RESP                          CSR_QSOC_PATCH_VER_REQ

#define CSR_TLV_DOWNLOAD_REQ                             0x1E
#define EDL_TLV_DNLD_RES_EVT                            0x04

#define CSR_QC_HCIVS_RESP_MIN_SIZE                           0x03
#define CSR_QC_HCIVS_INVALID_CODE                            0xFF

#define CMD_RSP_OFFSET                                   (1)
#define RES_TYPE_OFFSET                                  (2)
#define CSR_QC_HCIVS_BUILD_VER_OFFSET                    (9)
#define CSR_QC_HCIVS_TLV_DOWNLOAD_RES_STATUS_OFFSET      (3)
#define CSR_QC_HCIVS_SET_BAUD_RES_STATUS_OFFSET          (2)
#define CSR_QC_HCIVS_SOC_VER_OFFSET                      (2)

#define MAX_LEN_BIN_FILE_SEGMENT                         (243)

/* Definitions for vendor specific command complete response opcodes offset */
#define CSR_QC_HCIVS_CC_OPCODE_OFFSET                      (2)
#define CSR_QC_HCIVS_CC_SUB_OPCODE_OFFSET                  (5)
#define CSR_QC_HCIVS_CC_NVM_ACCESS_TAG_VALUE_OFFSET        (8)
#define CSR_QC_HCIVS_CC_BUILD_VER_OFFSET                   (13)
#define CSR_QC_HCIVS_CC_SET_BAUD_RES_STATUS_OFFSET         (4)
#define CSR_QC_HCIVS_CC_STATUS_OFFSET                      (4)

/*QC TM Panic codes*/
#define SET_BAUD_FAILURE                                 0x1
#define GET_PATCH_VER_FAILURE                            0x2
#define PATCH_FILE_READ_FAILURE                          0x3
#define READ_UPDATE_NVM_FAILURE                          0x4
#define PATCH_M0_DOWNLOAD_FAILURE                        0x5
#define PATCH_AUP_DOWNLOAD_FAILURE                       0x6

#define SET_ADDRESS_FAILURE                              0x07

typedef struct
{
    CsrUint32 currentIndex;
    CsrUint8  type;
    CsrUint32 payloadLength;
    CsrUint8 *payload;
} CsrQcBin;

void CsrQcSetBaudrateReqSend(CsrUint8 baud);
void CsrQcGetPatchVerReqSend(void);
void CsrQcHciResetReqSend(void);
void CsrQcControllerLogReqSend(CsrBool logEnable);
CsrBool CsrQcNvmChunkReqSend(CsrQcBin *qcBinVar);
void CsrQcReadDebugControlTagReqSend(void);
void CsrQcWriteDebugControlTagReqSend(CsrUint8 *debugControl, CsrUint8 length);
void CsrTmQcPanic(CsrUint32 p);

#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_QC_HCIVS_LIB_H__ */
