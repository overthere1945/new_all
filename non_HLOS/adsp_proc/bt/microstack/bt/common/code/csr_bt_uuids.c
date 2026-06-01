/******************************************************************************
 Copyright (c) 2016-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_types.h"
#include "csr_bt_profiles.h"
#include "csr_util.h"
#include "csr_pmem.h"
#include "csr_macro.h"
#include "csr_bt_uuids.h"


/****************************** UUID Conversion *******************************
 * Bluetooth UUIDs are 128-bit values. They can be represented as 16-bit or
 * 32-bit values as well.
 *
 * SIG assigned UUIDs start with a base UUID.
 *      Bluetooth base UUID - 00000000-0000-1000-8000-00805F9B34FB
 *
 * A 16-bit or 32-bit UUID can be converted into 128-bit UUID as follows -
 *      128-bit UUID = (16-bit UUID * (2^96)) + Bluetooth base UUID
 *      128-bit UUID = (32-bit UUID * (2^96)) + Bluetooth base UUID
 *
 * A 16-bit UUID can also be converted to 32-bit UUID format by zero-extending
 * the 16-bit value to 32-bits.
 *
 * For example a 16-bit UUID = 0x1234 can be written as
 *      32-bit UUID = 0x00001234
 *      128-bit UUID = 0x00001234-0000-1000-8000-00805F9B34FB
 *
 * Similarly a 32-bit UUID = 0x12345678 can be written as
 *      128-bit UUID = 0x12345678-0000-1000-8000-00805F9B34FB
 *
 * UUID values are transmitted in Little Endian format. Thus Synergy BT stores
 * UUIDs in Little Endian format.
 *****************************************************************************/

/* Position in 128-bit base UUID where 16-bit or 32-bit UUID to be added */
#define CSR_BT_UUID_BASE_POS (CSR_BT_UUID128_SIZE - CSR_BT_UUID32_SIZE)

/* Bluetooth base UUID */
static const CsrBtUuid128 baseUuid =
{
  0xFB, 0x34, 0x9B, 0x5F,
  0x80, 0x00, 0x00, 0x80,
  0x00, 0x10, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

CsrBool CsrBtUuidExpand(const CsrBtUuid *uuid, CsrBtUuid *expandedUuid)
{
    if (uuid->length == CSR_BT_UUID128_SIZE)
    { /* Already expanded. Just copy same UUID */
        SynMemCpyS(expandedUuid->uuid, sizeof(expandedUuid->uuid), uuid->uuid, sizeof(expandedUuid->uuid));
    }
    else
    {
        CsrBtUuid32 uuid32 = 0;

        if (uuid->length == CSR_BT_UUID16_SIZE)
        {
            /* Corresponding 32-bit UUID is numerically same as the 16-bit UUID */
            uuid32 = CSR_GET_UINT16_FROM_LITTLE_ENDIAN(uuid->uuid);
        }
        else if (uuid->length == CSR_BT_UUID32_SIZE)
        {
            uuid32 = CSR_GET_UINT32_FROM_LITTLE_ENDIAN(uuid->uuid);
        }
        else
        { /* Cannot be expanded */
            return (FALSE);
        }

        /* Copy base UUID */
        SynMemCpyS(expandedUuid->uuid, sizeof(expandedUuid->uuid), baseUuid, sizeof(expandedUuid->uuid));

        /* Put 32-bit UUID as MSB in Little Endian format */
        CSR_COPY_UINT32_TO_LITTLE_ENDIAN(uuid32,
                                         &(uuid->uuid[CSR_BT_UUID_BASE_POS]));
    }

    expandedUuid->length = CSR_BT_UUID128_SIZE;

    return (TRUE);
}

void CsrBtUuid16ToUuid(CsrBtUuid16 uuid16, CsrBtUuid *uuid)
{
    uuid->length = CSR_BT_UUID16_SIZE;
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(uuid16, &(uuid->uuid));
}

void CsrBtUuid32ToUuid(CsrBtUuid32 uuid32, CsrBtUuid *uuid)
{
    uuid->length = CSR_BT_UUID32_SIZE;
    CSR_COPY_UINT32_TO_LITTLE_ENDIAN(uuid32, &(uuid->uuid));
}

CsrBool CsrBtUuidCompare(const CsrBtUuid *uuid1, const CsrBtUuid *uuid2)
{
    const CsrBtUuid *tmpUuid1, *tmpUuid2;
    CsrBtUuid expandedUuid1, expandedUuid2;

    if (uuid1->length == uuid2->length)
    {
        tmpUuid1 = uuid1;
        tmpUuid2 = uuid2;
    }
    else
    {
        if (!CsrBtUuidExpand(uuid1, &expandedUuid1))
        { /* uuid1 cannot be expanded */
            return (FALSE);
        }

        if (!CsrBtUuidExpand(uuid2, &expandedUuid1))
        { /* uuid2 cannot be expanded */
            return (FALSE);
        }

        tmpUuid1 = &expandedUuid1;
        tmpUuid2 = &expandedUuid2;
    }

    return (!CsrMemCmp(tmpUuid1->uuid, tmpUuid2->uuid, tmpUuid1->length));
}

