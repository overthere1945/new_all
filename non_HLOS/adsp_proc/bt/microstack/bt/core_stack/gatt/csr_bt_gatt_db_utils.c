/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#include "csr_bt_gatt_private.h"

#ifndef CSR_BT_GATT_INSTALL_FLAT_DB
/* Helpers to Create a Database*/
CsrBtGattDb *CsrBtGattUtilCreateDbEntryFromUuid128(CsrBtGattDb        *head,
                                                   CsrBtGattHandle    *attrHandle,
                                                   CsrBtUuid128       uuid128,
                                                   CsrBtGattPermFlags attrPermission,
                                                   CsrBtGattAttrFlags attrFlags,
                                                   CsrUint16          attrValueLength,
                                                   const CsrUint8     *attrValue,
                                                   CsrBtGattDb        **tail)
{
    CsrBtGattDb *db = CsrPmemAlloc(sizeof(CsrBtGattDb));
    db->handle  = *attrHandle;
    *attrHandle = (CsrBtGattHandle)(db->handle + 1); 
    db->perm    = attrPermission;
    db->flags   = attrFlags;
    db->next    = NULL;
    db->uuid[0] = (CsrUint32)((uuid128[15] << 24) | (uuid128[14] << 16) | (uuid128[13] << 8) | uuid128[12]);
    db->uuid[1] = (CsrUint32)((uuid128[11] << 24) | (uuid128[10] << 16) | (uuid128[9] << 8) | uuid128[8]);
    db->uuid[2] = (CsrUint32)((uuid128[7] << 24) | (uuid128[6] << 16) | (uuid128[5] << 8) | uuid128[4]);
    db->uuid[3] = (CsrUint32)((uuid128[3] << 24) | (uuid128[2] << 16) | (uuid128[1] << 8) | uuid128[0]);

    if (attrValueLength > 0 && attrValue)
    {
        db->size_value   = attrValueLength;
        db->value        = (CsrUint8 *)CsrPmemAlloc(db->size_value);
        SynMemCpyS(db->value, db->size_value, attrValue, db->size_value);
    }
    else
    {
        db->value = NULL;
        db->size_value = 0;
    }

    if (head == NULL)
    {
        /* This it the first entry in the dataBase */
        head = db;
        *tail = head;
    }
    else
    {
        (*tail)->next = db;
        *tail = db;
    }
    return (head);
}

CsrBtGattDb *CsrBtGattUtilCreateDbEntryFromUuid16(CsrBtGattDb        *head,
                                                  CsrBtGattHandle    *attrHandle,
                                                  CsrBtUuid16        uuid16,
                                                  CsrBtGattPermFlags attrPermission,
                                                  CsrBtGattAttrFlags attrFlags,
                                                  CsrUint16          attrValueLength,
                                                  const CsrUint8     *attrValue,
                                                  CsrBtGattDb        **tail)
{
    CsrBtUuid128 uuid128 = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
                            0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  
    uuid128[12] = (CsrUint8)(uuid16 & 0x00ff); 
    uuid128[13] = (CsrUint8)((uuid16 & 0xff00) >> 8); 
    return (CsrBtGattUtilCreateDbEntryFromUuid128(head,
                                                  attrHandle,
                                                  uuid128,
                                                  attrPermission,
                                                  attrFlags,
                                                  attrValueLength,
                                                  attrValue,
                                                  tail));
}

CsrBtGattDb *CsrBtGattUtilCreatePrimaryServiceWith16BitUuid(CsrBtGattDb        *head,
                                                            CsrBtGattHandle    *attrHandle,
                                                            CsrBtUuid16        serviceUuid16,
                                                            CsrBool            leOnly,
                                                            CsrBtGattDb        **tail)
{
    CsrUint8 uuid[CSR_BT_UUID16_SIZE];
    CsrBtGattAttrFlags attrFlags;

    /* Note setting the attrFlags to CSR_BT_GATT_ATTR_FLAGS_DISABLE_BREDR
       is in this case only to indicate that GATT shall try to generate 
       a SDP record. Before the create local database it sent to ATT, GATT 
       makes sure that attrFlags always is set to CSR_BT_GATT_ATTR_FLAGS_NONE 
       in the function called csrBtGattBrEdrSupportHandler */
    if (leOnly)
    {
        attrFlags = CSR_BT_GATT_ATTR_FLAGS_DISABLE_BREDR;
    }
    else
    {
        attrFlags = CSR_BT_GATT_ATTR_FLAGS_NONE;
    }
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(serviceUuid16, uuid);
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_PRIMARY_SERVICE_DECL,
                                                 CSR_BT_GATT_PERM_FLAGS_READ,
                                                 attrFlags,
                                                 CSR_BT_UUID16_SIZE,
                                                 uuid,
                                                 tail));
}

CsrBtGattDb *CsrBtGattUtilCreatePrimaryServiceWith128BitUuid(CsrBtGattDb     *head,
                                                             CsrBtGattHandle *attrHandle,
                                                             CsrBtUuid128    uuid128,
                                                             CsrBool         leOnly,
                                                             CsrBtGattDb     **tail)
{
    CsrBtGattAttrFlags attrFlags;

    /* Note setting the attrFlags to CSR_BT_GATT_ATTR_FLAGS_DISABLE_BREDR
       is in this case only to indicate that GATT shall try to generate 
       a SDP record. Before the create local database it sent to ATT, GATT 
       makes sure that attrFlags always is set to CSR_BT_GATT_ATTR_FLAGS_NONE 
       in the function called csrBtGattBrEdrSupportHandler */
    if (leOnly)
    {
        attrFlags = CSR_BT_GATT_ATTR_FLAGS_DISABLE_BREDR;
    }
    else
    {
        attrFlags = CSR_BT_GATT_ATTR_FLAGS_NONE;
    }
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_PRIMARY_SERVICE_DECL,
                                                 CSR_BT_GATT_PERM_FLAGS_READ,
                                                 attrFlags,
                                                 CSR_BT_UUID128_SIZE,
                                                 (CsrUint8 *)uuid128,
                                                 tail));
}

CsrBtGattDb *CsrBtGattUtilCreateSecondaryServiceWith16BitUuid(CsrBtGattDb     *head,
                                                              CsrBtGattHandle *attrHandle,
                                                              CsrBtUuid16     serviceUuid16,
                                                              CsrBtGattDb     **tail)
{
    CsrUint8 uuid[CSR_BT_UUID16_SIZE];
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(serviceUuid16, uuid);
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_SECONDARY_SERVICE_DECL,
                                                 CSR_BT_GATT_PERM_FLAGS_READ,
                                                 CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                 CSR_BT_UUID16_SIZE,
                                                 uuid,
                                                 tail));
}

CsrBtGattDb *CsrBtGattUtilCreateSecondaryServiceWith128BitUuid(CsrBtGattDb     *head,
                                                               CsrBtGattHandle *attrHandle,
                                                               CsrBtUuid128    uuid128,
                                                               CsrBtGattDb     **tail)
{
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_SECONDARY_SERVICE_DECL,
                                                 CSR_BT_GATT_PERM_FLAGS_READ,
                                                 CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                 CSR_BT_UUID128_SIZE,
                                                 (CsrUint8 *)uuid128,
                                                 tail));
}

CsrBtGattDb *CsrBtGattUtilCreateIncludeDefinitionWithUuid(CsrBtGattDb     *head,
                                                          CsrBtGattHandle *attrHandle,
                                                          CsrBtGattHandle inclServiceAttrHandle,
                                                          CsrBtGattHandle endGroupHandle,  
                                                          CsrBtUuid16     serviceUuid,
                                                          CsrBtGattDb     **tail)
{
    CsrUint8 value[CSR_BT_GATT_INCLUDE_WITH_UUID_LENGTH];
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(inclServiceAttrHandle, value);
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(endGroupHandle, &(value[2]));
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(serviceUuid, &(value[4]));
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_INCLUDE_DECL,
                                                 CSR_BT_GATT_PERM_FLAGS_READ,
                                                 CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                 CSR_BT_GATT_INCLUDE_WITH_UUID_LENGTH,
                                                 value,
                                                 tail));
}

CsrBtGattDb *CsrBtGattUtilCreateIncludeDefinitionWithoutUuid(CsrBtGattDb     *head,
                                                             CsrBtGattHandle *attrHandle,
                                                             CsrBtGattHandle inclServiceAttrHandle,
                                                             CsrBtGattHandle endGroupHandle,  
                                                             CsrBtGattDb     **tail)
{
    CsrUint8 value[CSR_BT_GATT_INCLUDE_WITHOUT_UUID_LENGTH];
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(inclServiceAttrHandle, value);
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(endGroupHandle, &(value[2]));
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_INCLUDE_DECL,
                                                 CSR_BT_GATT_PERM_FLAGS_READ,
                                                 CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                 CSR_BT_GATT_INCLUDE_WITHOUT_UUID_LENGTH,
                                                 value,
                                                 tail));
}

static CsrBtGattPermFlags csrBtGattUtilCharacDefinitionGetPermission(CsrBtGattPropertiesBits properties)
{
    CsrBtGattPermFlags attrValuePermission = CSR_BT_GATT_PERM_FLAGS_NONE;
    
    if (CSR_MASK_IS_SET(properties, CSR_BT_GATT_CHARAC_PROPERTIES_READ))
    {
        attrValuePermission |= CSR_BT_GATT_PERM_FLAGS_READ;
    }

    if (CSR_MASK_IS_SET(properties, CSR_BT_GATT_CHARAC_PROPERTIES_WRITE_WITHOUT_RESPONSE))
    {
        attrValuePermission |= CSR_BT_GATT_PERM_FLAGS_WRITE_CMD;
    }

    if (CSR_MASK_IS_SET(properties, CSR_BT_GATT_CHARAC_PROPERTIES_WRITE))
    {
        attrValuePermission |= CSR_BT_GATT_PERM_FLAGS_WRITE_REQ;
    }

    if (CSR_MASK_IS_SET(properties, CSR_BT_GATT_CHARAC_PROPERTIES_AUTH_SIGNED_WRITES))
    {
        attrValuePermission |= CSR_BT_GATT_PERM_FLAGS_AUTH_SIGNED_WRITES;
    }
    return attrValuePermission;
}

CsrBtGattDb *CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(CsrBtGattDb             *head,
                                                              CsrBtGattHandle         *attrHandle,
                                                              CsrBtGattPropertiesBits properties,
                                                              CsrBtUuid16             uuid16,
                                                              CsrBtGattAttrFlags      attrValueFlags,
                                                              CsrUint16               attrValueLength,
                                                              const CsrUint8          *attrValue,
                                                              CsrBtGattDb             **tail)
{
    CsrUint8  value[CSR_BT_GATT_CHARAC_DECLARATION_MIN_LENGTH];
    CsrBtGattPermFlags attrValuePermission = csrBtGattUtilCharacDefinitionGetPermission(properties);
    value[0] = properties;
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN((*attrHandle + 1), &(value[1]));
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(uuid16, &(value[3]));
    head = CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                attrHandle,
                                                CSR_BT_GATT_UUID_CHARACTERISTIC_DECL,
                                                CSR_BT_GATT_PERM_FLAGS_READ,
                                                CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                CSR_BT_GATT_CHARAC_DECLARATION_MIN_LENGTH,
                                                (CsrUint8*)&value,
                                                tail);

    return (CsrBtGattUtilCreateDbEntryFromUuid16(head, 
                                                 attrHandle, 
                                                 uuid16, 
                                                 attrValuePermission, 
                                                 attrValueFlags, 
                                                 attrValueLength, 
                                                 attrValue, 
                                                 tail));
}



CsrBtGattDb *CsrBtGattUtilCreateCharacDefinitionWith128BitUuid(CsrBtGattDb            *head,
                                                               CsrBtGattHandle         *attrHandle,
                                                               CsrBtGattPropertiesBits properties,
                                                               CsrBtUuid128            uuid128,
                                                               CsrBtGattAttrFlags      attrValueFlags,
                                                               CsrUint16               attrValueLength,
                                                               const CsrUint8          *attrValue,
                                                               CsrBtGattDb             **tail)
{
    
    CsrUint8 value[CSR_BT_GATT_CHARAC_DECLARATION_MAX_LENGTH];
    CsrBtGattPermFlags attrValuePermission = csrBtGattUtilCharacDefinitionGetPermission(properties);

    value[0] = properties;
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN((*attrHandle + 1), &(value[1]));
    SynMemCpyS(&(value[3]), CSR_BT_UUID128_SIZE, uuid128, CSR_BT_UUID128_SIZE); 
    
    head = CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                attrHandle,
                                                CSR_BT_GATT_UUID_CHARACTERISTIC_DECL,
                                                CSR_BT_GATT_PERM_FLAGS_READ,
                                                CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                CSR_BT_GATT_CHARAC_DECLARATION_MAX_LENGTH,
                                                value,
                                                tail);

    return (CsrBtGattUtilCreateDbEntryFromUuid128(head, 
                                                  attrHandle, 
                                                  uuid128, 
                                                  attrValuePermission, 
                                                  attrValueFlags, 
                                                  attrValueLength, 
                                                  attrValue, 
                                                  tail));


}

CsrBtGattDb *CsrBtGattUtilCreateCharacExtProperties(CsrBtGattDb                 *head,
                                                    CsrBtGattHandle             *attrHandle,
                                                    CsrBtGattExtPropertiesBits  extProperties,
                                                    CsrBtGattDb                 **tail)
{
    CsrUint8 value[] = {0,0};
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(extProperties, value);
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_CHARACTERISTIC_EXTENDED_PROPERTIES_DESC,
                                                 CSR_BT_GATT_PERM_FLAGS_READ,
                                                 CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                 sizeof(value),
                                                 value,
                                                 tail));
}

CsrBtGattDb *CsrBtGattUtilCreateCharacUserDescription(CsrBtGattDb                 *head,
                                                      CsrBtGattHandle             *attrHandle,
                                                      const CsrUtf8String         *userDescription,
                                                      CsrBtGattPermFlags          attrPermission,
                                                      CsrBtGattAttrFlags          attrFlags,
                                                      CsrBtGattDb                 **tail)
{
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_CHARACTERISTIC_USER_DESCRIPTION_DESC,
                                                 attrPermission,
                                                 attrFlags,
                                                 (CsrUint16)(CsrUtf8StringLengthInBytes(userDescription) + 1),
                                                 userDescription,
                                                 tail));

}

CsrBtGattDb *CsrBtGattUtilCreateClientCharacConfiguration(CsrBtGattDb        *head,
                                                          CsrBtGattHandle    *attrHandle,
                                                          CsrBtGattAttrFlags attrFlags,
                                                          CsrBtGattDb        **tail)
{
    CsrUint8 value[] = {0,0};
    attrFlags |= (CSR_BT_GATT_ATTR_FLAGS_IRQ_READ | CSR_BT_GATT_ATTR_FLAGS_IRQ_WRITE);
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC,
                                                 (CSR_BT_GATT_PERM_FLAGS_READ | 
                                                  CSR_BT_GATT_PERM_FLAGS_WRITE),
                                                 attrFlags,
                                                 sizeof(value),
                                                 value,
                                                 tail));
}

CsrBtGattDb *CsrBtGattUtilCreateServerCharacConfiguration(CsrBtGattDb            *head,
                                                          CsrBtGattHandle        *attrHandle,
                                                          CsrBtGattAttrFlags     attrFlags,
                                                          CsrBtGattSrvConfigBits configurationBits,
                                                          CsrBtGattDb            **tail)
{
    CsrUint8 value[] = {0,0};
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(configurationBits, value);
    attrFlags |= CSR_BT_GATT_ATTR_FLAGS_IRQ_WRITE;
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_SERVER_CHARACTERISTIC_CONFIGURATION_DESC,
                                                 (CSR_BT_GATT_PERM_FLAGS_READ | 
                                                  CSR_BT_GATT_PERM_FLAGS_WRITE),
                                                 attrFlags,
                                                 sizeof(value),
                                                 value,
                                                 tail));
}

CsrBtGattDb *CsrBtGattUtilCreateCharacPresentationFormat(CsrBtGattDb      *head,
                                                         CsrBtGattHandle  *attrHandle,
                                                         CsrBtGattFormats format,
                                                         CsrInt8          exponent,
                                                         CsrUint16        unit, 
                                                         CsrUint8         nameSpace,
                                                         CsrUint16        description, 
                                                         CsrBtGattDb      **tail)
{
    CsrUint8 value[CSR_BT_GATT_CHARAC_PRESENTATION_FORMAT_LENGTH];
    value[0] = format;
    value[1] = exponent;
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(unit, &(value[2]));
    value[4] = nameSpace;
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(description, &(value[5]));
    return (CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                 attrHandle,
                                                 CSR_BT_GATT_UUID_CHARACTERISTIC_PRESENTATION_FORMAT_DESC,
                                                 CSR_BT_GATT_PERM_FLAGS_READ,
                                                 CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                 CSR_BT_GATT_CHARAC_PRESENTATION_FORMAT_LENGTH,
                                                 value,
                                                 tail));    
}

CsrBtGattDb *CsrBtGattUtilCreateCharacAggregateFormat(CsrBtGattDb            *head,
                                                      CsrBtGattHandle        *attrHandle,
                                                      CsrUint16              handlesCount,
                                                      const CsrBtGattHandle  *handles,
                                                      CsrBtGattDb            **tail)
{
    CsrUint16 i, t = 0;
    CsrUint16 length = (CsrUint16) (handlesCount * sizeof(CsrBtGattHandle));
    CsrUint8  *value = (CsrUint8 *) CsrPmemAlloc(handlesCount * sizeof(CsrBtGattHandle));

    for (i = 0; i < handlesCount; i++)
    {
        CSR_COPY_UINT16_TO_LITTLE_ENDIAN(handles[i], &(value[t]));
        t+=2;
    }

    head = CsrBtGattUtilCreateDbEntryFromUuid16(head,
                                                attrHandle,
                                                CSR_BT_GATT_UUID_CHARACTERISTIC_AGGREGATE_FORMAT_DESC,
                                                CSR_BT_GATT_PERM_FLAGS_READ,
                                                CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                length,
                                                value,
                                                tail);    
    CsrPmemFree(value);
    return (head);
}

#ifndef CSR_BT_GATT_EXCLUDE_MANDATORY_DB_REGISTRATION
CsrBtGattDb *CsrBtGattGetMandatoryDbEntry(GattMainInst *inst, CsrUint8* pLocalLeFeatures)
{
    CsrBtGattDb *head       = NULL;
    CsrBtGattDb *tail       = NULL;
    CsrUint16   attrHandle  = CSR_BT_GATT_ATTR_HANDLE_START;
    const CsrUint8 serviceChangeValue[] = {0,0,0,0};
#ifdef CSR_BT_GATT_INSTALL_EATT
    const CsrUint8 ssfValue = 0x1;
#endif

#ifdef CSR_BT_GATT_CACHING
    const CsrUint8 csfValue = 0;
    CsrUint8 dbHashValue[CSR_BT_DB_HASH_SIZE];

    /* As per core spec, Vol 3, Part G, Section 7.1 SERVICE CHANGED
     * The Service Changed characteristic Attribute Handle on the server shall not
     * change if the server has a trusted relationship with any client.
     * Hence always register GATT service first, so that the service changed handle
     * always remains same */

    /* Add the Primary Service for GATT */
    head = CsrBtGattUtilCreatePrimaryServiceWith16BitUuid(head, 
                                                          &attrHandle, 
                                                          CSR_BT_GATT_UUID_GENERIC_ATTRIBUTE_SERVICE,
                                                          FALSE,
                                                          &tail);

    /* Add A Characteristic Definition for the Service Change characteristic */
    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            CSR_BT_GATT_CHARAC_PROPERTIES_INDICATE,
                                                            CSR_BT_GATT_UUID_SERVICE_CHANGED_CHARAC,
                                                            CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                            sizeof(serviceChangeValue),
                                                            serviceChangeValue,
                                                            &tail);
    
    head = CsrBtGattUtilCreateClientCharacConfiguration(head,
                                                        &attrHandle,
                                                        CSR_BT_LE_PERMISSION_SERVICE_CHANGED,
                                                        &tail);

    /* Add a Characteristic Definition for the Client Supported Feature characteristic */
    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            (CSR_BT_GATT_CHARAC_PROPERTIES_READ | CSR_BT_GATT_CHARAC_PROPERTIES_WRITE),
                                                            CSR_BT_GATT_UUID_CLIENT_SUPPORTED_FEATURES_CHARAC,
                                                            (CSR_BT_GATT_ATTR_FLAGS_IRQ_READ | CSR_BT_GATT_ATTR_FLAGS_IRQ_WRITE),
                                                            sizeof(csfValue),
                                                            &csfValue,
                                                            &tail);

    /* Add a Characteristic Definition for the Database Hash characteristic */
    CsrMemSet(dbHashValue, 0, CSR_BT_DB_HASH_SIZE);
    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            CSR_BT_GATT_CHARAC_PROPERTIES_READ,
                                                            CSR_BT_GATT_UUID_DATABASE_HASH_CHARAC,
                                                            CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                            sizeof(dbHashValue),
                                                            dbHashValue,
                                                            &tail);
#endif
#ifdef CSR_BT_GATT_INSTALL_EATT
    /* Add a Characteristic Definition for the Server Supported Feature characteristic */
    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            CSR_BT_GATT_CHARAC_PROPERTIES_READ,
                                                            CSR_BT_GATT_UUID_SERVER_SUPPORTED_FEATURES_CHARAC,
                                                            CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                            sizeof(ssfValue),
                                                            &ssfValue,
                                                            &tail);
#endif

{
#ifdef CSR_BT_GATT_INSTALL_SERVER_GAP_SERVICE
    const CsrUint8 peripheralPreferredConnectionValue[] = {0,0,0,0,0,0,0,0};
    const CsrUint8 appearanceValue[] = {0,0};
#ifdef INSTALL_GATT_SECURITY_LEVELS
    const CsrUint8 slcValue[] = {LE_GATT_SECURITY_LEVELS_VALUE};
#endif
#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
    const CsrUint8 centralAddrResolution[] = {CSR_BT_GATT_CAR_SUPPORTED};
#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA
    const CsrUint8 rpaOnly[] = {CSR_BT_GATT_RPA_ONLY_VALUE_SET};
#endif /* CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA */
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */


    /* Add the Primary Service for GAP */
    head = CsrBtGattUtilCreatePrimaryServiceWith16BitUuid(head, 
                                                          &attrHandle, 
                                                          CSR_BT_GATT_UUID_GENERIC_ACCESS_SERVICE,
                                                          FALSE,
                                                          &tail);
    
    /* Add A Characteristic Definition for Device Name */
    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            CSR_BT_GATT_CHARAC_PROPERTIES_READ,
                                                            CSR_BT_GATT_UUID_DEVICE_NAME_CHARAC,
                                                            CSR_BT_GATT_ATTR_FLAGS_DYNLEN,
                                                            (CsrUint16)(CsrStrLen((CsrCharString*)inst->localName) + 1),
                                                            (const CsrUint8 *) inst->localName,
                                                            &tail);

    /* Add A Characteristic Definition for Appearance  */
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(CSR_BT_LE_APPEARANCE_VALUE, appearanceValue);
    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            CSR_BT_GATT_CHARAC_PROPERTIES_READ,
                                                            CSR_BT_GATT_UUID_APPEARANCE_CHARAC,
                                                            CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                            CSR_BT_EIR_DATA_TYPE_APPEARANCE_SIZE,
                                                            appearanceValue,
                                                            &tail);

    /* Add A Characteristic Definition for the Peripheral Preferred Connection Parameters  characteristic */
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(CSR_BT_LE_DEFAULT_CONN_INTERVAL_MIN, peripheralPreferredConnectionValue);
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(CSR_BT_LE_DEFAULT_CONN_INTERVAL_MAX, &(peripheralPreferredConnectionValue[2]));
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(CSR_BT_LE_DEFAULT_CONN_LATENCY, &(peripheralPreferredConnectionValue[4]));
    CSR_COPY_UINT16_TO_LITTLE_ENDIAN(CSR_BT_LE_DEFAULT_CONN_SUPERVISION_TIMEOUT, &(peripheralPreferredConnectionValue[6]));

    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            CSR_BT_GATT_CHARAC_PROPERTIES_READ,
                                                            CSR_BT_GATT_UUID_PERIPHERAL_PREFERRED_CONN_PARAM_CHARAC,
                                                            CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                            sizeof(peripheralPreferredConnectionValue),
                                                            peripheralPreferredConnectionValue,
                                                            &tail);

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
    /* Set CAR and RPA characteristics only if LL_PRIVACY is supported */
    if (pLocalLeFeatures && CSR_BT_LE_LOCAL_FEATURE_SUPPORTED(pLocalLeFeatures,
                                          CSR_BT_LE_FEATURE_LL_PRIVACY))
    {
        /* Add A Characteristic Definition for Central address resolution */
        head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                                &attrHandle,
                                                                CSR_BT_GATT_CHARAC_PROPERTIES_READ,
                                                                CSR_BT_GATT_UUID_CENTRAL_ADDRESS_RESOLUTION_CHARAC,
                                                                CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                                sizeof(centralAddrResolution),
                                                                centralAddrResolution,
                                                                &tail);

#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA
        /* Add A Characteristic Definition for Resolvable Private address only */
        head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                                &attrHandle,
                                                                CSR_BT_GATT_CHARAC_PROPERTIES_READ,
                                                                CSR_BT_GATT_UUID_RESOLVABLE_PRIVATE_ADDRESS_ONLY_CHARAC,
                                                                CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                                sizeof(rpaOnly),
                                                                rpaOnly,
                                                                &tail);
#endif /* CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA */
    }
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */

#ifndef EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL
    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            (CSR_BT_GATT_CHARAC_PROPERTIES_READ | CSR_BT_GATT_CHARAC_PROPERTIES_INDICATE),
                                                            GATT_UUID_ENCRYPTED_DATA_KEY_MATERIAL_CHARAC,
                                                            (CSR_BT_GATT_ATTR_FLAGS_READ_AUTHENTICATION |
                                                             CSR_BT_GATT_ATTR_FLAGS_AUTHORISATION |
                                                             CSR_BT_GATT_ATTR_FLAGS_IRQ_READ),
                                                            0,
                                                            NULL,
                                                            &tail);

    head = CsrBtGattUtilCreateClientCharacConfiguration(head,
                                                        &attrHandle,
                                                        (CSR_BT_GATT_ATTR_FLAGS_IRQ_READ |
                                                        CSR_BT_GATT_ATTR_FLAGS_IRQ_WRITE |
                                                        CSR_BT_GATT_ATTR_FLAGS_READ_AUTHENTICATION |
                                                        CSR_BT_GATT_ATTR_FLAGS_AUTHORISATION |
                                                        CSR_BT_GATT_ATTR_FLAGS_WRITE_AUTHENTICATION),
                                                        &tail);
#endif /* EXCLUDE_GATT_GAP_ENCRYPTED_DATA_KEY_MATERIAL */

#ifdef INSTALL_GATT_SECURITY_LEVELS
    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            CSR_BT_GATT_CHARAC_PROPERTIES_READ,
                                                            GATT_UUID_SECURITY_LEVEL_CHARAC,
                                                            CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                            sizeof(slcValue),
                                                            slcValue,
                                                            &tail);
#endif /* INSTALL_GATT_SECURITY_LEVELS */
#else
    CSR_UNUSED(pLocalLeFeatures);
#endif /* CSR_BT_GATT_INSTALL_SERVER_GAP_SERVICE */
}

#ifndef CSR_BT_GATT_CACHING
    /* Add the Primary Service for GATT */
    /* attrHandle = HANDLE_GATT_SERVICE; */
    head = CsrBtGattUtilCreatePrimaryServiceWith16BitUuid(head, 
                                                          &attrHandle, 
                                                          CSR_BT_GATT_UUID_GENERIC_ATTRIBUTE_SERVICE,
                                                          FALSE,
                                                          &tail);


    /* Add A Characteristic Definition for the Service Change characteristic */
    head = CsrBtGattUtilCreateCharacDefinitionWith16BitUuid(head,
                                                            &attrHandle,
                                                            CSR_BT_GATT_CHARAC_PROPERTIES_INDICATE,
                                                            CSR_BT_GATT_UUID_SERVICE_CHANGED_CHARAC,
                                                            CSR_BT_GATT_ATTR_FLAGS_NONE,
                                                            sizeof(serviceChangeValue),
                                                            serviceChangeValue,
                                                            &tail);
    
    head = CsrBtGattUtilCreateClientCharacConfiguration(head,
                                                        &attrHandle,
                                                        CSR_BT_LE_PERMISSION_SERVICE_CHANGED,
                                                        &tail);
#endif

    return head;
}
#endif /* ifndef CSR_BT_GATT_EXCLUDE_MANDATORY_DB_REGISTRATION */

#endif
