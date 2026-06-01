/******************************************************************************
 Copyright (c) 2017-2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CSR_HCI_COMMON_H_
#define CSR_HCI_COMMON_H_

#include "csr_synergy.h"
#include "csr_macro.h"


#define CSR_HCI_COMMAND_OPCODE_SIZE             2
#define CSR_HCI_EVENT_CODE_SIZE                 1
#define CSR_HCI_EVENT_LENGTH_SIZE               1

#define CSR_HCI_MANUFACTURER_EXTENSION_OGF      0x3F
#define CSR_HCI_MANUFACTURER_EXTENSION_MASK     CSR_HCI_MANUFACTURER_EXTENSION_OGF << 2

#define CSR_HCI_EV_COMMAND_STATUS               0x0F
#define CSR_HCI_EV_COMMAND_COMPLETE             0x0E
#define CSR_HCI_EV_NUM_OF_COMP_PKTS             0x13
#define CSR_HCI_NOP                             0x0000

#define CSR_HCI_EV_VENDOR_SPECIFIC              0xFF


/* Create opcode from OGF and OCF */
#define CSR_HCI_GET_OPCODE(_ogf, _ocf)          ((((CSR_MSB16(_ocf) & 0x03) | _ogf) << 8) | CSR_LSB16(_ocf))

#define CSR_HCI_GET_OCF(_opcode)                (_opcode & 0x3FF)
#define CSR_HCI_GET_OGF(_opcode)                ((CSR_MSB16(_opcode)) >> 2)

/* Create vendor specific opcode from OCF */
#define CSR_HCI_GET_VS_OPCODE(_ocf)             CSR_HCI_GET_OPCODE(CSR_HCI_MANUFACTURER_EXTENSION_MASK, _ocf)
#define CSR_HCI_IS_VS_OPCODE(_opcode)           CSR_HCI_GET_OGF(_opcode) == CSR_HCI_MANUFACTURER_EXTENSION_OGF

#define CSR_HCI_EV_COMMAND_COMPLETE_OFFSET_OPCODE   2
#define CSR_HCI_EV_COMMAND_STATUS_OFFSET_OPCODE     3

#define CSR_HCI_MAX_HCI_CMD_PAYLOAD_SIZE        254
#define CSR_HCI_COMMAND_VENDOR_HDR_SIZE         4
#define CSR_HCI_EVENT_CODE_SIZE                 1

#define CSR_HCI_ACL_HDR_SIZE                    4
#define CSR_HCI_ACL_HANDLE_MASK                 0x0FFF

#define CSR_HCI_SCO_HDR_SIZE                    3
#define CSR_HCI_SCO_HANDLE_MASK                 0x0FFF


#endif /* CSR_HCI_COMMON_H_ */
