#ifndef CSR_QVSC_DOWNLOAD_H_
#define CSR_QVSC_DOWNLOAD_H_
/******************************************************************************
 Copyright (c) 2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
 ***************************************************************************** */

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_macro.h"
#include "csr_hci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef CsrUint16 CsrQvsOcf;
#define CSR_QVS_OCF_DOWNLOAD_COMMAND                ((CsrQvsOcf) 0x0000)
#define CSR_QVS_OCF_DOWNLOAD_KCS_REQ                ((CsrQvsOcf) 0x0180)

typedef CsrUint8 CsrQvsSubOpcode;
#define CSR_QVS_EDL_START_REQ                       ((CsrQvsSubOpcode) 0x00)
#define CSR_QVS_EDL_DNLOAD_REQ                      ((CsrQvsSubOpcode) 0x01)
#define CSR_QVS_EDL_UPLOAD_REQ                      ((CsrQvsSubOpcode) 0x02)
#define CSR_QVS_EDL_GETSTATUS_REQ                   ((CsrQvsSubOpcode) 0x03)
#define CSR_QVS_EDL_SETPC_REQ                       ((CsrQvsSubOpcode) 0x04)
#define CSR_QVS_EDL_RESET_REQ                       ((CsrQvsSubOpcode) 0x05)
#define CSR_QVS_EDL_GETAPPVER_REQ                   ((CsrQvsSubOpcode) 0x06)
#define CSR_QVS_EDL_GETDLVER_REQ                    ((CsrQvsSubOpcode) 0x07)
#define CSR_QVS_EDL_VERIFY_REQ                      ((CsrQvsSubOpcode) 0x08)
#define CSR_QVS_EDL_SIGNATURE_REQ                   ((CsrQvsSubOpcode) 0x09)
#define CSR_QVS_EDL_POKE16_REQ                      ((CsrQvsSubOpcode) 0x0A)
#define CSR_QVS_EDL_PEEK16_REQ                      ((CsrQvsSubOpcode) 0x0B)
#define CSR_QVS_EDL_POKE32_REQ                      ((CsrQvsSubOpcode) 0x0C)
#define CSR_QVS_EDL_PEEK32_REQ                      ((CsrQvsSubOpcode) 0x0D)
#define CSR_QVS_EDL_NVMADD_REQ                      ((CsrQvsSubOpcode) 0x0E)
#define CSR_QVS_EDL_CLK_BAUD_CHG_REQ                ((CsrQvsSubOpcode) 0x0F)
#define CSR_QVS_EDL_MEMORY_ACCESS_REQ               ((CsrQvsSubOpcode) 0x10)
#define CSR_QVS_EDL_ERASE_FLASH_REQ                 ((CsrQvsSubOpcode) 0x11)
#define CSR_QVS_EDL_BLOCK_POKE16_REQ                ((CsrQvsSubOpcode) 0x12)
#define CSR_QVS_EDL_BLOCK_POKE32_REQ                ((CsrQvsSubOpcode) 0x13)
#define CSR_QVS_EDL_READ_CALIBRATION_DATALOG        ((CsrQvsSubOpcode) 0x14)
#define CSR_QVS_EDL_PATCH_SETREGION                 ((CsrQvsSubOpcode) 0x15)
#define CSR_QVS_EDL_PATCH_SET_REQ                   ((CsrQvsSubOpcode) 0x16)
#define CSR_QVS_EDL_PATCH_ATTACH                    ((CsrQvsSubOpcode) 0x17)
#define CSR_QVS_EDL_PATCH_DETACH                    ((CsrQvsSubOpcode) 0x18)
#define CSR_QVS_EDL_PATCH_GETVER                    ((CsrQvsSubOpcode) 0x19)
#define CSR_QVS_EDL_PATCH_BURN_REQ                  ((CsrQvsSubOpcode) 0x1A)
#define CSR_QVS_EDL_SYSCFG_BURN_REQ                 ((CsrQvsSubOpcode) 0x1B)
#define CSR_QVS_EDL_TOPCFG_BURN_REQ                 ((CsrQvsSubOpcode) 0x1C)
#define CSR_QVS_EDL_READ_PS_REQ                     ((CsrQvsSubOpcode) 0x1D)
#define CSR_QVS_TLV_DOWNLOAD_REQ                    ((CsrQvsSubOpcode) 0x1E)
#define CSR_QVS_EDL_WRITE_PS_REQ                    ((CsrQvsSubOpcode) 0x1F)
#define CSR_QVS_EDL_GET_BUILD_INFO                  ((CsrQvsSubOpcode) 0x20)
#define CSR_QVS_EDL_GET_BOARD_ID                    ((CsrQvsSubOpcode) 0x23)
#define CSR_QVS_EDL_STORE_PS                        ((CsrQvsSubOpcode) 0x24)

typedef CsrUint8 CsrQvsEdlAppType;
#define CSR_QVS_EDL_APP_TYPE_FLASH                  ((CsrQvsEdlAppType) 0x00)
#define CSR_QVS_EDL_APP_TYPE_BOOT                   ((CsrQvsEdlAppType) 0x01)
#define CSR_QVS_EDL_APP_TYPE_RAM                    ((CsrQvsEdlAppType) 0x02)
#define CSR_QVS_EDL_APP_TYPE_ROM                    ((CsrQvsEdlAppType) 0x03)

typedef CsrUint8 CsrQvsStatus;
#define CSR_QVS_STATUS_SUCCESS                      ((CsrQvsStatus) 0x00)
#define CSR_QVS_STATUS_LENGTH_ERROR                 ((CsrQvsStatus) 0x01)
#define CSR_QVS_STATUS_VERSION_ERROR                ((CsrQvsStatus) 0x02)
#define CSR_QVS_STATUS_SIGNATURE_ERROR              ((CsrQvsStatus) 0x03)
#define CSR_QVS_STATUS_PATCH_UNFOUND                ((CsrQvsStatus) 0x04)


/* Kymera capability download command opcode */
#define CSR_QVSC_CAP_DOWNLOAD_ADD_KCS_REQ           CSR_HCI_GET_VS_OPCODE(CSR_QVS_OCF_DOWNLOAD_KCS_REQ)

/* Kymera capability download command sub-opcodes */
#define CSR_QVSC_SUBOP_CAP_DOWNLOAD_ADD_KCS                 0x01
#define CSR_QVSC_SUBOP_CAP_DOWNLOAD_REMOVE_KCS              0x02
#define CSR_QVSC_SUBOP_CAP_DOWNLOAD_COMPLETE                0x03

#define CSR_QVSC_EV_CC_CAP_DOWNLOAD_SIZE_MIN                7

#define CSR_QVSC_EV_VS_CLASS_DOWNLOAD                       0x00
#define CSR_QVSC_EV_VS_DOWNLOAD_OFFSET_SUBOPCODE            3

#define CSR_QVSC_EV_VS_DOWNLOAD_RES_TLV                     0x04
#define CSR_QVSC_EV_VS_DOWNLOAD_RES_TLV_OFFSET_STATUS       0x04

#define CSR_QVSC_EV_CC_CAP_DOWNLOAD_OFFSET_OPCODE           3
#define CSR_QVSC_EV_CC_CAP_DOWNLOAD_OFFSET_STATUS           5
#define CSR_QVSC_EV_CC_CAP_DOWNLOAD_OFFSET_SUBOPCODE        6
#define CSR_QVSC_EV_CC_CAP_DOWNLOAD_OFFSET_KCS_BUNDLE_ID    7

#define CSR_QVSC_TLV_SEGMENT_LENGTH_MAX                     243

#ifdef __cplusplus
}
#endif

#endif /* CSR_QVSC_DOWNLOAD_H_ */
