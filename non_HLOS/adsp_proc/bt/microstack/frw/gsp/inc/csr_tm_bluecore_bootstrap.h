/******************************************************************************
 Copyright (c) 2008-2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef CSR_TM_BLUECORE_BOOTSTRAP_H__
#define CSR_TM_BLUECORE_BOOTSTRAP_H__

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_prim_defs.h"
#include "csr_macro.h"

#ifndef EXCLUDE_CSR_BCCMD_MODULE
#include "csr_bccmd_prim.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXCLUDE_CSR_BCCMD_MODULE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTmBlueCoreBuildBccmdSetMsg
 *
 *  DESCRIPTION
 *      This function build a bccmd SETREQ message with the message structure
 *
 *         +---------+---------+---------+---------+---------+----------------+
 *         |  type   |  length |  seqNo  |  varId  |  status |    payload     |
 *         +---------+---------+---------+---------+---------+----------------+
 *
 * uint16s |    1    |    2    |    3    |    4    |    5    |                |
 *         +---------+---------+---------+---------+---------+----------------+
 *
 *  PARAMETERS
 *
 *      varId         : The identifier of the variable being manipulated by
 *                      the message.
 *
 *      payloadLength : The length of the message payload field measured in
 *                      16-bit integers.
 *
 *      *payload      : The payload field of the BCCMD message. The payload
 *                      must be const data, e.g. not an allocated pointer,
 *                      and it will be converte to XAP format by this function.
 *                      This means that 8-bit integers must travel as 16-bit
 *                      integers, and 32-bit integers must travel as two 16-bit
 *                      integers, as illustrated below
 *
 *                              +--------------+--------------+--------------+--------------+
 *                              |     Byte 4   |    Byte 3    |    Byte 2    |     Byte 1   |
 *                              |      MSB     |              |              |      LSB     |
 *                              +--------------+--------------+--------------+--------------+
 *
 *                      uint32s |                             1                             |
 *                              +-----------------------------------------------------------+
 *
 *                              +--------------+--------------+  +-------------+--------------+
 *                              |     Byte 4   |    Byte 3    |  |   Byte 2    |     Byte 1   |
 *                              |      MSB     |     LSB      |  |    MSB      |      LSB     |
 *                              +--------------+--------------+  +-------------+--------------+
 *
 *                      uint16s |              1              |  |             2              |
 *                              +-----------------------------+  +----------------------------+
 *
 *----------------------------------------------------------------------------*/
CsrUint8 *CsrTmBlueCoreBuildBccmdSetMsg(CsrUint16 varId,
                                        CsrUint16 payloadLength,
                                        const CsrUint16 *payload);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTmBlueCoreBuildBccmdPsSetMsg
 *
 *  DESCRIPTION
 *      This function build a SETREQ bccmd message with VarId 0x7003.
 *      E.g. a persistent store command with the structure
 *
 *         +---------+---------+---------+---------+---------+---------+---------+---------+----------------+
 *         |  type   |  length |  seqNo  |  varId  |  status |   key   | length  |  stores |     psValue    |
 *         +---------+---------+---------+---------+---------+---------+---------+---------+----------------+
 *
 * uint16s |    1    |    2    |    3    |    4    |    5    |    6    |    7    |    8    |        x       |
 *         +---------+---------+---------+---------+---------+---------+---------+---------+----------------+
 *
 *  PARAMETERS
 *      key           : The identifier of the PS Key of the database element
 *                      being accessed.
 *
 *      stores        : Controls the searching of the four component stores
 *                      that make up the fullPersistent Store. Stores field
 *                      values are defined in csr_bccmd_prim.h
 *
 *      psValuelength : The psValuelength of the PS Value field measured in
 *                      16-bit integers.
 *
 *      *psValue      : The psValue field of the BCCMD message. The psValue
 *                      must be const data, e.g. not an allocated pointer,
 *                      and it will be converte to XAP format by this function.
 *                      This means that 8-bit integers must travel as 16-bit
 *                      integers, and 32-bit integers must travel as two 16-bit
 *                      integers, as illustrated below
 *
 *                              +--------------+--------------+--------------+--------------+
 *                              |     Byte 4   |    Byte 3    |    Byte 2    |     Byte 1   |
 *                              |      MSB     |              |              |      LSB     |
 *                              +--------------+--------------+--------------+--------------+
 *
 *                      uint32s |                             1                             |
 *                              +-----------------------------------------------------------+
 *
 *                              +--------------+--------------+  +-------------+--------------+
 *                              |     Byte 4   |    Byte 3    |  |   Byte 2    |     Byte 1   |
 *                              |      MSB     |     LSB      |  |    MSB      |      LSB     |
 *                              +--------------+--------------+  +-------------+--------------+
 *
 *                      uint16s |              1              |  |             2              |
 *                              +-----------------------------+  +----------------------------+
 *
 *----------------------------------------------------------------------------*/
CsrUint8 *CsrTmBlueCoreBuildBccmdPsSetMsg(CsrUint16 key,
                                          CsrUint16 stores,
                                          CsrUint16 psValuelength,
                                          const CsrUint16 *psValue);
#endif /* !EXCLUDE_CSR_BCCMD_MODULE */

#ifdef CSR_USE_QCA_CHIP
#define BINARY_FILE_TYPE_TLV          0x01
#define BINARY_FILE_TYPE_NVM          0x02
#define BINARY_FILE_TYPE_AUP          0x03

#ifdef CSR_QCA_CHIP_32BIT_SOC_SUPPORT
CsrBool CsrHciGetPatchNvmFileNameEx(CsrUint32 socVer, CsrUint16 qcaBuildVer);
#else
/* DEPRECATED: This API is deprecated and shall be replaced by
 * CsrHciGetPatchNvmFileNameEx in next major release.
 */
CsrBool CsrHciGetPatchNvmFileName(CsrUint16 socVer, CsrUint16 qcaBuildVer);
#endif
CsrUint8 *CsrTmBluecoreReadBinFile(CsrUint8 fileType, CsrSize *length);
CsrBool CsrUpdateNvmTags(CsrUint8 *buffer);
CsrUint8 CsrQsocBaudrateMap(CsrUint32 baud);
CsrBool CsrTmQcGetFirmwareLogSetting(void);

extern void CsrBootstrapSetAupFileName(CsrCharString *value);
extern void CsrBootstrapSetM0FileName(CsrCharString *value);
extern void CsrBootstrapSetNVMFileName(CsrCharString *value);

#else /* CSR_USE_QCA_CHIP */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTmBlueCoreGetBootstrap
 *
 *  DESCRIPTION
 *      This function is called by the TM to get bootstrap array.
 *      *Must be implemented in app*
 *
 *    PARAMETERS
 *
 *      buildId[in]:        Build ID
 *
 *      entries[out]:       Number of entries in the bootstrap array.
 *
 *    RETURN:
 *                          A pointer array that contains all the BlueCore
 *                          Command (BCCMD) that must be send duing the Boot
 *                          strap procedure. The pointer array must be allocate
 *                          by the application and it will be own by TM.
 *                          Please note that the BCCMD's will
 *                          be sent in the order of placement in array.
 *----------------------------------------------------------------------------*/
CsrUint8 **CsrTmBlueCoreGetBootstrap(CsrUint16 buildId, CsrUint16 *entries);
#endif /* !CSR_USE_QCA_CHIP */


#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_BLUECORE_BOOTSTRAP_H__ */
